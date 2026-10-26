/* Renderer cameras: markers and the camera rasters (see zrwcam.h). */
#include "zge/com/zglrw/zrwcam.h"

#include "compat/crt.h"
#include "rw/rwcore.h"
#include "rw/rwplugins.h"
#include "zge/com/zglrw/zrwmodelcreate.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"

static const char kZrwCamFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwCam.c";

ORIG_FN(0x004dcb10, void *, RwRasterCreate, (int width, int height, int depth, int flags));
ORIG_FN(0x004dcbd0, int, RwRasterDestroy, (void *raster));

#define gZrwCamReady  ORIG_VAR(0x00674018, int, gZrwCamReady)
#define gZrwCamPool   ORIG_VAR(0x0067401c, ZMemFreeList *, gZrwCamPool)
#define gZrwCamRaster ORIG_VAR(0x006976c0, void *, gZrwCamRaster)
#define gZrwZRaster   ORIG_VAR(0x006976c4, void *, gZrwZRaster)
#define gZrwCamWidth  ORIG_VAR(0x00697770, int, gZrwCamWidth)
#define gZrwCamHeight ORIG_VAR(0x00697774, int, gZrwCamHeight)

BMX_ADDR(0x00492130)
ZrwMarker *zrwcamFindMarker(int hash, int index)
{
    ZrwMarker *marker = gZrwMarkers;

    while (marker != NULL) {
        if (marker->hash == hash && marker->index == index) {
            return marker;
        }
        marker = marker->next;
    }
    return NULL;
}

/* True when the pool holds a camera (the setters then refuse to change the raster size). */
static int camerasExist(void)
{
    ZMemFreeNode *node = zmemFreeListFirst(gZrwCamPool);

    return node != NULL && zmemFreeNodePayload(node) != NULL;
}

/* Drops the z-buffer and (when it existed) the camera raster. */
static void destroyRasters(void)
{
    if (gZrwZRaster != NULL) {
        RwRasterDestroy(gZrwZRaster);
        gZrwZRaster = NULL;
        if (gZrwCamRaster != NULL) {
            RwRasterDestroy(gZrwCamRaster);
            gZrwCamRaster = NULL;
        }
    }
}

BMX_ADDR(0x00492340)
int zrwCameraInitialize(void)
{
    int width;
    int height;

    if (gZrwCamReady != 0) {
        zdebugFail("Already initialized", kZrwCamFile, 0x10a, 0);
    }
    gZrwCamPool = zmemFreeListCreate(200, 2, BMX_FNPTR(ZMemDestroyFn, 0x00492970, zrwCameraDestroy));
    height = gZrwCamHeight;
    width = gZrwCamWidth;
    if (camerasExist()) {
        zdebugFail("Can't set screen resoltion if cameras exist", kZrwCamFile, 0x149, 0);
        return 0;
    }
    destroyRasters();
    gZrwCamWidth = width;
    gZrwCamHeight = height;
    gZrwCamRaster = RwRasterCreate(width, height, 0, 2);
    if (gZrwCamRaster == NULL) {
        zdebugFail("Error creating camera raster", kZrwCamFile, 0xf2, 0);
    } else {
        gZrwZRaster = RwRasterCreate(gZrwCamWidth, gZrwCamHeight, 0, 1);
        if (gZrwZRaster == NULL) {
            zdebugFail("Error creating camera z buffer", kZrwCamFile, 0xf8, 0);
        }
    }
    gZrwCamReady = 1;
    return zrollerShadowStart() != 0;
}

BMX_ADDR(0x00492480)
int zrwCameraShutdown(void)
{
    if (gZrwCamReady != 0) {
        if (zrollerShadowStop() == 0) {
            return 0;
        }
        if (gZrwZRaster != NULL) {
            RwRasterDestroy(gZrwZRaster);
            gZrwZRaster = NULL;
        }
        if (gZrwCamRaster != NULL) {
            RwRasterDestroy(gZrwCamRaster);
            gZrwCamRaster = NULL;
        }
        zmemFreeListDestroy(gZrwCamPool);
        gZrwCamReady = 0;
    }
    return 1;
}

