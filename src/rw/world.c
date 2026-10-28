/*
 * RenderWare 3.1 world (RpWorld): attaching cameras, clumps and lights, the ties between objects and BSP
 * sectors, the for-all walks, destruction, materials and the PVS plugin.
 *
 * Most functions of the original fail through RwErrorSet with the world plugin's id (2) or the PVS plugin's
 * id (0x106) and a core error code, and return 0 (or -1 from the counting getters).
 */
#include "rw/rwworld.h"

#include "rw/rwclump.h"

/*
 * The line and ray walks of the BSP (the same intersection structure is passed twice: in EDX and again on
 * the stack, where the original reads it). Excluded from the port for now.
 */
ORIG_FN_REGPARM(3, 0x004bcd90, void, rpWorldForAllSectorsIntersectingLine,
                (RpWorld * world, RpIntersection *intersection, RpIntersectionCallBackWorldSector callback,
                 int32_t unused0, int32_t unused1, int32_t unused2, RpIntersection *again, void *data));
ORIG_FN_REGPARM(3, 0x004bda80, void, rpWorldForAllSectorsIntersectingRay,
                (RpWorld * world, RpIntersection *intersection, RpIntersectionCallBackWorldSector callback,
                 int32_t unused0, int32_t unused1, int32_t unused2, RpIntersection *again, void *data));

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

/* Puts the camera into the world (the original loads both arguments into EAX and EDX first). */
BMX_ADDR(0x004b81f0)
RpWorld *RpWorldAddCamera(RpWorld *world, RwCamera *camera)
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
BMX_ADDR(0x004b86a0)
RpWorld *RpWorldAddClump(RpWorld *world, RpClump *clump)
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
BMX_ADDR(0x004b8af0)
RpWorld *RpWorldAddLight(RpWorld *world, RpLight *light)
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

/* ---- walking the BSP --------------------------------------------------------------------------------- */

/*
 * Calls the callback for the world sectors an intersection touches. A point walks the tree (both children
 * where they overlap), an atomic visits the sectors it is tied to (keeping a marker in the list so the
 * callback may remove the current tie); lines and rays are the original's. The callback's result only matters
 * for atomics. Anything else is error 1. A point's coordinates are compared as integers, like the plane
 * values.
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
        rpWorldForAllSectorsIntersectingRay(world, intersection, callback, 0, 0, 0, intersection, data);
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
 * EAX and falls into this; its tail has no other entry, so the whole thing is ported at the stub's address.
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

/* Frees the PVS data of the world and of its sectors. (Stub at 0x4cc320 and tail ported as one, like above.)
 */
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
    pvs->pad38 = 0;
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
