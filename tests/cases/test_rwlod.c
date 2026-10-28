/*
 * Tests for the LOD atomic plugin (rw/plugins.c, 0x505b30..0x506280) and the libpng glue callbacks next to
 * it. The engine instance is the fixture of dt_rw.h with room for the plugin's data in an atomic; the
 * geometry functions, the camera and frame queries, the stream functions and the atomic's callbacks are
 * logging stubs, so the order of every call is compared with the original.
 */
#include "dt_rw.h"
#include "rw/rwclump.h"
#include "rw/rwplugins.h"
#include "rw/rwstream.h"

enum {
    kEngineBytes = 0x400,
    kAtomicBytes = 0x90,
    kLodAt = 0x74, /* where an atomic keeps its LOD data pointer */
};

#define REG2(addr, ret, ta, tb) ((ret(__attribute__((regparm(2))) *)(ta, tb))(uintptr_t)(addr))
#define REG1(addr, ret, ta)     ((ret(__attribute__((regparm(1))) *)(ta))(uintptr_t)(addr))

/* ---- stubs -------------------------------------------------------------------------------------------- */

static int32_t gSelectResult;

static int32_t BMX_CDECL cbSelect(RpAtomic *atomic)
{
    rwtLog(0x5e1);
    rwtLogPtr(atomic);
    return gSelectResult;
}

static RpAtomic *BMX_CDECL cbRender(RpAtomic *atomic)
{
    rwtLog(0x4e4d);
    rwtLogPtr(atomic);
    return dt_u32() % 3 == 0 ? NULL : atomic;
}

static void *BMX_CDECL stSetGeometry(RpAtomic *atomic, RpGeometry *geometry, uint32_t flags)
{
    rwtLog(0x4b1ad0);
    rwtLogPtr(atomic);
    rwtLogPtr(geometry);
    rwtLog(flags);
    return atomic;
}

static void *BMX_CDECL stAddRef(RpGeometry *geometry)
{
    rwtLog(0x4b5a30);
    rwtLogPtr(geometry);
    return geometry;
}

static int BMX_CDECL stGeometryDestroy(RpGeometry *geometry)
{
    rwtLog(0x4b5ab0);
    rwtLogPtr(geometry);
    return 1;
}

static RwCamera *gCamera;

static RwCamera *BMX_CDECL stCurrentCamera(void)
{
    rwtLog(0x4de810);
    return gCamera;
}

static void *BMX_CDECL stFrameLtm(RwFrame *frame)
{
    rwtLog(0x4d2b10);
    rwtLogPtr(frame);
    return frame;
}

static void *BMX_CDECL lodEntryAlloc(RwFreeList *freeList)
{
    rwtLog(0x4dbc30);
    rwtLogPtr(freeList);
    gRwtEntryCalls++;
    if (gRwtEntryFailAt != 0 && gRwtEntryCalls == gRwtEntryFailAt) {
        return NULL;
    }
    return dt_alloc(0x40);
}

static RwGlobals *lodEnv(const DtCtx *c)
{
    dt_seed(c);
    RwGlobals *small = rwtEnv();
    RwGlobals *g = dt_alloc(kEngineBytes);
    __builtin_memcpy(g, small, kRwtEngineBytes);
    g->freeListAlloc = lodEntryAlloc;
    RwEngineInstance = g;
    rpLodAtomicOffset = kLodAt;
    rpLodFreeList = (RwFreeList *)dt_alloc(8);
    return g;
}

static void lodStubs(void)
{
    dt_stub(0x004b1ad0, NULL, (void *)stSetGeometry);
    dt_stub(0x004b5a30, NULL, (void *)stAddRef);
    dt_stub(0x004b5ab0, NULL, (void *)stGeometryDestroy);
}

/* ---- fixtures -------------------------------------------------------------------------------------------
 */

static RpGeometry *mkGeometry(void)
{
    return dt_alloc(0x48);
}

