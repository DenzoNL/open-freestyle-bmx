# High frame rate: fixed tick, interpolated rendering (P6 enhancement)

Decided 2026-10-31: the user took the recommendation for every decision H1..H10 (default off until HF4 is clean
and the user has played it). Order: HF0 first.

Status 2026-10-31: **design, not decided, nothing implemented.** Scope: the native exe (`bmxnative.exe`) only; the hook
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
| 2 | zrwworldUpdateEffects 0x495bd0 -> each effect type's update (+0x20) | **yes**: particles (below) |
| 2 | zrollerShadowDraw 0x49b3d0 (draws the shadow, clears `ready`) | yes |
| 2 | ngappDrawDebug 0x45a4d0 (debug faces) | no |
| 5 | zrollerUpdateShadow 0x49b520 (ray probes around the rider, sets `ready`: drawn one draw later) | yes, render-only |
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
- **P3 Particles.** The step is wall-time plus one `vel` per call, run from the draw: extra steps make particles fly
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
