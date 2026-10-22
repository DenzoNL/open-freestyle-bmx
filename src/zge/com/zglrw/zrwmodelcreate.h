/*
 * Model loading and instancing (C:\NgEngine\Zge\Com\ZglRw\zrwModelCreate.c): reads a MODL chunk (definition,
 * materials, up to four levels of detail and a collision clump) into a ZrwModel, clones models and frees
 * them. By address the file also holds the soft shadow sampler (0x49b3d0-0x49b520, probably a separate
 * source file) and the two decompressors (0x49bd10, 0x49c080) the asset reader uses.
 */
#ifndef ZGE_COM_ZGLRW_ZRWMODELCREATE_H
#define ZGE_COM_ZGLRW_ZRWMODELCREATE_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/com/zglrw/zrwmodel.h"
#include "zge/zmath.h"
#include "zge/zmem.h"

/* ---- shadow sampler ----------------------------------------------------------------------------- */

/* The shadow state of the roller (ZrollerShadow in ngifzroller.h): the model whose object casts the shadow
 * and the offset of the ground probes' eye from that object. */
typedef struct ZrwShadow {
    uint8_t pad00[8];
    const ZrwModel *model; /* 0x08 its user value (+0x14) is the scene object whose position is sampled */
    ZVec3 offset;          /* 0x0c */
} ZrwShadow;

/* Point of the 6x6 probe grid and the vertex built from its ground hit (0x1c bytes). */
typedef struct ZrwShadowPoint {
    ZVec3 pos;
    uint32_t pad0c;
} ZrwShadowPoint;

typedef struct ZrwShadowVertex {
    ZVec3 pos;        /* 0x00 */
    uint32_t pad0c;   /* 0x0c */
    float u;          /* 0x10 */
    float v;          /* 0x14 */
    uint8_t color[4]; /* 0x18 */
} ZrwShadowVertex;

/* Quad of the shadow mesh: a header word and its four corner vertices (0x14 bytes). */
typedef struct ZrwShadowQuad {
    uint32_t head;
    ZrwShadowVertex *corner[4];
} ZrwShadowQuad;

_Static_assert(sizeof(ZrwShadowVertex) == 0x1c, "shadow vertex");
_Static_assert(sizeof(ZrwShadowQuad) == 0x14, "shadow quad");

#define kZrwShadowGrid  6
#define kZrwShadowCells 5

/* The probe grid is 6x6 points; the mesh has 5x5 quads. */
struct ZrwUpdateNode;
typedef ZrwShadowQuad ZrwShadowQuadList[25];
typedef ZrwShadowPoint ZrwShadowPointList[36];
typedef ZrwShadowVertex ZrwShadowVertexList[36];
typedef uint32_t ZrwShadowMissList[36];

#define gZrwShadowQuads    ORIG_VAR(0x00674930, ZrwShadowQuadList, gZrwShadowQuads)
#define gZrwShadowPoints   ORIG_VAR(0x006746f0, ZrwShadowPointList, gZrwShadowPoints)
#define gZrwShadowOffsets  ORIG_VAR(0x00674b34, ZrwShadowPointList, gZrwShadowOffsets)
#define gZrwShadowVertices ORIG_VAR(0x00674d74, ZrwShadowVertexList, gZrwShadowVertices)
#define gZrwShadowMiss     ORIG_VAR(0x00675164, ZrwShadowMissList, gZrwShadowMiss)
#define gZrwShadowHead     ORIG_VAR(0x006751f4, uint32_t, gZrwShadowHead)
#define gZrwShadowNode     ORIG_VAR(0x006751f8, struct ZrwUpdateNode *, gZrwShadowNode)
#define gZrwShadowReady    ORIG_VAR(0x006751fc, uint32_t, gZrwShadowReady)
#define gZrwShadowScale    ORIG_VAR(0x00675200, float, gZrwShadowScale)
#define gZrwShadowDouble   ORIG_VAR(0x00675204, float, gZrwShadowDouble)
#define gZrwShadowInit     ORIG_VAR(0x0067520c, uint32_t, gZrwShadowInit)
#define gZrwShadowActive   ORIG_VAR(0x00674b28, uint32_t, gZrwShadowActive)

/* Draws the quads built by the last zrollerUpdateShadow (once); returns 1. */
int zrollerShadowDraw(void);
/* Unregisters the shadow's update callback. Returns 1. */
int zrollerShadowStop(void);
/* Update callback of the roller's shadow: probes the ground on a 6x6 grid and builds the quad mesh. */
void zrollerUpdateShadow(void *state);

/* ---- decompression ------------------------------------------------------------------------------ */

/*
 * Method 1 (hash table based, byte 0 == 1 means stored): `work` is 0x4010 bytes of scratch. Writes the
 * length of the output to *outLength.
 */
