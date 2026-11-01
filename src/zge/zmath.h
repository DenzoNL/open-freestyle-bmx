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
 * Lookup tables and trigonometry (C:\\NgEngine\\Zge\\ZgeMods\\zobjCamera.c by address, source file unknown).
 * Functions that return on the x87 stack are declared `double`: the value in st0 is not rounded to
 * float until the caller stores it (see CONTRIBUTING.md, "Floating point").
 */

/* Tables filled by zmathInitTables. Angles index them as (angle & 0xffff) >> 3. */
enum {
    kZmathSinEntries = 0x2800, /* a full turn plus a quarter, so cos = sin shifted by a quarter */
    kZmathCosOffset = 0x800,
    kZmathInverseEntries = 0x2001, /* -1.0 .. 1.0 in steps of 1/4096 */
};

typedef struct ZMathTables {
    float sin[kZmathSinEntries];            /* 0x65e2bc */
    float *cos;                             /* 0x6682bc points at &sin[kZmathCosOffset] */
    int16_t asinData[kZmathInverseEntries]; /* 0x6682c0 asin(x) as ZAngle */
    int16_t pad66c2c2;                      /* 0x66c2c2 */
    int16_t *asin;                          /* 0x66c2c4 points at the centre (x = 0) of asinData */
    int16_t atanData[kZmathInverseEntries]; /* 0x66c2c8 atan(x) as ZAngle */
    int16_t pad6702ca;                      /* 0x6702ca */
    int16_t *atan;                          /* 0x6702cc points at the centre of atanData */
    uint32_t randomSeed;                    /* 0x6702d0 */
} ZMathTables;

_Static_assert(__builtin_offsetof(ZMathTables, cos) == 0xa000, "table layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZMathTables, asin) == 0xe008, "table layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZMathTables, randomSeed) == 0x12014, "table layout");

#define gZmathTables ORIG_VAR(0x0065e2bc, ZMathTables, gZmathTables)

/* Builds the sin/asin/atan tables; always returns 1. */
int zmathInitTables(void);
/* Calls zmathInitTables; always returns 1. */
int zmathInitialize(void);
/* Sine and cosine of an angle from the tables (the low 16 bits of `a` are used). */
double zmathSin(ZAngle a);
double zmathCos(ZAngle a);
/* Arc sine / cosine of s clamped to [-1, 1], as angles. */
ZAngle zmathAsin(float s);
ZAngle zmathAcos(float c);
/* sin / cos of an angle, or a large constant when cos is (nearly) zero. */
double zmathTan(ZAngle a);
/* Angle of the vector (x, y) via the atan table; the original has no zero special case. */
ZAngle zmathAtan2(float y, float x);
/* out = v * 0.9999 / |v|; returns |v| unrounded. */
double zvec3NormalizeScaled(ZVec3 *out, const ZVec3 *v);
double zvec3Length(const ZVec3 *v);
/* Steps the engine's pseudo random generator (seed at 0x6702d0) and returns the new value. */
uint32_t zmathRandom(void);
/* Euler angles of a rotation matrix. The original takes two arguments; the third is ignored. */
void zmatrixToEuler(ZAngle3 *out, const ZMatrix *m, int order);

#endif
