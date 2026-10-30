/*
 * Tests for rw/d3dpipe.c, the D3D7 driver's instancing pipelines of atomics and world sectors: instancing
 * (resource entries, morph targets, triangles), the colour stages, the five light routines and the render
 * stage. The driver's state lives in the exe's data (0x696900..); the engine is dt_rw.h's fixture; resources,
 * frames, matrix and vector maths and the driver's function pointers are logging stubs.
 *
 * Float code runs with the x87 control word of a camera update (24-bit precision, rounding down), as in
 * test_rwd3dim3d.c: the wrappers below set it around both sides.
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwclump.h"
#include "rw/rwd3d.h"
#include "rw/rwlight.h"
#include "rw/rwworld.h"

enum { kVerts = 40, kSlack = 256 };

static uint16_t cameraCw(void)
{
    uint16_t old, cw;
    __asm__ volatile("fnstcw %0" : "=m"(old));
    cw = (uint16_t)((old & 0xf0ff) | 0x0400);
    __asm__ volatile("fldcw %0" ::"m"(cw));
    return old;
}

static void restoreCw(uint16_t cw)
{
    __asm__ volatile("fldcw %0" ::"m"(cw));
}

static int gAllocFail;  /* the resource allocation fails */
static uint32_t gFlags; /* what the geometry / world flag getters return */
static RwMatrix *gLtm;  /* what RwFrameGetLTM returns */
static int gCullResult; /* bit n: the n-th cull call culls */
static int gCullCalls;

/* ---- stubs ---- */

static RwResEntry *BMX_CDECL stAllocEntry(void *owner, RwResEntry **ownerRef, int32_t size, void *notify)
{
    RwResEntry *entry;
    rwtLog(0xa1);
    rwtLogPtr(owner);
    rwtLogPtr((const void *)ownerRef);
    rwtLog((uint32_t)size);
    rwtLogPtr(notify);
    if (gAllocFail) {
        return NULL;
    }
    entry = dt_alloc((uint32_t)size + kSlack);
    entry->size = size;
    entry->owner = owner;
    entry->ownerRef = ownerRef;
    *ownerRef = entry;
    return entry;
}

static int BMX_CDECL stFreeEntry(RwResEntry *entry)
{
    rwtLog(0xa2);
    rwtLogPtr(entry);
    if (entry->ownerRef != NULL) {
        *entry->ownerRef = NULL;
    }
    return 1;
}

static RwResEntry *BMX_CDECL stUseEntry(RwResEntry *entry)
{
    rwtLog(0xa3);
    rwtLogPtr(entry);
    return entry;
}

static RwMatrix *BMX_CDECL stGetLTM(RwFrame *frame)
{
    rwtLog(0xa4);
    rwtLogPtr(frame);
    return gLtm;
}

static RwMatrix *BMX_CDECL stMultiply(RwMatrix *out, const RwMatrix *a, const RwMatrix *b)
{
    rwtLog(0xa5);
    rwtLogPtr(out);
    rwtLogPtr(a);
    rwtLogPtr(b);
    for (int i = 0; i < 16; i++) {
        ((uint32_t *)out)[i] = dt_u32();
    }
    return out;
}

static uint32_t BMX_CDECL stGeometryFlags(const RpGeometry *geometry)
{
    rwtLog(0xa6);
    rwtLogPtr(geometry);
    return gFlags;
}

static uint32_t BMX_CDECL stWorldFlags(const RpWorld *world)
{
    rwtLog(0xa7);
    rwtLogPtr(world);
    return gFlags & 0xff;
}

static __attribute__((regparm(1))) void stMorph(RpAtomic *atomic)
{
    rwtLog(0xa8);
    rwtLogPtr(atomic);
}

static void BMX_CDECL stSetUV(uint32_t flags)
{
    rwtLog(0xa9);
    rwtLog(flags);
}

static void BMX_CDECL stGrowBuffers(uint32_t extra)
{
    rwtLog(0xaa);
    rwtLog(extra);
}

/* Logs a vector by value (the routines pass vectors on their stack). */
static void logV3d(const RwV3d *v)
{
    rwtLogFloatBits(v->x);
    rwtLogFloatBits(v->y);
    rwtLogFloatBits(v->z);
}

/* The vector stubs permute and negate the components: exact, so both sides get the same bits. */
static RwV3d *BMX_CDECL stTransformPoints(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *m)
{
    const RwV3d v = *in;
    rwtLog(0xab);
    logV3d(in);
    rwtLog((uint32_t)count);
    rwtLogPtr(m);
    out->x = v.y;
    out->y = -v.z;
    out->z = v.x;
    return out;
}

