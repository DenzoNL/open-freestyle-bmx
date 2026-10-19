/* Quaternions (C:\NgEngine\Zge\zquat.c). */
#ifndef ZGE_ZQUAT_H
#define ZGE_ZQUAT_H

#include "zge/zmath.h"

void zquatIdentity(ZQuat *q);
void zquatMul(ZQuat *out, const ZQuat *a, const ZQuat *b);
ZAngle zquatToAxisAngle(ZVec3 *axis, const ZQuat *q);
void zquatToScaledAxis(ZVec3 *out, const ZQuat *q);
void zquatFromEuler(ZQuat *out, const ZAngle3 *euler, int order);
void zquatToEuler(ZAngle3 *out, ZQuat *q, int order);
void zquatToMatrix(ZMatrix *m, ZQuat *q);
ZQuat *zquatFromMatrix(ZQuat *out, const ZMatrix *m);
float zquatNormalize(ZQuat *q);

#endif
