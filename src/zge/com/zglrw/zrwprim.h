/*
 * Renderer primitives (C:\NgEngine\Zge\Com\ZglRw\zrwPrim.c): polygons are handed to the drawing routine as
 * {texture, vertex pointers, count} records. The wrappers split triangle, quad and strip lists into such
 * records, quads can be clipped against the camera's 2D rectangle first. The packet filed 0x48c570 (a list
 * function, now in zlist.c) here too.
 */
#ifndef ZGE_COM_ZGLRW_ZRWPRIM_H
#define ZGE_COM_ZGLRW_ZRWPRIM_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

/* Screen space vertex (0x1c bytes). The clipping leaves z and the word at 0xc of a clipped vertex unset. */
typedef struct ZrwPrimVertex {
    float x;          /* 0x00 */
    float y;          /* 0x04 */
    float z;          /* 0x08 */
    float pad0c;      /* 0x0c */
    float u;          /* 0x10 */
    float v;          /* 0x14 */
    uint8_t color[4]; /* 0x18 */
} ZrwPrimVertex;

_Static_assert(sizeof(ZrwPrimVertex) == 0x1c, "prim vertex");

/* A polygon as the drawing routine takes it. */
typedef struct ZrwPrimPoly {
    uint32_t texture;
    ZrwPrimVertex *const *vertices;
    uint32_t count;
} ZrwPrimPoly;

/* The primitive system's buffers (0x672898, 0x18 bytes, cleared at initialisation). */
typedef struct ZrwPrimState {
    int initialized;     /* 0x00 */
    uint32_t vertexSize; /* 0x04 0x24 */
    void *vertices;      /* 0x08 */
    uint32_t vertexMax;  /* 0x0c 0x40 */
    void *indices;       /* 0x10 */
    uint32_t indexMax;   /* 0x14 0x40 */
} ZrwPrimState;

_Static_assert(sizeof(ZrwPrimState) == 0x18, "prim state");

#define gZrwPrim ORIG_VAR(0x00672898, ZrwPrimState, gZrwPrim)

/* Clip rectangle of the camera, left / top / right / bottom (at +0x94 of the camera record). */
typedef float ZrwClipRect[4];

/* Point on the edge b-a where x (y) has the given value: position, texture coordinates and colour. */
void zrwprimLerpAtX(float x, const ZrwPrimVertex *a, const ZrwPrimVertex *b, ZrwPrimVertex *out);
void zrwprimLerpAtY(float y, const ZrwPrimVertex *a, const ZrwPrimVertex *b, ZrwPrimVertex *out);
/*
 * Clips a polygon of `count` vertices (optionally transformed by `xform`) against the rectangle and draws
 * what is left with the draw routine; returns the number of vertices drawn (0 when nothing is left).
 */
uint32_t zrwprimClipPoly(uint32_t flags, uint32_t texture, ZrwPrimVertex *const *vertices, uint32_t count,
                         const ZMatrix *xform, const float *rect);
int zrwPrimSystemInitialize(void);
int zrwPrimSystemShutdown(void);
/* Triangle and quad records: the texture, then the vertex pointers inline. */
typedef struct ZrwPrimTriangle {
    uint32_t texture;
    ZrwPrimVertex *vertices[3];
} ZrwPrimTriangle;
typedef struct ZrwPrimQuad {
    uint32_t texture;
    ZrwPrimVertex *vertices[4];
} ZrwPrimQuad;

void zrwprimDrawTriangles(uint32_t flags, const ZrwPrimTriangle *triangles, int count, uint32_t arg);
/* Strips: {texture, vertex pointers, count}, drawn as triangles 0 1 2, 3 2 1, 2 3 4, ... */
void zrwprimDrawStrips(uint32_t flags, const ZrwPrimPoly *strips, int count, uint32_t arg);
void zrwprimDrawQuads(uint32_t flags, const ZrwPrimQuad *quads, int count, uint32_t arg);

/* Draws `count` polygons (not ported). */
ORIG_FN(0x0048d170, void, zrwprimDrawPolys,
        (uint32_t flags, const ZrwPrimPoly *polys, int count, uint32_t arg, uint32_t arg2));
/* The camera record whose clip rectangle and enable bit the quad drawing consults (pcinput.c). */
ORIG_FN(0x00493070, uint8_t *, zrwGetClipCamera, (void));

#endif
