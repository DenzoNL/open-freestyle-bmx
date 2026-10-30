/*
 * RenderWare 3.1 world (RpWorld): attaching cameras, clumps and lights, the ties between objects and BSP
 * sectors, the for-all walks, destruction, materials and the PVS plugin.
 *
 * Most functions of the original fail through RwErrorSet with the world plugin's id (2) or the PVS plugin's
 * id (0x106) and a core error code, and return 0 (or -1 from the counting getters).
 */
#include "rw/rwworld.h"

#include "rw/rwclump.h"
#include "rw/rwcollision.h"
#include "rw/rwim.h"

/*
 * The camera set-up of the PVS generation (0x4ce6d0, original code of the camera group): EAX = world, EDX =
 * raster, ECX = z raster and the far clip plane on the x87 stack (popped by the callee). It creates the
 * camera and image the generation renders with (rpPVSCamera, rpPVSImage).
 */
#ifdef BMX_HOOK
static void rpPVSCameraCreate(RpWorld *world, RwRaster *raster, RwRaster *zRaster, float farClip)
{
    __asm__ volatile("flds %[far]\n\tcall *%[fn]"
                     : "+a"(world), "+d"(raster), "+c"(zRaster)
                     : [far] "m"(farClip), [fn] "r"((uintptr_t)0x004ce6d0)
                     : "memory", "cc", "st");
}
#else
void rpPVSCameraCreate(RpWorld *world, RwRaster *raster, RwRaster *zRaster, float farClip);
#endif

enum {
    kWorldPluginId = 2,
    kPvsPluginId = 0x106,
    kPvsCountPluginId = 0x185,
    kPvsStageStart = 0x14,
    kPvsStageEnd = 0x16,
};

/* Original code the walks and hooks call by address (not functions of their own in the exe's symbols). */
#define CB_RENDER_SECTORS RW_CB(RwObjectCallBack, 0x004ba430)
#define CB_ADD_ATOMIC     RW_CB(RwObjectCallBack, 0x004b8680)
#define CB_REMOVE_ATOMIC  RW_CB(RwObjectCallBack, 0x004b8750)
#define CB_PVS_RENDER     RW_CB(RpWorldSectorCallBackRender, 0x004cc1f0)
#define CB_PVS_VIEW       RW_CB(RpIntersectionCallBackWorldSector, 0x004cc4e0)
#define CB_PVS_UNLOAD     RW_CB(RwObjectCallBack, 0x004cc300)
#define CB_PVS_COUNT      RW_CB(RwObjectCallBack, 0x004ceef0)
#define CB_PVS_PREPARE    RW_CB(RwObjectCallBack, 0x004ce620)
#define CB_PVS_SAMPLE     RW_CB(RwObjectCallBack, 0x004ce250)

static void reportError(int32_t pluginId, int32_t code)
{
    RwError error;
    error.pluginID = pluginId;
    error.errorCode = _rwerror(code);
    RwErrorSet(&error);
}

/* The common entry check of the world functions: the module must be open and the world given. */
static int worldUsable(const RpWorld *world)
{
    if (rpWorldModuleCount == 0) {
        reportError(kWorldPluginId, rwERR_NOTOPEN);
        return 0;
    }
    if (world == NULL) {
        reportError(kWorldPluginId, rwERR_NULLP);
        return 0;
    }
    return 1;
}

static void *engineFree(void *block)
{
    RwEngineInstance->memoryFuncs.rwfree(block);
    return NULL;
}

/* Inserts the link at the front of the list. */
static void linkFirst(RwLLLink *head, RwLLLink *link)
{
    RwLLLink *first = head->next;
    link->prev = head;
    link->next = first;
    first->prev = link;
    head->next = link;
}

static void linkOut(RwLLLink *link)
{
    link->prev->next = link->next;
    link->next->prev = link->prev;
}

/* The data the world plugin keeps in a camera, a clump (the world it is in) or a light (the same). */
static RpWorldCameraData *cameraData(const RwCamera *camera)
{
    return (RpWorldCameraData *)((uint8_t *)camera + rpWorldCameraOffset);
}

static RpWorld **clumpWorldSlot(const RpClump *clump)
{
    return (RpWorld **)((uint8_t *)clump + rpWorldClumpOffset);
}

static RpWorld **lightWorldSlot(const RpLight *light)
{
    return (RpWorld **)((uint8_t *)light + rpWorldLightOffset);
}

/* RpClump fields this module uses (the clump type itself is in rw/clump.c): its link in the world's list. */
static RwLLLink *clumpInWorldLink(RpClump *clump)
{
    return (RwLLLink *)((uint8_t *)clump + 0x14);
}

static uint8_t *clumpStamp(RpClump *clump)
{
    return (uint8_t *)clump + 0x12;
}

/* The list of sector ties of an atomic (RpTie.lWorldSectorInAtomic links). */
static RwLLLink *atomicSectorList(const RpAtomic *atomic)
{
    return (RwLLLink *)((uint8_t *)atomic + 0x68);
}

static RpTieGlobals *tieGlobals(void)
{
    return (RpTieGlobals *)((uint8_t *)RwEngineInstance + rpTieOffset);
}

static RpWorldGlobals *worldGlobals(void)
{
    return (RpWorldGlobals *)((uint8_t *)RwEngineInstance + rpWorldGlobalsOffset);
}

static RpPVSWorldData *pvsData(const RpWorld *world)
{
    return *(RpPVSWorldData **)((const uint8_t *)world + rpPVSWorldOffset);
}

/* ---- cameras ---------------------------------------------------------------------------------------- */

/* Puts the camera into the world (the body of RpWorldAddCamera: EAX = world, EDX = camera). */
BMX_ADDR(0x004b81f8)
__attribute__((regparm(2))) RpWorld *rpWorldAddCameraBody(RpWorld *world, RwCamera *camera)
{
    RpWorldCameraData *data = cameraData(camera);
    RwFrame *frame = RwCameraGetFrame(camera);
    if (frame != NULL) {
        RwFrameUpdateObjects(frame);
    }
    data->world = world;
    camera->version++;
    return world;
}

/* The original loads both arguments into EAX and EDX and falls into the body. */
BMX_ADDR(0x004b81f0)
RpWorld *RpWorldAddCamera(RpWorld *world, RwCamera *camera)
{
    return rpWorldAddCameraBody(world, camera);
}

/* Frees the camera's list of sectors in view; 1. */
BMX_ADDR(0x004b9110)
int rpWorldCameraDataDestroy(RpWorldCameraData *data)
{
    if (data->sectors != NULL) {
        engineFree((void *)data->sectors);
    }
    data->numSectors = 0;
    data->capacity = 0;
    data->sectors = NULL;
    return 1;
}

/* Takes the camera out of its world; 0 when it was in none. Returns the world argument otherwise. */
BMX_ADDR(0x004b8240)
RpWorld *RpWorldRemoveCamera(RpWorld *world, RwCamera *camera)
{
    RpWorldCameraData *data = cameraData(camera);
    if (data->world == NULL) {
        return NULL;
    }
    rpWorldCameraDataDestroy(data);
    data->world = NULL;
    camera->version++;
    return world;
}

BMX_ADDR(0x004b82a0)
RpWorld *RwCameraGetWorld(const RwCamera *camera)
{
    return cameraData(camera)->world;
}

/* Calls the callback for each sector in the camera's view until it returns NULL. */
BMX_ADDR(0x004b8810)
RwCamera *RpCameraForAllSectorsInFrustum(RwCamera *camera, RwObjectCallBack callback, void *data)
{
    const RpWorldCameraData *cam = cameraData(camera);
    RpWorldSector **sectors = cam->sectors;
    for (int32_t n = cam->numSectors; n != 0; n--) {
        if (callback(*sectors, data) == NULL) {
            break;
        }
        sectors++;
    }
    return camera;
}

/* ---- ties, clumps and lights ------------------------------------------------------------------------ */

/* Unlinks a light tie from its sector and light, and frees it; 1. */
BMX_ADDR(0x004b77e0)
int _rpLightTieDestroy(RpLightTie *tie)
{
    RwFreeList *freeList = tieGlobals()->lightTieFreeList;
    linkOut(&tie->worldSectorInLight);
    linkOut(&tie->lightInWorldSector);
    RwEngineInstance->freeListFree(freeList, tie);
    return 1;
}

/* The same for an atomic tie, only when both ends are set; 1. */
BMX_ADDR(0x004b7840)
int _rpTieDestroy(RpTie *tie)
{
    if (tie->apAtom != NULL && tie->worldSector != NULL) {
        RwFreeList *freeList = tieGlobals()->tieFreeList;
        linkOut(&tie->lWorldSectorInAtomic);
        linkOut(&tie->lAtomicInWorldSector);
        RwEngineInstance->freeListFree(freeList, tie);
    }
    return 1;
}

/* Adds the clump to the world, ties its atomics to the sectors; 0 (error 3) if it is in a world already. */
BMX_ADDR(0x004b86a8)
__attribute__((regparm(2))) RpWorld *rpWorldAddClumpBody(RpWorld *world, RpClump *clump)
{
    RpWorld **slot = clumpWorldSlot(clump);
    if (*slot != NULL) {
        reportError(kWorldPluginId, 3);
        return NULL;
    }
    linkFirst(&world->clumpList, clumpInWorldLink(clump));
    world->numClumpsInWorld++;
    RwFrame *frame = clump->object.parent;
    *slot = world;
    RpClumpForAllAtomics(clump, CB_ADD_ATOMIC, world);
    if (frame != NULL) {
        RwMatrixUpdate(&frame->modelling);
        RwFrameUpdateObjects(frame);
    }
    (*clumpStamp(clump))++;
    return world;
}

BMX_ADDR(0x004b86a0)
RpWorld *RpWorldAddClump(RpWorld *world, RpClump *clump)
{
    return rpWorldAddClumpBody(world, clump);
}

/* Takes the clump out of its world (not the one passed in: the world's own count and cursor change). */
BMX_ADDR(0x004b8770)
RpWorld *RpWorldRemoveClump(RpWorld *world, RpClump *clump)
{
    RpWorld **slot = clumpWorldSlot(clump);
    RpWorld *clumpWorld = *slot;
    if (clumpWorld == NULL) {
        reportError(kWorldPluginId, 4);
        return NULL;
    }
    clumpWorld->numClumpsInWorld--;
    RwLLLink *link = clumpInWorldLink(clump);
    if (link == clumpWorld->currentClumpLink) {
        clumpWorld->currentClumpLink = link->next;
    }
    linkOut(link);
    RpClumpForAllAtomics(clump, CB_REMOVE_ATOMIC, world);
    *slot = NULL;
    (*clumpStamp(clump))++;
    return world;
}

BMX_ADDR(0x004b8870)
RpWorld *RpClumpGetWorld(const RpClump *clump)
{
    return *clumpWorldSlot(clump);
}

/*
 * Adds the light to the world: ambient and directional lights (type below 0x80) to one list, point and spot
 * lights, whose frame is updated first, to the other.
 */
BMX_ADDR(0x004b8af8)
__attribute__((regparm(2))) RpWorld *rpWorldAddLightBody(RpWorld *world, RpLight *light)
{
    *lightWorldSlot(light) = world;
    if (light->object.object.subType < 0x80) {
        linkFirst(&world->directionalLightList, &light->inWorld);
    } else {
        RwFrame *frame = RpLightGetFrame(light);
        if (frame != NULL) {
            RwFrameUpdateObjects(frame);
        }
        linkFirst(&world->lightList, &light->inWorld);
    }
    light->stamp++;
    return world;
}

BMX_ADDR(0x004b8af0)
RpWorld *RpWorldAddLight(RpWorld *world, RpLight *light)
{
    return rpWorldAddLightBody(world, light);
}

/* Removes the light from its sectors and from the world. */
BMX_ADDR(0x004b8b80)
RpWorld *RpWorldRemoveLight(RpWorld *world, RpLight *light)
{
    *lightWorldSlot(light) = NULL;
    const RwLLLink *end = &light->sectors;
    RwLLLink *link = light->sectors.next;
    while (link != end) {
        RpLightTie *tie = (RpLightTie *)((uint8_t *)link - offsetof(RpLightTie, worldSectorInLight));
        link = link->next;
        _rpLightTieDestroy(tie);
    }
    linkOut(&light->inWorld);
    light->stamp++;
    return world;
}

/* ---- materials -------------------------------------------------------------------------------------- */

/* The original returns the material in EAX without saying so. */
BMX_ADDR(0x004b9310)
RpMaterial *RpMaterialSetColor(RpMaterial *material, const RwRGBA *color)
{
    material->color = *color;
    material->stamp++;
    return material;
}

/* Takes a reference of the new texture, releases the old one; the stamp grows by two. */
BMX_ADDR(0x004b9380)
RpMaterial *RpMaterialSetTexture(RpMaterial *material, RwTexture *texture)
{
    if (texture != NULL) {
        RwTextureAddRef(texture);
    }
    if (material->texture != NULL) {
        RwTextureDestroy(material->texture);
    }
    material->stamp += 2;
    material->texture = texture;
    return material;
}

BMX_ADDR(0x004b93d0)
RwTexture *RpMaterialGetTexture(const RpMaterial *material)
{
    return material->texture;
}

/* ---- the world -------------------------------------------------------------------------------------- */

BMX_ADDR(0x004b9fe0)
const RwBBox *RpWorldGetBBox(const RpWorld *world)
{
    if (!worldUsable(world)) {
        return NULL;
    }
    return &world->boundingBox;
}

