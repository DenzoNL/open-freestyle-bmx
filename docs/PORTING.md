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
| `regparm`, `__stdcall` / `BMX_STDCALL` | 251 / 132 | i386 calling conventions (no meaning on x64 / arm64; `BMX_REGPARM(n)` since X5 part 1) |
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
| X4 | Linux x64: case-insensitive file lookup, `/` paths, the save / config locations, packaging (AppImage or a tarball) (2026-11-06: builds, headless ride hash equal to arm64 under Rosetta) | sim hash equal; runs on the Steam Deck |
| X5 | Portable x87 maths (X4-A, exhaustive tests); MSL shaders (X5-A); macOS arm64 app bundle | sim hash equal on arm64 |
| X6 | Android arm64: SDL3 Android project, game files chosen through the system file picker, controller (the Thor's built-in pad), 1920x1080 widescreen (2026-11-06: step 1, libmain.so and a debug APK build; not run on a device yet) | plays on the Thor |

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
- **X2d, the 28-bit pointers:** done; the packet links are offsets from an anchor on 64-bit (below).
- **X2e, the casts:** each `(uint32_t)(uintptr_t)` reviewed: a real 32-bit value, an index, or a pointer that must
  stay a pointer (done; below).
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
- **Third batch:** crt.c's rwFexist goes through the engine's file functions instead of an original address.
  The placed prop and the prop list node are one type (PhysPropNode = ZPropInst, zprop.h), and so are the prop
  definition and the tree node (ZPropDef = ZbspTreeNode, with ZbspTreeObject as its data from the id on). The
  roller's shadow embeds ZrwShadow (zrwshadowstate.h), and its two model slots are pointers. The original
  defined only 4 bytes at 0x5fb710 and the roller wrote slots[1] past them; src/data now defines both slots.
  ZObjCamera and ZObjLight start with ZObj's head, and asserts tie ZrwColTriangle to RpCollisionTriangle and
  OvlFont to ZOverFont.
- **Fourth batch:** the file function table is RenderWare's RwFileFunctions everywhere (zrwfile.c's copy,
  zprofile.c's saved one). The RAM disk callbacks zprofile.c installs have the table's signatures (`void *`
  handle slot, `size_t` counts); before, they were stored through a `void **`. The effect tables are typed:
  ZfxEffect is the effect type of zrwworld.c and the item class of zobjpemit.c, and ZfxEmitter is their
  record. The spawners' first argument, called `enabled`, is the effect, and the renderers take the 0 that
  the world's effect object passes. The effect object is a ZObj whose data is ZobjPEmitState (the world's
  ZrwEffectHost). The particle steps are the gravity and floor modifiers' apply functions (ZfxModifier,
  ZfxParticle).
- **Fifth batch:** the level's triangle (ZbspLevelFace, now in ztaginf.h) is the renderer's collision triangle,
  so the debug and query code tests the tag pointers instead of reading them as words. The hit list carries
  the tag as a pointer. ZrwMaterial spells the `rwMaterial` overlay in slots: on 64-bit, `pad2c[0x44]` would
  have put the RpMaterial over slot 4's texture handle. ZSkeletonOwner is ZObj.data. PhysRider needed
  nothing more: the overlay's byte offsets all lie before the first pointer, and an assert on every width
  pins that.
- **Left (X2c/X2e):** the emitter object's ops (gZrwWorldEffectOps, `void *` slots, a ZObjOps); the
  `(uint32_t)(uintptr_t)` casts (X2e: query / request arrays, zfileOpenRamDisk addresses, RW render state
  values, the texture words of zrwprim and ZfxParticle).

### X2d (done 2026-11-03): the 28-bit packet links

The PSX-style display lists (ngifpsx.c, the overlay pools of zoverii.c, the menu frame polygons of frontend.c /
frontend_options.c, and wide.c's walker) chain packets through the low 28 bits of each packet's first word; the
top nibble is the packet's tag. Encoded with `(uint32_t)(uintptr_t)p & 0x0fffffff` (about 12 sites) and decoded
with `(uint32_t *)(uintptr_t)(word & 0x0fffffff)` (3 sites); 0x0fffffff ends a chain. Every packet and ordering
table is static data (src/data: the ordering table, the primitive buffers, the overlay pools, the menu polygons),
which on 32-bit sits well below 256 MB. On 64-bit the address does not fit.

Options:
1. **Widen the link** to a pointer. Every packet layout (byte offsets read with get32 / put8) shifts, the packets
   stop matching the original's, and the hook tests of those functions lose their reference. Rejected.
2. **Links as offsets from an anchor** in the program's data: encode `p - anchor` in 28 bits (signed, so the
   packets may lie on either side of it), decode `anchor + offset`. 32-bit builds keep the anchor at 0, so the link is
   the address as now: the hook build, the tests and the replay hashes are unchanged. On 64-bit the anchor is a
   src/data symbol; the image's data spans a few MB, far inside the ±128 MB a signed 28-bit offset reaches.
   An assert in the encoder catches a packet outside that range (heap memory, say).
3. **An index into a packet arena**: one allocation that holds every packet, links as indices. Moving the
   buffers out of src/data changes the hook build's view of the original's data. Rejected for that reason.

Recommendation: 2, in one header (compat/pktlink.h: `bmxPacketLink(const void *)` / `bmxPacketAt(uint32_t)`) used by
all the sites, so the representation is decided in one place.

Decision (2026-11-03, the user agreed): option 2, done. compat/pktlink.h holds both encodings, and the five
files that link or walk packets use it (ngifpsx.c, zoverii.c, frontend.c, frontend_options.c, wide.c). The
32-bit builds are unchanged: the hook cases pass and the replays stay identical. Should the HUD / UI ever be
reworked, give it a proper draw list instead of building on the PSX packet chains (PLAN.md, backlog).

### X2e (done 2026-11-04): the casts

148 casts of a pointer to 32 bits (or back) outside src/data at the start. Each is a real 32-bit value (keep), an
index, or a pointer the types should carry. `tools/width_audit.py` finds the places the compiler cannot see:
an ORIG_FN declaration or a BMX_FNPTR cast whose parameter is a pointer while the ported function takes a 32-bit
integer (or the other way round), one original variable declared as a pointer in one file and an integer in
another, and function pointers kept at one address whose aliases disagree. On 32-bit those calls work; on
64-bit the integer side keeps half the pointer.

- **First batch:** object handles (`zobjSetVisible`, the actor's object and shadow, the owning prop of the
  collision faces) are pointers; the ray hit's `object` really is a tree-object index and stays an int. The
  file layer: the device table's six unused slots were `uint8_t[0x18]`, three pointers on 64-bit, which put
  open/close/read on the wrong slots; they are `void *[6]` now, with an all-width size assert. Device control
  takes the attribute keys and values as pointers and returns the file name; seek and tell take the callback
  and the position as pointers. `zfileOpenRamDisk` takes the block. The name-table-2 handlers (the effect
  creators) take the object, and their `ngiffxSpawn` declaration matches zrwworldApplyEffect.
- **The RAM disk's names:** a memory block is opened as the file `rd00:<address>:<size>`, the address written by
  zramdiskControl and parsed back by zramdiskOpen. 32-bit builds keep `%#10x` (the oracle compares the
  name). 64-bit builds write the address with `%#18llx` and parse it with strtoull (zstrtoul's unsigned long is
  32 bits on Windows); the 0x80-byte name buffers have room. The alternative, a separate open-from-memory call,
  would bypass the device layer the original goes through, for no gain.
- **Scenes:** the search state (ZsceneQuery) and the handler request (ZsceneRequest) are structs instead of
  int arrays; the animation table and the action motion table hold pointers.
- **Render states:** RenderWare's render state value is an `intptr_t` (a number, or the raster, fog density or
  fog table pointer); the engine's current raster (gZrwRenderExtra, the state stack, the 0x561490 hook) is a
  pointer. The getter copies the raster and fog table pointers whole.
- **The audit's first findings:** declarations of the window procedure's wParam, the camera viewport and clear
  flag setters, zrwCreateWorld's result, the shape-kind query; the update register / unregister hooks
  (0x561470 / 0x561474: the node is a pointer, and the four globals holding nodes); the parameter-set hook's
  third argument; the primitive draws' matrix; gFxSceneObject, gZrollerUpdateHandle, the LOD free list.