BMX_ADDR(0x004924f0)
int zrwCameraSetResolution(int width, int height)
{
    if (camerasExist()) {
        zdebugFail("Can't set screen resoltion if cameras exist", kZrwCamFile, 0x149, 0);
        return 0;
    }
    destroyRasters();
    gZrwCamWidth = width;
    gZrwCamHeight = height;
    gZrwCamRaster = RwRasterCreate(width, height, 0, 2);
    if (gZrwCamRaster == NULL) {
        zdebugFail("Error creating camera raster", kZrwCamFile, 0xf2, 0);
        return 1;
    }
    gZrwZRaster = RwRasterCreate(gZrwCamWidth, gZrwCamHeight, 0, 1);
    if (gZrwZRaster == NULL) {
        zdebugFail("Error creating camera z buffer", kZrwCamFile, 0xf8, 0);
    }
    return 1;
}

/* ---- cameras ------------------------------------------------------------------------------------ */

/* RenderWare library calls (statically linked in the exe). */
ORIG_FN(0x004deae0, void *, RwCameraCreate, (void));
ORIG_FN(0x004dea90, void, RwCameraDestroy, (void *camera));
ORIG_FN(0x004d2870, void *, RwFrameCreate, (void));
ORIG_FN(0x004d2920, void, RwFrameDestroy, (void *frame));
ORIG_FN(0x004dea60, void, RwCameraSetFrame, (void *camera, void *frame));
ORIG_FN(0x004dea80, void *, RwCameraGetFrame, (void *camera));
ORIG_FN(0x004d3200, void, RwFrameSetIdentity, (void *frame));
ORIG_FN(0x004d2b00, float *, RwFrameGetMatrix, (void *frame));
ORIG_FN(0x004de920, void, RwCameraSetProjection, (void *camera, int projection));
ORIG_FN(0x004dcc80, void, RwRasterSubRaster, (void *raster, void *parent, const int32_t *rect));
ORIG_FN(0x004dcd70, void *, RwRasterGetParent, (void *raster));
ORIG_FN(0x004de580, void, RwCameraSetRaster, (void *camera, void *raster));
ORIG_FN(0x004de5b0, void, RwCameraSetZRaster, (void *camera, void *raster));
ORIG_FN(0x004de5a0, int16_t *, RwCameraGetRaster, (void *camera));
ORIG_FN(0x004de5d0, void *, RwCameraGetZRaster, (void *camera));
ORIG_FN(0x004de9b0, void, RwCameraSetViewWindow, (void *camera, const float *window));
ORIG_FN(0x004dea10, const float *, RwCameraGetViewWindow, (void *camera));
ORIG_FN(0x004de540, void, RwCameraSetViewOffset, (void *camera, const float *offset));
ORIG_FN(0x004de5e0, void, RwCameraSetNearClipPlane, (void *camera, float distance));
ORIG_FN(0x004de620, void, RwCameraSetFarClipPlane, (void *camera, float distance));
ORIG_FN(0x004de660, void, RwCameraSetFogDistance, (void *camera, float distance));
ORIG_FN(0x004de790, int, RwCameraBeginUpdate, (void *camera));
ORIG_FN(0x004de7f0, void, RwCameraEndUpdate, (void *camera));
ORIG_FN(0x004dcd80, int, RwRasterPushContext, (void *raster));
ORIG_FN(0x004dce20, void, RwRasterPopContext, (void));
/* Scene hooks around a camera's update (the engine's lighting and sorting state). */
ORIG_FN(0x004b81f0, void, rwSceneBegin, (void *state, void *camera));
ORIG_FN(0x004b82a0, void *, rwSceneFind, (void *camera));
/* Two arguments: the original reuses its second push of the camera (rwSceneFind takes one). */
ORIG_FN(0x004b8240, void, rwSceneRelease, (void *scene, void *camera));
void zrwResetRenderState(void);