BMX_ADDR(0x004ba050)
const RwV3d *RpWorldGetOrigin(const RpWorld *world)
{
    if (!worldUsable(world)) {
        return NULL;
    }
    return &world->worldOrigin;
}

BMX_ADDR(0x004ba0c0)
int32_t RpWorldGetNumMaterials(const RpWorld *world)
{
    if (!worldUsable(world)) {
        return -1;
    }
    return world->matList.numMaterials;
}

/* The material at the index; NULL with an error (code 2) for an index out of range. */
BMX_ADDR(0x004ba140)
RpMaterial *RpWorldGetMaterial(const RpWorld *world, int32_t index)
{
    if (!worldUsable(world)) {
        return NULL;
    }
    if (index < 0 || index >= world->matList.numMaterials) {
        reportError(kWorldPluginId, 2);
        return NULL;
    }
    return world->matList.materials[index];
}

BMX_ADDR(0x004ba1f0)
int32_t RpWorldGetNumClumps(const RpWorld *world)
{
    if (!worldUsable(world)) {
        return -1;
    }
    return world->numClumpsInWorld;
}

BMX_ADDR(0x004bafa0)
RpWorldSectorCallBackRender RpWorldGetRenderCallBack(const RpWorld *world)
{
    if (!worldUsable(world)) {
        return NULL;
    }
    return world->renderCallBack;
}

/* Renders the world: the original walks the current camera's sectors with its render callback (0x4ba430). */
BMX_ADDR(0x004ba460)
RpWorld *RpWorldRender(RpWorld *world)
{
    if (!worldUsable(world)) {
        return NULL;
    }
    RpCameraForAllSectorsInFrustum(RwEngineInstance->curCamera, CB_RENDER_SECTORS, world);
    return world;
}

/* Calls the callback for every clump of the world until it returns NULL. */
BMX_ADDR(0x004bb7d0)
RpWorld *RpWorldForAllClumps(RpWorld *world, RwObjectCallBack callback, void *data)
{
    if (!worldUsable(world)) {
        return NULL;
    }
    const RwLLLink *end = &world->clumpList;
    RwLLLink *link = world->clumpList.next;
    while (link != end) {
        RpClump *clump = (RpClump *)((uint8_t *)link - 0x14);
        link = link->next;
        if (callback(clump, data) == NULL) {
            return world;
        }
    }
    return world;
}

/* Calls the callback for every light, the ambient and directional ones first. */
BMX_ADDR(0x004bba60)
RpWorld *RpWorldForAllLights(RpWorld *world, RwObjectCallBack callback, void *data)
{
    if (!worldUsable(world)) {
        return NULL;
    }
    RwLLLink *lists[2] = {&world->directionalLightList, &world->lightList};
    for (int i = 0; i < 2; i++) {
        const RwLLLink *end = lists[i];
        RwLLLink *link = lists[i]->next;
        while (link != end) {
            RpLight *light = (RpLight *)((uint8_t *)link - offsetof(RpLight, inWorld));
            link = link->next;
            if (callback(light, data) == NULL) {
                return world;
            }
        }
    }
    return world;
}

/* Calls the callback for every world sector, depth first, until it returns NULL. */
BMX_ADDR(0x004bbb80)
RpWorld *RpWorldForAllWorldSectors(RpWorld *world, RwObjectCallBack callback, void *data)
{
    if (world == NULL || callback == NULL) {
        reportError(kWorldPluginId, rwERR_NULLP);
        return NULL;
    }
    RpSector *stack[64];
    int32_t top = 0;
    RpSector *node = world->rootSector;
    stack[0] = NULL;
    do {
        if (node->type < 0) {
            if (callback(node, data) == NULL) {
                return world;
            }
            node = stack[top--];
        } else {
            const RpPlaneSector *plane = (const RpPlaneSector *)node;
            node = plane->leftSubTree;
            stack[++top] = plane->rightSubTree;
        }
    } while (top >= 0);
    return world;
}

/* ---- destroying a world ------------------------------------------------------------------------------ */

/* Releases a leaf sector's render data, its ties and its plugin data (not the sector's own memory). */
static void sectorLeafDeinit(RpWorldSector *sector)
{
    if (sector->repEntry != NULL) {
        RwResourcesFreeResEntry(sector->repEntry);
    }
    RwLLLink *lists[3] = {&sector->collAtomicsInWorldSector, &sector->noCollAtomicsInWorldSector,
                          &sector->lightsInWorldSector};
    for (int i = 0; i < 3; i++) {
        const RwLLLink *end = lists[i];
        RwLLLink *link = lists[i]->next;
        while (link != end) {
            RwLLLink *next = link->next;
            if (i < 2) {
                _rpTieDestroy((RpTie *)link);
            } else {
                _rpLightTieDestroy((RpLightTie *)link);
            }
            link = next;
        }
    }
    _rwPluginRegistryDeInitObject(&rpWorldSectorRegistry, sector);
}

/* Releases the ties and plugin data of every leaf below the sector (the sectors' memory stays). */
BMX_ADDR(0x004ba760)
__attribute__((regparm(1))) void rpSectorTreeDeinit(RpSector *sector)
{
    if (sector->type == -1) {
        sectorLeafDeinit((RpWorldSector *)sector);
    } else if (sector->type != -2) {
        const RpPlaneSector *plane = (const RpPlaneSector *)sector;
        rpSectorTreeDeinit(plane->leftSubTree);
        rpSectorTreeDeinit(plane->rightSubTree);
    }
}

/* Frees a heap block of a sector and clears its pointer. */
static void sectorFreeBlock(void *slot)
{
    void **block = (void **)slot;
    if (*block != NULL) {
        engineFree(*block);
        *block = NULL;
    }
}

/* Destroys the sector and everything below it: ties, plugin data, geometry arrays and the sectors. */
BMX_ADDR(0x004ba820)
__attribute__((regparm(1))) void rpSectorTreeDestroy(RpSector *sector)
{
    if (sector->type == -1) {
        RpWorldSector *leaf = (RpWorldSector *)sector;
        sectorLeafDeinit(leaf);
        sectorFreeBlock((void *)&leaf->vertices);
        sectorFreeBlock((void *)&leaf->normals);
        sectorFreeBlock((void *)&leaf->preLitLum);
        sectorFreeBlock((void *)&leaf->polygons);
        sectorFreeBlock((void *)&leaf->colSectorRoot);
        sectorFreeBlock((void *)&leaf->texCoords);
    } else if (sector->type != -2) {
        RpPlaneSector *plane = (RpPlaneSector *)sector;
        rpSectorTreeDestroy(plane->leftSubTree);
        plane->leftSubTree = NULL;
        rpSectorTreeDestroy(plane->rightSubTree);
        plane->rightSubTree = NULL;
    }
    engineFree(sector);
}

/*
 * Destroys the world (RpWorldDestroy at 0x4ba9c0 loads its argument into EAX and falls into this): leaves the
 * engine's list of worlds, drops the mesh header of every sector, the material list and the sectors (unless
 * private flag 1 says they belong to someone else; then only their ties and plugin data go), then the world.
 * The original is only entered by its caller at 0x4bb080 and by that stub, so only this entry is hooked.
 */
BMX_ADDR(0x004ba9c4)
__attribute__((regparm(1))) int rpWorldDestroyBody(RpWorld *world)
{
    if (rpWorldModuleCount == 0) {
        reportError(kWorldPluginId, rwERR_NOTOPEN);
        return 0;
    }
    if (world == NULL) {
        reportError(kWorldPluginId, rwERR_NULLP);
        return 0;
    }
    RpWorldGlobals *globals = worldGlobals();
    const RwLLLink *end = &globals->worlds;
    for (RwLLLink *link = globals->worlds.next; link != end; link = link->next) {
        RpWorldListEntry *entry = (RpWorldListEntry *)((uint8_t *)link - offsetof(RpWorldListEntry, link));
        if (entry->world == world) {
            RwFreeList *freeList = globals->entryFreeList;
            linkOut(link);
            RwEngineInstance->freeListFree(freeList, entry);
            break;
        }
    }
    RpSector *stack[64];
    int32_t top = 0;
    RpSector *node = world->rootSector;
    do {
        if (node->type < 0) {
            RpWorldSector *leaf = (RpWorldSector *)node;
            if (leaf->mesh != NULL) {
                _rpMeshHeaderDestroy(leaf->mesh);
                leaf->mesh = NULL;
            }
            node = stack[top--];
        } else {
            const RpPlaneSector *plane = (const RpPlaneSector *)node;
            node = plane->leftSubTree;
            stack[++top] = plane->rightSubTree;
        }
    } while (top >= 0);
    world->stamp++;
    _rpMaterialListDeinitialize(&world->matList);
    if ((world->object.privateFlags & 1) == 0) {
        rpSectorTreeDestroy(world->rootSector);
    } else {
        rpSectorTreeDeinit(world->rootSector);
    }
    _rwPluginRegistryDeInitObject(&rpWorldRegistry, world);
    engineFree(world);
    return 1;
}

/* ---- creation ---------------------------------------------------------------------------------------- */

#define CB_WORLD_RENDER RW_CB(RpWorldSectorCallBackRender, 0x004bc360)

static void listInit(RwLLLink *head)
{
    head->next = head;
    head->prev = head;
}

/*
 * Builds the render mesh of every sector without one (the original's own copy of the loop in
 * _rpWorldGenerateSectorMeshes, with unsigned polygon counts). 0 when a mesh could not be built.
 */
static int worldCreateMeshes(RpWorld *world)
{
    RpSector *stack[64];
    int32_t top = 0;
    stack[0] = NULL;
    RpSector *node = world->rootSector;
    do {
        if (node->type < 0) {
            RpWorldSector *sector = (RpWorldSector *)node;
            RpMaterial **materials = world->matList.materials + sector->matListWindowBase;
            if (sector->mesh == NULL) {
                RpBuildMesh *build = RpBuildMeshCreate(sector->numPolygons);
                if (build == NULL) {
                    return 0;
                }
                for (int32_t i = 0; i < (int32_t)sector->numPolygons; i++) {
                    const RpPolygon *polygon = &sector->polygons[i];
                    _rpBuildMeshAddTriangle(build, materials[polygon->matIndex], polygon->vertIndex[0],
                                            polygon->vertIndex[1], polygon->vertIndex[2]);
                }
                RpMeshHeader *mesh = _rpMeshGenerate(build, (world->object.flags & 1) != 0 ? 1 : 0);
                if (mesh == NULL) {
                    RpBuildMeshDestroy(build);
                    return 0;
                }
                sector->mesh = mesh;
            }
            node = stack[top--];
        } else {
            const RpPlaneSector *plane = (const RpPlaneSector *)node;
            node = plane->leftSubTree;
            stack[++top] = plane->rightSubTree;
        }
    } while (top >= 0);
    return 1;
}

/*
 * Creates an empty world of one sector for the box: no polygons, default surface properties and render
 * callback, entered in the engine's list of worlds. The world origin is (|sup - inf| - inf) per axis (the
 * length of the diagonal minus the box's inf corner), and the box of the world and its sector is the origin
 * plus the given box. The accessors the original inlines (RpWorldSetFlags and the like) re-check the module
 * count and the world, which cannot fail here.
 */
BMX_ADDR(0x004bb080)
RpWorld *RpWorldCreate(const RwBBox *boundingBox)
{
    if (rpWorldModuleCount == 0) {
        reportError(kWorldPluginId, rwERR_NOTOPEN);
        return NULL;
    }
    if (boundingBox == NULL) {
        reportError(kWorldPluginId, rwERR_NULLP);
        return NULL;
    }
    RpWorld *world = RwEngineInstance->memoryFuncs.rwmalloc((size_t)rpWorldRegistry.sizeOfStruct);
    if (world == NULL) {
        RwError error;
        error.pluginID = kWorldPluginId;
        error.errorCode = _rwerror(rwERR_NOMEM, rpWorldRegistry.sizeOfStruct);
        RwErrorSet(&error);
        return NULL;
    }
    world->object.parent = NULL;
    world->object.privateFlags = 0;
    world->object.flags = 0;
    world->object.subType = 0;
    world->object.type = 7;
    _rpMaterialListInitialize(&world->matList);
    world->stamp = 0;
    world->flags = 2;
    world->stamp++;
    world->object.flags = 0;
    world->stamp++;
    RpWorldSector *sector =
        RwEngineInstance->memoryFuncs.rwmalloc((size_t)rpWorldSectorRegistry.sizeOfStruct);
    if (sector == NULL) {
        RwError error;
        error.pluginID = kWorldPluginId;
        error.errorCode = _rwerror(rwERR_NOMEM, 4); /* sic: 4, not the sector size */
        RwErrorSet(&error);
        RwEngineInstance->memoryFuncs.rwfree(world);
        return NULL;
    }
    sector->texCoords = NULL;
    sector->normals = NULL;
    sector->polygons = NULL;
    sector->vertices = NULL;
    sector->preLitLum = NULL;
    sector->numPolygons = 0;
    sector->numVertices = 0;
    sector->mesh = NULL;
    sector->repEntry = NULL;
    sector->colSectorRoot = NULL;
    listInit(&sector->lightsInWorldSector);
    listInit(&sector->noCollAtomicsInWorldSector);
    listInit(&sector->collAtomicsInWorldSector);
    sector->type = -1;
    world->rootSector = (RpSector *)sector;

    const RwBBox *box = boundingBox;
    RwV3d size;
    size.x = (float)((double)box->sup.x - box->inf.x);
    size.y = (float)((double)box->sup.y - box->inf.y);
    size.z = (float)((double)box->sup.z - box->inf.z);
    const double length = RwV3dLength((const ZVec3 *)&size);
    const double originX = length - box->inf.x;
    world->worldOrigin.x = (float)originX;
    world->worldOrigin.y = (float)(length - box->inf.y);
    world->worldOrigin.z = (float)(length - box->inf.z);
    sector->boundingBox.inf.x = (float)(originX + box->inf.x);
    sector->boundingBox.inf.y = (float)((double)world->worldOrigin.y + box->inf.y);
    sector->boundingBox.inf.z = (float)((double)world->worldOrigin.z + box->inf.z);
    sector->boundingBox.sup.x = (float)((double)world->worldOrigin.x + box->sup.x);
    sector->boundingBox.sup.y = (float)((double)world->worldOrigin.y + box->sup.y);
    sector->boundingBox.sup.z = (float)((double)world->worldOrigin.z + box->sup.z);
    world->boundingBox = sector->boundingBox;

    listInit(&world->clumpList);
    world->currentClumpLink = &world->clumpList;
    world->numClumpsInWorld = 0;
    world->surfaceProps = rpWorldDefaultSurfaceProps;
    world->stamp++;
    listInit(&world->lightList);
    listInit(&world->directionalLightList);
    world->renderCallBack = CB_WORLD_RENDER;
    world->stamp++;

    RpWorldGlobals *globals = worldGlobals();
    const int32_t worldSize = rpWorldRegistry.sizeOfStruct;
    RpWorldListEntry *entry = RwEngineInstance->freeListAlloc(globals->entryFreeList);
    if (entry != NULL) {
        entry->size = (uint32_t)worldSize;
        entry->world = world;
        linkFirst(&globals->worlds, &entry->link);
    }
    _rwPluginRegistryInitObject(&rpWorldRegistry, world);
    _rwPluginRegistryInitObject(&rpWorldSectorRegistry, sector);
    if (!worldCreateMeshes(world)) {
        rpWorldDestroyBody(world);
        return NULL;
    }
    return world;
}

