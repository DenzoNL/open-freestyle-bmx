/*
 * Model rendering and collision debugging (C:\NgEngine\Zge\Com\ZglRw\zrwModelRender.c): the per-frame
 * callback that renders a model's clumps, the immediate mode drawing of geometry, and the world's collision
 * triangle tree (a BSP of leaves holding triangles) with its debug drawing.
 */
#ifndef ZGE_COM_ZGLRW_ZRWMODELRENDER_H
#define ZGE_COM_ZGLRW_ZRWMODELRENDER_H

#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"
#include "zge/com/zglrw/zrwmodel.h"
#include "zge/zmath.h"

/* A collision triangle (0x5c bytes). */
typedef struct ZrwRenderColTriangle {
    ZVec3 v0; /* 0x00 */
    uint32_t pad0c;
    ZVec3 v1; /* 0x10 */
    uint32_t pad1c;
    ZVec3 v2; /* 0x20 */
    uint32_t pad2c;
    ZVec3 normal; /* 0x30 */
    uint32_t pad3c;
    uint32_t tag;          /* 0x40 zero: unused triangle */
    uint32_t drawEdge[3];  /* 0x44 edges v0-v1, v1-v2, v2-v0 are drawn in debug */
    uint32_t drawPoint[3]; /* 0x50 the corners (offset by the normal) are marked in debug */
} ZrwRenderColTriangle;

_Static_assert(offsetof(ZrwRenderColTriangle, v1) == 0x10, "collision triangle");
_Static_assert(offsetof(ZrwRenderColTriangle, v2) == 0x20, "collision triangle");
_Static_assert(offsetof(ZrwRenderColTriangle, normal) == 0x30, "collision triangle");
_Static_assert(offsetof(ZrwRenderColTriangle, tag) == 0x40, "collision triangle");
_Static_assert(offsetof(ZrwRenderColTriangle, drawEdge) == 0x44, "collision triangle");
_Static_assert(offsetof(ZrwRenderColTriangle, drawPoint) == 0x50, "collision triangle");
_Static_assert(sizeof(ZrwRenderColTriangle) == 0x5c, "collision triangle");

/* A node of the triangle tree (0x3c bytes): split nodes (types 2, 4, 8 split on x, y, z) and leaves (1). */
typedef struct ZrwRenderColNode {
    float split;  /* 0x00 */
    ZVec3 boxMin; /* 0x04 */
    uint32_t pad10;
    ZVec3 boxMax; /* 0x14 */
    uint32_t pad20;
    uint8_t type; /* 0x24 */
    uint8_t pad25;
    uint16_t triangleCount;           /* 0x26 */
    struct ZrwRenderColNode *below;   /* 0x28 */
    struct ZrwRenderColNode *above;   /* 0x2c */
    ZrwRenderColTriangle **triangles; /* 0x30 */
    uint8_t pad34[8];
} ZrwRenderColNode;

_Static_assert(offsetof(ZrwRenderColNode, boxMin) == 0x04, "collision node");
_Static_assert(offsetof(ZrwRenderColNode, boxMax) == 0x14, "collision node");
_Static_assert(offsetof(ZrwRenderColNode, type) == 0x24, "collision node");
_Static_assert(offsetof(ZrwRenderColNode, triangleCount) == 0x26, "collision node");
_Static_assert(offsetof(ZrwRenderColNode, below) == 0x28, "collision node");
BMX_LAYOUT32_ASSERT(offsetof(ZrwRenderColNode, triangles) == 0x30, "collision node");
BMX_LAYOUT32_ASSERT(sizeof(ZrwRenderColNode) == 0x3c, "collision node");

/* Result of zrwModelFindTriangle (kept in the exe's data): the triangle's corners and tag. */
typedef struct ZrwRenderColHit {
    uint8_t valid;
    uint8_t pad01[3];
    ZVec3 v0; /* 0x04 */
    uint32_t pad10;
    ZVec3 v1; /* 0x14 */
    uint32_t pad20;
    ZVec3 v2; /* 0x24 */
    uint32_t pad30;
    uint32_t tag; /* 0x34 */
} ZrwRenderColHit;

_Static_assert(offsetof(ZrwRenderColHit, v0) == 0x04, "collision hit");
_Static_assert(offsetof(ZrwRenderColHit, v1) == 0x14, "collision hit");
_Static_assert(offsetof(ZrwRenderColHit, v2) == 0x24, "collision hit");
_Static_assert(offsetof(ZrwRenderColHit, tag) == 0x34, "collision hit");
_Static_assert(sizeof(ZrwRenderColHit) == 0x38, "collision hit");

