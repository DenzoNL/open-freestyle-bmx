/*
 * RenderWare 3.1 clumps, atomics, geometries and materials (rpworld), plus the binary world, sector and
 * material list readers/writers that live in the same code range.
 *
 * Most of this is integer-only stream reading. Internal helpers the original calls with register arguments
 * are regparm(n). The small routines the engine only reaches through pointers (the atomic's sync and render
 * callbacks, the clump's default callback and the qsort comparators of the mesh builder) are not functions
 * of their own in the exe's symbol list; they stay original code and are passed by address (RW_CB).
 */
#include "rw/rwclump.h"

#include "compat/crt.h"
#include "rw/rwim.h"

/* Callbacks that are still original code. */
#define RP_ATOMIC_SYNC_CALLBACK   0x004b1490
#define RP_ATOMIC_RENDER_CALLBACK 0x004b3350
#define RP_CLUMP_DEFAULT_CALLBACK 0x004b20f0

/* The pluginID RpWorld reports its errors with. */
enum { kRpErrorPlugin = 2 };

static void rpReport(int32_t code)
{
    RwError error;
    error.pluginID = kRpErrorPlugin;
    error.errorCode = _rwerror(code);
    RwErrorSet(&error);
}

static void rpReportNoMem(int32_t size)
{
    RwError error;
    error.pluginID = kRpErrorPlugin;
    error.errorCode = _rwerror(rwERR_NOMEM, size);
    RwErrorSet(&error);
}

/* Module data in the engine instance. */
static RpClumpGlobals *clumpGlobals(void)
{
    return (RpClumpGlobals *)((uint8_t *)RwEngineInstance + rpClumpModuleOffset);
}

static RwFreeList *geometryFreeList(void)
{
    return *(RwFreeList **)((uint8_t *)RwEngineInstance + rpGeometryOffset);
}

static RwFreeList *materialFreeList(void)
{
    return *(RwFreeList **)((uint8_t *)RwEngineInstance + rpMaterialOffset);
}

static RwFreeList *frameFreeList(void)
{
    return *(RwFreeList **)((uint8_t *)RwEngineInstance + rwFrameOffset);
}

static void *engineAlloc(size_t size)
{
    return RwEngineInstance->memoryFuncs.rwmalloc(size);
}

static void engineFree(void *block)
{
    RwEngineInstance->memoryFuncs.rwfree(block);
}

/* ---- frames (the clone helpers the clump code needs) ------------------------------------------------ */

/* Unlinks a frame from the engine's dirty list. */
static void frameUnlinkDirty(RwFrame *frame)
{
    if ((frame->object.privateFlags & 3) != 0) {
        frame->dirtyLink.prev->next = frame->dirtyLink.next;
        frame->dirtyLink.next->prev = frame->dirtyLink.prev;
    }
}

/* Adds a hierarchy's root to the dirty list (once) and flags it for a full resync. */
static void frameQueueDirty(RwFrame *root)
{
    const uint8_t flags = root->object.privateFlags;
    if ((flags & 3) == 0) {
        root->dirtyLink.next = RwEngineInstance->dirtyFrameList.next;
        root->dirtyLink.prev = &RwEngineInstance->dirtyFrameList;
        RwEngineInstance->dirtyFrameList.next->prev = &root->dirtyLink;
        RwEngineInstance->dirtyFrameList.next = &root->dirtyLink;
    }
    root->object.privateFlags = flags | 3;
}

/* Destroys a frame's subtree (children first) without telling the parent's hierarchy more than needed. */
BMX_ADDR(0x004d2530)
__attribute__((regparm(1))) void rwFrameDestroyRecursive(RwFrame *frame)
{
    RwFrame *child = frame->child;
    while (child != NULL) {
        RwFrame *next = child->next;
        rwFrameDestroyRecursive(child);
        child = next;
    }
    _rwPluginRegistryDeInitObject(&rwFrameRegistry, frame);
    if (frame->object.parent != NULL) {
        RwFrameRemoveChild(frame);
    }
    frameUnlinkDirty(frame);
    child = frame->child;
    while (child != NULL) {
        child->object.parent = NULL;
        child = child->next;
    }
    RwEngineInstance->freeListFree(frameFreeList(), frame);
}

/* Clones a frame and its children (EAX = frame, EDX = the clone hierarchy's root or NULL). */
BMX_ADDR(0x004d2350)
__attribute__((regparm(2))) RwFrame *rwFrameCloneRecursive(RwFrame *frame, RwFrame *root)
{
    RwFrame *clone = RwEngineInstance->freeListAlloc(frameFreeList());
    if (clone == NULL) {
        return NULL;
    }
    if (root == NULL) {
        root = clone;
    }
    *(uint32_t *)&clone->object = *(const uint32_t *)&frame->object;
    clone->object.parent = NULL;
    clone->objectList.prev = &clone->objectList;
    clone->objectList.next = &clone->objectList;
    for (int i = 0; i < 16; i++) {
        ((uint32_t *)&clone->modelling)[i] = ((const uint32_t *)&frame->modelling)[i];
    }
    clone->root = root;
    clone->next = NULL;
    clone->child = NULL;
    frame->root = clone;
    for (RwFrame *child = frame->child; child != NULL; child = child->next) {
        RwFrame *childClone = rwFrameCloneRecursive(child, root);
        if (childClone == NULL) {
            RwFrame *c = clone->child;
            while (c != NULL) {
                RwFrame *next = c->next;
                rwFrameDestroyRecursive(c);
                c = next;
            }
            _rwPluginRegistryDeInitObject(&rwFrameRegistry, clone);
            if (clone->object.parent != NULL) {
                RwFrameRemoveChild(clone);
            }
            frameUnlinkDirty(clone);
            c = clone->child;
            while (c != NULL) {
                c->object.parent = NULL;
                c = c->next;
            }
            RwEngineInstance->freeListFree(frameFreeList(), clone);
            return NULL;
        }
        childClone->next = clone->child;
        clone->child = childClone;
        childClone->object.parent = clone;
    }
    _rwPluginRegistryInitObject(&rwFrameRegistry, clone);
    _rwPluginRegistryCopyObject(&rwFrameRegistry, clone, frame);
    return clone;
}

/* Clones the hierarchy below (and including) a frame; the clone's subtree is flagged for a resync. */
BMX_ADDR(0x004d25e0)
RwFrame *RwFrameCloneHierarchy(RwFrame *frame)
{
    RwFrame *clone = rwFrameCloneRecursive(frame, NULL);
    if (clone != NULL) {
        clone->object.privateFlags = rwFRAMEPRIVATESUBTREESYNCLTM | rwFRAMEPRIVATESUBTREESYNCOBJ;
        frameQueueDirty(clone->root);
    }
    return clone;
}

/* Gives a hierarchy's root to a frame and everything below it (a parentless frame is its own root). */
BMX_ADDR(0x004d2680)
void rwFrameResetRoot(RwFrame *frame)
{
    RwFrame *root = frame->object.parent == NULL ? frame : ((RwFrame *)frame->object.parent)->root;
    frame->version++;
    frame->root = root;
    for (RwFrame *child = frame->child; child != NULL; child = child->next) {
        rwFrameSetRoot(child, root);
    }
}

/* Makes `child` the first child of `parent` (detaching it from any previous parent first). */
BMX_ADDR(0x004d2b50)
RwFrame *RwFrameAddChild(RwFrame *parent, RwFrame *child)
{
    if (child->object.parent != NULL) {
        RwFrameRemoveChild(child);
    }
    child->next = parent->child;
    parent->child = child;
    child->object.parent = parent;
    if ((child->object.privateFlags & 3) != 0) {
        child->dirtyLink.prev->next = child->dirtyLink.next;
        child->dirtyLink.next->prev = child->dirtyLink.prev;
        child->object.privateFlags = 0;
    }
    RwFrame *root = parent->root;
    child->version++;
    child->root = root;
    for (RwFrame *c = child->child; c != NULL; c = c->next) {
        rwFrameSetRoot(c, root);
    }
    child->object.privateFlags |= rwFRAMEPRIVATESUBTREESYNCLTM | rwFRAMEPRIVATESUBTREESYNCOBJ;
    frameQueueDirty(child->root);
    child->version++;
    parent->version++;
    return parent;
}

/* ---- atomics and clumps ----------------------------------------------------------------------------- */

/* Counts an atomic (callback of RpClumpGetNumAtomics); returns the atomic. */
BMX_ADDR(0x004b1430)
RpAtomic *rwClumpCountAtomic(RpAtomic *atomic, int32_t *count)
{
    (*count)++;
    return atomic;
}

BMX_ADDR(0x004b1440)
int32_t RpClumpGetNumAtomics(RpClump *clump)
{
    int32_t count = 0;
    RwLLLink *link = clump->atomicList.next;
    while (link != &clump->atomicList) {
        RpAtomic *atomic = (RpAtomic *)((uint8_t *)link - offsetof(RpAtomic, inClumpLink));
        link = link->next;
        if (rwClumpCountAtomic(atomic, &count) == NULL) {
            break;
        }
    }
    return count;
}

/* Renders the atomics whose flags have bit 2 set: updates the frame's LTM, then the atomic's render callback.
 */
BMX_ADDR(0x004b1940)
RpClump *RpClumpRender(RpClump *clump)
{
    for (RwLLLink *link = clump->atomicList.next; link != &clump->atomicList; link = link->next) {
        RpAtomic *atomic = (RpAtomic *)((uint8_t *)link - offsetof(RpAtomic, inClumpLink));
        if ((atomic->object.object.flags & 4) != 0) {
            RwFrameGetLTM(atomic->object.object.parent);
            atomic->renderCallBack(atomic);
        }
    }
    return clump;
}

/* Calls callback for every atomic of the clump until it returns NULL (the next link is read first). */
BMX_ADDR(0x004b1990)
RpClump *RpClumpForAllAtomics(RpClump *clump, RwObjectCallBack callback, void *data)
{
    RwLLLink *link = clump->atomicList.next;
    while (link != &clump->atomicList) {
        RpAtomic *atomic = (RpAtomic *)((uint8_t *)link - offsetof(RpAtomic, inClumpLink));
        link = link->next;
        if (callback(atomic, data) == NULL) {
            break;
        }
    }
    return clump;
}

/* Makes the geometry the atomic's: takes a reference, drops the old one, copies the bounding sphere. */
static void atomicSetGeometry(RpAtomic *atomic, RpGeometry *geometry)
{
    RpGeometry *old = atomic->geometry;
    if (geometry == old) {
        return;
    }
    if (geometry != NULL) {
        RpGeometryAddRef(geometry);
        old = atomic->geometry;
    }
    if (old != NULL) {
        RpGeometryDestroy(old);
    }
    atomic->geometry = geometry;
    if (geometry != NULL) {
        atomic->boundingSphere = geometry->morphTarget[0].boundingSphere;
    }
    if (atomic->object.object.parent != NULL) {
        RwFrameUpdateObjects(atomic->object.object.parent);
    }
    atomic->stamp++;
}

/* A new atomic: no frame, geometry or clump, identity interpolator, default render callback. */
static RpAtomic *atomicCreate(void)
{
    RpAtomic *atomic = RwEngineInstance->freeListAlloc(clumpGlobals()->atomicFreeList);
    if (atomic == NULL) {
        return NULL;
    }
    atomic->stamp++;
    atomic->object.object.type = 1;
    atomic->object.object.privateFlags = 1;
    atomic->object.object.subType = 0;
    atomic->object.object.parent = NULL;
    atomic->repEntry = NULL;
    atomic->object.object.flags = 5;
    atomic->object.sync = RW_CB(void *, RP_ATOMIC_SYNC_CALLBACK);
    _rwObjectHasFrameSetFrame(&atomic->object, NULL);
    atomic->object.object.privateFlags |= 1;
    atomic->llWorldSectorsInAtomic.prev = &atomic->llWorldSectorsInAtomic;
    atomic->llWorldSectorsInAtomic.next = &atomic->llWorldSectorsInAtomic;
    atomic->geometry = NULL;
    atomic->interpolator.startMorphTarget = 0;
    atomic->interpolator.endMorphTarget = 0;
    atomic->clump = NULL;
    atomic->inClumpLink.next = NULL;
    atomic->inClumpLink.prev = NULL;
    atomic->interpolator.position = 0.0F;
    atomic->worldBoundingSphere.radius = 0.0F;
    atomic->worldBoundingSphere.center.z = 0.0F;
    atomic->worldBoundingSphere.center.y = 0.0F;
    atomic->worldBoundingSphere.center.x = 0.0F;
    atomic->boundingSphere.radius = 0.0F;
    atomic->boundingSphere.center.z = 0.0F;
    atomic->boundingSphere.center.y = 0.0F;
    atomic->boundingSphere.center.x = 0.0F;
    atomic->interpolator.flags = 3;
    atomic->interpolator.time = 1.0F;
    atomic->interpolator.recipTime = 1.0F;
    atomic->stamp = 1;
    atomic->renderCallBack = RW_CB(RpAtomicRenderCallBack, RP_ATOMIC_RENDER_CALLBACK);
    _rwPluginRegistryInitObject(&rpAtomicRegistry, atomic);
    return atomic;
}

/* Destroys an atomic: plugin data, render resources, geometry, frame link and memory. Always 1. */
BMX_ADDR(0x004b1b64)
__attribute__((regparm(1))) int RpAtomicDestroy(RpAtomic *atomic)
{
    _rwPluginRegistryDeInitObject(&rpAtomicRegistry, atomic);
    if (atomic->repEntry != NULL) {
        RwResourcesFreeResEntry(atomic->repEntry);
    }
    if (atomic->geometry != NULL) {
        RpGeometryDestroy(atomic->geometry);
        atomic->geometry = NULL;
        if (atomic->object.object.parent != NULL) {
            RwFrameUpdateObjects(atomic->object.object.parent);
        }
        atomic->stamp++;
    }
    _rwObjectHasFrameReleaseFrame(&atomic->object);
    RwEngineInstance->freeListFree(clumpGlobals()->atomicFreeList, atomic);
    return 1;
}

/* A copy of an atomic: same flags, geometry (a new reference), interpolator and plugin data; no frame. */
BMX_ADDR(0x004b1c74)
__attribute__((regparm(1))) RpAtomic *RpAtomicClone(RpAtomic *source)
{
    RpAtomic *clone = atomicCreate();
    if (clone == NULL) {
        return NULL;
    }
    const uint32_t objectHeader = *(const uint32_t *)&source->object.object;
    clone->object.object.parent = NULL;
    *(uint32_t *)&clone->object.object = objectHeader;
    _rwObjectHasFrameSetFrame(&clone->object, NULL);
    clone->stamp++;
    clone->object.object.privateFlags |= 1;
    atomicSetGeometry(clone, source->geometry);
    clone->boundingSphere = source->boundingSphere;
    clone->renderCallBack = source->renderCallBack;
    clone->interpolator.flags = source->interpolator.flags;
    clone->interpolator.startMorphTarget = source->interpolator.startMorphTarget;
    clone->interpolator.endMorphTarget = source->interpolator.endMorphTarget;
    clone->interpolator.time = source->interpolator.time;
    clone->interpolator.recipTime = source->interpolator.recipTime;
    clone->interpolator.position = source->interpolator.position;
    _rwPluginRegistryCopyObject(&rpAtomicRegistry, clone, source);
    return clone;
}

/* Gives the atomic its frame and marks it changed (EAX = atomic, EDX = frame). */
BMX_ADDR(0x004b3318)
__attribute__((regparm(2))) RpAtomic *RpAtomicSetFrame(RpAtomic *atomic, RwFrame *frame)
{
    _rwObjectHasFrameSetFrame(&atomic->object, frame);
    atomic->object.object.privateFlags |= 1;
    atomic->stamp++;
    return atomic;
}

/* Links the atomic into the clump's atomic list (EAX = clump, EDX = atomic). */
BMX_ADDR(0x004b2278)
__attribute__((regparm(2))) void RpClumpAddAtomic(RpClump *clump, RpAtomic *atomic)
{
    atomic->inClumpLink.prev = &clump->atomicList;
    atomic->inClumpLink.next = clump->atomicList.next;
    clump->atomicList.next->prev = &atomic->inClumpLink;
    clump->atomicList.next = &atomic->inClumpLink;
    atomic->clump = clump;
    atomic->stamp++;
    clump->stamp++;
}

/* Clone callback: copies one atomic into the new clump on the clone of its frame. */
BMX_ADDR(0x004b1e50)
RpAtomic *rwClumpCloneAtomic(RpAtomic *atomic, RwClumpCloneData *data)
{
    RpAtomic *clone = RpAtomicClone(atomic);
    if (clone == NULL) {
        data->ok = 0;
        return NULL;
    }
    if (atomic->object.object.parent == NULL) {
        RpAtomicDestroy(clone);
        RwError error;
        error.pluginID = kRpErrorPlugin;
        error.errorCode = _rwerror(5);
        RwErrorSet(&error);
        return NULL;
    }
    RpAtomicSetFrame(clone, ((RwFrame *)atomic->object.object.parent)->root);
    RpClumpAddAtomic(data->clone, clone);
    return atomic;
}

