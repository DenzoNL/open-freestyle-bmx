/*
 * Camera objects (C:\NgEngine\Zge\ZgeMods\zobjCamera.c) and, by address, the engine's lookup tables,
 * vector and matrix helpers (0x480d20-0x481fc0; the data file places them here).
 *
 * Floating point follows the original x87 code exactly: `double` is a value held in an x87 register
 * (53-bit precision control), `float` is a value the original stored to memory.
 */
#include "zge/zgemods/zobjcamera.h"

#include "compat/crt.h"
#include "compat/x87.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjpemit.h"
#include "zge/zobj.h"
#include "zge/zquat.h"

#define ANGLE_TO_RAD 0x1.921fb6p-14f /* 2*pi / 0x10000 */
#define RAD_TO_ANGLE 0x1.45f306p+13f /* 0x10000 / (2*pi) */

/* Table position of an angle. */
static inline uint32_t tableIndex(uint32_t angle)
{
    return (angle & 0xffff) >> 3;
}

BMX_ADDR(0x00480d20)
int zmathInitTables(void)
{
    ZMathTables *t = &gZmathTables;
    int n;

    t->cos = &t->sin[kZmathCosOffset];
    for (n = 0; n < kZmathSinEntries; n++) {
        /* The angle wraps at 16 bits, so the tail of the table repeats the first quarter. */
        t->sin[n] = bmxX87SinScaled((int16_t)(n * 8), ANGLE_TO_RAD);
    }
    t->asin = &t->asinData[kZmathInverseEntries / 2];
    for (n = 0; n < kZmathInverseEntries; n++) {
        const double v = n == 0 || n == kZmathInverseEntries - 1
                             ? bmxX87HalfPiScaled(n == 0, RAD_TO_ANGLE)
                             : bmxX87AsinScaled(n - 0x1000, 0x1p-12, RAD_TO_ANGLE);
        t->asinData[n] = (int16_t)bmxFtol(v);
    }
    t->atan = &t->atanData[kZmathInverseEntries / 2];
    for (n = 0; n < kZmathInverseEntries; n++) {
        t->atanData[n] = (int16_t)bmxFtol(bmxX87AtanScaled(n - 0x1000, 0x1p-12, RAD_TO_ANGLE));
    }
    return 1;
}

BMX_ADDR(0x00481f50)
int zmathInitialize(void)
{
    zmathInitTables();
    return 1;
}

BMX_ADDR(0x00480e40)
double zmathSin(ZAngle a)
{
    return gZmathTables.sin[tableIndex((uint32_t)a)];
}

BMX_ADDR(0x00480eb0)
double zmathCos(ZAngle a)
{
    return gZmathTables.cos[tableIndex((uint32_t)a)];
}

/* Clamps to [-1, 1] and returns the table index of -v * 4096. NaN clamps to -1. */
static int clampedIndex(float v)
{
    double d = v;

    if (!(d >= 1.0)) {
        if (!(d > -1.0)) {
            d = -1.0;
        }
    } else {
        d = 1.0;
    }
    return bmxFtol(d * -4096.0f);
}

BMX_ADDR(0x00480e60)
ZAngle zmathAsin(float s)
{
    return gZmathTables.asin[-clampedIndex(s)];
}

BMX_ADDR(0x00480ed0)
ZAngle zmathAcos(float c)
{
    return (ZAngle)(0x3fff - gZmathTables.asin[-clampedIndex(c)]);
}

BMX_ADDR(0x00480f30)
double zmathTan(ZAngle a)
{
    const uint32_t i = tableIndex((uint32_t)a);
    const double cosv = gZmathTables.cos[i];

    if (__builtin_fabs(cosv) < 1e-5f) {
        return 99999.9921875f;
    }
    return gZmathTables.sin[i] / cosv;
}

/*
 * atan table lookup for the ratio of the smaller to the larger magnitude; `inverted` when the ratio is
 * x / y (the result is then 0x4000 - angle). Out-of-range ratios (only NaN gets there) take the
 * reciprocal path, as in the original.
 */
