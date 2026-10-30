/*
 * Tests for rw/d3dstate.c and rw/d3dim2d.c, the render states and Im2D side of the D3D7 driver. Direct3D is a
 * fake IDirect3DDevice7 whose methods log their arguments: the original calls it through its vtable, the port
 * through src/hook/d3dcom.c, so both logs must match call for call.
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwd3d.h"

enum { kExtOffset = 0x40, kVerts = 24 };

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

/* ---- the fake device ---- */

static void logVertices(const void *vertices, uint32_t n)
{
    const uint32_t *w = vertices;
    for (uint32_t i = 0; i < n * 8 && i < 8 * 64; i++) {
        rwtLog(w[i]);
    }
}

static int BMX_STDCALL fakeSetRenderState(void *self, uint32_t state, uint32_t value)
{
    rwtLog(0xd58);
    rwtLog(state);
    rwtLog(dt_canon(value));
    return 0;
}

static int BMX_STDCALL fakeSetTextureStageState(void *self, uint32_t stage, uint32_t type, uint32_t value)
{
    rwtLog(0xda0);
    rwtLog(stage);
    rwtLog(type);
    rwtLog(value);
    return 0;
}

static int BMX_STDCALL fakeSetTexture(void *self, uint32_t stage, void *texture)
{
    rwtLog(0xd98);
    rwtLog(stage);
    rwtLogPtr(texture);
    return 0;
}

static int BMX_STDCALL fakeDrawPrimitive(void *self, uint32_t type, uint32_t fvf, const void *vertices,
                                         uint32_t count, uint32_t flags)
{
    rwtLog(0xd70);
    rwtLog(type);
    rwtLog(fvf);
    rwtLog(count);
    rwtLog(flags);
    logVertices(vertices, count);
    return 0;
}

static int BMX_STDCALL fakeDrawIndexed(void *self, uint32_t type, uint32_t fvf, const void *vertices,
                                       uint32_t numVertices, const uint16_t *indices, uint32_t numIndices,
                                       uint32_t flags)
{
    rwtLog(0xd74);
    rwtLog(type);
    rwtLog(fvf);
    rwtLog(numVertices);
    rwtLog(numIndices);
    rwtLog(flags);
    for (uint32_t i = 0; i < numIndices && i < 256; i++) {
        const uint16_t index = indices[i];
        rwtLog(index);
        if (index < numVertices) {
            logVertices((const RwD3DTLVertex *)vertices + index, 1);
        }
    }
    return 0;
}

static void *gVtbl[0x40];
static void *gDevice[1] = {(void *)gVtbl};

static void *BMX_CDECL stRealloc(void *block, size_t size)
{
    rwtLog(0x4a5100);
    rwtLogPtr(block);
    rwtLog((uint32_t)size);
    if (dt_u32() % 9 == 0) {
        return NULL;
    }
    void *p = dt_alloc((uint32_t)size);
    if (block != NULL) {
        __builtin_memcpy(p, block, size);
    }
    return p;
}

static void BMX_CDECL stDeferCallback(uint8_t *record)
{
    rwtLog(0xe1);
    rwtLogPtr(record);
    rwtLog(*(uint32_t *)record);
}

static RwRaster *gRasters[4];

/* A raster with the driver's plugin data: colour key, texture handle, alpha flag. */
static RwRaster *makeRaster(void)
{
    RwRaster *r = dt_alloc(kExtOffset + 0x20);
    RwD3DRasterInfo *info = dt_alloc(sizeof *info);
    dt_fill(r, kExtOffset + 0x20);
    info->flags = dt_u32();
    *(int32_t *)((uint8_t *)r + kExtOffset + 4) = dt_u32() % 3 == 0 ? (int32_t)dt_u32() : 0;
    *(void **)((uint8_t *)r + kExtOffset + 0xc) = dt_alloc(4);
    *(RwD3DRasterInfo **)((uint8_t *)r + kExtOffset + 0x10) = info;
    return r;
}

