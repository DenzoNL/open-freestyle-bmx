# SDL3 render driver for RenderWare (P4)

Status 2026-10-30: decided; steps 1 and 2 done (the menu draws); step 3's CPU port done (instancing pipelines,
oracle-tested), its native runs still to do. The user chose option A for D1..D7 (the recommendations below).
The game and RenderWare (708/711) are ported. What `bmxnative.exe` still lacks is RW's Direct3D 7 device driver (`lib/d3d7driver` in data/functions-rw.csv: 250 functions, 107 KB, never ported).
The decisions below record the options that were considered; A is the chosen one in each.

## Where the driver plugs in (facts)

RW talks to its driver through three tables in the original's data, all set up by `RwEngineInit`/`Open`:

- **Device** (0x566fe4, RwDevice): gamma 1.0, system function 0x4ee010 (open/close/start/stop, modes, subsystems,
  standards), z near/far, `RenderStateSet`/`Get` (0x4d7320/0x4d7c10), Im2D line/triangle/primitive/indexed
  (0x4eb2c0..0x4eb2f0), Im3D slots empty (Im3D goes through the pipelines).
- **Standards** (0x56701c): 18 (id, function) pairs: camera begin/end update and clear, raster create/destroy/
  lock/unlock/palette lock/sub-raster/show, raster render (fast/scaled), image <-> raster, RGB <-> pixel,
  texture set raster, native texture read/write/size.
- **Core extension table** (0x69bee0): 4 hooks (extension attach 0x4eac90, Im3D/render-state hooks). This
  is where bmxnative traps now.

Plus the world/atomic **instancing pipelines** (0x4c8ba0..0x4cb000, moved to `lib/d3d7driver` in the
classification): they morph and light vertices on the CPU. Step 3 found that they use no D3D vertex buffers:
they are Im3D pipelines that feed the Im3D TL buffer and index lists (step 3 below).

The game itself touches DirectDraw directly in two places only: the quit prompt (`zrwzgl.c`, GDI surface flip)
and FMV playback (`zmovie.c`).

## Decisions (A chosen in each)

### D1. Cut at RW's device interface (recommended) or higher up

- **A (recommended):** implement RwDevice, the standards and the extension table in C against SDL3 GPU. All
  ported code (game + RW) stays untouched and faithful; the driver is the only new code.
- B: replace RW's render-facing API (RwCamera*, RwIm2D*, RpWorldRender...) with our own calls. Less code to
  emulate, but it cuts through ported, oracle-tested code and loses "RW 3.1 with a different driver".

### D2. Vertex lighting: on the CPU like the original, or in shaders

- **A (recommended):** port the instancing pipelines faithfully (CPU transform/light/fog into a vertex buffer),
  GPU only rasterises. Vertex colours are then bit-identical to the original (they are data, testable in the
  emulator like everything else), which fits "vanilla stays vanilla" and keeps screenshot diffs tight. Cost:
  ~15 pipeline functions plus helpers, partly x87 (one Opus batch).
- B: light in a vertex shader. Less porting, faster, but colours differ slightly (float precision, D3D7's
  lighting formula re-derived) and only screenshots can check them.

### D3. Shaders: compiled offline (recommended) or at runtime

SDL3 GPU needs SPIR-V (Vulkan), MSL (Metal) and DXIL/DXBC (D3D12) per shader.

- **A (recommended):** write HLSL, compile offline with SDL_shadercross into all three, commit the blobs as
  generated headers. No runtime dependency; the shader compiler is only needed when shaders change.
- B: ship SDL_shadercross and compile at startup (larger binaries, slower start, another dependency).

### D4. Vanilla output resolution

- **A (recommended):** render at the selected mode's resolution (as the original does: 640x480 etc.) into an
  offscreen target and scale it to the window/screen with letterboxing. Pixel-faithful; a native-resolution and
  hor+ widescreen option comes later as an opt-in enhancement.
- B: render at window resolution from the start (sharper, but no longer what the original showed).

### D5. Paletted and 16-bit rasters

SDL3 GPU has no paletted textures and spotty 16-bit formats.

- **A (recommended):** every texture is RGBA8 on the GPU; paletted/565/1555/4444 rasters keep the original
  format in a CPU copy (what `RwRasterLock` returns, so game code that writes pixels behaves the same) and are
  converted on unlock/upload.
- B: keep 16-bit GPU formats where available (less memory, platform-dependent behaviour).

### D6. The device-selection dialog

The original asks for the device and mode in a Win32 dialog built from the exe's resources.

