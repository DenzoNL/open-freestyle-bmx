# Which layouts matter on 64-bit (X2b, 2026-11-01)

Slice X2b of docs/PORTING.md "X2 plan": for every struct whose layout asserts fail when the native build is compiled
for x86_64, why its layout matters. Classes:

- **FILE**: bytes from a file (or written to one) land in the struct as a whole. Needs an on-disk type with an
  explicit reader / writer (X2c); the asserts move to the on-disk type.
- **EXTERNAL**: a fixed external ABI (Windows, DirectX, the RW driver interface). In every case found, our own code
  defines both sides (src/platform stand-ins, src/driver), so the layout can change freely, provided the literal
  sizes that go with it change too.
- **MEMORY**: in-memory only. The assert can become 32-bit only (it still documents the oracle's layout), but many of
  these structs are also reached through literal offsets, literal sizes or a second type laid over the same memory.
  Those are hazards in their own right and are listed with the struct and again in the hazard sections below.

## How the list was made

`tools/probe64.py` (removed since; a copy with 3 workers, under `nice -n 19`) with llvm-mingw 20261020 x86_64 clang:
**1750 distinct failing asserts in 75 files**; without src/data/layout.h (1311) and src/data/imports.c (21, both
going away in X2a) that leaves **418 asserts in 73 files**, which group into **169 (header, struct) rows, 167
distinct structs** (ZbspLevelFace fails in ngifzbsp.h and ztaginf.c, RwGlobals in rwcore.h and plugins.c).

## Result in one paragraph

Almost nothing loads file bytes straight into a pointer-bearing struct. The original's loaders (Z-Axis and
RenderWare alike) mostly read each record into a local header type or an `int32_t header[]` and copy field by field,
and RenderWare never stores pointers in a stream. **7 structs are FILE**, all small: the `.zsd` sound bank image
(ZsdHeader, ZsdSample, ZsndSeqHeader, relocated in place), the demo replay header (Replay), the zprim font asset
(ZPrimFont, one runtime pointer slot inside the file data) and two RW plugin chunks (RpSkinBoneBlock,
RpLodAtomicData). **SAVE.DAT contains no pointers** and none of the failing structs. **8 are EXTERNAL**, all with
both sides ours. The other **152 are MEMORY**. The real 64-bit work is in the MEMORY hazards: about 70 literal
allocation sizes (free lists, plugin registrations, mallocs), about 60 literal byte offsets into pointer-bearing
structs, a dozen pairs of types that describe the same memory, and one real heap overflow
(zrwmodelcreate.c:178).

## Summary table

Paths are under src/. "lit" = a literal byte size or offset that has to become `sizeof` / `offsetof` / a field.

