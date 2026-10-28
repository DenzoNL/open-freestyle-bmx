/*
 * Tests for the RenderWare functions that are only reached through function pointers: the free list entry
 * points, the open/close routines of the engine's modules and the small callbacks the registries keep
 * (rw/core.c, engine.c, math.c, texture.c, camera.c, stream.c, im.c, clump.c, light.c, world.c, plugins.c).
 * The engine instance is the fixture of dt_rw.h, copied into a larger block so module data fits behind it.
 * Register-argument helpers (rwV3dSetTransformFuncs, rwFrameListFill, rpAtomicUpdateBoundingSphere) are
 * reached through cdecl wrappers.
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwcamera.h"
#include "rw/rwclump.h"
#include "rw/rwim.h"
#include "rw/rwlight.h"
#include "rw/rwmath.h"
#include "rw/rwplugins.h"
#include "rw/rwstream.h"
#include "rw/rwtexture.h"
#include "rw/rwworld.h"

#define ORIG_FREELIST_CREATE DT_ORIG(0x004dba90, RwFreeList *(*)(int32_t, int32_t, int32_t))
#define ORIG_FREELIST_ALLOC  DT_ORIG(0x004dbc30, void *(*)(RwFreeList *))

/* ---- fixtures --------------------------------------------------------------------------------------- */

enum { kEngineBytes = 0x800 };

/* The engine fixture in a block with room for module data; every list/instance starts cleared. */
static RwGlobals *modEnv(DtCtx *c)
{
    dt_seed(c);
    RwGlobals *small = rwtEnv();
    RwGlobals *g = dt_alloc(kEngineBytes);
    __builtin_memcpy(g, small, kRwtEngineBytes);
    RwEngineInstance = g;
    rwFreeListHead.next = rwFreeListHead.prev = &rwFreeListHead;
    return g;
}

/* A module data offset behind the fixed part of the engine instance (room for 0x230 bytes of data). */
static int32_t modOffset(void)
{
    return 0x190 + 4 * dt_range(0, 64);
}

/* Fail the n-th allocation / free list entry of the call under test (call at the end of a setup). */
static void injectFailures(void)
{
    gRwtMallocFailAt = dt_u32() % 3 == 0 ? dt_range(1, 6) : 0;
    gRwtEntryFailAt = dt_u32() % 4 == 0 ? 1 : 0;
}

static uint8_t *at(RwGlobals *g, int32_t offset)
{
    return (uint8_t *)g + offset;
}

static void setup_open(DtCtx *c)
{
    modEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_INT(c, 1, modOffset());
    injectFailures();
    rwtEnd();
}

