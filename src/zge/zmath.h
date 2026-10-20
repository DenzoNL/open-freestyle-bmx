/* Engine maths types and helpers (original source file unknown; no asserts). */
#ifndef ZGE_ZMATH_H
#define ZGE_ZMATH_H

#include <stdint.h>

#include "compat/orig.h"

/* Angles are 16-bit fixed point: 0x10000 is a full turn. */
typedef int16_t ZAngle;

typedef struct ZVec3 {
    float x, y, z;
} ZVec3;

typedef struct ZAngle3 {
    ZAngle x, y, z;
} ZAngle3;

typedef struct ZQuat {
    float x, y, z, w;
} ZQuat;

/* RenderWare RwMatrix layout: three basis rows and a position, each padded to 16 bytes. */
typedef struct ZMatrix {
    ZVec3 right;
    uint32_t flags;
    ZVec3 up;
    uint32_t pad1;
    ZVec3 at;
    uint32_t pad2;
    ZVec3 pos;
    uint32_t pad3;
} ZMatrix;

_Static_assert(sizeof(ZMatrix) == 64, "RwMatrix layout");

/*
 * Not yet ported. Functions that return on the x87 stack are declared `double`: the value in st0
 * is not rounded to float until the caller stores it (see CONTRIBUTING.md, "Floating point").
 */
ORIG_FN(0x00480d20, int, zmathInitTables, (void));
ORIG_FN(0x00480e40, double, zmathSin, (ZAngle a));
ORIG_FN(0x00480e60, ZAngle, zmathAsin, (float s));
ORIG_FN(0x00480eb0, double, zmathCos, (ZAngle a));
ORIG_FN(0x00480ed0, ZAngle, zmathAcos, (float c));
ORIG_FN(0x00480ff0, ZAngle, zmathAtan2, (float y, float x));
ORIG_FN(0x004811c0, double, zvec3NormalizeScaled, (ZVec3 * out, const ZVec3 *v));
ORIG_FN(0x00481310, double, zvec3Length, (const ZVec3 *v));
/* Steps the engine's pseudo random generator (seed at 0x6702d0) and returns the new value. */
ORIG_FN(0x00481f80, uint32_t, zmathRandom, (void));
ORIG_FN(0x00481fc0, void, zmatrixToEuler, (ZAngle3 * out, const ZMatrix *m, int order));

#endif
