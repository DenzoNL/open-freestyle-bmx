/*
 * The movies' picture on the SDL3 driver (docs/PLATFORM.md P13, slice S9): MSVFW32 DrawDibOpen / DrawDibDraw
 * / DrawDibClose and USER32 GetDC / ReleaseDC / GetClientRect, which only zge/zmovie.c calls (zmoviePlay,
 * zmovieUpdate, zmovieDraw). GetDC hands out a token DC that only DrawDibDraw and ReleaseDC take. DrawDibDraw
 * uploads the frame (a 24-bit bottom-up DIB from AVIStreamGetFrame) into a texture and blits it, nearest
 * filtered, at the requested rectangle into a target of the presented size, which is then presented like a
 * camera raster (letterboxed into the window, D4) but not counted as a shown frame.
 *
 * The presented size is the mode's in full screen (what the original's client rectangle was under D3D7's
 * exclusive mode) and the window's client size when windowed; GetClientRect reports it, so zmovieUpdate's
 * rectangle covers the target.
 *
 * Dev aid: BMX_MOVIE_SHOT=<file.bmp> (a `%d` takes the draw number) with BMX_MOVIE_SHOT_FRAME=<n>[,<m>...]:
 * save the target after the n-th DrawDibDraw of the process (GPU readback), logging the movie frame it
 * showed.
 */
#include <SDL3/SDL.h>
#include <string.h>

#include "driver/driver.h"
#include "platform/platform.h"
#include "platform/standins.h"

enum { kDibHeader = 40 };

/* BITMAPINFOHEADER */
typedef struct DibHeader {
    uint32_t size;
    int32_t width, height;
    uint16_t planes, bitCount;
    uint32_t compression, sizeImage;
    int32_t xPelsPerMeter, yPelsPerMeter;
    uint32_t clrUsed, clrImportant;
} DibHeader;

_Static_assert(sizeof(DibHeader) == kDibHeader, "BITMAPINFOHEADER");

static const uint32_t kTokenDc = 0x0bad0dc1; /* the DC GetDC hands out */
static uint32_t gDrawDibs;                   /* DrawDibOpen handles given out (1, 2, ...) */
static SDL_GPUTexture *gFrame;               /* the DIB, RGBA */
static int32_t gFrameWidth, gFrameHeight;
static SDL_GPUTexture *gTarget; /* what is presented */
static int32_t gTargetWidth, gTargetHeight;
static uint8_t *gRgba;
static int gDraws;
static int gWarned;

/* The size the driver presents (see above), 640 x 480 before the device exists. */
static void presentedSize(int32_t *width, int32_t *height)
{
    *width = 640;
    *height = 480;
    if (drvState.currentMode >= 0) {
        const DrvVideoMode *const mode = &drvState.modes[drvState.currentMode];

        if ((mode->flags & kDrvModeExclusive) != 0) {
            *width = mode->width;
            *height = mode->height;
            return;
        }
        drvGpuWindowSize(width, height);
        if (*width <= 0 || *height <= 0) {
            *width = 640;
            *height = 480;
        }
    }
}

void drvMovieRelease(void)
{
    SDL_GPUDevice *const device = drvGpuDevice();

    if (device != NULL) {
        if (gFrame != NULL) {
            SDL_ReleaseGPUTexture(device, gFrame);
        }
        if (gTarget != NULL) {
            SDL_ReleaseGPUTexture(device, gTarget);
        }
    }
    gFrame = gTarget = NULL;
    gFrameWidth = gFrameHeight = gTargetWidth = gTargetHeight = 0;
}

void *BMX_WINAPI bmxStandGetDC(void *window)
{
    (void)window;
    return (void *)(uintptr_t)kTokenDc;
}

int32_t BMX_WINAPI bmxStandReleaseDC(void *window, void *dc)
{
    (void)window;
    return dc == (void *)(uintptr_t)kTokenDc;
}

int32_t BMX_WINAPI bmxStandGetClientRect(void *window, int32_t *rect)
{
    int32_t width, height;

    (void)window;
    if (rect == NULL) {
        return 0;
    }
    presentedSize(&width, &height);
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
    return 1;
}

void *BMX_WINAPI bmxStandDrawDibOpen(void)
{
    return (void *)(uintptr_t)++gDrawDibs;
}

int32_t BMX_WINAPI bmxStandDrawDibClose(void *drawDib)
{
    (void)drawDib;
    return 1;
}

static SDL_GPUTexture *texture(SDL_GPUDevice *device, int32_t width, int32_t height,
                               SDL_GPUTextureUsageFlags usage)
{
    SDL_GPUTextureCreateInfo info;

    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.usage = usage;
    info.width = (Uint32)width;
    info.height = (Uint32)height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    return SDL_CreateGPUTexture(device, &info);
}

