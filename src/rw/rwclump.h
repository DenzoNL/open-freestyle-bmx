/*
 * RenderWare 3.1 clumps, atomics, geometries, materials and the binary world/sector readers (rw/clump.c).
 *
 * Game code reads geometry, morph target and material fields directly, so the layouts are RenderWare's.
 * Functions the original takes its first arguments in registers for (internal helpers that were not
 * exported) are declared regparm(n) like the other RW slices. The world, sector and material types are in
 * rw/rwworld.h, RpAtomic and RpClump in rw/rwplugins.h.
 */
#ifndef RW_RWCLUMP_H
#define RW_RWCLUMP_H

#include "rw/rwmath.h"
#include "rw/rwstream.h"
#include "rw/rwtexture.h"
#include "rw/rwworld.h"

/* The per-object data of the clump/atomic module (engine plugin data): the two free lists. */
typedef struct RpClumpGlobals {
    struct RwIm3DPipeline *pipeline; /* 0x00 the render pipeline of atomics */
    RwFreeList *atomicFreeList;      /* 0x04 */
    RwFreeList *clumpFreeList;       /* 0x08 */
} RpClumpGlobals;

#define rpClumpModuleOffset ORIG_VAR(0x00676710, int32_t, rpClumpModuleOffset)
#define rpGeometryOffset    ORIG_VAR(0x00676720, int32_t, rpGeometryOffset)
#define rpMaterialOffset    ORIG_VAR(0x00676750, int32_t, rpMaterialOffset)
#define rpMeshOffset        ORIG_VAR(0x006966e8, int32_t, rpMeshOffset)
#define rpMaterialRegistry  ORIG_VAR(0x00566800, RwPluginRegistry, rpMaterialRegistry)

/* A triangle: three vertex indices and a material index (-1 = none). */
typedef struct RpTriangle {
    uint16_t vertIndex[3];
    int16_t matIndex;
} RpTriangle;

/* Format flags of a geometry (RpGeometry.flags). */
enum {
    rpGEOMETRYTRISTRIP = 0x01,
    rpGEOMETRYPOSITIONS = 0x02,
    rpGEOMETRYTEXTURED = 0x04,
    rpGEOMETRYPRELIT = 0x08,
    rpGEOMETRYNORMALS = 0x10,
    rpGEOMETRYLIGHT = 0x20,
    rpGEOMETRYMODULATEMATERIALCOLOR = 0x40,
};

/* The instance list of triangles of one material in a geometry (a mesh header holds these). */
typedef struct RpMesh {
    uint16_t *indices;    /* 0x00 */
    uint32_t numIndices;  /* 0x04 */
    RpMaterial *material; /* 0x08 */
} RpMesh;

typedef struct RpMeshHeader {
    uint32_t flags;           /* 0x00 1 = strips */
    uint16_t numMeshes;       /* 0x04 */
    uint16_t serialNum;       /* 0x06 */
    uint32_t totalIndices;    /* 0x08 */
    uint32_t firstMeshOffset; /* 0x0c */
} RpMeshHeader;

_Static_assert(sizeof(RpMesh) == 0xc, "RpMesh layout");
_Static_assert(sizeof(RpMeshHeader) == 0x10, "RpMeshHeader layout");

/* A geometry: counts, triangles, per vertex data and its morph targets (each with its own vertices). */
struct RpGeometry {
    uint8_t type;                     /* 0x00 8 */
    uint8_t subType;                  /* 0x01 */
    uint8_t flags;                    /* 0x02 rpGEOMETRY* */
    uint8_t privateFlags;             /* 0x03 */
    uint32_t pad04;                   /* 0x04 */
    uint8_t stamp;                    /* 0x08 incremented when the geometry changes */
    uint8_t pad09[3];                 /* 0x09 */
    uint16_t lockedSinceLastInst;     /* 0x0c lock mode bits since the mesh was built */
    uint16_t refCount;                /* 0x0e */
    RpMaterialList matList;           /* 0x10 */
    int32_t numTriangles;             /* 0x1c */
    int32_t numVertices;              /* 0x20 */
    int32_t numMorphTargets;          /* 0x24 */
    RpTriangle *triangles;            /* 0x28 */
    uint32_t *preLitLum;              /* 0x2c */
    RwTexCoords *texCoords;           /* 0x30 */
    RwSurfaceProperties surfaceProps; /* 0x34 */
    RpMeshHeader *mesh;               /* 0x40 */
    RpMorphTarget *morphTarget;       /* 0x44 */
};