static int atanRatio(double r, int inverted)
{
    const int16_t *table = gZmathTables.atan;
    int base;

    if (r <= 1.0f && !(r < -1.0f)) {
        return inverted ? 0x4000 - table[-bmxFtol(r * -4096.0f)] : table[-bmxFtol(r * -4096.0f)];
    }
    base = r > 1.0f ? 0x4000 : -0x4000;
    base = (int16_t)(base - table[-bmxFtol((1.0f / r) * -4096.0f)]);
    return inverted ? 0x4000 - base : base;
}

BMX_ADDR(0x00480ff0)
ZAngle zmathAtan2(float y, float x)
{
    int flip = 0, add = 0;
    double r;
    int angle;

    /* The comparisons are x87 FCOMs: NaN counts as "below zero". */
    if (!(y >= 0.0f)) {
        y = __builtin_fabsf(y);
        flip = -1;
    }
    if (!(x >= 0.0f)) {
        x = __builtin_fabsf(x);
        add = 0x8000;
        flip++;
    }
    if (y <= x || y != y || x != x) {
        r = (double)y / x;
        angle = atanRatio(r, 0);
    } else {
        r = (double)x / y;
        angle = atanRatio(r, 1);
    }
    angle += add;
    return (ZAngle)(flip != 0 ? -angle : angle);
}

BMX_ADDR(0x00481f80)
uint32_t zmathRandom(void)
{
    const uint32_t s = gZmathTables.randomSeed;
    uint32_t v = ((s >> 16 | s << 16) + (s >> 28 | s << 4)) ^ 0x87654321u;

    v = v >> 16 | v << 16;
    gZmathTables.randomSeed = v;
    return v;
}

BMX_ADDR(0x00481120)
void zvec3Zero(ZVec3 *v)
{
    v->z = 0.0f;
    v->y = 0.0f;
    v->x = 0.0f;
}

BMX_ADDR(0x00481130)
void zvec3Add(ZVec3 *out, const ZVec3 *a, const ZVec3 *b)
{
    out->x = (float)((double)a->x + b->x);
    out->y = (float)((double)a->y + b->y);
    out->z = (float)((double)a->z + b->z);
}

BMX_ADDR(0x00481160)
void zvec3Sub(ZVec3 *out, const ZVec3 *a, const ZVec3 *b)
{
    out->x = (float)((double)a->x - b->x);
    out->y = (float)((double)a->y - b->y);
    out->z = (float)((double)a->z - b->z);
}

BMX_ADDR(0x004811c0)
double zvec3NormalizeScaled(ZVec3 *out, const ZVec3 *v)
{
    const double len = __builtin_sqrt(((double)v->x * v->x + (double)v->y * v->y) + (double)v->z * v->z);
    const double s = 0.9998999834060669 / len;

    out->x = (float)(s * v->x);
    out->y = (float)(s * v->y);
    out->z = (float)(s * v->z);
    return len;
}

BMX_ADDR(0x00481210)
void zvec3Scale(ZVec3 *out, const ZVec3 *in, float s)
{
    out->x = (float)((double)s * in->x);
    out->y = (float)((double)s * in->y);
    out->z = (float)((double)s * in->z);
}

BMX_ADDR(0x00481240)
void zvec3ScaleXYZ(ZVec3 *out, const ZVec3 *in, float sx, float sy, float sz)
{
    out->x = (float)((double)sx * in->x);
    out->y = (float)((double)sy * in->y);
    out->z = (float)((double)sz * in->z);
}

BMX_ADDR(0x004812c0)
double zvec3LengthSq(const ZVec3 *v)
{
    return ((double)v->x * v->x + (double)v->y * v->y) + (double)v->z * v->z;
}

BMX_ADDR(0x004812f0)
void zvec3Negate(ZVec3 *out, const ZVec3 *in)
{
    out->x = -in->x;
    out->y = -in->y;
    out->z = -in->z;
}

