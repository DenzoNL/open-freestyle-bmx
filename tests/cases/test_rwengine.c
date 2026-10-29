/*
 * Tests for rw/engine.c: the device queries, engine init/open/start, the memory interface, the resource
 * arena and the small render state and matrix helpers. The device is a stub that logs every request and
 * answers from a per-iteration script; the engine instance is the fixture from dt_rw.h.
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwcore.h"
#include "rw/rwplugins.h"

#define ORIG_STRCPY DT_ORIG(0x004a8ca0, char *(*)(char *, const char *))

static uint32_t gDevOk;   /* bit n: device request n succeeds */
static int32_t gDevValue; /* the number a query hands back */

/* The device driver's system function: logs the request and answers from the script. */
static int BMX_CDECL stub_system(int request, void *out, void *inOut, int in)
{
    rwtLog(0xd5);
    rwtLog((uint32_t)request);
    rwtLogPtr(out);
    rwtLogPtr(inOut);
    rwtLog((uint32_t)in);
    if (request < 0 || request > 31 || !((gDevOk >> request) & 1)) {
        return 0;
    }
    switch (request) {
    case rwDEVICESYSTEMGETNUMSUBSYSTEMS:
    case rwDEVICESYSTEMGETCURRENTSUBSYSTEM:
    case rwDEVICESYSTEMGETNUMMODES:
    case rwDEVICESYSTEMGETMODE:
        *(int32_t *)out = gDevValue;
        break;
    case rwDEVICESYSTEMGETSUBSYSTEMINFO:
    case rwDEVICESYSTEMGETMODEINFO:
        for (int i = 0; i < 4; i++) {
            ((uint32_t *)out)[i] = 0x1000u + (uint32_t)in * 16u + (uint32_t)i;
        }
        break;
    case rwDEVICESYSTEMREGISTER: {
        RwDevice *device = out;
        device->fpSystem = stub_system;
        device->gammaCorrection = 1.0f;
        break;
    }
    case rwDEVICESYSTEMSTANDARDS:
        for (int i = 0; i < in; i++) {
            ((uint32_t *)out)[i] = 0x5000u + (uint32_t)i;
        }
        break;
    default:
        break;
    }
    return 1;
}

static int BMX_CDECL stub_render_state(int32_t state, int32_t value)
{
    rwtLog(0xe5);
    rwtLog((uint32_t)state);
    rwtLog((uint32_t)value);
    return (int)(dt_u32() & 0xff);
}

static int BMX_CDECL stub_im2d_primitive(int32_t type, void *vertices, int32_t count)
{
    rwtLog(0x2d);
    rwtLog((uint32_t)type);
    rwtLogPtr(vertices);
    rwtLog((uint32_t)count);
    return (int)(dt_u32() & 0xff);
}

static RwGlobals *device_env(DtCtx *c)
{
    dt_seed(c);
    RwGlobals *g = rwtEnv();
    g->dOpenDevice.fpSystem = stub_system;
    g->dOpenDevice.fpRenderStateSet = stub_render_state;
    g->dOpenDevice.fpIm2DRenderPrimitive = stub_im2d_primitive;
    g->stringFuncs.rwstrcpy = ORIG_STRCPY;
    gDevOk = dt_u32() % 4 == 0 ? 0 : (dt_u32() % 3 == 0 ? dt_u32() : 0xffffffffu);
    gDevValue = dt_u32() % 3 == 0 ? (int32_t)dt_u32() : dt_range(0, 12);
    return g;
}

static void setup_num_subsystems(DtCtx *c)
{
    device_env(c);
    rwtEnd();
}

static void setup_subsystem_info(DtCtx *c)
{
    device_env(c);
    DT_ARG_PTR(c, 0, dt_alloc(96));
    DT_ARG_INT(c, 1, dt_u32() % 3 ? 0 : dt_range(1, 5));
    rwtEnd();
}

static void setup_index(DtCtx *c)
{
    device_env(c);
    DT_ARG_INT(c, 0, dt_u32() % 3 ? 0 : dt_range(-2, 20));
    rwtEnd();
}

static void setup_mode_info(DtCtx *c)
{
    device_env(c);
    DT_ARG_PTR(c, 0, dt_alloc(96));
    DT_ARG_INT(c, 1, dt_range(-2, 20));
    rwtEnd();
}