/* Destroys an atomic (callback form: ignores its data argument); returns the atomic. */
BMX_ADDR(0x004b21c0)
RpAtomic *rwClumpDestroyAtomic(RpAtomic *atomic, void *data)
{
    (void)data;
    RpAtomicDestroy(atomic);
    return atomic;
}

/* Frees a half-built or finished clump: plugin data, atomics, frame hierarchy and memory. */
static void clumpRelease(RpClump *clump)
{
    _rwPluginRegistryDeInitObject(&rpClumpRegistry, clump);
    RwLLLink *link = clump->atomicList.next;
    while (link != &clump->atomicList) {
        RpAtomic *atomic = (RpAtomic *)((uint8_t *)link - offsetof(RpAtomic, inClumpLink));
        link = link->next;
        if (rwClumpDestroyAtomic(atomic, NULL) == NULL) {
            break;
        }
    }
    if (clump->object.parent != NULL) {
        RwFrameDestroyHierarchy(clump->object.parent);
    }
    RwEngineInstance->freeListFree(clumpGlobals()->clumpFreeList, clump);
}

/* A new empty clump with the default callback. */
static RpClump *clumpCreate(void)
{
    RpClump *clump = RwEngineInstance->freeListAlloc(clumpGlobals()->clumpFreeList);
    if (clump != NULL) {
        clump->stamp = 0;
        clump->inWorldLink.next = NULL;
        clump->inWorldLink.prev = NULL;
        clump->object.parent = NULL;
        clump->object.privateFlags = 0;
        clump->object.flags = 0;
        clump->object.subType = 0;
        clump->atomicList.prev = &clump->atomicList;
        clump->atomicList.next = &clump->atomicList;
        clump->callback = RW_CB(void *, RP_CLUMP_DEFAULT_CALLBACK);
        clump->object.type = 2;
        _rwPluginRegistryInitObject(&rpClumpRegistry, clump);
    }
    return clump;
}

/* A copy of a clump: its frame hierarchy cloned, every atomic cloned onto the matching frame. */
BMX_ADDR(0x004b1ee0)
RpClump *RpClumpClone(RpClump *source)
{
    RpClump *clone = clumpCreate();
    if (clone == NULL) {
        return NULL;
    }
    clone->object.parent = RwFrameCloneHierarchy(source->object.parent);
    clone->stamp++;
    if (clone->object.parent == NULL) {
        clumpRelease(clone);
        return NULL;
    }
    RwClumpCloneData data = {source, clone, 1};
    RwLLLink *link = source->atomicList.next;
    while (link != &source->atomicList) {
        RpAtomic *atomic = (RpAtomic *)((uint8_t *)link - offsetof(RpAtomic, inClumpLink));
        link = link->next;
        if (rwClumpCloneAtomic(atomic, &data) == NULL) {
            break;
        }
    }
    rwFrameResetRoot(source->object.parent);
    if (data.ok != 0) {
        clone->callback = source->callback;
        _rwPluginRegistryCopyObject(&rpClumpRegistry, clone, source);
        return clone;
    }
    clumpRelease(clone);
    return NULL;
}

/* Destroys a clump with its atomics and frames (its entry loads the argument for the body at 0x4b21e4). */
BMX_ADDR(0x004b21e0)
int RpClumpDestroy(RpClump *clump)
{
    clumpRelease(clump);
    return 1;
}

/* One atomic chunk of a clump stream (EAX = stream, EDX = the clump's frame list). */
BMX_ADDR(0x004b2ca0)
__attribute__((regparm(2))) RpAtomic *RpAtomicStreamRead(RwStream *stream, RwFrameList *frameList)
{
    uint32_t length;
    uint32_t version;
    if (RwStreamFindChunk(stream, 1, &length, &version) == 0) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        rpReport(rwERR_BADVERSION);
        return NULL;
    }
    int32_t header[3] = {0, 0, 0};
    if (RwStreamRead(stream, header, length) != length) {
        return NULL;
    }
    RwMemNative32(header, 0xc);
    RpAtomic *atomic = atomicCreate();
    if (atomic == NULL) {
        _rwFrameListDeinitialize(frameList);
        return NULL;
    }
    atomic->object.object.flags = (uint8_t)header[1];
    atomic->stamp++;
    if (RwStreamFindChunk(stream, 0xf, NULL, &version) == 0 || version < 0x301 || version > 0x303) {
        _rwFrameListDeinitialize(frameList);
        RpAtomicDestroy(atomic);
        return NULL;
    }
    RpGeometry *geometry = RpGeometryStreamRead(stream);
    if (geometry == NULL) {
        _rwFrameListDeinitialize(frameList);
        RpAtomicDestroy(atomic);
        return NULL;
    }
    atomicSetGeometry(atomic, geometry);
    _rwObjectHasFrameSetFrame(&atomic->object, frameList->frames[header[0]]);
    atomic->stamp++;
    atomic->object.object.privateFlags |= 1;
    RpGeometryDestroy(geometry);
    if (_rwPluginRegistryReadDataChunks(&rpAtomicRegistry, stream, atomic) == NULL) {
        return NULL;
    }
    return atomic;
}

/* Reads a clump chunk: struct (atomic count), frame list, the atomics, then the plugin data. */
BMX_ADDR(0x004b2790)
RpClump *RpClumpStreamRead(RwStream *stream)
{
    uint32_t length;
    uint32_t version;
    int32_t numAtomics;
    RwFrameList frameList;
    if (RwStreamFindChunk(stream, 1, &length, &version) == 0) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        rpReport(rwERR_BADVERSION);
        return NULL;
    }
    if (RwStreamRead(stream, &numAtomics, 4) != 4) {
        return NULL;
    }
    RwMemNative32(&numAtomics, 4);
    RpClump *clump = clumpCreate();
    if (clump == NULL) {
        return NULL;
    }
    if (RwStreamFindChunk(stream, 0xe, NULL, &version) == 0) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        clumpRelease(clump);
        rpReport(rwERR_BADVERSION);
        return NULL;
    }
    if (_rwFrameListStreamRead(stream, &frameList) == NULL) {
        clumpRelease(clump);
        return NULL;
    }
    clump->object.parent = frameList.frames[0];
    clump->stamp++;
    for (int32_t i = 0; i < numAtomics; i++) {
        if (RwStreamFindChunk(stream, 0x14, NULL, &version) == 0 || version < 0x301 || version > 0x303) {
            clumpRelease(clump);
            return NULL;
        }
        RpAtomic *atomic = RpAtomicStreamRead(stream, &frameList);
        if (atomic == NULL) {
            clumpRelease(clump);
            return NULL;
        }
        RpClumpAddAtomic(clump, atomic);
    }
    _rwFrameListDeinitialize(&frameList);
    if (_rwPluginRegistryReadDataChunks(&rpClumpRegistry, stream, clump) == NULL) {
        clumpRelease(clump);
        return NULL;
    }
    return clump;
}

/* ---- geometry accessors ------------------------------------------------------------------------------ */

/* The checks every geometry accessor starts with: the module is open and the object is not NULL. */
static int geometryCheck(const void *object)
{
    if (rpGeometryModuleCount == 0) {
        rpReport(rwERR_NOTOPEN);
        return 0;
    }
    if (object == NULL) {
        rpReport(rwERR_NULLP);
        return 0;
    }
    return 1;
}

BMX_ADDR(0x004b43e0)
RpMorphTarget *RpGeometryGetMorphTarget(RpGeometry *geometry, int32_t index)
{
    if (!geometryCheck(geometry)) {
        return NULL;
    }
    if (index >= 0 && index < geometry->numMorphTargets) {
        return &geometry->morphTarget[index];
    }
    return NULL;
}

BMX_ADDR(0x004b44e0)
RwTexCoords *RpGeometryGetTexCoords(RpGeometry *geometry)
{
    return geometryCheck(geometry) ? geometry->texCoords : NULL;
}

BMX_ADDR(0x004b4550)
int32_t RpGeometryGetNumVertices(const RpGeometry *geometry)
{
    return geometryCheck(geometry) ? geometry->numVertices : -1;
}

BMX_ADDR(0x004b45d0)
ZVec3 *RpMorphTargetGetVertices(const RpMorphTarget *morphTarget)
{
    return geometryCheck(morphTarget) ? morphTarget->verts : NULL;
}

BMX_ADDR(0x004b46b0)
RpTriangle *RpGeometryGetTriangles(RpGeometry *geometry)
{
    return geometryCheck(geometry) ? geometry->triangles : NULL;
}

BMX_ADDR(0x004b4720)
int32_t RpGeometryGetNumTriangles(const RpGeometry *geometry)
{
    return geometryCheck(geometry) ? geometry->numTriangles : -1;
}

/* The index-th material of the geometry; reports an error for an index outside the list. */
BMX_ADDR(0x004b47a0)
RpMaterial *RpGeometryGetMaterial(RpGeometry *geometry, int32_t index)
{
    if (!geometryCheck(geometry)) {
        return NULL;
    }
    if (index < 0 || geometry->matList.numMaterials <= index) {
        rpReport(2);
        return NULL;
    }
    return geometry->matList.materials[index];
}

BMX_ADDR(0x004b4ce0)
int32_t RpGeometryGetNumMaterials(const RpGeometry *geometry)
{
    return geometryCheck(geometry) ? geometry->matList.numMaterials : -1;
}

/* Marks parts of the geometry as being edited; locking bit 0 (vertex data) throws the mesh away. */
BMX_ADDR(0x004b4f20)
RpGeometry *RpGeometryLock(RpGeometry *geometry, int32_t lockMode)
{
    if (!geometryCheck(geometry)) {
        return NULL;
    }
    geometry->lockedSinceLastInst = (uint16_t)(geometry->lockedSinceLastInst | (uint32_t)lockMode);
    geometry->stamp++;
    if ((lockMode & 1) != 0 && geometry->mesh != NULL) {
        _rpMeshHeaderDestroy(geometry->mesh);
        geometry->mesh = NULL;
    }
    return geometry;
}

/* Builds the mesh (triangles grouped per material) unless one exists. */
BMX_ADDR(0x004b4fd0)
RpGeometry *RpGeometryUnlock(RpGeometry *geometry)
{
    if (!geometryCheck(geometry)) {
        return NULL;
    }
    if (geometry->mesh != NULL) {
        return geometry;
    }
    RpBuildMesh *build = RpBuildMeshCreate((uint32_t)geometry->numTriangles);
    if (build == NULL) {
        return NULL;
    }
    for (int32_t i = 0; i < geometry->numTriangles; i++) {
        const RpTriangle *triangle = &geometry->triangles[i];
        RpMaterial *material = triangle->matIndex == -1
                                   ? NULL
                                   : _rpMaterialListGetMaterial(&geometry->matList, triangle->matIndex);
        _rpBuildMeshAddTriangle(build, material, triangle->vertIndex[0], triangle->vertIndex[1],
                                triangle->vertIndex[2]);
    }
    RpMeshHeader *mesh = _rpMeshGenerate(build, (geometry->flags & rpGEOMETRYTRISTRIP) != 0 ? 1 : 0);
    if (mesh == NULL) {
        RpBuildMeshDestroy(build);
        return NULL;
    }
    geometry->mesh = mesh;
    return geometry;
}

BMX_ADDR(0x004b6140)
uint32_t RpGeometryGetFlags(const RpGeometry *geometry)
{
    return geometryCheck(geometry) ? geometry->flags : 0;
}

BMX_ADDR(0x004b61b0)
RpGeometry *RpGeometrySetFlags(RpGeometry *geometry, uint32_t flags)
{
    if (!geometryCheck(geometry)) {
        return NULL;
    }
    geometry->flags = (uint8_t)flags;
    geometry->stamp++;
    return geometry;
}

BMX_ADDR(0x004b5a30)
RpGeometry *RpGeometryAddRef(RpGeometry *geometry)
{
    if (!geometryCheck(geometry)) {
        return NULL;
    }
    geometry->refCount++;
    return geometry;
}

/* Appends an empty morph target (RpGeometryAddMorphTarget, inlined where the original needs it). */
static int geometryAddMorphTarget(RpGeometry *geometry)
{
    if (!geometryCheck(geometry)) {
        return 0;
    }
    const int32_t size = (geometry->numMorphTargets * 4 + 4) * 7;
    RpMorphTarget *morphTargets;
    if (geometry->morphTarget == NULL) {
        morphTargets = RwEngineInstance->memoryFuncs.rwmalloc((size_t)size);
    } else {
        morphTargets = RwEngineInstance->memoryFuncs.rwrealloc(geometry->morphTarget, (size_t)size);
    }
    if (morphTargets == NULL) {
        rpReportNoMem(size);
        return 0;
    }
    geometry->morphTarget = morphTargets;
    RpMorphTarget *morphTarget = &morphTargets[geometry->numMorphTargets];
    morphTarget->boundingSphere.center.x = 0.0F;
    morphTarget->boundingSphere.center.y = 0.0F;
    morphTarget->boundingSphere.center.z = 0.0F;
    morphTarget->boundingSphere.radius = 0.0F;
    morphTarget->parentGeom = geometry;
    morphTarget->verts = NULL;
    morphTarget->normals = NULL;
    const int32_t numVertices = geometry->numVertices;
    if (numVertices != 0) {
        if ((geometry->flags & rpGEOMETRYNORMALS) == 0) {
            morphTarget->verts = engineAlloc((size_t)numVertices * 12);
        } else {
            morphTarget->verts = engineAlloc((size_t)numVertices * 24);
            morphTarget->normals = (ZVec3 *)((uint8_t *)morphTarget->verts + (size_t)numVertices * 12);
        }
        if (morphTarget->verts == NULL) {
            rpReportNoMem(geometry->numVertices * 12);
            return 0;
        }
    }
    const int32_t previous = geometry->numMorphTargets++;
    geometry->stamp++;
    return previous >= 0;
}

/* A geometry with the given counts and one (empty) morph target; EAX = vertices, EDX = triangles, ECX =
 * format. */
BMX_ADDR(0x004b523c)
__attribute__((regparm(3))) RpGeometry *RpGeometryCreate(int32_t numVertices, int32_t numTriangles,
                                                         uint32_t format)
{
    if (rpGeometryModuleCount == 0) {
        rpReport(rwERR_NOTOPEN);
        return NULL;
    }
    if (numVertices < 0 || numTriangles < 0) {
        RwError error;
        error.pluginID = kRpErrorPlugin;
        error.errorCode = _rwerror(rwERR_INVALIDSIZE, "negative parameter passed");
        RwErrorSet(&error);
        return NULL;
    }
    if (numVertices > 0xffff) {
        rpReport(6);
        return NULL;
    }
    RpGeometry *geometry = RwEngineInstance->freeListAlloc(geometryFreeList());
    if (geometry == NULL) {
        return NULL;
    }
    if (_rpMaterialListInitialize(&geometry->matList) == NULL) {
        RwEngineInstance->freeListFree(geometryFreeList(), geometry);
        return NULL;
    }
    geometry->preLitLum = NULL;
    geometry->texCoords = NULL;
    geometry->mesh = NULL;
    geometry->numMorphTargets = 0;
    geometry->lockedSinceLastInst = 0;
    geometry->pad04 = 0;
    geometry->privateFlags = 0;
    geometry->flags = 0;
    geometry->subType = 0;
    geometry->morphTarget = NULL;
    geometry->stamp = 0;
    geometry->refCount = 1;
    geometry->type = 8;
    if (rpGeometryModuleCount == 0) {
        rpReport(rwERR_NOTOPEN);
    } else {
        geometry->stamp++;
        geometry->flags = (uint8_t)format;
    }
    geometry->numVertices = numVertices;
    if ((format & rpGEOMETRYPRELIT) != 0 && numVertices != 0) {
        geometry->preLitLum = engineAlloc((size_t)numVertices << 2);
        if (geometry->preLitLum == NULL) {
            _rpMaterialListDeinitialize(&geometry->matList);
            RwEngineInstance->freeListFree(geometryFreeList(), geometry);
            rpReportNoMem(numVertices << 2);
            return NULL;
        }
    }
    if ((format & rpGEOMETRYTEXTURED) != 0 && numVertices != 0) {
        geometry->texCoords = engineAlloc((size_t)numVertices << 3);
        if (geometry->texCoords == NULL) {
            if (geometry->preLitLum != NULL) {
                engineFree(geometry->preLitLum);
            }
            _rpMaterialListDeinitialize(&geometry->matList);
            RwEngineInstance->freeListFree(geometryFreeList(), geometry);
            rpReportNoMem(numVertices << 3);
            return NULL;
        }
    }
    geometry->triangles = NULL;
    geometry->numTriangles = numTriangles;
    if (numTriangles != 0) {
        geometry->triangles = engineAlloc((size_t)numTriangles << 3);
        if (geometry->triangles == NULL) {
            if (geometry->preLitLum != NULL) {
                engineFree(geometry->preLitLum);
            }
            if (geometry->texCoords != NULL) {
                engineFree(geometry->texCoords);
            }
            _rpMaterialListDeinitialize(&geometry->matList);
            RwEngineInstance->freeListFree(geometryFreeList(), geometry);
            rpReportNoMem(numTriangles << 3);
            return NULL;
        }
        for (int32_t i = 0; i < numTriangles; i++) {
            geometry->triangles[i].matIndex = -1;
        }
    }
    if (geometryAddMorphTarget(geometry)) {
        geometry->surfaceProps.diffuse = 1.0F;
        geometry->surfaceProps.specular = 1.0F;
        geometry->surfaceProps.ambient = 1.0F;
        _rwPluginRegistryInitObject(&rpGeometryRegistry, geometry);
        return geometry;
    }
    if (geometry->preLitLum != NULL) {
        engineFree(geometry->preLitLum);
    }
    if (geometry->texCoords != NULL) {
        engineFree(geometry->texCoords);
    }
    if (geometry->triangles != NULL) {
        engineFree(geometry->triangles);
    }
    _rpMaterialListDeinitialize(&geometry->matList);
    RwEngineInstance->freeListFree(geometryFreeList(), geometry);
    return NULL;
}

