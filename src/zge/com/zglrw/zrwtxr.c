/* Renderer textures (C:\NgEngine\Zge\Com\ZglRw\zrwTxr.c). */
#include "zge/com/zglrw/zrwtxr.h"

#include <stddef.h>

#include "compat/crt.h"
#include "rw/rwcore.h"
#include "rw/rwplugins.h"
#include "rw/rwstream.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/ziff.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zprofile.h"
#include "zge/zstring.h"

static const char kZrwTxrFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwTxr.c";

/* RenderWare library calls (statically linked in the exe, not ported). */
ORIG_FN(0x004dfcb0, void *, RwTextureRead, (const char *name, const char *maskName));
ORIG_FN(0x004e6d90, void *, RtBMPImageRead, (const char *name));
ORIG_FN(0x005068a0, void *, RtPNGImageRead, (const char *name));
ORIG_FN(0x004e3a60, int, RwImageGetWidth, (void *image));
ORIG_FN(0x004e3a70, int, RwImageGetHeight, (void *image));
ORIG_FN(0x004e3a80, int, RwImageGetDepth, (void *image));
ORIG_FN(0x004e3a90, uint8_t *, RwImageGetPixels, (void *image));
ORIG_FN(0x004e3aa0, uint8_t *, RwImageGetPalette, (void *image));
ORIG_FN(0x004e3960, int, RwImageDestroy, (void *image));
ORIG_FN(0x004dcb10, void *, RwRasterCreate, (int width, int height, int depth, int flags));
ORIG_FN(0x004e7f70, void *, RwRasterSetFromImage, (void *raster, void *image));
ORIG_FN(0x004dcc30, int, RwRasterGetWidth, (void *raster));
ORIG_FN(0x004dcc40, int, RwRasterGetHeight, (void *raster));
ORIG_FN(0x004dcc50, int, RwRasterGetStride, (void *raster));
ORIG_FN(0x004dcc60, int, RwRasterGetDepth, (void *raster));
ORIG_FN(0x004dcc70, uint32_t, RwRasterGetFormat, (void *raster));
ORIG_FN(0x004dd160, uint8_t *, RwRasterLockPalette, (void *raster, int mode));
ORIG_FN(0x004dd1d0, void *, RwRasterUnlockPalette, (void *raster));
ORIG_FN(0x004e5330, void, zrwPixelToRGBA, (uint8_t *rgba, uint32_t pixel, uint32_t format));
ORIG_FN(0x004df650, void *, RwTextureGetRaster, (void *texture));
ORIG_FN(0x004df5f0, void *, RwTextureSetRaster, (void *texture, void *raster));
ORIG_FN(0x004df910, void *, RwTextureCreate, (void *raster));
ORIG_FN(0x004df760, int, RwTextureDestroy, (void *texture));
ORIG_FN(0x004df990, const char *, RwTextureGetName, (void *texture));
ORIG_FN(0x004df9b0, void *, RwTextureSetName, (void *texture, const char *name));
ORIG_FN(0x004dff70, void *, RwTextureSetFilterMode, (void *texture, int mode));
ORIG_FN(0x004dffa0, void *, RwTextureSetAddressing, (void *texture, int mode));
ORIG_FN(0x004dfb20, void *, RwTexDictionaryAddTexture, (void *dictionary, void *texture));
ORIG_FN(0x004df670, void *, RwTexDictionaryCreate, (void));
ORIG_FN(0x004df810, int, RwTexDictionaryDestroy, (void *dictionary));
ORIG_FN(0x004dfc80, void *, RwTexDictionarySetCurrent, (void *dictionary));
ORIG_FN(0x004e4a30, int, RwImageRegisterImageFormat, (const char *extension, void *read, void *write));
ORIG_FN(0x004df580, int, RwTextureSetMipmapping, (int enable));
ORIG_FN(0x004df980, void *, RwTextureAddRef, (void *texture));
ORIG_FN(0x004df550, void *, RwTextureSetFindCallBack, (void *callback));
ORIG_FN(0x004df540, void *, RwTextureGetReadCallBack, (void));
ORIG_FN(0x004df520, int, RwTextureSetReadCallBack, (void *callback));
/* The image writers registered next to the readers, and worlds (RpWorld) with their PVS. */
ORIG_FN(0x004e7eb0, int, rwBmpImageWrite, (void));
ORIG_FN(0x004cc640, int, rwWorldIsPvsReady, (void *world));
ORIG_FN(0x004cc320, void, rwWorldPvsUnload, (void *world));
ORIG_FN(0x004ba9c0, int, RpWorldDestroy, (void *world));
ORIG_FN(0x004bb080, void *, RpWorldCreate, (const float *bbox));
ORIG_FN(0x004ba050, const float *, RpWorldGetOrigin, (void *world));
ORIG_FN(0x004b9fe0, const float *, RpWorldGetBBox, (void *world));
ORIG_FN(0x004ba460, void, rwWorldRender, (void *world));
ORIG_FN(0x004cef40, uint32_t, rwWorldGetPvsState, (void *world));
ORIG_FN(0x004cc0f0, void, rwWorldSetPvsCallback, (void *world, void *callback));
ORIG_FN(0x004ce960, int, rwWorldPvsCreate,
        (void *world, uint32_t a, uint32_t b, uint32_t c, uint32_t d, void *callback, uint32_t e));
