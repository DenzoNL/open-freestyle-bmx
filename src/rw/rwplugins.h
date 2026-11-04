/*
 * RenderWare 3.1 plugin registration (rw/plugins.c): the per-object-type wrappers around the plugin
 * registry, the plugin attach routines (world, PVS, anim, bone, LOD atomic, core) and the skin helpers of
 * the bone/anim plugins.
 *
 * The attach routines register callbacks that are still original code (or another slice's); those are
 * passed by address (RW_CB).
 */
#ifndef RW_RWPLUGINS_H
#define RW_RWPLUGINS_H

#include "compat/layout.h"
#include "rw/rwcore.h"

typedef struct RwSphere {
    ZVec3 center;
    float radius;
} RwSphere;

typedef struct RwBBox {
    ZVec3 sup;
    ZVec3 inf;
} RwBBox;

/* RpMorphTarget: one vertex set of a geometry. */
typedef struct RpMorphTarget {
    void *parentGeom;        /* 0x00 */
    RwSphere boundingSphere; /* 0x04 */
    ZVec3 *verts;            /* 0x14 */
    ZVec3 *normals;          /* 0x18 */
} RpMorphTarget;

BMX_LAYOUT32_ASSERT(offsetof(RpMorphTarget, boundingSphere) == 4, "RpMorphTarget layout");
BMX_LAYOUT32_ASSERT(offsetof(RpMorphTarget, normals) == 0x18, "RpMorphTarget layout");

/* RpGeometry and RpTriangle are in rw/rwclump.h. */
typedef struct RpGeometry RpGeometry;
typedef struct RpAtomic RpAtomic;
typedef struct RpClump RpClump;
struct RwResEntry;

typedef RpAtomic *(BMX_CDECL *RpAtomicRenderCallBack)(RpAtomic *atomic);

/* Morph interpolation state of an atomic: from start to end morph target, position / time of the way. */
typedef struct RpInterpolator {
    uint32_t flags;           /* 0x00 bit 1: the bounding sphere is out of date */
    int16_t startMorphTarget; /* 0x04 */
    int16_t endMorphTarget;   /* 0x06 */
    float time;               /* 0x08 */
    float recipTime;          /* 0x0c */
    float position;           /* 0x10 */
} RpInterpolator;

/*
 * An RpAtomic: a geometry on a frame; it is in at most one clump (inClumpLink). object.privateFlags bit 0
 * says the world bounding sphere must be recomputed. collisionTag lets the collision code visit an atomic
 * once per query; llWorldSectorsInAtomic holds its ties to the world sectors.
 */
struct RpAtomic {
    RwObjectHasFrame object;               /* 0x00 type 1, flags bit 2: render, parent = frame */
    uint8_t stamp;                         /* 0x14 incremented when the atomic changes */
    uint8_t pad15[3];                      /* 0x15 */
    struct RwResEntry *repEntry;           /* 0x18 render resources */
    RpGeometry *geometry;                  /* 0x1c */
    RwSphere boundingSphere;               /* 0x20 in the frame's space */
    RwSphere worldBoundingSphere;          /* 0x30 */
    RpClump *clump;                        /* 0x40 */
    RwLLLink inClumpLink;                  /* 0x44 link in the clump's atomic list */
    RpAtomicRenderCallBack renderCallBack; /* 0x4c */
    RpInterpolator interpolator;           /* 0x50 */
    uint16_t renderFrame;                  /* 0x64 RwGlobals.renderFrame when it was last rendered */
    uint16_t collisionTag;                 /* 0x66 RwGlobals.lightFrame of the last query that visited it */
    RwLLLink llWorldSectorsInAtomic;       /* 0x68 RpTie links of the sectors it touches */
};

