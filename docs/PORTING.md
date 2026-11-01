# 64-bit and cross-platform builds (P4 / P5)

Status 2026-11-01: design; decisions taken (the user): X1-A, X2-A (data in the source), X4-A (bit-exact x87
emulation); X3-A and X5-A as recommended. Goal (the user's): 64-bit builds and builds for other platforms. The
targets are Windows x64 (the user's 3440x1440 desktop), Linux x64 (and the Steam Deck), macOS arm64 and
Android arm64 (an AYN Thor, 1920x1080; its Snapdragon has no 32-bit mode, so 32-bit Android is out).

## Where we stand

- **All code is ported.** 808 of 811 functions, 100% of the bytes (`tools/progress.py`). bmxnative.exe runs
  none of the original's code.
- **It still borrows the original's address space** (docs/NATIVE.md). The loader maps the user's NgBMX.exe at
  0x400000 and copies its data sections there; the ported code reads its globals at those addresses. The code range
  holds jumps to our functions, so function pointers stored in the data land in our code.
- **Platform layer done for Windows** (docs/PLATFORM.md): SDL3 window, input, audio, GPU (DXIL and SPIR-V shaders),
  own AVI/Cinepak, file shims. The import table is SDL3, KERNEL32, bcrypt and the UCRT.

What ties the game to 32-bit x86 at fixed addresses, measured in `src/`:

| Hazard | Count | Why it matters |
|---|---|---|
| `ORIG_VAR(0x...)` globals | 1603 uses | read at the original's addresses; their initial values come from the exe's data |
| Function pointers in the data, `BMX_FNPTR` | 333 | 32-bit code addresses in tables (RW plugins, zobj ops, effect types...) |
| `ORIG_FN` calls by address | 186 | calls through the original's addresses (now jumps) |
| `_Static_assert(offsetof / sizeof)` | 416 / 247 | layouts pinned to 4-byte pointers |
| `(uint32_t)(uintptr_t)` pointer casts | 165 | pointers stored in 32-bit fields |
| `regparm`, `__stdcall` / `BMX_STDCALL` | 251 / 132 | i386 calling conventions (no meaning on x64 / arm64) |
| Inline x87 assembly | 14 files | `fsin`, `fcos`, `fptan`, `fpatan` and 80-bit sequences, for bit-exact results (src/compat/x87.h) |
| Assets loaded straight into structs | zramdisk, ziff, zasset, RW streams, SAVE.DAT | on-disk layouts with 32-bit pointer fields (PLAN.md "64-bit hazard") |

## Decisions

### X1. Route

- **A (recommended):** straight to one 64-bit codebase for all four targets, in slices (below), with the 32-bit
  builds kept as the reference throughout.
- B: first a Linux i386 build that maps the image at 0x400000 like Windows does (Steam Deck sooner, little work),
  then 64-bit. Distros are dropping i386 and it does nothing for macOS or Android, so the work is mostly thrown away.

### X2. The original's data (initial values of the globals, the tables)

Size: the exe's initialised data is ~130 KB (`.rdata` 15 KB, `.data` 110 KB on disk, `.data1` 16 KB), ~70 KB of
it non-zero, ~29 KB of it strings; the rest of `.data` (1.2 MB) is zero-initialised. For comparison, the code we
already rewrote as C is 1.35 MB.
- **A (recommended, discussed with the user 2026-11-01):** the data in the source, like the code. A tool extracts
  each global once into a named, typed C initialiser (`gJudgeBias[10] = {10, 8, 2, ...}`, pointers as `&symbol`
  or the function's name), then it is tidied like ported code. The tables are the developers' source initialisers,
  of the same kind as the constants in the ported functions; the decompilation projects (Super Mario 64, Ocarina of
  Time) keep code and data tables in the repo and take only the assets from the user's copy. Builds then need no
  NgBMX.exe at all, only the game's data folders; releases can be published. **What never goes into the repo or a
  release:** the game's files (levels, models, textures, sounds, movies, music) and NgBMX.exe. Brand names in the
  strings (Dave Mirra, Acclaim, sponsors) stay out of the project's name and release titles.
- B: keep reading NgBMX.exe at startup and copy each global's initial value into a C variable through a generated
  table that also translates the pointers inside. No game data in the repo, but more runtime machinery, the exe
  stays a requirement, and the globals stay unnamed blobs.
- C: generate the initialisers at build time from the exe (never committed). Binaries contain the data anyway.

### X3. Pointers in structs

- **A (recommended, PLAN.md already chose it):** in-memory structs use native pointers; every on-disk format
  (assets, RW streams, SAVE.DAT) gets its own fixed-width type with an explicit reader / writer. The layout asserts
  stay on the on-disk types. The biggest slice. The zmem pools are sized for 32-bit structs and need checking.
- B: 32-bit handles (offsets into one arena) instead of pointers, so the layouts stay. Every dereference changes,
  and the code is much harder to read. Not recommended.
- C: keep 32-bit pointers by allocating everything in the low 4 GB. Not possible on macOS arm64 (the low 4 GB are
  reserved), not guaranteed on Android. Not recommended.

### X4. Floating point on arm64

The game's maths is bit-exact to the original today: SSE2 for float / double, and real x87 instructions where the
original used them. x86-64 still has the x87 unit, so Windows x64 and Linux x64 keep the inline assembly
(GCC syntax, clang and mingw). arm64 has no x87.
- **A (recommended):** portable C versions of the handful of x87 operations, bit-exact with the hardware. They can
  be tested exhaustively on this PC: the inputs are floats or ints (2^32 cases each), compared against the real
  `fsin` etc. Also `-ffp-contract=off` everywhere (arm64 compilers would otherwise fuse multiply-adds and change
  results). Then the simulation hash is the same on every platform, and replays and the golden traces work on all
  of them.
- B: use the platform's libm on arm64. Differences in the last bits, invisible in play, but the simulation hash
  then differs from the 32-bit reference on arm64 and the oracle cannot vouch for those builds.

### X5. Shaders for Metal

SDL3 GPU needs MSL on macOS. Today tools/shaders.sh uses DXC for DXIL and SPIR-V.
- **A (recommended):** convert the SPIR-V to MSL with SPIRV-Cross in the same script, commit the generated
  headers like the others. Small.
- B: write the MSL by hand. Two copies of every shader to keep in step.

## Slices (with X1-X5 as recommended)

The 32-bit hook build and the Unicorn oracle stay the reference for every function. A cross-build check is the
safety net for everything else: the same scripted run (BMX_KEYS) or recorded golden trace on two builds must give
the same per-tick BMX_SIM_HASH.

| Slice | Content | Done when |
|---|---|---|
| X0 | Cross-build check: replay a golden trace / key script on two exes, compare BMX_SIM_HASH per tick (also the "golden-trace replay" backlog item) | native32 vs native32 equal, a deliberate change caught |
| X1 | The layout flip on 32-bit Windows: native32 built without fixed addresses. Globals become C variables with initialisers in the source (X2-A), function pointers in the data become function names, `ORIG_FN` calls direct; no image at 0x400000, no code range, no NgBMX.exe | sim hash equal to today's native32; the hook build and oracle unchanged |
| X2 | Pointer-size cleanliness, still 32-bit: on-disk types and readers for the asset loaders, RW streams and SAVE.DAT; the pointer casts removed | sim hash equal; old saves load |
| X3 | Windows x64 (llvm-mingw x86_64, SDL3 x64) | sim hash equal to native32; the user plays it |
| X4 | Linux x64: case-insensitive file lookup, `/` paths, the save / config locations, packaging (AppImage or a tarball) | sim hash equal; runs on the Steam Deck |
| X5 | Portable x87 maths (X4-A, exhaustive tests); MSL shaders (X5-A); macOS arm64 app bundle | sim hash equal on arm64 |
| X6 | Android arm64: SDL3 Android project, game files chosen through the system file picker, controller (the Thor's built-in pad), 1920x1080 widescreen | plays on the Thor |

### X0 (done 2026-11-01)

- **Recording** (`BMX_RECORD=<file>`, src/native/replay.c): the input as the game receives it from the DirectInput
  stand-in, changes only, keyed by shown frame and call index within the frame, plus the pad the game saw and the
  starting SAVE.DAT. Some loops poll thousands of times per frame (as often as the clock lets them), so only
  changes are stored: a scripted minute is 15-18 KB (the first, every-call version was 1.6 GB). A frame mark about
  once a second keeps the replay as long as the session.
- **Replay** (`BMX_REPLAY=<file>`): the recorded pad stands in (no real one is opened), the focus is ignored, the
  window stays in the background; each call gets the latest recorded state at or before its (frame, index). The
  run ends 60 frames after the last record.
- **Check** (`tools/xcheck.py --game-dir <dir> --recording <file> <exe> [<exe>...]`): puts the recording's
  SAVE.DAT in place (and back), replays on each exe and compares the BMX_SIM_HASH files; names the first tick that
  differs.
- **Verified:** a scripted ride (pedal, steer, jumps; 973 ticks) recorded and replayed: identical to the recorded
  run on every tick, also with `[general] vanilla = 1` on the replay (no widescreen, no interpolation). A build with
  one gravity term scaled by 1.0001 in the rolling physics is caught (differs from tick 280); a copy of the same
  exe is identical.
- **For the user:** `native-run\record.ps1` plays with the personal ini and saves `recordings\rec-<time>.bmxr`.
  Sessions with the DualSense, several levels, tricks, grinds and bails give X1-X3 their reference.

X1 and X2 are the bulk of the work (thousands of lines touched, mechanical but wide). X3 and X4 should be small
once X2 is done. X5 and X6 are moderate. The order puts the user's own desktop (X3) first.

## Not in scope

- Running without the user's game files: the assets, movies and music stay theirs.
- 32-bit ARM builds.
