# Native build, step 1: our own 32-bit exe (P4, option A)

Goal: an exe of our own that runs the ported game code without the original's code, on 32-bit Windows,
as the first step of the platform flip. 64-bit cleanliness (Mac, Android, Linux x64) comes after, as a
separate step.

## Idea: recreate the original's address space, without its code

All ported code is compiled exactly as in the hook build (`BMX_HOOK`: `ORIG_VAR` and `ORIG_FN` are fixed
addresses). Our exe (`bmxnative.exe`, linked at a high base such as 0x20000000, no ASLR) then rebuilds
the original's memory layout at 0x400000..0x6b3000 before running any game code:

1. **Data from the user's own NgBMX.exe.** Read the install's `NgBMX.exe` (sha256 in
   `tools/known_exe.txt`; refuse others), `VirtualAlloc` the image range at its original base and copy
   every section except `.text` and `_rwcseg` (code). Uninitialised tails stay zero. Because the data
   sits at its original addresses, all 1232 `ORIG_VAR` globals and every pointer inside the data are
   valid without relocation. No game data goes into the repo.
2. **Code range = jumps and traps.** Fill `.text`/`_rwcseg` with `int3`, then write `jmp <our port>` at
   every original address in the generated hook table (`build/.../hooks_table.c`, the same table the
   hook DLL patches). Function pointers stored in the data (and `BMX_FNPTR` values) therefore land in
   our code. A vectored exception handler turns any `int3` hit into a report: address, the caller
   (return address on the stack), and the name from `data/functions.csv` or the labels when known,
   then exits cleanly. That report is the to-do list, in the order the game needs things.
3. **Imports.** Walk the exe's import directory and fill its `.idata` slots: with our own stand-in
   (`src/platform`, docs/PLATFORM.md) where there is one, with a trap stub when the slot is never reached natively
   (the MSVC CRT, WinMain, the D3D7 driver), and with the real Windows function (`LoadLibraryA` / `GetProcAddress`,
   DLLs loaded only when needed) for the rest. The real ones are replaced slice by slice by SDL3-backed stand-ins
   (window, input, audio, movies), which is what makes the exe portable.
4. **Entry.** `WinMain` (0x48a460) is not ported. `main` of bmxnative performs its job: command line
   (`-r`), the init sequence it calls, then the frame loop. Start from the original's control flow
   (work/bundles/0048a460.json) and call our ports; anything unported shows up as a trap report.

## Toolchain

- llvm-mingw (`~/tools/llvm-mingw-*/bin/i686-w64-mingw32-clang`): a 32-bit Windows target with a C
  library and Windows headers, unlike the freestanding hook toolchain. Same SSE2 flags as the hook build
  (`-msse2 -mfpmath=sse`), so the game code behaves exactly as tested.
- SDL3 (`~/tools/sdl3/SDL3-*/i686-w64-mingw32`). Step 1 only uses it for the window and timer.
- CMake preset `native32` with its own toolchain file; the game objects are built with `BMX_HOOK=1`
  (fixed addresses), the loader without it.

## What the original exe still provides at this step (and later replaces)

Inventory at 99.8% ported (2026-10-26): ~210 RenderWare functions (`Rw*`, `Rp*`, `Rt*`, `rw*`), ~10 MSVC
CRT functions (sprintf, sscanf, strtoul, malloc family...), 51 Win32 import slots, ~70 stale `ORIG_FN`
declarations of functions that are ported under another name, and WinMain. The CRT functions get jumps
to equivalents in our C library, checked against MSVC6 behaviour where it differs (strcmp returns
-1/0/1; printf formatting of floats).

## Later: drop the dependency on the user's NgBMX.exe

Step 1 copies the original's initialised data (515 globals: tables, strings, settings) from the user's
own NgBMX.exe at startup, so bmxnative.exe must sit next to the original and cannot replace it yet.
Before a release, every ORIG_VAR with initial data gets a native definition (generated from the
original's formula where possible, else read from the game's asset files) and the remaining ORIG_FN
targets are ours. Then the exe needs only the user's asset files and can be dropped into the game
folder as NgBMX.exe (save files stay compatible: same formats). Other platforms have no exe to
replace: they point at a copy of the game's assets folder.

## Done for step 1

`bmxnative.exe` starts from the game folder, loads the data, and runs the init sequence until the first
trap; the trap report names the function. A short list of the first traps goes into this file.

## Build and run

```
export PATH="$HOME/tools/llvm-mingw-20261020-ucrt-x86_64/bin:$PATH"   # i686-w64-mingw32-clang
cmake --preset native32 -DBMX_WORK_DIR=<repo>/work                    # work/ = labels.raw, for names in reports
cmake --build build/native32 -j 2
cd <game folder> && <repo>/build/native32/bmxnative.exe Scene.Txt -r640.480.16   # needs NgBMX.exe in the working dir
```

