/*
 * Tests for NgIFzroller.c: roller tables, actor creation, start positions and the debug-print hooks.
 */
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzres.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/com/zge/zactor.h"
#include "dt.h"
#include "dt_ngif.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zobj.h"

/* ---- fixtures ------------------------------------------------------------------------------------ */

#define MODELS ((uint32_t *)0x0068c3e0) /* 512 entries of 5 words, handle in the last */

typedef ZMemFreeList *(*FreeListCreateFn)(uint32_t, uint32_t, ZMemDestroyFn);
typedef void *(*FreeListAllocFn)(ZMemFreeList *);
typedef ZList *(*ListCreateFn)(int);
typedef void (*ListInsertFn)(ZList *, void *);

static ZMemFreeList *makePool(uint32_t elem, int live)
{
    ZMemFreeList *list = DT_ORIG(0x00485e80, FreeListCreateFn)(elem, (uint32_t)dt_range(1, 6), NULL);

    for (int i = 0; i < live; i++) {
        DT_ORIG(0x00486050, FreeListAllocFn)(list);
    }
    return list;
}

static ZList *makeEdgeList(int items)
{
    ZList *list = DT_ORIG(0x0048c260, ListCreateFn)(0x80);

    for (int i = 0; i < items; i++) {
        DT_ORIG(0x0048c460, ListInsertFn)(list, (void *)(uintptr_t)(0x2000 + i * 0x38));
    }
    return list;
}

/* ---- model table, init, startup, shutdown ---------------------------------------------------------- */

static void BMX_CDECL stubFreeHandle(int handle)
{
    ngLog(0x561024);
    ngLog((uint32_t)handle);
}

static void setup_free_models(DtCtx *c)
{
    ngBegin(c, 0);
    dt_fill(MODELS, 0x2800);
    for (int i = 0; i < 512; i++) {
        if (dt_u32() & 1) {
            MODELS[i * 5 + 4] = 0;
        }
    }
    gZrollerFreeHandle = stubFreeHandle;
}

static int BMX_CDECL stubResInit(void)
{
    ngLog(0x46be10);
    return (int)(dt_u32() & 1);
}

static void setup_init(DtCtx *c)
{
    ngBegin(c, 0);
    dt_fill(&gZrollerShadow, sizeof gZrollerShadow);
    gZbspEdgePool = (dt_u32() & 1) ? (ZMemFreeList *)(uintptr_t)dt_u32() : NULL;
    gZbspFacePool = (dt_u32() & 1) ? (ZMemFreeList *)(uintptr_t)dt_u32() : NULL;
    gZrollerEdges = (dt_u32() & 1) ? (ZList *)(uintptr_t)dt_u32() : NULL;
    dt_stub(0x0046be10, NULL, (void *)stubResInit);
}

static int BMX_CDECL stubResReset(void)
{
    ngLog(0x46bef0);
    return (int)(dt_u32() & 1);
}

static void setup_startup(DtCtx *c)
{
    ngBegin(c, 0);
    dt_fill(&gZrollerDrawCallbacks, sizeof gZrollerDrawCallbacks);
    gZrollerSlotCount = (int)dt_u32();
    gZrollerReserved = (int)dt_u32();
    dt_stub(0x0046bef0, NULL, (void *)stubResReset);
}

static void BMX_CDECL stubResShutdown(void)
{
    ngLog(0x46bed0);
}

static void setup_shutdown(DtCtx *c)
{
    ngBegin(c, 0);
    dt_stub(0x0046bed0, NULL, (void *)stubResShutdown);
}

/* ---- free lists -------------------------------------------------------------------------------- */

static void setup_free_node(DtCtx *c)
{
    ZMemFreeList *list;
    void *payload = NULL;

    ngBegin(c, 0);
    list = makePool(0x88, 0);
    for (int i = 0, n = dt_range(1, 5); i < n; i++) {
        void *p = DT_ORIG(0x00486050, FreeListAllocFn)(list);

        payload = (i == 0 || (dt_u32() & 1)) ? p : payload;
    }
    DT_ARG_PTR(c, 0, payload);
    DT_ARG_PTR(c, 1, list);
}