ORIG_FN(0x004c3ae0, int, rwWorldPvsRead, (void *world, void *stream));
ORIG_FN(0x004de5a0, uint32_t, rwCameraGetWidth, (void *camera));
ORIG_FN(0x004de5d0, uint32_t, rwCameraGetHeight, (void *camera));

/* Other modules: the camera (pcinput.c), the world callbacks, the RAM disk file hook (zprofile.c), materials.
 */
ORIG_FN(0x00492eb0, uint8_t *, zrwGetCurrentCamera, (void));
ORIG_FN(0x00493060, void *, zrwCameraGetRwCamera, (const uint8_t *camera));
/* The world's PVS progress callback (ported in zrwworld.c); only its address is used here. */
ORIG_FN(0x00496620, int, zrwWorldPvsCallback, (int stage, float progress));
ORIG_FN(0x004cd510, void, rwWorldPvsDefaultCallback, (void));
ORIG_FN(0x00489ba0, void, zrwmatTextureLoadBegin, (void));
ORIG_FN(0x00489be0, void, zrwmatTextureLoaded, (ZrwTexture * texture));

/* Renderer interface and helpers the exe's data points at. */
typedef uint32_t(BMX_CDECL *ZrwTimeFn)(void);
typedef void(BMX_CDECL *ZrwSetBoundsFn)(int mode, const float *bounds, int flags);
typedef void(BMX_CDECL *ZrwReleaseFn)(void *object);
#define gZrwTimeGetTime     ORIG_VAR(0x00561480, ZrwTimeFn, gZrwTimeGetTime)
#define gZrwSetBounds       ORIG_VAR(0x00561484, ZrwSetBoundsFn, gZrwSetBounds)
#define gZrwMaterialRelease ORIG_VAR(0x00561024, ZrwReleaseFn, gZrwMaterialRelease)
#define gZrwTextureRelease  ORIG_VAR(0x00562400, ZrwReleaseFn, gZrwTextureRelease)
/* Camera data: the capture size seen by the world's PVS and a 0 terminated list of the file names. */
#define gZrwPvsKeys   ORIG_VAR(0x0056212c, const char *, gZrwPvsKeys)
#define gZrwPvsBuffer ORIG_VAR(0x00674208, uint8_t, gZrwPvsBuffer)
/* Defaults of the particle callbacks: gravity, and floor height with bounce factor. */
#define gZrwDefaultGravity ORIG_VAR(0x005623c0, ZVec3, gZrwDefaultGravity)
typedef float ZrwBounceParams[2];
#define gZrwDefaultBounce ORIG_VAR(0x005623d0, ZrwBounceParams, gZrwDefaultBounce)
/* File extensions handed to the RAM disk and registered with RenderWare. */
typedef char ZrwExtension[4];
#define kZrwExtBmp    ORIG_VAR(0x005625b4, ZrwExtension, kZrwExtBmp)
#define kZrwExtPng    ORIG_VAR(0x005625b8, ZrwExtension, kZrwExtPng)
#define kZrwFormatBmp ORIG_VAR(0x0056276c, ZrwExtension, kZrwFormatBmp)
#define kZrwFormatPng ORIG_VAR(0x00562768, ZrwExtension, kZrwFormatPng)
#define kZrwFormatDds ORIG_VAR(0x00562764, ZrwExtension, kZrwFormatDds)

/* ---- world and particle callbacks, TODO(file): probably belong to zrwworld.c ------------------------ */