#define gZrwCurrentCamera ORIG_VAR(0x00674020, ZrwCamera *, gZrwCurrentCamera)
#define gZrwSceneState    ORIG_VAR(0x006976c8, void *, gZrwSceneState)
#define gZrwActiveRw      ORIG_VAR(0x006976f4, void *, gZrwActiveRw)
#define gZrwInUpdate      ORIG_VAR(0x006976f8, int, gZrwInUpdate)
/* Offset and scale that bring world positions into the renderer's range. */
#define gZrwWorldOffset       ORIG_VAR(0x006976d8, ZVec3, gZrwWorldOffset)
#define gZrwWorldScale        ORIG_VAR(0x00697720, float, gZrwWorldScale)
#define gZrwCamViewportWord   ORIG_VAR(0x0069772c, uint32_t, gZrwCamViewportWord)
#define gZrwCamViewWindowWord ORIG_VAR(0x00697730, uint32_t, gZrwCamViewWindowWord)

/* Sets the view window from a field of view: half the tangent, and 3/4 of it vertically. */
static void applyFov(void *rw, ZAngle fov)
{
    const double half = zmathTan(fov) * 0.5f;
    float window[2];

    window[0] = (float)half;
    window[1] = (float)(half * 0.75f);
    RwCameraSetViewWindow(rw, window);
}

/* Frees the RwCamera with its frame and rasters and the record. */
static void releaseCamera(ZrwCamera *camera)
{
    int16_t *raster;
    void *zraster;

    if (RwCameraGetFrame(camera->rwCamera) != NULL) {
        RwFrameDestroy(RwCameraGetFrame(camera->rwCamera));
    }
    raster = RwCameraGetRaster(camera->rwCamera);
    if (raster != NULL) {
        RwRasterDestroy(raster);
    }
    zraster = RwCameraGetZRaster(camera->rwCamera);
    if (zraster != NULL) {
        RwRasterDestroy(zraster);
    }
    RwCameraDestroy(camera->rwCamera);
    zmemFreeListFree(gZrwCamPool, camera);
}

BMX_ADDR(0x004925f0)
ZrwCamera *zrwCameraCreate(uint32_t flags, void *owner)
{
    ZrwCamera *camera = zmemFreeListAlloc(gZrwCamPool);
    void *rw;

    memset(camera, 0, 200);
    camera->owner = owner;
    camera->flags = (camera->flags & 0xffffff01u) | 1;
    rw = RwCameraCreate();
    if (rw == NULL) {
        zmemFreeListFree(gZrwCamPool, camera);
        return NULL;
    }
    camera->rwCamera = rw;
    RwCameraSetFrame(rw, RwFrameCreate());
    void *frame = RwCameraGetFrame(rw);

    if (frame == NULL) {
        if (camera->rwCamera == NULL) {
            return NULL;
        }
        releaseCamera(camera);
        return NULL;
    }
    RwFrameSetIdentity(frame);
    if ((flags & 1) != 0) {
        RwCameraSetProjection(rw, 2);
    }
    if ((flags & 2) != 0) {
        return camera;
    }
    {
        /* The rasters are sub rasters of the global ones, sized to the screen (the size words as unsigned).
         */
        const float rectF[4] = {0.0f, 0.0f, (float)(uint32_t)gZrwCamWidth, (float)(uint32_t)gZrwCamHeight};
        const int32_t rect[4] = {0, 0, bmxFtol(rectF[2]), bmxFtol(rectF[3])};
        void *camRaster = gZrwCamRaster;
        void *zRaster;
        void *colour;
        void *depth;
        int16_t *check;

        camera->state |= 4;
        colour = RwRasterCreate(0, 0, 0, 2);
        RwRasterSubRaster(colour, camRaster, rect);
        zRaster = gZrwZRaster;
        depth = RwRasterCreate(0, 0, 0, 1);
        RwRasterSubRaster(depth, zRaster, rect);
        RwCameraSetRaster(camera->rwCamera, colour);
        RwCameraSetZRaster(camera->rwCamera, depth);
        check = RwCameraGetRaster(rw);
        if (check != NULL && RwRasterGetParent(RwCameraGetRaster(rw)) != NULL &&
            RwCameraGetZRaster(rw) != NULL && RwRasterGetParent(RwCameraGetZRaster(rw)) != NULL) {
            void *rw2;

            zrwCameraSetRect(camera, rectF);
            rw2 = camera->rwCamera;
            camera->fov = 0x271c;
            applyFov(rw2, 0x271c);
            camera->clipOffset = 1.0f;
            RwCameraSetFogDistance(camera->rwCamera, (float)((double)camera->farClip - 1.0f));
            rw2 = camera->rwCamera;
            camera->nearClip = 0.5f;
            RwCameraSetNearClipPlane(rw2, 0.5f);
            camera->farClip = 75.0f;
            RwCameraSetFarClipPlane(rw2, 75.0f);
            RwCameraSetFogDistance(rw2, (float)(75.0f - (double)camera->clipOffset));
            return camera;
        }
        if (camera->rwCamera == NULL) {
            return NULL;
        }
        releaseCamera(camera);
        return NULL;
    }
}