BMX_LAYOUT32_ASSERT(offsetof(RpAtomic, stamp) == 0x14, "RpAtomic layout");
BMX_LAYOUT32_ASSERT(offsetof(RpAtomic, repEntry) == 0x18, "RpAtomic layout");
BMX_LAYOUT32_ASSERT(offsetof(RpAtomic, geometry) == 0x1c, "RpAtomic layout");
BMX_LAYOUT32_ASSERT(offsetof(RpAtomic, boundingSphere) == 0x20, "RpAtomic layout");
BMX_LAYOUT32_ASSERT(offsetof(RpAtomic, worldBoundingSphere) == 0x30, "RpAtomic layout");
BMX_LAYOUT32_ASSERT(offsetof(RpAtomic, clump) == 0x40, "RpAtomic layout");
BMX_LAYOUT32_ASSERT(offsetof(RpAtomic, inClumpLink) == 0x44, "RpAtomic layout");
BMX_LAYOUT32_ASSERT(offsetof(RpAtomic, renderCallBack) == 0x4c, "RpAtomic layout");
BMX_LAYOUT32_ASSERT(offsetof(RpAtomic, interpolator) == 0x50, "RpAtomic layout");
BMX_LAYOUT32_ASSERT(offsetof(RpAtomic, collisionTag) == 0x66, "RpAtomic layout");
BMX_LAYOUT32_ASSERT(offsetof(RpAtomic, llWorldSectorsInAtomic) == 0x68, "RpAtomic layout");
_Static_assert(sizeof(RpInterpolator) == 0x14, "RpInterpolator layout");

/* An RpClump: atomics on a frame hierarchy (parent is the root frame). */
struct RpClump {
    RwObject object;      /* 0x00 type 2 */
    RwLLLink atomicList;  /* 0x08 */
    uint8_t pad10[2];     /* 0x10 */
    uint8_t stamp;        /* 0x12 incremented when the clump changes */
    uint8_t pad13;        /* 0x13 */
    RwLLLink inWorldLink; /* 0x14 in the world's list of clumps */
    void *callback;       /* 0x1c */
};

BMX_LAYOUT32_ASSERT(offsetof(RpClump, stamp) == 0x12, "RpClump layout");
BMX_LAYOUT32_ASSERT(offsetof(RpClump, callback) == 0x1c, "RpClump layout");

/* Atomic flags (RpAtomic.object.object.flags). */
enum {
    rpATOMICCOLLISIONTEST = 0x01,
    rpATOMICRENDER = 0x04,
};

/* The frame of a clump / an atomic (RenderWare's RpClumpGetFrame / RpAtomicGetFrame macros). */
static inline RwFrame *RpClumpGetFrame(const RpClump *clump)
{
    return clump->object.parent;
}

static inline RwFrame *RpAtomicGetFrame(const RpAtomic *atomic)
{
    return atomic->object.object.parent;
}

/* A bone as the skin code uses it: the matrix applied to its vertices and a flag word (bit 1: active). */
typedef struct RpSkinBone {
    RwFrame *frame;   /* 0x00 the bone's frame */
    RwMatrix invRest; /* 0x04 the inverse of the frame's rest pose, in the clump frame's space */
    RwMatrix matrix;  /* 0x44 */
    uint32_t flags;   /* 0x84 */
} RpSkinBone;

/* A bone block as the geometry chunk (0x10e) stores it. */
typedef struct RpSkinBoneBlockFile {
    int32_t firstVertex;
    int32_t numVertices;
    int32_t key;
} RpSkinBoneBlockFile;

_Static_assert(sizeof(RpSkinBoneBlockFile) == 12, "RpSkinBoneBlockFile layout");

/* A run of vertices driven by one bone; the list ends with a block whose numVertices is 0. */
typedef struct RpSkinBoneBlock {
    int32_t firstVertex;
    int32_t numVertices;
    int32_t key; /* identifies the bone's frame among the clump's frames */
    RpSkinBone *bone;
} RpSkinBoneBlock;

/* The bone plugin's data in a geometry (it holds a pointer to this): blocks and the rest pose. */
typedef struct RpSkinGeometry {
    RpSkinBoneBlock *blocks;
    ZVec3 *origVerts;
    ZVec3 *origNormals;
} RpSkinGeometry;

BMX_LAYOUT32_ASSERT(sizeof(RpSkinBoneBlock) == 0x10, "RpSkinBoneBlock layout");
BMX_LAYOUT32_ASSERT(sizeof(RpSkinBone) == 0x88, "RpSkinBone layout");
BMX_LAYOUT32_ASSERT(sizeof(RpSkinGeometry) == 0xc, "RpSkinGeometry layout");