/* Removes a morph target (RpGeometryRemoveMorphTarget as inlined into the destructor). */
static void geometryRemoveMorphTarget(RpGeometry *geometry, int32_t index)
{
    if (rpGeometryModuleCount == 0) {
        rpReport(rwERR_NOTOPEN);
        return;
    }
    if (geometry == NULL) {
        rpReport(rwERR_NULLP);
        return;
    }
    RpMorphTarget *morphTarget = RpGeometryGetMorphTarget(geometry, index);
    const int32_t numMorphTargets = geometry->numMorphTargets;
    if (morphTarget == NULL) {
        return;
    }
    if (morphTarget->verts != NULL) {
        engineFree(morphTarget->verts);
        morphTarget->verts = NULL;
    }
    morphTarget->normals = NULL;
    for (int32_t i = index; i < numMorphTargets - 1; i++) {
        uint32_t *dst = (uint32_t *)&geometry->morphTarget[i];
        const uint32_t *src = (const uint32_t *)&geometry->morphTarget[i + 1];
        for (int w = 0; w < 7; w++) {
            dst[w] = src[w];
        }
    }
    geometry->numMorphTargets--;
    geometry->stamp++;
}

/*
 * Drops one reference; the last one releases the geometry with its morph targets, materials and mesh. The
 * entry (0x4b5ab0) only loads the argument for the body at 0x4b5ab4, which nothing else calls.
 */
BMX_ADDR(0x004b5ab0)
int RpGeometryDestroy(RpGeometry *geometry)
{
    if (rpGeometryModuleCount == 0) {
        rpReport(rwERR_NOTOPEN);
        return 0;
    }
    if (geometry == NULL) {
        rpReport(rwERR_NULLP);
        return 0;
    }
    if (--geometry->refCount != 0) {
        return 1;
    }
    RpGeometryLock(geometry, 0x3f);
    _rwPluginRegistryDeInitObject(&rpGeometryRegistry, geometry);
    if (geometry->preLitLum != NULL) {
        engineFree(geometry->preLitLum);
        geometry->preLitLum = NULL;
    }
    if (geometry->texCoords != NULL) {
        engineFree(geometry->texCoords);
        geometry->texCoords = NULL;
    }
    int32_t index = geometry->numMorphTargets;
    while (index != 0) {
        index--;
        geometryRemoveMorphTarget(geometry, index);
        index = geometry->numMorphTargets;
    }
    if (geometry->morphTarget != NULL) {
        engineFree(geometry->morphTarget);
        geometry->morphTarget = NULL;
    }
    for (int32_t i = 0; i < geometry->numTriangles; i++) {
        const int32_t matIndex = geometry->triangles[i].matIndex;
        if (matIndex != -1 && matIndex < geometry->matList.numMaterials) {
            RpMaterialDestroy(_rpMaterialListGetMaterial(&geometry->matList, matIndex));
        }
        geometry->triangles[i].matIndex = -1;
    }
    if (geometry->triangles != NULL) {
        engineFree(geometry->triangles);
        geometry->triangles = NULL;
    }
    _rpMaterialListDeinitialize(&geometry->matList);
    RwEngineInstance->freeListFree(geometryFreeList(), geometry);
    return 1;
}

/* Bytes of the geometry's chunk: the struct (counts, pre-lit colours, texture coordinates, triangles), the
 * morph targets, the material list and the plugins. 0 with an error without the geometry module or geometry.
 */
BMX_ADDR(0x004b6230)
int32_t RpGeometryStreamGetSize(const RpGeometry *geometry)
{
    if (rpGeometryModuleCount == 0) {
        rpReport(rwERR_NOTOPEN);
        return 0;
    }
    if (geometry == NULL) {
        rpReport(rwERR_NULLP);
        return 0;
    }
    const int32_t numVertices = geometry->numVertices;
    int32_t size = 0x1c;
    if (numVertices != 0) {
        if ((geometry->flags & rpGEOMETRYPRELIT) != 0) {
            size = numVertices * 4 + 0x1c;
        }
        if ((geometry->flags & rpGEOMETRYTEXTURED) != 0) {
            size += numVertices * 8;
        }
        size += geometry->numTriangles * 8;
    }
    const RpMorphTarget *target = geometry->morphTarget;
    for (int32_t i = 0; i < geometry->numMorphTargets; i++, target++) {
        size += 0x18;
        if (target->verts != NULL) {
            size += numVertices * 12;
        }
        if (target->normals != NULL) {
            size += numVertices * 12;
        }
    }
    const int32_t materials = _rpMaterialListStreamGetSize(&geometry->matList);
    const int32_t plugins = _rwPluginRegistryGetSize(&rpGeometryRegistry, geometry);
    return size + 0xc + materials + 0xc + 0xc + plugins;
}

/* Bytes of the geometry's struct chunk (RpGeometryStreamGetSize without the material list and plugins). */
static uint32_t geometryStructSize(const RpGeometry *geometry)
{
    const uint32_t numVertices = (uint32_t)geometry->numVertices;
    uint32_t size = 0x1c;
    if (numVertices != 0) {
        if ((geometry->flags & rpGEOMETRYPRELIT) != 0) {
            size = numVertices * 4 + 0x1c;
        }
        if ((geometry->flags & rpGEOMETRYTEXTURED) != 0) {
            size += numVertices * 8;
        }
        size += (uint32_t)geometry->numTriangles * 8;
    }
    const RpMorphTarget *target = geometry->morphTarget;
    for (int32_t i = 0; i < geometry->numMorphTargets; i++, target++) {
        size += 0x18;
        if (target->verts != NULL) {
            size += numVertices * 12;
        }
        if (target->normals != NULL) {
            size += numVertices * 12;
        }
    }
    return size;
}

/* Writes a geometry chunk (the inverse of RpGeometryStreamRead). The original's second module check
 * and the null checks inside the struct block are dead code and left out. */
BMX_ADDR(0x004b6350)
RpGeometry *RpGeometryStreamWrite(RpGeometry *geometry, RwStream *stream)
{
    if (rpGeometryModuleCount == 0) {
        rpReport(rwERR_NOTOPEN);
        return NULL;
    }
    if (geometry == NULL || stream == NULL) {
        rpReport(rwERR_NULLP);
        return NULL;
    }
    const int32_t materials = _rpMaterialListStreamGetSize(&geometry->matList);
    const int32_t plugins = _rwPluginRegistryGetSize(&rpGeometryRegistry, geometry);
    const uint32_t total =
        geometryStructSize(geometry) + 0xc + (uint32_t)materials + 0x18 + (uint32_t)plugins;
    if (_rwStreamWriteVersionedChunkHeader(stream, 0xf, (int32_t)total, 0x303) == NULL) {
        return NULL;
    }
    if (_rwStreamWriteVersionedChunkHeader(stream, 1, (int32_t)geometryStructSize(geometry), 0x303) == NULL) {
        return NULL;
    }
    uint32_t header[7];
    header[0] = geometry->flags;
    header[1] = (uint32_t)geometry->numTriangles;
    header[2] = (uint32_t)geometry->numVertices;
    header[3] = (uint32_t)geometry->numMorphTargets;
    __builtin_memcpy(&header[4], &geometry->surfaceProps, 0xc);
    RwMemNative16(&header[4], 0xc);
    RwMemLittleEndian32(header, 0x1c);
    if (RwStreamWrite(stream, header, 0x1c) == NULL) {
        return NULL;
    }
    const uint32_t numVertices = (uint32_t)geometry->numVertices;
    if (numVertices != 0) {
        if ((geometry->flags & rpGEOMETRYPRELIT) != 0 &&
            RwStreamWrite(stream, geometry->preLitLum, numVertices * 4) == NULL) {
            return NULL;
        }
        if ((geometry->flags & rpGEOMETRYTEXTURED) != 0 &&
            RwStreamWriteReal(stream, geometry->texCoords, numVertices * 8) == NULL) {
            return NULL;
        }
        if (geometry->numTriangles != 0) {
            const uint16_t *raw = (const uint16_t *)geometry->triangles;
            uint32_t remaining = (uint32_t)geometry->numTriangles;
            do {
                uint32_t pair[2];
                pair[0] = (uint32_t)raw[0] << 16 | raw[1];
                pair[1] = (uint32_t)raw[2] << 16 | raw[3];
                raw += 4;
                RwMemLittleEndian32(pair, 8);
                if (RwStreamWrite(stream, pair, 8) == NULL) {
                    return NULL;
                }
            } while (--remaining != 0);
        }
    }
    for (int32_t i = 0; i < geometry->numMorphTargets; i++) {
        const RpMorphTarget *target = &geometry->morphTarget[i];
        uint32_t chunk[6];
        __builtin_memcpy(chunk, &target->boundingSphere, 0x10);
        chunk[4] = target->verts != NULL ? 1 : 0;
        chunk[5] = target->normals != NULL ? 1 : 0;
        RwMemNative16(chunk, 0x10);
        RwMemLittleEndian32(chunk, 0x18);
        if (RwStreamWrite(stream, chunk, 0x18) == NULL) {
            return NULL;
        }
        if (chunk[4] != 0 && RwStreamWriteReal(stream, target->verts, numVertices * 12) == NULL) {
            return NULL;
        }
        if (chunk[5] != 0 && RwStreamWriteReal(stream, target->normals, numVertices * 12) == NULL) {
            return NULL;
        }
    }
    if (_rpMaterialListStreamWrite(&geometry->matList, stream) == NULL) {
        return NULL;
    }
    if (_rwPluginRegistryWriteDataChunks(&rpGeometryRegistry, stream, geometry) == NULL) {
        return NULL;
    }
    return geometry;
}

/* Reads a geometry chunk: struct, (pre-lit colours, texture coordinates, triangles), morph targets,
 * materials. */
BMX_ADDR(0x004b68e0)
RpGeometry *RpGeometryStreamRead(RwStream *stream)
{
    if (rpGeometryModuleCount == 0) {
        rpReport(rwERR_NOTOPEN);
        return NULL;
    }
    if (stream == NULL) {
        rpReport(rwERR_NULLP);
        return NULL;
    }
    uint32_t version;
    if (RwStreamFindChunk(stream, 1, NULL, &version) == 0) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        rpReport(rwERR_BADVERSION);
        return NULL;
    }
    int32_t header[7];
    if (RwStreamRead(stream, header, 0x1c) != 0x1c) {
        return NULL;
    }
    RwMemNative32(header, 0x1c);
    RwMemFloat32ToReal(&header[4], 0xc);
    const uint32_t format = (uint32_t)header[0];
    RpGeometry *geometry = RpGeometryCreate(header[2], header[1], format & 0xff);
    if (geometry == NULL) {
        return NULL;
    }
    ((int32_t *)&geometry->surfaceProps)[0] = header[4];
    ((int32_t *)&geometry->surfaceProps)[1] = header[5];
    ((int32_t *)&geometry->surfaceProps)[2] = header[6];
    for (int32_t remaining = header[3] - 1; remaining > 0; remaining--) {
        if (!geometryAddMorphTarget(geometry)) {
            RpGeometryDestroy(geometry);
            return NULL;
        }
    }
    const int32_t numVertices = geometry->numVertices;
    if (numVertices != 0) {
        if ((format & rpGEOMETRYPRELIT) != 0 &&
            RwStreamRead(stream, geometry->preLitLum, (uint32_t)numVertices * 4) !=
                (uint32_t)numVertices * 4) {
            RpGeometryDestroy(geometry);
            return NULL;
        }
        /* The pre-3.1 texture coordinate layout (version <= 0x300) is unreachable: versions are checked. */
        if ((format & rpGEOMETRYTEXTURED) != 0 &&
            RwStreamReadReal(stream, geometry->texCoords, (uint32_t)numVertices * 8) == NULL) {
            RpGeometryDestroy(geometry);
            return NULL;
        }
        RpTriangle *triangle = geometry->triangles;
        for (int32_t i = geometry->numTriangles; i != 0; i--) {
            uint16_t raw[4];
            if (RwStreamRead(stream, raw, 8) != 8) {
                RpGeometryDestroy(geometry);
                return NULL;
            }
            RwMemNative32(raw, 8);
            triangle->vertIndex[0] = raw[1];
            triangle->vertIndex[1] = raw[0];
            triangle->vertIndex[2] = raw[3];
            triangle->matIndex = (int16_t)raw[2];
            triangle++;
        }
    }
    for (int32_t i = 0; i < geometry->numMorphTargets; i++) {
        RpMorphTarget *morphTarget = &geometry->morphTarget[i];
        float raw[6];
        if (RwStreamRead(stream, raw, 0x18) != 0x18) {
            RpGeometryDestroy(geometry);
            return NULL;
        }
        RwMemFloat32ToReal(raw, 0xc);
        RwMemFloat32ToReal(&raw[3], 4);
        RwMemNative32(raw, 0x18);
        RwSphere sphere = {{raw[0], raw[1], raw[2]}, raw[3]};
        RpMorphTargetSetBoundingSphere(morphTarget, &sphere);
        const int32_t hasVerts = ((const int32_t *)raw)[4];
        const int32_t hasNormals = ((const int32_t *)raw)[5];
        const uint32_t numVerts = (uint32_t)geometry->numVertices;
        const int32_t span =
            (int32_t)((uint32_t)(uintptr_t)morphTarget->normals - (uint32_t)(uintptr_t)morphTarget->verts);
        if (hasVerts != 0 && hasNormals != 0 && span / 12 == (int32_t)numVerts) {
            if (RwStreamReadReal(stream, morphTarget->verts, numVerts * 24) == NULL) {
                RpGeometryDestroy(geometry);
                return NULL;
            }
            continue;
        }
        if (hasVerts != 0 && RwStreamReadReal(stream, morphTarget->verts, numVerts * 12) == NULL) {
            RpGeometryDestroy(geometry);
            return NULL;
        }
        if (hasNormals != 0 && RwStreamReadReal(stream, morphTarget->normals, numVerts * 12) == NULL) {
            RpGeometryDestroy(geometry);
            return NULL;
        }
    }
    if (RwStreamFindChunk(stream, 8, NULL, &version) == 0) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        RpGeometryDestroy(geometry);
        rpReport(rwERR_BADVERSION);
        return NULL;
    }
    if (_rpMaterialListStreamRead(stream, &geometry->matList) == NULL) {
        RpGeometryDestroy(geometry);
        return NULL;
    }
    for (int32_t i = 0; i < geometry->numTriangles; i++) {
        RpMaterialAddRef(_rpMaterialListGetMaterial(&geometry->matList, geometry->triangles[i].matIndex));
    }
    if (_rwPluginRegistryReadDataChunks(&rpGeometryRegistry, stream, geometry) == NULL) {
        RpGeometryDestroy(geometry);
        return NULL;
    }
    if (RpGeometryUnlock(geometry) == NULL) {
        RpGeometryDestroy(geometry);
        return NULL;
    }
    return geometry;
}

/* ---- plugin registry helpers -------------------------------------------------------------------------- */

/* Lets every plugin of the registry copy its data from `source` to `destination`. */
BMX_ADDR(0x004e8a60)
RwPluginRegistry *_rwPluginRegistryCopyObject(RwPluginRegistry *registry, void *destination,
                                              const void *source)
{
    for (RwPluginRegEntry *entry = registry->firstRegEntry; entry != NULL; entry = entry->nextRegEntry) {
        entry->copyCB(destination, source, entry->offset, entry->size);
    }
    return registry;
}

