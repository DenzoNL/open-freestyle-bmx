# High frame rate: fixed tick, interpolated rendering (P6 enhancement)

Decided 2026-10-31: the user took the recommendation for every decision H1..H10 (default off until HF4 is clean
and the user has played it). Order: HF0 first.

Status 2026-11-01: **HF0-HF3 done, HF4 runs done** (not committed at the time of writing), see "HF1-HF3
implementation" and "HF4 runs" at the end. `[video] interpolate` stays 0 by default until the user's playtest
(HF6, `native-run2\fpstest.ps1`). Earlier: HF0 (measurement) corrected P3 and P4 and added rules for HF1-HF3, see
"HF0 findings". Scope: the native exe (`bmxnative.exe`) only; the hook
DLL and the oracle tests stay as they are. Rules that apply (PLAN.md Decisions): vanilla stays vanilla, enhancements
attach at seams, options are tagged visual-only or gameplay-changing. Everything here is meant to be **visual-only**:
the simulation keeps its tick and stays bit-identical (BMX_SIM_HASH equal with the option on and off).
Recommendations are marked **rec**. The model is the reference from another port of that era: logic stays at 30
ticks per second, and character, camera and animation motion are interpolated between the two latest ticks, so the
picture trails by one tick.

## What the game does today (facts)

**The loop is ours.** `src/native/main.c` `main()` does the job of WinMain (0x48a460, never ported): PeekMessage,
otherwise the frame limiter, then `zrwOnIdle` (0x48b570) and `zrwDrawFrame` (0x48b690), one after the other, always as
a pair. The limiter (copied from the original):

- `gRateA` = `gZrwFrameLimit` (0x69771c), set by zrwSetParameter(0) (0x48ab10). `gRateB` (0x54d05c) is the .rdata
  0.0f: while the limit is 0 (before ngifInitialize) nothing is limited. `gRateC` (0x54dbb8) is the numerator (ms
  per second; the value is consistent with the trace, read it from .rdata in HF0 to be sure).
- A frame runs when `!(gRateC / gRateA > elapsed)`, with `elapsed` from timeGetTime (1 ms, `bmxTimeGetTime`). At a
  limit of 30 that is **elapsed >= 34 ms (29.4 Hz)**, at 60 **>= 17 ms (58.8 Hz)**: the ~34 ms per frame of the golden
  trace. Then `gLastTick` (0x67275c) `+= elapsed`: a late frame shifts the schedule, there is no catch-up.
- The wait is a busy loop (no Sleep): one core at 100%.

**The tick rate is 30 (29.4 Hz) in a level, 60 (58.8 Hz) in the frontend.** The game's `fov` locals are the limiter
(docs/WIDESCREEN.md): ngifInitialize (0x45b270) sets 60, the level load in ngifRunFrame (0x45b8a0) sets 30, the
return to the frontend sets 60 again. The run timer counts ticks: `gGameTimer = gGameRideSeconds * 30` (game.c).

