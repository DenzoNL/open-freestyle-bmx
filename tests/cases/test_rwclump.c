/*
 * Tests for rw/clump.c, part 1: clumps, atomics and the frame clone helpers. The engine instance is the
 * fixture of dt_rwclump.h; plugin registries, frame notifications and the geometry reference counting are
 * logging stubs. The register-argument helpers (RpAtomicClone, RpAtomicDestroy, RpAtomicSetFrame,
 * RpClumpAddAtomic, RpAtomicStreamRead, rwFrameCloneRecursive, rwFrameDestroyRecursive) are reached through
 * their standard-ABI callers.
 */
#include "dt_rwclump.h"

static void clRandomFails(void)
{
    for (int i = 0; i < 8; i++) {
        gClFailAt[i] = dt_u32() % 6 == 0 ? dt_range(1, 3) : 0;
        gClFailCalls[i] = 0;
    }
}

static void clGeometryStubs(void)
{
    dt_stub(0x004b5ab0, NULL, (void *)stGeometryDestroy);
    dt_stub(0x004b5a30, NULL, (void *)stGeometryAddRef);
}

/* ---- atomics in clumps ------------------------------------------------------------------------------ */

static void setup_count_atomic(DtCtx *c)
{
    clEnv(c);
    int32_t *count = dt_alloc(4);
    *count = dt_range(-3, 100);
    DT_ARG_PTR(c, 0, clAtomic(NULL, 0));
    DT_ARG_PTR(c, 1, count);
    rwtEnd();
}

static void setup_num_atomics(DtCtx *c)
{
    clEnv(c);
    DT_ARG_PTR(c, 0, clClump(dt_range(0, 5), NULL));
    rwtEnd();
}

static RpAtomic *BMX_CDECL clRenderCb(RpAtomic *atomic)
{
    rwtLog(0x7e4d3);
    rwtLogPtr(atomic);
    return atomic;
}

static void setup_render(DtCtx *c)
{
    clEnv(c);
    dt_stub(0x004d2b10, NULL, (void *)stGetLTM);
    RpClump *clump = clClump(dt_range(0, 4), NULL);
    for (RwLLLink *l = clump->atomicList.next; l != &clump->atomicList; l = l->next) {
        RpAtomic *a = (RpAtomic *)((uint8_t *)l - offsetof(RpAtomic, inClumpLink));
        a->renderCallBack = clRenderCb;
        a->object.object.parent = clFrame();
    }
    DT_ARG_PTR(c, 0, clump);
    rwtEnd();
}