/* The registries of the object types, in .data. */
#define rpClumpRegistry       ORIG_VAR(0x005667b0, RwPluginRegistry, rpClumpRegistry)
#define rpAtomicRegistry      ORIG_VAR(0x005667c4, RwPluginRegistry, rpAtomicRegistry)
#define rpGeometryRegistry    ORIG_VAR(0x005667e0, RwPluginRegistry, rpGeometryRegistry)
#define rpWorldRegistry       ORIG_VAR(0x00566820, RwPluginRegistry, rpWorldRegistry)
#define rpWorldSectorRegistry ORIG_VAR(0x00566860, RwPluginRegistry, rpWorldSectorRegistry)
#define rpLightRegistry       ORIG_VAR(0x00566880, RwPluginRegistry, rpLightRegistry)
#define rwFrameRegistry       ORIG_VAR(0x00566ee0, RwPluginRegistry, rwFrameRegistry)
#define rwCameraRegistry      ORIG_VAR(0x00566f50, RwPluginRegistry, rwCameraRegistry)
#define rwTextureRegistry     ORIG_VAR(0x00566f98, RwPluginRegistry, rwTextureRegistry)

/* Module open counts (plugins can only be registered while they are 0) and plugin offsets. */
#define rpGeometryModuleCount ORIG_VAR(0x00676724, int32_t, rpGeometryModuleCount)
#define rpWorldModuleCount    ORIG_VAR(0x00676764, int32_t, rpWorldModuleCount)
#define rpWorldCameraOffset   ORIG_VAR(0x00676738, int32_t, rpWorldCameraOffset)
#define rpWorldAtomicOffset   ORIG_VAR(0x0067673c, int32_t, rpWorldAtomicOffset)
#define rpWorldClumpOffset    ORIG_VAR(0x00676740, int32_t, rpWorldClumpOffset)
#define rpWorldLightOffset    ORIG_VAR(0x00676744, int32_t, rpWorldLightOffset)
#define rpPVSSectorOffset     ORIG_VAR(0x006966fc, int32_t, rpPVSSectorOffset)
#define rpPVSWorldOffset      ORIG_VAR(0x0069670c, int32_t, rpPVSWorldOffset)
#define rpAnimFrameOffset     ORIG_VAR(0x00678288, int32_t, rpAnimFrameOffset)
#define rpAnimClumpOffset     ORIG_VAR(0x0067828c, int32_t, rpAnimClumpOffset)
#define rpBoneFrameOffset     ORIG_VAR(0x00678290, int32_t, rpBoneFrameOffset)
#define rpBoneEngineOffset    ORIG_VAR(0x00678298, int32_t, rpBoneEngineOffset)
#define rpBoneModuleCount     ORIG_VAR(0x0067829c, int32_t, rpBoneModuleCount)
#define rpBoneStreamResult    ORIG_VAR(0x006782a0, int32_t, rpBoneStreamResult)
#define rpBoneGeometryOffset  ORIG_VAR(0x006782a4, int32_t, rpBoneGeometryOffset)
#define rpLodEngineOffset     ORIG_VAR(0x00697370, int32_t, rpLodEngineOffset)
#define rpLodAtomicOffset     ORIG_VAR(0x00697374, int32_t, rpLodAtomicOffset)
#define rpLodCamera           ORIG_VAR(0x00697378, void *, rpLodCamera)

/* Register a plugin on an object type: data size, plugin id and constructor/destructor/copy callbacks. */
int32_t RpAtomicRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int32_t RpClumpRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int32_t RpGeometryRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int32_t RpWorldRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int32_t RpWorldSectorRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                                    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int32_t RpLightRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int32_t RwFrameRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int32_t RwCameraRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int32_t RwTextureRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                                RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int32_t RwEngineRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB);

/* Stream callbacks of an already registered plugin. */
int32_t RpAtomicRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                     RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB);
int32_t RpClumpRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                    RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB);
int32_t RpGeometryRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                       RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB);
int32_t RpWorldRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                    RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB);
int32_t RpWorldSectorRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                          RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB);
int32_t RwFrameRegisterPluginStream(int32_t pluginID, RwPluginDataChunkRead readCB,
                                    RwPluginDataChunkWrite writeCB, RwPluginDataChunkGetSize getSizeCB);

/* Attach routines: 1 on success, 0 if any registration failed. */
int CorePluginAttach(void);
int _rpWorldObjRegisterExtensions(void);
int RpWorldPluginAttach(void);
int RpPVSPluginAttach(void);
int RpAnimPluginAttach(void);
int RpBonePluginAttach(void);
int RpLODAtomicPluginAttach(void);