/* ---- walking the BSP --------------------------------------------------------------------------------- */

/* ---- the line walk ----------------------------------------------------------------------------------- */

static int32_t floatBits(float f)
{
    int32_t bits;
    __builtin_memcpy(&bits, &f, 4);
    return bits;
}

/* Whether two floats have the same sign bit. */
static int sameSign(float a, float b)
{
    return ((floatBits(a) ^ floatBits(b)) & (int32_t)0x80000000) == 0;
}

/* a - b with b opaque to the optimiser, so the subtraction is not rewritten as a + -b (which flips the sign
 * of a NaN b; the x87 keeps it). */
static double subtractOpaque(double a, double b)
{
    __asm__("" : "+x"(b));
    return a - b;
}

/* The point where the line crosses a plane, from the signed distances of its ends: start - (end - start) * t
 * with t = startDistance / (endDistance - startDistance), each coordinate rounded to a float. */
static RwV3d lineSplit(const RwLine *line, float startDistance, float endDistance)
{
    const double t = (double)startDistance / ((double)endDistance - (double)startDistance);
    RwV3d point;
    point.x = (float)subtractOpaque(line->start.x, ((double)line->end.x - (double)line->start.x) * t);
    point.y = (float)subtractOpaque(line->start.y, ((double)line->end.y - (double)line->start.y) * t);
    point.z = (float)subtractOpaque(line->start.z, ((double)line->end.z - (double)line->start.z) * t);
    return point;
}

/* The distance of a line end from a plane value along the plane's axis, rounded to a float. */
static float planeDistance(const RwV3d *end, int32_t axisOffset, float value)
{
    float coordinate;
    __builtin_memcpy(&coordinate, (const uint8_t *)end + axisOffset, 4);
    return (float)((double)coordinate - (double)value);
}

/*
 * Calls the callback for the world sectors a line passes through, clipping the line at the planes. The
 * distances of both ends from the left and right values decide (by their bits, so -0 counts as negative):
 * a line entirely on one side goes down that side; a line inside the overlap of both values goes down both
 * (the side of the start first); otherwise the line is cut where it crosses the value(s) it straddles, the
 * part beyond each cut going to the other child. The callback gets the stack copy of the intersection; a
 * NULL result stops the walk. Returns the world.
 */
BMX_ADDR(0x004bcd90)
__attribute__((regparm(3))) RpWorld *
rpWorldForAllSectorsIntersectingLine(RpWorld *world, RpIntersection *intersection,
                                     RpIntersectionCallBackWorldSector callback, int32_t unused0,
                                     int32_t unused1, int32_t unused2, RpIntersection *again, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    RwLine line = intersection->t.line;
    RpSector *nodes[64];
    RwLine lines[64];
    int32_t top = 0;
    RpSector *node = world->rootSector;
    for (;;) {
        if (node->type < 0) {
            if (callback(again, (RpWorldSector *)node, data) == NULL) {
                return world;
            }
            if (top == 0) {
                return world;
            }
            node = nodes[top];
            line = lines[top];
            top--;
            continue;
        }
        const RpPlaneSector *plane = (const RpPlaneSector *)node;
        const float startRight = planeDistance(&line.start, plane->type, plane->rightValue);
        const float startLeft = planeDistance(&line.start, plane->type, plane->leftValue);
        const float endLeft = planeDistance(&line.end, plane->type, plane->leftValue);
        const float endRight = planeDistance(&line.end, plane->type, plane->rightValue);
        if (floatBits(startRight) < 0 && floatBits(endRight) < 0) {
            node = plane->leftSubTree;
            continue;
        }
        if (floatBits(startLeft) > 0 && floatBits(endLeft) > 0) {
            node = plane->rightSubTree;
            continue;
        }
        top++;
        if (sameSign(startLeft, endLeft) && sameSign(startRight, endRight)) {
            lines[top] = line;
            if (floatBits(startLeft) >= floatBits(endLeft)) {
                nodes[top] = plane->leftSubTree;
                node = plane->rightSubTree;
            } else {
                nodes[top] = plane->rightSubTree;
                node = plane->leftSubTree;
            }
        } else if (!sameSign(startLeft, endLeft) && floatBits(startRight) > 0 && floatBits(endRight) > 0) {
            const RwV3d cut = lineSplit(&line, startLeft, endLeft);
            if (floatBits(startLeft) < 0) {
                nodes[top] = plane->rightSubTree;
                lines[top] = line;
                line.end = cut;
                node = plane->leftSubTree;
            } else {
                nodes[top] = plane->leftSubTree;
                lines[top].start = cut;
                lines[top].end = line.end;
                node = plane->rightSubTree;
            }
        } else if (!sameSign(startRight, endRight) && floatBits(startLeft) < 0 && floatBits(endLeft) < 0) {
            const RwV3d cut = lineSplit(&line, startRight, endRight);
            if (floatBits(startRight) < 0) {
                nodes[top] = plane->rightSubTree;
                lines[top].start = cut;
                lines[top].end = line.end;
                node = plane->leftSubTree;
            } else {
                nodes[top] = plane->leftSubTree;
                lines[top] = line;
                line.end = cut;
                node = plane->rightSubTree;
            }
        } else if (floatBits(startLeft) < 0) {
            nodes[top] = plane->rightSubTree;
            lines[top].start = lineSplit(&line, startRight, endRight);
            lines[top].end = line.end;
            line.end = lineSplit(&line, startLeft, endLeft);
            node = plane->leftSubTree;
        } else {
            nodes[top] = plane->leftSubTree;
            lines[top].start = lineSplit(&line, startLeft, endLeft);
            lines[top].end = line.end;
            line.end = lineSplit(&line, startRight, endRight);
            node = plane->rightSubTree;
        }
    }
}

/* ---- the sphere walk --------------------------------------------------------------------------------- */

/* The sphere walk's limits (all 0.0f in the exe's data): smallest radius, squared radius and distance. */
#define rpSphereMinRadius   ORIG_VAR(0x0069b328, float, rpSphereMinRadius)
#define rpSphereMinRadiusSq ORIG_VAR(0x0069b330, float, rpSphereMinRadiusSq)
#define rpSphereMinDistance ORIG_VAR(0x0069b334, float, rpSphereMinDistance)

/* A node to visit later with the bounding box of its space. */
typedef struct RpSectorBox {
    RpSector *node;
    RwBBox box;
} RpSectorBox;

/* The larger of two squared distances, compared by their bits as signed integers like the original. */
static float largerSquare(float a, float b)
{
    int32_t aBits;
    int32_t bBits;
    __builtin_memcpy(&aBits, &a, 4);
    __builtin_memcpy(&bBits, &b, 4);
    return aBits > bBits ? a : b;
}

/* The squared distance of v from the farther face of [inf, sup] along one axis (each square a float). */
static float farFaceSquare(float inf, float sup, float v)
{
    const double toInf = (double)inf - (double)v;
    const double toSup = (double)sup - (double)v;
    return largerSquare((float)(toInf * toInf), (float)(toSup * toSup));
}

/*
 * Calls the callback for the world sectors a sphere may touch, walking the BSP with the box of each node's
 * space (the left child's box ends at the plane's left value, the right child's starts at its right value).
 * The node test compares the squared radius and the squared distance to the box's farthest corner against
 * the limits above, both 0, so only a NaN distance ends a branch. The callback gets the stack copy of the
 * intersection; a NULL result stops the walk. Returns the world (NULL for a negative radius).
 */
BMX_ADDR(0x004bda80)
__attribute__((regparm(3))) RpWorld *
rpWorldForAllSectorsIntersectingSphere(RpWorld *world, RpIntersection *intersection,
                                       RpIntersectionCallBackWorldSector callback, int32_t unused0,
                                       int32_t unused1, int32_t unused2, RpIntersection *again, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    const RwSphere *sphere = &intersection->t.sphere;
    if ((double)rpSphereMinRadius > (double)sphere->radius) {
        RwError error;
        error.pluginID = kWorldPluginId;
        error.errorCode = _rwerror(rwERR_INVALIDSIZE, "Negative Sphere radius");
        RwErrorSet(&error);
        return NULL;
    }
    const float radiusSquared = (float)((double)sphere->radius * (double)sphere->radius);
    int32_t radiusSquaredBits;
    __builtin_memcpy(&radiusSquaredBits, &radiusSquared, 4);
    const double radiusSq = radiusSquaredBits < 0 ? 0.0 : (double)radiusSquared;
    RpSectorBox stack[64];
    int32_t top = 0;
    RpSector *node = world->rootSector;
    RwBBox box = world->boundingBox;
    for (;;) {
        const float dx = farFaceSquare(box.inf.x, box.sup.x, sphere->center.x);
        const float dy = farFaceSquare(box.inf.y, box.sup.y, sphere->center.y);
        const float dz = farFaceSquare(box.inf.z, box.sup.z, sphere->center.z);
        const double distanceSq = ((double)dx + (double)dy) + (double)dz;
        if (!((double)rpSphereMinRadiusSq > radiusSq) && distanceSq >= (double)rpSphereMinDistance) {
            if (node->type >= 0) {
                const RpPlaneSector *plane = (const RpPlaneSector *)node;
                top++;
                stack[top].node = plane->rightSubTree;
                stack[top].box = box;
                __builtin_memcpy((uint8_t *)&stack[top].box.inf + plane->type, &plane->rightValue, 4);
                __builtin_memcpy((uint8_t *)&box.sup + plane->type, &plane->leftValue, 4);
                node = plane->leftSubTree;
                continue;
            }
            if (callback(again, (RpWorldSector *)node, data) == NULL) {
                return world;
            }
        }
        if (top == 0) {
            return world;
        }
        node = stack[top].node;
        box = stack[top].box;
        top--;
    }
}

/*
 * Calls the callback for the world sectors an intersection touches. A point walks the tree (both children
 * where they overlap), an atomic visits the sectors it is tied to (keeping a marker in the list so the
 * callback may remove the current tie); lines and spheres go through the walks above. The callback's result
 * only matters for atomics. Anything else is error 1. A point's coordinates are compared as integers, like
 * the plane values.
 */
BMX_ADDR(0x004c1cf0)
RpWorld *_rpWorldForAllSectorsIntersecting(RpWorld *world, RpIntersection *intersection,
                                           RpIntersectionCallBackWorldSector callback, void *data)
{
    switch (intersection->type) {
    case rpINTERSECTPOINT: {
        RpSector *stack[64];
        int32_t top = 0;
        RpSector *node = world->rootSector;
        stack[0] = NULL;
        while (top >= 0) {
            if (node->type < 0) {
                callback(intersection, (RpWorldSector *)node, data);
                node = stack[top--];
                continue;
            }
            const RpPlaneSector *plane = (const RpPlaneSector *)node;
            int32_t value;
            int32_t leftBits;
            int32_t rightBits;
            __builtin_memcpy(&value, (const uint8_t *)intersection + plane->type, 4);
            __builtin_memcpy(&leftBits, &plane->leftValue, 4);
            __builtin_memcpy(&rightBits, &plane->rightValue, 4);
            if (leftBits > value) {
                node = plane->leftSubTree;
                if (value > rightBits) {
                    stack[++top] = plane->rightSubTree;
                }
            } else if (value > rightBits) {
                node = plane->rightSubTree;
            } else {
                node = stack[top--];
            }
        }
        return world;
    }
    case rpINTERSECTLINE:
        rpWorldForAllSectorsIntersectingLine(world, intersection, callback, 0, 0, 0, intersection, data);
        return world;
    case rpINTERSECTSPHERE:
        rpWorldForAllSectorsIntersectingSphere(world, intersection, callback, 0, 0, 0, intersection, data);
        return world;
    case rpINTERSECTATOMIC: {
        RwLLLink *head = atomicSectorList(intersection->t.object);
        RwLLLink *link = head->next;
        while (link != head) {
            RpWorldSector *sector =
                ((RpTie *)((uint8_t *)link - offsetof(RpTie, lWorldSectorInAtomic)))->worldSector;
            /* A marker node standing in for the current link's successor; the callback may unlink the link.
             */
            RwLLLink marker;
            marker.next = link->next;
            marker.prev = link;
            link->next->prev = &marker;
            link->next = &marker;
            void *keepGoing = callback(intersection, sector, data);
            marker.prev->next = marker.next;
            marker.next->prev = marker.prev;
            if (keepGoing == NULL) {
                return world;
            }
            link = marker.next;
            if (link == head) {
                break;
            }
        }
        return world;
    }
    default:
        reportError(kWorldPluginId, 1);
        return NULL;
    }
}

