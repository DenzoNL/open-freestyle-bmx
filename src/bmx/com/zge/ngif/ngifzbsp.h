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
/* A face near p (within `radius`, normal within `cosLimit` of up), or NULL. */
ORIG_FN(0x00468070, void *, zbspFindFaceNear, (const ZVec3 *p, float radius, float cosLimit));

#endif