/* The vtable-style world extension attach (calls the first entry of the table at 0x69b520). */
void **rpWorldExtGetFuncs(void);
int rpWorldExtPluginAttach(void);

/* Geometry morph targets (the bounding sphere and normals of morph target 0). */
RpMorphTarget *RpMorphTargetSetBoundingSphere(RpMorphTarget *morphTarget, const RwSphere *sphere);
ZVec3 *RpMorphTargetGetVertexNormals(const RpMorphTarget *morphTarget);

/* Frame helpers. */
RwFrame *RwFrameForAllObjects(RwFrame *frame, RwObjectCallBack callback, void *data);
uint32_t RwFrameGetUserData(const RwFrame *frame);
void RwFrameSetUserData(RwFrame *frame, uint32_t data);
void RpLODAtomicSetCamera(void *camera);

/* Vector length by the engine's square root table. */
float RwV3dLength(const ZVec3 *vector);

/* Skin plugin: pose the clump's skinned geometry from its bones; copy the rest pose and find bone frames. */
int RpSkinClumpUpdate(RpClump *clump);
RpClump *RpSkinClumpSetup(RpClump *clump);
void RpSkinFindBoneFrames(RpSkinBoneBlock *blocks, RwFrame *frame);

/* The bone plugin's frame walk callbacks, bone data of frames and plugin callbacks (rw/plugins.c). */
void *rpBoneFindAtomic(void *object, void *found);
void *rpBoneFindSkinAtomic(void *object, void *found);
__attribute__((regparm(1))) RpSkinBone *rpBoneGet(RwFrame *frame);
__attribute__((regparm(2))) RwFrame *rpBoneSet(RwFrame *frame, RpSkinBone *bone);
RpSkinBone *rpBoneCreate(void);
__attribute__((regparm(1))) void rpBoneDestroy(RpSkinBone *bone);
void *rpBoneFindFrame(void *frame, void *search);
void *rpBoneAttachFrames(void *frame, void *reference);
void *rpBoneEngineOpen(void *engine, int32_t offset);
void *rpBoneEngineClose(void *engine);
void *rpBoneGeometryInit(void *geometry);
void *rpBoneGeometryDeinit(void *geometry);
void *rpBoneGeometryCopy(void *destination);
void *rpBoneGeometryWrite(void *stream, int32_t length, const void *geometry);
int32_t rpBoneGeometrySize(const void *geometry);
void *rpBoneFrameInit(void *frame);
void *rpBoneFrameDeinit(void *frame);
void *rpBoneFrameCopy(void *destination, const void *source);
void *rpAnimInterpolatorDeinit(void *interpolator);
void *rpAnimInterpolatorInit(void *interpolator);
void *rpAnimFrameInit(void *frame);
__attribute__((regparm(1))) const char *rpAnimSequenceName(const void *sequence);
uint32_t rpAnimSequenceType(const uint32_t *sequence);
__attribute__((regparm(1))) int32_t rpAnimSequenceSize(const uint32_t *words);
int32_t rpAnimFrameChunkSize(const void *frame);
void *rpAnimClumpAddFrameSequence(void *sequence, void *clump);
__attribute__((regparm(2))) void *rpAnimClumpAddFrameSequences(void *frame, void *clump);
void *rpAnimClumpAddFrameSequencesCallback(void *frame, void *clump);
__attribute__((regparm(1))) RpClump *rpAnimClumpAddAllSequences(RpClump *clump);
void *rpAnimClumpChunkRead(void *stream, int32_t length, RpClump *clump);
void *rpAnimClumpChunkWrite(void *stream, int32_t length, const void *clump);
int32_t rpAnimClumpChunkSize(const void *clump);
void *rpAnimClumpCopy(void *destination, const void *source);

/* Matrix inverse helpers of RwMatrixInvert (register arguments: EAX = out, EDX = in). */
__attribute__((regparm(2))) RwMatrix *RwMatrixInvertOrthonormal(RwMatrix *out, const RwMatrix *in);
__attribute__((regparm(2))) RwMatrix *RwMatrixInvertGeneral(RwMatrix *out, const RwMatrix *in);

/* The animation plugin's engine module (rw/plugins.c). */
void *WorldObjectOpen(void *engine, int32_t offset);
void *WorldObjectClose(void *engine);
/* Adds the sequence `name` of the clump's frames to the clump (EAX = clump, EDX = name, ECX = play callback
 * or NULL, the data in the 4th stack slot). NULL when it exists already, no frame has it, or out of memory.
 */
