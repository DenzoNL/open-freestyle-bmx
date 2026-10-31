# Widescreen, native resolution and HUD layout (P6 enhancement)

Status 2026-10-31: **decided, not implemented.** The user took the recommendation (marked **rec**) for every
decision W1..W11; for W9 the default stays vanilla (640x480 4:3) until a clean playtest, then decided with
screenshots. Order: WS1-WS4 first. Scope: the native exe (`bmxnative.exe`, SDL3 driver) only; the hook
DLL and the oracle tests stay as they are. Rules that apply (PLAN.md Decisions): vanilla stays vanilla; enhancements
attach at seams, not as option checks inside faithful code; options are tagged visual-only or gameplay-changing.
Everything here is meant to be visual-only.

## What the game does today (facts)

**One screen size for 3D and 2D.** `[video] width/height` (or `-r`) picks a mode (src/driver/device.c
`bmxStandDialogBoxParamA`; the list holds the display's own full screen sizes plus the dialog's five 4:3 sizes, at 16
and 32 bit). zrwGraphicLibraryInitialize (0x48afe0) copies it to `gZrwScreenW/H` (0x697770/0x697774). The same two
words are `gZrwCamWidth/Height` in zrwcam.c, so the camera raster and z-buffer (zrwCameraInitialize 0x492340) and every
camera's rectangle (zrwCameraCreate 0x4925f0) are the mode's size. The driver presents that raster letterboxed into the
window with a linear blit (src/driver/gpu.c `letterbox`, `presentTarget`; SDL3-DRIVER.md D4). Resolution is fixed at
start-up: zrwCameraSetResolution (0x4924f0) refuses while a camera exists, and the game keeps one camera all session.

**Consequence: a native-resolution mode already works, but stretched.** `[video] width = 2560, height = 1440` renders
3D and 2D at 2560x1440 today. Both are anamorphic (4:3 content stretched by 1.333 horizontally), because:

- **3D:** the view window is always 4:3. zrwcam.c `applyFov` (inlined in zrwCameraCreate 0x4925f0,
  zrwCameraSetFov 0x492a60 and zrwCameraSetRect 0x492cc0) sets `window = {tan(fov) * 0.5, tan(fov) * 0.5 * 0.75}`
  through RwCameraSetViewWindow (0x4de9b0). RW maps the window onto the raster's width and height
  (rwD3DIm3DCameraBegin 0x4eb150: `rwD3DXScale/YScale` = raster size, `rwD3DRecipWindow`), so a 4:3 window on a 16:9
  raster stretches.
- **2D:** all 2D is laid out in a virtual 640x480 screen (some PSX leftovers in 512x480) and scaled per axis to the
  mode. Details under W4-W6.