/* Bytes the plugins of the registry add to a stream chunk (their chunks with headers). */
BMX_ADDR(0x004e8d60)
int32_t _rwPluginRegistryGetSize(const RwPluginRegistry *registry, const void *object)
{
    int32_t total = 0;
    for (const RwPluginRegEntry *entry = registry->firstRegEntry; entry != NULL;
         entry = entry->nextRegEntry) {
        if (entry->getSizeCB != NULL) {
            const int32_t size = entry->getSizeCB(object, entry->offset, entry->size);
            if (size > 0) {
                total += size + 0xc;
            }
        }
    }
    return total;
}

/* ---- materials -------------------------------------------------------------------------------------- */

BMX_ADDR(0x004b91f0)
RpMaterial *RpMaterialAddRef(RpMaterial *material)
{
    material->refCount++;
    return material;
}

/* Drops one reference; the last one releases the texture and the material. Always 1. */
BMX_ADDR(0x004b9290)
int RpMaterialDestroy(RpMaterial *material)
{
    material->refCount--;
    if (material->refCount != 0) {
        return 1;
    }
    _rwPluginRegistryDeInitObject(&rpMaterialRegistry, material);
    if (material->texture != NULL) {
        RwTextureDestroy(material->texture);
    }
    material->texture = NULL;
    material->stamp = (uint8_t)(material->stamp + 2);
    RwEngineInstance->freeListFree(materialFreeList(), material);
    return 1;
}

#define rpMaterialDefaultSurfaceProps ORIG_VAR(0x00566814, RwSurfaceProperties, rpMaterialDefaultSurfaceProps)
#define rwEmptyString                 ORIG_VAR(0x00676d90, char, rwEmptyString)

/* The engine's strlen (RwEngineInstance->stringFuncs). */
static uint32_t engineStrlen(const char *string)
{
    return (uint32_t)((size_t(BMX_CDECL *)(const char *))RwEngineInstance->stringFuncs.rwstrlen)(string);
}

/* Bytes of a material's chunk (without the chunk header of the material itself). */
BMX_ADDR(0x004b94a0)
int32_t RpMaterialStreamGetSize(const RpMaterial *material)
{
    int32_t size = 0x28;
    if (material->texture != NULL) {
        size = RwTextureStreamGetSize(material->texture) + 0x34;
    }
    const int32_t plugins = _rwPluginRegistryGetSize(&rpMaterialRegistry, material);
    return size + 0xc + plugins;
}

/* Writes a material chunk: struct (flags, colour, textured flag, surface properties), texture, plugin data.
 */
BMX_ADDR(0x004b94e0)
RpMaterial *RpMaterialStreamWrite(RpMaterial *material, RwStream *stream)
{
    int32_t size = 0x28;
    if (material->texture != NULL) {
        size = RwTextureStreamGetSize(material->texture) + 0x34;
    }
    const int32_t plugins = _rwPluginRegistryGetSize(&rpMaterialRegistry, material);
    if (_rwStreamWriteVersionedChunkHeader(stream, 7, plugins + 0xc + size, 0x303) == NULL ||
        _rwStreamWriteVersionedChunkHeader(stream, 1, 0x1c, 0x303) == NULL) {
        return NULL;
    }
    /* Word 2 is never set in the original (stack garbage in the file); zero here. */
    int32_t buffer[7] = {0, 0, 0, 0, 0, 0, 0};
    __builtin_memcpy(&buffer[1], &material->color, sizeof buffer[1]);
    buffer[3] = material->texture != NULL ? 1 : 0;
    buffer[4] = ((const int32_t *)&material->surfaceProps)[0];
    buffer[5] = ((const int32_t *)&material->surfaceProps)[1];
    buffer[6] = ((const int32_t *)&material->surfaceProps)[2];
    RwMemNative16(&buffer[4], 0xc);
    RwMemLittleEndian32(buffer, 0x1c);
    if (RwStreamWrite(stream, buffer, 0x1c) == NULL) {
        return NULL;
    }
    if (material->texture != NULL && RwTextureStreamWrite(material->texture, stream) == NULL) {
        return NULL;
    }
    if (_rwPluginRegistryWriteDataChunks(&rpMaterialRegistry, stream, material) == NULL) {
        return NULL;
    }
    return material;
}

/* Reads a material chunk: colour and surface properties, an optional texture chunk, then plugin data. */
BMX_ADDR(0x004b9630)
RpMaterial *RpMaterialStreamRead(RwStream *stream)
{
    uint32_t length;
    uint32_t version;
    if (RwStreamFindChunk(stream, 1, &length, &version) == 0) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        rpReport(rwERR_BADVERSION);
        return NULL;
    }
    int32_t buffer[7] = {0, 0, 0, 0, 0, 0, 0};
    const uint32_t read = RwStreamRead(stream, buffer, length);
    const int32_t color = buffer[1];
    if (read != length) {
        return NULL;
    }
    RwMemNative32(buffer, 0x1c);
    buffer[1] = color;
    RpMaterial *material = RwEngineInstance->freeListAlloc(materialFreeList());
    if (material == NULL) {
        return NULL;
    }
    material->stamp = 0;
    material->texture = NULL;
    material->color = (RwRGBA){0xff, 0xff, 0xff, 0xff};
    material->refCount = 1;
    material->surfaceProps = rpMaterialDefaultSurfaceProps;
    material->stamp = 1;
    _rwPluginRegistryInitObject(&rpMaterialRegistry, material);
    __builtin_memcpy(&material->color, &buffer[1], sizeof material->color);
    material->stamp++;
    if (length < 0x11) {
        material->surfaceProps = rpMaterialDefaultSurfaceProps;
        material->stamp++;
    } else {
        RwMemFloat32ToReal(&buffer[4], 0xc);
        material->surfaceProps.ambient = ((const float *)buffer)[4];
        material->surfaceProps.specular = ((const float *)buffer)[5];
        material->stamp++;
        material->surfaceProps.diffuse = ((const float *)buffer)[6];
    }
    material->texture = NULL;
    if (buffer[3] != 0) {
        if (RwStreamFindChunk(stream, 6, NULL, &length) == 0) {
            RpMaterialDestroy(material);
            return NULL;
        }
        if (length < 0x301 || length > 0x303) {
            RpMaterialDestroy(material);
            rpReport(rwERR_BADVERSION);
            return NULL;
        }
        material->texture = RwTextureStreamRead(stream);
    }
    if (_rwPluginRegistryReadDataChunks(&rpMaterialRegistry, stream, material) != NULL) {
        return material;
    }
    RpMaterialDestroy(material);
    return NULL;
}

/* ---- material lists --------------------------------------------------------------------------------- */

BMX_ADDR(0x004c4f90)
RpMaterial *_rpMaterialListGetMaterial(const RpMaterialList *list, int32_t index)
{
    return list->materials[index];
}

BMX_ADDR(0x004c56a0)
RpMaterialList *_rpMaterialListInitialize(RpMaterialList *list)
{
    list->space = 0;
    list->numMaterials = 0;
    list->materials = NULL;
    return list;
}

/* Releases the references the list holds and its array. */
BMX_ADDR(0x004c5640)
RpMaterialList *_rpMaterialListDeinitialize(RpMaterialList *list)
{
    RpMaterial **materials = list->materials;
    if (materials != NULL) {
        for (int32_t i = 0; i < list->numMaterials; i++) {
            RpMaterialDestroy(materials[i]);
            materials[i] = NULL;
        }
        engineFree((void *)materials);
    }
    list->space = 0;
    list->numMaterials = 0;
    list->materials = NULL;
    return list;
}

/* The index of the last earlier entry holding the same material, or -1 (materials are written once). */
static int32_t materialListFindEarlier(const RpMaterialList *list, int32_t index)
{
    int32_t found = index - 1;
    if (index != 0) {
        RpMaterial *const material = list->materials[index];
        for (; found >= 0; found--) {
            if (list->materials[found] == material) {
                break;
            }
        }
    }
    return found;
}

/* Bytes of the material list chunk: the index table and every distinct material. */
BMX_ADDR(0x004c50e0)
int32_t _rpMaterialListStreamGetSize(const RpMaterialList *list)
{
    int32_t size = list->numMaterials * 4 + 0x10;
    for (int32_t i = 0; i < list->numMaterials; i++) {
        if (materialListFindEarlier(list, i) < 0) {
            size += RpMaterialStreamGetSize(list->materials[i]) + 0xc;
        }
    }
    return size;
}

/* Writes the material list chunk: the count, an index per material (-1 for a first use), the new materials.
 */
BMX_ADDR(0x004c5160)
RpMaterialList *_rpMaterialListStreamWrite(const RpMaterialList *list, RwStream *stream)
{
    int32_t size = list->numMaterials * 4 + 0x10;
    for (int32_t i = 0; i < list->numMaterials; i++) {
        if (materialListFindEarlier(list, i) < 0) {
            size += RpMaterialStreamGetSize(list->materials[i]) + 0xc;
        }
    }
    if (_rwStreamWriteVersionedChunkHeader(stream, 8, size, 0x303) == NULL ||
        _rwStreamWriteVersionedChunkHeader(stream, 1, list->numMaterials * 4 + 4, 0x303) == NULL ||
        RwStreamWriteInt32(stream, &list->numMaterials, 4) == NULL) {
        return NULL;
    }
    for (int32_t i = 0; i < list->numMaterials; i++) {
        const int32_t earlier = materialListFindEarlier(list, i);
        if (RwStreamWriteInt32(stream, &earlier, 4) == NULL) {
            return NULL;
        }
    }
    for (int32_t i = 0; i < list->numMaterials; i++) {
        if (materialListFindEarlier(list, i) < 0 &&
            RpMaterialStreamWrite(list->materials[i], stream) == NULL) {
            return NULL;
        }
    }
    return (RpMaterialList *)list;
}

/* Appends a material to the list (growing the array 5 entries at a time) and takes a reference. */
static void materialListAppend(RpMaterialList *list, RpMaterial *material)
{
    if (list->numMaterials < list->space) {
        list->materials[list->numMaterials] = material;
        RpMaterialAddRef(material);
        list->numMaterials++;
        return;
    }
    const int32_t size = list->space * 4 + 0x14;
    RpMaterial **materials;
    if (list->materials == NULL) {
        materials = (RpMaterial **)engineAlloc((size_t)size);
    } else {
        materials =
            (RpMaterial **)RwEngineInstance->memoryFuncs.rwrealloc((void *)list->materials, (size_t)size);
    }
    if (materials == NULL) {
        rpReportNoMem(size);
        return;
    }
    list->materials = materials;
    list->space += 5;
    materials[list->numMaterials] = material;
    RpMaterialAddRef(material);
    list->numMaterials++;
    for (int32_t i = list->numMaterials; i < list->space; i++) {
        list->materials[i] = NULL;
    }
}

/* Reads a material list chunk into an empty list; an index of -1 introduces a new material chunk. */
BMX_ADDR(0x004c5330)
RpMaterialList *_rpMaterialListStreamRead(RwStream *stream, RpMaterialList *list)
{
    uint32_t length;
    uint32_t version;
    if (RwStreamFindChunk(stream, 1, &length, &version) == 0) {
        return NULL;
    }
    if (version <= 0x300 || version >= 0x304) {
        rpReport(rwERR_BADVERSION);
        return NULL;
    }
    int32_t count;
    if (RwStreamReadInt32(stream, &count, 4) == NULL) {
        return NULL;
    }
    _rpMaterialListInitialize(list);
    int32_t *indices = engineAlloc((size_t)count << 2);
    if (RwStreamReadInt32(stream, indices, (uint32_t)count << 2) == NULL) {
        engineFree(indices);
        _rpMaterialListDeinitialize(list);
        return NULL;
    }
    for (int32_t i = 0; i < count; i++) {
        RpMaterial *material;
        if (indices[i] < 0) {
            if (RwStreamFindChunk(stream, 7, NULL, &version) == 0) {
                engineFree(indices);
                _rpMaterialListDeinitialize(list);
                return NULL;
            }
            if (version < 0x301 || version > 0x303) {
                rpReport(rwERR_BADVERSION);
                return NULL;
            }
            material = RpMaterialStreamRead(stream);
            if (material == NULL) {
                engineFree(indices);
                _rpMaterialListDeinitialize(list);
                return NULL;
            }
        } else {
            material = list->materials[indices[i]];
            RpMaterialAddRef(material);
        }
        materialListAppend(list, material);
        RpMaterialDestroy(material);
    }
    engineFree(indices);
    return list;
}

/* ---- textures in streams ---------------------------------------------------------------------------- */

/* Bytes of a texture chunk: two padded names, the filter word and the texture's plugin data. */
BMX_ADDR(0x004e9d20)
int32_t RwTextureStreamGetSize(const RwTexture *texture)
{
    const char *name = texture->name != NULL ? texture->name : &rwEmptyString;
    const int32_t nameLength = (int32_t)engineStrlen(name);
    const char *mask = texture->mask != NULL ? texture->mask : &rwEmptyString;
    const int32_t maskLength = (int32_t)engineStrlen(mask);
    const int32_t plugins = _rwPluginRegistryGetSize(&rwTextureRegistry, texture);
    return (int32_t)(((uint32_t)nameLength + 4U) & 0xfffffffcU) +
           (int32_t)(((uint32_t)maskLength + 4U) & 0xfffffffcU) + 0x34 + plugins;
}

/* Writes a string chunk padded to four bytes (EAX = string or NULL for the empty string, EDX = stream). */
BMX_ADDR(0x004e9988)
__attribute__((regparm(2))) const char *rwStreamWriteString(const char *string, RwStream *stream)
{
    if (string == NULL) {
        string = &rwEmptyString;
    }
    const uint32_t length = engineStrlen(string);
    const uint32_t padded = (length + 4U) & 0xfffffffcU;
    if (_rwStreamWriteVersionedChunkHeader(stream, 2, (int32_t)padded, 0x303) == NULL ||
        RwStreamWrite(stream, string, padded) == NULL) {
        return NULL;
    }
    return string;
}

/* Reads the next string chunk (type 2, or type 0x13 with 16 bit characters) into a new buffer (EAX = stream).
 */
BMX_ADDR(0x004e9a14)
__attribute__((regparm(1))) char *rwStreamReadString(RwStream *stream)
{
    uint32_t type;
    uint32_t length;
    uint32_t version;
    int ok = _rwStreamReadChunkHeader(stream, &type, &length, &version);
    for (;;) {
        if (ok == 0) {
            return NULL;
        }
        if (version < 0x301 || version > 0x303) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(rwERR_BADVERSION);
            RwErrorSet(&error);
            return NULL;
        }
        if (type == 2 || type == 0x13) {
            break;
        }
        if (RwStreamSkip(stream, length) == NULL) {
            return NULL;
        }
        ok = _rwStreamReadChunkHeader(stream, &type, &length, &version);
    }
    uint8_t *raw = engineAlloc(length);
    if (raw == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_NOMEM, (int32_t)length);
        RwErrorSet(&error);
        return NULL;
    }
    if (RwStreamRead(stream, raw, length) != length) {
        engineFree(raw);
        return NULL;
    }
    const uint32_t count = type == 2 ? length : length >> 1;
    char *string = engineAlloc(count);
    if (string == NULL) {
        engineFree(raw);
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_NOMEM, (int32_t)count);
        RwErrorSet(&error);
        return NULL;
    }
    for (uint32_t i = 0; i < count; i++) {
        string[i] = type == 2 ? (char)raw[i] : (char)((const uint16_t *)raw)[i];
    }
    engineFree(raw);
    return string;
}

/* Writes a texture chunk: names, filter and addressing, plugin data. */
BMX_ADDR(0x004e9d90)
RwTexture *RwTextureStreamWrite(RwTexture *texture, RwStream *stream)
{
    const char *name = texture->name != NULL ? texture->name : &rwEmptyString;
    const uint32_t nameLength = engineStrlen(name);
    const char *mask = texture->mask != NULL ? texture->mask : &rwEmptyString;
    const uint32_t maskLength = engineStrlen(mask);
    const int32_t plugins = _rwPluginRegistryGetSize(&rwTextureRegistry, texture);
    const int32_t size =
        (int32_t)(((maskLength + 4U) & 0xfffffffcU) + 0x34 + ((nameLength + 4U) & 0xfffffffcU)) + plugins;
    if (_rwStreamWriteVersionedChunkHeader(stream, 6, size, 0x303) == NULL ||
        _rwStreamWriteVersionedChunkHeader(stream, 1, 4, 0x303) == NULL) {
        return NULL;
    }
    const uint32_t filter = (uint32_t)RwTextureGetFilterMode(texture);
    const uint32_t addressing = (uint32_t)RwTextureGetAddressing(texture);
    uint32_t flag = 0;
    if (texture->raster != NULL && (texture->raster->format & 0x10) == 0) {
        flag = 1;
    }
    uint32_t word = (filter & 0xff) | ((addressing & 0xff) << 8) | (flag << 8);
    RwMemLittleEndian32(&word, 4);
    if (RwStreamWrite(stream, &word, 4) == NULL || rwStreamWriteString(texture->name, stream) == NULL ||
        rwStreamWriteString(texture->mask, stream) == NULL ||
        _rwPluginRegistryWriteDataChunks(&rwTextureRegistry, stream, texture) == NULL) {
        return NULL;
    }
    return texture;
}

