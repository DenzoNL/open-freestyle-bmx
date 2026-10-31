/*
 * SDL3 side of the driver: the video subsystem (display modes), the GPU device on the game's own window,
 * render targets, clears and the present (D4: the camera raster at the mode's size, scaled into the window
 * with letterboxing).
 *
 * Dev aids (environment): BMX_SHOT=<file.bmp> saves the camera raster of the shown frame number
 * BMX_SHOT_FRAME (default 1; a comma separated list saves several, a `%d` in the file name takes the frame
 * number) as a BMP; BMX_QUIT_FRAMES=<n> exits the process after n shown frames.
 */
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "driver/driver.h"
#include "native/native.h"
#include "platform/platform.h"

static SDL_Window *gpuWindow;
static SDL_GPUDevice *gpuDevice;
static int videoInited;
static int shownFrames;

int drvGpuShownFrames(void)
{
    return shownFrames;
}

/* Whether `frame` is in BMX_SHOT_FRAME's comma separated list (frame 1 without the variable). */
static int isShotFrame(const char *list, int frame)
{
    if (list == NULL) {
        return frame == 1;
    }
    while (*list) {
        char *end;
        const long n = SDL_strtol(list, &end, 10);

        if (end == list) {
            break;
        }
        if (n == frame) {
            return 1;
        }
        list = *end == ',' ? end + 1 : end;
    }
    return 0;
}

static int sdlFail(const char *what)
{
    bmxLog("driver: %s failed: %s\n", what, SDL_GetError());
    return 0;
}

int drvGpuInitVideo(void)
{
    if (videoInited) {
        return 1;
    }
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        return sdlFail("SDL_InitSubSystem(VIDEO)");
    }
    videoInited = 1;
    return 1;
}

void drvGpuQuitVideo(void)
{
    if (videoInited) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        videoInited = 0;
    }
}

/* The windowed mode: the primary display's desktop size and depth (16 or 32). */
int drvGpuDesktopMode(DrvVideoMode *mode)
{
    const SDL_DisplayMode *desktop = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());

    if (desktop == NULL) {
        return sdlFail("SDL_GetDesktopDisplayMode");
    }
    mode->width = desktop->w;
    mode->height = desktop->h;
    mode->depth = SDL_BYTESPERPIXEL(desktop->format) == 2 ? 16 : 32;
    mode->flags = 0;
    return 1;
}

/* The primary display's full screen sizes (duplicates and refresh rates are left to the caller). */
int drvGpuFullscreenSizes(int32_t (*sizes)[2], int max)
{
    int count = 0, n = 0;
    SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &count);

    if (modes == NULL) {
        return 0;
    }
    for (int i = 0; i < count && n < max; i++) {
        sizes[n][0] = modes[i]->w;
        sizes[n][1] = modes[i]->h;
        n++;
    }
    SDL_free((void *)modes);
    return n;
}

/* The GPU device on the game's window (RwEngineOpen's handle: the SDL window src/platform/window.c made). */
int drvGpuStart(void *window)
{
    gpuWindow = bmxWindowSdl(window);
    if (gpuWindow == NULL) {
        bmxLog("driver: the window handle %p is not the game's SDL window\n", window);
        return 0;
    }
    gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL |
                                        SDL_GPU_SHADERFORMAT_DXBC | SDL_GPU_SHADERFORMAT_MSL,
                                    false, NULL);
    if (gpuDevice == NULL) {
        sdlFail("SDL_CreateGPUDevice");
        drvGpuStop();
        return 0;
    }
    if (!SDL_ClaimWindowForGPUDevice(gpuDevice, gpuWindow)) {
        sdlFail("SDL_ClaimWindowForGPUDevice");
        drvGpuStop();
        return 0;
    }
    bmxLog("driver: SDL %d.%d.%d, GPU driver %s\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION,
           SDL_GetGPUDeviceDriver(gpuDevice));
    if (!drvDrawStart()) {
        drvGpuStop();
        return 0;
    }
    return 1;
}

struct SDL_GPUDevice *drvGpuDevice(void)
{
    return gpuDevice;
}

void drvGpuStop(void)
{
    if (gpuDevice != NULL) {
        drvMovieRelease();
        drvDrawStop();
        if (gpuWindow != NULL) {
            SDL_ReleaseWindowFromGPUDevice(gpuDevice, gpuWindow);
        }
        SDL_DestroyGPUDevice(gpuDevice);
        gpuDevice = NULL;
    }
    gpuWindow = NULL; /* the window is the platform's (window.c) */
}

