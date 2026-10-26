# Replacing RenderWare (P4)

The original statically links RenderWare 3.1 (stream version 0x0301; library code above 0x4a4a80, plus
`_rwcseg`). Our ported code calls 197 RW functions through `ORIG_FN` (inventory 2026-10-26, list via
`ORIG_FN` declarations with addresses >= 0x4a4a80). `bmxnative.exe` traps on the first one,
`RwEngineInit`.

## Constraint: RW 3.1 object layouts

Ported code reads RW objects directly (frame matrices and LTMs, clump/atomic/geometry/material fields,
world sectors, rasters) with RW's own layouts, and RW's plugin registry hands out offsets into those
objects (`RpClumpRegisterPlugin`, `RwTextureRegisterPlugin`, the anim/bone/LOD/PVS plugins). Whatever
replaces RW must keep RW 3.1's object layouts and plugin mechanics. A clean-room renderer with its own
internals doesn't fit under the game; the replacement is "RenderWare 3.1 with a different driver".

## Groups (197 functions)

| Group | Examples | Count | Affects game state? |
|---|---|---|---|
| Engine, subsystems, memory, errors | rwEngineInit/Open/Start, rwEngine*SubSystem*, *VideoMode*, RwOsGetMemoryInterface, RwResourcesSetArenaSize, RwEngineGetMetrics, RwErrorGet, rwExit | ~17 | no (setup) |
| Plugins | rpWorld/Anim/Bone/LodAtomic/Pvs PluginAttach, RpClumpRegisterPlugin, RwTextureRegisterPlugin, RpSkinClumpSetup | ~8 | layouts |
| Math and frames | RwMatrix* (Create/Destroy/Multiply/Rotate/Scale/Translate/Update), RwFrame* (Create/Destroy/GetMatrix/LTM/Translate/SetIdentity/ForAllChildren/UserData/UpdateObjects/Count), rwV3dTransformPoints/Vectors | ~19 | **yes** via zrwcol (collision queries), camera |
| Collision | rwWorldIntersectTriangles/Objects, rwObjectIntersectTriangles | 3 | **yes** (zrwcol.c) |
| World and sectors, PVS | RpWorld* (Create/Destroy/StreamRead/Add/Remove/ForAll*/BBox/Origin/Materials), RpWorldSector*, rwWorldPvs*, rwScene*, rwWorldRender | ~30 | data layout; PVS only for drawing |
| Clumps, atomics, geometry, materials | RpClump* (StreamRead/Clone/Destroy/ForAllAtomics/Render), RpGeometry* (Lock/Unlock/Flags/MorphTarget/TexCoords/Counts), RpMorphTargetGetVertices, RpMaterial*, rwModel* | ~25 | data layout (vertices are read by game code) |
| Lights | RpLight* (Create/Destroy/Color/Radius/Cone/Frame/Flags/Type), rwLight* | ~16 | no |
| Textures, images, rasters | RwTexture* , RwTexDictionary*, RwImage*, RtBMP/PNGImageRead, RwRaster* (Create/Lock/Palette/SubRaster/Context/Show) | ~45 | no (pixels only) |
| Camera | RwCamera* (Create/Destroy/Frame/Raster/ZRaster/Clip planes/Fog/View window/Projection/Begin/EndUpdate/Clear) | ~21 | no (drawing) |
| Immediate mode, render state | RwIm2D*, RwIm3D*, rwRenderStateSet | ~8 | no |
| Streams and files | RwStreamOpen/Close/FindChunk, RwOSGetFileInterface, rwfopen/fclose/fseek/ftell | ~8 | data |
| Charset | RtCharset* | 4 | no |

## Options

1. **Port RW 3.1 from the exe, like the game code** (decompile, oracle-tested), and replace only the
   bottom layer, RW's Direct3D 7 driver (device, rasters, Im2D/Im3D, pipelines), with SDL3 GPU.
   Bit-exact by construction, layouts automatically right. Cost: the reachable part of the RW library
   (size unknown yet; bundles were only exported up to 0x4a4a80, so the first step is re-running the
   Ghidra export over 0x4a4a80..0x54b000 and measuring the call closure of the 197 functions).
2. **librw** (open-source MIT reimplementation of RW 3.6, the base of re3/reVC) adapted to 3.1 layouts.
   Has GL/D3D backends and stream readers ready, but 3.6 differs from 3.1 in layouts and behaviour, and
   collision/math would not be bit-exact without verification.
