/*
 * RenderWare 3.1 plugins: registration wrappers, the attach routines of the world, PVS, anim, bone, LOD
 * atomic and core plugins, and the small skin/frame/morph target helpers that live next to them.
 */
#include "rw/rwplugins.h"

#include "compat/crt.h"
#include "rw/rwmath.h"

/* Callees in other RenderWare groups (clump), still original code. */
ORIG_FN(0x004b43e0, RpMorphTarget *, RpGeometryGetMorphTarget, (void *geometry, int32_t index));
ORIG_FN(0x004b4550, int32_t, RpGeometryGetNumVertices, (const void *geometry));
ORIG_FN(0x004b45d0, ZVec3 *, RpMorphTargetGetVertices, (const RpMorphTarget *morphTarget));
ORIG_FN(0x004b4f20, void *, RpGeometryLock, (void *geometry, int32_t lockMode));
ORIG_FN(0x004b4fd0, void *, RpGeometryUnlock, (void *geometry));

/* The error record the RpWorld module (plugin id 2) reports. */
static void rpReportError(int32_t code)
{
    RwError error;
    error.pluginID = 2;
    error.errorCode = _rwerror(code);
    RwErrorSet(&error);
}

/* The exact product of two floats (x87 and SSE2 doubles both hold it without rounding). */
static double rwMul(float a, float b)
{
    return (double)a * (double)b;
}

/* ---- registration wrappers -------------------------------------------------------------------------- */

BMX_ADDR(0x004b3190)
int32_t RpAtomicRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&rpAtomicRegistry, size, pluginID, constructCB, destructCB, copyCB);
}

BMX_ADDR(0x004b31e0)
int32_t RpClumpRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&rpClumpRegistry, size, pluginID, constructCB, destructCB, copyCB);
}

BMX_ADDR(0x004b3230)
int32_t RpAtomicRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                     RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB)
{
    return _rwPluginRegistryAddPluginStream(&rpAtomicRegistry, pluginID, readCB, writeCB, getSizeCB);
}

BMX_ADDR(0x004b3270)
int32_t RpClumpRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                    RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB)
{
    return _rwPluginRegistryAddPluginStream(&rpClumpRegistry, pluginID, readCB, writeCB, getSizeCB);
}

/* Fails (-1) once the geometry module is open, or for a negative size. */
BMX_ADDR(0x004b5f60)
int32_t RpGeometryRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    if (rpGeometryModuleCount != 0) {
        rpReportError(rwERR_ALREADYOPEN);
        return -1;
    }
    if (size < 0) {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(rwERR_INVALIDSIZE, "Invalid data block size specified");
        RwErrorSet(&error);
        return -1;
    }
    return _rwPluginRegistryAddPlugin(&rpGeometryRegistry, size, pluginID, constructCB, destructCB, copyCB);
}

/* Needs all three stream callbacks, else reports a null pointer and fails (-1). */
BMX_ADDR(0x004b6020)
int32_t RpGeometryRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                       RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB)
{
    if (readCB == NULL || writeCB == NULL || getSizeCB == NULL) {
        rpReportError(rwERR_NULLP);
        return -1;
    }
    return _rwPluginRegistryAddPluginStream(&rpGeometryRegistry, pluginID, readCB, writeCB, getSizeCB);
}

/* Fails (-1) once the world module is open, or for a negative size. */
BMX_ADDR(0x004bbe40)
int32_t RpWorldRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    if (rpWorldModuleCount != 0) {
        rpReportError(rwERR_ALREADYOPEN);
        return -1;
    }
    if (size < 0) {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(rwERR_INVALIDSIZE, "Invalid data block size specified");
        RwErrorSet(&error);
        return -1;
    }
    return _rwPluginRegistryAddPlugin(&rpWorldRegistry, size, pluginID, constructCB, destructCB, copyCB);
}

BMX_ADDR(0x004bbf00)
int32_t RpWorldRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                    RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB)
{
    if (readCB == NULL || writeCB == NULL || getSizeCB == NULL) {
        rpReportError(rwERR_NULLP);
        return -1;
    }
    return _rwPluginRegistryAddPluginStream(&rpWorldRegistry, pluginID, readCB, writeCB, getSizeCB);
}

