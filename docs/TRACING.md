# Recording gameplay traces

The hook DLL can record a trace of a play session on real Windows. Traces drive replays later (golden
traces): they hold every `timeGetTime` result and, per controller poll (one per gameplay frame), the raw
pad input, the random seed, and the rider and camera state.

## Record

1. Build the hook preset (`cmake --build --preset hook`) and copy `build/hook/dinput.dll` next to
   `NgBMX.exe` in the game folder.
2. Create an empty marker file in the game folder:
   - `bmxtrace.orig`: **the original game** (golden traces). Only the controller poll, the trace hook
     point, runs our (verified identical) code.
   - `bmxtrace.on`: our port (every ported function), to compare against the original later.

   If both exist, `bmxtrace.orig` wins. Without a marker the DLL runs the port and records nothing.
3. Play. Each launch writes `bmxtrace-<tick>.bin` into the game folder (roughly 200 bytes per frame,
   about 40 MB per hour at 60 fps).
4. Delete the marker file when done, and remove `dinput.dll` to go back to the plain game.

Good sessions to record: a few runs per level, one with lots of tricks and grinds, a few crashes and
bails, a run with a second controller connected, and a menu-heavy session (career, options).
Note the settings used (resolution, controller type) alongside the files.

## Check

```sh
python3 -I tools/trace_dump.py bmxtrace-123456.bin --summary
python3 -I tools/trace_dump.py bmxtrace-123456.bin --frames 0:20
```

Keep traces in `work/traces/` (gitignored). They contain no game assets, but they are large.

## Limits (version 2)

- Only `contrlerPoll` input is recorded. Keyboard input to the menus goes through other paths and isn't
  captured yet.
- `timeGetTime` calls from other threads (sound) can't be replayed. Before the first controller poll they are kept, tagged `t`; after it they are dropped (the sound thread spins on it: 1.4 GB per minute otherwise).
- Rider checkpoints cover state, surface, state levels, flags, timer, position, velocity, forward, up,
  rotation and spin for all four rider slots; the camera checkpoint is eye and look-at of the selected
  camera.