BMX_ADDR(0x00497200)
int zrwWorldReset(void)
{
    float box[6] = {100.0f, 100.0f, 100.0f, -100.0f, -100.0f, -100.0f};
    float bounds[8];
    const float *origin, *extent;
    void *world;

    if (gZrwWorldActive == 0) {
        return 1;
    }
    if (rwWorldIsPvsReady(gZrwWorld) != 0) {
        rwWorldPvsUnload(gZrwWorld);
        gZrwPvsLoaded = 0;
        gZrwPvsEnabled = 0;
    }
    if (gZrwWorld != NULL) {
        RpWorldDestroy(gZrwWorld);
        gZrwWorld = NULL;
    }
    world = RpWorldCreate(box);
    if (world == NULL) {
        return 0;
    }
    gZrwWorld = world;
    origin = RpWorldGetOrigin(world);
    gZrwWorldCenter.x = origin[0];
    gZrwWorldCenter.y = origin[1];
    gZrwWorldCenterScaled.x = (float)((double)gZrwInvScale * gZrwWorldCenter.x);
    gZrwWorldCenter.z = origin[2];
    gZrwWorldCenterScaled.y = (float)((double)gZrwWorldCenter.y * gZrwInvScale);
    gZrwWorldCenterScaled.z = (float)((double)gZrwWorldCenter.z * gZrwInvScale);
    extent = RpWorldGetBBox(gZrwWorld);
    bounds[0] = (float)(((double)extent[3] - gZrwWorldCenter.x) * gZrwInvScale);
    bounds[1] = (float)-(((double)extent[1] - gZrwWorldCenter.y) * gZrwInvScale);
    bounds[2] = (float)-(((double)extent[2] - gZrwWorldCenter.z) * gZrwInvScale);
    bounds[4] = (float)(((double)extent[0] - gZrwWorldCenter.x) * gZrwInvScale);
    bounds[5] = (float)-(((double)extent[4] - gZrwWorldCenter.y) * gZrwInvScale);
    bounds[6] = (float)-(((double)extent[5] - gZrwWorldCenter.z) * gZrwInvScale);
    gZrwSetBounds(2, bounds, 0);
    if (gZrwWorldMaterials != NULL) {
        for (void *material = zlistFirst(gZrwWorldMaterials); material != NULL;
             material = zlistFirst(gZrwWorldMaterials)) {
            gZrwTextureRelease(gZrwMaterialTexture(material, 0));
            gZrwMaterialRelease(material);
            zlistRemoveCurrent(gZrwWorldMaterials);
        }
    }
    gZrwWorldActive = 0;
    return 1;
}

BMX_ADDR(0x00497400)
void *zrwWorldFindMaterial(int hash)
{
    for (void *material = zlistFirst(gZrwWorldMaterials); material != NULL;
         material = zlistNext(gZrwWorldMaterials)) {
        if (hash == gZrwTextureHash(gZrwMaterialTexture(material, 0))) {
            return material;
        }
    }
    return NULL;
}

BMX_ADDR(0x00497450)
int zrwWorldMaterialCount(void)
{
    return zlistCount(gZrwWorldMaterials);
}

BMX_ADDR(0x00497460)
void *zrwWorldGetMaterial(int index)
{
    return zlistGet(gZrwWorldMaterials, index);
}

BMX_ADDR(0x00497480)
int zrwWorldHasPvs(void)
{
    return rwWorldIsPvsReady(gZrwWorld) != 0;
}

/* Rebuilds the world's PVS for the size of the current camera. Returns 0 without a camera. */
BMX_ADDR(0x004974a0)
int zrwWorldSetupPvs(void)
{
    uint8_t *camera = zrwGetCurrentCamera();
    void *rwCamera;
    uint32_t width, height, extra;

    if (camera == NULL) {
        return 0;
    }
    rwCamera = zrwCameraGetRwCamera(camera);
    width = rwCameraGetWidth(rwCamera);
    height = rwCameraGetHeight(rwCamera);
    extra = *(uint32_t *)(camera + 0xb0);
    gZrwWorldPvsState = rwWorldGetPvsState(gZrwWorld);
    rwWorldSetPvsCallback(gZrwWorld, (void *)(uintptr_t)zrwWorldPvsCallback);
    if (rwWorldIsPvsReady(gZrwWorld) != 0) {
        rwWorldPvsUnload(gZrwWorld);
        gZrwPvsLoaded = 0;
        gZrwPvsEnabled = 0;
    }
    if (rwWorldPvsCreate(gZrwWorld, width, height, extra, 0, (void *)(uintptr_t)rwWorldPvsDefaultCallback,
                         0) != 0) {
        gZrwPvsLoaded = 1;
        gZrwPvsEnabled = 1;
    }
    return 1;
}

BMX_ADDR(0x00497570)
int zrwWorldSetPvsEnabled(int enabled)
{
    if (gZrwPvsLoaded != 0) {
        gZrwPvsEnabled = enabled;
        return 1;
    }
    if (enabled == 0) {
        gZrwPvsEnabled = 0;
        return 1;
    }
    gZdebugPrint("ZRWWORLD: No PVS data\n");
    return 0;
}

BMX_ADDR(0x004975b0)
int zrwWorldLoadPvs(void)
{
    uint8_t *values[1] = {&gZrwPvsBuffer};
    uint32_t data = zfileDeviceControl(zfileDefaultDevice(), 1, (uint32_t)(uintptr_t)&gZrwPvsKeys,
                                       (uint32_t)(uintptr_t)values);
    void *stream;
    int ok;

    if (data == 0) {
        return 0;
    }
    stream = RwStreamOpen(2, 2, (void *)(uintptr_t)data);
    if (stream == NULL) {
        return 0;
    }
    ok = rwWorldPvsRead(gZrwWorld, stream);
    RwStreamClose(stream, NULL);
    return ok != 0;
}

BMX_ADDR(0x00497620)
int zrwWorldRender(void)
{
    if (gZrwWorld != NULL) {
        rwWorldRender(gZrwWorld);
    }
    return 1;
}

