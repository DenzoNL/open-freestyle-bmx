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

## Windows handover (2026-11-06, Mac session)

The Mac session (12:30-17:00) added four platforms' worth of code. All of it builds; nothing new has run
with a visible window yet. Details per slice in docs/PORTING.md: "X5 part 1" (macOS arm64), "X5 part 2"
(portable x87), "X5 part 3" (headless null GPU), "X4 (Linux x64)", "X6 step 1 (Android arm64)".

What changed that touches the Windows builds (all behind `#if` / new code paths, but unverified there):
- `BMX_REGPARM(n)` / `BMX_CDECL` / `BMX_STDCALL` replace the raw i386 attributes (252 regparm sites). The hook
  build's objects were compared byte for byte on the Mac (identical); native32/native64 not built.
- `compat/x87.h` dispatches: the x87 asm (`bmxX87Hw*`) on x86 hook/native builds unless `BMX_X87_SOFT`, else
  `compat/x87soft.c` (integer emulation of the x87). Control word through `bmxX87ControlWord()`.
- New headless mode `BMX_GPU=null` (or `[video] backend = null`) in src/driver.
- 64-bit bug fixed in game.c: the goal markers read `PhysFace` at 32-bit offsets (crashed on arm64 with a face
  goal; native64 has the same bug, it only didn't show with your save).
- src/native: POSIX halves; `bmxLog` declared once; tidy fixes in crt.c / trap.c / hfinterp.c; draw.c's shader
  selection rewritten (DXIL, then SPIR-V, then MSL).

**To do on the Windows PC, in this order** (PowerShell, repo root, after `git pull`):

1. Build and gate: `cmake --build --preset native32`, `cmake --build --preset native64` (warning-free), then
   `PYTHON=py tools/check.sh` (Git Bash) with native32 built. Fix anything Windows-only that broke.
2. Shaders: `tools/shaders.sh` (dxc + spirv-cross) must regenerate the DXIL / SPIR-V headers byte-identically
   and the same MSL.
3. Replays unchanged on Windows: `tools/xcheck.py` with your recording and the scripted ride on native32 vs
   native64, as before (same hashes as the X3 reference).
4. Real x87 vs the portable emulation (only real hardware can say; PORTING.md "X5 part 2"):
   `cmake --build --preset native64 --target bmxx87test`, run `build\native64\bmxx87test.exe`, and the same
   for native32. Already run on nixbox's Intel i5-13400 (Linux): every game table, tan, AngleOfCosine, Rotate
   and MatrixMultiply equal; **Fsin/Fcos on arbitrary run-time angles differ in ~1 of 100k cases**
   (15-26 per 2M) and the scaled asin/atan with arbitrary scales in 54 of 250k (Intel's FSIN isn't exactly the
   correctly rounded 66-bit-pi result the model computes). Your PC tells whether AMD/your CPU agrees with
   Intel; then decide whether those last-bit cases matter (see 6).
5. Headless vs windowed, and arm64 vs x86 end to end (PORTING.md "X5 part 3", needs an empty `save\` copy of
   the game and an empty ini):
   `py -I tools\xcheck.py --game-dir <game> --keys tools\scripts\ride.keys --args=-r640.480.16 --ini empty.ini --ref-hash data\simhash\ride.arm64.hash --out <abs path> build\native64\bmxnative.exe BMX_GPU=null build\native64\bmxnative.exe`
   Expected: windowed, null and the arm64 reference identical (2973 ticks). Repeat with native32. Then the
   end-to-end soft check: build `-DBMX_X87_SOFT=ON` into `build\native64-soft` and xcheck your recording on
   both exes.
6. Done on nixbox already (real Intel i5-13400, Linux x86_64, headless null GPU): the ride gives the
   **same BMX_SIM_HASH as the arm64 reference on the real x87 and with `BMX_X87_SOFT=ON`** (2973 ticks; the hw
   and soft hash files byte-identical). So the rare last-bit FSIN / atan differences of step 4 do not reach this
   ride. Your recording (longer, more levels) on native64 vs a native64 soft build is the stronger check.

**64-bit level rendering (fixed 2026-11-06, check on native64 first):** floats passed through int / pointer
prototypes (zrwSetClipPlane, the fog distance 0x493100, the light routines in rw/light.c) broke every 64-bit build:
in a level no 3D geometry was drawn (one flat fog colour) and lights crashed. Windows x64 passes floats in XMM
registers too, so **native64 almost certainly showed the same green level** before this fix: play a level on
native64 now. Also compare the hint icon in "tap [icon] to turn the bike around": it is missing on the Mac (no pad).

**Mac:** a windowed run (muted) shows menus, movies and a level correctly on Metal (2026-11-06, Greenville start).
User playtest (2026-11-06, ~17:00, keyboard only) found:
- **Rider T-posed on the bike, no animation at all** (macOS arm64). Next thing to chase: likely another
  64-bit / arm64 issue in the anim / skin / bone path (rw/plugins.c RpAnim*/skin helpers with ST0-argument
  entries that are naked asm on i386 and C callers elsewhere, interpolator float arguments through int
  prototypes like the clip-plane bug, or zactor/motion bank layouts). Check native64 on Windows: if the rider
  animates there it's arm64-specific (x87soft / FPCR / the BMX_X87_ENTRY C paths); if not, 64-bit. The sim hash
  can't catch it (it hashes rider physics, not the skinned pose); BMX_VTX_DUMP of a level frame on native32 vs
  arm64 would show it.
- **Movie skip works with the MacBook's built-in keyboard but not a USB keyboard** (the USB one does work in the
  menus? unknown). Likely the DirectInput keyboard stand-in on SDL3 picks one keyboard id instead of all
  keyboards (SDL_GetKeyboardState is global; per-device SDL_KeyboardID filtering in src/platform/input.c).
- Acclaim and Z-Axis logos played to the end before the window had focus; check whether the original lets
  Enter skip them on Windows.

**Mac (when at home, the user's call):** `nix develop -c cmake --preset macos && cmake --build --preset macos`
(or `--target bmxnative_app`), run from a copy of the game folder: menus, a level, the MSL output against the
Windows frames, input with a pad, a movie, audio. Untested so far: input with a pad, audio, long sessions, the .app bundle.

**Android (the Thor):** `nix develop .#android`, `tools/android_sdl3.sh`, `android/build.sh`; install
`android/app/build/outputs/apk/debug/app-debug.apk`, `adb push <game>/. /sdcard/Android/data/org.opendmfbmx.app/files/game/`,
then the 5-step list in PORTING.md "X6 step 1"; logcat tag `opendmfbmx`.

**Linux:** `cmake --preset linux && cmake --build --preset linux` (built on nixbox and in an OrbStack Fedora
amd64 VM `bmx-linux` on the Mac; delete it with `orb delete bmx-linux` when no longer needed). A windowed run
and the Steam Deck are open.

### nixbox ride

A first run trapped at frame ~900 ("There is already a scene open"). Cause: the game copy
(`~/bmx/ridecopy`, from the CD install) had the levels only as top-level `Level_-_XX` folders, while the scene
names `ASSETS\<LEVEL>\...`: the install layout the game expects has `Assets/<Level>` (and `bin/audio`,
`bin/movie`). Fixed on nixbox with relative symlinks `Assets/<Level> -> ../Level_-_XX`. The assert text was
misleading: clang merged zsceneOpen's zdebugFail calls into one call site, so gdb shows line 525 for all of
them; the real failure was the scene file load. Follow-ups: log the path when a scene / level file fails to load,
and have xcheck check the `Assets/<Level>` layout. Results and the run commands: `~/xc-hw`, `~/xc-soft`,
`~/gdbride.sh` on nixbox (python3 via `nix shell nixpkgs#python3`).

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
  hashes are equal on / off at every aspect tried. Unit tests: `bmxwidetest`. Defaults on since 2026-11-01 (PLAN.md Decisions; `[general] vanilla = 1` for the original). Before: vanilla
  until the user's playtest: `native-run2\widetest.ps1` (default 3440x1440 21:9; `-Aspect 16:9` for 1920x1080). **Next:**
  that playtest, then WS6 (done 2026-10-31, not committed: `ui_filter` 0/1/2 and `menus = 2`, docs/WIDESCREEN.md "WS6").
  **WS5 done 2026-10-31** (not committed): `[video] hud = 1` anchors the HUD per element (timer, name, flash left;
  score, height meter, replay arrow, icon right; combo, caption, hints, messages centred) within a centred
  `hud_aspect` area (default 178 = 16:9; 0 = full width). Seam: a jump over the ported ngifDrawList to a copy that
  tells the Im2D wrapper each packet's anchor; elements come from the layer-0 zover pools (glyph chains) and from
  zoverDrawText's glyph runs in the 3D list (docs/WIDESCREEN.md "WS5"). `hud = 0`, widescreen off and exact 4:3 stay
  byte-identical; sim hash equal. Try it with `widetest.ps1 -Hud 1` (`-HudAspect 0` for the full 21:9 width).
- High fps (29.4 Hz tick kept, interpolated draws): design in docs/HIGHFPS.md (H1-H10 decided as recommended).
  **HF0 done 2026-10-31** (not committed at the time of writing): the `BMX_HF_GUARD=1..4` dev aid
  (src/native/hfguard.c; frames and sim hash unchanged with it unset). Findings, in docs/HIGHFPS.md "HF0 findings":
  - The particle step runs in the tick; the draw only renders particles, and the RNG is untouched by the draw.
  - The shadow probes in the draw write simulation state: they widen gZrwColBox and advance RW's lightFrame.
  - Collision walks RenderWare's sector tie lists, and every frame sync re-ties the synced atomics. HF2's restore
    must replay the tick's dirty-frame order; tested, this gives the same ties and frames.
  - Drawing the HUD ordering table twice is clean.
  - Pacing: a ~1.5 ms draw (6 ms worst) and ~32 ms of slack per 34 ms tick at 640x480.
  - The far clip / fog distance is wall-clock driven (ngiffxAdaptQuality). It climbs from 60 to 190 during the first
    ~20 s of a level, so mid-climb screenshots, such as frame 1500 of the ProQuest script, can differ between
    builds.

  **HF1-HF3 done 2026-11-01** (not committed at the time of writing; docs/HIGHFPS.md "HF1-HF3 implementation",
  "HF4 runs"): `[video] interpolate = 1` (default 0), `max_fps` (0 = display refresh), `vsync` (1). In levels the
  30 Hz tick keeps the original limiter; extra draws in between (only when the worst recent draw still ends before
  the tick is due; the loop sleeps otherwise) show every model object and the renderer camera's view interpolated
  one tick behind (src/native/hfinterp.c, maths in hfmath.c, `bmxhftest`). Restore before each tick by recompute plus
  the recorded dirty-list order; extra draws skip the shake node, re-arm the HUD flush flags, wrap the shadow probes
  in a save / restore of gZrwColBox / gZrwCol / lightFrame and present uncounted. Runs: `interpolate = 0` frames
  and sim hash identical to 91cb83d; `interpolate = 1` at 120 / 144 / 240 Hz, vsync on / off, 3440x1440: sim hash
  identical tick for tick (riding, pause, END RUN?, results, replay); `BMX_HF_ALPHA=1` frames identical; the guard
  saw no simulation state change from 10942 extra draws and every restore exact. Skip thresholds: objects 500 units
  / 90 degrees per tick, camera 256 / 60. **Next:** the user's playtest, `native-run2\fpstest.ps1` (`-Widescreen`
  for 3440x1440, `-MaxFps`, `-Vsync 0`, `-Off`), then the default decision (H8) and HF5.
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