BMX_ADDR(0x00481310)
double zvec3Length(const ZVec3 *v)
{
    return __builtin_sqrt(((double)v->x * v->x + (double)v->y * v->y) + (double)v->z * v->z);
}

BMX_ADDR(0x00481340)
void zvec3RotateX(ZVec3 *out, const ZVec3 *in, ZAngle angle)
{
    const uint32_t i = tableIndex((uint32_t)angle);

    bmxX87Rotate(&out->y, &out->z, in->y, in->z, &gZmathTables.cos[i], &gZmathTables.sin[i]);
}

BMX_ADDR(0x00481390)
void zvec3RotateY(ZVec3 *out, const ZVec3 *in, ZAngle angle)
{
    const uint32_t i = tableIndex((uint32_t)angle);

    bmxX87Rotate(&out->x, &out->z, in->x, in->z, &gZmathTables.cos[i], &gZmathTables.sin[i]);
}

BMX_ADDR(0x004813e0)
void zvec3RotateZ(ZVec3 *out, const ZVec3 *in, ZAngle angle)
{
    const uint32_t i = tableIndex((uint32_t)angle);

    bmxX87Rotate(&out->x, &out->y, in->x, in->y, &gZmathTables.cos[i], &gZmathTables.sin[i]);
}

BMX_ADDR(0x00481430)
void zvec3RotateEuler(ZVec3 *out, const ZVec3 *in, const ZAngle3 *angles)
{
    const float y = in->y, z = in->z;
    uint32_t i = tableIndex((uint16_t)angles->x);
    float c = gZmathTables.cos[i], s = gZmathTables.sin[i];
    float x;
    double v1, v2, v3, v4;
    float v3f, sy;

    x = out->x;
    v1 = (double)c * y - (double)s * z;
    out->y = (float)v1;
    v2 = (double)c * z + (double)s * y;
    out->z = (float)v2;

    i = tableIndex((uint16_t)angles->y);
    c = gZmathTables.cos[i];
    sy = gZmathTables.sin[i];
    v3 = (double)c * x - (double)sy * v2;
    out->x = (float)v3;
    v4 = (double)c * v2 + (double)sy * x;
    out->z = (float)v4;
    v3f = (float)v3;

    i = tableIndex((uint16_t)angles->z);
    c = gZmathTables.cos[i];
    s = gZmathTables.sin[i];
    out->x = (float)((double)c * v3f - (double)s * v1);
    out->y = (float)((double)c * v1 + (double)s * v3f);
}

BMX_ADDR(0x00481540)
double zvec3Dot(const ZVec3 *a, const ZVec3 *b)
{
    return ((double)a->z * b->z + (double)a->y * b->y) + (double)a->x * b->x;
}

BMX_ADDR(0x00481560)
void zvec3Cross(ZVec3 *out, const ZVec3 *a, const ZVec3 *b)
{
    const double x = (double)b->z * a->y - (double)b->y * a->z;
    const double y = (double)b->x * a->z - (double)b->z * a->x;
    const float z = (float)((double)b->y * a->x - (double)b->x * a->y);

    out->x = (float)x;
    out->z = z;
    out->y = (float)y;
}

BMX_ADDR(0x004815b0)
void zvec3Cross3(ZVec3 *out, const ZVec3 *a, const ZVec3 *b, const ZVec3 *c)
{
    const double e1x = (double)b->x - a->x;
    const float e1y = (float)((double)b->y - a->y);
    const double e1z = (double)b->z - a->z;
    const float e2x = (float)((double)c->x - b->x);
    const float e2y = (float)((double)c->y - b->y);
    const double e2z = (double)c->z - b->z;
    const float x = (float)(e2z * e1y - (double)e2y * e1z);
    const double y = e1z * e2x - e2z * e1x;
    const float z = (float)((double)e2y * e1x - (double)e2x * e1y);

    out->x = x;
    out->z = z;
    out->y = (float)y;
}

