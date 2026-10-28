/*
 * Tests for rw/world.c: cameras, clumps, lights and materials of a world, the ties, the for-all walks, world
 * destruction and the PVS plugin. The engine instance is the fixture of dt_rw.h, enlarged by the module data
 * of the world plugin; its free lists and memory interface log. The callees outside the slice (frame and
 * matrix updates, resource entries, plugin registries, texture references, the PVS sector callbacks) are
 * logging stubs.
 *
 * rpWorldDestroyBody (EAX = world) is reached through the original's stub at 0x4ba9c0, which loads the stack
 * argument and falls into it; the sector tree functions and rpPVSCameraDestroy are exercised through the
 * callers. RpPVSCreate's camera set-up (0x4ce6d0, x87 argument) is replaced by a naked stub.
 */
#include "dt_rw.h"
#include "dt_zmem.h"
#include "rw/rwworld.h"

enum {
    kEngineBytes = 0x400,
    kTieOffset = 0x200,
    kWorldGlobalsOffset = 0x210,
    kCameraData = 0xa0, /* world plugin data in a camera */
    kCameraBytes = 0xc0,
    kClumpWorld = 0x30, /* where a clump / light keeps its world */
    kClumpBytes = 0x40,
    kLightWorld = 0x48,
    kLightBytes = 0x58,
    kPvsOffset = 0x74, /* the PVS plugin's pointer in a world */
    kWorldBytes = 0x90,
    kSectorBytes = 0x60,
};

#define FREELIST_TIE      ((RwFreeList *)0x20001000)
#define FREELIST_LIGHTTIE ((RwFreeList *)0x20002000)
#define FREELIST_ENTRY    ((RwFreeList *)0x20003000)

static void listInit(RwLLLink *l)
{
    l->next = l->prev = l;
}

static void listAddTail(RwLLLink *head, RwLLLink *link)
{
    link->prev = head->prev;
    link->next = head;
    head->prev->next = link;
    head->prev = link;
}

/* ---- callbacks and stubs ---------------------------------------------------------------------------- */

static int gCbCalls, gCbStopAt; /* the walk callbacks stop (return NULL) at the n-th call; 0 never */

static void *BMX_CDECL cbWalk(void *object, void *data)
{
    rwtLog(0xc0de);
    rwtLogPtr(object);
    rwtLogPtr(data);
    gCbCalls++;
    return gCbStopAt != 0 && gCbCalls >= gCbStopAt ? NULL : (void *)1;
}

static void BMX_CDECL progress(int32_t stage, float value)
{
    rwtLog(0x9a09);
    rwtLog((uint32_t)stage);
    rwtLogFloatBits(value);
}

static void *BMX_CDECL renderCb(RpWorldSector *sector)
{
    rwtLog(0x4e4d);
    rwtLogPtr(sector);
    return sector;
}

static void *BMX_CDECL stFrameUpdate(RwFrame *frame)
{
    rwtLog(0x4d2a80);
    rwtLogPtr(frame);
    return frame;
}

static void *BMX_CDECL stMatrixUpdate(RwMatrix *matrix)
{
    rwtLog(0x4d04d0);
    rwtLogPtr(matrix);
    return matrix;
}

static void *BMX_CDECL stForAllAtomics(RpClump *clump, RwObjectCallBack callback, void *data)
{
    rwtLog(0x4b1990);
    rwtLogPtr(clump);
    rwtLog((uint32_t)(uintptr_t)callback);
    rwtLogPtr(data);
    return clump;
}

static int BMX_CDECL stFreeRes(RwResEntry *entry)
{
    rwtLog(0x4dc380);
    rwtLogPtr(entry);
    return 1;
}

static void *BMX_CDECL stDeInit(RwPluginRegistry *registry, void *object)
{
    rwtLog(0x4e8ab0);
    rwtLogPtr(registry);
    rwtLogPtr(object);
    return registry;
}

static int BMX_CDECL stSectorData(void *data)
{
    rwtLog(0x4c4870);
    rwtLogPtr(data);
    return 1;
}

static int BMX_CDECL stMatListDeinit(RpMaterialList *list)
{
    rwtLog(0x4c5640);
    rwtLogPtr(list);
    rwtLog((uint32_t)list->numMaterials);
    return 1;
}

/* The real RpWorldSetRenderCallBack: stores the callback and bumps the world's stamp. */
static RpWorld *BMX_CDECL stSetRenderCb(RpWorld *world, RpWorldSectorCallBackRender callback)
{
    rwtLog(0x4baf20);
    rwtLogPtr(world);
    rwtLog((uint32_t)(uintptr_t)callback);
    world->renderCallBack = callback;
    world->stamp++;
    return world;
}

static void *BMX_CDECL stTexAddRef(RwTexture *texture)
{
    rwtLog(0x4df980);
    rwtLogPtr(texture);
    return texture;
}

static int BMX_CDECL stTexDestroy(RwTexture *texture)
{
    rwtLog(0x4df760);
    rwtLogPtr(texture);
    return 1;
}

static void *BMX_CDECL stSectorRender(RpWorldSector *sector, RpWorld *world)
{
    rwtLog(0x4ba430);
    rwtLogPtr(sector);
    rwtLogPtr(world);
    return sector;
}

static void stubFrames(void)
{
    dt_stub(0x004d2a80, NULL, (void *)stFrameUpdate);
}

/* The PVS sector callbacks (code addresses the world passes to its walks). */
static void *BMX_CDECL stPvsPrepare(RpWorldSector *sector, RpPVSWorldData *pvs)
{
    rwtLog(0x4ce620);
    rwtLogPtr(sector);
    pvs->numEntries += 1 + (int32_t)(((uint32_t)(uintptr_t)sector >> 4) % 3);
    return sector;
}

static void *BMX_CDECL stPvsSample(RpWorldSector *sector, const uint32_t *args)
{
    rwtLog(0x4ce250);
    rwtLogPtr(sector);
    rwtLogPtr((const void *)(uintptr_t)args[0]);
    rwtLogPtr((const void *)(uintptr_t)args[1]);
    return sector;
}

static void *BMX_CDECL stPvsCount(RpWorldSector *sector, int32_t *count)
{
    rwtLog(0x4ceef0);
    rwtLogPtr(sector);
    *count += sector->type == -1 ? 1 : 2;
    return sector;
}

static void *BMX_CDECL stPvsUnload(RpWorldSector *sector, void *data)
{
    rwtLog(0x4cc300);
    rwtLogPtr(sector);
    rwtLogPtr(data);
    return sector;
}

static void *BMX_CDECL stPvsView(RpIntersection *isect, RpWorldSector *sector, RpPVSWorldData *pvs)
{
    rwtLog(0x4cc4e0);
    rwtLogPtr(isect);
    rwtLog((uint32_t)isect->t.pointBits[0]);
    rwtLog((uint32_t)isect->t.pointBits[1]);
    rwtLog((uint32_t)isect->t.pointBits[2]);
    rwtLog((uint32_t)isect->type);
    rwtLogPtr(sector);
    rwtLogPtr(pvs);
    return sector;
}

static void stubPvsCallbacks(void)
{
    dt_stub(0x004ce620, NULL, (void *)stPvsPrepare);
    dt_stub(0x004ce250, NULL, (void *)stPvsSample);
    dt_stub(0x004ceef0, NULL, (void *)stPvsCount);
    dt_stub(0x004cc300, NULL, (void *)stPvsUnload);
    dt_stub(0x004cc4e0, NULL, (void *)stPvsView);
}

/* ---- fixtures --------------------------------------------------------------------------------------- */

/*
 * The engine instance with the world plugin's data (tie free lists, the list of worlds and its free list)
 * and the module open (or, now and then when `maybeClosed`, not).
 */