static RwV3d *BMX_CDECL stTransformVectors(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *m)
{
    const RwV3d v = *in;
    rwtLog(0xac);
    logV3d(in);
    rwtLog((uint32_t)count);
    rwtLogPtr(m);
    out->x = -v.z;
    out->y = v.x;
    out->z = v.y;
    return out;
}

static double BMX_CDECL stNormalize(RwV3d *out, const RwV3d *in)
{
    const RwV3d v = *in;
    rwtLog(0xad);
    logV3d(in);
    out->x = v.z;
    out->y = v.y;
    out->z = -v.x;
    return 1.0;
}

static double BMX_CDECL stInvSqrt(int32_t bits)
{
    const uint32_t r = 0x5f3759dfu - ((uint32_t)bits >> 1);
    float f;
    rwtLog(0xae);
    rwtLog((uint32_t)bits);
    __builtin_memcpy(&f, &r, 4);
    return f;
}

static double BMX_CDECL stSqrt(int32_t bits)
{
    const uint32_t r = ((uint32_t)bits >> 1) + 0x1fc00000u;
    float f;
    rwtLog(0xaf);
    rwtLog((uint32_t)bits);
    __builtin_memcpy(&f, &r, 4);
    return f;
}

static void BMX_CDECL stLight(RwD3DIm3DWork *work, RpLight *light, const RwMatrix *inverse, float scale,
                              const RwSurfaceProperties *surface)
{
    rwtLog(0xb0);
    rwtLogPtr(work);
    rwtLogPtr(light);
    rwtLogPtr(inverse);
    rwtLogFloatBits(scale);
    rwtLogPtr(surface);
}

static int BMX_CDECL stStateSet(int32_t state, int32_t value)
{
    rwtLog(0xb1);
    rwtLog((uint32_t)state);
    rwtLog((uint32_t)value);
    return 1;
}

static int BMX_CDECL stBitsAlloc(int32_t count)
{
    rwtLog(0xb2);
    rwtLog((uint32_t)count);
    return 1;
}

static int BMX_CDECL stListResize(RwD3DIndexList *list, uint32_t capacity)
{
    rwtLog(0xb3);
    rwtLogPtr(list);
    rwtLog(capacity);
    return 1;
}

static int BMX_CDECL stCull(int32_t a, int32_t b, int32_t c)
{
    rwtLog(0xb4);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
    rwtLog((uint32_t)c);
    return (gCullResult >> (gCullCalls++ & 31)) & 1;
}

static int BMX_CDECL stClipCull(int32_t a, int32_t b, int32_t c)
{
    rwtLog(0xb5);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
    rwtLog((uint32_t)c);
    return (gCullResult >> (gCullCalls++ & 31)) & 1;
}

static void BMX_CDECL stClipTri(int32_t a, int32_t b, int32_t c)
{
    rwtLog(0xb6);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
    rwtLog((uint32_t)c);
}

static void BMX_CDECL stIndexTri(int32_t a, int32_t b, int32_t c)
{
    rwtLog(0xb7);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
    rwtLog((uint32_t)c);
}

static void BMX_CDECL stIndexStart(void)
{
    rwtLog(0xb8);
}

static void BMX_CDECL stIndexFlush(void)
{
    rwtLog(0xb9);
}

static void *BMX_CDECL stRealloc(void *block, size_t size)
{
    rwtLog(0x4a5100);
    rwtLogPtr(block);
    rwtLog((uint32_t)size);
    if (dt_u32() % 7 == 0) {
        return NULL;
    }
    return dt_alloc((uint32_t)size);
}

/* ---- fixtures ---- */

/*
 * Random floats in [lo, hi], sometimes 0 or -0. No huge or tiny values: x87 intermediates keep their 15-bit
 * exponent under 24-bit precision, so results that overflow or go denormal on SSE differ
 * (docs/SDL3-DRIVER.md).
 */
static float someFloat(float lo, float hi)
{
    const uint32_t k = dt_u32() % 16;
    return k == 0 ? 0.0f : k == 1 ? -0.0f : dt_float(lo, hi);
}

static void fillFloats(float *f, int n, float lo, float hi)
{
    for (int i = 0; i < n; i++) {
        f[i] = someFloat(lo, hi);
    }
}