static void setup_engine_only(DtCtx *c)
{
    modEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

/* ---- free list entry points ------------------------------------------------------------------------- */

static void setup_alloc_real(DtCtx *c)
{
    modEnv(c);
    static const int32_t alignments[] = {0, 1, 4, 8, 16};
    RwFreeList *fl = ORIG_FREELIST_CREATE(dt_range(1, 100), dt_range(1, 12), alignments[dt_u32() % 5]);
    for (int i = dt_range(0, 30); i > 0; i--) {
        ORIG_FREELIST_ALLOC(fl);
    }
    DT_ARG_PTR(c, 0, fl);
    injectFailures();
    rwtEnd();
}

static void setup_free_real(DtCtx *c)
{
    modEnv(c);
    RwFreeList *fl = ORIG_FREELIST_CREATE(dt_range(1, 100), dt_range(1, 12), 4);
    for (int i = dt_range(1, 20); i > 0; i--) {
        ORIG_FREELIST_ALLOC(fl);
    }
    DT_ARG_PTR(c, 0, fl);
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

static void setup_alloc_plain(DtCtx *c)
{
    modEnv(c);
    RwFreeList *fl = dt_alloc(sizeof(RwFreeList));
    fl->entrySize = dt_range(0, 400);
    DT_ARG_PTR(c, 0, fl);
    gRwtMallocFailAt = dt_u32() % 4 == 0 ? 1 : 0;
    rwtEnd();
}

static void setup_free_plain(DtCtx *c)
{
    modEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(RwFreeList)));
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

/* ---- engine modules --------------------------------------------------------------------------------- */

static void setup_resources_init(DtCtx *c)
{
    dt_seed(c);
    uint32_t *r = dt_alloc(0x40);
    dt_fill(r, 0x24);
    DT_ARG_PTR(c, 0, r);
}

static void BMX_CDECL destroy_notify(RwResEntry *entry)
{
    rwtLog(0xde57);
    rwtLogPtr(entry);
    rwtLog((uint32_t)entry->size);
}

static void link_list(RwLLLink *head, RwResEntry **entries, int n)
{
    RwLLLink *prev = head;
    for (int i = 0; i < n; i++) {
        entries[i] = dt_alloc(sizeof(RwResEntry));
        entries[i]->size = dt_range(1, 5000);
        entries[i]->ownerRef = (RwResEntry **)dt_alloc(8);
        *entries[i]->ownerRef = entries[i];
        entries[i]->destroyNotify = dt_u32() % 2 ? destroy_notify : NULL;
        entries[i]->link.prev = prev;
        prev->next = &entries[i]->link;
        prev = &entries[i]->link;
    }
    prev->next = head;
    head->prev = prev;
}

static void setup_resources_close(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rwResourcesOffset = offset;
    RwResourcesGlobals *r = (RwResourcesGlobals *)at(g, offset);
    RwResEntry *a[6], *b[6];
    r->entriesA.next = r->entriesA.prev = &r->entriesA;
    r->entriesB.next = r->entriesB.prev = &r->entriesB;
    link_list(&r->entriesA, a, dt_range(0, 4));
    link_list(&r->entriesB, b, dt_range(0, 4));
    r->currentSize = dt_range(0, 30000);
    r->maxSize = dt_range(0, 100000);
    r->reusageSize = dt_range(0, 100);
    r->usedEntries = &r->entriesB;
    r->freeEntries = &r->entriesA;
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

static char *BMX_CDECL stub_strcpy(char *dst, const char *src)
{
    rwtLogPtr(dst);
    rwtLog((uint32_t)(uintptr_t)src);
    return dst;
}

static int BMX_CDECL stub_system_script(int request, void *out, void *inOut, int in)
{
    rwtLog(0x5c);
    rwtLog((uint32_t)request);
    rwtLogPtr(out);
    rwtLogPtr(inOut);
    rwtLog((uint32_t)in);
    return (int)(dt_u32() % 3 == 0);
}

static void setup_device_request(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    g->stringFuncs.rwstrcpy = stub_strcpy;
    RwDevice *device = dt_alloc(sizeof(RwDevice));
    device->fpSystem = stub_system_script;
    DT_ARG_PTR(c, 0, device);
    static const int32_t requests[] = {0, 5, 9, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 3};
    DT_ARG_INT(c, 1, requests[dt_u32() % 11]);
    DT_ARG_PTR(c, 2, dt_alloc(0x40));
    DT_ARG_PTR(c, 3, dt_alloc(0x20));
    DT_ARG_INT(c, 4, dt_u32() % 2 ? 0 : dt_range(1, 5));
    rwtEnd();
}

/* ---- frames, vectors, matrices ---------------------------------------------------------------------- */

static void setup_frame_close(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rwFrameOffset = offset;
    *(RwFreeList **)at(g, offset) = dt_u32() % 3 == 0 ? NULL : ORIG_FREELIST_CREATE(0xa8, 0x28, 4);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

static void fill_matrix(RwMatrix *m)
{
    float *f = (float *)m;
    for (int i = 0; i < 16; i++) {
        f[i] = dt_float_edgy(-100.0F, 100.0F);
    }
}

static void setup_matrix_multiply(DtCtx *c)
{
    dt_seed(c);
    RwMatrix *out = dt_alloc(sizeof(RwMatrix));
    RwMatrix *a = dt_alloc(sizeof(RwMatrix));
    RwMatrix *b = dt_alloc(sizeof(RwMatrix));
    fill_matrix(a);
    fill_matrix(b);
    DT_ARG_PTR(c, 0, out);
    DT_ARG_PTR(c, 1, a);
    DT_ARG_PTR(c, 2, b);
}

static void setup_transform(DtCtx *c)
{
    dt_seed(c);
    const int32_t n = dt_range(-1, 8);
    RwV3d *in = dt_alloc(12 * (uint32_t)(n > 0 ? n : 1));
    for (int i = 0; i < 3 * n; i++) {
        ((float *)in)[i] = dt_float_edgy(-1000.0F, 1000.0F);
    }
    RwMatrix *m = dt_alloc(sizeof(RwMatrix));
    fill_matrix(m);
    DT_ARG_PTR(c, 0, dt_alloc(12 * (uint32_t)(n > 0 ? n : 1)));
    DT_ARG_PTR(c, 1, in);
    DT_ARG_INT(c, 2, n);
    DT_ARG_PTR(c, 3, m);
}

/* In-place use, as RwV3dTransformPoints does it. */
static void setup_transform_inplace(DtCtx *c)
{
    setup_transform(c);
    c->args[0] = c->args[1];
}

static void setup_vector_module(DtCtx *c, int tables)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rwFastSqrtOffset = offset;
    uint32_t *d = (uint32_t *)at(g, offset);
    d[0] = tables && dt_u32() % 4 ? (uint32_t)(uintptr_t)dt_alloc(32) : 0;
    d[1] = tables && dt_u32() % 4 ? (uint32_t)(uintptr_t)dt_alloc(32) : 0;
    d[2] = dt_u32();
    d[3] = dt_u32();
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

static void setup_vector_close(DtCtx *c)
{
    setup_vector_module(c, 1);
}

static void setup_vector_free(DtCtx *c)
{
    setup_vector_module(c, 1);
}

static void setup_vector_open(DtCtx *c)
{
    modEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_INT(c, 1, modOffset());
    gRwtMallocFailAt = dt_u32() % 3 == 0 ? dt_range(1, 2) : 0;
    rwtEnd();
}

/* rwV3dSetTransformFuncs(EAX = points, EDX = vectors) through a cdecl wrapper. */
static int set_funcs_orig(uint32_t points, uint32_t vectors)
{
    return ((int(__attribute__((regparm(2))) *)(uint32_t, uint32_t))(uintptr_t)0x004d3608)(points, vectors);
}

static int set_funcs_impl(uint32_t points, uint32_t vectors)
{
    return rwV3dSetTransformFuncs((RwV3dTransformFunc)(uintptr_t)points,
                                  (RwV3dTransformFunc)(uintptr_t)vectors);
}

static void setup_set_funcs(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rwFastSqrtOffset = offset;
    dt_fill(at(g, offset), 16);
    DT_ARG_INT(c, 0, dt_u32() % 3 ? dt_u32() : 0);
    DT_ARG_INT(c, 1, dt_u32() % 3 ? dt_u32() : 0);
    rwtEnd();
}

static RwFrame *new_frame(void)
{
    return dt_alloc(sizeof(RwFrame));
}

/* A random hierarchy: each frame gets 0..2 children and a sibling chain (the walk follows next and child). */
static RwFrame *build_frames(int depth)
{
    RwFrame *first = NULL;
    RwFrame **link = &first;
    for (int n = dt_range(1, 3); n > 0; n--) {
        RwFrame *f = new_frame();
        *link = f;
        link = &f->next;
        if (depth > 0 && dt_u32() % 2) {
            f->child = build_frames(depth - 1);
        }
    }
    return first;
}

static int fill_orig(RwFrame ***cursor, RwFrame *frame)
{
    return ((int(__attribute__((regparm(2))) *)(RwFrame ***, RwFrame *))(uintptr_t)0x004e8fc0)(cursor, frame);
}

static int fill_impl(RwFrame ***cursor, RwFrame *frame)
{
    return rwFrameListFill(cursor, frame);
}

static void setup_frame_fill(DtCtx *c)
{
    dt_seed(c);
    RwFrame **buffer = (RwFrame **)dt_alloc(4 * 64);
    RwFrame ***cursor = (RwFrame ***)dt_alloc(4);
    *cursor = buffer;
    DT_ARG_PTR(c, 0, cursor);
    DT_ARG_PTR(c, 1, dt_u32() % 5 ? build_frames(2) : NULL);
}

static void setup_frame_list_init(DtCtx *c)
{
    modEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_PTR(c, 1, build_frames(2));
    gRwtMallocFailAt = dt_u32() % 4 == 0 ? 1 : 0;
    rwtEnd();
}

/* ---- rasters, textures, images, camera, streams ----------------------------------------------------- */

static void setup_raster_close(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rwRasterOffset = offset;
    RwRasterGlobals *r = (RwRasterGlobals *)at(g, offset);
    r->freeList = dt_u32() % 3 == 0 ? NULL : ORIG_FREELIST_CREATE(0x28, 10, 0);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

static RwTexture *BMX_CDECL stub_texture_destroy(RwTexture *texture)
{
    rwtLog(0xde7);
    rwtLogPtr(texture);
    return texture;
}

static void setup_texture_close(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rwTextureOffset = offset;
    RwTextureGlobals *t = (RwTextureGlobals *)at(g, offset);
    t->textureFreeList = dt_u32() % 5 == 0 ? NULL : ORIG_FREELIST_CREATE(0x28, 7, 0);
    t->dictFreeList = dt_u32() % 5 == 0 ? NULL : ORIG_FREELIST_CREATE(0x1c, 3, 0);
    t->dictList.next = t->dictList.prev = &t->dictList;
    RwTexDictionary *first = NULL;
    for (int d = dt_range(0, 3); d > 0; d--) {
        RwTexDictionary *dict = dt_alloc(sizeof(RwTexDictionary));
        dict->texturesInDict.next = dict->texturesInDict.prev = &dict->texturesInDict;
        for (int n = dt_range(0, 3); n > 0; n--) {
            RwTexture *tex = dt_alloc(sizeof(RwTexture));
            tex->lInDictionary.prev = dict->texturesInDict.prev;
            tex->lInDictionary.next = &dict->texturesInDict;
            dict->texturesInDict.prev->next = &tex->lInDictionary;
            dict->texturesInDict.prev = &tex->lInDictionary;
        }
        dict->lInInstance.next = &t->dictList;
        dict->lInInstance.prev = t->dictList.prev;
        t->dictList.prev->next = &dict->lInInstance;
        t->dictList.prev = &dict->lInInstance;
        first = first == NULL ? dict : first;
    }
    t->currentDict = dt_u32() % 2 ? first : dt_alloc(8);
    t->field20 = dt_u32() % 2 ? dt_alloc(16) : NULL;
    t->field24 = (uint16_t)dt_u32();
    DT_ARG_PTR(c, 0, dt_alloc(16));
    dt_stub(0x004df730, NULL, (void *)stub_texture_destroy);
    rwtEnd();
}

/* ---- Im3D --------------------------------------------------------------------------------------------- */

static void setup_im3d_entries(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    rwIm3DOffset = modOffset();
    (void)g;
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

static void setup_render_pipeline_close(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rwIm3DOffset = offset;
    RwIm3DGlobals *m = (RwIm3DGlobals *)at(g, offset);
    m->pipelineFreeList = dt_u32() % 3 == 0 ? NULL : ORIG_FREELIST_CREATE(0x1c, 5, 0);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

static void setup_pipeline_default(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_PTR(c, 1, dt_alloc(16));
}

static int BMX_CDECL stub_system_pipeline(int request, void *out, void *inOut, int in)
{
    rwtLog(0x5d);
    rwtLog((uint32_t)request);
    rwtLogPtr(out);
    rwtLogPtr(inOut);
    rwtLog((uint32_t)in);
    return dt_u32() % 3 != 0;
}

static void setup_pipeline_open(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rwIm3DOffset = offset;
    RwIm3DGlobals *m = (RwIm3DGlobals *)at(g, offset);
    m->pipelineFreeList = dt_alloc(sizeof(RwFreeList));
    g->dOpenDevice.fpSystem = stub_system_pipeline;
    gRwtEntryFailAt = dt_u32() % 4 == 0 ? 1 : 0;
    rwtEnd();
}

static void setup_pipeline_release(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rwIm3DOffset = offset;
    ((RwIm3DGlobals *)at(g, offset))->pipelineFreeList = dt_alloc(sizeof(RwFreeList));
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    rwtEnd();
}

static void BMX_CDECL stub_ext_slot(void *pipeline)
{
    rwtLog(0xe5107);
    rwtLogPtr(pipeline);
}

static void setup_ext_slot(DtCtx *c)
{
    dt_seed(c);
    modEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    rwtEnd();
}

/* The core extension's function table at 0x69bee0: entries 1 and 3 point at the logging stub. */
static void stub_ext_table(void)
{
    uint32_t *table = (uint32_t *)(uintptr_t)0x0069bee0;
    table[1] = (uint32_t)(uintptr_t)stub_ext_slot;
    table[3] = (uint32_t)(uintptr_t)stub_ext_slot;
}

static void setup_ext_slot_stubbed(DtCtx *c)
{
    setup_ext_slot(c);
    stub_ext_table();
}

static void setup_im3d_module_open(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    rwIm3DOffset = modOffset();
    ((RwIm3DGlobals *)at(g, rwIm3DOffset))->pipelineFreeList = dt_alloc(sizeof(RwFreeList));
    g->dOpenDevice.fpSystem = stub_system_pipeline;
    stub_ext_table();
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_INT(c, 1, modOffset());
    gRwtEntryFailAt = dt_u32() % 4 == 0 ? 1 : 0;
    rwtEnd();
}

static void setup_im3d_module_close(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    rwIm3DOffset = modOffset();
    ((RwIm3DGlobals *)at(g, rwIm3DOffset))->pipelineFreeList = dt_alloc(sizeof(RwFreeList));
    const int32_t offset = modOffset();
    rwIm3DPipelineOffset = offset;
    *(void **)at(g, offset) = dt_u32() % 3 ? dt_alloc(0x20) : NULL;
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

/* ---- clumps, atomics, geometry, lights, world ------------------------------------------------------- */

__attribute__((regparm(1))) static int stub_atomic_destroy(RpAtomic *atomic)
{
    rwtLog(0xa7d);
    rwtLogPtr(atomic);
    return 1;
}

static int BMX_CDECL stub_geometry_destroy(RpGeometry *geometry)
{
    rwtLog(0x6d);
    rwtLogPtr(geometry);
    return 1;
}

static void setup_atomic_cb(DtCtx *c)
{
    modEnv(c);
    dt_stub(0x004b1b64, NULL, (void *)stub_atomic_destroy);
    DT_ARG_PTR(c, 0, dt_alloc(0x80));
    DT_ARG_PTR(c, 1, dt_alloc(16));
    rwtEnd();
}

/* The release only drops a reference (count > 1), or reports (module closed, no geometry). */
static void setup_geometry_cb(DtCtx *c)
{
    modEnv(c);
    *(int32_t *)(uintptr_t)0x00676724 = dt_u32() % 3 ? 1 : 0; /* rpGeometryModuleCount */
    RpGeometry *geometry = dt_u32() % 5 == 0 ? NULL : dt_alloc(sizeof(RpGeometry));
    if (geometry != NULL) {
        geometry->refCount = (uint16_t)dt_range(2, 6);
    }
    DT_ARG_PTR(c, 0, geometry);
    DT_ARG_PTR(c, 1, dt_alloc(16));
    rwtEnd();
}

static void setup_ptr_arg(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_alloc(0x40));
    DT_ARG_PTR(c, 1, dt_alloc(0x40));
}

static int BMX_CDECL stub_frame_destroy(RwFrame *frame)
{
    rwtLog(0x4d29c0);
    rwtLogPtr(frame);
    return 1;
}

/* A clump with a frame hierarchy (or none) and 0..3 atomics, in an engine whose clump free list is a marker.
 */
static void setup_clump_entry(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rpClumpModuleOffset = offset;
    ((RpClumpGlobals *)at(g, offset))->clumpFreeList = dt_alloc(8);
    dt_stub(0x004b1b64, NULL, (void *)stub_atomic_destroy);
    dt_stub(0x004d29c0, NULL, (void *)stub_frame_destroy);
    RpClump *clump = dt_alloc(0x80);
    clump->object.parent = dt_u32() % 3 == 0 ? NULL : dt_alloc(0xa8);
    clump->atomicList.next = clump->atomicList.prev = &clump->atomicList;
    for (int n = dt_range(0, 3); n > 0; n--) {
        RpAtomic *atomic = dt_alloc(sizeof(RpAtomic));
        atomic->inClumpLink.prev = clump->atomicList.prev;
        atomic->inClumpLink.next = &clump->atomicList;
        clump->atomicList.prev->next = &atomic->inClumpLink;
        clump->atomicList.prev = &atomic->inClumpLink;
    }
    DT_ARG_PTR(c, 0, clump);
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

static int clump_entry_orig(RpClump *clump)
{
    return ((int(__attribute__((regparm(1))) *)(RpClump *))(uintptr_t)0x004b21e4)(clump);
}

static int clump_entry_impl(RpClump *clump)
{
    return rpClumpEntryDestroy(clump);
}

static void setup_clump_close(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rpClumpModuleOffset = offset;
    rwIm3DOffset = offset + 0x40;
    ((RwIm3DGlobals *)at(g, rwIm3DOffset))->pipelineFreeList = dt_alloc(sizeof(RwFreeList));
    RpClumpGlobals *m = (RpClumpGlobals *)at(g, offset);
    m->pipeline = dt_alloc(0x20);
    m->atomicFreeList = ORIG_FREELIST_CREATE(0x70, 4, 4);
    m->clumpFreeList = ORIG_FREELIST_CREATE(0x30, 4, 4);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

/* An atomic with a geometry of 0..3 morph targets and a random interpolator state. */
static RpAtomic *make_atomic(int withGeometry)
{
    RpAtomic *atomic = dt_alloc(sizeof(RpAtomic));
    atomic->object.object.privateFlags = (uint8_t)dt_u32();
    atomic->interpolator.flags = dt_u32();
    atomic->interpolator.startMorphTarget = (int16_t)dt_range(-1, 4);
    atomic->interpolator.endMorphTarget =
        dt_u32() % 3 ? (int16_t)dt_range(-1, 4) : atomic->interpolator.startMorphTarget;
    atomic->interpolator.position = dt_float_edgy(-2.0F, 2.0F);
    atomic->interpolator.recipTime = dt_float_edgy(-2.0F, 2.0F);
    dt_fill(&atomic->boundingSphere, sizeof atomic->boundingSphere);
    if (withGeometry) {
        RpGeometry *geometry = dt_alloc(sizeof(RpGeometry));
        geometry->numMorphTargets = dt_range(0, 3);
        RpMorphTarget *targets = dt_alloc(sizeof(RpMorphTarget) * 5);
        for (int i = 0; i < 5; i++) {
            targets[i].boundingSphere.center.x = dt_float_edgy(-500.0F, 500.0F);
            targets[i].boundingSphere.center.y = dt_float_edgy(-500.0F, 500.0F);
            targets[i].boundingSphere.center.z = dt_float_edgy(-500.0F, 500.0F);
            targets[i].boundingSphere.radius = dt_float_edgy(0.0F, 300.0F);
        }
        geometry->morphTarget = targets;
        atomic->geometry = geometry;
    }
    return atomic;
}

static void setup_atomic_bsphere(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, make_atomic(dt_u32() % 8 != 0));
}

static int bsphere_orig(RpAtomic *atomic)
{
    ((void(__attribute__((regparm(1))) *)(RpAtomic *))(uintptr_t)0x004b1154)(atomic);
    return 0;
}

static int bsphere_impl(RpAtomic *atomic)
{
    rpAtomicUpdateBoundingSphere(atomic);
    return 0;
}

static void setup_atomic_sync(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, make_atomic(1));
}

static void setup_set_geometry(DtCtx *c)
{
    modEnv(c);
    dt_stub(0x004b5ab0, NULL, (void *)stub_geometry_destroy);
    dt_stub(0x004b5a30, NULL, (void *)stub_geometry_destroy);
    dt_stub(0x004d2a80, NULL, (void *)stub_geometry_destroy);
    RpAtomic *atomic = make_atomic(dt_u32() % 3 != 0);
    atomic->object.object.parent = dt_u32() % 2 ? dt_alloc(0xa8) : NULL;
    RpGeometry *geometry = dt_u32() % 5 ? atomic->geometry : NULL;
    if (dt_u32() % 2) {
        geometry = make_atomic(1)->geometry;
    }
    DT_ARG_PTR(c, 0, atomic);
    DT_ARG_PTR(c, 1, geometry);
    DT_ARG_INT(c, 2, dt_u32() % 2);
    rwtEnd();
}

static RwIm3DPipeline *BMX_CDECL stub_pipeline_push(RwIm3DPipeline *pipeline)
{
    rwtLog(0x9051);
    rwtLogPtr(pipeline);
    return dt_u32() % 3 ? pipeline : NULL;
}

static RwIm3DTransformInfo *BMX_CDECL stub_pipeline_run(RwIm3DTransformInfo *info)
{
    rwtLog(0x9052);
    rwtLogPtr(info);
    return dt_u32() % 3 ? info : NULL;
}

static RwIm3DPipeline *BMX_CDECL stub_pipeline_pop(void)
{
    rwtLog(0x9053);
    return NULL;
}

static void setup_atomic_render(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rpClumpModuleOffset = offset;
    ((RpClumpGlobals *)at(g, offset))->pipeline = dt_alloc(0x20);
    dt_stub(0x004e85b0, NULL, (void *)stub_pipeline_push);
    dt_stub(0x004e8600, NULL, (void *)stub_pipeline_run);
    dt_stub(0x004e8690, NULL, (void *)stub_pipeline_pop);
    DT_ARG_PTR(c, 0, dt_alloc(0x80));
    rwtEnd();
}

static void setup_geometry_close(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rpGeometryOffset = offset;
    *(RwFreeList **)at(g, offset) = ORIG_FREELIST_CREATE(0x48, 5, 0);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

static void setup_light_close(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    rpLightOffset = offset;
    *(RwFreeList **)at(g, offset) = ORIG_FREELIST_CREATE(0x44, 5, 0);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

static void BMX_CDECL stub_visit(void *context, RpLight *light, void *block, void *tail, void *extra)
{
    rwtLog(0x715);
    rwtLogPtr(context);
    rwtLogPtr(light);
    rwtLogPtr(block);
    rwtLogPtr(tail);
    rwtLogPtr(extra);
}

static void setup_light_visit(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    *(uint16_t *)at(g, 0x0c) = (uint16_t)dt_u32();
    RpLight *light = dt_alloc(sizeof(RpLight));
    light->object.object.flags = (uint8_t)dt_u32();
    light->lightFrame = dt_u32() % 3 ? (uint16_t)dt_u32() : *(uint16_t *)at(g, 0x0c);
    RpLightVisit *visit = dt_alloc(sizeof(RpLightVisit));
    visit->context = dt_alloc(8);
    visit->callback = stub_visit;
    visit->extra = dt_alloc(8);
    visit->tail = dt_alloc(8);
    dt_fill(visit->block, sizeof visit->block);
    DT_ARG_PTR(c, 0, light);
    DT_ARG_PTR(c, 1, visit);
    rwtEnd();
}

static void setup_tie_open(DtCtx *c)
{
    modEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_INT(c, 1, modOffset());
    gRwtMallocFailAt = dt_u32() % 3 == 0 ? dt_range(1, 4) : 0;
    rwtEnd();
}

static void setup_clump_ext(DtCtx *c)
{
    modEnv(c);
    rpWorldClumpOffset = 4 * dt_range(0, 16);
    DT_ARG_PTR(c, 0, dt_alloc(0x80));
    rwtEnd();
}

#define ANIM_OFFSET_VAR(addr) (*(int32_t *)(uintptr_t)(addr))

static void setup_anim_open(DtCtx *c)
{
    modEnv(c);
    ANIM_OFFSET_VAR(0x00678284) = dt_u32() % 2 ? 0 : dt_range(1, 3); /* the module count before this open */
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_INT(c, 1, modOffset());
    gRwtMallocFailAt = dt_u32() % 3 == 0 ? dt_range(1, 2) : 0;
    rwtEnd();
}

static void setup_anim_close(DtCtx *c)
{
    RwGlobals *g = modEnv(c);
    const int32_t offset = modOffset();
    ANIM_OFFSET_VAR(0x00678280) = offset;
    ANIM_OFFSET_VAR(0x00678284) = dt_u32() % 2 ? 1 : dt_range(2, 4); /* the last close frees the list */
    *(RwFreeList **)at(g, offset) = ORIG_FREELIST_CREATE(8, 0x32, 0);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

/* ---- world extension --------------------------------------------------------------------------------- */

static int alpha_format_orig(uint32_t format)
{
    return ((int(__attribute__((regparm(1))) *)(uint32_t))(uintptr_t)0x004c5ad0)(format);
}

static int alpha_format_impl(uint32_t format)
{
    return rpRasterFormatHasAlpha(format);
}

static void setup_alpha_format(DtCtx *c)
{
    dt_seed(c);
    static const uint32_t formats[] = {0x100, 0x300, 0x500, 0x200, 0x400, 0, 0xf00, 0x1100};
    DT_ARG_INT(c, 0, dt_u32() % 3 ? formats[dt_u32() % 8] : dt_u32());
}

static RpMaterial *random_material(void)
{
    if (dt_u32() % 6 == 0) {
        return NULL;
    }
    RpMaterial *m = dt_alloc(sizeof(RpMaterial));
    if (dt_u32() % 3 != 0) {
        RwRaster *raster = dt_alloc(sizeof(RwRaster));
        raster->format = (uint8_t)(dt_u32() % 8);
        RwTexture *texture = dt_alloc(sizeof(RwTexture));
        texture->raster = raster;
        m->texture = texture;
    }
    m->color.alpha = dt_u32() % 2 ? 0xff : (uint8_t)dt_u32();
    return m;
}

static const RpMesh *const *mesh_ref(RpMaterial *material)
{
    RpMesh *mesh = dt_alloc(sizeof(RpMesh));
    mesh->material = material;
    const RpMesh **ref = (const RpMesh **)dt_alloc(4);
    *ref = mesh;
    return (const RpMesh *const *)ref;
}

static void setup_mesh_compare(DtCtx *c)
{
    dt_seed(c);
    RpMaterial *a = random_material();
    RpMaterial *b = dt_u32() % 5 == 0 ? a : random_material();
    DT_ARG_PTR(c, 0, mesh_ref(a));
    DT_ARG_PTR(c, 1, mesh_ref(b));
}

static void setup_pipeline_init(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_u32() % 5 ? dt_alloc(sizeof(RwIm3DPipeline)) : NULL);
}

static void setup_light_select(DtCtx *c)
{
    dt_seed(c);
    static const uint8_t types[] = {0, 1, 2, 3, 0x80, 0x81, 0x82, 0x83};
    RpLight *light = dt_alloc(sizeof(RpLight));
    light->object.object.subType = types[dt_u32() % 8];
    DT_ARG_PTR(c, 0, light);
}

static void setup_no_args(DtCtx *c)
{
    dt_seed(c);
}

DT_CASES(rwmodules_cases) = {
    {"_rwFreeListAllocReal", 0x004dbc30, _rwFreeListAllocReal, 1, DT_RET_INT, setup_alloc_real, 300, 0},
    {"_rwFreeListFreeReal", 0x004dbd90, _rwFreeListFreeReal, 2, DT_RET_VOID, setup_free_real, 100, 0},
    {"rwFreeListAllocPlain", 0x004cf800, rwFreeListAllocPlain, 1, DT_RET_INT, setup_alloc_plain, 40, 0},
    {"rwFreeListFreePlain", 0x004cf820, rwFreeListFreePlain, 2, DT_RET_INT, setup_free_plain, 20, 0},
    {"_rwErrorOpen", 0x004dee30, _rwErrorOpen, 2, DT_RET_INT, setup_open, 30, 0},
    {"_rwColorOpen", 0x004eb2a0, _rwColorOpen, 1, DT_RET_INT, setup_engine_only, 10, 0},
    {"_rwColorClose", 0x004eb2b0, _rwColorClose, 1, DT_RET_INT, setup_engine_only, 10, 0},
    {"_rwMatrixOpen", 0x004d2080, _rwMatrixOpen, 2, DT_RET_INT, setup_open, 80, 0},
    {"ResourcesInit", 0x004dc990, ResourcesInit, 1, DT_RET_VOID, setup_resources_init, 20, 0},
    {"_rwResourcesOpen", 0x004dc210, _rwResourcesOpen, 2, DT_RET_INT, setup_open, 30, 0},
    {"_rwResourcesClose", 0x004dc250, _rwResourcesClose, 1, DT_RET_INT, setup_resources_close, 150, 0},
    {"_rwDeviceSystemRequest", 0x004cf3b0, _rwDeviceSystemRequest, 5, DT_RET_INT, setup_device_request, 300,
     0},
    {"_rwFrameOpen", 0x004d2280, _rwFrameOpen, 2, DT_RET_INT, setup_open, 80, 0},
    {"_rwFrameClose", 0x004d2310, _rwFrameClose, 1, DT_RET_INT, setup_frame_close, 40, 0},
    {"rwMatrixMultiplyDefault", 0x004d02c0, rwMatrixMultiplyDefault, 3, DT_RET_VOID, setup_matrix_multiply,
     300, 0},
    {"rwV3dTransformPointsDefault", 0x004d34d0, rwV3dTransformPointsDefault, 4, DT_RET_INT, setup_transform,
     300, 0},
    {"rwV3dTransformPointsDefault (in place)", 0x004d34d0, rwV3dTransformPointsDefault, 4, DT_RET_INT,
     setup_transform_inplace, 100, 0},
    {"rwV3dTransformVectorsDefault", 0x004d3570, rwV3dTransformVectorsDefault, 4, DT_RET_INT, setup_transform,
     300, 0},
    {"rwV3dTransformVectorsDefault (in place)", 0x004d3570, rwV3dTransformVectorsDefault, 4, DT_RET_INT,
     setup_transform_inplace, 100, 0},
    {"rwV3dSetTransformFuncs", (uint32_t)(uintptr_t)set_funcs_orig, set_funcs_impl, 2, DT_RET_INT,
     setup_set_funcs, 60, 0},
    {"rwV3dFreeSqrtTable", 0x004d3450, rwV3dFreeSqrtTable, 0, DT_RET_VOID, setup_vector_free, 40, 0},
    {"rwV3dFreeInvSqrtTable", 0x004d3490, rwV3dFreeInvSqrtTable, 0, DT_RET_VOID, setup_vector_free, 40, 0},
    {"rwVectorClose", 0x004d3b30, rwVectorClose, 1, DT_RET_INT, setup_vector_close, 40, 0},
    {"rwVectorOpen", 0x004d3b50, rwVectorOpen, 2, DT_RET_INT, setup_vector_open, 8, 0},
    {"rwFrameListFill", (uint32_t)(uintptr_t)fill_orig, fill_impl, 2, DT_RET_INT, setup_frame_fill, 100, 0},
    {"_rwFrameListInitialize", 0x004e96d0, _rwFrameListInitialize, 2, DT_RET_INT, setup_frame_list_init, 80,
     0},
    {"_rwStreamModuleOpen", 0x004e2350, _rwStreamModuleOpen, 2, DT_RET_INT, setup_open, 60, 0},
    {"_rwCameraOpen", 0x004dd430, _rwCameraOpen, 2, DT_RET_INT, setup_open, 60, 0},
    {"_rwRasterOpen", 0x004dc9e0, _rwRasterOpen, 2, DT_RET_INT, setup_open, 60, 0},
    {"_rwRasterClose", 0x004dcac0, _rwRasterClose, 1, DT_RET_INT, setup_raster_close, 40, 0},
    {"_rwTextureOpen", 0x004df120, _rwTextureOpen, 2, DT_RET_INT, setup_open, 150, 0},
    {"_rwTextureClose", 0x004df380, _rwTextureClose, 1, DT_RET_INT, setup_texture_close, 200, 0},
    {"_rwImageOpen", 0x004e33d0, _rwImageOpen, 2, DT_RET_INT, setup_open, 25, 0},
    {"rwIm3DDefaultBegin", 0x004e8390, rwIm3DDefaultBegin, 1, DT_RET_INT, setup_pipeline_default, 3, 0},
    {"rwIm3DDefaultEnd", 0x004e83a0, rwIm3DDefaultEnd, 0, DT_RET_INT, setup_pipeline_default, 3, 0},
    {"rwIm3DSetEntries", 0x004e83b0, rwIm3DSetEntries, 2, DT_RET_VOID, setup_im3d_entries, 20, 0},
    {"_rwRenderPipelineOpen", 0x004e83e0, _rwRenderPipelineOpen, 2, DT_RET_INT, setup_open, 60, 0},
    {"_rwRenderPipelineClose", 0x004e8480, _rwRenderPipelineClose, 1, DT_RET_INT, setup_render_pipeline_close,
     40, 0},
    {"rwPipelineDefaultInstance", 0x004e84c0, rwPipelineDefaultInstance, 1, DT_RET_INT,
     setup_pipeline_default, 3, 0},
    {"rwPipelineDefaultStage4", 0x004e84d0, rwPipelineDefaultStage4, 2, DT_RET_VOID, setup_pipeline_default,
     3, 0},
    {"rwPipelineDefaultStageData", 0x004e84e0, rwPipelineDefaultStageData, 0, DT_RET_VOID,
     setup_pipeline_default, 3, 0},
    {"rwPipelineDefaultStage", 0x004e84f0, rwPipelineDefaultStage, 1, DT_RET_VOID, setup_pipeline_default, 3,
     0},
    {"_rpWorldPipelineOpen", 0x004e8500, _rpWorldPipelineOpen, 0, DT_RET_INT, setup_pipeline_open, 60, 0},
    {"rwIm3DPipelineRelease", 0x004e8580, rwIm3DPipelineRelease, 1, DT_RET_INT, setup_pipeline_release, 20,
     0},
    {"rwCoreExtCallSlot3", 0x004e8700, rwCoreExtCallSlot3, 1, DT_RET_INT, setup_ext_slot_stubbed, 20, 0},
    {"rwCoreExtCallSlot1", 0x004e8720, rwCoreExtCallSlot1, 1, DT_RET_VOID, setup_ext_slot_stubbed, 20, 0},
    {"_rwIm3DOpen", 0x004dee70, _rwIm3DOpen, 2, DT_RET_INT, setup_im3d_module_open, 60, 0},
    {"_rwIm3DClose", 0x004deec0, _rwIm3DClose, 1, DT_RET_INT, setup_im3d_module_close, 40, 0},
    {"rpAtomicFreeListDestroyCB", 0x004b1070, rpAtomicFreeListDestroyCB, 2, DT_RET_VOID, setup_atomic_cb, 20,
     0},
    {"rpClumpDefaultCallback", 0x004b20f0, rpClumpDefaultCallback, 1, DT_RET_INT, setup_ptr_arg, 10, 0},
    {"_rpClumpClose", 0x004b1080, _rpClumpClose, 1, DT_RET_INT, setup_clump_close, 30, 0},
    {"rpClumpFreeListDestroyCB", 0x004b1060, rpClumpFreeListDestroyCB, 2, DT_RET_INT, setup_clump_entry, 150,
     0},
    {"rpClumpEntryDestroy", (uint32_t)(uintptr_t)clump_entry_orig, clump_entry_impl, 1, DT_RET_INT,
     setup_clump_entry, 150, 0},
    {"rpLightVisitEnabled", 0x004b17c0, rpLightVisitEnabled, 2, DT_RET_INT, setup_light_visit, 100, 0},
    {"rpAtomicUpdateBoundingSphere", (uint32_t)(uintptr_t)bsphere_orig, bsphere_impl, 1, DT_RET_INT,
     setup_atomic_bsphere, 500, 0},
    {"rpAtomicSyncCallback", 0x004b1490, rpAtomicSyncCallback, 1, DT_RET_INT, setup_atomic_sync, 200, 0},
    {"RpAtomicSetGeometry", 0x004b1ad0, RpAtomicSetGeometry, 3, DT_RET_INT, setup_set_geometry, 200, 0},
    {"rpAtomicRenderCallback", 0x004b3350, rpAtomicRenderCallback, 1, DT_RET_INT, setup_atomic_render, 60, 0},
    {"_rpGeometryOpen", 0x004b3390, _rpGeometryOpen, 2, DT_RET_INT, setup_open, 60, 0},
    {"rpGeometryFreeListDestroyCB", 0x004b33f0, rpGeometryFreeListDestroyCB, 2, DT_RET_VOID,
     setup_geometry_cb, 20, 0},
    {"_rpGeometryClose", 0x004b3400, _rpGeometryClose, 1, DT_RET_INT, setup_geometry_close, 30, 0},
    {"_rpMeshOpen", 0x004c4710, _rpMeshOpen, 2, DT_RET_INT, setup_open, 10, 0},
    {"_rpMeshClose", 0x004c4740, _rpMeshClose, 1, DT_RET_INT, setup_engine_only, 10, 0},
    {"_rpLightOpen", 0x004c2310, _rpLightOpen, 2, DT_RET_INT, setup_open, 60, 0},
    {"_rpLightClose", 0x004c2380, _rpLightClose, 1, DT_RET_INT, setup_light_close, 30, 0},
    {"rpLightVisitOnce", 0x004b1800, rpLightVisitOnce, 2, DT_RET_INT, setup_light_visit, 200, 0},
    {"_rpTieOpen", 0x004b7570, _rpTieOpen, 2, DT_RET_INT, setup_tie_open, 60, 0},
    {"WorldDeInitClumpExt", 0x004b8fe0, WorldDeInitClumpExt, 1, DT_RET_INT, setup_ptr_arg, 10, 0},
    {"WorldInitClumpExt", 0x004b8ff0, WorldInitClumpExt, 1, DT_RET_INT, setup_clump_ext, 20, 0},
    {"_rpBspOpen", 0x004bc3a0, _rpBspOpen, 1, DT_RET_INT, setup_engine_only, 5, 0},
    {"_rpBspClose", 0x004bc3b0, _rpBspClose, 1, DT_RET_INT, setup_engine_only, 5, 0},
    {"_rpSectorOpen", 0x004c2020, _rpSectorOpen, 1, DT_RET_INT, setup_engine_only, 5, 0},
    {"_rpSectorClose", 0x004c2030, _rpSectorClose, 1, DT_RET_INT, setup_engine_only, 5, 0},
    {"_rpBinaryWorldOpen", 0x004c2c30, _rpBinaryWorldOpen, 1, DT_RET_INT, setup_engine_only, 5, 0},
    {"_rpBinaryWorldClose", 0x004c2c40, _rpBinaryWorldClose, 1, DT_RET_INT, setup_engine_only, 5, 0},
    {"_rpMaterialListOpen", 0x004c56b0, _rpMaterialListOpen, 1, DT_RET_INT, setup_engine_only, 5, 0},
    {"_rpMaterialListClose", 0x004c56c0, _rpMaterialListClose, 1, DT_RET_INT, setup_engine_only, 5, 0},
    {"WorldObjectOpen (anim)", 0x004fcd50, WorldObjectOpen, 2, DT_RET_INT, setup_anim_open, 60, 0},
    {"WorldObjectClose (anim)", 0x004fcda0, WorldObjectClose, 1, DT_RET_INT, setup_anim_close, 40, 0},
    {"rpWorldExtAttachNothing", 0x004c5ab0, rpWorldExtAttachNothing, 0, DT_RET_INT, setup_no_args, 3, 0},
    {"rpRasterFormatHasAlpha", (uint32_t)(uintptr_t)alpha_format_orig, alpha_format_impl, 1, DT_RET_INT,
     setup_alpha_format, 60, 0},
    {"rpMeshCompareMaterials", 0x004c5af0, rpMeshCompareMaterials, 2, DT_RET_INT, setup_mesh_compare, 300, 0},
    {"rpWorldPipelineInit", 0x004c9270, rpWorldPipelineInit, 1, DT_RET_INT, setup_pipeline_init, 20, 0},
    {"rpAtomicPipelineInit", 0x004c9730, rpAtomicPipelineInit, 1, DT_RET_INT, setup_pipeline_init, 20, 0},
    {"rpWorldExtLightSelectCallback", 0x004caa60, rpWorldExtLightSelectCallback, 1, DT_RET_VOID,
     setup_light_select, 40, 0},
};
