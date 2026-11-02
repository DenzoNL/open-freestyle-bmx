/*
 * Raster pixels and images: lock / unlock of the CPU copy (D5), the texture format choice and image upload
 * of the SDL3 driver (standards 7, 8, 9, 15, 16).
 */
#include <string.h>

#include "driver/driver.h"

/* Row size of the CPU copy: whole bytes, DWORD aligned like a DirectDraw surface's pitch. */
static int32_t cpuStride(const RwRaster *raster)
{
    return ((raster->width * raster->depth + 7) / 8 + 3) & ~3;
}

/* The top level parent that owns the pixels of a (sub) raster. RW's context stack bottom is a pseudo raster
 * with no parent (_rwRasterOpen). */
RwRaster *drvTopRaster(RwRaster *raster)
{
    while (raster->parent != NULL && raster->parent != raster) {
        raster = raster->parent;
    }
    return raster;
}

/* The CPU copy of a raster's top level parent, allocated (zeroed) on first use. NULL for z rasters. */
static uint8_t *rasterCpu(RwRaster *raster)
{
    RwRaster *const top = drvTopRaster(raster);
    DrvRasterExt *const ext = drvRasterExt(top);

    if (ext->cpu == NULL && top->type != rwRASTERTYPEZBUFFER && (top->flags & rwRASTERDONTALLOCATE) == 0) {
        const uint32_t size = (uint32_t)(cpuStride(top) * top->height);

        ext->cpu = RwEngineInstance->memoryFuncs.rwmalloc(size);
        if (ext->cpu != NULL) {
            memset(ext->cpu, 0, size);
            ext->cpuSize = size;
        }
    }
    return ext->cpu;
}

/*
 * 0x4d5150 (standard 15): out receives the pixels, in = lock mode (1 write, 2 read) | mip level << 8. The
 * raster's width, height and stride become the level's (saved behind the public fields); pixels points at
 * the raster's origin inside its parent. Only level 0 exists (the driver offers no mipmapped formats).
 */
int drvRasterLock(void *out, void *raster, int32_t in)
{
    DRV_TRACE("drvRasterLock", (uint32_t)(uintptr_t)out, (uint32_t)(uintptr_t)raster,
              (uint32_t)(uintptr_t)in);
    RwRaster *const r = raster;
    uint8_t *const cpu = rasterCpu(r);
    const int32_t level = (in >> 8) & 0xff;
    int32_t stride;

    if (cpu == NULL) {
        return 0;
    }
    if (level != 0) {
        drvSetError(_rwerror(kDrvErrDevice, "Failed to lock D3D surface"));
        return 0;
    }
    stride = cpuStride(drvTopRaster(r));
    r->originalStride = r->stride;
    r->originalHeight = r->height;
    r->originalWidth = r->width;
    r->pixels = cpu + (r->depth + 7) / 8 * r->originX + r->originY * stride;
    r->stride = stride;
    *(uint8_t **)out = r->pixels;
    drvRasterExt(r)->lockFlags = (uint8_t)level;
    if ((in & 2) != 0) {
        r->privateFlags |= 2;
    }
    if ((in & 1) != 0) {
        r->privateFlags |= 4;
    }
    return 1;
}

/* 0x4d5300 (standard 16): restores the level 0 size and stride. */
int drvRasterUnlock(void *out, void *raster, int32_t in)
{
    DRV_TRACE("drvRasterUnlock", (uint32_t)(uintptr_t)out, (uint32_t)(uintptr_t)raster,
              (uint32_t)(uintptr_t)in);
    RwRaster *const r = raster;

    if (rasterCpu(r) == NULL) {
        return 0;
    }
    if ((r->privateFlags & 4) != 0) {
        drvDrawTextureChanged(r);
    }
    r->width = r->originalWidth;
    r->stride = r->originalStride;
    r->height = r->originalHeight;
    r->pixels = NULL;
    r->privateFlags &= 0xf9;
    return 1;
}

/* 0x4f9ea0: 1 opaque, 2 alpha only near 0 or 255 (one bit is enough), 3 real alpha. */
static int imageAlphaClass(const RwImage *image)
{
    const int32_t width = RwImageGetWidth(image);
    const int32_t height = RwImageGetHeight(image);
    const int32_t depth = RwImageGetDepth(image);
    const uint8_t *row = image->pixels;
    int seen = 0;

    if (depth != 4 && depth != 8 && depth != 32) {
        return 1;
    }
    for (int32_t y = 0; y < height; y++, row += image->stride) {
        for (int32_t x = 0; x < width; x++) {
            uint8_t a;

            if (depth == 32) {
                a = row[x * 4 + 3];
                if (a == 0xff) {
                    continue;
                }
            } else {
                a = image->palette[row[x] * 4 + 3];
                if (a >= 0xf0) {
                    continue;
                }
            }
            seen = 1;
            if (a > 0xf) {
                return 3;
            }
        }
    }
    return seen ? 2 : 1;
}

/*
 * 0x4f83a0 (standard 9): out the raster to fill in, inOut the image, in the create flags. A texture gets the
 * image's size (within the device limits) and a 16 bit format by its alpha: 565 opaque, 1555 one bit alpha,
 * 4444 alpha (the original's choice on a D3D7 device without paletted textures). No mipmap flags and no
 * power of two sizes: the driver reports no mipmap filtering and no size restriction.
 */