/* A level that is surely there, with the vertex buffers (random contents, colours in camera vertices). */
static RwD3DIm3DLevel *pipeEnv(DtCtx *c)
{
    RwD3DIm3DLevel *level;

    dt_seed(c);
    rwtEnv()->memoryFuncs.rwrealloc = stRealloc;
    rwD3DTLBuffer = dt_alloc(kVerts * 2 * sizeof(RwD3DTLVertex));
    rwD3DCamBuffer = dt_alloc(kVerts * 2 * sizeof(RwD3DCamVertex));
    dt_fill(rwD3DTLBuffer, kVerts * 2 * sizeof(RwD3DTLVertex));
    dt_fill(rwD3DCamBuffer, kVerts * 2 * sizeof(RwD3DCamVertex));
    for (int i = 0; i < kVerts * 2; i++) {
        RwD3DCamVertex *cam = &rwD3DCamBuffer[i];
        cam->clip = (uint8_t)(dt_u32() % 3 == 0 ? dt_u32() & 0x3f : 0);
        fillFloats((float *)cam + 4, 4, -20.0f, 300.0f);
    }
    rwD3DBufferSize = (uint32_t)dt_range(0, 3 * kVerts);
    dt_fill(rwD3DIm3DLevels, 10 * sizeof(RwD3DIm3DLevel));
    level = &rwD3DIm3DLevels[dt_range(0, 9)];
    level->base = dt_range(0, kVerts);
    level->numVertices = dt_range(0, kVerts);
    level->numTL = dt_range(0, kVerts);
    level->orClip = dt_u32() % 2 ? dt_u32() & 0x3f : 0;
    level->andClip = dt_u32() % 8 == 0 ? dt_u32() | 1 : 0;
    rwD3DIm3DLevel = level;
    rwD3DIm3DDepth = 1;
    rwD3DTLBase = rwD3DTLBuffer + level->base % kVerts;
    rwD3DCamBase = rwD3DCamBuffer + level->base % kVerts;
    rwD3DSetUVStage = stSetUV;
    gAllocFail = 0;
    gCullResult = (int)dt_u32();
    gCullCalls = 0;
    gLtm = dt_alloc(sizeof(RwMatrix));
    fillFloats((float *)gLtm, 16, -30.0f, 30.0f);
    dt_stub(0x004dc460, NULL, stAllocEntry);
    dt_stub(0x004dc380, NULL, stFreeEntry);
    dt_stub(0x004dc680, NULL, stUseEntry);
    dt_stub(0x004d2b10, NULL, stGetLTM);
    dt_stub(0x004d04f0, NULL, stMultiply);
    dt_stub(0x004eacc0, NULL, stGrowBuffers);
    return level;
}

/* A work object (an instance) with `n` vertices of 0x1c or 0x24 bytes (or 4 more); normals of length <= 1. */
static RwD3DIm3DWork *makeWork(int n)
{
    RwD3DIm3DWork *work = dt_alloc(sizeof *work);
    const int32_t stride = (dt_u32() % 2 ? 0x24 : 0x1c) + 4 * dt_range(0, 1);
    uint8_t *v = dt_alloc((uint32_t)((n + 2) * stride));

    dt_fill(work, sizeof *work);
    dt_fill(v, (uint32_t)((n + 1) * stride));
    work->numVertices = n;
    work->stride = stride;
    work->vertices = v;
    for (int i = 0; i < n; i++) {
        RwD3DIm3DVertex *vertex = (RwD3DIm3DVertex *)(v + i * stride);
        fillFloats((float *)vertex, 3, -20.0f, 20.0f);
        fillFloats((float *)vertex + 3, 3, -1.0f, 1.0f);
    }
    return work;
}

/*
 * A mesh header with `numMeshes` meshes (strips or lists) indexing up to kVerts vertices; strips have at
 * least `minStrip` indices (the render stage loops 2^32 times on shorter ones, like the original).
 */
static RpMeshHeader *makeMesh(int numMeshes, int minStrip)
{
    RpMeshHeader *header = dt_alloc(0x10 + 0x10 + numMeshes * sizeof(RpMesh));
    RpMesh *mesh;

    header->flags = dt_u32() % 3 == 0 ? 1 : 0;
    header->numMeshes = (uint16_t)numMeshes;
    header->serialNum = (uint16_t)dt_u32();
    header->firstMeshOffset = dt_u32() % 2 ? 0x10 : 0;
    header->totalIndices = 0;
    mesh = (RpMesh *)((uint8_t *)header + 0x10 + header->firstMeshOffset);
    for (int i = 0; i < numMeshes; i++) {
        const int n = dt_range((header->flags & 1) != 0 ? minStrip : 0, 24);
        mesh[i].numIndices = (uint32_t)n;
        mesh[i].indices = dt_alloc(2 * (uint32_t)n + 2);
        for (int k = 0; k < n; k++) {
            mesh[i].indices[k] = (uint16_t)dt_range(0, kVerts - 1);
        }
        mesh[i].material = NULL;
        header->totalIndices += (uint32_t)n;
    }
    if (header->flags & 1) {
        header->totalIndices += 2u * (uint32_t)numMeshes; /* room for every strip's two extra indices */
    }
    return header;
}

