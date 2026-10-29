/*
 * RenderWare 3.1 world (rw/world.c): the world with its BSP of plane sectors and world sectors, the clumps,
 * lights and cameras attached to it, materials, and the PVS plugin.
 *
 * The world keeps three kinds of links to the objects around it: the ties between atomics and sectors
 * (RpTie), between lights and sectors (RpLightTie) and the camera's list of sectors in its view. The object
 * a plugin extends stores its link at the offset the plugin registered (rpWorldClumpOffset and friends).
 */
#ifndef RW_RWWORLD_H
#define RW_RWWORLD_H

#include "rw/rwcamera.h"
#include "rw/rwlight.h"
#include "rw/rwmath.h"
#include "rw/rwstream.h"
#include "rw/rwtexture.h"

/* A node of the world BSP: type < 0 is a world sector (-1 owns its data, -2 is a stub), otherwise a plane. */
typedef struct RpSector {
    int32_t type; /* 0x00 plane sectors: byte offset of the split axis in an RwV3d (0, 4, 8) */
} RpSector;

/*
 * Plane sector. The children overlap: everything below leftValue is in the left subtree, everything above
 * rightValue in the right one. The walks compare these values with coordinates as integers (the bit
 * patterns), which orders non-negative floats correctly.
 */
typedef struct RpPlaneSector {
    int32_t type;           /* 0x00 */
    float value;            /* 0x04 */
    RpSector *leftSubTree;  /* 0x08 */
    RpSector *rightSubTree; /* 0x0c */
    float leftValue;        /* 0x10 */
    float rightValue;       /* 0x14 */
} RpPlaneSector;

/* A polygon of a world sector: material and three vertex indices, one byte each. */
typedef struct RpPolygon {
    uint8_t matIndex;
    uint8_t vertIndex[3];
} RpPolygon;

/* A texture coordinate pair (sets of them are stored back to back per vertex). */
typedef struct RwTexCoords {
    float u;
    float v;
} RwTexCoords;

/* Ambient, specular and diffuse reflection factors of a world, geometry or material. */
typedef struct RwSurfaceProperties {
    float ambient;
    float specular;
    float diffuse;
} RwSurfaceProperties;

struct RpMeshHeader; /* rw/rwclump.h */

/*
 * World sector (a BSP leaf). colSectorRoot is the polygon split tree of the collision code: nodes of three
 * bytes (flags/count, split vertex, first polygon), node n has the children 2n and 2n + 1, node 1 is the
 * root. The three lists hold the ties of the atomics with and without collision and of the lights.
 */
typedef struct RpWorldSector {
    int32_t type;                        /* 0x00 negative */
    uint16_t matListWindowBase;          /* 0x04 first world material the polygons index */
    uint16_t numVertices;                /* 0x06 */
    uint16_t numPolygons;                /* 0x08 */
    uint8_t stamp;                       /* 0x0a */
    uint8_t pad0b;                       /* 0x0b */
    RpPolygon *polygons;                 /* 0x0c */
    RwV3d *vertices;                     /* 0x10 */
    uint32_t *normals;                   /* 0x14 packed, four bytes each */
    RwTexCoords *texCoords;              /* 0x18 */
    uint32_t *preLitLum;                 /* 0x1c */
    RwResEntry *repEntry;                /* 0x20 cached render data */
    RwLLLink collAtomicsInWorldSector;   /* 0x24 RpTie links of the atomics touching the sector */
    RwLLLink noCollAtomicsInWorldSector; /* 0x2c */
    RwLLLink lightsInWorldSector;        /* 0x34 RpLightTie links */
    RwBBox boundingBox;                  /* 0x3c */
    const uint8_t *colSectorRoot;        /* 0x54 NULL: no polygons to test */
    struct RpMeshHeader *mesh;           /* 0x58 render mesh built from the polygons */
} RpWorldSector;

