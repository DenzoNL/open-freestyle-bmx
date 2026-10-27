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
