/*
 * Raster and camera standards of the SDL3 driver. The RwRaster fields RenderWare and the game read back
 * (type, flags, format, depth, stride, pixels) are set by the original's rules (0x4d5b80, 0x4d5780); the
 * pixels live in an SDL GPU texture (camera and z rasters) or in a CPU copy (D5).
 */
#include <string.h>

#include "driver/driver.h"
#include "rw/rwd3d.h"

/* The driver's camera plugin data (0x20 bytes, like the original's). */
typedef struct DrvCameraExt {
    uint8_t lastClear[4]; /* colour of the last clear */
    uint8_t pad04[0x1c];
} DrvCameraExt;

_Static_assert(sizeof(DrvCameraExt) == 0x20, "the original's camera plugin size");

/* The matrix globals of engine plugin 0x401 (rw/engine.c): +8 is RwMatrixMultiply's routine. */
#define rwMatrixGlobalsOffset ORIG_VAR(0x00676970, int32_t, rwMatrixGlobalsOffset)

/* Render order hint (the original's 0x566f20): 1 front to back, 2 back to front (used from step 3 on). */
int32_t drvRenderOrderHint = 1;

/* The camera between its begin and end update. */
static RwCamera *drvCurrentCamera;

int drvCameraUpdating(void)
{
    return drvCurrentCamera != NULL;
}

RwCamera *drvCameraCurrent(void)
{
    return drvCurrentCamera;
}

static DrvCameraExt *cameraExt(const RwCamera *camera)
{
    return (DrvCameraExt *)((uint8_t *)camera + drvCameraOffset);
}

/* 0x4d4ea0: camera plugin constructor. */
void *drvCameraCtor(void *camera, int32_t offset, int32_t size)
{
    DRV_TRACE("drvCameraCtor", (uint32_t)(uintptr_t)camera, (uint32_t)(uintptr_t)offset,
              (uint32_t)(uintptr_t)size);
    memset(cameraExt(camera), 0, sizeof(DrvCameraExt));
    return camera;
}

/* 0x4d4f80: camera plugin destructor. */
void *drvCameraDtor(void *camera, int32_t offset, int32_t size)
{
    DRV_TRACE("drvCameraDtor", (uint32_t)(uintptr_t)camera, (uint32_t)(uintptr_t)offset,
              (uint32_t)(uintptr_t)size);
    if (drvCurrentCamera == camera) {
        drvCurrentCamera = NULL;
    }
    return camera;
}

/* GPU texture behind a camera's frame or z raster (sub rasters share their parent's), created on first use.
 */
static struct SDL_GPUTexture *rasterTexture(RwRaster *raster)
{
    RwRaster *top;
    DrvRasterExt *ext;

    if (raster == NULL) {
        return NULL;
    }
    top = drvTopRaster(raster);
    ext = drvRasterExt(top);
    if (ext->texture == NULL && (top->type == rwRASTERTYPECAMERA || top->type == rwRASTERTYPEZBUFFER)) {
        ext->texture = drvGpuCreateTarget(top->width, top->height, top->type == rwRASTERTYPEZBUFFER ? -1 : 0);
    }
    return ext->texture;
}

/* The original's x87 state switch (_rwcseg 0x54b000 / 0x54b02e): set once per camera update. */
#define rwCameraFpuSet ORIG_VAR(0x006769a0, int32_t, rwCameraFpuSet)
/* The camera being updated, and the render target the device calls draw to (the primary surface's context).
 */
#define rwD3DCurrentCamera  ORIG_VAR(0x00697348, RwCamera *, rwD3DCurrentCamera)
#define rwD3DPrimaryContext ((uint8_t *)(uintptr_t)0x00697320)
#define rwD3DFogRange       ORIG_VAR(0x0069b98c, float, rwD3DFogRange)

static uint16_t savedFpuControl;

/* 0x54b000: 24-bit precision, rounding down, for everything inside the camera update. */
static void fpuCameraMode(void)
{
    uint16_t cw;
    __asm__ volatile("fnstcw %0" : "=m"(savedFpuControl));
    cw = (uint16_t)((savedFpuControl & 0xf0ff) | 0x0400);
    __asm__ volatile("fldcw %0" ::"m"(cw));
}

/* 0x54b02e */
static void fpuRestore(void)
{
    __asm__ volatile("fldcw %0" ::"m"(savedFpuControl));
}