/* Result of zrwModelQuery (0x6737b0): a flag and a pointer to the hit list (0x672908). */
typedef struct ZrwRenderColQuery {
    uint16_t found;
    uint8_t pad02[2];
    void *list;
} ZrwRenderColQuery;

BMX_LAYOUT32_ASSERT(offsetof(ZrwRenderColQuery, list) == 0x04, "collision query");

/* The tree's root (0x6728c4) and the "draw while searching" flag (0x672900). */
#define gZrwColRoot  ORIG_VAR(0x006728c4, ZrwRenderColNode, gZrwColRoot)
#define gZrwColDebug ORIG_VAR(0x00672900, int, gZrwColDebug)
#define gZrwColHit   ORIG_VAR(0x00673708, ZrwRenderColHit, gZrwColHit)
#define gZrwColQuery ORIG_VAR(0x006737b0, ZrwRenderColQuery, gZrwColQuery)

/* State of the model renderer (0x28 bytes, memset at initialisation). */
typedef struct ZrwModelRenderState {
    int initialized;      /* 0x673e80 */
    uint32_t pad84;       /* 0x673e84 */
    void *atomicCallback; /* 0x673e88 callback handed to the atomics of a model */
    void *updateNode;     /* 0x673e8c the registered per-frame callback */
    void *vertices;       /* 0x673e90 scratch buffers, grown for the biggest geometry drawn */
    void *buffer2;        /* 0x673e94 */
    void *buffer3;        /* 0x673e98 */
    uint16_t *indices;    /* 0x673e9c */
    int vertexCapacity;   /* 0x673ea0 */
    int indexCapacity;    /* 0x673ea4 */
} ZrwModelRenderState;

BMX_LAYOUT32_ASSERT(sizeof(ZrwModelRenderState) == 0x28, "model render state");

#define gZrwModelRender ORIG_VAR(0x00673e80, ZrwModelRenderState, gZrwModelRender)

/* Debug drawing of collision data (colours are RGBA words; NULL skips that part). */
void zrwModelDrawNodeBox(const ZrwRenderColNode *node, const uint32_t *color);
void zrwModelDrawTriangle(const ZrwRenderColTriangle *triangle, const uint32_t *pointColor,
                          const uint32_t *edgeColor, const uint32_t *fillColor);
void zrwModelDrawTriangleEdge(const ZrwRenderColTriangle *triangle, uint8_t edge, const uint32_t *color);

/* The leaf of the tree that holds `point`. */
ZrwRenderColNode *zrwModelFindNode(const ZVec3 *point);
/* Whether `point` lies inside the triangle (v0, v1, v2) with this normal. */
int zrwModelPointInTriangle(const ZVec3 *v0, const ZVec3 *v1, const ZVec3 *v2, const ZVec3 *normal,
                            const ZVec3 *point);
/* The triangle closest to `point` (along its normal) within maxDistance, as &gZrwColHit, or NULL. */
ZrwRenderColHit *zrwModelFindTriangle(const ZVec3 *point, float maxDistance);
/*
 * Collects the triangles (mode 1), marked edges (2) or marked corners (3) of the tree within `radius` of
 * `point` into the hit list at 0x672908, counting them in *found (at most 0x40).
 */
void zrwColSearch(int mode, const ZrwRenderColNode *node, uint16_t *found, const ZVec3 *point, float radius);
/* The marked edges near `point` as a fresh query result, or NULL when there are none. */
ZrwRenderColQuery *zrwModelQuery(const ZVec3 *point, float radius);

/* Atomic render callbacks (they return the atomic); the first two chain to ZrwModelDef.atomicRender. */
void *zrwModelRenderBillboard(void *object);
void *zrwModelRenderCallback(void *object);
/* RpClumpForAllAtomics callback of zrwModelRender: draws the atomic through immediate mode. */
void *zrwModelRenderAtomic(void *atomic);
void *zrwFrameDrawLines(void *frame, const ZVec3 *parentPosition);
void zrwModelDrawFrames(const ZrwModel *model);
int zrwModelRender(ZrwModel *model);
void zrwModelRenderDirect(ZrwModel *model);
typedef void *(BMX_CDECL *ZrwObjectCallback)(void *object);
void *zrwAtomicSetRenderCallback(void *atomic, void *callback);
void zrwModelSetRenderCallback(void *model, ZrwObjectCallback callback);
void zrwModelSetDefaultRenderCallback(ZrwModel *model);
void zrwModelRenderRegister(void *arg);
void zrwModelRenderUnregister(void);
int zrwModelRenderInitialize(void);
int zrwModelRenderShutdown(void);

#endif