static RwGlobals *worldEnv(const DtCtx *c, int maybeClosed)
{
    dt_seed(c);
    RwGlobals *small = rwtEnv();
    RwGlobals *g = dt_alloc(kEngineBytes);
    __builtin_memcpy(g, small, kRwtEngineBytes);
    RwEngineInstance = g;
    rpTieOffset = kTieOffset;
    rpWorldGlobalsOffset = kWorldGlobalsOffset;
    rpWorldCameraOffset = kCameraData;
    rpWorldClumpOffset = kClumpWorld;
    rpWorldLightOffset = kLightWorld;
    rpPVSWorldOffset = kPvsOffset;
    rpWorldModuleCount = maybeClosed && dt_u32() % 6 == 0 ? 0 : 1;
    RpTieGlobals *ties = (RpTieGlobals *)((uint8_t *)g + kTieOffset);
    ties->tieFreeList = FREELIST_TIE;
    ties->lightTieFreeList = FREELIST_LIGHTTIE;
    RpWorldGlobals *worlds = (RpWorldGlobals *)((uint8_t *)g + kWorldGlobalsOffset);
    worlds->entryFreeList = FREELIST_ENTRY;
    listInit(&worlds->worlds);
    rpPVSWorld = NULL;
    rpPVSCamera = NULL;
    rpPVSImage = NULL;
    return g;
}

static RpWorld *mkWorld(void)
{
    RpWorld *w = dt_alloc(kWorldBytes);
    w->object.type = 7;
    w->object.privateFlags = (uint8_t)dt_u32();
    w->flags = dt_u32();
    w->matList.numMaterials = dt_range(0, 5);
    w->matList.space = dt_range(0, 9);
    w->matList.materials = (RpMaterial **)dt_alloc(5 * sizeof(void *));
    for (int i = 0; i < 5; i++) {
        w->matList.materials[i] = dt_u32() % 5 == 0 ? NULL : dt_alloc(0x10);
    }
    w->numClumpsInWorld = dt_range(-2, 9);
    listInit(&w->clumpList);
    listInit(&w->lightList);
    listInit(&w->directionalLightList);
    w->currentClumpLink = (RwLLLink *)(uintptr_t)(dt_u32() & ~3U);
    w->worldOrigin.x = dt_float_edgy(-500.0F, 500.0F);
    w->worldOrigin.y = dt_float_edgy(-500.0F, 500.0F);
    w->worldOrigin.z = dt_float_edgy(-500.0F, 500.0F);
    w->boundingBox.sup.x = dt_float_edgy(-500.0F, 500.0F);
    w->boundingBox.inf.z = dt_float_edgy(-500.0F, 500.0F);
    w->renderCallBack = dt_u32() % 3 == 0 ? NULL : renderCb;
    w->stamp = (uint8_t)dt_u32();
    return w;
}

static RpPVSWorldData *mkPvs(RpWorld *world)
{
    RpPVSWorldData *p = dt_alloc(0x60);
    p->numSectors = dt_range(0, 40);
    p->visibility = dt_u32() % 3 == 0 ? NULL : dt_alloc(8);
    p->rowBuffer = dt_u32() % 3 == 0 ? NULL : dt_alloc(8);
    p->savedRenderCallBack = dt_u32() % 2 == 0 ? NULL : renderCb;
    p->hooked = (int32_t)(dt_u32() % 2);
    p->progressCallBack = dt_u32() % 3 == 0 ? NULL : progress;
    p->numEntries = dt_range(-1, 6);
    p->entries = dt_u32() % 2 == 0 ? NULL : dt_alloc(8);
    p->entryIndices = dt_u32() % 2 == 0 ? NULL : dt_alloc(8);
    p->statsA = dt_u32();
    p->statsB = dt_u32();
    *(RpPVSWorldData **)((uint8_t *)world + kPvsOffset) = p;
    return p;
}

static RwFrame *mkFrame(void)
{
    RwFrame *f = dt_alloc(sizeof(RwFrame));
    listInit(&f->objectList);
    return f;
}

static RwCamera *mkCamera(RpWorld *world, int sectors)
{
    RwCamera *cam = dt_alloc(kCameraBytes);
    cam->object.object.type = 4;
    cam->object.object.parent = dt_u32() % 3 == 0 ? NULL : mkFrame();
    cam->version = (uint8_t)dt_u32();
    RpWorldCameraData *data = (RpWorldCameraData *)((uint8_t *)cam + kCameraData);
    data->world = world;
    if (sectors > 0) {
        data->sectors = (RpWorldSector **)dt_alloc((uint32_t)sectors * sizeof(void *));
        for (int i = 0; i < sectors; i++) {
            data->sectors[i] = dt_alloc(kSectorBytes);
        }
        data->numSectors = sectors;
        data->capacity = sectors + dt_range(0, 3);
    } else {
        data->sectors = dt_u32() % 2 == 0 ? NULL : (RpWorldSector **)dt_alloc(8);
        data->numSectors = dt_range(0, 1) * 0;
    }
    return cam;
}

static RpClump *mkClump(RpWorld *world)
{
    RpClump *clump = dt_alloc(kClumpBytes);
    clump->object.type = 2;
    clump->object.parent = dt_u32() % 3 == 0 ? NULL : mkFrame();
    listInit(&clump->atomicList);
    *(RpWorld **)((uint8_t *)clump + kClumpWorld) = world;
    *((uint8_t *)clump + 0x12) = (uint8_t)dt_u32();
    return clump;
}

/* Puts the clump into the world's list (at the front, like RpWorldAddClump). */
static void clumpIntoWorld(RpWorld *world, RpClump *clump)
{
    RwLLLink *link = (RwLLLink *)((uint8_t *)clump + 0x14);
    link->next = world->clumpList.next;
    link->prev = &world->clumpList;
    world->clumpList.next->prev = link;
    world->clumpList.next = link;
}

static RpLight *mkLight(RpWorld *world, uint32_t type)
{
    RpLight *l = dt_alloc(kLightBytes);
    l->object.object.type = 3;
    l->object.object.subType = (uint8_t)type;
    l->object.object.parent = dt_u32() % 3 == 0 ? NULL : mkFrame();
    l->stamp = (uint8_t)dt_u32();
    listInit(&l->sectors);
    listInit(&l->inWorld);
    *(RpWorld **)((uint8_t *)l + kLightWorld) = world;
    return l;
}

static uint32_t lightType(void)
{
    static const uint32_t types[] = {1, 2, 0x80, 0x81, 0x82, 0x7f, 0xff, 0};
    return types[dt_u32() % 8];
}

/* A leaf sector (type -1, or -2: a stub sector) with random blocks and ties in its three lists. */
static RpWorldSector *mkLeaf(int32_t type)
{
    RpWorldSector *s = dt_alloc(kSectorBytes);
    s->type = type;
    s->repEntry = dt_u32() % 2 == 0 ? NULL : (RwResEntry *)dt_alloc(0x18);
    void **blocks[] = {(void **)&s->vertices,  (void **)&s->normals,  (void **)&s->preLitLum,
                       (void **)&s->texCoords, (void **)&s->polygons, (void **)&s->colSectorRoot};
    for (int i = 0; i < 6; i++) {
        *blocks[i] = dt_u32() % 3 == 0 ? NULL : dt_alloc(8);
    }
    s->mesh = dt_u32() % 2 == 0 ? NULL : dt_alloc(8);
    listInit(&s->collAtomicsInWorldSector);
    listInit(&s->noCollAtomicsInWorldSector);
    listInit(&s->lightsInWorldSector);
    if (type != -1) {
        return s;
    }
    RwLLLink *lists[3] = {&s->collAtomicsInWorldSector, &s->noCollAtomicsInWorldSector,
                          &s->lightsInWorldSector};
    for (int i = 0; i < 3; i++) {
        const int n = dt_range(0, 3);
        RwLLLink *other = dt_alloc(sizeof(RwLLLink)); /* the atomic's / light's own list */
        listInit(other);
        for (int k = 0; k < n; k++) {
            if (i < 2) {
                RpTie *tie = dt_alloc(sizeof(RpTie));
                tie->apAtom = dt_u32() % 4 == 0 ? NULL : (RpAtomic *)dt_alloc(0x70);
                tie->worldSector = dt_u32() % 4 == 0 ? NULL : s;
                listAddTail(lists[i], &tie->lAtomicInWorldSector);
                listAddTail(other, &tie->lWorldSectorInAtomic);
            } else {
                RpLightTie *tie = dt_alloc(sizeof(RpLightTie));
                tie->light = (RpLight *)dt_alloc(8);
                tie->worldSector = s;
                listAddTail(lists[i], &tie->lightInWorldSector);
                listAddTail(other, &tie->worldSectorInLight);
            }
        }
    }
    return s;
}