SDL3 (the mingw devel package's `i686-w64-mingw32` folder) is found by the `native32` configure under
`~/tools/sdl3/SDL3-*/i686-w64-mingw32` (`$HOME` or `%USERPROFILE%`), or set `-DBMX_SDL3_DIR=<that folder>`. The
build copies `SDL3.dll` next to `bmxnative.exe`; nothing has to be copied into the game folder.

Command line: like the original, the first word is the scene database (`Scene.Txt` by default) and `-r<w>.<h>.<depth>`
skips the device dialog by picking the full screen mode with that height and depth. `-r...` alone would become the
scene path (`zsceneLoadDatabase` fails with "Database not found:-r640"), so pass `Scene.Txt` first. Without `-r` the
original's device dialog comes up (D6).

Environment (dev aids):

| Variable | Effect |
|---|---|
| `BMX_TRAP_SKIP=N` | log the first N traps and fake a `return 1` from each instead of stopping (a survey; state is bogus after the first skip) |
| `BMX_NO_MOVIES=1` | sets the original's own movie skip switch (0x67288c, which the game never sets): no ACCLAIM/ZAXIS/INTRO.AVI |
| `BMX_MUTE=1` | mutes this process's audio session (WASAPI) before the game opens waveOut / DirectSound |
| `BMX_QUIT_FRAMES=N` | exit after N shown frames (`RwRasterShowRaster`) |
| `BMX_SHOT=<file.bmp>`, `BMX_SHOT_FRAME=N[,M...]` | save the shown camera raster of frame N (default 1; a list saves each, a `%d` in the file name becomes the frame number) as a BMP (GPU readback) |
| `BMX_KEYS=<script>` | scripted keyboard, see "Scripted input" below: e.g. `f300:enter,+150:enter,+100:up*400,+20:up+e*30` |
| `BMX_DRIVER_TRACE=1` | log every SDL3 driver entry with its first arguments |
| `BMX_PNG_CHECK=1` or `=<folder>` | compare our PNG reader with the original libpng on every PNG read (and on every `*.png` in the folder), see "PNG reader" |
| `BMX_TIME_BASE=<n>` | the value timeGetTime / GetTickCount show at process start (default 0x36ee80; `0xfffff000` runs through the 32-bit wrap) |
| `BMX_TIMER_LOG=1` | log the sound timer's statistics (calls, interval mean / min / max / sd, late calls, callback time) whenever the timer is stopped and at `BMX_QUIT_FRAMES` |
| `BMX_FILE_LOG=1` | log every sound file the game opens: reads, bytes, FNV-1a 64 hash of the bytes read |
| `BMX_FILE_CHECK=1` | also repeat every file call (CreateFileA / ReadFile / SetFilePointer) through the real kernel32 and compare; logs `filecheck ok` / `MISMATCH` per file (implies `BMX_FILE_LOG`) |
| `BMX_DEBUG_OUTPUT=1` | log the game's OutputDebugStringA text (dropped otherwise, as without a debugger) |
| `SDL_GPU_DRIVER=vulkan` | SDL's own: pick the GPU backend (default here: direct3d12) |

Output goes to stderr and to `bmxnative.log` next to the exe. Note: a desktop capture (BitBlt / CopyFromScreen) of
the window shows black while the D3D12 swapchain is presented as an independent flip; `PrintWindow` with
`PW_RENDERFULLCONTENT` sees the real content.

Sources: `src/native/loader.c` (hash check, image, imports, jumps), `trap.c` (exception handler, names),
`main.c` (WinMain's job), `keys.c` (BMX_KEYS), `range.S` (see below), the SDL3 render driver in `src/driver/` (docs/SDL3-DRIVER.md)
and the platform stand-ins in `src/platform/` (docs/PLATFORM.md, "Platform layer" below).
The game objects are compiled with `BMX_HOOK=1`, `-ffreestanding -fno-builtin` and the hook build's SSE2 flags; the
loader is hosted C. The driver (object library `bmx_driver`) is hosted C with `BMX_HOOK=1` (RW globals at their
original addresses) and is never part of the hook DLL. It is not in build/hook's compile database, so `tools/check.sh`
leaves it out of clang-tidy; tidy it with `clang-tidy -p build/native32 src/driver/*.c` (clang-tidy 21). The same holds for
the PNG reader in `src/png/` (object library `bmx_png`, freestanding, see "PNG reader instead of libpng") and
`src/native/pngcheck.c`: `clang-tidy -p build/native32 src/png/*.c src/native/pngcheck.c`. `src/platform/*.c` and `tests/platform/test_platform.c` are
left out of the hook tidy list too (`clang-tidy -p build/native32 src/platform/*.c tests/platform/test_platform.c`).

### Loader additions for the driver step (2026-10-29)

- **Driver jumps.** After the hook table, the loader writes `bmxDriverJumps` (src/driver/device.c): a jump from each
  original D3D7 driver entry we implement to the SDL3 driver. The device, standards and core extension tables in the
  copied data keep their original addresses; unimplemented driver functions stay int3 and are reported by name.
- **Kept parts of .text.** Two ranges of the original's code section are copied instead of filled with int3: the
  incremental linker's thunk table 0x401005..0x402617 (1130 `jmp <function>`; function pointers in the data point at
  these, e.g. 0x401e4c) and 0x4afc60..0x4b0f20, the object tables of dinput.lib's `c_dfDIJoystick` /
  `c_dfDIKeyboard` that the formats in .rdata point at (without them `SetDataFormat` fails).
- **Import shim `AVISaveOptionsFree`.** zmovieClose frees each stream's lpFormat with the CRT, then passes the options
  to `AVISaveOptionsFree`, which GlobalFrees it again. In the original that block is on MSVC6's own heap and the
  second free fails harmlessly; with our CRT on the process heap it was fatal (0xC0000374 heap corruption). The shim
  only clears lpFormat / lpParms (now in src/platform/avi.c, entered in the import table).
- **The game folder's dinput.dll** (the hook DLL, when installed there) is loaded by the import fill as `DINPUT.dll`
  (`bmxImportKeepsDll`, although the only slot is a stand-in) and patches the range too; bmxnative's own jumps are written after it, so they win. It still installs its crash
  handler and trace in the native process. Not a problem so far, but worth knowing.

### Design change found while building: the exe owns the range

`VirtualAlloc(0x400000, ...)` fails with `ERROR_INVALID_ADDRESS` (487) in a normal exe: before any of our
code runs, the kernel maps the code page tables (`C_437.NLS` and others) into the lowest free addresses,
which are 0x400000.. when the exe sits high. Unmapping them is not an option (ANSI conversions use them).
So `bmxnative.exe` is linked with image base 0x3f0000 and section alignment 0x10000, and `range.S` puts a
zero-filled 0x2b3000 block at the very start of `.text`, i.e. exactly 0x400000..0x6b3000. The PE loader
reserves the range for us, like it did for the original; the loader only `VirtualProtect`s it RWX and fills
it. Constraints: `range.S` must be the first object on the link line and the start files (`crt2.o`,
`crtbegin.o`) come after it (`-nostartfiles` plus explicit objects in CMakeLists.txt), because lld keeps
object order inside `.text`. The block is 2.7 MB of zeros in the exe file. Our own code lives above 0x6b3000.

Also: the exe's own `jmp [import slot]` thunks (`FF 25`, 141 of them, e.g. 0x489cb0 timeGetTime) are
recreated by scanning the original's code at load time, otherwise they would trap.

## First traps

Run from the game folder, 2026-10-26. 2117 hook jumps written, 137 import slots filled, 141 thunks.

```
TRAP: unported code reached at 004d01f0  RwEngineInit+0
  caller: 00730364 in port of AttachPlugins [0x48b1a0]
```

With `BMX_TRAP_SKIP=40` the survey continues (all in the init path, WinMain -> AttachPlugins/InitApp):

| trap | name | called from |
|---|---|---|
| 004d01f0 | RwEngineInit | AttachPlugins |
| 004bc1c0, 00500250, 00505870, 005062b0, 004ccac0 | RpWorld/Anim/Bone/LODAtomic/PVS PluginAttach | AttachPlugins |
| 004dffd0 | RwTextureRegisterPlugin | rwMatRegisterPlugin (0x498b10) |
| 004b31e0 | RpClumpRegisterPlugin | zrwMdlRegisterPlugin (0x488890) |
| 004d0020 | RwEngineOpen | AttachPlugins |
| 004cf870, 004cf8b0, 004cf940, 004cf980, 004cfb40, 004cfa60 | RwEngineGetNumSubSystems, GetSubSystemInfo, GetCurrentSubSystem, SetSubSystem, SetVideoMode, GetVideoModeInfo | psSelectDevice (0x489d70) |
| 004cfd50 | RwEngineStart | AttachPlugins |
| 004dba80 | RwOsGetMemoryInterface | InitApp (0x48b450) |

After that the fake return value 1 is used as a pointer by InitApp and it faults, which is expected: the
RenderWare engine core (device, memory interface, plugin registry) is the next thing to implement. The
CRT is not reached yet.

After the first RenderWare slice (rw/core.c, rw/engine.c, rw/plugins.c: engine init/open/start, memory interface,
plugin registry, attach routines) the first trap is the next RW group, `_rwFSOpen` (0x4e82e0, rw/stream.c), called from
RwEngineInit.

After the second slice (rw/stream.c, rw/im.c: file system table, streams, Im3D) `_rwFSOpen` runs and the first trap is
the CRT `malloc` (0x4a50d2, the engine's default memory interface), reached from `RwFreeListCreate` (plugin registry
open in RwEngineInit). That and the stdio entries `_rwFSOpen` installs (`rwfopen` 0x4a5c98 and friends) are CRT
functions that need equivalents in our C library, like the other `skip` rows of data/functions-rw.csv.

## C runtime layer (src/native/crt.c, crtasm.S)

The original links MSVC6's CRT statically, so its code is int3 here. After the hook jumps, `bmxCrtInstall` (called
at the end of `bmxLoadImage`) writes a `jmp` from each CRT entry the game or RenderWare can reach to an
implementation; the table in `crt.c` has the original address, replacement and name (54 entries). The set comes from
the CRT functions that ported code calls (callers in `work/bundles*`), the `ORIG_FN`/`RW_CB` addresses in `src`, and
the stdio/string tables `_rwFSOpen`/`_rwStringOpen` fill. `data/functions-rw.csv` is incomplete here (it lacks
sprintf 0x51dd9c, rwfread 0x51db3d, strtok...), `work/labels.raw` has them.

