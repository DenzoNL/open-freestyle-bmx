/*
 * RenderWare 3.1 plugins: registration wrappers, the attach routines of the world, PVS, anim, bone, LOD
 * atomic and core plugins, and the small skin/frame/morph target helpers that live next to them.
 */
#include "rw/rwplugins.h"

#include "compat/crt.h"
#include "compat/x87.h"
#include "rw/rwanim.h"
#include "rw/rwclump.h"
#include "rw/rwmath.h"

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

/* ---- bone plugin: callbacks and frame data ------------------------------------------------------------ */

/* The first atomic of a frame (frame object walk callback): stores it in *found and stops. */
BMX_ADDR(0x00502b00)
void *rpBoneFindAtomic(void *object, void *found)
{
    if (*(uint8_t *)object == 1) {
        *(void **)found = object;
        return NULL;
    }
    return object;
}

/* The first atomic whose geometry has bone plugin data. */
BMX_ADDR(0x00502b50)
void *rpBoneFindSkinAtomic(void *object, void *found)
{
    if (*(uint8_t *)object == 1) {
        const RpAtomic *atomic = object;
        if (*(void **)((uint8_t *)atomic->geometry + rpBoneGeometryOffset) != NULL) {
            *(void **)found = object;
            return NULL;
        }
    }
    return object;
}

/* The bone data of a frame (EAX = frame); NULL without a frame. */
BMX_ADDR(0x00502ba0)
__attribute__((regparm(1))) RpSkinBone *rpBoneGet(RwFrame *frame)
{
    if (frame == NULL) {
        return NULL;
    }
    return *(RpSkinBone **)((uint8_t *)frame + rpBoneFrameOffset);
}

/* Stores the bone data in a frame (EAX = frame, EDX = bone); the frame, NULL without one. */
BMX_ADDR(0x00502b80)
__attribute__((regparm(2))) RwFrame *rpBoneSet(RwFrame *frame, RpSkinBone *bone)
{
    if (frame == NULL) {
        return NULL;
    }
    *(RpSkinBone **)((uint8_t *)frame + rpBoneFrameOffset) = bone;
    return frame;
}

/* A new bone: two identity matrices (the rest pose inverse and the pose; the flag words are only or-ed, the
 * padding words are not written), no frame, flags 5. NULL when out of memory. */
BMX_ADDR(0x00502bc0)
RpSkinBone *rpBoneCreate(void)
{
    uint32_t *p = RwEngineInstance->memoryFuncs.rwmalloc(sizeof(RpSkinBone));
    if (p == NULL) {
        return NULL;
    }
    p[4] |= 0x20003;
    p[0x14] |= 0x20003;
    p[0xb] = p[6] = p[1] = p[0x1b] = p[0x16] = p[0x11] = 0x3f800000;
    p[5] = p[3] = p[2] = p[10] = p[9] = p[7] = p[0xf] = p[0xe] = p[0xd] = 0;
    p[0x15] = p[0x13] = p[0x12] = p[0x1a] = p[0x19] = p[0x17] = p[0x1f] = p[0x1e] = p[0x1d] = 0;
    p[0] = 0;
    p[0x21] = 5;
    return (RpSkinBone *)p;
}

/* Frees a bone (EAX = bone) and clears the frame's pointer to it. */
BMX_ADDR(0x00502c60)
__attribute__((regparm(1))) void rpBoneDestroy(RpSkinBone *bone)
{
    if (bone != NULL) {
        RwFrame *frame = bone->frame;
        RwEngineInstance->memoryFuncs.rwfree(bone);
        if (frame != NULL) {
            *(RpSkinBone **)((uint8_t *)frame + rpBoneFrameOffset) = NULL;
        }
    }
}

/* Frame walk callback: finds the frame whose user data is the key (search[1]); search[0] gets its bone. */
BMX_ADDR(0x00502c90)
void *rpBoneFindFrame(void *frame, void *search)
{
    RwFrame *f = frame;
    void **data = (void **)search;
    if (data != NULL && RwFrameGetUserData(f) == (uint32_t)(uintptr_t)data[1]) {
        data[0] = rpBoneGet(f);
        return NULL;
    }
    RwFrameForAllChildren(f, BMX_FNPTR(RwObjectCallBack, 0x00502c90, rpBoneFindFrame), search);
    return frame;
}

/* Gives every frame below this one that has an animation key (user data other than -1) a bone with the
 * inverse of its rest pose relative to the reference frame (walks all children, whether or not it did). */
BMX_ADDR(0x00502ce0)
void *rpBoneAttachFrames(void *frame, void *reference)
{
    RwFrame *f = frame;
    if (f != NULL && reference != NULL) {
        if (rpBoneGet(f) == NULL && RwFrameGetUserData(f) != 0xffffffffU) {
            RpSkinBone *bone = rpBoneCreate();
            if (bone != NULL) {
                bone->frame = f;
                rpBoneSet(f, bone);
            }
            RwMatrixInvert(&bone->invRest, RwFrameGetLTM(f));
            RwMatrixTransform(&bone->invRest, RwFrameGetMatrix(reference), rwCOMBINEPRECONCAT);
        }
        RwFrameForAllChildren(f, BMX_FNPTR(RwObjectCallBack, 0x00502ce0, rpBoneAttachFrames), reference);
    }
    return frame;
}

/* Engine module of the plugin: remembers its offset and counts the opens. */
BMX_ADDR(0x00505500)
void *rpBoneEngineOpen(void *engine, int32_t offset)
{
    rpBoneModuleCount++;
    rpBoneEngineOffset = offset;
    return engine;
}

BMX_ADDR(0x00505520)
void *rpBoneEngineClose(void *engine)
{
    rpBoneModuleCount--;
    return engine;
}

/* Geometry data: a pointer to the blocks and rest pose (RpSkinGeometry), empty at first, freed with the
 * geometry; copies share nothing (the copy callback only returns the destination). */
BMX_ADDR(0x00505530)
void *rpBoneGeometryInit(void *geometry)
{
    *(RpSkinGeometry **)((uint8_t *)geometry + rpBoneGeometryOffset) = NULL;
    return geometry;
}

BMX_ADDR(0x00505550)
void *rpBoneGeometryDeinit(void *geometry)
{
    RpSkinGeometry **slot = (RpSkinGeometry **)((uint8_t *)geometry + rpBoneGeometryOffset);
    RpSkinGeometry *skin = *slot;
    if (skin != NULL) {
        if (skin->blocks != NULL) {
            RwEngineInstance->memoryFuncs.rwfree(skin->blocks);
            skin->blocks = NULL;
        }
        if (skin->origVerts != NULL) {
            RwEngineInstance->memoryFuncs.rwfree(skin->origVerts);
            skin->origVerts = NULL;
        }
        if (skin->origNormals != NULL) {
            RwEngineInstance->memoryFuncs.rwfree(skin->origNormals);
            skin->origNormals = NULL;
        }
        *(RpSkinGeometry **)((uint8_t *)geometry + rpBoneGeometryOffset) = NULL;
        RwEngineInstance->memoryFuncs.rwfree(skin);
    }
    return geometry;
}

BMX_ADDR(0x005055e0)
void *rpBoneGeometryCopy(void *destination)
{
    return destination;
}

/* Writes the bone blocks, each as 3 words (vertex range and key), up to and including the terminating block;
 * NULL when a write fails. */
BMX_ADDR(0x005055f0)
void *rpBoneGeometryWrite(void *stream, int32_t length, const void *geometry)
{
    (void)length;
    const RpSkinGeometry *skin = *(const RpSkinGeometry **)((const uint8_t *)geometry + rpBoneGeometryOffset);
    const RpSkinBoneBlock *block = skin->blocks;
    if (block != NULL) {
        while (block->numVertices != 0) {
            if (RwStreamWriteInt32(stream, block, 0xc) == NULL) {
                return NULL;
            }
            block++;
        }
        if (RwStreamWriteInt32(stream, block, 0xc) == NULL) {
            return NULL;
        }
    }
    return stream;
}

/* Stream size of the bone blocks (with the terminating one); 0 without blocks. */
BMX_ADDR(0x005057b0)
int32_t rpBoneGeometrySize(const void *geometry)
{
    const RpSkinGeometry *skin = *(const RpSkinGeometry **)((const uint8_t *)geometry + rpBoneGeometryOffset);
    if (skin == NULL || skin->blocks == NULL) {
        return 0;
    }
    int32_t count = 0;
    for (const RpSkinBoneBlock *block = skin->blocks; block->numVertices != 0; block++) {
        count++;
    }
    return count * 12 + 12;
}

/* Frame data: the bone pointer, empty at first; a frame's bone goes with it; copying copies the bone. */
BMX_ADDR(0x005057f0)
void *rpBoneFrameInit(void *frame)
{
    *(RpSkinBone **)((uint8_t *)frame + rpBoneFrameOffset) = NULL;
    return frame;
}

BMX_ADDR(0x00505810)
void *rpBoneFrameDeinit(void *frame)
{
    RpSkinBone *bone = *(RpSkinBone **)((uint8_t *)frame + rpBoneFrameOffset);
    if (bone != NULL) {
        rpBoneDestroy(bone);
    }
    return frame;
}

