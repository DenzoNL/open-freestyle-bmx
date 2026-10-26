# Handoff: how this port is run

Read this first in a new Claude Code session or on a new machine. It holds what isn't obvious from the code.
Then read `PLAN.md` (strategy, decisions, open items) and `CONTRIBUTING.md` (porting rules).

## State

- Hook phase. Ported functions are verified bit for bit against the original `NgBMX.exe` in an emulator.
  Progress: `python3 -I tools/progress.py` (functions and bytes per source file).
- `main` on `ssh://forgejo@switchbyte.dev/denzo/opendmfbmx.git` is the truth. Commit and push to main
  directly, but batch pushes (CI runs on main).
- The original exe lives **only** on the orphan branch `oracle`. Never merge it into main. Never push
  `oracle` to a public host. If the repo ever moves to GitHub, push main only and delete `oracle`.
- CI (Forgejo, Linux docker runner) runs the gate on every push to main. It has no exe or assets, so
  the differential cases are skipped there and run locally.

## Next up (2026-10-26)

Hook-phase porting is done: 2118/2120 functions (WinMain and the EpicFail trap stay original on
purpose). Full suite: every case passes at full iterations.

1. **Trace smoke test on the Windows PC** (no Wine needed), following `docs/TRACING.md`:
   - `git pull`, `cmake --build --preset hook`;
   - copy `build/hook/dinput.dll` next to `NgBMX.exe` and create an empty `bmxtrace.orig` in the game
     folder;
   - start the game, ride for a minute, quit normally;
   - `py -I tools/trace_dump.py <game folder>/bmxtrace-<n>.bin --frames 0:10`.

   Good: thousands of frames, rider positions changing, pad button bits changing with input. Bad: no
   file, or a crash at startup. The trace path has never run in a real process yet, so fix whatever
   breaks in `src/hook/trace.c`.
2. If that works, record real sessions (list in `docs/TRACING.md`) into `work/traces/`.
3. Then: coverage of the 7 big `game.c` functions, the open items in `PLAN.md`, and planning the
   platform flip (native build, SDL3 renderer replacing RenderWare).

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
git clone ssh://forgejo@switchbyte.dev/denzo/opendmfbmx.git && cd opendmfbmx
git fetch origin oracle && mkdir -p work && git show origin/oracle:NgBMX.exe > work/NgBMX.exe
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

## Porting workflow (what the orchestrating session does)

1. **Pick a slice.** `tools/slice.py plan` lists files bottom-up. Check what's left:
   `python3 -I tools/progress.py`. Prefer whole files with no RenderWare calls first.
2. **Split by difficulty.** Functions of 1 KB or more that use x87 go to an **Opus** agent, a few at a time.
   Everything else goes to **Sonnet** in slices of 80–200 functions. Measured: Sonnet ≈ 5–6k tokens per
   function and is reliable on normal code, but failed outright on big FP functions. Opus is ≈ 30–60k tokens
   per big function.
3. **Build the packet** (keep it ≲ 60k tokens):
   `tools/slice.py packet <files…> --exclude <big-x87 addrs> -o work/slices/<name>.md`
   or `--addrs a,b,c` for specific functions.
4. **Launch** a worktree-isolated agent whose prompt is "read `docs/agent-brief.md` and follow it", plus
   the slice description, packet path, target files and slice-specific hints. One agent at a time when
   the user is also working (budget, below).
5. **Merge** when it reports:
   ```sh
   git -C <worktree> add -A -N . && git -C <worktree> diff HEAD --binary > /tmp/slice.patch
   git apply --3way /tmp/slice.patch
   ```
   Resolve conflicts (usually headers where several slices declared the same callee):
   - Ported prototypes win over `ORIG_FN` declarations.
   - Match functions by **address**, not by name, and rename callers to the ported name.
   - After the merge: `python3 -I tools/progress.py`, build, `tests/oracle/run.sh -n 5` (all cases), the
     format check, then commit and push.
   - Remove the worktree, and stop the agent if it's still running background work.
6. **Watch for** agents that "finish" but keep spending tokens on leftover background runs. Stop them
   after merging.

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

## Budget

The user's weekly Claude usage is shared with paid work. Target ≤ 90% weekly by Friday (~18% per
workday). Ask for the `/usage` % before big batches. Three parallel agents (including one Opus) hit the
hourly limit once.