**The simulation is frame-locked, not dt-stepped.** zrwOnIdle measures `dt` from zrwTimeGetTime (0x489cb0) and passes
it to ngifRunFrame, which only adds it to `gNgElapsed` (0x5ab454); the only reader is the debug getter
ngiffxGetDebugFloat (0x45c6d0). Physics, camera, motion playback and the overlay advance one step per call. A PC that
cannot keep 29.4 Hz plays in slow motion. The controller is polled once per tick (`zpadPoll` in ngifRunFrame; the
golden trace's checkpoint is per contrlerPoll, i.e. per tick).

**One tick** (gNgMode 0x5ab3a0 == 2, a level): update list 7; ngifRunFrame: ngiffxAdaptQuality, zpadPoll,
cameraPlaybackActive, ngifResetOrderTable (0x463820: clears the PSX ordering table, sets `gNgifFlushPending`
0x5d24fc), actionUpdate (physics, cameraUpdate 0x405780 -> cameraApply 0x407470, overlay), ngappUpdateSun,
zactorSyncRiderActors (0x4659e0 -> zactorSyncPose 0x4654b0), bmxSoundUpdate; then list 8: zobjUpdateAll (0x487020),
ngiffxAnimateSkids (0x45f650), ztimerUpdate (0x4a4280).

**One draw** (zrwDrawFrame 0x48b690): list 0; per camera: list 1, zrwtxrFlush, list 2, list 5, list 4; list 3;
RwRasterShowRaster. Lists are `gZrwUpdateLists` (0x672768, 9 lists, zrwRegisterUpdateFunction 0x48ba30). Known nodes:

| List | Callback | Mutates state? |
|---|---|---|
| 1 | zrwModelRender 0x48fbc0 (sky: moved to the camera, drawn) | its own frame, recomputed each draw |
| 1 | ngifpsxUpdate 0x45fcf0 (screen-shake script: `gNgifScript` 0x5ab4f0 += 2 per call) | **yes, per draw** |
| 2 | ngifFlushFrame 0x463860 / ngifFlushFrame2 0x4639a0 (draw the OT if pending, then clear pending) | **yes**: a second draw in the same tick shows no HUD |
| 2 | zrwworldUpdateEffects 0x495bd0 -> each effect type's +0x20, which is its **render** (ngiffx*Render) | no (HF0: the particle step is in the tick) |
| 2 | zrollerShadowDraw 0x49b3d0 (draws the shadow, clears `ready`) | yes |
| 2 | ngappDrawDebug 0x45a4d0 (debug faces) | no |
| 5 | zrollerUpdateShadow 0x49b520 (ray probes around the rider, sets `ready`: drawn one draw later) | yes, **and simulation state** (HF0: gZrwColBox, RW lightFrame) |
| 4 | zprofileUpdate 0x4a35f0, zrwdbg DebugUpdateFunction 0x47f660 | debug |

**Wall time vs tick count.** Tick-counted: everything in the simulation (physics, camera easing, motion playback,
gGameTimer, the overlay's fades). Wall-time (timeGetTime): particles (zobjPEmitUpdate 0x4954b0 and its copy
ngiffxUpdateEmitter 0x45e4e0 step by the ms since the last update **plus one extra `vel` per call**, a quirk, so the
particle speed depends on how often it is called; ngiffx *Render fade by age; zrwParticleAccelerate 0x497650), the skid
texture animation (`1000 / fps` ms), ngiffxAdaptQuality 0x45c710 (counts ticks per 100 ms; below 26 Hz it lowers
`gFxQuality` 0x55bd08), the sound pump (16 ms winmm timer thread -> winchanPump 0x46e010, independent of the loop),
movies (own loop), ztimerUpdate (no ported code adds a timer). The particle spawners draw from the simulation RNG
(randomSigned -> zmathRandom 0x481f80, the seed hashed by BMX_SIM_HASH).

**Where the per-tick pose lives** (all written in the tick, read by the draw):
- Model objects: `ZObj` (0x3c, pool `gZobjPool` 0x6722d4, type 0 = model, `flags & 7`) `pos`/`rot`; its `ZrwModel`
  (`ZObj.model`, 0x8c) `position` 0x18, `scale` 0x28, `rotation` 0x38 (quat), `activeLod` 0x48; per part of the active
  LOD a `ZrwPartDef` (= ZSkeletonNode, 0x44) with `offset` 0x08 and `rotation` 0x18 (quat), written by
  zrwmatSetNodeRotation (0x49a570) from zactorSyncPose.
- zrwModelUpdate (0x488650, reached through the slot `gZrwVtUpdate` 0x560e98 from zrwObjUpdate 0x488d60 in
  zobjUpdateAll) turns those into the RwFrame matrices: root = rotation x scale + position, parts = quat + offset;
  then RwFrameUpdateObjects, RpSkinClumpUpdate for skinned models, `onUpdate` (no setter caller in the port), and
  **copies the root into the collision clump's frame**. It is a pure function of the model's fields plus gZrwScale,
  the world centre and the mirror flag: same inputs, same bits.
- Camera: `gNgCamera` (0x68ff98, the one camera of the game) is a ZObj type 3; cameraApply -> zobjCameraLookAt
  (0x4822c0) sets `pos`/`rot` (cache invalidated by zobjCameraMoved/Rotated 0x4820f0/0x482110); in list 8
  zobjCameraDraw (0x4820c0) hands the view to the renderer (`gZrwCamSetView`). FOV through the slot 0x561d98 (the wide
  wrapper).
- The simulation reads frames back: part positions through the skeleton nodes' world matrices
  (zactorGetPartPosition 0x464b50 -> `gZrwVtNodeToWorld`), collision through the collision clumps. **RwFrames are
  simulation-visible state.**
- Cuts: cameraUpdate's `cam->changed == 1` jumps eye and look-at to the goals, zeroes `eyeVel` and calls
  zsndSrcClearDoppler; the line-of-sight cut after 20 blocked ticks; replay view changes (comdataSetView,
  zrollerStartCrossFade). Level load runs zrwOnIdle three times inside ngifRunFrame (no draw between).

## Decisions

### H1. Interpolate or extrapolate

- **A (rec): interpolate between the two latest ticks, one tick behind** (the reference). Draw at time t with
  `alpha = (t - t_N) / T`, T = the tick length (34 ms in a level), clamped to [0, 1]: pose = lerp(N-1, N, alpha).
  Never shows a state the simulation did not reach. Cost: the picture trails by up to one tick (H7).
- B: extrapolate from N along the last delta (no added latency). Overshoots at every landing, bail and grind
  start (the rider sinks into the ground for a frame). Rejected.
- C: run the simulation faster (60 Hz ticks). Changes gameplay (camera easing, physics, timers, motion speed). Out of
  scope: that is not "original tick rate kept".

### H2. What is interpolated

| Thing | v1 | How |
|---|---|---|
| Rider, bike (root and every part) | yes | model inputs (ZrwModel position/rotation/scale, ZrwPartDef quats/offsets) |
| Other model objects: props, physics props, attached objects | yes | same, generic over the ZObj pool (children are already world-space in their model after zobjApplyParent) |
| Camera eye / orientation | yes | ZObjCamera pos/rot |
| Shadow | follows | ZObj `pos` interpolated too, so zrollerUpdateShadow probes under the drawn rider |
| Sky | follows | moved to the (interpolated) camera by zrwModelRender |
| FOV | v2 | through the slot 0x561d98; easing steps are small |
| Screen shake (ngifpsxUpdate) | no | stays per tick (H4) |
| Particles | no (v1), v2 option | stepped per tick; drawn at their tick position (H4, pitfall P3) |
| HUD, timer, menus, 2D | no | redrawn from the tick's ordering table |
| Lights, sun | no | static or slow |

Quaternions: nlerp with the sign flip (dot < 0 -> negate one), normalised in double, then the float store; positions
lerp in double. A pure maths file with unit tests (like `driver/widemath.c`).

### H3. The seam

`main.c` is native code, not a ported function: the scheduler, snapshot and restore live there and in a new
`src/native/hfinterp.c` (+ `hfmath.c`). No jump over a ported function is needed for v1.

- **A (rec): loop in main.c, inputs-level pose, restore by recompute.**
  1. **Tick** (unchanged condition, the original's limiter maths): `hfRestore()` (below), then `onIdle()` exactly as
     today. After it, `hfCapture()`: walk `gZobjPool` (zobjUpdateAll's walk), for every active type-0 object store
     {ZObj pos/rot, ZrwModel position/rotation/scale/activeLod, the active LOD's part quats and offsets} keyed by
     (ZObj, ZrwModel, clump) pointer; the camera's pos/rot. Previous = the last capture.
  2. **Draw** (as often as `max_fps` and the deadline allow, H5): `hfApply(alpha)` writes the lerped values raw into
     those same fields (no game setter: zobjSetPosition would fire `moved` hooks), then calls `gZrwVtUpdate(model)`
     (zrwModelUpdate through its slot) per model and, for the camera, `cache->valid = 0` + zobjCameraDraw. Then the
     draw (A4). Poses stay applied between draws; the next draw overwrites them.
  3. **Restore** (once, right before the next tick): copy the captured tick-N values back, `gZrwVtUpdate` per model
     and zobjCameraDraw again (same function, same inputs: the frames, the skin matrices and the collision clump frames
     are the tick's bits again), then sync the dirty frames as the camera's begin-update does (`rwFrameSyncDirtyList`,
     src/rw/math.c, called from src/rw/camera.c), so the next tick starts with clean frames and world ties as vanilla
     does after its draw (pitfall P5).
  4. **Which draw function.** The first draw after a tick calls the real zrwDrawFrame (every callback runs once per
     tick, as in vanilla: shake script, particle step, shadow probes). Extra draws call `hfDrawFrame`, a native copy of
     zrwDrawFrame (~60 lines, like wideDrawList is a copy of ngifDrawList) that runs the same lists with a filter on
     `node->fn`: ngifpsxUpdate (jump 0x401a4b) skipped (the offset it set stays); the two flush nodes (0x402207,
     0x4020f4) run with the pending flag re-armed to what it was before the tick draw and cleared after; the effects
     node (0x495bd0) replaced by a draw-only walk (HF0 finds the step/draw split of the type update at +0x20); the
     zrwdbg / zprofile nodes skipped; everything else as is. The present goes through the driver's uncounted path
     (like drvGpuPresentMovie), so `drvGpuShownFrames` (BMX_KEYS `f<N>`, BMX_SHOT_FRAME, BMX_QUIT_FRAMES) keeps
     counting tick frames.
- B: interpolate at the RwFrame level (snapshot every clump's frame matrices after the tick, lerp matrices,
  RwFrameUpdateObjects + RpSkinClumpUpdate per draw, restore bytes). Catches frames nobody models, but matrices with
  scale need decomposing, the skin matrices and RW's dirty / tie state must be restored by hand, and ZObj `pos` (the
  shadow) is missed. More fragile than recompute.
- C: snapshot at zactorSyncPose / cameraApply via jumps. Covers only the current rider and bike (zactorSyncRiderActors
  syncs those two) and the camera; props stay at 30 Hz. Rejected.
- Only in a level: `gNgMode == 2` and limit 30. The frontend is pure 2D at 58.8 Hz (nothing to interpolate): the
  loop behaves as today there, as during loading and movies (their own loops).

### H4. Render-side state the game mutates (pitfalls)

- **P1 Screen shake.** ngifpsxUpdate advances its script per draw; at 144 Hz it would run 5x faster. Kept per tick
  (A4). v2: interpolate the offset between script steps.
- **P2 HUD.** ngifFlushFrame clears `gNgifFlushPending` after the tick draw; without the re-arm the extra draws show
  no HUD (flicker). Check in HF0 that ngifDrawList (0x462350) / ngifDrawPrimitive do not modify packets (draw the OT
  twice in one frame, compare). The wide HUD copy (`wideDrawList`) works per draw already.
- **P3 Particles.** (HF0: wrong premise, the step runs in the tick; see "HF0 findings".) The step is wall-time plus one `vel` per call, run from the draw: extra steps make particles fly
  faster. A4 steps once per tick and only draws in extra draws. Particles are then at tick N while the rider is at
  N-1+alpha: sparks lead the pegs by up to one tick. v2: draw a moving particle at lerp(prev, pos, alpha)
  (`prev` is the position before the last step; spark streaks need the step before that, one more field in our
  copy of the draw, not in the game's struct). **Spawns must stay out of the draw:** if a type update spawns
  (randomSigned -> zmathRandom), extra calls would move the RNG and break the simulation: HF0 checks the seed
  around every draw (P8).
- **P4 Shadow.** Vanilla draws the shadow probed in the previous draw (list 5 after list 2). With ZObj `pos`
  interpolated it follows the drawn rider. The probes are ray casts (zcollide): HF0 checks they leave no cache the
  physics reads.
- **P5 Frames are simulation state.** Physics reads part positions and collision clumps from RwFrames; the restore
  (A3) must leave them bit-identical, and RW's world ties (atomics re-tied to sectors at frame sync) must match: if
  any collision query walks RpWorld sector lists, their order depends on the sequence of syncs. HF0: find whether
  ngifzcollide / zrwcol use RW sector ties; the guard (P8) hashes the frames and the tie lists before and after.
- **P6 LOD, PVS.** rpLodAtomicSelectLevel (0x505b30) and RpPVSSetViewPosition use the camera of the draw: render-only
  state, may change per draw (visual only; covered by the hash).
- **P7 Sound.** cameraApply's listener (bmxSoundPlayAt 0xfa1) and bmxSoundUpdate run in the tick from the tick's
  camera: unchanged. The 16 ms pump thread mixes only (winchanPump); it must not read the camera or a pose while a
  draw has them interpolated. The camera-to-listener transform zsystemToListener (0x45a340) is called from zsndsrc.c
  (sound sources, updated from bmxSoundUpdate on the main thread); HF0 confirms no pump-thread caller.
- **P8 Unknown mutators.** Everything above is from reading; the gate is empirical: `BMX_HF_GUARD=1` hashes the
  BMX_SIM_HASH fields, the RNG seed, every captured pose and every model's frames, skin and collision frames before
  `hfApply` and after `hfRestore` and logs the first difference with the draw number.

### H5. Pacing, vsync, VRR, CPU

- Ticks keep the original condition and time source. Draws in between are scheduled from SDL's ns clock: the next
  draw at `max(last_draw + 1/max_fps, now)`, **never started if it would end after the next tick's due time**
  (estimated from the last draws' cost, plus the present's wait). A late tick changes nothing in the hash but lowers
  the tick rate (no catch-up) and, under 26 Hz, ngiffxAdaptQuality's quality; this must not happen because of us.
- In this mode the wait sleeps (SDL_DelayPrecise up to ~1 ms before the deadline) instead of spinning: less CPU and
  power than vanilla. The vanilla busy wait stays as it is (PLATFORM.md: sleeping there is a behaviour change).
- Present mode: today SDL GPU's default (VSYNC; `drvGpuStart` sets none). `vsync = 1` keeps VSYNC, `0` asks for
  IMMEDIATE (MAILBOX when IMMEDIATE is not supported; SDL_WindowSupportsGPUPresentMode). With VSYNC the swapchain
  acquire can block up to one refresh: the deadline estimate must include it. VRR (G-Sync / FreeSync): VSYNC on with
  `max_fps` a few below the refresh (e.g. 141 at 144 Hz) gives no tearing and no queue. Vanilla 29.4 Hz is below
  most VRR ranges (the driver's frame doubling handles it).
- `max_fps = 0` means the display's refresh rate (SDL_GetCurrentDisplayMode), so 120 / 144 / 165 Hz need no setting.

### H6. Discontinuities (draw tick N, no interpolation)

- **Per object:** position jump over a threshold, or rotation over 90° in one tick (spins: about 24° per tick at
  720°/s, so they still interpolate), or a new / reused pool slot (key not in the previous capture, or a different
  clump). Covers the respawn after a bail, the replay start and restart, props reset.
- **Camera:** eye jump over a threshold or a turn over 60° in a tick; also when the eye moved by more than its own
  `eyeVel` (0x0 after a cut, cameraUpdate sets `eyeVel = eye - old eye` otherwise). Covers chase cuts, replay view
  cameras, the attract demo's orbit start.
- **Global:** gNgMode changed, a level was loaded (ngifRunFrame's three nested zrwOnIdle), the limiter value
  changed, the pause menu opened or closed (no motion anyway), a tick later than 2 T (alpha clamp).
- Thresholds as constants, measured in HF4 from logged per-tick maxima of normal riding (BMX_KEYS runs plus a real
  run); not ini keys.

### H7. Input latency

Vanilla: the tick polls the pad, the draw right after shows tick N (plus up to one refresh of vsync). Interpolated:
tick N is fully on screen one tick later, so **about +17 ms on average, +34 ms at most** at 29.4 Hz, minus what the
faster refresh saves (a 144 Hz present waits at most 7 ms instead of 33 ms for the next 30 Hz frame). Polling later
does not help: the tick's input sample time is the tick's. Trade-off for the user; the option is off by default
(H8). An extrapolation mode (H1-B) is the only way to remove it and is rejected.

### H8. Config keys and default

Integers, `[video]`, added to `kKnownKeys` (src/platform/config.c):

| Key | Values | Vanilla default |
|---|---|---|
| `interpolate` | 0 off (main.c loop as today), 1 interpolate between ticks in levels | 0 |
| `max_fps` | 0 = the display's refresh rate, n = cap (ignored with `interpolate = 0`) | 0 |
| `vsync` | 1 VSYNC (today), 0 IMMEDIATE / MAILBOX | 1 |

Dev aids (environment, like BMX_SIM_HASH): `BMX_HF_GUARD=1` (P8), `BMX_HF_ALPHA=1` (always draw tick N: the seam
path without motion, for byte-identity, H9.3), `BMX_HF_LOG=1` (draws per tick, missed deadlines, snaps).

**The user's call: default.**
- **Vanilla default (`interpolate = 0`):** zero risk, the original's feel and latency; 29.4 Hz on a 144 Hz screen.
- **Exception pattern (`interpolate = 1`, like dpad_steering / smooth_resample):** smooth on every modern display.
  Cost: +17 ms average latency on a trick game, particles a tick ahead of the rider, HUD at 29.4 Hz over a smooth
  scene, any missed mutator is seen by everyone.
- **Rec:** default 0 until HF4's hashes are clean and the user has played two levels with it, then decide.
  `vsync = 1` and `max_fps = 0` either way. Tag: visual-only (replays and saves record nothing).

### H9. Verification

1. **Off is untouched:** `interpolate = 0` runs today's loop (log `hf: off`); BMX_SHOT frames byte-identical to the
   build before.
2. **Simulation identical:** BMX_SIM_HASH files equal tick for tick between `interpolate = 0` and `= 1` at
   `max_fps` 60, 144, 240 and 0, `vsync` 0 and 1, with the WS4 script (ProQuest into Vail Trails, riding, pause, END
   RUN?, results, replay) plus a bail and respawn, a level reload, the attract demo and a Session run. BMX_KEYS stays
   keyed on tick frames (H3-A4), so the same script drives both.
3. **The seam is transparent:** `BMX_HF_ALPHA=1` with `interpolate = 1`: BMX_SHOT tick frames byte-identical to
   `interpolate = 0` (proves capture / apply / restore / hfDrawFrame change no pixel when nothing moves).
4. **Guard:** BMX_HF_GUARD clean over all runs of 2. (no difference between before-apply and after-restore).
5. **Unit tests** (tests/platform style, no window): nlerp / lerp maths, the snap rules, and the scheduler: for a
   synthetic clock the tick decisions equal the vanilla limiter's (same ms, same `gLastTick`), and no draw is
   scheduled past a tick's due time.
6. **Golden traces:** need the hooked game (docs/TRACING.md); not runnable natively, as in WS4.
7. **Playtest (user):** two levels at 144 Hz with a pad: grinds (sparks), bails, the replay cameras, pause.

### H10. Slices

| Slice | What | Size | Model |
|---|---|---|---|
| HF0 | Research with instruments, no feature: dump every update-list node per mode at run time; BMX_HF_GUARD skeleton (seed and sim hash around each draw, a second zrwDrawFrame per tick as a test); find the step / draw split of the effect type update (P3), whether zcollide / zrwcol use RW sector ties (P5), shadow probe caches (P4), ngifDrawList re-entry (P2); read gRateC | ~150 + notes | Opus |
| HF1 | Scheduler in main.c behind `interpolate`: original tick condition, draw pacing, deadline guard, sleep, `max_fps` / `vsync` keys, uncounted present in gpu.c, present mode | ~200 | Sonnet |
| HF2 | `hfinterp.c` + `hfmath.c`: capture / apply / restore over the ZObj pool and the camera, snap rules, unit tests | ~350 | Opus |
| HF3 | `hfDrawFrame` (native copy of 0x48b690 with the node filter), flush re-arm, draw-only effects | ~200 | Opus |
| HF4 | Runs of H9.1-5 at all rates; thresholds from logs; fixes | runs | Sonnet |
| HF5 | v2 polish: particles at lerp(prev, pos), FOV and shake interpolation | ~150 | Sonnet (Opus review) |
| HF6 | User playtest, default decision (H8), docs (NATIVE.md, PLATFORM.md config list) | - | - |

HF0-HF4 give a complete interpolated mode in levels; HF5 is polish. HF0 decides whether A4's filter list is complete:
if the guard finds a mutator not listed here, it joins the filter before HF2 starts.

## HF0 findings (2026-10-31)

**Instrument.** `BMX_HF_GUARD=1..4` (src/native/hfguard.c, docs/NATIVE.md). When it is set, main.c hands each tick
to `bmxHfGuardFrame`; when it is unset nothing changes.

The sub-hashes only read memory:
- `sim`: the BMX_SIM_HASH fields;
- `rng`: the random seed;
- `frames`: per active type 0 object, ZObj pos/rot, the ZrwModel inputs, and the floats and flags of every RwFrame
  modelling and world matrix of its clump and collision clump;
- `ties`: the atomic order in every world sector's two tie lists;
- `colbox`: gZrwColBox;
- `colstate`: gZrwCol;
- `shadow`: the ready flag, the miss array and the vertex array;
- `particles`: every effect emitter's bounds and time stamp, plus its live particles;
- `hud`: both flush flags, gNgifPrim3D, gNgifScript and the 0x400 ordering table words;
- `camera`: gNgCamera pos/rot;
- `lightframe`: RwGlobals.lightFrame.

What each level does:
- **Level 1** hashes before and after every draw and times the loop.
- **Level 2** runs the draw through a copy of zrwDrawFrame with a checkpoint after every step and list node, which
  attributes each change.
- **Level 3** also runs, in a level, a second draw per tick. That draw skips the shake node, re-arms the flush flags,
  and runs no list 3 and no present. It then does an identity recompute of every model (zrwModelUpdate through its
  slot, in pool order) plus a frame sync.
- **Level 4** is level 3 with the tick's dirty frame order replayed before that sync.

**Runs.**
- From `native-run2` with `-r640.480.16`, `BMX_MUTE=1 BMX_NO_MOVIES=1 BMX_QUIT_FRAMES=3200`.
- BMX_KEYS: Enter x10 into ProQuest / Vail Trails (riding from tick ~932), then pedalling, steering, hops and
  E/S/F/D tricks.
- BMX_SIM_HASH, plus BMX_SHOT frames 300, 1500, 1800, 2200, 2600 and 3000.
- Display at 240 Hz.

**Byte identity.**
- The build with the guard compiled in and unset, against the build before (83d6985): all six frames
  byte-identical, and the sim file identical (2173 ticks).
- Every guard level 1-4 gave the same sim file and the same six frames as the run without the guard. That includes
  levels 3 and 4, which double the shadow probes and re-tie atomics.

### Q1. The limiter constants (from work/NgBMX.exe)

- **The constants.** `gRateC` 0x54dbb8 = **1000.0f** (0x447a0000) and `gRateB` 0x54d05c = 0.0f.
- **The tick condition.** WinMain 0x48a775..0x48a7cc computes `elapsed = timeGetTime() - gLastTick` as a uint32,
  zero-extended to a qword for `FILD`. Then `FLD 1000.0; FDIV gRateA; FCOMPP; TEST AH,0x41; JZ skip` skips while
  `1000 / limit > elapsed`.
  - So a tick runs at **elapsed >= 34 ms (limit 30)** and **>= 17 ms (limit 60)**.
  - Then `gLastTick += elapsed`.
  - main.c's copy is exact.
- **Measured.**
  - In a level, over 2168 ticks: interval 33.99 ms mean (max 34.0-35.2 ms per run), i.e. 29.4 Hz.
  - In the frontend, over 930 ticks: 16.99 ms, i.e. 58.8 Hz.
- **Other wall-clock constants.**
  - zrwOnIdle's `dt` is ms x 0x1.0624dep-10 (0.001). It only feeds the debug float.
  - Its fps counter uses 1000 ms windows.
  - ngiffxAdaptQuality uses 100 ms windows, thresholds 26 / 28 fps and a range of 60..200 (see "Wall-clock render
    state" below).
  - The skid animation steps every `1000 / fps` ms.

### Q2. What a draw changes (guard levels 1/2)

| Sub-hash | Changed by the draw? | Where (level 2) |
|---|---|---|
| sim, rng, particles, camera | **never** (3101 draws, up to 12 live particles in an emitter) | - |
| frames | 2000 of 2168 level draws | `camBegin`: RwCameraBeginUpdate's rwFrameSyncDirtyList writes the world matrices (the same values RwFrameGetLTM computes on demand) |
| ties | 568 of 2168 | `camBegin`: the sync calls WorldAtomicSync, which destroys and re-adds every synced atomic's ties with `linkFirst` |
| colbox, colstate, lightframe | every level draw | node 0x49b520 zrollerUpdateShadow (36 zrwcolQueryLine probes) |
| shadow | every level draw | 0x49b520 (probes), 0x49b3d0 (the draw clears `ready`) |
| hud | every draw | 0x402207 ngifFlushFrame (level), 0x4020f4 ngifFlushFrame2 (frontend): the pending flag only |

**The draw neither spawns particles nor advances the RNG.** The particle step is not in the draw at all:
- The effect object's ops (0x562098) have `update` = 0x495a10 zobjPEmitObjectUpdate. zobjUpdateAll runs it in the
  tick (list 8).
- It calls each emitter class's +0x1c, which is zobjPEmitUpdate 0x4954b0 (ngiffxUpdateEmitter 0x45e4e0 for one
  type).
- List 2's zrwworldUpdateEffects calls +0x20. In all seven tables at 0x55c278 + 0x38n, +0x20 is a render function:
  `ngiffx*Render` 0x45cc50, 0x45d420, 0x45d730, 0x45db20, 0x45e010, 0x45e2d0, 0x45eaa0.
- Rendering reads timeGetTime only for the fade.
- So the "+1 vel per call" quirk happens once per tick, and an extra draw cannot speed particles up.

**The shadow probes leave state the physics reads.**
- zrwcolQueryLine widens gZrwColBox by 1e-5 on every call, before anything else. The box still grows at these
  magnitudes: it changed on every draw.
- The box is read by every zrwcolQuerySphere (ngifzbsp, `zrwBBoxContainsPoint`) and by line queries with flag 0x20.
- The probes also advance RwGlobals.lightFrame. That is the 16-bit collision tag object queries compare
  `collisionTag` against, so the probes move its wrap point.
- gZrwCol is scratch: each query rewrites it.
- What matters is the **number** of probe calls per tick, not their inputs: the widening is unconditional.

**Known mutators.**
- Confirmed: the flush flags, the shadow's `ready` and the dirty-frame sync.
- The shake script was not triggered in these runs (no change at 0x401a4b), so its effect stands as read from the
  code.
- No other draw-time mutator showed up. The ordering table, gNgifPrim3D and the shake script pointer never changed.

### Q3. Collision and RenderWare's sector lists

**Collision reads the tie lists.**
- ngifzcollide calls zrwcolLineTest with flags 3 (world + solid objects); ngifzbsp calls zrwcolSphereTest with
  flags 3.
- Both go through `rwWorldIntersectObjects` -> `_rpCollWorldAtomics*`, which walks each sector's
  `collAtomicsInWorldSector` in list order.
- The order affects the results:
  - zrwcolKeepNearest keeps the later of two hits at equal distance (`distance > nearest` rejects only farther
    ones).
  - zrwcolRecordHit appends sphere hits in walk order.
- So the tie order is simulation-visible in principle. Only syncs set it (rwFrameSyncDirtyList, from the camera's
  begin-update in the draw).
- The shadow probes use flags 1 (world triangles only) and never walk the tie lists.

**Recompute and sync order, measured.**
- **Level 3** (identity recompute in pool order plus a sync):
  - The frame floats and flags stay bit-identical (0 changes).
  - The tie order **changed on 1999 of 2169 ticks**.
  - The RwMatrix padding words differ, because zrwModelUpdate copies its shared scratch matrix.
- **Level 4** (the same recompute, with the dirty list put back in the order the tick queued it, recorded after
  onIdle):
  - The ties changed **0 times** and the frames 0 times in 2169 ticks.
  - One dirty root per tick is not in the tick's list. Level 4 appends it after the recorded ones, and the ties
    still matched.
- The sim hash stayed identical even with level 3's reordered ties. These runs never hit a case where the order
  decides, but HF2 must not rely on that.

### Q4. Drawing the HUD twice; the effects split

- **Drawing the HUD twice is safe.**
  - ngifDrawList and ngifDrawPrimitive only read the packets.
  - Level 3 drew the ordering table a second time per tick with the flags re-armed, over 2169 ticks. The only `hud`
    changes were the re-arm and the flush clearing it again. The OT words, gNgifPrim3D and the script pointer did not
    change.
  - One reader is not idempotent: ngifDrawAreaPrim 0x45fe50 (packet code 0xf8) advances gNgifPrim3D by 0x80 bytes per
    draw. These runs never reached it, and ngifResetOrderTable resets the cursor every tick, so it is harmless.
- **The effects split is clean:** the step is in the tick (zobjUpdateAll, class +0x1c), the draw in list 2 (+0x20
  render).
  - HF3 needs **no draw-only walk**: the effects node runs as it is in extra draws.
  - Only the render fade moves with wall time, which is visual.

### Q5. Pacing (level 1: the draw timed alone; 640x480 on the user's PC, 240 Hz display, BMX_KEYS window)

| | ticks | interval | onIdle | draw (mean / max) | swapchain wait | slack (mean / min) |
|---|---|---|---|---|---|---|
| level | 2168 | 33.99 ms | 0.16 ms | 1.48 / 6.03 ms | 0.02 ms | 32.3 / 27.8 ms |
| frontend | 930 | 16.99 ms | 0.47 ms | 1.01 / 43 ms (loading) | 0.37 ms | 15.5 ms |

- **Draws per tick.** A 34 ms tick spans 4.1 refreshes at 120 Hz, 4.9 at 144, 5.6 at 165 and 8.2 at 240. That leaves
  room for 3, 4, 4-5 and 7 extra draws per tick at ~1.5 ms each (6 ms worst seen).
- **Deadline guard.** The worst draw is close to a 165 Hz period (6.06 ms), so HF1's guard matters from 165 Hz up.
- **VSYNC.** At 30 presents per second, VSYNC presents did not block (0.02 ms). With 4-8 presents per tick they will,
  so the deadline estimate must include the acquire, as H5 says.
- **Resolution.** This 640x480 run does not show the GPU cost of larger modes. HF4 measures at the user's resolution.

### Wall-clock render state (frames that differ between builds)

**No tree or foliage model is animated by the wall clock.**
- ngiffxFindSkidObjects finds **0 "ANIM" objects** in Vail Trails (the guard logs the count).
- zrwModelRender and the world render read no clock.
- The only clock readers in the draw are the particle renders' fade.

**The wall-clock input that changes the picture is ngiffxAdaptQuality 0x45c710.** It runs in the tick, but
timeGetTime drives it.
- **What it sets.** It passes its quality to zrwSetParameter(5, camera, &q) (slot 0x561484 -> 0x48ab10), which calls
  zrwSetClipPlane 0x4929f0 = zrwCameraSetClip. So **gFxQuality is the camera's far clip, and the fog distance is
  `far - clipOffset`.**
- **The climb.**
  - At the level load the 100 ms window measures ~2.5 fps, so the far clip drops to 60.
  - It then climbs one 100 ms step at a time (every third tick) towards 200, and stops at the first value within 10
    of 200.
  - Measured: **60 at tick 932, 188.92 at tick ~1500, 190.0521 at tick ~1524**, then fixed.
- **Why runs differ.**
  - The values are the same in every run.
  - The tick each step lands on depends on the phase of the 100 ms windows against the ticks: the runs differed by
    one tick (1499 vs 1500).
  - So **frame 1500 of the ProQuest script falls in the middle of the climb.** A build or run whose load ends at a
    different phase draws it with another far clip and fog distance.
  - That shows on distant geometry at the fog / clip edge, which in Vail Trails is tree trunks and foliage.
  - The same explains the earlier "f1600 variants" (docs/WIDESCREEN.md).
- **Frames to compare.** Frames after the settle (tick ~1530, e.g. f1800) do not depend on it, unless a window
  measures below 26 fps.
- **Vanilla has the same sensitivity.** A single tick ~30 ms late makes a window of 3 ticks in ~130 ms (~23 fps).
  The adapter then steps by (fps - 28) x 10 ≈ -50: a visible far-clip pop that takes ~20 s to recover.

### Pitfalls: confirmed / ruled out

- **P1 shake:** as read from the code (not triggered in the runs). The filter stays.
- **P2 HUD:** confirmed. The re-arm works, and drawing the OT twice is clean (Q4).
- **P3 particles:** **premise corrected.** The step is in the tick; draws only render. Particles sit at their tick
  position, so "sparks lead the pegs by up to a tick" still holds for v1.
- **P4 shadow:** **confirmed, and worse than assumed.** The probes write simulation-visible state (colbox,
  lightFrame).
- **P5 ties:** **confirmed** (Q3). The recompute must replay the tick's sync order.
- **P6 LOD / PVS:** not hashed separately. Frames and ties covered what the simulation reads.
- **P7 sound:** confirmed by reading.
  - zsystemToListener 0x45a340 (zobjCameraGetMatrix(gNgCamera, 0)) has two callers:
    - zsndSrcUpdate, via bmxSoundUpdateStreams <- action.c / ngapp.c;
    - zsndSrcMeasure, via bmxSoundMeasure <- bmxSoundPlay*.
  - Both run on the main thread, in the tick. The pump thread does not read the camera.
- **P8 unknown mutators:** found **one**, the shadow probes' colbox / lightFrame. The particle question is resolved,
  and nothing else changed.
- **New P9, the far clip / fog adapter:** it is wall-clock state in the tick (see above). The scheduler must never
  make a tick late, or the far clip pops in the interpolated mode where vanilla would not.

### Plan changes for HF1-HF3

**HF1**
- The deadline guard is a correctness requirement, not polish: a tick late by tens of ms drops the far clip (P9) and
  shifts the 100 ms windows.
- Estimate the draw at the worst recent draw (~6 ms here), not the mean.
- Compare against the pacing line of `BMX_HF_GUARD=1`.

**HF2**
- Restore by recompute is right for the matrices: floats and flags come out bit-identical. The padding words do not.
  If byte-exact padding is wanted, restore the root modelling matrices raw (64 bytes per model).
- **Record the dirty-frame list (head to tail) right after onIdle.** At the restore:
  1. recompute the models;
  2. relink the dirty list in the recorded order, with frames not in the record after them;
  3. call rwFrameSyncDirtyList.

  Without this the sector tie order differs on most ticks. hfguard.c's `recordDirty` / `replayDirty` is the tested
  reference.
- The first draw after a tick already runs with interpolated poses, so its sync reorders the ties. The replayed
  restore sync puts the order back before the next tick.

**HF3**
- The effects node runs unchanged in extra draws: drop the "draw-only walk" item.
- The shadow probe node 0x49b520 must not run extra times as it is. Two options:
  - **rec:** in extra draws, save gZrwColBox (32 bytes), gZrwCol (0x50 bytes) and RwGlobals.lightFrame (2 bytes)
    before the node and restore them after. The shadow then follows the interpolated rider, and the simulation sees
    exactly one probe set per tick.
  - Or skip the node in extra draws and re-arm `gZrwShadowReady`, so the shadow stays at the tick pose.
- The first (real) draw's probes are the vanilla ones, so their count per tick does not change.
- Re-arm the flush flags as planned, but capture them after onIdle, which is where the tick sets them.
- **HF4:** run the guard at levels 1 and 2 on the interpolated build. Level 4's checks are the ones H9.4 asks for.

## Slice status

| Slice | Status |
|---|---|
| HF0 | done 2026-10-31 (`BMX_HF_GUARD`, findings above) |
| HF1 | done 2026-11-01: `[video] interpolate / max_fps / vsync`, the scheduler, the uncounted present |
| HF2 | done 2026-11-01: capture / apply / restore, nlerp / lerp, skip rules, `bmxhftest` |
| HF3 | done 2026-11-01: `hfDrawFrame` |
| HF4 | runs done 2026-11-01 (below); thresholds set; open: the user's display / resolution pacing, a bail run |
| HF5 | not started (particles at lerp, FOV and shake interpolation) |
| HF6 | open: the user's playtest (`native-run2\fpstest.ps1`), then the default (H8) |

## HF1-HF3 implementation (2026-11-01)

**Code.** `src/native/hfinterp.c` (loop, capture / apply / restore, `hfDrawFrame`), `src/native/hfmath.c/.h` (pure:
limiter, due time, scheduler, lerp / nlerp, angles, skip rules), `tests/platform/test_hf.c` (`bmxhftest`). Seams:
main.c calls `bmxHfStep` instead of its own loop body when `bmxHfInstall` says the mode is on (the off path is
untouched: `hf: off`); `drvGpuSetUncounted` in driver/gpu.c; `setPresentMode` in drvGpuStart (only with `vsync = 0`);
`bmxTimeNs` / `bmxTimeMsAt` in platform/kernel.c (bmxTimeGetTime is now `bmxTimeMsAt(elapsedNs())`, the same value);
three keys in config.c; `BMX_HF_GUARD` hooks in hfguard.c.

**HF1, the loop.**
- Outside a level (gNgMode != 2, a limit other than 30, or the debug PVS light on) `bmxHfStep` is the original loop:
  `hfLimiterTick` (the original arithmetic on the same clock, unit-tested against a verbatim copy of main.c's code),
  busy waiting, onIdle + zrwDrawFrame. Movies have their own loops and are not affected.
- In a level the tick condition is the same; between ticks an extra draw starts when its slot has come (last draw
  + 1 / max_fps) and the worst of the last 64 draw durations (start to end of present, the swapchain wait included)
  plus a 1.5 ms margin ends before the tick is due (`hfTickDueNs`: the instant the millisecond clock reaches
  `gLastTick + 34`). Otherwise the loop sleeps (`SDL_DelayNS`) to the next slot or to 2 ms before the tick, and spins
  the last 2 ms on the limiter.
- `max_fps = 0` takes the refresh rate of the window's display (SDL_GetCurrentDisplayMode).
- `vsync = 0` asks SDL for IMMEDIATE, else MAILBOX (log `driver: vsync off (immediate)`); `vsync = 1` sets nothing
  (SDL GPU's default VSYNC, as before). It applies with `interpolate = 0` too.

**HF2, capture / apply / restore.**
- **Which objects:** exactly those zobjUpdateAll (0x487020) updated in the tick, in its order: active non-attached
  objects of the pool, then their active children (zobjApplyParent) and grandchildren (zobjRefresh). Type 0 with a
  model are captured: ZObj pos / rot, ZrwModel position / rotation / scale, and the offset / rotation of every
  matched part of the active LOD (the ZrwPartDef nodes zactorSyncPose writes). Key for the previous capture:
  ZObj, ZrwModel, clump, LOD, model flags and the part def pointers.
- **The camera** (a change from H3-A): not the ZObj camera's pos / rot, but the view the tick's zobjCameraDraw handed
  to the renderer camera, `ZrwCamera.view` (rotation and eye). Apply interpolates eye (lerp) and rotation
  (zquatFromMatrix, nlerp, zquatToMatrix) and calls `gZrwCamSetView` (0x561da0) directly; the camera object and its
  matrix cache are never touched (the sound listener reads them in the tick). Why: the first version wrote pos / rot
  and invalidated the cache, and with `BMX_HF_ALPHA=1` the pause, END RUN?, results and replay frames differed from
  vanilla in 1000-6000 scattered pixels: there the tick leaves the cache's view stale against pos / rot, and the
  renderer gets the cached view. Taking the view from the renderer camera reproduces it exactly.
- **Rotation maths:** the game's rotations are quaternions (ZQuat x, y, z, w) everywhere that is interpolated, so
  nlerp: sign flip on a negative dot, normalised in double, stored as float. Positions and scale lerp in double. At
  alpha 0 / 1 the inputs' bits come back unchanged (no renormalisation).
- **Apply** (before every draw, the real one included): the values at alpha written raw into the fields (no setter,
  no `moved` hooks), then `gZrwVtUpdate` (zrwModelUpdate through its slot) for every captured model in capture
  order, then the camera view.
- **Restore** (before the next tick): the tick's values written back, every captured model recomputed through the
  slot (all of them, so the dirty list holds the same set the tick queued), the captured view handed to the renderer
  camera, the dirty list relinked in the order recorded after onIdle (frames not in the record after them;
  hfguard.c's tested `replayDirty`), then `rwFrameSyncDirtyList`.
- **Alpha** = (draw start - tick start) / 34 ms, clamped to [0, 1]: the real draw right after a tick shows
  about tick N - 1, the last extra draw about N - 0.2.

**HF2, skip rules (H6).** An object is drawn at the tick pose for that tick when it has no previous capture with
the same key (`new`), when the previous capture is unusable (`global`: first level tick, back from the frontend or
loading, or a tick interval over 2 x 34 ms), or when its model position moved more than `objectJump` or its root
rotation (model or ZObj) turned more than `objectTurn` in the tick. A part that turned more than `objectTurn` keeps
its tick rotation (the bike wheels at speed: interpolating them the short way would turn them backwards). The camera
keeps the tick view on `cameraJump`, `cameraTurn`, or a move while the selected camera state's `eyeVel` is zero
(cameraUpdate's jump to the goals). Thresholds below ("HF4 runs").

**HF3, `hfDrawFrame`** (a copy of zrwDrawFrame 0x48b690 for extra draws):
- the screen shake node (0x401a4b, ngifpsxUpdate) is skipped;
- the two flush flags are re-armed to their values after onIdle, before list 0;
- the shadow probe node (0x49b520, zrollerUpdateShadow) runs inside a save / restore of `gZrwColBox` (32 bytes),
  `gZrwCol` (0x50) and `RwGlobals.lightFrame`. Chosen over skipping it because the shadow then follows the drawn
  (interpolated) rider instead of stepping at 30 Hz under a smooth rider, while the simulation still sees exactly one
  probe set per tick (the real draw's, at the vanilla count; the widening does not depend on the probes' inputs).
  The probes use flags 1 (world triangles only), so no atomic's collisionTag is written.
- the effects node runs unchanged (HF0 Q4); list 4 (zprofile / zrwdbg debug nodes) is skipped; lists 0 and 3 run;
- the present goes through `RwRasterShowRaster` with `drvGpuSetUncounted(1)`: `drvGpuShownFrames` (BMX_KEYS frames,
  BMX_SHOT_FRAME, BMX_QUIT_FRAMES) counts ticks only, and no screenshot is taken from an extra draw.
- A failing node leaves its list as the original's runListAt does; with gZrwDebugLightOn set there are no extra
  draws.

## HF4 runs (2026-11-01)

Setup: from `native-run2`, `-r640.480.16` (one run at 3440x1440 windowed, `widescreen = 1`, `hud = 1`),
`BMX_MUTE=1 BMX_NO_MOVIES=1 BMX_QUIT_FRAMES=4000`, `nice -n 19`, the display at 240 Hz, the user possibly gaming.
BMX_KEYS: ProQuest into Vail Trails, riding with steering, hops and E/S/F/D tricks from frame 1500, Esc at 2700
(PAUSE), end run, yes (END RUN?), RUN FINISHED, down, down, view replay (replay cameras from ~3350):

```
f300:enter,+150:enter x9,f1500:up*400,+100:up+left*20,+40:up+e*10,+60:up*200,+20:right*15,+30:up+s*10,+80:up*300,
+20:f*8,+40:up+d*10,+60:up*300,f2700:esc,f2780:up,f2840:enter,f2920:down,f2980:enter,f3150:down,f3250:down,f3350:enter
```

BMX_SHOT frames 300 (menu), 1500 (riding, mid far-clip climb), 1800, 2200, 2600 (riding), 2760 (PAUSE), 2960 (END
RUN?), 3100 (RUN FINISHED), 3500, 3900 (replay). Base: the build of 91cb83d. BMX_SIM_HASH: 2973 ticks.

| Run | Sim hash vs base | Frames vs base |
|---|---|---|
| `interpolate = 0` (three runs over the slices, the last on the final build) | identical | 10 / 10 identical |
| `interpolate = 1`, max_fps 144 | identical | differ (interpolated), f300 identical |
| `interpolate = 1`, max_fps 120 | identical | differ (interpolated) |
| `interpolate = 1`, max_fps 144, `vsync = 0` | identical | differ (interpolated) |
| `interpolate = 1`, max_fps 0 (240 Hz) | identical | differ (interpolated) |
| `interpolate = 1`, 3440x1440 `widescreen = 1`, `hud = 1`, max_fps 0 | identical | (other size) |
| `interpolate = 1`, max_fps 144, `BMX_HF_GUARD=1` | identical | differ (interpolated) |
| `interpolate = 1`, max_fps 144, `BMX_HF_ALPHA=1` (four runs, the last on the final build) | identical | **10 / 10 identical** |
| `interpolate = 1`, max_fps 144, `vsync = 0`, `BMX_HF_ALPHA=1` (three runs) | identical | 10 / 10 identical to `interpolate = 0, vsync = 0`; see "vsync = 0" below |

**Guard** (`BMX_HF_GUARD=1` with `interpolate = 1`, 10942 extra draws in 2971 level ticks): sim, rng, colbox,
colstate, particles, hud, camera (the camera object) and lightframe were changed by **0** extra draws; only the
render state changed (frames: the sync of the interpolated poses, ties, the shadow). Every one of the 2970 restores
gave back the capture bit for bit (object pos / rot, model fields, the floats and flags of every clump and collision
clump frame, the part defs, the camera object's pos / rot and valid cache matrices, the renderer camera's view and
world matrices and the floats of its RwCamera frame).

**Pacing** (640x480; draws per tick = the real one plus the extras):

| max_fps | draws per tick (ticks) | extra draws past the due time | late ticks (> 1 ms) |
|---|---|---|---|
| 120 | 4: 2828, 3: 77 | 0 | 11 of 2971 (max 10.4 ms: one sleep woke 10.4 ms late) |
| 144 | 5: 2268, 4: 637 | 0 | 10 (max 5.4 ms) |
| 144, vsync 0 | 5: 2256, 4: 649 | 1 (1.5 ms) | 15 (max 4.1 ms) |
| 0 (240 Hz) | 7: 2698, 6: 188, 8: 19 | 0 | 10 (max 9.3 ms) |
| 0, 3440x1440 | 7: 2622, 6: 99, 3-5: 174 | 0 | 6 (max 11.0 ms: a sleep woke 11 ms late) |

64 ticks per run have the real draw only (the first level ticks, before two captures exist, and ticks after a
long draw); the worst single draw is 26-29 ms (the first one in a level). Late ticks are rare and come from late wake-ups of `SDL_DelayNS`
(`nice -n 19` with other load on the machine), not from draws; none is near the ~30 ms that would move the far clip
(P9), and the far clip path of the vanilla runs was reproduced (the BMX_HF_ALPHA=1 frames at f1500, mid-climb, are
identical).

**Thresholds** (per-tick motion of every matched object and the camera over the whole run; game units):
- Objects: 229032 of 246499 below 1/16 (static props), riding up to 64, the Vail Trails train cars 40 (up to 80
  when it speeds up); above that only teleports: the respawn after the level intro (4634 units) and the train
  wrapping (11960), both >= 2048. **objectJump = 500.**
- Object root turns: up to 33 degrees per tick in riding and tricks (spins are ~24); the respawn turns 89 degrees.
  **objectTurn = 90** (H6). Part turns up to 90 occur (886 part turns per run were above 90 and kept their tick
  rotation).
- Camera: chase and replay cameras move up to 128 per tick; replay view changes and the level intro move 512 to
  over 4096. **cameraJump = 256.** Turns: below 10 degrees except one each in 20-30, 30-45, 45-60, 60-90 and four
  above 90 (cuts). **cameraTurn = 60** (H6). The `eyeVel` cut rule never fired in these runs (the cuts were caught by
  the jump / turn rules); it stays as a backstop.
- Skips per run: 63 object jumps (train wraps, respawn), 8 camera jumps, 174 object / 2 camera global skips.

**vsync = 0.** With `vsync = 0` the frames from f1500 on differ from the (vsync) base in 1-85 scattered pixels on
distant foliage at the fog edge, with `interpolate = 0` as well, and come in two variants run to run (two
`interpolate = 0` runs differed from each other by 51-83 pixels). The far clip path is the same (190.0521 from tick
592). It is the present mode, not the seam: `interpolate = 0, vsync = 0` and `interpolate = 1, vsync = 0,
BMX_HF_ALPHA=1` gave the same variant and then 10 / 10 identical frames; three alpha-1 runs with vsync 0 were
identical to each other. (All `vsync = 1` runs are byte-identical to the base.) Open: why the present mode reaches
the camera raster (likely the known uninitialised-data variation of the 3D scene, docs/WIDESCREEN.md WS6).

**Open (HF4).**
- The user's playtest at 3440x1440 on the real display (VRR or not), a pad, a bail and respawn, the attract demo, a
  Session run; the scripted runs had no bail.
- Late wake-ups: if the playtest log shows late ticks, raise the 2 ms spin window (`kSpinNs`) or sleep in smaller
  steps.
- `vsync = 1` at max_fps above the refresh rate: draws wait in the swapchain acquire and the worst-draw estimate
  grows to a refresh period, which lowers the extra draws per tick (by design; `max_fps = 0` avoids it).