_Static_assert(offsetof(RpWorldSector, polygons) == 0x0c, "RpWorldSector layout");
_Static_assert(offsetof(RpWorldSector, texCoords) == 0x18, "RpWorldSector layout");
_Static_assert(offsetof(RpWorldSector, preLitLum) == 0x1c, "RpWorldSector layout");
_Static_assert(offsetof(RpWorldSector, repEntry) == 0x20, "RpWorldSector layout");
_Static_assert(offsetof(RpWorldSector, boundingBox) == 0x3c, "RpWorldSector layout");
_Static_assert(offsetof(RpWorldSector, collAtomicsInWorldSector) == 0x24, "RpWorldSector layout");
_Static_assert(offsetof(RpWorldSector, lightsInWorldSector) == 0x34, "RpWorldSector layout");
_Static_assert(offsetof(RpWorldSector, colSectorRoot) == 0x54, "RpWorldSector layout");
_Static_assert(offsetof(RpWorldSector, mesh) == 0x58, "RpWorldSector layout");

/* Links an atomic and a world sector it touches (in both of their lists). */
typedef struct RpTie {
    RwLLLink lAtomicInWorldSector; /* 0x00 in RpWorldSector.collAtomicsInWorldSector or the other list */
    RpAtomic *apAtom;              /* 0x08 */
    RwLLLink lWorldSectorInAtomic; /* 0x0c in the atomic's list of sectors */
    RpWorldSector *worldSector;    /* 0x14 */
} RpTie;

/* The same for a light: in the sector's light list and in the light's list of sectors (RpLight.sectors). */
typedef struct RpLightTie {
    RwLLLink lightInWorldSector; /* 0x00 */
    RpLight *light;              /* 0x08 */
    RwLLLink worldSectorInLight; /* 0x0c */
    RpWorldSector *worldSector;  /* 0x14 */
} RpLightTie;

_Static_assert(sizeof(RpTie) == 0x18, "RpTie layout");
_Static_assert(sizeof(RpLightTie) == 0x18, "RpLightTie layout");

/* RpMaterial: texture, colour, reference count, a change stamp and the surface properties. */
typedef struct RpMaterial {
    RwTexture *texture;               /* 0x00 */
    RwRGBA color;                     /* 0x04 */
    uint16_t refCount;                /* 0x08 */
    uint8_t stamp;                    /* 0x0a bumped by every setter (1 for the colour, 2 for the texture) */
    uint8_t pad0b;                    /* 0x0b */
    RwSurfaceProperties surfaceProps; /* 0x0c */
} RpMaterial;

_Static_assert(offsetof(RpMaterial, refCount) == 0x08, "RpMaterial layout");
_Static_assert(offsetof(RpMaterial, stamp) == 0x0a, "RpMaterial layout");
_Static_assert(sizeof(RpMaterial) == 0x18, "RpMaterial layout");

/* The materials of a geometry or world: a growable array (space 5 at a time). */
typedef struct RpMaterialList {
    RpMaterial **materials; /* 0x00 */
    int32_t numMaterials;   /* 0x04 */
    int32_t space;          /* 0x08 */
} RpMaterialList;

/* Render callback of a world: draws one world sector. */
typedef void *(BMX_CDECL *RpWorldSectorCallBackRender)(RpWorldSector *sector);

typedef struct RpWorld {
    RwObject object;                  /* 0x00 privateFlags bit 0: the sectors belong to someone else */
    uint32_t flags;                   /* 0x08 set by RpWorldSetKind (the stream reader sets 2) */
    RpMaterialList matList;           /* 0x0c */
    RpSector *rootSector;             /* 0x18 */
    int32_t numClumpsInWorld;         /* 0x1c */
    RwLLLink *currentClumpLink;       /* 0x20 the next clump a for-all walk would visit */
    RwLLLink clumpList;               /* 0x24 clumps, linked at RpClump + 0x14 */
    RwLLLink lightList;               /* 0x2c local lights (point, spot), linked at RpLight.inWorld */
    RwLLLink directionalLightList;    /* 0x34 ambient and directional lights */
    RwV3d worldOrigin;                /* 0x3c */
    RwBBox boundingBox;               /* 0x48 */
    RwSurfaceProperties surfaceProps; /* 0x60 */
    RpWorldSectorCallBackRender renderCallBack; /* 0x6c */
    uint8_t stamp;                              /* 0x70 incremented when the world is destroyed */
} RpWorld;