3. **Hybrid:** option 1 for everything that affects game state or data layout (engine core, plugins,
   math and frames, collision, stream readers that build worlds/clumps/textures), and our own code (with
   librw as a reference) for the presentation-only groups (camera, rasters, immediate mode, lights,
   charset), drawn with SDL3 GPU.

Recommendation: 3.

## Measured (2026-10-26)

Bundles for the RW range: `BUNDLES_OUT=work/bundles-rw BUNDLES_FROM=4a4a80 BUNDLES_TO=54b000
GHIDRA_PROJECT=ngbmx-rw tools/ghidra/export.sh work/NgBMX.exe work/labels.raw` (1155 functions, 319 KB,
including the statically linked CRT, libpng and zlib). Static call closure of the 197 entry functions:

| Group | Entries | Functions in closure | KB |
|---|---|---|---|
| engine/memory/errors | 16 | 42 | 5.2 |
| plugins | 8 | 37 | 4.7 |
| math/frames | 20 | 30 | 5.2 |
| collision | 3 | 44 | 20.3 |
| world/sector/pvs/scene | 29 | 160 | 39.8 |
| clump/geometry/material | 22 | 113 | 34.5 |
| streams/files | 8 | 50 | 11.0 |
| texture/image/raster | 44 | 216 | 90.8 |
| lights | 15 | 27 | 4.2 |
| camera | 20 | 30 | 1.5 |
| immediate/renderstate | 8 | 14 | 0.9 |
| charset | 4 | 43 | 6.8 |
| **total, unique** | 197 | 607 | 186.8 |

Notes: texture/image/raster is mostly libpng + zlib behind RtPNGImageRead (pixels only: any correct
decoder, e.g. stb_image, gives the same result). Camera and immediate mode look tiny because their work
goes through the device driver's function-pointer table (the D3D7 driver), which a static closure can't
follow; that driver is what SDL3 replaces. The faithful-port part (engine, plugins, math, collision,
world, clump, streams) is roughly 100-120 KB, about a quarter of the game code.

## Tooling

`data/functions-rw.csv` (same columns as `functions.csv`) lists the RW functions to port: the static call
closure of the entry functions (ORIG_FN declarations and BMX_ADDR annotations at addresses >= 0x4a4a80),
713 functions. 409 are `orig` (about 104 KB), grouped into `src/rw/<group>.c` (engine, plugins, math,
collision, world, clump, stream, texture, light, camera, im, charset, and `core` for internals reached
from three or more groups: errors, plugin registry, free lists). 304 are `skip` with file `lib/crt`,
`lib/libpng` or `lib/zlib`: the MSVC CRT and the decoder behind RtPNGImageRead, which the replacement
takes from the platform and a real image library. The heuristics are in the docstring of `tools/rwmap.py`.
Group assignment is a first guess (address ranges per RW module, labels, who reaches a function): move a
function by editing its file column and keeping it when you regenerate.

```sh
export BMX_WORK=<repo>/work        # only in a git worktree: work/ is gitignored and lives in the main checkout
python3 -I tools/rwmap.py          # regenerate data/functions-rw.csv from work/bundles-rw, work/labels.raw and src/
python3 -I tools/slice.py plan --rw                          # slices of whole groups, bottom-up
python3 -I tools/slice.py packet rw/math.c rw/core.c         # packet -> $BMX_WORK/slices/rw-math+rw-core.md
python3 -I tools/progress.py       # game and RW progress, reported separately (skip rows not counted)
```

A packet works like a game one (pseudo-C, call graph, disassembly for x87 functions, ORIG_FN list for
callees outside the slice) and also lists the `skip` library callees. Ported RW code goes under `src/rw/`
with `BMX_ADDR` annotations like the game code; `progress.py` recognises them and `CMakeLists.txt` builds
`src/rw/*.c`. Rerun `rwmap.py` after porting: ported rows keep their file and status.

Known gaps: calls through function-pointer tables (device driver, plugin callbacks) are not in the
closure, and two callbacks the game passes by address (`rwBmpImageWrite` 0x4e7eb0 and
`rwWorldPvsDefaultCallback` 0x4cd510) are not Ghidra functions, so they have no bundle or CSV row yet
(create the functions in Ghidra and re-export to add them).

## Next

1. Port engine core + memory + plugins (the first traps in `bmxnative.exe`), oracle-tested like the game.
