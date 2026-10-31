/*
 * The widescreen seams (docs/WIDESCREEN.md W8-A): pointer swaps at existing indirection slots, installed only
 * with `[video] widescreen = 1`. No ported function changes.
 *
 * - 3D (W2): the zgl camera slot 0x561d98 (zrwCameraSetFov 0x492a60) gets wideSetFov, which calls the
 *   original and then rewrites the view window the original just set. Every FOV change of the game's one
 *   camera goes through the slot, the creation's included (zobjCameraCreate calls it right after
 *   zrwCameraCreate).
 * - 2D (W4-W6): the engine device's fpIm2DRenderPrimitive (rwD3DIm2DPrimitive 0x4d9920, the funnel of all
 *   game 2D) gets wideIm2DPrimitive, which copies the vertices, classifies the primitive and maps it into the
 *   centred 4:3 area unless it is a cover. Clipping happened before in stretched space; the map is affine, so
 *   clip and draw agree.
 * - Movies (W5): DrawDibDraw's destination rectangle (movie.c calls drvWideMovieRect).
 */
#include <SDL3/SDL.h>

#include "driver/driver.h"
#include "driver/wide.h"
#include "platform/platform.h"
#include "rw/rwcamera.h"
#include "rw/rwd3d.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/com/zglrw/zrwzgl.h"

const DrvWideConfig *drvWideConfig(void)
{
    static DrvWideConfig cfg;
    static int loaded;

    if (!loaded) {
        loaded = 1;
        cfg.enabled = bmxConfigInt("video.widescreen", 0) != 0;
        cfg.aspect = bmxConfigInt("video.aspect", 1);
        cfg.fovScale = bmxConfigInt("video.fov_scale", 100);
        cfg.maxHfov = bmxConfigInt("video.max_hfov", 0);
        cfg.hud = bmxConfigInt("video.hud", 0);
        cfg.menus = bmxConfigInt("video.menus", 0);
        cfg.movies = bmxConfigInt("video.movies", 0);
        cfg.scaleFilter = bmxConfigInt("video.scale_filter", 0);
        if (cfg.fovScale < 10 || cfg.fovScale > 400) {
            bmxLog("wide: fov_scale %d out of range (10..400), using 100\n", cfg.fovScale);
            cfg.fovScale = 100;
        }
    }
    return &cfg;
}

/* ---- 3D: the FOV slot ---- */

static ZrwCamSetFovFn gSetFovNext;

static void BMX_CDECL wideSetFov(void *camera, int fov)
{
    ZrwCamera *const cam = camera;
    float window[2];
    const RwV2d *current;

    gSetFovNext(camera, fov);
    if ((int16_t)fov == 0 || cam->rwCamera == NULL) {
        return; /* the original left the window alone (it was adjusted when it was set) */
    }
    current = RwCameraGetViewWindow(cam->rwCamera);
    window[0] = current->x;
    window[1] = current->y;
    if (drvWideViewWindow(drvWideConfig(), cam->width, cam->height, window)) {
        const RwV2d next = {window[0], window[1]};

        RwCameraSetViewWindow(cam->rwCamera, &next);
    }
}

void drvWideInstall(void)
{
    const DrvWideConfig *const cfg = drvWideConfig();

    if (!cfg->enabled) {
        bmxLog("wide: 0 slots (widescreen off)\n");
        return;
    }
    gSetFovNext = gZrwCamSetFov;
    gZrwCamSetFov = wideSetFov;
    bmxLog("wide: 1 slot (camera fov 0x561d98); aspect %d, fov_scale %d, max_hfov %d, hud %d, menus %d, "
           "movies %d; the 2D funnel at the device start\n",
           cfg->aspect, cfg->fovScale, cfg->maxHfov, cfg->hud, cfg->menus, cfg->movies);
}

/* ---- 2D: the Im2D funnel ---- */

static RwIm2DRenderPrimitiveFunc gIm2DNext;
static DrvWideVertex *gCopy, *gClip, *gScratch; /* gCopyCap vertices each (n + 2 for a clipped fan) */
static int32_t gCopyCap;