- **A (recommended for now):** keep that dialog on Windows in step 1 (it is ported game code); replace it with a
  config file / command line (`-r<w>.<h>.<depth>` already exists) when the exe moves to other platforms.
- B: replace it now with SDL-based mode selection.

### D7. How the driver is verified

Exact pixel equality with D3D7 is impossible (rasterisation rules, driver differences). Proposal: the
pipeline/vertex code is oracle-tested as usual (D2-A); the GPU part is checked by screenshot comparison against
the original running on this PC, at the same frames of a replayed golden trace, with a per-pixel tolerance and
a threshold on the fraction of differing pixels. Needs golden traces recorded first (docs/TRACING.md).

## Not decided here (separate steps)

FMV playback (Cinepak AVI decoder, replaces DirectShow in `zmovie.c`), DirectInput -> SDL3 gamepad/keyboard,
DirectSound -> SDL3 audio, the quit prompt's DirectDraw GDI flip.

## Order

1. Driver skeleton: device system function (open/start/modes from SDL display modes), standards with CPU-only
   rasters, clear + show raster: bmxnative opens a window and shows clear colour. **Done 2026-10-29** (src/driver/,
   docs/NATIVE.md "First traps with the SDL3 driver skeleton"): the menu loop runs with clear + present. What exists:
   - Plug-in: `bmxDriverJumps` (device.c), written by the native loader only; the three tables keep the original
     addresses, unimplemented entries trap by name.
   - Implemented: system function (all requests the original handles), RwRasterRegisterPlugin, core extension entries
     0-3 and the 0x40e plugin ctor/dtor, camera plugin ctor/dtor, camera begin/end update, camera clear, show raster,
     raster create/destroy/lock/unlock/sub raster, set raster context, image find raster format, raster set from image,
     texture set raster, render order hint, the null standard, the four Im2D device entries of the static device table (no-ops in the original; but see step 2).
   - Stubs: RenderStateSet/Get keep the values (0 outside a camera update like the original; caps-based refusals and
     the GPU state come with step 2). Core extension entry 1 only sets z test/write (its Im3D camera state is step 3).
   - Still original addresses (trap when reached): the Im3D device entries and pipeline stages (0x4f69b0, 0x4f6050,
     0x4f6090, 0x4f64e0, 0x4f6a50, 0x4f6b00, 0x4f6b10, 0x4f6b70, 0x4ea960, 0x4eaa10, 0x4f97c0), raster render
     (fast/scaled), raster clear (rect), image get from raster, RGB <-> pixel, palette lock/unlock, native texture
     read/write/size, the instancing pipelines.
   - Decisions taken in step 1 (beyond D1-D7), to review:
     - One sub system "SDL3 GPU H/W". Modes: [0] windowed = desktop size and depth (16/32), then for every size of the
       primary display's full screen modes plus the dialog's five (640x480 ... 1600x1200), ascending, 16 and 32 bit,
       flag exclusive. "Exclusive" modes do not change the display mode (D4: render at the mode size, scale).
     - Display formats: camera/normal rasters 565 at 16 bit, 888 at 32 bit (X8R8G8B8 maps to 888); z rasters 16 bit,
       format 16. Strides are packed rows (DWORD aligned for the CPU copy) instead of the DirectDraw pitch.
     - Texture formats as a D3D7 device without paletted textures: by the image's alpha class (0x4f9ea0's rule) 565
       opaque, 1555 one-bit alpha, 4444 alpha, for 4/8/32-bit images alike. No mipmap caps and no power-of-two or square
       restriction, so RW never sets the mip format bits and never resamples or quantises (the RECLASS assumption in
       RENDERWARE.md). On a real D3D7 HAL with mip filter caps the original forces power-of-two sizes and mip flags for
       textures read with mipmapping: those raster sizes and format bits differ. Max texture size reported 2048, free
       texture memory 64 MB.
     - The CPU copy is allocated on first lock with the RW memory interface; pixel conversion truncates like 0x4eefc0
       but without its "never pure black" raise. No GPU upload yet.
     - Camera rasters are RGBA8 render targets created on first use; clears force alpha 1 (the original's camera
       surfaces have no alpha; DWM shows an alpha-0 swapchain as black). Later draws need an RGB write mask for the same
       reason. Clears cover the whole target (sub-rect clears come with step 2).
     - The window is the game's own HWND wrapped by SDL (`SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER`); SDL subclasses
       it and the game's message loop keeps dispatching; the driver drops SDL's queued events at every present.
     - Camera begin update sets RwMatrixMultiply's routine to 0x54b038 like the original (see open points).
   - Open points found: the original's camera begin update also switches the x87 to 24-bit precision with round-down
     (_rwcseg 0x54b000, restored by 0x54b02e at end update) and routes RwMatrixMultiply to the x87 routine 0x54b038 for
     good. Our ported code runs SSE2, so float results of code that runs inside a camera update, and every
     RwMatrixMultiply after the first update, are not modelled yet. 0x54b038 trapped when first called; since
     2026-10-29 the native loader keeps that routine (0x54b038..0x54b114, self-contained x87 code) as original code
     as a stopgap (docs/NATIVE.md "First trap after the PNG reader").