BMX_ADDR(0x00492970)
int zrwCameraDestroy(ZrwCamera *camera)
{
    if (camera->rwCamera != NULL) {
        releaseCamera(camera);
    }
    return 1;
}

BMX_ADDR(0x004929f0)
void zrwCameraSetClip(ZrwCamera *camera, float nearClip, float farClip)
{
    void *rw = camera->rwCamera;

    if (nearClip < 0.0f || nearClip > 0.0f) {
        camera->nearClip = nearClip;
        RwCameraSetNearClipPlane(rw, nearClip);
    }
    if (farClip < 0.0f || farClip > 0.0f) {
        camera->farClip = farClip;
        RwCameraSetFarClipPlane(rw, farClip);
        RwCameraSetFogDistance(rw, (float)((double)farClip - camera->clipOffset));
    }
}

BMX_ADDR(0x00492a60)
void zrwCameraSetFov(ZrwCamera *camera, int fov)
{
    void *rw = camera->rwCamera;

    if ((int16_t)fov != 0) {
        camera->fov = (uint16_t)fov;
        applyFov(rw, (ZAngle)fov);
    }
}

BMX_ADDR(0x00492ab0)
uint16_t zrwCameraGetFov(const ZrwCamera *camera)
{
    return camera->fov;
}

BMX_ADDR(0x00492ac0)
void zrwCameraSetView(ZrwCamera *camera, const ZMatrix *view, const ZVec3 *eye)
{
    void *rw = camera->rwCamera;
    const float *v = (const float *)view;
    void *frame;
    float *m;
    float y, z;

    zmatrixCopyRotation(&camera->view, view);
    camera->view.pos = *eye;
    frame = RwCameraGetFrame(rw);
    m = RwFrameGetMatrix(frame);
    RwFrameSetIdentity(frame);
    m[0] = -v[0];
    m[1] = -v[4];
    m[2] = v[8];
    m[4] = -v[1];
    m[5] = v[5];
    m[6] = v[9];
    m[8] = v[2];
    m[9] = -v[6];
    m[10] = -v[10];
    y = (float)((double)gZrwWorldOffset.y + -eye->y);
    z = (float)((double)gZrwWorldOffset.z + -eye->z);
    m[13] = (float)((double)gZrwWorldScale * y);
    m[12] = (float)(((double)eye->x + gZrwWorldOffset.x) * gZrwWorldScale);
    m[14] = (float)((double)gZrwWorldScale * z);
    RwMatrixUpdate((RwMatrix *)m);
    memcpy(&camera->world, m, sizeof camera->world);
}