/* ---- PVS ---------------------------------------------------------------------------------------------- */

/* The PVS plugin errors: id 0x106, and the core code for a missing argument. */
static void pvsNullError(void)
{
    reportError(kPvsPluginId, rwERR_NULLP);
}

BMX_ADDR(0x004cc0f0)
RpWorld *RpPVSSetProgressCallBack(RpWorld *world, RpPVSProgressCallBack callback)
{
    if (world == NULL) {
        pvsNullError();
        return NULL;
    }
    RpPVSWorldData *pvs = pvsData(world);
    if (pvs == NULL) {
        pvsNullError();
        return NULL;
    }
    pvs->progressCallBack = callback;
    return world;
}

/* Replaces the world's render callback by the PVS one (once); keeps the old one for RpPVSUnhook. */
BMX_ADDR(0x004cc270)
RpWorld *RpPVSHook(RpWorld *world)
{
    if (world != NULL) {
        RpPVSWorldData *pvs = pvsData(world);
        if (pvs->visibility != NULL && pvs->hooked == 0) {
            pvs->savedRenderCallBack = RpWorldGetRenderCallBack(world);
            RpWorldSetRenderCallBack(world, CB_PVS_RENDER);
            pvs->hooked = 1;
        }
    }
    return world;
}

/*
 * Puts the world's own render callback back. The original is a four byte stub that loads the argument into
 * EAX and falls into the body at 0x4cc2c4; this function is both. The hook jmp at 0x4cc2c0 covers the body's
 * first byte, so the body has no hook of its own: its callers 0x4cc710 and 0x4cc324 are ported and call this.
 */
BMX_ADDR(0x004cc2c0)
RpWorld *RpPVSUnhook(RpWorld *world)
{
    if (world != NULL) {
        RpPVSWorldData *pvs = pvsData(world);
        if (pvs->hooked != 0) {
            RpWorldSetRenderCallBack(world, pvs->savedRenderCallBack);
            pvs->hooked = 0;
        }
    }
    return world;
}

/* Frees the PVS data of the world and of its sectors. Stub at 0x4cc320 and body at 0x4cc324 ported as one,
 * like above; the body's only caller 0x4cc850 is ported and calls this. */
BMX_ADDR(0x004cc320)
RpWorld *RpPVSUnload(RpWorld *world)
{
    if (world != NULL) {
        RpPVSUnhook(world);
        RpPVSWorldData *pvs = pvsData(world);
        if (pvs != NULL) {
            if (pvs->visibility != NULL) {
                engineFree(pvs->visibility);
                pvs->visibility = NULL;
            }
            pvs->numEntries = 0;
            if (pvs->entries != NULL) {
                engineFree(pvs->entries);
            }
            if (pvs->entryIndices != NULL) {
                engineFree(pvs->entryIndices);
            }
            pvs->entryIndices = NULL;
            pvs->entries = NULL;
        }
        RpWorldForAllWorldSectors(world, CB_PVS_UNLOAD, NULL);
    }
    return world;
}

/* Tells the PVS which sector the viewer is in: walks the sectors around the position. */
BMX_ADDR(0x004cc510)
RpWorld *RpPVSSetViewPosition(RpWorld *world, const RwV3d *position)
{
    if (world != NULL && position != NULL) {
        RpPVSWorldData *pvs = pvsData(world);
        if (pvs->visibility != NULL) {
            RpIntersection intersection = {0};
            __builtin_memcpy(intersection.t.pointBits, position, sizeof intersection.t.pointBits);
            intersection.type = rpINTERSECTPOINT;
            _rpWorldForAllSectorsIntersecting(world, &intersection, CB_PVS_VIEW, pvs);
        }
    }
    return world;
}

/* 1 when the world has PVS data. */
BMX_ADDR(0x004cc640)
int32_t RpPVSQuery(const RpWorld *world)
{
    if (world == NULL || pvsData(world)->visibility == NULL) {
        return 0;
    }
    return 1;
}

/* Hands out the two counters of the PVS data and clears them. */
BMX_ADDR(0x004cc670)
RpWorld *RpPVSStatisticsGet(RpWorld *world, uint32_t *statsA, uint32_t *statsB)
{
    if (world == NULL) {
        return NULL;
    }
    RpPVSWorldData *pvs = pvsData(world);
    *statsA = pvs->statsA;
    *statsB = pvs->statsB;
    pvs->statsA = 0;
    pvs->statsB = 0;
    return world;
}

/* ---- the PVS plugin's callbacks: render, view walk, plugin constructors/destructors and streaming ---- */

#define CB_PVS_LOAD RW_CB(RwObjectCallBack, 0x004cc7d0)

/* The pointer a sector's PVS plugin data holds (the row of the sector: its index, then rows of cells). */
static uint8_t **pvsSectorSlot(const RpWorldSector *sector)
{
    return (uint8_t **)((uint8_t *)sector + rpPVSSectorOffset);
}

/* Renders a sector when the PVS row of the sector the viewer is in has its bit set; counts polygons seen
 * (statsA) and drawn (statsB). Without a row everything is drawn. Callback of the world's render walk. */
BMX_ADDR(0x004cc1f0)
void *pvsRenderSector(RpWorldSector *sector)
{
    const int32_t *id = (const int32_t *)*pvsSectorSlot(sector);
    RpPVSWorldData *pvs = pvsData(rpWorldSectorGetWorld(sector));
    if (sector->numPolygons != 0 && id != NULL && pvs->viewRow != NULL) {
        const int32_t index = *id;
        pvs->statsA += sector->numPolygons;
        if (((int32_t)(int8_t)pvs->viewRow[index >> 3] & (1 << (index & 7))) == 0) {
            return NULL;
        }
        pvs->statsB += sector->numPolygons;
    }
    return pvs->savedRenderCallBack(sector);
}

/* Clears the PVS slot of a sector (the unload walk's callback; the sector is the result to go on). */
BMX_ADDR(0x004cc300)
void *pvsSectorUnload(void *sector)
{
    *pvsSectorSlot(sector) = NULL;
    return sector;
}

/* a - b with b opaque to the optimiser (see subtractOpaque: the sign of a NaN decides). */
static float pvsSubtract(float a, float b)
{
    __asm__("" : "+x"(b));
    return a - b;
}

/*
 * The row of visibility bits of the cell of the sector that holds the position (EAX = sector, EDX =
 * position): descends the sector's collision tree as many levels as the granularity says (a node's byte 0 bit
 * 7 marks a split, bits 5..6 give the axis (x4 as a byte offset), byte 1 the split vertex; a negative
 * distance goes to the first child), then doubles up to the cell level. A sector without polygons has only
 * the first row.
 */
BMX_ADDR(0x004cc3c0)
__attribute__((regparm(2))) const uint8_t *pvsCellRow(const RpWorldSector *sector, const float *position)
{
    const RwV3d *vertices = sector->vertices;
    const RpPVSWorldData *pvs = pvsData(rpWorldSectorGetWorld((RpWorldSector *)(uintptr_t)sector));
    const uint8_t *data = *pvsSectorSlot(sector);
    const uint8_t *tree = sector->colSectorRoot;
    int32_t node = 1;
    if (sector->numPolygons == 0) {
        return data + 4;
    }
    for (int32_t level = 0; level < pvs->granularity; level++) {
        const uint8_t flags = tree[node * 3];
        if ((flags & 0x80) == 0) {
            while (node < pvs->numCells) {
                node *= 2;
            }
            break;
        }
        const uint32_t axis = (uint32_t)(flags >> 3) & 0xc;
        const uint32_t vertex = tree[node * 3 + 1];
        const float split = *(const float *)((const uint8_t *)vertices + vertex * 12 + axis);
        const float distance = pvsSubtract(*(const float *)((const uint8_t *)position + axis), split);
        node = node * 2 + (floatBits(distance) < 0 ? 0 : 1);
    }
    return data + 4 + (node - pvs->numCells) * pvs->rowBytes;
}

/* The view position walk's callback: remembers the sector and the row of its cell; 0 stops the walk. */
BMX_ADDR(0x004cc4e0)
void *pvsViewSector(RpIntersection *intersection, RpWorldSector *sector, RpPVSWorldData *pvs)
{
    pvs->viewSector = sector;
    pvs->viewRow = pvsCellRow(sector, &intersection->t.point.x);
    return NULL;
}

/* The constructor of the world's PVS data: a zeroed header of 0x58 bytes (not all of it: the sizes at 0x0c
 * and 0x10 are set when the data is read or generated). The plugin slot gets the pointer even when the
 * allocation failed; the result is the world then, NULL otherwise. */
BMX_ADDR(0x004cc6b0)
void *pvsWorldConstruct(void *world)
{
    RpPVSWorldData *pvs = RwEngineInstance->memoryFuncs.rwmalloc(0x58);
    void *result = NULL;
    if (pvs != NULL) {
        pvs->cursor = NULL;
        pvs->entryIndices = NULL;
        pvs->entries = NULL;
        pvs->numEntries = 0;
        pvs->rowBuffer = NULL;
        pvs->progressCallBack = NULL;
        pvs->hooked = 0;
        pvs->savedRenderCallBack = NULL;
        pvs->numSectors = 0;
        pvs->rowStride = 0;
        pvs->rowBytes = 0;
        pvs->visibility = NULL;
        pvs->viewRow = NULL;
        result = world;
    }
    *(RpPVSWorldData **)((uint8_t *)world + rpPVSWorldOffset) = pvs;
    return result;
}

/* The destructor: puts the render callback back, frees the data and clears the slot. */
BMX_ADDR(0x004cc710)
void *pvsWorldDestruct(RpWorld *world)
{
    RpPVSWorldData *pvs = pvsData(world);
    RpPVSUnhook(world);
    if (pvs != NULL) {
        if (pvs->visibility != NULL) {
            engineFree(pvs->visibility);
            pvs->visibility = NULL;
        }
        pvs->numEntries = 0;
        if (pvs->entries != NULL) {
            engineFree(pvs->entries);
        }
        if (pvs->entryIndices != NULL) {
            engineFree(pvs->entryIndices);
        }
        pvs->entryIndices = NULL;
        pvs->entries = NULL;
        engineFree(pvs);
    }
    *(RpPVSWorldData **)((uint8_t *)world + rpPVSWorldOffset) = NULL;
    return world;
}

/* Stream size of the world's PVS chunk: its 5 header words when there are visibility rows, else none. */
BMX_ADDR(0x004cc7b0)
int32_t pvsWorldChunkSize(const RpWorld *world)
{
    return pvsData(world)->visibility != NULL ? 0x14 : 0;
}

/* The load walk's callback: moves the sector's row from its own block into the world's one (the cursor) and
 * tracks the largest vertex count. */
BMX_ADDR(0x004cc7d0)
void *pvsSectorLoad(RpWorldSector *sector, RpPVSWorldData *pvs)
{
    const int32_t vertices = RpWorldSectorGetNumVertices(sector);
    pvs->numEntries = vertices > pvs->numEntries ? vertices : pvs->numEntries;
    uint8_t *target = pvs->cursor;
    uint8_t *source = *pvsSectorSlot(sector);
    __builtin_memcpy(target, source, (uint32_t)pvs->rowStride);
    *pvsSectorSlot(sector) = target;
    engineFree(source);
    pvs->cursor = target + pvs->rowStride;
    return sector;
}

/* Reads the PVS chunk of a world: five header words and the rows of all sectors (read before by the sector
 * chunks), which are packed into one block. A header that does not match its sizes drops everything. */
BMX_ADDR(0x004cc850)
RwStream *pvsWorldChunkRead(RwStream *stream, int32_t length, RpWorld *world)
{
    (void)length;
    RpPVSWorldData *pvs = pvsData(world);
    pvs->numEntries = 0;
    if (pvs->entries != NULL) {
        engineFree(pvs->entries);
    }
    if (pvs->entryIndices != NULL) {
        engineFree(pvs->entryIndices);
    }
    int32_t header[5];
    RwStreamRead(stream, header, 0x14);
    RwMemNative32(header, 0x14);
    pvs->numSectors = header[0];
    pvs->rowBytes = header[1];
    pvs->rowStride = header[2];
    pvs->granularity = header[3];
    pvs->numCells = header[4];
    pvs->visibility = RwEngineInstance->memoryFuncs.rwmalloc((uint32_t)header[2] * (uint32_t)header[0]);
    pvs->cursor = pvs->visibility;
    RpWorldForAllWorldSectors(world, CB_PVS_LOAD, pvs);
    if ((((uint32_t)(pvs->numCells * pvs->rowBytes) + 0xbU) & 0xfffffffcU) != (uint32_t)header[2]) {
        RpPVSUnload(world);
    }
    return stream;
}