BMX_ADDR(0x00481640)
void zmatrixTransformPoint(ZVec3 *out, const ZMatrix *m, const ZVec3 *v)
{
    const float x = v->x, y = v->y, z = v->z;

    out->x = (float)(((double)z * m->at.x + (double)y * m->up.x) + (double)x * m->right.x + m->pos.x);
    out->y = (float)(((double)z * m->at.y + (double)y * m->up.y) + (double)x * m->right.y + m->pos.y);
    out->z = (float)(((double)z * m->at.z + (double)y * m->up.z) + (double)x * m->right.z + m->pos.z);
}

BMX_ADDR(0x00481780)
void zmatrixTransformVector(ZVec3 *out, const ZMatrix *m, const ZVec3 *v)
{
    const float x = v->x, y = v->y, z = v->z;

    out->x = (float)(((double)z * m->at.x + (double)y * m->up.x) + (double)x * m->right.x);
    out->y = (float)(((double)z * m->at.y + (double)y * m->up.y) + (double)x * m->right.y);
    out->z = (float)(((double)z * m->at.z + (double)y * m->up.z) + (double)x * m->right.z);
}

BMX_ADDR(0x004817e0)
void zvec3Lerp(ZVec3 *out, const ZVec3 *a, const ZVec3 *b, float t)
{
    out->x = (float)(((double)b->x - a->x) * t + a->x);
    out->y = (float)(((double)b->y - a->y) * t + a->y);
    out->z = (float)(((double)b->z - a->z) * t + a->z);
}

BMX_ADDR(0x00481820)
void zvec3AddScalar(ZVec3 *out, const ZVec3 *in, float s)
{
    out->x = (float)((double)s + in->x);
    out->y = (float)((double)s + in->y);
    out->z = (float)((double)s + in->z);
}

BMX_ADDR(0x00481850)
void zvec3SubScalar(ZVec3 *out, const ZVec3 *in, float s)
{
    out->x = (float)((double)in->x - s);
    out->y = (float)((double)in->y - s);
    out->z = (float)((double)in->z - s);
}

BMX_ADDR(0x00481880)
void zmatrixIdentity(ZMatrix *m)
{
    m->pos.z = 0.0f;
    m->pos.y = 0.0f;
    m->pos.x = 0.0f;
    m->at.z = 1.0f;
    m->up.y = 1.0f;
    m->right.x = 1.0f;
    m->up.z = 0.0f;
    m->right.z = 0.0f;
    m->at.y = 0.0f;
    m->right.y = 0.0f;
    m->at.x = 0.0f;
    m->up.x = 0.0f;
}

BMX_ADDR(0x004818b0)
void zmatrixZeroPos(ZMatrix *m)
{
    m->pos.z = 0.0f;
    m->pos.y = 0.0f;
    m->pos.x = 0.0f;
}

BMX_ADDR(0x00481cb0)
void zmatrixCopyBasis(ZMatrix *out, const ZMatrix *in)
{
    out->right = in->right;
    out->up = in->up;
    out->at = in->at;
}

BMX_ADDR(0x004818f0)
ZMatrix *zmatrixTranslate(ZMatrix *out, const ZMatrix *in, const ZVec3 *offset)
{
    if (out != in) {
        zmatrixCopyBasis(out, in);
    }
    out->pos.x = (float)((double)in->pos.x + offset->x);
    out->pos.y = (float)((double)in->pos.y + offset->y);
    out->pos.z = (float)((double)in->pos.z + offset->z);
    return out;
}

/* Copies position then basis when the matrices differ. */
static void copyUnlessSame(ZMatrix *out, const ZMatrix *in)
{
    if (in != out) {
        out->pos = in->pos;
        zmatrixCopyBasis(out, in);
    }
}