BMX_ADDR(0x00505830)
void *rpBoneFrameCopy(void *destination, const void *source)
{
    RpSkinBone *to = *(RpSkinBone **)((uint8_t *)destination + rpBoneFrameOffset);
    const RpSkinBone *from = *(const RpSkinBone *const *)((const uint8_t *)source + rpBoneFrameOffset);
    if (to != NULL && from != NULL) {
        __builtin_memcpy(to, from, sizeof(RpSkinBone));
        to->frame = destination;
    }
    return destination;
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

/* ---- bone plugin: matrices and the geometry chunk ------------------------------------------------------ */

/*
 * Frame walk of RpSkinClumpUpdate: the bone of a frame that is synced and has user data (not -1) gets the
 * matrix inverse rest pose x LTM x the walk's matrix (flags: bit 0 cleared, bit 1 set), otherwise both flag
 * bits are cleared; then the children. Frames without a bone pass the walk on unchanged.
 */
BMX_ADDR(0x00504340)
void *rpBoneUpdateMatrices(void *frame, void *data)
{
    const RpSkinChildSearch *walk = data;
    RpSkinBone *bone = rpBoneGet(frame);
    if (bone == NULL) {
        RwFrameForAllChildren(frame, RW_CB(RwObjectCallBack, 0x00504340), data);
        return frame;
    }
    if (RwFrameDirty(frame) == 0 || RwFrameGetUserData(frame) == 0xffffffffU) {
        bone->flags &= ~3U;
    } else {
        bone->matrix = bone->invRest;
        RwMatrixTransform(&bone->matrix, RwFrameGetLTM(frame), rwCOMBINEPOSTCONCAT);
        RwMatrixTransform(&bone->matrix, walk->parentInverse, rwCOMBINEPOSTCONCAT);
        RwFrameUpdateObjects(frame);
        bone->flags = (bone->flags & ~1U) | 2;
    }
    RpSkinChildSearch child;
    child.reserved = 0;
    child.parentInverse = walk->parentInverse;
    child.zero = bone->flags;
    RwFrameForAllChildren(frame, RW_CB(RwObjectCallBack, 0x00504340), &child);
    return frame;
}

/*
 * Reads the bone plugin's geometry chunk: blocks of three words (first vertex, count, frame key) until the
 * chunk length is used up, each widened by an empty bone pointer, and the skin data that holds them (no rest
 * pose yet). The stream, NULL on a failed read (the blocks leak) or without memory (error).
 */
BMX_ADDR(0x00505670)
void *rpBoneGeometryRead(void *stream, int32_t length, void *geometry)
{
    const int32_t size = length / 3 * 4;
    RpSkinBoneBlock *blocks = RwEngineInstance->memoryFuncs.rwmalloc((size_t)size);
    if (blocks == NULL) {
        RwError error;
        error.pluginID = 0x10e;
        error.errorCode = _rwerror(rwERR_NOMEM, size);
        RwErrorSet(&error);
        return NULL;
    }
    RpSkinBoneBlock *block = blocks;
    for (int32_t read = 0; read < length; read += 12, block++) {
        if (RwStreamReadInt32(stream, block, 12) == NULL) {
            return NULL;
        }
        block->bone = NULL;
    }
    RpSkinGeometry *skin = RwEngineInstance->memoryFuncs.rwmalloc(sizeof *skin);
    if (skin == NULL) {
        RwEngineInstance->memoryFuncs.rwfree(blocks);
        RwError error;
        error.pluginID = 0x10e;
        error.errorCode = _rwerror(rwERR_NOMEM, (int32_t)sizeof *skin);
        RwErrorSet(&error);
        return NULL;
    }
    *(RpSkinGeometry **)((uint8_t *)geometry + rpBoneGeometryOffset) = skin;
    skin->blocks = blocks;
    skin->origNormals = NULL;
    skin->origVerts = NULL;
    return stream;
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

/* ---- animation plugin: engine module ---------------------------------------------------------------- */

/* Engine part of the animation plugin: the free list (8 byte entries) is shared by all engine instances. */
BMX_ADDR(0x004fcd50)
void *WorldObjectOpen(void *engine, int32_t offset)
{
    rpAnimModuleCount++;
    rpAnimEngineOffset = offset;
    if (rpAnimModuleCount == 1) {
        *(RwFreeList **)((uint8_t *)RwEngineInstance + rpAnimEngineOffset) = RwFreeListCreate(8, 0x32, 0);
    }
    return engine;
}

BMX_ADDR(0x004fcda0)
void *WorldObjectClose(void *engine)
{
    rpAnimModuleCount--;
    if (rpAnimModuleCount == 0) {
        RwFreeListDestroy(*(RwFreeList **)((uint8_t *)RwEngineInstance + rpAnimEngineOffset));
    }
    return engine;
}

/* ---- animation plugin: sequences of a clump --------------------------------------------------------- */

/* The interpolator state of an animation node starts empty: state 0 and no instance blocks (+0x2c, +0x30). */
BMX_ADDR(0x00502a30)
void *rpAnimInterpolatorInit(void *interpolator)
{
    uint32_t *words = interpolator;
    words[0] = 0;
    words[0xc] = 0;
    words[0xb] = 0;
    return interpolator;
}

/* A new node: no frame, empty interpolator state, last in its list. NULL when out of memory. */
BMX_ADDR(0x004fc4f0)
RpAnimNode *rpAnimNodeCreate(void)
{
    RpAnimNode *node = RwEngineInstance->memoryFuncs.rwmalloc(sizeof(RpAnimNode));
    if (node == NULL) {
        return NULL;
    }
    node->frame = NULL;
    rpAnimInterpolatorInit(&node->state);
    node->next = NULL;
    return node;
}

/* Frame constructor: no sequences, user data -1. */
BMX_ADDR(0x004fbce0)
void *rpAnimFrameInit(void *frame)
{
    RpAnimFramePlugin *plugin = (RpAnimFramePlugin *)((uint8_t *)frame + rpAnimFrameOffset);
    plugin->priority = 0;
    plugin->data = NULL;
    plugin->userData = 0xffffffffU;
    return frame;
}

/* An empty frame sequence data block (0x10 bytes, all zero); NULL when out of memory. */
BMX_ADDR(0x004fbca0)
struct RpAnimFrameData *rpAnimFrameDataCreate(void)
{
    uint32_t *words = RwEngineInstance->memoryFuncs.rwmalloc(0x10);
    if (words == NULL) {
        return NULL;
    }
    words[3] = 0;
    words[2] = 0;
    words[1] = 0;
    words[0] = 0;
    return (struct RpAnimFrameData *)words;
}

/* The name and the type (1: keys of 0xc bytes, 2: keys of 0x10 bytes) of a frame sequence. */
BMX_ADDR(0x00500044)
__attribute__((regparm(1))) const char *rpAnimSequenceName(const void *sequence)
{
    return ((const RpAnimFrameSequence *)sequence)->name;
}

BMX_ADDR(0x00500049)
uint32_t rpAnimSequenceType(const uint32_t *sequence)
{
    return sequence[2];
}

/* Frame-sequence chunk size: the name (with its length word and terminator), the 0x11 byte header, the
 * keys of the type's size and 12 bytes per further entry (EAX = sequence). */
BMX_ADDR(0x004fd710)
__attribute__((regparm(1))) int32_t rpAnimSequenceSize(const uint32_t *sequence)
{
    _Static_assert(offsetof(RwGlobals, stringFuncs) + offsetof(RwStringFunctions, rwstrlen) == 0x114,
                   "rwstrlen offset");
    const int32_t length = ((int32_t(BMX_CDECL *)(const char *))RwEngineInstance->stringFuncs.rwstrlen)(
        (const char *)(uintptr_t)sequence[1]);
    const int32_t type = (int32_t)sequence[2];
    const int32_t count = (int32_t)sequence[3];
    int32_t size = length + 0x11;
    if (type == 1) {
        size = count * 12 + 0x11 + length;
    } else if (type == 2) {
        size = count * 16 + 0x11 + length;
    }
    return (int32_t)sequence[5] * 12 + size;
}

/* Stream size of a frame's animation chunk: 8 without data, else 0x14 and the sizes of all its sequences
 * (list A, then list B). */
BMX_ADDR(0x004fd6a0)
int32_t rpAnimFrameChunkSize(const void *frame)
{
    const RpAnimFrameData *data =
        *(const RpAnimFrameData *const *)((const uint8_t *)frame + rpAnimFrameOffset);
    if (data == NULL) {
        return 8;
    }
    int32_t size = 0x14;
    for (const RpAnimFrameLink *link = data->listA; link != NULL; link = link->next) {
        size += rpAnimSequenceSize((const uint32_t *)link->sequence);
    }
    for (const RpAnimFrameLink *link = data->listB; link != NULL; link = link->next) {
        size += rpAnimSequenceSize((const uint32_t *)link->sequence);
    }
    return size;
}

/* Adds the frame's sequence to the clump by name, as a sequence without play callback; the sequence. */
BMX_ADDR(0x004fd770)
void *rpAnimClumpAddFrameSequence(void *sequence, void *clump)
{
    const char *name = rpAnimSequenceName(sequence);
    rpAnimClumpAddSequence(clump, name, NULL, 0, 0, 0, NULL);
    return sequence;
}

/*
 * Adds the sequences of a frame to the clump (EAX = frame, EDX = clump): list B first, then list A (a list
 * ends early at a link without a sequence, and a list B that does skips list A), then the same for the
 * children. The entry at 0x4fd7b0 (a stub that loads the stack arguments and falls into this) is the
 * walk callback.
 */
BMX_ADDR(0x004fd7b8)
__attribute__((regparm(2))) void *rpAnimClumpAddFrameSequences(void *frame, void *clump)
{
    const RpAnimFrameData *data =
        *(const RpAnimFrameData *const *)((const uint8_t *)frame + rpAnimFrameOffset);
    if (data != NULL) {
        const RpAnimFrameLink *link = data->listB;
        int stopped = 0;
        while (link != NULL) {
            const RpAnimFrameLink *next = link->next;
            if (rpAnimClumpAddFrameSequence(link->sequence, clump) == NULL) {
                stopped = 1;
                break;
            }
            link = next;
        }
        if (stopped == 0) {
            link = data->listA;
            while (link != NULL) {
                const RpAnimFrameLink *next = link->next;
                if (rpAnimClumpAddFrameSequence(link->sequence, clump) == NULL) {
                    break;
                }
                link = next;
            }
        }
    }
    RwFrameForAllChildren(
        frame, BMX_FNPTR(RwObjectCallBack, 0x004fd7b0, rpAnimClumpAddFrameSequencesCallback), clump);
    return frame;
}

BMX_ADDR(0x004fd7b0)
void *rpAnimClumpAddFrameSequencesCallback(void *frame, void *clump)
{
    return rpAnimClumpAddFrameSequences(frame, clump);
}

/* Clump ext: adds the sequences of all frames below the clump's root frame to it (EAX = clump). */
BMX_ADDR(0x004fd840)
__attribute__((regparm(1))) RpClump *rpAnimClumpAddAllSequences(RpClump *clump)
{
    if (clump == NULL) {
        return NULL;
    }
    RwFrame *frame = clump->object.parent;
    if (frame != NULL) {
        rpAnimClumpAddFrameSequences(frame, clump);
    }
    return clump;
}

/* Reads the clump chunk: a flag word, when set the sequences of the frames are added to the clump. */
BMX_ADDR(0x004fd870)
void *rpAnimClumpChunkRead(void *stream, int32_t length, RpClump *clump)
{
    (void)length;
    uint32_t flag;
    if (RwStreamReadInt32(stream, &flag, 4) == NULL) {
        return NULL;
    }
    if (flag != 0) {
        rpAnimClumpAddAllSequences(clump);
    }
    return stream;
}

/* Writes the clump chunk: 1 when the clump has sequences, else 0. NULL when the write fails. */
BMX_ADDR(0x004fd8c0)
void *rpAnimClumpChunkWrite(void *stream, int32_t length, const void *clump)
{
    (void)length;
    uint32_t flag = 0;
    if ((uintptr_t)clump + (uintptr_t)rpAnimClumpOffset != 0 &&
        *(const int32_t *)((const uint8_t *)clump + rpAnimClumpOffset) > 0) {
        flag = 1;
    }
    if (RwStreamWriteInt32(stream, &flag, 4) == NULL) {
        return NULL;
    }
    return stream;
}

/* Stream size of the clump chunk: 4, or -1 when there is no sequence (no chunk). */
BMX_ADDR(0x004fd920)
int32_t rpAnimClumpChunkSize(const void *clump)
{
    if ((uintptr_t)clump + (uintptr_t)rpAnimClumpOffset != 0 &&
        *(const int32_t *)((const uint8_t *)clump + rpAnimClumpOffset) > 0) {
        return 4;
    }
    return -1;
}

/* Copies a clump's animation sequences to another: those of both lists with their name, play callback and
 * data (the three unused argument slots of the add routine hold whatever the stack had). */
BMX_ADDR(0x004fc780)
void *rpAnimClumpCopy(void *destination, const void *source)
{
    const RpAnimClumpData *data = (const RpAnimClumpData *)((const uint8_t *)source + rpAnimClumpOffset);
    for (const RpAnimClumpSequence *s = data->sequences->next; s != NULL; s = s->next) {
        rpAnimClumpAddSequence(destination, s->name, s->callBack, 0, 0, 0, s->data);
    }
    for (const RpAnimClumpSequence *s = data->others->next; s != NULL; s = s->next) {
        rpAnimClumpAddSequence(destination, s->name, s->callBack, 0, 0, 0, s->data);
    }
    return destination;
}

/* Frees the two blocks an animation interpolator owns (at +0x2c and +0x30); returns the interpolator. */
BMX_ADDR(0x005029e0)
void *rpAnimInterpolatorDeinit(void *interpolator)
{
    void **blocks = (void **)((uint8_t *)interpolator + 0x2c);
    if (blocks[0] != NULL) {
        RwEngineInstance->memoryFuncs.rwfree(blocks[0]);
        blocks[0] = NULL;
    }
    if (blocks[1] != NULL) {
        RwEngineInstance->memoryFuncs.rwfree(blocks[1]);
        blocks[1] = NULL;
    }
    return interpolator;
}

#define CB_ANIM_FIND_NODES   RW_CB(RwObjectCallBack, 0x004fc540)
#define CB_ANIM_DEFAULT_PLAY 0x00502990
#define rpAnimDurationStart  ORIG_VAR(0x0069c63c, float, rpAnimDurationStart)
#define rpAnimSumStartA      ORIG_VAR(0x0069c640, float, rpAnimSumStartA)
#define rpAnimSumStartB      ORIG_VAR(0x0069c644, float, rpAnimSumStartB)

static int animStrcmp(const char *a, const char *b)
{
    return ((int(BMX_CDECL *)(const char *, const char *))RwEngineInstance->stringFuncs.rwstrcmp)(a, b);
}

static void *animMalloc(size_t size)
{
    return RwEngineInstance->memoryFuncs.rwmalloc(size);
}

static RpAnimFrameSequence *animFindSequence(const RpAnimFrameLink *link, const char *name)
{
    for (; link != NULL; link = link->next) {
        if (animStrcmp(link->sequence->name, name) == 0) {
            return link->sequence;
        }
    }
    return NULL;
}

/* An instance of the sequence for a node (NULL without memory). */
static RpAnimInstance *animInstance(RpAnimFrameSequence *sequence)
{
    RpAnimInstance *instance = animMalloc(sizeof *instance);
    if (instance == NULL) {
        return NULL;
    }
    instance->time = 0.0F;
    instance->keys = NULL;
    instance->sequence = NULL;
    instance->sequence = sequence;
    instance->time = 0.0F;
    instance->keys = sequence->keys;
    return instance;
}

static void animNodeFree(RpAnimNode *node)
{
    rpAnimInterpolatorDeinit((uint8_t *)node + offsetof(RpAnimNode, state));
    RwEngineInstance->memoryFuncs.rwfree(node);
}

/* max(start, the sums of key times (from sumStart) of the matching sequences in the list), in double. */
static double animLongest(const RpAnimFrameLink *link, const char *name, double longest, float sumStart)
{
    for (; link != NULL; link = link->next) {
        if (animStrcmp(link->sequence->name, name) != 0) {
            continue;
        }
        double sum = sumStart;
        const RpAnimFrameSequence *sequence = link->sequence;
        for (int32_t i = 0; i < sequence->numKeys; i++) {
            float time;
            memcpy(&time, sequence->keys + i * 0xc + 8, sizeof time);
            sum += time;
        }
        if (sum > longest) {
            longest = sum;
        }
    }
    return longest;
}

/*
 * Adds the sequence `name` to the clump (WorldCopyClumpExt copies a clump's sequences with it): fails when
 * the clump has it already or has no frame. Collects a node for the clump's frame and (0x4fc540) every frame
 * below it that has a sequence of that name, then puts a new clump sequence after the head of the clump's
 * list (callBack NULL: the default player), its duration the longest sum of key times. NULL on failure
 * (without memory the nodes and the sequence leak, as in the original).
 */
BMX_ADDR(0x004fc7fc)
__attribute__((regparm(3))) RpClump *rpAnimClumpAddSequence(RpClump *clump, const char *name, void *callBack,
                                                            uint32_t unused0, uint32_t unused1,
                                                            uint32_t unused2, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    RpAnimClumpData *anim = (RpAnimClumpData *)((uint8_t *)clump + rpAnimClumpOffset);
    for (const RpAnimClumpSequence *s = anim->others; s != NULL; s = s->next) {
        if (animStrcmp(s->name, name) == 0) {
            return NULL;
        }
    }
    for (const RpAnimClumpSequence *s = anim->sequences; s != NULL; s = s->next) {
        if (animStrcmp(s->name, name) == 0) {
            return NULL;
        }
    }
    RwFrame *frame = clump->object.parent;
    if (frame == NULL) {
        return NULL;
    }
    if (anim->frameList.frames == NULL && _rwFrameListInitialize(&anim->frameList, frame) == NULL) {
        return NULL;
    }
    RpAnimNodeSearch search;
    search.name = name;
    search.nodes = NULL;
    const RpAnimFrameData *frameData = *(RpAnimFrameData **)((uint8_t *)frame + rpAnimFrameOffset);
    int walk = 1;
    if (frameData != NULL) {
        RpAnimFrameSequence *a = animFindSequence(frameData->listA, name);
        RpAnimFrameSequence *b = animFindSequence(frameData->listB, name);
        if (a != NULL || b != NULL) {
            RpAnimNode *node = rpAnimNodeCreate();
            walk = 0;
            if (node != NULL) {
                node->frame = frame;
                int ok = 1;
                if (a != NULL) {
                    node->a = animInstance(a);
                    ok = node->a != NULL;
                }
                if (ok && b != NULL) {
                    node->b = animInstance(b);
                    ok = node->b != NULL;
                    if (ok) {
                        node->state = 2;
                    }
                }
                if (ok) {
                    node->state = 3;
                    node->next = search.nodes;
                    search.nodes = node;
                    node->frame = frame;
                    walk = 1;
                } else {
                    animNodeFree(node);
                }
            }
        }
    }
    if (walk) {
        RwFrameForAllChildren(frame, CB_ANIM_FIND_NODES, &search);
    }
    if (search.nodes == NULL) {
        return NULL;
    }
    RpAnimClumpSequence *sequence = animMalloc(sizeof *sequence);
    if (sequence != NULL) {
        char *copy = NULL;
        if (name != NULL) {
            const size_t length =
                ((size_t(BMX_CDECL *)(const char *))RwEngineInstance->stringFuncs.rwstrlen)(name);
            copy = animMalloc(length + 1);
            if (copy != NULL) {
                RwEngineInstance->stringFuncs.rwstrcpy(copy, name);
            }
        }
        if (copy == NULL) {
            sequence = NULL;
        } else {
            sequence->name = copy;
            sequence->priority = 0;
            sequence->nodes = NULL;
            sequence->callBack = NULL;
            sequence->data = NULL;
            sequence->next = NULL;
            sequence->time = 0.0F;
            sequence->duration = 0.0F;
            sequence->direction = 1;
        }
    }
    if (sequence == NULL) {
        animNodeFree(search.nodes);
        return NULL;
    }
    sequence->nodes = search.nodes;
    sequence->callBack = callBack != NULL ? callBack : RW_CB(void *, CB_ANIM_DEFAULT_PLAY);
    sequence->data = data;
    anim->numSequences++;
    double longest = rpAnimDurationStart;
    sequence->next = anim->sequences->next;
    anim->sequences->next = sequence;
    for (const RpAnimNode *node = search.nodes; node != NULL; node = node->next) {
        const RpAnimFrameData *nodeData = *(RpAnimFrameData **)((uint8_t *)node->frame + rpAnimFrameOffset);
        longest = animLongest(nodeData->listA, name, longest, rpAnimSumStartA);
        longest = animLongest(nodeData->listB, name, longest, rpAnimSumStartB);
    }
    sequence->duration = (float)longest;
    return clump;
}

/* ---- animation plugin: frame sequences, streams and playback ------------------------------------------ */

#define rpAnimHeadName ORIG_VAR(0x0069c6b0, const char, rpAnimHeadName)

static RwFreeList *animLinkFreeList(void)
{
    return *(RwFreeList **)((uint8_t *)RwEngineInstance + rpAnimEngineOffset);
}

static RpAnimFramePlugin *animFramePlugin(const void *frame)
{
    return (RpAnimFramePlugin *)((uint8_t *)frame + rpAnimFrameOffset);
}

static void animFree(void *block)
{
    RwEngineInstance->memoryFuncs.rwfree(block);
}

static size_t animStrlen(const char *s)
{
    return ((size_t(BMX_CDECL *)(const char *))RwEngineInstance->stringFuncs.rwstrlen)(s);
}

/* a - b with b opaque to the optimiser (no rewrite as a + -b, which flips the sign of a NaN b). */
static double animSub(double a, double b)
{
    __asm__("" : "+x"(b));
    return a - b;
}

/* Frees the name, keyframes and keys of a frame sequence (not the sequence); returns it. */
BMX_ADDR(0x00502a40)
RpAnimFrameSequence *rpAnimSequenceDeinit(RpAnimFrameSequence *sequence)
{
    if (sequence->name != NULL) {
        animFree(sequence->name);
        sequence->name = NULL;
    }
    if (sequence->frames != NULL) {
        animFree(sequence->frames);
        sequence->frames = NULL;
    }
    sequence->numFrames = 0;
    if (sequence->keys != NULL) {
        animFree(sequence->keys);
        sequence->keys = NULL;
    }
    sequence->numKeys = 0;
    return sequence;
}

/*
 * Sets up a frame sequence with one reference: a copy of the name, room for the keyframes of the type (other
 * types leave the keyframe pointer as it was) and the keys. NULL on failure (without a name nothing is
 * allocated; a later failure frees what the sequence has).
 */
BMX_ADDR(0x00502580)
RpAnimFrameSequence *rpAnimSequenceInit(RpAnimFrameSequence *sequence, const char *name, int32_t type,
                                        int32_t numFrames, int32_t numKeys)
{
    sequence->refCount = 1;
    sequence->name = NULL;
    if (name != NULL) {
        sequence->name = animMalloc(animStrlen(name) + 1);
        if (sequence->name != NULL) {
            RwEngineInstance->stringFuncs.rwstrcpy(sequence->name, name);
        }
    }
    if (sequence->name == NULL) {
        return NULL;
    }
    if (type == 1) {
        sequence->frames = animMalloc((size_t)numFrames * 12);
    } else if (type == 2) {
        sequence->frames = animMalloc((size_t)numFrames * 16);
    }
    if (sequence->frames == NULL) {
        rpAnimSequenceDeinit(sequence);
        return NULL;
    }
    sequence->numFrames = numFrames;
    sequence->type = type;
    sequence->keys = animMalloc((size_t)numKeys * 12);
    if (sequence->keys == NULL) {
        rpAnimSequenceDeinit(sequence);
        return NULL;
    }
    sequence->numKeys = numKeys;
    return sequence;
}

/* A new frame sequence (EAX = name, EDX = type, ECX = number of keyframes, the number of keys in the 4th
 * stack slot); returned even when setting it up failed. NULL only without memory for the block. */
BMX_ADDR(0x004fffdc)
__attribute__((regparm(3))) RpAnimFrameSequence *rpAnimSequenceCreate(const char *name, int32_t type,
                                                                      int32_t numFrames, uint32_t unused0,
                                                                      uint32_t unused1, uint32_t unused2,
                                                                      int32_t numKeys)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    RpAnimFrameSequence *sequence = animMalloc(sizeof *sequence);
    if (sequence == NULL) {
        return NULL;
    }
    rpAnimSequenceInit(sequence, name, type, numFrames, numKeys);
    return sequence;
}

/* Drops a reference (EAX = sequence) and destroys the sequence with the last one; always 1. */
BMX_ADDR(0x004fd244)
__attribute__((regparm(1))) int rpAnimSequenceRelease(RpAnimFrameSequence *sequence)
{
    if (--sequence->refCount == 0) {
        rpAnimSequenceDeinit(sequence);
        animFree(sequence);
    }
    return 1;
}

/* Releases the sequences of both lists of a frame's data, gives the links back and frees it (EAX); 1. */
BMX_ADDR(0x004fbbc0)
__attribute__((regparm(1))) int rpAnimFrameDataDestroy(RpAnimFrameData *data)
{
    for (int list = 0; list < 2; list++) {
        RpAnimFrameLink *link = list == 0 ? data->listA : data->listB;
        while (link != NULL) {
            RpAnimFrameLink *next = link->next;
            rpAnimSequenceRelease(link->sequence);
            RwEngineInstance->freeListFree(animLinkFreeList(), link);
            link = next;
        }
    }
    animFree(data);
    return 1;
}

/* Frame destructor: frees the frame's sequences (and resets the user data to -1 when it had some). */
BMX_ADDR(0x004fbd00)
void *rpAnimFrameDeinit(void *frame)
{
    RpAnimFramePlugin *plugin = animFramePlugin(frame);
    if (plugin->data != NULL) {
        rpAnimFrameDataDestroy(plugin->data);
        plugin->userData = 0xffffffffU;
        plugin->priority = 0;
        plugin->data = NULL;
    }
    animFramePlugin(frame)->data = NULL;
    return frame;
}

/*
 * Adds a sequence to the frame's list of its type (EAX = frame, EDX = sequence) unless the list has one of
 * that name, taking a reference. Creates the frame's data on demand. The frame, or NULL for another type or
 * without memory for the data (the link from the free list is not checked).
 */
BMX_ADDR(0x004fbdf8)
__attribute__((regparm(2))) void *rpAnimFrameAddSequence(void *frame, RpAnimFrameSequence *sequence)
{
    RpAnimFrameData *data = animFramePlugin(frame)->data;
    if (data == NULL) {
        data = rpAnimFrameDataCreate();
        if (data == NULL) {
            return NULL;
        }
        animFramePlugin(frame)->data = data;
    }
    RpAnimFrameLink **list;
    int32_t *count;
    if (sequence->type == 1) {
        list = &data->listA;
        count = &data->numA;
    } else if (sequence->type == 2) {
        list = &data->listB;
        count = &data->numB;
    } else {
        return NULL;
    }
    for (const RpAnimFrameLink *link = *list; link != NULL; link = link->next) {
        if (animStrcmp(link->sequence->name, sequence->name) == 0) {
            return frame;
        }
    }
    RpAnimFrameLink *link = RwEngineInstance->freeListAlloc(animLinkFreeList());
    link->sequence = sequence;
    link->next = *list;
    *list = link;
    (*count)++;
    sequence->refCount++;
    return frame;
}

/* Frame copy: the destination shares the source's sequences (list B first) and takes its user data. */
BMX_ADDR(0x004fbd50)
void *rpAnimFrameCopy(void *destination, const void *source)
{
    const RpAnimFrameData *data = animFramePlugin(source)->data;
    if (data == NULL) {
        return destination;
    }
    RpAnimFrameData *copy = rpAnimFrameDataCreate();
    if (copy == NULL) {
        return NULL;
    }
    animFramePlugin(destination)->data = copy;
    if (data->numB > 0) {
        for (const RpAnimFrameLink *link = data->listB; link != NULL; link = link->next) {
            rpAnimFrameAddSequence(destination, link->sequence);
        }
    }
    if (data->numA > 0) {
        for (const RpAnimFrameLink *link = data->listA; link != NULL; link = link->next) {
            rpAnimFrameAddSequence(destination, link->sequence);
        }
    }
    animFramePlugin(destination)->userData = animFramePlugin(source)->userData;
    return destination;
}

/*
 * Frame walk of rpAnimClumpAddSequence: a frame with a sequence of the searched name in list A or B gets a
 * node (pushed onto the search's list) with instances of them, then its children are searched. Without memory
 * the walk stops there (NULL; a half built node is freed).
 */
BMX_ADDR(0x004fc540)
void *rpAnimClumpFindNodes(void *frame, void *search)
{
    RpAnimNodeSearch *s = search;
    const RpAnimFrameData *data = animFramePlugin(frame)->data;
    if (data != NULL) {
        RpAnimFrameSequence *a = animFindSequence(data->listA, s->name);
        RpAnimFrameSequence *b = animFindSequence(data->listB, s->name);
        if (a != NULL || b != NULL) {
            RpAnimNode *node = rpAnimNodeCreate();
            if (node == NULL) {
                return NULL;
            }
            node->frame = frame;
            if (a != NULL) {
                node->a = animInstance(a);
                if (node->a == NULL) {
                    animNodeFree(node);
                    return NULL;
                }
            }
            if (b != NULL) {
                node->b = animInstance(b);
                if (node->b == NULL) {
                    animNodeFree(node);
                    return NULL;
                }
                node->state = 2;
            }
            node->state = 3;
            node->next = s->nodes;
            s->nodes = node;
            node->frame = frame;
        }
    }
    RwFrameForAllChildren(frame, CB_ANIM_FIND_NODES, search);
    return frame;
}

/* Frees a clump sequence's name and nodes, then the sequence. */
static void animClumpSequenceFree(RpAnimClumpSequence *sequence)
{
    if (sequence->name != NULL) {
        animFree(sequence->name);
        sequence->name = NULL;
    }
    if (sequence->nodes != NULL) {
        RpAnimNode *node = sequence->nodes;
        while (node != NULL) {
            RpAnimNode *next = node->next;
            animNodeFree(node);
            node = next;
        }
        sequence->nodes = NULL;
    }
    animFree(sequence);
}

/* An empty clump sequence named after the list heads (NULL without memory; the block leaks without a name).
 */
static RpAnimClumpSequence *animClumpHead(void)
{
    RpAnimClumpSequence *head = animMalloc(sizeof *head);
    if (head == NULL) {
        return NULL;
    }
    const char *name = &rpAnimHeadName;
    char *copy = animMalloc(animStrlen(name) + 1);
    if (copy == NULL) {
        return NULL;
    }
    RwEngineInstance->stringFuncs.rwstrcpy(copy, name);
    head->name = copy;
    head->priority = 0;
    head->nodes = NULL;
    head->callBack = NULL;
    head->data = NULL;
    head->next = NULL;
    head->time = 0.0F;
    head->duration = 0.0F;
    head->direction = 1;
    return head;
}

/*
 * The clump's animation data starts with the heads of its two sequence lists. NULL without memory; when the
 * second head fails the original frees a sequence through a NULL pointer (and crashes).
 */
BMX_ADDR(0x005027c0)
RpAnimClumpData *rpAnimClumpDataInit(RpAnimClumpData *data)
{
    data->frameList.frames = NULL;
    data->frameList.numFrames = 0;
    data->numSequences = 0;
    data->sequences = animClumpHead();
    if (data->sequences == NULL) {
        return NULL;
    }
    data->others = animClumpHead();
    if (data->others == NULL) {
        RpAnimClumpSequence *volatile broken = data->others;
        animClumpSequenceFree(broken);
        return NULL;
    }
    return data;
}

/* Frees the sequences of both lists (heads included) and the frame list's array. */
BMX_ADDR(0x005026a0)
RpAnimClumpData *rpAnimClumpDataDeinit(RpAnimClumpData *data)
{
    for (RpAnimClumpSequence *s = data->sequences; s != NULL;) {
        RpAnimClumpSequence *next = s->next;
        animClumpSequenceFree(s);
        s = next;
    }
    data->sequences = NULL;
    for (RpAnimClumpSequence *s = data->others; s != NULL;) {
        RpAnimClumpSequence *next = s->next;
        animClumpSequenceFree(s);
        s = next;
    }
    data->numSequences = 0;
    data->others = NULL;
    _rwFrameListDeinitialize(&data->frameList);
    return data;
}

/* Clump constructor and destructor of the plugin. */
BMX_ADDR(0x004fc740)
void *rpAnimClumpInit(void *clump)
{
    rpAnimClumpDataInit((RpAnimClumpData *)((uint8_t *)clump + rpAnimClumpOffset));
    return clump;
}

BMX_ADDR(0x004fc760)
void *rpAnimClumpDeinit(void *clump)
{
    rpAnimClumpDataDeinit((RpAnimClumpData *)((uint8_t *)clump + rpAnimClumpOffset));
    return clump;
}

/* ---- streams ---------------------------------------------------------------------------------------- */

/*
 * Reads a frame sequence: name (length word, at most 63 characters), type, keyframe and key counts, the
 * keyframes as reals and the keys. NULL on a failed read or without memory (with some leaks, as the
 * original). EAX = stream.
 */
BMX_ADDR(0x004fcf20)
__attribute__((regparm(1))) RpAnimFrameSequence *rpAnimSequenceRead(RwStream *stream)
{
    int32_t length;
    char raw[0x40];
    char name[0x41];
    if (RwStreamReadInt32(stream, &length, 4) == NULL || RwStreamRead(stream, raw, (uint32_t)length) == 0) {
        return NULL;
    }
    for (int32_t i = 0; i < length; i++) {
        name[i] = raw[i];
    }
    name[length] = '\0';
    int32_t type;
    int32_t numFrames;
    int32_t numKeys;
    if (RwStreamReadInt32(stream, &type, 4) == NULL || RwStreamReadInt32(stream, &numFrames, 4) == NULL ||
        RwStreamReadInt32(stream, &numKeys, 4) == NULL) {
        return NULL;
    }
    RpAnimFrameSequence *sequence = rpAnimSequenceCreate(name, type, numFrames, 0, 0, 0, numKeys);
    if (sequence == NULL) {
        return NULL;
    }
    if (type == 1) {
        void *frames = sequence->frames;
        if (RwStreamReadReal(stream, frames, (uint32_t)numFrames * 12) == NULL) {
            animFree(frames);
            return NULL;
        }
        if (frames == NULL) {
            return NULL;
        }
    } else if (type == 2) {
        void *frames = sequence->frames;
        if (RwStreamReadReal(stream, frames, (uint32_t)numFrames * 16) == NULL || frames == NULL) {
            return NULL;
        }
    }
    RpAnimKey *keys = (RpAnimKey *)sequence->keys;
    RpAnimKey *key = keys;
    for (int32_t i = 0; i < numKeys; i++, key++) {
        int32_t value;
        if (RwStreamReadInt32(stream, &value, 4) == NULL) {
            animFree(keys);
            return NULL;
        }
        key->start = value;
        if (RwStreamReadInt32(stream, &value, 4) == NULL) {
            animFree(keys);
            return NULL;
        }
        key->end = value;
        float duration;
        if (RwStreamReadReal(stream, &duration, 4) == NULL) {
            animFree(keys);
            return NULL;
        }
        key->duration = duration;
    }
    if (keys == NULL) {
        return NULL;
    }
    return sequence;
}

/*
 * The frame's animation chunk: the user data, a flag and, when set, the priority word and the sequences of
 * the two lists, each added to the frame (and released by the reader). The stream, NULL on a failed read.
 */
BMX_ADDR(0x004fcdd0)
void *rpAnimFrameChunkRead(void *stream, int32_t length, void *frame)
{
    (void)length;
    RpAnimFramePlugin *plugin = animFramePlugin(frame);
    if (RwStreamReadInt32(stream, &plugin->userData, 4) == NULL) {
        return NULL;
    }
    int32_t flag;
    if (RwStreamReadInt32(stream, &flag, 4) == NULL) {
        return NULL;
    }
    if (flag == 0) {
        return stream;
    }
    int32_t counts[2];
    if (RwStreamReadInt32(stream, &plugin->priority, 4) == NULL ||
        RwStreamReadInt32(stream, &counts[0], 4) == NULL ||
        RwStreamReadInt32(stream, &counts[1], 4) == NULL) {
        return NULL;
    }
    for (int list = 0; list < 2; list++) {
        for (int32_t i = 0; i < counts[list]; i++) {
            RpAnimFrameSequence *sequence = rpAnimSequenceRead(stream);
            if (sequence == NULL) {
                return NULL;
            }
            rpAnimFrameAddSequence(frame, sequence);
            rpAnimSequenceRelease(sequence);
        }
    }
    return stream;
}

/* Writes `count` keyframes (EAX = keyframes, EDX = count, ECX = stream, the byte count is stored through the
 * 4th stack slot): 16 bytes each (rotations) or 12 (translations). NULL when the write fails. */
BMX_ADDR(0x004fbb30)
__attribute__((regparm(3))) void *rpAnimWriteRotations(const void *frames, int32_t count, RwStream *stream,
                                                       uint32_t unused0, uint32_t unused1, uint32_t unused2,
                                                       int32_t *size)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    *size = 0;
    if (RwStreamWriteReal(stream, frames, (uint32_t)count * 16) == NULL) {
        return NULL;
    }
    *size += count * 16;
    return (void *)frames;
}