/* Writes the five header words of the PVS chunk (little endian). */
BMX_ADDR(0x004cc940)
RwStream *pvsWorldChunkWrite(RwStream *stream, int32_t length, const RpWorld *world)
{
    (void)length;
    const RpPVSWorldData *pvs = pvsData(world);
    int32_t header[5] = {pvs->numSectors, pvs->rowBytes, pvs->rowStride, pvs->granularity, pvs->numCells};
    RwMemLittleEndian32(header, 0x14);
    RwStreamWrite(stream, header, 0x14);
    return stream;
}

/* Constructor and destructor of the sector's PVS data: both empty the slot (the rows live in the world's
 * block and are freed with it). */
BMX_ADDR(0x004cc9b0)
void *pvsSectorConstruct(void *sector)
{
    *pvsSectorSlot(sector) = NULL;
    return sector;
}

BMX_ADDR(0x004cc9d0)
void *pvsSectorDestruct(void *sector)
{
    *pvsSectorSlot(sector) = NULL;
    return sector;
}

/* Stream size of a sector's PVS chunk: one row of the world's data (the world is looked up twice, as the
 * original does, which matters for its error report). */
BMX_ADDR(0x004cc9f0)
int32_t pvsSectorChunkSize(RpWorldSector *sector)
{
    const RpPVSWorldData *pvs = pvsData(rpWorldSectorGetWorld(sector));
    (void)rpWorldSectorGetWorld(sector);
    return pvs->visibility != NULL ? pvs->rowStride : 0;
}

/* Reads a sector's PVS chunk: `length` bytes whose first word (the sector's index) is native-swapped. */
BMX_ADDR(0x004cca30)
RwStream *pvsSectorChunkRead(RwStream *stream, int32_t length, RpWorldSector *sector)
{
    uint8_t *rows = RwEngineInstance->memoryFuncs.rwmalloc((uint32_t)length);
    RwStreamRead(stream, rows, (uint32_t)length);
    RwMemNative32(rows, 4);
    *pvsSectorSlot(sector) = rows;
    return stream;
}

BMX_ADDR(0x004cca90)
RwStream *pvsSectorChunkWrite(RwStream *stream, int32_t length, const RpWorldSector *sector)
{
    RwStreamWrite(stream, *pvsSectorSlot(sector), (uint32_t)length);
    return stream;
}

/* The generation's prepare walk: numbers the sector, gives it a zeroed row (the index word, then one row of
 * rowBytes per cell with a bit set for the sector's own index in every cell) and moves the cursor on. */
BMX_ADDR(0x004ce620)
void *pvsSectorPrepare(RpWorldSector *sector, RpPVSWorldData *pvs)
{
    const int32_t index = pvs->sectorCounter;
    uint8_t *row = pvs->cursor;
    pvs->sectorCounter = index + 1;
    *(int32_t *)row = index;
    const int32_t vertices = RpWorldSectorGetNumVertices(sector);
    pvs->numEntries = vertices > pvs->numEntries ? vertices : pvs->numEntries;
    __builtin_memset(row + 4, 0, (uint32_t)(pvs->rowBytes * pvs->numCells));
    for (int32_t cell = 0; cell < pvs->numCells; cell++) {
        const uint32_t id = *(uint32_t *)row;
        row[4 + cell * pvs->rowBytes + (id >> 3)] |= (uint8_t)(1U << (id & 7));
    }
    pvs->cursor = row + pvs->rowStride;
    *pvsSectorSlot(sector) = row;
    return sector;
}

/* The counting walk's callback: adds one to the count behind data; error 0x185 and NULL without a sector or
 * counter. */
BMX_ADDR(0x004ceef0)
void *pvsSectorCount(void *sector, int32_t *count)
{
    if (sector == NULL || count == NULL) {
        reportError(kPvsCountPluginId, rwERR_NULLP);
        return NULL;
    }
    *count += 1;
    return sector;
}

/* Counts the world sectors (error 0x185, result -1, without a world). */
BMX_ADDR(0x004cef40)
int32_t rpPVSCountSectors(RpWorld *world)
{
    if (world == NULL) {
        reportError(kPvsCountPluginId, rwERR_NULLP);
        return -1;
    }
    int32_t count = 0;
    RpWorldForAllWorldSectors(world, CB_PVS_COUNT, &count);
    return count;
}

/* Destroys the camera, rasters and image of a PVS generation (EAX = world; the generation's world is the
 * argument). */
BMX_ADDR(0x004ce890)
__attribute__((regparm(1))) void rpPVSCameraDestroy(RpWorld *world)
{
    RwImageFreePixels(rpPVSImage);
    RwImageDestroy(rpPVSImage);
    RpWorldRemoveCamera(world, rpPVSCamera);
    RwFrameDestroy(RwCameraGetFrame(rpPVSCamera));
    RwCameraSetFrame(rpPVSCamera, NULL);
    RwRasterDestroy(RwCameraGetZRaster(rpPVSCamera));
    RwCameraSetZRaster(rpPVSCamera, NULL);
    RwRasterDestroy(RwCameraGetRaster(rpPVSCamera));
    RwCameraSetRaster(rpPVSCamera, NULL);
    RwCameraDestroy(rpPVSCamera);
}

/* The two words the sampling callback of the PVS generation receives. */
typedef struct RpPVSSampleArgs {
    void *callback;
    void *data;
} RpPVSSampleArgs;

/*
 * Computes the PVS of the world: sizes the visibility rows from the sector count and the granularity, lets
 * the prepare callback count the entries, and (when a callback is given) renders from a camera (set up by the
 * original camera code with the far clip plane on the x87 stack) while the progress callback reports.
 * The scratch buffers are always freed; the visibility data stays. Returns the world, 0 without one.
 */
BMX_ADDR(0x004ce960)
RpWorld *RpPVSCreate(RpWorld *world, RwRaster *raster, RwRaster *zRaster, float farClip, int32_t granularity,
                     void *callback, void *callbackData)
{
    if (world == NULL) {
        reportError(kPvsPluginId, rwERR_NULLP);
        return NULL;
    }
    rpPVSWorld = world;
    RpPVSWorldData *pvs = pvsData(world);
    pvs->numEntries = 0;
    if (pvs->entries != NULL) {
        engineFree(pvs->entries);
    }
    if (pvs->entryIndices != NULL) {
        engineFree(pvs->entryIndices);
    }
    pvs->granularity = granularity;
    const int32_t numCells = (int32_t)(1U << ((uint32_t)granularity & 31U));
    pvs->entries = NULL;
    pvs->numCells = numCells;
    pvs->entryIndices = NULL;
    const int32_t numSectors = rpPVSCountSectors(world);
    pvs->numSectors = numSectors;
    const int32_t rowBytes = (int32_t)(((uint32_t)numSectors + 7U) >> 3);
    pvs->rowBytes = rowBytes;
    const uint32_t rowStride = ((uint32_t)numCells * (uint32_t)rowBytes + 0xbU) & 0xfffffffcU;
    pvs->rowStride = (int32_t)rowStride;
    pvs->visibility = RwEngineInstance->memoryFuncs.rwmalloc((size_t)((uint32_t)numSectors * rowStride));
    pvs->rowBuffer = RwEngineInstance->memoryFuncs.rwmalloc((size_t)pvs->rowBytes);
    pvs->sectorCounter = 0;
    pvs->cursor = pvs->visibility;
    RpWorldForAllWorldSectors(world, CB_PVS_PREPARE, pvs);
    if (pvs->numEntries > 0) {
        pvs->entries = RwEngineInstance->memoryFuncs.rwmalloc((size_t)((uint32_t)pvs->numEntries * 0x24U));
        pvs->entryIndices = RwEngineInstance->memoryFuncs.rwmalloc((size_t)((uint32_t)pvs->numEntries * 6U));
    }
    if (callback != NULL) {
        RpPVSSampleArgs args = {callback, callbackData};
        rpPVSCameraCreate(world, raster, zRaster, farClip);
        if (pvs->progressCallBack != NULL) {
            pvs->progressCallBack(kPvsStageStart, 0.0F);
        }
        RpWorldForAllWorldSectors(world, CB_PVS_SAMPLE, &args);
        if (pvs->progressCallBack != NULL) {
            pvs->progressCallBack(kPvsStageEnd, 100.0F);
        }
        rpPVSCameraDestroy(world);
    }
    if (pvs->rowBuffer != NULL) {
        engineFree(pvs->rowBuffer);
    }
    if (pvs->entries != NULL) {
        engineFree(pvs->entries);
    }
    if (pvs->entryIndices != NULL) {
        engineFree(pvs->entryIndices);
    }
    pvs->entryIndices = NULL;
    pvs->entries = NULL;
    pvs->rowBuffer = NULL;
    return world;
}

/* ---- world sector accessors (from the collision slice) --------------------------------------------------
 */

BMX_ADDR(0x004c2050)
int32_t RpWorldSectorGetNumVertices(const RpWorldSector *sector)
{
    return sector->numVertices;
}

BMX_ADDR(0x004c2060)
int32_t RpWorldSectorGetNumPolygons(const RpWorldSector *sector)
{
    return sector->numPolygons;
}

/* ---- module open/close and extension constructors --------------------------------------------------- */

#define rpTieModuleCount          ORIG_VAR(0x00676734, int32_t, rpTieModuleCount)
#define rpBspModuleCount          ORIG_VAR(0x00676784, int32_t, rpBspModuleCount)
#define rpSectorModuleCount       ORIG_VAR(0x00676794, int32_t, rpSectorModuleCount)
#define rpBinaryWorldModuleCount  ORIG_VAR(0x006767b4, int32_t, rpBinaryWorldModuleCount)
#define rpMaterialListModuleCount ORIG_VAR(0x006767e4, int32_t, rpMaterialListModuleCount)

/* Ties module (world objects in sectors): two free lists of 0x18 byte ties; the world render frame is 1. */
BMX_ADDR(0x004b7570)
void *_rpTieOpen(void *engine, int32_t offset)
{
    rpTieOffset = offset;
    RwFreeList **lists = (RwFreeList **)((uint8_t *)RwEngineInstance + offset);
    lists[0] = RwFreeListCreate(0x18, 100, 0);
    if (lists[0] == NULL) {
        return NULL;
    }
    ((RwFreeList **)((uint8_t *)RwEngineInstance + rpTieOffset))[1] = RwFreeListCreate(0x18, 0x14, 0);
    if (((RwFreeList **)((uint8_t *)RwEngineInstance + rpTieOffset))[1] == NULL) {
        RwFreeListDestroy(((RwFreeList **)((uint8_t *)RwEngineInstance + rpTieOffset))[0]);
        ((RwFreeList **)((uint8_t *)RwEngineInstance + rpTieOffset))[0] = NULL;
        return NULL;
    }
    RwEngineInstance->renderFrame = 1;
    rpTieModuleCount++;
    return engine;
}

/* Calls entry 1 of the world extension's function table (0x69b520) on the atomics' render pipeline. */
BMX_ADDR(0x004c46b0)
int rpWorldExtCallSlot1(void *pipeline)
{
    return ((int(BMX_CDECL *)(void *))rpWorldExtGetFuncs()[1])(pipeline);
}

/* Calls entry 2 of the world extension's function table (0x69b520) on the world's render pipeline. */
BMX_ADDR(0x004c46d0)
int rpWorldExtCallSlot2(void *pipeline)
{
    return ((int(BMX_CDECL *)(void *))rpWorldExtGetFuncs()[2])(pipeline);
}

/*
 * World module: the sector render pipeline (made by the device) and the free list of 0x10 byte world entries,
 * the empty list of worlds, zeroed default surface properties, the extension's entry 2 on the pipeline and
 * the light hook of the pipeline (+0x10). Counts the open. NULL (nothing left behind) if a part fails.
 */
BMX_ADDR(0x004bc090)
void *WorldOpen(void *engine, int32_t offset)
{
    rpWorldDefaultSurfaceProps.ambient = 0.0F;
    rpWorldDefaultSurfaceProps.diffuse = 0.0F;
    rpWorldDefaultSurfaceProps.specular = 0.0F;
    rpWorldGlobalsOffset = offset;
    worldGlobals()->pipeline = _rpWorldPipelineOpen();
    if (worldGlobals()->pipeline == NULL) {
        return NULL;
    }
    worldGlobals()->entryFreeList = RwFreeListCreate(0x10, 3, 0);
    if (worldGlobals()->entryFreeList == NULL) {
        rwIm3DPipelineRelease(worldGlobals()->pipeline);
        worldGlobals()->pipeline = NULL;
        return NULL;
    }
    worldGlobals()->worlds.next = &worldGlobals()->worlds;
    worldGlobals()->worlds.prev = &worldGlobals()->worlds;
    rpWorldExtCallSlot2(worldGlobals()->pipeline);
    *(void **)((uint8_t *)worldGlobals()->pipeline + 0x10) =
        BMX_FNPTR(void *, 0x004bae70, rpWorldLightObject);
    rpWorldModuleCount++;
    return engine;
}

/* Closes the world module: frees the entry list and the pipeline. */
BMX_ADDR(0x004bc020)
void *WorldClose(void *engine)
{
    if (worldGlobals()->entryFreeList != NULL) {
        RwFreeListDestroy(worldGlobals()->entryFreeList);
        worldGlobals()->entryFreeList = NULL;
    }
    if (worldGlobals()->pipeline != NULL) {
        rwIm3DPipelineRelease(worldGlobals()->pipeline);
        worldGlobals()->pipeline = NULL;
    }
    rpWorldModuleCount--;
    return engine;
}