BMX_ADDR(0x00497640)
void zrwWorldNop(void) {}

/* Particle system: +0 the list of particles, +0x28 the time of the last step. */
BMX_ADDR(0x00497650)
void zrwParticleAccelerate(int enabled, uint32_t *system, const ZVec3 *acceleration)
{
    const uint32_t now = gZrwTimeGetTime();
    const double elapsed = (double)(uint32_t)(now - system[10]);
    ZVec3 delta;
    const ZVec3 *accel = acceleration != NULL ? acceleration : &gZrwDefaultGravity;

    delta.x = (float)(elapsed * accel->x * 0x1.0624dep-10f);
    delta.y = (float)(elapsed * accel->y * 0x1.0624dep-10f);
    delta.z = (float)(elapsed * accel->z * 0x1.0624dep-10f);
    if (enabled != 0 && system != NULL) {
        ZList *list = (ZList *)(uintptr_t)system[0];

        for (uint8_t *particle = zlistFirst(list); particle != NULL; particle = zlistNext(list)) {
            zvec3Add((ZVec3 *)(particle + 0x24), (ZVec3 *)(particle + 0x24), &delta);
        }
    }
}

BMX_ADDR(0x004976f0)
void zrwParticleNop(void) {}

/* Particles whose y reaches the floor value are set onto it and have their velocity (+0x28) reflected. */
BMX_ADDR(0x00497700)
void zrwParticleFloorBounce(int enabled, uint32_t *system, const float *params)
{
    const float *p = params != NULL ? params : gZrwDefaultBounce;
    const float floor = p[0];
    const float bounce = p[1];

    if (enabled != 0 && system != NULL) {
        ZList *list = (ZList *)(uintptr_t)system[0];

        for (uint8_t *particle = zlistFirst(list); particle != NULL; particle = zlistNext(list)) {
            float *y = (float *)(particle + 8);

            if ((double)*y >= (double)floor) {
                float *velocity = (float *)(particle + 0x28);

                *y = floor;
                *velocity = (float)-((double)bounce * *velocity);
            }
        }
    }
}

/* ---- raster and image helpers -------------------------------------------------------------------------- */

/* Prints the format name, size, stride and depth of a raster. */
BMX_ADDR(0x00497780)
void zrwRasterLog(void *raster)
{
    uint32_t format;
    int width, height, stride, depth;
    const char *name;

    if (raster == NULL) {
        return;
    }
    format = RwRasterGetFormat(raster);
    width = RwRasterGetWidth(raster);
    height = RwRasterGetHeight(raster);
    stride = RwRasterGetStride(raster);
    depth = RwRasterGetDepth(raster);
    switch (format) {
    case 0x800:
    case 0x700:
        name = "rwRASTERFORMAT24";
        break;
    case 0x400:
        name = "rwRASTERFORMATLUM8";
        break;
    case 0x200:
        name = "rwRASTERFORMAT565";
        break;
    case 0:
        name = "rwRASTERFORMATDEFAULT";
        break;
    case 0x100:
        name = "rwRASTERFORMAT1555";
        break;
    case 0x300:
        name = "rwRASTERFORMAT4444";
        break;
    case 0x500:
    case 0x600:
        name = "rwRASTERFORMAT888";
        break;
    case 0x2000:
        name = "rwRASTERFORMATPAL8";
        break;
    case 0x900:
        name = "rwRASTERFORMAT32";
        break;
    case 0xa00:
        name = "rwRASTERFORMAT555";
        break;
    case 0xf00:
        name = "rwRASTERFORMATPIXELFORMATMASK";
        break;
    case 0x4000:
        name = "rwRASTERFORMATPAL4";
        break;
    case 0x8000:
        name = "rwRASTERFORMATMIPMAP";
        break;
    case 0xff00:
        name = "rwRASTERFORMATMASK";
        break;
    default:
        name = "Unknown";
        break;
    }
    gZdebugPrint("Format: %s\nWidth: %d\nHeight: %d\nStride: %d\nDepth: %d\n", name, width, height, stride,
                 depth);
}

BMX_ADDR(0x004978f0)
void zrwImageApplyColorKey(void *image)
{
    const int depth = RwImageGetDepth(image);

    if (depth == 4 || depth == 8) {
        uint8_t *entry = RwImageGetPalette(image);

        for (uint32_t n = 1u << (depth & 31); n != 0; n--, entry += 4) {
            if (entry[0] == gZrwTxr.key[0] && entry[1] == gZrwTxr.key[1] && entry[2] == gZrwTxr.key[2]) {
                entry[0] = entry[1] = entry[2] = entry[3] = 0;
            }
        }
    } else {
        uint8_t *row = RwImageGetPixels(image);
        const uint32_t width = (uint32_t)RwImageGetWidth(image);
        const uint32_t height = (uint32_t)RwImageGetHeight(image);
        const int32_t stride = *(int32_t *)((uint8_t *)image + 0x10);

        for (uint32_t y = height; y != 0; y--, row += stride) {
            uint8_t *pixel = row;

            for (uint32_t x = width; x != 0; x--, pixel += 4) {
                if (pixel[0] == gZrwTxr.key[0] && pixel[1] == gZrwTxr.key[1] && pixel[2] == gZrwTxr.key[2]) {
                    pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
                }
            }
        }
    }
}

