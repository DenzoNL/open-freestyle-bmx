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
- CI (Forgejo, Linux docker runner) runs only on pull requests and manually (Actions → ci → Run
  workflow), not on push: run `tools/check.sh` locally before pushing. It has no assets, so
  the differential cases are skipped there and run locally.

## Next up (2026-10-31)

- Game: 2117/2120 (WinMain, the EpicFail trap and the 1-byte crtNop stay original on purpose).
- RenderWare: 708/711 ported (the three 4-byte stubs stay original: a hook jmp would overlap the next
  function). What remains is the D3D7 driver (`lib/d3d7driver`, never ported): replace it with the SDL3
  driver at the device / standards / extension tables. Design decided 2026-10-29: docs/SDL3-DRIVER.md
  (cut at RW's device interface, CPU vertex lighting ported faithfully, offline-compiled shaders, render at the
  game's mode resolution, RGBA8 textures with CPU copies, original device dialog for now, screenshot diffs).
- SDL3 driver step 1 done 2026-10-29 (src/driver/): `bmxnative.exe Scene.Txt -r640.480.16` runs the menu loop with
  clear + present in an SDL3 GPU window (300 frames, no trap).
- PNG reader done 2026-10-29 (src/png/, docs/NATIVE.md "PNG reader instead of libpng"): replaces libpng in the native
  exe, byte-identical to libpng 1.0.5 on 112 files (all game PNGs + synthetic set) via `BMX_PNG_CHECK`. Next blocker:
  the D3D driver's Im3D code, 0x4ea960 from rwIm3DPipelinePush (0x4e85b0), i.e. SDL3 driver step 2/3. The x87
  RwMatrixMultiply routine 0x54b038 runs as kept original code for now (stopgap, SDL3-DRIVER.md open points).
- SDL3 driver step 2 done 2026-10-30 (docs/SDL3-DRIVER.md step 2, docs/NATIVE.md "Step 2: the menu draws"): the main
  menu draws like the original. The D3D7 driver's CPU side (Im3D transform/clip, render states, Im2D with the alpha
  sort; 80 functions) is ported into src/rw/d3d*.c with oracle tests at 0 ULP; its Direct3D calls go through five cut
  functions (rw/rwd3d.h) to SDL3 GPU (src/driver/draw.c). Shaders: tools/shaders.sh (dxc; DXIL + SPIR-V, MSL missing).
  Next: step 3, the instancing pipelines, first trap 0x4c92d0 from rwIm3DPipelineRun. Also: run the menu once more
  with the final (stripped) shader headers, and the D7 screenshot diff against the original.
- SDL3 driver step 3 CPU port done 2026-10-30 (docs/SDL3-DRIVER.md step 3, docs/NATIVE.md "Step 3"): the world
  sector and atomic instancing pipelines (instancing, morph targets, the five light routines, colour and render
  stages; 15 functions) are ported into src/rw/d3dpipe.c with oracle tests at 0 ULP. They are Im3D pipelines without
  D3D vertex buffers, so the cut and src/driver needed no change. Lesson: clang folds `x - y * -d` into `x + y * d`
  even at -O0, which is wrong when rounding down; keep negations opaque in camera-mode float code (negMul).
  Native (2026-10-31, after porting the pointer-only light stage 0x4b1860): 4000 frames without a trap, the attract
  mode's demo level renders. The hooked game with steps 2 + 3 passes the smoke test (menus, level, riding).
  **Next:** drive the native exe into a level by keyboard and port its traps (docs/NATIVE.md "Step 3"); then the D7
  screenshot diff harness (step 4). rwmap.py misses functions reached only through pointers stored by code it
  never saw: expect more of those as traps.
- Native into the levels done 2026-10-31 (docs/NATIVE.md "Scripted input"): `BMX_KEYS="f300:enter,+150:enter,..."`
  drives the native exe by a scripted keyboard (a stand-in DirectInput at the import; works unfocused, never takes
  the focus, ignores the user's real keyboard/pad; MessageBoxA answered too). ProQuest, a full run, pause / end run /
  replay / main menu, Freeride in all seven unlocked levels, Options, Cool Stuff and QUIT run **without a trap**, so
  there is no next trap in single-player play. **Next:** compare native and hooked frames (docs/SDL3-DRIVER.md
  step 3 "Visual check"; the rider shadow is absent in the hooked original at the Vail Trails start too), then the D7 screenshot diff harness (step 4); Session and
  two-player modes were not driven yet.
- Golden traces: record sessions with `bmxtrace.orig` (docs/TRACING.md) if not done yet.

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

## Budget

The user's weekly Claude usage is shared with paid work. Target ≤ 90% weekly by Friday (~18% per
workday). Ask for the `/usage` % before big batches. Three parallel agents (including one Opus) hit the
hourly limit once.