BMX_ADDR(0x004fbb70)
__attribute__((regparm(3))) void *rpAnimWriteTranslations(const void *frames, int32_t count, RwStream *stream,
                                                          uint32_t unused0, uint32_t unused1,
                                                          uint32_t unused2, int32_t *size)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    *size = 0;
    if (RwStreamWriteReal(stream, frames, (uint32_t)count * 12) == NULL) {
        return NULL;
    }
    *size += count * 12;
    return (void *)frames;
}

/* Writes a frame sequence as rpAnimSequenceRead reads it; *size gets the bytes written (EAX = sequence,
 * EDX = stream, ECX = size). NULL when a write fails or the end of the keys is a NULL pointer. */
BMX_ADDR(0x004fd400)
__attribute__((regparm(3))) RpAnimFrameSequence *rpAnimSequenceWrite(RpAnimFrameSequence *sequence,
                                                                     RwStream *stream, int32_t *size)
{
    *size = 0;
    const char *name = sequence->name;
    int32_t value = (int32_t)animStrlen(name) + 1;
    int32_t written = 4;
    if (RwStreamWriteInt32(stream, &value, 4) == NULL) {
        return NULL;
    }
    *size += written;
    written = value;
    if (RwStreamWrite(stream, name, (uint32_t)value) == NULL) {
        return NULL;
    }
    *size += written;
    const int32_t words[3] = {sequence->type, sequence->numFrames, sequence->numKeys};
    for (int i = 0; i < 3; i++) {
        value = words[i];
        written = 4;
        if (RwStreamWriteInt32(stream, &value, 4) == NULL) {
            return NULL;
        }
        *size += written;
    }
    if (words[0] == 1) {
        if (rpAnimWriteTranslations(sequence->frames, words[1], stream, 0, 0, 0, &written) == NULL) {
            return NULL;
        }
        *size += written;
    } else if (words[0] == 2) {
        if (rpAnimWriteRotations(sequence->frames, words[1], stream, 0, 0, 0, &written) == NULL) {
            return NULL;
        }
        *size += written;
    }
    const RpAnimKey *key = (const RpAnimKey *)sequence->keys;
    written = 0;
    for (int32_t i = 0; i < words[2]; i++, key++) {
        value = key->start;
        if (RwStreamWriteInt32(stream, &value, 4) == NULL) {
            return NULL;
        }
        written += 4;
        value = key->end;
        if (RwStreamWriteInt32(stream, &value, 4) == NULL) {
            return NULL;
        }
        written += 4;
        const float duration = key->duration;
        if (RwStreamWriteReal(stream, &duration, 4) == NULL) {
            return NULL;
        }
        written += 4;
    }
    if (key == NULL) {
        return NULL;
    }
    *size += written;
    return sequence;
}