BMX_ADDR(0x004c2250)
int32_t RpWorldSectorRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                                    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&rpWorldSectorRegistry, size, pluginID, constructCB, destructCB,
                                      copyCB);
}

BMX_ADDR(0x004c22a0)
int32_t RpWorldSectorRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                          RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB)
{
    return _rwPluginRegistryAddPluginStream(&rpWorldSectorRegistry, pluginID, readCB, writeCB, getSizeCB);
}

BMX_ADDR(0x004c2600)
int32_t RpLightRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&rpLightRegistry, size, pluginID, constructCB, destructCB, copyCB);
}

BMX_ADDR(0x004cf4b0)
int32_t RwEngineRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB)
{
    return _rwPluginRegistryAddPlugin(&rwEngineRegistry, size, pluginID, constructCB, destructCB, NULL);
}

BMX_ADDR(0x004d33d0)
int32_t RwFrameRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&rwFrameRegistry, size, pluginID, constructCB, destructCB, copyCB);
}

BMX_ADDR(0x004e8f80)
int32_t RwFrameRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                    RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB)
{
    return _rwPluginRegistryAddPluginStream(&rwFrameRegistry, pluginID, readCB, writeCB, getSizeCB);
}

BMX_ADDR(0x004ded30)
int32_t RwCameraRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&rwCameraRegistry, size, pluginID, constructCB, destructCB, copyCB);
}

BMX_ADDR(0x004dffd0)
int32_t RwTextureRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                                RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&rwTextureRegistry, size, pluginID, constructCB, destructCB, copyCB);
}

/* ---- attach routines -------------------------------------------------------------------------------- */

/* A plugin of the engine registry: data size, plugin id, constructor and destructor addresses. */
typedef struct RwEnginePlugin {
    int32_t size;
    int32_t pluginID;
    uint32_t constructCB;
    uint32_t destructCB;
} RwEnginePlugin;

/*
 * Registers the core plugins on the engine registry: error slot, matrix globals, colour, frame, camera,
 * texture, raster and resource data and the rest. Returns 0 if any registration failed.
 */
BMX_ADDR(0x004cf520)
int CorePluginAttach(void)
{
    static const RwEnginePlugin first[] = {
        {8, 0x40f, 0x004dee30, 0x004dee20},    {0x10, 0x401, 0x004d3b50, 0x004d3b30},
        {0, 0x40d, 0x004eb2a0, 0x004eb2b0},    {0xc, 0x402, 0x004d2080, 0x004d2040},
        {4, 0x403, 0x004d2280, 0x004d2310},    {4, 0x404, 0x004e2350, 0x004e23b0},
        {4, 0x405, 0x004dd430, 0x004dd490},    {0x220, 0x406, 0x004e33d0, 0x004e36e0},
        {0x68, 0x407, 0x004dc9e0, 0x004dcac0}, {0x2c, 0x408, 0x004df120, 0x004df380},
        {0x38, 0x409, 0x004e83e0, 0x004e8480},
    };
    static const RwEnginePlugin last[] = {
        {4, 0x40a, 0x004dee70, 0x004deec0},
        {0x24, 0x40b, 0x004dc210, 0x004dc250},
    };
    int32_t status = 0;
    for (size_t i = 0; i < sizeof(first) / sizeof(first[0]); i++) {
        status |=
            _rwPluginRegistryAddPlugin(&rwEngineRegistry, first[i].size, first[i].pluginID,
                                       RW_CTOR(first[i].constructCB), RW_DTOR(first[i].destructCB), NULL);
    }
    status |= rwCoreExtPluginAttach();
    for (size_t i = 0; i < sizeof(last) / sizeof(last[0]); i++) {
        status |= _rwPluginRegistryAddPlugin(&rwEngineRegistry, last[i].size, last[i].pluginID,
                                             RW_CTOR(last[i].constructCB), RW_DTOR(last[i].destructCB), NULL);
    }
    return status >= 0;
}