/* Reads a texture chunk: filter word, name and mask (found in the dictionary or loaded), plugin data. */
BMX_ADDR(0x004e9f20)
RwTexture *RwTextureStreamRead(RwStream *stream)
{
    uint32_t length;
    uint32_t version;
    if (RwStreamFindChunk(stream, 1, &length, &version) == 0) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_BADVERSION);
        RwErrorSet(&error);
        return NULL;
    }
    int32_t word = 0;
    if (RwStreamRead(stream, &word, length) != length) {
        return NULL;
    }
    RwMemNative32(&word, 4);
    const uint32_t filter = (uint32_t)word & 0xff;
    const int32_t flags = word >> 16;
    const uint32_t addressing = ((uint32_t)word >> 8) & 0xff;
    RwTextureCallBackFind savedFind = RwTextureGetFindCallBack();
    const int32_t savedMipmapping = RwTextureGetMipmapping();
    if (filter == 3 || filter == 4 || filter == 5 || filter == 6) {
        RwTextureSetFindCallBack((RwTextureCallBackFind)(uintptr_t)1);
        RwTextureSetMipmapping((flags & 1) == 0 ? 1 : 0);
    } else {
        RwTextureSetFindCallBack(NULL);
        RwTextureSetMipmapping(0);
    }
    char *name = rwStreamReadString(stream);
    if (name == NULL) {
        RwTextureSetFindCallBack(savedFind);
        RwTextureSetMipmapping(savedMipmapping);
        return NULL;
    }
    char *mask = rwStreamReadString(stream);
    if (mask == NULL) {
        RwTextureSetFindCallBack(savedFind);
        RwTextureSetMipmapping(savedMipmapping);
        engineFree(name);
        return NULL;
    }
    RwTexture *texture = RwTextureRead(name, mask);
    if (texture == NULL) {
        engineFree(name);
        engineFree(mask);
        _rwPluginRegistrySkipDataChunks(&rwTextureRegistry, stream);
        RwTextureSetFindCallBack(savedFind);
        RwTextureSetMipmapping(savedMipmapping);
        return NULL;
    }
    if (texture->refCount == 1) {
        RwTextureSetFilterMode(texture, (int32_t)filter);
        RwTextureSetAddressing(texture, (int32_t)addressing);
    }
    engineFree(name);
    engineFree(mask);
    RwTextureSetFindCallBack(savedFind);
    RwTextureSetMipmapping(savedMipmapping);
    if (_rwPluginRegistryReadDataChunks(&rwTextureRegistry, stream, texture) != NULL) {
        return texture;
    }
    return NULL;
}

/* The texture module's mipmapping flag (read only here). */
BMX_ADDR(0x004df5a0)
int32_t RwTextureGetMipmapping(void)
{
    return ((const RwTextureGlobals *)((const uint8_t *)RwEngineInstance + rwTextureOffset))->mipmapping;
}

BMX_ADDR(0x004dff90)
int32_t RwTextureGetFilterMode(const RwTexture *texture)
{
    return texture->filterMode;
}

BMX_ADDR(0x004dffc0)
int32_t RwTextureGetAddressing(const RwTexture *texture)
{
    return texture->addressing;
}

/* ---- mesh building ---------------------------------------------------------------------------------- */

/* A build mesh: the triangles (space for `capacity`) a geometry or sector hands to the mesh generator. */
BMX_ADDR(0x004c4750)
RpBuildMesh *RpBuildMeshCreate(uint32_t numTriangles)
{
    RpBuildMesh *build = engineAlloc(sizeof(RpBuildMesh));
    if (build == NULL) {
        rpReportNoMem(0xc);
        return NULL;
    }
    build->numTriangles = 0;
    if (numTriangles == 0) {
        build->capacity = 0;
        build->triangles = NULL;
    } else {
        build->triangles = engineAlloc(numTriangles * sizeof(RpBuildMeshTriangle));
        if (build->triangles == NULL) {
            engineFree(build);
            rpReportNoMem((int32_t)(numTriangles * 0xc));
            return NULL;
        }
        build->capacity = numTriangles;
    }
    return build;
}

BMX_ADDR(0x004c4830)
int RpBuildMeshDestroy(RpBuildMesh *build)
{
    if (build->triangles != NULL) {
        engineFree(build->triangles);
        build->triangles = NULL;
    }
    engineFree(build);
    return 1;
}

/* Frees a mesh header (the shared empty header of a mesh without triangles is never freed). */
BMX_ADDR(0x004c4870)
int _rpMeshHeaderDestroy(RpMeshHeader *header)
{
    if (header->flags != 0 || header->numMeshes != 0 || header->serialNum != 0 || header->totalIndices != 0 ||
        header->firstMeshOffset != 0) {
        engineFree(header);
    }
    return 1;
}

/* Appends a triangle, growing the array by one entry when it is full. */
BMX_ADDR(0x004c48c0)
RpBuildMesh *_rpBuildMeshAddTriangle(RpBuildMesh *build, RpMaterial *material, uint16_t v0, uint16_t v1,
                                     uint16_t v2)
{
    if (build->capacity <= build->numTriangles) {
        const int32_t size = (int32_t)((build->numTriangles + 1) * sizeof(RpBuildMeshTriangle));
        RpBuildMeshTriangle *triangles;
        if (build->numTriangles == 0) {
            triangles = engineAlloc(sizeof(RpBuildMeshTriangle));
        } else {
            triangles = RwEngineInstance->memoryFuncs.rwrealloc(build->triangles, (size_t)size);
        }
        if (triangles == NULL) {
            rpReportNoMem(size);
            return NULL;
        }
        build->triangles = triangles;
        build->capacity = build->numTriangles + 1;
    }
    RpBuildMeshTriangle *triangle = &build->triangles[build->numTriangles];
    triangle->material = material;
    triangle->vertIndex[0] = v0;
    triangle->vertIndex[1] = v1;
    triangle->vertIndex[2] = v2;
    build->numTriangles++;
    return build;
}

/* Calls callback(mesh, header, data) for each mesh of the header until it returns 0. */
BMX_ADDR(0x004c49d0)
RpMeshHeader *_rpMeshHeaderForAllMeshes(RpMeshHeader *header, RpMeshCallBack callback, void *data)
{
    RpMesh *mesh = (RpMesh *)((uint8_t *)header + header->firstMeshOffset + sizeof(RpMeshHeader));
    for (uint32_t i = header->numMeshes; i != 0; i--) {
        if (callback(mesh, header, data) == 0) {
            return header;
        }
        mesh++;
    }
    return header;
}

#define rpMeshEmptyHeader    ORIG_VAR(0x006767c0, RpMeshHeader, rpMeshEmptyHeader)
#define rpMeshStripMode      ORIG_VAR(0x006767d0, int32_t, rpMeshStripMode)
#define RP_MESH_SORT_COMPARE 0x004c5af0
#define RP_MESH_LIST_FINISH  0x004c5bd0

ORIG_FN(0x004a4aa7, void, rwQsort, (void *base, uint32_t count, uint32_t width, void *compare));
ORIG_FN(0x004c6490, RpMeshHeader *, _rpMeshGenerateTriStrip,
        (RpBuildMesh * build, int32_t mode, uint32_t flags));
ORIG_FN(0x004c7560, RpMeshHeader *, _rpMeshGenerateTriStripAlt,
        (RpBuildMesh * build, int32_t mode, uint32_t flags));

/* A mesh header with one mesh per material, plain triangle lists (the build mesh sorted by material). */
BMX_ADDR(0x004c5e30)
RpMeshHeader *_rpMeshGenerateTriList(RpBuildMesh *build, uint32_t flags)
{
    const uint32_t count = build->numTriangles;
    RpBuildMeshTriangle **sorted = (RpBuildMeshTriangle **)engineAlloc(count << 2);
    if (sorted == NULL) {
        rpReportNoMem((int32_t)(count << 2));
        return NULL;
    }
    for (uint32_t i = 0; i < count; i++) {
        sorted[i] = &build->triangles[i];
    }
    rwQsort((void *)sorted, count, 4, RW_CB(void *, RP_MESH_SORT_COMPARE));
    int32_t numMeshes = 1;
    if (count > 1) {
        RpMaterial *current = sorted[0]->material;
        for (uint32_t i = 1; i < count; i++) {
            if (sorted[i]->material != current) {
                numMeshes++;
                current = sorted[i]->material;
            }
        }
    }
    const int32_t meshBytes = numMeshes * 0xc;
    const int32_t total = (int32_t)(count * 6) + 0x10 + meshBytes;
    RpMeshHeader *header = engineAlloc((size_t)total);
    if (header == NULL) {
        engineFree((void *)sorted);
        rpReportNoMem(total);
        return NULL;
    }
    header->numMeshes = 1;
    header->flags = 0;
    RpMesh *mesh = (RpMesh *)(header + 1);
    uint16_t *indices = (uint16_t *)((uint8_t *)mesh + meshBytes);
    uint16_t *serial = (uint16_t *)((uint8_t *)RwEngineInstance + rpMeshOffset);
    header->firstMeshOffset = 0;
    header->serialNum = *serial;
    header->totalIndices = count * 3;
    (*serial)++;
    mesh[0].numIndices = 0;
    mesh[0].indices = indices;
    mesh[0].material = sorted[0]->material;
    uint32_t index = 0;
    do {
        RpMesh *current = mesh;
        if (sorted[index]->material != mesh->material) {
            current = mesh + 1;
            current->numIndices = 0;
            current->material = sorted[index]->material;
            current->indices = indices;
            header->numMeshes++;
        }
        indices[0] = sorted[index]->vertIndex[0];
        indices[1] = sorted[index]->vertIndex[1];
        indices[2] = sorted[index]->vertIndex[2];
        indices += 3;
        current->numIndices += 3;
        mesh = current;
        index++;
    } while (index < count);
    _rpMeshHeaderForAllMeshes(header, RW_CB(RpMeshCallBack, RP_MESH_LIST_FINISH), NULL);
    engineFree((void *)sorted);
    return header;
}

/* Turns a build mesh into a mesh header (strips when asked to) and frees the build mesh. */
BMX_ADDR(0x004c4a30)
RpMeshHeader *_rpMeshGenerate(RpBuildMesh *build, uint32_t flags)
{
    if (build == NULL) {
        return NULL;
    }
    if (build->numTriangles == 0) {
        if (build->triangles != NULL) {
            engineFree(build->triangles);
            build->triangles = NULL;
        }
        engineFree(build);
        return &rpMeshEmptyHeader;
    }
    RpMeshHeader *header;
    if ((flags & 1) == 0) {
        header = _rpMeshGenerateTriList(build, flags);
    } else if (rpMeshStripMode == 2) {
        header = _rpMeshGenerateTriStripAlt(build, 1, flags);
    } else {
        header = _rpMeshGenerateTriStrip(build, rpMeshStripMode == 1 ? 1 : 0, flags);
    }
    if (header == NULL) {
        return NULL;
    }
    if (build->triangles != NULL) {
        engineFree(build->triangles);
        build->triangles = NULL;
    }
    engineFree(build);
    return header;
}

/* ---- bounding boxes and matrices (rw/math leftovers) ------------------------------------------------- */

/* Starts a box at a single point. */
BMX_ADDR(0x004e9900)
void rwBBoxInitialize(RwBBox *bbox, const ZVec3 *point)
{
    bbox->inf.x = point->x;
    bbox->inf.y = point->y;
    bbox->inf.z = point->z;
    bbox->sup.x = point->x;
    bbox->sup.y = point->y;
    bbox->sup.z = point->z;
}

/* Grows a box to hold a point (a NaN coordinate replaces the maximum: the original compares with FCOMP). */
BMX_ADDR(0x004e9810)
RwBBox *rwBBoxAddPoint(RwBBox *bbox, const ZVec3 *point)
{
    if (point->x < bbox->inf.x) {
        bbox->inf.x = point->x;
    }
    if (point->y < bbox->inf.y) {
        bbox->inf.y = point->y;
    }
    if (point->z < bbox->inf.z) {
        bbox->inf.z = point->z;
    }
    if (!(bbox->sup.x >= point->x)) {
        bbox->sup.x = point->x;
    }
    if (!(bbox->sup.y >= point->y)) {
        bbox->sup.y = point->y;
    }
    if (!(bbox->sup.z >= point->z)) {
        bbox->sup.z = point->z;
    }
    return bbox;
}

BMX_ADDR(0x004d0470)
double rwMatrixDeterminant(const RwMatrix *matrix)
{
    const float *p = (const float *)matrix;
    const double d1 = (double)p[9] * (double)p[4] - (double)p[8] * (double)p[5];
    const double d2 = (double)p[8] * (double)p[6] - (double)p[4] * (double)p[10];
    const double d3 = (double)p[10] * (double)p[5] - (double)p[6] * (double)p[9];
    const double g = d1 * (double)p[2];
    const double i = d2 * (double)p[1];
    const double k = d3 * (double)p[0];
    return (i + g) + k;
}

typedef float RwMatrixNormalLimits[3];
#define rwMatrixNormalErrorLimits   ORIG_VAR(0x0069b8b4, RwMatrixNormalLimits, rwMatrixNormalErrorLimits)
#define rwMatrixNormalErrorLimit(n) (rwMatrixNormalErrorLimits[n])

/* How far the squared lengths of the three axes are from 1: sum of squared deviations. */
BMX_ADDR(0x004d2190)
double rwMatrixNormalError(const RwMatrix *matrix)
{
    const float *p = (const float *)matrix;
    const double f1 =
        (((double)p[10] * (double)p[10] + (double)p[9] * (double)p[9]) + (double)p[8] * (double)p[8]) -
        (double)rwMatrixNormalErrorLimit(2);
    const double f2 =
        (((double)p[6] * (double)p[6] + (double)p[5] * (double)p[5]) + (double)p[4] * (double)p[4]) -
        (double)rwMatrixNormalErrorLimit(1);
    const double f3 =
        (((double)p[2] * (double)p[2] + (double)p[1] * (double)p[1]) + (double)p[0] * (double)p[0]) -
        (double)rwMatrixNormalErrorLimit(0);
    return (f2 * f2 + f1 * f1) + f3 * f3;
}

/* Sum of the squared dot products between the axes (zero for an orthogonal matrix). */
BMX_ADDR(0x004d2210)
double rwMatrixOrthogonalError(const RwMatrix *matrix)
{
    const float *p = (const float *)matrix;
    const double t1 =
        ((double)p[2] * (double)p[6] + (double)p[5] * (double)p[1]) + (double)p[4] * (double)p[0];
    const double t2 =
        ((double)p[2] * (double)p[10] + (double)p[1] * (double)p[9]) + (double)p[0] * (double)p[8];
    const double t3 =
        ((double)p[10] * (double)p[6] + (double)p[9] * (double)p[5]) + (double)p[8] * (double)p[4];
    return (t2 * t2 + t1 * t1) + t3 * t3;
}

/* ---- the world (binary world and sector reader/writer) ----------------------------------------------- */

#define rpWorldScale0             ORIG_VAR(0x0069b4f0, float, rpWorldScale0)
#define rpWorldScale1             ORIG_VAR(0x0069b4f4, float, rpWorldScale1)
#define rpWorldScale2             ORIG_VAR(0x0069b4f8, float, rpWorldScale2)
#define rpWorldReadScale0         ORIG_VAR(0x0069b4fc, float, rpWorldReadScale0)
#define rpWorldReadScale1         ORIG_VAR(0x0069b500, float, rpWorldReadScale1)
#define rpWorldReadScale2         ORIG_VAR(0x0069b504, float, rpWorldReadScale2)
#define RP_WORLD_DEFAULT_CALLBACK 0x004bc360

/* The world module data in the engine instance: the free list of world records and their list. */
static uint8_t *worldModule(void)
{
    return (uint8_t *)RwEngineInstance + rpWorldGlobalsOffset;
}

/* Marks a world as existing (with its allocation size) in the module's list. */
BMX_ADDR(0x004b9ed0)
void rpWorldRegister(RpWorld *world, uint32_t size)
{
    RpWorldListEntry *record = RwEngineInstance->freeListAlloc(*(RwFreeList **)(worldModule() + 4));
    if (record != NULL) {
        RwLLLink *head = (RwLLLink *)(worldModule() + 8);
        record->size = size;
        record->world = world;
        record->link.next = head->next;
        record->link.prev = head;
        head->next->prev = &record->link;
        head->next = &record->link;
    }
}