/* The default render callback of a world: runs the sector through the world's render pipeline. */
BMX_ADDR(0x004bc360)
void *rpWorldDefaultRender(RpWorldSector *sector)
{
    if (rwIm3DPipelinePush(worldGlobals()->pipeline) == NULL) {
        return NULL;
    }
    void *result = rwIm3DPipelineRun((RwIm3DTransformInfo *)sector);
    rwIm3DPipelinePop();
    return result;
}

/* Clump extension (the world's data in a clump): the destructor does nothing, the constructor clears it. */
BMX_ADDR(0x004b8fe0)
void *WorldDeInitClumpExt(void *clump)
{
    return clump;
}

BMX_ADDR(0x004b8ff0)
void *WorldInitClumpExt(void *clump)
{
    *(uint32_t *)((uint8_t *)clump + rpWorldClumpOffset) = 0;
    return clump;
}

/* Module open/close routines that only count: the world's BSP module, sectors, binary worlds, material lists.
 */
BMX_ADDR(0x004bc3a0)
void *_rpBspOpen(void *engine)
{
    rpBspModuleCount++;
    return engine;
}

BMX_ADDR(0x004bc3b0)
void *_rpBspClose(void *engine)
{
    rpBspModuleCount--;
    return engine;
}

BMX_ADDR(0x004c2020)
void *_rpSectorOpen(void *engine)
{
    rpSectorModuleCount++;
    return engine;
}

BMX_ADDR(0x004c2030)
void *_rpSectorClose(void *engine)
{
    rpSectorModuleCount--;
    return engine;
}

BMX_ADDR(0x004c2c30)
void *_rpBinaryWorldOpen(void *engine)
{
    rpBinaryWorldModuleCount++;
    return engine;
}

BMX_ADDR(0x004c2c40)
void *_rpBinaryWorldClose(void *engine)
{
    rpBinaryWorldModuleCount--;
    return engine;
}

BMX_ADDR(0x004c56b0)
void *_rpMaterialListOpen(void *engine)
{
    rpMaterialListModuleCount++;
    return engine;
}

BMX_ADDR(0x004c56c0)
void *_rpMaterialListClose(void *engine)
{
    rpMaterialListModuleCount--;
    return engine;
}

/* ---- more module and extension callbacks ------------------------------------------------------------ */

#define rpMaterialModuleCount ORIG_VAR(0x00676754, int32_t, rpMaterialModuleCount)

/* Ties module close: destroys both free lists of ties. */
BMX_ADDR(0x004b7650)
void *_rpTieClose(void *engine)
{
    RwFreeList **lists = (RwFreeList **)((uint8_t *)RwEngineInstance + rpTieOffset);
    if (lists[1] != NULL) {
        RwFreeListDestroy(lists[1]);
        ((RwFreeList **)((uint8_t *)RwEngineInstance + rpTieOffset))[1] = NULL;
    }
    lists = (RwFreeList **)((uint8_t *)RwEngineInstance + rpTieOffset);
    if (lists[0] != NULL) {
        RwFreeListDestroy(lists[0]);
        ((RwFreeList **)((uint8_t *)RwEngineInstance + rpTieOffset))[0] = NULL;
    }
    rpTieModuleCount--;
    return engine;
}

/* Material module: the free list of materials (the registry's size); NULL when it cannot be created. */
BMX_ADDR(0x004b9150)
void *_rpMaterialOpen(void *engine, int32_t offset)
{
    rpMaterialOffset = offset;
    RwFreeList *list = RwFreeListCreate(rpMaterialRegistry.sizeOfStruct, 0x14, 0);
    *(RwFreeList **)((uint8_t *)RwEngineInstance + rpMaterialOffset) = list;
    if (*(RwFreeList **)((uint8_t *)RwEngineInstance + rpMaterialOffset) == NULL) {
        return NULL;
    }
    rpMaterialModuleCount++;
    return engine;
}

BMX_ADDR(0x004b91b0)
void *_rpMaterialClose(void *engine)
{
    RwFreeList *list = *(RwFreeList **)((uint8_t *)RwEngineInstance + rpMaterialOffset);
    if (list != NULL) {
        RwFreeListDestroy(list);
        *(RwFreeList **)((uint8_t *)RwEngineInstance + rpMaterialOffset) = NULL;
    }
    rpMaterialModuleCount--;
    return engine;
}

BMX_ADDR(0x004b9330)
const RwRGBA *RpMaterialGetColor(const RpMaterial *material)
{
    return &material->color;
}

BMX_ADDR(0x004c2240)
const RwBBox *RpWorldSectorGetBBox(const RpWorldSector *sector)
{
    return &sector->boundingBox;
}

/* Renders the sector with the render callback of the engine's current world. */
BMX_ADDR(0x004c21e0)
void *rpWorldSectorRenderCurrent(RpWorldSector *sector)
{
    return ((RpWorld *)RwEngineInstance->curWorld)->renderCallBack(sector);
}

/* Renders the atomics tied to a sector that are in view and not yet rendered this frame (EAX = sector): the
 * ties with collision first, then those without. An atomic is stamped with the render frame whether or not
 * it is inside the frustum. */
BMX_ADDR(0x004ba360)
__attribute__((regparm(1))) void rpSectorRenderAtomics(RpWorldSector *sector)
{
    RwLLLink *lists[2] = {&sector->collAtomicsInWorldSector, &sector->noCollAtomicsInWorldSector};
    for (int i = 0; i < 2; i++) {
        for (RwLLLink *link = lists[i]->next; link != lists[i]; link = link->next) {
            RpAtomic *atomic = ((RpTie *)link)->apAtom;
            if ((atomic->object.object.flags & 4) != 0 &&
                atomic->renderFrame != RwEngineInstance->renderFrame) {
                RwSphere *sphere = RpAtomicGetWorldBoundingSphere(atomic);
                if (RwCameraFrustumTestSphere(RwEngineInstance->curCamera, &sphere->center.x) != 0) {
                    atomic->renderCallBack(atomic);
                }
                atomic->renderFrame = RwEngineInstance->renderFrame;
            }
        }
    }
}

/* The default render callback of a world's sectors, for the walk of RpWorldRender (0x4ba460): the sector is
 * rendered through the world's render callback, and when that says yes its atomics are rendered. */
BMX_ADDR(0x004ba430)
void *rpWorldSectorRenderWithAtomics(RpWorldSector *sector)
{
    if (rpWorldSectorRenderCurrent(sector) != NULL) {
        rpSectorRenderAtomics(sector);
    }
    return sector;
}

/* ---- lighting a sector with the lights of the world ------------------------------------------------------
 */

/* Calls the routine on a light that is switched on (flag bit 1) with unit strength; returns the light. */
BMX_ADDR(0x004bae30)
RpLight *rpLightApplyToObject(RpLight *light, RpLightApplyArgs *args)
{
    if ((light->object.object.flags & 2) != 0) {
        args->apply(args->object, light, 0, 1.0F, args->surfaceProps);
    }
    return light;
}

/* Calls the callback on every light of the current world's ambient/directional list (EAX = callback, EDX =
 * data), until it says no; always 1. */
BMX_ADDR(0x004b9ac8)
__attribute__((regparm(2))) int rpWorldForAllDirectionalLights(RwObjectCallBack callback, void *data)
{
    RwLLLink *head = &((RpWorld *)RwEngineInstance->curWorld)->directionalLightList;
    RwLLLink *link = head->next;
    while (link != head) {
        RwLLLink *next = link->next;
        RpLight *light = (RpLight *)((uint8_t *)link - offsetof(RpLight, inWorld));
        if (light != NULL && callback(light, data) == NULL) {
            return 1;
        }
        link = next;
    }
    return 1;
}

/* Calls the callback on every light tied to the sector (EAX = sector, EDX = callback, ECX = data), until it
 * says no; the sector. */
BMX_ADDR(0x004b9b2c)
__attribute__((regparm(3))) RpWorldSector *rpSectorForAllLights(RpWorldSector *sector,
                                                                RwObjectCallBack callback, void *data)
{
    RwLLLink *head = &sector->lightsInWorldSector;
    RwLLLink *link = head->next;
    while (link != head) {
        RwLLLink *next = link->next;
        RpLight *light = ((RpLightTie *)link)->light;
        if (light != NULL && callback(light, data) == NULL) {
            return sector;
        }
        link = next;
    }
    return sector;
}

/* Stack argument entries of the two walks above: they load the arguments and fall into the bodies. */
BMX_ADDR(0x004b9ac0)
int RpWorldForAllDirectionalLights(RwObjectCallBack callback, void *data)
{
    return rpWorldForAllDirectionalLights(callback, data);
}

BMX_ADDR(0x004b9b20)
RpWorldSector *RpWorldSectorForAllLights(RpWorldSector *sector, RwObjectCallBack callback, void *data)
{
    return rpSectorForAllLights(sector, callback, data);
}

/* The pipeline's light hook (stored at +0x10 of the world pipeline): lights the object with the world's
 * lights and those tied to its sector. Returns the sector, or the object when the sector is not lit. */
BMX_ADDR(0x004bae70)
void *rpWorldLightObject(void *object, RpLightApplyFn apply)
{
    RpWorldSector *sector = *(RpWorldSector **)((uint8_t *)object + 0xc);
    if ((*((uint8_t *)sector + 2) & 0x20) == 0) {
        return object;
    }
    RpLightApplyArgs args = {object, apply, &((RpWorld *)RwEngineInstance->curWorld)->surfaceProps};
    rpWorldForAllDirectionalLights(BMX_FNPTR(RwObjectCallBack, 0x004bae30, rpLightApplyToObject), &args);
    return rpSectorForAllLights(sector, BMX_FNPTR(RwObjectCallBack, 0x004bae30, rpLightApplyToObject), &args);
}

/* ---- atomics and the world ---------------------------------------------------------------------------- */

/* The world plugin's data of an atomic (at rpWorldAtomicOffset): the world it is in and its own sync
 * callback. */
typedef struct RpWorldAtomicData {
    RpWorld *world; /* 0x00 */
    void *sync;     /* 0x04 saved RpAtomic.object.sync */
} RpWorldAtomicData;

static RpWorldAtomicData *atomicData(const RpAtomic *atomic)
{
    return (RpWorldAtomicData *)((uint8_t *)atomic + rpWorldAtomicOffset);
}

/* Records the world in the atomic and refreshes its frame's objects; no sector ties yet. */
BMX_ADDR(0x004b82b8)
__attribute__((regparm(2))) RpWorld *rpWorldAddAtomicBody(RpWorld *world, RpAtomic *atomic)
{
    RpWorldAtomicData *data = atomicData(atomic);
    RwFrame *frame = atomic->object.object.parent;
    if (frame != NULL) {
        RwFrameUpdateObjects(frame);
    }
    data->world = world;
    atomic->stamp++;
    return world;
}

/* Frees what the atomic has cached for rendering and its ties with sectors, and leaves the world. */
BMX_ADDR(0x004b82f8)
__attribute__((regparm(2))) RpWorld *rpWorldRemoveAtomicBody(RpWorld *world, RpAtomic *atomic)
{
    RpWorldAtomicData *data = atomicData(atomic);
    if (atomic->repEntry != NULL) {
        RwResourcesFreeResEntry(atomic->repEntry);
    }
    RwLLLink *end = &atomic->llWorldSectorsInAtomic;
    RwLLLink *link = end->next;
    while (link != end) {
        RpTie *tie = (RpTie *)((uint8_t *)link - offsetof(RpTie, lWorldSectorInAtomic));
        link = link->next;
        if (tie->apAtom != NULL && tie->worldSector != NULL) {
            linkOut(&tie->lWorldSectorInAtomic);
            linkOut(&tie->lAtomicInWorldSector);
            RwEngineInstance->freeListFree(tieGlobals()->tieFreeList, tie);
        }
    }
    data->world = NULL;
    atomic->stamp++;
    return world;
}

/* RpClumpForAllAtomics callbacks: the world is the data argument. */
BMX_ADDR(0x004b8680)
void *rpWorldAddAtomicCallback(void *atomic, void *world)
{
    rpWorldAddAtomicBody(world, atomic);
    return atomic;
}

BMX_ADDR(0x004b8750)
void *rpWorldRemoveAtomicCallback(void *atomic, void *world)
{
    rpWorldRemoveAtomicBody(world, atomic);
    return atomic;
}

/* Destroys the ties of the atomic (the ones with both ends set; the others stay in its list). */
BMX_ADDR(0x004b78b0)
__attribute__((regparm(1))) void rpAtomicTiesDestroy(RpAtomic *atomic)
{
    RwLLLink *end = &atomic->llWorldSectorsInAtomic;
    RwLLLink *link = end->next;
    while (link != end) {
        RpTie *tie = (RpTie *)((uint8_t *)link - offsetof(RpTie, lWorldSectorInAtomic));
        link = link->next;
        if (tie->apAtom != NULL && tie->worldSector != NULL) {
            linkOut(&tie->lWorldSectorInAtomic);
            linkOut(&tie->lAtomicInWorldSector);
            RwEngineInstance->freeListFree(tieGlobals()->tieFreeList, tie);
        }
    }
}

/* a - b with b opaque to the optimiser (no rewrite as a + -b, which flips the sign of a NaN b). */
static double worldSub(double a, double b)
{
    __asm__("" : "+x"(b));
    return a - b;
}

