/*
 * Collision queries against the renderer's world (C:\NgEngine\Zge\Com\ZglRw\zrwCol.c). A query stores the
 * triangles it finds as hit records in a growing buffer and keeps the nearest one; the lines and spheres
 * themselves are intersected by RenderWare. The packet filed the DirectInput polling functions here too
 * (they are in pcinput.c).
 */
#ifndef ZGE_COM_ZGLRW_ZRWCOL_H
#define ZGE_COM_ZGLRW_ZRWCOL_H

#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"
#include "rw/rwcamera.h"
#include "rw/rwclump.h"
#include "rw/rwcollision.h"
#include "rw/rwmath.h"
#include "rw/rwworld.h"

/*
 * A triangle as RenderWare's intersection code reports it (0x28 bytes): plane normal, the index of the
 * triangle (a polygon of the world sector, or a triangle of the atomic's geometry) and the vertex pointers.
 */
typedef struct ZrwColTriangle {
    float normal[3];        /* 0x00 */
    uint32_t pad0c;         /* 0x0c */
    uint32_t pad10[2];      /* 0x10 */
    int32_t index;          /* 0x18 polygon / triangle index */
    const float *vertex[3]; /* 0x1c xyz of each corner, in RenderWare space */
} ZrwColTriangle;

_Static_assert(offsetof(ZrwColTriangle, index) == 0x18, "collision triangle");
BMX_LAYOUT32_ASSERT(offsetof(ZrwColTriangle, vertex) == 0x1c, "collision triangle");
BMX_LAYOUT32_ASSERT(sizeof(ZrwColTriangle) == 0x28, "collision triangle");
/* It is RenderWare's RpCollisionTriangle, in the game's own spelling: the two must agree on every width. */
_Static_assert(sizeof(ZrwColTriangle) == sizeof(RpCollisionTriangle) &&
                   offsetof(ZrwColTriangle, index) == offsetof(RpCollisionTriangle, index) &&
                   offsetof(ZrwColTriangle, vertex) == offsetof(RpCollisionTriangle, vertices),
               "ZrwColTriangle = RpCollisionTriangle");

/* Colour of the last line hit (0x674158, r g b a). */
typedef uint8_t ZrwColColor[4];
#define gZrwColColor ORIG_VAR(0x00674158, ZrwColColor, gZrwColColor)

/* Hit record in game space (0x4c bytes); the y and z axes are mirrored relative to RenderWare. */
typedef struct ZrwColHit {
    float normal[3];    /* 0x00 */
    uint32_t pad0c;     /* 0x0c */
    float vertex[3][4]; /* 0x10 corner xyz, one padding word each */
    float distance;     /* 0x40 */
    void *objectData;   /* 0x44 user data of the object hit (its ZrwModel; NULL for the static world) */
    void *materialData; /* 0x48 user data of the material */
} ZrwColHit;

BMX_LAYOUT32_ASSERT(sizeof(ZrwColHit) == 0x4c, "collision hit");

/* What a sphere query hands back: its hit records (in gZrwCol's buffer) and how many. */
typedef struct ZrwColSphereResult {
    ZrwColHit *hits;
    uint32_t count;
} ZrwColSphereResult;
_Static_assert(offsetof(ZrwColHit, distance) == 0x40, "collision hit");

/* Result of the line query (0x58 bytes): the nearest hit and the point where the line meets it. */
typedef struct ZrwColLineHit {
    ZrwColHit hit;
    float point[3]; /* 0x4c */
} ZrwColLineHit;

BMX_LAYOUT32_ASSERT(sizeof(ZrwColLineHit) == 0x58, "line hit");

/* State of the collision system (0x674160, 0x50 bytes, cleared at initialisation). */
typedef struct ZrwColState {
    int initialized;     /* 0x00 */
    ZrwColHit *cursor;   /* 0x04 next free record */
    uint32_t count;      /* 0x08 records stored */
    ZrwColHit *hits;     /* 0x0c record buffer */
    uint32_t capacity;   /* 0x10 records the buffer holds */
    uint32_t flags;      /* 0x14 query flags: 1 world, 2 / 4 solid / not solid objects, 8 skip ids */
    int found;           /* 0x18 */
    float nearest;       /* 0x1c distance of the nearest hit so far */
    ZrwColTriangle best; /* 0x20 copy of the nearest triangle */
    uint32_t bestUser;   /* 0x48 the RpAtomic it belongs to, 0 for the world */
    uint32_t bestSub;    /* 0x4c the RpWorldSector it belongs to, 0 for an atomic */
} ZrwColState;