/* The first entry of the table at 0x69b520 is the attach routine of the world extension module. */
BMX_ADDR(0x004c5ac0)
void **rpWorldExtGetFuncs(void)
{
    return RW_CB(void **, 0x0069b520);
}

BMX_ADDR(0x004c46a0)
int rpWorldExtPluginAttach(void)
{
    return ((int(BMX_CDECL *)(void))rpWorldExtGetFuncs()[0])();
}

/*
 * The plugins of the world's object types (engine, camera, atomic, clump, light, geometry and sector
 * extensions with id 0x509/0x50e) and then the world extension module. 0 if anything failed.
 */
BMX_ADDR(0x004b8e00)
int _rpWorldObjRegisterExtensions(void)
{
    int32_t status = RwEngineRegisterPlugin(8, 0x509, RW_CTOR(0x004b7570), RW_DTOR(0x004b7650));
    rpWorldCameraOffset =
        RwCameraRegisterPlugin(0x1c, 0x509, RW_CTOR(0x004b90c0), RW_DTOR(0x004b9080), RW_COPY(0x004b77a0));
    status |= rpWorldCameraOffset;
    rpWorldAtomicOffset =
        RpAtomicRegisterPlugin(8, 0x509, RW_CTOR(0x004b9040), RW_DTOR(0x004b9010), RW_COPY(0x004b7b00));
    status |= rpWorldAtomicOffset;
    rpWorldClumpOffset =
        RpClumpRegisterPlugin(4, 0x509, RW_CTOR(0x004b8ff0), RW_DTOR(0x004b8fe0), RW_COPY(0x004b7b10));
    status |= rpWorldClumpOffset;
    rpWorldLightOffset =
        RpLightRegisterPlugin(8, 0x509, RW_CTOR(0x004b8fc0), RW_DTOR(0x004b8fa0), RW_COPY(0x004b7e50));
    status |= rpWorldLightOffset;
    status |= RpGeometryRegisterPlugin(0, 0x50e, NULL, NULL, NULL);
    status |= RpWorldSectorRegisterPlugin(0, 0x50e, NULL, NULL, NULL);
    status |= RpGeometryRegisterPluginStream(0x50e, RW_READ(0x004b8d00), RW_WRITE(0x004b8cc0),
                                             RW_GETSIZE(0x004b8d40));
    status |= RpWorldSectorRegisterPluginStream(0x50e, RW_READ(0x004b8da0), RW_WRITE(0x004b8d60),
                                                RW_GETSIZE(0x004b8de0));
    if (status >= 0) {
        return rpWorldExtPluginAttach() != 0;
    }
    return 0;
}

/* Registers the world module's engine plugins (ids 0x501..0x50c), then its object extensions. */
BMX_ADDR(0x004bc1c0)
int RpWorldPluginAttach(void)
{
    static const RwEnginePlugin plugins[] = {
        {4, 0x501, 0x004b9150, 0x004b91b0},    {8, 0x502, 0x004c4710, 0x004c4740},
        {4, 0x503, 0x004b3390, 0x004b3400},    {0xc, 0x504, 0x004b0f20, 0x004b1080},
        {4, 0x505, 0x004c2310, 0x004c2380},    {0, 0x506, 0x004bc3a0, 0x004bc3b0},
        {0, 0x50c, 0x004c56b0, 0x004c56c0},    {0, 0x50a, 0x004c2020, 0x004c2030},
        {0x10, 0x507, 0x004bc090, 0x004bc020}, {0, 0x50b, 0x004c2c30, 0x004c2c40},
    };
    int32_t status = 0;
    for (size_t i = 0; i < sizeof(plugins) / sizeof(plugins[0]); i++) {
        status |= RwEngineRegisterPlugin(plugins[i].size, plugins[i].pluginID,
                                         RW_CTOR(plugins[i].constructCB), RW_DTOR(plugins[i].destructCB));
    }
    if (status >= 0) {
        return _rpWorldObjRegisterExtensions() != 0;
    }
    return 0;
}