| Struct | Header | Class | Evidence | Notes |
|---|---|---|---|---|
| **FILE** | | | | |
| ZsdHeader | bmx/pc/zge/sound/winload.h:96 | FILE (.zsd) | winloadRegister winload.c:71-150; offsets to pointers in place :98-103 | 6 table pointers hold file offsets |
| ZsdSample | winload.h:51 | FILE (.zsd) | winload.c:145 writes the resident `data` pointer into the file record (+0x08) | |
| ZsndSeqHeader | bmx/com/zge/zsndseq.h:52 | FILE (.zsd track def) | same record as ZsdTrackDef; winload.c:124 `data += (uint32_t)(uintptr_t)image`; bmxsound.c:355 casts | 32-bit add, also phrase entries winload.c:119 |
| Replay | bmx/com/action/replay.h:13 | FILE (Replay.ZAL demos; dev .rpy writer) | action.c:828 zassetRead over the header from +0x0c (replay.c:449-466); writer overlay.c:2309-2314 | `cursor` (0x34) is a pointer inside the file bytes; replaySetMode resets it (replay.c:374, :413) |
| ZPrimFont | bmx/com/zge/zprim.h:157 | FILE (zprim .ZAL font) | zprim.c:1441 zassetRead into a DMM block; zprim.c:1468 `data->pages = NULL`; :1907-1917 | `pages` (0x0c) is a runtime pointer slot in the file image; glyph records follow at 0x18 |
| RpSkinBoneBlock | rw/rwplugins.h:134 | FILE (RW chunk 0x10e) | rpBoneGeometryRead plugins.c:907-925 reads 12 raw bytes per block, sets `bone = NULL` | array sized `length/3*4` (16-byte blocks) |
| RpLodAtomicData | rw/rwplugins.h:310 | FILE (RW chunk 0x112) | LODAtomicCallBackReadChunk plugins.c:2735-2783 reads the 0x40-byte chunk raw (`RwStreamRead(stream, lod, chunkLength)`); writer plugins.c:2786-2804 writes the live struct | `levels[10]` pointers in the file only tested for non-NULL; uncertain whether the game's DFFs contain this chunk |
| **EXTERNAL (both sides ours)** | | | | |
| ZpadDiVtbl | zge/pc/pcinput.h:67 | EXTERNAL (IDirectInputDevice7A vtable) | platform/diface.h, platform/input.c:996 | `uint32_t pad` slots must become pointer slots; BMX_INPUT_CHECK (inputcheck.c) uses the real dinput.dll (32-bit only) |
| ZpadDirectInputVtbl | pcinput.h:140 | EXTERNAL (IDirectInput7A vtable) | platform/input.c:1085 | as above |
| ZMovieWaveHeader | zge/zmovie.h:52 | EXTERNAL (WAVEHDR) | zmovie.c:200-209; other side platform/waveout.c:46 | lit 0x20 header size zmovie.c:93, :259 |
| WaveHeader | platform/waveout.c:57 | EXTERNAL (WAVEHDR) | checked against kHeaderSize 0x20 waveout.c:241/255/274 | `WaveOut.window` is uint32_t |
| DsBufferDesc | bmx/pc/zge/sound/winds.h:137 | EXTERNAL (DSBUFFERDESC) | stand-in platform/dsface.h:75 already guarded by `sizeof(void*) != 4`; dsound.c:1078 accepts sizeof or 0x14 | same field order both sides |
| DrvRasterExt | driver/driver.h:94 | EXTERNAL (RW raster plugin of the driver) | registered with lit 0x20 device.c:194 | rw/d3dstate.c:80 `raster + ext + 0xc`, :100 `ext + 0x10` |
| RwDevice | rw/rwcore.h:111 | EXTERNAL (driver function table) | filled by src/driver | |
| RwFileFunctions | rw/rwcore.h:167 | EXTERNAL (file interface) | `_rwFSOpen` stream.c:41 with our CRT | zrwfile.c:311 memcpy into ZrwFileOps (another type) |
| **MEMORY, game (bmx)** | | | | |
| PhysRider | bmx/com/action/physics_types.h:265 | MEMORY | gPhysRiders = gPlayerRecs `uint8_t[2][0xf70]` (layout.h:7662) | read by byte offset: kOvlPlayer* 19 uses (overlay.c, overlymp.c, action.c); stride 0xf70 prop.c:606, ngifzactor.c:325; replay snapshot kSnapActor = 0xf70 (replay.c:274); prop.c:588 `+0x448`; game.c:2926/2930, rider.c:457 memsets with lit sizes |
| PhysContactSnap | physics_types.h:17 | MEMORY | physContactSnapshotFix physics.c:1095 | `data[500]`; contactSize returns lit 0x88 / 0x38 / 500 (physics.c:1068-1090) |
| PhysEdge, PhysFace, PhysGroundHit | physics_types.h:375/413/440 | MEMORY | zbspBuildFace / zcollide | sizes lit in contactSize; owner read at +0x10 / +0x48; `int32_t object` holds a pointer; game.c:1609, physics_air.c:45 `face + 0x16` |
| PhysPropFrame | bmx/com/action/physics_hit.h:38 | MEMORY | renderer scene frame | |
| PhysPropNode | physics_hit.h:62 | MEMORY | **same object as ZPropInst** (zprop.c builds it, ngifzbsp walks it) | frame 0x1c = ZPropInst.object, tag 0x24 = .extra (OBJI blob) |
| PhysPropObject | physics_hit.h:81 | MEMORY | zbspBuildCollObject; ngifzbsp.c:402 `zmemAlloc(500)` | PhysCollObject (0x1f0) over the same memory, `+0x1ec` read in physics.c:1423/1875/1880, physics_air.c:51, rider.c:2047 |
| PhysRayHit | physics_hit.h:122 | MEMORY | zcollideRay result | `int32_t object` holds a pointer |
| PropRecord, PropState | bmx/com/action/prop.h:52/69 | MEMORY | copied into replay part B: lit 0xb40 replay.c:224/360/386; prop.c:143 lit 0x300 | PropNodeState holds a pointer |
| CameraView | action/camera.h:105 | MEMORY | | |
| ContrlerPad | action/contrler.h:44 | MEMORY | gContrlerPads | action.c:296-309 `pad + 0xe` |
| FxBlock | action/fx.c:42 | MEMORY (.data parameter block) | gFxEmitBlock 0x554ab8, gFxCreateBlock 0x554ad8, adjacent | fx.c:148/189/284/364/450 `node + 0x30` |
| OvlElement, OvlScreen | action/overlay.h:106/107 | MEMORY (.data tables of function pointers) | | |
| OvlFont | overlay.h:116 | MEMORY | filled field by field by zoverParseFont (zoverii.c:3078) | also written as ZOverFont (two types) |
| ExtrasTitle | menu/frontend.h:578 | MEMORY (.data) | frontend_extras.c:144-159 | |
| GuiElement, GuiScreenState | menu/gui/gui.h:145/166 | MEMORY | on-disk records are already separate types (GuiScreenFileElement, guisfile.h:28, guisfile.c:75) | |
| GuiPad | gui/guiinput.h:35 | MEMORY | guiinput.c:18 `(uint8_t *)&gGuiPad + (index << 5)` | lit 32-byte stride (an original quirk) |
| GuiToggleData | gui/guitoggl.h:28 | MEMORY | | |
| ZRig (+ ZRigHit, ZRigConstraint, ZRigHinge, ZRigHingeDef) | bmx/com/zge/bmx/zoverii.h:153-185 | MEMORY | replay snapshot copies lit 0xe88 (replay.c:292, :307) | rider.c:2051 `motion + 0x88 + i*0xb4` (ZRigPoint has no pointers, so it survives) |
| ZfxEmitter | ngif/ngiffx.h:70 | MEMORY | zobjpemit.c:27 lit `zmemAlloc(0x30)`, pool lit 0x60 (:33) | |
| ZfxEffect | ngiffx.h:82 | MEMORY (.data handler table 0x55c278 + 0x38*n) | | uncertain: byte-stride indexing not checked |
| ZfxSkid | ngiffx.h:118 | MEMORY | gFxSkids | `texture[16]` uint32 handles hold pointers |
| ZbspLevelFace | ngif/ngifzbsp.h:41, zgemods/ztaginf.c:25 | MEMORY, built from a file record | ztaginfLoad ztaginf.c:205-257 copies TagFaceRecord (0x50, the on-disk type) field by field | fixups in 32 bits: `dataPointer` = `offset + (uint32_t)(uintptr_t)gZtaginfData` (ztaginf.c:198-201); pad50 holds 3 pointers in u32 (:249-251); lit kFaceStride 0x6c (:18, :172, :233) |
| ZbspTreeObject | ngifzbsp.h:94 | MEMORY | **same object as ZPropDef + 8** | def 0x20 = ZPropDef.extra; ngifzbsp.c:239-240 `obj + 0x84`, `holder + 0x48` |
| ZbspFaceHit, ZbspFaceQuery | ngifzbsp.h:197/198 | MEMORY | static buffers | `int32_t object` holds a pointer; ngifzbsp.c:769 `hit + 0x10 + i*0x10` |
| ZcollideProbe | ngif/ngifzcollide.h:66 | MEMORY | | points into itself; uncertain which type consumers read |
| ZmemdmmHandle | ngif/ngifzmemdmm.h:33 | MEMORY | ngifzmemdmm.c:17 `(uint32_t)top - handle*0x10`; :121-148 | lit stride, pointer truncated to u32 |
| ZresState | ngif/ngifzres.h:44 | MEMORY | gZres 0x5f8f60 | ngifzres.c:55/:100 read 8 bytes over two adjacent u32s (fine); levelInfo / riderRecord are byte records |
| ZresOverlaySlot | ngifzres.h:81 | MEMORY | | |
| ZActor | bmx/com/zge/zactor.h:69 | MEMORY | snapshot copies lit 0x278 (replay.c:290/304) | replay.c:284 `actor + 0x48`; ngifzactor.c:249 `object + 0x30` |
| ZMotionChannel, ZMotionPlayer | zactor.h:166/184 | MEMORY | snapshot copies lit 0xd60 (replay.c:291/305) | the motion file data has no pointers |
| ZPrimAsset | zprim.h:87 | MEMORY | pool zprim.c:56 lit 0x800 | |
| ZPrimState | zprim.h:68 | MEMORY | zprim.c:44 `zmemdmmAlloc(0x348)` = 10 x 0x54 | lit size |
| ZsndNote, ZsndSeqTrack, ZsndSeqEvent | zsndseq.h:104/105/124 | MEMORY | pools at 0x64eb38 / 0x64f048 | |
| ZsndSource | bmx/com/zge/zsndsrc.h:51 | MEMORY | | |
| WinloadFile | winload.h:139 | MEMORY | gWinloadFiles slots | |
| WinchanChannel, WinchanInitDesc | bmx/pc/zge/sound/winchan.h:82/241 | MEMORY | bmxsound.c:102 | |
| WinstrmSource, WinstrmNode, WinstrmStream | winstrm.h:114-116 | MEMORY | stream data read raw (winstrm.c:760/779) | |
| **MEMORY, engine (zge)** | | | | |
| ZassetLibrary | zge/zasset.h:34 | MEMORY | zasset.c:47; directory parsed with entryField (:24) | the handle is an `int` holding the pointer (zasset.c:18, :97) |
| ZfileDevice | zge/zfile.h:36 | MEMORY | device tables in data 0x563240 | `pad08[0x18]` = 6 function slots as bytes; zfileDeviceControl passes pointers as u32 (ngapp.c:508, zrwramdisk.c:106) |
| ZramdiskFile | zgemods/zramdisk.h:37 | MEMORY | pool lit 0x10 zramdisk.c:303 | address passed as `"rd00:%#10x"` text and `zfileOpenRamDisk(uint32_t)` (zramdisk.c:48/141, zscene.c:240/405/447, zrwworld.c:261, ngifzres.c:53/98) |
| Ziff | zge/ziff.h:37 | MEMORY | pool lit 0x108 ziff.c:31 | 8-byte read over chunkId+chunkSize (ziff.c:110, fine) |
| ZiffState | ziff.h:59 | MEMORY | data 0x6746d0 | callback pool lit 0x10 (ziff.c:30); ziff.c:422/424 `a + 0xc` / `a + 0x10`; ziff.c:390 lit `zmemAlloc(0x14)` |
| ZSkeletonNode | ziff.h:134 | MEMORY | ziff.c:461 | ZSkeletonOwner = ZObj view at +0x30 (ziff.c:498, zrwmat.c:150) |
| ZMathTables | zge/zmath.h:64 | MEMORY | data 0x65e2bc | pointers into itself |
| ZMovieStream, ZMovie | zmovie.h:90/91 | MEMORY | zmovie.c:306-330 | waveOut handle as u32 |
| ZObj | zge/zobj.h:64 | MEMORY | pools lit 0x3c / 0x10 zobj.c:151-152 | base of many views; `pad30` carries a pointer (skeleton / effect host / camera cache) |
| ZObjCamera, ZObjLight | zgemods/zobjcamera.h:119, zobjlight.h:33 | MEMORY | views of ZObj | |
| ZFont | zgemods/zfont.h:48 | MEMORY | ZFNT parsed with read32 (zfont.c:37) | |
| ZPropDef | zgemods/zprop.h:38 | MEMORY | PDIN read into local ZPropDefHeader (zprop.c:58-102) | same allocation as ZbspTreeNode / ZbspTreeObject |
| ZPropInst | zprop.h:39 | MEMORY | PRBL read into local ZPropInstHeader (zprop.c:104-195) | same allocation as PhysPropNode |
| ZtaginfNode | zgemods/ztaginf.h:33 | MEMORY | TAGN into local TagNodeRecord (ztaginf.c:150-188) | same object as ZrwRenderColNode (0x6728c4) but 0x34 vs 0x3c |
| ZProfileState, ZProfileEntry | zge/zprofile.h:54/70 | MEMORY | pool lit 0x24 zprofile.c:291 | |
| ZsceneNode, ZsceneEntry, ZsceneState | zge/zscene.h:33/53/65 | MEMORY | Scene.Txt text; pools lit 0x78 / 0x10 zscene.c:485-486, lit `zmemAlloc(0xc)` :415/:459 | queries return pointers in `int32_t query[]` (zscene.c:172/176/193/540/544); zscene.c:795 `((uint32_t *)library)[3]` as a pointer |
| ZtokenFrame | zge/ztoken.h:28 | MEMORY | | |
| ZpadDevice | zge/pc/pcinput.h:45 | MEMORY | kZpadDeviceBytes 0x74 (zinput.c:20) | pcinput.c:15-16 stores ZpadDiRecord* as `device[0x70/4] = (uint32_t)`; pool lit 0x14 pcinput.c:251; pcinput.c:136 `state + 0x30`, :198 lit 0x58 |
| ZtimerHeap, ZtimerNode | zge/pc/pcprofile.h:28/47 | MEMORY | pcprofile.c:59 | |
| ZrwMarker | zglrw/zrwcam.h:20 | MEMORY | SDPT read into local ScnptsChunk (zscnpts.c:46-90) | pool lit 0x50 (zscene.c:879/906) |
| ZrwCamera | zrwcam.h:73 | MEMORY | pool lit 200 zrwcam.c:69; memset lit 200 zrwcam.c:190 | |
| ZrwColTriangle | zglrw/zrwcol.h:33 | MEMORY | second type for RpCollisionTriangle | |
| ZrwColState | zrwcol.h:75 | MEMORY | data 0x674160 | bestUser / bestSub hold pointers in u32 (zrwcol.c:58, 268-284); `zmemAlloc(0x13000)` hits (zrwcol.c:34) |
| ZrwDbgScratch | zglrw/zrwdbg.c:57 | MEMORY | .bss 0x650180 | |
| ZrwFileOps | zglrw/zrwfile.h:43 | MEMORY | zrwfile.c:311 memcpy from RwFileFunctions (another type) | fine while both are arrays of pointers |
| ZrwLight | zglrw/zrwlight.h:21 | MEMORY | | |
| ZrwMatSlot, ZrwMaterial, ZrwMatState | zglrw/zrwmat.h:42/45/86 | MEMORY | MADF read raw into gZrwMadfRec (no pointers, zrwmat.c:199) | ZrwMaterial: `slot[5]` overlaps `rwMaterial` at 0x70 (32-bit-only union); zrwtxr.c:630 clump plugin lit 4 |
| ZrwModelPart, ZrwModelLod, ZrwModelDef, ZrwModel | zglrw/zrwmodel.h:134-154 | MEMORY | MLOD / MDEF into local headers (zrwmodelcreate.c:713, :753-764) | zrwmodel.c:519 plugin lit 4 |
| ZrwModelCreateState | zglrw/zrwmodelcreate.h:39 | MEMORY | data 0x675218 | |
| ZrwRenderColNode, ZrwRenderColQuery, ZrwModelRenderState | zglrw/zrwmodelrender.h:60/89/111 | MEMORY | | ColNode = ZtaginfNode (two declarations) |
| ZrwPrimState | zglrw/zrwprim.h:50 | MEMORY | | lit `zmemAlloc(0x90f / 0x80f)` (zrwprim.c:315/317) |
| ZrwShadow, ZrwShadowQuad | zglrw/zrwshadow.h:25/53 | MEMORY | views of ZrollerShadow / ZrwPrimQuad | `uint32_t texture` (pointer? uncertain) |
| ZrwTexture, ZrwTextureState | zglrw/zrwtxr.h:32/51 | MEMORY | pool lit 0x28 zrwtxr.c:433 | |
| ZrwEffectItem, ZrwEffectHost, ZrwParticleSystem | zglrw/zrwworld.h:67/84/157 | MEMORY | zrwworld.c:105 lit `zmemAlloc(0x3c)` | EffectItem is a view of the fx record; host reached through ZObj.pad30 |
| **MEMORY, RenderWare** | | | | |
| RwMatrixGlobals | rw/engine.c:18 | MEMORY | engine plugin, lit 0x10 (plugins.c:194) | |
| RwGlobals | rw/rwcore.h:192, plugins.c:1110 | MEMORY | RwEngineInstance | `stdFunc[]` is a u32 array of function pointers (camera.c:449/461/668); light.c:206/252 `RwEngineInstance + 0x0c`; native/crt.c:915/920 `engine + 0xc4 / 0xc8` |
| RwFreeList | rwcore.h:231 | MEMORY | core.c:13 | free stack assumes 4-byte pointers (core.c:30, :32, :108, :130) |
| RwFrame, RwObjectHasFrame | rwcore.h:285/290 | MEMORY (frame list read field by field, math.c:818) | | frames array `count << 2` (math.c:836); plugins.c:803 `parent + 0x1c`; world.c:2001 `object + 0xc` |
| RwPluginRegEntry, RwPluginRegistry | rwcore.h:346/347 | MEMORY | free list lit 0x34 (core.c:241) | registration sizes are literals (below) |
| RwResEntry, RwResourcesGlobals | rwcore.h:441/442 | MEMORY | plugin lit 0x24 (plugins.c:202) | |
| RwD3DIm3DWork | rw/rwd3d.h:124 | MEMORY | | d3dpipe.c:183-194 `+0x70`, vertices at `+0x88`; :268-281 `+0x30`, `0x48` |
| RwD3DIm3DLevel, RwD3DStateCache | rwd3d.h:146/273 | MEMORY | | not handed to D3D (our driver's cache) |
| RwD3DSavedState | rwd3d.h:325 | MEMORY | deferred alpha records d3dim2d.c | record layout lit `+0x44/0x48/0x4c/0x50` (d3dim2d.c:340-371, 570-574, 638-644), sizes 0x84 / 0xa4 / `n*0x22+0x50`; a `void *` stored in a 4-byte callback word (d3dim2d.c:63-95) |
| RwIm3DPipeline, RwIm3DGlobals | rw/rwim.h:30/47 | MEMORY | free list lit 0x1c (im.c:153); plugin lit 0x38 | d3dim3d.c:578-581 fill it as a u32 array of function pointers; world.c:1727 `pipeline + 0x10` |
| RpLight, RpLightVisit | rw/rwlight.h:25/45 | MEMORY | | |
| RpMorphTarget, RpGeometry, RpMesh, RpBuildMeshTriangle | rwplugins.h:32, rwclump.h:90/62/216 | MEMORY (read field by field, clump.c:1179-1277) | | clump.c:1262 adjacency test via `(uint32_t)(uintptr_t)`; world.c:2708 mesh block lit `0x10`; clump.c:1921-1924 lit 0xc; d3dpipe.c:59/919 `header + 0x10 + firstMeshOffset` |
| RpAtomic, RpClump | rwplugins.h:75/100 | MEMORY (read field by field) | | world.c:114/119/125 `clump + 0x14`, `+0x12`, `atomic + 0x68` |
| RpSkinBone, RpSkinGeometry | rwplugins.h:144/145 | MEMORY | geometry plugin slot lit 4 (plugins.c:344) | |
| RwStream | rw/rwstream.h:60 | MEMORY | free list lit 0x20 (stream.c:471) | |
| RwRaster, RwRasterGlobals | rw/rwtexture.h:62/74 | MEMORY / driver interface (ours) | plugin lit 0x68 | d3dstate.c:80/100 |
| RwTexture, RwTexDictionary, RwTextureGlobals | rwtexture.h:93/106/127 | MEMORY (texture read field by field, clump.c:1781) | plugin lit 0x2c | |
| RwImage, RwImageFormat, RwImageGlobals | rwtexture.h:143/158/179 | MEMORY (BMP / PNG parsed into locals) | free list lit 0x34 (texture.c:1576), plugin lit 0x220 | texture.c:1777-1782 addresses rows and the palette in u32 (truncates) |
| RpWorldSector, RpWorld, RpMaterial | rw/rwworld.h:84/154/123 | MEMORY (read field by field, clump.c:1420, :2604-2717, :2901) | | **RpPlaneSector block sized `header[9]*0x18`** (clump.c:2921) and taken with `worldCursorTake(cursor, 0x18)`, written as `((int32_t *)node)[4/5]` (clump.c:2751-2763), but it holds two child pointers |
| RpTie, RpLightTie | rwworld.h:110/111 | MEMORY | free lists lit 0x18 (world.c:1673/1677) | |
| RpWorldCameraData, RpIntersection | rwworld.h:178/247 | MEMORY | plugin lit 0x1c (plugins.c:241) | |
| RpPVSWorldData | rwworld.h:286 | MEMORY (5 words decoded, world.c:1420) | lit `rwmalloc(0x58)` (world.c:1346), plugin slot lit 4 | |
| RpAnimFrameSequence, RpAnimNode, RpAnimClumpSequence | rw/rwanim.h:37/103/104 | MEMORY (decoded, plugins.c:1856) | | plugins.c:1100-1122 read the sequence as `const uint32_t *` with `[1]` as the name pointer; plugins.c:1045 `words[0xb/0xc]`, :1273 `interpolator + 0x2c`; lit 0x10 / 8 / 0xc / 0x14 |
| RpLodAtomicData (accesses) | | (FILE above) | | plugins.c:2555/2777 `lod + 0x18 + 4*level` |
| RpCollisionTriangle, RpCollisionQuery | rw/rwcollision.h:25/53 | MEMORY | stack locals | |

Counts: FILE 7, EXTERNAL 8 (all ours on both sides), MEMORY 152. Plus 1 MEMORY struct built from a file record with 32-bit fixups (ZbspLevelFace).

## FILE: detail

### .zsd sound banks (SFXCOM / MUSIC / SPEECH, bin\audio)

- **Loader:** winloadLoad reads the whole file into a zmem block. winloadRegister (winload.c:71-150) checks `"WIN "` at +4 (:95) and then relocates in place:
  - the header's 6 table pointers (`voices`, `samples`, `trackDefs`, `sampleFiles`, `phrases`, `keymaps`, :98-103);
  - each entry of `phrases[]` and `keymaps[]`, which are tables of u32 offsets (:115, :130);
  - the phrase entries' and trackDefs' `data`, with `+= (uint32_t)(uintptr_t)image` (:119, :124). These pass the asserts but truncate on 64-bit;
  - `ZsdSample.data`, set to the resident sample's address (:145).
- **ZsndSeqHeader** is the same record as ZsdTrackDef (bmxsound.c:355 casts winloadGetTrackDef to it).
- **On-disk types:** `ZsdFileHeader` / `ZsdFileSample` / `ZsdFileTrackDef` with `uint32_t` offsets, asserted on every build.
- **Reader, preferred:** keep the image as bytes and build a native `ZsdBank` beside it. The bank holds `ZsdVoice *voices`, `const ZsdPhrase **phrases`, `const uint8_t **trackData` and so on, plus a per-sample `data` array. The winloadGet* functions are already the only accessors.
- **Reader, alternative:** keep the offsets and resolve them on each access.

### Replay header (Replay.ZAL demos)

- action.c:813-828 creates a recorder (flags 7: no parts; size 0x2800) and reads the demo asset over the header from `padIndex` (+0x0c) onwards, followed by the frame stream (replay.c:449-466).
- **File layout:** padIndex, field10, partA..partCEnd (6 x u32), headerSize, size, `cursor` (a stale 32-bit pointer), buttons, sticks[2] (0x34 bytes), then the frames.
- `cursor` is reset by replaySetMode (replay.c:374, :413), so the value from the file is ignored.
- The size check at action.c:825 implies the shipped demos carry no parts A or B. If they did, those parts would be raw PhysRider / ZActor / ZMotionPlayer / ZRig / PropState memory (see the MEMORY hazards).
- **On-disk type:** `ReplayFileHeader` (0x34 bytes, `uint32_t cursor32`), with a reader that copies it into the native Replay.
- **Writer:** the dev writer overlay.c:2307-2314 (`s:\bmx\...\demoNNNN.rpy`) gets the matching writer.

### ZPrimFont (zprim .ZAL font asset, gGuiAssetFile)

- The asset is read into a DMM block (zprim.c:1441) and used in place.
- Only `pages` (0x0c) is a runtime pointer, written into the file image (zprim.c:1468, :1907-1917). The glyph records at 0x18 have no pointers.
- **Fix:** move `pages` into a side table keyed by the ZPrimAsset, or make it a u32 index into gZprimFontPageList, and assert the file layout as a `ZPrimFontFile`.
- **Related (X2e):** ZPrimFrame.texture and the page entries are int32 "handles" that hold pointers from gRwTextureLoad (zprim.c:808), stored in slots inside the file image.

### RpSkinBoneBlock (RW bone plugin, chunk 0x10e)

- rpBoneGeometryRead (plugins.c:907-925) reads 12 bytes per block, {firstVertex, numVertices, key}, straight into the struct with `RwStreamReadInt32`, then sets `bone = NULL`. The bone is resolved from `key` later.
- The allocation is `length/3*4`, which assumes 16-byte blocks.
- **On-disk type:** `{int32 first, count, key}`, with a loop that fills `n * sizeof(RpSkinBoneBlock)`.

### RpLodAtomicData (RW LOD atomic plugin, chunk 0x112)

- **Reader:** LODAtomicCallBackReadChunk (plugins.c:2735-2783) reads the whole 0x40-byte struct chunk raw, then calls `RwMemNative32(lod, 0x40)`.
- `levels[i]` from the file is only tested for non-NULL ("a geometry chunk follows") and is then replaced by the geometry that is read. `select` and `hooked` are overwritten. `savedRender` keeps the stale file value until rpLodAtomicHook runs; whether it is always overwritten is uncertain.
- **Writer** (plugins.c:2786-2804): it byte-swaps a copy but writes the live struct, pointers included.
- **On-disk type:** `{int32 hooked; uint32 savedRender, select; float range, scale; int32 current; uint32 levels[10]}`, with an explicit reader and writer. The `lod + 0x18 + 4*i` accesses (plugins.c:2555, :2777) become `levels[i]`.
- Uncertain: whether any of the game's DFFs contain this chunk at all. A count while loading would tell.

### SAVE.DAT: not a FILE struct

- `.\save\SAVE.DAT`: saveFileRead (memcard.c:50) and saveFileWrite (:90).
- It is 0x235c bytes (memcard.h:28), and only the size word (+4) is checked. memcard.c:72-74 and :101-103 copy three blocks:

| File offset | Size | Source | Content |
|---|---|---|---|
| 0x0 | 4 | | magic |
| 0x4 | 4 | | size |
| 0x8 | 0xc3c | 0x6955e0 | gOptionFlags, gPhysSyncFlags, gSoundVolumes, gRiderData (`uint8_t[12][0x104]`) and padding, up to gActionLanguage (layout.h:7683-7690) |
| 0xc44 | 0x1680 | 0x551fd8 | the first 0x5a0 dwords of gComdataRecords: records of u32s and names; no pointers in the range (no `p_` members, checked in layout.h) |
| 0x22c4 | 0x98 | 0x6952a0 | gProfiles = OvlProfile[2] (0x4c each, char name) |

- The 90 KB in PORTING.md is the declared view `ComdataRecordTable[0x1200]` (comdata.h:15). The save copies only 0x1680 bytes of it.
- **Nothing in SAVE.DAT changes size on 64-bit.** The work is X2a-style: the 0xc3c memcpy spans several globals and their padding.
- **Proposal:** an explicit `SaveFile` type (magic, size, an options struct of the named globals at their file offsets, `records[0x5a0]`, `profiles[2]`) with a reader and writer that copy each named global. Then the globals can leave their blocks, and old saves load because the file layout stays the same.

## EXTERNAL: detail

Every EXTERNAL struct has our own code on both sides, so none is a real fixed ABI. Each needs only:

- **The DirectInput vtables** (ZpadDiVtbl, ZpadDirectInputVtbl): `uint32_t` placeholder slots become `void *`. The game side is pcinput.h; the stand-in side is platform/diface.h and input.c. The only real-DLL user, BMX_INPUT_CHECK (platform/inputcheck.c), stays 32-bit.
- **WAVEHDR** (ZMovieWaveHeader and platform/waveout.c's WaveHeader): the literal 0x20 header size (zmovie.c:93, :259; waveout.c kHeaderSize, :241/255/274) becomes `sizeof`, and `WaveOut.window` becomes a pointer.
- **DSBUFFERDESC** (DsBufferDesc): already prepared (dsface.h:75 guard; dsound.c:1078 accepts sizeof).
- **DrvRasterExt:** the registration size 0x20 (driver/device.c:194) becomes `sizeof`, and rw/d3dstate.c:80 / :100 (`raster + ext + 0xc`, `ext + 0x10`) become field accesses.
- **RwDevice and RwFileFunctions:** our driver and CRT fill them; there is nothing to do beyond the RwGlobals hazards below.

## MEMORY hazards

These are what actually break on 64-bit once the asserts are 32-bit only. Grouped by kind.

### 1. A real overflow

- **zrwmodelcreate.c:178:** `zmemAlloc(0x4010)` is the scratch for zassetDecompressHashed. That function puts a table of 0x1000 pointers in it (zrwmodelcreate.c:87, :103), which is 0x8000 bytes on 64-bit.
- This decoder is `zassetDecompress(1, ...)`, used by zassetRead for libraries with compression method 0 (zasset.c:258). Every such asset would overwrite the heap.

### 2. Allocation sizes as literals (about 70)

- **zge free lists:**
  - ziff.c:30/31 (0x10, 0x108)
  - zramdisk.c:303 (0x10)
  - zrwcam.c:69 (200)
  - zrwtxr.c:433 (0x28)
  - zrwzgl.c:420 (0x14)
  - pcinput.c:251 (0x14)
  - zinput.c:20 (0x74)
  - zobj.c:151/152 (0x3c, 0x10)
  - zprofile.c:291 (0x24)
  - zscene.c:485/486 (0x10, 0x78)
  - zscene.c:879/906 (0x50)
  - zobjpemit.c:33 (0x60)
  - ngapp.c:572 (0x80)
- **zge single allocations:**
  - zobjpemit.c:27/53/78 (0x30, 0x10)
  - zrwworld.c:105 (0x3c)
  - zscene.c:415/459 (0xc)
  - ziff.c:390 (0x14)
  - zprim.c:44 (0x348), :56 (0x800), :1165 (0x328)
  - ngifzbsp.c:402 (500, PhysPropObject)
- **RW plugin registration sizes:**
  - engine plugins, plugins.c:193-203 (0x220, 0x68, 0x2c, 0x38, 0x24, 0x10)
  - world module, plugins.c:269-273 (4, 8, 0xc, 0x10)
  - object plugins: camera 0x1c, atomic 8, clump 4, light 8, PVS 4+4, anim 4 / 0x14 / 0xc, bone 4+4, LOD 4
  - zrwmodel.c:519 (4) and zrwtxr.c:630 (4)
  - driver/device.c:194 (0x20)
  - Plugin offsets themselves are computed at run time from these sizes, so `sizeof` at registration fixes them.
- **RW free lists:**
  - core.c:241 (0x34)
  - im.c:153 (0x1c)
  - plugins.c:1024 (8), :2677 (0x40)
  - stream.c:471 (0x20)
  - texture.c:1576 (0x34)
  - world.c:1673/1677 (0x18), :1718 (0x10)
  - engine.c:606 (0x40, RwMatrix, no pointers)
- **RW fixed mallocs:**
  - world.c:1346 (0x58)
  - plugins.c:1081 (0x10, used as u32 words)
  - world.c:2708 (mesh block `0x10 + n*0x10`)
  - clump.c:1921-1924 (0xc)
  - clump.c:2921 (`header[9]*0x18` for RpPlaneSector)
- **RwFreeList itself** (core.c:30/32/108/130) assumes 4-byte pointers in its free stack.
- **contactSize** (physics.c:1068-1090) returns 0x88 / 0x38 / 500, and PhysContactSnap.data[500] holds a copy of the contact.
- **The replay snapshots** (replay.c:274-307): 0x278 ZActor, 0xd60 actor block, 0xe88 ZRig, kSnapActor = 0xf70 (PhysRider), totals 0x2de8 / 0x5bd0. Also PropState 0xb40 (replay.c:224/360/386).
- **memsets by literal size:**
  - zrwcam.c:190 (200)
  - zrwworld.c:399 (0xac)
  - zrwzgl.c:402/573/574 (0x10, 0x88, 0xa8)
  - zmem.c:30 (0x24)
  - pcinput.c:198 (0x58)
  - native/main.c:109 (0x34 over a range of globals)
  - prop.c:143 (0x300)

### 3. Several types for the same memory

Each pair has to become one type, or the two have to be reconciled, before the layouts change. Otherwise they drift apart silently.

- ZPropDef = ZbspTreeNode (ZbspTreeObject = ZPropDef + 8)
- ZPropInst = PhysPropNode
- PhysPropObject = PhysCollObject (`+0x1ec` propId)
- ZtaginfNode = ZrwRenderColNode (0x6728c4; 0x34 vs 0x3c)
- ZObj = ZObjCamera / ZObjLight / ZSkeletonOwner / the effect host at pad30
- ZrwColTriangle = RpCollisionTriangle
- ZrwShadowQuad = ZrwPrimQuad
- ZrwShadow = ZrollerShadow
- ZrwEffectItem = the fx record at +0x2c
- OvlFont = ZOverFont
- PhysRider = gPlayerRecs `uint8_t[2][0xf70]` (kOvlPlayer* byte offsets, 19 uses)
- PropState = gReplayTable = gPropRecords
- RwFileFunctions -> ZrwFileOps (memcpy, zrwfile.c:311)
- ZrwMaterial's `slot[5]` overlapping `rwMaterial` at 0x70

### 4. Pointers in 32-bit slots inside structs

These are for X2e, but they sit inside the structs above:

- RwGlobals.stdFunc
- RwIm3DPipeline (d3dim3d.c:578-581)
- the d3dim2d deferred callback word
- ZbspLevelFace.pad50 and ztaginf `dataPointer`
- `int32_t object` in PhysEdge / PhysFace / PhysRayHit / ZbspFaceHit
- ZfxSkid.texture
- ZrwColState.bestUser / bestSub
- ZpadDevice[0x70/4]
- the zscene query arrays
- ZmemdmmHandle
- RpPlaneSector children written as int32 (clump.c:2757-2763)
- texture.c:1777-1782 (palette address in u32)
- the asset library handle as `int`
- RAM disk addresses as u32 and in `rd00:%#10x` names

## Byte-offset accesses in the ported code

Found with three scans over src/ (src/data, src/hook and native/loader.c excluded):

- a cast to a byte pointer plus a constant;
- a cast plus a hex offset on the same line;
- a cast-then-index such as `((uint32_t *)x)[n]`.

That gives 88 + 65 + 90 hits, about 190 distinct lines. A pointer variable that is already `uint8_t *` and used without a cast on the line is not caught, so the true count is higher (uncertain). They split into two groups.

### Into structs that hold pointers (a hazard: the offset moves on 64-bit), by file

| File | Count | Lines and what they reach |
|---|---|---|
| rw/d3dim2d.c | 14 | deferred record `+0x44/0x48/0x4c/0x50` (:340, :348, :368-371, :570-574, :638-644) |
| bmx/com/action/game.c | 10 | `prop + 8` (:698), `contact + 2` (:776), `marker + 0x1f0` -> `+0x1c` (:1336), `marker + 4` (:1343), `marker + 0x2c` (:1479/1482), `c + 0x93` (:1591), `contact + 0x16` (:1609), `object + 2` (:1615), `gGameRiders[..] + 0xf7` (:1970), `c + 0x9d` (:1992), PhysRider memsets (:2926, :2930) |
| bmx/com/action/physics.c | 8 | `near + 0x3c` (:299), `prop / contact + 0x1ec` (:1423-1424, :1875, :1880), `obj + 0xe70` / `+0x88` (:2133-2141) |
| bmx/com/action/fx.c | 6 | `node + 0x30` (:148, :189, :284, :364, :450), `obj + 0x10` (:479) |
| rw/world.c | 6 + 6 | `clump + 0x14 / 0x12`, `atomic + 0x68` (:114/119/125), `pipeline + 0x10` (:1727), `object + 0xc`, `sector + 2` (:2001-2002); `RwEngineInstance + rpTieOffset` as `RwFreeList *[2]` (:1677-1680, :1844-1849, run-time offset, fine); `light + offset` (:2340) |
| rw/d3dpipe.c | 6 | work `+0x88` / `+0x48` (:190-194, :276-281), mesh header `+0x10` (:59, :919) |
| rw/plugins.c | 5 + 4 | `parent + 0x1c` (:803), `interpolator + 0x2c` (:1273), `lod + 0x18 + 4*i` (:2555, :2777), `png + 0x54` (:2851, pinned by png.c:105); `RwMatrix.pad1` byte (:959-1009, no pointers, fine) |
| bmx/action/rider.c | 4 | `hit[0x10] + 0x1ec`, `motion + 0x88 + i*0xb4`, `node + 0x1c` (:2047-2054) |
| bmx/zge/zactor.c | 3 | motion data `p + 0xc/0xe/0x10` (:1182-1185, file data without pointers, fine), `hdr + 2` (:486) |
| bmx/com/action/action.c | 3 | `pad + 0xe` (:296-309, ContrlerPad) |
| bmx/zge/ngif/ngifzbsp.c | 3 | `obj + 0x84`, `holder + 0x48` (:239-240), `hit + 0x10 + i*0x10` (:769) |
| bmx/action/overlay.c | 4 | `target + 7`, `rider + 8`, `record + 7` (:1588, :1692, :1708, OvlProfile names, no pointers before them), `value + 10` (:3404) |
| bmx/action/replay.c | 1 + 3 | `actor + 0x48` (:284); `snap + kSnapBikePos` (:293-295) |
| rw/light.c | 2 | `RwEngineInstance + 0x0c` (:206, :252) |
| rw/d3dstate.c | 2 | `raster + ext + 0xc`, `ext + 0x10` (:80, :100) |
| native/crt.c | 2 | `engine + 0xc4 / 0xc8` (:915, :920, RwGlobals file functions) |
| native/hfguard.c, hfinterp.c, zglrw/zrwzgl.c | 2 + 3 + 2 | `camera[8] & 4` (RwCamera / object flags byte; offset 8 sits after one pointer, so it moves); hfinterp.c:864 `zrw + 0x0c` |
| bmx/action/physics_air.c | 2 | `face + 0x16`, `object + 0x1ec` (:45, :51) |
| bmx/zge/ngif/ngiffx.c, ngifzactor.c | 1 + 1 | `object + 0x30` (ngiffx.c:204, ngifzactor.c:249) |
| bmx/action/prop.c | 1 | `gPhysRiders[p] + 0x448` (:588) |
| driver/texture.c | 1 | `raster + 0x2c` (:18, DrvRasterSaved) |
| zge/ziff.c | 2 | `a + 0xc`, `a + 0x10` (:422, :424, anim record chains) |
| zge/pc/pcinput.c | 2 | `state + 0x30` (:136, DIJOYSTATE, no pointers), `instance + 4` (:271, DIDEVICEINSTANCE GUID, no pointers) |
| zge/zscene.c | 2 | `record + 8` (:234), `((uint32_t *)library)[3]` as a pointer (:795) |
| bmx/menu/frontend_select.c | 2 | `&gRiders[shown] + 4 + offset` (:1479, :1489) |
| bmx/menu/gui/guiinput.c | 1 | `&gGuiPad + (index << 5)` (:18) |
| bmx/zge/ngif/ngifzmemdmm.c | ~4 | `top - handle*0x10` (:17, :121-148) |
| zge/zgemods/ztaginf.c | 3 | kFaceStride 0x6c (:172, :233, :249) |

### Into byte records or file data (no pointers before the offset; safe)

| File | Count | What |
|---|---|---|
| rw/clump.c | 40 | stream headers as `int32_t header[]` / `float raw[]` (:1208-1260, :1401-1460, :2479-2623, :2757-2763 except the node write above, :2865-2964) |
| bmx/action/game.c (rdS16 / rdFloat) | 32 | marker records (:1250-1257 and others) |
| bmx/zge/ngif/ngifpsx.c (rd16 / rd32) | 68 | PSX primitive packets (:371-461); `p + 0x9c` (:893, :1169) |
| bmx/action/fx.c (rdS16 / rdFloat) | 8 | emitter parameter records |
| rw/collision.c (rwAxis) | 16 | vector component by offset |
| bmx/zge/zactor.c | 9 | motion keys as int16 (:1201-1279) |
| bmx/zge/zprim.c | 5 + 2 | RLE / palette decode (:1106, :1259, :1392-1405) |
| rw/math.c, rw/engine.c | 9 | `RwMatrix.pad1` byte |
| zglrw/zrwdbg.c | 4 | text queue records, vector words |
| others | ~10 | zanim.c:27 (name records), winload.c:95 (the "WIN " tag), zmovie.c:207 and driver/movie.c:319 (data after the header), zmem.c:408 (block header), core.c:75, ngifzroller.c:193, rider.c:447 (roller record byte 7), zrwworld.c:261 (the file size word) |

## Proposed order for X2c (the on-disk types), by format

1. **zassetDecompressHashed scratch** (zrwmodelcreate.c:178): `0x10 + 0x1000 * sizeof(void *)`. It is not an on-disk type but a one-line fix, and without it no compressed asset loads on 64-bit.
2. **SAVE.DAT:** the explicit `SaveFile` type with a reader and writer (above). It has no pointers, so it is safe and testable now: an old save must load and write back byte-identical. It also unblocks X2a for the 0xc3c and 0x1680 views.
3. **.zsd sound banks:** `ZsdFile*` types with u32 offsets plus the native `ZsdBank` side table. This is the only real offset-to-pointer image, and it covers ZsdHeader, ZsdSample and ZsndSeqHeader (bmxsound.c:355, winload.c:71-150).
4. **Replay.ZAL demos:** `ReplayFileHeader` (0x34 bytes) with a reader and the dev writer. Before this, check that the shipped demos carry no parts A, B or C, e.g. a one-off count in the loader (uncertain).
5. **zprim fonts:** take `ZPrimFont.pages` out of the file image, and the texture handles in ZPrimFrame / font pages with it (X2e).
6. **RW plugin chunks:** RpLodAtomicData (reader and writer; first find out whether any DFF has chunk 0x112) and RpSkinBoneBlock (a 12-byte record loop).
7. **Level tag data (TAGF):** already read through TagFaceRecord. Only the 32-bit `dataPointer` / pad50 fixups and kFaceStride change (ztaginf.c:172-257).

In parallel with X2c, and needed before the X3 build runs:

- the literal sizes (section 2) as `sizeof`, mechanical;
- the byte offsets into pointer-bearing structs (the first table) as fields;
- the view pairs (section 3) merged.

Each can be done and checked with xcheck in the 32-bit build, where the values do not change.
