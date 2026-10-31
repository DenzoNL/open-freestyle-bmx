# Widescreen, native resolution and HUD layout (P6 enhancement)

Status 2026-10-31: **WS1-WS6 implemented** (WS5 and WS6 not committed; "Implementation status" at the end), the
default stays vanilla until the user's playtest (WS7, `native-run2\widetest.ps1`). The user took the
recommendation (marked **rec**) for every decision W1..W11; for W9 the default stays vanilla (640x480 4:3) until a
clean playtest, then decided with screenshots. Order: WS1-WS4 first. Scope: the native exe (`bmxnative.exe`, SDL3
driver) only; the hook DLL and the oracle tests stay as they are. Rules that apply (PLAN.md Decisions): vanilla stays vanilla; enhancements
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

## Implementation status (WS1-WS4, 2026-10-31)

Code (native exe only; the hook DLL, the oracle tests and every ported function are unchanged):
`src/driver/wide.h`, `widemath.c` (the maths, no SDL / game state), `wide.c` (the seams), `src/native/simhash.c`
(BMX_SIM_HASH), small hooks in `driver/gpu.c` (present), `movie.c` (DrawDib rectangle), `device.c` (`width = 0`,
the 2D install after rwD3DIm2DOpen), `raster.c` (`drvCameraCurrent`), `native/loader.c` (installs),
`native/main.c` + `platform/window.c` (`bmxVideoSize`), `platform/config.c` (keys). Unit tests:
`bmxwidetest` (tests/platform/test_wide.c, 93 checks).

| Slice | State | Notes |
|---|---|---|
| WS1 | done | `[video]` keys `widescreen aspect fov_scale max_hfov hud menus movies scale_filter` (W9; `hud_aspect` came with WS5, `ui_filter` with WS6); `width = 0` / `height = 0` = the primary display's desktop size, full screen and windowed (`bmxVideoSize`); `scale_filter` 0 linear letterbox (unchanged), 1 nearest, 2 integer factor (linear letterbox when not even 1x fits) in `presentTarget` |
| WS2 | done | `wideSetFov` in the zgl slot 0x561d98 (data pointer, written by the loader when `widescreen = 1`; log `wide: 1 slot`, else `wide: 0 slots`): calls the original, reads the window it set, applies W2-A (hor+ above 4:3, vert+ below, identity at exactly 4:3), then `fov_scale` and the `max_hfov` cap; RwCameraSetViewWindow only when something changed. BMX_SIM_HASH (below) |
| WS3 | done | `wideIm2DPrimitive` wraps the engine device's fpIm2DRenderPrimitive at the device start. Only while the screen camera is in its update (camera raster = gZrwScreenW x H, not 4:3); classifier W6-A: untextured and the x extent reaching both edges (within 2 virtual pixels) = cover, stays stretched; textured full width = backdrop, pillarboxed unless `menus = 1`; everything else centred 4:3 unless `hud = 2` (`hud = 1` acts as 0 until WS5). Moved primitives are copied, mapped and **clipped to the 4:3 area** (finding 2 below). Movies: DrawDibDraw's destination mapped the same way (`movies = 0`), the target stays black around it |
| WS4 | done | runs below; one fix (the clip) |
| WS5 | done | `hud = 1`, `hud_aspect` (below, "WS5: HUD edge anchoring") |
| WS6 | done | `ui_filter`, `menus = 2` (below, "WS6") |

**Byte identity** (BMX_SHOT BMPs of the same BMX_KEYS run: ProQuest into Vail Trails, riding, pause, END RUN?,
RUN FINISHED, view replay; frames 200..3200; build without the feature = 6e7f88a):
- 640x480, `widescreen = 0` and `= 1`: identical to the old build. One caveat found on the way: frames 1300 and
  1600 come out in one of two variants run to run (59 and 1 pixels differ), with the old and the new build alike and
  with the option on and off alike; every run matched one of the two variants exactly. Pre-existing run-to-run
  noise, not the feature (open point).
- 1920x1080 stretched (`widescreen = 0`): 13/13 identical to the old build.
- 1024x768 (exact 4:3), `widescreen = 1`: 12/12 identical to the old build.