static void BMX_CDECL stub_set_gamma(float gamma)
{
    rwtLog(0x4e5570);
    rwtLogFloatBits(gamma);
}

static void setup_start(DtCtx *c)
{
    RwGlobals *g = device_env(c);
    gRwtCtorCalls = 0;
    gRwtCtorFailAt = dt_u32() % 3 == 0 ? dt_range(1, 4) : 0;
    rwEngineRegistry = *rwtRegistry(dt_range(0, 4), 0x100);
    g->dOpenDevice.gammaCorrection = dt_float_edgy(0.5f, 3.0f);
    g->engineStatus = dt_range(0, 2);
    dt_stub(0x004e5570, NULL, (void *)stub_set_gamma);
    rwtEnd();
}

static void setup_open(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    RwEngineInstance = dt_u32() % 2 ? NULL : RwEngineInstance;
    RwDefaultGlobals.memoryFuncs.rwmalloc = rwtMalloc;
    RwDefaultGlobals.memoryFuncs.rwfree = rwtFree;
    RwDefaultGlobals.engineStatus = dt_range(0, 1);
    RwDefaultGlobals.curWorld = (void *)(uintptr_t)dt_u32();
    rwEngineRegistry.sizeOfStruct = (int32_t)sizeof(RwGlobals) + dt_range(0, 5) * 8;
    rwD3D7Device.fpSystem = stub_system;
    gDevOk = dt_u32() % 4 == 0 ? dt_u32() : 0xffffffffu;
    gDevValue = 0;
    rwEngineInstanceCount = dt_range(0, 2);
    rwtEnd();
}

static void setup_get_metrics(DtCtx *c)
{
    RwGlobals *g = device_env(c);
    g->metrics = (int32_t *)(uintptr_t)dt_u32();
    rwtEnd();
}

static void setup_instances(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    rwEngineInstanceCount = (int32_t)dt_u32();
    rwtEnd();
}

static void setup_memory_interface(DtCtx *c)
{
    device_env(c);
}

static void setup_nothing(DtCtx *c)
{
    dt_seed(c);
}

/* ---- RwEngineInit and the memory interface ---------------------------------------------------------- */

static int BMX_CDECL stub_ext_attach(void)
{
    rwtLog(0x4eac90);
    return (int)(dt_u32() % 5) - 1;
}

static int BMX_CDECL stub_fs_open_result(void)
{
    rwtLog(0x4e82e0);
    return dt_u32() % 6 != 0;
}

static void setup_init(DtCtx *c)
{
    dt_seed(c);
    gRwtLogN = 0;
    RwMemoryFunctions *funcs = dt_alloc(sizeof *funcs);
    funcs->rwmalloc = rwtMalloc;
    funcs->rwfree = rwtFree;
    funcs->rwrealloc = NULL;
    funcs->rwcalloc = NULL;
    rwEngineInstanceCount = dt_u32() % 9 == 0 ? 1 : 0;
    dt_stub(0x004eac90, NULL, (void *)stub_ext_attach);
    dt_stub(0x004e82e0, NULL, (void *)stub_fs_open_result);
    DT_ARG_PTR(c, 0, dt_u32() % 2 ? funcs : NULL);
    DT_ARG_INT(c, 1, dt_u32() % 4);
}

static void setup_memory_open(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    RwMemoryFunctions *funcs = dt_alloc(sizeof *funcs);
    funcs->rwmalloc = rwtMalloc;
    funcs->rwfree = rwtFree;
    funcs->rwrealloc = (RwMemReallocFunc)(uintptr_t)dt_u32();
    funcs->rwcalloc = (RwMemCallocFunc)(uintptr_t)dt_u32();
    dt_fill(&rwFreeListHead, sizeof rwFreeListHead);
    DT_ARG_PTR(c, 0, dt_u32() % 2 ? funcs : NULL);
    rwtEnd();
}

static void setup_calloc(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_INT(c, 0, dt_range(0, 20));
    DT_ARG_INT(c, 1, dt_range(0, 30));
}

/* ---- resource arena --------------------------------------------------------------------------------- */

static void BMX_CDECL notify(RwResEntry *entry)
{
    rwtLog(0x707);
    rwtLogPtr(entry);
    rwtLog((uint32_t)entry->size);
}

