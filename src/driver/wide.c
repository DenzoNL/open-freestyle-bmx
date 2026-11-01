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
 * - HUD anchoring (W4-B, `hud = 1`): a jump over the entry of the ported ngifDrawList (0x462350) to
 *   wideDrawList, a copy that, before each packet is drawn, tells the 2D funnel the anchor of the HUD element
 *   the packet belongs to (left, centre, right; None for everything outside the HUD layer). The elements are
 *   known there: the zover pools of layer 0 (a text is one glyph chain) and zoverDrawText's runs of glyph
 *   packets in the 3D list. The funnel maps left / right elements to the edges of the hud_aspect area.
 */
#include <SDL3/SDL.h>

#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "driver/driver.h"
#include "driver/wide.h"
#include "hook/hook.h"
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
        cfg.enabled = bmxConfigOption("video.widescreen", 1, 0) != 0;
        cfg.aspect = bmxConfigInt("video.aspect", 1);
        cfg.fovScale = bmxConfigInt("video.fov_scale", 100);
        cfg.maxHfov = bmxConfigInt("video.max_hfov", 0);
        cfg.hud = bmxConfigOption("video.hud", 1, 0);
        cfg.hudAspect = bmxConfigInt("video.hud_aspect", 178);
        cfg.menus = bmxConfigOption("video.menus", 2, 0);
        cfg.movies = bmxConfigInt("video.movies", 0);
        cfg.scaleFilter = bmxConfigInt("video.scale_filter", 0);
        cfg.uiFilter = bmxConfigOption("video.ui_filter", 2, 0);
        if (cfg.uiFilter < 0 || cfg.uiFilter > 2) {
            bmxLog("wide: ui_filter %d out of range (0..2), using 0\n", cfg.uiFilter);
            cfg.uiFilter = 0;
        }
        if (cfg.hudAspect < 0) {
            bmxLog("wide: hud_aspect %d out of range (0, or the aspect * 100), using 178\n", cfg.hudAspect);
            cfg.hudAspect = 178;
        }
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
    bmxLog("wide: 1 slot (camera fov 0x561d98); aspect %d, fov_scale %d, max_hfov %d, hud %d, hud_aspect %d, "
           "menus %d, movies %d; the 2D funnel at the device start\n",
           cfg->aspect, cfg->fovScale, cfg->maxHfov, cfg->hud, cfg->hudAspect, cfg->menus, cfg->movies);
}

/* ---- 2D: the Im2D funnel ---- */

/* The anchor of the packet wideDrawList is drawing (kDrvWideAnchorNone outside it and for non-HUD packets).
 */
static int gHudAnchor;

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

/* The next stage of the funnel; the textured draws it batches are sampled by `ui` (ui_filter, W7). */
static int drawNext(int32_t type, void *vertices, int32_t numVertices, int ui)
{
    int result;

    drvDrawSetUiMode(ui);
    result = gIm2DNext(type, vertices, numVertices);
    drvDrawSetUiMode(0);
    return result;
}

static int BMX_CDECL wideIm2DPrimitive(int32_t type, void *vertices, int32_t numVertices)
{
    const RwCamera *const camera = drvCameraCurrent();
    const DrvWideConfig *const cfg = drvWideConfig();
    const DrvWideVertex *draw;
    int32_t w, h, n;
    float lo, hi;
    int axis, cls, ui;

    if (camera == NULL || numVertices <= 0 || vertices == NULL) {
        return gIm2DNext(type, vertices, numVertices);
    }
    /* Only the game's screen camera (not a render-to-texture raster such as the roller's shadow). */
    w = camera->frameBuffer->width;
    h = camera->frameBuffer->height;
    if (w != (int32_t)gZrwScreenW || h != (int32_t)gZrwScreenH) {
        return gIm2DNext(type, vertices, numVertices);
    }
    ui = cfg->uiFilter;
    if (!cfg->enabled || drvWideIs43(w, h)) {
        return drawNext(type, vertices, numVertices, ui);
    }
    cls = drvWideClassify(&((const RwD3DTLVertex *)vertices)->sx, sizeof(RwD3DTLVertex), numVertices,
                          rwD3DState.textureRaster != NULL, w, h);
    if (!drvWideMoves(cfg, cls) || !grow(numVertices + 2)) {
        return drawNext(type, vertices, numVertices, ui);
    }
    {
        float bars[2][4];
        const int nBars = drvWideBars(cfg, cls, w, h, bars);

        for (int i = 0; i < nBars; i++) {
            drvDrawFillRect(bars[i][0], bars[i][1], bars[i][2], bars[i][3]);
        }
    }
    if (drvWideBlurFill(cfg, cls)) {
        /* menus = 2: the backdrop stretched over the whole screen, blurred and darkened, under the 4:3 one.
         */
        drawNext(type, vertices, numVertices, 3);
    }
    SDL_memcpy(gCopy, vertices, (size_t)numVertices * sizeof *gCopy);
    /* What lay beyond the 4:3 screen's edge stays hidden (the pillarbox, or the 3D around a centred HUD); an
     * edge anchored HUD element is clipped where its virtual screen lands. */
    if (gHudAnchor != kDrvWideAnchorNone) {
        axis = drvWideMapHud(&gCopy->sx, sizeof *gCopy, numVertices, w, h, gHudAnchor,
                             drvWideConfig()->hudAspect, &lo, &hi);
    } else {
        drvWideMap2D(&gCopy->sx, sizeof *gCopy, numVertices, w, h);
        axis = drvWideArea(w, h, &lo, &hi);
    }
    draw = gCopy;
    n = numVertices;
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
    return drawNext(type, (void *)draw, n, ui);
}

