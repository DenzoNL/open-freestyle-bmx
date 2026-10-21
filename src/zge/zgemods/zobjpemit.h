/* Particle emitters and look-at (C:\NgEngine\Zge\ZgeMods\zobjpemit.c). Not ported yet; callees only. */
#ifndef ZGE_ZGEMODS_ZOBJPEMIT_H
#define ZGE_ZGEMODS_ZOBJPEMIT_H

#include "compat/orig.h"
#include "zge/zmath.h"

/* Builds the view matrix of a camera at `eye` looking at `target`; returns out. Callers pass a fourth, unused
 * 0. */
ORIG_FN(0x00495860, ZMatrix *, zobjLookAtMatrix, (ZMatrix * out, const ZVec3 *eye, const ZVec3 *target));

/* Builds the clipping plane through `point` for the face direction `dir` (a plane is 5 floats). */
ORIG_FN(0x004956f0, void, zobjPlaneFromFace, (float *plane, const float *point, const float *dir));
/* Clips the segment a-b against a plane in place; returns 0 when nothing is left. */
ORIG_FN(0x00495730, int, zobjClipSegmentToPlane, (float *a, float *b, const float *plane));

#endif