BMX_ADDR(0x00492c10)
ZMatrix *zrwCameraGetViewMatrix(ZrwCamera *camera)
{
    return &camera->view;
}

BMX_ADDR(0x00492c20)
int zrwCameraProject(const ZrwCamera *camera, const ZVec3 *point, float *out)
{
    if (point->z > camera->nearClip && point->z < camera->farClip) {
        const float *window = RwCameraGetViewWindow(camera->rwCamera);
        const double inv = 1.0f / (double)point->z;

        out[0] = (float)((double)camera->width * inv * point->x);
        out[1] = (float)((double)camera->height * inv * point->y);
        out[0] = (float)((double)camera->offsetX + out[0]);
        out[1] = (float)((double)camera->offsetY + out[1]);
        out[0] = (float)((double)window[0] * out[0]);
        out[1] = (float)((double)window[1] * out[1]);
        return 1;
    }
    return 0;
}

BMX_ADDR(0x00492cc0)
void zrwCameraSetRect(ZrwCamera *camera, const float *rect)
{
    void *rw = camera->rwCamera;
    int32_t ints[4];
    int16_t *raster;
    void *zraster;

    camera->width = bmxFtol(rect[2]);
    camera->height = bmxFtol(rect[3]);
    ints[0] = bmxFtol(rect[0]);
    ints[1] = bmxFtol(rect[1]);
    ints[2] = bmxFtol(rect[2]);
    ints[3] = bmxFtol(rect[3]);
    raster = RwCameraGetRaster(rw);
    if (raster != NULL) {
        RwRasterSubRaster(raster, RwRasterGetParent(raster), ints);
    }
    zraster = RwCameraGetZRaster(rw);
    if (zraster != NULL) {
        RwRasterSubRaster(zraster, RwRasterGetParent(zraster), ints);
    }
    if (camera->fov != 0) {
        applyFov(camera->rwCamera, (ZAngle)camera->fov);
    }
    raster = RwCameraGetRaster(rw);
    camera->offsetX = (float)raster[14] /* +0x1c */;
    camera->offsetY = (float)raster[15] /* +0x1e */;
}

BMX_ADDR(0x00492de0)
void zrwCameraSetViewport(ZrwCamera *camera, uint8_t relative, const float *rect)
{
    if (rect == NULL) {
        camera->viewport[0] = 0.0f;
        camera->viewport[1] = 0.0f;
        camera->viewport[2] = (float)camera->width;
        camera->state &= ~1u;
        camera->viewport[3] = (float)camera->height;
        return;
    }
    camera->state |= 1;
    if ((relative & 1) != 0) {
        const float h = (float)camera->height;

        camera->viewport[0] = (float)((double)camera->width * rect[0]);
        camera->viewport[1] = (float)((double)camera->height * rect[1]);
        camera->viewport[2] = (float)((double)camera->width * rect[2] + camera->viewport[0]);
        camera->viewport[3] = (float)((double)h * rect[3] + camera->viewport[1]);
        return;
    }
    camera->viewport[0] = rect[0];
    camera->viewport[1] = rect[1];
    camera->viewport[2] = (float)((double)rect[2] + camera->viewport[0]);
    camera->viewport[3] = (float)((double)rect[3] + camera->viewport[1]);
}

BMX_ADDR(0x00492eb0)
ZrwCamera *zrwCameraFirst(void)
{
    ZMemFreeNode *node = zmemFreeListFirst(gZrwCamPool);

    if (node == NULL) {
        return NULL;
    }
    return zmemFreeNodePayload(node);
}

BMX_ADDR(0x00492ed0)
ZrwCamera *zrwCameraNext(ZrwCamera *camera)
{
    ZMemFreeNode *node = zmemFreeNodeNext(zmemFreeNodeFromPayload(camera));

    if (node == NULL) {
        return NULL;
    }
    return zmemFreeNodePayload(node);
}