- Compiler helpers, in assembly with the original conventions (`crtasm.S`): `_ftol` (ST0 in, EDX:EAX out),
  `_CIsqrt`, `_chkstk` (EAX = bytes; `compat/crt.c`'s `__chkstk` jumps to it), `__allmul`/`__aulldiv`/`__aullrem`
  (stack args, `ret 16`), `__allshl` (EDX:EAX, CL), `_setjmp3`/`_longjmp` (MSVC `jmp_buf` layout, without the SEH
  chain). Checked against C arithmetic for edge values.
- Host: malloc/free/realloc (`malloc(0)` gives a block, `realloc(p, 0)` frees and returns NULL like MSVC6),
  stdio (`rwfopen`...`rwfseek`/`rwftell`, fflush; the game only passes the `FILE *` back, no FILE internals are read,
  `rwfputs` returns 0/-1), strtoul, strtok, sscanf (`vsscanf`), pow, ceil, `_finite`, strstr/strchr/strcpy...
- Wrapped for MSVC behaviour: strcmp/strncmp/`_strcmpi` return -1/0/1; `atoi` wraps instead of saturating; qsort is
  MSVC6's own algorithm (pivot to the front, cutoff 8, same stack discipline; not stable, so the order of equal
  elements matches the original); the printf family (vsprintf, sprintf, printf, fprintf) has its own formatter:
  floats use 17 significant digits then round half up (`%.0f` of 0.5 is `1`, `%.2f` of 0.125 is `0.13`), zero padding
  past the 17th digit, three-digit exponents (`e+010`), `1.#INF00`/`1.#QNAN0`/`-1.#IND00`, `%p` as 8 uppercase hex
  digits, `%g` stripping like MSVC. RenderWare's own helpers the engine tables point at (rwstricmp, rwstrupr, rwstrlwr,
  rwstrchr, rwstrrchr at 0x4ee1d0..0x4ee2d0, rwfexist 0x4e8290) are included since they are plain C.