/* Registers the potentially visible set data of worlds and world sectors (id 0x106) with streaming. */
BMX_ADDR(0x004ccac0)
int RpPVSPluginAttach(void)
{
    rpPVSWorldOffset = RpWorldRegisterPlugin(4, 0x106, RW_CTOR(0x004cc6b0), RW_DTOR(0x004cc710), NULL);
    if (rpPVSWorldOffset < 0) {
        return 0;
    }
    if (RpWorldRegisterPluginStream(0x106, RW_READ(0x004cc850), RW_WRITE(0x004cc940),
                                    RW_GETSIZE(0x004cc7b0)) < 0) {
        return 0;
    }
    rpPVSSectorOffset = RpWorldSectorRegisterPlugin(4, 0x106, RW_CTOR(0x004cc9b0), RW_DTOR(0x004cc9d0), NULL);
    if (rpPVSSectorOffset >= 0) {
        if (RpWorldSectorRegisterPluginStream(0x106, RW_READ(0x004cca30), RW_WRITE(0x004cca90),
                                              RW_GETSIZE(0x004cc9f0)) >= 0) {
            return 1;
        }
    }
    return 0;
}

/* Registers the animation plugin (id 0x108) on the engine, clumps and frames, with streaming. */
BMX_ADDR(0x00500250)
int RpAnimPluginAttach(void)
{
    if (RwEngineRegisterPlugin(4, 0x108, RW_CTOR(0x004fcd50), RW_DTOR(0x004fcda0)) < 0) {
        return 0;
    }
    rpAnimClumpOffset =
        RpClumpRegisterPlugin(0x14, 0x108, RW_CTOR(0x004fc740), RW_DTOR(0x004fc760), RW_COPY(0x004fc780));
    if (rpAnimClumpOffset < 0) {
        return 0;
    }
    rpAnimFrameOffset =
        RwFrameRegisterPlugin(0xc, 0x108, RW_CTOR(0x004fbce0), RW_DTOR(0x004fbd00), RW_COPY(0x004fbd50));
    if (rpAnimFrameOffset < 0) {
        return 0;
    }
    if (RwFrameRegisterPluginStream(0x108, RW_READ(0x004fcdd0), RW_WRITE(0x004fd280),
                                    RW_GETSIZE(0x004fd6a0)) < 0) {
        return 0;
    }
    if (RpClumpRegisterPluginStream(0x108, RW_READ(0x004fd870), RW_WRITE(0x004fd8c0),
                                    RW_GETSIZE(0x004fd920)) < 0) {
        return 0;
    }
    return 1;
}

/* Registers the bone (skin) plugin (id 0x10e) on the engine, geometries and frames. */
BMX_ADDR(0x00505870)
int RpBonePluginAttach(void)
{
    if (RwEngineRegisterPlugin(0, 0x10e, RW_CTOR(0x00505500), RW_DTOR(0x00505520)) < 0) {
        return 0;
    }
    rpBoneGeometryOffset =
        RpGeometryRegisterPlugin(4, 0x10e, RW_CTOR(0x00505530), RW_DTOR(0x00505550), RW_COPY(0x005055e0));
    if (rpBoneGeometryOffset < 0) {
        return 0;
    }
    rpBoneStreamResult = RpGeometryRegisterPluginStream(0x10e, RW_READ(0x00505670), RW_WRITE(0x005055f0),
                                                        RW_GETSIZE(0x005057b0));
    if (rpBoneStreamResult < 0) {
        return 0;
    }
    rpBoneFrameOffset =
        RwFrameRegisterPlugin(4, 0x10e, RW_CTOR(0x005057f0), RW_DTOR(0x00505810), RW_COPY(0x00505830));
    return rpBoneFrameOffset >= 0;
}

/* Registers the LOD atomic plugin (id 0x112); succeeds if its atomic data could be registered. */
BMX_ADDR(0x005062b0)
int RpLODAtomicPluginAttach(void)
{
    rpLodEngineOffset = RwEngineRegisterPlugin(0, 0x112, RW_CTOR(0x00506240), RW_DTOR(0x00506280));
    rpLodAtomicOffset =
        RpAtomicRegisterPlugin(4, 0x112, RW_CTOR(0x00506190), RW_DTOR(0x00506200), RW_COPY(0x005061b0));
    RpAtomicRegisterPluginStream(0x112, RW_READ(0x00505e60), RW_WRITE(0x00506030), RW_GETSIZE(0x00506140));
    rpLodReserved = 0;
    rpLodCamera = NULL;
    return rpLodAtomicOffset >= 0;
}

