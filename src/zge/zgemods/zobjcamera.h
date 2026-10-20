/*
 * Vector and matrix helpers that data/functions.csv files under zobjcamera.c (probably misplaced).
 * Not ported yet; the names follow what the callers do with them.
 */
#ifndef ZGE_ZGEMODS_ZOBJCAMERA_H
#define ZGE_ZGEMODS_ZOBJCAMERA_H

#include "compat/orig.h"
#include "zge/zmath.h"

/* A zero vector; passed as the middle point of zvec3Cross3 to get a plain cross product. */
#define gZmathOrigin ORIG_VAR(0x006702d8, ZVec3, gZmathOrigin)

ORIG_FN(0x00481120, void, zvec3Zero, (ZVec3 * v));
ORIG_FN(0x00481130, void, zvec3Add, (ZVec3 * out, const ZVec3 *a, const ZVec3 *b));
/* out = a - b. */
ORIG_FN(0x00481160, void, zvec3Sub, (ZVec3 * out, const ZVec3 *a, const ZVec3 *b));
ORIG_FN(0x00481210, void, zvec3Scale, (ZVec3 * out, const ZVec3 *in, float s));
ORIG_FN(0x004812f0, void, zvec3Negate, (ZVec3 * out, const ZVec3 *in));
/* out = in rotated about the X / Y / Z axis. Only the low 16 bits of the angle are read. */
ORIG_FN(0x00481340, void, zvec3RotateX, (ZVec3 * out, const ZVec3 *in, ZAngle angle));
ORIG_FN(0x00481390, void, zvec3RotateY, (ZVec3 * out, const ZVec3 *in, ZAngle angle));
ORIG_FN(0x004813e0, void, zvec3RotateZ, (ZVec3 * out, const ZVec3 *in, ZAngle angle));
/* Rotates a vector about x, then y, then z (every caller passes the same vector as `out` and `in`). */
ORIG_FN(0x00481430, void, zvec3RotateEuler, (ZVec3 * out, const ZVec3 *in, const ZAngle3 *angles));
ORIG_FN(0x00481540, double, zvec3Dot, (const ZVec3 *a, const ZVec3 *b));
/* out = (b - a) x (c - b). */
ORIG_FN(0x004815b0, void, zvec3Cross3, (ZVec3 * out, const ZVec3 *a, const ZVec3 *b, const ZVec3 *c));
/* out = m * v, including the translation row. */
ORIG_FN(0x00481640, void, zmatrixTransformPoint, (ZVec3 * out, const ZMatrix *m, const ZVec3 *v));
ORIG_FN(0x00481880, void, zmatrixIdentity, (ZMatrix * m));
/* Clears the position row. */
ORIG_FN(0x004818b0, void, zmatrixZeroPos, (ZMatrix * m));
/* Matrix rotations about X / Y / Z; only the low 16 bits of the angle are read. */
ORIG_FN(0x00481930, void, zmatrixRotateX, (ZMatrix * out, const ZMatrix *in, ZAngle angle));
ORIG_FN(0x00481a00, void, zmatrixRotateY, (ZMatrix * out, const ZMatrix *in, ZAngle angle));
ORIG_FN(0x00481ad0, void, zmatrixRotateZ, (ZMatrix * out, const ZMatrix *in, ZAngle angle));
ORIG_FN(0x00481ba0, ZMatrix *, zmatrixRotateEuler, (ZMatrix * out, const ZMatrix *in, const ZAngle3 *angles));
ORIG_FN(0x00481c60, ZMatrix *, zmatrixCopyRotation, (ZMatrix * out, const ZMatrix *in));
ORIG_FN(0x00481cf0, void, zmatrixMul, (ZMatrix * out, const ZMatrix *a, const ZMatrix *b));
/* Transposes the 3x3 rotation part. */
ORIG_FN(0x00481e90, void, zmatrixTranspose, (ZMatrix * out, const ZMatrix *in));

/* Sine table lookup; only the low 16 bits of the angle are read (angle >> 3 indexes the table). */
ORIG_FN(0x00480e40, double, zmathSinTable, (uint32_t angle));

#endif