/* Writes the frame's animation chunk as rpAnimFrameChunkRead reads it; the stream, NULL on a failed write. */
BMX_ADDR(0x004fd280)
void *rpAnimFrameChunkWrite(void *stream, int32_t length, const void *frame)
{
    (void)length;
    const RpAnimFramePlugin *plugin = animFramePlugin(frame);
    const RpAnimFrameData *data = plugin->data;
    if (RwStreamWriteInt32(stream, &plugin->userData, 4) == NULL) {
        return NULL;
    }
    const int32_t flag = data != NULL;
    if (RwStreamWriteInt32(stream, &flag, 4) == NULL) {
        return NULL;
    }
    if (flag == 0) {
        return stream;
    }
    if (RwStreamWriteInt32(stream, &plugin->priority, 4) == NULL) {
        return NULL;
    }
    const int32_t numA = data->numA;
    if (RwStreamWriteInt32(stream, &numA, 4) == NULL) {
        return NULL;
    }
    const int32_t numB = data->numB;
    if (RwStreamWriteInt32(stream, &numB, 4) == NULL) {
        return NULL;
    }
    int32_t size;
    for (const RpAnimFrameLink *link = data->listA; link != NULL; link = link->next) {
        if (rpAnimSequenceWrite(link->sequence, stream, &size) == NULL) {
            return NULL;
        }
    }
    for (const RpAnimFrameLink *link = data->listB; link != NULL; link = link->next) {
        if (rpAnimSequenceWrite(link->sequence, stream, &size) == NULL) {
            return NULL;
        }
    }
    return stream;
}