/* A list head with n entries; the sizes add up to *total. Each entry has an owner slot pointing to it. */
static RwLLLink *make_entries(int n, int32_t *total, RwResEntry **last)
{
    RwLLLink *head = dt_alloc(sizeof *head);
    head->next = head->prev = head;
    for (int i = 0; i < n; i++) {
        RwResEntry *e = dt_alloc(sizeof *e);
        e->size = dt_range(1, 200);
        *total += e->size;
        e->ownerRef = (RwResEntry **)dt_alloc(4);
        *(RwResEntry **)e->ownerRef = e;
        e->destroyNotify = dt_u32() % 2 ? notify : NULL;
        e->link.next = head;
        e->link.prev = head->prev;
        head->prev->next = &e->link;
        head->prev = &e->link;
        *last = e;
    }
    return head;
}

static void resources_env(DtCtx *c, int32_t extra)
{
    RwGlobals *g = device_env(c);
    RwResourcesGlobals *res = rwtResources(g);
    int32_t total = 0;
    RwResEntry *last = NULL;
    res->usedEntries = make_entries(dt_range(0, 4), &total, &last);
    res->freeEntries = make_entries(dt_range(0, 3), &total, &last);
    res->currentSize = total + extra;
    res->maxSize = dt_range(0, 400);
    res->reusageSize = dt_range(0, 99);
}

static void setup_free_res_entry(DtCtx *c)
{
    dt_seed(c);
    resources_env(c, 0);
    RwResourcesGlobals *res = rwtResources(RwEngineInstance);
    RwLLLink *list = dt_u32() % 2 ? res->usedEntries : res->freeEntries;
    if (list->next == list) {
        int32_t total = 0;
        RwResEntry *e = NULL;
        RwLLLink *one = make_entries(1, &total, &e);
        res->currentSize += total;
        DT_ARG_PTR(c, 0, e);
        (void)one;
    } else {
        DT_ARG_PTR(c, 0, list->next);
    }
    rwtEnd();
}

static void setup_purge(DtCtx *c)
{
    dt_seed(c);
    resources_env(c, 0);
    rwtEnd();
}

static void setup_set_arena(DtCtx *c)
{
    dt_seed(c);
    resources_env(c, dt_u32() % 5 == 0 ? dt_range(1, 300) : 0);
    DT_ARG_INT(c, 0, dt_range(-10, 700));
    rwtEnd();
}

/* A new entry for an owner: the arena full or not, with entries to evict or none, malloc failing now and
 * then. */
static void setup_alloc_res_entry(DtCtx *c)
{
    dt_seed(c);
    resources_env(c, dt_u32() % 5 == 0 ? dt_range(1, 300) : 0);
    gRwtMallocFailAt = dt_u32() % 4 == 0 ? dt_range(1, 3) : 0;
    DT_ARG_PTR(c, 0, dt_alloc(4));
    DT_ARG_PTR(c, 1, dt_alloc(4));
    DT_ARG_INT(c, 2, dt_u32() % 8 == 0 ? 0 : dt_range(1, 450));
    DT_ARG_PTR(c, 3, dt_u32() % 2 ? notify : NULL);
    rwtEnd();
}

static void setup_use_res_entry(DtCtx *c)
{
    dt_seed(c);
    resources_env(c, 0);
    RwResourcesGlobals *res = rwtResources(RwEngineInstance);
    RwLLLink *list = dt_u32() % 2 ? res->usedEntries : res->freeEntries;
    if (list->next == list) {
        int32_t total = 0;
        RwResEntry *e = NULL;
        make_entries(1, &total, &e);
        DT_ARG_PTR(c, 0, e);
    } else {
        DT_ARG_PTR(c, 0, dt_u32() % 2 ? list->next : list->prev);
    }
    rwtEnd();
}

/* ---- render state and matrices ---------------------------------------------------------------------- */

static void setup_near_z(DtCtx *c)
{
    RwGlobals *g = device_env(c);
    const uint32_t bits = dt_u32() % 4 == 0 ? 0x7fa00000u : dt_u32(); /* includes a signalling NaN */
    __builtin_memcpy(&g->dOpenDevice.zBufferNear, &bits, 4);
    rwtEnd();
}