BMX_ADDR(0x00481930)
ZMatrix *zmatrixRotateX(ZMatrix *out, const ZMatrix *in, ZAngle angle)
{
    const uint32_t i = tableIndex((uint32_t)angle);
    const float c = gZmathTables.cos[i], s = gZmathTables.sin[i];
    ZVec3 *rows[4];
    int r;

    copyUnlessSame(out, in);
    rows[0] = &out->right;
    rows[1] = &out->up;
    rows[2] = &out->at;
    rows[3] = &out->pos;
    for (r = 0; r < 4; r++) {
        const float y = rows[r]->y, z = rows[r]->z;

        rows[r]->y = (float)((double)y * c - (double)s * z);
        rows[r]->z = (float)((double)c * z + (double)y * s);
    }
    return out;
}

BMX_ADDR(0x00481a00)
const ZMatrix *zmatrixRotateY(ZMatrix *out, const ZMatrix *in, ZAngle angle)
{
    const uint32_t i = tableIndex((uint32_t)angle);
    const float c = gZmathTables.cos[i], s = gZmathTables.sin[i];
    ZVec3 *rows[4];
    int r;

    copyUnlessSame(out, in);
    rows[0] = &out->right;
    rows[1] = &out->up;
    rows[2] = &out->at;
    rows[3] = &out->pos;
    for (r = 0; r < 4; r++) {
        const float x = rows[r]->x, z = rows[r]->z;

        rows[r]->x = (float)((double)x * c + (double)s * z);
        rows[r]->z = (float)((double)c * z - (double)x * s);
    }
    return in;
}

BMX_ADDR(0x00481ad0)
const ZMatrix *zmatrixRotateZ(ZMatrix *out, const ZMatrix *in, ZAngle angle)
{
    const uint32_t i = tableIndex((uint32_t)angle);
    const float c = gZmathTables.cos[i], s = gZmathTables.sin[i];
    ZVec3 *rows[4];
    int r;

    copyUnlessSame(out, in);
    rows[0] = &out->right;
    rows[1] = &out->up;
    rows[2] = &out->at;
    rows[3] = &out->pos;
    for (r = 0; r < 4; r++) {
        const float x = rows[r]->x, y = rows[r]->y;

        rows[r]->x = (float)((double)x * c - (double)s * y);
        rows[r]->y = (float)((double)c * y + (double)x * s);
    }
    return in;
}

BMX_ADDR(0x00481ba0)
ZMatrix *zmatrixRotateEuler(ZMatrix *out, const ZMatrix *in, const ZAngle3 *angles)
{
    zmatrixRotateX(out, in, angles->x);
    zmatrixRotateY(out, out, angles->y);
    zmatrixRotateZ(out, out, angles->z);
    return out;
}

BMX_ADDR(0x00481be0)
ZMatrix *zmatrixScaleDiagonal(ZMatrix *out, const ZMatrix *in, const ZVec3 *scale)
{
    copyUnlessSame(out, in);
    out->right.x = in->right.x * scale->x;
    out->up.y = in->up.y * scale->y;
    out->at.z = in->at.z * scale->z;
    return out;
}

BMX_ADDR(0x00481c60)
ZMatrix *zmatrixCopyRotation(ZMatrix *out, const ZMatrix *in)
{
    out->pos = in->pos;
    zmatrixCopyBasis(out, in);
    return out;
}