**Determinism** (W10.5): `BMX_SIM_HASH=<file>` writes `<tick> <hash>` per controller poll: FNV-1a 64 over the
golden-trace recorder's fields (seed, pads, the selected camera's eye / look-at, all four riders' state, surface,
state levels, flags, timer, position, velocity, forward, up, rotation, spin) plus each rider's combo score and
gGameTimer. Two runs of the same build agree tick for tick. The same script with `widescreen = 1` at 1920x1080,
3440x1440, 1920x1200, 3840x1080 and 1280x1024, and with the option off at 640x480 and 1920x1080, gave the same hash
for every tick (1773..2173 ticks, menus, riding, pause, results and the replay included): the seams are
visual-only as far as this coverage goes. (A hash of the whole 4 x 0xf70 rider array was tried and dropped: it
differs between two runs of the same build from the first tick, so the struct holds process-specific bytes.)
Golden-trace replays (W10.5's second half) were not run: traces need the hooked game (docs/TRACING.md).

**Screenshot findings** (21:9 3440x1440 and 16:9 1920x1080 primary; also 16:10 1920x1200 and 32:9 3840x1080
windowed, 5:4 1280x1024):
1. Frontend, loading screen, movies: pillarboxed 4:3 with black sides; 5:4 letterboxes top and bottom (vert+).
   Pause and results dim the whole width (cover rule works for the dim rect); HUD timer / score and the replay's
   arrow sit in the centred 4:3 area; menus' text centred. No missing level geometry at the sides of the Vail Trails
   start, the pause / results backgrounds or the replay cameras that were seen.
2. **Fixed:** the game draws 2D partly or wholly off screen (the frontend's icon column slides out to the left when
   it leaves; in vanilla the screen edge hides it). Mapped into the 4:3 area such primitives landed in the
   pillarbox, and the loading screen (which never clears the camera) kept them there. Moved primitives are now
   clipped to the 4:3 area on the CPU (`drvWideClipFan` for zrwprim's triangle fans, `drvWideClipLines` for its line
   lists; attributes interpolated linearly, 2D has one rhw). zrwprimClipPoly only clips to the camera viewport, so
   the clip is ours.
3. FOV at 3440x1440 (hor+, 4:3 view inside): chase f 60 -> **114.4°** horizontal (66.0° vertical, as at 4:3);
   f 55 -> 104.0°, replay view cams f 30 -> 54.7°, chase mode 3 f 70 -> 135.8°. At 1920x1080: 98.2 / 87.2 / 42.1 /
   122.7. The image at 114° shows the usual rectilinear stretch towards the sides; `max_hfov` matters at 21:9 only
   for the wider chase modes (a cap of 120 changes f 70 only; 105 would trim f 60 too), and `fov_scale` 90 gives
   f 60 -> 108.8°. Worth trying in the playtest; not needed for correctness.
4. Banding in the sky / grass is the 16-bit mode (depth 16, the default and vanilla), not the feature.
5. The present filter (`scale_filter`) is not visible in BMX_SHOT frames (they are the camera raster); covered by the
   unit tests only.

**Open points:** the playtest (W10.6: all levels, replays, attract demo, cut-scenes with bars, the goal lists);
the run-to-run variation above; `-r<w>.<h>.<depth>`
still overrides `[video]` (and has the original's pick bug).

## WS5: HUD edge anchoring (2026-10-31, not committed)

`[video] hud = 1` (with `widescreen = 1`) anchors the HUD per element; `hud_aspect` (aspect · 100, default 178)
bounds it: the anchored HUD lives in a centred area `hud_aspect · H / 100` wide, at least the 4:3 area and at most
the screen, on whole pixels (3440x1440: 2563 wide, x 438..3002; 1920x1080: the full width; `hud_aspect = 0`: the full
width). Every element keeps the 4:3 scale (H / 480 per virtual pixel); only its offset changes:

- **left:** virtual x 0 at the area's left edge (x' = lo + x · s);
- **right:** virtual x 640 at the area's right edge (x' = hi - (640 - x) · s);
- **centre** (and everything that is not a HUD element): the centred 4:3 map of WS3, unchanged.