/* ---- playback --------------------------------------------------------------------------------------- */

/* Puts an instance at the start of its sequence (backward: at the last key) with the time t (EAX = instance,
 * EDX = direction, ST0 = t). */
__attribute__((used, noinline)) static RpAnimInstance *rpAnimInstanceResetImpl(RpAnimInstance *instance,
                                                                               int32_t forward, double t)
{
    instance->time = (float)t;
    const RpAnimFrameSequence *sequence = instance->sequence;
    if (forward != 0) {
        instance->keys = sequence->keys;
    } else {
        instance->keys = sequence->keys + sequence->numKeys * 12 - 12;
    }
    return instance;
}

BMX_ADDR(0x004fbf30)
__attribute__((naked)) RpAnimInstance *rpAnimInstanceReset(void)
{
    __asm__ volatile("subl $8, %esp\n\t"
                     "fstpl (%esp)\n\t"
                     "pushl %edx\n\t"
                     "pushl %eax\n\t"
                     "call _rpAnimInstanceResetImpl\n\t"
                     "addl $16, %esp\n\t"
                     "ret");
}

/*
 * Moves the instance's time on by dt and steps over the keys it passes (setting bit 1 of the flags): forward
 * while the time reaches the key's duration, backward while it does not stay below it; at either end of the
 * keys the time stops at the duration of the last key reached. Returns the position in the current key as a
 * fraction of its duration (backward: from its end).
 */
