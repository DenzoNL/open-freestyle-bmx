# 64-bit and cross-platform builds (P4 / P5)

Status 2026-11-01: design; decisions taken (the user): X1-A, X2-A (data in the source), X4-A (bit-exact x87
emulation); X3-A and X5-A as recommended. Goal (the user's): 64-bit builds and builds for other platforms. The
targets are Windows x64 (the user's 3440x1440 desktop), Linux x64 (and the Steam Deck), macOS arm64 and
Android arm64 (an AYN Thor, 1920x1080; its Snapdragon has no 32-bit mode, so 32-bit Android is out).

## Where we stand

- **All code is ported.** 808 of 811 functions, 100% of the bytes (`tools/progress.py`). bmxnative.exe runs
  none of the original's code.
- **It no longer needs NgBMX.exe** (since X1b, below; docs/NATIVE.md "The original's data in the source"). The
  original's data is C source (src/data), one object per original data section with the original's layout; the
  ported code still names its globals by original address (`ORIG_VAR`), which is now that address's place in its
  block. Nothing is mapped at the original's addresses.
- **Platform layer done for Windows** (docs/PLATFORM.md): SDL3 window, input, audio, GPU (DXIL and SPIR-V shaders),
  own AVI/Cinepak, file shims. The import table is SDL3, KERNEL32 and the UCRT.

What ties the game to 32-bit x86 at fixed addresses, measured in `src/`:

| Hazard | Count | Why it matters |
|---|---|---|
| `ORIG_VAR(0x...)` globals | 1603 uses | named by original address (since X1b an offset into a block of src/data with the original's layout) |
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

### X1 plan (2026-11-01)

Survey of what still depends on the original's addresses in the native build:
- 1415 distinct `ORIG_VAR` globals; 166 `ORIG_FN` calls by address; ~370 other raw original addresses (RW plugin
  and pipeline tables, CRT entry points, a few code addresses such as update-list callbacks); 13 `BMX_HOOK`
  conditionals in game code.
- The exe's relocation table lists every absolute address it holds: **1284 pointers in the data** (741 to code:
  function tables; 543 into data), the rest in code (irrelevant natively). The initialised data is ~130 KB, the
  zero-initialised part 1.2 MB.

Two stages, each checked with tools/xcheck.py against the user's recording (work/recordings):
- **X1a, relocated data image.** The native build stops mapping the exe at 0x400000. The loader copies the data
  sections (0x54d000..0x6a2000) into an ordinary allocation and applies the exe's relocations to the pointers in
  them: data pointers move with the image, code pointers become our ported functions (the hook table; incremental
  link thunks resolved to their targets). Game code is compiled in a new mode, `BMX_IMAGE`: `ORIG_VAR(addr)` is
  the image base plus the offset, raw addresses go through one macro, `BMX_FNPTR` is the function, `ORIG_FN` calls
  the ported function. No code range, no int3 traps, no jump table. Still reads NgBMX.exe. The hook build (oracle,
  hook DLL) is unchanged.
- **X1b, the data into the source.** A generator turns each global into a named C definition with its initialiser
  (pointers as `&symbol` or a function name), module by module; the image shrinks to nothing, and with it the need
  for NgBMX.exe. Code that relies on two globals being adjacent (a memset over a range, an index past an array's
  end) is found by the layout check (each carved global's bytes compared with the image at start-up, in a debug
  run) and by xcheck.

### X1a (done 2026-11-01): relocated data image

bmxnative.exe no longer maps anything at 0x400000 and loads none of the original's code; details in docs/NATIVE.md
"Relocated data image".
- **`BMX_IMAGE`** (src/compat/orig.h), defined for everything in bmxnative.exe; `BMX_HOOK` stays on for the game
  objects as the target's conventions. `ORIG_VAR` = `*(type *)BMX_ORIG(addr)`, `BMX_ORIG(addr)` = addr +
  `gBmxImageDelta`; `BMX_FNPTR` = the function; `ORIG_FN*` declare the function with the generated assembler name
  of what serves the address; `RW_CB` = `BMX_ORIG_PTR` (a run-time lookup: RenderWare passes values from tables
  of original addresses). In the hook build every macro expands as before (bmxtest.dll's code is identical,
  dinput.dll's differs only in data addresses shifted by the embedded source path).
- **tools/gen_origmap.py**: the union of the ports (2925), crt.c's table (54), bmxDriverJumps (24), bmxPngJumps
  (17), RpWorldDestroy's stub (a cdecl wrapper), 19 trap stubs for unported addresses that ORIG_FN names but nothing
  reaches, and 21 stubs for the exe's import thunks that ported code calls (AVIFile, DrawDib,
  DirectInputCreateEx) = 3061 addresses. It fails on an unserved or non-canonical `ORIG_FN` address and on a
  `BMX_FNPTR` that names another function than the one serving its address. The CRT, driver and PNG functions in
  those tables became global for it.
- **Loader**: copies 0x54d000..0x6a2000 into an allocation, fills the imports there, applies the 1284 relocations
  in the data: 543 data, 478 code, 17 through incremental-link thunks, 2 into a copy of dinput.lib's object
  tables, 244 to code nothing serves (switch tables of unported CRT / D3D7 code; trap stubs). No int3 range, no
  jumps at original addresses, no `.orig` section in bmxnative (range.S stays for bmxplatformtest); the exe is
  linked at 0x20000000. BMX_PNG_CHECK and BMX_TRAP_SKIP are retired (they needed the original's code range).
- **Raw addresses converted by hand** (the ones not behind ORIG_VAR / ORIG_FN / BMX_FNPTR / RW_CB): data addresses
  as `BMX_ORIG` (rwd3d.h fog and Im3D tables, rwclump.h pipeline tables, d3dpipe.c, the driver's primary context
  and standards table, zscene.c chunk lists, ngifzres.c, zrwmodelcreate.c's hash dummy, the high frame rate's
  effect ops, main.c's WinMain globals); code addresses used as values as `BMX_FNPTR` (ngapp.c's two update
  callbacks, one of them an incremental-link thunk; the high frame rate's node filters; the driver's null standard,
  Im3D transform and matrix multiply; the PVS camera call in world.c; main.c's WinMain callees as ORIG_FN);
  compat/crt.c's `__chkstk` and 64-bit helper forwards jump to the generated names.
- **Verified**: tools/xcheck.py on the user's recording (rec-20261101-140610, 5594 ticks): identical to the
  fixed-address build on every tick (also equal to the 84da8fd hash file); the scripted ride (1014 ticks)
  identical; the gate.

What X1b needs: the data copy is now one block addressed through `BMX_ORIG` / `ORIG_VAR`, and every pointer in it
is known from the relocations, so a generator can carve it into named C definitions (pointers as `&symbol` or the
serving function from bmxOrigMap) and drop NgBMX.exe; `RW_CB`'s run-time lookup then becomes compile-time where
the tables of original addresses (rw/plugins.c attach routines) turn into tables of functions.

### X1b (done 2026-11-01): the data into the source

bmxnative.exe reads nothing from NgBMX.exe any more; details in docs/NATIVE.md "The original's data in the source".

- **Shape: one C object per original data section, with the original's exact layout** (`bmxRdata`, `bmxData`,
  `bmxBss`, `bmxImports`, `bmxData1`, `bmxRwBss`, `bmxInputTables`), not one symbol per global. The plan was
  separate symbols with the adjacency cases grouped; the survey showed the declarations are not object extents:
  - 61 declarations span several globals without containing them (views): byte-array tables over rider and trick
    data (gActionMapTable, gRiderStickDir, gRiderTrickFlags...), gComdataRecords (a 90 KB view from 0x551fd8 that
    covers 476 other declarations: SAVE.DAT is one memcpy over that range), gPhysRiders and gPlayerRecs over the
    same range with different element sizes, gGameRecords over the game-state globals after it;
  - declarations smaller than the object: gFxHandlerTable is a uint8_t at the start of a table of three function
    pointers (`zfxRegisterFuncTable(&gFxHandlerTable, 3)`), likewise gFxGravity, gFxFloorColl, gZglFileDeviceRw /
    Ram, gButtonGlyphs, gScoreSpinMul (7 such, the generator's log);
  - ranges: main.c's initGlobals clears 0x697780..+0x34 in one memset.

  So a carve into symbols sized by the declarations would have broken code silently in many places at once, and
  only game runs (which were not possible while the user played) could find them. Exact layout is correct by
  construction (asserted at compile time, checked byte for byte against the exe); the blocks are still named,
  typed C: their members are the globals, typed as declared, with initialisers and real pointers. Splitting a
  global out of its block into its own symbol is X2's work, one at a time, with xcheck (below).
- **What the blocks hold** (src/data/layout.h, src/data/<block>.c, 1 MB of source, most of it the layout header's
  member list and the 1373 `#undef`s of the headers' macros): 1347 globals as members (1665 declarations at 1462
  addresses; the others are aliases, named in the member's comment), 118 of them through a same-layout mirror
  because their type is local to a .c file (later: move those types into headers); 2051 strings named from their
  text (`sErrorParsingFontIcons`); 924 pointers, 1600 float / word / byte runs and 1600 padding runs nothing names
  (`p_` / `d_` / `pad_<address>`, split wherever the original's code or a pointer refers to an address: the readable
  names are later tidy-up); the 137 import slots; the dinput.lib object tables. The 1.2 MB zero tail is `bmxBss`: no
  initialiser, no bytes in the exe file.
- **Pointers** (the exe's 1284 relocations in the data, plus 268 in the input tables): 813 to data as
  `&block.member.field` expressions, 739 to code as the function's name (incremental-link thunks followed; 237
  addresses nothing serves get a reporting stub each). Casts only where the field's type differs from the
  function's (decided by compiling and reading the compiler's complaints).
- **Imports**: the slots are block members; `bmxImportSlots` (src/data/imports.c) names each slot's DLL and function
  for the loader. The 21 original `jmp [slot]` thunks that ported code calls are now stubs in imports.c through the
  same slot (made from the exe's bytes once), not run-time-patched stubs in origmap.c.
- **Removed from the loader**: reading NgBMX.exe, its sha256 check (and with it bcrypt.dll), the data copy and
  `gBmxImageDelta`, the relocation pass and `BMX_RELOC_LOG`, the thunk table and input-table copies, the scan for the
  exe's import thunks. `bmxLoadImage` is `bmxLoaderInit`: fill the import slots, write the opt-in jumps.
- **`ORIG_VAR(addr)` / `BMX_ORIG(addr)`** = `BMX_DATA_AT(addr)`: the block holding the address plus the offset
  (folded at compile time for constant addresses). The hook build and the oracle are unchanged: bmxtest.dll's and
  dinput.dll's code is identical before and after (only the PE time stamps differ).
- **Blocks start on a page**, like their sections (`.idata`'s block from the section start, the data / zero-tail
  split on the page after the last initialised byte, 0x56a000), so pointers into them keep the original's low 12
  bits, as they did in X1a's image.
- **The exe is now linked at 0x01000000** (was 0x20000000): the data is part of the image now, and the PSX-style
  ordering tables (ngifpsx.c, zoverii.c, the front end) store pointers to it in 28 bits, the top 4 bits being the
  packet's length. Linked at 0x20000000 the first replay crashed in the HUD's draw list (`*entry & 0x0fffffff`).
  The same 28-bit pointers are an X3 (64-bit) hazard: those tables need their own packet type there.
- **Verified**: `tools/gen_data.py --check` (in the gate): all 132830 bytes of the compiled blocks equal the
  exe's, every pointer points where the exe's did; the gate; bmxwidetest and bmxhftest. tools/xcheck.py: the
  user's recording (rec-20261101-140610, 5594 ticks) replayed in native-run3 **with NgBMX.exe renamed away**:
  identical to the native32-84da8fd hash file on every tick; the scripted ride (1014 ticks) identical to X1a's
  build; a run with the movies (ACCLAIM, ZAXIS, INTRO) and the ride, 900 frames, no fault.
- **Found on the way**: zrwmodel.h and zrwzgl.h name 0x697720 / 0x697724 the other way round (gZrwScale and
  gZrwInvScale) and declare gZrwFlags as uint8_t / uint32_t; each module's code is right for its own names, but one
  pair is misnamed (later tidy-up).

What X2 gets from this, and what is left of X1 for it:
- Every global has a typed definition with its initial value and real pointers; to move one out of its block
  (needed for 64-bit, where the layouts change size): check its uses for adjacency (the views listed in the
  generator's log, ranges, `&g + n`), make it a separate definition in its module, replace its member by padding,
  point `ORIG_VAR` uses at the name; xcheck after each batch.
- `ORIG_VAR` still names by original address; X2 replaces it by the names (the members' names are the code's).
- `RW_CB` is still a run-time lookup (bmxOrigPtr); the plugin attach tables could become tables of functions.
- The 118 mirrored types and the synthesized names (`d_` / `p_` / `pad_`, the strings) want tidying by hand.

X1 and X2 are the bulk of the work (thousands of lines touched, mechanical but wide). X3 and X4 should be small
once X2 is done. X5 and X6 are moderate. The order puts the user's own desktop (X3) first.

### X2 plan (2026-11-01)

Inventory: the native build compiled for x86_64 with `-fsyntax-only` (scratchpad probe, to become tools/probe64.py):
- **1750 distinct layout asserts fail.** 1311 are the section blocks' offset asserts in src/data/layout.h (they go
  as globals leave the blocks); ~440 are in 73 hand-written headers, structs that hold pointers (physics_types.h
  65, rwworld.h 30, zrwmodel.h 24, rwcore.h 21, rwplugins.h 19, zactor.h 15, rwtexture.h 15, ...).
- **165 explicit `(uint32_t)(uintptr_t)` casts** (the compiler cannot see them): driver/raster.c 19, zscene.c 12,
  driver/texture.c 12, winload.c 10, zrwzgl.c 8, fx.c 6, ...
- **28-bit pointers** in the PSX-style ordering tables and packets (ngifpsx.c, zoverii.c, the front end; found in
  X1b).
- The rest is small: the trap handler's x86 context, a few data initialisers (`uintptr_t` into `uint32_t`).

Slices, each checked with xcheck against the user's recordings and run in the 32-bit build (the oracle keeps
checking every function):
- **X2a, globals out of the blocks:** each global its own definition, the views (61, the generator's log) turned
  into explicit structs or accessors; the blocks shrink to nothing; src/data/layout.h goes.
- **X2b, which layouts matter:** classify every struct with a failing assert: (1) loaded from or saved to a file
  (assets via zramdisk / ziff / zasset, RW streams, SAVE.DAT) -> an on-disk type with an explicit reader / writer;
  (2) shared with the original only in the oracle -> the assert stays, 32-bit only; (3) in-memory only -> the
  assert becomes 32-bit only, the layout is free.
- **X2c, the on-disk types and readers** from X2b, one format at a time.
- **X2d, the 28-bit pointers:** a packet / ordering-table representation that holds full pointers (or indices into
  a packet arena).
- **X2e, the casts:** each `(uint32_t)(uintptr_t)` reviewed: a real 32-bit value, an index, or a pointer that must
  stay a pointer.
- Then **X3**: the x86_64 build links and runs.

### X2a (done 2026-11-01): globals out of the blocks

Each original global is its own C definition with its own initialiser; details in docs/NATIVE.md "The original's
data in the source".
- **4042 definitions** (src/data/<module>.c by declaring module; declarations in src/data/globals.h): 1038 globals,
  1472 strings, 837 float / word / byte runs, 425 zero objects, 39 pointers and pointer tables something refers to,
  and 231 groups. The section blocks and src/data/layout.h are gone; 7439 bytes of padding nothing refers to are
  dropped.
- **Groups** keep together what the code uses as one piece, as a struct with the original offsets (asserted): the
  views (the 61 of X1b, bounded where their declared type is bigger than what the code uses: gComdataRecords to
  SAVE.DAT's 0x1680 bytes, gPhysRiders to 2 riders, the unused gActionMapTable to gGameLevelInfoIndex's 12), the ranges copied or cleared at once (SAVE.DAT's three ranges,
  initGlobals' memset, zrwworld.c's 0xac-byte memset from gZrwWorldReady, which X1b had missed), globals with the bytes
  after them that nothing refers to (tables declared shorter than they are), and the 11 open-ended tables (arrays
  declared with one element) together with the global after them. SAVE.DAT's three ranges
  are explicit groups: `bmxSaveRecords` (0x551fd8+0x1680) and `bmxSaveOptions` (0x6955e0+0xc3c) exactly,
  `bmxSaveProfiles` (0x6952a0+0x98) with the bytes after it that nothing refers to.
- **`ORIG_VAR` / `BMX_ORIG`** natively name the definition: `BMX_DATA_<addr>` (src/data/addrs.h) is the symbol plus
  the offset. Two sites wrote computed addresses and now index a base (rw/clump.c's normal error limits,
  ngifzres.c's bike table); the hook build's code is identical. Run-time addresses: `bmxDataFind` over
  src/data/units.c.
- **Adjacency, checked statically**: every memset / memcpy with a constant size on a global (the two new ranges came
  from this), pointer arithmetic from a global's address (`(&gZoverSprites)[i]`, `&gGuiPad + (i << 5)`,
  `&gZrollerSlots + n`: all inside their definitions or bounded), comparisons of pointers to different globals
  (none: all six compare within one array), constant out-of-bounds indices (the compiler's -Warray-bounds, clean).
  Not checkable statically: a loop that runs past a declared array into the next global something refers to; the
  bytes after a global that nothing refers to stay with it for that reason. **Found by the first replay**: the
  rider's motion table is declared `gPhysMotions[1]` at 0x6962fc and read as `gPhysMotions[index]` on into
  gActionMotions (0x696300, the same table shifted by one entry, filled by action.c); split apart, level tick 29
  read a NULL motion and crashed. Since then every array declared with one element stays with the global after it
  (11 such: gPhysMotions, the rider step tables, the PSX primitive buffers, gZbspPropFaces, the roller name tables,
  the skeleton tables). Alignment: groups keep their members'
  original alignment; the page placement of X1b is gone (no code found that depends on it).
- **Verified**: `tools/gen_data.py --check` (in the gate; each definition its own section): 1372840 bytes and every
  pointer (813 data, 739 code) equal to the exe's; the gate; bmxwidetest, bmxhftest; the hook build's code
  unchanged; tools/xcheck.py with the user's recording (5594 ticks, NgBMX.exe moved away) identical to
  rec-20261101-140610.native32-84da8fd.hash. **Pending** (the user plays): the scripted ride.

### X2c (partly done 2026-11-01): on-disk types, sizes, offsets, 32-bit-only layout asserts

Work list: docs/notes/layouts64.md. Rule kept throughout: value-identical in the 32-bit builds (the
mechanical commits leave bmxtest.dll, dinput.dll and bmxnative.exe .text/.data/.rdata byte-identical).
- **Done:** zassetDecompressHashed's scratch sized from its pointer table; on-disk types + readers for the
  FILE structs (.zsd banks: ZsdFile*, the in-memory tables built over the file's on 32-bit, beside it on
  64-bit; Replay: ReplayFileHeader, offsets via replayAt; zprim font pages through compat/slot32.h; RW
  chunks 0x10e / 0x112: RpSkinBoneBlockFile, RpLodAtomicFile; tag faces: typed fixups). SAVE.DAT confirmed
  pointer-free (no 64-bit failures in its groups). ~45 literal allocation sizes -> sizeof (zge free lists,
  all RW plugin registrations, RwFreeList's stack, plane sectors, meshes, replay parts as ReplaySnapshot,
  WAVEHDR, DirectInput vtable slots). Byte offsets -> fields (RwGlobals.lightVisit, RpClump.inWorldLink,
  ZObj.data, ZrwCamera fields, Im3D work record, PhysRider stride, physObjectProp). Type pairs merged:
  PhysCollObject = PhysPropObject, ZsndSeqHeader = ZsdTrackDef, RwMatrixGlobals = RwMatrixModule;
  ZtaginfNode / ZrwRenderColNode asserted to agree. compat/layout.h: BMX_LAYOUT32_ASSERT; the 409
  failing in-memory asserts converted. tools/probe64.py (--list, other errors): **418 -> 0 failing asserts
  outside src/data** (src/data: 1780, generator).
- **Found after the merge (2026-11-02):** X2a's adjacency check looked at memset / memcpy only; four `zmemFill`
  calls cleared runs of globals that X2a had made separate definitions (zrwInitApp 0x88 from gZrwRaster and
  0xa8 from gZrwInitialized, zrwGraphicLibraryInitialize 0x10 from gZrwScreenW, zprim's 10 words from
  gZprimLibrary). They only worked because the linker kept the definitions in source order. Now cleared global
  by global (`ZRW_CLEAR`); replays unchanged. A wider search (every fill / copy with a literal size on a global)
  found no other.
- **Verified:** xcheck rec-20261101-140610 (5594 ticks) = native32-84da8fd hash, scripted ride (1014) equal
  (after the FILE, sizes and offsets commits); attract mode demo (Replay.ZAL, 2130 ticks) equal to the X2a
  exe; oracle subsets (winload with real assets, zsndseq, zprim, replay, RW plugins, light, clump).
  Pending: full gate after the last commits.
- **src/data on 64-bit (2026-11-02):** src/data compiles for x86_64 with every assert holding.
  - Offsets that differ with the pointer width are written `BMX_DATA_OFF(o32, o64)` (compat/layout.h) and
    asserted on both widths: the group member and extent asserts of globals.h (132 groups), addrs.h (198 globals
    reached inside a group) and units.c (28 units behind a `lead_`). tools/data_off64.py computes o64 from an
    x86_64 compile of the headers and rewrites the three files; re-run it after changing a type inside a group.
    Chosen over giving the code the group types (their member names are the code's ORIG_VAR macros), per-member
    assembler symbols (object-format specific) and run-time offset variables (not constant expressions).
  - Three addrs.h entries named a field by byte offset and now name it: gZglMaterials = gZrwMat.pool,
    gZrwMarkers = gScnpts.markers (was the u32 `field14`), gTrickBarspinNames = &gTrickTurnNames[46].
  - Pointers in u32 slots typed as pointers: the effect tables' slots (ZfxEffect.addModifier, slots0c), the
    first slot of four object operation tables, gZrwWorldPathArgs.
  - The two group memsets by literal size (zrwworld 0xac, main.c 0x34) clear global by global.
  - The import thunks of src/data/imports.c are i386 code: their slot offsets are asserted on 32-bit only.
  - Open for X3: bmxDataFind maps an original address linearly inside a unit (units.c's original ranges),
    which is wrong past a moved member on 64-bit.
- **Second batch (2026-11-02):**
  - PropState is the whole 0x694460..0x694fa4 block (records, a word, 64 node states); what is saved, restored and
    kept as the defaults is `kPropStateSaved` = up to the last state's third word (0xb40 on 32-bit);
    gPropRecords / gPropNodeStates are its fields. replay.c's part B uses the same size (parts A-C never reach a
    file: the demos are created with flags 7).
  - The contact a rider or snapshot copies is a union of PhysFace, PhysEdge and PhysPropObject (500 bytes on
    32-bit); contactSize is their sizeof. physics_types.h is maintained by hand (its spec is stale since X2c).
  - PhysPropInfo was a stand-in for PropRecord (facing = orient.up); PropRecord moved next to PhysPropObject.
  - d3dim2d's deferred records are RwD3DDeferVerts / RwD3DDeferPrim; the callback slot is pointer-sized.
  - Pointers that travelled as u32: the model / texture user data (zrwMdlSetUserData, zrwTextureSetUserData,
    zrwMatGetUserData), ZrwModel.userValue, ZrwColHit.objectData / materialData, PhysFace.owner (a ZrwColHit),
    ZbspObjectFrameFn. The rig hit callbacks (riderCollide, guiPlayVelocitySound) take ZRig / ZRigHit (ZoverHitFn
    = ZRigHitFn) instead of a u32 motion and a word array.
  - RwRaster declared to its 0x38 bytes (the driver's saved level 0 size are its original* fields; the raster
    module's root raster is RwRasterGlobals.root); src/data's 14 plugin registries start at sizeof(type).
  - Byte offsets named: game.c (prop marker frame, contact tag), physics_air.c, rider.c (ray hit, rig point
    velocity), ngifzbsp.c (material ref, hit corners), world.c (light hook = stage4, work owner). Checked and
    left: game.c's trick grid offsets (PhysRider has no pointer before 0x488), zrwcol / zrwprim / model render
    buffers (pointer-free; written as capacity times size), frontend_select (an asset number), zscene (done in
    X2c), guiinput's 32-byte stride (pointer-free).
  - tools/xcheck.py --shots compares screenshots of the replay too (with `vanilla = 1`); the ride's frames 1000,
    1200, 1400 and 1800 are byte-identical to the exe before this batch.
- **ngifzmemdmm:** the handle record size, the smallest payload (a free-block header) and the rounding come from
  the types (0x10, 12 and 4 where pointers are 32 bits wide); "no free block" is UINTPTR_MAX, above every block.
- **Left (X2c/X2e):** crt.c rwFexist (original address); type pairs ZPropDef/ZbspTreeNode, ZPropInst/PhysPropNode,
  ZrwRenderColTriangle = ZbspLevelFace (pointers read as u32 tags), ZObj views, OvlFont/ZOverFont, ZrwShadow*,
  ZrwColTriangle; the `(uint32_t)(uintptr_t)` casts (X2e: query / request arrays, zfileOpenRamDisk addresses, RW
  render state values). X2d: the 28-bit ordering tables.

## Not in scope

- Running without the user's game files: the assets, movies and music stay theirs.
- 32-bit ARM builds.