A moved primitive is clipped to where its own virtual screen [0, 640] lands (WS3's clip, per anchor). Menu screens
(layer 1), menus, backdrops, covers and movies behave as in WS3. Narrower than 4:3 (5:4) and exactly 4:3, `hud = 1`
does nothing.

**Seam (W8-A3).** `drvWideJumps` (src/driver/wide.c) gives the loader a jump over the entry of the ported
`ngifDrawList` (0x462350; ngifFlushFrame calls it directly) to `wideDrawList`, a copy (the same 10 lines) that sets
the anchor of each packet before ngifDrawPrimitive and clears it after; the Im2D wrapper reads it. Installed only for
`widescreen = 1` and `hud = 1` (log `wide: 1 hud jumps`, else `wide: 0 hud jumps`). The copy is a no-op at 4:3.

**Elements at the draw list.** The HUD reaches the ordering table two ways (overlay.c, zoverii.c), both with the
element still recoverable:

1. **zover pools of layer 0** (pool 1: glyphs, pool 0: rectangles / gradients / sprites; both frames): the packet's
   address says which item it is. At the start of each draw list, a read-only walk of the pools builds every text from
   its glyph chain (the head is the glyph no other glyph points at; `next` at 0x9e) and its box; each text gets one
   anchor, each shape its own. A text or shape that lies inside a **wider text** on the same lines takes that text's
   anchor (the hint lines' button glyphs, separate one-glyph texts placed inside the line). Pools 2 / 3 (layer 1 =
   menu screens) get none.
2. **The 3D list** (gNgifPrim3DBuffer .. gNgifPrim3D): `zoverDrawText` (name, combo lines, combo score, height labels,
   indicator / replay arrow) writes one 0x9c byte FT4 packet per glyph back to back and links them into table entry 0
   in a row; `zoverDrawRect` one F4. Only while `gZoverLayer == 0` (no menu screen) and only for 2D packets (no float
   flag). At the first packet of a run the copy looks ahead along the chain: consecutive packets that are 0x9c apart,
   FT4, on the same lines and with x0 = the other's x1 + 1 (zoverDrawText's spacing, exact) are one text
   (`drvWideGlyphRun`); a rect is its own element. Fades and letterbox bars from the same buffer stay covers (the
   funnel's cover rule comes first).

**Anchor of an element** (`drvWideHudAnchorOf`, from its x extent): centre when its middle is within 8 virtual pixels
of 320 (ovlCentreX texts land within 1); otherwise left if the middle is left of 320, right if right of it. Thirds (W4's
first idea) would split the left-aligned flash lines (x 0x19, long lines reach past 213) and put the icon at 0x1b1
in the centre; the "centred or which side" rule matches every element below.

| Element (overlay.c) | Path | Box (virtual) | Anchor |
|---|---|---|---|
| Timer (2) | pool text | 0x35.., y 400 | left |
| Score (1) | pool text | right-aligned at 0x24b | right |
| Rider name (3) | 3D list run | 0x35.., y 0x3c | left |
| Combo lines, "TO", combo score (4) | 3D list runs | centred on 0x140 | centre |
| Height meter (5): frame, ticks, labels, marker | pool rects + 3D list rects / runs | 0x213..0x24b | right |
| Balance meter (6) | pool gradients, rects | 0xd5..0xf3 | (left; draws nothing on PC) |
| Caption (7) | pool text | centred 0x140, y 0x132 | centre |
| Indicator / replay arrow (9) | 3D list run (icon font) | centred on 0x240, y 0x30 | right |
| Hint lines + button glyphs (10) | pool texts | centred 0x140; buttons inside | centre (buttons by containment) |
| Goal message (11), status message (12) | pool texts | centred 0x140 | centre |
| Flash message (13) | pool texts | x 0x19, four lines | left |
| Icon sprite (14) | pool sprite | 0x1b1, 0x148 | right |
| physHudText 0x10ba (physics.c) | pool text | centred on 0x210, y 0x3c | right |
| Fade (16), letterbox (0) | 3D list | full width | cover (stretched) |
| Menu screens (pause, challenges, results, records, save) | pools 2 / 3, 3D list with layer 1 | - | centred 4:3 (as WS3) |

**Verification.**
- Unit tests (`bmxwidetest`, now 179 checks): the anchor rule on the elements above, packet boxes (FT4, F4, the 3D
  list's float flag, mode packets), the pool walk on synthetic pools (timer / score / a centred combo line spanning all
  thirds: one anchor per element, not per glyph; a hint line stored backwards with a button inside; cycles; free
  items), zoverDrawText runs (two texts back to back, other lines, a rect, forward order), the hud_aspect area and
  the anchored map (3440x1440 and 1920x1080 edges, hud_aspect 0, centre = drvWideMap2D bit for bit, 4:3 untouched).
- Byte identity (BMX_SHOT, the WS4 script, frames 300..3000): `widescreen = 0` (`hud = 1` in the ini) at 1920x1080:
  13/13 identical to WS4's off run and to the build before the feature; `hud = 1` at 1024x768: 12/12 identical to
  WS4's on1024 and base1024; `hud = 0` at 1920x1080: 12/13 identical to WS4's on1080, the 13th (f1600, 305 pixels in
  the 3D scene) is the known run-to-run variation. `hud = 1` at 3440x1440 and 1920x1080: menu frames (challenges,
  pause, END RUN?, RUN FINISHED) identical to `hud = 0`, only HUD frames differ (one RUN FINISHED frame at 1080 had 34
  scattered 3D pixels: the same variation).
- BMX_SIM_HASH: `hud = 1` at 3440x1440 and 1920x1080, `hud = 0` at 1920x1080 and `widescreen = 0`: the same hash for
  all 1974 ticks.
- Screenshots (3440x1440, 1920x1080): timer and score at the 16:9 edges on 21:9 (x 597 and 2843 of 3440) and at the
  screen edges on 16:9; the height meter (frame, ticks, labels, marker together) at the right; the replay arrow at
  the right; challenges, pause, END RUN?, RUN FINISHED centred 4:3 as before.

**Open (WS5).** The combo / trick text was not caught in a screenshot (the scripted hops did not land a trick); it is a
centred element by the unit tests and by its code (ovlCentreX 0x140). Not seen yet: hint lines with buttons, goal /
status / flash messages, the icon sprite, physHudText, the balance meter. The playtest (W10.6) should look at them
with `widetest.ps1 -Hud 1`. Default stays `hud = 0` (W9) until then.

## WS6: ui_filter and the blurred menu fill (2026-10-31, not committed)

**`[video] ui_filter`** (W7-A; 0 default): how the textured draws of the 2D funnel are sampled. 0 = the game's own
(nearest for PSX quads, render flag 4). 1 = linear. 2 = sharp bilinear: nearest inside a texel and a linear ramp one
output pixel wide at its edges (the pixel shader computes the pixels-per-texel from the screen space derivatives of
the texel coordinate, so it needs no size from the CPU and works for glyphs, sprites and the 512 wide backdrops alike;
a scale below 1 counts as 1). Only draws the game samples with nearest are changed (a draw the game already samples
linearly stays); untextured draws and everything outside the funnel (3D, movies) are untouched. It is independent of
`widescreen`: the funnel wrapper is also installed for `ui_filter != 0` (with `widescreen = 0` it only sets the
sampling mode), and it applies at 4:3 too. At the exact 4:3 raster of the original (640x480) 1 and 2 are therefore not
the vanilla bytes; only 0 is. The filter is in the camera raster, so BMX_SHOT frames show it (unlike `scale_filter`).

**`scale_filter`:** no sharp variant: the design (W7) lists linear / nearest / integer only, and `ui_filter = 2` (applied
at the raster) already gives the sharp look of the HUD on a scaled 640x480; a second shader for the present blit would
mean replacing SDL_BlitGPUTexture with a pipeline for a case integer scaling covers. Not added.

**`menus = 2`** (W5-C): a backdrop (textured, full width) is drawn twice: first the original vertices stretched over
the whole screen with the blur mode (5 x 5 taps four texels apart on the linear sampler, triangular weights, darkened
to 50 %), then, as with `menus = 0`, the 4:3 pillarboxed one on top. So the sides show the blurred, dimmed picture
instead of black; no extra render pass or target. Loading and legal pictures (full-screen quads) get the same.
Movies (`movies`) keep their black pillarbox. Covers (fades, dim rects) and the HUD are not affected.

**Shaders.** `tl.hlsl` is compiled twice by tools/shaders.sh: `tl_ps` as before (the `UI` code is `#ifdef`ed out;
its header is byte-identical to the committed one, so the 3D and default 2D shader is unchanged) and `ui_ps`
(`-DUI`, DXIL + SPIR-V) used by the pipelines of draws with mode 2 or 3 (`PipelineKey.ui`). The fragment uniform
`params.w` carries the mode, `fogColor.a` the blur's brightness. Mode 1 needs no shader: the sampler is linear.
(A first attempt put the UI paths into `psMain` itself: the recompiled shader changed 658 scattered 3D pixels of a
default frame, so the variant is separate.)

**Code.** `drvDrawSetUiMode` (draw.c) is set around the next stage of the funnel by `drawNext` (wide.c);
`drvWideBlurFill`, `drvWideSharpCoord` (the C twin of the shader's coordinate, for the unit tests) in widemath.c.

**Verification** (3440x1440 borderless window, `widescreen = 1`, ProQuest into Vail Trails, frames 250 / 450 / 900 /
1500 / 1710 = main menu, rider select, challenge screen with backdrop, riding, pause):
- `bmxwidetest` 750 checks, 0 failures (new: the blur-fill rule, `drvWideSharpCoord`: identity at s = 1, flat at
  the texel centre, the one-pixel ramp, continuity); `bmxmovietest` 0 failures.
- Defaults vs the build before WS6: frames 250, 450, 900, 1710 byte-identical, frame 1500 (riding) differs in 658
  scattered 3D pixels. Not the feature: the new build with the sampling code forced off (`mode = 0`) and with
  `mode = uiMode` (a runtime 0) reproduces the old build's frame 1500 exactly, while the same code as shipped gives
  the 658 pixel variant, deterministically over two runs. Identical default behaviour in all three, so this is the
  known build-to-build variation of the 3D scene (the open point above), now seen as a difference between two builds
  rather than between two runs. Open: finding its cause (probably uninitialised data in a 3D path).
- Screenshots and crops: scratchpad `widescreen6\` (def, lin, sharp, blur; `crop_hud_timer.png`: the timer digits
  nearest / linear / sharp at 5x; `crop_menu_text.png`: menu text). Linear blurs the 3 px per texel glyphs into
  mush, sharp keeps the nearest crispness with smooth edges; the fill shows a dim, blurred backdrop at the sides.

**Open (WS6).** `ui_filter` is not applied to the movies. The fill is stretched, so at 21:9 the picture is 2.3x
wider than at 4:3 (blur hides it); a mirrored fill would continue the picture but is more code. MSL (Metal) is still
not generated for any shader.

Note (2026-10-31, merge of WS6): the WS6 build's frame 1500 differed from WS5 on tree edges only; explained
by the wall-clock far clip (HF0 note above), not by WS6.

**Note (HF0, 2026-10-31): the frame variation explained.** The "f1600 variants" above, and riding frames that differ
in a few hundred pixels on tree trunks and foliage between builds that draw the same way, come from wall-clock render
state, not from the seams. No tree is animated by the clock: Vail Trails has no "ANIM" texture objects. The cause is
ngiffxAdaptQuality 0x45c710, and the chain runs:
1. It measures ticks per 100 ms of timeGetTime.
2. It sets the camera's **far clip** to its quality value, and the fog distance follows (`far - clipOffset`), through
   zrwSetParameter(5) -> zrwCameraSetClip 0x4929f0.
3. The level load drops the far clip to 60. It then climbs in 100 ms steps and settles at 190.05 about 590 ticks
   later: 188.92 at tick ~1500 and 190.05 at tick ~1524 in the ProQuest script.
4. The tick each step lands on depends on where the 100 ms windows fall against the ticks. That differs by a tick
   between runs and builds.
5. Frames taken during the climb, such as f1500 and f1600, can therefore have another far clip and fog distance at
   distant geometry.

The fix for comparisons: diff frames after the settle (f1800 and later in that script). The BMX_HF_GUARD log line
`quality (far clip)` shows the value per tick (docs/HIGHFPS.md "HF0 findings").
