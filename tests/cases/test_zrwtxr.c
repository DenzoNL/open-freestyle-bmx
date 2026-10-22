/*
 * Tests for the renderer texture glue (zrwtxr.c) and the world and particle callbacks filed with it.
 * RenderWare is stubbed: the stubs log their arguments and answer from fixtures (fake images, rasters,
 * textures and worlds), so the order and values of every call are compared between original and port.
 */
#include <stdarg.h>

#include "dt.h"
#include "dt_ngif.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwtxr.h"
#include "zge/zfile.h"
#include "zge/zlist.h"

/* ---- fixtures and answers ------------------------------------------------------------------------- */

/* Fake image: +0 width, +4 height, +8 depth, +0x10 stride, +0x14 pixels, +0x18 palette. */
enum { kImgW = 0, kImgH = 4, kImgD = 8, kImgStride = 0x10, kImgPixels = 0x14, kImgPalette = 0x18 };
/* Fake raster: +0 width, +4 height, +8 stride, +0xc depth, +0x10 format, +0x14 palette. */
enum { kRasW = 0, kRasH = 4, kRasStride = 8, kRasDepth = 0xc, kRasFormat = 0x10, kRasPalette = 0x14 };

static uint8_t gRaster[0x40];     /* the raster RwTextureGetRaster answers for a NULL texture */
static uint8_t gNewTexture[0x40]; /* made by RwTextureCreate */
static uint8_t gWorldOrigin[16];
static uint8_t gWorldBounds[32];
static void *gImageAnswer;      /* what RtBMPImageRead / RtPNGImageRead return */
static void *gTextureAnswer;    /* what RwTextureRead and the original read callback return */
static void *gRasterAnswers[2]; /* the two RwRasterCreate calls */
static int gRasterCalls;
static int gSetFromImageOk, gTextureCreateOk;
static void *gDictAnswer;
static int gPvsReady, gPvsCreateOk, gPvsReadOk;
static void *gWorldAnswer;
static void *gCamera;
static uint32_t gChunkSize, gDeviceControlAnswer, gStreamAnswer;
static uint32_t gTime;
static void *gPaletteBuffer;
static int gLockOk;

static void BMX_CDECL zrwPrint(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    ngLog(0x480240);
    ngLog(ngHash(fmt));
    for (const char *p = fmt; *p; p++) {
        if (p[0] == '%' && p[1] == 's') {
            const char *s = va_arg(ap, const char *);

            ngLog(s != NULL ? ngHash(s) : 0);
        } else if (p[0] == '%' && (p[1] == 'd' || p[1] == 'x')) {
            ngLog((uint32_t)va_arg(ap, int));
        }
    }
    va_end(ap);
}

static uint32_t rd(const void *base, uint32_t off)
{
    return *(const uint32_t *)((const uint8_t *)base + off);
}

/* ---- stubs ---------------------------------------------------------------------------------------- */

static void *BMX_CDECL stubTextureRead(const char *name, const char *mask)
{
    ngLog(0x4dfcb0);
    ngLog(ngHash(name));
    ngLogPtr(mask);
    return gTextureAnswer;
}

static void *BMX_CDECL stubErrorGet(void *error)
{
    ngLog(0x4ded90);
    ngLogPtr(error);
    return error;
}

static void *BMX_CDECL stubBmpRead(const char *name)
{
    ngLog(0x4e6d90);
    ngLog(ngHash(name));
    return gImageAnswer;
}

static void *BMX_CDECL stubPngRead(const char *name)
{
    ngLog(0x5068a0);
    ngLog(ngHash(name));
    return gImageAnswer;
}

static int BMX_CDECL stubImageWidth(void *image)
{
    ngLog(0x4e3a60);
    ngLogPtr(image);
    return (int)rd(image, kImgW);
}

static int BMX_CDECL stubImageHeight(void *image)
{
    ngLog(0x4e3a70);
    ngLogPtr(image);
    return (int)rd(image, kImgH);
}

static int BMX_CDECL stubImageDepth(void *image)
{
    ngLog(0x4e3a80);
    ngLogPtr(image);
    return (int)rd(image, kImgD);
}

static void *BMX_CDECL stubImagePixels(void *image)
{
    ngLog(0x4e3a90);
    ngLogPtr(image);
    return (void *)(uintptr_t)rd(image, kImgPixels);
}

static void *BMX_CDECL stubImagePalette(void *image)
{
    ngLog(0x4e3aa0);
    ngLogPtr(image);
    return (void *)(uintptr_t)rd(image, kImgPalette);
}

static int BMX_CDECL stubImageDestroy(void *image)
{
    ngLog(0x4e3960);
    ngLogPtr(image);
    return 1;
}

static void *BMX_CDECL stubRasterCreate(int w, int h, int depth, int flags)
{
    ngLog(0x4dcb10);
    ngLog((uint32_t)w);
    ngLog((uint32_t)h);
    ngLog((uint32_t)depth);
    ngLog((uint32_t)flags);
    return gRasterAnswers[(gRasterCalls++) & 1];
}

static void *BMX_CDECL stubSetFromImage(void *raster, void *image)
{
    ngLog(0x4e7f70);
    ngLogPtr(raster);
    ngLogPtr(image);
    return gSetFromImageOk ? raster : NULL;
}

static int BMX_CDECL stubRasterWidth(void *raster)
{
    ngLog(0x4dcc30);
    ngLogPtr(raster);
    return (int)rd(raster, kRasW);
}

static int BMX_CDECL stubRasterHeight(void *raster)
{
    ngLog(0x4dcc40);
    ngLogPtr(raster);
    return (int)rd(raster, kRasH);
}

static int BMX_CDECL stubRasterStride(void *raster)
{
    ngLog(0x4dcc50);
    ngLogPtr(raster);
    return (int)rd(raster, kRasStride);
}

static int BMX_CDECL stubRasterDepth(void *raster)
{
    ngLog(0x4dcc60);
    ngLogPtr(raster);
    return (int)rd(raster, kRasDepth);
}

static uint32_t BMX_CDECL stubRasterFormat(void *raster)
{
    ngLog(0x4dcc70);
    ngLogPtr(raster);
    return rd(raster, kRasFormat);
}