- rand/srand: the CRT has none (no label, no caller); the game uses its own `zmathRand` (0x481f80).
- Not covered (not reachable yet): the CRT startup/exit path (0x4a51f9 WinMainCRTStartup, doexit, `_ioinit`, locale),
  the FILE-based internals (`_output`, `_flsbuf`, `_read`/`_write`), `time`/`ctime` (0x51df10, 0x51dfec), `raise`,
  the float-exception hooks `pcprofile.c` installs into the original's printf tables (`crtFpInit` calls
  0x4a6415/0x4a63c5; they are unneeded with our own formatter but will trap if `crtFpInit` runs).
  `pow` is the host's, so results can differ from the x87 original in the last ulp (texture gamma tables).

New first trap after the CRT layer (2026-10-28): `_rwFreeListAllocReal` (0x4dbc30, RenderWare free lists), called
from `_rwPluginRegistryAddPlugin` (0x4e88d0). `malloc` and the stdio table are no longer traps. A
`BMX_TRAP_SKIP` survey shows no further CRT trap before the fake return value faults.

Second pointer-only batch (2026-10-28: world plugin callbacks, mesh stream, LOD atomic plugin, clump/light module
callbacks, default texture read callback; 2664 hook jumps): the first trap is unchanged, `0x4eac90`, because the
driver's attach routine runs before any of these callbacks.

First trap after the pointer-only RenderWare functions (2026-10-28): `0x4eac90`, entry 0 of the core extension table at
0x69bee0 (`rwCoreExtGetFuncs()[0]`, called by `CorePluginAttach` through `rwCoreExtPluginAttach`). It belongs to the
Direct3D driver (classified `lib/d3d7driver` in data/functions-rw.csv), so this is where the SDL3 driver has to plug
in: it provides the attach routine and the other table entries (render state hook, Im3D entries) plus the device
table (0x566fe4, `D3D3System` 0x4ee010, `rwD3D3Standards` 0x56701c). With `BMX_TRAP_SKIP` the next traps are
`D3D3System` (three calls from `RwEngineOpen`), then a null device function. Everything before that now runs in
ported code: free lists, the memory wrappers, every engine module open routine (error, vector, colour, matrix, frame,
stream, camera, image, raster, texture, resources, Im3D, render pipeline), the world/mesh/geometry/light/sector
modules and the animation engine module.

### First traps with the SDL3 driver skeleton (2026-10-29)