void drvWideInstall2D(void)
{
    RwDevice *const device = rwD3DEngineDevice;

    if ((!drvWideConfig()->enabled && drvWideConfig()->uiFilter == 0) || device == NULL ||
        device->fpIm2DRenderPrimitive == (RwIm2DRenderPrimitiveFunc)wideIm2DPrimitive) {
        return;
    }
    gIm2DNext = device->fpIm2DRenderPrimitive;
    device->fpIm2DRenderPrimitive = (RwIm2DRenderPrimitiveFunc)wideIm2DPrimitive;
    bmxLog("wide: Im2D funnel wrapped (fpIm2DRenderPrimitive); ui_filter %d\n", drvWideConfig()->uiFilter);
}

/* ---- HUD anchoring: the ngifDrawList copy ---- */

static uint8_t gGlyphAnchor[2][128], gShapeAnchor[2][16]; /* layer 0's pools 1 and 0, both frames */
static int32_t gRunLeft;                                  /* packets of the current text run still to come */
static int gRunAnchor;

static int inside(const void *p, const void *base, const void *end)
{
    return (uintptr_t)p >= (uintptr_t)base && (uintptr_t)p < (uintptr_t)end;
}

/* The 3D list's buffer: from gNgifPrim3DBuffer up to the pointer gNgifPrim3D, which follows it. */
static int inPrim3D(const void *p)
{
    return inside(p, gNgifPrim3DBuffer, (const uint8_t *)&gNgifPrim3D);
}

/* The next packet of the ordering table chain after p (NULL at the end marker). */
static const uint8_t *nextPacket(const uint8_t *p)
{
    const uint32_t word = *(const uint32_t *)p;

    return (word & 0x0fffffffu) == 0x0fffffffu ? NULL : (const uint8_t *)(uintptr_t)(word & 0x0fffffffu);
}

/* The anchor of the run of 3D list packets that starts at p (one zoverDrawText, or a single packet). */
static int runAnchor(const uint8_t *p)
{
    enum { kMaxRun = 256 };
    const uint8_t *run[kMaxRun];
    int32_t n = 0;
    int16_t box[4], g[4];

    for (const uint8_t *q = p; q != NULL && n < kMaxRun && inPrim3D(q); q = nextPacket(q)) {
        run[n++] = q;
    }
    n = drvWideGlyphRun(run, n);
    gRunLeft = n - 1;
    if (!drvWidePacketBox(p, box)) {
        return kDrvWideAnchorNone;
    }
    for (int32_t i = 1; i < n; i++) {
        if (drvWidePacketBox(run[i], g)) {
            box[0] = g[0] < box[0] ? g[0] : box[0];
            box[2] = g[2] > box[2] ? g[2] : box[2];
        }
    }
    return drvWideHudAnchorOf(box[0], box[2]);
}

/* The anchor of packet p (visited in chain order: a text run's anchor is decided at its first glyph). */
static int packetAnchor(const uint8_t *p)
{
    if (inside(p, gZoverPool1, (const uint8_t *)gZoverPool1 + sizeof gZoverPool1)) {
        const size_t i = (size_t)(p - (const uint8_t *)gZoverPool1) / sizeof(ZOverItem);

        return gGlyphAnchor[i / 128][i % 128];
    }
    if (inside(p, gZoverPool0, (const uint8_t *)gZoverPool0 + sizeof gZoverPool0)) {
        const size_t i = (size_t)(p - (const uint8_t *)gZoverPool0) / sizeof(ZOverItem);

        return gShapeAnchor[i / 16][i % 16];
    }
    /* The 3D list holds the HUD's per-frame drawing (zoverDrawText, zoverDrawRect) while no menu screen
     * (layer 1) is up; also fades and bars, which the funnel keeps as covers anyway. */
    if (!inPrim3D(p) || gZoverLayer != 0) {
        gRunLeft = 0;
        return kDrvWideAnchorNone;
    }
    if (gRunLeft > 0) {
        gRunLeft--;
        return gRunAnchor;
    }
    gRunAnchor = runAnchor(p);
    return gRunAnchor;
}

/* ngifDrawList (0x462350), plus the anchor of each packet for the 2D funnel. */
static void wideDrawList(uint32_t *entry)
{
    const int anchored = (int64_t)gZrwScreenW * 3 > (int64_t)gZrwScreenH * 4;
    uint32_t word = *entry;

    if (anchored) {
        for (int f = 0; f < 2; f++) {
            drvWideHudPools(gZoverPool1[f][0].b, 128, gZoverPool0[f][0].b, 16, gGlyphAnchor[f],
                            gShapeAnchor[f]);
        }
        gRunLeft = 0;
    }
    while ((word & 0x0fffffffu) != 0x0fffffffu) {
        entry = (uint32_t *)(uintptr_t)(*entry & 0x0fffffffu);
        gHudAnchor = anchored ? packetAnchor((const uint8_t *)entry) : kDrvWideAnchorNone;
        if ((*entry & 0xf0000000u) != 0) {
            ngifDrawPrimitive((const uint8_t *)entry);
        }
        word = *entry;
    }
    gHudAnchor = kDrvWideAnchorNone;
}

static BmxHook gJumps[1];

const BmxHook *drvWideJumps(uint32_t *count)
{
    const DrvWideConfig *const cfg = drvWideConfig();

    gJumps[0].orig = (uint32_t)(uintptr_t)ngifDrawList;
    gJumps[0].impl = (void *)wideDrawList;
    *count = cfg->enabled && cfg->hud == 1 ? 1u : 0u;
    return gJumps;
}

/* ---- movies ---- */

void drvWideMovieRect(int32_t rect[4], int32_t targetW, int32_t targetH)
{
    const DrvWideConfig *const cfg = drvWideConfig();

    if (cfg->enabled && cfg->movies == 0) {
        drvWideMapRect(rect, targetW, targetH);
    }
}