static uint8_t *BMX_CDECL stubLockPalette(void *raster, int mode)
{
    ngLog(0x4dd160);
    ngLogPtr(raster);
    ngLog((uint32_t)mode);
    return gLockOk ? gPaletteBuffer : NULL;
}

static void *BMX_CDECL stubUnlockPalette(void *raster)
{
    ngLog(0x4dd1d0);
    ngLogPtr(raster);
    return raster;
}

static void BMX_CDECL stubPixelToRgba(uint8_t *rgba, uint32_t pixel, uint32_t format)
{
    ngLog(0x4e5330);
    ngLog(pixel);
    ngLog(format);
    rgba[0] = (uint8_t)pixel;
    rgba[1] = (uint8_t)(pixel >> 8);
    rgba[2] = (uint8_t)(pixel >> 16);
    rgba[3] = (uint8_t)(pixel >> 24) ^ (uint8_t)format;
}

/* A fake RwTexture: +0 raster, +8 name. A NULL texture (a failed read) answers the fixed raster. */
static void *BMX_CDECL stubTextureRaster(void *texture)
{
    ngLog(0x4df650);
    ngLogPtr(texture);
    return texture != NULL ? (void *)(uintptr_t)rd(texture, 0) : (void *)gRaster;
}

static void *BMX_CDECL stubTextureSetRaster(void *texture, void *raster)
{
    ngLog(0x4df5f0);
    ngLogPtr(texture);
    ngLogPtr(raster);
    *(void **)texture = raster;
    return texture;
}

static void *BMX_CDECL stubTextureCreate(void *raster)
{
    ngLog(0x4df910);
    ngLogPtr(raster);
    return gTextureCreateOk ? (void *)gNewTexture : NULL;
}

static int BMX_CDECL stubTextureDestroy(void *texture)
{
    ngLog(0x4df760);
    ngLogPtr(texture);
    return 1;
}

static const char *BMX_CDECL stubTextureGetName(void *texture)
{
    ngLog(0x4df990);
    ngLogPtr(texture);
    return (const char *)texture + 8;
}

static void *BMX_CDECL stubTextureSetName(void *texture, const char *name)
{
    ngLog(0x4df9b0);
    ngLogPtr(texture);
    ngLog(ngHash(name));
    return texture;
}

static void *BMX_CDECL stubSetFilter(void *texture, int mode)
{
    ngLog(0x4dff70);
    ngLogPtr(texture);
    ngLog((uint32_t)mode);
    return texture;
}

static void *BMX_CDECL stubSetAddressing(void *texture, int mode)
{
    ngLog(0x4dffa0);
    ngLogPtr(texture);
    ngLog((uint32_t)mode);
    return texture;
}

static void *BMX_CDECL stubDictAdd(void *dict, void *texture)
{
    ngLog(0x4dfb20);
    ngLogPtr(dict);
    ngLogPtr(texture);
    return texture;
}

static void *BMX_CDECL stubDictCreate(void)
{
    ngLog(0x4df670);
    return gDictAnswer;
}

static int BMX_CDECL stubDictDestroy(void *dict)
{
    ngLog(0x4df810);
    ngLogPtr(dict);
    return 1;
}

static void *BMX_CDECL stubDictSetCurrent(void *dict)
{
    ngLog(0x4dfc80);
    ngLogPtr(dict);
    return dict;
}

static int BMX_CDECL stubRegisterFormat(const char *ext, void *read, void *write)
{
    ngLog(0x4e4a30);
    ngLog(ngHash(ext));
    ngLogPtr(read);
    ngLogPtr(write);
    return 1;
}

static int BMX_CDECL stubMipmapping(int enable)
{
    ngLog(0x4df580);
    ngLog((uint32_t)enable);
    return 1;
}

static void *BMX_CDECL stubSetFind(void *callback)
{
    ngLog(0x4df550);
    ngLogPtr(callback);
    return callback;
}

static void *BMX_CDECL stubGetReadCb(void)
{
    ngLog(0x4df540);
    return (void *)0x4df000;
}

static int BMX_CDECL stubSetReadCb(void *callback)
{
    ngLog(0x4df520);
    ngLogPtr(callback);
    return 1;
}

static void BMX_CDECL stubRegisterFs(void)
{
    ngLog(0x4a3500);
}

static void BMX_CDECL stubBufferSize(const void *data, uint32_t size, const char *extension)
{
    ngLog(0x4a3580);
    ngLogPtr(data);
    ngLog(size);
    ngLog(extension != NULL ? ngHash(extension) : 0);
}

static void BMX_CDECL stubRamDiskClear(void)
{
    ngLog(0x4a35c0);
}

static void BMX_CDECL stubLoadBegin(void)
{
    ngLog(0x489ba0);
}

static void BMX_CDECL stubTextureLoaded(void *texture)
{
    ngLog(0x489be0);
    ngLogPtr(texture);
}

static uint32_t BMX_CDECL stubChunkSize(void *iff)
{
    ngLog(0x49abc0);
    ngLogPtr(iff);
    return gChunkSize;
}

static int BMX_CDECL stubIffRead(void *iff, uint8_t *dst, uint32_t size)
{
    ngLog(0x49ac30);
    ngLogPtr(iff);
    ngLogPtr(dst);
    ngLog(size);
    for (uint32_t i = 0; i < size && i < 64; i++) {
        dst[i] = (uint8_t)(i * 7 + 3);
    }
    return 1;
}

static void *BMX_CDECL stubSetupCallback(uint32_t id, void *cb, int flags)
{
    ngLog(0x49aad0);
    ngLog(id);
    ngLogPtr(cb);
    ngLog((uint32_t)flags);
    return NULL;
}

static void *BMX_CDECL stubFreeListCreate(uint32_t elem, uint32_t count, void *destroy)
{
    ngLog(0x485e80);
    ngLog(elem);
    ngLog(count);
    ngLogPtr(destroy);
    return (void *)(uintptr_t)0x4500;
}

static int BMX_CDECL stubFreeListDestroy(void *list)
{
    ngLog(0x485fb0);
    ngLogPtr(list);
    return 1;
}