/*
 * 0x4d45a0 (standard 1): RwCameraBeginUpdate's device part. Like the original it routes RwMatrixMultiply to
 * the core's x87 routine at 0x54b038 (_rwcseg), switches the x87 to the camera mode, makes the camera's
 * raster the target of the device calls and sets the vertex fog range (fog plane to far plane). The
 * exponential fog tables the original rebuilds when the far plane changes (fog types 2 and 3) are not built
 * yet.
 */
int drvCameraBeginUpdate(void *out, void *camera, int32_t in)
{
    DRV_TRACE("drvCameraBeginUpdate", (uint32_t)(uintptr_t)out, (uint32_t)(uintptr_t)camera,
              (uint32_t)(uintptr_t)in);
    RwCamera *const cam = camera;
    struct SDL_GPUTexture *color, *depth;
    RwRaster *top;
    uint32_t mxcsr;

    *(uint32_t *)((uint8_t *)RwEngineInstance + rwMatrixGlobalsOffset + 8) = 0x0054b038;
    if (rwCameraFpuSet == 0) {
        fpuCameraMode();
        rwCameraFpuSet = 1;
    }
    rwD3DCurrentCamera = cam;
    rwD3DContext = rwD3DPrimaryContext;
    rwD3DFogStart = rwD3DBits(cam->fogPlane);
    if (cam->farPlane != rwD3DFogEnd) {
        rwD3DFogEnd = cam->farPlane;
    }
    mxcsr = rwD3DRoundBegin();
    rwD3DFogScale = rwD3DFogRange / (cam->farPlane - cam->fogPlane);
    rwD3DRoundEnd(mxcsr);
    if (rwD3DState.fogType == 2 || rwD3DState.fogType == 3) {
        rwD3DFogStart = 0;
    }
    drvCurrentCamera = cam;
    color = rasterTexture(cam->frameBuffer);
    if (color == NULL) {
        return 0;
    }
    depth = cam->zBuffer != NULL ? rasterTexture(cam->zBuffer) : NULL;
    top = drvTopRaster(cam->frameBuffer);
    drvDrawSetTarget(color, depth, top->width, top->height, cam->frameBuffer->originX,
                     cam->frameBuffer->originY, cam->frameBuffer->width, cam->frameBuffer->height);
    return 1;
}

/* 0x4d4a80 (standard 10): draws the deferred translucent primitives, then the batch; the x87 back to normal.
 */
int drvCameraEndUpdate(void *out, void *camera, int32_t in)
{
    DRV_TRACE("drvCameraEndUpdate", (uint32_t)(uintptr_t)out, (uint32_t)(uintptr_t)camera,
              (uint32_t)(uintptr_t)in);
    rwD3DDeferFlush();
    if (rwCameraFpuSet != 0) {
        fpuRestore();
        rwCameraFpuSet = 0;
    }
    rwD3DCurrentCamera = NULL;
    drvDrawFlush();
    rwD3DContext = NULL;
    drvCurrentCamera = NULL;
    return 1;
}

/* 0x4d4c10 (standard 0x15): flags 1 clears the frame buffer to *color (RwRGBA), 2 the z buffer. */
int drvCameraClear(void *camera, void *color, int32_t flags)
{
    DRV_TRACE("drvCameraClear", (uint32_t)(uintptr_t)camera, *(const uint32_t *)color, (uint32_t)flags);
    RwCamera *const cam = camera;
    const uint8_t *rgba = color;
    struct SDL_GPUTexture *frame = rasterTexture(cam->frameBuffer);
    struct SDL_GPUTexture *depth = cam->zBuffer != NULL ? rasterTexture(cam->zBuffer) : NULL;

    if (frame == NULL) {
        return 0;
    }
    if ((flags & 1) != 0) {
        memcpy(cameraExt(cam)->lastClear, rgba, 4);
    }
    return drvGpuClear(frame, depth, rgba, flags & 3);
}

/* 0x4d4ae0 (standard 0x14): shows a camera raster in the window (flags bit 0: wait for vsync). */
int drvRasterShowRaster(void *raster, void *window, int32_t flags)
{
    DRV_TRACE("drvRasterShowRaster", (uint32_t)(uintptr_t)raster, (uint32_t)(uintptr_t)window,
              (uint32_t)(uintptr_t)flags);
    RwRaster *const r = raster;
    struct SDL_GPUTexture *texture = rasterTexture(r);

    if (texture == NULL) {
        return 0;
    }
    return drvGpuPresent(texture, r->originX, r->originY, r->width, r->height);
}