static RpLodAtomicData *mkLod(void)
{
    uint32_t *d = dt_alloc(0x40);
    d[0] = dt_u32() % 2;
    d[1] = dt_u32() % 2 ? (uint32_t)(uintptr_t)cbRender : 0;
    d[2] = (uint32_t)(uintptr_t)cbSelect;
    ((float *)d)[3] = dt_float_edgy(-2.0F, 2.0F);
    ((float *)d)[4] = dt_float_edgy(-2.0F, 2.0F);
    d[5] = (uint32_t)(dt_u32() % 5 == 0 ? -1 : dt_range(0, 9));
    for (int i = 0; i < 10; i++) {
        d[6 + i] = dt_u32() % 3 == 0 ? 0 : (uint32_t)(uintptr_t)mkGeometry();
    }
    return (RpLodAtomicData *)d;
}

static RpAtomic *mkAtomic(RpLodAtomicData *lod)
{
    RpAtomic *a = dt_alloc(kAtomicBytes);
    a->object.object.type = 1;
    a->stamp = (uint8_t)dt_u32();
    a->renderCallBack = dt_u32() % 2 ? cbRender : (RpAtomicRenderCallBack)(uintptr_t)(dt_u32() & ~3U);
    a->geometry = dt_u32() % 3 == 0 ? NULL : mkGeometry();
    if (lod != NULL && dt_u32() % 2 == 0) {
        const uint32_t *d = (const uint32_t *)lod;
        a->geometry = (RpGeometry *)(uintptr_t)d[6 + dt_u32() % 10];
    }
    *(RpLodAtomicData **)((uint8_t *)a + kLodAt) = lod;
    return a;
}

static void fillFrame(void *frame)
{
    float *f = frame;
    for (int i = 0; i < 0x80 / 4; i++) {
        f[i] = dt_float_edgy(-60.0F, 60.0F);
    }
}

static void setup_select(DtCtx *c)
{
    lodEnv(c);
    dt_stub(0x004de810, NULL, (void *)stCurrentCamera);
    dt_stub(0x004d2b10, NULL, (void *)stFrameLtm);
    RpLodAtomicData *lod = dt_u32() % 8 == 0 ? NULL : mkLod();
    RpAtomic *atomic = mkAtomic(lod);
    atomic->object.object.parent = dt_alloc(0xa8);
    fillFrame(atomic->object.object.parent);
    gCamera = dt_u32() % 8 == 0 ? NULL : dt_alloc(0xa0);
    if (gCamera != NULL) {
        gCamera->object.object.parent = dt_alloc(0xa8);
        fillFrame(gCamera->object.object.parent);
    }
    if (dt_u32() % 10 == 0) {
        rpLodAtomicOffset = -4;
    }
    DT_ARG_PTR(c, 0, dt_u32() % 10 == 0 ? NULL : atomic);
    rwtEnd();
}

static RpAtomic *applyOrig(RpAtomic *atomic, RpLodAtomicData *lod)
{
    return REG2(0x00505c60, RpAtomic *, RpAtomic *, RpLodAtomicData *)(atomic, lod);
}

static RpAtomic *applyImpl(RpAtomic *atomic, RpLodAtomicData *lod)
{
    return rpLodAtomicApply(atomic, lod);
}

static void setup_apply(DtCtx *c)
{
    lodEnv(c);
    lodStubs();
    RpLodAtomicData *lod = dt_u32() % 8 == 0 ? NULL : mkLod();
    gSelectResult = dt_range(-3, 14);
    DT_ARG_PTR(c, 0, mkAtomic(lod));
    DT_ARG_PTR(c, 1, lod);
    rwtEnd();
}

static void setup_render(DtCtx *c)
{
    lodEnv(c);
    lodStubs();
    RpLodAtomicData *lod = dt_u32() % 8 == 0 ? NULL : mkLod();
    if (lod != NULL) {
        lod->savedRender = cbRender;
    }
    gSelectResult = dt_range(-3, 14);
    if (dt_u32() % 10 == 0) {
        rpLodAtomicOffset = -4;
    }
    DT_ARG_PTR(c, 0, dt_u32() % 10 == 0 ? NULL : mkAtomic(lod));
    rwtEnd();
}

static void hookArgs(DtCtx *c)
{
    lodEnv(c);
    RpLodAtomicData *lod = dt_u32() % 6 == 0 ? NULL : mkLod();
    if (lod != NULL) {
        lod->hooked = (int32_t)(dt_u32() % 3 == 0 ? dt_u32() % 4 : dt_u32() % 2);
    }
    DT_ARG_PTR(c, 0, mkAtomic(lod));
    DT_ARG_PTR(c, 1, lod);
    rwtEnd();
}