/* A random BSP tree: plane sectors with integer-ordered plane values, leaves of both kinds. */
static RpSector *mkTree(int depth)
{
    if (depth == 0 || dt_u32() % 3 == 0) {
        return (RpSector *)mkLeaf(dt_u32() % 8 == 0 ? -2 : -1);
    }
    RpPlaneSector *p = dt_alloc(kSectorBytes);
    p->type = 4 * dt_range(0, 2);
    p->leftSubTree = mkTree(depth - 1);
    p->rightSubTree = mkTree(depth - 1);
    const int32_t a = dt_range(-3, 12);
    const int32_t b = dt_range(-3, 12);
    __builtin_memcpy(&p->leftValue, &a, 4);
    __builtin_memcpy(&p->rightValue, &b, 4);
    return (RpSector *)p;
}

/* ---- cameras ---------------------------------------------------------------------------------------- */

static void setup_add_camera(DtCtx *c)
{
    worldEnv(c, 0);
    stubFrames();
    RpWorld *w = mkWorld();
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : w);
    DT_ARG_PTR(c, 1, mkCamera(NULL, 0));
    rwtEnd();
}

static void setup_remove_camera(DtCtx *c)
{
    worldEnv(c, 0);
    RpWorld *w = mkWorld();
    DT_ARG_PTR(c, 0, w);
    DT_ARG_PTR(c, 1, mkCamera(dt_u32() % 4 == 0 ? NULL : w, dt_range(0, 3)));
    rwtEnd();
}

static void setup_camera_world(DtCtx *c)
{
    worldEnv(c, 0);
    DT_ARG_PTR(c, 0, mkCamera(dt_u32() % 3 == 0 ? NULL : mkWorld(), 0));
    rwtEnd();
}

static void setup_camera_data(DtCtx *c)
{
    worldEnv(c, 0);
    RwCamera *cam = mkCamera(mkWorld(), dt_range(0, 3));
    DT_ARG_PTR(c, 0, (uint8_t *)cam + kCameraData);
    rwtEnd();
}

static void setup_for_all_sectors(DtCtx *c)
{
    worldEnv(c, 0);
    gCbCalls = 0;
    gCbStopAt = dt_u32() % 2 == 0 ? 0 : dt_range(1, 4);
    DT_ARG_PTR(c, 0, mkCamera(mkWorld(), dt_range(0, 5)));
    DT_ARG_PTR(c, 1, cbWalk);
    DT_ARG_PTR(c, 2, dt_alloc(8));
    rwtEnd();
}

static void setup_world_render(DtCtx *c)
{
    RwGlobals *g = worldEnv(c, 1);
    dt_stub(0x004ba430, NULL, (void *)stSectorRender);
    RpWorld *w = mkWorld();
    g->curCamera = mkCamera(w, dt_range(0, 4));
    DT_ARG_PTR(c, 0, dt_u32() % 6 == 0 ? NULL : w);
    rwtEnd();
}

/* ---- ties, clumps, lights ---------------------------------------------------------------------------- */

static void setup_light_tie(DtCtx *c)
{
    worldEnv(c, 0);
    RpWorldSector *s = mkLeaf(-1);
    RwLLLink *head = &s->lightsInWorldSector;
    if (head->next == head) {
        RpLightTie *tie = dt_alloc(sizeof(RpLightTie));
        RwLLLink *other = dt_alloc(sizeof(RwLLLink));
        listInit(other);
        listAddTail(head, &tie->lightInWorldSector);
        listAddTail(other, &tie->worldSectorInLight);
    }
    DT_ARG_PTR(c, 0, head->next);
    rwtEnd();
}

static void setup_tie(DtCtx *c)
{
    worldEnv(c, 0);
    RpWorldSector *s = mkLeaf(-1);
    RwLLLink *head = &s->collAtomicsInWorldSector;
    if (head->next == head) {
        RpTie *tie = dt_alloc(sizeof(RpTie));
        tie->apAtom = (RpAtomic *)dt_alloc(8);
        tie->worldSector = s;
        RwLLLink *other = dt_alloc(sizeof(RwLLLink));
        listInit(other);
        listAddTail(head, &tie->lAtomicInWorldSector);
        listAddTail(other, &tie->lWorldSectorInAtomic);
    }
    DT_ARG_PTR(c, 0, head->next);
    rwtEnd();
}

static void clumpStubs(void)
{
    stubFrames();
    dt_stub(0x004b1990, NULL, (void *)stForAllAtomics);
    dt_stub(0x004d04d0, NULL, (void *)stMatrixUpdate);
}

static void setup_add_clump(DtCtx *c)
{
    worldEnv(c, 0);
    clumpStubs();
    RpWorld *w = mkWorld();
    RpWorld *other = mkWorld();
    RpClump *clump = mkClump(dt_u32() % 4 == 0 ? other : NULL);
    clumpIntoWorld(w, mkClump(w));
    DT_ARG_PTR(c, 0, w);
    DT_ARG_PTR(c, 1, clump);
    rwtEnd();
}

static void setup_remove_clump(DtCtx *c)
{
    worldEnv(c, 0);
    clumpStubs();
    RpWorld *w = mkWorld();
    RpWorld *clumpWorld = dt_u32() % 5 == 0 ? NULL : mkWorld();
    RpClump *clump = mkClump(clumpWorld);
    if (clumpWorld != NULL) {
        RpClump *another = mkClump(clumpWorld);
        clumpIntoWorld(clumpWorld, another);
        clumpIntoWorld(clumpWorld, clump);
        if (dt_u32() % 3 == 0) {
            clumpWorld->currentClumpLink = (RwLLLink *)((uint8_t *)clump + 0x14);
        }
    }
    DT_ARG_PTR(c, 0, w);
    DT_ARG_PTR(c, 1, clump);
    rwtEnd();
}

static void setup_clump_world(DtCtx *c)
{
    worldEnv(c, 0);
    DT_ARG_PTR(c, 0, mkClump(dt_u32() % 3 == 0 ? NULL : mkWorld()));
    rwtEnd();
}

static void setup_add_light(DtCtx *c)
{
    worldEnv(c, 0);
    stubFrames();
    RpWorld *w = mkWorld();
    DT_ARG_PTR(c, 0, w);
    DT_ARG_PTR(c, 1, mkLight(NULL, lightType()));
    rwtEnd();
}

static void setup_remove_light(DtCtx *c)
{
    worldEnv(c, 0);
    RpWorld *w = mkWorld();
    RpLight *l = mkLight(w, lightType());
    listAddTail(&w->lightList, &l->inWorld);
    listAddTail(&w->lightList, &mkLight(NULL, 0x80)->inWorld);
    const int n = dt_range(0, 3);
    for (int i = 0; i < n; i++) {
        RpWorldSector *s = (RpWorldSector *)mkLeaf(-1);
        RpLightTie *tie = dt_alloc(sizeof(RpLightTie));
        tie->light = l;
        tie->worldSector = s;
        listAddTail(&s->lightsInWorldSector, &tie->lightInWorldSector);
        listAddTail(&l->sectors, &tie->worldSectorInLight);
    }
    DT_ARG_PTR(c, 0, w);
    DT_ARG_PTR(c, 1, l);
    rwtEnd();
}

/* ---- materials ---------------------------------------------------------------------------------------- */

static RpMaterial *mkMaterial(void)
{
    RpMaterial *m = dt_alloc(sizeof(RpMaterial) + 4);
    m->texture = dt_u32() % 3 == 0 ? NULL : dt_alloc(0x28);
    m->color.red = (uint8_t)dt_u32();
    m->color.blue = (uint8_t)dt_u32();
    m->stamp = (uint8_t)dt_u32();
    return m;
}

static void setup_set_color(DtCtx *c)
{
    worldEnv(c, 0);
    DT_ARG_PTR(c, 0, mkMaterial());
    RwRGBA *color = dt_alloc(4);
    dt_fill(color, 4);
    DT_ARG_PTR(c, 1, color);
    rwtEnd();
}

static void setup_set_texture(DtCtx *c)
{
    worldEnv(c, 0);
    dt_stub(0x004df980, NULL, (void *)stTexAddRef);
    dt_stub(0x004df760, NULL, (void *)stTexDestroy);
    DT_ARG_PTR(c, 0, mkMaterial());
    DT_ARG_PTR(c, 1, dt_u32() % 3 == 0 ? NULL : dt_alloc(0x28));
    rwtEnd();
}

