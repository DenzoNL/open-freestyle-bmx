# Open Freestyle BMX

A native, open-source port of the 2000 PC BMX game *Dave Mirra Freestyle BMX*: rebuilt from the ground up for
today's Windows PCs and Android handhelds, at any resolution and frame rate.

<!-- Screenshots to add before the release: 21:9 desktop in a level, the AYN Thor, a menu with the blurred sides.
     Keep the soundtrack out of any video. -->

## What you need

- **The PC version of the game**, installed or copied from your own disc. This project contains none of the
  game's files (models, textures, levels, sounds, music, movies): it reads them from your copy.
- **Windows 10 or 11 (64-bit)**, or an **Android 8+ handheld or phone** with an arm64 processor and Vulkan
  (tested on the AYN Thor).

## Install

**Windows**

1. Download `OpenFreestyleBMX-windows-x64.zip` from the [Releases](../../releases) page and unzip it anywhere.
2. Run `OpenFreestyleBMX.exe`. The first time, it asks for your game folder (the one with `Scene.Txt` and the
   `Assets` folder in it) and remembers it. You can also copy the two files into the game folder and run it
   from there.

Windows may warn that the program is from an unknown publisher (it is not signed). Choose *More info*, then
*Run anyway*.

**Android**

1. Copy your game folder to the device (for example into `Download`).
2. Install `OpenFreestyleBMX-android.apk` from the [Releases](../../releases) page.
3. Start it and choose the game folder. It is copied into the app once (about 250 MB); after that the copy in
   `Download` can go.

Saves live in the game folder's `save` folder, as in the original, so a save from the PC works on Android and
the other way around.

## Settings

The first start writes `OpenFreestyleBMX.ini` next to the program (on Android: in the app's files folder,
`Android/data/org.openfreestylebmx.app/files`). Every setting is listed there with an explanation; remove the
`;` at the start of a line to change it. The ones most people want:

| Setting | Default | |
|---|---|---|
| `[video] width`, `height` | 0 | the screen's size; any resolution works |
| `[video] fullscreen` | 1 | 0: a window |
| `[video] interpolate` | 1 | smooth motion at your display's refresh rate; 0: the original 30 frames per second |
| `[video] hud` | 1 | the HUD at the screen's edges on wide screens; 0: in the 4:3 middle |
| `[general] vanilla` | 0 | 1: the original game's look and behaviour for everything you don't set yourself |
| `[audio] mute` | 0 | 1: no sound |
| `[audio] music_dir` | | a folder with your own copies of the soundtrack as FLAC files (below) |

**Controllers:** Xbox and PlayStation controllers work as they did in the original; other pads (like a
handheld's built-in controls) are laid out by position like a PlayStation pad: bottom button jumps, Start opens
the menu. The keyboard works as in the original.

**Music in CD quality:** the game stores its soundtrack as compressed mono. With `[audio] music_dir` pointing at
a folder with the album tracks as FLAC files, it plays them instead, cut exactly like the game's own edits, in
stereo. See [docs/MUSIC.md](docs/MUSIC.md).

## What's different from the original

This is not a patched copy of the original program, and not a machine translation of it. The game's ~3,000
functions were rewritten as readable C, and verified against the original executable: a harness runs the
rewritten code and the original side by side in an emulator, on close to 3,000 test cases, and they must give
identical results down to the last bit of floating point. On top of that faithful base:

- **Modern systems, no compatibility layers.** DirectX 6, DirectInput, DirectSound and the old Windows APIs are
  replaced by [SDL3](https://libsdl.org): Direct3D 12 or Vulkan underneath, current controllers, current audio.
- **64-bit, and on ARM.** The game ran only as a 32-bit x86 program. It now builds for x86-64 and arm64; its
  floating point behaviour is reproduced exactly, so a recorded ride plays out identically on a PC and an
  Android handheld.
- **Any resolution and aspect ratio**, ultrawide included: the 3D view widens instead of stretching, the HUD sits
  at the screen's edges, menus and movies keep their shape, and 2D graphics stay sharp.
- **High frame rates.** The game logic keeps its original 30 updates per second (so the riding feels exactly
  the same), and the picture is interpolated in between up to your display's refresh rate.
- **Full draw distance** instead of the original's pop-in, and your own music in CD quality.
- **Everything optional:** `vanilla = 1` gives the original's look and behaviour.

## Status

The first release is in preparation. Windows (64-bit) and Android are the targets; it also builds for Linux
and macOS. Problems and questions: [Issues](../../issues). If the game crashes, attach `OpenFreestyleBMX.log`
from next to the program (on Android, from the app's files folder).

## For developers

How the port works, the verification harness and the build: [docs/NATIVE.md](docs/NATIVE.md),
[docs/PORTING.md](docs/PORTING.md), [docs/PLATFORM.md](docs/PLATFORM.md), [CONTRIBUTING.md](CONTRIBUTING.md) and
[PLAN.md](PLAN.md); the session handover is in [docs/HANDOFF.md](docs/HANDOFF.md).

```sh
cmake --preset native64 && cmake --build --preset native64   # Windows x64 (llvm-mingw, SDL3)
android/build.sh                                             # Android (NDK r29, SDL3 source)
tools/check.sh                                               # format, lint, differential tests
```

## Legal

Open Freestyle BMX is an independent fan project, not affiliated with or endorsed by the game's publisher or
developer. It contains none of the game's files; you need your own copy of the game. The source includes the
original program's built-in text and tables (menu strings, physics and scoring tables), which the rewritten code
needs to behave identically. The names of the game and its soundtrack belong to their owners.

The source code is released under the [MIT licence](LICENSE).