static int hookOrig(RpAtomic *atomic, RpLodAtomicData *lod)
{
    REG2(0x00505d10, void, RpAtomic *, RpLodAtomicData *)(atomic, lod);
    return 0;
}

static int hookImpl(RpAtomic *atomic, RpLodAtomicData *lod)
{
    rpLodAtomicHook(atomic, lod);
    return 0;
}

static int unhookOrig(RpAtomic *atomic, RpLodAtomicData *lod)
{
    REG2(0x00505d40, void, RpAtomic *, RpLodAtomicData *)(atomic, lod);
    return 0;
}

static int unhookImpl(RpAtomic *atomic, RpLodAtomicData *lod)
{
    rpLodAtomicUnhook(atomic, lod);
    return 0;
}

static void setup_data_init(DtCtx *c)
{
    lodEnv(c);
    DT_ARG_PTR(c, 0, dt_u32() % 6 == 0 ? NULL : mkLod());
    rwtEnd();
}

static void setup_data_create(DtCtx *c)
{
    lodEnv(c);
    gRwtEntryFailAt = dt_u32() % 3 == 0 ? 1 : 0;
    rwtEnd();
}

static RpLodAtomicData *copyOrig(RpLodAtomicData *dst, RpLodAtomicData *src)
{
    return REG2(0x00505db0, RpLodAtomicData *, RpLodAtomicData *, RpLodAtomicData *)(dst, src);
}

static RpLodAtomicData *copyImpl(RpLodAtomicData *dst, RpLodAtomicData *src)
{
    return rpLodAtomicDataCopy(dst, src);
}

static void setup_data_copy(DtCtx *c)
{
    lodEnv(c);
    lodStubs();
    DT_ARG_PTR(c, 0, dt_u32() % 6 == 0 ? NULL : mkLod());
    DT_ARG_PTR(c, 1, dt_u32() % 6 == 0 ? NULL : mkLod());
    rwtEnd();
}

static int destroyOrig(RpLodAtomicData *lod)
{
    REG1(0x00505e10, void, RpLodAtomicData *)(lod);
    return 0;
}

static int destroyImpl(RpLodAtomicData *lod)
{
    rpLodAtomicDataDestroy(lod);
    return 0;
}

static void setup_data_destroy(DtCtx *c)
{
    lodEnv(c);
    lodStubs();
    DT_ARG_PTR(c, 0, dt_u32() % 6 == 0 ? NULL : mkLod());
    rwtEnd();
}

static void setup_open(DtCtx *c)
{
    lodEnv(c);
    rpLodFreeList = NULL;
    gRwtMallocFailAt = dt_u32() % 3 == 0 ? dt_range(1, 2) : 0;
    rwFreeListHead.next = rwFreeListHead.prev = &rwFreeListHead;
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_INT(c, 1, dt_u32());
    rwtEnd();
}

#define ORIG_FREELIST_CREATE DT_ORIG(0x004dba90, RwFreeList *(*)(int32_t, int32_t, int32_t))