BMX_ADDR(0x00492f00)
int zrwCameraBegin(ZrwCamera *camera)
{
    void *rw = camera->rwCamera;

    gZrwCurrentCamera = camera;
    rwSceneBegin(gZrwSceneState, rw);
    RpLODAtomicSetCamera(gZrwActiveRw);
    gZrwActiveRw = camera->rwCamera;
    if (RwCameraBeginUpdate(gZrwActiveRw) != 0 && RwRasterPushContext(RwCameraGetRaster(rw)) != 0) {
        zrwResetRenderState();
        if ((camera->state & 2) == 0) {
            RwRenderStateSet(0xc, 0);
            gZrwInUpdate = 1;
            return 1;
        }
        RwRenderStateSet(0xc, 1);
        RwRenderStateSet(0xd, (int)(((uint32_t)(uint8_t)(camera->clearColor >> 24) << 24) |
                                    ((uint32_t)(uint8_t)camera->clearColor << 16) |
                                    ((uint32_t)(uint8_t)(camera->clearColor >> 8) << 8) |
                                    (uint8_t)(camera->clearColor >> 16)));
        RwRenderStateSet(0xe, 1);
        gZrwInUpdate = 1;
        return 1;
    }
    return 0;
}

BMX_ADDR(0x00492fe0)
int zrwCameraEnd(void)
{
    if (gZrwActiveRw == NULL) {
        gZdebugPrint("RwCam: No camera\n");
        return 0;
    }
    RwRenderStateSet(0xc, 0);
    gZrwInUpdate = 0;
    RwRasterPopContext();
    RwCameraEndUpdate(gZrwActiveRw);
    rwSceneRelease(rwSceneFind(gZrwActiveRw), gZrwActiveRw);
    gZrwActiveRw = NULL;
    gZrwCurrentCamera = NULL;
    return 1;
}

BMX_ADDR(0x00493060)
void *zrwCameraGetRw(const ZrwCamera *camera)
{
    return camera->rwCamera;
}

BMX_ADDR(0x00493070)
ZrwCamera *zrwCameraGetCurrent(void)
{
    return gZrwCurrentCamera;
}

BMX_ADDR(0x00493080)
ZMatrix *zrwCameraGetWorldMatrix(ZrwCamera *camera)
{
    return &camera->world;
}

BMX_ADDR(0x00493090)
void *zrwCameraGetOwner(const ZrwCamera *camera)
{
    return camera->owner;
}

BMX_ADDR(0x004930a0)
void zrwCameraSetViewportWord(ZrwCamera *camera, uint32_t value)
{
    gZrwCamViewportWord = value;
}

BMX_ADDR(0x004930b0)
void zrwCameraSetViewWindowWord(ZrwCamera *camera, const uint32_t *value)
{
    gZrwCamViewWindowWord = *value;
}

BMX_ADDR(0x004930c0)
void zrwCameraSetClearFlag(ZrwCamera *camera, uint32_t flag)
{
    camera->state = ((flag & 1) << 1) | (camera->state & 0xfffffffdu);
}

BMX_ADDR(0x004930e0)
void zrwCameraSetClearColor(ZrwCamera *camera, const uint32_t *color)
{
    camera->clearColor = *color;
}

BMX_ADDR(0x00493100)
void zrwCameraSetClipOffset(ZrwCamera *camera, float offset)
{
    camera->clipOffset = offset;
    RwCameraSetFogDistance(camera->rwCamera, (float)((double)camera->farClip - offset));
}

BMX_ADDR(0x00493130)
void zrwCameraSetViewOffset(ZrwCamera *camera, float x, float y)
{
    const float offset[2] = {x, y};

    RwCameraSetViewOffset(camera->rwCamera, offset);
}