**The FOV in numbers.** Cameras keep `fov` in degrees; cameraUpdate (camera.c, before cameraApply 0x407470) converts
it to a 16-bit ZAngle (x 65536/360) and cameraApply calls zobjCameraSetFov (0x4822a0) -> the zgl slot
`gZrwCamSetFov` (0x561d98, holds zrwCameraSetFov 0x492a60) -> applyFov. Note the game's "fov" f is not the angle of
the view: the half-width of the window is `tan(f)/2`, so horizontal FOV = 2·atan(tan(f)/2), vertical =
2·atan(0.375·tan(f)). Values come from `gCameraTuning` (0x551c54): chase modes 50/60/60/70, air 60, the eleven
replay "view" cameras 30-65, orbit types 60; scripted cuts override 60/25/30/15 (camera.c chase/scripted goals). The
create-time default is 0x271c (55°). Every camera of the game (riding, replay view cams type 1, orbit type 3, the
attract demo's random orbit type 4, the frontend) is the single `gNgCamera` (0x68ff98) and goes through cameraApply,
so the slot 0x561d98 sees every FOV change after creation. (The `fov` locals of 60/30 in ngifInitialize / the level
load in ngapp.c are zrwSetParameter(0) = the frame limiter `gZrwFrameLimit`, not a field of view.)

| game f | 4:3 h / v | 16:9 h (hor+) | 21:9 (64:27) h | 32:9 h |
|---|---|---|---|---|
| 30 (view cams) | 32.2 / 24.4 | 42.1 | 54.3 | 75.2 |
| 55 (default) | 71.1 / 56.3 | 87.2 | 103.5 | 124.6 |
| 60 (chase) | 81.8 / 66.0 | 98.2 | 114.0 | 133.2 |
| 70 (chase mode 3) | 107.9 / 91.7 | 122.7 | 135.5 | 149.5 |

## Where the game assumes 640x480 or 4:3

| Place | Assumption | Scales with the mode? |
|---|---|---|
| zrwcam.c applyFov (0x4925f0 / 0x492a60 / 0x492cc0) | window.y = 0.75 · window.x | No: fixed 4:3 frustum |
| ngifpsx.c `fillVertex` (all PSX prims: ngifDrawPolyF3/F4/FT4/G4/GT4, lines) | x · W/640 (W/512 with prim flag 2), y · H/480 | Per axis: stretches |
| ngifSetClip (0x464030) | 640x480 (0x27f/0x1df) means "no clip"; other rects · W/512, H/480, +16 lines | Per axis |
| ngifDrawAreaPrim (0x45fe50) | viewport rect lifted by H/480·32 | Per axis |
| ngifSetPictureScreen (0x463b20) | menu backdrop quad 512x480 (y -32..480), flag 2 | Per axis |
| zprimInit (0x4779e0) | default clip 0,0,639,479 | n/a |
| overlay.c (HUD, pause/menu screens) | 640x480 layout: left margin 0x35, right edge 0x24a/0x24b, bottom y 400, centre 0x13f/0x140, `ovlCentreY` 0xf0 | Through fillVertex |
| `gOvlBarSize` 0x557260 = {640, 60}, `gOvlFadeSize` 0x557298 = {640, 480}, `gCameraRect` 0x56b410 = {640, 480} | full-width bars and fades | Through fillVertex |
| ngappDrawSprite (0x45b6e0) | loading/legal picture as a W x H quad from zrwGetParameter(10/11) | Fills the mode |
| ngappDrawLoading (0x45c150) | progress bar on a 640x480 layout, ·W/640, ·H/480 | Per axis |
| zmovie.c zmoviePlay/zmovieUpdate | 320x240 Cinepak drawn into the client rect (src/driver/movie.c DrawDib stand-in) | Fills the mode |
| zprofile.c (debug profiler only) | screen width minus margins | yes |

Everything 2D in the game ends in zrwprim's 2D branch (flag 0x2000000: zrwprimDrawPolys 0x48d170, DrawLines 0x48ccf0,
reached through the zgl slots 0x561728 lines / 0x56172c triangles / 0x561738 quads, plus a direct call from
ngappDrawSprite) and from there in `RwIm2DRenderPrimitive` -> the engine device slot `fpIm2DRenderPrimitive`
(rwcore.h, set by d3dim2d.c to rwD3DIm2DPrimitive 0x4d9920). Im3D and the world pipelines never use that slot: they
use `RwIm2DRenderIndexedPrimitive` (d3dim3d.c). The only other non-indexed Im2D user is RW's debug charset. That
makes the slot a clean funnel for "all game 2D, nothing 3D".

## Decisions

### W1. Rendering resolution

- **A (rec):** keep the D4 design (render at the mode size, present letterboxed) and make the native size easy:
  `[video] width = 0, height = 0` means the desktop size (full screen) or the window's start size (windowed). No
  code in the game changes: the mode list already contains the display's sizes. Aspect is then handled by W2-W6.
- B: render 3D at the window size and 2D at 640x480 into a separate layer composited on top. Rejected: the overlay's
  PSX blend modes (fades, dim rects, glyph alpha) blend against the 3D image; a separate layer breaks them.