static void setup_free_lists(DtCtx *c)
{
    ngBegin(c, 0);
    gZrollerEdges = (dt_u32() % 4) ? makeEdgeList(dt_range(0, 9)) : NULL;
    gZbspFacePool = (dt_u32() % 4) ? makePool(0x88, dt_range(0, 9)) : NULL;
    gZbspEdgePool = (dt_u32() % 4) ? makePool(0x38, dt_range(0, 9)) : NULL;
}

static int BMX_CDECL drawA(void)
{
    ngLog(0xa);
    return 7;
}

static int BMX_CDECL drawB(void)
{
    ngLog(0xb);
    return 9;
}

static void setup_run_draw(DtCtx *c)
{
    ngBegin(c, 0);
    for (int i = 0; i < 8; i++) {
        const uint32_t r = dt_u32() % 3;

        gZrollerDrawCallbacks[i] = r == 0 ? NULL : (r == 1 ? drawA : drawB);
    }
}

static void setup_add_draw(DtCtx *c)
{
    int full;

    ngBegin(c, 1);
    full = dt_u32() % 4 == 0;
    for (int i = 0; i < 8; i++) {
        gZrollerDrawCallbacks[i] = (full || (dt_u32() & 1)) ? drawA : NULL;
    }
    DT_ARG_PTR(c, 0, (c->seed & 1) ? drawB : (ZrollerDrawFn)(uintptr_t)dt_u32());
}

/* ---- prop assets -------------------------------------------------------------------------------- */

static int BMX_CDECL stubBuildEdges(PhysPropObject *object, PhysEdge **out)
{
    const int n = (int)(dt_u32() % 6) - 1;

    ngLog(0x46a3f0);
    ngLogPtr(object);
    for (int i = 0; i < n && i < 4; i++) {
        out[i] = (PhysEdge *)(uintptr_t)(0x10000 + dt_u32() % 1000 * 0x38);
    }
    return n;
}

static void BMX_CDECL stubBuildColl(PhysPropNode *node)
{
    ngLog(0x468bc0);
    ngLogPtr(node);
    node->object = dt_alloc(sizeof(PhysPropObject));
}

static ZbspObjectDef gDefs[4];
static ZbspTreeObject gSources[4];

static void setup_load_assets(DtCtx *c)
{
    PhysPropNode *head = NULL;

    ngBegin(c, 0);
    gZrollerEdges = makeEdgeList(dt_range(0, 20));
    for (int i = 0; i < 4; i++) {
        memset(&gDefs[i], 0, sizeof gDefs[i]);
        gDefs[i].flags = (uint8_t)dt_u32();
        memset(&gSources[i], 0, sizeof gSources[i]);
        gSources[i].def = (dt_u32() % 5) ? &gDefs[i] : NULL;
    }
    for (int i = dt_range(0, 6); i > 0; i--) {
        PhysPropNode *n = dt_alloc(sizeof *n);

        n->next = head;
        n->object = (dt_u32() & 1) ? dt_alloc(sizeof(PhysPropObject)) : NULL;
        n->source = (dt_u32() % 6) ? &gSources[dt_range(0, 3)] : NULL;
        head = n;
    }
    *zobjPropList() = head;
    dt_stub(0x00468bc0, (void *)zbspBuildCollObject, (void *)stubBuildColl);
    dt_stub(0x0046a3f0, (void *)zcollideBuildPropEdges, (void *)stubBuildEdges);
}

/* ---- actors ------------------------------------------------------------------------------------ */

static void *gRiderObj, *gBikeObj, *gShadowObj, *gActor;

static void *newSceneObject(void)
{
    uint8_t *o = dt_alloc(0x40);

    *(uint32_t *)(o + 0x2c) = dt_u32() | 0x1000; /* model handle */
    return o;
}

static void *BMX_CDECL stubFindModel(int flags, const char *name)
{
    ngLog(0x491410);
    ngLog((uint32_t)flags);
    ngLog(name ? ngHash(name) : 0);
    if (name == NULL) {
        return gBikeObj;
    }
    if (ngHash(name) == ngHash("Rider Shadow") || ngHash(name) == ngHash("Bike Shadow")) {
        return gShadowObj;
    }
    return name[0] == 'D' && name[1] == 'M' && name[2] == 'b' ? gBikeObj : gRiderObj;
}