- **Second round:** the audit also compares each function pointer variable with the function src/data stores
  in it. That found the model table (0x560e84..: generic `void *(void *, void *)` slots for functions taking
  hashes and flag bytes; each slot now has its target's type, and the clump query no longer comes back through
  an `int`), the parameter queries' third argument, the assert handler's function name, the light's position
  op, the line query's exclusion list. Other pointers given their types: asset libraries (`ZassetLibrary *`
  instead of an int handle), GUI message arguments (`intptr_t`, like WPARAM / LPARAM), zprimSet / zprimGet
  values, light colour blocks, the child walk's argument, skeleton nodes (the actor's part rows, ZObjLinks),
  the sphere query's result, the excluded-clump list, the engine's file device table, window and calloc slots,
  the waveOut handles and callback (pointer-sized, as in WinMM's 64-bit API), the pad device's DirectInput
  record, zprim's frame data address, the Im3D pipeline's stages, the world sector streams' world argument,
  the image pixel addresses (offsets from the pixel pointer, wrapping at 32 bits as the original's do), the
  jumps over our own functions (BmxHook.orig; the rel32 is range-checked), the frame-pacing tools' node ids,
  printf's `%p`, the driver's trace arguments.
- **Two real 64-bit bugs on the way:** the driver read the original's standards table (`{id, function}`
  pairs, which src/data lays out with a pointer-sized function) as 32-bit word pairs, now a typed pair; and the
  model table's clump query returned the clump through an `int`.
- **Kept (a 32-bit value on purpose):** numbers passed in pointer parameters (render flags, the viewport word,
  the mipmap switch, a sound's argument), the original's quirks that use a pointer's low bits as data (zrwColSearch's
  split key, zsndseq's default argument bytes, the palette read's first value), debug prints of an address,
  compat/pktlink.h and compat/slot32.h themselves, the RAM disk's 32-bit name, crash.c and trap.c (32-bit hook
  build; trap.c's context fields are X3).
- **Integers back into pointers:** the sweep the other way (a 32-bit word cast to a pointer) found the scene's
  animation list freed through word offsets, the scene point's data word, the animation sequence's name; all read
  through their types now. The texture handles: below.

#### The texture handles

A texture (a ZrwMaterial, from the heap) is handed around as a 32-bit word: in the PSX packets' texture words,
the effect parameter blocks (ZfxParticle and the spark / ripple / skid / splash words), zprim's sprite frames
(inside file images used in place), the menu and shadow globals. Three engine hooks (0x561024 free, 0x561034 get
raster, 0x561048 load) are declared with a pointer by some callers and a 32-bit integer by others.

Options:
1. **Widen the words** to pointers. The packets, parameter blocks and sprite frames change layout (their byte
   offsets are read with get32 and file images are used in place), so the oracle loses its reference for those
   functions and the file images need readers. Rejected.
2. **Handles through compat/slot32** (the table X2c added for the zprim font's page table): where pointers are
   32 bits the word is the pointer, as now (hook build, tests and replays unchanged). On 64-bit the word is a
   small handle: the material gets one when it is created, every 32-bit word holds that handle, the hooks'
   integer callers turn it back into the pointer, and freeing the material drops it.
3. **Materials from an arena near the program** (as X2d's anchor), the word an offset. Materials come from
   RenderWare's and the engine's heaps; moving them all into one arena touches the allocators. Rejected.

Recommendation: 2, behind a pair of functions next to the material code (the handle of a material, the material
of a handle) so the representation is decided in one place.

Decision (2026-11-04, the user agreed): option 2, done. `zrwmatHandle` / `zrwmatFromHandle` (zrwmat.h) are the
pair; on 64-bit the material remembers its handle in a field only that build has, and zrwmatReleaseTexture drops
it with the last reference. The texture hooks (0x561024 free, 0x56103c clone, 0x561034 get raster, 0x561048
load, the find / load by hash ones) take and return materials in every declaration; the words that hold them
(zprim's frames, the shared table at 0x68c3e0 in all four of its views, the effect globals and parameter
blocks, the 3D list's packets, the primitives, the shadow quads) hold handles, converted where they are made
and where they are used. The skid animation's table is ours alone and holds materials. Found on the way: the
overlay view of the 0x68c3e0 table kept a `void *` at +0x10, which made its entries 24 bytes on 64-bit inside
a table of 20-byte entries; the effect skid search walked the prop definitions as 32-bit words.

tools/width_audit.py reports nothing now, and the gate runs it (tools/check.sh, "pointer widths").

### X3 (in progress, 2026-11-05): Windows x64

One codebase builds both: the `native32` preset as before (unchanged behaviour, the replays stay identical) and the
new `native64` preset (cmake/x86_64-mingw.cmake, llvm-mingw x86_64, SDL3's x86_64-w64-mingw32 package). The
differences are compile-time switches on the pointer width (`BMX_LAYOUT32`, compat/layout.h) or the CPU
(`__i386__`).

**Build:**
- Assembler names: `__asm__("...")` labels go through `BMX_SYM("name")` (compat/layout.h): the i386 Windows
  targets put an underscore before C names, x86_64 does not (src/data, gen_data.py, gen_origmap.py, 4 headers).
- The naked entries with the original's register conventions (EAX / EDX / ECX and ST0, 8 in src/rw) are i386 only
  (`BMX_X86_ENTRY`, compat/orig.h); on x86_64 they are a trap and our callers call the C implementation (world.c's
  PVS camera). The MSVC compiler helpers of crtasm.S are i386 only; the crt table names trap stubs on x86_64.
- The PNG reader's error jump: the CRT's `_setjmp3` / `_longjmp` are i386 code; x86_64 uses
  `__builtin_setjmp` / `__builtin_longjmp` (5 pointer slots of the png struct's 64-byte jmp_buf).
- The import thunks (`BMX_IMPORT_THUNK`) jump RIP-relative through slot offset times 2 (every slot a pointer).
- trap.c reads RIP / RSP; under a debugger it does not install its handler (the debugger reports the fault).

**Bugs found running it** (all invisible on 32-bit):
- RenderWare's standard function table (`RwGlobals.stdFunc`) was `uint32_t[]`, filled through a `void **`.
- The driver's standards table in src/data sat behind a 12-byte lead: typed as (slot, function) pairs now.
- Sizes written in 4-byte units for pointer arrays: the GUI sort (qsort element 4), RW frame lists, the PNG row
  table, zfont glyph tables, zlist, the material list, morph targets, the mesh sort, the 2D sort items, the
  camera's visible sector list.
- Byte offsets into module data: the world module's free list and list (now `RpWorldGlobals`); the PNG io pointer
  at 0x54 (`bmxPngIoPtr`).
- `PhysPropFrame` was a view of `ZObj` with its pointers as padding: now the same type.
- The collision query kept the nearest atomic / sector in `uint32_t` fields, passed in as `uint32_t` arguments.
- `zrwmatClone` copied the material's 64-bit handle with the record: clone and original shared one handle.
- src/data declared the small HUD font (0x56bf38) as a byte and padding: the 64-bit `OvlFont` overran it.
- Copies with 32-bit byte counts: the goal marker lists (64 pointers copied as 0x100 bytes), the LOD atomic data's
  header (0x18 bytes up to `levels`).
- The x87 matrix product (`bmxX87MatrixMultiply`, compat/x87.h) addressed its matrices through EAX / EDX / EBX:
  on x86_64 that drops the upper half of each pointer. It uses the 64-bit registers there now.
- tests/platform: `GWL_HINSTANCE` is not declared for 64-bit Windows; `GWLP_HINSTANCE` (the same -6) is.

The game code calls no libm function (the transcendental functions go through compat/x87.h's real x87
instructions, everything else is SSE2 on both widths), and the x87 control word is set by `fldcw` directly (not
`_controlfp`, which ignores the precision on x64), so neither explains the last-bit difference.
Found running the scripted ride and the user's recording on both widths (all invisible on 32-bit):
- Records src/data held as loose words where the code reads structs with pointers: the 18 overlay menu screens
  (now `gOvlScreenRecords`, `OvlScreen`; `gOvlMpModeLines` is screen 14's item count), the pad profiles (the three
  `ZpadProfile` records with their NULL-ended name and GUID lists, `gZpadProfiles` a 3-entry list; the button map
  is read as `profile->map`, not at a literal +8).
- The prop lists' nodes: `ZPropDef` / `PhysPropNode` had a 32-bit word where zrwList keeps its `prev` pointer, and
  32-bit words for the release functions zmem's deferred flush calls (`defRelease`, `shapeRelease`,
  `tagRelease`); the flush walks the real types now instead of its own 32-bit views. The effect modifier node
  had the same `prev` word.
- The controller record read through byte offsets (`contrlerGet` returned bytes): `riderReadPads` read the
  buttons and sticks 4 bytes early on 64-bit (the device pointer grew). `contrlerGet` returns `ContrlerPad *`.
- `ngappLinkFaces` stores the linked face in the level file's tag record, whose slot is a 32-bit word: on
  64-bit the pointer zeroed the next record's id (faces lost their surface kind). Its readers only test the slot
  for zero, so 64-bit stores the face index + 1 (32-bit unchanged).
- `physMoveScripted` read the rigs' centre, basis and velocity at literal offsets; `ZRig`'s centre moves on 64-bit
  (its hinges hold pointers). Named fields now.

#### The SSE rounding after a driver call

On 32-bit Windows this PC's graphics driver, compiling a pipeline (SDL_CreateGPUGraphicsPipeline, first use of a
render state), leaves the calling thread's MXCSR rounding set from the x87 control word. Pipelines are made during
the camera update, when the x87 rounds down (camera mode), so from the first one on (tick 0) the 32-bit game
logic ran its SSE maths rounding down; the 64-bit driver does not do this. That was the last-bit difference from
tick 1. It also means the 32-bit reference replays depended on the GPU driver.
1. Save and restore both control words around every SDL GPU call that runs driver code (`DRV_FPU_GUARDED`,
   driver.h: object creation, submit, swapchain, window claim). The game logic rounds to nearest as the port
   intends, on any driver and width. The 32-bit replays change once (new reference hashes).
2. Imitate the accident on 64-bit (round down after the first pipeline): keeps the old reference hashes, but
   encodes this driver's quirk; another driver gives other results.
Decision (2026-11-05, confirmed by the user 2026-11-07): option 1.

#### The engine instance's plugin data

`EngineOpen` mallocs the engine instance and copies only the static part; the modules' data behind it stays as
the heap returned it. The image module never sets its name buffer fields and relies on them being zero; with a
reused heap block the size is garbage, no name buffer is made and every image lookup fails (on 64-bit about one
start in seven: the shadow texture, then a crash in LoadTexture). The original has the same read.
1. Zero the plugin part in `EngineOpen` (engine.c). What the original evidently relied on; the only change is
   where the original read uninitialised memory.
2. Initialise just the image module's two fields in `_rwImageOpen`: narrower, but any other module with the
   same assumption stays exposed.
Decision (2026-11-05, confirmed by the user 2026-11-07): option 1.

**tools/data_size64.py** looks for the src/data class above: every `ORIG_VAR` site whose type, on 64-bit, reaches
past the definition (or group member) that src/data gives its address, while it fits on 32-bit. It compiles a probe
after src/data's headers for both widths, and for types local to a .c file a probe that follows that file. Found
and fixed besides the font: the collision list `gZrwColList` (src/data had its own copy of the record with a
32-bit `tag`, now the code's `ZrwRenderColHit`), the motion bank table (src/data reserved 0x18 bytes; it is one
bank now, as only `zmotionLoadExtraBank`, which nothing calls, would add a second over the next globals), and the
level name table (declared 32 entries, src/data has the 14 levels). Now: 1633 sites checked, 0 found (the one it
cannot name is a float table). It takes some minutes, so it is not in the gate.

#### zmem's headers

zmem hides a 32-bit size word before every heap and static block, and its free lists and pools a link header
before each node; the original adds them with literal 4s and 8s. On 64-bit that gave 4-byte-aligned payloads and
node headers too small for their two pointers.
1. Natural sizes on 64-bit: the header region before a payload is 8 bytes (the size word in its last 4), sizes
   round up to 8, free-list and pool headers are their structs' sizes. 32-bit unchanged.
2. Keep 4-byte alignment on 64-bit (x86 tolerates it): undefined behaviour in C, breaks atomics, and arm64 later.
3. Plain malloc on 64-bit: loses the static region's mark / release.

Decision (2026-11-05): option 1 (32-bit byte-identical, the oracle unaffected).

**Debug aids:** `BMX_GUARD_HEAP=1` (src/native/crt.c) puts every C-runtime block before an inaccessible page so an
overrun faults at the write; with lldb (llvm-mingw's) that gives the writer's backtrace.

**Where it stands (2026-11-05):** the scripted ride (1014 ticks) and the user's recording (6189 ticks) give the same
BMX_SIM_HASH on native32 and native64 on every tick. The 32-bit reference hashes were made again after the FPU
guard (above). Next: the user plays the 64-bit build; a release build.

#### X3 verification (2026-11-07)

Bugs found after the level fixes, all 64-bit layout bugs that the replay hashes cannot see:
- found by playing: the height meter's scale beside its bar (src/driver/wide.c bounded the 3D list by a global
  that only follows it in the original layout); a crash on respawn in ProQuest on the Thor (physics.c read the
  face query's hit list at the 32-bit offset 0x3c, 0x40 on 64-bit).
- found by AddressSanitizer (`-fsanitize=address -fsanitize-recover=address
  -fsanitize-address-use-after-return=never` on native64; trap.c leaves faults to it): the Im3D level stack
  overran four untyped placeholders (now one typed global); the sim hash read riders 2-3 past gPlayerRecs; the
  echo buffer and ovlHeightSet's argument (both the original's too). The use-after-return mode is off because
  clang miscompiles a prologue with it on mingw (movaps to a misaligned slot in RtPNGImageRead).
- found by the screenshot comparison: the hint lines' button icons were blank (gHintGlyphs['A'..'L'] reaches
  12 bytes past the table, which moved on 64-bit; now read by name, gHintGlyphLetters).
- found by reading: the texture state's colour key flag (raw +4 into RwD3DRasterExt).

**Screenshot comparison** (native32 vs native64, desktop settings at 1920x1080 windowed, `interpolate = 0`
because blending by wall-clock time makes every frame differ): `tools/xcheck.py --shots <frames>` for the user
recording (25 shots), ride.keys (19) and one freeride run into each of the 7 levels (5 each), then
`tools/bmpdiff.py` on the two folders. After the fixes: identical or within rounding (a few thousand pixels
off by a shade or two, a polygon edge 2-3 pixels apart), the physics hashes identical everywhere.

A probe of every ORIG_VAR view's size against the data it lands on (32-bit and 64-bit compiles of all headers)
found no further view that runs into data that moves on 64-bit, beyond the hint table and gPhysRiders.

### X5 part 1 (macOS arm64, 2026-11-06): builds, links, starts

A `macos` preset builds bmxnative for macOS arm64 with the host clang (nix's in `nix develop`) and SDL3 from nix,
warning-free under the same `-Wall -Wextra -Werror`. The x87 sequences run on compat/x87soft.c there (X5 part 2,
below; part of bmx_game through the src/compat glob). Whether the arm64 sim hash equals native32's is untested:
no headless run gets past the GPU device (below).

**Build** (macOS, `nix develop`, which now also provides spirv-cross and sets `SDL3_ROOT`):
```sh
cmake --preset macos && cmake --build --preset macos        # build/macos/bmxnative (rpath to nix's SDL3)
cmake --build --preset macos --target bmxnative_app         # build/macos/bmxnative.app (SDL3 inside, ad-hoc signed)
```
Without nix: `-DSDL3_DIR=<prefix>/lib/cmake/SDL3`. Run it from the game folder (the install layout: `Scene.Txt`,
`Assets\`, `bin\audio`, `bin\movie`, `save\`) as `bmxnative Scene.Txt`, like the Windows exe.

**What changed** (Windows builds unchanged by construction: every switch is `__i386__` / `__x86_64__` / `_WIN32`):
- Calling conventions: the 252 `__attribute__((regparm(n)))` are `BMX_REGPARM(n)` (compat/orig.h; the attribute on
  i386 only), `BMX_CDECL` / `BMX_STDCALL` are empty off i386 and Windows, the trap entries' `cdecl` is
  `BMX_TRAP_CDECL`. tools/gen_hooks.py, audit_common.py and void_eax_audit.py accept `BMX_REGPARM(n)` before a
  ported function. The hook build's bmxtest.dll and dinput.dll have byte-identical `.text` and `.data` (and `.reloc`)
  (`.rdata` differs only in the CodeView GUID of the debug directory).
- x87 state: the x87 control word on arm64 is x87soft's variable (`bmxX87ControlWord` / `bmxX87SetControlWord`,
  which the driver's camera mode sets: 24 bit, round down). rw/rwd3d.h copies its rounding mode into FPCR's RMode
  (compat/fpu.h) where x86 copies it into MXCSR, so the float code inside rwD3DRoundBegin / End rounds down as the
  SSE code does on x86 (`rwD3DFistp` = `rintf` in the current mode, 0x80000000 out of range, like cvtss2si).
  `DRV_FPU_GUARDED` saves and restores FPCR (in the MXCSR slot) and the control word around SDL GPU calls.
  `fnclex` (crtFpInit) is x86 only.
  `BMX_FP_OPAQUE(v)` (the negation guards of rw/*.c) is `"+x"` on x86, `"+w"` on arm64.
- `-ffp-contract=off` for every arm64 object (no fused multiply-adds); `-fms-extensions -fno-math-errno` like the
  Windows toolchains. The game objects stay `-ffreestanding -fno-builtin`.
- compat/crt.c: `rep movsb` / `rep stosb` only on x86 (byte loops elsewhere; clang vectorises them and does not
  turn them back into memcpy calls, checked in the disassembly); no MSVC `__chkstk` off Windows.
- The PNG reader's error jump: the png struct starts with the C library's `jmp_buf` on arm64 (`setjmp` /
  `longjmp`, declared `returns_twice` for the `-fno-builtin` objects); x86_64 keeps `__builtin_setjmp`.
- The naked i386 entries (`BMX_X86_ENTRY`) are `brk #1` on arm64; the import thunks of src/data/imports.c are
  `adrp / ldr / br x16` (Mach-O and ELF spellings).
- src/native: loader.c, trap.c and crt.c have POSIX halves (`#if defined(_WIN32)`, so a Linux build can share
  them): `bmxExitProcess` (ExitProcess; elsewhere stdio flushed, `_exit`); the dead-slot and trap stubs are C
  functions, one per index (no code is written at run time: macOS signs every code page), and `bmxRealProc`
  returns NULL (the Windows-comparing checks report themselves unavailable; `BMX_FILE_CHECK` is ignored); the
  fault handler is `sigaction` (SIGSEGV / BUS / ILL / FPE / TRAP) with pc, sp and lr from the signal context
  (`BMX_NO_TRAP=1` leaves faults to a debugger); bmxnative.log next to the executable (`_NSGetExecutablePath`,
  `/proc/self/exe`); `BMX_GUARD_HEAP` on mmap / mprotect with the system page size (16 KB). `__ImageBase` is
  `__dso_handle`.
- The opt-in replacements (smooth mixer, wide HUD, full draw distance; all on by default) are jumps written over
  our own functions on Windows. Elsewhere winchanMix, ngifDrawList and ngiffxAdaptQuality start with
  `BMX_REDIRECT_VOID`, a call through a slot their provider fills (`BMX_REDIRECT_SLOT` / `_SET`, compat/orig.h; no
  code at all on Windows). The log says `N redirects` instead of `N jumps`.
- LP64: `long` is 64 bits. No game struct holds one; the `(long)` printf arguments go through our own formatter,
  which reads `%ld` as 32 bits (correct for the low half of a 64-bit slot); `crtStrtoul` emulates MSVC's 32-bit
  `unsigned long` (saturates at 0xffffffff, negates in 32 bits); the game's sscanf formats are `%d` / `%X` only.
- Linking: src/data keeps the original's layout, including a table of pointers at odd addresses (`BmxUnalignedPtr`,
  a CRT switch table nothing reaches); Mach-O's chained fixups refuse those, so bmxnative links with
  `-Wl,-no_fixup_chains`.
- Shaders (X5-A): tools/shaders.sh converts the SPIR-V to MSL 2.1 with SPIRV-Cross and writes it as a third array
  (`<name>_msl`, the source text) into src/driver/shaders/*.h; `tools/shaders.sh --msl-only` redoes only the MSL
  from the SPIR-V already in the headers (no dxc; that is how the committed MSL was made). SPIRV-Cross's default
  indices are SDL3's MSL layout for these shaders (uniform buffer 0 at `[[buffer(0)]]`, texture and sampler 0,
  `[[stage_in]]` attributes by location; entry points keep their names). draw.c picks DXIL, then SPIR-V, then MSL.
- Paths (docs/PLATFORM.md P18): `bmxPathResolve` turns `\` into `/` and, for a path that does not exist as
  written, matches each component without regard to case (unit-checked with a case-sensitive `access` on the game
  folder: `bin\audio\MUSIC.ZSD` -> `bin/audio/music.zsd`, `ASSETS\tables.ZAL` -> `Assets/tables.zal`; macOS's
  default APFS is case-insensitive anyway, Linux and case-sensitive volumes need it). crt.c's fopen, the sound
  loader's CreateFileA and the AVI reader go through it. **Locations:** saves stay in the game folder's `save\`
  (as on Windows; the P18 profile redirect is still open); bmxnative.ini is read from `BMX_CONFIG`, else next to the
  executable (`Contents/Resources` in the bundle), else `~/Library/Application Support/opendmfbmx/bmxnative/`
  (SDL_GetPrefPath); bmxnative.log is written next to the executable.
- Headless message boxes: with `SDL_VIDEODRIVER=offscreen` or `dummy` a MessageBoxA is logged and answered as
  closed (SDL would still show a Cocoa alert).
- CMakeLists.txt: a non-Windows branch (find_package(SDL3), rpath, bmxwidetest / bmxhftest, the bundle target);
  bmxplatformtest and bmxmovietest stay Windows-only (they compare with Windows' DLLs).

**Verified (on the Mac):** the hook build (bmxtest.dll / dinput.dll code identical to before) and
`tests/oracle/run.sh -n 5` with the game files (3009/3009); the format check; the macos build warning-free;
bmxwidetest (849 checks) and bmxhftest (329) pass on arm64; every source of the native build compiles
(`-fsyntax-only`, `-Werror`) for i686-w64-mingw32 and x86_64-w64-mingw32 against the mingw-w64 headers. Headless
start (`SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy BMX_MUTE=1 BMX_KEYS=...` in an install-layout view of the
game folder, from build/macos and from the bundle): import slots (69 stand-ins, 68 dead, 0 real), 3061 original
addresses served, config, the three redirects, the window, then `SDL_CreateGPUDevice failed: No supported SDL_GPU
backend found!` and a clean exit. SDL's Metal backend needs a Cocoa (or UIKit) video driver, and the offscreen
driver cannot make a window without EGL, so no headless run gets past device creation: rendering, the MSL shaders
and sim-hash replays are untested.

**For the Windows machine to re-check:** `native32` and `native64` build warning-free; the gate
(tools/check.sh, including clang-tidy with `-p build/native32` on src/native, src/platform, src/driver, src/png);
the scripted ride and the user's recording give the same BMX_SIM_HASH as before on both widths (the changes there
are new `#if` branches, the emulated FPU word unused, `describe()` comparing with `uintptr_t`, `bmxExitProcess`
around ExitProcess, the draw.c shader selection rewritten); `tools/shaders.sh` (with dxc and spirv-cross) still
regenerates DXIL / SPIR-V byte-identical and the same MSL; bmxplatformtest.

**Left:** the sim hash on arm64 against native32 (needs a run past the GPU device); a run with a real window on the
Mac (the user's call: menus, a level, the MSL output against the Windows frames, audio); input on macOS (DirectInput
stand-in on SDL3; untested), the movie path; saves in a per-user folder (P18 seam); a Universal or x86_64 macOS build
(not needed for X5); notarisation / a proper Info.plist and icon; Linux (X4) can reuse the POSIX halves and the path
resolver. Observed on the way: on Windows x86_64 the loader's machine-code dead-slot and trap stubs push their
arguments i386-style while `deadSlot` / `bmxOrigTrap` read registers there, so their reports would show garbage on
native64 (only reached in a failure; the C stubs above would fix it).


### X5 part 2 (portable x87, 2026-11-06)

Decision X4-A: compat/x87.h's helpers in portable C, bit-exact with the x87 sequences they replace.

**Design.** compat/x87soft.c emulates the x87 in integer arithmetic only, so no host FPU mode, NaN convention or
libm leaks in (it also builds for the freestanding i386 targets: no 64-bit division helper, no int-to-float
conversion):
- A register is the 80-bit format (64-bit significand, 15-bit exponent). FADD / FSUB / FMUL / FSQRT compute the
  exact result and round once to the precision control (24 / 53 / 64 bits) with the rounding control, in the
  extended exponent range; stores to float / double round again (with denormals and overflow per mode): the
  original's double rounding. NaNs follow the x87 rules (SDM table 4-7: QNaN over SNaN, otherwise the larger
  significand, on a tie the positive one; invalid gives the negative QNaN indefinite). Memory operands of
  FMUL / FADD keep signalling NaNs signalling; FLD quietens them.
- FSIN / FCOS / FPTAN reduce the operand exactly by k * P/2 (k nearest) with P Intel's 66-bit pi
  (0x3.243F6A8885A308D3, the documented microcode constant; `bmxX87SoftSetReductionPi(1)` switches to pi to
  128 bits for the test), then evaluate Taylor series in 128-bit floating point (~2^-120) and round to the
  register's 64 bits, nearest. FPATAN is atan2 to 128 bits, rounded the same way. sin(r) is rounded as exact r
  plus a tail, so a tiny r on a 64-bit midpoint rounds right. |x| >= 2^63: FSIN / FCOS return x (C2), FPTAN
  pushes nothing (bmxX87TanFloat then stores the indefinite NaN, as its FSTP ST(0) / FSTP pair underflows).
- The control word: on x86 `bmxX87ControlWord()` / `bmxX87SetControlWord()` (compat/x87soft.h) are FNSTCW /
  FLDCW, so the soft path follows the real one; elsewhere a variable starting at Windows' 0x027f.
  driver/raster.c's camera mode (24 bit, round down) goes through them now (the same instructions on x86).
- Linux x64 (slice X4) starts its threads with the x87 at 64-bit precision (0x037f), Windows at 53 (0x027f);
  the hw helpers would then round differently. That build needs an FLDCW 0x027f at startup (or BMX_X87_SOFT
  with the soft variable's 0x027f).
- compat/x87.h: the asm versions are `bmxX87Hw*` (unchanged text); the public names call them when
  `BMX_HOOK && x86 && !BMX_X87_SOFT`, else `bmxX87Soft*`. There is no libm path any more (nothing needed one).
  The hook build's objects of all five call-site files disassemble identically to before.
- Cost on this Mac (M-series, -O2): FSIN ~1.3 us, MatrixMultiply ~530 ns, Rotate ~50 ns. The tables are built
  once; Fsin runs per animated bone per key change, MatrixMultiply per frame hierarchy update in the camera.

**Per helper: call sites, domain, method.**

| Helper | Call site, domain | Notes |
|---|---|---|
| SinScaled | zmathInitTables: n = (int16)(8i), all 65536 int16, k = 2pi/65536 as float; startup cw | n*k exact (40 bits); the float results do not depend on which pi reduces (equal to CR sinf) |
| CosScaled | winchan LFO table: n = 0..4095, k = pi/2048 double | n = 1024 / 3072 (cos ~ 7e-10) depend on the 66-bit pi in the double, not in the uint16 table entry |
| AsinScaled, HalfPiScaled, AtanScaled | zmathInitTables: n = -4095..4094 (asin, the ends are HalfPi), -4096..4095 (atan), step 2^-12, scale 0x10000/2pi | FPATAN's 64-bit result is multiplied before the store, so a binary64 atan is not enough |
| Fsin, Fcos | RwMatrixRotate: (double)(pi/180 as float) * angle, any float angle; the slerp (1 / fsin(angle), angle in [0, pi/2]) and animSin (angle * t, [0, pi/2]) | large angles: the 66-bit reduction matters |
| AngleOfCosine | rpAnimSlerpCacheSetup: c in [0, 1] (NaN dots end as 1) | FSQRT + FPATAN, stored to float |
| TanFloat | RpLightSetConeAngle: any float | out of range / NaN / inf as above |
| Rotate | zvec3RotateX/Y/Z: any floats, table sin / cos; whatever cw is current | the camera cw if called inside a camera update |
| MatrixMultiply | rwMatrixMultiplyX87 (the camera's RwMatrixMultiply): any floats, camera cw (24 bit, down) | loads of a row before its stores (out may alias) |

Other x87 assembly in src (out of scope here): the naked i386 entries of src/rw (`BMX_X86_ENTRY`, world.c's
`flds; call`), crtasm.S (`_ftol`, `_CIsqrt`, i386 only), the control-word save / restore in driver/driver.h
(`DRV_FPU_GUARDED`) and rw/rwd3d.h (`rwD3DRoundBegin` copies the x87 rounding into MXCSR). On arm64 the last two
need ports of their own: rwD3DRoundBegin must take the rounding from `bmxX87ControlWord()` into FPCR.

**CORE-MATH as the base?** (asked 2026-11-06.) CORE-MATH has correctly rounded binary64 sin / cos / tan / atan2;
its binary80 directory has atan2l only (no sinl / cosl / tanl), written for x86's `long double` and `__int128`.
A correctly rounded binary64 result followed by the helper's own rounding steps differs from the x87 because
(1) the x87 rounds to 64 bits first (double rounding, ~1 in 2000 near a binary64 midpoint), (2) its FSIN /
FCOS / FPTAN reduce with the 66-bit pi, (3) AsinScaled / AtanScaled multiply FPATAN's 64-bit result before
rounding. Measured with tests/platform/x87_mpfr.c (MPFR at 53 bits = CORE-MATH's results by definition):

| Helper (real domain) | CR library + the helper's rounding vs the x87 model |
|---|---|
| SinScaled (65536) | equal (CR binary32 sinf) |
| AngleOfCosine (200k c in [0, 1]) | equal (CR acos, then float) |
| CosScaled (4096) | 3 doubles differ; the uint16 LFO table equal |
| AsinScaled / AtanScaled (8190 each) | 29% of the doubles differ; the int16 tables equal |
| Fsin / Fcos, RwMatrixRotate angles | 24% differ (~6 in 100k below pi/4: double rounding; the rest: the 66-bit reduction) |
| Fsin, slerp angles | 49 in 200k differ (double rounding) |
| TanFloat (random floats) | 11% differ |

So a library would have served only SinScaled and AngleOfCosine (and the three tables' downstream integers);
one emulation for all is simpler than a library plus exception tables, and nothing was vendored.

**Verified here (macOS arm64):**
- tests/platform/x87_mpfr.c: FSIN / FCOS (both pi models) and FPATAN register results against an independent
  MPFR model of the same definition (exact reduction, 400-bit function, one rounding to 64 bits): 0 differences
  in 300000 random operands per function over exponents -80..62, the slerp range and +-3 ulp around k*pi/2 for
  k < 2000.
- tests/platform/x87_unicorn.py: Unicorn (QEMU softfloat floatx80, which honours precision and rounding
  control) runs the real sequences built for i386 (tests/platform/x87_hwshim.c): FADD / FSUB / FMUL / FSQRT with
  their FSTP m80 / float / double results under all 12 control words, operands including NaNs (signalling and
  quiet), infinities, zeros, extended denormals, the ends of the exponent range and near-cancellation, plus
  Rotate, MatrixMultiply and HalfPiScaled on random and edge floats: 0 differences in 300000 + 75000 cases.
  Unicorn's FSIN / FCOS / FPTAN / FPATAN call the host's double libm (checked), so it cannot judge those. One
  Unicorn deviation found and excluded from its inputs: its FLD m32 keeps a signalling NaN signalling (QEMU 5.0
  softfloat, fixed in later QEMU), where the SDM's x87 loads the quiet NaN (the soft path does that; only the
  Windows run confirms it).
- No Rosetta 2 on this Mac (x86_64 binaries: "Bad CPU type"), so nothing ran on x86 hardware semantics here.
- The gate: hook build, tests/oracle/run.sh -n 5 (3009 / 3009), clang-format, clang-tidy on x87soft.c;
  tests/platform/test_x87.c builds for arm64 macOS and x86_64 (-Wall -Wextra -Werror).

**Only the Windows PC can verify** (real FSIN / FCOS / FPTAN / FPATAN; the NaN-tie and out-of-range FPTAN
behaviour; whether an AMD CPU reduces with the 66-bit pi like Intel), with `bmxx87test` (tests/platform/test_x87.c:
hw against soft for every helper, finite domains exhaustively under the default and the camera control word,
continuous ones on random + edge inputs, Rotate / MatrixMultiply under all 12 control words; then the same
against the 128-bit pi to show which one the CPU uses). Commands (PowerShell, repo root):

    cmake --preset native64; cmake --build --preset native64 --target bmxx87test
    build\native64\bmxx87test.exe                       # a few minutes; "all equal" expected
    build\native64\bmxx87test.exe --all-floats fsin --threads 16    # then fcos, rotate, tan, acos (~2^32 each)
    cmake --preset native32; cmake --build --preset native32 --target bmxx87test
    build\native32\bmxx87test.exe

Each line also prints a hash of the results; on x86 the `hw` hash of a domain must equal the `soft` hash
macOS printed for the same arguments (below: `build/x87/test_x87` built from test_x87.c and x87soft.c with
clang on arm64), which shows the arm64 compile gives the same answers as the x86 one:

    $ build/x87/test_x87            (macOS arm64, default arguments: --random 2000000)
    SinScaled table                           65536 cases   soft daa39d60c4e216ba
    CosScaled LFO table                        4096 cases   soft e7a08b053b9ea4e8
    AsinScaled table                           8190 cases   soft 8162b6323c1056b7
    AtanScaled table                           8192 cases   soft 904e08f8a05a9e92
    HalfPiScaled                                  2 cases   soft a648bfcacd240665
    SinScaled table (camera cw)               65536 cases   soft 8a72a795c2d4c3fe
    CosScaled LFO table (camera cw)            4096 cases   soft 9959ee16e8786fc1
    AsinScaled table (camera cw)               8190 cases   soft ab3bbc858c4b750e
    AtanScaled table (camera cw)               8192 cases   soft 29213ad75eab4f32
    HalfPiScaled (camera cw)                      2 cases   soft 56505a8a6475e2ec
    Fsin/Fcos RwMatrixRotate                2000026 cases   soft 9f43e7c60ea0e293
    Fsin slerp/anim [0, pi/2]               2000000 cases   soft f641dea9b753dd16
    Fsin/Fcos any double                    2000023 cases   soft c654458de5ed7053
    AngleOfCosine [0, 1]                    2000000 cases   soft 41a49469a9fb70f4
    AngleOfCosine any double                 500023 cases   soft a361c6c937115e6e
    TanFloat                                2000026 cases   soft 73f6ecd96fc8a228
    Rotate/MatrixMultiply default+camera       250000 cases   soft 3781e497ed32b5b0
    Rotate/MatrixMultiply all CWs            250000 cases   soft 654e08fe59d7a76f
    HalfPi/Asin/AtanScaled any scale         250000 cases   soft 790f7749c1eb1ca2

End to end (the proof for the whole game): a soft build on x86 replays the user's recording with the same
BMX_SIM_HASH on every tick as the normal build:

    cmake --preset native64 -B build\native64-soft -DBMX_X87_SOFT=ON
    cmake --build build\native64-soft --target bmxnative
    py -I tools\xcheck.py --game-dir <game folder> --recording work\recordings\<file>.bmxr build\native64\bmxnative.exe build\native64-soft\bmxnative.exe

(the same with native32 / `-B build\native32-soft`). Both builds must be made from the same sources.


### X5 part 3 (headless null GPU, arm64 sim hash, 2026-11-06)

`BMX_GPU=null` (or `[video] backend = null` in bmxnative.ini) runs the complete game without GPU work and without
a visible window: the simulation, and the whole CPU side of drawing (the game's draw lists, RenderWare's pipelines,
instancing and lighting, Im2D / Im3D, the D3D7 driver's CPU code in src/rw/d3d*.c with its alpha sort, the
widescreen funnel, the shadow probes) up to the five cut functions of rw/rwd3d.h. That got the first sim hash on
arm64 (SDL's Metal backend needs a Cocoa window, part 1), and gives CI-style headless runs on any OS.

**What it changes** (all below the cut, src/driver and one platform function):
- `bmxGpuNull()` (src/platform/window.c), called first thing in main: decides once (env `BMX_GPU`, else the
  config) and, when null, selects SDL's `dummy` video and audio drivers unless `SDL_VIDEODRIVER` /
  `SDL_AUDIODRIVER` name one. The window is a dummy window of the same size; message boxes are logged and
  answered as closed (part 1).
- gpu.c: no device, no swapchain claim, no present mode. `drvGpuCreateTarget` returns one placeholder handle
  (non-NULL, never dereferenced) for any non-empty size; clears and presents flush (drop) the batch and return 1;
  `drvGpuPresent` still counts shown frames, so `BMX_KEYS`, `BMX_QUIT_FRAMES`, BMX_REPLAY's frame keys and the
  high frame rate mode's uncounted presents behave as windowed; screenshots (`BMX_SHOT`) are skipped (logged).
- draw.c: the cut functions keep their state (render states, texture stage states, bound texture) exactly as
  before; a draw is dropped where it would be appended to the batch (after the same checks), so no pipelines,
  samplers, buffers or texture uploads exist. `drvDrawStart` makes no shaders.
- movie.c: DrawDibDraw accepts and drops the frame after the same argument and rectangle checks (returns TRUE).
- raster.c: one log line if a camera update / clear / show ever names a raster that is not a camera or z raster
  (see below); none in the runs.
- texture.c (raster lock / unlock, the CPU copies the game reads back, image to raster) is untouched: those copies
  are made and kept as before; only their GPU upload never happens.

**Why the simulation cannot differ.** Everything above the cut runs the same code with the same inputs; only
what crosses the cut upwards could make a difference, and it is the same in both modes:
1. Return values: the five cut functions return 0 (D3D_OK) in both modes; the standards return what they did
   (begin update, clear and show 1, since a camera or z raster's target is non-NULL in both). A failed SDL call
   in windowed mode would be the only difference: it is logged, and the reference runs have none.
2. Data the game reads back: the raster plugin fields the ported render states read (colour key, texture handle,
   format info) are set by drvRasterCreate on the CPU; locks return the CPU copies (texture.c, unchanged). The
   only GPU-dependent field above the cut is the NULL-ness of `ext->texture`, through rasterTexture; for camera
   and z rasters it is non-NULL in both modes, and a texture raster is never a camera's target (that would be the
   raster.c log line).
3. Shown-frame count and window size: counted per RwRasterShowRaster in both; the dummy window has the size the
   game asked for, as the background window has under Windows (the mode is fixed by `-r640.480.16`).
4. FPU state: every SDL GPU call is inside `DRV_FPU_GUARDED`, which puts MXCSR / FPCR and the x87 control word
   back; in null mode the calls are not made. Either way the game's float state after a driver call equals the
   state before it (on native32 that is exactly what the guard was made for, X3).
5. Time: the game's limiter (main.c, hfinterp.c) waits on `bmxTimeGetTime` in both modes; a present without vsync
   only makes the loop spin instead of block. The simulation advances one fixed tick per onIdle and reads no clock
   (the one wall-clock adapter, ngiffxAdaptQuality, is replaced by `full_draw_distance`, on by default; with
   `full_draw_distance = 0` the far clip is wall-clock driven in both modes and neither is reproducible). BMX_KEYS
   and BMX_REPLAY key on shown frames, not time. The number of extra interpolated draws per tick depends on the
   display refresh (dummy: 60) and the clock, as it does between two windowed runs: HF1-HF3 showed that the
   extra draws restore all simulation state they touch (shadow probes included).
6. Window focus: the dummy driver may report focus where a background window gets none; scripted and replayed
   input ignore the focus, and the game does not pause on WM_ACTIVATE (only DirectSound's focus muting, which is
   audio).

**Bug found on the way (64-bit, all non-32-bit builds):** the goal markers (game.c `gameDrawGoalMarkers` /
`gameDrawPlaneMarker`) read face records (`PhysFace`) at the 32-bit offsets +0x2c (the prop's collision object)
and +0x30 / +0x40 / +0x50 (the corners). On 64-bit the pointers before them move the fields, so a ProQuest level
with an active face goal (type 1, 6 or 0x15: the fresh save's Vail Trails goals) read garbage as an object
pointer and crashed on the first draw (SIGSEGV in gameDrawEdgeMarker); native64 with the user's save had no
such goal active. Now typed field accesses (`face->object`, `p0..p2`); 32-bit offsets unchanged (asserted in
physics_types.h).

**The scripted ride.** tools/scripts/ride.keys is the HF4 script (docs/HIGHFPS.md "HF4 runs"; the X3 "scripted
ride" of 1014 ticks lives only on the Windows machine): ten Enters into ProQuest / Vail Trails, riding with
steering, hops and E/S/F/D tricks from frame 1500, Esc at 2700 (PAUSE), end run, RUN FINISHED, view replay; 4000
shown frames. It starts from **no save** (an empty `save\` folder) so every machine starts from the same state.
On the Mac (from a copy of the game in the install layout; `bmxnative.ini` empty, i.e. the defaults):

    cd <game copy> && rm -f save/SAVE.DAT
    BMX_GPU=null BMX_MUTE=1 BMX_NO_MOVIES=1 BMX_CONFIG=<empty.ini> BMX_QUIT_FRAMES=4000 \
      BMX_KEYS="$(cat tools/scripts/ride.keys)" BMX_SIM_HASH=ride.arm64.hash \
      build/macos/bmxnative Scene.Txt -r640.480.16

or `python3 -I tools/xcheck.py --game-dir <game copy> --keys tools/scripts/ride.keys --args=-r640.480.16
--ini <empty.ini> BMX_GPU=null build/macos/bmxnative BMX_GPU=null build/macos/bmxnative`.

**Results (macOS arm64, x87soft, 2026-11-06):**
- The ride runs headless to the end without a trap (after the goal marker fix): 4000 shown frames, **2973 ticks**,
  ~123 s (the game's own 30 Hz pacing), no "raster of type" line. The hash file is
  data/simhash/ride.arm64.hash, sha256 `b60f0dfd98ac9a6997a3e8a97fef476814584b77c245c5dcc7793218ec60aedc`.
  (2973 ticks is also what HF4 logged for this script on Windows with the user's save.)
  **Replaced 2026-11-07 by data/simhash/ride.hash** (sha256 `bafee79e...`, made on native32 and identical on
  native64 and `BMX_GPU=null`): AddressSanitizer showed that the hash read riders 2 and 3 past the end of
  gPlayerRecs (other globals on 32-bit, unrelated memory on 64-bit); it now hashes the two riders the data holds,
  so every hash file made before differs. The old arm64 file equalled the 32-bit and 64-bit Windows ones, so
  arm64 should give the new file too (unchecked).
- Two runs: byte-identical hash files (the second while a hook build loaded the machine).
- `[general] vanilla = 1` (no widescreen, no HUD anchoring, no interpolation; through xcheck `--ref-hash`):
  identical to the default-config file, so the draw-side options stay visual-only on arm64 too.
- Perturbed build (air gravity in physVelocityAir scaled by 1.0001, reverted afterwards): same tick count,
  differs from **tick 535** (the first jump) on.
- Not possible here: a windowed run on the Mac (no visible windows on this machine during work), hence no
  null-vs-windowed comparison on arm64; that is the Windows step below.

**For the Windows machine** (PowerShell, repo root; `<game>` = a copy of the install with an empty `save\`, e.g.
`native-run2` with its SAVE.DAT moved away; xcheck removes SAVE.DAT for each run and puts the folder's own back
afterwards). An empty ini makes both runs use the defaults (`ni empty.ini`):

    cmake --build --preset native64 --target bmxnative
    py -I tools\xcheck.py --game-dir <game> --keys tools\scripts\ride.keys --args=-r640.480.16 --ini empty.ini `
       --ref-hash data\simhash\ride.hash --out xcheck-null `
       build\native64\bmxnative.exe BMX_GPU=null build\native64\bmxnative.exe

That runs native64 windowed (in the background, never focused) as `run0` and headless as `run1`, and compares
both with the arm64 file: `run0` vs `ref` is arm64 against x86_64 (X5's goal), `run1` vs `ref` the null mode.
Expected: `ref`, `run0` and `run1` identical (2973 ticks). If only `run1` differs from `run0`, the null mode
leaks (the first differing tick names when); if `run0` differs from `ref` but not from `run1`, it is an arm64 /
x87soft difference (then the same against native32 and a `BMX_X87_SOFT=ON` x86 build narrows it down). The same
with `build\native32\bmxnative.exe` in place of native64 checks the 32-bit build.


### X4 (Linux x64, 2026-11-06): builds, runs headless, sim hash equal (under Rosetta)

A `linux` preset (cmake/x86_64-linux.cmake: the host's clang, SSE2, no FMA contraction) builds bmxnative for
x86_64 Linux on the POSIX halves and the path resolver of X5 part 1; SDL3 comes through its CMake config. Linux
x86_64 keeps the x87 inline assembly like Windows x64 (compat/x87.h's hardware helpers, not x87soft). Built and run
in an OrbStack Fedora 44 amd64 machine on this Mac, i.e. x86_64 under Rosetta 2 (below); real hardware (the user's
nixbox, the Steam Deck) is still to do.

**Build** (Linux, repo root):
```sh
nix develop                                         # clang (wrapped), SDL3 (SDL3_ROOT), patchelf, cmake, ninja
cmake --preset linux && cmake --build --preset linux             # build/linux/bmxnative (rpath to nix's SDL3)
cmake --build --preset linux --target bmxnative_tarball          # build/linux/opendmfbmx-linux-x86_64{,.tar.gz}
```
Without nix: clang, cmake >= 3.28, ninja, python3 and the distribution's SDL3 development package (or SDL3 built
from source, `-DSDL3_DIR=<prefix>/lib/cmake/SDL3`); `OPENDMFBMX_LINUX_CLANG=clang-21` names another clang. GCC is
not supported (the sources use clang's diagnostic pragmas and warning names under `-Werror`). Run it from the game
folder in the install layout (`Scene.Txt`, `Assets/`, `bin/audio`, `bin/movie`, `save/`), any case of the names:
`cd <game> && <build>/bmxnative Scene.Txt`.

**What changed** (Windows builds unchanged: every change is `__linux__`, `!_WIN32` or a no-op there):
- x87 state: Linux starts a process with the x87 control word at 0x037f (64-bit precision), Windows at 0x027f
  (53-bit); the hardware x87 helpers would round differently. `bmxFpuWindowsDefaults()` (compat/fpu.h) loads
  0x027f and MXCSR 0x1f80 (Windows' and Linux's default anyway: nearest, masked, no FTZ / DAZ) first thing in
  main and in the winmm timer thread; other threads (SDL's audio, the music worker) copy the creating thread's state
  (Linux's clone does) and are made outside camera mode. A no-op on Windows and arm64 (x87soft's variable starts at
  0x027f). Also on an x86_64 macOS build, should one ever be made.
- trap.c defines `_GNU_SOURCE` on Linux: glibc under `-std=c17` hides `sigaction`, `siginfo_t`, `REG_RIP`,
  `readlink` and `strsignal` (the only file that failed the Linux compile).
- CMakeLists.txt: `-ffp-contract=off` for every non-Windows build (x86_64 has no FMA without `-mfma`; this guards
  against an `-march` in CFLAGS); bmxwidetest / bmxhftest link libm (its own library on Linux; the link found it);
  the `bmxnative_tarball` target (tools/linux_package.sh); bmxx87test on Linux x86_64 too (hardware against soft).
- replay.c reads the recording's starting SAVE.DAT through `bmxPathResolve` (a `Save/` folder on a case-sensitive
  file system).
- flake.nix: on Linux the default shell adds a wrapped clang (its stdenv compiler is gcc) and patchelf.
- Reused unchanged from X5 part 1: the redirects instead of runtime jumps (`BMX_REDIRECTS` is 1 off Windows), the
  x86_64 import thunks (RIP-relative `jmp`, ELF spelling), the fault handler (`REG_RIP` / `REG_RSP` from the
  signal context), `/proc/self/exe` for the log, `__dso_handle` as the instance, LP64 handling.

**Locations** (the macOS policy): bmxnative.ini from `BMX_CONFIG`, else next to the executable, else SDL's pref path
(`$XDG_DATA_HOME/opendmfbmx/bmxnative/`, by default `~/.local/share/opendmfbmx/bmxnative/`); saves in the game
folder's `save/` (the working directory's, as on Windows); bmxnative.log next to the executable. The game's paths
(`Assets\Tables.ZAL`, `bin\audio\MUSIC.ZSD`) are resolved case-insensitively per component (`bmxPathResolve`),
which Linux needs: its file systems are case-sensitive.

**Packaging.** The tarball holds `bmxnative` and `lib/libSDL3.so.0`, the executable's rpath `$ORIGIN/lib`; the
user unpacks it anywhere and runs it with the game folder as the working directory. A build in `nix develop`
links against the nix store's glibc and loader, so its tarball runs only where nix is (NixOS). For other
distributions and the Steam Deck, build outside nix on an old enough glibc: the Steam Runtime 3 "sniper" SDK
container (Debian 11, glibc 2.31; SDL3 built from source there, it has only SDL2), then the same target. An
AppImage (not made): `linuxdeploy --appdir AppDir -e build/linux/bmxnative -l <libSDL3.so.0> -d bmxnative.desktop
-i <icon> --output appimage`; SDL3 opens X11 / Wayland / PipeWire / Vulkan from the system at run time, so nothing
else goes in; it runs with the caller's working directory, i.e. `cd <game> && opendmfbmx.AppImage Scene.Txt`. On
the Steam Deck: add it as a non-Steam game with the game folder as "Start in" and `Scene.Txt` as the launch
option; Steam Input presents the Deck's controls as an Xbox pad, which the DirectInput stand-in reads.

**Verified on the Mac** (nix's binary cache substitutes x86_64-linux packages without a Linux builder: glibc 2.44
headers and libraries, linux-headers 7.2, SDL3 3.4.18, gcc 16's crtbegin / libgcc):
- Every source of the native build (170 units of the macos compile database) compiles `-fsyntax-only -Werror`
  for `x86_64-unknown-linux-gnu` against those headers (before the trap.c fix: trap.c failed, nothing else).
- A full cross build of the `linux` configuration (clang 21 `--target=x86_64-unknown-linux-gnu`, ld.lld, a
  scratch toolchain file with `-nostdlibinc -isystem <glibc>/include -isystem <linux-headers>/include
  --gcc-toolchain=<gcc>` and `-B/-L <glibc>/lib`, `-DSDL3_DIR=<sdl3-dev>/lib/cmake/SDL3`) compiles warning-free
  and links bmxnative, bmxwidetest and bmxhftest: a PIE, `NEEDED` libSDL3.so.0 / libm.so.6 / libc.so.6, no
  `TEXTREL`, non-executable stack; main and the timer thread start with the `fldcw 0x27f`.
- The same 170 units also for i686-w64-mingw32 and x86_64-w64-mingw32 (mingw-w64 14 headers), `-Werror`.

**Run on Linux x86_64 under Rosetta** (OrbStack `bmx-linux`: Fedora 44 amd64, clang 22.1.8, SDL3-devel 3.4.18;
the x87 is Rosetta's emulation, not hardware):
- `cmake --preset linux && cmake --build --preset linux`: warning-free (`-Werror`), links.
- bmxhftest (329 checks) and bmxwidetest (849) pass.
- **The headless ride** (`BMX_GPU=null`, dummy video / audio, `BMX_MUTE=1`, empty ini, no save, the X5 part 3
  command with build/linux/bmxnative): 4000 shown frames, 2973 ticks, no trap, ~124 s; the BMX_SIM_HASH file is
  **byte-identical to data/simhash/ride.arm64.hash**. So Linux x86_64 with the x87 instructions gives arm64 /
  x87soft's simulation tick for tick.
- bmxx87test: Rotate / MatrixMultiply under all 12 control words, the SinScaled / AsinScaled tables,
  AngleOfCosine equal; Rosetta's FSIN / FCOS / FPTAN / FPATAN differ from the model in the last bits (e.g. 2 of
  8192 AtanScaled doubles, 1 CosScaled under the camera cw, ~1% of TanFloat, more for huge arguments), and also
  from the 128-bit-pi model, i.e. Rosetta does not reproduce the x87's transcendental results. They did not reach
  the ride's hash. The soft hashes equal the arm64 ones printed in X5 part 2 (same answers on both
  architectures). Real x86 hardware still has to show `all equal` (Windows PC, nixbox).

**Game folder layout:** the scenes name `ASSETS\<LEVEL>\...`; a CD install that keeps the levels as top-level
`Level_-_XX` folders needs `Assets/<Level>` links to them (and `bin/audio` -> `Audio`, `bin/movie` -> `Movies`),
otherwise the level load fails in zsceneOpen (2026-11-06 on nixbox). With that, nixbox's real x87 and a
`BMX_X87_SOFT=ON` build both give data/simhash/ride.arm64.hash exactly (2973 ticks).

**For the nixbox** (x86_64 NixOS, real hardware; from a copy of the game in the install layout with an empty `save/` and an empty
ini; headless, no window or sound):
```sh
cmake --preset linux && cmake --build --preset linux
build/linux/bmxhftest && build/linux/bmxwidetest && build/linux/bmxx87test     # x87: "all equal" expected
python3 -I tools/xcheck.py --game-dir <game copy> --keys tools/scripts/ride.keys --args=-r640.480.16 \
  --ini <empty.ini> --ref-hash data/simhash/ride.hash BMX_GPU=null build/linux/bmxnative
```
Expected: the run and `ref` identical (2973 ticks; Linux x86_64 with the hardware x87 against arm64 with
x87soft). If not, the same with a `-DBMX_X87_SOFT=ON` build (`cmake --preset linux -B build/linux-soft
-DBMX_X87_SOFT=ON`) tells the x87 from the rest. Then a windowed run (Vulkan, SPIR-V shaders), sound, the pad,
movies, a save, and the tarball on another distribution / the Steam Deck.

**Left:** the sim hash and bmxx87test on real x86 hardware; a windowed run: the Vulkan path, audio (PipeWire / PulseAudio through
SDL), input, the Steam Deck; a release build in the sniper container; an AppImage if wanted; GCC (not planned).


### X6 step 1 (Android arm64, 2026-11-06): libmain.so and an APK build, not run yet

An `android` preset builds bmxnative for Android arm64-v8a as `libmain.so` (SDL3's Android convention: SDL's Java
activity loads `libSDL3.so` and `libmain.so` and calls `SDL_main` on its own thread), warning-free under the same
`-Wall -Wextra -Werror`, with the NDK r29 from nixpkgs. `android/` is a Gradle project after SDL3's
android-project template that packs both libraries into an APK; `android/build.sh` builds it. Nothing has run on
a device or an emulator yet (no Android runtime on the Mac; the Thor is the user's step, below).

**Build** (macOS arm64 or x86_64 Linux, repo root):
```sh
nix develop .#android          # NDK r29, SDK platform 35, build-tools 35, gradle 8.14, JDK 17, SDL3_SRC
android/build.sh               # SDL3 (once), libmain.so, then the APK:
                               #   android/app/build/outputs/apk/debug/app-debug.apk
```
or the parts: `tools/android_sdl3.sh` (SDL3 3.4.16 from nixpkgs' `sdl3.src`, the desktop builds' version, built
with the NDK's CMake toolchain as a shared library into build/android-sdl3/prefix; SDL's one C++ file, hidapi, takes
the static C++ runtime), then `cmake --preset android && cmake --build --preset android` (build/android/libmain.so).
The shell's package set allows unfree packages and accepts the Android SDK licence for itself only (flake.nix; no
global nix config). The first `gradle` run fetches the Android Gradle plugin 8.7.3 (SDL's template's) from
Google's Maven repository. `android/build.sh release` gives a release APK signed with the debug key (no release key
yet).

On Windows (Git Bash, no nix; checked 2026-11-07) the same script runs with the tools set by hand: `ANDROID_HOME`
(an SDK with platforms;android-35, build-tools;35.0.0, platform-tools), `ANDROID_NDK_ROOT` (ndk;29.0.14206865,
r29), `SDL3_SRC` (the SDL3-3.4.16 source release), `JAVA_HOME` (JDK 21 works for AGP 8.7.3) and gradle 8.14's
`bin` on `PATH`; the scripts turn MSYS `/c/...` paths into `C:/...` (cygpath) for CMake and Gradle. Calling
`cmake --preset android` directly needs `ANDROID_NDK_ROOT` in the `C:/...` form.

**What changed** (other builds unchanged: everything is `__ANDROID__` or `if(ANDROID)`, plus `.hidden` on the arm64
ELF import thunks):
- cmake/aarch64-android.cmake: the NDK's android.toolchain.cmake (from `ANDROID_NDK_ROOT`) with arm64-v8a, API 26
  (Android 8; SDL3 needs 21, its Vulkan GPU backend 24+; the Thor runs Android 13) and no C++ runtime. Android is
  `__linux__` and `__aarch64__`, so the POSIX halves of X5 part 1 / X4 apply as they are: x87soft (no x87 on arm64),
  `-ffp-contract=off` (CMakeLists.txt, every non-Windows build; checked in the compile database: all 171 units),
  FPCR rounding for the camera mode, the redirects, the sigaction fault handler with pc / sp / lr from bionic's
  arm64 `ucontext_t`, the case-insensitive path resolver. Shaders: SDL GPU's Vulkan backend takes the SPIR-V
  (draw.c's DXIL / SPIR-V / MSL choice), no MSL needed.
- CMakeLists.txt: under `ANDROID` bmxnative is `add_library(SHARED)` named `main`, linked to SDL3, libm and liblog
  with `--no-undefined`. All C symbols default to hidden there (`CMAKE_C_VISIBILITY_PRESET`), so the library's own
  symbols bind locally: the arm64 import thunks reach `bmxImports` with `adrp`, which a preemptible symbol in a
  shared object does not allow; `SDL_main` stays exported (SDL_main.h declares it visible). The thunks themselves
  are `.hidden` on ELF now (data/data.h), so the dynamic table exports only `SDL_main` (and compiler-rt's weak
  `__emutls_get_address`, TLS below API 29).
- main.c includes `<SDL3/SDL_main.h>` on Android (main becomes SDL_main) and first calls `bmxAndroidPrepare`
  (src/native/android.c): the app's external files folder (`SDL_GetAndroidExternalStoragePath`) is the place for
  everything; its `game/` is made the working directory (no game folder: an SDL message box with the adb command,
  then SDL_main returns 1 and SDL ends the activity); `BMX_CONFIG` points at its bmxnative.ini unless set.
- trap.c: bmxnative.log in that folder (an app's /proc/self/exe is app_process); every bmxLog line also goes to
  logcat (`__android_log_vprint`, tag `opendmfbmx`; stderr goes nowhere in an app). The fault handler is installed
  with `sigaction` as on Linux; ART's libsigchain sits in front of it and keeps its own SIGSEGV uses (Java null
  checks) to itself, so only native faults reach the report.
- android/: Gradle project (settings / build.gradle, AGP 8.7.3; app/build.gradle: `org.opendmfbmx.app`, minSdk 26,
  target / compile 35, build-tools 35.0.0, arm64-v8a only, no native build: app/libs/arm64-v8a gets the two .so
  from android/build.sh; SDL's Java sources come from `$SDL3_SRC/android-project/app/src/main/java`, so they always
  match the SDL3 the library was built from; debug symbols kept in the APK); `BmxActivity` (SDLActivity with
  libraries `SDL3`, `main` and the argument `Scene.Txt`); AndroidManifest.xml after SDL's: sensorLandscape,
  `android.hardware.vulkan.version` 1.0 required, gamepad / touchscreen / leanback not required (LEANBACK_LAUNCHER
  category too), `isGame`, VIBRATE, no storage permission (none needed for the app's own external folder). No icon
  yet.
- flake.nix: the `android` shell (aarch64-darwin and x86_64-linux; nixpkgs has no Android SDK for aarch64-linux).

**Verified here** (macOS arm64):
- `cmake --build --preset android`: warning-free, links. `file`: `ELF 64-bit LSB shared object, ARM aarch64`;
  `llvm-nm -D --defined-only`: `SDL_main` (and `__emutls_get_address`); `NEEDED` libSDL3.so, libm, liblog, libdl,
  libc; no `TEXTREL`; LOAD segments 16 KB aligned (Android 15's page size; NDK r29 default). The SDL JNI entry points
  are in libSDL3.so (SDL's Java side calls `nativeRunMain`, which finds `SDL_main` in libmain.so by name).
- Undefined symbols of libmain.so: 260, all resolved at link time (`--no-undefined`): 179 SDL_*, the rest bionic
  libc / libm (incl. the `_FORTIFY_SOURCE` `__*_chk` ones; the newest is `LIBC_N`, API 24) and liblog's
  `__android_log_print` / `__android_log_vprint`. None outside the NDK and SDL3.
- `android/build.sh`: app-debug.apk (19 MB) with lib/arm64-v8a/libSDL3.so and libmain.so stored uncompressed and
  16 KB aligned (`zipalign -c -P 16 4`: OK); `aapt2 dump badging`: package org.opendmfbmx.app, minSdk 26, target 35,
  launchable `org.opendmfbmx.app.BmxActivity`, native-code arm64-v8a.
- The other builds: the hook build and `tests/oracle/run.sh -n 5` (3009/3009), clang-format on all sources, the
  macos build warning-free. The `linux` preset is unchanged (its configure needs a Linux host; the CMake changes
  are `if(ANDROID)` only, and the one source change outside `__ANDROID__`, `.hidden` on the arm64 ELF thunks, is
  compiled by this Android build).

**On the Thor** (the user's step; developer options with USB debugging on, `adb` from the android shell's
platform-tools or any SDK). `<game>` is the game folder in the install layout (Scene.Txt, Assets/, bin/audio,
bin/movie, save/; an empty save/ starts fresh):

    adb install -r android/app/build/outputs/apk/debug/app-debug.apk
    adb shell am start -n org.opendmfbmx.app/.BmxActivity     # once: creates the app's folder; shows "No game folder"
    adb push <game>/. /sdcard/Android/data/org.opendmfbmx.app/files/game/
    adb push bmxnative.ini /sdcard/Android/data/org.opendmfbmx.app/files/      # optional
    adb logcat -s opendmfbmx SDL SDL/APP AndroidRuntime DEBUG                  # in a second terminal
    adb shell am start -n org.opendmfbmx.app/.BmxActivity

(`/sdcard/Android/data/org.opendmfbmx.app/files` is `/storage/emulated/0/Android/data/...`; adb can write there
without root; the log is also at `.../files/bmxnative.log`, `adb pull` it.) What to test, in this order:
1. It starts: logcat shows the import slots / config / redirects lines as on the desktop and no trap; a fault
   report names the address (logcat and bmxnative.log).
2. The GPU: `SDL_CreateGPUDevice` on Vulkan (the Adreno driver), the menu draws (SPIR-V shaders), full screen,
   landscape. If not: `[video] backend = null` in bmxnative.ini (the null GPU of X5 part 3) runs it without
   drawing, to separate the GPU from the rest.
3. The pad: the built-in controls through the DirectInput stand-in (SDL gamepad); whether B / Back ends the
   activity (SDL's back-button handling; `SDL_ANDROID_TRAP_BACK_BUTTON` if so); the menus by pad only.
4. Sound (SDL on AAudio), a movie (or `BMX_NO_MOVIES`), a level, a save (game/save/SAVE.DAT appears), quitting
   (main's `_exit` ends the process; Android shows nothing), pausing via the home button and coming back.
5. The sim hash: the headless ride of X5 part 3 with `[video] backend = null` and the script in the environment is
   not possible from the launcher; an `adb shell` run of the same libmain.so through a small executable would be
   the next tool if the Thor's hash is wanted (expected equal to data/simhash/ride.hash, same code as macOS).

**Left** (X6 steps 2+): the device run above; game files through the system file picker (SAF) instead of adb; the
1920x1080 config defaults for the Thor (widescreen, hud, interpolate at 60 / 120 Hz); suspend / resume (SDL's
`SDL_EVENT_WILL_ENTER_BACKGROUND`: the GPU swapchain and audio); an icon and a release key; a sim-hash run on
the device.


## Not in scope

- Running without the user's game files: the assets, movies and music stay theirs.
- 32-bit ARM builds.