/* An existing instance of an object: matching the mesh and layout or not. */
static RwD3DIm3DWork *makeInstance(void *owner, RwResEntry **ownerRef, const RpMeshHeader *mesh,
                                   uint32_t type, uint32_t stride, int32_t numVertices, uint32_t header)
{
    const uint32_t tris = mesh->totalIndices;
    RwD3DIm3DWork *work = dt_alloc(header + (uint32_t)numVertices * stride + 6 * tris + kSlack);

    dt_fill(work, header);
    work->entry.owner = owner;
    work->entry.ownerRef = ownerRef;
    work->serialNum = dt_u32() % 4 ? mesh->serialNum : (uint32_t)dt_u32();
    work->type = (uint16_t)(dt_u32() % 4 ? type : dt_u32() % 3);
    work->stride = (int32_t)(dt_u32() % 4 ? stride : 0x20);
    work->numVertices = numVertices;
    work->vertices = (uint8_t *)work + header;
    work->triangles = (uint8_t *)work + header + (uint32_t)numVertices * stride;
    work->triangleStride = 6;
    work->triangleOffset = 0;
    work->matrix = header == 0x88 ? &work->combined : NULL;
    *ownerRef = &work->entry;
    return work;
}

static void pipelineTables(void)
{
    for (uint32_t i = 0; i <= 4; i++) {
        rpPipelineTypes[i] = (i & 4) != 0 ? 2 : 0;
        rpPipelineSizes[i] = (i & 4) != 0 ? 0x24 : 0x1c;
        rpPipelineTypesB[i] = (i & 4) != 0 ? 2 : 0;
        rpPipelineSizesB[i] = (i & 4) != 0 ? 0x24 : 0x1c;
    }
}

static RwCamera *makeCamera(RwGlobals *g)
{
    RwCamera *camera = dt_alloc(sizeof *camera);
    g->curCamera = camera;
    return camera;
}

/* ---- instancing ---- */

static void morphOrig(RpAtomic *atomic)
{
    const uint16_t cw = cameraCw();
    ((void(__attribute__((regparm(1))) *)(RpAtomic *))(uintptr_t)0x004c8ba0)(atomic);
    restoreCw(cw);
}

static void morphImpl(RpAtomic *atomic)
{
    const uint16_t cw = cameraCw();
    rwD3DAtomicMorph(atomic);
    restoreCw(cw);
}

static void setup_morph(DtCtx *c)
{
    pipeEnv(c);
    RpAtomic *atomic = dt_alloc(sizeof *atomic);
    RpGeometry *geometry = dt_alloc(sizeof *geometry);
    const int n = dt_range(0, kVerts);
    const int numTargets = dt_range(1, 4);
    const int normals = dt_u32() % 2;
    RwD3DIm3DWork *work;

    geometry->flags = (uint8_t)(dt_u32() & 0x1c);
    if (!normals) {
        geometry->flags &= (uint8_t)~rpGEOMETRYNORMALS;
    }
    geometry->numVertices = n;
    geometry->numMorphTargets = numTargets;
    geometry->morphTarget = dt_alloc((uint32_t)numTargets * sizeof(RpMorphTarget));
    for (int t = 0; t < numTargets; t++) {
        RpMorphTarget *target = &geometry->morphTarget[t];
        target->verts = dt_alloc((uint32_t)n * sizeof(RwV3d) + 4);
        fillFloats((float *)target->verts, 3 * n, -50.0f, 50.0f);
        if (normals || dt_u32() % 2) {
            target->normals = dt_alloc((uint32_t)n * sizeof(RwV3d) + 4);
            fillFloats((float *)target->normals, 3 * n, -1.0f, 1.0f);
        }
    }
    atomic->geometry = geometry;
    atomic->interpolator.startMorphTarget = (int16_t)dt_range(0, numTargets);
    atomic->interpolator.endMorphTarget =
        dt_u32() % 4 == 0 ? atomic->interpolator.startMorphTarget : (int16_t)dt_range(0, numTargets);
    /* the interpolation needs the end target's normals where the start target has them */
    if (geometry->morphTarget[atomic->interpolator.startMorphTarget % numTargets].normals != NULL) {
        for (int t = 0; t < numTargets; t++) {
            if (geometry->morphTarget[t].normals == NULL) {
                geometry->morphTarget[t].normals = dt_alloc((uint32_t)n * sizeof(RwV3d) + 4);
            }
        }
    }
    atomic->interpolator.position = someFloat(0.0f, 2.0f);
    atomic->interpolator.recipTime = someFloat(0.1f, 4.0f);
    work = makeWork(n);
    atomic->repEntry = &work->entry;
    DT_ARG_PTR(c, 0, atomic);
    rwtEnd();
}