void zassetDecompressHashed(void *work, const uint8_t *src, int srcLength, uint8_t *dst, int *outLength);
/* zassetDecompress (zasset.h) is method 1 (hashed) or 2 (LZO1X); it returns 0 on success, 1 on a bad method
 * or stream. */

/* ---- frames and clumps -------------------------------------------------------------------------- */

/* One node of a part hierarchy (0x28 bytes): the frame's user data hash, the frame and its position. */
typedef struct ZrwPartNode {
    uint32_t hash;               /* 0x00 */
    uint32_t pad04;              /* 0x04 */
    void *def;                   /* 0x08 */
    void *frame;                 /* 0x0c */
    float pos[3];                /* 0x10 */
    struct ZrwPartNode *parent;  /* 0x1c */
    struct ZrwPartNode *sibling; /* 0x20 */
    struct ZrwPartNode *child;   /* 0x24 */
} ZrwPartNode;

_Static_assert(sizeof(ZrwPartNode) == 0x28, "part node");

/* State of the model loader (0x675218, 0x24 bytes, cleared at initialisation). */
typedef struct ZrwModelCreateState {
    int initialized;     /* 0x00 */
    ZMemFreeList *pool;  /* 0x04 model records (0x8c bytes) */
    uint32_t pad08;      /* 0x08 */
    ZrwPartNode *cursor; /* 0x0c next free node while a hierarchy is built */
    void *lodClumps[4];  /* 0x10 the clump of every level of detail loaded so far */
    ZrwModel *last;      /* 0x20 the model the last MODL chunk produced */
} ZrwModelCreateState;

_Static_assert(sizeof(ZrwModelCreateState) == 0x24, "model create state");

#define gZrwModelCreate ORIG_VAR(0x00675218, ZrwModelCreateState, gZrwModelCreate)

/* RwFrameForAllChildren callbacks that move a frame's 32 bit user data into / out of its matrix (at.x, at.y).
 */
void *zrwFrameUserDataToMatrix(void *frame, void *arg);
void *zrwFrameMatrixToUserData(void *frame, void *arg);
/* RwFrameForAllObjects callback: stores the first object and stops (returns NULL). */
void *zrwFrameCollectFirst(void *object, void *out);
/* First atomic of a clump (NULL when there is none). */
void *zrwModelCreateFirstAtomic(void *clump);
/* Adds a node for the frame under `parent` (NULL for the root) at the cursor, then recurses. */
void *zrwFrameBuildPartNode(void *frame, void *parentNode);

/* Hands the model to the clumps and links every material to the texture of the clump's matching RpMaterial.
 */
void zrwModelSetRwInfo(ZrwModel *model);
/* RpClumpForAllAtomics callback: adds the triangles and vertices of the atomic's geometry to counts[0..1]. */
void *zrwAtomicCountGeometry(void *atomic, int *counts);
/* Prints the atomic count, triangle and vertex counts and geometry flags of a clump. */
void zrwClumpDump(void *clump);
/* Prints a frame and its children with their names from the model's entry table. */
void *zrwFrameDumpHierarchy(void *frame, void *model);
void zrwModelDumpHierarchy(const ZrwModel *model);
/* Prints the clump, hierarchy and materials of a model. */
void zrwModelDumpInfo(const ZrwModel *model);
void zrwModelPrintUsage(void);

/* Clones a model (a shared definition, its clumps cloned); a skinned model is loaded again from its file. */
ZrwModel *zrwModelCreateInstance(const ZrwModel *model);
/* Reads a clump from memory (descriptor: data pointer and size, freed afterwards) into the model: the
 * collision clump when `collision` is 1, else the next level of detail. */
int zrwModelAddClump(ZrwModel *model, const uint32_t *memory, const char *imagePath, int collision);

/* IFF chunk handlers: the MLOD (level of detail), MCOL (collision) and MDEF (definition) chunks. */
int zrwModelLoadLod(void *iff, ZrwModel *model);
int zrwModelLoadCollision(void *iff, ZrwModel *model);
int zrwModelReadDef(void *iff, ZrwModel *model);
/* Renderer material callback that collects a loaded material into the model. */
int zrwModelAddMaterial(void *material, ZrwModel *model);
int zrwModelLoadMaterials(void *iff, ZrwModel *model);
/* MODL chunk handler. Its tail is also a function of its own in the data file. */
int zrwModelLoad(void *iff);
int zrwModelLoadTail(void *iff, ZrwModel *model);

int zrwModelCreateInitialize(void);
int zrwModelCreateShutdown(void);
/* Reads the chunk the reader is on and returns the model it created. */
ZrwModel *zrwModelLoadFromChunk(void *iff);
/* Loads a MODL block held in memory / in a file. */
ZrwModel *zrwModelLoadFromBuffer(const uint32_t *buffer);
ZrwModel *zrwModelLoadFromFile(const char *path);
/* Placeholder that asserts "Not coded" and returns 0. */
int zrwModelNotCoded(void);
int zrwModelDestroy(ZrwModel *model);

#endif
