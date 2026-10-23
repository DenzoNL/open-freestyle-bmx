/* Renderer cameras: markers and the camera rasters (see zrwcam.h). */
#include "zge/com/zglrw/zrwcam.h"

#include "zge/com/zglrw/zrwmodelcreate.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"

static const char kZrwCamFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwCam.c";

ORIG_FN(0x004dcb10, void *, RwRasterCreate, (int width, int height, int depth, int flags));
ORIG_FN(0x004dcbd0, int, RwRasterDestroy, (void *raster));
/* Destroy callback of the camera pool (zrwcam.c, not ported yet). */
ORIG_FN(0x00492970, void, zrwcamDestroyRecord, (void *camera));

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
    gZrwCamPool = zmemFreeListCreate(200, 2, (ZMemDestroyFn)zrwcamDestroyRecord);
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
