/*
 * RenderWare 3.1 collision (rw/collision.c): intersections of a line, point, sphere, box or atomic with the
 * world's triangles, with the atomics in the world, and with the triangles of one atomic.
 *
 * The internal walkers use the library's register convention: EAX, EDX and ECX carry the first three
 * arguments, the caller leaves three stack words unused and passes the rest after them (declared here as
 * regparm(3) functions with three `unused` words). The box test runs the Graphics Gems triangle-cube test
 * (Voorhies) on the triangle mapped into the unit cube.
 */
#ifndef RW_RWCOLLISION_H
#define RW_RWCOLLISION_H

#include "rw/rwclump.h"

/* RwLine, RpIntersectType and RpIntersection live in rwworld.h (the BSP walks use them too). */

/* A triangle hit, as the callbacks see it: unit normal, first corner, triangle index, corner pointers. */
typedef struct RpCollisionTriangle {
    RwV3d normal;       /* 0x00 */
    RwV3d point;        /* 0x0c */
    int32_t index;      /* 0x18 */
    RwV3d *vertices[3]; /* 0x1c */
} RpCollisionTriangle;

_Static_assert(sizeof(RpCollisionTriangle) == 0x28, "RpCollisionTriangle layout");

/* Callbacks: they return their object to go on, NULL to stop the query. */
typedef RpCollisionTriangle *(BMX_CDECL *RpIntersectionCallBackWorldTriangle)(RpIntersection *intersection,
                                                                              RpWorldSector *sector,
                                                                              RpCollisionTriangle *triangle,
                                                                              float distance, void *data);
typedef RpAtomic *(BMX_CDECL *RpIntersectionCallBackAtomic)(RpIntersection *intersection,
                                                            RpWorldSector *sector, RpAtomic *atomic,
                                                            float distance, void *data);
typedef RpCollisionTriangle *(BMX_CDECL *RpIntersectionCallBackGeometryTriangle)(
    RpIntersection *intersection, RpCollisionTriangle *triangle, float distance, void *data);

/*
 * What the sector walkers hand to the per-sector tests: the user callback and data, the volume (for lines the
 * part clipped to the sector) and an atomic to skip (0x24 bytes).
 */
typedef struct RpCollisionQuery {
    void *callBack; /* 0x00 */
    void *data;     /* 0x04 */
    union {
        RwLine line;
        RwSphere sphere;
        RwBBox box;
    } t;               /* 0x08 */
    RpAtomic *exclude; /* 0x20 */
} RpCollisionQuery;

_Static_assert(sizeof(RpCollisionQuery) == 0x24, "RpCollisionQuery layout");

/* Per world sector callback of the line walker (original code: the line tests against triangles / atomics).
 */
typedef RpWorldSector *(BMX_CDECL *RpSectorCallBack)(RpIntersection *intersection, RpWorldSector *sector,
                                                     RpCollisionQuery *query);

/* ---- public entry points ---------------------------------------------------------------------------- */

/* RpCollisionWorldForAllIntersections: the world's triangles hit by the volume (not a point). */
RpWorld *rwWorldIntersectTriangles(RpWorld *world, RpIntersection *intersection,
                                   RpIntersectionCallBackWorldTriangle callBack, void *data);
/* RpWorldForAllAtomicIntersections: the atomics in the world whose bounding spheres the volume meets. */
RpWorld *rwWorldIntersectObjects(RpWorld *world, RpIntersection *intersection,
                                 RpIntersectionCallBackAtomic callBack, void *data);
/* RpAtomicForAllIntersections: the atomic's triangles hit by a line, a sphere or another atomic's sphere. */
RpAtomic *rwObjectIntersectTriangles(RpAtomic *atomic, RpIntersection *intersection,
                                     RpIntersectionCallBackGeometryTriangle callBack, void *data);

/* The world bounding sphere of an atomic, brought up to date (morph interpolation, frame). */
RwSphere *RpAtomicGetWorldBoundingSphere(RpAtomic *atomic);

/* ---- internals -------------------------------------------------------------------------------------- */

/* Triangle-cube test helpers (unit cube centred on the origin): bevel outcodes of a point. */
__attribute__((regparm(1))) uint32_t _rpCollBevel2d(const RwV3d *p);
__attribute__((regparm(1))) uint32_t _rpCollBevel3d(const RwV3d *p);
/* Face outcodes of p1 + alpha * (p2 - p1), masked: EAX = p1, EDX = p2, ECX = mask, alpha in ST0 (asm entry).
 */
uint32_t _rpCollCheckPoint(void);
/* 0 when the segment p1-p2 crosses one of the cube faces in `outcodes` inside the cube, else 1. */
__attribute__((regparm(3))) int32_t _rpCollCheckLine(const RwV3d *p1, const RwV3d *p2, uint32_t outcodes);
/* 1 when p is outside the triangle a, b, c (all in the plane), 0 inside. */
__attribute__((regparm(3))) int32_t _rpCollPointOutsideTriangle(const RwV3d *p, const RwV3d *a,
                                                                const RwV3d *b, uint32_t unused0,
                                                                uint32_t unused1, uint32_t unused2,
                                                                const RwV3d *c);
/* 1 when the triangle v0, v1, v2 meets the box (the matrix maps the box to the unit cube). */
__attribute__((regparm(3))) int32_t _rpCollTriangleBoxIntersect(
    const RwBBox *box, RwV3d *normal, const RwV3d *v0, uint32_t unused0, uint32_t unused1, uint32_t unused2,
    const RwV3d *v1, const RwV3d *v2, const float *distance, const RwMatrix *matrix);