static void setup_atomic_instance(DtCtx *c)
{
    RwD3DIm3DLevel *level = pipeEnv(c);
    RwGlobals *g = RwEngineInstance;
    RpAtomic *atomic = dt_alloc(sizeof *atomic);
    RpGeometry *geometry = dt_alloc(sizeof *geometry);
    const int n = dt_range(0, kVerts);

    (void)level;
    makeCamera(g);
    pipelineTables();
    dt_stub(0x004b6140, NULL, stGeometryFlags);
    dt_stub(0x004c8ba0, NULL, stMorph);
    gFlags = dt_u32();
    gAllocFail = dt_u32() % 8 == 0;
    geometry->flags = (uint8_t)(dt_u32() & 0x1c);
    geometry->numVertices = n;
    geometry->lockedSinceLastInst = (uint16_t)dt_u32();
    geometry->texCoords = dt_alloc((uint32_t)n * sizeof(RwTexCoords) + 4);
    dt_fill(geometry->texCoords, (uint32_t)n * sizeof(RwTexCoords));
    geometry->preLitLum = dt_alloc((uint32_t)n * 4 + 4);
    dt_fill(geometry->preLitLum, (uint32_t)n * 4);
    geometry->mesh = makeMesh(dt_range(0, 4), 0);
    atomic->geometry = dt_u32() % 10 == 0 ? NULL : geometry;
    atomic->object.object.parent = dt_alloc(16);
    atomic->interpolator.flags = dt_u32();
    if (dt_u32() % 2) {
        const uint32_t textured = geometry->flags & 4;
        makeInstance(atomic, &atomic->repEntry, geometry->mesh, rpPipelineTypes[textured],
                     rpPipelineSizes[textured], n, 0x88);
    }
    DT_ARG_PTR(c, 0, atomic);
    rwtEnd();
}

static RwD3DIm3DWork *sectorOrig(RpWorldSector *sector)
{
    const uint16_t cw = cameraCw();
    RwD3DIm3DWork *r = ((RwD3DIm3DWork * (BMX_CDECL *)(RpWorldSector *))(uintptr_t)0x004c92d0)(sector);
    restoreCw(cw);
    return r;
}

static RwD3DIm3DWork *sectorImpl(RpWorldSector *sector)
{
    const uint16_t cw = cameraCw();
    RwD3DIm3DWork *r = rwD3DSectorInstance(sector);
    restoreCw(cw);
    return r;
}

static void setup_sector_instance(DtCtx *c)
{
    pipeEnv(c);
    RwGlobals *g = RwEngineInstance;
    RpWorldSector *sector = dt_alloc(sizeof *sector);
    RpWorld *world = dt_alloc(sizeof *world);
    const int n = dt_range(0, kVerts);

    makeCamera(g);
    pipelineTables();
    dt_stub(0x004bbcc0, NULL, stWorldFlags);
    g->curWorld = world;
    world->object.flags = (uint8_t)(dt_u32() & 0x1c);
    gFlags = dt_u32();
    gAllocFail = dt_u32() % 8 == 0;
    sector->numVertices = (uint16_t)n;
    sector->vertices = dt_alloc((uint32_t)n * sizeof(RwV3d) + 4);
    fillFloats((float *)sector->vertices, 3 * n, -100.0f, 100.0f);
    sector->normals = dt_alloc((uint32_t)n * 4 + 4);
    dt_fill(sector->normals, (uint32_t)n * 4);
    sector->texCoords = dt_alloc((uint32_t)n * sizeof(RwTexCoords) + 4);
    dt_fill(sector->texCoords, (uint32_t)n * sizeof(RwTexCoords));
    sector->preLitLum = dt_alloc((uint32_t)n * 4 + 4);
    dt_fill(sector->preLitLum, (uint32_t)n * 4);
    sector->mesh = makeMesh(dt_range(0, 4), 0);
    if (dt_u32() % 2) {
        const uint32_t textured = world->object.flags & 4;
        makeInstance(sector, &sector->repEntry, sector->mesh, rpPipelineTypesB[textured],
                     rpPipelineSizesB[textured], n, 0x48);
    }
    DT_ARG_PTR(c, 0, sector);
    rwtEnd();
}

/* ---- colours ---- */

static void setup_apply_light(DtCtx *c)
{
    pipeEnv(c);
    RpLight *light = dt_alloc(sizeof *light);
    light->worldExtCallback = (void *)stLight;
    DT_ARG_PTR(c, 0, makeWork(dt_range(0, 4)));
    DT_ARG_PTR(c, 1, light);
    DT_ARG_PTR(c, 2, dt_u32() % 2 ? dt_alloc(sizeof(RwMatrix)) : NULL);
    DT_ARG_FLOAT(c, 3, dt_float(0.25f, 4.0f));
    DT_ARG_PTR(c, 4, dt_alloc(sizeof(RwSurfaceProperties)));
    rwtEnd();
}

