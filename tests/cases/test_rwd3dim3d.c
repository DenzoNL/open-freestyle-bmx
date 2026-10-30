/*
 * Tests for rw/d3dim3d.c, the Im3D side of the D3D7 driver: transform stack, index lists, the plugin, the
 * pipeline stages and the draw entries. The driver's state lives in the exe's data (0x696900..); the engine
 * is dt_rw.h's fixture, the callees through the driver's function pointers are logging stubs.
 *
 * Float code runs with the x87 control word of a camera update (24-bit precision, rounding down), which the
 * original's camera begin update sets: the wrappers below set it around both sides.
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwd3d.h"
#include "rw/rwtexture.h"

enum { kVerts = 48 };

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

static int gCullResult; /* bit n: the n-th cull call culls */
static int gCullCalls;

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

static int BMX_CDECL stCull(int32_t a, int32_t b, int32_t c)
{
    rwtLog(0xc0);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
    rwtLog((uint32_t)c);
    return (gCullResult >> (gCullCalls++ & 31)) & 1;
}

static int BMX_CDECL stClipCull(int32_t a, int32_t b, int32_t c)
{
    rwtLog(0xc1);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
    rwtLog((uint32_t)c);
    return (gCullResult >> (gCullCalls++ & 31)) & 1;
}

static void BMX_CDECL stClipTri(int32_t a, int32_t b, int32_t c)
{
    rwtLog(0xc2);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
    rwtLog((uint32_t)c);
}

static void BMX_CDECL stClipLine(int32_t a, int32_t b)
{
    rwtLog(0xc3);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
}

static void BMX_CDECL stIndexTri(int32_t a, int32_t b, int32_t c)
{
    rwtLog(0xc4);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
    rwtLog((uint32_t)c);
}

static void BMX_CDECL stIndexLine(int32_t a, int32_t b)
{
    rwtLog(0xc5);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
}

static void BMX_CDECL stIndexStart(void)
{
    rwtLog(0xc6);
}

static void BMX_CDECL stIndexFlush(void)
{
    rwtLog(0xc7);
}

static void BMX_CDECL stUVStage(RwD3DIm3DWork *work)
{
    rwtLog(0xc8);
    rwtLogPtr(work);
}

static void BMX_CDECL stSetUV(uint32_t flags)
{
    rwtLog(0xc9);
    rwtLog(flags);
}

static void BMX_CDECL stProjection(int32_t projection)
{
    rwtLog(0xca);
    rwtLog((uint32_t)projection);
}

static int BMX_CDECL stIm2DIndexed(int32_t type, void *vertices, int32_t n, void *indices, int32_t count)
{
    rwtLog(0xcb);
    rwtLog((uint32_t)type);
    rwtLogPtr(vertices);
    rwtLog((uint32_t)n);
    rwtLogPtr(indices);
    rwtLog((uint32_t)count);
    return 1;
}

static RwMatrix *BMX_CDECL stMultiply(RwMatrix *out, const RwMatrix *a, const RwMatrix *b)
{
    rwtLog(0xcc);
    rwtLogPtr(out);
    rwtLogPtr(a);
    rwtLogPtr(b);
    for (int i = 0; i < 16; i++) {
        ((uint32_t *)out)[i] = dt_u32();
    }
    return out;
}

static int BMX_CDECL stStateSet(int32_t state, int32_t value)
{
    rwtLog(0xcd);
    rwtLog((uint32_t)state);
    rwtLog((uint32_t)value);
    return 1;
}

static void BMX_CDECL stGrowBuffers(uint32_t extra)
{
    rwtLog(0xce);
    rwtLog(extra);
}

/*
 * Random floats in [lo, hi], sometimes 0 or -0. No huge or tiny edge values: under the original's 24-bit
 * x87 precision intermediates keep the 15-bit exponent, so results that overflow or go denormal on SSE
 * differ (a known limit of the float model, docs/SDL3-DRIVER.md).
 */
static void fillFloats(float *f, int n, float lo, float hi)
{
    for (int i = 0; i < n; i++) {
        const uint32_t k = dt_u32() % 16;
        f[i] = k == 0 ? 0.0f : k == 1 ? -0.0f : dt_float(lo, hi);
    }
}

/*
 * The driver state: engine, vertex buffers with random contents (clip flags of each camera vertex from
 * `clipMask`), a top level at a random depth, logging stubs in every function pointer.
 */