_Static_assert(sizeof(DrvWideVertex) == sizeof(RwD3DTLVertex), "the funnel's vertex layout");
_Static_assert(offsetof(DrvWideVertex, color) == offsetof(RwD3DTLVertex, color),
               "the funnel's vertex layout");
_Static_assert(offsetof(DrvWideVertex, u) == offsetof(RwD3DTLVertex, u), "the funnel's vertex layout");

static int grow(int32_t need)
{
    if (need > gCopyCap) {
        DrvWideVertex *const copy = SDL_realloc(gCopy, (size_t)need * sizeof *gCopy);
        DrvWideVertex *const clip = copy != NULL ? SDL_realloc(gClip, (size_t)need * sizeof *gClip) : NULL;
        DrvWideVertex *const scratch =
            clip != NULL ? SDL_realloc(gScratch, (size_t)need * sizeof *gScratch) : NULL;

        gCopy = copy != NULL ? copy : gCopy;
        gClip = clip != NULL ? clip : gClip;
        gScratch = scratch != NULL ? scratch : gScratch;
        if (scratch == NULL) {
            return 0;
        }
        gCopyCap = need;
    }
    return 1;
}

static int BMX_CDECL wideIm2DPrimitive(int32_t type, void *vertices, int32_t numVertices)
{
    const RwCamera *const camera = drvCameraCurrent();
    const DrvWideVertex *draw;
    int32_t w, h, n;
    float lo, hi;
    int axis;

    if (camera == NULL || numVertices <= 0 || vertices == NULL) {
        return gIm2DNext(type, vertices, numVertices);
    }
    /* Only the game's screen camera (not a render-to-texture raster such as the roller's shadow). */
    w = camera->frameBuffer->width;
    h = camera->frameBuffer->height;
    if (w != (int32_t)gZrwScreenW || h != (int32_t)gZrwScreenH || drvWideIs43(w, h)) {
        return gIm2DNext(type, vertices, numVertices);
    }
    if (!drvWideMoves(drvWideConfig(),
                      drvWideClassify(&((const RwD3DTLVertex *)vertices)->sx, sizeof(RwD3DTLVertex),
                                      numVertices, rwD3DState.textureRaster != NULL, w, h)) ||
        !grow(numVertices + 2)) {
        return gIm2DNext(type, vertices, numVertices);
    }
    SDL_memcpy(gCopy, vertices, (size_t)numVertices * sizeof *gCopy);
    drvWideMap2D(&gCopy->sx, sizeof *gCopy, numVertices, w, h);
    draw = gCopy;
    n = numVertices;
    /* What lay beyond the 4:3 screen's edge stays hidden (the pillarbox, or the 3D around a centred HUD). */
    axis = drvWideArea(w, h, &lo, &hi);
    if (drvWideOutside(gCopy, numVertices, axis, lo, hi)) {
        if (type == 5) { /* rwPRIMTYPETRIFAN: zrwprim's polygons */
            n = drvWideClipFan(gCopy, numVertices, axis, lo, hi, gClip, gScratch);
            draw = gClip;
        } else if (type == 1) { /* rwPRIMTYPELINELIST: zrwprim's lines */
            n = drvWideClipLines(gCopy, numVertices, axis, lo, hi, gClip);
            draw = gClip;
        }
        if (n == 0) {
            return 1; /* nothing left on screen */
        }
    }
    return gIm2DNext(type, (void *)draw, n);
}

void drvWideInstall2D(void)
{
    RwDevice *const device = rwD3DEngineDevice;

    if (!drvWideConfig()->enabled || device == NULL ||
        device->fpIm2DRenderPrimitive == (RwIm2DRenderPrimitiveFunc)wideIm2DPrimitive) {
        return;
    }
    gIm2DNext = device->fpIm2DRenderPrimitive;
    device->fpIm2DRenderPrimitive = (RwIm2DRenderPrimitiveFunc)wideIm2DPrimitive;
    bmxLog("wide: Im2D funnel wrapped (fpIm2DRenderPrimitive)\n");
}

/* ---- movies ---- */

void drvWideMovieRect(int32_t rect[4], int32_t targetW, int32_t targetH)
{
    const DrvWideConfig *const cfg = drvWideConfig();

    if (cfg->enabled && cfg->movies == 0) {
        drvWideMapRect(rect, targetW, targetH);
    }
}
