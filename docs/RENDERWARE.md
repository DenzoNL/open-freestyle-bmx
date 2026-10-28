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
closure of the entry functions (ORIG_FN declarations and BMX_ADDR annotations at addresses >= 0x4a4a80) and the
closure of the functions reached only through pointers (engine and plugin tables, default callbacks, constructors
and destructors of the plugin registry, stream callbacks, free list slots). Rows are `orig` (to port, grouped
into `src/rw/<group>.c`), `ported`, or `skip`: `lib/crt`, `lib/libpng` and `lib/zlib` (the replacement takes those from
the platform and a real image library) and `lib/d3d7driver` (the Direct3D 7 device driver: our SDL3 driver
replaces it, it is never ported). The heuristics are in the docstring of `tools/rwmap.py`; in short:

- Pointer targets: `RW_CB`/`BMX_FNPTR` addresses in `src/`, initial values of `ORIG_VAR` globals, dwords in the
  exe's data sections and `PUSH`/`MOV` immediates in RW code that are function starts.
- Driver: the closure of the device table (0x566fe4), the standard functions table (0x56701c), the core extension
  table (0x69bee0) and every function with a "D3D" community label, minus what the other roots reach. What no root
  claims but only serves rendering is moved by hand-checked rules (`RECLASS` in tools/rwmap.py, one comment each,
  kept on regeneration; see "Classification of the pointer-only functions" below).
- Pointer targets need a Ghidra function: `tools/ghidra/SeedPtrs.java` creates them (RenderWare pads with
  `lea esi,[esi+0]` style bytes behind a ret). `rwmap.py` lists pointer targets without a bundle on stderr; re-export
  with `BUNDLES_OUT=work/bundles-rw BUNDLES_FROM=4a4a80 BUNDLES_TO=54b000 GHIDRA_PROJECT=ngbmx-rw tools/ghidra/export.sh
  work/NgBMX.exe work/labels.raw` (the Ghidra project must not live under a path with a `.` directory such as
  `.claude/worktrees`). Still missing: 0x4b0f20 (clump module open), 0x4a9f70 (CRT) and the `_rwcseg` block at 0x54b000.
- Group assignment is a first guess (address ranges per RW module, labels, who reaches a function): move a function by
  editing its file column and keeping it when you regenerate. Rows of `skip` and `ported` functions keep their file.

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