static double animAdvance(RpAnimInstance *instance, double dt, int32_t forward, int32_t *flags)
{
    instance->time = (float)(dt + instance->time);
    const RpAnimFrameSequence *sequence = instance->sequence;
    const RpAnimKey *key = (const RpAnimKey *)instance->keys;
    int32_t index = (int32_t)(((const uint8_t *)key - sequence->keys) / 12);
    if (forward != 0) {
        if (!((double)key->duration > (double)instance->time)) {
            for (;;) {
                if (index >= sequence->numKeys - 1) {
                    instance->time = key->duration;
                    break;
                }
                const double duration = key->duration;
                key++;
                index++;
                instance->time = (float)animSub(instance->time, duration);
                *flags |= 2;
                instance->keys = (uint8_t *)key;
                if (!((double)instance->time >= (double)key->duration)) {
                    break;
                }
            }
        }
        return (double)instance->time / key->duration;
    }
    if (!((double)key->duration > (double)instance->time)) {
        for (;;) {
            if (index <= 0) {
                instance->time = key->duration;
                break;
            }
            instance->time = (float)animSub(instance->time, key->duration);
            key--;
            index--;
            *flags |= 2;
            instance->keys = (uint8_t *)key;
            if ((double)key->duration > (double)instance->time) {
                break;
            }
        }
    }
    return animSub(key->duration, instance->time) / key->duration;
}

/*
 * Sets up the slerp cache from q0 to q1 (x, y, z, w): q1 is negated when the dot product is negative (the
 * shorter way round); the cosine is clamped to [-1, 1] (1 below -1 and for NaN). The angle is
 * atan2(sqrt((1 + c)(1 - c)), c); from c = 0.99999 on the blend is linear, else both quaternions are scaled
 * by 1 / sin(angle).
 */
BMX_ADDR(0x0051ce10)
void rpAnimSlerpCacheSetup(const float *q0, const float *q1, RpAnimSlerpCache *cache)
{
    cache->q0[3] = q0[3];
    cache->q0[0] = q0[0];
    cache->q0[1] = q0[1];
    cache->q0[2] = q0[2];
    const double dot =
        (((double)q0[3] * q1[3] + (double)q0[2] * q1[2]) + (double)q1[1] * q0[1]) + (double)q1[0] * q0[0];
    double c;
    if (0.0 > dot) {
        c = -1.0 <= dot ? -dot : 1.0;
        cache->q1[3] = -q1[3];
        cache->q1[0] = -q1[0];
        cache->q1[1] = -q1[1];
        cache->q1[2] = -q1[2];
    } else {
        c = 1.0 >= dot ? dot : 1.0;
        cache->q1[3] = q1[3];
        cache->q1[0] = q1[0];
        cache->q1[1] = q1[1];
        cache->q1[2] = q1[2];
    }
    cache->angle = bmxX87AngleOfCosine(c);
    cache->linear = c >= 0x1.fffeb0p-1;
    if (cache->linear != 0) {
        return;
    }
    const double scale = 1.0 / bmxX87Fsin(cache->angle);
    for (int i = 0; i < 4; i++) {
        cache->q0[i] = (float)(cache->q0[i] * scale);
        cache->q1[i] = (float)(cache->q1[i] * scale);
    }
}

/* x87 FSIN of a value in a register. */
static double animSin(double a)
{
    return bmxX87Fsin(a);
}

/*
 * Rotation of a frame by its rotation instance (EAX = instance, EDX = interpolator state, ECX = frame,
 * direction in the 4th stack slot, ST0 = dt): advances the instance, renews the slerp cache when the key
 * changed, and sets the frame's matrix to the rotation of the keyframes' quaternions at the key's position
 * (the first one up to 0, the second from 1, interpolated between), keeping the matrix's translation.
 */
__attribute__((used, noinline)) static RpAnimInstance *rpAnimRotationUpdateImpl(RpAnimInstance *instance,
                                                                                RpAnimInterpolator *interp,
                                                                                RwFrame *frame, double dt,
                                                                                int32_t forward)
{
    interp->state |= 1;
    const float (*frames)[4] = instance->sequence->frames;
    const double t = animAdvance(instance, dt, forward, &interp->state);
    const RpAnimKey *key = (const RpAnimKey *)instance->keys;
    const float *q0 = frames[key->start];
    const float *q1 = frames[key->end];
    RwMatrix *m = RwFrameGetMatrix(frame);
    const RwV3d position = m->pos;
    if ((interp->state & 2) != 0) {
        rpAnimSlerpCacheSetup(q0, q1, &interp->cache);
        interp->state &= ~2;
    }
    double x;
    double y;
    double z;
    double w;
    if (!(t > 0.0)) {
        x = q0[0];
        y = q0[1];
        z = q0[2];
        w = q0[3];
    } else if (!(t < 1.0)) {
        x = q1[0];
        y = q1[1];
        z = q1[2];
        w = q1[3];
    } else {
        const RpAnimSlerpCache *c = &interp->cache;
        double s0 = animSub(1.0, t);
        double s1 = t;
        if (c->linear == 0) {
            const float angle = c->angle;
            s0 = animSin(angle * s0);
            s1 = animSin(angle * t);
        }
        x = c->q1[0] * s1 + c->q0[0] * s0;
        y = c->q1[1] * s1 + s0 * c->q0[1];
        z = c->q1[2] * s1 + s0 * c->q0[2];
        w = s0 * c->q0[3] + s1 * c->q1[3];
    }
    const double zz = z * z;
    const double yy = y * y;
    const double xx = x * x;
    m->right.x = (float)animSub(1.0, (zz + yy) * 2.0);
    m->up.y = (float)animSub(1.0, (zz + xx) * 2.0);
    m->at.z = (float)animSub(1.0, (yy + xx) * 2.0);
    const double zw = z * w;
    const double xy = x * y;
    const double xz = x * z;
    const double yw = y * w;
    m->pos.z = 0.0F;
    m->pos.y = 0.0F;
    m->pos.x = 0.0F;
    m->right.y = (float)((zw + xy) * 2.0);
    m->up.x = (float)(animSub(xy, zw) * 2.0);
    m->at.x = (float)((yw + xz) * 2.0);
    m->right.z = (float)(animSub(xz, yw) * 2.0);
    const double xw = x * w;
    const double yz = y * z;
    m->up.z = (float)((xw + yz) * 2.0);
    m->at.y = (float)(animSub(yz, xw) * 2.0);
    RwMatrixUpdate(m);
    m->flags = (m->flags | 3) & 0xfffdffffU;
    RwFrameTranslate(frame, &position, rwCOMBINEPOSTCONCAT);
    return instance;
}

BMX_ADDR(0x004fbf60)
__attribute__((naked)) RpAnimInstance *rpAnimRotationUpdate(uint32_t unused0, uint32_t unused1,
                                                            uint32_t unused2, int32_t forward)
{
    __asm__ volatile("pushl 0x10(%esp)\n\t"
                     "subl $8, %esp\n\t"
                     "fstpl (%esp)\n\t"
                     "pushl %ecx\n\t"
                     "pushl %edx\n\t"
                     "pushl %eax\n\t"
                     "call _rpAnimRotationUpdateImpl\n\t"
                     "addl $24, %esp\n\t"
                     "ret");
}

/*
 * Moves a node's frame on by dt (EAX = interpolator state, EDX = frame, ECX = direction, ST0 = dt): the
 * rotation instance first, then the translation instance, whose keyframes are interpolated linearly into the
 * frame's matrix translation (replacing it).
 */
