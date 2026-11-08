# Handoff: how this port is run

Read this first in a new Claude Code session or on a new machine. It holds what isn't obvious from the code.
Then `docs/RELEASE-PLAN.md` (what's left before the release, and the decisions), `docs/PORTING.md` (the native
port step by step, X0-X6), `PLAN.md` (the original strategy and decisions) and `CONTRIBUTING.md` (the rules).
Dated session logs that used to be here are in the git history (before the release cleanup, 2026-11-08).

## State

- All code is ported. The program is the native exe `OpenFreestyleBMX` (64-bit Windows is the release build;
  Android arm64, Linux and macOS build too); it reads nothing from `NgBMX.exe`. The hook build (`hook` preset) and
  the differential tests against the original exe in an emulator stay as the proof of faithfulness: the gate
  (`tools/check.sh`) runs them, about 2,960 cases.
- `main` on GitHub, `https://github.com/DenzoNL/open-freestyle-bmx` (public), is the truth since 2026-11-08.
  Commit and push to main directly, after the gate. The history was rewritten to the GitHub noreply author
  address when it moved, so older clones do not share hashes with it.
- The original exe lives **only** on the orphan branch `oracle`. Never merge it into main. Never push
  `oracle` to a public host: it stays on the owner's private Forgejo server (git remote `forgejo`), and the
  `origin` remote pushes main only. Anyone else uses the exe from their own copy of the game.
- No CI: run `tools/check.sh` locally before pushing (the differential cases need the exe in `work/`; without
  the game folder the asset cases are skipped).

## Machine setup

The toolchain is Nix. Everything (clang/lld cross compiler, CMake, Ninja, Python with unicorn/pefile/
capstone, clang-tools, Ghidra) comes from `flake.nix`.

- **macOS (arm64) / Linux:** `nix develop`. Ghidra is included on x86_64.
- **Windows, native (no Nix):** install the same tools yourself, then run the scripts from **Git Bash**:
  ```powershell
  winget install LLVM.LLVM Kitware.CMake Ninja-build.Ninja Python.Python.3.12 Git.Git EclipseAdoptium.Temurin.21.JDK
  py -m pip install unicorn pefile capstone pytest
  ```
  Ghidra comes from github.com/NationalSecurityAgency/ghidra/releases (unzip it, put `support\` on
  `PATH` so `analyzeHeadless` resolves). Without Nix, the CMake toolchain uses `clang` and `lld-link` from
  `PATH` (LLVM's installer adds them). The generated `kernel32.lib` needs `llvm-dlltool`, which ships with
  LLVM. In Git Bash, if `python3` is not on `PATH`, run the scripts with `PYTHON=py` (e.g. `PYTHON=py tools/check.sh`).
- **Windows via WSL2:** Ubuntu in WSL2 plus multi-user Nix with flakes, then `nix develop` as on Linux.

```sh
git clone https://github.com/DenzoNL/open-freestyle-bmx.git && cd open-freestyle-bmx
mkdir -p work && cp <your game folder>/NgBMX.exe work/  # the owner: git show forgejo/oracle:NgBMX.exe
nix develop                                  # (skip on native Windows)
tools/setup-work.sh work/NgBMX.exe          # verify hash; Ghidra export -> work/bundles (~10 min)
cmake --preset hook && cmake --build --preset hook
tests/oracle/run.sh -n 5                     # quick: all cases, 5 iterations, parallel workers
tools/check.sh                               # full gate: format, build, clang-tidy, CSV, all cases
```

Game assets for asset tests: point `BMX_GAME_DIR` at an extracted install. The layout comes from
`unshield -d game x data1.cab` on the CD's `data1.cab` (see `tests/oracle/win32.py` for the path aliases).
Without it, `DT_NEEDS_ASSETS` cases are skipped. The community labels used for names are in
`fromasmtodisasm/DAVE-MIRRA-LABELS` (give `tools/setup-work.sh` a `labels.tsv`: `addr<TAB>name`).

## Harness essentials (`tests/oracle/`)

- `oracle.py` loads the exe and `bmxtest.dll` in Unicorn and runs the real CRT startup up to WinMain
  under `win32.py` (deterministic heap, clock, files over the install read-only, registry "not found").
  Each case runs orig and port from the same snapshot and compares the return value and every written
  byte. `-j` sets parallel workers (default: performance cores).
- `dt_stub(orig, NULL, stub)` stubs a callee on **both** sides for one iteration (it finds our port of
  `orig` automatically). `dt_canon()` makes stack pointers in stub logs comparable. `dt_fill()` and
  memset/memcpy/memmove run natively in the harness for speed. Code and `.rdata` are write-protected.
- Tests compile at `-O0`, so stub logs can't be optimised away. The full suite is slow mostly because of
  fixture setup; the parallel runner fixes that.

## Lessons that cost real time (don't relearn them)

- Ghidra shows some functions as `void` that leave a value in EAX (init functions ending in `MOV EAX,1`)
  that callers test. Void ports pass their tests but break the real game (first in-game run: "no devices
  connected", zfile.c:240). Run `python3 -I tools/void_eax_audit.py .` after porting; it must report none.
- Argument counts that are wrong in port and stub alike pass the tests (rwSceneFind takes 1, rwSceneRelease 2).
  `py -I tools/argcount_audit.py .` checks ORIG_FN / port signatures against the original callee and the caller's
  ADD ESP; triage by hand (`-v` lists harmless unread extra arguments).
- A data pointer can start out at the ported function that calls through it (zrwTimeGetTime via 0x561480: endless
  recursion in-game). `py -I tools/selfref_audit.py .` reads the exe's initial values; must report no cycles.
- (int16_t) and AND 0xffff widen differently (zprimLoadGroup: ids >= 0x8000). `py -I tools/extend_audit.py .`
  compares MOVSX/MOVZX/AND before pushes and on parameters with the port's casts and types; no A/B findings
  (`extend_audit: ok - why` above a BMX_ADDR marks a hand-checked function).
- Ghidra pseudo-C regroups FP sums and drops pushed arguments. FP and argument lists come from the
  disassembly.
- The x87 model: `double` = register, `float` = stored. LLVM's x87 backend doesn't round floats, so the hook
  build uses SSE2. `_ftol` goes through int64 (`bmxFtol`).
- MSVC6 `strcmp`/`strncmp` return -1/0/1.
- Seeding functions from jump tables, padding or case labels creates fake functions. Ghidra thunks can be
  real tail-call wrappers. `int3; ret` is a real function.
- Stub logs: mask narrow arguments (16-bit handles carry garbage). Never log stack addresses (use
  `dt_canon`) or uninitialised bytes. Terminate every table a scan relies on.
- Unicorn: flush the translation cache when patching code; per-address code hooks are slower than
  patching. Python `-I` drops the script dir from `sys.path`.
- A stub must never call `dt_fill` during the run: the fill stream isn't rewound between the original
  and the port, so the two sides see different bytes (false failures).
- Never `pkill` test processes, because other agents' runs die too.
