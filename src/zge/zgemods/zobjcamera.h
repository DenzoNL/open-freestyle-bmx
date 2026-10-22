/*
 * Camera objects (C:\NgEngine\Zge\ZgeMods\zobjCamera.c) and, by address, the engine's vector and matrix
 * helpers 0x481120-0x481f00 (data/functions.csv files them under zobjcamera.c; the source file is
 * unknown). Matrices are row-major RwMatrix (ZMatrix), vectors are rows.
 */
#ifndef ZGE_ZGEMODS_ZOBJCAMERA_H
#define ZGE_ZGEMODS_ZOBJCAMERA_H

#include "compat/orig.h"
#include "zge/zmath.h"
#include "zge/zmem.h"

/* A zero vector; passed as the middle point of zvec3Cross3 to get a plain cross product. */
#define gZmathOrigin ORIG_VAR(0x006702d8, ZVec3, gZmathOrigin)

/* Older name of zmathSin (same address) that takes the angle as an unsigned word. */
#define zmathSinTable(angle) zmathSin((ZAngle)(angle))

void zvec3Zero(ZVec3 *v);
void zvec3Add(ZVec3 *out, const ZVec3 *a, const ZVec3 *b);
/* out = a - b. */
void zvec3Sub(ZVec3 *out, const ZVec3 *a, const ZVec3 *b);
void zvec3Scale(ZVec3 *out, const ZVec3 *in, float s);
/* out = (sx * in.x, sy * in.y, sz * in.z). */
void zvec3ScaleXYZ(ZVec3 *out, const ZVec3 *in, float sx, float sy, float sz);
/* x*x + y*y + z*z, unrounded. */
double zvec3LengthSq(const ZVec3 *v);
void zvec3Negate(ZVec3 *out, const ZVec3 *in);
/* out = in rotated about the X / Y / Z axis. Only the low 16 bits of the angle are read. */
void zvec3RotateX(ZVec3 *out, const ZVec3 *in, ZAngle angle);
void zvec3RotateY(ZVec3 *out, const ZVec3 *in, ZAngle angle);
void zvec3RotateZ(ZVec3 *out, const ZVec3 *in, ZAngle angle);
/*
 * Rotates a vector about x, then y, then z (every caller passes the same vector as `out` and `in`).
 * Original quirk: x is read from `out`, not `in`.
 */
void zvec3RotateEuler(ZVec3 *out, const ZVec3 *in, const ZAngle3 *angles);
double zvec3Dot(const ZVec3 *a, const ZVec3 *b);
/* out = a x b. */
void zvec3Cross(ZVec3 *out, const ZVec3 *a, const ZVec3 *b);
/* out = (b - a) x (c - b). */
void zvec3Cross3(ZVec3 *out, const ZVec3 *a, const ZVec3 *b, const ZVec3 *c);
/* out = a + (b - a) * t. */
void zvec3Lerp(ZVec3 *out, const ZVec3 *a, const ZVec3 *b, float t);
/* out = in + s / in - s on every component. */
void zvec3AddScalar(ZVec3 *out, const ZVec3 *in, float s);
void zvec3SubScalar(ZVec3 *out, const ZVec3 *in, float s);

/* out = v * m, including the translation row. */
void zmatrixTransformPoint(ZVec3 *out, const ZMatrix *m, const ZVec3 *v);
/* out = v * m without the translation row. */
void zmatrixTransformVector(ZVec3 *out, const ZMatrix *m, const ZVec3 *v);
/* Sets the identity; leaves the flags and padding words alone. */
void zmatrixIdentity(ZMatrix *m);
/* Clears the position row. */
void zmatrixZeroPos(ZMatrix *m);
/* out = in with its position moved by offset; returns out. */
ZMatrix *zmatrixTranslate(ZMatrix *out, const ZMatrix *in, const ZVec3 *offset);
/*
 * Matrix rotations about X / Y / Z; only the low 16 bits of the angle are read. They copy `in` to `out`
 * first when the two differ. The original returns `out` from X but `in` from Y and Z.
 */