/*
 * Ties the atomic to every sector its world bounding box touches. The box is the sphere's centre -+ radius;
 * it is skipped altogether when a minimum has its sign bit set (negative, also -0 and negative NaNs). The BSP
 * is walked with an explicit stack of 64 entries; the floats of the planes and the box are compared as
 * integers (signed), which equals the float order only for non-negative values. A plane's type is the byte
 * offset of its axis in the box (0, 4, 8). Each leaf gets a tie from the tie free list (not checked for
 * NULL), linked into the sector's list (the one for atomics with collision when flag bit 0 of the atomic is
 * set) and the atomic's list. EAX = world, EDX = atomic.
 */
BMX_ADDR(0x004b7920)
__attribute__((regparm(2))) void rpAtomicAddTies(RpWorld *world, RpAtomic *atomic)
{
    const RwSphere *sphere = RpAtomicGetWorldBoundingSphere(atomic);
    const float x = sphere->center.x;
    const float y = sphere->center.y;
    const float z = sphere->center.z;
    const double radius = (double)sphere->radius;
    float bounds[6];
    bounds[0] = (float)worldSub((double)x, radius);
    bounds[1] = (float)worldSub((double)y, radius);
    bounds[2] = (float)worldSub((double)z, radius);
    if ((int32_t)((uint32_t)floatBits(bounds[0]) | (uint32_t)floatBits(bounds[2]) |
                  (uint32_t)floatBits(bounds[1])) < 0) {
        return;
    }
    bounds[3] = (float)(radius + (double)x);
    bounds[4] = (float)((double)y + radius);
    bounds[5] = (float)((double)z + radius);
    RpSector *stack[65] = {0};
    int32_t top = 0;
    RpSector *node = world->rootSector;
    for (;;) {
        const int32_t type = *(const int32_t *)node;
        if (type >= 0) {
            const RpPlaneSector *plane = (const RpPlaneSector *)node;
            const int32_t low = floatBits(*(const float *)((const uint8_t *)bounds + type));
            const int32_t high = floatBits(*(const float *)((const uint8_t *)bounds + type + 0xc));
            if (low < floatBits(plane->leftValue)) {
                node = plane->leftSubTree;
                if (floatBits(plane->rightValue) >= high) {
                    continue;
                }
                stack[++top] = plane->rightSubTree;
                continue;
            }
            if (floatBits(plane->rightValue) < high) {
                node = plane->rightSubTree;
                continue;
            }
            node = stack[top--];
        } else {
            RpWorldSector *sector = (RpWorldSector *)node;
            RpTie *tie = RwEngineInstance->freeListAlloc(tieGlobals()->tieFreeList);
            tie->worldSector = sector;
            tie->apAtom = atomic;
            if ((atomic->object.object.flags & 1) == 0) {
                linkFirst(&sector->noCollAtomicsInWorldSector, &tie->lAtomicInWorldSector);
            } else {
                linkFirst(&sector->collAtomicsInWorldSector, &tie->lAtomicInWorldSector);
            }
            linkFirst(&atomic->llWorldSectorsInAtomic, &tie->lWorldSectorInAtomic);
            node = stack[top--];
        }
        if (top < 0) {
            break;
        }
    }
}

/*
 * The atomic's sync callback (the world plugin's replacement): counts a change, runs the callback it
 * replaced and, for an atomic in a world, renews its ties. Returns the atomic, NULL if the old callback did.
 */
BMX_ADDR(0x004b7ab0)
RpAtomic *WorldAtomicSync(RpAtomic *atomic)
{
    const int32_t offset = rpWorldAtomicOffset;
    atomic->stamp++;
    RpWorldAtomicData *data = (RpWorldAtomicData *)((uint8_t *)atomic + offset);
    if (((void *(BMX_CDECL *)(RpAtomic *))data->sync)(atomic) == NULL) {
        return NULL;
    }
    RpWorld *world = data->world;
    if (world != NULL) {
        rpAtomicTiesDestroy(atomic);
        rpAtomicAddTies(world, atomic);
    }
    return atomic;
}

static float worldAxis(const float *v, int32_t type)
{
    return *(const float *)((const uint8_t *)v + type);
}

/*
 * Rebuilds the camera's list of world sectors in view (EAX = world, EDX = camera; returns the world). The BSP
 * of the camera data's world is walked with the frustum's bounding box: a plane whose ranges the box overlaps
 * on one side only sends the walk there (the differences are rounded to floats and tested by sign bit), else
 * both subtrees are visited, the one on the camera's side of the plane first (last for worlds of kind 2;
 * position and plane compared as integer bit patterns). The explicit stack has 64 entries. A sector is in
 * view unless the corner of its bounding box nearest to a clip plane is outside it (distance > 0 as an
 * integer). The array grows by 50 entries; without memory the error is set and the list keeps what it has.
 */
BMX_ADDR(0x004b7e80)
__attribute__((regparm(2))) RpWorld *rpWorldCameraSyncSectors(RpWorld *world, RwCamera *camera)
{
    const int32_t offset = rpWorldCameraOffset;
    RpWorldCameraData *data = (RpWorldCameraData *)((uint8_t *)camera + offset);
    int32_t count = 0;
    const RwMatrix *ltm = RwFrameGetLTM(RwCameraGetFrame(camera));
    int32_t position[3];
    __builtin_memcpy(position, &ltm->pos, sizeof position);
    float inf[3];
    float sup[3];
    __builtin_memcpy(inf, &camera->frustumBoundBox[3], sizeof inf);
    __builtin_memcpy(sup, &camera->frustumBoundBox[0], sizeof sup);
    const int farFirst = world->flags == 2;
    RpSector *stack[65] = {0};
    int32_t top = 0;
    RpSector *node = data->world->rootSector;
    for (;;) {
        const int32_t type = node->type;
        if (type >= 0) {
            const RpPlaneSector *plane = (const RpPlaneSector *)node;
            const float left = (float)worldSub(worldAxis(inf, type), plane->leftValue);
            const float right = (float)worldSub(plane->rightValue, worldAxis(sup, type));
            if (floatBits(left) >= 0) {
                node = plane->rightSubTree;
                continue;
            }
            if (floatBits(right) >= 0) {
                node = plane->leftSubTree;
                continue;
            }
            const int beyond = *(const int32_t *)((const uint8_t *)position + type) > floatBits(plane->value);
            if (farFirst ? beyond : !beyond) {
                stack[++top] = plane->rightSubTree;
                node = plane->leftSubTree;
            } else {
                stack[++top] = plane->leftSubTree;
                node = plane->rightSubTree;
            }
            continue;
        }
        RpWorldSector *sector = (RpWorldSector *)node;
        const float *corners = (const float *)&sector->boundingBox;
        int32_t i = 0;
        for (; i < 6; i++) {
            const RwFrustumPlane *p = &camera->frustumPlanes[i];
            const double sum = ((double)p->normal.z * corners[p->closest[2] * 3 + 2] +
                                (double)p->normal.y * corners[p->closest[1] * 3 + 1]) +
                               (double)p->normal.x * corners[p->closest[0] * 3];
            if (floatBits((float)worldSub(sum, p->distance)) > 0) {
                break;
            }
        }
        if (i == 6) {
            if (count >= data->capacity) {
                const size_t size = (size_t)data->capacity * 4 + 200;
                RpWorldSector **sectors =
                    (RpWorldSector **)(data->sectors != NULL ? RwEngineInstance->memoryFuncs.rwrealloc(
                                                                   (void *)data->sectors, size)
                                                             : RwEngineInstance->memoryFuncs.rwmalloc(size));
                if (sectors == NULL) {
                    RwError error;
                    error.pluginID = kWorldPluginId;
                    error.errorCode = _rwerror(rwERR_NOMEM, size);
                    RwErrorSet(&error);
                    data->numSectors = count;
                    return world;
                }
                data->sectors = sectors;
                data->capacity += 50;
            }
            data->sectors[count++] = sector;
        }
        node = stack[top--];
        if (top < 0) {
            break;
        }
    }
    data->numSectors = count;
    return world;
}

enum { rwSTANDARDWORLDCAMERASYNC = 22 };

/*
 * The camera's sync callback (the world plugin's replacement): runs the callback it replaced, then for a
 * camera in a world rebuilds the camera's sector list and tells the device (standard function 22, with a
 * flag for worlds of kind 1). Counts the change; returns the camera, NULL if the old callback did.
 */
BMX_ADDR(0x004b7730)
RwCamera *WorldCameraSync(RwCamera *camera)
{
    const int32_t offset = rpWorldCameraOffset;
    RpWorldCameraData *data = (RpWorldCameraData *)((uint8_t *)camera + offset);
    if (((void *(BMX_CDECL *)(RwCamera *))data->sync)(camera) == NULL) {
        return NULL;
    }
    RpWorld *world = data->world;
    if (world != NULL) {
        rpWorldCameraSyncSectors(world, camera);
        ((void(BMX_CDECL *)(void *, void *, int32_t))(
             uintptr_t)RwEngineInstance->stdFunc[rwSTANDARDWORLDCAMERASYNC])(NULL, NULL, world->flags == 1);
    }
    camera->version++;
    return camera;
}

/* Destroys the ties of the light with sectors. */
BMX_ADDR(0x004b7b40)
__attribute__((regparm(1))) void rpLightTiesDestroy(RpLight *light)
{
    RwLLLink *end = &light->sectors;
    RwLLLink *link = end->next;
    while (link != end) {
        RpLightTie *tie = (RpLightTie *)((uint8_t *)link - offsetof(RpLightTie, worldSectorInLight));
        link = link->next;
        linkOut(&tie->worldSectorInLight);
        linkOut(&tie->lightInWorldSector);
        RwEngineInstance->freeListFree(tieGlobals()->lightTieFreeList, tie);
    }
}

/*
 * The light's sync callback (the world plugin's replacement): counts a change and runs the callback it
 * replaced (on failure it sets an error and still returns the light). A local light (type >= 0x80) in a world
 * with a frame is tied anew to the sectors of the box at its position -+ radius: a maximum with its sign bit
 * set is an error (NULL), negative minimums become 0. The BSP walk is rpAtomicAddTies' (64 entry stack, bit
 * pattern compares); each sector gets a tie from the light tie free list (not checked) in its light list and
 * the light's list of sectors.
 */
BMX_ADDR(0x004b7bb0)
RpLight *WorldLightSync(RpLight *light)
{
    const int32_t offset = rpWorldLightOffset;
    light->stamp++;
    if (((void *(BMX_CDECL *)(RpLight *))((void **)((uint8_t *)light + offset))[1])(light) == NULL) {
        RwError error;
        error.pluginID = kWorldPluginId;
        error.errorCode = _rwerror(rwERR_NULLP);
        RwErrorSet(&error);
        return light;
    }
    if ((int32_t)RpLightGetType(light) < 0x80) {
        return light;
    }
    RpWorld *world = *(RpWorld **)((uint8_t *)light + offset);
    RwFrame *frame = RpLightGetFrame(light);
    if (world == NULL || frame == NULL) {
        return light;
    }
    const float radius = light->radius;
    rpLightTiesDestroy(light);
    const RwMatrix *ltm = RwFrameGetLTM(frame);
    const float x = ltm->pos.x;
    const float y = ltm->pos.y;
    const float z = ltm->pos.z;
    float bounds[6];
    bounds[3] = (float)((double)radius + x);
    bounds[5] = (float)((double)radius + z);
    bounds[4] = (float)((double)radius + y);
    if ((int32_t)((uint32_t)floatBits(bounds[3]) | (uint32_t)floatBits(bounds[5]) |
                  (uint32_t)floatBits(bounds[4])) < 0) {
        RwError error;
        error.pluginID = kWorldPluginId;
        error.errorCode = _rwerror(0);
        RwErrorSet(&error);
        return NULL;
    }
    bounds[0] = (float)worldSub((double)x, radius);
    bounds[2] = (float)worldSub((double)z, radius);
    bounds[1] = (float)worldSub((double)y, radius);
    if ((int32_t)((uint32_t)floatBits(bounds[0]) | (uint32_t)floatBits(bounds[2]) |
                  (uint32_t)floatBits(bounds[1])) < 0) {
        for (int i = 0; i < 3; i++) {
            if (floatBits(bounds[i]) < 0) {
                bounds[i] = 0.0F;
            }
        }
    }
    RpSector *stack[65] = {0};
    int32_t top = 0;
    RpSector *node = world->rootSector;
    for (;;) {
        const int32_t type = node->type;
        if (type >= 0) {
            const RpPlaneSector *plane = (const RpPlaneSector *)node;
            const int32_t low = floatBits(worldAxis(bounds, type));
            const int32_t high = floatBits(worldAxis(bounds + 3, type));
            if (low < floatBits(plane->leftValue)) {
                node = plane->leftSubTree;
                if (floatBits(plane->rightValue) >= high) {
                    continue;
                }
                stack[++top] = plane->rightSubTree;
                continue;
            }
            if (floatBits(plane->rightValue) < high) {
                node = plane->rightSubTree;
                continue;
            }
            node = stack[top--];
        } else {
            RpWorldSector *sector = (RpWorldSector *)node;
            RpLightTie *tie = RwEngineInstance->freeListAlloc(tieGlobals()->lightTieFreeList);
            tie->light = light;
            tie->worldSector = sector;
            linkFirst(&sector->lightsInWorldSector, &tie->lightInWorldSector);
            linkFirst(&light->sectors, &tie->worldSectorInLight);
            node = stack[top--];
        }
        if (top < 0) {
            break;
        }
    }
    return light;
}

/*
 * The atomic's plugin data starts out empty; its sync callback is chained behind 0x4b7ab0. The halfword at
 * 0x64 is the render frame minus one.
 */
