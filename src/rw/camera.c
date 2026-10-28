/* RenderWare 3.1 camera: creation, clip planes and view window, and the calls that reach the device. */
#include "compat/crt.h"
#include "rw/rwcamera.h"
#include "rw/rwmath.h"

typedef int(BMX_CDECL *RwCameraClearFunc)(RwCamera *camera, const void *color, int32_t flags);

static RwFreeList *rwCameraFreeList(void)
{
    return *(RwFreeList **)((uint8_t *)RwEngineInstance + rwCameraOffset);
}

/*
 * Derives zScale and zShift from the clip planes and the device's screen z range: the screen range is
 * shrunk a little at both ends, then mapped linearly onto the planes (the reciprocals of the planes for a
 * perspective camera). EAX = camera.
 */
BMX_ADDR(0x004de480)
__attribute__((regparm(1))) void CameraUpdateZShiftScale(RwCamera *camera)
{
    double farValue;
    double nearValue;
    if (camera->projection == rwPARALLEL) {
        farValue = camera->farPlane;
        nearValue = camera->nearPlane;
    } else {
        farValue = (double)1.0f / (double)camera->farPlane;
        nearValue = (double)1.0f / (double)camera->nearPlane;
    }
    const double nearScreen = RwIm2DGetNearScreenZ();
    const double farScreen = RwIm2DGetFarScreenZ();
    const double top = (nearScreen - farScreen) * (double)0x1.a36e2ep-14f + farScreen;
    const double bottom = (top - nearScreen) * (double)0x1.a36e2ep-14f + nearScreen;
    const double scale = (top - bottom) / (farValue - nearValue);
    camera->zScale = (float)scale;
    camera->zShift = (float)(top - farValue * scale);
}

/*
 * The perspective corners step by twice the window along right and up: x, y, z for right, then for up. The
 * original reads these factors (all 2.0f) from its data; reading them here too keeps the compiler from
 * rewriting a - b * 2 as a + b * -2, which flips the sign of a NaN.
 */
typedef float RwClipCornerSteps[6];
#define rwPerspCornerSteps ORIG_VAR(0x0069bc70, RwClipCornerSteps, rwPerspCornerSteps)

/* x87 helpers: products and negations of floats are exact in a double (as on the x87 at 53 bits). */
static double camMul(float a, float b)
{
    return (double)a * (double)b;
}

/* a - b with b opaque to the optimiser: it may not rewrite the subtraction as a + -b or push the negation
 * into b's product, which flips the sign of a NaN b (the x87 keeps it). */
static double camSub(double a, double b)
{
    __asm__("" : "+x"(b));
    return a - b;
}

static float camNeg(float a)
{
    return (float)-(double)a;
}

/* 1 when the float's sign bit is clear, else 0 (the original shifts the bits right by 31 and adds 1). */
static uint8_t camNotNegative(float f)
{
    uint32_t bits;
    memcpy(&bits, &f, sizeof bits);
    return (uint8_t)((bits >> 31) ^ 1u);
}

static void camSetClosest(RwFrustumPlane *plane)
{
    plane->closest[0] = camNotNegative(plane->normal.x);
    plane->closest[1] = camNotNegative(plane->normal.y);
    plane->closest[2] = camNotNegative(plane->normal.z);
}

/* The negation of a plane through a corner: normal = -source normal, distance = corner . normal. */
static void camOppositePlane(RwFrustumPlane *plane, const RwFrustumPlane *source, const ZVec3 *corner)
{
    plane->normal.y = camNeg(source->normal.y);
    const double y = camMul(corner->y, plane->normal.y);
    const double z = (double)corner->z * -(double)source->normal.z;
    plane->normal.z = camNeg(source->normal.z);
    plane->normal.x = camNeg(source->normal.x);
    plane->distance = (float)((z + y) + camMul(corner->x, plane->normal.x));
}

static int32_t camFloatBits(float f)
{
    int32_t bits;
    memcpy(&bits, &f, sizeof bits);
    return bits;
}

/*
 * Perspective camera: the four near and four far corners of the frustum in world space (the view window
 * spans the frame's right and up vectors at distance 1 along at, shifted by the view offset), then the far
 * and near planes along at and the four side planes from cross products of corner differences, normalised
 * with the table based 1 / sqrt. Values are kept as doubles where the original keeps them on the x87 stack.
 */