typedef void (*StageFn)(RwD3DIm3DWork *work);

static void stageOrig(uint32_t addr, RwD3DIm3DWork *work)
{
    const uint16_t cw = cameraCw();
    ((void(BMX_CDECL *)(RwD3DIm3DWork *))(uintptr_t)addr)(work);
    restoreCw(cw);
}

static void stageImpl(StageFn fn, RwD3DIm3DWork *work)
{
    const uint16_t cw = cameraCw();
    fn(work);
    restoreCw(cw);
}

static void colorsOrig(RwD3DIm3DWork *work)
{
    stageOrig(0x004c9880, work);
}

static void colorsImpl(RwD3DIm3DWork *work)
{
    stageImpl(rwD3DLightColors, work);
}

static void renderOrig(RwD3DIm3DWork *work)
{
    stageOrig(0x004cac00, work);
}

static void renderImpl(RwD3DIm3DWork *work)
{
    stageImpl(rwD3DRenderMeshes, work);
}

static void setup_stage(DtCtx *c)
{
    pipeEnv(c);
    RwD3DIm3DWork *work = makeWork(dt_range(0, kVerts));
    work->flags = dt_u32();
    work->lit = (uint16_t)dt_u32();
    /* camera colours from the light routines: sometimes far out of range, NaN or past int32 */
    for (int i = 0; i < kVerts; i++) {
        float *rgba = &rwD3DCamBase[i].r;
        for (int k = 0; k < 4; k++) {
            const uint32_t pick = dt_u32() % 32;
            if (pick == 0) {
                const uint32_t nan = dt_u32() % 2 ? 0x7fc00000u : 0xffc00000u;
                __builtin_memcpy(&rgba[k], &nan, 4);
            } else if (pick == 1) {
                rgba[k] = dt_u32() % 2 ? 3e9f : -3e9f;
            } else if (pick == 2) {
                const uint32_t above = 0x437f0001u; /* the next float above 255 */
                __builtin_memcpy(&rgba[k], &above, 4);
            } else if (pick < 6) {
                rgba[k] = 255.0f;
            }
        }
    }
    DT_ARG_PTR(c, 0, work);
    rwtEnd();
}

/* ---- light routines ---- */

typedef void (*LightFn)(RwD3DIm3DWork *, RpLight *, const RwMatrix *, float, const RwSurfaceProperties *);

static void lightOrig(uint32_t addr, RwD3DIm3DWork *work, RpLight *light, const RwMatrix *inverse,
                      float scale, const RwSurfaceProperties *surface)
{
    const uint16_t cw = cameraCw();
    ((RwD3DLightFunc)(uintptr_t)addr)(work, light, inverse, scale, surface);
    restoreCw(cw);
}

static void lightImpl(LightFn fn, RwD3DIm3DWork *work, RpLight *light, const RwMatrix *inverse, float scale,
                      const RwSurfaceProperties *surface)
{
    const uint16_t cw = cameraCw();
    fn(work, light, inverse, scale, surface);
    restoreCw(cw);
}

// NOLINTBEGIN(bugprone-macro-parentheses)
#define LIGHT_WRAPPERS(name, addr, impl)                                                                     \
    static void name##Orig(RwD3DIm3DWork *work, RpLight *light, const RwMatrix *inverse, float scale,        \
                           const RwSurfaceProperties *surface)                                               \
    {                                                                                                        \
        lightOrig(addr, work, light, inverse, scale, surface);                                               \
    }                                                                                                        \
    static void name##Impl(RwD3DIm3DWork *work, RpLight *light, const RwMatrix *inverse, float scale,        \
                           const RwSurfaceProperties *surface)                                               \
    {                                                                                                        \
        lightImpl(impl, work, light, inverse, scale, surface);                                               \
    }
// NOLINTEND(bugprone-macro-parentheses)

LIGHT_WRAPPERS(ambient, 0x004c9b20, rwD3DLightAmbient)
LIGHT_WRAPPERS(directional, 0x004c9c20, rwD3DLightDirectional)
LIGHT_WRAPPERS(point, 0x004c9e20, rwD3DLightPoint)
LIGHT_WRAPPERS(spot, 0x004ca130, rwD3DLightSpot)
LIGHT_WRAPPERS(softSpot, 0x004ca5b0, rwD3DLightSoftSpot)

