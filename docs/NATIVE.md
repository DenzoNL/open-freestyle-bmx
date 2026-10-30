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
3. **Imports.** Walk the exe's import directory and fill its `.idata` slots with the real Windows
   functions (`LoadLibraryA`/`GetProcAddress`) for now. Later they are replaced one by one by SDL3-backed
   shims (window, input, timer, audio), which is what makes the exe portable.
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
| `BMX_SHOT=<file.bmp>`, `BMX_SHOT_FRAME=N` | save the shown camera raster of frame N (default 1) as a BMP (GPU readback) |
| `BMX_DRIVER_TRACE=1` | log every SDL3 driver entry with its first arguments |
| `BMX_PNG_CHECK=1` or `=<folder>` | compare our PNG reader with the original libpng on every PNG read (and on every `*.png` in the folder), see "PNG reader" |
| `SDL_GPU_DRIVER=vulkan` | SDL's own: pick the GPU backend (default here: direct3d12) |

Output goes to stderr and to `bmxnative.log` next to the exe. Note: a desktop capture (BitBlt / CopyFromScreen) of
the window shows black while the D3D12 swapchain is presented as an independent flip; `PrintWindow` with
`PW_RENDERFULLCONTENT` sees the real content.

Sources: `src/native/loader.c` (hash check, image, imports, jumps), `trap.c` (exception handler, names),
`main.c` (WinMain's job), `range.S` (see below), and the SDL3 render driver in `src/driver/` (docs/SDL3-DRIVER.md).
The game objects are compiled with `BMX_HOOK=1`, `-ffreestanding -fno-builtin` and the hook build's SSE2 flags; the
loader is hosted C. The driver (object library `bmx_driver`) is hosted C with `BMX_HOOK=1` (RW globals at their
original addresses) and is never part of the hook DLL. It is not in build/hook's compile database, so `tools/check.sh`
leaves it out of clang-tidy; tidy it with `clang-tidy -p build/native32 src/driver/*.c` (clang-tidy 21). The same holds for
the PNG reader in `src/png/` (object library `bmx_png`, freestanding, see "PNG reader instead of libpng") and
`src/native/pngcheck.c`: `clang-tidy -p build/native32 src/png/*.c src/native/pngcheck.c`.

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
  only clears lpFormat / lpParms.
- **The game folder's dinput.dll** (the hook DLL, when installed there) is loaded by the import fill as `DINPUT.dll`
  and patches the range too; bmxnative's own jumps are written after it, so they win. It still installs its crash
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

### Open points from the CRT layer (2026-10-28)

- (Done) RW functions reached only through function pointers are now in data/functions-rw.csv: tools/rwmap.py
  collects the targets of RW_CB/BMX_FNPTR, function-pointer globals, data tables and immediates in RW code, and
  separates the D3D driver (docs/RENDERWARE.md, Tooling). A function that is only passed as `BMX_FNPTR(type, addr, fn)`
  needs `BMX_ADDR(addr)` too: without it the hook table has no jump at `addr` and the native exe traps there
  (rwDefaultCalloc, rwPluginDefault*, rwPluginRegEntryRelease had none).
- `pow` and other libm calls come from the host: they can differ from the original's x87 results in the
  last bit (texture gamma tables). Replace with x87-faithful versions before comparing screenshots.