_Static_assert(offsetof(RpGeometry, refCount) == 0x0e, "RpGeometry layout");
_Static_assert(offsetof(RpGeometry, matList) == 0x10, "RpGeometry layout");
_Static_assert(offsetof(RpGeometry, numTriangles) == 0x1c, "RpGeometry layout");
_Static_assert(offsetof(RpGeometry, triangles) == 0x28, "RpGeometry layout");
_Static_assert(offsetof(RpGeometry, surfaceProps) == 0x34, "RpGeometry layout");
_Static_assert(offsetof(RpGeometry, morphTarget) == 0x44, "RpGeometry layout");

/* ---- clumps and atomics ----------------------------------------------------------------------------- */

/* Shared state of the clump clone callback: the source, the new clump and whether all atomics cloned. */
typedef struct RwClumpCloneData {
    RpClump *source;
    RpClump *clone;
    int32_t ok;
} RwClumpCloneData;

RpAtomic *rwClumpCountAtomic(RpAtomic *atomic, int32_t *count);
int32_t RpClumpGetNumAtomics(RpClump *clump);
RpClump *RpClumpRender(RpClump *clump);
/* Also declared in rwmath.h. */
// NOLINTNEXTLINE(readability-redundant-declaration)
RpClump *RpClumpForAllAtomics(RpClump *clump, RwObjectCallBack callback, void *data);
__attribute__((regparm(1))) int RpAtomicDestroy(RpAtomic *atomic);
__attribute__((regparm(1))) RpAtomic *RpAtomicClone(RpAtomic *source);
RpAtomic *rwClumpCloneAtomic(RpAtomic *atomic, RwClumpCloneData *data);
RpClump *RpClumpClone(RpClump *source);
RpAtomic *rwClumpDestroyAtomic(RpAtomic *atomic, void *data);
int RpClumpDestroy(RpClump *clump);
int rpWorldExtAttachNothing(void);
__attribute__((regparm(1))) int rpRasterFormatHasAlpha(uint32_t format);
int rpMeshCompareMaterials(const RpMesh *const *a, const RpMesh *const *b);
/*
 * Vertex type and size (bytes) of the instancing pipelines' vertices, by rpGEOMETRYTEXTURED / the world's
 * textured flag (index 0 or 4): rpWorldPipelineInit fills the first pair, which the atomic instance stage
 * (0x4c8e30) reads, rpAtomicPipelineInit the second, which the world sector instance stage (0x4c92d0) reads.
 */
#define rpPipelineTypes  ((uint8_t *)(uintptr_t)0x006767f0)
#define rpPipelineSizes  ((uint8_t *)(uintptr_t)0x006767f8)
#define rpPipelineSizesB ((uint8_t *)(uintptr_t)0x00676800)
#define rpPipelineTypesB ((uint8_t *)(uintptr_t)0x00676808)
int rpWorldPipelineInit(struct RwIm3DPipeline *pipeline);
int rpAtomicPipelineInit(struct RwIm3DPipeline *pipeline);
RpAtomic *RpAtomicSetGeometry(RpAtomic *atomic, RpGeometry *geometry, uint32_t flags);
__attribute__((regparm(1))) void rpAtomicUpdateBoundingSphere(RpAtomic *atomic);
RpAtomic *rpAtomicSyncCallback(RpAtomic *atomic);
RpAtomic *rpAtomicRenderCallback(RpAtomic *atomic);
__attribute__((regparm(1))) int rpClumpEntryDestroy(RpClump *clump);
int rpClumpFreeListDestroyCB(void *clump, void *data);
void rpAtomicFreeListDestroyCB(void *atomic, void *data);
void *rpClumpDefaultCallback(void *clump);
void *_rpClumpOpen(void *engine, int32_t offset);
void *_rpClumpClose(void *engine);
void *_rpGeometryOpen(void *engine, int32_t offset);
void rpGeometryFreeListDestroyCB(void *geometry, void *data);
void *_rpGeometryClose(void *engine);
void *_rpMeshOpen(void *engine, int32_t offset);
void *_rpMeshClose(void *engine);
__attribute__((regparm(2))) void RpClumpAddAtomic(RpClump *clump, RpAtomic *atomic);
RpClump *RpClumpStreamRead(RwStream *stream);
__attribute__((regparm(2))) RpAtomic *RpAtomicStreamRead(RwStream *stream, RwFrameList *frameList);
__attribute__((regparm(2))) RpAtomic *RpAtomicSetFrame(RpAtomic *atomic, RwFrame *frame);