static void BMX_CDECL stubSetColor(void *model, const uint8_t *rgba)
{
    ngLog(0x560ef4);
    ngLogPtr(model);
    ngLog((uint32_t)rgba[0] | (uint32_t)rgba[1] << 8 | (uint32_t)rgba[2] << 16 | (uint32_t)rgba[3] << 24);
}

static void BMX_CDECL stubSetFlags(void *model, int set, int clear)
{
    ngLog(0x560ed8);
    ngLogPtr(model);
    ngLog((uint32_t)set);
    ngLog((uint32_t)clear);
}

static int BMX_CDECL stubRegister(int priority, void *callback, void *arg, int flag)
{
    ngLog(0x561470);
    ngLog((uint32_t)priority);
    ngLog(dt_canon((uint32_t)(uintptr_t)callback));
    ngLog(dt_canon((uint32_t)(uintptr_t)arg));
    ngLog((uint32_t)flag);
    return (int)(dt_u32() | 1);
}

static void BMX_CDECL stubUnregister(int handle)
{
    ngLog(0x561474);
    ngLog((uint32_t)handle);
}

static void BMX_CDECL stubClearPose(void)
{
    ngLog(0x4646f0);
}

static void BMX_CDECL stubVisible(int32_t object, int visible)
{
    ngLog(0x487600);
    ngLog((uint32_t)object);
    ngLog((uint32_t)visible);
}

static void BMX_CDECL stubSetupActor(void *actor)
{
    ngLog(0x465130);
    ngLogPtr(actor);
}

static void *BMX_CDECL stubActorCreate(int type)
{
    ngLog(0x472780);
    ngLog((uint32_t)type);
    return gActor;
}

static void BMX_CDECL stubSetSkeleton(ZActor *actor, int skeleton)
{
    ngLog(0x4761e0);
    ngLogPtr(actor);
    ngLog((uint32_t)skeleton);
}

static void actorHooks(void)
{
    gRiderObj = newSceneObject();
    gBikeObj = newSceneObject();
    gShadowObj = newSceneObject();
    gActor = (dt_u32() % 5) ? dt_alloc(sizeof(ZActor)) : NULL;
    gZobjSetColor = stubSetColor;
    gZobjSetFlags = stubSetFlags;
    gZrollerRegister = stubRegister;
    gZrollerUnregister = stubUnregister;
    gZrollerUpdateHandle = (dt_u32() & 1) ? (int)dt_u32() : 0;
    gZrollerShadow.shadow.slots = gZrollerShadowSlots;
    dt_fill(&gZrollerShadowSlots, sizeof gZrollerShadowSlots);
    dt_stub(0x004646f0, NULL, (void *)stubClearPose);
    dt_stub(0x00491410, NULL, (void *)stubFindModel);
    dt_stub(0x00487600, NULL, (void *)stubVisible);
    dt_stub(0x00472780, (void *)zactorCreate, (void *)stubActorCreate);
    dt_stub(0x004761e0, (void *)zactorSetSkeleton, (void *)stubSetSkeleton);
}

static void setup_create_player(DtCtx *c)
{
    uint16_t *def = dt_alloc(4);

    ngBegin(c, 1);
    actorHooks();
    gZrollerSlotCount = dt_range(0, 3);
    def[0] = (uint16_t)dt_range(0, 11);
    def[1] = (uint16_t)dt_range(0, 3);
    dt_stub(0x00465130, NULL, (void *)stubSetupActor);
    DT_ARG_PTR(c, 0, def);
}

static void setup_create_bike(DtCtx *c)
{
    uint32_t *def = dt_alloc(4);

    ngBegin(c, 1);
    actorHooks();
    *def = c->seed % 9 == 0 ? 0x31 : (uint32_t)dt_range(0, 0x30);
    dt_stub(0x00465340, NULL, (void *)stubSetupActor);
    DT_ARG_PTR(c, 0, def);
}

/* ---- small tables ------------------------------------------------------------------------------- */

static void setup_record(DtCtx *c)
{
    ngBegin(c, 0);
    gZrollerSlotCount = dt_range(0, 3);
    DT_ARG_INT(c, 0, dt_u32() % 3 ? (uint32_t)dt_range(0, 3) : dt_u32());
}