BMX_ADDR(0x004dd4d0)
__attribute__((regparm(1))) void CameraBuildPerspClipPlanes(RwCamera *camera)
{
    /* No vectorising: packed code turns a - b into a + -b, which flips the sign of a NaN b. */
#pragma clang fp exceptions(strict)
    const RwFrame *frame = (const RwFrame *)camera->object.object.parent;
    const RwMatrix *ltm = &frame->ltm;
    ZVec3 *corner = camera->frustumCorners;
    /* corner directions: at +- right * window.x +- up * window.y */
    const double rightX = camMul(camera->viewWindow.x, ltm->right.x);
    const double upX = camMul(camera->viewWindow.y, ltm->up.x);
    const double x0 = upX + ((double)ltm->at.x + rightX);
    const double x1 = camSub(x0, rightX * (double)rwPerspCornerSteps[0]);
    const double x2 = camSub(x1, (double)rwPerspCornerSteps[3] * upX);
    const double x3 = rightX * (double)rwPerspCornerSteps[0] + x2;
    const double rightY = camMul(ltm->right.y, camera->viewWindow.x);
    const double rightZ = camMul(ltm->right.z, camera->viewWindow.x);
    const double upY = camMul(ltm->up.y, camera->viewWindow.y);
    const double upZ = camMul(ltm->up.z, camera->viewWindow.y);
    const double offsetUpZ = camMul(ltm->up.z, camera->viewOffset.y);
    const double offsetUpY = camMul(ltm->up.y, camera->viewOffset.y);
    const double offsetUpX = camMul(ltm->up.x, camera->viewOffset.y);
    const double y0 = ((double)ltm->at.y + rightY) + upY;
    const double z0 = ((double)ltm->at.z + rightZ) + upZ;
    corner[3].x = (float)x3;
    corner[2].x = (float)x2;
    const double y1 = camSub(y0, rightY * (double)rwPerspCornerSteps[1]);
    const double y2 = camSub(y1, upY * (double)rwPerspCornerSteps[4]);
    corner[1].y = (float)y1;
    corner[2].y = (float)y2;
    corner[3].y = (float)(rightY * (double)rwPerspCornerSteps[1] + y2);
    corner[0].y = (float)y0;
    corner[0].z = (float)z0;
    const double z1 = camSub(z0, rightZ * (double)rwPerspCornerSteps[2]);
    const double z2 = camSub(z1, upZ * (double)rwPerspCornerSteps[5]);
    corner[1].z = (float)z1;
    corner[1].x = (float)x1;
    corner[0].x = (float)x0;
    corner[2].z = (float)z2;
    corner[3].z = (float)(rightZ * (double)rwPerspCornerSteps[2] + z2);
    /* the eye: position - right * offset.x + up * offset.y */
    const double offsetX = -(double)camera->viewOffset.x;
    const double eyeX = (double)ltm->right.x * offsetX + offsetUpX;
    const double eyeY = (double)ltm->right.y * offsetX + offsetUpY;
    const double eyeZ = offsetX * (double)ltm->right.z + offsetUpZ;
    for (int i = 0; i < 4; i++) {
        ZVec3 *nearCorner = &corner[i];
        ZVec3 *farCorner = &corner[i + 4];
        const double dx = camSub(nearCorner->x, eyeX);
        nearCorner->x = (float)((double)ltm->pos.x + eyeX);
        const double dy = camSub(nearCorner->y, eyeY);
        nearCorner->y = (float)(eyeY + (double)ltm->pos.y);
        const double dz = camSub(nearCorner->z, eyeZ);
        nearCorner->z = (float)((double)ltm->pos.z + eyeZ);
        nearCorner->x = (float)((double)nearCorner->x + (double)camera->nearPlane * dx);
        nearCorner->y = (float)((double)nearCorner->y + (double)camera->nearPlane * dy);
        nearCorner->z = (float)((double)nearCorner->z + (double)camera->nearPlane * dz);
        farCorner->x = (float)(eyeX + (double)ltm->pos.x);
        farCorner->y = (float)(eyeY + (double)ltm->pos.y);
        farCorner->z = (float)((double)ltm->pos.z + eyeZ);
        farCorner->x = (float)((double)farCorner->x + dx * (double)camera->farPlane);
        farCorner->y = (float)((double)farCorner->y + dy * (double)camera->farPlane);
        farCorner->z = (float)((double)farCorner->z + dz * (double)camera->farPlane);
    }
    RwFrustumPlane *plane = camera->frustumPlanes;
    /* far plane: normal = at, through far corner 0; near plane: the opposite, through near corner 0 */
    plane[0].normal = ltm->at;
    const double farZ = camMul(ltm->at.z, corner[4].z);
    const double farY = camMul(ltm->at.y, corner[4].y);
    const double farX = camMul(ltm->at.x, corner[4].x);
    camOppositePlane(&plane[1], &plane[0], &corner[0]);
    plane[0].distance = (float)((farZ + farY) + farX);
    /* left: (c6 - c5) x (c1 - c5) and so on, unnormalised first */
    const double ax = (double)corner[6].x - (double)corner[5].x;
    const double az = (double)corner[1].z - (double)corner[5].z;
    const double bz = (double)corner[6].z - (double)corner[5].z;
    const double bx = (double)corner[1].x - (double)corner[5].x;
    plane[2].normal.y = (float)camSub(ax * az, bz * bx);
    const double yy2 = camMul(plane[2].normal.y, plane[2].normal.y);
    const double ay = (double)corner[1].y - (double)corner[5].y;
    const double by = (double)corner[6].y - (double)corner[5].y;
    plane[2].normal.x = (float)camSub(bz * ay, az * by);
    const double nz2 = camSub(bx * by, ax * ay);
    plane[2].normal.z = (float)nz2;
    const double len2 = (yy2 + nz2 * nz2) + camMul(plane[2].normal.x, plane[2].normal.x);
    const double scale2 = _rwFastInvSqrt(camFloatBits((float)len2));
    /* top: (c1 - c5) x (c4 - c5) */
    const double cz = (double)corner[4].z - (double)corner[5].z;
    const double cx = (double)corner[4].x - (double)corner[5].x;
    const double cy = (double)corner[4].y - (double)corner[5].y;
    plane[3].normal.y = (float)camSub(bx * cz, az * cx);
    const double nz3 = camSub(cx * ay, bx * cy);
    const double yy3 = camMul(plane[3].normal.y, plane[3].normal.y);
    plane[2].normal.x = (float)((double)plane[2].normal.x * scale2);
    plane[3].normal.z = (float)nz3;
    const double sum3 = nz3 * nz3 + yy3;
    const double nx3 = camSub(cy * az, cz * ay);
    plane[2].normal.y = (float)((double)plane[2].normal.y * scale2);
    plane[3].normal.x = (float)nx3;
    const double xx3 = camMul(plane[3].normal.x, plane[3].normal.x);
    const double unitZ2 = scale2 * (double)plane[2].normal.z;
    plane[2].normal.z = (float)unitZ2;
    const double len3 = sum3 + xx3;
    plane[2].distance = (float)(camMul(plane[2].normal.x, corner[1].x) +
                                (unitZ2 * (double)corner[1].z + camMul(corner[1].y, plane[2].normal.y)));
    const double scale3 = _rwFastInvSqrt(camFloatBits((float)len3));
    /* right: (c4 - c7) x (c3 - c7) */
    const double dx = (double)corner[4].x - (double)corner[7].x;
    const double ez = (double)corner[3].z - (double)corner[7].z;
    const double dz = (double)corner[4].z - (double)corner[7].z;
    const double ex = (double)corner[3].x - (double)corner[7].x;
    const double dy = (double)corner[4].y - (double)corner[7].y;
    plane[4].normal.y = (float)camSub(dx * ez, dz * ex);
    const double ey = (double)corner[3].y - (double)corner[7].y;
    const double yy4 = camMul(plane[4].normal.y, plane[4].normal.y);
    const double nz4 = camSub(ex * dy, dx * ey);
    plane[3].normal.x = (float)((double)plane[3].normal.x * scale3);
    plane[4].normal.z = (float)nz4;
    const double sum4 = nz4 * nz4 + yy4;
    const double nx4 = camSub(dz * ey, ez * dy);
    plane[3].normal.y = (float)((double)plane[3].normal.y * scale3);
    plane[4].normal.x = (float)nx4;
    const double xx4 = camMul(plane[4].normal.x, plane[4].normal.x);
    const double unitZ3 = scale3 * (double)plane[3].normal.z;
    const double len4 = sum4 + xx4;
    const double y3 = camMul(plane[3].normal.y, corner[1].y);
    plane[3].normal.z = (float)unitZ3;
    plane[3].distance = (float)(camMul(plane[3].normal.x, corner[1].x) + (unitZ3 * (double)corner[1].z + y3));
    const double scale4 = _rwFastInvSqrt(camFloatBits((float)len4));
    /* bottom: (c3 - c7) x (c6 - c7) */
    const double fz = (double)corner[6].z - (double)corner[7].z;
    const double fx = (double)corner[6].x - (double)corner[7].x;
    const double fy = (double)corner[6].y - (double)corner[7].y;
    plane[5].normal.y = (float)camSub(ex * fz, ez * fx);
    const double nz5 = camSub(fx * ey, ex * fy);
    const double yy5 = camMul(plane[5].normal.y, plane[5].normal.y);
    plane[4].normal.x = (float)((double)plane[4].normal.x * scale4);
    plane[5].normal.z = (float)nz5;
    const double sum5 = nz5 * nz5 + yy5;
    const double nx5 = camSub(fy * ez, fz * ey);
    plane[4].normal.y = (float)((double)plane[4].normal.y * scale4);
    plane[5].normal.x = (float)nx5;
    const double xx5 = camMul(plane[5].normal.x, plane[5].normal.x);
    const double unitZ4 = scale4 * (double)plane[4].normal.z;
    plane[4].normal.z = (float)unitZ4;
    const double len5 = sum5 + xx5;
    plane[4].distance = (float)(camMul(plane[4].normal.x, corner[3].x) +
                                (unitZ4 * (double)corner[3].z + camMul(corner[3].y, plane[4].normal.y)));
    const double scale5 = _rwFastInvSqrt(camFloatBits((float)len5));
    plane[5].normal.y = (float)((double)plane[5].normal.y * scale5);
    const double unitZ5 = (double)plane[5].normal.z * scale5;
    const double y5 = camMul(corner[3].y, plane[5].normal.y);
    plane[5].normal.z = (float)unitZ5;
    plane[5].normal.x = (float)(scale5 * (double)plane[5].normal.x);
    plane[5].distance = (float)(((double)corner[3].z * unitZ5 + y5) + camMul(corner[3].x, plane[5].normal.x));
    for (int i = 0; i < 6; i++) {
        camSetClosest(&plane[i]);
    }
}