static void setup_render_state(DtCtx *c)
{
    RwGlobals *g = device_env(c);
    g->curCamera = dt_u32() % 3 ? dt_alloc(16) : NULL;
    DT_ARG_INT(c, 0, dt_range(0, 20));
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
    rwtEnd();
}

static void setup_im2d(DtCtx *c)
{
    device_env(c);
    DT_ARG_INT(c, 0, dt_range(0, 5));
    DT_ARG_PTR(c, 1, dt_alloc(64));
    DT_ARG_INT(c, 2, dt_range(0, 30));
    rwtEnd();
}

static void BMX_CDECL stub_multiply(RwMatrix *out, const RwMatrix *a, const RwMatrix *b)
{
    uint32_t words[16];
    rwtLog(0x3a7);
    rwtLogPtr(out);
    rwtLogPtr(a);
    rwtLogPtr(b);
    for (int i = 0; i < 16; i++) {
        words[i] = ((const uint32_t *)a)[i] * 3u + ((const uint32_t *)b)[i];
    }
    words[3] = ((const uint32_t *)a)[3] ^ ((const uint32_t *)b)[3] ^ (dt_u32() & 0x20003u);
    __builtin_memcpy(out, words, sizeof words);
}

static void random_matrix(RwMatrix *m, uint32_t identityBit)
{
    dt_fill(m, sizeof *m);
    switch (dt_u32() % 4) {
    case 0:
        m->flags = identityBit | (dt_u32() & 3u);
        break;
    case 1:
        m->flags &= ~(identityBit | 3u);
        break;
    default:
        m->flags = (m->flags & ~identityBit) | (dt_u32() & 3u);
    }
}

static void matrix_env(DtCtx *c)
{
    RwGlobals *g = device_env(c);
    uint8_t *mg = (uint8_t *)g + kRwtMatrixOffset;
    ((uint32_t *)mg)[1] = dt_u32() % 5 == 0 ? 0x30000u : 0x20000u;
    ((void **)mg)[2] = (void *)stub_multiply;
}

static void setup_matrix_update(DtCtx *c)
{
    matrix_env(c);
    RwMatrix *m = dt_alloc(sizeof *m);
    dt_fill(m, sizeof *m);
    DT_ARG_PTR(c, 0, m);
    rwtEnd();
}

static void setup_matrix_multiply(DtCtx *c)
{
    matrix_env(c);
    const uint32_t identity = ((uint32_t *)((uint8_t *)RwEngineInstance + kRwtMatrixOffset))[1];
    RwMatrix *m[3];
    for (int i = 0; i < 3; i++) {
        m[i] = dt_alloc(sizeof(RwMatrix));
        random_matrix(m[i], identity);
    }
    switch (dt_u32() % 6) {
    case 0:
        m[0] = m[1];
        break;
    case 1:
        m[0] = m[2];
        break;
    default:
        break;
    }
    DT_ARG_PTR(c, 0, m[0]);
    DT_ARG_PTR(c, 1, m[1]);
    DT_ARG_PTR(c, 2, m[2]);
    rwtEnd();
}

static int BMX_CDECL stub_ext_first(void)
{
    rwtLog(0x4eac90);
    return (int)dt_u32();
}

static void setup_ext_attach(DtCtx *c)
{
    dt_seed(c);
    gRwtLogN = 0;
    dt_stub(0x004eac90, NULL, (void *)stub_ext_first);
}

