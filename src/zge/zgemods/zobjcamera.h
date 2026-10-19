/*
 * Vector and matrix helpers that data/functions.csv files under zobjcamera.c (probably misplaced).
 * Not ported yet; the names follow what the callers do with them.
 */
#ifndef ZGE_ZGEMODS_ZOBJCAMERA_H
#define ZGE_ZGEMODS_ZOBJCAMERA_H

#include "compat/orig.h"
#include "zge/zmath.h"

/* out = in rotated about the X / Y / Z axis. Only the low 16 bits of the angle are read. */
ORIG_FN(0x00481340, void, zvec3RotateX, (ZVec3 * out, const ZVec3 *in, ZAngle angle));
ORIG_FN(0x00481390, void, zvec3RotateY, (ZVec3 * out, const ZVec3 *in, ZAngle angle));
ORIG_FN(0x004813e0, void, zvec3RotateZ, (ZVec3 * out, const ZVec3 *in, ZAngle angle));
ORIG_FN(0x00481540, double, zvec3Dot, (const ZVec3 *a, const ZVec3 *b));
/* out = (b - a) x (c - b). */
ORIG_FN(0x004815b0, void, zvec3Cross3, (ZVec3 * out, const ZVec3 *a, const ZVec3 *b, const ZVec3 *c));
ORIG_FN(0x00481880, void, zmatrixIdentity, (ZMatrix * m));
ORIG_FN(0x00481a00, void, zmatrixRotateY, (ZMatrix * out, const ZMatrix *in, ZAngle angle));
ORIG_FN(0x00481ba0, ZMatrix *, zmatrixRotateEuler, (ZMatrix * out, const ZMatrix *in, const ZAngle3 *angles));
ORIG_FN(0x00481c60, ZMatrix *, zmatrixCopyRotation, (ZMatrix * out, const ZMatrix *in));
ORIG_FN(0x00481cf0, void, zmatrixMul, (ZMatrix * out, const ZMatrix *a, const ZMatrix *b));

#endif
