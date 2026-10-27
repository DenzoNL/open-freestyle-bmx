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

_Static_assert(offsetof(RpMorphTarget, boundingSphere) == 4, "RpMorphTarget layout");
_Static_assert(offsetof(RpMorphTarget, normals) == 0x18, "RpMorphTarget layout");

/* A triangle of a geometry: three vertex indices and the material. */
typedef struct RpTriangle {
    uint16_t vertIndex[3];
    int16_t matIndex;
} RpTriangle;

/* Partial RpGeometry: the triangles and the morph targets (each with its own vertices). */
typedef struct RpGeometry {
    uint8_t pad00[0x1c];
    int32_t numTriangles;       /* 0x1c */
    int32_t numVertices;        /* 0x20 */
    int32_t numMorphTargets;    /* 0x24 */
    RpTriangle *triangles;      /* 0x28 */
    uint8_t pad2c[0x18];        /* 0x2c */
    RpMorphTarget *morphTarget; /* 0x44 */
} RpGeometry;

_Static_assert(offsetof(RpGeometry, triangles) == 0x28, "RpGeometry layout");
_Static_assert(offsetof(RpGeometry, morphTarget) == 0x44, "RpGeometry layout");

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
 * Partial RpAtomic: its frame link (RwObjectHasFrame), geometry, bounding spheres, morph interpolator, the
 * tag the collision code uses to visit an atomic once per query and its ties to the world sectors.
 * object.privateFlags bit 0 says the world bounding sphere must be recomputed.
 */
typedef struct RpAtomic {
    RwObjectHasFrame object;         /* 0x00 */
    uint8_t pad14[8];                /* 0x14 */
    void *geometry;                  /* 0x1c */
    RwSphere boundingSphere;         /* 0x20 in the frame's space */
    RwSphere worldBoundingSphere;    /* 0x30 */
    uint8_t pad40[4];                /* 0x40 */
    RwLLLink inClumpLink;            /* 0x44 link in the clump's atomic list */
    uint8_t pad4c[4];                /* 0x4c */
    RpInterpolator interpolator;     /* 0x50 */
    uint8_t pad64[2];                /* 0x64 */
    uint16_t collisionTag;           /* 0x66 RwGlobals.lightFrame of the last query that visited it */
    RwLLLink llWorldSectorsInAtomic; /* 0x68 RpTie links of the sectors it touches */
} RpAtomic;

_Static_assert(offsetof(RpAtomic, geometry) == 0x1c, "RpAtomic layout");
_Static_assert(offsetof(RpAtomic, boundingSphere) == 0x20, "RpAtomic layout");
_Static_assert(offsetof(RpAtomic, inClumpLink) == 0x44, "RpAtomic layout");
_Static_assert(offsetof(RpAtomic, interpolator) == 0x50, "RpAtomic layout");
_Static_assert(offsetof(RpAtomic, collisionTag) == 0x66, "RpAtomic layout");
_Static_assert(offsetof(RpAtomic, llWorldSectorsInAtomic) == 0x68, "RpAtomic layout");

/* Partial RpClump: parent is its frame. */
typedef struct RpClump {
    RwObject object;
    RwLLLink atomicList; /* 0x08 */
} RpClump;

/* A bone as the skin code uses it: the matrix applied to its vertices and a flag word (bit 1: active). */
typedef struct RpSkinBone {
    uint8_t pad00[0x44];
    RwMatrix matrix; /* 0x44 */
    uint32_t flags;  /* 0x84 */
} RpSkinBone;

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

_Static_assert(sizeof(RpSkinBoneBlock) == 0x10, "RpSkinBoneBlock layout");
_Static_assert(sizeof(RpSkinGeometry) == 0xc, "RpSkinGeometry layout");

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
#define rpBoneStreamResult    ORIG_VAR(0x006782a0, int32_t, rpBoneStreamResult)
#define rpBoneGeometryOffset  ORIG_VAR(0x006782a4, int32_t, rpBoneGeometryOffset)
#define rpLodEngineOffset     ORIG_VAR(0x00697370, int32_t, rpLodEngineOffset)
#define rpLodAtomicOffset     ORIG_VAR(0x00697374, int32_t, rpLodAtomicOffset)
#define rpLodCamera           ORIG_VAR(0x00697378, void *, rpLodCamera)
#define rpLodReserved         ORIG_VAR(0x0069737c, int32_t, rpLodReserved)

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

/* Matrix inverse helpers of RwMatrixInvert (register arguments: EAX = out, EDX = in). */
__attribute__((regparm(2))) RwMatrix *RwMatrixInvertOrthonormal(RwMatrix *out, const RwMatrix *in);
__attribute__((regparm(2))) RwMatrix *RwMatrixInvertGeneral(RwMatrix *out, const RwMatrix *in);

#endif