/* Reads an image through RenderWare's BMP reader, clears the colour key and remembers its size. */
BMX_ADDR(0x004979e0)
void *BMPImageRead(const char *name)
{
    void *image = RtBMPImageRead(name);

    if (image != NULL) {
        zrwImageApplyColorKey(image);
        gZrwTxr.width = RwImageGetWidth(image);
        gZrwTxr.height = RwImageGetHeight(image);
        gZrwTxr.depth = RwImageGetDepth(image);
    }
    return image;
}

BMX_ADDR(0x00497a30)
void *PNGImageRead(const char *name)
{
    void *image = RtPNGImageRead(name);

    if (image != NULL) {
        zrwImageApplyColorKey(image);
        gZrwTxr.width = RwImageGetWidth(image);
        gZrwTxr.height = RwImageGetHeight(image);
        gZrwTxr.depth = RwImageGetDepth(image);
    }
    return image;
}

/* DDS images are not supported: the reader fails. */
BMX_ADDR(0x00497a80)
void *DDSImageRead(void)
{
    return NULL;
}

BMX_ADDR(0x00497a90)
void *rwMatConstructor(void *material)
{
    if (gZrwMatPluginOffset > 0) {
        *(uint32_t *)((uint8_t *)material + gZrwMatPluginOffset) = 0;
    }
    return material; /* the plugin registry treats NULL as failure */
}

BMX_ADDR(0x00497ab0)
void *rwMatDestructor(void *material)
{
    return material;
}

BMX_ADDR(0x00497ae0)
uint32_t zrwMatGetUserData(const void *material)
{
    if (gZrwMatPluginOffset < 1) {
        return 0;
    }
    return *(const uint32_t *)((const uint8_t *)material + gZrwMatPluginOffset);
}

/*
 * Makes a texture wrapper from an image file in memory: RenderWare reads it through the RAM disk (the file
 * system hook is told where the data is); when that fails the image is read directly (always as a BMP) and
 * turned into a raster by hand. `type` 0 is BMP, 1 PNG, 4 anything.
 */
BMX_ADDR(0x00497b00)
ZrwTexture *LoadTexture(const void *data, uint32_t size, const char *name, int type)
{
    const char *extension;
    void *texture;
    ZrwTexture *record;
    void *raster;
    RwError error;
    int depth, width, height;

    if (data == NULL || size == 0) {
        zdebugFail("Error in parameters", kZrwTxrFile, 0x1ed, 0);
        return NULL;
    }
    if (type == 0) {
        extension = kZrwExtBmp;
    } else if (type == 1) {
        extension = kZrwExtPng;
    } else if (type == 4) {
        extension = NULL;
    } else {
        zdebugFail("Bad EzglImageTypes", kZrwTxrFile, 0x1fd, 0);
        return NULL;
    }
    RegisterCallBacksForFileSystem();
    ImageBufferSizeType(data, size, extension);
    ImageBufferSizeType(data, size, extension);
    texture = RwTextureRead(name, NULL);
    zrwFileRamDiskClear();
    if (texture == NULL) {
        void *image;

        RwErrorGet(&error);
        RegisterCallBacksForFileSystem();
        ImageBufferSizeType(data, size, extension);
        image = RtBMPImageRead(name);
        if (image != NULL) {
            zrwImageApplyColorKey(image);
            gZrwTxr.width = RwImageGetWidth(image);
            gZrwTxr.height = RwImageGetHeight(image);
            gZrwTxr.depth = RwImageGetDepth(image);
            height = RwImageGetHeight(image);
            width = RwImageGetWidth(image);
            raster = RwRasterCreate(width, height, 0x10, 0x204);
            if (raster == NULL) {
                height = RwImageGetHeight(image);
                width = RwImageGetWidth(image);
                raster = RwRasterCreate(width, height, 0x20, 4);
            }
            if (raster != NULL) {
                gZrwTxr.width = RwRasterGetWidth(RwTextureGetRaster(texture));
                gZrwTxr.height = RwRasterGetHeight(RwTextureGetRaster(texture));
                raster = RwRasterSetFromImage(raster, image);
                RwImageDestroy(image);
                if (raster != NULL) {
                    void *created = RwTextureCreate(raster);

                    if (created != NULL) {
                        RwTextureSetRaster(created, raster);
                        RwTextureSetName(created, name);
                        texture = created;
                        RwTextureSetFilterMode(texture, gZrwTxr.mipmapped != 0 ? 6 : 2);
                        RwTextureSetAddressing(texture, (gZrwFlags & 1) != 0 ? 3 : 1);
                        RwTexDictionaryAddTexture(gZrwTxr.dictionary, texture);
                    }
                }
            }
        }
        zrwFileRamDiskClear();
        RwErrorGet(&error);
        if (texture == NULL) {
            zdebugFail("Failed to create texture", kZrwTxrFile, 0x277, 0);
            return NULL;
        }
    }
    record = zmemFreeListAlloc(gZrwTxr.textures);
    memset(record, 0, sizeof *record);
    record->flags = (record->flags & 0xffffff04) | 4;
    depth = RwRasterGetDepth(RwTextureGetRaster(texture));
    height = gZrwTxr.height;
    gZrwTxr.depth = depth;
    width = gZrwTxr.width;
    record->texture = texture;
    record->name = RwTextureGetName(texture);
    record->nameHash = zstrHash(record->name);
    record->width = width;
    record->refCount++;
    record->height = height;
    record->depth = depth;
    return record;
}

