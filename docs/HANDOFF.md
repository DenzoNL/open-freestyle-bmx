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
  RwMatrixMultiply routine 0x54b038 ran as kept original code at that time (stopgap; ported in S11 as rwMatrixMultiplyX87).
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
  step 3 "Visual check"; the rider shadow is absent in the hooked original at the Vail Trails start too), then the D7 screenshot diff harness (step 4).
  Second pass (docs/NATIVE.md "Session, movies and two-player"): Session (full run, gear up, level select, replay,
  exit) and Cool Stuff's intro movie and credits with movies on also run without a trap. Two-player modes do not
  exist in the PC version (player count only ever 0 or 1). Native runs go from a copy of the game folder
  (`C:\Users\Dennis\Games\dmfbmx\native-run`, no dinput.dll) so the user can play the hooked game meanwhile.
- Platform layer S1-S4 done 2026-10-31 (docs/PLATFORM.md 4a, docs/NATIVE.md "Platform layer", src/platform/, native only,
  hook DLL unchanged): the import table is `{dll, name, fn}` stand-ins + trap stubs for the 64 dead slots + the real
  function for the rest; time (timeGetTime / GetTickCount with Windows' resolutions, base and wrap), Sleep, QPC,
  critical sections, the winmm timer thread (16 ms pump on an SDL thread) and the sound loader's file calls run on
  SDL3 / our code. Checks: `bmxplatformtest.exe` (against the real kernel32 / winmm), `BMX_FILE_CHECK=1`,
  `BMX_TIMER_LOG=1`, scripted runs without a trap, frames identical to the previous build. Opus review wanted for
  the timer thread's concurrency (PLATFORM.md 5, "Timer-thread concurrency"). **Next:** S5/S6 (SDL input stand-in,
  d-pad), then S7 (DirectSound on SDL3 audio) and S8 (own AVI / Cinepak).
- Platform layer S5/S6 done 2026-10-31 (docs/PLATFORM.md 4b, docs/NATIVE.md "Input on SDL3", native only): every run
  reads keyboard and pad through a stand-in DirectInput on SDL3 (DirectInput's button numbering per pad family,
  its dead zone arithmetic, DIK scan codes, foreground semantics; BMX_KEYS is its scripted keyboard);
  `BMX_INPUT_CHECK=1` compares every call with the real dinput.dll in the same process; `bmxnative.ini`
  (`BMX_CONFIG`) with the opt-ins `[input] dpad_steering = 1` and `[input] stick_8way = 1` (stick snaps to 8
  directions, radial 10% dead zone, so pure down / up / left / right work; `inputtest.ps1 -Snap`; user test pending). New opt-in `[audio] smooth_resample = 0|1|2` (0 original mixer, 1 linear, 2 cubic): src/platform/smoothmix.c, a native-only copy of winchanMix that uses the position fraction, swapped in by a jump over the ported winchanMix (loader, `bmxSmoothJumps`); unit tests in tests/platform; compare by ear with `native-run\soundtest.ps1 -Smooth 0|1|2` (user test pending). The S3 timer review fixed a double join / slot reuse
  race and the timeKillEvent return codes. The user's first focused run found the pad dead in a level (a second
  exclusive pad object was refused; DirectInput allows it within a process) and lX / lY one step off (centre 128,
  not 127): both fixed, PLATFORM.md 4b. **Open:** the user's rerun of NATIVE.md test A; unfocused, the game's whole call sequence already matches the real DirectInput (0 HRESULT differences, same
  product name / GUID / type). **Next:** S7 (DirectSound on SDL3 audio), S8 (own AVI / Cinepak).
- Platform layer S7 done 2026-10-31 (docs/PLATFORM.md 4c, docs/NATIVE.md "DirectSound on SDL3", native only): the game's
  DirectSound runs on our own mixer in one SDL audio stream (LoadLibraryA / GetProcAddress / FreeLibrary stand-ins hand
  out our DirectSoundCreate; KERNEL32 has no real slot left). Cursors, lock rules, the volume / pan law (0.1 dB steps,
  1/65536 truncation) and focus muting were measured on dsound.dll in-process and copied: `bmxplatformtest` shows the
  API (193 results) and the cursor sequences of 12 kinds of buffer identical to dsound.dll after every mix, and equal
  real-time steps and levels; `BMX_SOUND_CHECK=1` mirrors the game's calls on the real dsound.dll (two scripted runs:
  `RESULT clean`, 0 differences in 46000 calls). **Open:** the user's listening test (`native-run\soundtest.ps1`).
  **Next:** S8 (own AVI / Cinepak), S9 (movie output and waveOut on SDL3).
