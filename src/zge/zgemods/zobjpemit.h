/* Particle emitters and look-at (C:\NgEngine\Zge\ZgeMods\zobjpemit.c). Not ported yet; callees only. */
#ifndef ZGE_ZGEMODS_ZOBJPEMIT_H
#define ZGE_ZGEMODS_ZOBJPEMIT_H

#include "compat/orig.h"
#include "zge/zmath.h"

/* Builds the view matrix of a camera at `eye` looking at `target`; returns out. Callers pass a fourth, unused
 * 0. */
ORIG_FN(0x00495860, ZMatrix *, zobjLookAtMatrix, (ZMatrix * out, const ZVec3 *eye, const ZVec3 *target));

#endif