static void setup_close(DtCtx *c)
{
    lodEnv(c);
    rwFreeListHead.next = rwFreeListHead.prev = &rwFreeListHead;
    rpLodFreeList = dt_u32() % 3 == 0 ? NULL : ORIG_FREELIST_CREATE(0x40, 4, 0);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

static void setup_ctor(DtCtx *c)
{
    lodEnv(c);
    DT_ARG_PTR(c, 0, mkAtomic(mkLod()));
    DT_ARG_INT(c, 1, kLodAt);
    rwtEnd();
}

static void setup_dtor(DtCtx *c)
{
    lodEnv(c);
    lodStubs();
    RpLodAtomicData *lod = dt_u32() % 5 == 0 ? NULL : mkLod();
    DT_ARG_PTR(c, 0, mkAtomic(lod));
    DT_ARG_INT(c, 1, kLodAt);
    rwtEnd();
}

static void setup_copy(DtCtx *c)
{
    lodEnv(c);
    lodStubs();
    gRwtEntryFailAt = dt_u32() % 5 == 0 ? 1 : 0;
    RpLodAtomicData *src = dt_u32() % 4 == 0 ? NULL : mkLod();
    RpLodAtomicData *dst = dt_u32() % 2 ? NULL : mkLod();
    DT_ARG_PTR(c, 0, mkAtomic(dst));
    DT_ARG_PTR(c, 1, mkAtomic(src));
    DT_ARG_INT(c, 2, kLodAt);
    DT_ARG_INT(c, 3, dt_u32());
    rwtEnd();
}

/* ---- stream ----------------------------------------------------------------------------------------------
 */

static uint32_t gVersions[12]; /* the versions the successive RwStreamFindChunk calls report (0: fail) */
static int gFindCalls;
static uint32_t gScriptLod[16];

static int BMX_CDECL stFindChunk(RwStream *stream, uint32_t type, uint32_t *length, uint32_t *version)
{
    rwtLog(0x4e2ec0);
    rwtLogPtr(stream);
    rwtLog(type);
    rwtLog(length != NULL);
    rwtLog(version != NULL);
    const uint32_t v = gFindCalls < 12 ? gVersions[gFindCalls] : 0;
    gFindCalls++;
    if (v == 0) {
        return 0;
    }
    if (length != NULL) {
        *length = 0x40;
    }
    if (version != NULL) {
        *version = v;
    }
    return 1;
}

static uint32_t BMX_CDECL stStreamRead(RwStream *stream, void *buffer, uint32_t length)
{
    rwtLog(0x4e2800);
    rwtLogPtr(stream);
    rwtLogPtr(buffer);
    rwtLog(length);
    __builtin_memcpy(buffer, gScriptLod, 0x40);
    return length;
}

static void *BMX_CDECL stNative32(void *mem, uint32_t size)
{
    rwtLog(0x4e2dc0);
    rwtLogPtr(mem);
    rwtLog(size);
    return mem;
}

static void *BMX_CDECL stLittleEndian32(void *mem, uint32_t size)
{
    rwtLog(0x4e2da0);
    rwtLog(size);
    rwtLogWords(mem, (int)(size / 4));
    return mem;
}

static RpGeometry *BMX_CDECL stGeometryRead(RwStream *stream)
{
    rwtLog(0x4b68e0);
    rwtLogPtr(stream);
    return dt_u32() % 5 == 0 ? NULL : mkGeometry();
}

static RwStream *BMX_CDECL stStreamWrite(RwStream *stream, const void *buffer, uint32_t length)
{
    rwtLog(0x4e29d0);
    rwtLogPtr(stream);
    rwtLogPtr(buffer);
    rwtLog(length);
    rwtLogWords(buffer, (int)(length / 4 > 16 ? 16 : length / 4));
    return stream;
}

static RwStream *BMX_CDECL stGeometryWrite(RpGeometry *geometry, RwStream *stream)
{
    rwtLog(0x4b6350);
    rwtLogPtr(geometry);
    rwtLogPtr(stream);
    return stream;
}

static int BMX_CDECL stGeometrySize(RpGeometry *geometry)
{
    rwtLog(0x4b6230);
    rwtLogPtr(geometry);
    return dt_range(0, 5000);
}

static void streamStubs(void)
{
    dt_stub(0x004e2ec0, NULL, (void *)stFindChunk);
    dt_stub(0x004e2800, NULL, (void *)stStreamRead);
    dt_stub(0x004e2dc0, NULL, (void *)stNative32);
    dt_stub(0x004e2da0, NULL, (void *)stLittleEndian32);
    dt_stub(0x004b68e0, NULL, (void *)stGeometryRead);
    dt_stub(0x004e29d0, NULL, (void *)stStreamWrite);
    dt_stub(0x004e2fd0, NULL, (void *)rwtStreamWriteHeader);
    dt_stub(0x004b6350, NULL, (void *)stGeometryWrite);
    dt_stub(0x004b6230, NULL, (void *)stGeometrySize);
    lodStubs();
    gRwtHeaderFailAt = gRwtHeaderCalls = 0;
    gFindCalls = 0;
}

static void setup_read(DtCtx *c)
{
    lodEnv(c);
    streamStubs();
    for (int i = 0; i < 12; i++) {
        const uint32_t pick = dt_u32() % 8;
        gVersions[i] = pick == 0 ? 0 : (pick == 1 ? dt_u32() % 0x400 : 0x301 + dt_u32() % 3);
    }
    if (dt_u32() % 2) {
        gVersions[0] = 0x301 + dt_u32() % 3;
    }
    for (int i = 0; i < 16; i++) {
        gScriptLod[i] = i >= 6 ? (dt_u32() % 2 ? (uint32_t)(uintptr_t)mkGeometry() : 0) : dt_u32();
    }
    gScriptLod[5] = (uint32_t)(dt_u32() % 4 == 0 ? -1 : dt_range(0, 9));
    gRwtEntryFailAt = 0;
    RpAtomic *atomic = mkAtomic(NULL);
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, atomic);
    DT_ARG_INT(c, 3, kLodAt);
    DT_ARG_INT(c, 4, dt_u32());
    rwtEnd();
}