static void setup_get_texture(DtCtx *c)
{
    worldEnv(c, 0);
    DT_ARG_PTR(c, 0, mkMaterial());
    rwtEnd();
}

/* ---- the world ---------------------------------------------------------------------------------------- */

/* The getters share their guard: module closed, world missing. */
static void setup_getter(DtCtx *c)
{
    worldEnv(c, 1);
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : mkWorld());
    rwtEnd();
}

static void setup_get_material(DtCtx *c)
{
    worldEnv(c, 1);
    RpWorld *w = dt_u32() % 6 == 0 ? NULL : mkWorld();
    DT_ARG_PTR(c, 0, w);
    DT_ARG_INT(c, 1, dt_u32() % 8 == 0 ? dt_u32() : (uint32_t)dt_range(-2, 7));
    rwtEnd();
}

static void setup_for_all_clumps(DtCtx *c)
{
    worldEnv(c, 1);
    gCbCalls = 0;
    gCbStopAt = dt_u32() % 2 == 0 ? 0 : dt_range(1, 3);
    RpWorld *w = mkWorld();
    const int n = dt_range(0, 4);
    for (int i = 0; i < n; i++) {
        clumpIntoWorld(w, mkClump(w));
    }
    DT_ARG_PTR(c, 0, dt_u32() % 6 == 0 ? NULL : w);
    DT_ARG_PTR(c, 1, cbWalk);
    DT_ARG_PTR(c, 2, dt_alloc(8));
    rwtEnd();
}

static void setup_for_all_lights(DtCtx *c)
{
    worldEnv(c, 1);
    gCbCalls = 0;
    gCbStopAt = dt_u32() % 2 == 0 ? 0 : dt_range(1, 5);
    RpWorld *w = mkWorld();
    for (int i = 0; i < 3; i++) {
        if (dt_u32() % 3 != 0) {
            listAddTail(i == 0 ? &w->lightList : &w->directionalLightList, &mkLight(w, 0x80)->inWorld);
        }
        listAddTail(&w->directionalLightList, &mkLight(w, 1)->inWorld);
    }
    DT_ARG_PTR(c, 0, dt_u32() % 6 == 0 ? NULL : w);
    DT_ARG_PTR(c, 1, cbWalk);
    DT_ARG_PTR(c, 2, dt_alloc(8));
    rwtEnd();
}

static void setup_for_all_sectors_world(DtCtx *c)
{
    worldEnv(c, 0);
    gCbCalls = 0;
    gCbStopAt = dt_u32() % 2 == 0 ? 0 : dt_range(1, 6);
    RpWorld *w = mkWorld();
    w->rootSector = mkTree(dt_range(0, 3));
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : w);
    DT_ARG_PTR(c, 1, dt_u32() % 8 == 0 ? NULL : (void *)cbWalk);
    DT_ARG_PTR(c, 2, dt_alloc(8));
    rwtEnd();
}

/* ---- destroying a world ------------------------------------------------------------------------------- */

static RpWorld *destroyTarget(RwGlobals *g)
{
    RpWorld *w = mkWorld();
    w->rootSector = mkTree(dt_range(0, 3));
    RpWorldGlobals *globals = (RpWorldGlobals *)((uint8_t *)g + kWorldGlobalsOffset);
    const int n = dt_range(0, 3);
    const int at = dt_range(0, n);
    for (int i = 0; i <= n; i++) {
        RpWorldListEntry *e = dt_alloc(sizeof(RpWorldListEntry));
        e->world = i == at && dt_u32() % 5 != 0 ? w : mkWorld();
        listAddTail(&globals->worlds, &e->link);
    }
    return w;
}

static int BMX_CDECL testWorldDestroy(RpWorld *world)
{
    return rpWorldDestroyBody(world);
}

static void setup_destroy(DtCtx *c)
{
    RwGlobals *g = worldEnv(c, 1);
    dt_stub(0x004dc380, NULL, (void *)stFreeRes);
    dt_stub(0x004e8ab0, NULL, (void *)stDeInit);
    dt_stub(0x004c4870, NULL, (void *)stSectorData);
    dt_stub(0x004c5640, NULL, (void *)stMatListDeinit);
    RpWorld *w = destroyTarget(g);
    DT_ARG_PTR(c, 0, dt_u32() % 10 == 0 ? NULL : w);
    rwtEnd();
}

/* ---- walking the BSP ---------------------------------------------------------------------------------- */

static RpTie *gTies[4];
static int gNumTies, gCbMode;
static int gRemoved[4];

static void unlinkTie(int i)
{
    if (!gRemoved[i]) {
        gRemoved[i] = 1;
        RwLLLink *l = &gTies[i]->lWorldSectorInAtomic;
        l->prev->next = l->next;
        l->next->prev = l->prev;
        /* a removed link may still point at the walk's marker (a stack address): clear it */
        l->next = l->prev = NULL;
    }
}

/* The atomic walk's callback: logs, and (by mode) unlinks the sector's tie and the next one. */
static void *BMX_CDECL cbIsect(RpIntersection *isect, RpWorldSector *sector, void *data)
{
    rwtLog(0x15ec);
    rwtLogPtr(isect);
    rwtLogPtr(sector);
    rwtLogPtr(data);
    gCbCalls++;
    int me = 0;
    while (me < gNumTies && gTies[me]->worldSector != sector) {
        me++;
    }
    if ((gCbMode & 1) && me < gNumTies) {
        unlinkTie(me);
    }
    if ((gCbMode & 2) && me + 1 < gNumTies) {
        unlinkTie(me + 1);
    }
    return gCbStopAt != 0 && gCbCalls >= gCbStopAt ? NULL : (void *)1;
}

/* The line and ray walks stay original code; stubs with the register convention they are called with. */
__attribute__((regparm(3))) static void *stLineWalk(RpWorld *world, RpIntersection *isect,
                                                    RpIntersectionCallBackWorldSector cb, int32_t a,
                                                    int32_t b, int32_t d, RpIntersection *again, void *data)
{
    (void)a;
    (void)b;
    (void)d;
    rwtLog(0x4bcd90);
    rwtLogPtr(world);
    rwtLogPtr(isect);
    rwtLogPtr(again);
    rwtLog((uint32_t)(uintptr_t)cb);
    rwtLogPtr(data);
    rwtLog((uint32_t)isect->type);
    return world;
}

static void setup_intersect(DtCtx *c)
{
    worldEnv(c, 0);
    gCbCalls = 0;
    gCbMode = 0;
    gCbStopAt = 0;
    gNumTies = 0;
    __builtin_memset(gRemoved, 0, sizeof gRemoved);
    dt_stub(0x004bcd90, NULL, (void *)stLineWalk);
    dt_stub(0x004bda80, NULL, (void *)stLineWalk);
    RpWorld *w = mkWorld();
    w->rootSector = mkTree(dt_range(0, 4));
    RpIntersection *isect = dt_alloc(sizeof(RpIntersection));
    static const int32_t types[] = {2, 2, 2, 2, 1, 3, 5, 5, 5, 4, 0, 6};
    isect->type = types[dt_u32() % 12];
    DT_ARG_PTR(c, 0, w);
    DT_ARG_PTR(c, 1, isect);
    DT_ARG_PTR(c, 3, dt_alloc(8));
    if (isect->type == rpINTERSECTATOMIC) {
        /* an atomic tied to a few sectors; the walk may stop or unlink while it goes */
        uint8_t *atomic = dt_alloc(0x80);
        RwLLLink *head = (RwLLLink *)(atomic + 0x68);
        listInit(head);
        gNumTies = dt_range(0, 4);
        for (int i = 0; i < gNumTies; i++) {
            gTies[i] = dt_alloc(sizeof(RpTie));
            gTies[i]->worldSector = (RpWorldSector *)mkLeaf(-1);
            listAddTail(head, &gTies[i]->lWorldSectorInAtomic);
        }
        isect->t.object = (RpAtomic *)atomic;
        gCbMode = dt_range(0, 3);
        gCbStopAt = dt_u32() % 2 == 0 ? 0 : dt_range(1, 3);
        DT_ARG_PTR(c, 2, cbIsect);
    } else {
        /* point coordinates: ints around the plane values */
        for (int i = 0; i < 3; i++) {
            isect->t.pointBits[i] = dt_range(-4, 13);
        }
        DT_ARG_PTR(c, 2, stPvsView);
    }
    rwtEnd();
}

