# Release plan (draft, 2026-11-07)

Goal: a first public release that someone who owns the game can play without reading developer docs. That means
a Windows `.exe` and an Android `.apk` on a GitHub release, a short README for players, and a repo whose front page
is about playing, with the developer material one level down.

Nothing here is started. The decisions (D1-D8) are the user's; each has a recommendation. Decided so far: D1, D2 (the recommendations, 2026-11-07), D3, D5, D8.

## Decisions

**D1. How the Windows exe finds the game.**
- A. Drop-in: copy `bmxnative.exe` + `SDL3.dll` into the installed game folder and run it (how it works today).
- B. Folder picker: on first start, if the game's data isn't next to the exe, ask for the folder (SDL3 has
  `SDL_ShowOpenFolderDialog`), remember it in the ini.
- C. B plus a settings window for the ini options.

Rec: **B for the first release, C later.** **Decided (user, 2026-11-07): B.** B is A plus one dialog: the drop-in way keeps working, and the exe can
live anywhere. A settings window is real UI work (its own toolkit or an in-game screen); until then the README
documents the handful of ini keys people want (resolution, widescreen, framerate, music folder).

**D2. How the Android app gets the game data.** adb push is fine for testing but no player will do it.
- A. A folder picker on first start (Android's Storage Access Framework), then copy the data into the app's
  storage once (~250 MB).
- B. Read through the picked folder's `content://` URIs every time (no copy, but slower and every file path in the
  platform layer goes through a different API).

Rec: **A.** **Decided (user, 2026-11-07): A.** The code points are known: `bmxAndroidPrepare` (src/native/android.c) and the Java activity
(android/app/src/main/java/org/opendmfbmx/app/BmxActivity.java). A PS1 disc image is not an option: the PlayStation
version stores its models and textures in different formats.

**D3. Which Windows build to ship.**
- A. 64-bit only. B. 64-bit and 32-bit. C. 32-bit only.

Rec: **A, once the 32 vs 64 screenshot comparison (below) is clean.**