/* Removes a world from the module's list. */
BMX_ADDR(0x004b9f60)
void rpWorldUnregister(RpWorld *world)
{
    RwLLLink *head = (RwLLLink *)(worldModule() + 8);
    RwLLLink *link = head->next;
    while (link != head) {
        RpWorldListEntry *record = (RpWorldListEntry *)((uint8_t *)link - offsetof(RpWorldListEntry, link));
        if (record->world == world) {
            link->prev->next = link->next;
            link->next->prev = link->prev;
            RwEngineInstance->freeListFree(*(RwFreeList **)(worldModule() + 4), record);
            return;
        }
        link = link->next;
    }
}

static int worldCheck(const void *object)
{
    if (rpWorldModuleCount == 0) {
        rpReport(rwERR_NOTOPEN);
        return 0;
    }
    if (object == NULL) {
        rpReport(rwERR_NULLP);
        return 0;
    }
    return 1;
}

/* The world's surface properties (RpWorld + 0x60; the stream stores them after the inverse origin). */
BMX_ADDR(0x004b9c30)
RpWorld *RpWorldSetSurfaceProperties(RpWorld *world, const RwSurfaceProperties *surfaceProps)
{
    if (rpWorldModuleCount == 0) {
        rpReport(rwERR_NOTOPEN);
        return NULL;
    }
    if (world == NULL || surfaceProps == NULL) {
        rpReport(rwERR_NULLP);
        return NULL;
    }
    world->surfaceProps.ambient = surfaceProps->ambient;
    world->surfaceProps.specular = surfaceProps->specular;
    world->surfaceProps.diffuse = surfaceProps->diffuse;
    world->stamp++;
    return world;
}

BMX_ADDR(0x004b9cc0)
const RwSurfaceProperties *RpWorldGetSurfaceProperties(const RpWorld *world)
{
    return worldCheck(world) ? &world->surfaceProps : NULL;
}

BMX_ADDR(0x004ba270)
RpWorld *RpWorldSetKind(RpWorld *world, int32_t kind)
{
    if (!worldCheck(world)) {
        return NULL;
    }
    world->flags = (uint32_t)kind;
    world->stamp++;
    return world;
}

BMX_ADDR(0x004baf20)
RpWorld *RpWorldSetRenderCallBack(RpWorld *world, RpWorldSectorCallBackRender callback)
{
    if (rpWorldModuleCount == 0) {
        rpReport(rwERR_NOTOPEN);
        return NULL;
    }
    if (world == NULL || callback == NULL) {
        rpReport(rwERR_NULLP);
        return NULL;
    }
    world->renderCallBack = callback;
    world->stamp++;
    return world;
}

BMX_ADDR(0x004bbc40)
RpWorld *RpWorldSetFlags(RpWorld *world, uint32_t flags)
{
    if (!worldCheck(world)) {
        return NULL;
    }
    world->object.flags = (uint8_t)flags;
    world->stamp++;
    return world;
}

BMX_ADDR(0x004bbcc0)
uint8_t RpWorldGetFlags(const RpWorld *world)
{
    return worldCheck(world) ? world->object.flags : 0;
}

/* The box around all sectors of the world, walking the tree with an explicit stack. */
BMX_ADDR(0x004b9b90)
int rwWorldCalcBBox(RpWorld *world, RwBBox *bbox)
{
    int started = 0;
    int depth = 0;
    RpSector *stack[65] = {0};
    RpSector *node = world->rootSector;
    do {
        int next;
        if (node->type < 0) {
            const RpWorldSector *sector = (const RpWorldSector *)node;
            if (started) {
                rwBBoxAddPoint(bbox, &sector->boundingBox.inf);
            } else {
                rwBBoxInitialize(bbox, &sector->boundingBox.inf);
                started = 1;
            }
            rwBBoxAddPoint(bbox, &sector->boundingBox.sup);
            next = depth - 1;
            node = stack[depth];
        } else {
            const RpPlaneSector *plane = (const RpPlaneSector *)node;
            next = depth + 1;
            RpSector *left = plane->leftSubTree;
            stack[next] = plane->rightSubTree;
            node = left;
        }
        depth = next;
    } while (depth > -1);
    return 1;
}

/* Bytes the collision tree occupies: three per node, up to the highest node a walk reaches. */
BMX_ADDR(0x004bb760)
int32_t rpCollisionTreeSize(const uint8_t *tree)
{
    if (tree == NULL) {
        return 0;
    }
    int32_t index = 1;
    int32_t highest = 1;
    int32_t depth = 0;
    int32_t stack[16] = {0};
    do {
        if (highest < index) {
            highest = index;
        }
        int32_t next;
        if ((tree[index * 3] & 0x80) == 0) {
            next = depth - 1;
            index = stack[depth];
        } else {
            next = depth + 1;
            index = index * 2;
            stack[next] = index + 1;
        }
        depth = next;
    } while (depth > -1);
    return highest * 3 + 3;
}

/* Builds the render mesh of every sector that has none (triangle lists from the sector's material window). */
BMX_ADDR(0x004ba5a0)
RpWorld *_rpWorldGenerateSectorMeshes(RpWorld *world)
{
    if (world == NULL) {
        rpReport(rwERR_NULLP);
        return NULL;
    }
    RpSector *stack[64] = {0};
    int32_t depth = 0;
    RpWorld *result = world;
    RpSector *node = world->rootSector;
    do {
        RpSector *next;
        if (node->type < 0) {
            RpWorldSector *sector = (RpWorldSector *)node;
            RpMaterial **materials = world->matList.materials;
            if (sector->mesh == NULL) {
                RpBuildMesh *build = RpBuildMeshCreate((uint32_t)(int32_t)(int16_t)sector->numPolygons);
                if (build == NULL) {
                    return NULL;
                }
                if ((int16_t)sector->numPolygons != 0) {
                    int32_t i = 0;
                    do {
                        const uint8_t *triangle = (const uint8_t *)sector->polygons + i * 4;
                        _rpBuildMeshAddTriangle(build, materials[sector->matListWindowBase + triangle[0]],
                                                triangle[1], triangle[2], triangle[3]);
                        i++;
                    } while (i < (int32_t)sector->numPolygons);
                }
                RpMeshHeader *mesh = _rpMeshGenerate(build, (world->object.flags & 1) == 0 ? 0 : 1);
                if (mesh == NULL) {
                    RpBuildMeshDestroy(build);
                    result = NULL;
                } else {
                    sector->mesh = mesh;
                }
                if (result == NULL) {
                    return NULL;
                }
            }
            next = stack[depth];
            depth--;
        } else {
            next = ((const RpPlaneSector *)node)->leftSubTree;
            depth++;
            stack[depth] = ((const RpPlaneSector *)node)->rightSubTree;
        }
        node = next;
        if (depth < 0) {
            return result;
        }
    } while (1);
}

/* Bytes of a sector's vertex and triangle data for the given format flags. */
BMX_ADDR(0x004c2c50)
__attribute__((regparm(3))) int32_t rpSectorDataSize(const RpWorldSector *sector, int32_t unused,
                                                     uint32_t flags)
{
    (void)unused;
    const uint32_t numVertices = sector->numVertices;
    uint32_t size = numVertices * 12;
    if ((flags & 0x10) != 0) {
        size = numVertices * 16;
    }
    size += 0x2c;
    if ((flags & 8) != 0) {
        size += numVertices * 4;
    }
    if ((flags & 4) != 0) {
        size += numVertices * 8;
    }
    size += (uint32_t)sector->numPolygons * 4;
    if (sector->colSectorRoot != NULL) {
        size += 0x180;
    }
    return (int32_t)size;
}

/* Bytes of a sector chunk: its data plus its plugin data. */
BMX_ADDR(0x004c2ca0)
__attribute__((regparm(3))) int32_t rpSectorStreamGetSize(const RpWorldSector *sector, int32_t unused,
                                                          uint32_t flags)
{
    const int32_t data = rpSectorDataSize(sector, unused, flags);
    return data + _rwPluginRegistryGetSize(&rpWorldSectorRegistry, sector) + 0x18;
}

/* Bytes of the tree of plane and sector chunks below a plane node. */
BMX_ADDR(0x004c3490)
__attribute__((regparm(3))) int32_t rpBspStreamGetSize(const RpPlaneSector *node, int32_t unused,
                                                       uint32_t flags)
{
    int32_t size;
    if (node->leftSubTree->type < 0) {
        size = rpSectorStreamGetSize((const RpWorldSector *)node->leftSubTree, unused, flags);
    } else {
        size = rpBspStreamGetSize((const RpPlaneSector *)node->leftSubTree, unused, flags);
    }
    size += 0x3c;
    if (node->rightSubTree->type < 0) {
        return size + rpSectorStreamGetSize((const RpWorldSector *)node->rightSubTree, unused, flags);
    }
    return size + rpBspStreamGetSize((const RpPlaneSector *)node->rightSubTree, unused, flags);
}

/* Writes a sector chunk (EAX = sector, EDX = stream; the flags are the fourth stack slot). */
BMX_ADDR(0x004c2cd0)
__attribute__((regparm(3))) RpWorldSector *rpSectorStreamWrite(const RpWorldSector *sector, RwStream *stream,
                                                               int32_t unused, int32_t pad0, int32_t pad1,
                                                               int32_t pad2, uint32_t flags)
{
    (void)pad0;
    (void)pad1;
    (void)pad2;
    if (_rwStreamWriteVersionedChunkHeader(stream, 9, rpSectorStreamGetSize(sector, unused, flags), 0x303) ==
        NULL) {
        return NULL;
    }
    if (_rwStreamWriteVersionedChunkHeader(stream, 1, rpSectorDataSize(sector, unused, flags), 0x303) ==
        NULL) {
        return NULL;
    }
    int32_t header[11];
    header[0] = sector->matListWindowBase;
    header[1] = sector->numPolygons;
    header[2] = sector->numVertices;
    header[3] = ((const int32_t *)&sector->boundingBox.inf)[0];
    header[4] = ((const int32_t *)&sector->boundingBox.inf)[1];
    header[5] = ((const int32_t *)&sector->boundingBox.inf)[2];
    header[6] = ((const int32_t *)&sector->boundingBox.sup)[0];
    header[7] = ((const int32_t *)&sector->boundingBox.sup)[1];
    header[8] = ((const int32_t *)&sector->boundingBox.sup)[2];
    header[10] = 0;
    header[9] = sector->colSectorRoot != NULL ? 1 : 0;
    RwMemNative16(&header[3], 0xc);
    RwMemNative16(&header[6], 0xc);
    RwMemLittleEndian32(header, 0x2c);
    if (RwStreamWrite(stream, header, 0x2c) == NULL) {
        return NULL;
    }
    const uint32_t numVertices = sector->numVertices;
    if (numVertices != 0) {
        if (RwStreamWriteReal(stream, sector->vertices, numVertices * 12) == NULL) {
            return NULL;
        }
        if ((flags & 0x10) != 0 && RwStreamWrite(stream, sector->normals, numVertices << 2) == NULL) {
            return NULL;
        }
        if ((flags & 8) != 0 && RwStreamWrite(stream, sector->preLitLum, numVertices << 2) == NULL) {
            return NULL;
        }
        if ((flags & 4) != 0 && RwStreamWriteReal(stream, sector->texCoords, numVertices << 3) == NULL) {
            return NULL;
        }
    }
    if (sector->numPolygons != 0 &&
        RwStreamWrite(stream, sector->polygons, (uint32_t)sector->numPolygons << 2) == NULL) {
        return NULL;
    }
    if (sector->colSectorRoot != NULL) {
        uint32_t copy[96];
        const int32_t size = rpCollisionTreeSize(sector->colSectorRoot);
        const uint32_t *source = (const uint32_t *)sector->colSectorRoot;
        for (uint32_t i = 0; i < ((uint32_t)size + 3U) >> 2; i++) {
            copy[i] = source[i];
        }
        if (RwStreamWrite(stream, copy, 0x180) == NULL) {
            return NULL;
        }
    }
    if (_rwPluginRegistryWriteDataChunks(&rpWorldSectorRegistry, stream, sector) == NULL) {
        return NULL;
    }
    return (RpWorldSector *)sector;
}

/* Writes a plane node chunk and, recursively, its two children (EAX = node, EDX = stream, ECX = world). */
BMX_ADDR(0x004c34f0)
__attribute__((regparm(3))) RpPlaneSector *rpBspStreamWrite(const RpPlaneSector *node, RwStream *stream,
                                                            int32_t world, int32_t pad0, int32_t pad1,
                                                            int32_t pad2, uint32_t flags)
{
    (void)pad0;
    (void)pad1;
    (void)pad2;
    if (_rwStreamWriteVersionedChunkHeader(stream, 10, rpBspStreamGetSize(node, world, flags), 0x303) ==
        NULL) {
        return NULL;
    }
    if (_rwStreamWriteVersionedChunkHeader(stream, 1, 0x18, 0x303) == NULL) {
        return NULL;
    }
    int32_t header[6];
    header[0] = node->type;
    header[1] = (int32_t)((const uint32_t *)node)[1];
    header[2] = node->leftSubTree->type < 0 ? 1 : 0;
    header[3] = node->rightSubTree->type < 0 ? 1 : 0;
    header[4] = (int32_t)((const uint32_t *)node)[4];
    header[5] = (int32_t)((const uint32_t *)node)[5];
    RwMemNative16(&header[1], 4);
    RwMemNative16(&header[4], 4);
    RwMemNative16(&header[5], 4);
    RwMemLittleEndian32(header, 0x18);
    if (RwStreamWrite(stream, header, 0x18) == NULL) {
        return NULL;
    }
    const RpSector *left = node->leftSubTree;
    const RpSector *right = node->rightSubTree;
    if (left->type < 0) {
        if (rpSectorStreamWrite((const RpWorldSector *)left, stream, world, 0, 0, 0, flags) == NULL) {
            return NULL;
        }
    } else if (rpBspStreamWrite((const RpPlaneSector *)left, stream, world, 0, 0, 0, flags) == NULL) {
        return NULL;
    }
    int ok;
    if (right->type < 0) {
        ok = rpSectorStreamWrite((const RpWorldSector *)right, stream, world, 0, 0, 0, flags) != NULL;
    } else {
        ok = rpBspStreamWrite((const RpPlaneSector *)right, stream, world, 0, 0, 0, flags) != NULL;
    }
    return ok ? (RpPlaneSector *)node : NULL;
}

/* Allocates from the single block the world is read into (the cursor moves on); reports when it is NULL. */
static void *worldCursorTake(uint8_t **cursor, uint32_t size, int *failed)
{
    uint8_t *block = *cursor;
    *cursor += size;
    *failed = block == NULL;
    return block;
}