/* ---- morph targets, frames, vectors ----------------------------------------------------------------- */

/* Sets the bounding sphere of a morph target; reports and returns NULL on a closed module or null args. */
BMX_ADDR(0x004b3e90)
RpMorphTarget *RpMorphTargetSetBoundingSphere(RpMorphTarget *morphTarget, const RwSphere *sphere)
{
    if (rpGeometryModuleCount == 0) {
        rpReportError(rwERR_NOTOPEN);
        return NULL;
    }
    if (morphTarget == NULL || sphere == NULL) {
        rpReportError(rwERR_NULLP);
        return NULL;
    }
    morphTarget->boundingSphere = *sphere;
    return morphTarget;
}

/* The vertex normals of a morph target (NULL if it has none), or NULL after an error. */
BMX_ADDR(0x004b4640)
ZVec3 *RpMorphTargetGetVertexNormals(const RpMorphTarget *morphTarget)
{
    if (rpGeometryModuleCount == 0) {
        rpReportError(rwERR_NOTOPEN);
        return NULL;
    }
    if (morphTarget == NULL) {
        rpReportError(rwERR_NULLP);
        return NULL;
    }
    return morphTarget->normals;
}

/*
 * Calls callback for every object attached to the frame until it returns NULL. The next link is read
 * before the call, so the callback may detach its object.
 */
BMX_ADDR(0x004d3370)
RwFrame *RwFrameForAllObjects(RwFrame *frame, RwObjectCallBack callback, void *data)
{
    RwLLLink *link = frame->objectList.next;
    while (link != &frame->objectList) {
        RwObjectHasFrame *object = (RwObjectHasFrame *)((uint8_t *)link - offsetof(RwObjectHasFrame, lFrame));
        link = link->next;
        if (callback(object, data) == NULL) {
            break;
        }
    }
    return frame;
}

/* The anim plugin's user data word of a frame. */
BMX_ADDR(0x00500220)
uint32_t RwFrameGetUserData(const RwFrame *frame)
{
    return *(const uint32_t *)((const uint8_t *)frame + rpAnimFrameOffset + 8);
}

BMX_ADDR(0x00500230)
void RwFrameSetUserData(RwFrame *frame, uint32_t data)
{
    *(uint32_t *)((uint8_t *)frame + rpAnimFrameOffset + 8) = data;
}

/* Remembers the camera the LOD atomic plugin measures distances from. */
BMX_ADDR(0x005065d0)
void RpLODAtomicSetCamera(void *camera)
{
    rpLodCamera = camera;
}

/*
 * Length of a vector from the engine's square root table: the sum of squares is kept as a float, and its
 * bits (offset by 0x800 for rounding) index the table (mantissa bits) and are added to the halved
 * exponent. 0.0 maps to 0.0.
 */
BMX_ADDR(0x004d3810)
float RwV3dLength(const ZVec3 *vector)
{
    union {
        float f;
        uint32_t bits;
    } sum;
    sum.f =
        (float)(rwMul(vector->x, vector->x) + (rwMul(vector->z, vector->z) + rwMul(vector->y, vector->y)));
    if (sum.bits != 0) {
        const uint32_t *table = *(uint32_t **)((uint8_t *)RwEngineInstance + rwFastSqrtOffset);
        sum.bits += 0x800;
        sum.bits = table[(sum.bits & 0xffffff) >> 12] + ((sum.bits & 0x7f800000) >> 1);
    }
    return sum.f;
}

/* ---- skin helpers ----------------------------------------------------------------------------------- */

/*
 * Finds the frame of every bone block among the children of frame (by the block's key) and stores it in
 * the block. The list ends with a block of zero vertices; a null list or frame does nothing.
 */