Known gaps: the 305 `orig` rows the pointer pass added (97 KB) are sorted below; 167 `orig` rows (44 KB) are left
(2026-10-28, after the x87 batches: RwMatrixTransform, the camera clip planes, the BSP line and sphere walks,
rpBspStreamRead, then RpWorldCreate, the sector line-triangle test and the anim plugin's add-sequence).

## Classification of the pointer-only functions (2026-10-28)

Rule: port what determines data or behaviour (objects, streams, plugin callbacks that build or destroy objects, math,
collision, image loading into an RwImage); the SDL3 driver replaces what only puts pixels on screen. Checked on the
pseudo-C of every row; none of the 305 calls a DirectDraw/Direct3D import (the driver code goes through the device
table, the exe's own data and the engine's standard functions).

Moved to `lib/d3d7driver` (`skip`, 50 rows, 33.3 KB) and to `lib/crt` (6 rows, 0.3 KB):

| Range | Rows / bytes | What and why |
|---|---|---|
| 0x4c8ba0..0x4cb000 | 14 / 9.7 KB | Instancing pipelines of world sectors and atomics: resource arena entries, vertex buffers in the D3D vertex format (0x20 byte vertices, 0x28 byte scratch vertices), morph interpolation, lighting and uv effects into them, the dynamic light stage. Hooked in by `rpWorldPipelineInit`/`rpAtomicPipelineInit`; only the device draws the result. |
| 0x4eacc0..0x4eb100 | 4 / 0.4 KB | Instance buffer allocators and the driver's instance hook (0x4eb0d0 calls a driver function pointer); callers are the pipelines above and the D3D mesh code. |
| 0x4ccd30..0x4ce000, 0x4ce250 | 11 / 6.0 KB | PVS generation: renders the world from sample points into an offscreen camera, reads the pixels back (RwImageSetFromRaster) and ORs the visible sectors into the PVS rows. Only `zrwWorldSetupPvs` uses it (the game loads precomputed PVS data); the result only culls drawing. The row setup (0x4ce620), counting and the runtime PVS callbacks stay `orig`. Unsure: revert by deleting the two rules if the in-game PVS build must work with SDL3. |
| 0x4e0160 | 1 / 2.5 KB | Converts the mip levels of a paletted raster (only the default mipmap generator 0x4e1140 calls it). |
| 0x4ef450..0x4f2e00 | 18 / 14.2 KB | Colour quantiser (histogram, variance minimisation, nearest palette entry), dithering and image resampling that turn an RwImage into the paletted or resized pixels of a raster. The raster format and size come from the device (0x4e7fc0 asks it) and the SDL3 driver offers 32 bit rasters of any size, so they never run. The texture readers 0x4e1b50/0x4e1470/0x4e0b10 keep calling them as `ORIG_FN`. `_rwpathisabsolute` (0x4f2ec0) is outside the range and ported. |
| 0x4f9c90, 0x4f9db0 | 2 / 0.5 KB | Block averages for the D3D3 raster standards (data at 0x567290). |
| 0x4ee1d0..0x4ee300, 0x4e8290 | 6 / 0.3 KB | `lib/crt`: rwstricmp/rwstrupr/rwstrlwr/rwstrchr/rwstrrchr and rwfexist, already implemented in src/native/crt.c. |

Tri-strip generator 0x4c60a0..0x4c8870 (9 functions, 10 KB) -> `lib/unreached`: `_rpMeshGenerate` (ported) calls it only
for geometry flagged rpGEOMETRYTRISTRIP, and an in-game breakpoint trace of the original (2026-10-28: boot, menus, level
load, riding) hit `_rpMeshGenerate` but none of the nine. If bmxnative ever traps in that range, port it after all.

Kept as `orig` although only render code calls them (unsure): `RwResourcesAllocateResEntry`/`RwResourcesUseResEntry`
(0x4dc460, 0x4dc680, public arena API the SDL3 instancing will use), `RpMaterialGetColor` (0x4b9330) and
`RpWorldSectorGetBBox` (0x4c2240) (accessors). Two bodies behind 4 byte stubs are folded into the ported stub function
(`FOLDED` in rwmap.py, `skip`): 0x4c24c4 (RpLightDestroy) and 0x4b5ab4 (RpGeometryDestroy).

Remaining to port, per file:

| File | Rows | Bytes | Notes |
|---|---|---|---|
| rw/world.c | 31 | 3.4 KB | camera/light sync (0x4b7730, 0x4b7bb0, 0x4b7e80 with x87), PVS plugin callbacks (0x4cc..), WorldOpen/Close, frustum sector walk 0x4ba360/0x4ba430 |
| rw/clump.c | 8 | 3.9 KB | RpBuildMesh 0x4c5bd0/0x4c5e10, RpGeometryStreamWrite 0x4b6350, 0x4b77..0x4b7e80 |
| rw/texture.c | 24 | 13.2 KB | texture readers 0x4e1470/0x4e1b50/0x4e0b10, default mipmap generator 0x4e1140, RwImageRead/ApplyMask/FindFileType/path walk 0x4e3c08..0x4e53c0, BMP writer 0x4e7860.. |
| rw/engine.c | 38 | 6.4 KB | anim plugin 0x4fbb30..0x502a40 (clump/frame data, stream callbacks), resource entries. The anim rows belong in plugins.c next to RpAnimPluginAttach and its engine module (0x4fc7fc moved there when ported); move the rest when they are ported |
| rw/plugins.c | 28 | 4.2 KB | bone plugin 0x502b00..0x505830, skin helpers 0x4fbf30..0x4ff960, anim node helpers 0x4fc540 (frame walk), 0x5029e0 |
| rw/camera.c | 8 | 1.7 KB | view matrices 0x4ddd40/0x4de2a0 (x87), CameraSync/BeginUpdate/EndUpdate |
| rw/stream.c | 20 | 0.6 KB | stream plugin vtable stubs 0x4e32f2..0x4e335f |
| rw/math.c | 1 | 0.1 KB | `_rwMatrixClose` |

Opus batch done (2026-10-28): RpWorldCreate 0x4bb080 (rw/world.c), the sector line-triangle walk 0x4be560 with its
Moeller-Trumbore kernel 0x4beab0 (EAX/EDX/ECX, stack slots 4..6 and the length in ST0; both moved to rw/collision.c,
where the other per-sector tests are) and rpAnimClumpAddSequence 0x4fc7fc (WorldCopyClumpExt's callee, moved from
engine.c to rw/plugins.c). Large without x87 (Sonnet, slow):
0x4b6350, 0x4e1470, 0x4e1b50, 0x4e3eb8, 0x4e4c50.

x87 porting notes from that batch: Ghidra prints `FMUL ST5` both for `ST0 = ST0 * ST5` and for `ST5 = ST5 * ST0`;
decode register forms from the bytes (D8 vs DC/DE) or with capstone. LLVM treats the sign of a NaN as unspecified:
it rewrites `a - b * k` as `a + b * -k` and packs pairs of subtractions into vector adds of negated operands, which
flips the sign of a NaN that the x87 keeps (and the sign bits feed the frustum planes' `closest` bytes). The camera
and line walk ports subtract through an empty `asm` that hides the subtrahend, and read the constant factors from
the exe's data as the original does. Where two different NaNs meet, the x87 keeps the larger significand and SSE the
first operand; the tests of these functions use no NaN inputs (infinities, which generate NaNs, are in).

Test pitfall: stubs that run during the call under test may draw from `dt_u32()` (DLL state, part of the snapshot both
sides start from) but not from `dt_fill()`, which the harness serves from its own state: the two sides then get
different bytes and every iteration diverges.

## Known issues

None open. Resolved: `rpBspStreamRead` (0x4c36a0) was disabled for "diverging on nested plane sectors". The port was
equivalent to the original; the divergence came from the test fixture. Its random stream script desynchronised the
reader (a failed scripted `RwStreamRead` does not advance the script, and the sector reader only checks the prelight
read against 0), so sector counts became garbage, both sides ran out of the emulator's instruction budget, and the
budget cut them off at different points. `RpWorldStreamRead_nested` in tests/cases/test_rwworldstream.c now writes a
well-formed tree (up to 8 planes, 4 levels deep, format bit 0x40000000 set at random) and the port is hooked again.

## Next

1. Port engine core + memory + plugins (the first traps in `bmxnative.exe`), oracle-tested like the game. Done up to
   the driver boundary (docs/NATIVE.md: first trap 0x4eac90, the driver's extension attach).
2. Plug in the SDL3 driver at the device table / standards table / extension table (it also takes over what
   "Classification" moved to `lib/d3d7driver`: instancing pipelines, PVS generation, palettised raster formats).
3. Port the rest of the pointer-only code (table above): the anim and bone plugin callbacks (0x4fbb30..0x505830), the
   texture readers and image loaders (0x4e0b10..0x4e53c0), the mesh builder (0x4c5bd0..0x4c8870) and the world
   plugin's sync callbacks. Done in this pass: world plugin callbacks of cameras, atomics, clumps, lights, the tie and
   material modules, `rpWorldSectorGetWorld`, the mesh stream plugin (0x50e), the LOD atomic plugin (0x112) with its
   libpng glue, the clump/light module callbacks and the default texture read callback.