2. Im2D + Im3D + render states: menus and text. **Done 2026-10-30**: the main menu draws (background, logo, rider
   artwork, the menu wheel with its translucent items) as in the original; docs/NATIVE.md "Step 2: the menu draws".
   - Correction to step 1: the D3D7 driver's Im2D entries are not no-ops. The device table's 0x4eb2c0..0x4eb2f0 are,
     but the device start (0x4dafd0) replaces them in the engine's device with 0x4d8ea0 / 0x4d8c80 / 0x4d9920 /
     0x4d9b00. Im3D draws through them too (its index lists end in RwIm2DRenderIndexedPrimitive).
   - **Shared ports (D2-A), in src/rw with `BMX_ADDR`**, verified at 0 ULP in the oracle harness, so the hook DLL runs
     them as well (against the original's globals; 0x696740.., 0x676bac.., 0x696900..):
     - `rw/d3dim3d.c` (35 functions): the core extension's engine plugin 0x40e (ctor/dtor 0x4eaaa0/0x4eabd0, camera
       begin 0x4eb150, render state hook 0x4eb120, pipeline stages 0x4f6b80), the Im3D transform stack, the
       transform stage 0x4f97c0 (camera space, clip flags, TL vertices, vertex fog index), colours, texture
       coordinates, culling, index lists and the four Im3D draw entries.
     - `rw/d3dclip.c` (18, ported by a sub agent): the polygon/line plane clippers, the 12 edge interpolators
       (perspective and parallel), the triangle and line clippers.
     - `rw/d3dstate.c` (10): RwDevice render state set 0x4d7320/0x4d7328 and get 0x4d7c10 with the caps refusals and
       the state cache, texture raster, blend, fog type/table states.
     - `rw/d3dim2d.c` (17): the four Im2D entries and the alpha sort: translucent primitives (alpha primitive buffer
       on, z test on) are copied with their state into a deferred list and drawn back to front at a flush (z test
       off, the buffer state off, camera end update), merged per record like the original.
   - **The cut**: every IDirect3DDevice7 call of those ports is one of five functions in rw/rwd3d.h
     (`rwD3DDevSetRenderState`, `...SetTextureStageState`, `...SetTexture`, `...DrawPrimitive`,
     `...DrawIndexedPrimitive`, Direct3D 7 arguments). src/hook/d3dcom.c forwards them through the original device's
     vtable (hook DLL and tests: the tests give both sides a fake device whose methods log, so the D3D call
     sequences are compared call for call); src/driver/draw.c implements them on SDL3 GPU.
   - **src/driver/draw.c**: keeps the D3D render and stage-0 states, batches each frame's TL vertices and draws
     (fans and strips become lists; pipelines cached by blend / z / cull / primitive class; samplers by filter and
     address), uploads dirty textures (CPU copy converted to RGBA8, D5) and replays the batch in one render pass on
     the camera raster at the camera's end update, before clears, at the present and before a texture with batched
     draws changes. RGB write mask (targets keep alpha 1), depth clamp instead of clip. Shaders: src/driver/shaders/
     tl.hlsl (vertex: screen pixels to clip space with D3D7's pixel centres, perspective through 1/rhw; fragment:
     stage 0 select/modulate, specular add, vertex fog, alpha test).
   - **Shaders (D3-A)**: SDL_shadercross has no Windows release (CI artifacts need a login), so tools/shaders.sh uses
     DirectXShaderCompiler v1.9.2609 (2026-09-29 release, ~/tools/dxc-1.9.2609/bin/x64/dxc.exe): DXIL (stripped of
     debug and reflection) and SPIR-V (`-spirv`, `[[vk::combinedImageSampler]]`), committed as
     src/driver/shaders/tl_vs.h / tl_ps.h. **MSL is a gap** (needs SPIRV-Cross or shadercross on a machine that has
     it); drvDrawStart refuses a GPU driver without DXIL or SPIR-V.
   - Native glue: the register request stores the raster/camera plugin offsets, the memory functions and the engine
     device where the ports read them (0x697364, 0x697368, 0x69734c, 0x676cdc); the start builds the caps block
     (0x6968f0), runs the ported 0x4dafd0 and sets the initial states like 0x4d7ed0; the camera begin update sets
     the x87 camera mode (below), the device context 0x697340 (the primary context 0x697320, so state sets work only
     inside a camera update, as in the original), the fog range and the draw target; the end update flushes the
     deferred list and the batch and restores the x87. The raster plugin data keeps the three fields the render
     states read at the original's offsets (colour key +4, texture handle +0xc = the raster, format record +0x10
     with the alpha flag).
   - **Decisions taken in step 2, to review:**
     - Ports shared with the hook DLL (`BMX_ADDR` in src/rw) for everything on the CPU, including the functions that
       call Direct3D, with the D3D calls behind the five cut functions. Alternative was native-only ports without
       oracle tests for the D3D-calling half.
     - x87 model: inside a camera update the original runs the x87 at 24-bit precision, rounding down (0x54b000).
       Ported float code of the driver calls `rwD3DRoundBegin/End`, which copies the x87 rounding mode into MXCSR,
       and computes every intermediate in float (PC24 makes the x87 round each operation to single precision);
       FISTP is `cvtss2si` in the current mode. Verified: the emulator honours PC/RC, the transform test fails
       without the mirroring and passes with it. Known limit: x87 intermediates keep their 15-bit exponent, so
       results that overflow or go denormal in single precision differ (test inputs stay in realistic ranges).
       The native camera begin update now really switches the x87 to the camera mode (the kept original routine
       0x54b038 runs in it, like in the original); game code ported to SSE2 still runs at nearest/double inside
       the camera update (pre-existing open point).
     - Caps of the native device: dither; all z compares, blends and alpha compares; flat/Gouraud colour and alpha;
       vertex fog (FOGFLAT|FOGGOURAUD, as real cards of the time); perspective; filters nearest/linear only (no mip
       filter caps, matching step 1's texture formats); address wrap/mirror/clamp (no border: SDL3 GPU has no border
       colour). With these, 0x4d7ed0's choices are: alpha test GREATER 0 when blending, linear filtering.
     - No colour-keyed textures (raster data +4 always 0); the colour key state is tracked but not drawn.
     - Alpha test compares at 8-bit precision in the shader (the reference is a byte); fog colour from
       D3DRENDERSTATE_FOGCOLOR blended by the specular alpha (vertex fog).
     - D3D7's TL vertices are not clipped by z on the GPU (depth clamp); x/y clipping is the GPU's.
   - Open points:
     - Fog density 0x4d6590 (x87 exp tables with f2xm1/fscale) is still the original's (int3 in the native exe); the
       game never sets that state. The camera begin update's exponential fog tables (fog types 2/3, also f2xm1) are
       not built natively; the game uses fog type 1 (linear).
     - Raster render (fast/scaled), raster clear rect, image from raster, RGB <-> pixel, palette lock/unlock, native
       texture read/write/size: not reached by the menu, still trap.
     - Culling winding through SDL3 GPU (D3DCULL_CW/CCW) is mapped but unverified: the driver sets D3DCULL_NONE and
       nothing changes it in the menu.
     - Screenshots compared by eye only (D7's diff harness is step 4). The final shader headers (DXIL stripped of
       debug/reflection) were regenerated after the screenshot runs and have not been run in a window yet.