BMX_ADDR(0x00505ad0)
void RpSkinFindBoneFrames(RpSkinBoneBlock *blocks, RwFrame *frame)
{
    if (blocks == NULL || frame == NULL || blocks->numVertices == 0) {
        return;
    }
    int32_t more;
    do {
        struct {
            void *found;
            int32_t key;
        } search = {NULL, blocks->key};
        RwFrameForAllChildren(frame, RW_CB(RwObjectCallBack, 0x00502c90), &search);
        more = blocks[1].numVertices;
        blocks->bone = search.found;
        blocks++;
    } while (more != 0);
}

/*
 * Prepares a skinned clump: finds the bone frames, and keeps a copy of the first morph target's vertices
 * (and normals) as the rest pose in the bone plugin's geometry data. Returns the clump, or NULL when it
 * has no frame, atomic or vertices, or the copy cannot be allocated. An existing rest pose copy is
 * replaced without being freed.
 */
BMX_ADDR(0x00505950)
RpClump *RpSkinClumpSetup(RpClump *clump)
{
    RwFrame *frame = clump->object.parent;
    if (frame == NULL) {
        return NULL;
    }
    RwFrameForAllChildren(frame, RW_CB(RwObjectCallBack, 0x00502ce0), frame);
    RpAtomic *atomic = NULL;
    RwFrameForAllObjects(frame, RW_CB(RwObjectCallBack, 0x00502b00), (void *)&atomic);
    void *geometry = atomic != NULL ? atomic->geometry : NULL;
    if (geometry == NULL) {
        return NULL;
    }
    RpMorphTarget *morphTarget = RpGeometryGetMorphTarget(geometry, 0);
    ZVec3 *verts = RpMorphTargetGetVertices(morphTarget);
    ZVec3 *normals = RpMorphTargetGetVertexNormals(morphTarget);
    if (verts == NULL) {
        return NULL;
    }
    const int32_t numVertices = RpGeometryGetNumVertices(geometry);
    RpSkinGeometry **slot = (RpSkinGeometry **)((uint8_t *)geometry + rpBoneGeometryOffset);
    RpSkinGeometry *skin = *slot;
    if (skin == NULL) {
        skin = RwEngineInstance->memoryFuncs.rwmalloc(sizeof(RpSkinGeometry));
        *slot = skin;
        skin->blocks = NULL;
        skin->origVerts = NULL;
        skin->origNormals = NULL;
    } else {
        RpSkinFindBoneFrames(skin->blocks, frame);
    }
    const size_t bytes = (size_t)numVertices * sizeof(ZVec3);
    skin->origVerts = RwEngineInstance->memoryFuncs.rwmalloc(bytes);
    if (skin->origVerts == NULL) {
        return NULL;
    }
    memcpy(skin->origVerts, verts, bytes);
    if (normals != NULL) {
        skin->origNormals = RwEngineInstance->memoryFuncs.rwmalloc(bytes);
        if (skin->origNormals != NULL) {
            memcpy(skin->origNormals, normals, bytes);
        }
    }
    return clump;
}

/* The data the bone frame search callback of RpSkinClumpUpdate works with. */
typedef struct RpSkinChildSearch {
    uint32_t reserved;
    RwMatrix *parentInverse;
    uint32_t zero;
} RpSkinChildSearch;

/*
 * Poses the skinned geometry of a clump: the vertices (and normals) of morph target 0 are the rest pose
 * run through the matrices of their bones, then the target's and the atomic's bounding sphere are
 * recomputed from the result. Returns 0 if the clump has no frame, atomic, vertices or cannot lock.
 */