/*
 * Parallel camera: the corners are the view window at the near and far distances, transformed by the frame's
 * LTM; the far, right and top planes take the LTM's at, right and up vectors, the near, left and bottom
 * planes their negations.
 */
BMX_ADDR(0x004ddeb0)
__attribute__((regparm(1))) void CameraBuildParallelClipPlanes(RwCamera *camera)
{
    const RwFrame *frame = (const RwFrame *)camera->object.object.parent;
    ZVec3 *corner = camera->frustumCorners;
    const float negY = camNeg(camera->viewWindow.y);
    const float negX = camNeg(camera->viewWindow.x);
    corner[7].y = corner[6].y = corner[3].y = corner[2].y = negY;
    corner[6].x = corner[5].x = corner[2].x = corner[1].x = negX;
    corner[5].z = corner[7].z = corner[6].z = corner[4].z = camera->farPlane;
    corner[1].z = corner[3].z = corner[2].z = corner[0].z = camera->nearPlane;
    corner[1].y = corner[5].y = corner[4].y = corner[0].y = camera->viewWindow.y;
    corner[3].x = corner[7].x = corner[4].x = corner[0].x = camera->viewWindow.x;
    RwV3dTransformPoints(corner, corner, 8, &frame->ltm);
    RwFrustumPlane *plane = camera->frustumPlanes;
    plane[0].normal = frame->ltm.at;
    plane[0].distance =
        (float)(camMul(plane[0].normal.x, corner[4].x) +
                (camMul(corner[4].y, plane[0].normal.y) + camMul(corner[4].z, plane[0].normal.z)));
    plane[4].normal = frame->ltm.right;
    plane[4].distance =
        (float)(camMul(corner[3].x, plane[4].normal.x) +
                (camMul(corner[3].y, plane[4].normal.y) + camMul(corner[3].z, plane[4].normal.z)));
    plane[3].normal = frame->ltm.up;
    plane[3].distance =
        (float)((camMul(corner[1].y, plane[3].normal.y) + camMul(corner[1].z, plane[3].normal.z)) +
                camMul(corner[1].x, plane[3].normal.x));
    camOppositePlane(&plane[1], &plane[0], &corner[0]);
    camOppositePlane(&plane[2], &plane[4], &corner[1]);
    camOppositePlane(&plane[5], &plane[3], &corner[3]);
    for (int i = 0; i < 6; i++) {
        camSetClosest(&plane[i]);
    }
}