/* ---- the PVS plugin ----------------------------------------------------------------------------------- */

static RpWorld *pvsWorld(int withData)
{
    RpWorld *w = mkWorld();
    w->rootSector = mkTree(dt_range(0, 3));
    if (withData) {
        mkPvs(w);
    }
    return w;
}

static void setup_pvs_progress(DtCtx *c)
{
    worldEnv(c, 0);
    RpWorld *w = pvsWorld(dt_u32() % 4 != 0);
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : w);
    DT_ARG_PTR(c, 1, dt_u32() % 3 == 0 ? NULL : (void *)progress);
    rwtEnd();
}

static void setup_pvs_hook(DtCtx *c)
{
    worldEnv(c, 0);
    dt_stub(0x004baf20, NULL, (void *)stSetRenderCb);
    RpWorld *w = pvsWorld(1);
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : w);
    rwtEnd();
}

static void setup_pvs_unload(DtCtx *c)
{
    worldEnv(c, 0);
    stubPvsCallbacks();
    dt_stub(0x004baf20, NULL, (void *)stSetRenderCb);
    RpWorld *w = pvsWorld(1);
    DT_ARG_PTR(c, 0, dt_u32() % 6 == 0 ? NULL : w);
    rwtEnd();
}

static void setup_pvs_view(DtCtx *c)
{
    worldEnv(c, 0);
    stubPvsCallbacks();
    RpWorld *w = pvsWorld(1);
    int32_t *pos = dt_alloc(12);
    for (int i = 0; i < 3; i++) {
        pos[i] = dt_range(-4, 13);
    }
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : w);
    DT_ARG_PTR(c, 1, dt_u32() % 5 == 0 ? NULL : pos);
    rwtEnd();
}

static void setup_pvs_query(DtCtx *c)
{
    worldEnv(c, 0);
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : pvsWorld(1));
    rwtEnd();
}

static void setup_pvs_stats(DtCtx *c)
{
    worldEnv(c, 0);
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : pvsWorld(1));
    DT_ARG_PTR(c, 1, dt_alloc(4));
    DT_ARG_PTR(c, 2, dt_alloc(4));
    rwtEnd();
}

static void setup_pvs_count(DtCtx *c)
{
    worldEnv(c, 0);
    stubPvsCallbacks();
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : pvsWorld(0));
    rwtEnd();
}

/* PVS generation: the camera set-up (0x4ce6d0) hands out a fixture camera and image. */
static RwCamera *gFixCamera;
static RwImage *gFixImage;

static void BMX_CDECL stCameraCreateC(RpWorld *world, RwRaster *raster, RwRaster *zRaster,
                                      float farClip) __asm__("_stCameraCreateC");
__attribute__((used)) static void BMX_CDECL stCameraCreateC(RpWorld *world, RwRaster *raster,
                                                            RwRaster *zRaster, float farClip)
{
    rwtLog(0x4ce6d0);
    rwtLogPtr(world);
    rwtLogPtr(raster);
    rwtLogPtr(zRaster);
    rwtLogFloatBits(farClip);
    rpPVSCamera = gFixCamera;
    rpPVSImage = gFixImage;
    ((RpWorldCameraData *)((uint8_t *)gFixCamera + kCameraData))->world = world;
}

/* EAX = world, EDX = raster, ECX = z raster, st0 = far clip (popped); calls the C stub with a cdecl frame. */
__attribute__((naked)) static void stCameraCreate(void)
{
    __asm__ volatile("subl $4, %esp\n\t"
                     "fstps (%esp)\n\t"
                     "pushl %ecx\n\t"
                     "pushl %edx\n\t"
                     "pushl %eax\n\t"
                     "call _stCameraCreateC\n\t"
                     "addl $16, %esp\n\t"
                     "ret");
}

static void BMX_CDECL stRasterDestroy(RwRaster *raster)
{
    rwtLog(0x4dcbd0);
    rwtLogPtr(raster);
}

static void *BMX_CDECL stCameraSetFrame(RwCamera *camera, RwFrame *frame)
{
    rwtLog(0x4dea60);
    rwtLogPtr(camera);
    rwtLogPtr(frame);
    camera->object.object.parent = frame;
    return camera;
}

static void BMX_CDECL stImageCall(RwImage *image)
{
    rwtLog(0x4e3960);
    rwtLogPtr(image);
}

static void setup_pvs_create(DtCtx *c)
{
    worldEnv(c, 0);
    stubPvsCallbacks();
    dt_stub(0x004ce6d0, NULL, (void *)stCameraCreate);
    dt_stub(0x004e3b90, NULL, (void *)stImageCall);
    dt_stub(0x004e3960, NULL, (void *)stImageCall);
    dt_stub(0x004d2920, NULL, (void *)stImageCall);
    dt_stub(0x004dcbd0, NULL, (void *)stRasterDestroy);
    dt_stub(0x004dea60, NULL, (void *)stCameraSetFrame);
    dt_stub(0x004dea90, NULL, (void *)stImageCall);
    RpWorld *w = pvsWorld(1);
    RpPVSWorldData *pvs = *(RpPVSWorldData **)((uint8_t *)w + kPvsOffset);
    if (dt_u32() % 3 == 0) {
        pvs->progressCallBack = NULL;
    }
    gFixCamera = mkCamera(NULL, 0);
    RwCamera *cam = gFixCamera;
    cam->frameBuffer = dt_alloc(0x28);
    cam->zBuffer = dt_alloc(0x28);
    gFixImage = dt_alloc(0x40);
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : w);
    DT_ARG_PTR(c, 1, dt_alloc(0x28));
    DT_ARG_PTR(c, 2, dt_alloc(0x28));
    DT_ARG_FLOAT(c, 3, dt_u32() % 5 == 0 ? __builtin_nanf("") : dt_float_edgy(0.0F, 500.0F));
    DT_ARG_INT(c, 4, (uint32_t)(dt_u32() % 4 == 0 ? dt_range(32, 38) : dt_range(0, 6)));
    DT_ARG_PTR(c, 5, dt_u32() % 3 == 0 ? NULL : dt_alloc(8));
    DT_ARG_PTR(c, 6, dt_alloc(8));
    rwtEnd();
}

/* ---- the register-argument bodies, atomics, extension callbacks and the sector's world --------------------
 * (rw/world.c: 0x4b76c0..0x4b9d50)
 */

#define ORIG_FREELIST_CREATE DT_ORIG(0x004dba90, RwFreeList *(*)(int32_t, int32_t, int32_t))

#define REG2(addr, ret, ta, tb) ((ret(__attribute__((regparm(2))) *)(ta, tb))(uintptr_t)(addr))
#define REG1(addr, ret, ta)     ((ret(__attribute__((regparm(1))) *)(ta))(uintptr_t)(addr))

static RpWorld *addCameraOrig(RpWorld *w, RwCamera *camera)
{
    return REG2(0x004b81f8, RpWorld *, RpWorld *, RwCamera *)(w, camera);
}

static RpWorld *addCameraImpl(RpWorld *w, RwCamera *camera)
{
    return rpWorldAddCameraBody(w, camera);
}

static RpWorld *addClumpOrig(RpWorld *w, RpClump *clump)
{
    return REG2(0x004b86a8, RpWorld *, RpWorld *, RpClump *)(w, clump);
}

static RpWorld *addClumpImpl(RpWorld *w, RpClump *clump)
{
    return rpWorldAddClumpBody(w, clump);
}

static RpWorld *addLightOrig(RpWorld *w, RpLight *light)
{
    return REG2(0x004b8af8, RpWorld *, RpWorld *, RpLight *)(w, light);
}

static RpWorld *addLightImpl(RpWorld *w, RpLight *light)
{
    return rpWorldAddLightBody(w, light);
}

static RpWorld *addAtomicOrig(RpWorld *w, RpAtomic *atomic)
{
    return REG2(0x004b82b8, RpWorld *, RpWorld *, RpAtomic *)(w, atomic);
}

