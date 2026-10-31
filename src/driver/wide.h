/*
 * Widescreen, native resolution, HUD anchoring and the present filter (docs/WIDESCREEN.md, slices WS1-WS5).
 * Native exe only.
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
    int hud;         /* 0 centred 4:3, 1 edge anchored (WS5), 2 stretch */
    int hudAspect;   /* hud_aspect: width of the anchored HUD as aspect * 100 (0 = the full width) */
    int menus;       /* 0 pillarbox, 1 stretch, 2 pillarbox on a blurred, darkened fill (WS6) */
    int movies;      /* 0 pillarbox, 1 stretch */
    int scaleFilter; /* scale_filter: 0 linear, 1 nearest, 2 integer (present, independent of widescreen) */
    int uiFilter;    /* ui_filter: 0 the game's sampling, 1 linear, 2 sharp bilinear (WS6; independent of
                        widescreen) */
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

/* ---- HUD anchoring (WS5, W4-B) ---- */

/* The anchor of a HUD element; None = not a HUD element (centred 4:3 like everything else). */
enum {
    kDrvWideAnchorNone = 0,
    kDrvWideAnchorLeft = 1,
    kDrvWideAnchorCentre = 2,
    kDrvWideAnchorRight = 3,
};

/*
 * The anchor of an element whose x extent in the 640 wide virtual screen is [x0, x1]: centre when its middle
 * lies within 8 virtual pixels of the screen's middle (ovlCentreX texts land within one), else left when the
 * middle is left of the screen's middle, else right.
 */
int drvWideHudAnchorOf(int32_t x0, int32_t x1);

/*
 * The box {x0, y0, x1, y1} of a display packet (an ngif PSX style packet: command byte at +7, 16 bit
 * virtual 640x480 corners). Returns 0 for a packet that is not a 2D polygon or line (other commands, or
 * the float flag of the 3D list).
 */
int drvWidePacketBox(const uint8_t *packet, int16_t box[4]);

/* zover pool items (ZOverItem, zoverii.h): 0xa0 bytes, kind at 0x9c, next glyph handle at 0x9e. */
enum {
    kDrvWideItemSize = 0xa0,
    kDrvWideMaxItems = 512,
};

/*
 * The anchors of the HUD layer's pool items (one frame): `glyphs` (pool 1, n of them) are texts, a glyph
 * chain per text; `shapes` (pool 0) are rectangles, gradients and sprites. Each text gets the anchor of
 * its whole chain's box, each shape its own; an element that lies inside a text's x extent on the same
 * lines (the button glyphs of a hint line) takes that text's anchor. Free items get None.
 */
void drvWideHudPools(const uint8_t *glyphs, int32_t nGlyphs, const uint8_t *shapes, int32_t nShapes,
                     uint8_t *glyphAnchor, uint8_t *shapeAnchor);

/*
 * The 3D list's text runs: zoverDrawText writes one 0x9c byte FT4 packet per glyph, back to back, each
 * at x = previous x1 + 1 on the same lines, and links them into ordering table entry 0 in a row (so the
 * draw order is reversed). `packets` are the next packets in draw order; returns how many of them, from
 * the first, belong to the first one's text (at least 1 when n > 0).
 */
int32_t drvWideGlyphRun(const uint8_t *const *packets, int32_t n);

/*
 * The anchored HUD area of a w x h screen wider than 4:3: hud_aspect * h / 100 wide (0: w), at least the
 * 4:3 area and at most w, centred on whole pixels. Returns 0 (nothing) for a screen not wider than 4:3.
 */
int drvWideHudArea(int32_t w, int32_t h, int hudAspect, float *lo, float *hi);

/*
 * Maps n vertices from the stretched screen like drvWideMap2D, but with the edge of the anchor: left keeps
 * the virtual left edge at the HUD area's left edge, right the right edge at its right edge, centre (and
 * None) is drvWideMap2D. [lo, hi] is where the virtual screen [0, 640] lands (what the moved primitive is
 * clipped to). Returns the axis as drvWideArea (-1: nothing moved).
 */
int drvWideMapHud(float *xy, size_t stride, int32_t n, int32_t w, int32_t h, int anchor, int hudAspect,
                  float *lo, float *hi);

/* The same for an integer rectangle {x, y, w, h} (movies), edges rounded to the nearest pixel. */
void drvWideMapRect(int32_t rect[4], int32_t w, int32_t h);

/*
 * The present (W7): where a width x height image goes in a w x h window, {x, y, w, h}. Returns the filter,
 * 0 linear or 1 nearest. Filter 0: letterboxed, linear (vanilla); 1: letterboxed, nearest; 2: the largest
 * whole factor that fits, nearest, black border (linear letterbox when even 1x does not fit).
 */
int drvWidePresentRect(int filter, int32_t width, int32_t height, uint32_t w, uint32_t h, uint32_t out[4]);

/* Whether a primitive of that class gets the blurred fill drawn stretched behind it (menus = 2: backdrops).
 */
int drvWideBlurFill(const DrvWideConfig *cfg, int cls);

/*
 * The sharp-bilinear sampling coordinate (WS6, the C twin of sharpUv in shaders/tl.hlsl): for the texel
 * coordinate `t` (texture coordinate * size) of a texture shown with `s` output pixels per texel, the
 * coordinate a linear sampler is given: nearest inside a texel, a linear ramp one output pixel wide at its
 * edges. s below 1 counts as 1 (plain bilinear).
 */
float drvWideSharpCoord(float t, float s);

/* ---- wide.c: the seams ---- */

/* The native loader: stores the FOV wrapper into the zgl slot 0x561d98 when widescreen is on; logs
 * `wide: <n> slots`. */
void drvWideInstall(void);
/* The device start, after rwD3DIm2DOpen: wraps the engine device's fpIm2DRenderPrimitive. */
void drvWideInstall2D(void);
/*
 * The loader's jumps (like bmxSmoothJumps): with widescreen = 1 and hud = 1, the entry of the ported
 * ngifDrawList to the copy that tells the 2D funnel each packet's HUD anchor; none otherwise.
 */
const struct BmxHook *drvWideJumps(uint32_t *count);
/* The movies (DrawDibDraw): pillarboxes the destination rectangle of a target_w x target_h target. */
void drvWideMovieRect(int32_t rect[4], int32_t targetW, int32_t targetH);

#endif