/*
 * A light near the vertices (most of them inside its radius and cone), white (privateFlags bit 0) or not; an
 * object space matrix or none; the light frame's LTM from the stub.
 */
static void setup_light(DtCtx *c)
{
    pipeEnv(c);
    RpLight *light = dt_alloc(sizeof *light);
    RwSurfaceProperties *surface = dt_alloc(sizeof *surface);
    RwD3DIm3DWork *work = makeWork(dt_range(0, kVerts));

    dt_stub(0x004d3a90, NULL, stTransformPoints);
    dt_stub(0x004d3ae0, NULL, stTransformVectors);
    dt_stub(0x004d3650, NULL, stNormalize);
    dt_stub(0x004d38d0, NULL, stInvSqrt);
    dt_stub(0x004d3880, NULL, stSqrt);
    light->object.object.privateFlags = (uint8_t)dt_u32();
    light->object.object.parent = dt_u32() % 10 == 0 ? NULL : dt_alloc(16);
    light->radius = someFloat(2.0f, 60.0f);
    fillFloats(light->color, 4, -0.5f, 1.5f);
    light->coneAngle = someFloat(0.0f, 2.5f);
    fillFloats((float *)surface, 3, 0.0f, 1.0f);
    fillFloats((float *)gLtm + 8, 3, -1.0f, 1.0f);
    fillFloats((float *)gLtm + 12, 3, -15.0f, 15.0f);
    DT_ARG_PTR(c, 0, work);
    DT_ARG_PTR(c, 1, light);
    DT_ARG_PTR(c, 2, dt_u32() % 2 ? dt_alloc(sizeof(RwMatrix)) : NULL);
    DT_ARG_FLOAT(c, 3, someFloat(0.25f, 4.0f));
    DT_ARG_PTR(c, 4, surface);
    rwtEnd();
}

/* ---- render stage ---- */

static void modulateOrig(uint32_t index, const float *color)
{
    const uint16_t cw = cameraCw();
    ((void(BMX_CDECL *)(uint32_t, const float *))(uintptr_t)0x004caad0)(index, color);
    restoreCw(cw);
}

static void modulateImpl(uint32_t index, const float *color)
{
    const uint16_t cw = cameraCw();
    rwD3DModulateColor(index, color);
    restoreCw(cw);
}

static void vertexColorOrig(uint32_t index)
{
    const uint16_t cw = cameraCw();
    ((void(BMX_CDECL *)(uint32_t))(uintptr_t)0x004cab70)(index);
    restoreCw(cw);
}

static void vertexColorImpl(uint32_t index)
{
    const uint16_t cw = cameraCw();
    rwD3DVertexColor(index);
    restoreCw(cw);
}

/* A vertex index with garbage in the upper half (the callers store only a word). */
static void setup_vertex_color(DtCtx *c)
{
    pipeEnv(c);
    float *color = dt_alloc(16);
    for (int k = 0; k < 3; k++) {
        color[k] = (float)dt_range(0, 255) * (1.0f / 255.0f);
    }
    color[3] = (float)dt_range(0, 255);
    DT_ARG_INT(c, 0, (dt_u32() & 0xffff0000u) | (uint32_t)dt_range(0, kVerts - 1));
    DT_ARG_PTR(c, 1, color);
    rwtEnd();
}

static RpMaterial *makeMaterial(void)
{
    RpMaterial *material = dt_alloc(sizeof *material);
    const uint32_t pick = dt_u32() % 4;

    if (pick == 0) {
        material->color = (RwRGBA){0xff, 0xff, 0xff, 0xff};
    } else {
        material->color = (RwRGBA){(uint8_t)dt_u32(), (uint8_t)dt_u32(), (uint8_t)dt_u32(),
                                   (uint8_t)(pick == 1 ? 0xff : dt_u32())};
    }
    if (dt_u32() % 3 != 0) {
        RwTexture *texture = dt_alloc(sizeof *texture);
        texture->raster = dt_u32() % 5 == 0 ? NULL : dt_alloc(16);
        texture->filterMode = dt_range(0, 6);
        texture->addressing = dt_range(0, 4);
        material->texture = texture;
    }
    return material;
}