static RwGlobals *d3dEnv(DtCtx *c, uint32_t clipMask)
{
    dt_seed(c);
    RwGlobals *g = rwtEnv();
    g->memoryFuncs.rwrealloc = stRealloc;
    rwD3DTLBuffer = dt_alloc(kVerts * 2 * sizeof(RwD3DTLVertex));
    rwD3DCamBuffer = dt_alloc(kVerts * 2 * sizeof(RwD3DCamVertex));
    dt_fill(rwD3DTLBuffer, kVerts * 2 * sizeof(RwD3DTLVertex));
    dt_fill(rwD3DCamBuffer, kVerts * 2 * sizeof(RwD3DCamVertex));
    for (int i = 0; i < kVerts * 2; i++) {
        rwD3DCamBuffer[i].clip = (uint8_t)(dt_u32() % 3 == 0 ? dt_u32() & clipMask : 0);
    }
    rwD3DBufferSize = (uint32_t)dt_range(0, 3 * kVerts);
    rwD3DIm3DDepth = (uint32_t)dt_range(0, 11);
    dt_fill(rwD3DIm3DLevels, 10 * sizeof(RwD3DIm3DLevel));
    for (int i = 0; i < 10; i++) {
        rwD3DIm3DLevels[i].base = dt_range(0, kVerts / 2);
        rwD3DIm3DLevels[i].numVertices = dt_range(0, kVerts / 2);
        rwD3DIm3DLevels[i].numTL = dt_range(0, kVerts);
        rwD3DIm3DLevels[i].andClip = dt_u32() % 4 == 0 ? dt_u32() : 0;
    }
    rwD3DIm3DLevel = dt_u32() % 8 == 0 ? NULL : &rwD3DIm3DLevels[dt_range(0, 9)];
    rwD3DTLBase = rwD3DTLBuffer + (rwD3DIm3DLevel != NULL ? rwD3DIm3DLevel->base : 0);
    rwD3DCamBase = rwD3DCamBuffer + (rwD3DIm3DLevel != NULL ? rwD3DIm3DLevel->base : 0);
    rwD3DTriIndices.indices = dt_alloc(64);
    rwD3DTriIndices.capacity = (uint32_t)dt_range(0, 32);
    rwD3DTriIndices.count = (uint32_t)dt_range(0, 30);
    rwD3DLineIndices.indices = dt_alloc(64);
    rwD3DLineIndices.capacity = (uint32_t)dt_range(0, 32);
    rwD3DLineIndices.count = (uint32_t)dt_range(0, 30);
    rwD3DTriCull = stCull;
    rwD3DClipTriCull = stClipCull;
    rwD3DClipTri = stClipTri;
    rwD3DClipLine = stClipLine;
    rwD3DIndexTri = stIndexTri;
    rwD3DIndexLine = stIndexLine;
    rwD3DIndexStart = stIndexStart;
    rwD3DIndexFlush = stIndexFlush;
    rwD3DSetUVStage = stSetUV;
    rwD3DSetProjection = stProjection;
    gCullResult = (int)dt_u32();
    gCullCalls = 0;
    dt_stub(0x004db950, NULL, stIm2DIndexed);
    dt_stub(0x004d04f0, NULL, stMultiply);
    dt_stub(0x004db820, NULL, stStateSet);
    return g;
}

/* A level that is surely there. */
static RwD3DIm3DLevel *needLevel(void)
{
    if (rwD3DIm3DLevel == NULL) {
        rwD3DIm3DLevel = &rwD3DIm3DLevels[0];
    }
    rwD3DTLBase = rwD3DTLBuffer + rwD3DIm3DLevel->base;
    rwD3DCamBase = rwD3DCamBuffer + rwD3DIm3DLevel->base;
    return rwD3DIm3DLevel;
}

static RwD3DIm3DWork *makeWork(int n, int32_t stride)
{
    RwD3DIm3DWork *work = dt_alloc(sizeof *work);
    uint8_t *v = dt_alloc((uint32_t)((n + 1) * stride));
    dt_fill(work, sizeof *work);
    dt_fill(v, (uint32_t)((n + 1) * stride));
    work->numVertices = n;
    work->stride = stride;
    work->vertices = v;
    return work;
}

/* ---- stack, buffers and index lists ---- */