BMX_LAYOUT32_ASSERT(offsetof(ZrwColState, best) == 0x20, "collision state");
BMX_LAYOUT32_ASSERT(offsetof(ZrwColState, bestUser) == 0x48, "collision state");
BMX_LAYOUT32_ASSERT(offsetof(ZrwColState, bestSub) == 0x4c, "collision state");
BMX_LAYOUT32_ASSERT(sizeof(ZrwColState) == 0x50, "collision state");

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

/* The same volume as a line (start, end) of type 1. */
typedef struct ZrwColLineVolume {
    float start[3];
    float end[3];
    int32_t type; /* 0x18 */
} ZrwColLineVolume;

_Static_assert(offsetof(ZrwColIntersection, type) == 0x18, "intersection volume");
_Static_assert(offsetof(ZrwColLineVolume, type) == 0x18, "line volume");
_Static_assert(sizeof(ZrwColLineVolume) == sizeof(ZrwColIntersection), "line volume");

int zrwColSystemInitialize(void);
int zrwColSystemShutdown(void);

/* Triangle callbacks: remember the nearest hit (the triangle is copied when it is nearer than the stored
 * one). */
ZrwColTriangle *zrwcolKeepNearestSub(void *arg, uint32_t sub, ZrwColTriangle *triangle, float distance,
                                     uint32_t user);
ZrwColTriangle *zrwcolKeepNearest(void *arg, ZrwColTriangle *triangle, float distance, uint32_t user);
/*
 * Atomic filters: skip by the flags of the query (bit 7 of the atomic's flags marks it solid, see
 * zrwModelSetCollision; the ids to skip are clump pointers), then enumerate the atomic's triangles with the
 * callback.
 */
RpAtomic *zrwcolFilterNearest(void *arg, void *unused, RpAtomic *atomic, void *unused2,
                              void *const *excluded);
RpAtomic *zrwcolFilterRecord(void *arg, void *unused, RpAtomic *atomic, void *unused2, void *const *excluded);
/* Appends a hit record for a triangle of the world (atomic NULL) or of an atomic. */
ZrwColTriangle *zrwcolRecordHit(void *arg, const RpWorldSector *sector, ZrwColTriangle *triangle,
                                float distance, RpAtomic *atomic);
ZrwColTriangle *zrwcolRecordObjectHit(void *arg, ZrwColTriangle *triangle, float distance, RpAtomic *atomic);
/* Sphere query: 1 and the hit buffer and count in out[0], out[1] when something was hit. */
int zrwcolQuerySphere(const float *center, float radius, uint32_t flags, void *const *excluded,
                      ZrwColSphereResult *out);
/* Sets gZrwColEnabled when the tag tables initialise; returns whether they did. */
int zrwcolEnableIfTagInfo(void);
/* Line and sphere tests through the function pointers at 0x562068 / 0x56206c. */
int zrwcolLineTest(const float *a, const float *b, int mode, void *const *excluded, float *out);
int zrwcolSphereTest(const float *center, float radius, uint32_t flags, void *const *excluded,
                     ZrwColSphereResult *out);

/*
 * Line query (held by the pointer at 0x562068): 1 and the nearest hit in *out when the line from `from` to
 * `to` hits the world (flags bit 0) or objects (bits 1, 2; bit 3 skips the `excluded` ids). With bit 5 the
 * line is first clipped to the world box, in place (0 when it misses the box).
 */
int zrwcolQueryLine(float *from, float *to, uint32_t flags, void *const *excluded, ZrwColLineHit *out);
/*
 * Prelit colour of the world at the point of the last line hit (in gZrwColColor, alpha 0), opaque black when
 * the hit was not on the world. Sorts the hit's corners by z in place.
 */
uint8_t *zrwcolHitColor(ZrwColLineHit *hit);

#endif