/* A state as the driver keeps it (small values, so the caches hit and miss). */
static void randomState(RwD3DStateCache *s)
{
    s->textureAlpha = dt_range(0, 1);
    s->alphaBlend = dt_range(0, 1);
    s->colorKey = dt_range(0, 1);
    s->alphaTest = dt_range(0, 1);
    s->vertexAlpha = dt_range(0, 1);
    s->textureAlpha = dt_range(0, 1);
    s->perspective = dt_range(0, 1);
    s->zTest = dt_range(0, 1);
    s->zWrite = dt_range(0, 1);
    s->shadeMode = dt_range(1, 2);
    s->srcBlend = dt_range(0, 12);
    s->destBlend = dt_range(0, 12);
    s->fogEnable = dt_range(0, 1);
    s->fogColor = dt_range(0, 2);
    s->alphaBlend = dt_range(0, 1);
    s->colorKey = dt_range(0, 1);
    s->alphaTest = dt_range(0, 1);
    s->address = dt_range(0, 5);
    s->borderColor = dt_range(0, 2);
    s->magFilter = dt_range(0, 3);
    s->minFilter = dt_range(0, 3);
    s->mipFilter = dt_range(0, 4);
    s->colorOp = dt_range(2, 4);
    s->alphaOp = dt_range(2, 4);
    s->textureRaster = dt_u32() % 3 == 0 ? NULL : gRasters[dt_range(0, 3)];
}

static void randomSaved(RwD3DSavedState *s)
{
    s->vertexAlpha = dt_range(0, 1);
    s->perspective = dt_range(0, 1);
    s->zTest = dt_range(0, 1);
    s->zWrite = dt_range(0, 1);
    s->shadeMode = dt_range(1, 2);
    s->srcBlend = dt_range(0, 12);
    s->destBlend = dt_range(0, 12);
    s->fogEnable = dt_range(0, 1);
    s->fogColor = dt_range(0, 2);
    s->address = dt_range(0, 5);
    s->borderColor = dt_range(0, 2);
    s->magFilter = dt_range(0, 3);
    s->minFilter = dt_range(0, 3);
    s->mipFilter = dt_range(0, 4);
    s->colorOp = dt_range(2, 4);
    s->alphaOp = dt_range(2, 4);
    s->textureRaster = dt_u32() % 3 == 0 ? NULL : gRasters[dt_range(0, 3)];
}

static RwD3DDeferList *makeList(void)
{
    RwD3DDeferList *list = dt_alloc(sizeof *list);
    list->arenaSize = (uint32_t)dt_range(0, 3) * 0x40;
    list->arena = list->arenaSize != 0 || dt_u32() % 2 ? dt_alloc(0x100) : NULL;
    list->used = (uint32_t)dt_range(0, (int32_t)list->arenaSize / 0x10) * 0x10;
    list->offsetCap = (uint32_t)dt_range(0, 3) * 5;
    list->offsets = list->offsetCap != 0 || dt_u32() % 2 ? dt_alloc(0x40) : NULL;
    list->count = list->offsetCap != 0 ? (uint32_t)dt_range(0, (int32_t)list->offsetCap) : 0;
    list->frames = (uint32_t)(dt_u32() % 4 == 0 ? 0x3f : dt_range(0, 0x3f));
    list->maxUsed = (uint32_t)dt_range(0, 0x100);
    return list;
}

