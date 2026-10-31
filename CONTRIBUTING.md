# Porting guide

This file is the contract for everyone porting code, human or agent. Read it fully before your first
slice. `PLAN.md` explains the overall strategy.

## Ground rules

- **Target binary:** `NgBMX.exe` with sha256 `97aa9077501f…` (see `tools/known_exe.txt`). Never commit
  it, its assets, or anything generated from it (`work/` is gitignored).
- **Behaviour is the spec.** A port is correct when the differential test shows it matches the original
  bit for bit. "Cleaner" behaviour is a bug. Original bugs are preserved. If a bug is worth fixing, it
  gets a `BMX_FIX_*` option that is off by default, plus a comment.
- **One function, one `BMX_ADDR`.** Every ported function is preceded by `BMX_ADDR(0x00xxxxxx)` on its
  own line. The hook table, progress report and duplicate checks are generated from these.

## Where things go

- Mirror the original tree. `C:\NgEngine\Zge\zquat.c` becomes `src/zge/zquat.c`, and
  `C:\zaxis\Bmx\src\com\action\physics.c` becomes `src/bmx/com/action/physics.c`. Function-to-file
  assignment comes from `data/functions.csv`. Where it says `affinity` or `edge`, the guess may be
  wrong, so move functions if the code clearly belongs elsewhere and update the CSV.
- Shared types go in the header of the module that owns them. Engine maths types live in `src/zge/zmath.h`.
- Tests go in `tests/cases/test_<module>.c`, registered with `DT_CASES(...)`.

## Calling original code

- Unported callees: `ORIG_FN(0xaddr, ret, name, (params));` in the owning module's header. When you
  port that function, delete the `ORIG_FN` line.
- Globals still in the exe: `#define gFoo ORIG_VAR(0xaddr, type, gFoo)`.
- Constants from `.rdata` become literals. Print them with `tools/rdata.py NgBMX.exe 0x54d048 …` and use
  hex-float literals (`0x1.921fb6p-14f`) whenever the value isn't a short exact decimal.
- Asserts: `zdebugFail("msg", <module>File, <original line>, 0)`, with the line number taken from the
  disassembly (the `PUSH 0x287` before `CALL 0x0047ed10`). The message text must match the original.
- The CRT goes through `compat/crt.h` only.

## Floating point (read this twice)

The original was compiled by MSVC6 for the x87 FPU. Windows runs the x87 at **53-bit precision**, so a
value held in an x87 register behaves exactly like a C `double`. A value the original stores to memory
(`FST`/`FSTP float ptr`) is rounded to `float`. Port FP code like this:

1. **Translate FP code from the disassembly, not from Ghidra's pseudo-C.** The pseudo-C regroups sums and
   hides which temporaries were spilled. Use it for control flow and naming only.
2. Every intermediate that stays on the x87 stack is a `double` in C. Every `FSTP/FST float ptr` to a
   local, a struct or an argument slot is a `float`. `FST` keeps the unrounded value in the register:
   if later code uses `ST(i)` rather than reloading, that later code uses the `double`.
