/*
 * Widescreen, native resolution and the present filter (docs/WIDESCREEN.md, slices WS1-WS3). Native exe only.
 *
 * Everything here is visual-only and attaches at seams (W8-A): the zgl camera slot 0x561d98 (the 3D view
 * window, W2), the engine device's fpIm2DRenderPrimitive (all game 2D, W4-W6), the DrawDib stand-in (movies,
 * W5) and the present blit (scale_filter, W7). With `[video] widescreen = 0` (the default) nothing is
 * installed; at an exact 4:3 mode every transform is the identity and is skipped, so the bytes stay the
 * original's.
 *
 * widemath.c holds the pure maths (no SDL, no game state: tests/platform/test_wide.c), wide.c the seams.
 */
#ifndef BMX_DRIVER_WIDE_H
#define BMX_DRIVER_WIDE_H

#include <stddef.h>
#include <stdint.h>

/* [video] keys (W9). */
typedef struct DrvWideConfig {
    int enabled;     /* widescreen: 1 installs the seams */
    int aspect;      /* 0 stretch, 1 hor+ fit (4:3 inside), 2 vert- */
    int fovScale;    /* fov_scale: percent of both view window axes (100 = none) */
    int maxHfov;     /* max_hfov: cap of the horizontal field of view in degrees (0 = none) */
    int hud;         /* 0 centred 4:3, 1 edge anchored (WS5, not yet: as 0), 2 stretch */
    int menus;       /* 0 pillarbox, 1 stretch (2: blurred fill, WS6, not yet: as 0) */
    int movies;      /* 0 pillarbox, 1 stretch */
    int scaleFilter; /* scale_filter: 0 linear, 1 nearest, 2 integer (present, independent of widescreen) */
} DrvWideConfig;

/* The configuration from bmxnative.ini (read once). */
const DrvWideConfig *drvWideConfig(void);

/* ---- widemath.c: pure maths ---- */

/* Whether a w x h screen is exactly 4:3 (then nothing is transformed). */
int drvWideIs43(int32_t w, int32_t h);

/*
 * The 3D view window (W2) for a w x h camera, from the 4:3 window the original just set. Returns 1 and
 * writes `window` when it changes, 0 (untouched) otherwise. Aspect 1 (hor+ fit): wider than 4:3 keeps y
 * and widens x to y * w / h, narrower keeps x and grows y to x * h / w (vert+); aspect 2 (vert-): keeps x,
 * y = x * h / w; aspect 0: as the original. Then fov_scale (percent) on both axes and the max_hfov cap
 * (window.x <= tan(max / 2), y scaled alike).
 */
int drvWideViewWindow(const DrvWideConfig *cfg, int32_t w, int32_t h, float window[2]);

/* Classes of a 2D primitive (W4-W6). */
enum {
    kDrvWideOther = 0,    /* HUD, menu items, text: centred 4:3 */
    kDrvWideCover = 1,    /* untextured, spans the full width: fades, bars, dim rects; stays stretched */
    kDrvWideBackdrop = 2, /* textured, spans the full width: backdrops, loading pictures; pillarboxed */
};

/*
 * Classifies a primitive of `n` vertices of a w x h screen. `xy` points at the first vertex's x (y follows
 * it), `stride` is the vertex size in bytes. Full width: the x extent reaches both edges within two
 * virtual (640 wide) pixels.
 */
int drvWideClassify(const float *xy, size_t stride, int32_t n, int textured, int32_t w, int32_t h);

/* Whether a primitive of that class is moved into the centred 4:3 area under `cfg`. */
int drvWideMoves(const DrvWideConfig *cfg, int cls);

/*
 * Maps n vertices in place from the stretched w x h screen into its centred 4:3 area: wider than 4:3,
 * x' = (w - 4/3 h) / 2 + x * (4/3 h) / w; narrower, y' = (h - 3/4 w) / 2 + y * (3/4 w) / h. A 4:3 screen
 * is left alone.
 */
void drvWideMap2D(float *xy, size_t stride, int32_t n, int32_t w, int32_t h);

/* A vertex of the 2D funnel: the layout of D3D7's TL vertex (RwD3DTLVertex, rw/rwd3d.h). */
typedef struct DrvWideVertex {
    float sx, sy, sz, rhw;
    uint32_t color, specular; /* D3DCOLOR, interpolated per byte */
    float u, v;
} DrvWideVertex;

/*
 * The centred 4:3 area of a w x h screen along the mapped axis: returns 0 (x, wider than 4:3) or 1 (y,
 * narrower) and the range [lo, hi]; -1 for 4:3. Moved primitives are clipped to it, as the screen edge
 * clipped them in 4:3 (the game draws sliding menus and messages partly off screen).
 */
int drvWideArea(int32_t w, int32_t h, float *lo, float *hi);
/* Whether a vertex lies outside [lo, hi] on the axis. */
int drvWideOutside(const DrvWideVertex *v, int32_t n, int axis, float lo, float hi);
/*
 * Clips a convex polygon (a triangle fan, as zrwprim draws every 2D polygon) to lo <= coord <= hi. `out` and
 * `scratch` hold n + 2 vertices. Returns the vertex count (0: nothing left). Attributes are interpolated
 * linearly in screen space (2D primitives have one rhw).
 */
int32_t drvWideClipFan(const DrvWideVertex *in, int32_t n, int axis, float lo, float hi, DrvWideVertex *out,
                       DrvWideVertex *scratch);
/* Clips a line list (pairs) the same way; `out` holds n vertices. Returns the vertex count. */
int32_t drvWideClipLines(const DrvWideVertex *in, int32_t n, int axis, float lo, float hi,
                         DrvWideVertex *out);

/* The same for an integer rectangle {x, y, w, h} (movies), edges rounded to the nearest pixel. */
void drvWideMapRect(int32_t rect[4], int32_t w, int32_t h);

/*
 * The present (W7): where a width x height image goes in a w x h window, {x, y, w, h}. Returns the filter,
 * 0 linear or 1 nearest. Filter 0: letterboxed, linear (vanilla); 1: letterboxed, nearest; 2: the largest
 * whole factor that fits, nearest, black border (linear letterbox when even 1x does not fit).
 */
int drvWidePresentRect(int filter, int32_t width, int32_t height, uint32_t w, uint32_t h, uint32_t out[4]);

/* ---- wide.c: the seams ---- */

/* The native loader: stores the FOV wrapper into the zgl slot 0x561d98 when widescreen is on; logs
 * `wide: <n> slots`. */
void drvWideInstall(void);
/* The device start, after rwD3DIm2DOpen: wraps the engine device's fpIm2DRenderPrimitive. */
void drvWideInstall2D(void);
/* The movies (DrawDibDraw): pillarboxes the destination rectangle of a target_w x target_h target. */
void drvWideMovieRect(int32_t rect[4], int32_t targetW, int32_t targetH);

#endif