_Static_assert(offsetof(RpWorld, matList) == 0x0c, "RpWorld layout");
_Static_assert(offsetof(RpWorld, rootSector) == 0x18, "RpWorld layout");
_Static_assert(offsetof(RpWorld, clumpList) == 0x24, "RpWorld layout");
_Static_assert(offsetof(RpWorld, directionalLightList) == 0x34, "RpWorld layout");
_Static_assert(offsetof(RpWorld, worldOrigin) == 0x3c, "RpWorld layout");
_Static_assert(offsetof(RpWorld, boundingBox) == 0x48, "RpWorld layout");
_Static_assert(offsetof(RpWorld, surfaceProps) == 0x60, "RpWorld layout");
_Static_assert(offsetof(RpWorld, renderCallBack) == 0x6c, "RpWorld layout");
_Static_assert(offsetof(RpWorld, stamp) == 0x70, "RpWorld layout");

/*
 * The world plugin's data of a camera (at RwCamera + rpWorldCameraOffset): the sectors in view, the world and
 * the camera's own callbacks the plugin replaced (it calls them from its begin/end update and sync).
 */
typedef struct RpWorldCameraData {
    RpWorldSector **sectors;        /* 0x00 */
    int32_t capacity;               /* 0x04 */
    int32_t numSectors;             /* 0x08 */
    RpWorld *world;                 /* 0x0c */
    RwCameraUpdateFunc beginUpdate; /* 0x10 saved RwCamera.beginUpdate */
    RwCameraUpdateFunc endUpdate;   /* 0x14 saved RwCamera.endUpdate */
    void *sync;                     /* 0x18 saved RwCamera.object.sync */
} RpWorldCameraData;

_Static_assert(sizeof(RpWorldCameraData) == 0x1c, "RpWorldCameraData layout");

/* The tie plugin's data in the engine instance (at rpTieOffset): free lists of the two kinds of ties. */
typedef struct RpTieGlobals {
    RwFreeList *tieFreeList;      /* 0x00 */
    RwFreeList *lightTieFreeList; /* 0x04 */
} RpTieGlobals;

/* An entry of the engine's list of worlds, with the world it stands for. */
typedef struct RpWorldListEntry {
    RpWorld *world; /* 0x00 */
    uint32_t size;  /* 0x04 bytes of the world's block (rpWorldRegister) */
    RwLLLink link;  /* 0x08 in RpWorldGlobals.worlds */
} RpWorldListEntry;

/* The world module's data in the engine instance (at rpWorldGlobalsOffset). */
typedef struct RpWorldGlobals {
    void *pipeline;            /* 0x00 the render pipeline of the world sectors (RwIm3DPipeline) */
    RwFreeList *entryFreeList; /* 0x04 free list of RpWorldListEntry */
    RwLLLink worlds;           /* 0x08 */
} RpWorldGlobals;

#define rpTieOffset ORIG_VAR(0x00676730, int32_t, rpTieOffset)

/* Module open/close routines and extension callbacks (rw/world.c). */
void *_rpTieOpen(void *engine, int32_t offset);
void *WorldDeInitClumpExt(void *clump);
void *WorldInitClumpExt(void *clump);
void *_rpBspOpen(void *engine);
void *_rpBspClose(void *engine);
void *_rpSectorOpen(void *engine);
void *_rpSectorClose(void *engine);
void *_rpBinaryWorldOpen(void *engine);
void *_rpBinaryWorldClose(void *engine);
void *_rpMaterialListOpen(void *engine);
void *_rpMaterialListClose(void *engine);
#define rpWorldGlobalsOffset ORIG_VAR(0x00676760, int32_t, rpWorldGlobalsOffset)
/* The surface properties a new world starts with. */
#define rpWorldDefaultSurfaceProps ORIG_VAR(0x00676768, RwSurfaceProperties, rpWorldDefaultSurfaceProps)

/* ---- intersections (the walks of the BSP) ----------------------------------------------------------- */

typedef struct RwLine {
    RwV3d start;
    RwV3d end;
} RwLine;

typedef enum RpIntersectType {
    rpINTERSECTNONE = 0,
    rpINTERSECTLINE = 1,
    rpINTERSECTPOINT = 2,
    rpINTERSECTSPHERE = 3,
    rpINTERSECTBOX = 4,
    rpINTERSECTATOMIC = 5, /* the walk goes over the sectors the atomic is tied to */
} RpIntersectType;