3. Keep the exact operand grouping of every add and subtract. Multiplication and addition operands may be
   swapped (that's exact), but `(a+b)+c` and `a+(b+c)` differ.
4. Functions that return a value in `st0` are declared `double` (callers may use it unrounded). If the
   callee pushed a `float` from memory, the value is exact either way.
5. `_ftol` (`CALL 0x004a4a80`) truncates `st0` to a **64-bit** integer and returns the low 32 bits (`EDX:EAX`).
   In range, that's a plain `(int)` cast. Out of range or NaN it differs: NaN and huge values give 0, and
   values beyond int32 wrap. Use `bmxFtol(double)` from `compat/crt.h` whenever the value isn't provably in
   range. A narrower result such as `MOV SI,AX` is a cast to `int16_t`.
   `MOV SI,AX` is a cast to `int16_t`.
6. `FCOMP` and `FNSTSW`, then `TEST AH,0x41 / JZ`, mean "jump if `st0 > operand`". `TEST AH,1` means
   `st0 < operand`. Compare the double in the register, not the rounded float, unless the original
   reloads it.
7. Never use `-ffast-math`, FMA contraction or `float` intermediates "for speed". The build enforces
   SSE2 with strict IEEE semantics on every target.

## Tests (differential)

- Every ported function gets at least one `DT_CASES` entry. Cover:
  - randomised inputs with `dt_float_edgy` / `dt_range` (≥200 iterations)
  - aliasing (`out == in`) where pointers could overlap
  - every assert path
  - every branch you can reach from arguments or globals
- The default tolerance is **0 ULP**. Exact match is the standard. Raising `ulps` needs a comment saying
  why. "It was close" isn't a reason. Find the spill or regrouping you got wrong.
- Return-kind flags (`DT_RET_INT16`, `DT_RET_FLOAT`, …) must match the original's return width. Many
  functions only set `AX` or `AL`.
- Functions that need engine state set it up in `setup()` through `ORIG_VAR` globals or by calling original
  initialisers. Process-wide initialisers belong in `dt_init[]` in `tests/cases/dt.c`.
- Callees that need a loaded level, a renderer or other missing state: stub them for the iteration with
  `dt_stub(orig, ourPortOrNULL, stub)` in `setup()`. The harness patches both the original entry and our
  port, so the two sides stay comparable. Don't patch code by hand.
- Stub logs are compared byte for byte (tests compile at `-O0`, so nothing is optimised away). Pass logged
  words through `dt_canon()`, which makes emulated-stack pointers comparable. Mask narrow arguments to the
  width the callee reads (16-bit handles often carry garbage in the upper half). Never log uninitialised
  bytes.
- Fixtures must be well-formed where the code relies on it: terminate every table the code scans for a
  terminator, keep indices in range and seed (`dt_seed`) before drawing random values. A scan that runs
  past a fixture reads memory the stubs are changing, and the two sides diverge at random.
- Run your module's cases with `-n 2000` before declaring done. Rare paths (and rare fixture bugs) often
  only show up after a few hundred iterations.
- The baseline is a real process: the exe's CRT startup has run up to `WinMain` under a small
  deterministic Win32 layer (`tests/oracle/win32.py`). That layer covers heap, time, registry ("not
  found") and files, and anything else traps loudly.
- File code can and should be tested on **real game files**. Paths are relative to the install root
  (`Assets\Tables.ZAL`, `bin\audio\...`) and resolved case-insensitively. Writes stay in memory and
  are compared like memory. Mark such cases `DT_NEEDS_ASSETS`, because CI has the exe but no assets.

## Style

- C17, `clang-format` (repo config) on every file, and `clang-tidy` must be clean.
- Names follow Z-Axis conventions where the original leaks them (`zsndSeq…`, `kzsndSeqMaxTracks`).
  Otherwise use `<module><Verb><Noun>` in lowerCamel (`zquatToMatrix`), `PascalCase` types with a module
  prefix (`ZQuat`), `gName` for globals and `kName` for constants.
- A comment says what the function does and anything surprising (in-place mutation, original quirks).
  Don't narrate the code.

## Workflow per slice

1. `tools/slice.py` produces the work packet: bundles, the current headers and the CSV rows.
2. Port all functions in the slice, leaves first.
3. `cmake --build --preset hook` and `tests/oracle/run.sh -k <module>` until everything passes exactly.
4. `clang-format -i`, then `clang-tidy`, then commit:
   `port(<module>): <n> functions (<addr range>)`.

## Cleanup conventions

Ported code is cleaned up module by module (pilot: `zrwmodel*.c`, 2026-10-31). A cleanup changes how the code
reads, never what it does: the object code may differ only where the compiler picks other instructions for the
same memory effects.

**What to change**

- Raw byte offsets (`*(uint16_t *)((uint8_t *)x + 0x0c)`, `((void **)clump)[1]`, `#define fooRefs(x) ...`)
  become struct fields. Use the existing RenderWare structs (`RpAtomic`, `RpClump`, `RwFrame`, `RpGeometry` in
  `rw/*.h`) and accessors (`RpClumpGetFrame`, `RpAtomicGetFrame`) before inventing new ones; the scene object is
  `ZObj` (`zge/zobj.h`). Grep the whole of `src/` and `tests/` for other users of the same offsets first.
- Every field a struct claims gets a `_Static_assert(offsetof(T, field) == 0x.., ...)` next to the struct, plus
  `sizeof` when the size is known (pool record sizes, memset/zmemCopy sizes). Then replace size literals with
  `sizeof` (`zmemCopy(dst, src, sizeof *dst)`), it is the same constant.
- Two structs at the same address with different names are one struct: merge them (`ZrwPartNode` and
  `ZrwModelPart`). Arrays hidden behind stride helpers become arrays of a record (`lodOf`/`partSetOf` at
  +0x50/+0x54, stride 12, were `ZrwModelLod lods[4]`).
- Name a `padNN` only when the code makes its meaning clear (who writes it, who reads it, what the value is). A
  field read by one function and written by another with a plain value stays `padNN`. The w slot after a `ZVec3`
  stays `padNN`.
- Names follow the code, not the guess the port started with: a field that holds the atomic's original render
  callback is `atomicRender`, not `atomicInfo`; a clump loaded from the MCOL chunk is `collisionClump`. Rename
  our own function names when they are wrong (`zrwModelSetActive` toggles collision: `zrwModelSetCollision`);
  update the `DT_CASES` name string with the symbol. Z-Axis names that leak from asserts stay.
- Magic numbers become named constants when the name is certain: chunk ids (`kChunkMlod = 0x444f4c4d /* "MLOD" */`),
  option bits (`kZrwModelSkinned`), RenderWare flags (`rpATOMICCOLLISIONTEST`, `rpGEOMETRYLIGHT`). Add missing RW
  SDK names to the `rw/*.h` enums rather than defining local ones.
- Comments say why or what is surprising (original quirks, inlined steps, unchecked indices), not what the next
  line does. Fix comments that turn out wrong.
- `TODO(file)`: move a function only when the right home is clear (an assert's file name, shared globals, the
  callers). Keep its `BMX_ADDR`, change its row in `data/functions.csv` (file column, confidence `manual`), and
  run `py -I tools/progress.py --check`. Move its prototype and the types it needs to the new module's header.

**What not to touch**

- Hooks, `BMX_ADDR`, `ORIG_VAR`, `ORIG_FN`, `BMX_FNPTR`, and the signatures of `BMX_ADDR` functions (parameter
  and return types stay, even `void *`; cast to a typed local inside). Renaming parameters is fine.
- Order and grouping of FP operations, float/double conversions (`(double)` casts, stored vs register values),
  integer widths and signedness, the order of calls and of stores around calls, and which memory is read and
  written. Keep duplicate loads the original does (`RpClumpGetFrame(clump)` twice around a call, not cached).
- Inlined copies of other functions (the root step of a frame callback, a dump routine): don't replace them by a
  call, the tests stub the callee and would see an extra call. Say in a comment that it is inlined.
- Unchecked indices that can go out of range in the original (`edge` up to 255): keep the address computation;
  a small helper with a comment is fine (`colCorner`).
- Patterns that comments mark as deliberate (negations kept opaque, `bitsToFloat`, `__builtin_fabs` compares).
- Anything you are not sure is equivalent. Skip it and note it.

**How to verify**

1. Before editing, build the hook preset and save the per-function disassembly of the module's objects:
   `python3 -I tools/fndis.py work/fn-before src/<path>.c ...` (every file you will touch, including the files
   functions move to). After the cleanup, rebuild, run it again into `work/fn-after` and
   `diff -r work/fn-before/fn work/fn-after/fn`. Expect identical code for almost every function; read every
   difference and make sure it is the same computation (other addressing of the same field, constant stores
   merged into a wider store, renamed relocations). This catches more than the tests do.
2. `cmake --build --preset hook`, then the oracle cases of every touched function and of every module whose
   tests use the changed types (`tests/oracle/run.sh -k zrwModel -k ... -j 4`), with the tests' fixtures updated
   for renamed fields.
3. `clang-format -i` and `clang-tidy --quiet -p build/hook` on the touched files; the native32 build when a header
   it compiles changed; `py -I tools/progress.py --check` when functions moved.