BMX_ADDR(0x004b9040)
void *WorldInitAtomicExt(void *atomic)
{
    RpAtomic *a = atomic;
    RpWorldAtomicData *data = atomicData(a);
    data->world = NULL;
    void *sync = a->object.sync;
    a->renderFrame = (uint16_t)(RwEngineInstance->renderFrame - 1);
    data->sync = sync;
    a->object.sync = RW_CB(void *, 0x004b7ab0);
    return atomic;
}

BMX_ADDR(0x004b9010)
void *WorldDeInitAtomicExt(void *atomic)
{
    RpAtomic *a = atomic;
    const int32_t offset = rpWorldAtomicOffset;
    rpAtomicTiesDestroy(a);
    a->object.sync = ((RpWorldAtomicData *)((uint8_t *)a + offset))->sync;
    return atomic;
}

BMX_ADDR(0x004b7b00)
void *WorldCopyAtomicExt(void *atomic)
{
    return atomic;
}

/* Clump copy: the clone joins the world of the source. */
BMX_ADDR(0x004b7b10)
void *WorldCopyClumpExt(void *clump, const void *source)
{
    RpWorld *world = *(RpWorld *const *)((const uint8_t *)source + rpWorldClumpOffset);
    if (world != NULL) {
        rpWorldAddClumpBody(world, clump);
    }
    return clump;
}

/* ---- lights ----------------------------------------------------------------------------------------------
 */

BMX_ADDR(0x004b8fc0)
void *WorldInitLightExt(void *light)
{
    RpLight *l = light;
    uint32_t *data = (uint32_t *)((uint8_t *)l + rpWorldLightOffset);
    data[0] = 0;
    data[1] = (uint32_t)(uintptr_t)l->object.sync;
    l->object.sync = RW_CB(void *, 0x004b7bb0);
    return light;
}

BMX_ADDR(0x004b8fa0)
void *WorldDeInitLightExt(void *light)
{
    rpLightTiesDestroy(light);
    return light;
}

BMX_ADDR(0x004b7e50)
void *WorldCopyLightExt(void *light, const void *source)
{
    RpWorld *world = *(RpWorld *const *)((const uint8_t *)source + rpWorldLightOffset);
    if (world != NULL) {
        rpWorldAddLightBody(world, light);
    }
    return light;
}

/* ---- cameras ----------------------------------------------------------------------------------------------
 */

/* The plugin takes over the camera's begin/end update and sync callbacks and keeps the old ones. */
BMX_ADDR(0x004b90c0)
void *WorldInitCameraExt(void *camera)
{
    RwCamera *c = camera;
    RpWorldCameraData *data = cameraData(c);
    data->sectors = NULL;
    data->numSectors = 0;
    data->capacity = 0;
    data->beginUpdate = c->beginUpdate;
    data->endUpdate = c->endUpdate;
    data->sync = c->object.sync;
    c->endUpdate = RW_CB(RwCameraUpdateFunc, 0x004b7700);
    c->beginUpdate = RW_CB(RwCameraUpdateFunc, 0x004b76c0);
    c->object.sync = RW_CB(void *, 0x004b7730);
    data->world = NULL;
    return camera;
}

BMX_ADDR(0x004b9080)
void *WorldDeInitCameraExt(void *camera)
{
    RwCamera *c = camera;
    const int32_t offset = rpWorldCameraOffset;
    rpWorldCameraDataDestroy(cameraData(c));
    RpWorldCameraData *data = (RpWorldCameraData *)((uint8_t *)c + offset);
    c->beginUpdate = data->beginUpdate;
    c->endUpdate = data->endUpdate;
    c->object.sync = data->sync;
    return camera;
}

/* Camera copy: the clone has no sectors in view yet and joins the world of the source. */
BMX_ADDR(0x004b77a0)
void *WorldCopyCameraExt(void *camera, const void *source)
{
    RpWorldCameraData *data = cameraData(camera);
    data->numSectors = 0;
    data->capacity = 0;
    data->sectors = NULL;
    RpWorld *world = ((const RpWorldCameraData *)((const uint8_t *)source + rpWorldCameraOffset))->world;
    if (world != NULL) {
        rpWorldAddCameraBody(world, camera);
    }
    return camera;
}

/* Begin update: the camera's world becomes the engine's current world, then the camera's own callback. */
BMX_ADDR(0x004b76c0)
RwCamera *WorldCameraBeginUpdate(RwCamera *camera)
{
    RpWorldCameraData *data = cameraData(camera);
    RwEngineInstance->curWorld = data->world;
    RwEngineInstance->renderFrame++;
    return data->beginUpdate(camera);
}

BMX_ADDR(0x004b7700)
RwCamera *WorldCameraEndUpdate(RwCamera *camera)
{
    RpWorldCameraData *data = cameraData(camera);
    RwEngineInstance->curWorld = NULL;
    return data->endUpdate(camera);
}

/* ---- the world a sector belongs to --------------------------------------------------------------------- */

/* Sector walk callback: stops (NULL) at the sector searched for and says so in the second word. */
typedef struct RpSectorSearch {
    void *target;
    int32_t found;
} RpSectorSearch;

BMX_ADDR(0x004b9d30)
void *rpWorldSectorFindCallback(void *sector, void *search)
{
    RpSectorSearch *s = search;
    if (sector != s->target) {
        return sector;
    }
    s->found = 1;
    return NULL;
}

/*
 * The world a sector belongs to: walks the engine's list of worlds. A world whose sectors form one block of
 * memory is tested by address range, any other by walking its BSP (an explicit stack, sixty-odd entries) for
 * the sector. NULL (and an error) without the world module.
 */
BMX_ADDR(0x004b9d50)
RpWorld *rpWorldSectorGetWorld(RpWorldSector *sector)
{
    if (rpWorldModuleCount == 0) {
        reportError(kWorldPluginId, rwERR_NOTOPEN);
        return NULL;
    }
    RwLLLink *head = &worldGlobals()->worlds;
    for (RwLLLink *link = head->next; link != head; link = link->next) {
        RpWorldListEntry *entry = (RpWorldListEntry *)((uint8_t *)link - offsetof(RpWorldListEntry, link));
        if ((entry->world->object.privateFlags & 1) != 0) {
            if ((uintptr_t)sector < (uintptr_t)entry->world ||
                (uintptr_t)sector >= (uintptr_t)entry->world + entry->size) {
                continue;
            }
            return entry->world;
        }
        RpSectorSearch search = {sector, 0};
        RpSector *stack[65] = {NULL};
        int32_t depth = 0;
        if (entry->world == NULL) {
            reportError(kWorldPluginId, rwERR_NULLP);
        } else {
            RpSector *node = entry->world->rootSector;
            do {
                if (node->type >= 0) {
                    stack[++depth] = ((RpPlaneSector *)node)->rightSubTree;
                    node = ((RpPlaneSector *)node)->leftSubTree;
                } else {
                    if (rpWorldSectorFindCallback(node, &search) == NULL) {
                        break;
                    }
                    node = stack[depth--];
                }
            } while (depth >= 0);
        }
        if (search.found != 0) {
            return entry->world;
        }
    }
    return NULL;
}

/* ---- the mesh stream plugin (0x50e) of geometries and sectors ----------------------------------------- */

/* Index of the material in the list, -1 when it is not there. */
BMX_ADDR(0x004c50a0)
int32_t _rpMaterialListFindMaterialIndex(const RpMaterialList *list, const RpMaterial *material)
{
    for (int32_t i = 0; i < list->numMaterials; i++) {
        if (list->materials[i] == material) {
            return i;
        }
    }
    return -1;
}

/* Bytes the mesh header takes in a stream chunk (the chunk size the size callbacks report). */
BMX_ADDR(0x004c4ea0)
int32_t _rpMeshSize(const RpMeshHeader *mesh)
{
    return (int32_t)(0xc + mesh->totalIndices * 4 + mesh->numMeshes * 8U);
}

/*
 * Writes the mesh header (flags, number of meshes, total indices) and for every mesh its index count and
 * the index of its material in the owner's material list (0 when it is not in it), then its indices as
 * 32-bit words. The second argument is unused. Returns the stream, NULL when a write failed.
 */
BMX_ADDR(0x004c4bc0)
RwStream *_rpMeshWrite(const RpMeshHeader *mesh, void *owner, RwStream *stream,
                       const RpMaterialList *materials)
{
    (void)owner;
    uint32_t header[3];
    header[0] = mesh->flags;
    header[1] = mesh->numMeshes;
    header[2] = mesh->totalIndices;
    if (RwStreamWriteInt32(stream, header, 0xc) == NULL) {
        return NULL;
    }
    const RpMesh *entry = (const RpMesh *)(mesh + 1);
    for (uint32_t n = mesh->numMeshes; n != 0; n--, entry++) {
        int32_t pair[2];
        pair[0] = (int32_t)entry->numIndices;
        pair[1] = _rpMaterialListFindMaterialIndex(materials, entry->material);
        if (pair[1] < 0) {
            pair[1] = 0;
        }
        if (RwStreamWriteInt32(stream, pair, 8) == NULL) {
            return NULL;
        }
        const uint16_t *index = entry->indices;
        for (uint32_t i = entry->numIndices; i != 0; i--) {
            uint32_t word = *index++;
            if (RwStreamWriteInt32(stream, &word, 4) == NULL) {
                return NULL;
            }
        }
    }
    return stream;
}

/*
 * Reads a mesh header written by _rpMeshWrite into one block (header, the meshes, then all indices as
 * 16-bit values) and stamps it with the next serial number of the mesh module. The block is not freed
 * when the stream runs dry. The second argument is unused. NULL on failure.
 */
BMX_ADDR(0x004c4d00)
RpMeshHeader *_rpMeshRead(RwStream *stream, void *owner, const RpMaterialList *materials)
{
    (void)owner;
    uint32_t header[3];
    if (RwStreamReadInt32(stream, header, 0xc) == NULL) {
        return NULL;
    }
    RpMeshHeader *mesh = RwEngineInstance->memoryFuncs.rwmalloc(header[2] * 2 + 0x10 + header[1] * 0x10);
    if (mesh == NULL) {
        return NULL;
    }
    mesh->flags = header[0];
    mesh->numMeshes = (uint16_t)header[1];
    mesh->serialNum = *(uint16_t *)((uint8_t *)RwEngineInstance + rpMeshOffset);
    mesh->firstMeshOffset = 0;
    mesh->totalIndices = header[2];
    (*(int16_t *)((uint8_t *)RwEngineInstance + rpMeshOffset))++;
    RpMesh *entry = (RpMesh *)(mesh + 1);
    uint16_t *index = (uint16_t *)(entry + header[1]);
    for (uint32_t n = mesh->numMeshes; n != 0; n--, entry++) {
        int32_t pair[2];
        if (RwStreamReadInt32(stream, pair, 8) == NULL) {
            return NULL;
        }
        entry->numIndices = (uint32_t)pair[0];
        RpMaterial *material = _rpMaterialListGetMaterial(materials, pair[1]);
        entry->indices = index;
        entry->material = material;
        for (uint32_t i = entry->numIndices; i != 0; i--) {
            uint32_t word;
            if (RwStreamReadInt32(stream, &word, 4) == NULL) {
                return NULL;
            }
            *index++ = (uint16_t)word;
        }
    }
    return mesh;
}

/* Stream callbacks of the geometry's mesh: the mesh at +0x40, the materials at +0x10. */
BMX_ADDR(0x004b8cc0)
RwStream *writeGeometryMesh(RwStream *stream, int32_t length, const void *geometry, int32_t offset,
                            int32_t size)
{
    (void)length;
    (void)offset;
    (void)size;
    const RpGeometry *g = geometry;
    return _rpMeshWrite(g->mesh, (void *)(uintptr_t)geometry, stream, &g->matList);
}

BMX_ADDR(0x004b8d00)
RwStream *readGeometryMesh(RwStream *stream, int32_t length, void *geometry, int32_t offset, int32_t size)
{
    (void)length;
    (void)offset;
    (void)size;
    RpGeometry *g = geometry;
    g->mesh = _rpMeshRead(stream, g, &g->matList);
    return g->mesh != NULL ? stream : NULL;
}

BMX_ADDR(0x004b8d40)
int32_t sizeGeometryMesh(const void *geometry, int32_t offset, int32_t size)
{
    (void)offset;
    (void)size;
    return _rpMeshSize(((const RpGeometry *)geometry)->mesh);
}

/* The same for a world sector: the mesh at +0x58, the materials of the world the sector is in. */
BMX_ADDR(0x004b8d60)
RwStream *writeSectorMesh(RwStream *stream, int32_t length, const void *sector, int32_t offset, int32_t size)
{
    (void)length;
    (void)offset;
    (void)size;
    RpWorld *world = rpWorldSectorGetWorld((RpWorldSector *)(uintptr_t)sector);
    return _rpMeshWrite(((const RpWorldSector *)sector)->mesh, world, stream, &world->matList);
}

BMX_ADDR(0x004b8da0)
RwStream *readSectorMesh(RwStream *stream, int32_t length, void *sector, int32_t offset, int32_t size)
{
    (void)length;
    (void)offset;
    (void)size;
    RpWorldSector *s = sector;
    RpWorld *world = rpWorldSectorGetWorld(s);
    s->mesh = _rpMeshRead(stream, world, &world->matList);
    return s->mesh != NULL ? stream : NULL;
}

BMX_ADDR(0x004b8de0)
int32_t sizeSectorMesh(const void *sector, int32_t offset, int32_t size)
{
    (void)offset;
    (void)size;
    return _rpMeshSize(((const RpWorldSector *)sector)->mesh);
}