__attribute__((regparm(3))) RpClump *rpAnimClumpAddSequence(RpClump *clump, const char *name, void *callBack,
                                                            uint32_t unused0, uint32_t unused1,
                                                            uint32_t unused2, void *data);

/* ---- LOD atomic plugin (rw/plugins.c) ---------------------------------------------------------------- */

struct RwStream;
#define rpLodFreeList ORIG_VAR(0x0069737c, RwFreeList *, rpLodFreeList)

/*
 * What the plugin keeps for an atomic (a pointer in the atomic at rpLodAtomicOffset, from a free list of 0x40
 * byte entries): up to ten geometries by distance, the one in use, and the atomic's own render callback while
 * the plugin's replaces it. `select` returns the level for an atomic (-1: keep the current one).
 */
typedef int32_t(BMX_CDECL *RpLodSelectCallBack)(RpAtomic *atomic);

/*
 * The plugin's stream chunk (0x112): the data block as the original kept it in memory, pointers as 32-bit
 * values. Readers only test `levels[i]` for NULL (a geometry follows); the callbacks are replaced on reading.
 */
typedef struct RpLodAtomicFile {
    int32_t hooked;       /* 0x00 */
    uint32_t savedRender; /* 0x04 */
    uint32_t select;      /* 0x08 */
    float range;          /* 0x0c */
    float scale;          /* 0x10 */
    int32_t current;      /* 0x14 */
    uint32_t levels[10];  /* 0x18 */
} RpLodAtomicFile;

_Static_assert(sizeof(RpLodAtomicFile) == 0x40, "RpLodAtomicFile layout");

typedef struct RpLodAtomicData {
    int32_t hooked;                     /* 0x00 1 while the atomic's render callback is the plugin's */
    RpAtomicRenderCallBack savedRender; /* 0x04 */
    RpLodSelectCallBack select;         /* 0x08 */
    float range;                        /* 0x0c */
    float scale;                        /* 0x10 distance multiplier of the default select callback */
    int32_t current;                    /* 0x14 */
    RpGeometry *levels[10];             /* 0x18 */
} RpLodAtomicData;

BMX_LAYOUT32_ASSERT(sizeof(RpLodAtomicData) == 0x40, "RpLodAtomicData layout");

int32_t rpLodAtomicSelectLevel(RpAtomic *atomic);
__attribute__((regparm(2))) RpAtomic *rpLodAtomicApply(RpAtomic *atomic, RpLodAtomicData *lod);
RpAtomic *rpLodAtomicRender(RpAtomic *atomic);
__attribute__((regparm(2))) void rpLodAtomicHook(RpAtomic *atomic, RpLodAtomicData *lod);
__attribute__((regparm(2))) void rpLodAtomicUnhook(RpAtomic *atomic, RpLodAtomicData *lod);
RpLodAtomicData *rpLodAtomicDataInit(RpLodAtomicData *lod);
RpLodAtomicData *rpLodAtomicDataCreate(void);
__attribute__((regparm(2))) RpLodAtomicData *rpLodAtomicDataCopy(RpLodAtomicData *dst, RpLodAtomicData *src);
__attribute__((regparm(1))) void rpLodAtomicDataDestroy(RpLodAtomicData *lod);
void *LODAtomicCallBackOpen(void *engine, int32_t offset);
void *LODAtomicCallBackClose(void *engine);
void *LODAtomicCallBackConstructor(void *atomic, int32_t offset);
void *LODAtomicCallBackDestructor(void *atomic, int32_t offset);
void *LODAtomicCallBackCopy(void *dst, const void *src, int32_t offset);
void *LODAtomicCallBackReadChunk(struct RwStream *stream, int32_t length, void *atomic, int32_t offset,
                                 int32_t size);
void *LODAtomicCallBackWriteChunk(struct RwStream *stream, int32_t length, const void *atomic, int32_t offset,
                                  int32_t size);
int32_t LODAtomicCallBackGetChunkSize(const void *atomic, int32_t offset, int32_t size);
void *rwPngMalloc(void *png, size_t size);
void rwPngFree(void *png, void *block);
uint32_t rwPngRead(void *png, void *buffer, uint32_t length);

#endif