static void setup_write(DtCtx *c)
{
    lodEnv(c);
    streamStubs();
    RpLodAtomicData *lod = dt_u32() % 5 == 0 ? NULL : mkLod();
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, mkAtomic(lod));
    DT_ARG_INT(c, 3, kLodAt);
    DT_ARG_INT(c, 4, dt_u32());
    rwtEnd();
}

static void setup_size(DtCtx *c)
{
    lodEnv(c);
    streamStubs();
    RpLodAtomicData *lod = dt_u32() % 5 == 0 ? NULL : mkLod();
    DT_ARG_PTR(c, 0, mkAtomic(lod));
    DT_ARG_INT(c, 1, kLodAt);
    DT_ARG_INT(c, 2, dt_u32());
    rwtEnd();
}

/* ---- libpng callbacks -------------------------------------------------------------------------------------
 */

static RwMemoryFunctions gMem;

static const RwMemoryFunctions *BMX_CDECL stMemInterface(void)
{
    rwtLog(0x4dba80);
    return &gMem;
}

static uint32_t BMX_CDECL stPngError(void *png, const char *message)
{
    rwtLog(0x507050);
    rwtLogPtr(png);
    rwtLog((uint32_t)(uintptr_t)message);
    return dt_u32();
}

static void pngAlloc(DtCtx *c)
{
    lodEnv(c);
    gMem.rwmalloc = rwtMalloc;
    gMem.rwfree = rwtFree;
    dt_stub(0x004dba80, NULL, (void *)stMemInterface);
    DT_ARG_PTR(c, 0, dt_alloc(0x60));
}

static void setup_png_malloc(DtCtx *c)
{
    pngAlloc(c);
    DT_ARG_INT(c, 1, dt_range(1, 400));
    rwtEnd();
}

static void setup_png_free(DtCtx *c)
{
    pngAlloc(c);
    DT_ARG_PTR(c, 1, dt_alloc(16));
    rwtEnd();
}

static void setup_png_read(DtCtx *c)
{
    lodEnv(c);
    dt_stub(0x004e2800, NULL, (void *)stStreamRead);
    dt_stub(0x00507050, NULL, (void *)stPngError);
    uint8_t *png = dt_alloc(0x60);
    *(void **)(png + 0x54) = dt_u32() % 3 == 0 ? NULL : dt_alloc(0x20);
    DT_ARG_PTR(c, 0, png);
    DT_ARG_PTR(c, 1, dt_alloc(0x40));
    DT_ARG_INT(c, 2, (uint32_t)dt_range(0, 64));
    rwtEnd();
}

/* ---- the geometry chunk size (rw/clump.c, 0x4b6230) ---------------------------------------------------- */

static int BMX_CDECL stMaterialListSize(const RpMaterialList *list)
{
    rwtLog(0x4c50e0);
    rwtLogPtr(list);
    return dt_range(0, 900);
}

static int BMX_CDECL stRegistrySize(const RwPluginRegistry *registry, const void *object)
{
    rwtLog(0x4e8d60);
    rwtLogPtr(registry);
    rwtLogPtr(object);
    return dt_range(0, 300);
}