/* The frame texture and the target, (re)made when a size changes; a new target starts black. */
static int prepare(SDL_GPUDevice *device, int32_t frameWidth, int32_t frameHeight)
{
    int32_t width, height;

    if (gFrame == NULL || gFrameWidth != frameWidth || gFrameHeight != frameHeight) {
        if (gFrame != NULL) {
            SDL_ReleaseGPUTexture(device, gFrame);
        }
        SDL_free(gRgba);
        gFrame = texture(device, frameWidth, frameHeight, SDL_GPU_TEXTUREUSAGE_SAMPLER);
        gRgba = SDL_malloc((size_t)frameWidth * (size_t)frameHeight * 4);
        gFrameWidth = frameWidth;
        gFrameHeight = frameHeight;
    }
    presentedSize(&width, &height);
    if (gTarget == NULL || gTargetWidth != width || gTargetHeight != height) {
        if (gTarget != NULL) {
            SDL_ReleaseGPUTexture(device, gTarget);
        }
        gTarget = drvGpuCreateTarget(width, height, 32);
        gTargetWidth = width;
        gTargetHeight = height;
        if (gTarget != NULL) {
            static const uint8_t kBlack[4] = {0, 0, 0, 255};

            drvGpuClear(gTarget, NULL, kBlack, 1);
        }
    }
    return gFrame != NULL && gRgba != NULL && gTarget != NULL;
}

/* Whether this draw (counted from 1) is in BMX_MOVIE_SHOT_FRAME's list. */
static int isShotDraw(int draw)
{
    const char *list = SDL_getenv("BMX_MOVIE_SHOT_FRAME");

    while (list != NULL && *list != 0) {
        char *end;
        const long n = SDL_strtol(list, &end, 10);

        if (end == list) {
            break;
        }
        if (n == draw) {
            return 1;
        }
        list = *end == ',' ? end + 1 : end;
    }
    return 0;
}

/* Uploads the DIB rows (bottom-up B, G, R) top-down as RGBA and blits [src] into [dst] of the target. */
static int drawFrame(SDL_GPUDevice *device, const DibHeader *h, const uint8_t *bits, const int32_t src[4],
                     const int32_t dst[4])
{
    const int32_t stride = ((h->width * 3) + 3) & ~3;
    const uint32_t size = (uint32_t)(h->width * h->height * 4);
    SDL_GPUTransferBufferCreateInfo info;
    SDL_GPUTransferBuffer *buffer;
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUTextureTransferInfo from;
    SDL_GPUTextureRegion region;
    SDL_GPUBlitInfo blit;
    uint8_t *mapped;

    for (int32_t y = 0; y < h->height; y++) {
        const uint8_t *const row = bits + (size_t)(h->height - 1 - y) * (size_t)stride;
        uint8_t *const out = gRgba + (size_t)y * (size_t)h->width * 4;

        for (int32_t x = 0; x < h->width; x++) {
            out[4 * x + 0] = row[3 * x + 2];
            out[4 * x + 1] = row[3 * x + 1];
            out[4 * x + 2] = row[3 * x + 0];
            out[4 * x + 3] = 255;
        }
    }
    SDL_zero(info);
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    info.size = size;
    buffer = SDL_CreateGPUTransferBuffer(device, &info);
    mapped = buffer != NULL ? SDL_MapGPUTransferBuffer(device, buffer, false) : NULL;
    if (mapped == NULL) {
        SDL_ReleaseGPUTransferBuffer(device, buffer);
        return 0;
    }
    memcpy(mapped, gRgba, size);
    SDL_UnmapGPUTransferBuffer(device, buffer);
    drvDrawFlush(); /* keep the order with the driver's own batches */
    cmd = SDL_AcquireGPUCommandBuffer(device);
    if (cmd == NULL) {
        SDL_ReleaseGPUTransferBuffer(device, buffer);
        return 0;
    }
    copy = SDL_BeginGPUCopyPass(cmd);
    SDL_zero(from);
    from.transfer_buffer = buffer;
    SDL_zero(region);
    region.texture = gFrame;
    region.w = (Uint32)h->width;
    region.h = (Uint32)h->height;
    region.d = 1;
    SDL_UploadToGPUTexture(copy, &from, &region, false);
    SDL_EndGPUCopyPass(copy);
    SDL_zero(blit);
    blit.source.texture = gFrame;
    blit.source.x = (Uint32)src[0];
    blit.source.y = (Uint32)src[1];
    blit.source.w = (Uint32)src[2];
    blit.source.h = (Uint32)src[3];
    blit.destination.texture = gTarget;
    blit.destination.x = (Uint32)dst[0];
    blit.destination.y = (Uint32)dst[1];
    blit.destination.w = (Uint32)dst[2];
    blit.destination.h = (Uint32)dst[3];
    blit.load_op = SDL_GPU_LOADOP_LOAD;
    blit.filter = SDL_GPU_FILTER_NEAREST;
    SDL_BlitGPUTexture(cmd, &blit);
    SDL_ReleaseGPUTransferBuffer(device, buffer);
    return SDL_SubmitGPUCommandBuffer(cmd);
}

