# opendmfbmx

A work-in-progress reimplementation of *Dave Mirra Freestyle BMX* (PC, 2000) in C17, heading for SDL3
on Windows, macOS, Linux and Android.

This repository contains **no game code or assets**. You need your own copy of the game. The port
reads everything from the original installation.

New session or machine? Start with `docs/HANDOFF.md`.

Status: hook phase. Ported functions are verified bit for bit against the original executable, which
runs in an emulator. See `PLAN.md` for the strategy and `CONTRIBUTING.md` for the porting rules.

```sh
nix develop
tools/setup-work.sh /path/to/install/NgBMX.exe   # verify hash, export Ghidra bundles (needs Ghidra)
tools/check.sh                                   # format, build, lint, differential tests
tools/progress.py                                # porting progress
```