/* ---- geometries ------------------------------------------------------------------------------------- */

RpMorphTarget *RpGeometryGetMorphTarget(RpGeometry *geometry, int32_t index);
RwTexCoords *RpGeometryGetTexCoords(RpGeometry *geometry);
int32_t RpGeometryGetNumVertices(const RpGeometry *geometry);
ZVec3 *RpMorphTargetGetVertices(const RpMorphTarget *morphTarget);
RpTriangle *RpGeometryGetTriangles(RpGeometry *geometry);
int32_t RpGeometryGetNumTriangles(const RpGeometry *geometry);
RpMaterial *RpGeometryGetMaterial(RpGeometry *geometry, int32_t index);
int32_t RpGeometryGetNumMaterials(const RpGeometry *geometry);
RpGeometry *RpGeometryLock(RpGeometry *geometry, int32_t lockMode);
RpGeometry *RpGeometryUnlock(RpGeometry *geometry);
__attribute__((regparm(3))) RpGeometry *RpGeometryCreate(int32_t numVertices, int32_t numTriangles,
                                                         uint32_t format);
RpGeometry *RpGeometryAddRef(RpGeometry *geometry);
int RpGeometryDestroy(RpGeometry *geometry);
uint32_t RpGeometryGetFlags(const RpGeometry *geometry);
RpGeometry *RpGeometrySetFlags(RpGeometry *geometry, uint32_t flags);
RpGeometry *RpGeometryStreamRead(RwStream *stream);
int32_t RpGeometryStreamGetSize(const RpGeometry *geometry);
RpGeometry *RpGeometryStreamWrite(RpGeometry *geometry, RwStream *stream);

/* ---- materials and material lists ------------------------------------------------------------------- */

RpMaterial *RpMaterialAddRef(RpMaterial *material);
int RpMaterialDestroy(RpMaterial *material);
int32_t RpMaterialStreamGetSize(const RpMaterial *material);
RpMaterial *RpMaterialStreamWrite(RpMaterial *material, RwStream *stream);
RpMaterial *RpMaterialStreamRead(RwStream *stream);
RpMaterial *_rpMaterialListGetMaterial(const RpMaterialList *list, int32_t index);
RpMaterialList *_rpMaterialListInitialize(RpMaterialList *list);
RpMaterialList *_rpMaterialListDeinitialize(RpMaterialList *list);
int32_t _rpMaterialListStreamGetSize(const RpMaterialList *list);
RpMaterialList *_rpMaterialListStreamWrite(const RpMaterialList *list, RwStream *stream);
RpMaterialList *_rpMaterialListStreamRead(RwStream *stream, RpMaterialList *list);

/* Plugin registry helpers used by the readers and writers. */
RwPluginRegistry *_rwPluginRegistryCopyObject(RwPluginRegistry *registry, void *destination,
                                              const void *source);
int32_t _rwPluginRegistryGetSize(const RwPluginRegistry *registry, const void *object);

/* Textures in streams (the string chunks of a texture chunk). */
int32_t RwTextureStreamGetSize(const RwTexture *texture);
__attribute__((regparm(2))) const char *rwStreamWriteString(const char *string, RwStream *stream);
__attribute__((regparm(1))) char *rwStreamReadString(RwStream *stream);
RwTexture *RwTextureStreamWrite(RwTexture *texture, RwStream *stream);
RwTexture *RwTextureStreamRead(RwStream *stream);
int32_t RwTextureGetMipmapping(void);
int32_t RwTextureGetFilterMode(const RwTexture *texture);
int32_t RwTextureGetAddressing(const RwTexture *texture);

/* ---- meshes ----------------------------------------------------------------------------------------- */

typedef struct RpBuildMeshTriangle {
    uint16_t vertIndex[3]; /* 0x00 */
    uint16_t pad06;        /* 0x06 */
    RpMaterial *material;  /* 0x08 */
} RpBuildMeshTriangle;

/* The triangles of a geometry or sector on their way to a mesh header. */
typedef struct RpBuildMesh {
    uint32_t capacity;              /* 0x00 triangles that fit */
    uint32_t numTriangles;          /* 0x04 */
    RpBuildMeshTriangle *triangles; /* 0x08 */
} RpBuildMesh;