BMX_ADDR(0x00481cf0)
void zmatrixMul(ZMatrix *out, const ZMatrix *a, const ZMatrix *b)
{
    const float a0 = a->right.x, a1 = a->right.y, a2 = a->right.z;
    const float a4 = a->up.x, a5 = a->up.y, a6 = a->up.z;
    const float a8 = a->at.x, a9 = a->at.y, a10 = a->at.z;
    const float a12 = a->pos.x, a13 = a->pos.y, a14 = a->pos.z;

    out->right.x = (float)(((double)a8 * b->right.z + (double)a4 * b->right.y) + (double)a0 * b->right.x);
    out->right.y = (float)(((double)a9 * b->right.z + (double)a5 * b->right.y) + (double)a1 * b->right.x);
    out->right.z = (float)(((double)a10 * b->right.z + (double)a6 * b->right.y) + (double)a2 * b->right.x);
    out->up.x = (float)(((double)a8 * b->up.z + (double)a4 * b->up.y) + (double)a0 * b->up.x);
    out->up.y = (float)(((double)a9 * b->up.z + (double)a5 * b->up.y) + (double)a1 * b->up.x);
    out->up.z = (float)(((double)a10 * b->up.z + (double)a6 * b->up.y) + (double)a2 * b->up.x);
    out->at.x = (float)(((double)a0 * b->at.x + (double)a8 * b->at.z) + (double)a4 * b->at.y);
    out->at.y = (float)(((double)a1 * b->at.x + (double)a9 * b->at.z) + (double)a5 * b->at.y);
    out->at.z = (float)(((double)a2 * b->at.x + (double)a10 * b->at.z) + (double)a6 * b->at.y);
    out->pos.x = (float)((((double)a8 * b->pos.z + (double)a4 * b->pos.y) + (double)a0 * b->pos.x) + a12);
    out->pos.y = (float)((((double)a9 * b->pos.z + (double)a5 * b->pos.y) + (double)a1 * b->pos.x) + a13);
    out->pos.z = (float)((((double)a10 * b->pos.z + (double)a6 * b->pos.y) + (double)a2 * b->pos.x) + a14);
}

BMX_ADDR(0x00481e90)
void zmatrixTranspose(ZMatrix *out, const ZMatrix *in)
{
    const float m1 = in->right.y, m2 = in->right.z, m4 = in->up.x, m6 = in->up.z, m8 = in->at.x,
                m9 = in->at.y;

    if (out != in) {
        out->right.x = in->right.x;
        out->up.y = in->up.y;
        out->at.z = in->at.z;
    }
    out->right.y = m4;
    out->right.z = m8;
    out->up.x = m1;
    out->at.x = m2;
    out->at.y = m6;
    out->up.z = m9;
}

BMX_ADDR(0x00481f00)
ZMatrix *zmatrixInvertRigid(ZMatrix *out, const ZMatrix *in)
{
    ZVec3 neg;

    zmatrixTranspose(out, in);
    neg.x = -in->pos.x;
    neg.y = -in->pos.y;
    neg.z = -in->pos.z;
    zmatrixTransformVector(&out->pos, out, &neg);
    return out;
}

BMX_ADDR(0x00481fc0)
void zmatrixToEuler(ZAngle3 *out, const ZMatrix *m, int order)
{
    const double len = __builtin_sqrt((double)m->right.y * m->right.y + (double)m->right.x * m->right.x);
    const float lenf = (float)len;

    if (len > 0.0010000000474974513) {
        const ZAngle x = zmathAtan2(m->up.z, m->at.z);
        const ZAngle y = zmathAtan2(-m->right.z, lenf);
        const ZAngle z = zmathAtan2(m->right.y, m->right.x);

        out->x = x;
        out->y = y;
        out->z = z;
        return;
    }
    {
        const ZAngle x = zmathAtan2(-m->at.y, m->up.y);
        const ZAngle y = zmathAtan2(-m->right.z, lenf);

        out->x = x;
        out->y = y;
        out->z = 0;
    }
}

static const char zobjCameraFile[] = "C:\\NgEngine\\Zge\\ZgeMods\\zobjCamera.c";

BMX_ADDR(0x00482090)
int zobjCameraDestroy(ZObjCamera *camera)
{
    gZrwCamDestroy(camera->camera);
    zmemFreeListFree(gZobjCameraCaches, camera->cache);
    return 1;
}

BMX_ADDR(0x004820c0)
int zobjCameraDraw(ZObjCamera *camera)
{
    ZMatrix *view = zobjCameraGetMatrix(camera, kZobjCameraMatrixView);

    gZrwCamSetView(camera->camera, view, &camera->pos);
    return 1;
}

BMX_ADDR(0x004820f0)
int zobjCameraMoved(ZObjCamera *camera)
{
    camera->cache->valid = 0;
    return 1;
}