static void *BMX_CDECL stubDefaultDevice(void)
{
    ngLog(0x483f10);
    return (void *)0x6100;
}

static uint32_t BMX_CDECL stubDeviceControl(void *device, uint32_t command, uint32_t a, uint32_t b)
{
    ngLog(0x483d30);
    ngLogPtr(device);
    ngLog(command);
    ngLog(a);
    ngLog(*(uint32_t *)(uintptr_t)b);
    return gDeviceControlAnswer;
}

/* The world. */
static int BMX_CDECL stubPvsReady(void *world)
{
    ngLog(0x4cc640);
    ngLogPtr(world);
    return gPvsReady;
}

static void BMX_CDECL stubPvsUnload(void *world)
{
    ngLog(0x4cc320);
    ngLogPtr(world);
}

static int BMX_CDECL stubWorldDestroy(void *world)
{
    ngLog(0x4ba9c0);
    ngLogPtr(world);
    return 1;
}

static void *BMX_CDECL stubWorldCreate(const float *box)
{
    ngLog(0x4bb080);
    for (int i = 0; i < 6; i++) {
        ngLogFloat(box[i]);
    }
    return gWorldAnswer;
}

static const float *BMX_CDECL stubWorldOrigin(void *world)
{
    ngLog(0x4ba050);
    ngLogPtr(world);
    return (const float *)gWorldOrigin;
}

static const float *BMX_CDECL stubWorldBBox(void *world)
{
    ngLog(0x4b9fe0);
    ngLogPtr(world);
    return (const float *)gWorldBounds;
}

static void BMX_CDECL stubWorldRender(void *world)
{
    ngLog(0x4ba460);
    ngLogPtr(world);
}

static uint32_t BMX_CDECL stubPvsState(void *world)
{
    ngLog(0x4cef40);
    ngLogPtr(world);
    return 0x5000 + (uint32_t)(uintptr_t)world;
}

static void BMX_CDECL stubSetPvsCallback(void *world, void *callback)
{
    ngLog(0x4cc0f0);
    ngLogPtr(world);
    ngLogPtr(callback);
}

static int BMX_CDECL stubPvsCreate(void *world, uint32_t a, uint32_t b, uint32_t c, uint32_t d, void *cb,
                                   uint32_t e)
{
    ngLog(0x4ce960);
    ngLogPtr(world);
    ngLog(a);
    ngLog(b);
    ngLog(c);
    ngLog(d);
    ngLogPtr(cb);
    ngLog(e);
    return gPvsCreateOk;
}

static int BMX_CDECL stubPvsRead(void *world, void *stream)
{
    ngLog(0x4c3ae0);
    ngLogPtr(world);
    ngLogPtr(stream);
    return gPvsReadOk;
}

static void *BMX_CDECL stubStreamOpen(int type, int access, void *data)
{
    ngLog(0x4e23e0);
    ngLog((uint32_t)type);
    ngLog((uint32_t)access);
    ngLogPtr(data);
    return (void *)(uintptr_t)gStreamAnswer;
}

static int BMX_CDECL stubStreamClose(void *stream, void *data)
{
    ngLog(0x4e2660);
    ngLogPtr(stream);
    ngLogPtr(data);
    return 1;
}

static uint32_t BMX_CDECL stubCamWidth(void *camera)
{
    ngLog(0x4de5a0);
    ngLogPtr(camera);
    return 640 + (uint32_t)(uintptr_t)camera % 7;
}

static uint32_t BMX_CDECL stubCamHeight(void *camera)
{
    ngLog(0x4de5d0);
    ngLogPtr(camera);
    return 480 + (uint32_t)(uintptr_t)camera % 5;
}

static uint8_t *BMX_CDECL stubCurrentCamera(void)
{
    ngLog(0x492eb0);
    return gCamera;
}

/* Pointers in the exe's data. */
static void BMX_CDECL stubSetBounds(int mode, const float *bounds, int flags)
{
    ngLog(0x48ab10);
    ngLog((uint32_t)mode);
    for (int i = 0; i < 3; i++) {
        ngLogFloat(bounds[i]);
        ngLogFloat(bounds[4 + i]);
    }
    ngLog((uint32_t)flags);
}

static void BMX_CDECL stubMaterialRelease(void *material)
{
    ngLog(0x489530);
    ngLogPtr(material);
}

static void BMX_CDECL stubTextureRelease(void *texture)
{
    ngLog(0x498810);
    ngLogPtr(texture);
}

static uint32_t BMX_CDECL stubTimeGetTime(void)
{
    ngLog(0x489cb0);
    return gTime;
}

static void *BMX_CDECL stubMaterialTexture(void *material, int index)
{
    ngLog(0x489a40);
    ngLogPtr(material);
    ngLog((uint32_t)index);
    return material;
}

static int BMX_CDECL stubTextureHash(void *texture)
{
    ngLog(0x4988b0);
    ngLogPtr(texture);
    return (int)(uintptr_t)texture & 0xff;
}

static void *BMX_CDECL stubReadCallback(const char *name, const char *mask)
{
    ngLog(0x4df000);
    ngLog(ngHash(name));
    ngLogPtr(mask);
    return gTextureAnswer;
}

static void fill_floats(void *p, uint32_t bytes)
{
    float *f = p;

    for (uint32_t i = 0; i < bytes / 4; i++) {
        f[i] = dt_float_edgy(-100.0f, 100.0f);
    }
}