/* 0x4d4570 (standard 0x16): render order hint, in = 0 front to back. */
int drvHintRenderFront2Back(void *out, void *inOut, int32_t in)
{
    DRV_TRACE("drvHintRenderFront2Back", (uint32_t)(uintptr_t)out, (uint32_t)(uintptr_t)inOut,
              (uint32_t)(uintptr_t)in);
    drvRenderOrderHint = in == 0 ? 1 : 2;
    rwD3DRenderOrder = drvRenderOrderHint;
    return 1;
}

/* The raster context: the target of raster render and clear standards (used from step 2 on). */
RwRaster *drvContextRaster;

/* 0x4f94b0 (standard 11): inOut the raster that becomes the context; 0 when it has no pixels of its own or
 * through its parent (the original then reads past the pseudo raster at the bottom of RW's context stack and
 * decides by whatever lies there). */
int drvSetRasterContext(void *out, void *raster, int32_t in)
{
    RwRaster *const r = raster;

    DRV_TRACE("drvSetRasterContext", (uint32_t)(uintptr_t)out, (uint32_t)(uintptr_t)raster, (uint32_t)in);
    if ((drvTopRaster(r)->flags & rwRASTERDONTALLOCATE) != 0) {
        return 0;
    }
    drvContextRaster = r;
    return 1;
}

/* ---- raster create / destroy ------------------------------------------------------------------------ */

/* Texture format for a raster format and depth (0x4d5780's texture branch); 0 when there is none. */
static int32_t textureFormat(int32_t format, int32_t *depth)
{
    static const struct {
        int32_t format;
        int32_t depth;
    } kFormats[] = {
        {kDrvFormat1555, 16}, {kDrvFormat565, 16}, {kDrvFormat4444, 16}, {kDrvFormat8888, 32},
        {kDrvFormat888, 32},  {kDrvFormatPal8, 8}, {kDrvFormatPal4, 4},
    };
    int32_t pixel = format & (kDrvFormatPixelMask | kDrvFormatPal8 | kDrvFormatPal4);

    if ((format & kDrvFormatPixelMask) == 0) {
        switch (format & (kDrvFormatPal8 | kDrvFormatPal4)) {
        case 0:
            pixel = *depth == 4                   ? kDrvFormatPal4
                    : *depth == 8                 ? kDrvFormatPal8
                    : *depth == 24                ? kDrvFormat888
                    : *depth == 32                ? kDrvFormat8888
                    : *depth == 0 || *depth == 16 ? kDrvFormat565
                                                  : 0;
            break;
        case kDrvFormatPal4:
            pixel = *depth == 0 || *depth == 4 ? kDrvFormatPal4 : 0;
            break;
        case kDrvFormatPal8:
            pixel = *depth == 0 || *depth == 8 ? kDrvFormatPal8 : 0;
            break;
        default:
            pixel = 0;
            break;
        }
        if (pixel == 0) {
            return -1;
        }
    }
    for (size_t i = 0; i < sizeof kFormats / sizeof kFormats[0]; i++) {
        if (kFormats[i].format == pixel) {
            if (*depth == 0) {
                *depth = kFormats[i].depth;
            } else if (*depth != kFormats[i].depth) {
                return -2;
            }
            return pixel;
        }
    }
    return -1;
}

/*
 * 0x4d5780: type and flags from the create flags, then the format and depth checks. Camera and normal
 * rasters take the display's format, z rasters the z buffer's, textures one of the texture formats.
 */