DT_CASES(rwengine_cases) = {
    {"RwEngineGetNumSubSystems", 0x004cf870, RwEngineGetNumSubSystems, 0, DT_RET_INT, setup_num_subsystems,
     60, 0},
    {"RwEngineGetSubSystemInfo", 0x004cf8b0, RwEngineGetSubSystemInfo, 2, DT_RET_INT, setup_subsystem_info,
     150, 0},
    {"RwEngineGetCurrentSubSystem", 0x004cf940, RwEngineGetCurrentSubSystem, 0, DT_RET_INT,
     setup_num_subsystems, 60, 0},
    {"RwEngineSetSubSystem", 0x004cf980, RwEngineSetSubSystem, 1, DT_RET_INT, setup_index, 100, 0},
    {"RwEngineGetNumVideoModes", 0x004cf9f0, RwEngineGetNumVideoModes, 0, DT_RET_INT, setup_num_subsystems,
     60, 0},
    {"RwEngineGetVideoModeInfo", 0x004cfa60, RwEngineGetVideoModeInfo, 2, DT_RET_INT, setup_mode_info, 100,
     0},
    {"RwEngineGetCurrentVideoMode", 0x004cfad0, RwEngineGetCurrentVideoMode, 0, DT_RET_INT,
     setup_num_subsystems, 60, 0},
    {"RwEngineSetVideoMode", 0x004cfb40, RwEngineSetVideoMode, 1, DT_RET_INT, setup_index, 100, 0},
    {"RwEngineStart", 0x004cfd50, RwEngineStart, 0, DT_RET_INT, setup_start, 300, 0},
    {"RwEngineOpen", 0x004d0020, RwEngineOpen, 1, DT_RET_INT, setup_open, 300, 0},
    {"RwEngineGetMetrics", 0x004d0120, RwEngineGetMetrics, 0, DT_RET_INT, setup_get_metrics, 20, 0},
    {"_rwGetNumEngineInstances", 0x004d0130, _rwGetNumEngineInstances, 0, DT_RET_INT, setup_instances, 20, 0},
    {"RwEngineInit", 0x004d01f0, RwEngineInit, 2, DT_RET_INT, setup_init, 200, 0},
    {"_rwDeviceGetHandle", 0x004ee140, _rwDeviceGetHandle, 0, DT_RET_INT, setup_nothing, 1, 0},
    {"_rwFSClose", 0x004e8380, _rwFSClose, 0, DT_RET_VOID, setup_nothing, 1, 0},
    {"rwCoreExtGetFuncs", 0x004eb110, rwCoreExtGetFuncs, 0, DT_RET_INT, setup_nothing, 1, 0},
    {"rwCoreExtPluginAttach", 0x004e86f0, rwCoreExtPluginAttach, 0, DT_RET_INT, setup_ext_attach, 20, 0},
    {"rwDefaultCalloc", 0x004db9a0, rwDefaultCalloc, 2, DT_RET_INT, setup_calloc, 80, 0},
    {"_rwMemoryOpen", 0x004db9e0, _rwMemoryOpen, 1, DT_RET_INT, setup_memory_open, 40, 0},
    {"_rwMemoryClose", 0x004dba70, _rwMemoryClose, 0, DT_RET_VOID, setup_nothing, 1, 0},
    {"RwOsGetMemoryInterface", 0x004dba80, RwOsGetMemoryInterface, 0, DT_RET_INT, setup_memory_interface, 10,
     0},
    {"RwResourcesFreeResEntry", 0x004dc380, RwResourcesFreeResEntry, 1, DT_RET_INT, setup_free_res_entry, 150,
     0},
    {"_rwResourcesPurge", 0x004dc3f0, _rwResourcesPurge, 0, DT_RET_VOID, setup_purge, 150, 0},
    {"RwResourcesSetArenaSize", 0x004dc6e0, RwResourcesSetArenaSize, 1, DT_RET_INT, setup_set_arena, 250, 0},
    {"RwResourcesAllocateResEntry", 0x004dc460, RwResourcesAllocateResEntry, 4, DT_RET_INT,
     setup_alloc_res_entry, 800, 0},
    {"RwResourcesUseResEntry", 0x004dc680, RwResourcesUseResEntry, 1, DT_RET_INT, setup_use_res_entry, 200,
     0},
    {"RwIm2DGetNearScreenZ", 0x004db800, RwIm2DGetNearScreenZ, 0, DT_RET_FLOAT, setup_near_z, 60, 0},
    {"RwRenderStateSet", 0x004db820, RwRenderStateSet, 2, DT_RET_INT, setup_render_state, 100, 0},
    {"RwIm2DRenderPrimitive", 0x004db920, RwIm2DRenderPrimitive, 3, DT_RET_INT, setup_im2d, 60, 0},
    {"RwMatrixUpdate", 0x004d04d0, RwMatrixUpdate, 1, DT_RET_INT, setup_matrix_update, 100, 0},
    {"RwMatrixMultiply", 0x004d04f0, RwMatrixMultiply, 3, DT_RET_INT, setup_matrix_multiply, 400, 0},
};