static RwGlobals *stateEnv(DtCtx *c)
{
    dt_seed(c);
    RwGlobals *g = rwtEnv();
    g->memoryFuncs.rwrealloc = stRealloc;
    gVtbl[0x58 / 4] = (void *)fakeSetRenderState;
    gVtbl[0x70 / 4] = (void *)fakeDrawPrimitive;
    gVtbl[0x74 / 4] = (void *)fakeDrawIndexed;
    gVtbl[0x98 / 4] = (void *)fakeSetTexture;
    gVtbl[0xa0 / 4] = (void *)fakeSetTextureStageState;
    uint8_t *context = dt_alloc(16);
    *(void **)(context + 8) = (void *)gDevice;
    rwD3DContext = context;
    uint8_t *caps = dt_alloc(0x130);
    for (int off = 0x100; off < 0x128; off += 4) {
        *(uint32_t *)(caps + off) = dt_u32() % 3 == 0 ? dt_u32() : 0xffffffffu;
    }
    rwD3DCaps = caps;
    rwD3DRasterExtOffset = kExtOffset;
    rwD3DMemory = &g->memoryFuncs;
    for (int i = 0; i < 4; i++) {
        gRasters[i] = makeRaster();
    }
    randomState(&rwD3DState);
    rwD3DState.fogType = dt_range(0, 3);
    rwD3DState.alphaSort = dt_range(0, 1);
    rwD3DAlphaTestCaps = dt_range(0, 1);
    rwD3DDefer = makeList();
    rwD3DRenderOrder = dt_range(1, 2);
    return g;
}

/* ---- render states ---- */

static int setOrig(int32_t state, int32_t value)
{
    return ((int(__attribute__((regparm(2))) *)(int32_t, int32_t))(uintptr_t)0x004d7328)(state, value);
}

static int setImpl(int32_t state, int32_t value)
{
    return rwD3DSetRenderState(state, value);
}

static void BMX_CDECL stFlush(void)
{
    rwtLog(0xe0);
}

static int32_t gFogTable[64];
static float gDensity;

static void setup_set(DtCtx *c)
{
    stateEnv(c);
    if (dt_u32() % 10 == 0) {
        rwD3DContext = NULL;
    }
    dt_stub(0x004d4990, NULL, stFlush);
    const int32_t state = dt_range(0, 18);
    int32_t value = dt_range(0, 12);
    if (state == 1) {
        value = dt_u32() % 3 == 0 ? 0 : (int32_t)(uintptr_t)gRasters[dt_range(0, 3)];
    } else if (state == 15) {
        gDensity = dt_float(0.0f, 2.0f);
        value = (int32_t)(uintptr_t)&gDensity;
        rwD3DContext = NULL; /* the original's exp tables (x87) are not ported: stop at the context check */
    } else if (state == 16) {
        for (int i = 0; i < 64; i++) {
            gFogTable[i] = (int32_t)dt_u32();
        }
        value = (int32_t)(uintptr_t)gFogTable;
    } else if (dt_u32() % 8 == 0) {
        value = (int32_t)dt_u32();
    }
    DT_ARG_INT(c, 0, state);
    DT_ARG_INT(c, 1, value);
    rwtEnd();
}

static void setup_get(DtCtx *c)
{
    stateEnv(c);
    rwD3DState.fogDensity = dt_float(0.0f, 1.0f);
    DT_ARG_INT(c, 0, dt_range(0, 18));
    DT_ARG_PTR(c, 1, dt_alloc(4));
    rwtEnd();
}

static void setup_raster(DtCtx *c)
{
    stateEnv(c);
    DT_ARG_PTR(c, 0, dt_u32() % 4 == 0 ? NULL : gRasters[dt_range(0, 3)]);
    rwtEnd();
}

static void setup_handle(DtCtx *c)
{
    stateEnv(c);
    DT_ARG_PTR(c, 0, gRasters[dt_range(0, 3)]);
    rwtEnd();
}

static void setup_value(DtCtx *c)
{
    stateEnv(c);
    DT_ARG_INT(c, 0, dt_u32() % 8 == 0 ? (int32_t)dt_u32() : dt_range(-1, 13));
    rwtEnd();
}

static void setup_table(DtCtx *c)
{
    stateEnv(c);
    for (int i = 0; i < 64; i++) {
        gFogTable[i] = (int32_t)dt_u32();
    }
    DT_ARG_PTR(c, 0, gFogTable);
    rwtEnd();
}

#define REGPARM1_PAIR(name, addr, port, type)                                                                \
    static int name##Orig(type v)                                                                            \
    {                                                                                                        \
        return ((int(__attribute__((regparm(1))) *)(type))(uintptr_t)(addr))(v);                             \
    }                                                                                                        \
    static int name##Impl(type v)                                                                            \
    {                                                                                                        \
        return port(v);                                                                                      \
    }