ZMatrix *zmatrixRotateX(ZMatrix *out, const ZMatrix *in, ZAngle angle);
const ZMatrix *zmatrixRotateY(ZMatrix *out, const ZMatrix *in, ZAngle angle);
const ZMatrix *zmatrixRotateZ(ZMatrix *out, const ZMatrix *in, ZAngle angle);
ZMatrix *zmatrixRotateEuler(ZMatrix *out, const ZMatrix *in, const ZAngle3 *angles);
/* Copies `in` and multiplies the diagonal by scale; returns out. */
ZMatrix *zmatrixScaleDiagonal(ZMatrix *out, const ZMatrix *in, const ZVec3 *scale);
/* Copies the position and the 3x3 basis (not the flag and padding words); returns out. */
ZMatrix *zmatrixCopyRotation(ZMatrix *out, const ZMatrix *in);
/* Copies the 3x3 basis only. */
void zmatrixCopyBasis(ZMatrix *out, const ZMatrix *in);
/* out = a * b. Stores one element at a time, so an `out` that aliases b changes later reads. */
void zmatrixMul(ZMatrix *out, const ZMatrix *a, const ZMatrix *b);
/* Transposes the 3x3 rotation part. */
void zmatrixTranspose(ZMatrix *out, const ZMatrix *in);
/* Inverse of a rigid transform: transposed basis, position = -pos * basis^T; returns out. */
ZMatrix *zmatrixInvertRigid(ZMatrix *out, const ZMatrix *in);

/* Cached derived matrices of a camera object; `valid` has bit n set once matrix n has been computed. */
typedef struct ZCameraCache {
    ZMatrix matrix[4]; /* 0 view, 1 its inverse, 2 rotation only (transposed), 3 yaw undo */
    uint32_t valid;    /* 0x100 */
} ZCameraCache;

_Static_assert(sizeof(ZCameraCache) == 0x104, "camera cache layout");

enum {
    kZobjCameraMatrixView = 0,
    kZobjCameraMatrixViewInverse = 1,
    kZobjCameraMatrixRotation = 2,
    kZobjCameraMatrixYaw = 3,
};

struct ZObjCamera;
typedef int(BMX_CDECL *ZObjCameraHook)(struct ZObjCamera *camera);

/* Type hooks of a camera object (the table at 0x560518 in the exe's data). */
typedef struct ZObjCameraVtable {
    ZObjCameraHook destroy; /* 0x482090 */
    ZObjCameraHook draw;    /* 0x4820c0 */
    ZObjCameraHook moved;   /* 0x4820f0 */
    ZObjCameraHook rotated; /* 0x482110 */
} ZObjCameraVtable;

/* A scene object of type 3. Position and rotation are set through zobjSetPosition / zobjSetRotation. */
typedef struct ZObjCamera {
    uint32_t flags;                 /* 0x00 */
    ZVec3 pos;                      /* 0x04 */
    uint8_t pad10[4];               /* 0x10 */
    ZQuat rot;                      /* 0x14 */
    uint32_t field24;               /* 0x24 */
    const ZObjCameraVtable *vtable; /* 0x28 */
    void *camera;                   /* 0x2c renderer camera */
    ZCameraCache *cache;            /* 0x30 */
    uint8_t pad34[0x3c - 0x34];
} ZObjCamera;

_Static_assert(sizeof(ZObjCamera) == 0x3c, "object layout");

#define gZobjCameraInitialized ORIG_VAR(0x006702f8, int, gZobjCameraInitialized)
#define gZobjCameraCaches      ORIG_VAR(0x006702fc, ZMemFreeList *, gZobjCameraCaches)
/* Look-at target of a new camera (0x670300) and its default position (0x560540, all zero). */
#define gZobjCameraTarget ORIG_VAR(0x00670300, ZVec3, gZobjCameraTarget)
#define gZobjCameraHome   ORIG_VAR(0x00560540, ZVec3, gZobjCameraHome)
#define gZobjCameraVtable ORIG_VAR(0x00560518, ZObjCameraVtable, gZobjCameraVtable)

int zobjCameraDestroy(ZObjCamera *camera);
int zobjCameraDraw(ZObjCamera *camera);
/* The moved / rotated hooks drop all cached matrices. */
int zobjCameraMoved(ZObjCamera *camera);
int zobjCameraRotated(ZObjCamera *camera);
/* Returns 0 when the renderer's camera system fails to start, else 1. */
int zobjCameraInitialize(void);
ZObjCamera *zobjCameraCreate(void);
/* Sets the field of view (a ZAngle in the low 16 bits; 0 is ignored by the renderer). */
void zobjCameraSetFov(ZObjCamera *camera, int fov);
/* Points the camera from eye at target. The original takes a fourth, unused argument. */
void zobjCameraLookAt(ZObjCamera *camera, const ZVec3 *eye, const ZVec3 *target, int unused);
/* Cached matrix of the given kind (kZobjCameraMatrix*); NULL (after an assert) for unknown kinds. */
ZMatrix *zobjCameraGetMatrix(ZObjCamera *camera, uint32_t kind);

#endif