__attribute__((used, noinline)) static RpAnimInterpolator *
rpAnimInterpolatorUpdateImpl(RpAnimInterpolator *interp, RwFrame *frame, int32_t forward, double dt)
{
    if (interp->b != NULL) {
        rpAnimRotationUpdateImpl(interp->b, interp, frame, dt, forward);
    }
    RpAnimInstance *instance = interp->a;
    if (instance == NULL) {
        return interp;
    }
    interp->state |= 1;
    const float (*frames)[3] = instance->sequence->frames;
    const double t = animAdvance(instance, dt, forward, &interp->state);
    const RpAnimKey *key = (const RpAnimKey *)instance->keys;
    const float *v0 = frames[key->start];
    const float *v1 = frames[key->end];
    RwMatrix *m = RwFrameGetMatrix(frame);
    const float dx = (float)animSub(v1[0], v0[0]);
    const float dy = (float)animSub(v1[1], v0[1]);
    const float dz = (float)animSub(v1[2], v0[2]);
    const float tx = (float)(t * dx);
    const float ty = (float)(t * dy);
    const float tz = (float)(t * dz);
    RwV3d translation;
    translation.x = (float)((double)tx + v0[0]);
    translation.y = (float)((double)ty + v0[1]);
    translation.z = (float)((double)tz + v0[2]);
    m->pos.x = 0.0F;
    m->pos.y = 0.0F;
    m->pos.z = 0.0F;
    RwFrameTranslate(frame, &translation, rwCOMBINEPOSTCONCAT);
    return interp;
}

BMX_ADDR(0x004fc2e0)
__attribute__((naked)) RpAnimInterpolator *rpAnimInterpolatorUpdate(void)
{
    __asm__ volatile("subl $8, %esp\n\t"
                     "fstpl (%esp)\n\t"
                     "pushl %ecx\n\t"
                     "pushl %edx\n\t"
                     "pushl %eax\n\t"
                     "call _rpAnimInterpolatorUpdateImpl\n\t"
                     "addl $20, %esp\n\t"
                     "ret");
}

#define rpAnimTimeOffset ORIG_VAR(0x0069c698, float, rpAnimTimeOffset)
#define rpAnimStartStep  ORIG_VAR(0x0069c69c, float, rpAnimStartStep)

typedef RpClump *(BMX_CDECL *RpAnimClumpSequenceCallBack)(RpClump *clump, RpAnimClumpSequence *sequence,
                                                          float time, void *data);

/*
 * Starts the clump's sequence `name` at time t (EAX = clump, EDX = name, ECX = direction, ST0 = t, the
 * priority in the 4th stack slot): moves it to the list of running sequences and resets the instances of its
 * nodes. While the time is within the sequence, the frames whose priority it reaches take it over and are
 * posed; past the end, the sequence goes back to the first list and its callback gets the time wrapped into
 * the duration. NULL when the clump has no such sequence (or it runs already).
 */
__attribute__((used, noinline)) static RpClump *
rpAnimClumpStartSequenceImpl(RpClump *clump, const char *name, int32_t forward, double t, int32_t priority)
{
    const int32_t offset = rpAnimClumpOffset;
    RpAnimClumpData *anim = (RpAnimClumpData *)((uint8_t *)clump + offset);
    RpAnimClumpSequence *prev = anim->sequences;
    RpAnimClumpSequence *sequence = prev->next;
    while (sequence != NULL && animStrcmp(sequence->name, name) != 0) {
        prev = sequence;
        sequence = sequence->next;
    }
    if (sequence == NULL) {
        return NULL;
    }
    prev->next = sequence->next;
    sequence->next = NULL;
    sequence->priority = priority;
    sequence->direction = forward;
    sequence->next = anim->others->next;
    anim->others->next = sequence;
    sequence->time = (float)t;
    for (RpAnimNode *node = sequence->nodes; node != NULL; node = node->next) {
        node->state = 2;
        if (node->a != NULL) {
            rpAnimInstanceResetImpl(node->a, forward, t);
        }
        if (node->b != NULL) {
            rpAnimInstanceResetImpl(node->b, forward, t);
        }
    }
    const double time = (double)sequence->time + rpAnimTimeOffset;
    sequence->time = (float)time;
    if ((double)sequence->duration >= time) {
        for (RpAnimNode *node = sequence->nodes; node != NULL; node = node->next) {
            RpAnimFramePlugin *plugin = animFramePlugin(node->frame);
            const int32_t direction = sequence->direction;
            if (sequence->priority >= plugin->priority) {
                plugin->priority = sequence->priority;
                rpAnimInterpolatorUpdateImpl((RpAnimInterpolator *)&node->state, node->frame, direction,
                                             rpAnimStartStep);
            }
        }
        return clump;
    }
    RpAnimClumpSequence *before = anim->others;
    RpAnimClumpSequence *found = before->next;
    while (found != NULL && animStrcmp(found->name, sequence->name) != 0) {
        before = found;
        found = found->next;
    }
    if (found == NULL) {
        return clump;
    }
    before->next = found->next;
    found->next = NULL;
    found->next = anim->sequences->next;
    anim->sequences->next = found;
    double wrapped = sequence->time;
    if (!((double)sequence->duration >= wrapped)) {
        do {
            wrapped = animSub(wrapped, sequence->duration);
            sequence->time = (float)wrapped;
        } while (!((double)sequence->duration >= wrapped));
    }
    ((RpAnimClumpSequenceCallBack)sequence->callBack)(clump, sequence, (float)wrapped, sequence->data);
    return clump;
}

BMX_ADDR(0x004ff960)
__attribute__((naked)) RpClump *rpAnimClumpStartSequence(uint32_t unused0, uint32_t unused1, uint32_t unused2,
                                                         int32_t priority)
{
    __asm__ volatile("pushl 0x10(%esp)\n\t"
                     "subl $8, %esp\n\t"
                     "fstpl (%esp)\n\t"
                     "pushl %ecx\n\t"
                     "pushl %edx\n\t"
                     "pushl %eax\n\t"
                     "call _rpAnimClumpStartSequenceImpl\n\t"
                     "addl $24, %esp\n\t"
                     "ret");
}

/* The default callback at the end of a sequence: starts it again (forward, same priority) at the time wrapped
 * into its duration. */
BMX_ADDR(0x00502990)
RpClump *rpAnimClumpDefaultCallBack(RpClump *clump, RpAnimClumpSequence *sequence, float time, void *data)
{
    (void)data;
    double t = time;
    const double duration = sequence->duration;
    if (!(duration >= t)) {
        do {
            t = animSub(t, duration);
        } while (!(duration >= t));
    }
    rpAnimClumpStartSequenceImpl(clump, sequence->name, 1, t, sequence->priority);
    return clump;
}
/* ---- LOD atomic plugin (id 0x112): the per-atomic data, its render hook and its stream callbacks ------- */

static RpLodAtomicData **lodSlot(const void *object, int32_t offset)
{
    return (RpLodAtomicData **)((uint8_t *)object + offset);
}

/* The original's inline double to int conversion: it truncates, values below 1 give 0. */
static int32_t truncateToInt(double x)
{
    uint64_t bits;
    memcpy(&bits, &x, sizeof bits);
    const uint32_t high = (uint32_t)(bits >> 32);
    const uint32_t low = (uint32_t)bits;
    const uint32_t exponent = (high >> 20) & 0x7ff;
    if (exponent <= 0x3fe) {
        return 0;
    }
    const uint32_t value = (((high << 11) | (low >> 21)) | 0x80000000U) >> ((0x41eU - exponent) & 31);
    return (int32_t)((high & 0x80000000U) != 0 ? 0U - value : value);
}

/*
 * The default level selection: the distance of the atomic from the current camera along the camera's view
 * direction (the dot product of the offset and the camera's at vector), times the data's scale, truncated
 * and clamped to 0..9. -1 when there is no plugin data, atomic or camera.
 */
BMX_ADDR(0x00505b30)
int32_t rpLodAtomicSelectLevel(RpAtomic *atomic)
{
    const int32_t offset = rpLodAtomicOffset;
    int32_t level = -1;
    RwCamera *camera = RwCameraGetCurrentCamera();
    if (offset >= 0 && atomic != NULL && camera != NULL) {
        const RpLodAtomicData *lod = *lodSlot(atomic, offset);
        if (lod != NULL) {
            const RwMatrix *ltm = RwFrameGetLTM(atomic->object.object.parent);
            const float ax = ltm->pos.x;
            const float ay = ltm->pos.y;
            const float az = ltm->pos.z;
            ltm = RwFrameGetLTM(RwCameraGetFrame(camera));
            const float cx = ltm->pos.x;
            const float cy = ltm->pos.y;
            const float cz = ltm->pos.z;
            ltm = RwFrameGetLTM(RwCameraGetFrame(camera));
            const double dz = (double)az - (double)cz;
            const double dy = (double)ay - (double)cy;
            const double dx = (double)ax - (double)cx;
            const double distance =
                (dx * (double)ltm->at.x + (dz * (double)ltm->at.z + dy * (double)ltm->at.y)) *
                (double)lod->scale;
            level = truncateToInt(distance);
            level = level < 0 ? 0 : (level < 10 ? level : 9);
        }
    }
    return level;
}

/*
 * EAX = atomic, EDX = LOD data. Selects the level (-1 keeps the current one, 10 and up the last) and gives
 * the atomic that geometry if it has another; remembers the level. Returns the atomic when the level has a
 * geometry, NULL otherwise (and when there is no data).
 */
BMX_ADDR(0x00505c60)
__attribute__((regparm(2))) RpAtomic *rpLodAtomicApply(RpAtomic *atomic, RpLodAtomicData *lod)
{
    RpAtomic *result = NULL;
    if (lod != NULL) {
        int32_t level = lod->select(atomic);
        if (level < 0) {
            level = lod->current;
        } else if (level >= 10) {
            level = 9;
        }
        RpGeometry *geometry = *(RpGeometry **)((uint8_t *)lod + 0x18 + 4 * level);
        if (geometry != atomic->geometry) {
            RpAtomicSetGeometry(atomic, geometry, 0);
        }
        lod->current = level;
        if (geometry != NULL) {
            result = atomic;
        }
    }
    return result;
}

