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
cd <game folder> && <repo>/build/native32/bmxnative.exe               # needs NgBMX.exe in the working dir
```

`BMX_TRAP_SKIP=N` logs the first N traps and fakes a `return 1` from each instead of stopping (a survey of
what the init sequence needs; state is bogus after the first skip). Output goes to stderr and to
`bmxnative.log` next to the exe. SDL3 is not used yet (the window is plain Win32, exactly as the original
WinMain creates it), so `SDL3.dll` is not needed.

Sources: `src/native/loader.c` (hash check, image, imports, jumps), `trap.c` (exception handler, names),
`main.c` (WinMain's job), `range.S` (see below). The game objects are compiled with `BMX_HOOK=1`,
`-ffreestanding -fno-builtin` and the hook build's SSE2 flags; the loader is hosted C.

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

### Open points from the CRT layer (2026-10-28)

- (Done) RW functions reached only through function pointers are now in data/functions-rw.csv: tools/rwmap.py
  collects the targets of RW_CB/BMX_FNPTR, function-pointer globals, data tables and immediates in RW code, and
  separates the D3D driver (docs/RENDERWARE.md, Tooling). A function that is only passed as `BMX_FNPTR(type, addr, fn)`
  needs `BMX_ADDR(addr)` too: without it the hook table has no jump at `addr` and the native exe traps there
  (rwDefaultCalloc, rwPluginDefault*, rwPluginRegEntryRelease had none).
- `pow` and other libm calls come from the host: they can differ from the original's x87 results in the
  last bit (texture gamma tables). Replace with x87-faithful versions before comparing screenshots.