/* A render target: RGBA8 colour (depth >= 0) or a depth texture (depth < 0). */
struct SDL_GPUTexture *drvGpuCreateTarget(int32_t width, int32_t height, int depth)
{
    SDL_GPUTextureCreateInfo info;
    SDL_GPUTexture *texture;

    if (gpuDevice == NULL || width <= 0 || height <= 0) {
        return NULL;
    }
    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.width = (Uint32)width;
    info.height = (Uint32)height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    if (depth < 0) {
        info.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    } else {
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    }
    texture = SDL_CreateGPUTexture(gpuDevice, &info);
    if (texture == NULL) {
        sdlFail("SDL_CreateGPUTexture");
    }
    return texture;
}

void drvGpuReleaseTexture(struct SDL_GPUTexture *texture)
{
    if (gpuDevice != NULL && texture != NULL) {
        SDL_ReleaseGPUTexture(gpuDevice, texture);
    }
}

/* Clears colour (flags 1) and / or depth (flags 2) of the whole target. */
int drvGpuClear(struct SDL_GPUTexture *color, struct SDL_GPUTexture *depth, const uint8_t rgba[4], int flags)
{
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUColorTargetInfo target;
    SDL_GPUDepthStencilTargetInfo depthTarget;
    SDL_GPURenderPass *pass;
    const int clearDepth = (flags & 2) != 0 && depth != NULL;

    if (flags == 0 || (flags == 2 && !clearDepth)) {
        return 1;
    }
    drvDrawFlush();
    cmd = SDL_AcquireGPUCommandBuffer(gpuDevice);
    if (cmd == NULL) {
        return sdlFail("SDL_AcquireGPUCommandBuffer");
    }
    SDL_zero(target);
    target.texture = color;
    target.clear_color.r = (float)rgba[0] / 255.0f;
    target.clear_color.g = (float)rgba[1] / 255.0f;
    target.clear_color.b = (float)rgba[2] / 255.0f;
    target.clear_color.a =
        1.0f; /* camera surfaces (565 / X888) have no alpha: keep it opaque for the present */
    target.load_op = (flags & 1) != 0 ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_zero(depthTarget);
    depthTarget.texture = depth;
    depthTarget.clear_depth = 1.0f;
    depthTarget.load_op = clearDepth ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    depthTarget.store_op = SDL_GPU_STOREOP_STORE;
    depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    pass = SDL_BeginGPURenderPass(cmd, &target, 1, depth != NULL ? &depthTarget : NULL);
    SDL_EndGPURenderPass(pass);
    return SDL_SubmitGPUCommandBuffer(cmd) ? 1 : sdlFail("SDL_SubmitGPUCommandBuffer");
}

/* Saves the colour target as a BMP (BMX_SHOT). */
static void saveShot(SDL_GPUTexture *texture, int32_t width, int32_t height, const char *path)
{
    SDL_GPUTransferBufferCreateInfo info;
    SDL_GPUTransferBuffer *buffer;
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUTextureRegion region;
    SDL_GPUTextureTransferInfo dst;
    SDL_GPUFence *fence;
    void *pixels;

    SDL_zero(info);
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size = (Uint32)(width * height * 4);
    buffer = SDL_CreateGPUTransferBuffer(gpuDevice, &info);
    cmd = buffer != NULL ? SDL_AcquireGPUCommandBuffer(gpuDevice) : NULL;
    if (cmd == NULL) {
        sdlFail("screenshot buffer");
        SDL_ReleaseGPUTransferBuffer(gpuDevice, buffer);
        return;
    }
    copy = SDL_BeginGPUCopyPass(cmd);
    SDL_zero(region);
    region.texture = texture;
    region.w = (Uint32)width;
    region.h = (Uint32)height;
    region.d = 1;
    SDL_zero(dst);
    dst.transfer_buffer = buffer;
    SDL_DownloadFromGPUTexture(copy, &region, &dst);
    SDL_EndGPUCopyPass(copy);
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    if (fence != NULL && SDL_WaitForGPUFences(gpuDevice, true, &fence, 1)) {
        pixels = SDL_MapGPUTransferBuffer(gpuDevice, buffer, false);
        if (pixels != NULL) {
            SDL_Surface *surface =
                SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_ABGR8888, pixels, width * 4);

            if (surface == NULL || !SDL_SaveBMP(surface, path)) {
                sdlFail("SDL_SaveBMP");
            } else {
                bmxLog("driver: saved %s (%dx%d)\n", path, (int)width, (int)height);
            }
            SDL_DestroySurface(surface);
            SDL_UnmapGPUTransferBuffer(gpuDevice, buffer);
        }
    }
    SDL_ReleaseGPUFence(gpuDevice, fence);
    SDL_ReleaseGPUTransferBuffer(gpuDevice, buffer);
}