/* The far z of the 2D immediate mode as the device reports it (returned on the x87 stack). */
BMX_ADDR(0x004db810)
float RwIm2DGetFarScreenZ(void)
{
    return RwEngineInstance->dOpenDevice.zBufferFar;
}

BMX_ADDR(0x004de540)
RwCamera *RwCameraSetViewOffset(RwCamera *camera, const RwV2d *offset)
{
    camera->viewOffset = *offset;
    if (camera->object.object.parent != NULL) {
        RwFrameUpdateObjects(camera->object.object.parent);
    }
    camera->version++;
    return camera;
}

BMX_ADDR(0x004de580)
RwCamera *RwCameraSetRaster(RwCamera *camera, RwRaster *raster)
{
    camera->frameBuffer = raster;
    camera->version++;
    return camera;
}

BMX_ADDR(0x004de5a0)
RwRaster *RwCameraGetRaster(const RwCamera *camera)
{
    return camera->frameBuffer;
}

BMX_ADDR(0x004de5b0)
RwCamera *RwCameraSetZRaster(RwCamera *camera, RwRaster *raster)
{
    camera->zBuffer = raster;
    camera->version++;
    return camera;
}

BMX_ADDR(0x004de5d0)
RwRaster *RwCameraGetZRaster(const RwCamera *camera)
{
    return camera->zBuffer;
}

