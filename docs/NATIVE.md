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

## Done for step 1

`bmxnative.exe` starts from the game folder, loads the data, and runs the init sequence until the first
trap; the trap report names the function. A short list of the first traps goes into this file.