REGPARM1_PAIR(texRaster, 0x004d6840, rwD3DSetTextureRaster, RwRaster *)
REGPARM1_PAIR(srcBlend, 0x004d6ba0, rwD3DSetSrcBlend, int32_t)
REGPARM1_PAIR(destBlend, 0x004d6f60, rwD3DSetDestBlend, int32_t)
REGPARM1_PAIR(fogSelect, 0x004d6530, rwD3DFogTableSelect, int32_t)
REGPARM1_PAIR(fogType, 0x004d6740, rwD3DSetFogType, int32_t)
REGPARM1_PAIR(fogTable, 0x004d67c0, rwD3DSetFogTable, const uint32_t *)

/* ---- the deferred list ---- */

static void setup_create(DtCtx *c)
{
    stateEnv(c);
    gRwtMallocFailAt = dt_range(0, 2);
    rwtEnd();
    gRwtMallocFailAt = dt_range(0, 2);
}

static void setup_alloc(DtCtx *c)
{
    stateEnv(c);
    DT_ARG_PTR(c, 0, rwD3DDefer);
    DT_ARG_INT(c, 1, dt_range(0, 0x200));
    DT_ARG_PTR(c, 2, (void *)(uintptr_t)dt_u32());
    rwtEnd();
    gRwtMallocFailAt = dt_range(0, 3);
}

/* A list whose records have the logging callback. */
static void fillRecords(RwD3DDeferList *list, int n)
{
    list->arena = dt_alloc((uint32_t)n * 8 + 8);
    list->offsets = dt_alloc((uint32_t)n * 4 + 4);
    list->count = (uint32_t)n;
    for (int i = 0; i < n; i++) {
        list->offsets[i] = (uint32_t)(n - 1 - i) * 8;
        *(void **)(list->arena + list->offsets[i]) = (void *)stDeferCallback;
        *(uint32_t *)(list->arena + list->offsets[i] + 4) = dt_u32();
    }
}

static void setup_run(DtCtx *c)
{
    stateEnv(c);
    fillRecords(rwD3DDefer, dt_range(0, 6));
    DT_ARG_PTR(c, 0, rwD3DDefer);
    DT_ARG_INT(c, 1, dt_range(0, 3));
    rwtEnd();
}

static void setup_reset(DtCtx *c)
{
    stateEnv(c);
    DT_ARG_PTR(c, 0, rwD3DDefer);
    rwtEnd();
}

static void setup_open(DtCtx *c)
{
    RwGlobals *g = stateEnv(c);
    rwD3DEngineDevice = &g->dOpenDevice;
    gRwtMallocFailAt = dt_range(0, 3);
    rwtEnd();
    gRwtMallocFailAt = dt_range(0, 3);
}

static void saveOrig(RwD3DSavedState *s)
{
    ((void(__attribute__((regparm(1))) *)(RwD3DSavedState *))(uintptr_t)0x004d48f0)(s);
}

static void saveImpl(RwD3DSavedState *s)
{
    rwD3DSaveState(s);
}

static void restoreOrig(const RwD3DSavedState *s)
{
    ((void(__attribute__((regparm(1))) *)(const RwD3DSavedState *))(uintptr_t)0x004d9350)(s);
}

static void restoreImpl(const RwD3DSavedState *s)
{
    rwD3DRestoreState(s);
}

static void setup_save(DtCtx *c)
{
    stateEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(0x60));
    rwtEnd();
}

static void setup_restore(DtCtx *c)
{
    stateEnv(c);
    RwD3DSavedState *s = dt_alloc(sizeof *s);
    randomSaved(s);
    DT_ARG_PTR(c, 0, s);
    rwtEnd();
}