/* The volume of a query and its type (0x1c bytes). The BSP walks read a point as int bit patterns. */
typedef struct RpIntersection {
    union {
        RwLine line;
        RwV3d point;
        int32_t pointBits[3];
        RwSphere sphere;
        RwBBox box;
        RpAtomic *object;
    } t;          /* 0x00 */
    int32_t type; /* 0x18 RpIntersectType */
} RpIntersection;

_Static_assert(sizeof(RpIntersection) == 0x1c, "RpIntersection layout");
_Static_assert(offsetof(RpIntersection, type) == 0x18, "RpIntersection layout");

typedef void *(BMX_CDECL *RpIntersectionCallBackWorldSector)(RpIntersection *intersection,
                                                             RpWorldSector *sector, void *data);

/* ---- the PVS plugin --------------------------------------------------------------------------------- */

/* Progress callback of the PVS generation: stage code and progress (the stages 0x14 and 0x16 bracket it). */
typedef void(BMX_CDECL *RpPVSProgressCallBack)(int32_t stage, float progress);

/*
 * The PVS plugin's data of a world (at RpWorld + rpPVSWorldOffset). The generation fills numSectors ..
 * rowBuffer from the world; hooked is set while the PVS render callback replaces the world's own.
 */
typedef struct RpPVSWorldData {
    int32_t numSectors;        /* 0x00 */
    int32_t rowBytes;          /* 0x04 bytes of a row of visibility bits */
    int32_t rowStride;         /* 0x08 a row padded to 4 bytes, plus 3 */
    int32_t granularity;       /* 0x0c */
    int32_t numCells;          /* 0x10 1 << granularity */
    uint8_t *visibility;       /* 0x14 numSectors rows of rowStride bytes; NULL: no PVS */
    RpWorldSector *viewSector; /* 0x18 sector the view position walk found */
    const uint8_t *viewRow;    /* 0x1c its row of visibility bits (NULL: none) */
    uint8_t *rowBuffer;        /* 0x20 */
    RpWorldSectorCallBackRender savedRenderCallBack; /* 0x24 */
    int32_t hooked;                                  /* 0x28 */
    uint8_t pad2c[8];                                /* 0x2c */
    RpPVSProgressCallBack progressCallBack;          /* 0x34 */
    int32_t sectorCounter;                           /* 0x38 next sector index of the generation */
    uint8_t *cursor;                                 /* 0x3c */
    uint8_t pad40[4];                                /* 0x40 */
    int32_t numEntries;                              /* 0x44 */
    void *entries;                                   /* 0x48 numEntries * 0x24 bytes */
    void *entryIndices;                              /* 0x4c numEntries * 6 bytes */
    uint32_t statsA;                                 /* 0x50 */
    uint32_t statsB;                                 /* 0x54 */
} RpPVSWorldData;

_Static_assert(offsetof(RpPVSWorldData, visibility) == 0x14, "RpPVSWorldData layout");
_Static_assert(offsetof(RpPVSWorldData, rowBuffer) == 0x20, "RpPVSWorldData layout");
_Static_assert(offsetof(RpPVSWorldData, progressCallBack) == 0x34, "RpPVSWorldData layout");
_Static_assert(offsetof(RpPVSWorldData, numEntries) == 0x44, "RpPVSWorldData layout");
_Static_assert(offsetof(RpPVSWorldData, statsA) == 0x50, "RpPVSWorldData layout");

/* State of a PVS generation in progress: the world, and the camera and image it renders with. */
#define rpPVSWorld  ORIG_VAR(0x00676810, RpWorld *, rpPVSWorld)
#define rpPVSCamera ORIG_VAR(0x00676814, RwCamera *, rpPVSCamera)
#define rpPVSImage  ORIG_VAR(0x00676818, RwImage *, rpPVSImage)

/* ---- cameras ---------------------------------------------------------------------------------------- */