/* RenderWare's texture read callback with the filter and addressing modes of the game applied. */
BMX_ADDR(0x004981a0)
void *zrwTextureReadCallback(const char *name, const char *mask)
{
    void *texture = gZrwTxr.readCallback(name, mask);

    if (texture == NULL) {
        return NULL;
    }
    RwTextureSetFilterMode(texture, gZrwTxr.mipmapped != 0 ? 6 : 2);
    if ((gZrwFlags & 1) != 0) {
        RwTextureSetAddressing(texture, 3);
    }
    return texture;
}

/* TBMP chunk: a BMP file in the IFF. Uses the fixed load buffer when one is set. */
BMX_ADDR(0x004981f0)
int ReadTBMP(void *iff)
{
    uint8_t *buffer;
    uint32_t size;
    ZrwTexture *texture;

    zrwmatTextureLoadBegin();
    size = ziffChunkSize(iff);
    if (gZrwTxr.loadBuffer == NULL) {
        buffer = zmemAlloc(size);
    } else {
        if (size > gZrwTxr.loadBufferSize) {
            zdebugFail("Load buffer is too small.", kZrwTxrFile, 0x396, 0);
        }
        buffer = gZrwTxr.loadBuffer;
    }
    ziffRead(iff, buffer, size);
    texture = LoadTexture(buffer, size, gZrwTxr.loadName, 0);
    if (texture != NULL) {
        zrwmatTextureLoaded(texture);
    } else {
        gZdebugPrint("zrwTxr: Error loading texture:%s\n", gZrwTxr.loadName);
    }
    if (buffer != gZrwTxr.loadBuffer) {
        zmemFree(buffer);
    }
    return 1;
}

BMX_ADDR(0x004982a0)
int ReadTPNG(void *iff)
{
    const uint32_t size = ziffChunkSize(iff);
    uint8_t *buffer = zmemAlloc(size);
    ZrwTexture *texture;

    ziffRead(iff, buffer, size);
    texture = LoadTexture(buffer, size, gZrwTxr.loadName, 1);
    if (texture != NULL) {
        zrwmatTextureLoaded(texture);
    } else {
        gZdebugPrint("zrwTxr: Error loading texture:%s\n", gZrwTxr.loadName);
    }
    zmemFree(buffer);
    return 1;
}

/* DDS chunks are skipped. */
BMX_ADDR(0x00498320)
int ReadTDDS(void)
{
    return 1;
}

/* Converts entry `index` of RGBA bytes into the raster's palette format (16 or 32 bits per entry). */
BMX_ADDR(0x004983f0)
void zrwPaletteEncodeEntry(void *out, uint32_t format, int index, const uint8_t *rgba)
{
    const uint8_t b0 = rgba[index * 4];
    const uint8_t b1 = rgba[index * 4 + 1];
    const uint8_t b2 = rgba[index * 4 + 2];
    const uint8_t b3 = rgba[index * 4 + 3];

    switch (format & 0xf00) {
    case 0x300:
        ((uint16_t *)out)[index] = (uint16_t)((((uint16_t)(b3 & 0xf0) << 4 | (uint16_t)(b2 & 0xf0)) << 4) |
                                              (uint16_t)((b1 & 0xf0) | (b0 >> 4)));
        break;
    case 0x100:
        ((uint16_t *)out)[index] =
            (uint16_t)(((((uint16_t)(b3 & 0x80) << 1 | (uint16_t)(b2 & 0xf8)) << 5 | (uint16_t)(b1 & 0xf8))
                        << 2) |
                       (uint16_t)(b0 >> 3));
        break;
    case 0x200:
        ((uint16_t *)out)[index] =
            (uint16_t)((((uint16_t)(b2 & 0xf8) << 6 | (uint16_t)(b1 & 0xfc)) << 2) | (uint16_t)(b0 >> 3));
        break;
    case 0x500:
        ((uint32_t *)out)[index] = ((uint32_t)b3 << 24) | ((uint32_t)b2 << 16) | ((uint32_t)b1 << 8) | b0;
        break;
    case 0x600:
        ((uint32_t *)out)[index] = ((uint32_t)b2 << 16) | ((uint32_t)b1 << 8) | b0;
        break;
    default:
        break;
    }
}