With the driver's step 1 (docs/SDL3-DRIVER.md) `bmxnative.exe Scene.Txt -r640.480.16` (with `BMX_NO_MOVIES=1
BMX_MUTE=1`) gets through RwEngineInit/Open/Start, InitApp and into the frame loop: the main menu runs, each frame does
camera begin update, clear (colour 0x00000000 in the menu, so the window shows black), end update and
`RwRasterShowRaster`, presented through SDL3 GPU (direct3d12) into the game's own window. Nothing else is drawn yet:
the menu's geometry goes through Im3D / the instancing pipelines, which are later steps.

On the way, three non-driver traps were fixed: `_rpClumpOpen` (0x4b0f20, clump module open, missing from
functions-rw.csv; ported in rw/clump.c with its ext caller 0x4c46b0 and oracle tests), the incremental-link thunks and
dinput tables in .text, and the double free in the movie close (see "Loader additions" above).

After ~1860 menu frames (about 30 s) the first trap is libpng: `png_create_read_struct`-area 0x5070d0 (labelled
png_error+0x80), called from `RtPNGImageRead` (0x5068a0). `BMX_TRAP_SKIP` gets only one step further (0x508e40,
png_read_info+0x1bb0) before the faked return value faults in RtPNGImageRead: the PNG reader (lib/libpng, `skip`) needs
a replacement decoder before a survey can go on. No unported driver entry was reached in those menu frames: no Im3D
or pipeline call, no raster render; the Im2D entries are reached but are no-ops in the original too. RenderStateSet/Get
are step 1 stubs that keep the values (153k sets in 1860 frames).

### PNG reader instead of libpng (src/png, 2026-10-29)

`RtPNGImageRead` (rw/texture.c) calls libpng 1.0.5, which the original links statically (lib/libpng and lib/zlib in
functions-rw.csv: `skip`, third-party code we do not port). In the native exe our own reader takes its place:
`src/png/png.c` implements the 16 libpng entry points RtPNGImageRead calls plus `png_error` (0x507050, which the read
callback `rwPngRead` calls on a short read), and the loader writes `bmxPngJumps` after the driver jumps, the same
mechanism as the SDL3 driver (no `BMX_ADDR`: the hook DLL keeps the original libpng). Own directory because it is
neither ported game/RW code nor native-exe glue: freestanding C (compiled like the game objects, object library
`bmx_png`) that only talks to RenderWare through the callbacks libpng gets, so it can later move into the test DLL or
another platform build unchanged.

- Same contract as libpng: the png struct starts with the MSVC `jmp_buf` RtPNGImageRead fills with `_setjmp3`;
  read function / io pointer sit at 0x50 / 0x54 (rwPngRead reads its RwStream there); every error (bad signature,
  CRC of a critical chunk, corrupt or truncated deflate data, Adler-32, short read) longjmps there through the CRT's
  `_longjmp` 0x51e084 (in the native exe our `bmxCrtLongjmp`, crtasm.S, which restores EBP/EBX/EDI/ESI/ESP/EIP;
  no SEH unwinding is needed because only plain C frames lie between). Data only through the read callback, memory
  only through the malloc / free callbacks of `png_create_read_struct_2` (the engine's memory interface).
- Transforms as libpng 1.0.5 applies them for what RtPNGImageRead requests: strip_16 (high byte), packing (palette
  1/2/4 bit: one index per byte), gray_1_2_4_to_8 (`PNG_EXPAND`: scaled to 0..255, i.e. 0x11/0x55/0xff steps, one
  byte per pixel; RtPNGImageRead still makes a depth-4 image with a grey identity palette and those 0..255 values in
  it, like the original), gray_to_rgb (GA -> RGBA), filler 0xff after (RGB -> RGBX), RGBA as is,
  PLTE / tRNS getters (tRNS pointer NULL for non-palette images, like libpng). Adam7: pixels are written to their
  final positions pass by pass, row by row, which leaves the same bytes as libpng's `png_combine_row` masks.
- Inflate: our own, in the structure of Mark Adler's puff.c (zlib licence; canonical Huffman decoded bit by bit),
  about 300 lines. All IDAT chunks are read and inflated in `png_read_image`, `png_read_end` reads the rest up to
  IEND. Data past the image in the zlib stream is ignored (libpng: a warning).
- Differences on corrupt input only: unknown filter type is an error (libpng 1.0.5 warns and zeroes the row);
  ancillary chunks with a bad CRC are dropped silently (libpng warns, then drops them). Packed or 16-bit output and
  palette expansion are not implemented (RtPNGImageRead never asks for them): `png_error`.

**The game's PNGs** are not loose files: 111 PNG images are embedded in the `.ZMD`/`.zmd` model files under
`assets/` (bike and rider textures), 22 distinct ones, all 8-bit RGBA (colour type 6), not interlaced, no tRNS.

**Verification** (`BMX_PNG_CHECK`, src/native/pngcheck.c): the loader then also keeps the original's libpng/zlib code
(0x506e40..0x51ce10, up to the next ported function; zlib code past the last labelled function is reached) instead of int3 and remembers the five entry bytes of each replaced function; RtPNGImageRead's
entry goes to a checker that runs the ported RtPNGImageRead once with the original libpng and once with ours and
compares width, height, depth, stride, every pixel byte the reader writes (`width` bytes per row for depths 4/8,
`4 * width` for 32, capped at the stride; the rest of a row is uninitialised heap in both) and the palette. The game
itself reads its PNGs from memory (the model files' embedded copies) through file functions it swaps into the engine
while a texture loads, and such a "file" cannot be opened twice; so its own reads (`PNGImageRead` 0x497a30, which
calls the ported RtPNGImageRead directly) only run through our reader and are logged, and the comparison runs on
`BMX_PNG_CHECK=<folder>`: at the first PNG read every `*.png` in that folder is checked, with `_rwFSOpen`'s stdio
functions put back for the duration. `tools/png_cases.py <folder>`
writes the synthetic set: every colour type / bit depth (gray 1/2/4/8/16, RGB 8/16, palette 1/2/4/8, GA 8/16, RGBA
8/16), plain and Adam7, at 1x1, 13x7 and 33x17, random pixels and filter types, palettes with and without tRNS
(and RGB tRNS), stored / fixed / dynamic deflate blocks, IDAT split over several chunks, ancillary chunks before
and after the data. No tRNS on grey images: RtPNGImageRead then dereferences libpng's NULL tRNS pointer (crashes
the original too). Files named `bad_*` (truncated, bad IHDR CRC) run only through our reader, which must longjmp
and make RtPNGImageRead return NULL: libpng's default error handler `fprintf`s to MSVC's `stderr`, which our CRT
layer cannot take. The oracle harness was not used: RtPNGImageRead's port calls libpng through fixed addresses, so a
port-side run with our reader would need code patching in the write-protected Unicorn code range; the in-exe check
runs the very same ported RtPNGImageRead against the very same original libpng code instead.

Result (2026-10-29): folder with the 90 synthetic files, the 22 PNGs extracted from the game's model files and the 2
corrupt ones: **112 identical to libpng 1.0.5, 0 mismatches**, both corrupt files rejected (RtPNGImageRead returns
NULL after the longjmp). In the same run the game read 20 PNGs with our reader (DUST01..16, TIRE, PEDAL..., 32x32 and
64x64 RGBA) before the next trap. Reproduce:

```
py -I tools/png_cases.py <folder>          # synthetic set; add the PNGs extracted from assets/*.zmd if wanted
cd <game folder> && BMX_PNG_CHECK='<folder, Windows path>' BMX_NO_MOVIES=1 BMX_MUTE=1 <repo>/build/native32/bmxnative.exe Scene.Txt -r640.480.16
grep pngcheck <repo>/build/native32/bmxnative.log
```

### First trap after the PNG reader (2026-10-29)

Two more `.text` pieces are kept as original code (kTextKeep in loader.c), both found on the way:

- `RpWorldDestroy` 0x4ba9c0, the 4-byte stub (`mov eax, [esp+4]`) that falls into the ported body at 0x4ba9c4: it
  is int3 otherwise and trapped from `ThisFoundChunk` (0x4966e0) when a level's world is replaced.
- **Stopgap, to review:** 0x54b038..0x54b114, the core's x87 matrix multiply in `_rwcseg` that the driver's camera
  begin update installs as RwMatrixMultiply's routine (SDL3-DRIVER.md open points). It is self-contained (no calls).
  Without it the first trap is 0x54b038 itself, called from RwMatrixMultiply (0x4d04f0). Porting it (or modelling
  the precision switch) replaces this.

`BMX_NO_MOVIES=1 BMX_MUTE=1 BMX_QUIT_FRAMES=4000 bmxnative.exe Scene.Txt -r640.480.16` then loads the bike and rider
textures with our PNG reader and stops at the D3D driver's Im3D code:

```
TRAP: unported code reached at 004ea960  RwTextureRegisterPluginStream+0xc80   (lib/d3d7driver, FUN_004ea960)
  caller (return address at esp): 0075d7af  in port of _rpWorldPipelineOpen+0xb0 [0x4e85b0]   (rwIm3DPipelinePush, rw/im.c)