static void setup_destroy_atomic(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    clFrameStubs();
    clGeometryStubs();
    dt_stub(0x004dc380, NULL, (void *)stFreeResEntry);
    DT_ARG_PTR(c, 0, clAtomic(dt_u32() % 3 == 0 ? NULL : clFrame(), dt_u32() % 2));
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

/* A source atomic with a frame in a hierarchy (or without one: the clone callback reports an error). */
static void setup_clone_atomic(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    clFrameStubs();
    clGeometryStubs();
    gClAllocFailAt = dt_u32() % 6 == 0 ? dt_range(1, 2) : 0;
    RwFrame *root = clFrame();
    RwFrame *child = clFrame();
    clAddChild(root, child);
    RpAtomic *source = clAtomic(dt_u32() % 5 == 0 ? NULL : child, dt_u32() % 2);
    source->object.object.privateFlags = (uint8_t)dt_u32();
    source->boundingSphere.radius = dt_float_edgy(0.0f, 10.0f);
    source->renderCallBack = dt_u32() % 2 ? clRenderCb : NULL;
    RwClumpCloneData *data = dt_alloc(sizeof *data);
    data->source = clClump(0, root);
    data->clone = clClump(dt_range(0, 2), root);
    data->ok = 1;
    DT_ARG_PTR(c, 0, source);
    DT_ARG_PTR(c, 1, data);
    rwtEnd();
}

static void setup_destroy_clump(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    clFrameStubs();
    clGeometryStubs();
    dt_stub(0x004dc380, NULL, (void *)stFreeResEntry);
    dt_stub(0x004d29c0, NULL, (void *)stReleaseFrame);
    DT_ARG_PTR(c, 0, clClump(dt_range(0, 3), dt_u32() % 3 == 0 ? NULL : clFrame()));
    rwtEnd();
}

/* A clump on a frame tree; the clone may fail to allocate at any step. */
static void setup_clone_clump(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    clFrameStubs();
    clGeometryStubs();
    gClAllocFailAt = dt_u32() % 4 == 0 ? dt_range(1, 8) : 0;
    RwFrame *root = clFrameTree(dt_range(1, 4));
    RpClump *clump = clClump(0, root);
    RwFrame *frame = root;
    const int n = dt_range(0, 3);
    for (int i = 0; i < n; i++) {
        RpAtomic *a = clAtomic(frame, dt_u32() % 2);
        a->clump = clump;
        clInsertLink(&clump->atomicList, &a->inClumpLink);
        if (frame->child != NULL && dt_u32() % 2) {
            frame = frame->child;
        }
    }
    clump->callback = dt_alloc(4);
    DT_ARG_PTR(c, 0, clump);
    rwtEnd();
}

/* ---- frames ----------------------------------------------------------------------------------------- */

static void setup_frame_clone(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    gClAllocFailAt = dt_u32() % 3 == 0 ? dt_range(1, 5) : 0;
    DT_ARG_PTR(c, 0, clFrameTree(dt_range(1, 5)));
    rwtEnd();
}

static void setup_reset_root(DtCtx *c)
{
    clEnv(c);
    RwFrame *tree = clFrameTree(dt_range(1, 5));
    RwFrame *frame = tree;
    if (tree->child != NULL && dt_u32() % 2) {
        frame = tree->child;
    }
    DT_ARG_PTR(c, 0, frame);
    rwtEnd();
}

static void setup_add_child(DtCtx *c)
{
    clEnv(c);
    RwFrame *parent = clFrameTree(dt_range(1, 3));
    RwFrame *other = clFrameTree(dt_range(1, 3));
    RwFrame *child = dt_u32() % 2 ? other : (parent->child != NULL ? parent->child : other);
    if (child->object.parent != NULL && dt_u32() % 2 == 0) {
        child = other;
    }
    DT_ARG_PTR(c, 0, parent);
    DT_ARG_PTR(c, 1, child);
    rwtEnd();
}

/* ---- reading a clump -------------------------------------------------------------------------------- */

static RwFrame *gClFrames[4];

static RwFrameList *BMX_CDECL stFrameListRead(void *stream, RwFrameList *list)
{
    rwtLog(0x4e9360);
    rwtLogPtr(stream);
    if (clFails(kClStubOther)) {
        return NULL;
    }
    list->frames = gClFrames;
    list->numFrames = 4;
    return list;
}

static RwFrameList *BMX_CDECL stFrameListDeinit(RwFrameList *list)
{
    rwtLog(0x4e96a0);
    rwtLogPtr(list);
    return list;
}

static RpGeometry *gClGeoms[4];
static int gClGeoCalls;

static RpGeometry *BMX_CDECL stGeometryRead(void *stream)
{
    rwtLog(0x4b68e0);
    rwtLogPtr(stream);
    return clFails(kClStubMesh) ? NULL : gClGeoms[gClGeoCalls++ & 3];
}

static void setup_clump_read(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    clFrameStubs();
    clGeometryStubs();
    clStreamScript();
    clRandomFails();
    for (int i = 0; i < 16; i++) {
        gClLengths[i] = 12;
    }
    for (int i = 0; i < 4; i++) {
        gClFrames[i] = clFrame();
        gClGeoms[i] = clGeometry(dt_range(0, 3), dt_range(0, 2), 0, 1);
    }
    gClGeoCalls = 0;
    gClAllocFailAt = dt_u32() % 8 == 0 ? dt_range(1, 3) : 0;
    dt_stub(0x004e9360, NULL, (void *)stFrameListRead);
    dt_stub(0x004e96a0, NULL, (void *)stFrameListDeinit);
    dt_stub(0x004b68e0, NULL, (void *)stGeometryRead);
    dt_stub(0x004dc380, NULL, (void *)stFreeResEntry);
    dt_stub(0x004d29c0, NULL, (void *)stReleaseFrame);
    DT_ARG_PTR(c, 0, dt_alloc(8));
    rwtEnd();
}

DT_CASES(rwclump_cases) = {
    {"rwClumpCountAtomic", 0x004b1430, rwClumpCountAtomic, 2, DT_RET_INT, setup_count_atomic, 30, 0},
    {"RpClumpGetNumAtomics", 0x004b1440, RpClumpGetNumAtomics, 1, DT_RET_INT, setup_num_atomics, 60, 0},
    {"RpClumpRender", 0x004b1940, RpClumpRender, 1, DT_RET_INT, setup_render, 100, 0},
    {"rwClumpDestroyAtomic", 0x004b21c0, rwClumpDestroyAtomic, 2, DT_RET_INT, setup_destroy_atomic, 150, 0},
    {"rwClumpCloneAtomic", 0x004b1e50, rwClumpCloneAtomic, 2, DT_RET_INT, setup_clone_atomic, 400, 0},
    {"RpClumpDestroy", 0x004b21e0, RpClumpDestroy, 1, DT_RET_INT, setup_destroy_clump, 150, 0},
    {"RpClumpClone", 0x004b1ee0, RpClumpClone, 1, DT_RET_INT, setup_clone_clump, 600, 0},
    {"RwFrameCloneHierarchy", 0x004d25e0, RwFrameCloneHierarchy, 1, DT_RET_INT, setup_frame_clone, 300, 0},
    {"rwFrameResetRoot", 0x004d2680, rwFrameResetRoot, 1, DT_RET_VOID, setup_reset_root, 150, 0},
    {"RwFrameAddChild", 0x004d2b50, RwFrameAddChild, 2, DT_RET_INT, setup_add_child, 300, 0},
    {"RpClumpStreamRead", 0x004b2790, RpClumpStreamRead, 1, DT_RET_INT, setup_clump_read, 1200, 0},
};