BMX_ADDR(0x00498580)
int zrwTextureLog(const ZrwTexture *texture)
{
    gZdebugPrint("Name:%s Width:%d Height:%d BPP:%d\n", texture->name, texture->width, texture->height,
                 texture->depth);
    zrwRasterLog(RwTextureGetRaster(texture->texture));
    return 1;
}

BMX_ADDR(0x00498660)
int zrwTextureSystemInitialize(void)
{
    const uint32_t key = 0x00ff00ff;

    if (gZrwTxr.initialized != 0) {
        zdebugFail("Already initialized", kZrwTxrFile, 0x4f6, 0);
    }
    memset(&gZrwTxr, 0, sizeof gZrwTxr);
    gZrwTxr.initialized = 1;
    gZrwTxr.textures =
        zmemFreeListCreate(0x28, 0x100, BMX_FNPTR(ZMemDestroyFn, 0x00498810, zrwTextureRelease));
    memcpy(gZrwTxr.key, &key, sizeof key);
    ziffSetupCallback(0x504d4254, BMX_FNPTR(int(BMX_CDECL *)(void *), 0x004981f0, ReadTBMP), 0);
    ziffSetupCallback(0x474e5054, BMX_FNPTR(int(BMX_CDECL *)(void *), 0x004982a0, ReadTPNG), 0);
    ziffSetupCallback(0x53444454, BMX_FNPTR(int(BMX_CDECL *)(void *), 0x00498320, ReadTDDS), 0);
    gZrwTxr.dictionary = RwTexDictionaryCreate();
    if (gZrwTxr.dictionary != NULL) {
        RwTexDictionarySetCurrent(gZrwTxr.dictionary);
        RwImageRegisterImageFormat(kZrwFormatBmp, BMX_FNPTR(void *, 0x004979e0, BMPImageRead),
                                   (void *)(uintptr_t)rwBmpImageWrite);
        RwImageRegisterImageFormat(kZrwFormatPng, BMX_FNPTR(void *, 0x00497a30, PNGImageRead), NULL);
        RwImageRegisterImageFormat(kZrwFormatDds, BMX_FNPTR(void *, 0x00497a80, DDSImageRead), NULL);
        gZrwTxr.mipmapped = 0;
        RwTextureSetMipmapping(0);
        RwTextureSetFindCallBack(NULL);
        gZrwTxr.readCallback = RwTextureGetReadCallBack();
        RwTextureSetReadCallBack(BMX_FNPTR(void *, 0x004981a0, zrwTextureReadCallback));
    }
    return 1;
}

BMX_ADDR(0x00498790)
int zrwTextureSystemShutdown(void)
{
    if (gZrwTxr.initialized != 0) {
        zmemFreeListDestroy(gZrwTxr.textures);
        RwTexDictionaryDestroy(gZrwTxr.dictionary);
        gZrwTxr.dictionary = NULL;
        RwTexDictionarySetCurrent(NULL);
        gZrwTxr.initialized = 0;
    }
    return 1;
}

BMX_ADDR(0x004987e0)
ZrwTexture *zrwTextureCreateRecord(void)
{
    ZrwTexture *record = zmemFreeListAlloc(gZrwTxr.textures);

    memset(record, 0, sizeof *record);
    record->flags = (record->flags & 0xffffff04) | 4;
    return record;
}

/* Free list destroy callback and release: drops one use of the RenderWare texture and frees the record. */
BMX_ADDR(0x00498810)
int zrwTextureRelease(ZrwTexture *texture)
{
    RwTextureDestroy(texture->texture);
    texture->refCount--;
    if (texture->refCount < 0) {
        zdebugFail("Texture reference count has become negative.", kZrwTxrFile, 0x551, 0);
    }
    if (texture->refCount == 0) {
        zmemFreeListFree(gZrwTxr.textures, texture);
    }
    return 1;
}

BMX_ADDR(0x00498870)
int zrwTextureGetWidth(const ZrwTexture *texture)
{
    return texture->width;
}

BMX_ADDR(0x00498880)
int zrwTextureGetHeight(const ZrwTexture *texture)
{
    return texture->height;
}

BMX_ADDR(0x00498890)
int zrwTextureGetDepth(const ZrwTexture *texture)
{
    return texture->depth;
}

BMX_ADDR(0x004988a0)
const char *zrwTextureGetName(const ZrwTexture *texture)
{
    return texture->name;
}

BMX_ADDR(0x004988b0)
int zrwTextureGetNameHash(const ZrwTexture *texture)
{
    return texture->nameHash;
}

/*
 * Reads the palette of a paletted raster as RGBA bytes into gZrwTxr.palette. Entries come from the raster in
 * its pixel format; an unknown format leaves the previous value in use (the first one is the texture
 * pointer itself).
 */