BMX_ADDR(0x00504490)
int RpSkinClumpUpdate(RpClump *clump)
{
    RwFrame *clumpFrame = clump->object.parent;
    if (clumpFrame == NULL) {
        return 0;
    }
    RpAtomic *atomic = NULL;
    RwFrameForAllObjects(clumpFrame, RW_CB(RwObjectCallBack, 0x00502b50), (void *)&atomic);
    if (atomic == NULL) {
        return 0;
    }
    /* The inverse of the atomic's frame matrix (frame + 0x1c). The original leaves some of the local
     * matrix uninitialised; the flags it ORs with 0x20003 are zero here. */
    RwMatrix inverse = {0};
    inverse.flags |= 0x20003;
    inverse.right.x = 1.0f;
    inverse.up.y = 1.0f;
    inverse.at.z = 1.0f;
    RwMatrixInvert(&inverse, (const RwMatrix *)((const uint8_t *)atomic->object.object.parent + 0x1c));
    void *geometry = atomic->geometry;
    RpMorphTarget *morphTarget = RpGeometryGetMorphTarget(geometry, 0);
    ZVec3 *verts = RpMorphTargetGetVertices(morphTarget);
    ZVec3 *normals = RpMorphTargetGetVertexNormals(morphTarget);
    if (verts == NULL) {
        return 0;
    }
    RpSkinChildSearch search = {0, &inverse, 0};
    RwFrameForAllChildren(clumpFrame, RW_CB(RwObjectCallBack, 0x00504340), &search);
    if (RpGeometryLock(geometry, 6) == NULL) {
        return 0;
    }
    const RpSkinGeometry *skin = *(RpSkinGeometry **)((uint8_t *)geometry + rpBoneGeometryOffset);
    ZVec3 *origVerts = skin->origVerts;
    ZVec3 *origNormals = skin->origNormals;
    const RpSkinBoneBlock *block = skin->blocks;
    if (normals == NULL || origNormals == NULL) {
        if (block != NULL && block->numVertices != 0) {
            int32_t more;
            do {
                if ((block->bone->flags & 2) != 0) {
                    RwV3dTransformPoints(verts + block->firstVertex, origVerts + block->firstVertex,
                                         block->numVertices, &block->bone->matrix);
                }
                more = block[1].numVertices;
                block++;
            } while (more != 0);
        }
    } else if (block != NULL && block->numVertices != 0) {
        int32_t more;
        do {
            if (block->bone != NULL && (block->bone->flags & 2) != 0) {
                RwV3dTransformPoints(verts + block->firstVertex, origVerts + block->firstVertex,
                                     block->numVertices, &block->bone->matrix);
                RwV3dTransformVectors(normals + block->firstVertex, origNormals + block->firstVertex,
                                      block->numVertices, &block->bone->matrix);
            }
            more = block[1].numVertices;
            block++;
        } while (more != 0);
    }
    const int32_t numVertices = RpGeometryGetNumVertices(geometry);
    RwBBox bbox;
    RwBBoxCalculate(&bbox, verts, numVertices);
    /* Half extents as floats (each step is stored to memory in the original), the centre and radius. */
    ZVec3 half;
    const float extentX = (float)((double)bbox.sup.x - (double)bbox.inf.x);
    half.x = (float)(0.5 * (double)extentX);
    const float extentY = (float)((double)bbox.sup.y - (double)bbox.inf.y);
    half.y = (float)(0.5 * (double)extentY);
    const float extentZ = (float)((double)bbox.sup.z - (double)bbox.inf.z);
    half.z = (float)(0.5 * (double)extentZ);
    RwSphere sphere;
    sphere.radius = RwV3dLength(&half);
    sphere.center.x = (float)((double)half.x + (double)bbox.inf.x);
    sphere.center.y = (float)((double)half.y + (double)bbox.inf.y);
    sphere.center.z = (float)((double)half.z + (double)bbox.inf.z);
    RpMorphTargetSetBoundingSphere(morphTarget, &sphere);
    atomic->boundingSphere = sphere;
    atomic->object.object.privateFlags |= 1;
    RpGeometryUnlock(geometry);
    return 1;
}

/* ---- RwMatrixInvert helpers ------------------------------------------------------------------------- */

/*
 * Inverse of a matrix with orthonormal rotation: the transpose, and the position as -(R^T p). Marks the
 * result orthonormal (flags | 3), clears the identity bit and bumps the version byte.
 */