static void setup_begin(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : dt_alloc(0x1c));
    rwtEnd();
}

static void setup_plain(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    rwtEnd();
}

static void setup_grow(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    needLevel();
    if (dt_u32() % 3 == 0) {
        rwD3DTLBuffer = NULL;
    }
    if (dt_u32() % 3 == 0) {
        rwD3DCamBuffer = NULL;
    }
    DT_ARG_INT(c, 0, dt_range(0, 300));
    rwtEnd();
}

static int listGrowOrig(RwD3DIndexList *list, uint32_t capacity)
{
    return ((int(__attribute__((regparm(2))) *)(RwD3DIndexList *, uint32_t))(uintptr_t)0x004eadc8)(list,
                                                                                                   capacity);
}

static int listGrowImpl(RwD3DIndexList *list, uint32_t capacity)
{
    return rwD3DIndexListGrow(list, capacity);
}

static void setup_list(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    DT_ARG_PTR(c, 0, dt_u32() % 2 ? &rwD3DTriIndices : &rwD3DLineIndices);
    DT_ARG_INT(c, 1, dt_range(0, 200));
    if (dt_u32() % 5 == 0) {
        rwD3DTriIndices.indices = NULL;
        rwD3DLineIndices.indices = NULL;
    }
    rwtEnd();
}

static void setup_index_tri(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    DT_ARG_INT(c, 0, (int32_t)dt_u32());
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
    DT_ARG_INT(c, 2, (int32_t)dt_u32());
    rwtEnd();
}

static void setup_level(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    needLevel();
    rwtEnd();
}

static void setup_open(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    gRwtMallocFailAt = dt_range(0, 4);
    rwD3DIm3DSpare0 = dt_u32() % 2 ? dt_alloc(8) : NULL;
    if (dt_u32() % 3 == 0) {
        rwD3DIm3DWork = NULL;
        rwD3DTLBuffer = NULL;
    }
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
    gRwtMallocFailAt = dt_range(0, 4);
}

/* ---- pipeline stages ---- */

static void setup_uv(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    needLevel()->orClip = dt_u32() % 2 ? dt_u32() : 0;
    DT_ARG_PTR(c, 0,
               makeWork(dt_range(0, kVerts / 2), (int32_t)sizeof(RwD3DIm3DVertex) + 4 * dt_range(0, 2)));
    rwtEnd();
}

static void setup_flags(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    needLevel();
    DT_ARG_INT(c, 0, dt_u32());
    rwtEnd();
}

static void setup_projection(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    DT_ARG_INT(c, 0, dt_range(0, 3));
    rwtEnd();
}

static void setup_state(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    DT_ARG_INT(c, 0, dt_range(3, 7));
    DT_ARG_INT(c, 1, dt_range(0, 3));
    rwtEnd();
}

static RwCamera *makeCamera(RwGlobals *g)
{
    RwCamera *camera = dt_alloc(sizeof *camera);
    RwRaster *raster = dt_alloc(sizeof *raster);
    dt_fill(camera, sizeof *camera);
    dt_fill(raster, sizeof *raster);
    raster->parent = raster;
    camera->frameBuffer = raster;
    camera->projection = dt_range(0, 3);
    fillFloats((float *)&camera->viewMatrix, 16, -4.0f, 4.0f);
    g->curCamera = camera;
    return camera;
}

static void setup_camera(DtCtx *c)
{
    RwGlobals *g = d3dEnv(c, 0x3f);
    DT_ARG_PTR(c, 0, makeCamera(g));
    rwtEnd();
}

static void setup_instance(DtCtx *c)
{
    RwGlobals *g = d3dEnv(c, 0x3f);
    needLevel();
    makeCamera(g);
    rwD3DIm3DWork = dt_alloc(sizeof(RwD3DIm3DWork));
    dt_fill(rwD3DIm3DWork, sizeof(RwD3DIm3DWork));
    RwIm3DTransformInfo *info = dt_alloc(sizeof *info);
    info->vertices = dt_alloc(16);
    info->numVertices = dt_range(0, 400);
    info->flags = (int32_t)dt_u32();
    info->ltm = dt_u32() % 2 ? dt_alloc(sizeof(RwMatrix)) : NULL;
    DT_ARG_PTR(c, 0, info);
    rwtEnd();
}

