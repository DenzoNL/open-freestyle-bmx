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

/*
 * The primitive system's vertex buffers (0x672898, 0x18 bytes, cleared at initialisation): one for
 * RenderWare's 3D immediate mode (0x24 byte vertices) and one for its 2D mode (0x20 byte vertices), both used
 * from the next 16 byte boundary and grown by the drawing routines in steps of 0x20 vertices.
 */
typedef struct ZrwPrimState {
    int initialized;     /* 0x00 */
    uint32_t vertexSize; /* 0x04 0x24 */
    void *im3d;          /* 0x08 */
    uint32_t im3dMax;    /* 0x0c 0x40 */
    void *im2d;          /* 0x10 */
    uint32_t im2dMax;    /* 0x14 0x40 */
} ZrwPrimState;

_Static_assert(sizeof(ZrwPrimState) == 0x18, "prim state");

#define gZrwPrim ORIG_VAR(0x00672898, ZrwPrimState, gZrwPrim)
/* Frame counter seen by the last draw call (0x672890) and the 2D depth nudge (0x6728b0, reset per call). */
#define gZrwPrimFrame     ORIG_VAR(0x00672890, int, gZrwPrimFrame)
#define gZrwPrimDepthBias ORIG_VAR(0x006728b0, float, gZrwPrimDepthBias)

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

/* A line record: the texture, then both vertex pointers inline. */
typedef struct ZrwPrimLine {
    uint32_t texture;
    ZrwPrimVertex *vertices[2];
} ZrwPrimLine;

void zrwprimDrawTriangles(uint32_t flags, const ZrwPrimTriangle *triangles, int count, const ZMatrix *xform);
/* Strips: {texture, vertex pointers, count}, drawn as triangles 0 1 2, 3 2 1, 2 3 4, ... */
void zrwprimDrawStrips(uint32_t flags, const ZrwPrimPoly *strips, int count, const ZMatrix *xform);
void zrwprimDrawQuads(uint32_t flags, const ZrwPrimQuad *quads, int count, const ZMatrix *xform);

/*
 * The drawing routines (0x48ccf0 is reached through the pointer at 0x561728). `texture` is the material whose
 * first raster is set. Flag 0x2000000 draws in screen space (2D immediate mode), else the vertices are game
 * space points (optionally transformed by `xform` first) moved into RenderWare's world; with flag 0x1000000
 * they are relative to the current camera instead. The low 24 bits of the flags are the render state.
 * `count` must not be 0 (it is decremented before it is tested). The callers push a fifth word 0 that is
 * never read.
 */
void zrwprimDrawLines(uint32_t flags, const ZrwPrimLine *lines, int count, const ZMatrix *xform);
void zrwprimDrawPolys(uint32_t flags, const ZrwPrimPoly *polys, int count, const ZMatrix *xform);
/* The camera record whose clip rectangle and enable bit the quad drawing consults (pcinput.c). */
ORIG_FN(0x00493070, uint8_t *, zrwGetClipCamera, (void));

#endif