RpWorld *RpWorldAddCamera(RpWorld *world, RwCamera *camera);
RpWorld *RpWorldRemoveCamera(RpWorld *world, RwCamera *camera);
RpWorld *RwCameraGetWorld(const RwCamera *camera);
RwCamera *RpCameraForAllSectorsInFrustum(RwCamera *camera, RwObjectCallBack callback, void *data);
int rpWorldCameraDataDestroy(RpWorldCameraData *data);

/* ---- clumps, lights ---------------------------------------------------------------------------------- */

RpWorld *RpWorldAddClump(RpWorld *world, RpClump *clump);
RpWorld *RpWorldRemoveClump(RpWorld *world, RpClump *clump);
RpWorld *RpClumpGetWorld(const RpClump *clump);
RpWorld *RpWorldAddLight(RpWorld *world, RpLight *light);
RpWorld *RpWorldRemoveLight(RpWorld *world, RpLight *light);
int _rpTieDestroy(RpTie *tie);
int _rpLightTieDestroy(RpLightTie *tie);

/* ---- materials -------------------------------------------------------------------------------------- */

RpMaterial *RpMaterialSetColor(RpMaterial *material, const RwRGBA *color);
RpMaterial *RpMaterialSetTexture(RpMaterial *material, RwTexture *texture);
RwTexture *RpMaterialGetTexture(const RpMaterial *material);

/* ---- the world -------------------------------------------------------------------------------------- */

const RwBBox *RpWorldGetBBox(const RpWorld *world);
const RwV3d *RpWorldGetOrigin(const RpWorld *world);
int32_t RpWorldGetNumMaterials(const RpWorld *world);
RpMaterial *RpWorldGetMaterial(const RpWorld *world, int32_t index);
int32_t RpWorldGetNumClumps(const RpWorld *world);
RpWorldSectorCallBackRender RpWorldGetRenderCallBack(const RpWorld *world);
RpWorld *RpWorldRender(RpWorld *world);
RpWorld *RpWorldForAllClumps(RpWorld *world, RwObjectCallBack callback, void *data);
RpWorld *RpWorldForAllLights(RpWorld *world, RwObjectCallBack callback, void *data);
RpWorld *RpWorldForAllWorldSectors(RpWorld *world, RwObjectCallBack callback, void *data);

/*
 * EAX = world. Destroys the world. RpWorldDestroy is the original's four byte stub at 0x4ba9c0 that loads the
 * argument into EAX and falls into this body; hooking both would overlap, so only the body is ported.
 */
ORIG_FN(0x004ba9c0, int, RpWorldDestroy, (RpWorld * world));
__attribute__((regparm(1))) int rpWorldDestroyBody(RpWorld *world);
/* An empty world of one sector for the box (NULL and an error when out of memory or a mesh fails). */
RpWorld *RpWorldCreate(const RwBBox *boundingBox);
/* EAX = sector: frees what a sector tree owns / only releases its ties and plugin data. */
__attribute__((regparm(1))) void rpSectorTreeDestroy(RpSector *sector);
__attribute__((regparm(1))) void rpSectorTreeDeinit(RpSector *sector);

/* Calls the callback for every sector the intersection touches (line and ray walks are still original). */
/* EAX = world, EDX = intersection (a line / a sphere), ECX = callback; the callback's copy of the
 * intersection and its data are the 4th and 5th stack words. */
__attribute__((regparm(3))) RpWorld *
rpWorldForAllSectorsIntersectingLine(RpWorld *world, RpIntersection *intersection,
                                     RpIntersectionCallBackWorldSector callback, int32_t unused0,
                                     int32_t unused1, int32_t unused2, RpIntersection *again, void *data);
__attribute__((regparm(3))) RpWorld *
rpWorldForAllSectorsIntersectingSphere(RpWorld *world, RpIntersection *intersection,
                                       RpIntersectionCallBackWorldSector callback, int32_t unused0,
                                       int32_t unused1, int32_t unused2, RpIntersection *again, void *data);
RpWorld *_rpWorldForAllSectorsIntersecting(RpWorld *world, RpIntersection *intersection,
                                           RpIntersectionCallBackWorldSector callback, void *data);

/* ---- PVS -------------------------------------------------------------------------------------------- */