/* Letterboxed destination of a width x height image in a w x h window. */
static void letterbox(int32_t width, int32_t height, Uint32 w, Uint32 h, SDL_GPUBlitRegion *dst)
{
    const double scale = SDL_min((double)w / width, (double)h / height);
    const Uint32 dw = (Uint32)(width * scale + 0.5), dh = (Uint32)(height * scale + 0.5);

    dst->x = (w - dw) / 2;
    dst->y = (h - dh) / 2;
    dst->w = dw;
    dst->h = dh;
}

/* Blits a target into the swapchain (black bars around it) and presents. */
static int presentTarget(struct SDL_GPUTexture *color, int32_t x, int32_t y, int32_t width, int32_t height)
{
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUTexture *swapchain = NULL;
    Uint32 w = 0, h = 0;

    drvDrawFlush();
    cmd = SDL_AcquireGPUCommandBuffer(gpuDevice);
    if (cmd == NULL) {
        return sdlFail("SDL_AcquireGPUCommandBuffer");
    }
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, gpuWindow, &swapchain, &w, &h)) {
        SDL_CancelGPUCommandBuffer(cmd);
        return sdlFail("SDL_WaitAndAcquireGPUSwapchainTexture");
    }
    if (swapchain != NULL) {
        SDL_GPUBlitInfo blit;

        SDL_zero(blit);
        blit.source.texture = color;
        blit.source.x = (Uint32)x;
        blit.source.y = (Uint32)y;
        blit.source.w = (Uint32)width;
        blit.source.h = (Uint32)height;
        blit.destination.texture = swapchain;
        letterbox(width, height, w, h, &blit.destination);
        blit.load_op = SDL_GPU_LOADOP_CLEAR;
        blit.clear_color.a = 1.0f;
        blit.filter = SDL_GPU_FILTER_LINEAR;
        SDL_BlitGPUTexture(cmd, &blit);
    }
    if (!SDL_SubmitGPUCommandBuffer(cmd)) {
        return sdlFail("SDL_SubmitGPUCommandBuffer");
    }
    return 1;
}

/* The client size of the game's window in pixels (0 x 0 before the device exists). */
void drvGpuWindowSize(int32_t *width, int32_t *height)
{
    int w = 0, h = 0;

    if (gpuWindow != NULL) {
        SDL_GetWindowSizeInPixels(gpuWindow, &w, &h);
    }
    *width = w;
    *height = h;
}

/* A movie frame (src/driver/movie.c): presented like a camera raster, but not counted as a shown frame, so
 * BMX_SHOT_FRAME / BMX_KEYS / BMX_QUIT_FRAMES numbers do not move during a movie. `shot`: save the target
 * too. */
int drvGpuPresentMovie(struct SDL_GPUTexture *color, int32_t width, int32_t height, const char *shot)
{
    if (!presentTarget(color, 0, 0, width, height)) {
        return 0;
    }
    if (shot != NULL) {
        saveShot(color, width, height, shot);
    }
    return 1;
}

/* Blits the camera raster's target into the swapchain (black bars around it) and presents. */
int drvGpuPresent(struct SDL_GPUTexture *color, int32_t x, int32_t y, int32_t width, int32_t height)
{
    const char *shot = SDL_getenv("BMX_SHOT");
    const char *shotFrame = SDL_getenv("BMX_SHOT_FRAME");
    const char *quit = SDL_getenv("BMX_QUIT_FRAMES");

    if (!presentTarget(color, x, y, width, height)) {
        return 0;
    }
    shownFrames++;
    if (shot != NULL && isShotFrame(shotFrame, shownFrames)) {
        char path[1024];
        const char *mark = SDL_strstr(shot, "%d");

        /* The frame number replaces the first `%d` (not a format string: the rest is copied as is). */
        if (mark != NULL) {
            SDL_snprintf(path, sizeof path, "%.*s%d%s", (int)(mark - shot), shot, shownFrames, mark + 2);
        }
        saveShot(color, width, height, mark != NULL ? path : shot);
    }
    if (quit != NULL && shownFrames >= SDL_atoi(quit)) {
        bmxLog("driver: BMX_QUIT_FRAMES reached after %d frames\n", shownFrames);
        bmxPlatformShutdown(); /* joins the sound timer thread; logs BMX_TIMER_LOG / BMX_INPUT_CHECK */
        _Exit(0);              /* like the original's _exit: no atexit handlers */
    }
    return 1;
}