static int setRasterFormat(RwRaster *raster, int32_t flags)
{
    const int32_t type = flags & 7;
    const int32_t mip = flags & (kDrvFormatMipmap | kDrvFormatAutoMipmap);
    int32_t format = flags & 0x6f00;
    int32_t wantFormat, wantDepth;

    raster->type = (uint8_t)type;
    raster->flags = (uint8_t)(flags & 0xf8);
    if (type == rwRASTERTYPETEXTURE) {
        format = textureFormat(flags, &raster->depth);
        if (format < 0) {
            drvSetError(_rwerror(format == -2 ? kDrvErrInvalidRasterDepth : kDrvErrInvalidRasterFormat));
            return 0;
        }
        raster->format = (uint8_t)((format | mip) >> 8);
        return 1;
    }
    if (type == rwRASTERTYPENORMAL || type == rwRASTERTYPECAMERA) {
        wantFormat = drvState.cameraFormat;
        wantDepth = drvState.cameraDepth;
    } else if (type == rwRASTERTYPEZBUFFER) {
        wantFormat = drvState.zFormat;
        wantDepth = drvState.zDepth;
    } else {
        drvSetError(_rwerror(kDrvErrInvalidRasterFormat));
        return 0;
    }
    if (mip != 0 || (format != 0 && format != wantFormat)) {
        drvSetError(_rwerror(kDrvErrInvalidRasterFormat));
        return 0;
    }
    if (raster->depth == 0) {
        raster->depth = wantDepth;
    } else if (raster->depth != wantDepth) {
        drvSetError(_rwerror(kDrvErrInvalidRasterDepth));
        return 0;
    }
    raster->format = (uint8_t)(wantFormat >> 8);
    return 1;
}

/*
 * 0x4d5b80 (standard 4): out unused, inOut the raster (width, height, depth set by RwRasterCreate), in the
 * create flags. A raster without size gets rwRASTERDONTALLOCATE. Camera and z rasters get a GPU target on
 * first use; their stride is the packed row size (the original's was the DirectDraw surface pitch). Textures
 * keep stride 0 until they are locked, like the original's.
 */
int drvRasterCreate(void *out, void *raster, int32_t flags)
{
    DRV_TRACE("drvRasterCreate", (uint32_t)(uintptr_t)out, (uint32_t)(uintptr_t)raster,
              (uint32_t)(uintptr_t)flags);
    RwRaster *const r = raster;

    r->pixels = NULL;
    memset(drvRasterExt(r), 0, sizeof(DrvRasterExt));
    if (!setRasterFormat(r, flags)) {
        return 0;
    }
    if (r->width == 0 || r->height == 0) {
        r->flags = rwRASTERDONTALLOCATE;
        r->stride = 0;
        if (r->depth == 0) {
            r->depth = 16;
        }
        return 1;
    }
    if ((r->flags & rwRASTERDONTALLOCATE) != 0) {
        return 1;
    }
    r->stride = r->type == rwRASTERTYPETEXTURE ? 0 : r->width * (r->depth / 8);
    if (r->type == rwRASTERTYPETEXTURE) {
        /* What the render states read: the texture handle and whether the format has alpha. */
        static const RwD3DRasterInfo kOpaque = {{0}, 0}, kAlpha = {{0}, 1};
        const int32_t format = (r->format << 8) & kDrvFormatPixelMask;
        DrvRasterExt *const ext = drvRasterExt(r);
        ext->handle = r;
        ext->info = format == kDrvFormat1555 || format == kDrvFormat4444 || format == kDrvFormat8888
                        ? &kAlpha
                        : &kOpaque;
        ext->dirty = 1;
    }
    return 1;
}

/* 0x4d5660 (standard 5). */
int drvRasterDestroy(void *out, void *raster, int32_t in)
{
    DRV_TRACE("drvRasterDestroy", (uint32_t)(uintptr_t)out, (uint32_t)(uintptr_t)raster,
              (uint32_t)(uintptr_t)in);
    RwRaster *const r = raster;
    DrvRasterExt *const ext = drvRasterExt(r);

    if (rwD3DState.textureRaster == r) {
        rwD3DState.textureRaster = NULL;
    }
    drvDrawForgetRaster(r);
    if (drvContextRaster == r) {
        drvContextRaster = NULL;
    }
    if ((r->flags & rwRASTERDONTALLOCATE) == 0) {
        drvGpuReleaseTexture(ext->texture);
        ext->texture = NULL;
        if (ext->cpu != NULL) {
            RwEngineInstance->memoryFuncs.rwfree(ext->cpu);
            ext->cpu = NULL;
        }
    }
    if (r->palette != NULL) {
        RwEngineInstance->memoryFuncs.rwfree(r->palette);
        r->palette = NULL;
    }
    r->height = 0;
    r->width = 0;
    r->stride = 0;
    r->type = 0;
    return 1;
}
