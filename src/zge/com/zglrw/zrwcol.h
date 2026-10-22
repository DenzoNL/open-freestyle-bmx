/*
 * Collision queries against the renderer's world (C:\NgEngine\Zge\Com\ZglRw\zrwCol.c). A query stores the
 * triangles it finds as hit records in a growing buffer and keeps the nearest one; the lines and spheres
 * themselves are intersected by RenderWare. The packet filed the DirectInput polling functions here too
 * (they are in pcinput.c).
 */
#ifndef ZGE_COM_ZGLRW_ZRWCOL_H
#define ZGE_COM_ZGLRW_ZRWCOL_H

#include <stdint.h>

#include "compat/orig.h"

/* A triangle of the world as RenderWare reports it (0x28 bytes): plane normal, material index, vertex
 * pointers. */
typedef struct ZrwColTriangle {
    float normal[3];        /* 0x00 */
    uint32_t pad0c;         /* 0x0c */
    uint32_t pad10[2];      /* 0x10 */
    int32_t index;          /* 0x18 material index inside the sector */
    const float *vertex[3]; /* 0x1c xyz of each corner, in RenderWare space */
} ZrwColTriangle;

_Static_assert(sizeof(ZrwColTriangle) == 0x28, "collision triangle");

/* World sector: the base of its material handles (+4) and the per triangle material bytes (+0xc, stride 4).
 */
typedef struct ZrwColSector {
    uint8_t pad00[4];
    int16_t base;       /* 0x04 */
    uint8_t pad06[6];   /* 0x06 */
    const uint8_t *map; /* 0x0c */
} ZrwColSector;

/* Scene object: its frame (+4) and model (+0x1c). */
typedef struct ZrwColObject {
    uint32_t pad00;
    void *frame; /* 0x04 */
    uint8_t pad08[0x14];
    void *model; /* 0x1c */
} ZrwColObject;

/* An object as the intersection filters see it: flag byte at +2 (bit 7: solid), id at +0x40. */
typedef struct ZrwColPoly {
    uint8_t pad00[2];
    uint8_t flags; /* 0x02 */
    uint8_t pad03[0x3d];
    int32_t id; /* 0x40 */
} ZrwColPoly;

/* Hit record in game space (0x4c bytes); the y and z axes are mirrored relative to RenderWare. */
typedef struct ZrwColHit {
    float normal[3];       /* 0x00 */
    uint32_t pad0c;        /* 0x0c */
    float vertex[3][4];    /* 0x10 corner xyz, one padding word each */
    float distance;        /* 0x40 */
    uint32_t objectData;   /* 0x44 user data of the object hit (0 for the static world) */
    uint32_t materialData; /* 0x48 user data of the material */
} ZrwColHit;

_Static_assert(sizeof(ZrwColHit) == 0x4c, "collision hit");

/* State of the collision system (0x674160, 0x50 bytes, cleared at initialisation). */
typedef struct ZrwColState {
    int initialized;    /* 0x00 */
    ZrwColHit *cursor;  /* 0x04 next free record */
    uint32_t count;     /* 0x08 records stored */
    ZrwColHit *hits;    /* 0x0c record buffer */
    uint32_t capacity;  /* 0x10 records the buffer holds */
    uint32_t flags;     /* 0x14 query flags: 1 world, 2 / 4 solid / not solid objects, 8 skip ids */
    int found;          /* 0x18 */
    float nearest;      /* 0x1c distance of the nearest hit so far */
    uint8_t best[0x28]; /* 0x20 copy of the nearest triangle */
    uint32_t bestUser;  /* 0x48 */
    uint32_t bestSub;   /* 0x4c */
} ZrwColState;

_Static_assert(sizeof(ZrwColState) == 0x50, "collision state");

#define gZrwCol ORIG_VAR(0x00674160, ZrwColState, gZrwCol)
/* Set by zrwcolEnableIfTagInfo (0x6741b0). */
#define gZrwColEnabled ORIG_VAR(0x006741b0, uint32_t, gZrwColEnabled)
/* Bounds of the renderer's world (min corner, pad, max corner, pad) the queries must start inside. */
typedef float ZrwColBox[8];
#define gZrwColBox ORIG_VAR(0x006976fc, ZrwColBox, gZrwColBox)