BMX_ADDR(0x004de5e0)
RwCamera *RwCameraSetNearClipPlane(RwCamera *camera, float nearClip)
{
    camera->nearPlane = nearClip;
    CameraUpdateZShiftScale(camera);
    camera->version++;
    return camera;
}

BMX_ADDR(0x004de610)
float RwCameraGetNearClipPlane(const RwCamera *camera)
{
    return camera->nearPlane;
}

BMX_ADDR(0x004de620)
RwCamera *RwCameraSetFarClipPlane(RwCamera *camera, float farClip)
{
    camera->farPlane = farClip;
    CameraUpdateZShiftScale(camera);
    camera->version++;
    return camera;
}

BMX_ADDR(0x004de650)
float RwCameraGetFarClipPlane(const RwCamera *camera)
{
    return camera->farPlane;
}

BMX_ADDR(0x004de660)
RwCamera *RwCameraSetFogDistance(RwCamera *camera, float fogDistance)
{
    camera->fogPlane = fogDistance;
    camera->version++;
    return camera;
}

/* The begin / end update callbacks belong to the driver; their result is the call's result. */
BMX_ADDR(0x004de790)
RwCamera *RwCameraBeginUpdate(RwCamera *camera)
{
    return camera->beginUpdate(camera);
}

BMX_ADDR(0x004de7f0)
RwCamera *RwCameraEndUpdate(RwCamera *camera)
{
    return camera->endUpdate(camera);
}

BMX_ADDR(0x004de810)
RwCamera *RwCameraGetCurrentCamera(void)
{
    return RwEngineInstance->curCamera;
}

