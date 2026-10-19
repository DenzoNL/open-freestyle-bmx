/* Quaternions (C:\NgEngine\Zge\zquat.c). */
#include "zge/zquat.h"

#include "compat/crt.h"
#include "zge/zdebug.h"

/*
 * Floating point follows the original x87 code exactly: `double` is a value held in an x87
 * register (53-bit precision control), `float` is a value the original stored to memory.
 */

static const char zquatFile[] = "C:\\NgEngine\\Zge\\zquat.c";

#define ZQUAT_EPSILON 1e-5f
#define ANGLE_TO_RAD  0x1.921fb6p-14f /* 2*pi / 0x10000 */
#define RAD_TO_ANGLE  0x1.45f306p+13f /* 0x10000 / (2*pi) */
#define ANGLE_TO_DEG  0x1.68p-8f      /* 360 / 0x10000 */
#define DEG_TO_ANGLE  0x1.6c16c2p+7f  /* 0x10000 / 360 */
#define INV_180       0x1.6c16c2p-8f  /* 1 / 180 */

BMX_ADDR(0x00483020)
void zquatIdentity(ZQuat *q)
{
    q->x = 0.0f;
    q->y = 0.0f;
    q->z = 0.0f;
    q->w = 1.0f;
}

/* out = a * b. Safe when out aliases a or b: all four results are computed before the first store. */
BMX_ADDR(0x00483070)
void zquatMul(ZQuat *out, const ZQuat *a, const ZQuat *b)
{
    const double w =
        (((double)b->w * a->w - (double)a->x * b->x) - (double)b->y * a->y) - (double)b->z * a->z;
    const double x =
        (((double)b->w * a->x + (double)b->x * a->w) + (double)b->z * a->y) - (double)b->y * a->z;
    const double y =
        (((double)b->y * a->w - (double)b->z * a->x) + (double)a->y * b->w) + (double)a->z * b->x;
    const double z =
        (((double)b->z * a->w + (double)b->y * a->x) - (double)a->y * b->x) + (double)a->z * b->w;

    out->x = (float)x;
    out->y = (float)y;
    out->z = (float)z;
    out->w = (float)w;
}

/*
 * Splits q into a unit rotation axis and returns the rotation angle (full angle, ZAngle units).
 * A (near) zero vector part yields the X axis and angle 0.
 */
BMX_ADDR(0x00483170)
ZAngle zquatToAxisAngle(ZVec3 *axis, const ZQuat *q)
{
    double len;
    float lenf;
    double half;

    axis->x = q->x;
    axis->y = q->y;
    axis->z = q->z;
    len = zvec3Length(axis);
    lenf = (float)len;
    if (len < ZQUAT_EPSILON) {
        axis->x = 1.0f;
        axis->y = 0.0f;
        axis->z = 0.0f;
        return 0;
    }
    axis->x = (float)((double)q->x / lenf);
    axis->y = (float)((double)q->y / lenf);
    axis->z = (float)((double)q->z / lenf);
    half = (double)zmathAtan2(lenf, q->w) * ANGLE_TO_DEG;
    return (ZAngle)(int)((half + half) * DEG_TO_ANGLE);
}

/*
 * Rotation vector of q: unit axis scaled by acos(w) in units of 180 degrees, flipped when the
 * sine of that angle is negative. The original normalises with a 0.9999 scale factor.
 */
BMX_ADDR(0x00483210)
void zquatToScaledAxis(ZVec3 *out, const ZQuat *q)
{
    const ZAngle a = zmathAcos(q->w);
    float scale;

    out->x = q->x;
    out->y = q->y;
    out->z = q->z;
    scale = (float)((double)a * ANGLE_TO_DEG * INV_180);
    zvec3NormalizeScaled(out, out);
    if (zmathSin(a) < 0.0) {
        out->x = -(scale * out->x);
        out->y = -(scale * out->y);
        out->z = -(scale * out->z);
        return;
    }
    out->x = scale * out->x;
    out->y = scale * out->y;
    out->z = scale * out->z;
}