/* Intersection volume handed to RenderWare: a sphere (centre, radius) of type 3. */
typedef struct ZrwColIntersection {
    float x, y, z, radius;
    uint32_t pad10[2];
    int32_t type; /* 0x18 */
} ZrwColIntersection;

int zrwColSystemInitialize(void);
int zrwColSystemShutdown(void);

/* Triangle callbacks: remember the nearest hit (the triangle is copied when it is nearer than the stored
 * one). */
ZrwColTriangle *zrwcolKeepNearestSub(void *arg, uint32_t sub, ZrwColTriangle *triangle, float distance,
                                     uint32_t user);
ZrwColTriangle *zrwcolKeepNearest(void *arg, ZrwColTriangle *triangle, float distance, uint32_t user);
/* Object filters: skip by the flags of the query, then enumerate the object's triangles with the callback. */
ZrwColPoly *zrwcolFilterNearest(void *arg, void *unused, ZrwColPoly *poly, void *unused2,
                                const int *excluded);
ZrwColPoly *zrwcolFilterRecord(void *arg, void *unused, ZrwColPoly *poly, void *unused2, const int *excluded);
/* Appends a hit record for a triangle of the world (object NULL) or of an object. */
ZrwColTriangle *zrwcolRecordHit(void *arg, const ZrwColSector *sector, ZrwColTriangle *triangle,
                                float distance, const ZrwColObject *object);
ZrwColTriangle *zrwcolRecordObjectHit(void *arg, ZrwColTriangle *triangle, float distance,
                                      const ZrwColObject *object);
/* Sphere query: 1 and the hit buffer and count in out[0], out[1] when something was hit. */
int zrwcolQuerySphere(const float *center, float radius, uint32_t flags, const int *excluded, uint32_t *out);
/* Sets gZrwColEnabled when the tag tables initialise. */
void zrwcolEnableIfTagInfo(void);
/* Line and sphere tests through the function pointers at 0x562068 / 0x56206c. */
int zrwcolLineTest(const float *a, const float *b, int mode, int flags, float *out);
int zrwcolSphereTest(const float *center, float radius, uint32_t flags, const int *excluded, uint32_t *out);

/* Not ported: the line query (0x493e20, held by the pointer at 0x562068) and the tag table setup. */
ORIG_FN(0x00493e20, int, zrwcolQueryLine, (const float *a, const float *b, int mode, int flags, float *out));
ORIG_FN(0x0048d6f0, int, ztaginfInitialize, (void));

/* RenderWare: matrices and points of frames, materials, and the intersection enumerators. */
ORIG_FN(0x004d2b10, void *, rwFrameGetLTM, (void *frame));
ORIG_FN(0x004d3a90, void, rwV3dTransformPoints, (float *out, const void *in, int count, const void *matrix));
ORIG_FN(0x004d3ae0, void, rwV3dTransformVectors, (float *out, const void *in, int count, const void *matrix));
ORIG_FN(0x004b46b0, uint8_t *, rwModelGetMaterialTable, (void *model));
ORIG_FN(0x004b47a0, void *, rwModelGetMaterial, (void *model, uint32_t index));
ORIG_FN(0x004b93d0, void *, rwMaterialGetData, (void *material));
ORIG_FN(0x004ba140, void *, rwWorldGetMaterial, (void *world, uint32_t index));
ORIG_FN(0x004c1b10, void, rwWorldIntersectTriangles,
        (void *world, const ZrwColIntersection *intersection, void *callback, void *arg));
ORIG_FN(0x004c1c10, void, rwWorldIntersectObjects,
        (void *world, const ZrwColIntersection *intersection, void *callback, const void *arg));
ORIG_FN(0x004c1f30, void, rwObjectIntersectTriangles, (void *object, void *arg, void *callback, void *user));

#endif