static void setup_render(DtCtx *c)
{
    pipeEnv(c);
    const int n = dt_range(0, kVerts);
    RwD3DIm3DWork *work = makeWork(n);
    RpMeshHeader *mesh = makeMesh(dt_range(0, 4), 2);
    RpMesh *meshes = (RpMesh *)((uint8_t *)mesh + 0x10 + mesh->firstMeshOffset);
    uint32_t tris = 0;

    dt_stub(0x004db820, NULL, stStateSet);
    dt_stub(0x004eafa0, NULL, stBitsAlloc);
    dt_stub(0x004eadc0, NULL, stListResize);
    rwD3DTriCull = stCull;
    rwD3DClipTriCull = stClipCull;
    rwD3DClipTri = stClipTri;
    rwD3DIndexTri = stIndexTri;
    rwD3DIndexStart = stIndexStart;
    rwD3DIndexFlush = stIndexFlush;
    rwD3DTriIndices.capacity = (uint32_t)dt_range(0, 200);
    rwD3DTriIndices.count = (uint32_t)dt_range(0, 100);
    rwD3DVertexBits = dt_alloc(kVerts / 8 + 8);
    dt_fill(rwD3DVertexBits, kVerts / 8 + 8);
    rwD3DVertexBitsSize = (uint32_t)dt_range(0, kVerts);
    for (int i = 0; i < mesh->numMeshes; i++) {
        meshes[i].material = makeMaterial();
        tris += (mesh->flags & 1) != 0 ? meshes[i].numIndices - 2 : meshes[i].numIndices / 3;
    }
    work->mesh = mesh;
    work->flags = dt_u32();
    work->triangleStride = 6 + 2 * dt_range(0, 1);
    work->triangleOffset = 2 * dt_range(0, 3);
    work->triangles = dt_alloc((tris + 1) * (uint32_t)work->triangleStride + 8);
    for (uint32_t i = 0; i < (tris + 1) * (uint32_t)work->triangleStride / 2; i++) {
        ((uint16_t *)work->triangles)[i] = (uint16_t)dt_range(0, kVerts - 1);
    }
    DT_ARG_PTR(c, 0, work);
    rwtEnd();
}

static void setup_bits(DtCtx *c)
{
    pipeEnv(c);
    gRwtMallocFailAt = dt_range(0, 2);
    rwD3DVertexBits = dt_u32() % 3 == 0 ? NULL : dt_alloc(16);
    rwD3DVertexBitsSize = dt_u32();
    DT_ARG_INT(c, 0, dt_range(0, 300));
    rwtEnd();
    gRwtMallocFailAt = dt_range(0, 2);
}

DT_CASES(rwd3dpipe_cases) = {
    {"rwD3DAtomicMorph", (uint32_t)(uintptr_t)morphOrig, morphImpl, 1, DT_RET_VOID, setup_morph, 400, 0},
    {"rwD3DAtomicInstance", 0x004c8e30, rwD3DAtomicInstance, 1, DT_RET_INT, setup_atomic_instance, 400, 0},
    {"rwD3DSectorInstance", (uint32_t)(uintptr_t)sectorOrig, sectorImpl, 1, DT_RET_INT, setup_sector_instance,
     400, 0},
    {"rwD3DApplyLight", 0x004c9790, rwD3DApplyLight, 5, DT_RET_VOID, setup_apply_light, 50, 0},
    {"rwD3DPrelight", 0x004c9800, rwD3DPrelight, 1, DT_RET_VOID, setup_stage, 100, 0},
    {"rwD3DLightColors", (uint32_t)(uintptr_t)colorsOrig, colorsImpl, 1, DT_RET_VOID, setup_stage, 400, 0},
    {"rwD3DLightAmbient", (uint32_t)(uintptr_t)ambientOrig, ambientImpl, 5, DT_RET_VOID, setup_light, 300, 0},
    {"rwD3DLightDirectional", (uint32_t)(uintptr_t)directionalOrig, directionalImpl, 5, DT_RET_VOID,
     setup_light, 400, 0},
    {"rwD3DLightPoint", (uint32_t)(uintptr_t)pointOrig, pointImpl, 5, DT_RET_VOID, setup_light, 500, 0},
    {"rwD3DLightSpot", (uint32_t)(uintptr_t)spotOrig, spotImpl, 5, DT_RET_VOID, setup_light, 500, 0},
    {"rwD3DLightSoftSpot", (uint32_t)(uintptr_t)softSpotOrig, softSpotImpl, 5, DT_RET_VOID, setup_light, 500,
     0},
    {"rwD3DModulateColor", (uint32_t)(uintptr_t)modulateOrig, modulateImpl, 2, DT_RET_VOID,
     setup_vertex_color, 200, 0},
    {"rwD3DVertexColor", (uint32_t)(uintptr_t)vertexColorOrig, vertexColorImpl, 1, DT_RET_VOID,
     setup_vertex_color, 200, 0},
    {"rwD3DRenderMeshes", (uint32_t)(uintptr_t)renderOrig, renderImpl, 1, DT_RET_VOID, setup_render, 400, 0},
    {"rwD3DVertexBitsAlloc", 0x004eafa0, rwD3DVertexBitsAlloc, 1, DT_RET_INT, setup_bits, 100, 0},
};