BMX_ADDR(0x004d10e0)
__attribute__((regparm(2))) RwMatrix *RwMatrixInvertOrthonormal(RwMatrix *out, const RwMatrix *in)
{
    out->right.x = in->right.x;
    out->right.y = in->up.x;
    out->right.z = in->at.x;
    out->up.x = in->right.y;
    out->up.y = in->up.y;
    out->up.z = in->at.y;
    out->at.x = in->right.z;
    out->at.y = in->up.z;
    out->at.z = in->at.z;
    const uint32_t flags = (out->flags | 3) & 0xfffdffff;
    const uint8_t version = (uint8_t)(((uint8_t *)&out->pad1)[0] + 1);
    out->pos.x = (float)-((rwMul(in->pos.z, in->right.z) + rwMul(in->pos.y, in->right.y)) +
                          rwMul(in->pos.x, in->right.x));
    out->pos.y =
        (float)-((rwMul(in->pos.z, in->up.z) + rwMul(in->pos.y, in->up.y)) + rwMul(in->pos.x, in->up.x));
    out->flags = flags;
    ((uint8_t *)&out->pad1)[0] = version;
    out->pos.z =
        (float)-((rwMul(in->pos.z, in->at.z) + rwMul(in->pos.y, in->at.y)) + rwMul(in->pos.x, in->at.x));
    return out;
}

/*
 * General inverse by cofactors: the adjugate stored as floats, scaled by 1/determinant (in double
 * precision) unless the determinant is 0 or NaN, then the position as -(M^-1 p). Clears the identity bit
 * and bumps the version byte. Every output element is read back from memory where the original does.
 */
BMX_ADDR(0x004d1190)
__attribute__((regparm(2))) RwMatrix *RwMatrixInvertGeneral(RwMatrix *out, const RwMatrix *in)
{
    out->right.x = (float)(rwMul(in->at.z, in->up.y) - rwMul(in->at.y, in->up.z));
    out->up.x = (float)(rwMul(in->at.x, in->up.z) - rwMul(in->at.z, in->up.x));
    out->at.x = (float)(rwMul(in->at.y, in->up.x) - rwMul(in->at.x, in->up.y));
    out->right.y = (float)(rwMul(in->at.y, in->right.z) - rwMul(in->at.z, in->right.y));
    out->up.y = (float)(rwMul(in->at.z, in->right.x) - rwMul(in->at.x, in->right.z));
    out->at.y = (float)(rwMul(in->at.x, in->right.y) - rwMul(in->at.y, in->right.x));
    out->right.z = (float)(rwMul(in->up.z, in->right.y) - rwMul(in->up.y, in->right.z));
    out->up.z = (float)(rwMul(in->up.x, in->right.z) - rwMul(in->up.z, in->right.x));
    out->at.z = (float)(rwMul(in->up.y, in->right.x) - rwMul(in->up.x, in->right.y));
    const double determinant =
        rwMul(in->right.x, out->right.x) + (rwMul(out->at.x, in->right.z) + rwMul(out->up.x, in->right.y));
    if (0.0 < determinant || 0.0 > determinant) {
        const double scale = 1.0 / determinant;
        out->right.y = (float)((double)out->right.y * scale);
        out->right.z = (float)((double)out->right.z * scale);
        out->up.y = (float)((double)out->up.y * scale);
        out->right.x = (float)((double)out->right.x * scale);
        out->up.x = (float)((double)out->up.x * scale);
        out->at.x = (float)((double)out->at.x * scale);
        out->at.y = (float)((double)out->at.y * scale);
        out->at.z = (float)((double)out->at.z * scale);
        out->up.z = (float)((double)out->up.z * scale);
    }
    const uint32_t flags = out->flags & 0xfffdffff;
    const uint8_t version = (uint8_t)(((uint8_t *)&out->pad1)[0] + 1);
    out->pos.x = (float)-((rwMul(in->pos.z, out->at.x) + rwMul(in->pos.y, out->up.x)) +
                          rwMul(in->pos.x, out->right.x));
    out->pos.y = (float)-((rwMul(out->at.y, in->pos.z) + rwMul(out->up.y, in->pos.y)) +
                          rwMul(out->right.y, in->pos.x));
    out->flags = flags;
    ((uint8_t *)&out->pad1)[0] = version;
    out->pos.z = (float)-(rwMul(out->right.z, in->pos.x) +
                          (rwMul(out->at.z, in->pos.z) + rwMul(out->up.z, in->pos.y)));
    return out;
}
