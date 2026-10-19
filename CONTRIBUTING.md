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
5. `_ftol` (`CALL 0x004a4a80`) is a truncating `(int)` cast of the current `st0`. A narrower result such as
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