static void setup_reserve(DtCtx *c)
{
    ngBegin(c, 0);
    gZrollerReserved = (int)dt_u32();
    DT_ARG_INT(c, 0, dt_u32() % 3 ? (uint32_t)dt_range(0, 9) : dt_u32());
}

static void setup_reset_reserve(DtCtx *c)
{
    ngBegin(c, 0);
    gZrollerReserved = (int)dt_u32();
}

static void BMX_CDECL stubQuery(int what, float *out, int arg)
{
    ngLog(0x561488);
    ngLog((uint32_t)what);
    ngLog((uint32_t)arg);
    for (int i = 0; i < 8; i++) {
        out[i] = i == 3 || i == 7 ? 12345.0f : ngRf();
    }
}

static void setup_bounds(DtCtx *c)
{
    ngBegin(c, 0);
    dt_fill(&gZrollerBounds, sizeof gZrollerBounds);
    gZrollerQuery = stubQuery;
}

/* ---- player start -------------------------------------------------------------------------------- */

static ZrwMarker *gStart, *gTarget;

static ZrwMarker *newMarker(void)
{
    ZrwMarker *m = dt_alloc(sizeof *m);
    ZMatrix id;
    ZAngle3 angles;

    zmatrixIdentity(&id);
    angles.x = (ZAngle)dt_u32();
    angles.y = (ZAngle)dt_u32();
    angles.z = (ZAngle)dt_u32();
    if (dt_u32() % 4 == 0) {
        angles.x = 0; /* pure yaw: |q.y| is large */
        angles.z = 0;
    } else if (dt_u32() % 4 == 0) {
        angles.y = 0; /* no yaw: q.y is zero */
    }
    zmatrixRotateEuler(&m->matrix, &id, &angles);
    ngRf3(&m->matrix.pos);
    return m;
}

static int gMarkerCalls;

/* The first lookup is the start marker ("Player"), the second the target ("PlayerT"). */
static ZrwMarker *BMX_CDECL stubFindMarker(int hash, int index)
{
    ngLog(0x492130);
    ngLog((uint32_t)hash);
    ngLog((uint32_t)index);
    return gMarkerCalls++ == 0 ? gStart : gTarget;
}

static void setup_start_pos(DtCtx *c)
{
    ZVec3 *pos = dt_alloc(sizeof *pos);
    ZAngle3 *rot = dt_alloc(sizeof *rot);

    ngBegin(c, 1);
    gMarkerCalls = 0;
    gStart = c->seed % 7 == 0 ? NULL : newMarker();
    gTarget = c->seed % 2 ? NULL : newMarker();
    if (gTarget != NULL && gStart != NULL) {
        /* Keep the horizontal direction away from zero. */
        gTarget->matrix.pos.x = gStart->matrix.pos.x + (float)dt_range(-500, 500) + 0.5f;
        gTarget->matrix.pos.z = gStart->matrix.pos.z + (float)dt_range(-500, 500) + 0.5f;
    }
    dt_stub(0x00492130, NULL, (void *)stubFindMarker);
    DT_ARG_INT(c, 0, dt_u32() % 4 ? (uint32_t)dt_range(0, 1) : dt_u32());
    DT_ARG_PTR(c, 1, pos);
    DT_ARG_PTR(c, 2, rot);
}

/* ---- cases ------------------------------------------------------------------------------------- */