int drvImageFindRasterFormat(void *out, void *image, int32_t flags)
{
    DRV_TRACE("drvImageFindRasterFormat", (uint32_t)(uintptr_t)out, (uint32_t)(uintptr_t)image,
              (uint32_t)(uintptr_t)flags);
    static const int32_t kByAlpha[4] = {0, kDrvFormat565, kDrvFormat1555, kDrvFormat4444};
    RwRaster *const r = out;
    const RwImage *const img = image;
    const int32_t type = flags & 7;

    r->width = img->width;
    r->height = img->height;
    r->depth = 0;
    r->type = (uint8_t)type;
    r->flags = (uint8_t)(flags & 0xf8);
    if (type == rwRASTERTYPETEXTURE) {
        r->format = (uint8_t)(kByAlpha[imageAlphaClass(img)] >> 8);
        r->width = r->width < 1 ? 1 : r->width > kDrvMaxTextureSize ? kDrvMaxTextureSize : r->width;
        r->height = r->height < 1 ? 1 : r->height > kDrvMaxTextureSize ? kDrvMaxTextureSize : r->height;
    } else if (type != rwRASTERTYPENORMAL && type != rwRASTERTYPEZBUFFER && type != rwRASTERTYPECAMERA) {
        drvSetError(_rwerror(kDrvErrInvalidRasterFormat));
        return 0;
    }
    return 1;
}

/* The top bits of an 8 bit channel in a field of n bits at shift s (truncation, like 0x4eefc0). */
static uint32_t channel(uint8_t value, int bits, int shift)
{
    return (uint32_t)(value >> (8 - bits)) << shift;
}

/* One RGBA pixel in the raster's format. Without an alpha channel, pixels under half alpha become black. */
static uint32_t packPixel(const uint8_t *rgba, int32_t format)
{
    const int keyed = rgba[3] < 0x80;

    switch (format) {
    case kDrvFormat565:
        return keyed ? 0 : channel(rgba[0], 5, 11) | channel(rgba[1], 6, 5) | channel(rgba[2], 5, 0);
    case kDrvFormat1555:
        return channel(rgba[3], 1, 15) | channel(rgba[0], 5, 10) | channel(rgba[1], 5, 5) |
               channel(rgba[2], 5, 0);
    case kDrvFormat4444:
        return channel(rgba[3], 4, 12) | channel(rgba[0], 4, 8) | channel(rgba[1], 4, 4) |
               channel(rgba[2], 4, 0);
    case kDrvFormat888:
        return keyed ? 0 : (uint32_t)rgba[0] << 16 | (uint32_t)rgba[1] << 8 | rgba[2];
    default:
        return (uint32_t)rgba[3] << 24 | (uint32_t)rgba[0] << 16 | (uint32_t)rgba[1] << 8 | rgba[2];
    }
}

static void storePixel(uint8_t *dst, int32_t depth, uint32_t pixel)
{
    for (int32_t i = 0; i < depth / 8; i++) {
        dst[i] = (uint8_t)(pixel >> (8 * i));
    }
}

/*
 * 0x4f79a0 (standard 7): out the raster, inOut the image (4/8 bit through its palette, or 32 bit). Writes the
 * pixels in the raster's format, locking the raster for writing unless it already is.
 */
int drvRasterSetFromImage(void *raster, void *image, int32_t in)
{
    DRV_TRACE("drvRasterSetFromImage", (uint32_t)(uintptr_t)raster, (uint32_t)(uintptr_t)image,
              (uint32_t)(uintptr_t)in);
    RwRaster *const r = raster;
    const RwImage *const img = image;
    const int wasLocked = (r->privateFlags & 4) != 0;
    const int32_t format = (r->format << 8) & kDrvFormatPixelMask;
    const int depthOk = (r->depth == 16 || r->depth == 24 || r->depth == 32) &&
                        (img->depth == 4 || img->depth == 8 || img->depth == 32);
    const uint8_t *src = img->pixels;
    uint8_t *dst;

    if ((r->flags & rwRASTERDONTALLOCATE) != 0) {
        return 0;
    }
    if (!wasLocked && RwRasterLock(r, 0, 1) == NULL) {
        return 0;
    }
    dst = r->pixels;
    for (int32_t y = 0; depthOk && dst != NULL && y < img->height && y < r->height; y++) {
        for (int32_t x = 0; x < img->width && x < r->width; x++) {
            const uint8_t *rgba = img->depth == 32 ? src + x * 4 : img->palette + src[x] * 4;

            storePixel(dst + x * (r->depth / 8), r->depth, packPixel(rgba, format));
        }
        src += img->stride;
        dst += r->stride;
    }
    if (!wasLocked) {
        RwRasterUnlock(r);
    }
    return depthOk;
}

/* 0x4d6490 (standard 8): out the texture, inOut the raster. */
int drvTextureSetRaster(void *texture, void *raster, int32_t in)
{
    DRV_TRACE("drvTextureSetRaster", (uint32_t)(uintptr_t)texture, (uint32_t)(uintptr_t)raster,
              (uint32_t)(uintptr_t)in);
    ((RwTexture *)texture)->raster = raster;
    return 1;
}

/*
 * 0x4d64b0 (standard 12): out the sub raster (rwRASTERDONTALLOCATE, size and origin set by RW), inOut the
 * raster it is cut from. It takes the parent's stride, depth, type and format; its pixels stay those of its
 * top level parent (the GPU target and the CPU copy are found through RwRaster.parent).
 */
int drvRasterSubRaster(void *subRaster, void *raster, int32_t in)
{
    DRV_TRACE("drvRasterSubRaster", (uint32_t)(uintptr_t)subRaster, (uint32_t)(uintptr_t)raster,
              (uint32_t)(uintptr_t)in);
    RwRaster *const sub = subRaster;
    const RwRaster *const parent = raster;

    sub->stride = parent->stride;
    sub->depth = parent->depth;
    sub->type = parent->type;
    sub->pixels = NULL;
    sub->format = parent->format;
    return 1;
}