/* The render callback that replaces the atomic's: picks the level, then calls the atomic's own callback. */
BMX_ADDR(0x00505cd0)
RpAtomic *rpLodAtomicRender(RpAtomic *atomic)
{
    if (rpLodAtomicOffset < 0 || atomic == NULL) {
        return NULL;
    }
    RpLodAtomicData *lod = *lodSlot(atomic, rpLodAtomicOffset);
    if (lod == NULL) {
        return NULL;
    }
    if (rpLodAtomicApply(atomic, lod) == NULL) {
        return NULL;
    }
    return lod->savedRender(atomic);
}

/* EAX = atomic, EDX = LOD data: puts the plugin's render callback on the atomic (once). */
BMX_ADDR(0x00505d10)
__attribute__((regparm(2))) void rpLodAtomicHook(RpAtomic *atomic, RpLodAtomicData *lod)
{
    if (lod != NULL && lod->hooked == 0) {
        lod->savedRender = atomic->renderCallBack;
        atomic->renderCallBack = RW_CB(RpAtomicRenderCallBack, 0x00505cd0);
        atomic->stamp++;
        lod->hooked = 1;
    }
}

/* EAX = atomic, EDX = LOD data: gives the atomic its own render callback back. */
BMX_ADDR(0x00505d40)
__attribute__((regparm(2))) void rpLodAtomicUnhook(RpAtomic *atomic, RpLodAtomicData *lod)
{
    if (lod != NULL && lod->hooked == 1) {
        if (lod->savedRender != NULL) {
            atomic->renderCallBack = lod->savedRender;
            atomic->stamp++;
        }
        lod->hooked = 0;
        lod->savedRender = NULL;
    }
}

/* Empty data: no levels, nothing selected, the default select callback. */
BMX_ADDR(0x00506780)
RpLodAtomicData *rpLodAtomicDataInit(RpLodAtomicData *lod)
{
    if (lod != NULL) {
        lod->savedRender = NULL;
        lod->hooked = 0;
        lod->select = RW_CB(RpLodSelectCallBack, 0x00505b30);
        lod->range = -1.0F;
        lod->current = -1;
        lod->scale = -1.0F;
        for (int i = 0; i < 10; i++) {
            lod->levels[i] = NULL;
        }
    }
    return lod;
}

/* A new, initialised data block from the plugin's free list (NULL when there is none). */
BMX_ADDR(0x00505d70)
RpLodAtomicData *rpLodAtomicDataCreate(void)
{
    RpLodAtomicData *lod = RwEngineInstance->freeListAlloc(rpLodFreeList);
    if (lod == NULL) {
        return NULL;
    }
    rpLodAtomicDataInit(lod);
    return lod;
}

/* EAX = destination, EDX = source: copies the data and takes a reference of every geometry. Returns the
 * source. */
BMX_ADDR(0x00505db0)
__attribute__((regparm(2))) RpLodAtomicData *rpLodAtomicDataCopy(RpLodAtomicData *dst, RpLodAtomicData *src)
{
    if (dst != NULL && src != NULL) {
        memcpy(dst, src, 0x18);
        for (int i = 0; i < 10; i++) {
            dst->levels[i] = src->levels[i];
            if (src->levels[i] != NULL) {
                RpGeometryAddRef(src->levels[i]);
            }
        }
    }
    return src;
}

/* EAX = data: releases the geometries, resets the data and gives it back to the free list. */
BMX_ADDR(0x00505e10)
__attribute__((regparm(1))) void rpLodAtomicDataDestroy(RpLodAtomicData *lod)
{
    if (lod != NULL) {
        for (int i = 0; i < 10; i++) {
            if (lod->levels[i] != NULL) {
                RpGeometryDestroy(lod->levels[i]);
            }
        }
        rpLodAtomicDataInit(lod);
        RwEngineInstance->freeListFree(rpLodFreeList, lod);
    }
}

/* Engine module of the plugin: the free list of data blocks (shared by all atomics). */
BMX_ADDR(0x00506240)
void *LODAtomicCallBackOpen(void *engine, int32_t offset)
{
    (void)offset;
    rpLodFreeList = RwFreeListCreate(0x40, 0x40, 0);
    if (rpLodFreeList == NULL) {
        return NULL;
    }
    return engine;
}

BMX_ADDR(0x00506280)
void *LODAtomicCallBackClose(void *engine)
{
    if (rpLodFreeList != NULL) {
        RwFreeListDestroy(rpLodFreeList);
    }
    rpLodFreeList = NULL;
    return engine;
}

BMX_ADDR(0x00506190)
void *LODAtomicCallBackConstructor(void *atomic, int32_t offset)
{
    *lodSlot(atomic, offset) = NULL;
    return atomic;
}

BMX_ADDR(0x00506200)
void *LODAtomicCallBackDestructor(void *atomic, int32_t offset)
{
    RpLodAtomicData *lod = *lodSlot(atomic, offset);
    if (lod != NULL) {
        rpLodAtomicUnhook(atomic, lod);
        rpLodAtomicDataDestroy(lod);
        *lodSlot(atomic, offset) = NULL;
    }
    return atomic;
}

/* Gives the clone its own data when the source has some. Returns the source (a quirk of the original). */
BMX_ADDR(0x005061b0)
void *LODAtomicCallBackCopy(void *dst, const void *src, int32_t offset)
{
    RpLodAtomicData *srcLod = *lodSlot(src, offset);
    if (srcLod != NULL) {
        RpLodAtomicData *dstLod = *lodSlot(dst, offset);
        if (dstLod == NULL) {
            dstLod = rpLodAtomicDataCreate();
        }
        if (dstLod != NULL) {
            rpLodAtomicDataCopy(dstLod, srcLod);
        }
        *lodSlot(dst, offset) = dstLod;
    }
    return (void *)(uintptr_t)src;
}

/* ---- the plugin's stream chunk: the data block, then the geometry of every level but the current --------
 */

BMX_ADDR(0x00505e60)
void *LODAtomicCallBackReadChunk(RwStream *stream, int32_t length, void *atomic, int32_t offset, int32_t size)
{
    (void)length;
    (void)size;
    RwStream *result = NULL;
    uint32_t chunkLength;
    uint32_t version;
    if (RwStreamFindChunk(stream, 1, &chunkLength, &version) == 0) {
        *lodSlot(atomic, offset) = NULL;
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        *lodSlot(atomic, offset) = NULL;
        RwError error;
        error.pluginID = 0x112;
        error.errorCode = _rwerror(rwERR_BADVERSION);
        RwErrorSet(&error);
        return NULL;
    }
    result = stream;
    RpLodAtomicData *lod = rpLodAtomicDataCreate();
    RwStreamRead(stream, lod, chunkLength);
    RwMemNative32(lod, 0x40);
    for (int32_t i = 0; i < 10; i++) {
        if (lod->levels[i] != NULL && i != lod->current) {
            if (RwStreamFindChunk(stream, 0xf, NULL, &version) != 0) {
                if (version >= 0x301 && version <= 0x303) {
                    RpGeometry *geometry = RpGeometryStreamRead(stream);
                    RpGeometryAddRef(geometry);
                    lod->levels[i] = geometry;
                } else {
                    RwError error;
                    error.pluginID = 0x112;
                    error.errorCode = _rwerror(rwERR_BADVERSION);
                    RwErrorSet(&error);
                }
            }
        }
    }
    if (lod->current >= 0) {
        RpGeometry *own = ((RpAtomic *)atomic)->geometry;
        RpGeometryAddRef(own);
        *(RpGeometry **)((uint8_t *)lod + 0x18 + 4 * lod->current) = own;
    }
    lod->select = RW_CB(RpLodSelectCallBack, 0x00505b30);
    lod->hooked = 0;
    rpLodAtomicHook(atomic, lod);
    *lodSlot(atomic, offset) = lod;
    return result;
}

BMX_ADDR(0x00506030)
void *LODAtomicCallBackWriteChunk(RwStream *stream, int32_t length, const void *atomic, int32_t offset,
                                  int32_t size)
{
    (void)length;
    (void)size;
    const RpLodAtomicData *lod = *lodSlot(atomic, offset);
    if (lod == NULL) {
        return stream;
    }
    _rwStreamWriteVersionedChunkHeader(stream, 1, 0x40, 0x303);
    uint32_t copy[16];
    memcpy(copy, lod, sizeof copy);
    RwMemLittleEndian32(copy, 0x40);
    RwStreamWrite(stream, lod, 0x40);
    for (int32_t i = 0; i < 10; i++) {
        if (lod->levels[i] != NULL && i != lod->current) {
            RpGeometryStreamWrite(lod->levels[i], stream);
        }
    }
    return stream;
}

BMX_ADDR(0x00506140)
int32_t LODAtomicCallBackGetChunkSize(const void *atomic, int32_t offset, int32_t size)
{
    (void)size;
    int32_t total = 0;
    const RpLodAtomicData *lod = *lodSlot(atomic, offset);
    if (lod != NULL) {
        total = 0x4c;
        for (int32_t i = 0; i < 10; i++) {
            if (lod->levels[i] != NULL && i != lod->current) {
                total += RpGeometryStreamGetSize(lod->levels[i]) + 0xc;
            }
        }
    }
    return total;
}

/* ---- libpng callbacks the RtPNGImageRead glue installs (the library itself is not ported) ------------- */

/* libpng's png_error: does not return in practice (it longjmps); the glue returns whatever it leaves in EAX.
 */
ORIG_FN(0x00507050, uint32_t, png_error, (void *png, const char *message));

/* libpng's allocator and free function: the engine's memory interface. */
BMX_ADDR(0x00506820)
void *rwPngMalloc(void *png, size_t size)
{
    (void)png;
    return RwOsGetMemoryInterface()->rwmalloc(size);
}

BMX_ADDR(0x00506840)
void rwPngFree(void *png, void *block)
{
    (void)png;
    RwOsGetMemoryInterface()->rwfree(block);
}

/* libpng's read function: reads from the RenderWare stream the PNG struct carries (at 0x54). */
BMX_ADDR(0x00506860)
uint32_t rwPngRead(void *png, void *buffer, uint32_t length)
{
    RwStream *stream = *(RwStream **)((uint8_t *)png + 0x54);
    if (stream != NULL) {
        return RwStreamRead(stream, buffer, length);
    }
    return png_error(png, NULL);
}