static void env(const DtCtx *c)
{
    ngBegin(c, 1);
    gZdebugPrint = zrwPrint;
    dt_stub(0x4dfcb0, NULL, (void *)stubTextureRead);
    dt_stub(0x4ded90, NULL, (void *)stubErrorGet);
    dt_stub(0x4e6d90, NULL, (void *)stubBmpRead);
    dt_stub(0x5068a0, NULL, (void *)stubPngRead);
    dt_stub(0x4e3a60, NULL, (void *)stubImageWidth);
    dt_stub(0x4e3a70, NULL, (void *)stubImageHeight);
    dt_stub(0x4e3a80, NULL, (void *)stubImageDepth);
    dt_stub(0x4e3a90, NULL, (void *)stubImagePixels);
    dt_stub(0x4e3aa0, NULL, (void *)stubImagePalette);
    dt_stub(0x4e3960, NULL, (void *)stubImageDestroy);
    dt_stub(0x4dcb10, NULL, (void *)stubRasterCreate);
    dt_stub(0x4e7f70, NULL, (void *)stubSetFromImage);
    dt_stub(0x4dcc30, NULL, (void *)stubRasterWidth);
    dt_stub(0x4dcc40, NULL, (void *)stubRasterHeight);
    dt_stub(0x4dcc50, NULL, (void *)stubRasterStride);
    dt_stub(0x4dcc60, NULL, (void *)stubRasterDepth);
    dt_stub(0x4dcc70, NULL, (void *)stubRasterFormat);
    dt_stub(0x4dd160, NULL, (void *)stubLockPalette);
    dt_stub(0x4dd1d0, NULL, (void *)stubUnlockPalette);
    dt_stub(0x4e5330, NULL, (void *)stubPixelToRgba);
    dt_stub(0x4df650, NULL, (void *)stubTextureRaster);
    dt_stub(0x4df5f0, NULL, (void *)stubTextureSetRaster);
    dt_stub(0x4df910, NULL, (void *)stubTextureCreate);
    dt_stub(0x4df760, NULL, (void *)stubTextureDestroy);
    dt_stub(0x4df990, NULL, (void *)stubTextureGetName);
    dt_stub(0x4df9b0, NULL, (void *)stubTextureSetName);
    dt_stub(0x4dff70, NULL, (void *)stubSetFilter);
    dt_stub(0x4dffa0, NULL, (void *)stubSetAddressing);
    dt_stub(0x4dfb20, NULL, (void *)stubDictAdd);
    dt_stub(0x4df670, NULL, (void *)stubDictCreate);
    dt_stub(0x4df810, NULL, (void *)stubDictDestroy);
    dt_stub(0x4dfc80, NULL, (void *)stubDictSetCurrent);
    dt_stub(0x4e4a30, NULL, (void *)stubRegisterFormat);
    dt_stub(0x4df580, NULL, (void *)stubMipmapping);
    dt_stub(0x4df550, NULL, (void *)stubSetFind);
    dt_stub(0x4df540, NULL, (void *)stubGetReadCb);
    dt_stub(0x4df520, NULL, (void *)stubSetReadCb);
    dt_stub(0x4a3500, NULL, (void *)stubRegisterFs);
    dt_stub(0x4a3580, NULL, (void *)stubBufferSize);
    dt_stub(0x4a35c0, NULL, (void *)stubRamDiskClear);
    dt_stub(0x489ba0, NULL, (void *)stubLoadBegin);
    dt_stub(0x489be0, NULL, (void *)stubTextureLoaded);
    dt_stub(0x49abc0, NULL, (void *)stubChunkSize);
    dt_stub(0x49ac30, NULL, (void *)stubIffRead);
    dt_stub(0x483f10, NULL, (void *)stubDefaultDevice);
    dt_stub(0x483d30, NULL, (void *)stubDeviceControl);
    dt_stub(0x4cc640, NULL, (void *)stubPvsReady);
    dt_stub(0x4cc320, NULL, (void *)stubPvsUnload);
    dt_stub(0x4ba9c0, NULL, (void *)stubWorldDestroy);
    dt_stub(0x4bb080, NULL, (void *)stubWorldCreate);
    dt_stub(0x4ba050, NULL, (void *)stubWorldOrigin);
    dt_stub(0x4b9fe0, NULL, (void *)stubWorldBBox);
    dt_stub(0x4ba460, NULL, (void *)stubWorldRender);
    dt_stub(0x4cef40, NULL, (void *)stubPvsState);
    dt_stub(0x4cc0f0, NULL, (void *)stubSetPvsCallback);
    dt_stub(0x4ce960, NULL, (void *)stubPvsCreate);
    dt_stub(0x4c3ae0, NULL, (void *)stubPvsRead);
    dt_stub(0x4e23e0, NULL, (void *)stubStreamOpen);
    dt_stub(0x4e2660, NULL, (void *)stubStreamClose);
    dt_stub(0x4de5a0, NULL, (void *)stubCamWidth);
    dt_stub(0x4de5d0, NULL, (void *)stubCamHeight);
    dt_stub(0x492eb0, NULL, (void *)stubCurrentCamera);
    *(void **)(uintptr_t)0x561484 = (void *)stubSetBounds;
    *(void **)(uintptr_t)0x561024 = (void *)stubMaterialRelease;
    *(void **)(uintptr_t)0x562400 = (void *)stubTextureRelease;
    *(void **)(uintptr_t)0x561480 = (void *)stubTimeGetTime;
    gZrwMaterialTexture = stubMaterialTexture;
    gZrwTextureHash = stubTextureHash;

    /* answers */
    fill_floats(gRaster, sizeof gRaster);
    fill_floats(gWorldOrigin, sizeof gWorldOrigin);
    fill_floats(gWorldBounds, sizeof gWorldBounds);
    gRasterCalls = 0;
    gSetFromImageOk = (dt_u32() % 6) != 0;
    gTextureCreateOk = (dt_u32() % 6) != 0;
    gTextureAnswer = NULL;
    gImageAnswer = NULL;
    gPvsReady = (int)(dt_u32() & 1);
    gPvsCreateOk = (int)(dt_u32() & 1);
    gPvsReadOk = (int)(dt_u32() & 1);
    gLockOk = (dt_u32() % 6) != 0;
    gChunkSize = (uint32_t)dt_range(1, 200);
    gTime = dt_u32();
    gStreamAnswer = (dt_u32() % 5 == 0) ? 0 : 0x7000;
    gDeviceControlAnswer = (dt_u32() % 5 == 0) ? 0 : 0x6200;
    gDictAnswer = (dt_u32() % 5 == 0) ? NULL : (void *)0x6300;
    gWorldAnswer = (dt_u32() % 5 == 0) ? NULL : (void *)(uintptr_t)0x6400;
    gCamera = NULL;
    gPaletteBuffer = dt_alloc(0x420);
    dt_fill(gPaletteBuffer, 0x420);
    gZrwScale = dt_float(0.01f, 5.0f);
    gZrwInvScale = (dt_u32() & 1) ? 1.0f / gZrwScale : dt_float(0.01f, 5.0f);
    gZrwWorldCenter.x = ngRf();
    gZrwWorldCenter.y = ngRf();
    gZrwWorldCenter.z = ngRf();
    gZrwFlags = (uint8_t)dt_u32();
    gZrwWorld = (dt_u32() % 6 == 0) ? NULL : (void *)0x6500;
}

