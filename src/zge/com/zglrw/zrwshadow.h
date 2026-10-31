/*
 * The roller's soft shadow (C:\NgEngine\Zge\Com\ZglRw\zrwShadow.c): an update callback probes the ground
 * below the rider on a 6x6 grid and builds a mesh of 5x5 quads from the hits, which a draw callback renders
 * with the "assets\shadowMap.Bmp" texture.
 */
#ifndef ZGE_COM_ZGLRW_ZRWSHADOW_H
#define ZGE_COM_ZGLRW_ZRWSHADOW_H

#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"
#include "zge/com/zglrw/zrwmodel.h"
#include "zge/zmath.h"

/* The shadow state of the roller (ZrollerShadow in ngifzroller.h, the update callback's argument): the model
 * whose object casts the shadow and the offset of the ground probes' eye from that object. */
typedef struct ZrwShadow {
    int32_t kind;          /* 0x00 */
    int32_t *slots;        /* 0x04 */
    const ZrwModel *model; /* 0x08 its user value is the scene object whose position is sampled */
    ZVec3 offset;          /* 0x0c */
} ZrwShadow;

_Static_assert(offsetof(ZrwShadow, model) == 0x08, "shadow state");
_Static_assert(offsetof(ZrwShadow, offset) == 0x0c, "shadow state");

/* Point of the 6x6 probe grid. */
typedef struct ZrwShadowPoint {
    ZVec3 pos;
    uint32_t pad0c;
} ZrwShadowPoint;

/* Vertex of the shadow mesh, built from a probe's ground hit (0x1c bytes). */
typedef struct ZrwShadowVertex {
    ZVec3 pos; /* 0x00 */
    uint32_t pad0c;
    float u;          /* 0x10 */
    float v;          /* 0x14 */
    uint8_t color[4]; /* 0x18 */
} ZrwShadowVertex;

/* Quad of the shadow mesh, as zrwprimDrawQuads takes it (ZrwPrimQuad): the texture and its four corners. */
typedef struct ZrwShadowQuad {
    uint32_t texture;
    ZrwShadowVertex *corner[4];
} ZrwShadowQuad;

_Static_assert(sizeof(ZrwShadowPoint) == 0x10, "shadow point");
_Static_assert(offsetof(ZrwShadowVertex, u) == 0x10, "shadow vertex");
_Static_assert(offsetof(ZrwShadowVertex, color) == 0x18, "shadow vertex");
_Static_assert(sizeof(ZrwShadowVertex) == 0x1c, "shadow vertex");
_Static_assert(sizeof(ZrwShadowQuad) == 0x14, "shadow quad");

/* The probe grid is 6x6 points; the mesh has 5x5 quads. */
#define kZrwShadowGrid  6
#define kZrwShadowCells 5

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
#define gZrwShadowTexture  ORIG_VAR(0x006751f4, uint32_t, gZrwShadowTexture)
#define gZrwShadowNode     ORIG_VAR(0x006751f8, struct ZrwUpdateNode *, gZrwShadowNode)
#define gZrwShadowReady    ORIG_VAR(0x006751fc, uint32_t, gZrwShadowReady)
#define gZrwShadowScale    ORIG_VAR(0x00675200, float, gZrwShadowScale)
#define gZrwShadowDouble   ORIG_VAR(0x00675204, float, gZrwShadowDouble)
#define gZrwShadowInit     ORIG_VAR(0x0067520c, uint32_t, gZrwShadowInit)
#define gZrwShadowActive   ORIG_VAR(0x00674b28, uint32_t, gZrwShadowActive)

/* Loads the shadow texture ("assets\shadowMap.Bmp") and registers the draw callback. Returns 1. */
int zrollerShadowStart(void);
/* Draws the quads built by the last zrollerUpdateShadow (once); returns 1. */
int zrollerShadowDraw(void);
/* Unregisters the shadow's update callback. Returns 1. */
int zrollerShadowStop(void);
/* Update callback of the roller's shadow: probes the ground on a 6x6 grid and builds the quad mesh. */
void zrollerUpdateShadow(void *state);

#endif
