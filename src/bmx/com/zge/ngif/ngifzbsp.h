/* Level geometry queries (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzbsp.c). Not ported yet; callees only. */
#ifndef BMX_COM_ZGE_NGIF_NGIFZBSP_H
#define BMX_COM_ZGE_NGIF_NGIFZBSP_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

/* Centroid of the three float vertices of a face (at +0x30, stride 0x10). */
ORIG_FN(0x00466c50, void, zbspFaceCentroid, (const void *face, ZVec3 *out));
/* Null-terminated list of edges within `radius` of p (static buffer). */
ORIG_FN(0x00466ed0, void **, zbspFindEdgesNear, (const ZVec3 *p, float radius));
/* The `index`-th edge tagged `id`, rebuilt in a static record, or NULL. */
ORIG_FN(0x00467410, void *, zbspFindEdge, (int unused, int id, int index, int unused2));
/* Point of the segment a-b (int16 coordinates, direction dir) closest to p. */
ORIG_FN(0x00468920, void, zbspClosestPointOnEdge,
        (const int16_t *a, const int16_t *b, const ZVec3 *dir, const ZVec3 *p, ZVec3 *out));
/* Null-terminated list of faces whose tag id lies in [lo, hi] (static buffer). */
ORIG_FN(0x00468ef0, void **, zbspFindTaggedFaces, (int lo, int hi));
/*
 * Faces near p (within `radius`, |normal.y| within `cosLimit`) as a ZbspFaceQuery in a static record, or
 * NULL.
 */
ORIG_FN(0x00468070, void *, zbspFindFaceNear, (const ZVec3 *p, float radius, float cosLimit));
/* Null-terminated list of the collision objects of props whose class lies in [lo, hi] (static buffer). */
ORIG_FN(0x00468e30, void **, zbspFindPropsNear, (int lo, int hi));
/* Which side of an edge is solid: 1 (flag 1), -1 (flag 2) or 0. */
ORIG_FN(0x00467690, int, zbspEdgeSide, (const void *edge));

/* One face of a zbspFindFaceNear result; the list ends with a NULL face. */
typedef struct ZbspFaceHit {
    void *face;  /* PhysFace */
    float dist;  /* distance of p above the face plane */
    ZVec3 point; /* closest point of the face */
    uint32_t pad14;
    int32_t object; /* owning prop's collision object, 0 for level geometry */
} ZbspFaceHit;

typedef struct ZbspFaceQuery {
    uint8_t pad00[0x3c];
    ZbspFaceHit *hits;
} ZbspFaceQuery;

_Static_assert(sizeof(ZbspFaceHit) == 0x1c, "ZbspFaceHit size");

#endif