/* 1 when the triangle meets the sphere; then the unit normal and the plane distance (negated). */
__attribute__((regparm(3))) int32_t _rpCollSphereTriangleIntersect(const RwSphere *sphere, RwV3d *normal,
                                                                   const RwV3d *v0, uint32_t unused0,
                                                                   uint32_t unused1, uint32_t unused2,
                                                                   const RwV3d *v1, const RwV3d *v2,
                                                                   float *distance);

/* Walk the world BSP along a line, calling sectorCallBack for each sector with the clipped line. */
__attribute__((regparm(3))) RpWorld *
_rpCollWorldForAllLineSectors(RpWorld *world, const RwLine *line, RpSectorCallBack sectorCallBack,
                              uint32_t unused0, uint32_t unused1, uint32_t unused2,
                              RpIntersection *intersection, RpCollisionQuery *query);

/* Per-sector tests (cdecl): NULL when the user callback stopped the query, else the sector. */
RpWorldSector *_rpCollSectorAtomicsSphere(RpIntersection *intersection, RpWorldSector *sector,
                                          RpCollisionQuery *query);
RpWorldSector *_rpCollSectorAtomicsBox(RpIntersection *intersection, RpWorldSector *sector,
                                       RpCollisionQuery *query);
RpWorldSector *_rpCollSectorTrianglesSphere(RpIntersection *intersection, RpWorldSector *sector,
                                            RpCollisionQuery *query);
RpWorldSector *_rpCollSectorTrianglesBox(RpIntersection *intersection, RpWorldSector *sector,
                                         RpCollisionQuery *query);
RpWorldSector *_rpCollSectorTrianglesLine(RpIntersection *intersection, RpWorldSector *sector,
                                          RpCollisionQuery *query);
/* Line (start, unit direction, length) against the triangle a, b, c: 1 and the hit as a fraction of the
 * length in *distance. EAX = start, EDX = direction, ECX = a, b / c / distance in the 5th to 7th stack slots,
 * the length in ST0 (popped) (asm entry). */
int32_t _rpCollLineTriangleIntersect(uint32_t unused0, uint32_t unused1, uint32_t unused2, uint32_t unused3,
                                     const RwV3d *b, const RwV3d *c, float *distance);

/* World walkers for one volume type (EAX = world, EDX = volume, ECX = user callback). */
__attribute__((regparm(3))) RpWorld *_rpCollWorldAtomicsPoint(RpWorld *world, const RwV3d *point,
                                                              void *callBack, uint32_t unused0,
                                                              uint32_t unused1, uint32_t unused2,
                                                              RpIntersection *intersection, void *data);
__attribute__((regparm(3))) RpWorld *_rpCollWorldAtomicsLine(RpWorld *world, const RwLine *line,
                                                             void *callBack, uint32_t unused0,
                                                             uint32_t unused1, uint32_t unused2,
                                                             RpIntersection *intersection, void *data);
__attribute__((regparm(3))) RpWorld *_rpCollWorldAtomicsSphere(RpWorld *world, const RwSphere *sphere,
                                                               void *callBack, uint32_t unused0,
                                                               uint32_t unused1, uint32_t unused2,
                                                               RpIntersection *intersection, void *data);
__attribute__((regparm(3))) RpWorld *_rpCollWorldAtomicsBox(RpWorld *world, const RwBBox *box, void *callBack,
                                                            uint32_t unused0, uint32_t unused1,
                                                            uint32_t unused2, RpIntersection *intersection,
                                                            void *data);
__attribute__((regparm(3))) RpWorld *_rpCollWorldAtomicsAtomic(RpWorld *world, RpAtomic *atomic,
                                                               void *callBack, uint32_t unused0,
                                                               uint32_t unused1, uint32_t unused2,
                                                               RpIntersection *intersection, void *data);
__attribute__((regparm(3))) RpWorld *_rpCollWorldTrianglesLine(RpWorld *world, const RwLine *line,
                                                               void *callBack, uint32_t unused0,
                                                               uint32_t unused1, uint32_t unused2,
                                                               RpIntersection *intersection, void *data);
__attribute__((regparm(3))) RpWorld *_rpCollWorldTrianglesSphere(RpWorld *world, const RwSphere *sphere,
                                                                 void *callBack, uint32_t unused0,
                                                                 uint32_t unused1, uint32_t unused2,
                                                                 RpIntersection *intersection, void *data);
__attribute__((regparm(3))) RpWorld *_rpCollWorldTrianglesBox(RpWorld *world, const RwBBox *box,
                                                              void *callBack, uint32_t unused0,
                                                              uint32_t unused1, uint32_t unused2,
                                                              RpIntersection *intersection, void *data);
__attribute__((regparm(3))) RpWorld *_rpCollWorldTrianglesAtomic(RpWorld *world, RpAtomic *atomic,
                                                                 void *callBack, uint32_t unused0,
                                                                 uint32_t unused1, uint32_t unused2,
                                                                 RpIntersection *intersection, void *data);

/* An atomic's triangles against a line or a sphere (EAX = atomic, EDX = volume, ECX = user callback). */
__attribute__((regparm(3))) RpAtomic *
_rpCollAtomicTrianglesLine(RpAtomic *atomic, const RwLine *line,
                           RpIntersectionCallBackGeometryTriangle callBack, uint32_t unused0,
                           uint32_t unused1, uint32_t unused2, RpIntersection *intersection, void *data);
__attribute__((regparm(3))) RpAtomic *
_rpCollAtomicTrianglesSphere(RpAtomic *atomic, const RwSphere *sphere,
                             RpIntersectionCallBackGeometryTriangle callBack, uint32_t unused0,
                             uint32_t unused1, uint32_t unused2, RpIntersection *intersection, void *data);

#endif