/*
 * The DIB: `bits` NULL means right after the header. Source width / height -1 = the whole DIB (zmovieDraw's
 * call). Only what the movies need: 24-bit uncompressed bottom-up DIBs, the token DC; anything else fails
 * (logged once). The destination is clipped to the target. Flags (DDF_BACKGROUND for other video streams) are
 * ignored.
 */
int32_t BMX_WINAPI bmxStandDrawDibDraw(void *drawDib, void *dc, int32_t x, int32_t y, int32_t width,
                                       int32_t height, void *bitmapInfo, void *bits, int32_t srcX,
                                       int32_t srcY, int32_t srcWidth, int32_t srcHeight, uint32_t flags)
{
    const DibHeader *const h = bitmapInfo;
    SDL_GPUDevice *const device = drvGpuDevice();
    int32_t src[4], dst[4];
    char path[512];
    const char *shot = NULL;
    const char *name;
    int32_t frame;

    (void)drawDib;
    (void)flags;
    if (h == NULL || dc != (void *)(uintptr_t)kTokenDc || device == NULL || h->size < kDibHeader ||
        h->bitCount != 24 || h->compression != 0 || h->width <= 0 || h->height <= 0) {
        if (!gWarned) {
            gWarned = 1;
            bmxLog("movie: DrawDibDraw: unsupported call (DC %p, device %p, %d bits, compression %u)\n", dc,
                   (void *)device, h != NULL ? h->bitCount : 0, h != NULL ? h->compression : 0);
        }
        return 0;
    }
    if (!prepare(device, h->width, h->height)) {
        return 0;
    }
    src[0] = srcX;
    src[1] = srcY;
    src[2] = srcWidth < 0 ? h->width : srcWidth;
    src[3] = srcHeight < 0 ? h->height : srcHeight;
    /* DIB rows are bottom-up: source y counts from the bottom. */
    src[1] = h->height - src[1] - src[3];
    dst[0] = x;
    dst[1] = y;
    dst[2] = width < 0 ? src[2] : width;
    dst[3] = height < 0 ? src[3] : height;
    if (src[0] < 0 || src[1] < 0 || src[2] <= 0 || src[3] <= 0 || src[0] + src[2] > h->width ||
        src[1] + src[3] > h->height || dst[2] <= 0 || dst[3] <= 0 || dst[0] < 0 || dst[1] < 0 ||
        dst[0] + dst[2] > gTargetWidth || dst[1] + dst[3] > gTargetHeight) {
        if (!gWarned) {
            gWarned = 1;
            bmxLog(
                "movie: DrawDibDraw: rectangle (%d, %d, %d x %d) from (%d, %d, %d x %d) outside the target\n",
                x, y, width, height, srcX, srcY, srcWidth, srcHeight);
        }
        return 0;
    }
    if (!drawFrame(device, h, bits != NULL ? (const uint8_t *)bits : (const uint8_t *)(h + 1), src, dst)) {
        return 0;
    }
    gDraws++;
    if (SDL_getenv("BMX_MOVIE_SHOT") != NULL && isShotDraw(gDraws)) {
        const char *const pattern = SDL_getenv("BMX_MOVIE_SHOT");
        const char *const mark = SDL_strstr(pattern, "%d");

        if (mark != NULL) {
            SDL_snprintf(path, sizeof path, "%.*s%d%s", (int)(mark - pattern), pattern, gDraws, mark + 2);
        } else {
            SDL_snprintf(path, sizeof path, "%s", pattern);
        }
        shot = path;
        frame = bmxAviFrameOf(h, &name);
        bmxLog("movie: draw %d: frame %d of \"%s\" into (%d, %d, %d x %d) of %d x %d -> %s\n", gDraws, frame,
               name, x, y, dst[2], dst[3], gTargetWidth, gTargetHeight, shot);
    }
    return drvGpuPresentMovie(gTarget, gTargetWidth, gTargetHeight, shot);
}
