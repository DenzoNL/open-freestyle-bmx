# Ray tracing / path tracing: notes (2026-11-01, not started)

The user asked how hard it would be to replace the static lighting with ray tracing or path tracing. Parked for
later; these notes keep what was found. See also PLAN.md "Enhancement backlog" (Vulkan RT backend, Remix, level
remaster).

## What the game gives us

- **No light data.** The levels' lighting is baked into RenderWare prelit vertex colours, painted by the artists
  (lamps in the indoor parks, shade under ramps). There are no light entities, no materials beyond one low-res
  texture per surface, no normal maps. Dynamic objects (rider, bike, props) get RW's ambient + directional lights.
  Any relighting therefore needs authored content per level: a sun and sky for the outdoor parks, the lamps of the
  indoor ones, and material properties for the textures. That is most of the work, not the tracer.
- **Pre-transformed vertices.** RenderWare 3's D3D7 driver transforms, lights and clips every vertex on the CPU
  (the ported pipeline in src/rw: d3dpipe.c, d3dclip.c, d3dim3d.c) and hands Direct3D screen-space TL vertices
  (`XYZRHW`); src/driver/draw.c draws exactly those (docs/SDL3-DRIVER.md). The GPU never sees world space.
- **What we do have:** the whole pipeline is ours, so the object-space geometry, the world sectors, the matrices,
  the textures and the camera are all reachable at the point where the CPU pipeline runs; the high frame rate work
  (docs/HIGHFPS.md) keeps the previous tick's poses, which motion vectors and temporal denoisers need.

## RTX Remix

Remix (NVIDIA, D3D9 fixed function only) builds its scene from world-space draws plus the transform matrices; it
treats pre-transformed (`XYZRHW`) draws as screen-space UI. **So Remix on the original exe (through a DirectX 7 to 9
wrapper) cannot path-trace the levels** — the game never submits world-space geometry. For our own exe it would
take a D3D9 fixed-function backend fed from inside the ported pipeline *before* the CPU transform (object-space
vertices + world / view / projection matrices, RW's lights as D3D lights), i.e. a second path through d3dpipe.c.
Possible, but not a quick experiment. Windows + NVIDIA only.

## Options

| Option | Result | Effort | Platforms |
|---|---|---|---|
| Baked GI per level (path traced once at load, or cached in the user folder) | soft sky light, bounce light, better shadows, written back as vertex colours (coarse on this low-poly geometry) or lightmaps (needs a UV unwrap, e.g. xatlas, and a second texture stage) | moderate; the lights per level are the content work | all, incl. Android |
| Hybrid real-time RT on top of the baked lighting | ray-traced rider shadow, AO, reflections | high: a Vulkan RT backend (SDL3 GPU has no ray tracing) fed with world-space geometry from the pipeline | RT-capable desktop GPUs |
| Full real-time path tracing (Quake II RTX style) | everything dynamic | highest: the backend, PBR materials for every texture, authored lights, a denoiser (SVGF / FidelityFX), tuning with the 29.4 Hz tick and interpolation | desktop NVIDIA / AMD |
| Remix via a D3D9 backend | Remix's path tracer and its toolkit for lights and materials | high (the backend described above) | Windows + NVIDIA |

Recommendation when this comes up again: after the 64-bit work (X2 puts the renderer behind a backend interface
anyway), start with a world-space export of one level (also the first step of the level remaster in PLAN.md) and a
baked-GI prototype; it works on every platform and shows what the levels gain from real lighting before a real-time
backend is built.
