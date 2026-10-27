/* RenderWare 3.1 camera: creation, clip planes and view window, and the calls that reach the device. */
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