- Platform layer S8/S9 done 2026-10-31 (docs/PLATFORM.md 4d, docs/NATIVE.md "Movies on our decoder and SDL3", native
  only): own AVI reader + Cinepak decoder (src/platform/avi.c, cinepak.c) replace AVIFIL32/iccvid, DrawDib draws on
  the SDL3 driver (src/driver/movie.c), waveOut plays on SDL3 audio and posts MM_WOM_DONE (src/platform/waveout.c).
  `bmxmovietest` (tests/platform/test_movie.c): every frame of the 7 movies byte-identical to iccvid (two iccvid
  quirks found and copied: the zero-tail selective V1 update, no codebook copy for key strips), StreamInfo /
  ReadFormat / TimeToSample / SampleToTime / Read equal to avifil32's at every position; DrawDib vs our GPU readback
  0 pixels differ; waveOut's results, flags and messages equal winmm's (bmxplatformtest); bytes queued = bytes read.
  The game's import slots now load only USER32 for real. **Open:** the user's watch test (`native-run2\movietest.ps1`).
  **Next:** S10 (SDL-owned window, message queue, message boxes).
- Platform layer S10 done 2026-10-31 (docs/PLATFORM.md 4e, docs/NATIVE.md "Window and messages on SDL3", native
  only): SDL owns the window (src/platform/window.c); our message queue feeds zrwMainWndProc (WM_ACTIVATE from SDL's
  focus, close -> WM_CLOSE -> WM_DESTROY -> WM_QUIT, WM_KEYDOWN, the movies' MM_WOM_*); MessageBoxA on
  SDL_ShowMessageBox; no device dialog (P16: `-r` or `[video]` in bmxnative.ini, default 640x480x16 full screen);
  `[ui] confirm_quit` defaults to 0 (the user's call: QUIT without the box). The import table needs no real DLL
  (`69 stand-ins, 68 dead, 0 real`) and bmxnative.exe no longer imports USER32 / WINMM. Findings: `-r` matches only
  height and depth (original bug, kept: `-r1024.768.16` gives 1366x768; use `[video]`); the Esc quit of the window
  procedure is unreachable in the shipped game (key lock before the first pump). **Open:** the user's focus test
  (`native-run\focustest.ps1`).
- Platform layer S11 done 2026-10-31 (docs/PLATFORM.md 4f, native only, hook DLL unchanged): main.c, keys.c and
  driver/gpu.c have no Win32 left (argv, `__ImageBase`, `_exit`, own DIK constants; the WASAPI session mute is gone:
  unmute bmxnative.exe once in the Windows mixer if an old BMX_MUTE run left it muted); winmm / ole32 no longer
  linked; `tools/check.sh` fails if `build/native32/bmxnative.exe` imports a DLL outside SDL3 / KERNEL32 / bcrypt /
  api-ms-win-crt-* (skipped without native32). A key held when the window gains the focus now reads as down (seed from
  the OS key state, Windows-only GetAsyncKeyState probe; the focus test with Enter held at start is the user's). The
  x87 RwMatrixMultiply routine 0x54b038 is ported (`rwMatrixMultiplyX87`, oracle-tested) and out of kTextKeep, which
  keeps only the thunk table, the RpWorldDestroy stub, and the dinput tables under BMX_INPUT_CHECK. **Next:** S12.
- Widescreen / native resolution / HUD layout (docs/WIDESCREEN.md, the user took every recommendation): **WS1-WS4
  done 2026-10-31** (native only; not committed at the time of writing). `[video] widescreen = 1` stores a hor+
  wrapper into the zgl camera slot 0x561d98 and wraps the engine device's fpIm2DRenderPrimitive (classifier: full-
  width untextured = cover, stays stretched; everything else mapped into the centred 4:3 area and clipped to it);
  movies pillarboxed in the DrawDib stand-in; `width = height = 0` = desktop size; `scale_filter` for the present.
  Off, and on at an exact 4:3 mode, BMX_SHOT frames are byte-identical to the build before; `BMX_SIM_HASH` per-tick
  hashes are equal on / off at every aspect tried. Unit tests: `bmxwidetest`. Default stays vanilla (W9) until the
  user's playtest: `native-run2\widetest.ps1` (default 3440x1440 21:9; `-Aspect 16:9` for 1920x1080). **Next:**
  that playtest, then WS5 (HUD edge anchoring) and WS6 (ui_filter, blurred menu fill).
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