RpWorld *RpPVSSetProgressCallBack(RpWorld *world, RpPVSProgressCallBack callback);
RpWorld *RpPVSHook(RpWorld *world);
RpWorld *RpPVSUnhook(RpWorld *world);
RpWorld *RpPVSUnload(RpWorld *world);
RpWorld *RpPVSSetViewPosition(RpWorld *world, const RwV3d *position);
int32_t RpPVSQuery(const RpWorld *world);
RpWorld *RpPVSStatisticsGet(RpWorld *world, uint32_t *statsA, uint32_t *statsB);
RpWorld *RpPVSCreate(RpWorld *world, RwRaster *raster, RwRaster *zRaster, float farClip, int32_t granularity,
                     void *callback, void *callbackData);
int32_t rpPVSCountSectors(RpWorld *world);
/* EAX = world: tears down the camera, rasters and image of a PVS generation. */
__attribute__((regparm(1))) void rpPVSCameraDestroy(RpWorld *world);

/* Rendering the atomics of a sector, lighting an object with the lights of the world and its sector, and the
 * world module's open/close. The lighting routine gets the object, the light, 0, 1.0 and the world's surface
 * properties. */
typedef void *(BMX_CDECL *RpLightApplyFn)(void *object, RpLight *light, int32_t zero, float one,
                                          const RwSurfaceProperties *surfaceProps);

typedef struct RpLightApplyArgs {
    void *object;
    RpLightApplyFn apply;
    const RwSurfaceProperties *surfaceProps;
} RpLightApplyArgs;

__attribute__((regparm(1))) void rpSectorRenderAtomics(RpWorldSector *sector);
void *rpWorldSectorRenderWithAtomics(RpWorldSector *sector);
RpLight *rpLightApplyToObject(RpLight *light, RpLightApplyArgs *args);
__attribute__((regparm(2))) int rpWorldForAllDirectionalLights(RwObjectCallBack callback, void *data);
__attribute__((regparm(3))) RpWorldSector *rpSectorForAllLights(RpWorldSector *sector,
                                                                RwObjectCallBack callback, void *data);
void *rpWorldLightObject(void *object, RpLightApplyFn apply);
void *WorldOpen(void *engine, int32_t offset);
void *WorldClose(void *engine);
void *rpWorldDefaultRender(RpWorldSector *sector);
int rpWorldExtCallSlot1(void *pipeline);
int rpWorldExtCallSlot2(void *pipeline);

/* The PVS plugin's callbacks (plugin constructors/destructors, stream chunks, sector walks). */
void *pvsRenderSector(RpWorldSector *sector);
void *pvsSectorUnload(void *sector);
/* EAX = sector, EDX = position: the row of visibility bits of the cell holding the position. */
__attribute__((regparm(2))) const uint8_t *pvsCellRow(const RpWorldSector *sector, const float *position);
void *pvsViewSector(RpIntersection *intersection, RpWorldSector *sector, RpPVSWorldData *pvs);
void *pvsWorldConstruct(void *world);
void *pvsWorldDestruct(RpWorld *world);
int32_t pvsWorldChunkSize(const RpWorld *world);
void *pvsSectorLoad(RpWorldSector *sector, RpPVSWorldData *pvs);
RwStream *pvsWorldChunkRead(RwStream *stream, int32_t length, RpWorld *world);
RwStream *pvsWorldChunkWrite(RwStream *stream, int32_t length, const RpWorld *world);
void *pvsSectorConstruct(void *sector);
void *pvsSectorDestruct(void *sector);
int32_t pvsSectorChunkSize(RpWorldSector *sector);
RwStream *pvsSectorChunkRead(RwStream *stream, int32_t length, RpWorldSector *sector);
RwStream *pvsSectorChunkWrite(RwStream *stream, int32_t length, const RpWorldSector *sector);
void *pvsSectorPrepare(RpWorldSector *sector, RpPVSWorldData *pvs);
void *pvsSectorCount(void *sector, int32_t *count);

/* Vertex and polygon counts of a world sector (zero-extended 16-bit fields). */
int32_t RpWorldSectorGetNumVertices(const RpWorldSector *sector);
int32_t RpWorldSectorGetNumPolygons(const RpWorldSector *sector);

/* ---- world plugin callbacks: modules, extensions of cameras, atomics, clumps and lights -------------------
 */