static RpWorld *addAtomicImpl(RpWorld *w, RpAtomic *atomic)
{
    return rpWorldAddAtomicBody(w, atomic);
}

static RpWorld *removeAtomicOrig(RpWorld *w, RpAtomic *atomic)
{
    return REG2(0x004b82f8, RpWorld *, RpWorld *, RpAtomic *)(w, atomic);
}

static RpWorld *removeAtomicImpl(RpWorld *w, RpAtomic *atomic)
{
    return rpWorldRemoveAtomicBody(w, atomic);
}

static int atomicTiesOrig(RpAtomic *atomic)
{
    REG1(0x004b78b0, void, RpAtomic *)(atomic);
    return 0;
}

static int atomicTiesImpl(RpAtomic *atomic)
{
    rpAtomicTiesDestroy(atomic);
    return 0;
}

static int lightTiesOrig(RpLight *light)
{
    REG1(0x004b7b40, void, RpLight *)(light);
    return 0;
}

static int lightTiesImpl(RpLight *light)
{
    rpLightTiesDestroy(light);
    return 0;
}

enum { kAtomicWorld = 0x74, kAtomicBytes = 0x90 };

/* An atomic with a frame (or none), a cached render entry (or none) and ties with sectors; some ties lack an
 * end. */
static RpAtomic *mkAtomic(void)
{
    rpWorldAtomicOffset = kAtomicWorld;
    RpAtomic *a = dt_alloc(kAtomicBytes);
    a->object.object.type = 1;
    a->object.object.parent = dt_u32() % 3 == 0 ? NULL : mkFrame();
    a->object.sync = (void *)(uintptr_t)(dt_u32() & ~3U);
    a->stamp = (uint8_t)dt_u32();
    a->repEntry = dt_u32() % 2 == 0 ? NULL : (RwResEntry *)dt_alloc(0x18);
    uint16_t spare = (uint16_t)dt_u32();
    __builtin_memcpy(a->pad64, &spare, 2);
    listInit(&a->llWorldSectorsInAtomic);
    for (int n = dt_range(0, 3); n > 0; n--) {
        RpTie *tie = dt_alloc(sizeof(RpTie));
        tie->apAtom = dt_u32() % 4 == 0 ? NULL : a;
        tie->worldSector = dt_u32() % 4 == 0 ? NULL : (RpWorldSector *)dt_alloc(8);
        RwLLLink *other = dt_alloc(sizeof(RwLLLink));
        listInit(other);
        listAddTail(other, &tie->lAtomicInWorldSector);
        listAddTail(&a->llWorldSectorsInAtomic, &tie->lWorldSectorInAtomic);
    }
    *(void **)((uint8_t *)a + kAtomicWorld) = dt_u32() % 2 ? NULL : mkWorld();
    *(uint32_t *)((uint8_t *)a + kAtomicWorld + 4) = dt_u32() & ~3U;
    return a;
}

static void setup_add_camera_body(DtCtx *c)
{
    worldEnv(c, 0);
    stubFrames();
    DT_ARG_PTR(c, 0, mkWorld());
    DT_ARG_PTR(c, 1, mkCamera(NULL, 0));
    rwtEnd();
}

static void setup_add_clump_body(DtCtx *c)
{
    setup_add_clump(c);
}

static void setup_atomic_world(DtCtx *c)
{
    worldEnv(c, 0);
    stubFrames();
    dt_stub(0x004dc380, NULL, (void *)stFreeRes);
    DT_ARG_PTR(c, 0, mkWorld());
    DT_ARG_PTR(c, 1, mkAtomic());
    rwtEnd();
}

/* The RpClumpForAllAtomics callbacks get the atomic first and the world second. */
static void setup_atomic_callback(DtCtx *c)
{
    setup_atomic_world(c);
    const uint32_t world = c->args[0];
    c->args[0] = c->args[1];
    c->args[1] = world;
}

static void setup_atomic_ties(DtCtx *c)
{
    worldEnv(c, 0);
    DT_ARG_PTR(c, 0, mkAtomic());
    rwtEnd();
}

static void setup_light_ties(DtCtx *c)
{
    worldEnv(c, 0);
    RpLight *l = mkLight(NULL, lightType());
    for (int i = dt_range(0, 3); i > 0; i--) {
        RpWorldSector *s = mkLeaf(-1);
        RpLightTie *tie = dt_alloc(sizeof(RpLightTie));
        tie->light = l;
        tie->worldSector = s;
        listAddTail(&s->lightsInWorldSector, &tie->lightInWorldSector);
        listAddTail(&l->sectors, &tie->worldSectorInLight);
    }
    DT_ARG_PTR(c, 0, l);
    rwtEnd();
}

static void setup_atomic_ext_init(DtCtx *c)
{
    RwGlobals *g = worldEnv(c, 0);
    *(uint16_t *)((uint8_t *)g + 8) = (uint16_t)dt_u32();
    DT_ARG_PTR(c, 0, mkAtomic());
    rwtEnd();
}

static void setup_clump_copy(DtCtx *c)
{
    worldEnv(c, 0);
    clumpStubs();
    RpWorld *w = mkWorld();
    RpClump *src = mkClump(dt_u32() % 3 == 0 ? NULL : w);
    RpClump *dst = mkClump(dt_u32() % 5 == 0 ? mkWorld() : NULL);
    DT_ARG_PTR(c, 0, dst);
    DT_ARG_PTR(c, 1, src);
    rwtEnd();
}

static void setup_light_ext_init(DtCtx *c)
{
    worldEnv(c, 0);
    RpLight *l = mkLight(NULL, lightType());
    l->object.sync = (void *)(uintptr_t)(dt_u32() & ~3U);
    DT_ARG_PTR(c, 0, l);
    rwtEnd();
}

static void setup_light_copy(DtCtx *c)
{
    worldEnv(c, 0);
    stubFrames();
    RpLight *src = mkLight(dt_u32() % 3 == 0 ? NULL : mkWorld(), lightType());
    RpLight *dst = mkLight(NULL, lightType());
    DT_ARG_PTR(c, 0, dst);
    DT_ARG_PTR(c, 1, src);
    rwtEnd();
}

static RwCamera *BMX_CDECL cbCamUpdate(RwCamera *camera)
{
    rwtLog(0xca70);
    rwtLogPtr(camera);
    return dt_u32() % 4 == 0 ? NULL : camera;
}

/* A camera whose plugin data holds saved callbacks: callable ones when `callable`, else random addresses. */
static RwCamera *mkExtCamera(RpWorld *world, int callable)
{
    RwCamera *cam = mkCamera(world, dt_range(0, 3));
    RpWorldCameraData *data = (RpWorldCameraData *)((uint8_t *)cam + kCameraData);
    data->beginUpdate = callable ? cbCamUpdate : (RwCameraUpdateFunc)(uintptr_t)(dt_u32() & ~3U);
    data->endUpdate = callable ? cbCamUpdate : (RwCameraUpdateFunc)(uintptr_t)(dt_u32() & ~3U);
    data->sync = (void *)(uintptr_t)(dt_u32() & ~3U);
    cam->beginUpdate = (RwCameraUpdateFunc)(uintptr_t)(dt_u32() & ~3U);
    cam->endUpdate = (RwCameraUpdateFunc)(uintptr_t)(dt_u32() & ~3U);
    cam->object.sync = (void *)(uintptr_t)(dt_u32() & ~3U);
    return cam;
}

static void setup_camera_ext(DtCtx *c)
{
    worldEnv(c, 0);
    DT_ARG_PTR(c, 0, mkExtCamera(dt_u32() % 2 ? NULL : mkWorld(), 0));
    rwtEnd();
}

static void setup_camera_copy(DtCtx *c)
{
    worldEnv(c, 0);
    stubFrames();
    DT_ARG_PTR(c, 0, mkExtCamera(NULL, 0));
    DT_ARG_PTR(c, 1, mkExtCamera(dt_u32() % 3 == 0 ? NULL : mkWorld(), 0));
    rwtEnd();
}