/* A deferred primitive record: state, D3D type, vertices with screen z, indices. */
static uint8_t *makeRecord(uint32_t type, int numVertices, int numIndices)
{
    uint8_t *record = dt_alloc(0x50 + (uint32_t)numVertices * 0x20 + (uint32_t)numIndices * 2 + 4);
    RwD3DTLVertex *v = (RwD3DTLVertex *)(record + 0x50);
    uint16_t *indices = (uint16_t *)(v + numVertices);
    randomSaved((RwD3DSavedState *)record);
    *(uint32_t *)(record + 0x44) = type;
    *(int32_t *)(record + 0x48) = numVertices;
    *(int32_t *)(record + 0x4c) = numIndices;
    for (int i = 0; i < numVertices; i++) {
        dt_fill(&v[i], sizeof v[i]);
        v[i].sz = dt_u32() % 6 == 0 ? 0.5f : dt_float(0.0f, 1.0f);
    }
    for (int i = 0; i < numIndices; i++) {
        indices[i] = (uint16_t)dt_range(0, numVertices - 1);
    }
    return record;
}

static void splitOrig(uint8_t *record)
{
    const uint16_t cw = cameraCw();
    ((void(BMX_CDECL *)(uint8_t *))(uintptr_t)0x004d9680)(record);
    restoreCw(cw);
}

static void splitImpl(uint8_t *record)
{
    const uint16_t cw = cameraCw();
    rwD3DDeferSplit(record);
    restoreCw(cw);
}

static void setup_split(DtCtx *c)
{
    stateEnv(c);
    rwD3DSortItems = dt_alloc(64 * sizeof(RwD3DSortItem));
    rwD3DSortCount = (uint32_t)dt_range(0, 8);
    DT_ARG_PTR(c, 0, makeRecord((uint32_t)dt_range(1, 7), dt_range(1, kVerts), dt_range(0, 40)));
    rwtEnd();
}

static void setup_compare(DtCtx *c)
{
    stateEnv(c);
    RwD3DSortItem *a = dt_alloc(sizeof *a), *b = dt_alloc(sizeof *b);
    a->key = dt_float(0.0f, 3.0f);
    b->key = dt_u32() % 4 == 0 ? a->key : dt_float(0.0f, 3.0f);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
    rwtEnd();
}

/* Sort items over a few records, with neighbours from the same record and equal keys. */
static void makeItems(int n)
{
    uint8_t *records[3];
    for (int i = 0; i < 3; i++) {
        records[i] = makeRecord(4, kVerts, 30);
    }
    rwD3DSortItems = dt_alloc((uint32_t)n * sizeof(RwD3DSortItem) + 16);
    rwD3DSortCount = (uint32_t)n;
    for (int i = 0; i < n; i++) {
        RwD3DSortItem *item = &rwD3DSortItems[i];
        uint8_t *record = records[dt_range(0, 2)];
        item->record = record;
        item->count = (uint16_t)(dt_u32() % 2 ? 3 : 2);
        item->type = (uint16_t)(item->count == 3 ? 4 : 2);
        item->indices = (uint16_t *)(record + 0x50 + kVerts * 0x20) + dt_range(0, 27);
        item->key = dt_u32() % 3 == 0 ? 1.0f : dt_float(0.0f, 3.0f);
    }
}

static void setup_sort(DtCtx *c)
{
    stateEnv(c);
    rwD3DContext = (uint8_t *)dt_alloc(16);
    *(void **)(rwD3DContext + 8) = (void *)gDevice;
    makeItems(dt_range(0, 12));
    rwD3DMergeCap = (uint32_t)dt_range(0, 12);
    rwD3DMergeBuf = dt_alloc(0x40);
    rwtEnd();
}

static void setup_callback(DtCtx *c)
{
    stateEnv(c);
    uint8_t *record = dt_alloc(0xa4);
    dt_fill(record, 0xa4);
    randomSaved((RwD3DSavedState *)record);
    DT_ARG_PTR(c, 0, record);
    rwtEnd();
}

static void BMX_CDECL stSortDraw(void)
{
    rwtLog(0xe2);
    rwtLog(rwD3DSortCount);
}