static void transformOrig(RwD3DIm3DWork *work)
{
    const uint16_t cw = cameraCw();
    ((void(BMX_CDECL *)(RwD3DIm3DWork *))(uintptr_t)0x004f97c0)(work);
    restoreCw(cw);
}

static void transformImpl(RwD3DIm3DWork *work)
{
    const uint16_t cw = cameraCw();
    rwD3DIm3DTransform(work);
    restoreCw(cw);
}

static void setup_transform(DtCtx *c)
{
    RwGlobals *g = d3dEnv(c, 0x3f);
    RwCamera *camera = makeCamera(g);
    RwD3DIm3DLevel *level = needLevel();
    const int n = dt_range(0, kVerts);
    RwD3DIm3DWork *work = makeWork(n, (int32_t)sizeof(RwD3DIm3DVertex) + 4 * dt_range(0, 2));
    RwMatrix *m = dt_alloc(sizeof *m);
    const float near = dt_float(0.01f, 2.0f), far = dt_float(10.0f, 200.0f), fog = dt_float(0.0f, 50.0f);

    camera->projection = dt_u32() % 3 == 0 ? 2 : 1;
    fillFloats((float *)m, 16, -3.0f, 3.0f);
    m->pos.z = dt_float(-10.0f, 60.0f);
    work->matrix = m;
    for (int i = 0; i < n; i++) {
        RwD3DIm3DVertex *v = (RwD3DIm3DVertex *)((uint8_t *)work->vertices + i * work->stride);
        fillFloats((float *)v, 3, -20.0f, 20.0f);
    }
    rwD3DNearZ = rwD3DBits(near);
    rwD3DFarZ = rwD3DBits(far);
    rwD3DFogStart = rwD3DBits(fog);
    rwD3DFogEnd = far;
    rwD3DFogScale = 255.0f / (far - fog);
    rwD3DZScale = dt_float(-2.0f, 2.0f);
    rwD3DZShift = dt_float(-1.0f, 1.0f);
    rwD3DXScale = (float)dt_range(1, 1600);
    rwD3DYScale = (float)dt_range(1, 1200);
    rwD3DXOffset = (float)dt_range(-64, 64);
    rwD3DYOffset = (float)dt_range(-64, 64);
    dt_fill((void *)rwD3DFogTable, 256 * 4);
    level->uvStage = stUVStage;
    DT_ARG_PTR(c, 0, work);
    rwtEnd();
}

static void setup_pipeline(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    uint32_t *p = dt_alloc(0x1c);
    dt_fill(p, 0x1c);
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : p);
    rwtEnd();
}

/* ---- culling ---- */

static int cullPerspOrig(int32_t a, int32_t b, int32_t c)
{
    const uint16_t cw = cameraCw();
    const int r = ((int(BMX_CDECL *)(int32_t, int32_t, int32_t))(uintptr_t)0x004f3de0)(a, b, c);
    restoreCw(cw);
    return r;
}

static int cullPerspImpl(int32_t a, int32_t b, int32_t c)
{
    const uint16_t cw = cameraCw();
    const int r = rwD3DTriCullPersp(a, b, c);
    restoreCw(cw);
    return r;
}

static int cullParallelOrig(int32_t a, int32_t b, int32_t c)
{
    const uint16_t cw = cameraCw();
    const int r = ((int(BMX_CDECL *)(int32_t, int32_t, int32_t))(uintptr_t)0x004f4e30)(a, b, c);
    restoreCw(cw);
    return r;
}

static int cullParallelImpl(int32_t a, int32_t b, int32_t c)
{
    const uint16_t cw = cameraCw();
    const int r = rwD3DTriCullParallel(a, b, c);
    restoreCw(cw);
    return r;
}

static int clipCullOrig(int32_t a, int32_t b, int32_t c)
{
    const uint16_t cw = cameraCw();
    const int r = ((int(BMX_CDECL *)(int32_t, int32_t, int32_t))(uintptr_t)0x004f3d20)(a, b, c);
    restoreCw(cw);
    return r;
}

static int clipCullImpl(int32_t a, int32_t b, int32_t c)
{
    const uint16_t cw = cameraCw();
    const int r = rwD3DClipTriCullPersp(a, b, c);
    restoreCw(cw);
    return r;
}