static void setup_camera_update(DtCtx *c)
{
    RwGlobals *g = worldEnv(c, 0);
    g->curWorld = dt_u32() % 2 ? mkWorld() : NULL;
    g->renderFrame = (uint16_t)(dt_u32() % 4 == 0 ? 0xffff : dt_u32());
    DT_ARG_PTR(c, 0, mkExtCamera(mkWorld(), 1));
    rwtEnd();
}

static void setup_tie_close(DtCtx *c)
{
    RwGlobals *g = worldEnv(c, 0);
    rwFreeListHead.next = rwFreeListHead.prev = &rwFreeListHead;
    RpTieGlobals *ties = (RpTieGlobals *)((uint8_t *)g + kTieOffset);
    ties->tieFreeList = dt_u32() % 3 == 0 ? NULL : ORIG_FREELIST_CREATE(0x18, 4, 0);
    ties->lightTieFreeList = dt_u32() % 3 == 0 ? NULL : ORIG_FREELIST_CREATE(0x18, 4, 0);
    *(int32_t *)(uintptr_t)0x00676734 = dt_range(1, 3); /* rpTieModuleCount */
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

#define MATERIAL_OFFSET_VAR (*(int32_t *)(uintptr_t)0x00676750)
#define MATERIAL_COUNT_VAR  (*(int32_t *)(uintptr_t)0x00676754)

static void setup_material_open(DtCtx *c)
{
    worldEnv(c, 0);
    rwFreeListHead.next = rwFreeListHead.prev = &rwFreeListHead;
    MATERIAL_COUNT_VAR = dt_range(0, 2);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_INT(c, 1, 0x300 + 4 * dt_range(0, 30));
    gRwtMallocFailAt = dt_u32() % 3 == 0 ? dt_range(1, 3) : 0;
    gRwtEntryFailAt = dt_u32() % 4 == 0 ? 1 : 0;
    rwtEnd();
}

static void setup_material_close(DtCtx *c)
{
    RwGlobals *g = worldEnv(c, 0);
    rwFreeListHead.next = rwFreeListHead.prev = &rwFreeListHead;
    MATERIAL_OFFSET_VAR = 0x300 + 4 * dt_range(0, 30);
    MATERIAL_COUNT_VAR = dt_range(1, 3);
    *(RwFreeList **)((uint8_t *)g + MATERIAL_OFFSET_VAR) =
        dt_u32() % 3 == 0 ? NULL : ORIG_FREELIST_CREATE(0x18, 5, 0);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    rwtEnd();
}

static void setup_material_ptr(DtCtx *c)
{
    worldEnv(c, 0);
    DT_ARG_PTR(c, 0, mkMaterial());
    rwtEnd();
}

static void setup_sector_ptr(DtCtx *c)
{
    worldEnv(c, 0);
    DT_ARG_PTR(c, 0, mkLeaf(-1));
    rwtEnd();
}

static void *BMX_CDECL cbSectorRender(RpWorldSector *sector)
{
    rwtLog(0x5ec7);
    rwtLogPtr(sector);
    return dt_u32() % 2 ? sector : NULL;
}

static void setup_sector_render(DtCtx *c)
{
    RwGlobals *g = worldEnv(c, 0);
    RpWorld *w = mkWorld();
    w->renderCallBack = cbSectorRender;
    g->curWorld = w;
    DT_ARG_PTR(c, 0, mkLeaf(-1));
    rwtEnd();
}

static void setup_sector_find(DtCtx *c)
{
    worldEnv(c, 0);
    void *sector = dt_alloc(8);
    uint32_t *search = dt_alloc(8);
    search[0] = (uint32_t)(uintptr_t)(dt_u32() % 2 ? sector : dt_alloc(8));
    search[1] = dt_u32() % 2;
    DT_ARG_PTR(c, 0, sector);
    DT_ARG_PTR(c, 1, search);
    rwtEnd();
}

static void collectLeaves(RpSector *s, RpWorldSector **out, int *n)
{
    if (s->type < 0) {
        if (*n < 40) {
            out[(*n)++] = (RpWorldSector *)s;
        }
        return;
    }
    collectLeaves(((RpPlaneSector *)s)->leftSubTree, out, n);
    collectLeaves(((RpPlaneSector *)s)->rightSubTree, out, n);
}

/* A list of 0..3 worlds, each one block (sectors inside it) or a BSP tree; the sector is one of their leaves,
 * an address inside a block, or unrelated. */
static void setup_sector_world(DtCtx *c)
{
    RwGlobals *g = worldEnv(c, 1);
    RpWorldGlobals *globals = (RpWorldGlobals *)((uint8_t *)g + kWorldGlobalsOffset);
    RpWorldSector *leaves[40];
    int numLeaves = 0;
    uint8_t *inside = NULL;
    for (int n = dt_range(0, 3); n > 0; n--) {
        RpWorldListEntry *e = dt_alloc(sizeof(RpWorldListEntry));
        e->world = mkWorld();
        if (dt_u32() % 2 == 0) {
            e->world->object.privateFlags = (uint8_t)(1 | dt_u32());
            e->size = (uint32_t)dt_range(0x90, 0x400);
            inside = (uint8_t *)e->world + dt_range(0, (int32_t)e->size + 0x40);
        } else {
            e->world->object.privateFlags = (uint8_t)(dt_u32() & ~1U);
            e->world->rootSector = mkTree(dt_range(0, 3));
            collectLeaves(e->world->rootSector, leaves, &numLeaves);
        }
        listAddTail(&globals->worlds, &e->link);
    }
    void *sector = dt_alloc(8);
    switch (dt_u32() % 3) {
    case 0:
        if (numLeaves > 0) {
            sector = leaves[dt_u32() % (uint32_t)numLeaves];
        }
        break;
    case 1:
        if (inside != NULL) {
            sector = inside;
        }
        break;
    default:
        break;
    }
    DT_ARG_PTR(c, 0, sector);
    rwtEnd();
}

DT_CASES(rwworld_cases) = {
    {"RpWorldAddCamera", 0x004b81f0, RpWorldAddCamera, 2, DT_RET_INT, setup_add_camera, 100, 0},
    {"RpWorldRemoveCamera", 0x004b8240, RpWorldRemoveCamera, 2, DT_RET_INT, setup_remove_camera, 150, 0},
    {"RwCameraGetWorld", 0x004b82a0, RwCameraGetWorld, 1, DT_RET_INT, setup_camera_world, 30, 0},
    {"RpCameraForAllSectorsInFrustum", 0x004b8810, RpCameraForAllSectorsInFrustum, 3, DT_RET_INT,
     setup_for_all_sectors, 200, 0},
    {"rpWorldCameraDataDestroy", 0x004b9110, rpWorldCameraDataDestroy, 1, DT_RET_INT, setup_camera_data, 60,
     0},
    {"_rpLightTieDestroy", 0x004b77e0, _rpLightTieDestroy, 1, DT_RET_INT, setup_light_tie, 60, 0},
    {"_rpTieDestroy", 0x004b7840, _rpTieDestroy, 1, DT_RET_INT, setup_tie, 100, 0},
    {"RpWorldAddClump", 0x004b86a0, RpWorldAddClump, 2, DT_RET_INT, setup_add_clump, 150, 0},
    {"RpWorldRemoveClump", 0x004b8770, RpWorldRemoveClump, 2, DT_RET_INT, setup_remove_clump, 200, 0},
    {"RpClumpGetWorld", 0x004b8870, RpClumpGetWorld, 1, DT_RET_INT, setup_clump_world, 30, 0},
    {"RpWorldAddLight", 0x004b8af0, RpWorldAddLight, 2, DT_RET_INT, setup_add_light, 150, 0},
    {"RpWorldRemoveLight", 0x004b8b80, RpWorldRemoveLight, 2, DT_RET_INT, setup_remove_light, 150, 0},
    {"RpMaterialSetColor", 0x004b9310, RpMaterialSetColor, 2, DT_RET_INT, setup_set_color, 60, 0},
    {"RpMaterialSetTexture", 0x004b9380, RpMaterialSetTexture, 2, DT_RET_INT, setup_set_texture, 100, 0},
    {"RpMaterialGetTexture", 0x004b93d0, RpMaterialGetTexture, 1, DT_RET_INT, setup_get_texture, 30, 0},
    {"RpWorldGetBBox", 0x004b9fe0, RpWorldGetBBox, 1, DT_RET_INT, setup_getter, 80, 0},
    {"RpWorldGetOrigin", 0x004ba050, RpWorldGetOrigin, 1, DT_RET_INT, setup_getter, 80, 0},
    {"RpWorldGetNumMaterials", 0x004ba0c0, RpWorldGetNumMaterials, 1, DT_RET_INT, setup_getter, 80, 0},
    {"RpWorldGetMaterial", 0x004ba140, RpWorldGetMaterial, 2, DT_RET_INT, setup_get_material, 200, 0},
    {"RpWorldGetNumClumps", 0x004ba1f0, RpWorldGetNumClumps, 1, DT_RET_INT, setup_getter, 80, 0},
    {"RpWorldGetRenderCallBack", 0x004bafa0, RpWorldGetRenderCallBack, 1, DT_RET_INT, setup_getter, 80, 0},
    {"RpWorldRender", 0x004ba460, RpWorldRender, 1, DT_RET_INT, setup_world_render, 100, 0},
    {"RpWorldForAllClumps", 0x004bb7d0, RpWorldForAllClumps, 3, DT_RET_INT, setup_for_all_clumps, 150, 0},
    {"RpWorldForAllLights", 0x004bba60, RpWorldForAllLights, 3, DT_RET_INT, setup_for_all_lights, 150, 0},
    {"RpWorldForAllWorldSectors", 0x004bbb80, RpWorldForAllWorldSectors, 3, DT_RET_INT,
     setup_for_all_sectors_world, 200, 0},
    /* RpWorldDestroy's stub at 0x4ba9c0 falls into the body at 0x4ba9c4 (and the sector trees) */
    {"rpWorldDestroyBody", 0x004ba9c0, testWorldDestroy, 1, DT_RET_INT, setup_destroy, 300, 0},
    {"_rpWorldForAllSectorsIntersecting", 0x004c1cf0, _rpWorldForAllSectorsIntersecting, 4, DT_RET_INT,
     setup_intersect, 400, 0},
    {"RpPVSSetProgressCallBack", 0x004cc0f0, RpPVSSetProgressCallBack, 2, DT_RET_INT, setup_pvs_progress, 100,
     0},
    {"RpPVSHook", 0x004cc270, RpPVSHook, 1, DT_RET_INT, setup_pvs_hook, 100, 0},
    {"RpPVSUnhook", 0x004cc2c0, RpPVSUnhook, 1, DT_RET_INT, setup_pvs_hook, 100, 0},
    {"RpPVSUnload", 0x004cc320, RpPVSUnload, 1, DT_RET_INT, setup_pvs_unload, 150, 0},
    {"RpPVSSetViewPosition", 0x004cc510, RpPVSSetViewPosition, 2, DT_RET_INT, setup_pvs_view, 200, 0},
    {"RpPVSQuery", 0x004cc640, RpPVSQuery, 1, DT_RET_INT, setup_pvs_query, 60, 0},
    {"RpPVSStatisticsGet", 0x004cc670, RpPVSStatisticsGet, 3, DT_RET_INT, setup_pvs_stats, 60, 0},
    {"rpPVSCountSectors", 0x004cef40, rpPVSCountSectors, 1, DT_RET_INT, setup_pvs_count, 100, 0},
    {"RpPVSCreate", 0x004ce960, RpPVSCreate, 7, DT_RET_INT, setup_pvs_create, 300, 0},
    {"rpWorldAddCameraBody", (uint32_t)(uintptr_t)addCameraOrig, addCameraImpl, 2, DT_RET_INT,
     setup_add_camera_body, 100, 0},
    {"rpWorldAddClumpBody", (uint32_t)(uintptr_t)addClumpOrig, addClumpImpl, 2, DT_RET_INT,
     setup_add_clump_body, 150, 0},
    {"rpWorldAddLightBody", (uint32_t)(uintptr_t)addLightOrig, addLightImpl, 2, DT_RET_INT, setup_add_light,
     150, 0},
    {"rpWorldAddAtomicBody", (uint32_t)(uintptr_t)addAtomicOrig, addAtomicImpl, 2, DT_RET_INT,
     setup_atomic_world, 150, 0},
    {"rpWorldRemoveAtomicBody", (uint32_t)(uintptr_t)removeAtomicOrig, removeAtomicImpl, 2, DT_RET_INT,
     setup_atomic_world, 300, 0},
    {"rpWorldAddAtomicCallback", 0x004b8680, rpWorldAddAtomicCallback, 2, DT_RET_INT, setup_atomic_callback,
     100, 0},
    {"rpWorldRemoveAtomicCallback", 0x004b8750, rpWorldRemoveAtomicCallback, 2, DT_RET_INT,
     setup_atomic_callback, 200, 0},
    {"rpAtomicTiesDestroy", (uint32_t)(uintptr_t)atomicTiesOrig, atomicTiesImpl, 1, DT_RET_INT,
     setup_atomic_ties, 200, 0},
    {"rpLightTiesDestroy", (uint32_t)(uintptr_t)lightTiesOrig, lightTiesImpl, 1, DT_RET_INT, setup_light_ties,
     150, 0},
    {"WorldInitAtomicExt", 0x004b9040, WorldInitAtomicExt, 1, DT_RET_INT, setup_atomic_ext_init, 100, 0},
    {"WorldDeInitAtomicExt", 0x004b9010, WorldDeInitAtomicExt, 1, DT_RET_INT, setup_atomic_ties, 200, 0},
    {"WorldCopyAtomicExt", 0x004b7b00, WorldCopyAtomicExt, 1, DT_RET_INT, setup_atomic_ties, 20, 0},
    {"WorldCopyClumpExt", 0x004b7b10, WorldCopyClumpExt, 2, DT_RET_INT, setup_clump_copy, 150, 0},
    {"WorldInitLightExt", 0x004b8fc0, WorldInitLightExt, 1, DT_RET_INT, setup_light_ext_init, 60, 0},
    {"WorldDeInitLightExt", 0x004b8fa0, WorldDeInitLightExt, 1, DT_RET_INT, setup_light_ties, 150, 0},
    {"WorldCopyLightExt", 0x004b7e50, WorldCopyLightExt, 2, DT_RET_INT, setup_light_copy, 100, 0},
    {"WorldInitCameraExt", 0x004b90c0, WorldInitCameraExt, 1, DT_RET_INT, setup_camera_ext, 60, 0},
    {"WorldDeInitCameraExt", 0x004b9080, WorldDeInitCameraExt, 1, DT_RET_INT, setup_camera_ext, 100, 0},
    {"WorldCopyCameraExt", 0x004b77a0, WorldCopyCameraExt, 2, DT_RET_INT, setup_camera_copy, 100, 0},
    {"WorldCameraBeginUpdate", 0x004b76c0, WorldCameraBeginUpdate, 1, DT_RET_INT, setup_camera_update, 100,
     0},
    {"WorldCameraEndUpdate", 0x004b7700, WorldCameraEndUpdate, 1, DT_RET_INT, setup_camera_update, 100, 0},
    {"_rpTieClose", 0x004b7650, _rpTieClose, 1, DT_RET_INT, setup_tie_close, 60, 0},
    {"_rpMaterialOpen", 0x004b9150, _rpMaterialOpen, 2, DT_RET_INT, setup_material_open, 60, 0},
    {"_rpMaterialClose", 0x004b91b0, _rpMaterialClose, 1, DT_RET_INT, setup_material_close, 60, 0},
    {"RpMaterialGetColor", 0x004b9330, RpMaterialGetColor, 1, DT_RET_INT, setup_material_ptr, 10, 0},
    {"RpWorldSectorGetBBox", 0x004c2240, RpWorldSectorGetBBox, 1, DT_RET_INT, setup_sector_ptr, 10, 0},
    {"rpWorldSectorRenderCurrent", 0x004c21e0, rpWorldSectorRenderCurrent, 1, DT_RET_INT, setup_sector_render,
     30, 0},
    {"rpWorldSectorFindCallback", 0x004b9d30, rpWorldSectorFindCallback, 2, DT_RET_INT, setup_sector_find, 40,
     0},
    {"rpWorldSectorGetWorld", 0x004b9d50, rpWorldSectorGetWorld, 1, DT_RET_INT, setup_sector_world, 400, 0},
};