/* Clears the camera's rasters through the engine's standard function table; NULL if the device fails. */
BMX_ADDR(0x004de8a0)
RwCamera *RwCameraClear(RwCamera *camera, const void *color, int32_t flags)
{
    const RwCameraClearFunc clear =
        (RwCameraClearFunc)(uintptr_t)RwEngineInstance->stdFunc[rwSTANDARDCAMERACLEAR];
    if (clear(camera, color, flags) == 0) {
        return NULL;
    }
    return camera;
}

BMX_ADDR(0x004de920)
RwCamera *RwCameraSetProjection(RwCamera *camera, int32_t projection)
{
    if (projection != rwPERSPECTIVE && projection != rwPARALLEL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_INVALIDSIZE, "Invalid projection type specified");
        RwErrorSet(&error);
        return NULL;
    }
    camera->projection = projection;
    if (camera->object.object.parent != NULL) {
        RwFrameUpdateObjects(camera->object.object.parent);
    }
    CameraUpdateZShiftScale(camera);
    camera->version++;
    return camera;
}

/* Sets the view window and its reciprocal (in double precision, stored as floats). */
BMX_ADDR(0x004de9b0)
RwCamera *RwCameraSetViewWindow(RwCamera *camera, const RwV2d *viewWindow)
{
    camera->viewWindow = *viewWindow;
    const double recipX = (double)1.0f / (double)camera->viewWindow.x;
    const double recipY = (double)1.0f / (double)camera->viewWindow.y;
    camera->recipViewWindow.x = (float)recipX;
    camera->recipViewWindow.y = (float)recipY;
    if (camera->object.object.parent != NULL) {
        RwFrameUpdateObjects(camera->object.object.parent);
    }
    camera->version++;
    return camera;
}

BMX_ADDR(0x004dea10)
const RwV2d *RwCameraGetViewWindow(const RwCamera *camera)
{
    return &camera->viewWindow;
}

BMX_ADDR(0x004dea60)
RwCamera *RwCameraSetFrame(RwCamera *camera, RwFrame *frame)
{
    _rwObjectHasFrameSetFrame(&camera->object, frame);
    return camera;
}

BMX_ADDR(0x004dea80)
RwFrame *RwCameraGetFrame(const RwCamera *camera)
{
    return camera->object.object.parent;
}

/* Frees the camera's plugin data and frame link, then the camera itself. */
BMX_ADDR(0x004dea90)
int RwCameraDestroy(RwCamera *camera)
{
    _rwPluginRegistryDeInitObject(&rwCameraRegistry, camera);
    _rwObjectHasFrameReleaseFrame(&camera->object);
    RwEngineInstance->freeListFree(rwCameraFreeList(), camera);
    return 1;
}

/*
 * A perspective camera with near 0.05, far 10, fog 5 and a 1x1 view window, no frame and no rasters. The
 * sync / begin / end callbacks are the driver routines at 0x4de400, 0x4de690 and 0x4de7b0.
 */
BMX_ADDR(0x004deae0)
RwCamera *RwCameraCreate(void)
{
    RwCamera *camera = RwEngineInstance->freeListAlloc(rwCameraFreeList());
    if (camera == NULL) {
        return NULL;
    }
    camera->viewOffset.y = 0.0f;
    camera->viewOffset.x = 0.0f;
    camera->version = 0;
    camera->zBuffer = NULL;
    camera->frameBuffer = NULL;
    camera->object.object.parent = NULL;
    camera->object.object.privateFlags = 0;
    camera->object.object.flags = 0;
    camera->object.object.subType = 0;
    camera->viewWindow.y = 1.0f;
    camera->viewWindow.x = 1.0f;
    camera->recipViewWindow.y = 1.0f;
    camera->recipViewWindow.x = 1.0f;
    camera->object.object.type = 4;
    camera->object.sync = RW_CB(void *, 0x004de400);
    camera->beginUpdate = RW_CB(RwCameraUpdateFunc, 0x004de690);
    camera->endUpdate = RW_CB(RwCameraUpdateFunc, 0x004de7b0);
    camera->nearPlane = 0x1.99999ap-5f;
    camera->farPlane = 10.0f;
    camera->fogPlane = 5.0f;
    camera->projection = rwPERSPECTIVE;
    CameraUpdateZShiftScale(camera);
    camera->viewMatrix.flags = 0;
    _rwPluginRegistryInitObject(&rwCameraRegistry, camera);
    return camera;
}