static void setup_cull(DtCtx *c)
{
    d3dEnv(c, 0x3f);
    needLevel();
    for (int i = 0; i < kVerts; i++) {
        fillFloats((float *)&rwD3DTLBase[i], 3, -700.0f, 700.0f);
        fillFloats((float *)&rwD3DCamBase[i], 3, -50.0f, 50.0f);
    }
    for (int i = 0; i < 3; i++) {
        DT_ARG_INT(c, i, dt_range(0, kVerts - 1));
    }
    if (dt_u32() % 8 == 0) {
        c->args[1] = c->args[0];
    }
    rwtEnd();
}

/* ---- draw entries ---- */

static void lineOrig(int32_t a, int32_t b)
{
    ((void(__attribute__((regparm(2))) *)(int32_t, int32_t))(uintptr_t)0x004f5f00)(a, b);
}

static void lineImpl(int32_t a, int32_t b)
{
    rwD3DRenderLineIndices(a, b);
}

static void triOrig(int32_t a, int32_t b, int32_t c)
{
    ((void(__attribute__((regparm(3))) *)(int32_t, int32_t, int32_t))(uintptr_t)0x004f5f80)(a, b, c);
}

static void triImpl(int32_t a, int32_t b, int32_t c)
{
    rwD3DRenderTriIndices(a, b, c);
}

static void setup_draw(DtCtx *c)
{
    d3dEnv(c, dt_u32() % 2 ? 0x3f : 0x01);
    needLevel();
    if (dt_u32() % 4 == 0) {
        dt_stub(0x004eacc0, NULL, stGrowBuffers);
    }
    for (int i = 0; i < 3; i++) {
        DT_ARG_INT(c, i, dt_range(0, kVerts - 1));
    }
    rwtEnd();
}

static void setup_primitive(DtCtx *c)
{
    d3dEnv(c, dt_u32() % 2 ? 0x3f : 0x03);
    needLevel()->numVertices = dt_range(0, kVerts);
    dt_stub(0x004eacc0, NULL, stGrowBuffers);
    DT_ARG_INT(c, 0, dt_range(0, 6));
    rwtEnd();
}

static void setup_indexed(DtCtx *c)
{
    d3dEnv(c, dt_u32() % 2 ? 0x3f : 0x03);
    needLevel();
    dt_stub(0x004eacc0, NULL, stGrowBuffers);
    const int n = dt_range(0, 40);
    uint16_t *indices = dt_alloc(2 * (uint32_t)n + 2);
    for (int i = 0; i < n; i++) {
        indices[i] = (uint16_t)dt_range(0, kVerts - 1);
    }
    DT_ARG_INT(c, 0, dt_range(0, 6));
    DT_ARG_PTR(c, 1, indices);
    DT_ARG_INT(c, 2, n);
    rwtEnd();
}