**Decided (user, 2026-11-07): A, 64-bit only.** Once native64 is verified, native32 is no longer supported as a
player build either: every cross-platform build is 64-bit anyway. What stays 32-bit regardless: the hook build and
the differential tests, because the original executable they compare against is 32-bit (part of D4, not a player
build). Dropping native32 means moving the checks that use it (the gate's "native32 imports" and "native32 data"
steps, the replay references) to native64 first.
  Done / revised 2026-11-07: the gate's import check covers native64 (and native32); the replay references are
  build-independent (data/simhash/ride.hash and the recording's hash are equal on both). The data check
  (`tools/gen_data.py --check`) compares the source's data byte for byte with the 32-bit original, so it needs a
  32-bit build by nature: **native32 stays as a test-only build** (never shipped), like the hook build.

**D4. The hook / differential-test machinery.** It is no longer needed to *play*: since X1b the native exe reads
nothing from NgBMX.exe. But it is still how correctness is proven: the gate's 2964 differential cases run the
ported functions against the original exe in an emulator, and tools like xcheck, the sim hash and the audits build
on it.
- A. Keep it, out of sight: player-facing README at the top, developer docs moved under `docs/dev/`.
- B. Move it to a separate branch or repo.
- C. Delete it.

Rec: **A.** Deleting it throws away the strongest evidence that this is a faithful port, and the claim the README
wants to make ("fully recompiled, verified") rests on it. Release artifacts contain none of it.
Revised 2026-11-07: leave the developer docs where they are instead of moving them to `docs/dev/`. The code
cites them by path in 151 places (`docs/PORTING.md X3`, ...); a move means rewriting all of those for no gain
to players. The README (players first, a short "For developers" section with the links) is what changes.

**D5. Name and version. Decided (user, 2026-11-07): Open Freestyle BMX**, v0.1.0 for the first release.
Drops the rider's name entirely (it may still be protected, and out of respect after his death); "Freestyle BMX"
on its own is the sport. Done the same day (src/compat/product.h):
- window title and Android app name "Open Freestyle BMX"; program `OpenFreestyleBMX.exe` (the CMake target stays
  `bmxnative`); settings `OpenFreestyleBMX.ini` (an existing `bmxnative.ini` next to the program is still read
  when the new one is missing) and log `OpenFreestyleBMX.log`; macOS bundle / Linux tarball named alike;
- Android application id **`org.openfreestylebmx.app`** (final from the first public release on: a change would
  be a different app, losing installs and saves);
- the repository moved to GitHub under the new name (D6). The local folder,
  code identifiers (`bmx*`, `BMX_*` environment variables) and the history keep the old name.

**D6. Where the release lives. Decided (user, 2026-11-08):** the repository moves to GitHub entirely,
`https://github.com/DenzoNL/open-freestyle-bmx` (public, no mirror). Only `main` goes there, its history rewritten to
the GitHub noreply author address first; old commits keep their mentions of the old server. The `oracle` branch
(only the original exe) stays on the private Forgejo server. Releases are made on GitHub.

**D7. Signing. Decided (user, 2026-11-08): as recommended.** Android: a release key (keep it out of the repo, back it up). Windows: unsigned, so SmartScreen
warns on first start. Rec: Android release key now; Windows code signing not worth the cost for a hobby release,
the README says what the warning looks like.

**D8. Legal check. Decided (user, 2026-11-07):**
- The original's strings and tables in `src/data/` are fine to publish (the rights holders can no longer be found).
  The README's "no game code or assets" sentence still gets reworded to say exactly what the repo holds.
- Screenshots of the game in the README are fine.
- **Music is the one concern.** Nothing of the soundtrack is in the repo or a release: the songs stay in the
  player's own game folder, and `[audio] music_dir` plays the player's own files. Before release, check that no
  track data, names or audio slipped in anywhere (docs/MUSIC.md, tools/gen_music_edits.py and its output), and
  keep screenshots / videos for the README free of the songs.
  **Found 2026-11-07, for the user to decide:** no audio is in the repo, but `data/music-edits.json` and
  docs/MUSIC.md hold the ten songs' artists, titles and albums and the game's edit timings (sample positions,
  gains), and the timings are compiled into the program (music_edits.c, the override needs them). Options:
  A. keep as is (facts about the songs, no audio; the override cannot work without the timings);
  B. keep the timings, drop artist / title / album names (match files by track number instead);
  C. take the music override out of the release.
  Rec: A, it is the kind of information a track list in a manual holds. **Decided (user, 2026-11-08): A.**
- **Licence. Decided (user, 2026-11-08): MIT** (`LICENSE`); SDL3 (zlib) and dr_flac ship their notices.

## Before release: work to finish

Correctness (64-bit, the release build):
1. AddressSanitizer runs of the ride and the user recording clean (in progress 2026-11-07; two real bugs fixed:
   the Im3D level stack and the sim hash, plus the echo buffer).
2. Screenshot comparison 32 vs 64-bit with the desktop settings: menus, every level, HUD modes, a replay,
   results screens. Differences beyond the known ±1 shade noise get fixed.
3. A longer play session on native64 by the user (the T-pose and height bar were found by playing).

Windows: D1-B folder picker; default ini written on first start with comments; `[audio] mute` as an ini key
(today `BMX_MUTE` is environment only).

Android: D2-A picker; a mute / volume ini key (as above); Back button and quitting behave; check pad-only menus;
release signing.
- Pad (2026-11-08): the Thor's built-in pad is a type SDL maps but does not know, so it read in SDL's raw order
  (the menu on L1, Back on a trick input). It now reads as `kBmxPadPositional`: PlayStation order by position,
  south jumps, Start opens the menu, Back (the Android key under the right stick and Select both arrive as SDL's
  Back) is Select. Commit 5bea270's message says the user pressed B for Back; it was the Android Back key, and
  its AC_BACK→Escape key mapping was taken out again (SDL already delivers that key as the gamepad's Back).
  Still to test on the Thor.

Repo:
- README for players (outline below); move PLAN.md, HANDOFF.md, agent-brief.md, notes and the porting docs to
  `docs/dev/`, keep a short "for developers" section linking there.
- A LICENSE file (there is none yet: the user picks one) plus the third-party notices that must ship (SDL3: zlib;
  dr_flac: public domain / MIT-0).
- Cleanup candidates (each checked before deleting, nothing removed blindly): the Mac/Linux packaging scripts stay
  (they work); tools that were one-off migrations (`data_off64.py`, `data_size64.py`, `probe64.py`, `extend_audit.py`?)
  move to `tools/dev/` or go if nothing calls them; `docs/HANDOFF.md` sections older than the current state.

## Release steps (once the above is done)

1. Tag `v0.1.0` on main after a full gate.
2. Build: `cmake --build --preset native64` (release flags), `android/build.sh release` with the release key.
3. Package (`tools/package_windows.py`): `OpenFreestyleBMX-0.1.0-windows-x64.zip` = exe, `SDL3.dll`, README, LICENSE, third-party notices, a sample
   ini. `opendmfbmx-v0.1.0-android.apk`. SHA-256 sums for both.
4. Check the zip on a clean Windows user account (no dev tools on PATH: catches missing DLLs, like the
   libwinpthread one bmxx87test needed) and the APK fresh-installed on the Thor.
5. `gh release create` on the GitHub mirror with short notes; never upload game files.

## README outline (players first)

1. One line: what it is ("a native, open-source PC port of a 2000 BMX game: modern Windows, Android handhelds").
2. Screenshots: desktop 21:9, the Thor, a menu with the blurred sides.
3. **What you need**: your own copy of the game (PC version).
4. **Install**: Windows (download, unzip, pick the game folder / drop into it); Android (install the APK, pick the
   folder).
5. **Settings**: the few ini keys that matter; controllers.
6. **What's new compared to the original**, the part that sets it apart from static recompilations:
   - Every function rewritten as readable C and checked bit for bit against the original executable (thousands of
     differential test cases in an emulator), not a machine translation of the binary with patches on top.
   - Runs as a 64-bit program, and on ARM (Android), with the original's floating point behaviour reproduced
     exactly: replays give the same result on 32-bit, 64-bit, x86 and ARM.
   - DirectX 6 and the Windows APIs replaced by SDL3 (Direct3D 12 / Vulkan / Metal underneath): runs on today's
     systems without compatibility layers.
   - Any resolution and aspect ratio, including ultrawide, with the HUD placed sensibly and sharp 2D.
   - High framerates through interpolation, while the game logic keeps its original 30 Hz timing.
   - Full draw distance, your own music, modern controllers.
   - Everything optional: `vanilla = 1` gives the original's behaviour.
7. Status and known issues; how to report bugs.
8. For developers: link to `docs/dev/`.
9. Legal: not affiliated with the publisher; you need to own the game.