static void setup_flush(DtCtx *c)
{
    stateEnv(c);
    dt_stub(0x004d9070, NULL, stSortDraw);
    rwD3DContext = (uint8_t *)dt_alloc(16);
    *(void **)(rwD3DContext + 8) = (void *)gDevice;
    fillRecords(rwD3DDefer, dt_range(0, 5));
    rwD3DDefer->used = dt_u32() % 4 == 0 ? 0 : (uint32_t)dt_range(1, 0x80);
    rwD3DSortItems = dt_u32() % 3 == 0 ? NULL : dt_alloc(0x100);
    rwD3DSortCap = (uint32_t)dt_range(0, 16);
    rwD3DSortNeed = (uint32_t)dt_range(0, 16);
    rwtEnd();
}

/* ---- Im2D entries ---- */

static RwD3DTLVertex *makeVertices(void)
{
    RwD3DTLVertex *v = dt_alloc(kVerts * sizeof *v);
    dt_fill(v, kVerts * sizeof *v);
    return v;
}

static void im2dEnv(DtCtx *c)
{
    stateEnv(c);
    rwD3DContext = (uint8_t *)dt_alloc(16);
    *(void **)(rwD3DContext + 8) = (void *)gDevice;
    if (dt_u32() % 2) {
        rwD3DState.alphaSort = 1;
        rwD3DState.zTest = 1;
    }
    rwD3DSortNeed = (uint32_t)dt_range(0, 9);
    rwD3DRemapCap = dt_range(0, kVerts + 2);
    rwD3DRemap = dt_u32() % 4 == 0 ? NULL : dt_alloc(2 * (kVerts + 4));
    rwD3DRemapUsed = dt_u32() % 4 == 0 ? NULL : dt_alloc(2 * (kVerts + 4));
    if (rwD3DRemap == NULL || rwD3DRemapUsed == NULL) {
        rwD3DRemapCap = 0;
    }
    gRwtMallocFailAt = dt_u32() % 5 == 0 ? dt_range(1, 2) : 0;
}

static void setup_line(DtCtx *c)
{
    im2dEnv(c);
    DT_ARG_PTR(c, 0, makeVertices());
    DT_ARG_INT(c, 1, kVerts);
    DT_ARG_INT(c, 2, dt_range(0, kVerts - 1));
    DT_ARG_INT(c, 3, dt_range(0, kVerts - 1));
    DT_ARG_INT(c, 4, dt_range(0, kVerts - 1));
    rwtEnd();
}

static void setup_primitive(DtCtx *c)
{
    im2dEnv(c);
    DT_ARG_INT(c, 0, dt_range(0, 6));
    DT_ARG_PTR(c, 1, makeVertices());
    DT_ARG_INT(c, 2, dt_range(0, kVerts));
    rwtEnd();
}

static void setup_indexed(DtCtx *c)
{
    im2dEnv(c);
    const int n = dt_range(0, 30);
    uint16_t *indices = dt_alloc(2 * (uint32_t)n + 2);
    const int used = dt_range(1, kVerts);
    for (int i = 0; i < n; i++) {
        indices[i] = (uint16_t)dt_range(0, used - 1);
    }
    DT_ARG_INT(c, 0, dt_range(0, 6));
    DT_ARG_PTR(c, 1, makeVertices());
    DT_ARG_INT(c, 2, used);
    DT_ARG_PTR(c, 3, indices);
    DT_ARG_INT(c, 4, n);
    rwtEnd();
}