DT_CASES(rwd3dim3d_cases) = {
    {"rwD3DIm3DBegin", 0x004ea960, rwD3DIm3DBegin, 1, DT_RET_INT, setup_begin, 300, 0},
    {"rwD3DIm3DEnd", 0x004eaa10, rwD3DIm3DEnd, 0, DT_RET_INT, setup_plain, 300, 0},
    {"rwD3DIm3DGrow", 0x004eacc0, rwD3DIm3DGrow, 1, DT_RET_INT, setup_grow, 300, 0},
    {"rwD3DIndexListGrow", (uint32_t)(uintptr_t)listGrowOrig, listGrowImpl, 2, DT_RET_INT, setup_list, 100,
     0},
    {"rwD3DIndexListResize", 0x004eadc0, rwD3DIndexListResize, 2, DT_RET_INT, setup_list, 100, 0},
    {"rwD3DIndexListCreate", 0x004eb270, rwD3DIndexListCreate, 1, DT_RET_VOID, setup_list, 50, 0},
    {"rwD3DIndexListDestroy", 0x004eb240, rwD3DIndexListDestroy, 1, DT_RET_VOID, setup_list, 50, 0},
    {"rwD3DIndexStartDefault", 0x004eae10, rwD3DIndexStartDefault, 0, DT_RET_VOID, setup_level, 50, 0},
    {"rwD3DIndexTriDefault", 0x004eaeb0, rwD3DIndexTriDefault, 3, DT_RET_VOID, setup_index_tri, 200, 0},
    {"rwD3DIndexLineDefault", 0x004eaf30, rwD3DIndexLineDefault, 2, DT_RET_VOID, setup_index_tri, 200, 0},
    {"rwD3DIndexFlushDefault", 0x004eae30, rwD3DIndexFlushDefault, 0, DT_RET_VOID, setup_level, 100, 0},
    {"rwD3DUVNone", 0x004eb030, rwD3DUVNone, 1, DT_RET_VOID, setup_uv, 10, 0},
    {"rwD3DUVCopy", 0x004eb040, rwD3DUVCopy, 1, DT_RET_VOID, setup_uv, 200, 0},
    {"rwD3DSetUVStageDefault", 0x004eb0f0, rwD3DSetUVStageDefault, 1, DT_RET_VOID, setup_flags, 50, 0},
    {"rwD3DSetUVStage0", 0x004eb0d0, rwD3DSetUVStage0, 1, DT_RET_VOID, setup_flags, 30, 0},
    {"rwD3DIm3DOpen", 0x004eaaa0, rwD3DIm3DOpen, 1, DT_RET_INT, setup_open, 100, 0},
    {"rwD3DIm3DClose", 0x004eabd0, rwD3DIm3DClose, 1, DT_RET_INT, setup_open, 100, 0},
    {"rwD3DIm3DCameraBegin", 0x004eb150, rwD3DIm3DCameraBegin, 1, DT_RET_VOID, setup_camera, 100, 0},
    {"rwD3DIm3DRenderState", 0x004eb120, rwD3DIm3DRenderState, 2, DT_RET_VOID, setup_state, 50, 0},
    {"rwD3DSetProjectionDefault", 0x004f69f0, rwD3DSetProjectionDefault, 1, DT_RET_VOID, setup_projection, 30,
     0},
    {"rwD3DIm3DInstance", 0x004f6a50, rwD3DIm3DInstance, 1, DT_RET_INT, setup_instance, 300, 0},
    {"rwD3DIm3DNothing", 0x004f6b00, rwD3DIm3DNothing, 1, DT_RET_VOID, setup_uv, 5, 0},
    {"rwD3DIm3DNothing6", 0x004f6b70, rwD3DIm3DNothing6, 1, DT_RET_VOID, setup_uv, 5, 0},
    {"rwD3DIm3DColors", 0x004f6b10, rwD3DIm3DColors, 1, DT_RET_VOID, setup_uv, 100, 0},
    {"rwD3DIm3DTransform", (uint32_t)(uintptr_t)transformOrig, transformImpl, 1, DT_RET_VOID, setup_transform,
     2000, 0},
    {"rwD3DIm3DPipeline", 0x004f6b80, rwD3DIm3DPipeline, 1, DT_RET_INT, setup_pipeline, 20, 0},
    {"rwD3DTriCullPersp", (uint32_t)(uintptr_t)cullPerspOrig, cullPerspImpl, 3, DT_RET_INT, setup_cull, 1000,
     0},
    {"rwD3DTriCullParallel", (uint32_t)(uintptr_t)cullParallelOrig, cullParallelImpl, 3, DT_RET_INT,
     setup_cull, 1000, 0},
    {"rwD3DClipTriCullPersp", (uint32_t)(uintptr_t)clipCullOrig, clipCullImpl, 3, DT_RET_INT, setup_cull,
     1000, 0},
    {"rwD3DRenderLineIndices", (uint32_t)(uintptr_t)lineOrig, lineImpl, 2, DT_RET_VOID, setup_draw, 300, 0},
    {"rwD3DRenderTriIndices", (uint32_t)(uintptr_t)triOrig, triImpl, 3, DT_RET_VOID, setup_draw, 300, 0},
    {"rwD3DIm3DRenderLine", 0x004f69b0, rwD3DIm3DRenderLine, 2, DT_RET_INT, setup_draw, 200, 0},
    {"rwD3DIm3DRenderTriangle", 0x004f6050, rwD3DIm3DRenderTriangle, 3, DT_RET_INT, setup_draw, 200, 0},
    {"rwD3DIm3DRenderPrimitive", 0x004f6090, rwD3DIm3DRenderPrimitive, 1, DT_RET_INT, setup_primitive, 400,
     0},
    {"rwD3DIm3DRenderIndexedPrimitive", 0x004f64e0, rwD3DIm3DRenderIndexedPrimitive, 3, DT_RET_INT,
     setup_indexed, 400, 0},
};