/* ---- fixture builders ----------------------------------------------------------------------------- */

static const uint32_t kFormats[] = {0x100, 0x200, 0x300, 0x400, 0x500,  0x600,  0x700,  0x800,
                                    0x900, 0xa00, 0xf00, 0,     0x2000, 0x4000, 0x8000, 0xff00};
static const uint32_t kPalFormats[] = {0x2100, 0x2200, 0x2300, 0x2400, 0x2500, 0x2600, 0x2700, 0x4800,
                                       0x2900, 0x4300, 0x2a00, 0x2000, 0x4000, 0x6800, 0x4100, 0x6000};

static void *make_raster(void)
{
    uint8_t *raster = dt_alloc(0x40);

    dt_fill(raster, 0x40);
    *(int *)(raster + kRasW) = dt_range(1, 512);
    *(int *)(raster + kRasH) = dt_range(1, 512);
    *(int *)(raster + kRasStride) = dt_range(1, 2048);
    *(int *)(raster + kRasDepth) = (dt_u32() % 8 == 0) ? dt_range(-4, 40) : 4 * dt_range(1, 8);
    *(uint32_t *)(raster + kRasFormat) =
        (dt_u32() % 3 == 0) ? kFormats[dt_u32() % 16] : kPalFormats[dt_u32() % 16];
    return raster;
}

/* A fake RwTexture: raster at +0, name from +8. */
static void *make_rwtexture(void)
{
    uint8_t *texture = dt_alloc(0x40);

    *(void **)texture = make_raster();
    for (int i = 0; i < 12; i++) {
        texture[8 + i] = (uint8_t)dt_range('a', 'z');
    }
    return texture;
}

static ZrwTexture *make_texture(void)
{
    ZrwTexture *tex = dt_alloc(sizeof *tex);

    dt_fill(tex, sizeof *tex);
    tex->refCount = dt_range(-1, 3);
    tex->depth = (dt_u32() % 6 == 0) ? dt_range(-2, 40) : dt_range(1, 9);
    tex->name = "tex-name";
    tex->texture = make_rwtexture();
    return tex;
}

static void *make_image(int depth)
{
    uint8_t *image = dt_alloc(0x40);
    const int w = dt_range(0, 6);
    const int h = dt_range(0, 5);
    const int stride = w * 4 + (dt_u32() % 3) * 4;
    uint8_t *pixels = dt_alloc((uint32_t)(stride * (h + 1) + 64));
    uint8_t *palette = dt_alloc(1024);

    *(int *)(image + kImgW) = w;
    *(int *)(image + kImgH) = h;
    *(int *)(image + kImgD) = depth;
    *(int *)(image + kImgStride) = stride;
    *(void **)(image + kImgPixels) = pixels;
    *(void **)(image + kImgPalette) = palette;
    dt_fill(pixels, (uint32_t)(stride * (h + 1) + 64));
    dt_fill(palette, 1024);
    /* sprinkle colour keys */
    for (int i = 0; i < w * h; i++) {
        if (dt_u32() % 3 == 0) {
            uint8_t *p = pixels + (i / (w ? w : 1)) * stride + (i % (w ? w : 1)) * 4;

            p[0] = gZrwTxr.key[0];
            p[1] = gZrwTxr.key[1];
            p[2] = gZrwTxr.key[2];
        }
    }
    for (int i = 0; i < 256; i++) {
        if (dt_u32() % 3 == 0) {
            palette[i * 4] = gZrwTxr.key[0];
            palette[i * 4 + 1] = gZrwTxr.key[1];
            palette[i * 4 + 2] = gZrwTxr.key[2];
        }
    }
    return image;
}

static int random_depth(void)
{
    static const int kDepths[] = {4, 8, 24, 32, 16, 1, 0};

    return kDepths[dt_u32() % 7];
}

static void begin(DtCtx *c)
{
    env(c);
    dt_zmem_env();
    gZrwTxr.key[0] = (dt_u32() % 3 == 0) ? 0xff : (uint8_t)dt_u32();
    gZrwTxr.key[1] = (dt_u32() % 3 == 0) ? 0x00 : (uint8_t)dt_u32();
    gZrwTxr.key[2] = (dt_u32() % 3 == 0) ? 0xff : (uint8_t)dt_u32();
    gZrwTxr.key[3] = 0;
    gZrwTxr.mipmapped = (int)(dt_u32() & 1);
    gZrwTxr.width = dt_range(0, 300);
    gZrwTxr.height = dt_range(0, 300);
    gZrwTxr.depth = dt_range(0, 32);
    gZrwTxr.loadName = "scene-texture";
    gZrwTxr.readCallback = stubReadCallback;
    gZrwTxr.loadBuffer = NULL;
    gZrwTxr.loadBufferSize = 0;
    gZrwTxr.dictionary = (void *)0x6600;
}

/* The texture free list, made by the original code. */
static void make_texture_list(void)
{
    typedef ZMemFreeList *(BMX_CDECL * CreateFn)(uint32_t, uint32_t, ZMemDestroyFn);

    gZrwTxr.textures = DT_ORIG(0x00485e80, CreateFn)(0x28, 0x100, DT_ORIG(0x00498810, ZMemDestroyFn));
}

static ZList *make_list(int count)
{
    ZList *list = dt_alloc(sizeof *list);

    list->items = (void **)dt_alloc(sizeof(void *) * 12);
    for (int i = 0; i < count; i++) {
        list->items[i] = (void *)(uintptr_t)(0x100 + dt_u32() % 7);
    }
    list->count = count;
    list->capacity = 12;
    list->cursor = dt_range(-1, count);
    return list;
}

/* ---- setups --------------------------------------------------------------------------------------- */