DT_CASES(roller_cases) = {
    {"zrollerFreeModelTable", 0x0046c1d0, zrollerFreeModelTable, 0, DT_RET_VOID, setup_free_models, 20, 0},
    {"zrollerInit", 0x0046c220, zrollerInit, 0, DT_RET_INT, setup_init, 40, 0},
    {"zrollerStartup", 0x0046c310, zrollerStartup, 0, DT_RET_INT, setup_startup, 40, 0},
    {"zrollerShutdown", 0x0046c360, zrollerShutdown, 0, DT_RET_INT, setup_shutdown, 2, 0},
    {"zrollerFreeNode", 0x0046c380, zrollerFreeNode, 2, DT_RET_VOID, setup_free_node, 40, 0},
    {"zrollerFreeLists", 0x0046c3b0, zrollerFreeLists, 0, DT_RET_VOID, setup_free_lists, 60, 0},
    {"zrollerRunDrawCallbacks", 0x0046c420, zrollerRunDrawCallbacks, 0, DT_RET_INT, setup_run_draw, 40, 0},
    {"zrollerLoadAssets", 0x0046c4b0, zrollerLoadAssets, 0, DT_RET_INT, setup_load_assets, 200, 0},
    {"zrollerCreatePlayerActor", 0x0046c580, zrollerCreatePlayerActor, 1, DT_RET_INT, setup_create_player,
     200, 0},
    {"zrollerCreateBikeActor", 0x0046c740, zrollerCreateBikeActor, 1, DT_RET_INT, setup_create_bike, 200, 0},
    {"zrollerGetRecord", 0x0046c920, zrollerGetRecord, 1, DT_RET_INT, setup_record, 100, 0},
    {"zrollerResetReserve", 0x0046cc80, zrollerResetReserve, 0, DT_RET_VOID, setup_reset_reserve, 4, 0},
    {"zrollerReserve", 0x0046ccd0, zrollerReserve, 1, DT_RET_INT, setup_reserve, 60, 0},
    {"zrollerGetWorldBounds", 0x0046ceb0, zrollerGetWorldBounds, 0, DT_RET_INT, setup_bounds, 40, 0},
    {"zrollerGetPlayerStartPos", 0x0046cf30, zrollerGetPlayerStartPos, 3, DT_RET_VOID, setup_start_pos, 400,
     0},
    {"zrollerReturnZero", 0x0046d350, zrollerReturnZero, 0, DT_RET_INT, ngSetupTrace, 2, 0},
    {"zrollerNop1", 0x0046d3a0, zrollerNop1, 0, DT_RET_VOID, ngSetupTrace, 2, 0},
    {"zrollerNop2", 0x0046d3c0, zrollerNop2, 0, DT_RET_VOID, ngSetupTrace, 2, 0},
    {"zrollerAddDrawCallback", 0x0046d690, zrollerAddDrawCallback, 1, DT_RET_VOID, setup_add_draw, 60, 0},
};

/* ---- debug-print hooks ----------------------------------------------------------------------- */

