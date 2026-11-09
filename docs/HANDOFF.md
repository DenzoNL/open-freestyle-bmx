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

## Next up (2026-11-09): v0.1.0

**Release status.** docs/RELEASE-PLAN.md is current: every decision is taken, the owner's 64-bit play session was
clean, the Thor's pad works, the Android release key exists (on the owner's Windows machine, outside the repo). Left:
build the release exe / APK / zip, check them on a clean Windows account and a fresh Thor install, tag `v0.1.0`,
publish on GitHub. Those steps run on Windows (the key and the Thor live there).

**The Linux build from a plain distribution (done 2026-11-09, on Omarchy / Arch, no nix on PATH).** A user had
reported "a lot of errors of all kinds", with clang and without. Reproduced and fixed; what it was:
1. A plain `cmake -B build` configured the **hook** build (`BMX_HOOK` defaulted ON): the i686 Windows DLL against
   the original exe, with the host's gcc (17 errors in 6 files). Now, off Windows, the defaults are the native build
   (`BMX_HOOK` OFF, `BMX_NATIVE32` ON); the presets set both explicitly as before.
2. **gcc** built the native configuration with 129 errors in 27 files (clang pragmas, strict aliasing, function
   type casts, redefinitions). Not worth supporting: CMakeLists.txt stops at configure unless the compiler id is
   Clang / AppleClang, with a message naming the preset and `CC=clang`.
3. `-Werror` is the `BMX_WERROR` option: ON in every preset (the gate, the release builds), OFF without a preset.
4. `cmake --preset linux` with the distribution's clang 23, CMake 4.4, Python 3.14 and SDL3 3.4.18 (Arch packages)
   built warning-free from a clean build directory; the headless reference ride (`tools/scripts/ride.keys`,
   `BMX_GPU=null`, 4000 frames) gave a BMX_SIM_HASH byte-identical to data/simhash/ride.hash, i.e. the real x87
   on this x86_64 PC matches arm64 / x87soft (PORTING.md X4 had this as "still to do on real hardware"). A windowed
   run (SDL 3.4.18, GPU driver vulkan, 3440x1440 full screen, Wayland, PipeWire) played the intro movies and the
   front end and shut down cleanly. The README's "Linux" section has the per-distribution packages and the commands.
5. bmxhftest (329 checks) and bmxwidetest (849) pass. **bmxx87test on real hardware (13th Gen Intel(R) Core(TM) i5-13600KF) reports
   DIFFERENCES** in the 66-bit-pi model: 15 / 26 / 25 of 2,000,000 random Fsin/Fcos cases and 54 of 250,000
   HalfPi/Asin/AtanScaled "any scale" cases differ in the last bits; every table-driven group (the game's actual
   inputs) and the ride hash are equal, so this is the known transcendental last-bit question of PORTING.md X4
   (Rosetta showed the same shape), now with a hardware data point. Not a Linux build issue; open.
The MSL array in src/driver/shaders/ui_ps.h was regenerated (`tools/shaders.sh --msl-only`), so macOS has the 2D
filter fix too. Decision for the release: no Linux binary download (glibc / distribution spread, and the tarball
target only serves nix-built binaries on nix machines); Linux players build from source with the README's three
commands. An AppImage or a Steam-runtime build stays an option (PORTING.md X4 "Packaging") if anyone asks.

**Playing on this machine: use the distro build, not the nix one.** The `nix develop` build links nix's SDL3,
which dlopens nix's vulkan-loader; on a non-NixOS host that loader cannot use the system's NVIDIA ICD (different
glibc), so `SDL_CreateGPUDevice` fails with "No supported SDL_GPU backend found" (2026-11-09). The nix build is
for the tarball on nix machines and for headless runs (`BMX_GPU=null`). For a window, build with the distro
toolchain off the nix PATH, into its own directory:
```sh
env -i HOME=$HOME PATH=/usr/bin:/bin:<a ninja on PATH> cmake -B build/linux-distro -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/x86_64-linux.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBMX_WERROR=ON
cmake --build build/linux-distro && cd work/gamedir && ../../build/linux-distro/OpenFreestyleBMX
```
(pacman has no ninja installed here and sudo needs a password; nix's ninja on PATH is harmless.)

**Game folder on this machine:** `work/gamedir` (ignored by git) is the owner's CD copy in the install layout
(Scene.Txt, Assets/ with the twelve level folders linked from the `Level_-_XX` components, bin/audio, bin/movie,
save/); `work/game` is the raw unshield output, `work/cd` the ISO, `work/rar` the archive from the owner's share.
The original exe is at `work/game/Program_Files/NgBMX.exe` (copy it to `work/NgBMX.exe` for the hook build and
`tools/setup-work.sh`).

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