static void setup_world_reset(DtCtx *c)
{
    begin(c);
    gZrwWorldActive = (dt_u32() % 5) != 0;
    gZrwPvsLoaded = (int)(dt_u32() & 1);
    gZrwPvsEnabled = (int)(dt_u32() & 1);
    gZrwWorldMaterials = (dt_u32() % 5 == 0) ? NULL : make_list(dt_range(0, 5));
}

static void setup_noargs(DtCtx *c)
{
    begin(c);
    gZrwWorldMaterials = make_list(dt_range(0, 6));
    gZrwWorldActive = (int)(dt_u32() & 1);
}

static void setup_world_find(DtCtx *c)
{
    setup_noargs(c);
    DT_ARG_INT(c, 0, 0x100 + dt_u32() % 8);
}

static void setup_world_get(DtCtx *c)
{
    setup_noargs(c);
    DT_ARG_INT(c, 0, dt_range(-1, 8));
}

static void setup_pvs_setup(DtCtx *c)
{
    uint8_t *camera = dt_alloc(0x100);

    begin(c);
    dt_fill(camera, 0x100);
    *(void **)(camera + 0xc) = (void *)(uintptr_t)(0x7100 + dt_u32() % 64);
    gCamera = (dt_u32() % 5 == 0) ? NULL : camera;
    gZrwPvsLoaded = (int)(dt_u32() & 1);
    gZrwPvsEnabled = (int)(dt_u32() & 1);
}

static void setup_pvs_enable(DtCtx *c)
{
    begin(c);
    gZrwPvsLoaded = (int)(dt_u32() & 1);
    gZrwPvsEnabled = (int)(dt_u32() & 1);
    DT_ARG_INT(c, 0, dt_u32() % 3 == 0 ? 0 : dt_u32());
}

static void setup_particles(DtCtx *c)
{
    uint32_t *system = dt_alloc(0x40);
    ZList *list = dt_alloc(sizeof *list);
    const int count = dt_range(0, 5);
    float *params = dt_alloc(16);

    begin(c);
    list->items = (void **)dt_alloc(sizeof(void *) * 8);
    for (int i = 0; i < count; i++) {
        uint8_t *particle = dt_alloc(0x40);

        fill_floats(particle, 0x40);
        /* some particles sit above, some below the floor */
        *(float *)(particle + 8) = dt_float_edgy(-2.0f, 2.0f);
        list->items[i] = particle;
    }
    list->count = count;
    list->capacity = 8;
    list->cursor = -1;
    system[0] = (uint32_t)(uintptr_t)list;
    system[10] = dt_u32();
    gTime = system[10] + (uint32_t)dt_range(-5, 5000);
    params[0] = dt_float_edgy(-1.0f, 1.0f);
    params[1] = dt_float_edgy(0.0f, 1.5f);
    params[2] = dt_float_edgy(-20.0f, 20.0f);
    DT_ARG_INT(c, 0, dt_u32() % 5 == 0 ? 0 : 1);
    DT_ARG_PTR(c, 1, (dt_u32() % 8 == 0) ? NULL : (void *)system);
    DT_ARG_PTR(c, 2, (dt_u32() % 4 == 0) ? NULL : (void *)params);
}

static void setup_particles_accel(DtCtx *c)
{
    setup_particles(c);
    if (c->args[1] == 0) {
        /* the original reads the system's timestamp before looking at the pointer: keep it valid */
        DT_ARG_PTR(c, 1, dt_alloc(0x40));
        DT_ARG_INT(c, 0, 0); /* an empty system has no list to walk */
    }
}

static void setup_raster(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, (dt_u32() % 8 == 0) ? NULL : make_raster());
}

static void setup_color_key(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_image(random_depth()));
}

static void setup_image_read(DtCtx *c)
{
    begin(c);
    gImageAnswer = (dt_u32() % 4 == 0) ? NULL : make_image(random_depth());
    DT_ARG_PTR(c, 0, "image.bmp");
}

static void setup_dds_read(DtCtx *c)
{
    begin(c);
}

static void setup_plugin(DtCtx *c)
{
    uint8_t *material = dt_alloc(0x80);

    begin(c);
    dt_fill(material, 0x80);
    gZrwMatPluginOffset = (dt_u32() % 5 == 0) ? dt_range(-1, 0) : dt_range(1, 20) * 4;
    DT_ARG_PTR(c, 0, material);
}

static const int kTypes[] = {0, 1, 4};

static void setup_load_texture(DtCtx *c)
{
    const uint32_t k = dt_u32() % 10;

    begin(c);
    make_texture_list();
    gTextureAnswer = (dt_u32() % 3 == 0) ? NULL : make_rwtexture();
    gImageAnswer = (dt_u32() % 4 == 0) ? NULL : make_image(random_depth());
    gRasterAnswers[0] = (dt_u32() % 3 == 0) ? NULL : make_raster();
    gRasterAnswers[1] = (dt_u32() % 3 == 0) ? NULL : make_raster();
    memcpy(gNewTexture, make_rwtexture(), 0x40);
    gZrwTxr.mipmapped = (int)(dt_u32() & 1);
    DT_ARG_PTR(c, 0, k == 0 ? NULL : dt_alloc(64));
    DT_ARG_INT(c, 1, k == 1 ? 0 : dt_range(1, 400));
    DT_ARG_PTR(c, 2, "tex-name");
    DT_ARG_INT(c, 3, k == 2 ? dt_range(2, 6) : kTypes[dt_u32() % 3]);
}

static void setup_read_callback(DtCtx *c)
{
    begin(c);
    gTextureAnswer = (dt_u32() % 3 == 0) ? NULL : make_rwtexture();
    DT_ARG_PTR(c, 0, "name");
    DT_ARG_PTR(c, 1, (dt_u32() & 1) ? "mask" : NULL);
}

static void setup_read_chunk(DtCtx *c)
{
    setup_load_texture(c);
    gZrwTxr.loadBuffer = (dt_u32() % 3 == 0) ? dt_alloc(256) : NULL;
    gZrwTxr.loadBufferSize = (uint32_t)dt_range(0, 260);
    DT_ARG_PTR(c, 0, dt_alloc(16));
}