3. Instancing pipelines (D2) + world/atomic rendering: levels. **CPU port done 2026-10-30; native runs still to do**
   (no window could be opened while the user was gaming). The first trap was 0x4c92d0 from rwIm3DPipelineRun
   0x4e8600, some time after frame 1500 of the 4000 frame menu run.
   - **What the pipelines are.** rpWorldPipelineInit / rpAtomicPipelineInit (rw/clump.c) build two Im3D pipelines
     out of the driver's stages; rwIm3DPipelineRun calls instance, stage 1 (the ported Im3D transform 0x4f97c0, which
     the device's pipeline request installs), stage 2, stage 4 (the world's light walk, which calls the stage data
     0x4c9790 per light), stage 5 and stage 6. There are **no Direct3D vertex buffers**: the instance stage caches
     the vertices (Im3D vertex layout, 0x1c or 0x24 bytes) and the triangles in a resource entry of the atomic or
     sector, the transform writes the Im3D camera and TL buffers, and the render stage draws through the Im3D index
     lists, which end in RwIm2DRenderIndexedPrimitive (step 2's cut). **The cut needs no extension** and
     src/driver/draw.c no change: world and atomic geometry reaches SDL3 GPU as indexed TL triangle lists, like
     Im3D. Texture, filter, addressing and vertex alpha come from RwRenderStateSet (rw/d3dstate.c).
   - **Shared ports (`BMX_ADDR`, src/rw/d3dpipe.c, 14 functions; 0x4eafa0 in rw/d3dim3d.c)**, oracle tests in
     tests/cases/test_rwd3dpipe.c (15 cases, 0 ULP, under the camera's x87 mode like step 2):
     - instancing: atomic 0x4c8e30 (resource entry reused while mesh serial, vertex type and stride match; refreshed
       by the geometry's lock bits: texture coordinates, prelit colours, triangles, positions / normals; LTM x view
       matrix) with the morph target interpolation 0x4c8ba0 (EAX = atomic); world sector 0x4c92d0 (built once;
       signed-byte normals times 1/128; the camera's view matrix);
     - colours: stage 2 0x4c9800 (prelit RwRGBA into the camera vertices' float colours), the light walk callback
       0x4c9790 (marks the instance lit, calls RpLight.worldExtCallback), the five light routines ambient 0x4c9b20,
       directional 0x4c9c20, point 0x4c9e20, spot 0x4ca130, soft spot 0x4ca5b0 (white and coloured variants each;
       light position / axis into object space by the inverse matrix the walk passes, radius scaled), stage 5
       0x4c9880 (clamp to 0..255 and FISTP into the D3DCOLOR, unless the instance modulates by the material colour);
     - render: stage 6 0x4cac00 (per mesh: Gouraud, vertex alpha for translucent material colours, the texture's
       raster / filter / addressing / border colour, each used vertex coloured once through a bit per vertex
       (0x4caad0 modulated by the material colour, 0x4cab70 plain; the bits come from 0x4eafa0), then the
       triangles through the driver's cull / clip / index function pointers and an index flush).
   - Layouts named on the way (rw/rwd3d.h): RwD3DIm3DWork is also the pipelines' resource entry (serial number,
     flags, triangle count / stride / offset / pointer, vertex type, lit flag, mesh header); RwD3DCamVertex has float
     r, g, b, a at 0x10; 0x696a5c / 0x696a60 are the vertex bits (rwD3DVertexBits / Size, formerly "spare" words).
   - **Decisions taken in step 3, to review:**
     - **Negation under round-down.** The spot lights' cone test computes `dy - dir.y * -d` (FCHS, FMUL, FSUBR).
       Rounding down, `y * -d` is not `-(y * d)`, but clang folds the negation into the subtraction (`dy + dir.y * d`)
       even at -O0 (a DAG combine that is only valid when rounding to nearest); `#pragma STDC FENV_ACCESS ON` did not
       stop it. The port makes the negated factor and the product opaque (`negMul`, an empty asm barrier like camSub
       in rw/camera.c). Found by the soft spot test (1-4 ULP), confirmed with exact rational arithmetic. Step 2's
       driver ports have no float negations.
     - The vector helpers the light routines call (RwV3dTransformPoints / Vectors, _rwV3dNormalize, _rwFastSqrt,
       _rwFastInvSqrt) are stubbed in the tests (exact permutations / bit tricks on both sides). Natively and in the
       hook DLL they are the earlier SSE2 ports; they run with MXCSR in the camera's rounding mode (rwD3DRoundBegin
       covers the whole routine) but not at 24-bit precision, so a transformed light position or axis can differ
       from the original in the last bit (the pre-existing "game code at SSE2 inside a camera update" open point).
       _rwFastSqrt / InvSqrt are table lookups and exact.
     - The pipeline tables 0x6767f0.. moved from rw/clump.c to rw/rwclump.h (shared). rpWorldPipelineInit fills the
       tables the *atomic* instance stage reads and installs that stage, rpAtomicPipelineInit the world sector's:
       the names of those two ported functions look swapped (not renamed here).
   - Open points: the native runs (the menu up to where 0x4c92d0 trapped, then the frontend's 3D model and a level);
     the D7 screenshot diff; the remaining standards (raster render fast / scaled, raster clear rect, image from
     raster, RGB <-> pixel, palette lock / unlock, native texture read / write / size; fog density 0x4d6590) still
     trap and get ported when a native run reaches them. The game calls RwRasterLockPalette / RwRGBAFromPixel only
     for paletted rasters (zrwtxr.c), which this driver never creates.
4. Screenshot diff harness against the original (D7).