BMX_ADDR(0x00482110)
int zobjCameraRotated(ZObjCamera *camera)
{
    camera->cache->valid = 0;
    return 1;
}

BMX_ADDR(0x00482130)
void zobjCameraInitialize(void)
{
    if (gZobjCameraInitialized != 0) {
        zdebugFail("Already initialized", zobjCameraFile, 169, 0);
    }
    if (gZrwCamInit() == 0) {
        return;
    }
    gZobjCameraCaches = zmemFreeListCreate(sizeof(ZCameraCache), 4, 0);
    gZobjCameraInitialized = 1;
}

BMX_ADDR(0x004821c0)
ZObjCamera *zobjCameraCreate(void)
{
    ZObjCamera *camera = zobjAlloc();
    ZMatrix view;
    ZQuat rot;

    if (camera == 0) {
        return 0;
    }
    zobjInit(camera, 3);
    camera->vtable = &gZobjCameraVtable;
    camera->camera = gZrwCamCreate(0, camera);
    if (camera->camera == 0) {
        zobjFree(camera);
        return 0;
    }
    camera->cache = zmemFreeListAlloc(gZobjCameraCaches);
    memset(camera->cache, 0, sizeof(ZCameraCache));
    gZrwCamSetFov(camera->camera, 0x271c);
    zobjLookAtMatrix(&view, &gZobjCameraHome, &gZobjCameraTarget);
    zquatFromMatrix(&rot, &view);
    zobjSetPosition(camera, &gZobjCameraHome);
    zobjSetRotation(camera, &rot);
    return camera;
}

BMX_ADDR(0x004822a0)
void zobjCameraSetFov(ZObjCamera *camera, int fov)
{
    gZrwCamSetFov(camera->camera, fov);
}

BMX_ADDR(0x004822c0)
void zobjCameraLookAt(ZObjCamera *camera, const ZVec3 *eye, const ZVec3 *target, int unused)
{
    ZMatrix view;
    ZQuat rot;

    zobjLookAtMatrix(&view, eye, target);
    zquatFromMatrix(&rot, &view);
    zobjSetPosition(camera, eye);
    zobjSetRotation(camera, &rot);
}

BMX_ADDR(0x00482310)
ZMatrix *zobjCameraGetMatrix(ZObjCamera *camera, uint32_t kind)
{
    ZCameraCache *cache = camera->cache;
    const uint32_t bit = 1u << (kind & 31);

    if ((cache->valid & bit) == 0) {
        switch (kind) {
        case kZobjCameraMatrixView:
            zquatToMatrix(&cache->matrix[0], &camera->rot);
            zmatrixTransformVector(&cache->matrix[0].pos, &cache->matrix[0], &camera->pos);
            zvec3Negate(&cache->matrix[0].pos, &cache->matrix[0].pos);
            break;
        case kZobjCameraMatrixViewInverse:
            zobjCameraGetMatrix(camera, kZobjCameraMatrixView);
            zmatrixInvertRigid(&cache->matrix[1], &cache->matrix[0]);
            break;
        case kZobjCameraMatrixRotation:
            zobjCameraGetMatrix(camera, kZobjCameraMatrixView);
            zmatrixTranspose(&cache->matrix[2], &cache->matrix[0]);
            zmatrixZeroPos(&cache->matrix[2]);
            break;
        case kZobjCameraMatrixYaw: {
            ZAngle yaw;

            zobjCameraGetMatrix(camera, kZobjCameraMatrixView);
            yaw = zmathAtan2(cache->matrix[0].at.x, cache->matrix[0].at.z);
            zmatrixIdentity(&cache->matrix[3]);
            zmatrixRotateY(&cache->matrix[3], &cache->matrix[3], (ZAngle)-yaw);
            break;
        }
        default:
            zdebugFail("Unknown type", zobjCameraFile, 362, 0);
            return 0;
        }
        cache->valid |= bit;
    }
    return &cache->matrix[kind];
}