static void setup_tds(DtCtx *c)
{
    begin(c);
}

static void setup_palette_encode(DtCtx *c)
{
    uint8_t *rgba = dt_alloc(1024);
    const int index = dt_range(0, 255);

    begin(c);
    dt_fill(rgba, 1024);
    DT_ARG_PTR(c, 0, dt_alloc(2048));
    DT_ARG_INT(c, 1, (dt_u32() % 4 == 0) ? dt_u32() : kFormats[dt_u32() % 16] | (dt_u32() & 0xf0ff));
    DT_ARG_INT(c, 2, index);
    DT_ARG_PTR(c, 3, rgba);
}

static void setup_texture(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_texture());
}

static void setup_texture_log(DtCtx *c)
{
    setup_texture(c);
}

static void setup_init(DtCtx *c)
{
    begin(c);
    gZrwTxr.initialized = (int)(dt_u32() & 1);
    dt_stub(0x49aad0, NULL, (void *)stubSetupCallback);
    dt_stub(0x485e80, NULL, (void *)stubFreeListCreate);
}

static void setup_shutdown(DtCtx *c)
{
    begin(c);
    gZrwTxr.initialized = (dt_u32() % 4) != 0;
    gZrwTxr.textures = (ZMemFreeList *)(uintptr_t)0x4500;
    dt_stub(0x485fb0, NULL, (void *)stubFreeListDestroy);
}

static void setup_record(DtCtx *c)
{
    begin(c);
    make_texture_list();
}

static void setup_release(DtCtx *c)
{
    ZrwTexture *tex;

    begin(c);
    make_texture_list();
    tex = zmemFreeListAlloc(gZrwTxr.textures);
    memset(tex, 0, sizeof *tex);
    tex->refCount = dt_range(0, 3);
    tex->texture = make_rwtexture();
    DT_ARG_PTR(c, 0, tex);
}

static void setup_pal_read(DtCtx *c)
{
    ZrwTexture *tex;
    uint8_t *raster;

    begin(c);
    tex = make_texture();
    raster = *(uint8_t **)tex->texture;
    /* mostly paletted rasters */
    if (dt_u32() % 5 != 0) {
        *(uint32_t *)(raster + kRasFormat) = kPalFormats[dt_u32() % 16];
    }
    /* the palette buffer holds 256 entries: larger depths would run past it */
    tex->depth = (dt_u32() % 5 == 0) ? dt_range(0, 8) : (dt_u32() & 1 ? 4 : 8);
    DT_ARG_PTR(c, 0, tex);
}

/* ---- accessors and plugin glue (0x498ac0..0x498cb0) ---------------------------------------------- */

static void *BMX_CDECL stubTextureAddRef(void *texture)
{
    ngLog(0x4df980);
    ngLogPtr(texture);
    return texture;
}

static int BMX_CDECL stubRegisterPlugin(int size, uint32_t id, void *ctor, void *dtor, void *copy)
{
    static const int answers[] = {-1, 8, 16, 0x20};

    ngLog(0x4dffd0);
    ngLog((uint32_t)size);
    ngLog(id);
    ngLogPtr(ctor);
    ngLogPtr(dtor);
    ngLogPtr(copy);
    return answers[dt_u32() % 4];
}

static int BMX_CDECL stubTextureLogEach(const ZrwTexture *texture)
{
    ngLog(0x498580);
    ngLogPtr(texture);
    return 1;
}

static void BMX_CDECL stubIndent(int delta)
{
    ngLog(0x560078);
    ngLog((uint32_t)delta);
}

static void setup_tex_addref(DtCtx *c)
{
    setup_texture(c);
    dt_stub(0x4df980, NULL, (void *)stubTextureAddRef);
}

static void setup_register_plugin(DtCtx *c)
{
    begin(c);
    gZrwMatPluginOffset = dt_range(-1, 40);
    dt_stub(0x4dffd0, NULL, (void *)stubRegisterPlugin);
}

static void setup_load_name(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, dt_u32() % 4 == 0 ? NULL : "other-name");
}

/* The RwTexture of the record is big enough for the plugin slot to be anywhere up to 0x3c. */
static void setup_texture_user(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_texture());
    DT_ARG_INT(c, 1, dt_u32());
    gZrwMatPluginOffset = dt_u32() % 5 == 0 ? dt_range(-1, 0) : dt_range(1, 15) * 4;
}

static void setup_material_param(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_u32() % 3 == 0 ? dt_u32() : dt_range(0, 1));
}

static void setup_texture_log_all(DtCtx *c)
{
    typedef void *(BMX_CDECL * AllocFn)(ZMemFreeList *);
    const int count = dt_range(0, 5);

    begin(c);
    make_texture_list();
    for (int i = 0; i < count; i++) {
        uint8_t *payload = DT_ORIG(0x00486050, AllocFn)(gZrwTxr.textures);

        dt_fill(payload, 0x28);
    }
    gZdebugIndent = stubIndent;
    dt_stub(0x498580, NULL, (void *)stubTextureLogEach);
}