DT_CASES(zroller_trace_cases) = {
    {"zrollerUpdateZGL", 0x0046c450, zrollerUpdateZGL, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerCreateChaser", 0x0046c480, zrollerCreateChaser, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerCreateProp", 0x0046c860, zrollerCreateProp, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerSetEngineState", 0x0046c890, zrollerSetEngineState, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerGetNumPlayerDefs", 0x0046c8c0, zrollerGetNumPlayerDefs, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerLoadMdlPlayerCB", 0x0046c8f0, zrollerLoadMdlPlayerCB, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerIsPlayerClose", 0x0046c950, zrollerIsPlayerClose, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerChangePlayerModeLook", 0x0046c980, zrollerChangePlayerModeLook, 0, DT_RET_VOID, ngSetupTrace, 4,
     0},
    {"zrollerAddPlayerMatsToLoadList", 0x0046c9b0, zrollerAddPlayerMatsToLoadList, 0, DT_RET_VOID,
     ngSetupTrace, 4, 0},
    {"zrollerChangePlayerSponsor", 0x0046c9e0, zrollerChangePlayerSponsor, 0, DT_RET_VOID, ngSetupTrace, 4,
     0},
    {"zrollerChangeBoardSponsor", 0x0046ca10, zrollerChangeBoardSponsor, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerLoadMdlGenericCB", 0x0046ca40, zrollerLoadMdlGenericCB, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerLoadMdlCommon", 0x0046ca70, zrollerLoadMdlCommon, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerLoadMdlCommonMat", 0x0046caa0, zrollerLoadMdlCommonMat, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerProcessModelFlags", 0x0046cad0, zrollerProcessModelFlags, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerProcessActorDrawFlags", 0x0046cb00, zrollerProcessActorDrawFlags, 0, DT_RET_VOID, ngSetupTrace,
     4, 0},
    {"zrollerSetAnimMaterialPalette", 0x0046cb30, zrollerSetAnimMaterialPalette, 0, DT_RET_VOID, ngSetupTrace,
     4, 0},
    {"zrollerInitializeLoadingScreen", 0x0046cb60, zrollerInitializeLoadingScreen, 0, DT_RET_VOID,
     ngSetupTrace, 4, 0},
    {"zrollerShutdownLoadingScreen", 0x0046cb90, zrollerShutdownLoadingScreen, 0, DT_RET_VOID, ngSetupTrace,
     4, 0},
    {"zrollerDisplayLoadingScreen", 0x0046cbc0, zrollerDisplayLoadingScreen, 0, DT_RET_VOID, ngSetupTrace, 4,
     0},
    {"zrollerSetLocal", 0x0046cbf0, zrollerSetLocal, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerLoadModule", 0x0046cc20, zrollerLoadModule, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerLoadCodeModule", 0x0046cc50, zrollerLoadCodeModule, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerAllocMaterials", 0x0046cca0, zrollerAllocMaterials, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerAddGenericMaterialsToLoad", 0x0046cd00, zrollerAddGenericMaterialsToLoad, 0, DT_RET_INT,
     ngSetupTrace, 4, 0},
    {"zrollerInitializeMaps", 0x0046cd30, zrollerInitializeMaps, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerSetMap", 0x0046cd60, zrollerSetMap, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerLoadMaps", 0x0046cd90, zrollerLoadMaps, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerLoadMatTerrainFirstCB", 0x0046cdc0, zrollerLoadMatTerrainFirstCB, 0, DT_RET_INT, ngSetupTrace, 4,
     0},
    {"zrollerLoadMatTerrainLastCB", 0x0046cdf0, zrollerLoadMatTerrainLastCB, 0, DT_RET_INT, ngSetupTrace, 4,
     0},
    {"zrollerAssociateMapMaterials", 0x0046ce20, zrollerAssociateMapMaterials, 0, DT_RET_VOID, ngSetupTrace,
     4, 0},
    {"zrollerAssociateMaterial", 0x0046ce50, zrollerAssociateMaterial, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerAssociateMaterialPalAnimation", 0x0046ce80, zrollerAssociateMaterialPalAnimation, 0, DT_RET_VOID,
     ngSetupTrace, 4, 0},
    {"zrollerGetPlayerRestartPos", 0x0046d200, zrollerGetPlayerRestartPos, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerInitPropLoadList", 0x0046d230, zrollerInitPropLoadList, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerAddPropToLoadList", 0x0046d260, zrollerAddPropToLoadList, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerCreateProps", 0x0046d290, zrollerCreateProps, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerLoadPropMaterials", 0x0046d2c0, zrollerLoadPropMaterials, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerCreatePropActor", 0x0046d2f0, zrollerCreatePropActor, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerDestroyActor", 0x0046d320, zrollerDestroyActor, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerGetAsyncKeyState", 0x0046d370, zrollerGetAsyncKeyState, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerSelectMap", 0x0046d3e0, zrollerSelectMap, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerSelectPlayer", 0x0046d410, zrollerSelectPlayer, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerSelectBikeNum", 0x0046d440, zrollerSelectBikeNum, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zrollerSetFogDistance", 0x0046d470, zrollerSetFogDistance, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerGetFogDistance", 0x0046d4a0, zrollerGetFogDistance, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerDisableFog", 0x0046d4d0, zrollerDisableFog, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerSetFogColor", 0x0046d500, zrollerSetFogColor, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerReplaceMaterial", 0x0046d530, zrollerReplaceMaterial, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerSetShadeMode", 0x0046d560, zrollerSetShadeMode, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerStartCrossFade", 0x0046d590, zrollerStartCrossFade, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerGetSceneState", 0x0046d5c0, zrollerGetSceneState, 2, DT_RET_INT, ngSetupSceneState, 4, 0},
    {"zrollerFreeSceneState", 0x0046d600, zrollerFreeSceneState, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerSetSceneState", 0x0046d630, zrollerSetSceneState, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zrollerSelectLook", 0x0046d660, zrollerSelectLook, 0, DT_RET_INT, ngSetupTrace, 4, 0},
};