/* ---- the PVS camera ---------------------------------------------------------------------------------- */

/* Rasters, images and the world, still original code (rw/texture.c, rw/world.c). */
ORIG_FN(0x004dcb10, RwRaster *, RwRasterCreate,
        (int32_t width, int32_t height, int32_t depth, int32_t flags));
ORIG_FN(0x004dcc80, RwRaster *, RwRasterSubRaster,
        (RwRaster * raster, RwRaster *parent, const int32_t *rect));
ORIG_FN(0x004e38e0, void *, RwImageCreate, (int32_t width, int32_t height, int32_t depth));
ORIG_FN(0x004e3ab0, void *, RwImageAllocatePixels, (void *image));
ORIG_FN(0x004b81f0, void *, RpWorldAddCamera, (void *world, RwCamera *camera));

/* The camera the PVS builder renders the world with, and its 128 x 128 x 32 image (0x676814, 0x676818). */
#define rwPvsCamera ORIG_VAR(0x00676814, RwCamera *, rwPvsCamera)
#define rwPvsImage  ORIG_VAR(0x00676818, void *, rwPvsImage)

/*
 * Creates the PVS camera: a frame, a 128 x 128 camera raster and z raster (sub rasters of the given ones when
 * there are any), near plane 0.25, the given far plane, a 1 x 1 view window, added to the world; and the
 * image it is read back into.
 */
__attribute__((used, noinline)) static void rwPvsCameraCreateImpl(void *world, RwRaster *raster,
                                                                  RwRaster *zRaster, double farClip)
{
    const int32_t rect[4] = {0, 0, 128, 128};
    rwPvsCamera = RwCameraCreate();
    RwCameraSetFrame(rwPvsCamera, RwFrameCreate());
    RwRaster *created;
    if (raster == NULL) {
        created = RwRasterCreate(128, 128, 0, 2);
    } else {
        created = RwRasterCreate(0, 0, 0, 2);
        RwRasterSubRaster(created, raster, rect);
    }
    RwCameraSetRaster(rwPvsCamera, created);
    if (zRaster == NULL) {
        created = RwRasterCreate(128, 128, 0, 1);
    } else {
        created = RwRasterCreate(0, 0, 0, 1);
        RwRasterSubRaster(created, zRaster, rect);
    }
    RwCameraSetZRaster(rwPvsCamera, created);
    RwCameraSetFarClipPlane(rwPvsCamera, (float)farClip);
    RwCameraSetNearClipPlane(rwPvsCamera, 0.25f);
    const RwV2d window = {1.0f, 1.0f};
    RwCameraSetViewWindow(rwPvsCamera, &window);
    RpWorldAddCamera(world, rwPvsCamera);
    rwPvsImage = RwImageCreate(128, 128, 32);
    RwImageAllocatePixels(rwPvsImage);
}

/*
 * The original's entry (labelled RwCameraClone): EAX = world, EDX = parent camera raster, ECX = parent z
 * raster, ST0 = far clip plane (popped).
 */
BMX_ADDR(0x004ce6d0)
__attribute__((naked)) void rwPvsCameraCreate(void)
{
    __asm__ volatile("subl $8, %esp\n\t"
                     "fstpl (%esp)\n\t"
                     "pushl %ecx\n\t"
                     "pushl %edx\n\t"
                     "pushl %eax\n\t"
                     "call _rwPvsCameraCreateImpl\n\t"
                     "addl $20, %esp\n\t"
                     "ret");
}

#define rwCameraModuleCount ORIG_VAR(0x00676d14, int32_t, rwCameraModuleCount)

/* Camera module: the free list of cameras (the registry's size, entries 4 aligned). NULL when it fails. */
BMX_ADDR(0x004dd430)
void *_rwCameraOpen(void *engine, int32_t offset)
{
    rwCameraOffset = offset;
    *(RwFreeList **)((uint8_t *)RwEngineInstance + offset) =
        RwFreeListCreate(rwCameraRegistry.sizeOfStruct, 4, 4);
    if (rwCameraFreeList() == NULL) {
        return NULL;
    }
    rwCameraModuleCount++;
    return engine;
}