_Static_assert(sizeof(RpBuildMeshTriangle) == 0xc, "RpBuildMeshTriangle layout");

typedef int(BMX_CDECL *RpMeshCallBack)(RpMesh *mesh, RpMeshHeader *header, void *data);

RpBuildMesh *RpBuildMeshCreate(uint32_t numTriangles);
int RpBuildMeshDestroy(RpBuildMesh *build);
int _rpMeshHeaderDestroy(RpMeshHeader *header);
RpBuildMesh *_rpBuildMeshAddTriangle(RpBuildMesh *build, RpMaterial *material, uint16_t v0, uint16_t v1,
                                     uint16_t v2);
RpMeshHeader *_rpMeshHeaderForAllMeshes(RpMeshHeader *header, RpMeshCallBack callback, void *data);
RpMeshHeader *_rpMeshGenerateTriList(RpBuildMesh *build, uint32_t flags);
RpMeshHeader *_rpMeshGenerate(RpBuildMesh *build, uint32_t flags);

/* ---- the world (binary world and sector reader/writer) ----------------------------------------------- */

void rpWorldRegister(RpWorld *world, uint32_t size);
void rpWorldUnregister(RpWorld *world);
RpWorld *RpWorldSetSurfaceProperties(RpWorld *world, const RwSurfaceProperties *surfaceProps);
const RwSurfaceProperties *RpWorldGetSurfaceProperties(const RpWorld *world);
RpWorld *RpWorldSetKind(RpWorld *world, int32_t kind);
RpWorld *RpWorldSetRenderCallBack(RpWorld *world, RpWorldSectorCallBackRender callback);
RpWorld *RpWorldSetFlags(RpWorld *world, uint32_t flags);
uint8_t RpWorldGetFlags(const RpWorld *world);
int rwWorldCalcBBox(RpWorld *world, RwBBox *bbox);
int32_t rpCollisionTreeSize(const uint8_t *tree);
RpWorld *_rpWorldGenerateSectorMeshes(RpWorld *world);

/* Sector and plane readers/writers: register arguments (EAX, EDX, ECX), then stack slots of which only the
 * fourth (the format flags) is read; the three pads stand for the slots the original never reads. */
__attribute__((regparm(3))) int32_t rpSectorDataSize(const RpWorldSector *sector, int32_t unused,
                                                     uint32_t flags);
__attribute__((regparm(3))) int32_t rpSectorStreamGetSize(const RpWorldSector *sector, int32_t unused,
                                                          uint32_t flags);
__attribute__((regparm(3))) int32_t rpBspStreamGetSize(const RpPlaneSector *node, int32_t unused,
                                                       uint32_t flags);
__attribute__((regparm(3))) RpWorldSector *rpSectorStreamWrite(const RpWorldSector *sector, RwStream *stream,
                                                               int32_t unused, int32_t pad0, int32_t pad1,
                                                               int32_t pad2, uint32_t flags);
__attribute__((regparm(3))) RpPlaneSector *rpBspStreamWrite(const RpPlaneSector *node, RwStream *stream,
                                                            int32_t world, int32_t pad0, int32_t pad1,
                                                            int32_t pad2, uint32_t flags);
__attribute__((regparm(3))) RpWorldSector *rpSectorStreamRead(RwStream *stream, uint8_t **cursor,
                                                              int32_t world, int32_t pad0, int32_t pad1,
                                                              int32_t pad2, uint32_t flags);
__attribute__((regparm(3))) RpPlaneSector *rpBspStreamRead(RwStream *stream, uint8_t **cursor, int32_t world,
                                                           int32_t pad0, int32_t pad1, int32_t pad2,
                                                           uint32_t flags);
RpWorld *RpWorldStreamWrite(RpWorld *world, RwStream *stream);
RpWorld *RpWorldStreamRead(RwStream *stream);

/* A list of 16 bit triangle indices (three per triangle) and its length in indices. */
typedef struct RwIndexList {
    uint16_t *indices;
    uint32_t count;
} RwIndexList;

RwIndexList *rpMeshSeparateTriangles(RwIndexList *list);
/* Zeroes size bytes (EAX = buffer, EDX unused, ECX = size) and returns the buffer. */
__attribute__((regparm(3))) void *rwZeroBytes(void *buffer, int32_t unused, uint32_t size);

#endif