- C: support resizing at run time. Not possible without touching ported code (zrwCameraSetResolution refuses while
  cameras exist); the window can still be resized, it just letterboxes the fixed raster as today.

### W2. Aspect of the 3D view

- **A (rec): hor+ that keeps the 4:3 view inside** ("fit"): for W/H > 4/3 keep window.y and set
  `window.x = window.y · W/H`; for W/H < 4/3 (5:4 = 1280x1024) keep window.x and set `window.y = window.x · H/W`
  (vert+). At exactly 4:3 (W·3 == H·4) nothing is touched, so the floats stay the original's.
- B: vert- (keep window.x, crop top and bottom): 16:9 at f 60 loses the vertical from 66° to 52°; the rider fills
  more of the screen and the ground ahead is cut. Worse for a trick game.
- C: stretch (today's behaviour with a wide mode). Stays available as the vanilla setting.
- Plus `[video] fov_scale` (percent, default 100) applied to both window axes after A, for players who find 32:9 at
  133° too much; and optionally `max_hfov` (degrees, 0 = none) that caps window.x on ultrawide.

### W3. Culling and draw distance

Facts: RW builds the frustum from the view window (CameraBuildPerspClipPlanes 0x4dd4d0, the corners from
`viewWindow.x/y`), then culls atomics by sphere (RwCameraFrustumTestSphere, world.c 1903) and sectors by the frustum's
box and planes (world.c 2188). A wider window widens culling automatically: no pop-in from RW. The PVS
(zrwworld.c, RpPVSQuery / RpPVSHook in zrwDrawFrame 0x48b690 when `gZrwPvsEnabled` 0x6976e8, named
`gZrwDebugLightOn` in zrwzgl.h) is a per-sector table of what is visible from a position in any direction
(RpPVSSetViewPosition with the camera's position only), so it does not depend on the FOV. LOD selection
(rpLodAtomicSelectLevel 0x505b30) uses depth along the view axis, also FOV-independent. Far clip and fog
(zrwSetParameter 5/9 -> zrwCameraSetClip 0x4929f0 / SetClipOffset 0x493100) are unchanged. No game-side
screen-space culling exists (zrwCameraProject 0x492c20, the only reader of the view window in the game, has no
caller in the port).

- **A (rec):** no culling change. Risk to check in the playtest: level geometry the designers never showed at the
  sides (open level edges, missing back faces, props placed just out of a 4:3 view), most likely in the replay
  "view" cameras (fixed positions) and the attract demo.
- B: widen the far clip / fog for wide modes (`draw_distance`): out of scope here (the graphics-options backlog).

### W4. HUD and overlays

Facts: the HUD is the zover overlay (zoverii.c, PSX packets in pools; layer 0 = HUD pools 0/1, layer 1 = menu
screens pools 2/3, zoverFlush 0x457b00), in 640x480 coordinates, drawn through the ngif ordering table
(ngifDrawList 0x462350 -> ngifDrawPrimitive 0x462050). Text is one item per glyph, chained.

| Element (overlay.c) | 640x480 position | Group |
|---|---|---|
| Timer `ovlTimerInit` 0x41c290 | x 0x35, y 400 | bottom left |
| Score `ovlScoreInit` 0x41bf00 | right-aligned at 0x24b, y 400 | bottom right |
| Rider name `ovlNameDraw` 0x41c500 | x 0x35, y 0x3c | top left |
| Height meter `ovlHeightInit` | rect at 0x224, 0x51, 0x26 x 0x12a | right |
| Indicator `ovlIndicatorDraw` 0x41d790 | centred on 0x240, y 0x30 | top right |
| Icon `ovlIconInit` 0x41e750 | sprite at 0x1b1, 0x148 | centre right |
| Combo / trick text `ovlComboDraw`, caption, status and goal messages, hints, flash | centred on 0x13f/0x140 | centre |
| Balance meter, element 8 | draw nothing on PC | - |
| Pause (screen 1), options, goal/challenge list (16-18), results/judges and run times (8-10), save (11) | 640x480 layouts, full-screen dim rect 0x280 x 0x1e0 (`ovlInit`) | menu |
| Fade (`ovlFadeDraw` 0x41bce0), cut-scene bars (`ovlLetterboxDraw` 0x41bc90) | full width | cover (W6) |

- **A (rec for slice 1): centred 4:3 HUD.** Every non-cover 2D primitive gets the same transform
  `x' = (W - 4/3·H)/2 + x · (4/3·H)/W` (undoes the stretch, pillarboxes the 2D). Robust, needs no knowledge of
  elements, keeps the original's composition. On 32:9 the HUD sits in a narrow centre band.
- **B (rec as slice 2, `hud = 1`): edge anchoring of the HUD layer only.** Left group shifted to the left edge,
  right group to the right edge, centre stays, all with A's uniform scale; menu screens stay centred 4:3. The anchor
  must be decided per element, not per glyph (a centred combo string spans all thirds of the screen), so it needs the
  zover chain: a native copy of ngifDrawList that marks the current packet, plus a per-frame read-only walk of the
  layer-0 pools that gives each glyph chain its bounding box and anchor (left third / right third / centre).
  `hud_aspect` (default 178 = 16:9) limits how far out the HUD goes on ultrawide.
- C: stretch (vanilla with a wide mode).
- D: scale the HUD up or down (`hud_scale`): cheap on top of A/B (one more factor); backlog.

### W5. Menus and full-screen 2D (backdrops, loading pictures, movies)

Facts: frontend menus are pure 2D (zprim -> ngif; no 3D in src/bmx/com/menu); backdrops are 512x480 PSX pictures
already stretched to 640 in vanilla (ngifSetPictureScreen), so their correct shape is 4:3. The loading/legal pictures
are full-screen quads (ngappDrawSprite). Movies are 320x240 (4:3).

- **A (rec): pillarbox at 4:3 with black sides.** Menus get W4-A's transform; a textured primitive that spans the full
  width (backdrop, loading picture) is drawn into the 4:3 centre; movies are pillarboxed in the DrawDib stand-in
  (src/driver/movie.c), which is our code.
- B: stretch (vanilla with a wide mode).
- C: pillarbox plus a fill: the backdrop stretched to full width, blurred and darkened, drawn behind (one extra
  textured quad and a blur pass in the driver). Nicer; later slice.

### W6. Full-screen 2D primitives (fades, bars, dim rects, PSX blend quads)

They must cover the whole wide screen, including the PSX blend modes (PLAN.md backlog: subtractive / additive
fades; those keep their blend state, only positions change).

- **A (rec): classify per primitive in the 2D funnel.** An untextured primitive whose x extent covers the full
  stretched width (0..W, i.e. 0..640 virtual) is a cover and keeps today's stretch (identity). That catches the fade
  {640, 480}, the cut-scene bars {640, 60} at y 0 and 0x1a4, the pause/menu dim rect, `cameraDrawLetterbox`
  {640, 480} and frontend clears (ngifFillRect over the full clip). Textured full-width primitives are backdrops (W5).
- B: hand-list the cover call sites (ovlFadeDraw, ovlLetterboxDraw, zoverAddRect with 640x480, ...) and jump-swap
  them. More code, misses future finds.

### W7. Text and sprite sampling at high resolution

Facts: textured PSX quads without prim flag 8 get render flag 4 (ngifpsx.c `quadFlags`), which zrwSetRenderState
(0x48bc60) turns into RwRenderState 7 (texture filter) = 1, nearest. Glyphs, sprites and picture screens are drawn
that way; in vanilla the 640x480 result is then upscaled with a linear blit (soft). At native resolution the same
textures are magnified 2-3x per texel with nearest: crisp but uneven (2.25x gives alternating 2- and 3-pixel texels,
which shimmers on moving text).

- **A (rec): `[video] ui_filter = 0` keeps the game's sampling, `1` linear, `2` sharp-bilinear** (nearest to the
  largest integer multiple, linear for the remainder; one shader variant). Applied by the driver to draws that come
  through the 2D funnel only. Recommend trying 2 as the wide default once seen.
- Present filter for modes smaller than the screen (vanilla 640x480 on a 1440p display): `[video] scale_filter = 0`
  linear (today), `1` nearest, `2` integer scale (largest whole factor, black border; 1440/480 = 3 exactly, 1080/480
  falls back to 2). Touches only gpu.c `letterbox` / `presentTarget`.

### W8. Where the code attaches (seams)

CONTRIBUTING.md reserves `BMX_FIX_*` for original bugs (compile-time, off by default); none exists in src yet, and
it would put option checks inside faithful code, so it does not fit runtime options. The repo already has two
runtime seams: jump tables written by the native loader (driver, PNG reader) and a jump over a ported entry to a
native copy (smoothmix: `bmxSmoothJumps`, src/native/loader.c). Proposal, all native-only and installed only when
`widescreen = 1`:

- **A (rec): pointer swaps at existing indirection slots, plus one small copy.**
  1. 3D: the loader stores `bmxWideSetFov` into the zgl slot 0x561d98 (originally zrwCameraSetFov 0x492a60). It calls
     the original, then rewrites window.x (or .y) from the window the original just set (W2). zobjCameraCreate
     (0x4821c0) calls the slot right after zrwCameraCreate's own applyFov, so creation is covered; zrwCameraSetRect
     has no other caller. No ported function changes.
  2. 2D: after device open the driver wraps `RwEngineInstance->dOpenDevice.fpIm2DRenderPrimitive` (the game's only
     non-indexed Im2D user). The wrapper copies the vertices, classifies (cover / backdrop / other, W5/W6), applies
     the transform and calls rwD3DIm2DPrimitive. Clipping (zrwprimClipPoly 0x48c840 against the camera viewport)
     already happened in stretched space, and the transform is affine per primitive, so clip and draw stay
     consistent without touching ngifSetClip / ngifDrawAreaPrim.
  3. HUD anchoring (W4-B only): a native copy of ngifDrawList (0x462350, 12 lines; jump over the ported entry like
     smoothmix) that records the packet being drawn, so the Im2D wrapper can look up the element's anchor.
  4. Driver-internal options (present filter, ui_filter sampler, movie pillarbox): plain option checks in src/driver,
     which is new code (D1), not faithful code.
- B: copies of applyFov's three callers (0x4925f0, 0x492a60, 0x492cc0) via jumps. More duplicated faithful code for
  the same result as A1.
- C: game-side hooks (option checks in zrwcam.c / ngifpsx.c / overlay.c). Breaks the seam rule; only as a
  `BMX_FIX`-style compile switch if A ever proves impossible.
- Rejected: lying at the screen-size query slot 0x561488 (zrwGetParameter) to make the 2D think the screen is 4:3:
  ngappDrawSprite calls zrwGetParameter directly, covers would get pillarboxed, and the camera raster shares the
  same global.

### W9. Config keys and defaults

Integers only (src/platform/config.c), `[video]`, all new keys added to `kKnownKeys`:

| Key | Values | Vanilla default |
|---|---|---|
| `width`, `height` | as today; `0` = desktop / window size | 640, 480 |
| `widescreen` | 0 off (a wide mode stretches, as today), 1 on (installs the seams of W8) | 0 |
| `aspect` | 0 stretch, 1 hor+ fit (W2-A), 2 vert- | 1 (used only with widescreen = 1) |
| `fov_scale` | percent | 100 |
| `hud` | 0 centred 4:3, 1 edge-anchored, 2 stretch | 0 (1 once slice 2 lands) |
| `hud_aspect` | max aspect · 100 for anchored HUD | 178 |
| `menus` | 0 pillarbox black, 1 stretch, 2 pillarbox + blurred fill | 0 |
| `movies` | 0 pillarbox, 1 stretch | 0 |
| `ui_filter` | 0 game, 1 linear, 2 sharp | 0 |
| `scale_filter` | 0 linear, 1 nearest, 2 integer | 0 |

**The user's call: default.**
- **Vanilla default (`widescreen = 0`, 640x480 letterboxed):** matches "vanilla stays vanilla", zero risk, no surprises
  from level edges never meant to be seen. Cost: every player on a 16:9 screen sees a small, soft 4:3 picture until
  they edit the ini.
- **Exception pattern (`widescreen = 1`, `width = height = 0`):** like dpad_steering and smooth_resample, the
  convenience most players want is on. Cost: the default view shows more than the original did; the sides of levels
  and replay cams need a full playtest first; any classifier miss is visible to everyone.
- Rec: vanilla default until the W10 playtest is clean, then decide on the exception with screenshots in hand.

### W10. Verification

1. **Off is byte-identical.** With `widescreen = 0` the loader installs nothing (log line `wide: 0 slots`).
   BMX_SHOT frames of the existing scripted runs (menus at frames 300 / 1500, ProQuest level start, pause, results)
   are byte-identical BMPs to a build without the feature, at 640x480 and at a stretched 1920x1080.
2. **On at 4:3 is byte-identical too:** `widescreen = 1` at 640x480 and 1024x768 (the identity guards in W2 and the
   2D transform) gives the same BMPs.
3. **Unit tests (tests/platform style, no window):** the view-window maths (hor+ / vert+ / vert-, the 4:3 identity
   path, fov_scale); the 2D classifier and transforms on synthetic vertex lists (fade, bars, dim rect, backdrop,
   glyph run, clipped quad); HUD anchor assignment on a recorded zover pool dump.
4. **Wide screenshots** of the same scripted runs at 16:9 (1920x1080), 16:10, 21:9 (2560x1080, 3440x1440), 32:9 and
   5:4 (vert+): frontend, loading screen, level start fade, HUD corners, combo text, pause, goal list, results, a
   cut-scene with bars, a replay with view cams, the attract demo, the intro movie. Checklist: nothing stretched;
   covers reach both edges; HUD inside `hud_aspect`; text not torn.
5. **Determinism.** The seams only change the RW view window and 2D vertex positions, but a wider frustum changes
   which atomics RW renders, and any render callback with state (animation, RNG) would then differ. Check: a per-tick
   hash of the simulation (rider position/velocity, RNG state, score, timer) logged in BMX_KEYS runs with
   `widescreen = 0` and `1` at 1920x1080 must match tick for tick; golden traces (docs/TRACING.md) replay clean; a
   replay recorded with the option off plays back identically with it on. Only then tag the options visual-only.
6. **Playtest (user):** all seven unlocked levels in Freeride plus a replay each, looking at the screen edges.
7. `tools/check.sh` and the hook DLL are unaffected (native-only files).

### W11. Implementation order

| Slice | What | Size | Model |
|---|---|---|---|
| WS1 | `[video]` keys, `width/height = 0`, `scale_filter` (present filter, integer scale) | ~120 lines | Sonnet |
| WS2 | hor+ slot wrapper (W2-A, fov_scale), unit tests, the per-tick simulation hash for W10.5 | ~200 | Sonnet (Opus review of the hash coverage) |
| WS3 | Im2D funnel wrapper: classifier, centred 4:3 HUD/menus, covers, backdrops pillarboxed, movies pillarboxed | ~300 | Opus (driver, classifier edge cases) |
| WS4 | Screenshot set and determinism runs at all aspects (W10.1-5); fixes | runs | Sonnet |
| WS5 | HUD edge anchoring: ngifDrawList copy, zover chain walk, `hud_aspect` | ~250 | Opus |
| WS6 | `ui_filter` (sharp-bilinear shader variant), blurred menu fill (`menus = 2`) | ~200 + shaders | Sonnet |
| WS7 | User playtest, default decision (W9), docs (NATIVE.md, PLATFORM.md config list) | - | - |

WS1-WS4 give a complete 16:9 / 21:9 mode with a centred HUD; WS5-WS6 are polish and can wait.