DT_CASES(zrwtxr_cases) = {
    {"zrwWorldReset", 0x00497200, zrwWorldReset, 0, DT_RET_INT, setup_world_reset, 1000, 0},
    {"zrwWorldFindMaterial", 0x00497400, zrwWorldFindMaterial, 1, DT_RET_INT, setup_world_find, 400, 0},
    {"zrwWorldMaterialCount", 0x00497450, zrwWorldMaterialCount, 0, DT_RET_INT, setup_noargs, 100, 0},
    {"zrwWorldGetMaterial", 0x00497460, zrwWorldGetMaterial, 1, DT_RET_INT, setup_world_get, 300, 0},
    {"zrwWorldHasPvs", 0x00497480, zrwWorldHasPvs, 0, DT_RET_INT, setup_noargs, 100, 0},
    {"zrwWorldSetupPvs", 0x004974a0, zrwWorldSetupPvs, 0, DT_RET_INT, setup_pvs_setup, 600, 0},
    {"zrwWorldSetPvsEnabled", 0x00497570, zrwWorldSetPvsEnabled, 1, DT_RET_INT, setup_pvs_enable, 300, 0},
    {"zrwWorldLoadPvs", 0x004975b0, zrwWorldLoadPvs, 0, DT_RET_INT, setup_noargs, 300, 0},
    {"zrwWorldRender", 0x00497620, zrwWorldRender, 0, DT_RET_INT, setup_noargs, 100, 0},
    {"zrwWorldNop", 0x00497640, zrwWorldNop, 0, DT_RET_VOID, setup_noargs, 20, 0},
    {"zrwParticleAccelerate", 0x00497650, zrwParticleAccelerate, 3, DT_RET_VOID, setup_particles_accel, 1500,
     0},
    {"zrwParticleNop", 0x004976f0, zrwParticleNop, 0, DT_RET_VOID, setup_noargs, 20, 0},
    {"zrwParticleFloorBounce", 0x00497700, zrwParticleFloorBounce, 3, DT_RET_VOID, setup_particles, 1500, 0},
    {"zrwRasterLog", 0x00497780, zrwRasterLog, 1, DT_RET_VOID, setup_raster, 600, 0},
    {"zrwImageApplyColorKey", 0x004978f0, zrwImageApplyColorKey, 1, DT_RET_VOID, setup_color_key, 800, 0},
    {"BMPImageRead", 0x004979e0, BMPImageRead, 1, DT_RET_INT, setup_image_read, 300, 0},
    {"PNGImageRead", 0x00497a30, PNGImageRead, 1, DT_RET_INT, setup_image_read, 300, 0},
    {"DDSImageRead", 0x00497a80, DDSImageRead, 0, DT_RET_INT, setup_dds_read, 20, 0},
    {"rwMatConstructor", 0x00497a90, rwMatConstructor, 1, DT_RET_VOID, setup_plugin, 200, 0},
    {"rwMatDestructor", 0x00497ab0, rwMatDestructor, 1, DT_RET_INT, setup_plugin, 20, 0},
    {"zrwMatGetUserData", 0x00497ae0, zrwMatGetUserData, 1, DT_RET_INT, setup_plugin, 200, 0},
    {"LoadTexture", 0x00497b00, LoadTexture, 4, DT_RET_INT, setup_load_texture, 3000, 0},
    {"zrwTextureReadCallback", 0x004981a0, zrwTextureReadCallback, 2, DT_RET_INT, setup_read_callback, 300,
     0},
    {"ReadTBMP", 0x004981f0, ReadTBMP, 1, DT_RET_INT, setup_read_chunk, 1500, 0},
    {"ReadTPNG", 0x004982a0, ReadTPNG, 1, DT_RET_INT, setup_read_chunk, 1500, 0},
    {"ReadTDDS", 0x00498320, ReadTDDS, 0, DT_RET_INT, setup_tds, 20, 0},
    {"zrwPaletteEncodeEntry", 0x004983f0, zrwPaletteEncodeEntry, 4, DT_RET_VOID, setup_palette_encode, 2000,
     0},
    {"zrwTextureLog", 0x00498580, zrwTextureLog, 1, DT_RET_INT, setup_texture_log, 300, 0},
    {"zrwTextureSystemInitialize", 0x00498660, zrwTextureSystemInitialize, 0, DT_RET_INT, setup_init, 300, 0},
    {"zrwTextureSystemShutdown", 0x00498790, zrwTextureSystemShutdown, 0, DT_RET_INT, setup_shutdown, 100, 0},
    {"zrwTextureCreateRecord", 0x004987e0, zrwTextureCreateRecord, 0, DT_RET_INT, setup_record, 100, 0},
    {"zrwTextureRelease", 0x00498810, zrwTextureRelease, 1, DT_RET_INT, setup_release, 400, 0},
    {"zrwTextureGetWidth", 0x00498870, zrwTextureGetWidth, 1, DT_RET_INT, setup_texture, 50, 0},
    {"zrwTextureGetHeight", 0x00498880, zrwTextureGetHeight, 1, DT_RET_INT, setup_texture, 50, 0},
    {"zrwTextureGetDepth", 0x00498890, zrwTextureGetDepth, 1, DT_RET_INT, setup_texture, 50, 0},
    {"zrwTextureGetName", 0x004988a0, zrwTextureGetName, 1, DT_RET_INT, setup_texture, 50, 0},
    {"zrwTextureGetNameHash", 0x004988b0, zrwTextureGetNameHash, 1, DT_RET_INT, setup_texture, 50, 0},
    {"zrwTextureReadPalette", 0x004988c0, zrwTextureReadPalette, 1, DT_RET_INT, setup_pal_read, 1500, 0},
    {"zrwTextureWritePalette", 0x00498a20, zrwTextureWritePalette, 1, DT_RET_INT, setup_pal_read, 1500, 0},
    {"zrwTextureGetRwTexture", 0x00498ac0, zrwTextureGetRwTexture, 1, DT_RET_INT, setup_texture, 50, 0},
    {"zrwTextureGetRaster", 0x00498ad0, zrwTextureGetRaster, 1, DT_RET_INT, setup_texture, 100, 0},
    {"zrwTextureAddRef", 0x00498af0, zrwTextureAddRef, 1, DT_RET_INT, setup_tex_addref, 100, 0},
    {"zrwMatRegisterPlugin", 0x00498b10, zrwMatRegisterPlugin, 0, DT_RET_INT, setup_register_plugin, 100, 0},
    {"zrwTextureSetLoadName", 0x00498b40, zrwTextureSetLoadName, 1, DT_RET_VOID, setup_load_name, 20, 0},
    {"zrwTextureSetUserData", 0x00498b50, zrwTextureSetUserData, 2, DT_RET_VOID, setup_texture_user, 300, 0},
    {"zrwSetMaterialParam", 0x00498b70, zrwSetMaterialParam, 1, DT_RET_VOID, setup_material_param, 100, 0},
    {"zrwSetMaterialValue", 0x00498b90, zrwSetMaterialValue, 1, DT_RET_VOID, setup_material_param, 20, 0},
    {"zrwTextureLogAll", 0x00498cb0, zrwTextureLogAll, 0, DT_RET_VOID, setup_texture_log_all, 100, 0},
};