static void setup_geometry_size(DtCtx *c)
{
    lodEnv(c);
    dt_stub(0x004c50e0, NULL, (void *)stMaterialListSize);
    dt_stub(0x004e8d60, NULL, (void *)stRegistrySize);
    rpGeometryModuleCount = dt_u32() % 8 == 0 ? 0 : 1;
    RpGeometry *g = dt_alloc(sizeof(RpGeometry));
    g->flags = (uint8_t)dt_u32();
    g->numVertices = dt_u32() % 5 == 0 ? 0 : dt_range(0, 50);
    g->numTriangles = dt_range(0, 60);
    g->numMorphTargets = dt_range(0, 4);
    g->morphTarget = dt_alloc(sizeof(RpMorphTarget) * 4);
    for (int i = 0; i < 4; i++) {
        g->morphTarget[i].verts = dt_u32() % 2 ? NULL : dt_alloc(16);
        g->morphTarget[i].normals = dt_u32() % 2 ? NULL : dt_alloc(16);
    }
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : g);
    rwtEnd();
}

DT_CASES(rwlod_cases) = {
    {"RpGeometryStreamGetSize", 0x004b6230, RpGeometryStreamGetSize, 1, DT_RET_INT, setup_geometry_size, 300,
     0},
    {"rpLodAtomicSelectLevel", 0x00505b30, rpLodAtomicSelectLevel, 1, DT_RET_INT, setup_select, 600, 0},
    {"rpLodAtomicApply", (uint32_t)(uintptr_t)applyOrig, applyImpl, 2, DT_RET_INT, setup_apply, 300, 0},
    {"rpLodAtomicRender", 0x00505cd0, rpLodAtomicRender, 1, DT_RET_INT, setup_render, 300, 0},
    {"rpLodAtomicHook", (uint32_t)(uintptr_t)hookOrig, hookImpl, 2, DT_RET_INT, hookArgs, 100, 0},
    {"rpLodAtomicUnhook", (uint32_t)(uintptr_t)unhookOrig, unhookImpl, 2, DT_RET_INT, hookArgs, 100, 0},
    {"rpLodAtomicDataInit", 0x00506780, rpLodAtomicDataInit, 1, DT_RET_INT, setup_data_init, 60, 0},
    {"rpLodAtomicDataCreate", 0x00505d70, rpLodAtomicDataCreate, 0, DT_RET_INT, setup_data_create, 60, 0},
    {"rpLodAtomicDataCopy", (uint32_t)(uintptr_t)copyOrig, copyImpl, 2, DT_RET_INT, setup_data_copy, 100, 0},
    {"rpLodAtomicDataDestroy", (uint32_t)(uintptr_t)destroyOrig, destroyImpl, 1, DT_RET_INT,
     setup_data_destroy, 100, 0},
    {"LODAtomicCallBackOpen", 0x00506240, LODAtomicCallBackOpen, 2, DT_RET_INT, setup_open, 60, 0},
    {"LODAtomicCallBackClose", 0x00506280, LODAtomicCallBackClose, 1, DT_RET_INT, setup_close, 60, 0},
    {"LODAtomicCallBackConstructor", 0x00506190, LODAtomicCallBackConstructor, 2, DT_RET_INT, setup_ctor, 20,
     0},
    {"LODAtomicCallBackDestructor", 0x00506200, LODAtomicCallBackDestructor, 2, DT_RET_INT, setup_dtor, 100,
     0},
    {"LODAtomicCallBackCopy", 0x005061b0, LODAtomicCallBackCopy, 4, DT_RET_INT, setup_copy, 200, 0},
    {"LODAtomicCallBackReadChunk", 0x00505e60, LODAtomicCallBackReadChunk, 5, DT_RET_INT, setup_read, 600, 0},
    {"LODAtomicCallBackWriteChunk", 0x00506030, LODAtomicCallBackWriteChunk, 5, DT_RET_INT, setup_write, 200,
     0},
    {"LODAtomicCallBackGetChunkSize", 0x00506140, LODAtomicCallBackGetChunkSize, 3, DT_RET_INT, setup_size,
     200, 0},
    {"rwPngMalloc", 0x00506820, rwPngMalloc, 2, DT_RET_INT, setup_png_malloc, 30, 0},
    {"rwPngFree", 0x00506840, rwPngFree, 2, DT_RET_VOID, setup_png_free, 30, 0},
    {"rwPngRead", 0x00506860, rwPngRead, 3, DT_RET_INT, setup_png_read, 60, 0},
};