void *_rpTieClose(void *engine);
void *_rpMaterialOpen(void *engine, int32_t offset);
void *_rpMaterialClose(void *engine);
/* Bodies behind the register-argument stubs: EAX = world, EDX = camera / clump / light / atomic. */
__attribute__((regparm(2))) RpWorld *rpWorldAddCameraBody(RpWorld *world, RwCamera *camera);
__attribute__((regparm(2))) RpWorld *rpWorldAddClumpBody(RpWorld *world, RpClump *clump);
__attribute__((regparm(2))) RpWorld *rpWorldAddLightBody(RpWorld *world, RpLight *light);
__attribute__((regparm(2))) RpWorld *rpWorldAddAtomicBody(RpWorld *world, RpAtomic *atomic);
__attribute__((regparm(2))) RpWorld *rpWorldRemoveAtomicBody(RpWorld *world, RpAtomic *atomic);
void *rpWorldAddAtomicCallback(void *atomic, void *world);
void *rpWorldRemoveAtomicCallback(void *atomic, void *world);
/* EAX = atomic / light: destroy the ties the object has with sectors. */
__attribute__((regparm(1))) void rpAtomicTiesDestroy(RpAtomic *atomic);
__attribute__((regparm(1))) void rpLightTiesDestroy(RpLight *light);
/* Ties an atomic to the sectors of its bounding box (EAX = world, EDX = atomic), and the sync callbacks of
 * atomics and cameras that call it. */
__attribute__((regparm(2))) void rpAtomicAddTies(RpWorld *world, RpAtomic *atomic);
RpAtomic *WorldAtomicSync(RpAtomic *atomic);
RwCamera *WorldCameraSync(RwCamera *camera);
__attribute__((regparm(2))) RpWorld *rpWorldCameraSyncSectors(RpWorld *world, RwCamera *camera);
RpLight *WorldLightSync(RpLight *light);
void *WorldInitCameraExt(void *camera);
void *WorldDeInitCameraExt(void *camera);
void *WorldCopyCameraExt(void *camera, const void *source);
void *WorldInitAtomicExt(void *atomic);
void *WorldDeInitAtomicExt(void *atomic);
void *WorldCopyAtomicExt(void *atomic);
void *WorldCopyClumpExt(void *clump, const void *source);
void *WorldInitLightExt(void *light);
void *WorldDeInitLightExt(void *light);
void *WorldCopyLightExt(void *light, const void *source);
RwCamera *WorldCameraBeginUpdate(RwCamera *camera);
RwCamera *WorldCameraEndUpdate(RwCamera *camera);
const RwRGBA *RpMaterialGetColor(const RpMaterial *material);
const RwBBox *RpWorldSectorGetBBox(const RpWorldSector *sector);
void *rpWorldSectorRenderCurrent(RpWorldSector *sector);
RpWorld *rpWorldSectorGetWorld(RpWorldSector *sector);
void *rpWorldSectorFindCallback(void *sector, void *search);

/* ---- mesh stream (the sector and geometry plugin 0x50e)
 * ---------------------------------------------------- */

struct RpMeshHeader *_rpMeshRead(RwStream *stream, void *owner, const RpMaterialList *materials);
RwStream *_rpMeshWrite(const struct RpMeshHeader *mesh, void *owner, RwStream *stream,
                       const RpMaterialList *materials);
int32_t _rpMeshSize(const struct RpMeshHeader *mesh);
int32_t _rpMaterialListFindMaterialIndex(const RpMaterialList *list, const RpMaterial *material);
RwStream *writeGeometryMesh(RwStream *stream, int32_t length, const void *geometry, int32_t offset,
                            int32_t size);
RwStream *readGeometryMesh(RwStream *stream, int32_t length, void *geometry, int32_t offset, int32_t size);
int32_t sizeGeometryMesh(const void *geometry, int32_t offset, int32_t size);
RwStream *writeSectorMesh(RwStream *stream, int32_t length, const void *sector, int32_t offset, int32_t size);
RwStream *readSectorMesh(RwStream *stream, int32_t length, void *sector, int32_t offset, int32_t size);
int32_t sizeSectorMesh(const void *sector, int32_t offset, int32_t size);

#endif