/* Only rotation order 0 is implemented, as in the original. */
BMX_ADDR(0x004832b0)
void zquatFromEuler(ZQuat *out, const ZAngle3 *euler, int order)
{
    float hy, hz;
    double hx;
    ZAngle ax, ay, az;
    float cx, cy, cz, sx, sy;
    double sz;
    float czcx, szcx, sxcz;
    double szsx;

    if (order != 0) {
        zdebugFail("Not coded", zquatFile, 520, 0);
    }
    /* Half angles via radians. The original keeps x in a register but spills y and z. */
    hx = (double)euler->x * ANGLE_TO_RAD * 0.5;
    hy = (float)((double)euler->y * ANGLE_TO_RAD * 0.5);
    hz = (float)((double)euler->z * ANGLE_TO_RAD * 0.5);
    ax = (ZAngle)(int)(hx * RAD_TO_ANGLE);
    cx = (float)zmathCos(ax);
    ay = (ZAngle)(int)((double)hy * RAD_TO_ANGLE);
    cy = (float)zmathCos(ay);
    az = (ZAngle)(int)((double)hz * RAD_TO_ANGLE);
    cz = (float)zmathCos(az);
    sx = (float)zmathSin(ax);
    sy = (float)zmathSin(ay);
    sz = zmathSin(az);

    czcx = (float)((double)cz * cx);
    szcx = (float)(sz * cx);
    sxcz = (float)((double)sx * cz);
    szsx = sz * sx;
    out->x = (float)((double)sxcz * cy - (double)szcx * sy);
    out->y = (float)(szsx * cy + (double)czcx * sy);
    out->z = (float)((double)szcx * cy - (double)sxcz * sy);
    out->w = (float)(szsx * sy + (double)czcx * cy);
}

/* Note: normalises q in place (through zquatToMatrix). */
BMX_ADDR(0x00483420)
void zquatToEuler(ZAngle3 *out, ZQuat *q, int order)
{
    ZMatrix m;

    if (order != 0) {
        zdebugFail("Not coded", zquatFile, 561, 0);
    }
    zquatToMatrix(&m, q);
    zmatrixToEuler(out, &m, order);
}

/*
 * Normalises q in place and returns the stored (float) length. The unrounded quotients are passed
 * back for callers that, like the inlined copy in zquatToMatrix, keep them in registers.
 */
static float normalizeInline(ZQuat *q, double *xd, double *zd)
{
    const double len = __builtin_sqrt((((double)q->x * q->x + (double)q->y * q->y) + (double)q->z * q->z) +
                                      (double)q->w * q->w);
    const float lenf = (float)len;

    if (len <= ZQUAT_EPSILON) {
        zdebugFail("Division by 0", zquatFile, 647, 0);
    }
    *xd = (double)q->x / lenf;
    q->x = (float)*xd;
    q->y = (float)((double)q->y / lenf);
    *zd = (double)q->z / lenf;
    q->z = (float)*zd;
    q->w = (float)((double)q->w / lenf);
    return lenf;
}

/* Writes the 3x3 rotation part of m only. Normalises q in place first. */
BMX_ADDR(0x00483470)
void zquatToMatrix(ZMatrix *m, ZQuat *q)
{
    double x, z, s, zs, zz;
    float xs, ys, wx, wy, wz, xx, xy, xz, yy, yz;
    const float lenf = normalizeInline(q, &x, &z);

    s = lenf > 0.0f ? 2.0 / lenf : 0.0;
    xs = (float)(x * s);
    ys = (float)(q->y * s);
    zs = z * s;
    wx = (float)((double)q->w * xs);
    wy = (float)((double)q->w * ys);
    wz = (float)(q->w * zs);
    xx = (float)(x * xs);
    xy = (float)(x * ys);
    xz = (float)(x * zs);
    yy = (float)((double)q->y * ys);
    yz = (float)(q->y * zs);
    zz = z * zs;

    m->right.x = (float)(1.0 - (zz + yy));
    m->right.y = (float)((double)xy + wz);
    m->right.z = (float)((double)xz - wy);
    m->up.x = (float)((double)xy - wz);
    m->up.y = (float)(1.0 - (zz + xx));
    m->up.z = (float)((double)yz + wx);
    m->at.x = (float)((double)xz + wy);
    m->at.y = (float)((double)yz - wx);
    m->at.z = (float)(1.0 - ((double)yy + xx));
}

BMX_ADDR(0x00483610)
ZQuat *zquatFromMatrix(ZQuat *out, const ZMatrix *m)
{
    ZAngle3 euler;

    zmatrixToEuler(&euler, m, 0);
    zquatFromEuler(out, &euler, 0);
    return out;
}

/* Normalises q in place and returns its previous length. Asserts on a zero quaternion. */
BMX_ADDR(0x00483670)
float zquatNormalize(ZQuat *q)
{
    double x, z;

    return normalizeInline(q, &x, &z);
}