BMX_ADDR(0x004988c0)
uint8_t *zrwTextureReadPalette(const ZrwTexture *texture)
{
    void *raster = RwTextureGetRaster(texture->texture);
    const uint32_t format = RwRasterGetFormat(raster);
    uint8_t *palette;
    uint32_t value = (uint32_t)(uintptr_t)texture;
    const uint16_t *next16;
    int count;

    if ((format & 0x6000) == 0) {
        return NULL;
    }
    palette = RwRasterLockPalette(raster, 2);
    if (palette == NULL) {
        return NULL;
    }
    next16 = (const uint16_t *)palette;
    count = (int)(1u << (texture->depth & 31));
    for (int i = 0; i < count; i++) {
        uint8_t rgba[4];

        switch (format & 0xf00) {
        case 0x100:
        case 0x200:
        case 0x300:
        case 0x700:
            value = *next16;
            break;
        case 0x400:
            value = palette[i];
            break;
        case 0x500:
        case 0x600:
        case 0x800:
        case 0x900:
            value = ((const uint32_t *)palette)[i];
            break;
        default:
            break;
        }
        zrwPixelToRGBA(rgba, value, format);
        gZrwTxr.palette[i * 4] = rgba[0];
        gZrwTxr.palette[i * 4 + 1] = rgba[1];
        next16++;
        gZrwTxr.palette[i * 4 + 2] = rgba[2];
        gZrwTxr.palette[i * 4 + 3] = rgba[3];
        count = (int)(1u << (texture->depth & 31));
    }
    RwRasterUnlockPalette(raster);
    return gZrwTxr.palette;
}

/* Writes gZrwTxr.palette back into the raster's palette. 0 for an unpaletted texture. */
BMX_ADDR(0x00498a20)
int zrwTextureWritePalette(const ZrwTexture *texture)
{
    void *raster = RwTextureGetRaster(texture->texture);
    const uint32_t format = RwRasterGetFormat(raster);
    uint8_t *palette;

    if ((format & 0x6000) == 0) {
        gZdebugPrint("Texture isn't paletted: %s\n", texture->name);
        return 0;
    }
    palette = RwRasterLockPalette(raster, 1);
    if (palette == NULL) {
        return 0;
    }
    for (int i = 0; i < (int)(1u << (texture->depth & 31)); i++) {
        zrwPaletteEncodeEntry(palette, format, i, gZrwTxr.palette);
    }
    RwRasterUnlockPalette(raster);
    return 1;
}

/*
 * Accessors and plugin glue at 0x498ac0..0x498cb0, filed under zanim.c by the packet but part of this
 * file's address range.
 */
BMX_ADDR(0x00498ac0)
void *zrwTextureGetRwTexture(const ZrwTexture *texture)
{
    return texture->texture;
}

BMX_ADDR(0x00498ad0)
void *zrwTextureGetRaster(const ZrwTexture *texture)
{
    return RwTextureGetRaster(texture->texture);
}

BMX_ADDR(0x00498af0)
void *zrwTextureAddRef(ZrwTexture *texture)
{
    texture->refCount++;
    return RwTextureAddRef(texture->texture);
}

BMX_ADDR(0x00498b10)
int zrwMatRegisterPlugin(void)
{
    gZrwMatPluginOffset =
        RwTextureRegisterPlugin(4, 0xff01, BMX_FNPTR(RwPluginObjectConstructor, 0x00497a90, rwMatConstructor),
                                BMX_FNPTR(RwPluginObjectDestructor, 0x00497ab0, rwMatDestructor), NULL);
    return gZrwMatPluginOffset != -1;
}

BMX_ADDR(0x00498b40)
void zrwTextureSetLoadName(const char *name)
{
    gZrwTxr.loadName = name;
}

BMX_ADDR(0x00498b50)
void zrwTextureSetUserData(const ZrwTexture *texture, uint32_t value)
{
    if (gZrwMatPluginOffset > 0) {
        *(uint32_t *)((uint8_t *)texture->texture + gZrwMatPluginOffset) = value;
    }
}

BMX_ADDR(0x00498b70)
void zrwSetMaterialParam(const void *value)
{
    gZrwTxr.mipmapped = (int)(uintptr_t)value;
    RwTextureSetMipmapping((int)(uintptr_t)value);
    RwTextureSetFindCallBack((void *)(uintptr_t)value);
}

BMX_ADDR(0x00498b90)
void zrwSetMaterialValue(int value)
{
    (void)value;
}

BMX_ADDR(0x00498cb0)
void zrwTextureLogAll(void)
{
    gZdebugPrint("ZGLTEXTURE: Objects in use:%d\n", zmemFreeListUsedCount(gZrwTxr.textures));
    gZdebugIndent(1);
    zmemFreeListForEach(gZrwTxr.textures,
                        BMX_FNPTR(void(BMX_CDECL *)(void *, void *), 0x00498580, zrwTextureLog), NULL);
    gZdebugIndent(-1);
}
