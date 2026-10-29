# SDL3 render driver for RenderWare (P4)

Status 2026-10-29: decided. The user chose option A for D1..D7 (the recommendations below).
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
classification): they transform, light, fog and morph vertices on the CPU into D3D vertex buffers.

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
     texture set raster, render order hint, the null standard, the four Im2D device entries (no-ops, as in the original).
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
2. Im2D + raster render: menus and text (the frontend is mostly Im2D).
3. Instancing pipelines (D2) + world/atomic rendering: levels.
4. Screenshot diff harness against the original (D7).