```

That is the SDL3 driver's step 2/3 territory (Im3D / pipelines).

### Step 2: the menu draws (2026-10-30)

With the SDL3 driver's step 2 (docs/SDL3-DRIVER.md: the D3D7 driver's Im3D, clipper, render states and Im2D ported
into src/rw with `BMX_ADDR`, the Direct3D calls on SDL3 GPU in src/driver/draw.c), `BMX_NO_MOVIES=1 BMX_MUTE=1
bmxnative.exe Scene.Txt -r640.480.16` draws the main menu as the original does: background, logo, the rider and
bike artwork, the menu wheel (PROQUEST highlighted, the other items translucent). `BMX_SHOT` of frames 300 and 1500
(640x480) checked by eye; no pixel diff against the original yet (D7, step 4).

The menu needs no instancing pipeline. Running on (`BMX_QUIT_FRAMES=4000`), the next trap is the first one of
step 3:

```
TRAP: unported code reached at 004c92d0  _rpMaterialListFindMaterialIndex+0x4230   (lib/d3d7driver)
  caller (return address at esp): 007343a7  in port of _rpWorldPipelineOpen+0x100 [0x4e8600]   (rwIm3DPipelineRun)
```

i.e. an atomic or world sector is rendered through its instancing pipeline (0x4c8ba0..0x4cb000).

New in the loader's view: 0x4d7320 (8-byte entry falling into 0x4d7328) and the other driver entries ported in
src/rw now get their jumps from the hook table; `bmxDriverJumps` lost the step 1 stand-ins (render state set/get,
the core extension ctor/dtor/entries 1-3).

### Step 3: the instancing pipelines (2026-10-30/31)

The world sector and atomic pipelines (0x4c8ba0..0x4cb000 and 0x4eafa0) are ported into src/rw/d3dpipe.c with
`BMX_ADDR` (docs/SDL3-DRIVER.md step 3), so the hook table gives 0x4c92d0 and the other stages their jumps, as well
as the five light routines rpWorldExtLightSelectCallback stores. They draw through Im3D's index lists and Im2D's
indexed primitive, which bmxnative already implements; no loader or driver change was needed.

First native run (2026-10-31): trap at 0x4b1860, the atomics' light stage that _rpClumpOpen installs (reached only
through that pointer, so tools/rwmap.py never saw it, nor the stack argument entries 0x4b9ac0 / 0x4b9b20 of the two
light walks it calls). Ported into rw/light.c (rpAtomicLightStage) and rw/world.c with oracle tests. After that,
`BMX_NO_MOVIES=1 BMX_MUTE=1 BMX_QUIT_FRAMES=4000 bmxnative.exe Scene.Txt -r640.480.16` runs 4000 frames without a
trap: the menu times out into the attract mode and the demo level renders (rider, bike, ramps, textures, "DEMO
PLAY"; screenshot at frame 3000). Into a level by scripted keys (2026-10-31, "Scripted input" below): ProQuest and
Freeride in all unlocked levels play without a trap.

### Scripted input (BMX_KEYS, 2026-10-31)

The game reads the keyboard only through DirectInput (pcinput.c: `GetDeviceState` on a `c_dfDIKeyboard` device in
foreground, non-exclusive mode, once per poll; the window procedure only watches WM_KEYDOWN for the quit key). So the
script replaces DirectInput itself, at the import: the loader's import shim for `DINPUT.dll!DirectInputCreateEx`
(src/native/keys.c) hands out a stand-in IDirectInput7 when `BMX_KEYS` is set. It enumerates no joystick (so the pad
falls back to the keyboard, as on a PC without a pad) and creates only the keyboard, whose state is the script's keys
held at the current shown frame (`drvGpuShownFrames`, the counter of `BMX_SHOT_FRAME` / `BMX_QUIT_FRAMES`). Nothing
real is read: a scripted run is the same with or without focus, and keys or pad input the user gives another program
never reach it. Without `BMX_KEYS` the shim forwards to the real DirectInputCreateEx.

A second shim, for `USER32.dll!MessageBoxA` (the slot 0x6997bc), answers the game's modal boxes in a scripted run:
without it the main menu's QUIT hung the run in "Do you want to quit the game?" (and its `MB_SETFOREGROUND` would take
the focus). It logs the text and answers Yes for Yes/No (the quit prompt quits: exit code -1 like the original's
`_exit(-1)`), Cancel for Yes/No/Cancel (an assert terminates) and OK otherwise. Unscripted runs get the real box.

A scripted run also does not take the focus: the window is created with `WS_EX_NOACTIVATE`, shown with
`SW_SHOWNOACTIVATE` and put at the bottom of the z order, and SDL gets `SDL_WINDOW_ACTIVATE_WHEN_SHOWN=0` /
`..._RAISED=0`. Checked by polling `GetForegroundWindow` during a run: unchanged. The game does not pause when it is
not active (WM_ACTIVATE only acquires / unacquires the devices, which the stand-in ignores).

Script: comma separated events `<when>:<keys>[*<hold>]`. `<when>` is `f<N>` (shown frame N) or `+<N>` (N frames after
the previous event's start). `<keys>` are names joined by `+`: `enter`, `esc`, `space`, `up`, `down`, `left`,
`right`, `a`..`z`, `0`..`9`, or a DIK code `0x..`. `<hold>` is the number of frames the keys stay down (default 2).
Every event is logged when it fires. The game's keyboard map for player 1 (0x561e68): arrows = stick, Enter = start,
Esc = select/back, E S F D = the four face buttons, W R 3 4 = shoulder buttons, Space = input 20.

Into a level (2026-10-31, `-r640.480.16`, `BMX_NO_MOVIES=1`): Enter at frame 300 and then every 150 frames, ten
times, goes main menu (ProQuest) -> rider (Dave Mirra) -> bike -> style -> level (Greenville / Vail Trails) ->
loading -> the challenges screen -> continue: the rider stands in Vail Trails at frame ~1400 and the timer runs.

```
BMX_KEYS="f300:enter,+150:enter,+150:enter,+150:enter,+150:enter,+150:enter,+150:enter,+150:enter,+150:enter,+150:enter"
```

Other paths (menu positions, all from a fresh start with the user's save restored before each run; the game writes
`save/SAVE.DAT` as it goes, so scripted runs change it):

- Main menu wheel: PROQUEST, SESSION, FREERIDE, COOL STUFF, OPTIONS, QUIT (`down` steps). Freeride:
  `f300:down,f360:down,f450:enter,f600:enter,f750:enter,f900:enter`, then `right` n times on the level screen and
  Enter (Vail Trails, Greenville Park, Backyard Pools, Woodward Trails, Lot 8, Titanic, Pro Tour Sleepy's Ditch are
  unlocked in that save; the rest are locked), then about 6 Enters through loading / challenges.
- In a level: `up` pedals, arrows steer, E/S/F/D tricks. Esc opens PAUSE (continue, view challenges, options,
  trick list, view records, end run); "end run" asks END RUN? (no, yes); then RUN FINISHED (another run, level
  select, view replay, trick list, view records, exit to main menu; the cursor starts at "another run") and EXIT?
  (no, yes) back to the main menu.

**Result (2026-10-31): no trap.** Scripted runs covered: ProQuest into Vail Trails, riding, tricks and bails, a full
2-minute run to RUN FINISHED, the pause menu and its sub screens, end run, the replay, back to the main menu;
Freeride in all seven unlocked levels (about 3000 frames of riding each); Options (controller, cheats, audio with the
volume sliders); Cool Stuff (challenges, high scores); QUIT (with the message box shim). Every run ended at
`BMX_QUIT_FRAMES` or by the quit, without a trap, so the expected candidates (raster render fast / scaled, raster
clear rect, image from raster, native texture read / write / size, palette lock, fog density 0x4d6590) are not
reached in single-player play: no ported code calls a raster render standard; image from raster only comes from the
mip generator (this driver sets no mip format bits); native textures and palettes are not in the game's files (the
models' textures are PNG); the game never sets fog density. Session, the movies and two-player play are covered
in the next section.

### Session, movies and two-player (2026-10-31, second pass)

Runs from a copy of the game folder (`C:\Users\Dennis\Games\dmfbmx\native-run`: the install without `dinput.dll`,
the `*.bin` traces and the save, with the user's `SAVE.DAT` copied in), so the hooked game in the real folder and
its save stay untouched while the user plays. Same switches as above; `save/SAVE.DAT` restored after each run.

- **Session** (main menu `down` once): `f300:down,f450:enter,+150:enter` x6 gets to Vail Trails with the 2:00 timer
  at frame ~1250. Covered: a full 2-minute run to RUN FINISHED (the session menu: another run, level select,
  gear up, view replay, trick list, view records, exit to main menu); another run; the session PAUSE menu
  (continue, options, trick list, view records, end run; `up` from continue wraps to end run) and END RUN?;
  gear up and level select (both ask EXIT? (no, yes), then the frontend's bike / style / level screens; level
  select into Greenville Park, its intro page, continue, riding); view replay (replay cameras); exit to main menu.
  Recipe for the run-finished menu: `f1500:esc,f1560:up,f1620:enter,f1700:down,f1760:enter` (end run, yes), then
  `down` steps from "another run" about 100 frames apart (keys pressed while the menu is still sliding in are
  lost).
- **Cool Stuff movies and credits with movies on** (no `BMX_NO_MOVIES`, `BMX_MUTE=1`, silent): main menu `down` x3,
  Enter; hub CHALLENGES, HIGH SCORES, MOVIES, CREDITS; MOVIES lists only INTRO MOVIE in the user's save (amish /
  mirra / nyquist.avi not unlocked). Playing INTRO (105 s) and CREDITS (221 s) goes through moviePlay's
  ngifShutdown / guiModeShutdown, the AVI player and ngifInit / guiModeInit and back to the list / hub without a
  trap. The movie is drawn by the original's AVI path into the window (seen with `PrintWindow` during the startup
  intro); `BMX_SHOT_FRAME` does not count movie frames, so script frames resume where they stopped. A scripted run
  with movies on spends ~155 s on ACCLAIM / ZAXIS / INTRO before frame 1 (keys are not pressed then: the frame
  counter does not move).
- **Two-player modes do not exist in the PC version.** The player count 0x69621c is only ever written with 0 or 1
  (the five `mov byte [0x69621c], imm` in the exe: comdata init, the main menu's ProQuest / Session / Freeride
  cases, the attract exit; no register stores), and the main menu and the select screen answer player 2's start
  (messages 0x2f / 0x31) with the "refused" sound 0x7d4. The split-screen camera / overlymp / two-player select
  code is dead code inherited from the console version; no menu reaches it.

**Result: no trap** in any of these runs (11 runs, the log only has the usual start lines), so nothing new to port.
By eye: the session HUD (timer bottom left, score bottom right, the height meter of the run's start), the PAUSE /
END RUN? / RUN FINISHED / EXIT? overlays, the frontend and the replay look right. No split screen to check.

**State 2026-10-31:** BMX_KEYS, the MessageBoxA shim, the multi-frame
`BMX_SHOT_FRAME` and the tidy fixes in loader.c / main.c build and were used for all runs above. No trap is open.
The four audits pass (0; 5/2/8/0/11; 0; 0/0/0) and the full gate passed after the merge. A native and a hooked frame
at the Vail Trails start look identical apart from the timer (no rider shadow in either). Every mode the PC menus
offer has now run natively without a trap. Next steps: compare more native and hooked frames (docs/SDL3-DRIVER.md
step 3), then the D7 screenshot diff (step 4).

### Platform layer: import table, time, sound timer, sound files (S1-S4, 2026-10-31)

docs/PLATFORM.md has the design and the status (4a). In the native exe: the loader fills each import slot from the
table in `src/platform/imports.c` (stand-in, else trap stub for the 64 dead slots, else the real function); the log
shows per DLL `N stand-ins, N dead (trap stubs), N real` and the DLLs still used (KERNEL32, USER32, WINMM, AVIFIL32,
MSVFW32 today; ADVAPI32, DDRAW and, unscripted, DINPUT only when asked). A call of a dead slot prints
`DEAD IMPORT called: <dll>!<name>, caller <addr>` and exits with code 3. The stand-ins: timeGetTime / GetTickCount /
Sleep / QPC / QPF / OutputDebugStringA / DebugBreak / critical sections (kernel.c), the winmm timer functions on an
SDL thread (winmm.c), the sound loader's CreateFileA / ReadFile / SetFilePointer / CloseHandle (fileio.c).

Checks (all 2026-10-31, `-r640.480.16`, `BMX_MUTE=1`, from the copy `C:\Users\Dennis\Games\dmfbmx\native-run`):

- `build/native32/bmxplatformtest.exe` (about 20 s; hosted, no game): time monotonic / 1 ms / tick steps 15-16 ms / same
  base / wrap through zero / elapsed and QPC equal to the real functions; critical sections under 3 threads
  (recursive, 600000 increments); the timer (62 calls per second, arguments, other thread, nothing after
  timeKillEvent returns, one-shot, kill from the callback, slot reuse; a side-by-side of the real winmm timer and
  ours: mean 16.003 vs 16.000 ms, sd 0.50 vs 0.11 ms, i.e. ours is at least as steady); the file shims against the real kernel32. 61152 checks, 0 failures.
- ProQuest into Vail Trails and Freeride (Greenville) by `BMX_KEYS`, with `BMX_TIMER_LOG=1`, `BMX_FILE_LOG=1`: no
  trap, no dead import, frames as before (frames 1000 / 1500 / 1990 of the ProQuest run are byte-identical (PNG
  md5) to a build of the previous commit run the same way; a transition frame can differ between two runs of the same
  build). The pump ran at mean 15.999 ms (sd 0.03-0.09 ms, 0 calls more than 2 ms late) over 3450-4784 calls.
- `BMX_FILE_CHECK=1` in the same ProQuest run: SFXCOM / MUSIC / SPEECH / SFXMEN00 / SFXGAME / SFXLEV00 / SFXCHR00 and
  the MUSIC stream (read on the timer thread): `filecheck ok` for all.
- Cool Stuff intro movie with movies on (`MOVIES` run: ACCLAIM, ZAXIS and INTRO at startup, then the INTRO MOVIE from
  the Cool Stuff list): all play for their real length (the log's `End Of Movie` comes 20 s, 19 s, 105 s and 105 s
  after the start: the GetTickCount / timeGetTime clocks are right); the sound timer is stopped and restarted around
  each movie (timer 1: 1646 calls, mean 15.999 ms, sd 0.055; timer 2 after the movie: 505 calls, mean 15.999 ms);
  `PrintWindow` frames of the startup movies (ACCLAIM, ZAXIS) look right (a `PrintWindow` of the Cool Stuff movie
  only shows the last swap chain frame: the GDI-drawn movie is not in it once the SDL window has presented).

Comparisons with the hooked game that need it (not run, the hooked exe stays untouched while the user plays):
(1) golden trace: record a session of the hooked game with `bmxtrace.orig` (docs/TRACING.md) and replay the same
inputs natively once the replay exists; S1-S4 do not touch the simulation (a fixed tick), and the native frames above
are identical to the previous commit's, so the expectation is "unchanged". (2) Pump interval of the hooked game: not
needed, the test program compares our timer with the real winmm directly (mean 16.0 ms in both). (3) File bytes: not
needed, `BMX_FILE_CHECK` compares with the real kernel32 in the same process.

### Open points from the CRT layer (2026-10-28)

- (Done) RW functions reached only through function pointers are now in data/functions-rw.csv: tools/rwmap.py
  collects the targets of RW_CB/BMX_FNPTR, function-pointer globals, data tables and immediates in RW code, and
  separates the D3D driver (docs/RENDERWARE.md, Tooling). A function that is only passed as `BMX_FNPTR(type, addr, fn)`
  needs `BMX_ADDR(addr)` too: without it the hook table has no jump at `addr` and the native exe traps there
  (rwDefaultCalloc, rwPluginDefault*, rwPluginRegEntryRelease had none).
- `pow` and other libm calls come from the host: they can differ from the original's x87 results in the
  last bit (texture gamma tables). Replace with x87-faithful versions before comparing screenshots.