/* Reads a sector chunk into the world's memory block (EAX = stream, EDX = block cursor, ECX = world). */
BMX_ADDR(0x004c2f50)
__attribute__((regparm(3))) RpWorldSector *rpSectorStreamRead(RwStream *stream, uint8_t **cursor,
                                                              int32_t world, int32_t pad0, int32_t pad1,
                                                              int32_t pad2, uint32_t flags)
{
    (void)world;
    (void)pad0;
    (void)pad1;
    (void)pad2;
    uint32_t version;
    if (RwStreamFindChunk(stream, 1, NULL, &version) == 0) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        rpReport(rwERR_BADVERSION);
        return NULL;
    }
    int32_t header[11];
    if (RwStreamRead(stream, header, 0x2c) != 0x2c) {
        return NULL;
    }
    RwMemNative32(header, 0x2c);
    RwMemFloat32ToReal(&header[3], 0xc);
    RwMemFloat32ToReal(&header[6], 0xc);
    int failed;
    RpWorldSector *sector = worldCursorTake(cursor, (uint32_t)rpWorldSectorRegistry.sizeOfStruct, &failed);
    if (failed) {
        rpReportNoMem(rpWorldSectorRegistry.sizeOfStruct);
        return NULL;
    }
    sector->type = -1;
    sector->boundingBox.inf.x = ((const float *)header)[3];
    sector->boundingBox.inf.y = ((const float *)header)[4];
    sector->boundingBox.inf.z = ((const float *)header)[5];
    sector->boundingBox.sup.x = ((const float *)header)[6];
    sector->boundingBox.sup.y = ((const float *)header)[7];
    sector->boundingBox.sup.z = ((const float *)header)[8];
    sector->matListWindowBase = (uint16_t)header[0];
    sector->numPolygons = (uint16_t)header[1];
    sector->lightsInWorldSector.prev = &sector->lightsInWorldSector;
    sector->lightsInWorldSector.next = &sector->lightsInWorldSector;
    sector->noCollAtomicsInWorldSector.prev = &sector->noCollAtomicsInWorldSector;
    sector->noCollAtomicsInWorldSector.next = &sector->noCollAtomicsInWorldSector;
    sector->collAtomicsInWorldSector.prev = &sector->collAtomicsInWorldSector;
    sector->mesh = NULL;
    sector->repEntry = 0;
    sector->collAtomicsInWorldSector.next = &sector->collAtomicsInWorldSector;
    sector->stamp = 0;
    sector->colSectorRoot = NULL;
    sector->texCoords = NULL;
    sector->normals = NULL;
    sector->preLitLum = NULL;
    sector->vertices = NULL;
    sector->polygons = NULL;
    sector->numVertices = (uint16_t)header[2];
    const int32_t numVertices = header[2];
    if (numVertices != 0) {
        const int32_t size = numVertices * 0xc;
        sector->vertices = worldCursorTake(cursor, (uint32_t)size, &failed);
        if (failed) {
            rpReportNoMem(size);
            return NULL;
        }
        if (RwStreamReadReal(stream, sector->vertices, (uint32_t)numVertices * 12) == NULL) {
            return NULL;
        }
        if ((flags & 0x10) != 0) {
            const int32_t normalsSize = numVertices * 4;
            sector->normals = worldCursorTake(cursor, (uint32_t)normalsSize, &failed);
            if (failed) {
                rpReportNoMem(normalsSize);
                return NULL;
            }
            if (RwStreamRead(stream, sector->normals, (uint32_t)normalsSize) != (uint32_t)normalsSize) {
                return NULL;
            }
        }
        if ((flags & 8) != 0) {
            const int32_t preLitSize = numVertices << 2;
            sector->preLitLum = worldCursorTake(cursor, (uint32_t)preLitSize, &failed);
            if (failed) {
                rpReportNoMem(preLitSize);
                return NULL;
            }
            if (RwStreamRead(stream, sector->preLitLum, (uint32_t)preLitSize) == 0) {
                return NULL;
            }
        }
        if ((flags & 4) != 0) {
            const int32_t texSize = (int32_t)((uint32_t)sector->numVertices << 3);
            sector->texCoords = worldCursorTake(cursor, (uint32_t)texSize, &failed);
            if (failed) {
                rpReportNoMem(texSize);
                return NULL;
            }
            if (RwStreamReadReal(stream, sector->texCoords, (uint32_t)numVertices << 3) == NULL) {
                return NULL;
            }
        }
    }
    if (header[1] != 0) {
        const int32_t size = header[1] * 4;
        sector->polygons = worldCursorTake(cursor, (uint32_t)size, &failed);
        if (failed) {
            rpReportNoMem(size);
            return NULL;
        }
        if (RwStreamRead(stream, sector->polygons, (uint32_t)size) != (uint32_t)size) {
            return NULL;
        }
    }
    if (header[9] == 0) {
        sector->colSectorRoot = NULL;
    } else {
        uint32_t tree[96];
        if (RwStreamRead(stream, tree, 0x180) != 0x180) {
            return NULL;
        }
        const int32_t size = rpCollisionTreeSize((const uint8_t *)tree);
        const uint32_t padded = ((uint32_t)size + 3U) & 0xfffffffcU;
        uint32_t *destination = worldCursorTake(cursor, padded, &failed);
        sector->colSectorRoot = (uint8_t *)destination;
        if (failed) {
            rpReportNoMem((int32_t)padded);
            return NULL;
        }
        for (uint32_t i = 0; i < ((uint32_t)size + 3U) >> 2; i++) {
            destination[i] = tree[i];
        }
    }
    _rwPluginRegistryInitObject(&rpWorldSectorRegistry, sector);
    if (_rwPluginRegistryReadDataChunks(&rpWorldSectorRegistry, stream, sector) != NULL) {
        return sector;
    }
    return NULL;
}

/* Reads a plane chunk and, recursively, its children (EAX = stream, EDX = block cursor, ECX = world). */
BMX_ADDR(0x004c36a0)
__attribute__((regparm(3))) RpPlaneSector *rpBspStreamRead(RwStream *stream, uint8_t **cursor, int32_t world,
                                                           int32_t pad0, int32_t pad1, int32_t pad2,
                                                           uint32_t flags)
{
    (void)pad0;
    (void)pad1;
    (void)pad2;
    uint32_t length;
    uint32_t version;
    if (RwStreamFindChunk(stream, 1, &length, &version) == 0) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        rpReport(rwERR_BADVERSION);
        return NULL;
    }
    int32_t header[6] = {0, 0, 0, 0, 0, 0};
    if (RwStreamRead(stream, header, length) != length) {
        return NULL;
    }
    RwMemNative32(header, 0x18);
    RwMemFloat32ToReal(&header[1], 4);
    RwMemFloat32ToReal(&header[4], 4);
    RwMemFloat32ToReal(&header[5], 4);
    int failed;
    RpPlaneSector *node = worldCursorTake(cursor, sizeof(RpPlaneSector), &failed);
    if (failed) {
        rpReportNoMem(sizeof(RpPlaneSector));
        return NULL;
    }
    /* The values are the chunk's float bit patterns. */
    node->type = header[0];
    __builtin_memcpy(&node->value, &header[1], sizeof node->value);
    if ((flags & 0x40000000) == 0) {
        __builtin_memcpy(&node->leftValue, &header[1], sizeof node->leftValue);
        __builtin_memcpy(&node->rightValue, &header[1], sizeof node->rightValue);
    } else {
        __builtin_memcpy(&node->leftValue, &header[4], sizeof node->leftValue);
        __builtin_memcpy(&node->rightValue, &header[5], sizeof node->rightValue);
    }
    RpSector *child;
    if (header[2] == 0) {
        if (RwStreamFindChunk(stream, 10, NULL, &version) == 0) {
            return NULL;
        }
        if (version < 0x301 || version > 0x303) {
            rpReport(rwERR_BADVERSION);
            return NULL;
        }
        child = (RpSector *)rpBspStreamRead(stream, cursor, world, 0, 0, 0, flags);
    } else {
        if (RwStreamFindChunk(stream, 9, NULL, &version) == 0) {
            return NULL;
        }
        if (version < 0x301 || version > 0x303) {
            rpReport(rwERR_BADVERSION);
            return NULL;
        }
        child = (RpSector *)rpSectorStreamRead(stream, cursor, world, 0, 0, 0, flags);
    }
    node->leftSubTree = child;
    if (child == NULL) {
        return NULL;
    }
    if (header[3] == 0) {
        if (RwStreamFindChunk(stream, 10, NULL, &version) == 0) {
            return NULL;
        }
        if (version < 0x301 || version > 0x303) {
            rpReport(rwERR_BADVERSION);
            return NULL;
        }
        child = (RpSector *)rpBspStreamRead(stream, cursor, world, 0, 0, 0, flags);
    } else {
        if (RwStreamFindChunk(stream, 9, NULL, &version) == 0) {
            return NULL;
        }
        if (version < 0x301 || version > 0x303) {
            rpReport(rwERR_BADVERSION);
            return NULL;
        }
        child = (RpSector *)rpSectorStreamRead(stream, cursor, world, 0, 0, 0, flags);
    }
    node->rightSubTree = child;
    return child != NULL ? node : NULL;
}

/* Writes a world chunk: struct (counts, scaled inverse origin, surface properties), materials, the tree,
 * plugin data. */
BMX_ADDR(0x004c3ae0)
RpWorld *RpWorldStreamWrite(RpWorld *world, RwStream *stream)
{
    const uint32_t flags = RpWorldGetFlags(world);
    (void)RpWorldGetFlags(world);
    const int32_t materialsSize = _rpMaterialListStreamGetSize(&world->matList);
    int32_t size;
    if (world->rootSector->type < 0) {
        size =
            rpSectorStreamGetSize((const RpWorldSector *)world->rootSector, (int32_t)(intptr_t)world, flags) +
            0x58 + materialsSize;
    } else {
        size = rpBspStreamGetSize((const RpPlaneSector *)world->rootSector, (int32_t)(intptr_t)world, flags) +
               0x58 + materialsSize;
    }
    const int32_t plugins = _rwPluginRegistryGetSize(&rpWorldRegistry, world);
    if (_rwStreamWriteVersionedChunkHeader(stream, 0xb, plugins + 0xc + size, 0x303) == NULL ||
        _rwStreamWriteVersionedChunkHeader(stream, 1, 0x34, 0x303) == NULL) {
        return NULL;
    }
    int32_t numSectorTriangles = 0;
    int32_t numSectorVertices = 0;
    int32_t numPlanes = 0;
    int32_t numSectors = 0;
    int32_t collisionBytes = 0;
    RpSector *stack[64] = {0};
    int32_t depth = 0;
    RpSector *node = world->rootSector;
    do {
        RpSector *next;
        int32_t nextDepth;
        if (node->type < 0) {
            const RpWorldSector *sector = (const RpWorldSector *)node;
            numSectorTriangles += sector->numPolygons;
            numSectorVertices += sector->numVertices;
            numSectors++;
            collisionBytes += (rpCollisionTreeSize(sector->colSectorRoot) + 3) & ~3;
            nextDepth = depth - 1;
            next = stack[depth];
        } else {
            nextDepth = depth + 1;
            next = ((const RpPlaneSector *)node)->leftSubTree;
            numPlanes++;
            stack[nextDepth] = ((const RpPlaneSector *)node)->rightSubTree;
        }
        node = next;
        depth = nextDepth;
    } while (depth > -1);
    int32_t header[13];
    header[12] = (int32_t)(flags | 0x40000000);
    header[0] = world->rootSector->type < 0 ? 1 : 0;
    ((float *)header)[1] = (float)((double)world->worldOrigin.x * (double)rpWorldScale0);
    ((float *)header)[2] = (float)((double)rpWorldScale1 * (double)world->worldOrigin.y);
    ((float *)header)[3] = (float)((double)rpWorldScale2 * (double)world->worldOrigin.z);
    const RwSurfaceProperties *surfaceProps = RpWorldGetSurfaceProperties(world);
    ((float *)header)[4] = surfaceProps->ambient;
    ((float *)header)[5] = surfaceProps->specular;
    ((float *)header)[6] = surfaceProps->diffuse;
    header[7] = numSectorTriangles;
    header[8] = numSectorVertices;
    header[9] = numPlanes;
    header[10] = numSectors;
    header[11] = collisionBytes;
    RwMemNative16(&header[1], 0xc);
    RwMemNative16(&header[4], 0xc);
    RwMemLittleEndian32(header, 0x34);
    if (RwStreamWrite(stream, header, 0x34) == NULL ||
        _rpMaterialListStreamWrite(&world->matList, stream) == NULL) {
        return NULL;
    }
    const void *written;
    if (world->rootSector->type < 0) {
        written = rpSectorStreamWrite((const RpWorldSector *)world->rootSector, stream,
                                      (int32_t)(intptr_t)world, 0, 0, 0, flags);
    } else {
        written = rpBspStreamWrite((const RpPlaneSector *)world->rootSector, stream, (int32_t)(intptr_t)world,
                                   0, 0, 0, flags);
    }
    if (written == NULL || _rwPluginRegistryWriteDataChunks(&rpWorldRegistry, stream, world) == NULL) {
        return NULL;
    }
    return world;
}

/* Reads a world chunk into one block: header counts, materials, the tree of planes and sectors, plugin data.
 */
BMX_ADDR(0x004c3e60)
RpWorld *RpWorldStreamRead(RwStream *stream)
{
    uint32_t length;
    uint32_t version;
    if (RwStreamFindChunk(stream, 1, &length, &version) == 0) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        rpReport(rwERR_BADVERSION);
        return NULL;
    }
    int32_t header[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (RwStreamRead(stream, header, length) != length) {
        return NULL;
    }
    RwMemNative32(header, 0x34);
    RwMemFloat32ToReal(&header[1], 0xc);
    RwMemFloat32ToReal(&header[4], 0xc);
    const uint32_t format = (uint32_t)header[12];
    int32_t total = rpWorldSectorRegistry.sizeOfStruct * header[10] + rpWorldRegistry.sizeOfStruct +
                    header[9] * (int32_t)sizeof(RpPlaneSector) + header[8] * 0xc;
    if ((format & 0x10) != 0) {
        total += header[8] * 4;
    }
    if ((format & 8) != 0) {
        total += header[8] * 4;
    }
    if ((format & 4) != 0) {
        total += header[8] * 8;
    }
    const uint32_t size = (uint32_t)(total + header[7] * 4 + header[11]);
    uint32_t *block = engineAlloc(size);
    if (block == NULL) {
        RwError error;
        error.pluginID = kRpErrorPlugin;
        error.errorCode = _rwerror(rwERR_NOMEM, (int32_t)size);
        RwErrorSet(&error);
        return NULL;
    }
    for (uint32_t i = 0; i < (size >> 2); i++) {
        block[i] = 0;
    }
    for (uint32_t i = 0; i < (size & 3); i++) {
        ((uint8_t *)block)[(size & ~3U) + i] = 0;
    }
    RpWorld *world = (RpWorld *)block;
    uint8_t *cursor = (uint8_t *)block + rpWorldRegistry.sizeOfStruct;
    world->object.parent = NULL;
    world->object.privateFlags = 1;
    world->object.flags = 0;
    world->object.subType = 0;
    world->object.type = 7;
    rpWorldRegister(world, size);
    RpWorldSetFlags(world, format & 0xff);
    world->stamp = 0;
    RpWorldSetKind(world, 2);
    RpWorldSetSurfaceProperties(world, (const RwSurfaceProperties *)&header[4]);
    const float readX = rpWorldReadScale0;
    const float readY = rpWorldReadScale1;
    const float readZ = rpWorldReadScale2;
    world->worldOrigin.x = (float)((double)readX * (double)((const float *)header)[1]);
    world->worldOrigin.y = (float)((double)readY * (double)((const float *)header)[2]);
    world->rootSector = NULL;
    world->worldOrigin.z = (float)((double)readZ * (double)((const float *)header)[3]);
    if (RwStreamFindChunk(stream, 8, NULL, &version) == 0) {
        rpWorldUnregister(world);
        engineFree(world);
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        rpReport(rwERR_BADVERSION);
        return NULL;
    }
    if (_rpMaterialListStreamRead(stream, &world->matList) == NULL) {
        rpWorldUnregister(world);
        engineFree(world);
        return NULL;
    }
    if (header[0] == 0) {
        if (RwStreamFindChunk(stream, 10, NULL, &version) == 0) {
            rpWorldUnregister(world);
            engineFree(world);
            return NULL;
        }
        if (version < 0x301 || version > 0x303) {
            rpReport(rwERR_BADVERSION);
            return NULL;
        }
        world->rootSector =
            (RpSector *)rpBspStreamRead(stream, &cursor, (int32_t)(intptr_t)world, 0, 0, 0, format);
    } else {
        if (RwStreamFindChunk(stream, 9, NULL, &version) == 0) {
            rpWorldUnregister(world);
            engineFree(world);
            return NULL;
        }
        if (version < 0x301 || version > 0x303) {
            rpReport(rwERR_BADVERSION);
            return NULL;
        }
        world->rootSector =
            (RpSector *)rpSectorStreamRead(stream, &cursor, (int32_t)(intptr_t)world, 0, 0, 0, format);
    }
    if (world->rootSector == NULL) {
        rpWorldUnregister(world);
        engineFree(world);
        return NULL;
    }
    world->numClumpsInWorld = 0;
    world->directionalLightList.prev = &world->directionalLightList;
    world->directionalLightList.next = &world->directionalLightList;
    world->lightList.prev = &world->lightList;
    world->lightList.next = &world->lightList;
    world->currentClumpLink = &world->clumpList;
    world->clumpList.prev = &world->clumpList;
    world->clumpList.next = &world->clumpList;
    rwWorldCalcBBox(world, &world->boundingBox);
    RpWorldSetRenderCallBack(world, RW_CB(RpWorldSectorCallBackRender, RP_WORLD_DEFAULT_CALLBACK));
    for (int32_t i = 0; i < world->matList.numMaterials; i++) {
        RpMaterial *material = world->matList.materials[i];
        RwTexture *texture = RpMaterialGetTexture(material);
        if (texture != NULL) {
            RpMaterialSetTexture(material, texture);
        }
    }
    _rwPluginRegistryInitObject(&rpWorldRegistry, world);
    if (_rwPluginRegistryReadDataChunks(&rpWorldRegistry, stream, world) == NULL) {
        rpWorldUnregister(world);
        engineFree(world);
        return NULL;
    }
    if (_rpWorldGenerateSectorMeshes(world) == NULL) {
        RpWorldDestroy(world);
        return NULL;
    }
    return world;
}

/* ---- module open/close and the callbacks the registries keep ---------------------------------------- */