DT_CASES(rwd3dim2d_cases) = {
    {"rwD3DSetRenderState", (uint32_t)(uintptr_t)setOrig, setImpl, 2, DT_RET_INT, setup_set, 3000, 0},
    {"rwD3DRenderStateSet", 0x004d7320, rwD3DRenderStateSet, 2, DT_RET_INT, setup_set, 500, 0},
    {"rwD3DRenderStateGet", 0x004d7c10, rwD3DRenderStateGet, 2, DT_RET_INT, setup_get, 800, 0},
    {"rwD3DRasterHandle", 0x004ef3d0, rwD3DRasterHandle, 1, DT_RET_INT, setup_handle, 30, 0},
    {"rwD3DSetTextureRaster", (uint32_t)(uintptr_t)texRasterOrig, texRasterImpl, 1, DT_RET_INT, setup_raster,
     600, 0},
    {"rwD3DSetSrcBlend", (uint32_t)(uintptr_t)srcBlendOrig, srcBlendImpl, 1, DT_RET_INT, setup_value, 400, 0},
    {"rwD3DSetDestBlend", (uint32_t)(uintptr_t)destBlendOrig, destBlendImpl, 1, DT_RET_INT, setup_value, 400,
     0},
    {"rwD3DFogTableSelect", (uint32_t)(uintptr_t)fogSelectOrig, fogSelectImpl, 1, DT_RET_INT, setup_value,
     100, 0},
    {"rwD3DSetFogType", (uint32_t)(uintptr_t)fogTypeOrig, fogTypeImpl, 1, DT_RET_INT, setup_value, 100, 0},
    {"rwD3DSetFogTable", (uint32_t)(uintptr_t)fogTableOrig, fogTableImpl, 1, DT_RET_INT, setup_table, 50, 0},
    {"rwD3DDeferCreate", 0x004ee700, rwD3DDeferCreate, 0, DT_RET_INT, setup_create, 30, 0},
    {"rwD3DDeferAlloc", 0x004ee7e0, rwD3DDeferAlloc, 3, DT_RET_INT, setup_alloc, 400, 0},
    {"rwD3DDeferRun", 0x004ee940, rwD3DDeferRun, 2, DT_RET_INT, setup_run, 100, 0},
    {"rwD3DDeferReset", 0x004ee9b0, rwD3DDeferReset, 1, DT_RET_INT, setup_reset, 300, 0},
    {"rwD3DIm2DOpen", 0x004dafd0, rwD3DIm2DOpen, 0, DT_RET_INT, setup_open, 50, 0},
    {"rwD3DSaveState", (uint32_t)(uintptr_t)saveOrig, saveImpl, 1, DT_RET_VOID, setup_save, 30, 0},
    {"rwD3DRestoreState", (uint32_t)(uintptr_t)restoreOrig, restoreImpl, 1, DT_RET_VOID, setup_restore, 600,
     0},
    {"rwD3DSortCompare", 0x004d9050, rwD3DSortCompare, 2, DT_RET_INT, setup_compare, 100, 0},
    {"rwD3DSortDraw", 0x004d9070, rwD3DSortDraw, 0, DT_RET_VOID, setup_sort, 600, 0},
    {"rwD3DDeferSplit", (uint32_t)(uintptr_t)splitOrig, splitImpl, 1, DT_RET_VOID, setup_split, 600, 0},
    {"rwD3DDeferLine", 0x004db740, rwD3DDeferLine, 1, DT_RET_VOID, setup_callback, 100, 0},
    {"rwD3DDeferTriangle", 0x004db7a0, rwD3DDeferTriangle, 1, DT_RET_VOID, setup_callback, 100, 0},
    {"rwD3DDeferFlush", 0x004d4990, rwD3DDeferFlush, 0, DT_RET_VOID, setup_flush, 400, 0},
    {"rwD3DIm2DLine", 0x004d8ea0, rwD3DIm2DLine, 4, DT_RET_INT, setup_line, 400, 0},
    {"rwD3DIm2DTriangle", 0x004d8c80, rwD3DIm2DTriangle, 5, DT_RET_INT, setup_line, 400, 0},
    {"rwD3DIm2DPrimitive", 0x004d9920, rwD3DIm2DPrimitive, 3, DT_RET_INT, setup_primitive, 600, 0},
    {"rwD3DIm2DIndexedPrimitive", 0x004d9b00, rwD3DIm2DIndexedPrimitive, 5, DT_RET_INT, setup_indexed, 800,
     0},
};