#define rpClumpModuleCount ORIG_VAR(0x00676714, int32_t, rpClumpModuleCount)
#define rpMeshModuleCount  ORIG_VAR(0x006966ec, int32_t, rpMeshModuleCount)

/* Free list callback of the clump module's atomic list: destroys the atomic. */
BMX_ADDR(0x004b1070)
void rpAtomicFreeListDestroyCB(void *atomic, void *data)
{
    RpAtomicDestroy(atomic);
}

/* The default callback of RpClumpForAllAtomics-style walks: returns its argument. */
BMX_ADDR(0x004b20f0)
void *rpClumpDefaultCallback(void *clump)
{
    return clump;
}

/* Releases a clump that is still alive when the module closes (EAX = clump); 1. Body at 0x4b21e4, 4 bytes
 * behind RpClumpDestroy's entry: no hook of its own (RpClumpDestroy's jmp covers it), its only caller
 * 0x4b1060 is ported. */
__attribute__((regparm(1))) int rpClumpEntryDestroy(RpClump *clump)
{
    clumpRelease(clump);
    return 1;
}

/* The free list callback that calls it. */
BMX_ADDR(0x004b1060)
int rpClumpFreeListDestroyCB(void *clump, void *data)
{
    (void)data;
    return rpClumpEntryDestroy(clump);
}

/*
 * Clump module open: the atomic (0x14 per block) and clump (10 per block) free lists, the atomics' render
 * pipeline with the world extension's entry 1 and the light hook at +0x10. NULL, with nothing left behind,
 * when a part fails.
 */
BMX_ADDR(0x004b0f20)
void *_rpClumpOpen(void *engine, int32_t offset)
{
    rpClumpModuleOffset = offset;
    clumpGlobals()->atomicFreeList = RwFreeListCreate(rpAtomicRegistry.sizeOfStruct, 0x14, 0);
    if (clumpGlobals()->atomicFreeList == NULL) {
        return NULL;
    }
    clumpGlobals()->clumpFreeList = RwFreeListCreate(rpClumpRegistry.sizeOfStruct, 10, 0);
    if (clumpGlobals()->clumpFreeList != NULL) {
        clumpGlobals()->pipeline = _rpWorldPipelineOpen();
        if (clumpGlobals()->pipeline != NULL) {
            rpWorldExtCallSlot1(clumpGlobals()->pipeline);
            rpClumpModuleCount++;
            clumpGlobals()->pipeline->stage4 = RW_CB(void(BMX_CDECL *)(void *, void *), 0x004b1860);
            return engine;
        }
        RwFreeListDestroy(clumpGlobals()->clumpFreeList);
        clumpGlobals()->clumpFreeList = NULL;
    }
    RwFreeListDestroy(clumpGlobals()->atomicFreeList);
    clumpGlobals()->atomicFreeList = NULL;
    return NULL;
}

/*
 * Clump module close: releases the clump pipeline, destroys the clumps and atomics still alive and both free
 * lists.
 */
BMX_ADDR(0x004b1080)
void *_rpClumpClose(void *engine)
{
    rwIm3DPipelineRelease(clumpGlobals()->pipeline);
    clumpGlobals()->pipeline = NULL;
    RwFreeListForAllUsed(clumpGlobals()->clumpFreeList, RW_CB(RwFreeListCallBack, 0x004b1060), NULL);
    RwFreeListForAllUsed(clumpGlobals()->atomicFreeList,
                         BMX_FNPTR(RwFreeListCallBack, 0x004b1070, rpAtomicFreeListDestroyCB), NULL);
    RwFreeListDestroy(clumpGlobals()->atomicFreeList);
    RwFreeListDestroy(clumpGlobals()->clumpFreeList);
    clumpGlobals()->atomicFreeList = NULL;
    clumpGlobals()->clumpFreeList = NULL;
    rpClumpModuleCount--;
    return engine;
}

/*
 * Brings the atomic's bounding sphere up to date: the sphere of the start morph target, or the blend of the
 * start and end morph targets by the interpolator's position (kept in an x87 register) when they differ and
 * both exist; the first morph target if an index is out of range. The vector parts go through float stores
 * before they are scaled. Clears the interpolator's dirty bit and marks the atomic's world sphere stale.
 */
BMX_ADDR(0x004b1154)
__attribute__((regparm(1))) void rpAtomicUpdateBoundingSphere(RpAtomic *atomic)
{
    RpGeometry *geometry = atomic->geometry;
    if (geometry == NULL) {
        return;
    }
    const int32_t start = atomic->interpolator.startMorphTarget;
    const int32_t end = atomic->interpolator.endMorphTarget;
    const int32_t count = geometry->numMorphTargets;
    const RpMorphTarget *targets = geometry->morphTarget;
    if (start != end && start < count && end < count) {
        const RpMorphTarget *from = &targets[start];
        const RpMorphTarget *to = &targets[end];
        const double t = (double)atomic->interpolator.position * (double)atomic->interpolator.recipTime;
        atomic->boundingSphere.radius =
            (float)(from->boundingSphere.radius +
                    (double)(to->boundingSphere.radius - (double)from->boundingSphere.radius) * t);
        const float x = (float)((double)to->boundingSphere.center.x - (double)from->boundingSphere.center.x);
        const float y = (float)((double)to->boundingSphere.center.y - (double)from->boundingSphere.center.y);
        const float z = (float)((double)to->boundingSphere.center.z - (double)from->boundingSphere.center.z);
        const float xs = (float)((double)x * t);
        const float ys = (float)((double)y * t);
        const float zs = (float)(t * (double)z);
        atomic->boundingSphere.center.x = (float)((double)xs + (double)from->boundingSphere.center.x);
        atomic->boundingSphere.center.y = (float)((double)ys + (double)from->boundingSphere.center.y);
        atomic->boundingSphere.center.z = (float)((double)zs + (double)from->boundingSphere.center.z);
    } else if (start < count && end < count) {
        atomic->boundingSphere = targets[start].boundingSphere;
    } else {
        atomic->boundingSphere = targets[0].boundingSphere;
    }
    atomic->interpolator.flags &= 0xfffffffd;
    atomic->object.object.privateFlags |= 1;
}

/* The atomic's sync callback: refreshes the bounding sphere if the interpolator asks for it. */
BMX_ADDR(0x004b1490)
RpAtomic *rpAtomicSyncCallback(RpAtomic *atomic)
{
    if ((atomic->interpolator.flags & 2) != 0) {
        rpAtomicUpdateBoundingSphere(atomic);
    }
    atomic->object.object.privateFlags |= 1;
    return atomic;
}

/*
 * Sets the atomic's geometry (adds a reference to the new one, drops the old one). Unless bit 0 of flags is
 * set the bounding sphere is that of the first morph target and the frame is told that an object changed.
 */
BMX_ADDR(0x004b1ad0)
RpAtomic *RpAtomicSetGeometry(RpAtomic *atomic, RpGeometry *geometry, uint32_t flags)
{
    RpGeometry *old = atomic->geometry;
    if (geometry != old) {
        if (geometry != NULL) {
            RpGeometryAddRef(geometry);
            old = atomic->geometry;
        }
        if (old != NULL) {
            RpGeometryDestroy(old);
        }
        atomic->geometry = geometry;
        if ((flags & 1) == 0) {
            if (geometry != NULL) {
                atomic->boundingSphere = geometry->morphTarget[0].boundingSphere;
            }
            if (atomic->object.object.parent != NULL) {
                RwFrameUpdateObjects(atomic->object.object.parent);
            }
        }
        atomic->stamp++;
    }
    return atomic;
}

/* The atomic's default render callback: runs it through the clump pipeline. NULL if that cannot be pushed. */
BMX_ADDR(0x004b3350)
RpAtomic *rpAtomicRenderCallback(RpAtomic *atomic)
{
    if (rwIm3DPipelinePush(clumpGlobals()->pipeline) == NULL) {
        return NULL;
    }
    RpAtomic *result = (RpAtomic *)rwIm3DPipelineRun((RwIm3DTransformInfo *)atomic);
    rwIm3DPipelinePop();
    return result;
}

/* Geometry module: the free list of geometries. NULL when it cannot be created. */
BMX_ADDR(0x004b3390)
void *_rpGeometryOpen(void *engine, int32_t offset)
{
    rpGeometryOffset = offset;
    *(RwFreeList **)((uint8_t *)RwEngineInstance + offset) =
        RwFreeListCreate(rpGeometryRegistry.sizeOfStruct, 0x14, 0);
    if (geometryFreeList() == NULL) {
        return NULL;
    }
    rpGeometryModuleCount++;
    return engine;
}

/* Free list callback of the geometry list: drops a reference. */
BMX_ADDR(0x004b33f0)
void rpGeometryFreeListDestroyCB(void *geometry, void *data)
{
    RpGeometryDestroy(geometry);
}

BMX_ADDR(0x004b3400)
void *_rpGeometryClose(void *engine)
{
    RwFreeListForAllUsed(geometryFreeList(),
                         BMX_FNPTR(RwFreeListCallBack, 0x004b33f0, rpGeometryFreeListDestroyCB), NULL);
    RwFreeListDestroy(geometryFreeList());
    rpGeometryModuleCount--;
    *(RwFreeList **)((uint8_t *)RwEngineInstance + rpGeometryOffset) = NULL;
    return engine;
}

/* Mesh module: a one-halfword marker in the engine instance (1) and a counter. */
BMX_ADDR(0x004c4710)
void *_rpMeshOpen(void *engine, int32_t offset)
{
    rpMeshOffset = offset;
    *(uint16_t *)((uint8_t *)RwEngineInstance + offset) = 1;
    rpMeshModuleCount++;
    return engine;
}

BMX_ADDR(0x004c4740)
void *_rpMeshClose(void *engine)
{
    rpMeshModuleCount--;
    return engine;
}

/* ---- world extension: the table at 0x69b520 and the material sort ----------------------------------- */

/* Entry 0 of the world extension's table: its attach routine, nothing to register. */
BMX_ADDR(0x004c5ab0)
int rpWorldExtAttachNothing(void)
{
    return 1;
}

/* True for the raster formats with an alpha channel (1555, 4444, 8888: format bits 0x100, 0x300, 0x500). */
BMX_ADDR(0x004c5ad0)
__attribute__((regparm(1))) int rpRasterFormatHasAlpha(uint32_t format)
{
    return format == 0x100 || format == 0x300 || format == 0x500;
}

/* Sort key of a mesh: 2 if its material is translucent (alpha format texture or colour alpha not 255). */
static uint32_t rpMaterialSortBits(const RpMaterial *material, uint32_t bits)
{
    if (material != NULL) {
        if (material->texture != NULL) {
            if (rpRasterFormatHasAlpha(RwRasterGetFormat(RwTextureGetRaster(material->texture)) & 0xf00)) {
                bits |= 2;
            }
        }
        if (material->color.alpha != 0xff) {
            bits |= 2;
        }
    }
    return bits;
}

/*
 * qsort comparator of two RpMesh pointers: orders by material, opaque ones before translucent ones (the
 *
 * order bit 2 gives), and by the material's address within the same class. 0 for the same material.
 */
BMX_ADDR(0x004c5af0)
int rpMeshCompareMaterials(const RpMesh *const *a, const RpMesh *const *b)
{
    const RpMaterial *first = (*a)->material;
    const RpMaterial *second = (*b)->material;
    if (first == second) {
        return 0;
    }
    uint32_t firstBits = (uintptr_t)second < (uintptr_t)first;
    uint32_t secondBits = (uintptr_t)first <= (uintptr_t)second;
    firstBits = rpMaterialSortBits(first, firstBits);
    secondBits = rpMaterialSortBits(second, secondBits);
    return (int)(firstBits - secondBits);
}

/* Stages of the world's render pipeline and the vertex layouts of its five mesh types (still original code).
 */
BMX_ADDR(0x004c9270)
int rpWorldPipelineInit(RwIm3DPipeline *pipeline)
{
    if (pipeline == NULL) {
        return 0;
    }
    pipeline->instance = RW_CB(RwIm3DInstanceFunc, 0x004c8e30);
    pipeline->stage2 = RW_CB(RwIm3DStageFunc, 0x004c9800);
    pipeline->stageData = RW_CB(void *, 0x004c9790);
    pipeline->stage5 = RW_CB(RwIm3DStageFunc, 0x004c9880);
    pipeline->stage6 = RW_CB(RwIm3DStageFunc, 0x004cac00);
    for (uint32_t i = 0; i <= 4; i++) {
        rpPipelineTypes[i] = (i & 4) != 0 ? 2 : 0;
        rpPipelineSizes[i] = (i & 4) != 0 ? 0x24 : 0x1c;
    }
    return 1;
}

/* The same for the pipeline of atomics (another instance stage, the tables at 0x676800 / 0x676808). */
BMX_ADDR(0x004c9730)
int rpAtomicPipelineInit(RwIm3DPipeline *pipeline)
{
    if (pipeline == NULL) {
        return 0;
    }
    pipeline->instance = RW_CB(RwIm3DInstanceFunc, 0x004c92d0);
    pipeline->stage2 = RW_CB(RwIm3DStageFunc, 0x004c9800);
    pipeline->stageData = RW_CB(void *, 0x004c9790);
    pipeline->stage5 = RW_CB(RwIm3DStageFunc, 0x004c9880);
    pipeline->stage6 = RW_CB(RwIm3DStageFunc, 0x004cac00);
    for (uint32_t i = 0; i <= 4; i++) {
        rpPipelineTypesB[i] = (i & 4) != 0 ? 2 : 0;
        rpPipelineSizesB[i] = (i & 4) != 0 ? 0x24 : 0x1c;
    }
    return 1;
}

/* ---- triangle index order ----------------------------------------------------------------------------- */

/* Zeroes `size` bytes; returns the buffer. EAX = buffer, EDX is unused, ECX = size. */
BMX_ADDR(0x004c5e10)
__attribute__((regparm(3))) void *rwZeroBytes(void *buffer, int32_t unused, uint32_t size)
{
    (void)unused;
    uint8_t *bytes = buffer;
    for (uint32_t i = 0; i < size; i++) {
        bytes[i] = 0;
    }
    return buffer;
}

/*
 * Reorders a triangle list in place so that the triangles come in passes of triangles that share no
 * vertex:
 * each pass takes, in the order of the list, every remaining triangle none of whose vertices is
 * used by a
 * triangle taken in the same pass (a bit set sized by the largest index marks the used ones),
 * until all are
 * taken. Works on a copy of the list (a taken triangle is overwritten with 0xffff); a
 * triangle of three
 * 0xffff is "taken" already (a list that contains one never ends). Leaves the list alone
 * when an allocation
 * fails. The bit set is sized (largest index + 31) / 8 bytes, one byte short for some
 * sizes (the original
 * writes into the allocation's slack then). Returns the list.
 */
BMX_ADDR(0x004c5bd0)
RwIndexList *rpMeshSeparateTriangles(RwIndexList *list)
{
    uint16_t *indices = list->indices;
    const uint32_t count = list->count;
    const uint32_t triangles = count / 3;
    uint32_t largest = 0;
    for (uint32_t i = 0; i < count; i++) {
        largest |= indices[i];
    }
    const uint32_t setBytes = (largest + 0x1f) >> 3;
    uint32_t *used = RwEngineInstance->memoryFuncs.rwmalloc(setBytes);
    if (used == NULL) {
        return list;
    }
    uint16_t *copy = RwEngineInstance->memoryFuncs.rwmalloc(count * 2);
    if (copy != NULL) {
        memcpy(copy, indices, count * 2);
        uint32_t taken = 0;
        uint16_t *out = indices;
        if (triangles != 0) {
            do {
                rwZeroBytes(used, 0, setBytes);
                for (uint16_t *tri = copy; tri < copy + triangles * 3; tri += 3) {
                    const uint32_t a = tri[0];
                    const uint32_t b = tri[1];
                    const uint32_t c = tri[2];
                    if ((a & b & c) == 0xffff) {
                        continue;
                    }
                    const uint32_t wordA = used[a >> 5];
                    const uint32_t bitA = 1u << (a & 0x1f);
                    if ((bitA & wordA) != 0 || ((1u << (b & 0x1f)) & used[b >> 5]) != 0 ||
                        ((1u << (c & 0x1f)) & used[c >> 5]) != 0) {
                        continue;
                    }
                    used[a >> 5] = wordA | bitA;
                    used[tri[1] >> 5] |= 1u << (tri[1] & 0x1f);
                    taken++;
                    used[tri[2] >> 5] |= 1u << (tri[2] & 0x1f);
                    out[0] = tri[0];
                    out[1] = tri[1];
                    out[2] = tri[2];
                    out += 3;
                    tri[0] = tri[1] = tri[2] = 0xffff;
                }
            } while (taken < triangles);
        }
        RwEngineInstance->memoryFuncs.rwfree(copy);
    }
    RwEngineInstance->memoryFuncs.rwfree(used);
    return list;
}
