/*
 * bmxwidetest: the widescreen maths (src/driver/widemath.c, docs/WIDESCREEN.md W10.3) on synthetic input:
 * the view window (hor+, vert+, vert-, the 4:3 identity, fov_scale, max_hfov), the 2D classifier (fade, bars,
 * dim rect, backdrop, glyphs, clipped quads), the 2D and rectangle maps, the clip to the 4:3 area and the
 * present rectangle; the HUD anchoring (WS5): element anchors, packet boxes, the pool walk (texts as glyph
 * chains, containment), zoverDrawText runs, the anchored map and the hud_aspect area.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "driver/wide.h"

static int gChecks, gFailures;

#define CHECK(cond)                                                                                          \
    do {                                                                                                     \
        gChecks++;                                                                                           \
        if (!(cond)) {                                                                                       \
            gFailures++;                                                                                     \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                           \
        }                                                                                                    \
    } while (0)

static const double kPi = 3.14159265358979323846;

/* The vertex the 2D funnel sees (RwD3DTLVertex, 0x20 bytes). */
typedef struct Vertex {
    float sx, sy, sz, rhw;
    unsigned color, specular;
    float u, v;
} Vertex;

/* applyFov (zrwcam.c) for a game fov in degrees: {tan(f) / 2, 3/4 of it}. */
static void fovWindow(double degrees, float window[2])
{
    const double half = tan(degrees * kPi / 180.0) * 0.5;

    window[0] = (float)half;
    window[1] = (float)(half * 0.75f);
}

static double hfov(const float window[2])
{
    return 2.0 * atan(window[0]) * 180.0 / kPi;
}

/* Bit equality of float arrays (the identity paths must not touch a bit). */
static int sameBits(const void *a, const void *b, size_t floats)
{
    for (size_t i = 0; i < floats; i++) {
        uint32_t x, y;

        memcpy(&x, (const float *)a + i, sizeof x);
        memcpy(&y, (const float *)b + i, sizeof y);
        if (x != y) {
            return 0;
        }
    }
    return 1;
}

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void testViewWindow(void)
{
    DrvWideConfig cfg = {.enabled = 1, .aspect = 1, .fovScale = 100};
    float w[2], orig[2];

    CHECK(drvWideIs43(640, 480) && drvWideIs43(1024, 768) && drvWideIs43(1600, 1200));
    CHECK(!drvWideIs43(1920, 1080) && !drvWideIs43(1280, 1024) && !drvWideIs43(3440, 1440));

    /* 4:3: nothing is touched (the original's floats stay). */
    fovWindow(60, w);
    memcpy(orig, w, sizeof w);
    CHECK(drvWideViewWindow(&cfg, 640, 480, w) == 0 && sameBits(w, orig, 2));
    CHECK(drvWideViewWindow(&cfg, 1024, 768, w) == 0 && sameBits(w, orig, 2));
    cfg.aspect = 2;
    CHECK(drvWideViewWindow(&cfg, 1600, 1200, w) == 0 && sameBits(w, orig, 2));
    cfg.aspect = 0;
    CHECK(drvWideViewWindow(&cfg, 1920, 1080, w) == 0 && sameBits(w, orig, 2));
    CHECK(drvWideViewWindow(&cfg, 0, 0, w) == 0);

    /* hor+: y kept, x = y * w / h; the table of W2 (f 60: 81.8 at 4:3, 98.2 at 16:9). */
    cfg.aspect = 1;
    CHECK(near(hfov(orig), 81.8, 0.05));
    CHECK(drvWideViewWindow(&cfg, 1920, 1080, w) == 1);
    CHECK(w[1] == orig[1] && near(w[0], orig[1] * 16.0 / 9.0, 1e-6));
    CHECK(near(hfov(w), 98.2, 0.05));
    /* 21:9, the user's 3440x1440: f 60 -> 114.4, f 30 -> 54.7, f 70 -> 135.8. */
    memcpy(w, orig, sizeof w);
    CHECK(drvWideViewWindow(&cfg, 3440, 1440, w) == 1 && near(hfov(w), 114.4, 0.05));
    fovWindow(30, w);
    CHECK(drvWideViewWindow(&cfg, 3440, 1440, w) == 1 && near(hfov(w), 54.7, 0.05));
    fovWindow(70, w);
    CHECK(drvWideViewWindow(&cfg, 3440, 1440, w) == 1 && near(hfov(w), 135.8, 0.05));
    /* 32:9: f 60 -> 133.2 (the table). */
    memcpy(w, orig, sizeof w);
    CHECK(drvWideViewWindow(&cfg, 3840, 1080, w) == 1 && near(hfov(w), 133.2, 0.05));
    /* 5:4 (vert+): x kept, y = x * h / w (more than the 4:3 view's y). */
    memcpy(w, orig, sizeof w);
    CHECK(drvWideViewWindow(&cfg, 1280, 1024, w) == 1);
    CHECK(w[0] == orig[0] && near(w[1], orig[0] * 0.8, 1e-6) && w[1] > orig[1]);
    /* vert-: x kept, y = x * 9 / 16 (less than the 4:3 view's y). */
    cfg.aspect = 2;
    memcpy(w, orig, sizeof w);
    CHECK(drvWideViewWindow(&cfg, 1920, 1080, w) == 1);
    CHECK(w[0] == orig[0] && near(w[1], orig[0] * 9.0 / 16.0, 1e-6) && w[1] < orig[1]);

    /* fov_scale: both axes, also at 4:3. */
    cfg.aspect = 1;
    cfg.fovScale = 80;
    memcpy(w, orig, sizeof w);
    CHECK(drvWideViewWindow(&cfg, 640, 480, w) == 1);
    CHECK(near(w[0], orig[0] * 0.8, 1e-6) && near(w[1], orig[1] * 0.8, 1e-6));
    memcpy(w, orig, sizeof w);
    CHECK(drvWideViewWindow(&cfg, 1920, 1080, w) == 1 && near(w[0] / w[1], 16.0 / 9.0, 1e-5));

    /* max_hfov: caps x at tan(max / 2), y scaled alike (aspect kept); no effect below the cap. */
    cfg.fovScale = 100;
    cfg.maxHfov = 120;
    memcpy(w, orig, sizeof w);
    CHECK(drvWideViewWindow(&cfg, 3440, 1440, w) == 1 && near(hfov(w), 114.4, 0.05)); /* below */
    fovWindow(70, w);
    CHECK(drvWideViewWindow(&cfg, 3440, 1440, w) == 1 && near(hfov(w), 120.0, 1e-3));
    CHECK(near(w[0] / w[1], 3440.0 / 1440.0, 1e-5));
    cfg.maxHfov = 100;
    memcpy(w, orig, sizeof w);
    CHECK(drvWideViewWindow(&cfg, 640, 480, w) == 0); /* 81.8 < 100 at 4:3: still the identity */
    CHECK(drvWideViewWindow(&cfg, 3840, 1080, w) == 1 && near(hfov(w), 100.0, 1e-3));
}

static void quad(Vertex v[4], float x0, float y0, float x1, float y1)
{
    memset(v, 0, 4 * sizeof *v);
    v[0].sx = x0, v[0].sy = y0;
    v[1].sx = x1, v[1].sy = y0;
    v[2].sx = x0, v[2].sy = y1;
    v[3].sx = x1, v[3].sy = y1;
    for (int i = 0; i < 4; i++) {
        v[i].sz = 0.5f, v[i].rhw = 2.0f, v[i].color = 0x80112233u, v[i].u = 0.25f, v[i].v = 0.75f;
    }
}

static int classify(const Vertex *v, int n, int textured, int w, int h)
{
    return drvWideClassify(&v->sx, sizeof *v, n, textured, w, h);
}

static void testClassify(void)
{
    const int w = 1920, h = 1080;
    const float sx = w / 640.0f, sy = h / 480.0f;
    Vertex v[6];

    /* Fade {640, 480} and dim rect: untextured over the full width -> cover. */
    quad(v, 0, 0, (float)w, (float)h);
    CHECK(classify(v, 4, 0, w, h) == kDrvWideCover);
    /* The same textured: a backdrop (picture screen, loading picture). */
    CHECK(classify(v, 4, 1, w, h) == kDrvWideBackdrop);
    /* Cut-scene bars {640, 60} at y 0 and 0x1a4. */
    quad(v, 0, 0, (float)w, 60 * sy);
    CHECK(classify(v, 4, 0, w, h) == kDrvWideCover);
    quad(v, 0, 0x1a4 * sy, (float)w, 480 * sy);
    CHECK(classify(v, 4, 0, w, h) == kDrvWideCover);
    /* A little off the edges (within two virtual pixels): still full width. */
    quad(v, -1.5f * sx, 0, w + 1.5f * sx, 10);
    CHECK(classify(v, 4, 0, w, h) == kDrvWideCover);
    quad(v, 1.5f * sx, 0, (float)w, 10);
    CHECK(classify(v, 4, 0, w, h) == kDrvWideCover);
    quad(v, 3 * sx, 0, (float)w, 10);
    CHECK(classify(v, 4, 0, w, h) == kDrvWideOther);
    /* A glyph, a HUD rect, the height meter, a menu item. */
    quad(v, 0x35 * sx, 400 * sy, 0x3d * sx, 412 * sy);
    CHECK(classify(v, 4, 1, w, h) == kDrvWideOther);
    quad(v, 0x224 * sx, 0x51 * sy, (0x224 + 0x26) * sx, (0x51 + 0x12a) * sy);
    CHECK(classify(v, 4, 0, w, h) == kDrvWideOther);
    /* Clipped quads: one edge only. */
    quad(v, 0, 100, 900, 200);
    CHECK(classify(v, 4, 0, w, h) == kDrvWideOther);
    quad(v, 1000, 100, (float)w, 200);
    CHECK(classify(v, 4, 1, w, h) == kDrvWideOther);
    /* A triangle list of a full-width quad (6 vertices). */
    quad(v, 0, 0, (float)w, (float)h);
    v[4] = v[1], v[5] = v[2];
    CHECK(classify(v, 6, 0, w, h) == kDrvWideCover);
    /* No vertices. */
    CHECK(classify(v, 0, 0, w, h) == kDrvWideOther);

    {
        DrvWideConfig cfg = {.enabled = 1, .aspect = 1, .fovScale = 100};

        CHECK(!drvWideMoves(&cfg, kDrvWideCover));
        CHECK(drvWideMoves(&cfg, kDrvWideBackdrop) && drvWideMoves(&cfg, kDrvWideOther));
        cfg.menus = 1;
        CHECK(!drvWideMoves(&cfg, kDrvWideBackdrop) && drvWideMoves(&cfg, kDrvWideOther));
        cfg.menus = 0;
        cfg.hud = 2;
        CHECK(drvWideMoves(&cfg, kDrvWideBackdrop) && !drvWideMoves(&cfg, kDrvWideOther));
        cfg.hud = 1; /* WS5 not yet: as 0 */
        CHECK(drvWideMoves(&cfg, kDrvWideOther));
        /* WS6: the blurred fill goes behind backdrops only, and only with menus = 2 (the backdrop still
         * moves). */
        cfg.hud = 0;
        CHECK(!drvWideBlurFill(&cfg, kDrvWideBackdrop));
        cfg.menus = 1;
        CHECK(!drvWideBlurFill(&cfg, kDrvWideBackdrop));
        cfg.menus = 2;
        CHECK(drvWideBlurFill(&cfg, kDrvWideBackdrop) && drvWideMoves(&cfg, kDrvWideBackdrop));
        CHECK(!drvWideBlurFill(&cfg, kDrvWideOther) && !drvWideBlurFill(&cfg, kDrvWideCover));
    }
}

static void testSharp(void)
{
    /* s = 1 is plain bilinear: the coordinate is unchanged. */
    for (int i = 0; i < 32; i++) {
        const float t = (float)i * 0.125f;

        CHECK(fabsf(drvWideSharpCoord(t, 1.0f) - t) < 1e-6f);
        CHECK(fabsf(drvWideSharpCoord(t, 0.25f) - t) < 1e-6f); /* below 1 counts as 1 */
    }
    /* s = 3: the centre of the texel is flat (nearest), the ramp is one output pixel (1/3 texel) wide at the
     * edges, and the coordinate is continuous and monotonic across texel borders. */
    CHECK(fabsf(drvWideSharpCoord(2.5f, 3.0f) - 2.5f) < 1e-6f);
    CHECK(fabsf(drvWideSharpCoord(2.2f, 3.0f) - 2.0f) <
          1e-6f); /* the texel's left half holds the left texel's centre */
    CHECK(fabsf(drvWideSharpCoord(2.8f, 3.0f) - 3.0f) < 1e-6f);
    CHECK(fabsf(drvWideSharpCoord(3.0f, 3.0f) - 3.0f) <
          1e-6f); /* on the border: halfway between two centres */
    {
        float prev = drvWideSharpCoord(0.0f, 3.0f);

        for (int i = 1; i < 500; i++) {
            const float t = (float)i * 0.01f;
            const float c = drvWideSharpCoord(t, 3.0f);

            CHECK(c >= prev - 1e-6f && c - prev < 0.04f); /* no jumps (slope s at most) */
            prev = c;
        }
    }
}

static void testMap(void)
{
    Vertex v[4], keep[4];

    /* 16:9: 0..1920 -> 240..1680, y unchanged, the other fields untouched. */
    quad(v, 0, 0, 1920, 1080);
    memcpy(keep, v, sizeof v);
    drvWideMap2D(&v->sx, sizeof *v, 4, 1920, 1080);
    CHECK(v[0].sx == 240.0f && v[1].sx == 1680.0f && v[0].sy == 0.0f && v[3].sy == 1080.0f);
    for (int i = 0; i < 4; i++) {
        CHECK(sameBits(&v[i].sy, &keep[i].sy, sizeof *v / sizeof(float) - 1));
    }
    /* The centre stays; 640-space x 0x13f/0x140 lands where it would in a 1440x1080 4:3 screen. */
    quad(v, 960, 0, 0x140 * 3.0f, 0);
    drvWideMap2D(&v->sx, sizeof *v, 2, 1920, 1080);
    CHECK(v[0].sx == 960.0f && near(v[1].sx, 240 + 0x140 * 1440.0 / 640.0, 1e-3));
    /* 21:9 (3440x1440): the 4:3 area is 1920 wide at 760. */
    quad(v, 0, 0, 3440, 1440);
    drvWideMap2D(&v->sx, sizeof *v, 4, 3440, 1440);
    CHECK(v[0].sx == 760.0f && v[1].sx == 2680.0f);
    /* 5:4: 0..1024 -> 32..992 vertically, x unchanged. */
    quad(v, 0, 0, 1280, 1024);
    drvWideMap2D(&v->sx, sizeof *v, 4, 1280, 1024);
    CHECK(v[0].sy == 32.0f && v[3].sy == 992.0f && v[0].sx == 0.0f && v[1].sx == 1280.0f);
    /* 4:3: bytes unchanged. */
    quad(v, 1.25f, 3.5f, 639.75f, 479.5f);
    memcpy(keep, v, sizeof v);
    drvWideMap2D(&v->sx, sizeof *v, 4, 640, 480);
    CHECK(sameBits(v, keep, 4 * (sizeof(Vertex) / sizeof(float))));

    {
        int32_t r[4] = {0, 0, 1920, 1080};

        drvWideMapRect(r, 1920, 1080);
        CHECK(r[0] == 240 && r[1] == 0 && r[2] == 1440 && r[3] == 1080);
        r[0] = 0, r[1] = 0, r[2] = 3440, r[3] = 1440;
        drvWideMapRect(r, 3440, 1440);
        CHECK(r[0] == 760 && r[2] == 1920 && r[3] == 1440);
        r[0] = 0, r[1] = 0, r[2] = 1280, r[3] = 1024;
        drvWideMapRect(r, 1280, 1024);
        CHECK(r[0] == 0 && r[1] == 32 && r[2] == 1280 && r[3] == 960);
        r[0] = 10, r[1] = 20, r[2] = 320, r[3] = 240;
        drvWideMapRect(r, 640, 480);
        CHECK(r[0] == 10 && r[1] == 20 && r[2] == 320 && r[3] == 240);
    }
}

static DrvWideVertex vtx(float x, float y, uint32_t color, float u)
{
    const DrvWideVertex v = {x, y, 0.5f, 2.0f, color, 0xff000000u, u, 0.5f};

    return v;
}

static void testClip(void)
{
    DrvWideVertex in[8], out[8], scratch[8];
    float lo = 0, hi = 0;
    int32_t n;
    float minX = 1e9f, maxX = -1e9f;

    CHECK(drvWideArea(1920, 1080, &lo, &hi) == 0 && lo == 240.0f && hi == 1680.0f);
    CHECK(drvWideArea(3440, 1440, &lo, &hi) == 0 && lo == 760.0f && hi == 2680.0f);
    CHECK(drvWideArea(1280, 1024, &lo, &hi) == 1 && lo == 32.0f && hi == 992.0f);
    CHECK(drvWideArea(640, 480, &lo, &hi) == -1);
    lo = 240.0f, hi = 1680.0f;

    /* A quad (fan order) half off the left edge: cut at 240, u and colour interpolated. */
    in[0] = vtx(100, 10, 0xff000000u, 0.0f);
    in[1] = vtx(500, 10, 0xff0000c8u, 1.0f);
    in[2] = vtx(500, 50, 0xff0000c8u, 1.0f);
    in[3] = vtx(100, 50, 0xff000000u, 0.0f);
    CHECK(drvWideOutside(in, 4, 0, lo, hi));
    n = drvWideClipFan(in, 4, 0, lo, hi, out, scratch);
    CHECK(n == 4);
    for (int i = 0; i < n; i++) {
        minX = out[i].sx < minX ? out[i].sx : minX;
        maxX = out[i].sx > maxX ? out[i].sx : maxX;
        if (out[i].sx == 240.0f) {
            CHECK(near(out[i].u, 0.35, 1e-6) && out[i].color == 0xff000046u && out[i].rhw == 2.0f);
            CHECK(out[i].specular == 0xff000000u && out[i].v == 0.5f);
        }
    }
    CHECK(minX == 240.0f && maxX == 500.0f);
    /* Wholly outside: nothing; wholly inside: unchanged (not even tested as outside). */
    in[1].sx = in[2].sx = 200;
    CHECK(drvWideClipFan(in, 4, 0, lo, hi, out, scratch) == 0);
    in[0].sx = in[3].sx = 300;
    in[1].sx = in[2].sx = 400;
    CHECK(!drvWideOutside(in, 4, 0, lo, hi));
    /* Over both edges: 240..1680. */
    in[0].sx = in[3].sx = 0;
    in[1].sx = in[2].sx = 1920;
    n = drvWideClipFan(in, 4, 0, lo, hi, out, scratch);
    CHECK(n == 4);
    minX = 1e9f, maxX = -1e9f;
    for (int i = 0; i < n; i++) {
        minX = out[i].sx < minX ? out[i].sx : minX;
        maxX = out[i].sx > maxX ? out[i].sx : maxX;
    }
    CHECK(minX == 240.0f && maxX == 1680.0f);
    /* A triangle with one corner outside becomes a quad. */
    in[0] = vtx(1600, 0, 0, 0), in[1] = vtx(1800, 100, 0, 0), in[2] = vtx(1600, 200, 0, 0);
    CHECK(drvWideClipFan(in, 3, 0, lo, hi, out, scratch) == 4);
    /* Vertical (5:4): clip y to 32..992. */
    in[0] = vtx(0, 0, 0, 0), in[1] = vtx(100, 0, 0, 0), in[2] = vtx(100, 100, 0, 0),
    in[3] = vtx(0, 100, 0, 0);
    n = drvWideClipFan(in, 4, 1, 32.0f, 992.0f, out, scratch);
    CHECK(n == 4);
    for (int i = 0; i < n; i++) {
        CHECK(out[i].sy >= 32.0f && out[i].sy <= 100.0f);
    }
    /* Lines: inside kept, crossing cut, outside dropped. */
    in[0] = vtx(300, 0, 0, 0), in[1] = vtx(400, 0, 0, 0);
    in[2] = vtx(100, 5, 0, 0), in[3] = vtx(300, 5, 0, 0);
    in[4] = vtx(1700, 9, 0, 0), in[5] = vtx(1900, 9, 0, 0);
    in[6] = vtx(1700, 9, 0, 0), in[7] = vtx(1000, 9, 0, 0);
    n = drvWideClipLines(in, 8, 0, lo, hi, out);
    CHECK(n == 6);
    CHECK(out[0].sx == 300.0f && out[1].sx == 400.0f);
    CHECK(out[2].sx == 240.0f && out[3].sx == 300.0f);
    CHECK(out[4].sx == 1680.0f && out[5].sx == 1000.0f);
}

static void testPresent(void)
{
    uint32_t r[4];

    /* 0: letterbox, linear (the vanilla present). */
    CHECK(drvWidePresentRect(0, 640, 480, 2560, 1440, r) == 0);
    CHECK(r[0] == 320 && r[1] == 0 && r[2] == 1920 && r[3] == 1440);
    CHECK(drvWidePresentRect(0, 640, 480, 3440, 1440, r) == 0 && r[0] == 760 && r[2] == 1920);
    CHECK(drvWidePresentRect(0, 1920, 1080, 1920, 1080, r) == 0 && r[0] == 0 && r[2] == 1920);
    /* 1: the same rectangle, nearest. */
    CHECK(drvWidePresentRect(1, 640, 480, 2560, 1440, r) == 1 && r[0] == 320 && r[2] == 1920);
    /* 2: integer factor: 1440 / 480 = 3 exactly; 1080 / 480 -> 2 with a border. */
    CHECK(drvWidePresentRect(2, 640, 480, 3440, 1440, r) == 1);
    CHECK(r[0] == 760 && r[1] == 0 && r[2] == 1920 && r[3] == 1440);
    CHECK(drvWidePresentRect(2, 640, 480, 1920, 1080, r) == 1);
    CHECK(r[0] == 320 && r[1] == 60 && r[2] == 1280 && r[3] == 960);
    /* 2 when not even 1x fits: the linear letterbox. */
    CHECK(drvWidePresentRect(2, 1920, 1080, 1000, 700, r) == 0 && r[2] == 1000 && r[3] == 563);
}

/* ---- HUD anchoring (WS5) ---- */

static void put16(uint8_t *p, unsigned off, int v)
{
    p[off] = (uint8_t)v;
    p[off + 1] = (uint8_t)((unsigned)v >> 8);
}

/* A glyph or sprite packet as zoverAddText / zoverAddSprite / ngifDrawTextGlyph write it (FT4, code 0x2c). */
static void ft4(uint8_t *p, int x0, int y0, int x1, int y1)
{
    p[7] = 0x2c;
    p[0x34] = 0x80; /* flag word 0x7f80: no float flag */
    p[0x35] = 0x7f;
    put16(p, 8, x0);
    put16(p, 10, y0);
    put16(p, 0x12, x1);
    put16(p, 0x14, y0);
    put16(p, 0x1c, x0);
    put16(p, 0x1e, y1);
    put16(p, 0x26, x1);
    put16(p, 0x28, y1);
}

/* A rectangle as zoverAddRect writes it (F4, code 0x28). */
static void f4(uint8_t *p, int x, int y, int w, int h)
{
    p[7] = 0x28;
    p[0x1c] = 0x80;
    p[0x1d] = 0x7f;
    put16(p, 8, x);
    put16(p, 10, y);
    put16(p, 0xc, x + w);
    put16(p, 0xe, y);
    put16(p, 0x10, x);
    put16(p, 0x12, y + h);
    put16(p, 0x14, x + w);
    put16(p, 0x16, y + h);
}

static uint8_t gGlyphs[128][0xa0], gShapes[16][0xa0];

/* A text of `count` glyphs of width `gw` at x, y as zoverAddText chains it from pool item `first` on. */
static void addText(int first, int count, int x, int y, int gw, int h)
{
    for (int i = 0; i < count; i++) {
        uint8_t *const it = gGlyphs[first + i];

        ft4(it, x, y, x + gw, y + h);
        it[0x9c] = 5;
        put16(it, 0x9e, i + 1 < count ? 0x1000 | (first + i + 1) : 0xffff);
        x += gw + 1;
    }
}

static void testHudAnchor(void)
{
    int16_t box[4];
    uint8_t p[0xa0] = {0};

    /* Timer at 0x35, score right-aligned at 0x24b, centred texts (ovlCentreX 0x13f / 0x140, odd widths). */
    CHECK(drvWideHudAnchorOf(0x35, 0x35 + 60) == kDrvWideAnchorLeft);
    CHECK(drvWideHudAnchorOf(0x24b - 14, 0x24b) == kDrvWideAnchorRight);
    CHECK(drvWideHudAnchorOf(0x13f - 101, 0x13f - 101 + 203) == kDrvWideAnchorCentre);
    CHECK(drvWideHudAnchorOf(0x140 - 240, 0x140 + 240) == kDrvWideAnchorCentre);
    /* A long text at the left edge (the flash message at 0x19) stays left; the height meter, the
     * indicator at 0x240 and the icon at 0x1b1 go right. */
    CHECK(drvWideHudAnchorOf(0x19, 0x19 + 500) == kDrvWideAnchorLeft);
    CHECK(drvWideHudAnchorOf(0x224, 0x224 + 0x26) == kDrvWideAnchorRight);
    CHECK(drvWideHudAnchorOf(0x240 - 8, 0x240 + 8) == kDrvWideAnchorRight);
    CHECK(drvWideHudAnchorOf(0x1b1, 0x1b1 + 32) == kDrvWideAnchorRight);
    /* Just outside the centre band (8 virtual pixels). */
    CHECK(drvWideHudAnchorOf(300, 356) == kDrvWideAnchorCentre);
    CHECK(drvWideHudAnchorOf(300, 358) == kDrvWideAnchorRight);
    CHECK(drvWideHudAnchorOf(282, 340) == kDrvWideAnchorLeft);

    /* Packet boxes: FT4 and F4 as the pools hold them; a 3D list polygon (float flag) is not 2D. */
    ft4(p, 10, 20, 30, 45);
    CHECK(drvWidePacketBox(p, box) && box[0] == 10 && box[1] == 20 && box[2] == 30 && box[3] == 45);
    memset(p, 0, sizeof p);
    f4(p, -40, 400, 30, 2);
    CHECK(drvWidePacketBox(p, box) && box[0] == -40 && box[1] == 400 && box[2] == -10 && box[3] == 402);
    p[0x1c] |= 0x20;
    CHECK(!drvWidePacketBox(p, box));
    memset(p, 0, sizeof p);
    p[7] = 0xe1; /* a draw mode packet */
    CHECK(!drvWidePacketBox(p, box));
}

static void testHudPools(void)
{
    uint8_t ga[128], sa[16];

    memset(gGlyphs, 0, sizeof gGlyphs);
    memset(gShapes, 0, sizeof gShapes);
    /* Timer "1:57" (left), score "1234" right-aligned at 0x24b, a centred combo line that spans all three
     * thirds, written in pool order with gaps between them (as allocations and frees leave them). */
    addText(0, 4, 0x35, 400, 12, 26);
    addText(10, 4, 0x24b - 4 * 13 + 1, 400, 12, 26);
    addText(20, 30, 0x140 - (30 * 15 - 1) / 2, 0x132, 14, 26);
    /* A hint line centred at y 0x83, with a one-glyph button text inside it at its left end (left third on
     * its own) and the chain stored backwards in the pool (a chain is followed, not the pool order). */
    for (int i = 0; i < 20; i++) {
        uint8_t *const it = gGlyphs[79 - i];
        const int x = 0x140 - (20 * 21 - 1) / 2 + i * 21;

        ft4(it, x, 0x83, x + 20, 0x83 + 26);
        it[0x9c] = 5;
        put16(it, 0x9e, i + 1 < 20 ? 0x1000 | (79 - i - 1) : 0xffff);
    }
    addText(90, 1, 0x140 - (20 * 21 - 1) / 2 + 2 * 21, 0x83 - 2, 24, 30);
    /* A button glyph of the same size somewhere else (not inside a wider text): its own anchor. */
    addText(95, 1, 0x40, 0x100, 24, 30);
    /* Height meter (right), the meter's marker, and a sprite inside the combo line's box. */
    f4(gShapes[0], 0x224, 0x51, 0x26, 0x12a);
    gShapes[0][0x9c] = 3;
    f4(gShapes[3], 0x213, 0x17a, 0x10, 2);
    gShapes[3][0x9c] = 3;
    ft4(gShapes[5], 0x60, 0x134, 0x80, 0x140);
    gShapes[5][0x9c] = 2;

    drvWideHudPools(&gGlyphs[0][0], 128, &gShapes[0][0], 16, ga, sa);
    for (int i = 0; i < 4; i++) {
        CHECK(ga[i] == kDrvWideAnchorLeft);
        CHECK(ga[10 + i] == kDrvWideAnchorRight);
    }
    for (int i = 0; i < 30; i++) {
        CHECK(ga[20 + i] == kDrvWideAnchorCentre); /* per element, not per glyph */
    }
    CHECK(ga[60] == kDrvWideAnchorCentre && ga[79] == kDrvWideAnchorCentre);
    CHECK(ga[90] == kDrvWideAnchorCentre); /* inside the hint line */
    CHECK(ga[95] == kDrvWideAnchorLeft);
    CHECK(ga[5] == kDrvWideAnchorNone && ga[127] == kDrvWideAnchorNone);
    CHECK(sa[0] == kDrvWideAnchorRight && sa[3] == kDrvWideAnchorRight);
    CHECK(sa[5] == kDrvWideAnchorCentre); /* inside the combo line */
    CHECK(sa[1] == kDrvWideAnchorNone);
    /* A cycle without a head does not hang and gets per-glyph anchors. */
    put16(gGlyphs[3], 0x9e, 0x1000);
    drvWideHudPools(&gGlyphs[0][0], 128, &gShapes[0][0], 16, ga, sa);
    CHECK(ga[0] == kDrvWideAnchorLeft && ga[3] == kDrvWideAnchorLeft);
}

static void testGlyphRun(void)
{
    /* zoverDrawText into the 3D list: 0x9c byte packets back to back; draw order is reversed. */
    static uint8_t buf[12][0x9c];
    const uint8_t *order[12];
    int x = 0x35;

    memset(buf, 0, sizeof buf);
    for (int i = 0; i < 5; i++) { /* "TO" style text: 5 glyphs */
        ft4(buf[i], x, 0x100, x + 9, 0x11a);
        x += 10;
    }
    x = 0x200; /* the next call: another text on the same lines */
    for (int i = 5; i < 8; i++) {
        ft4(buf[i], x, 0x100, x + 9, 0x11a);
        x += 10;
    }
    f4(buf[8], 0x224, 0x80, 0x13, 1); /* zoverDrawRect */
    for (int i = 0; i < 9; i++) {
        order[i] = buf[8 - i];
    }
    CHECK(drvWideGlyphRun(order, 9) == 1);     /* the rect alone */
    CHECK(drvWideGlyphRun(order + 1, 8) == 3); /* the second text */
    CHECK(drvWideGlyphRun(order + 4, 5) == 5); /* the first text */
    CHECK(drvWideGlyphRun(order + 4, 3) == 3); /* bounded by n */
    CHECK(drvWideGlyphRun(order, 0) == 0);
    /* Two texts that touch with the one pixel gap but on other lines are two runs. */
    ft4(buf[7], 0x200 + 20, 0x120, 0x200 + 29, 0x13a);
    CHECK(drvWideGlyphRun(order + 1, 8) == 1);
    /* Forward order (written in draw order) is accepted too. */
    {
        const uint8_t *fwd[5] = {buf[0], buf[1], buf[2], buf[3], buf[4]};

        CHECK(drvWideGlyphRun(fwd, 5) == 5);
    }
}

static void testMapHud(void)
{
    Vertex v[2] = {{0}, {0}};
    float lo, hi;

    /* 3440x1440, hud_aspect 178: area 2563 wide -> [438, 3002]; the 4:3 part is 1920 wide. */
    CHECK(drvWideHudArea(3440, 1440, 178, &lo, &hi) && lo == 438.0f && hi == 3002.0f);
    CHECK(drvWideHudArea(3440, 1440, 0, &lo, &hi) && lo == 0.0f && hi == 3440.0f);
    CHECK(drvWideHudArea(3440, 1440, 100, &lo, &hi) && lo == 760.0f && hi == 2680.0f); /* at least 4:3 */
    CHECK(drvWideHudArea(1920, 1080, 178, &lo, &hi) && lo == 0.0f &&
          hi == 1920.0f); /* 16:9: the full width */
    CHECK(!drvWideHudArea(1024, 768, 178, &lo, &hi));
    CHECK(!drvWideHudArea(1280, 1024, 178, &lo, &hi));

    /* Left: virtual 0 (stretched 0) lands on the area's left edge; virtual 640 at left + 1920. */
    v[0].sx = 0.0f;
    v[1].sx = 3440.0f * 0x35 / 640.0f;
    CHECK(drvWideMapHud(&v[0].sx, sizeof v[0], 2, 3440, 1440, kDrvWideAnchorLeft, 178, &lo, &hi) == 0);
    CHECK(v[0].sx == 438.0f && fabsf(v[1].sx - (438.0f + 0x35 * 3.0f)) < 0.01f);
    CHECK(lo == 438.0f && hi == 438.0f + 1920.0f);
    /* Right: virtual 640 on the right edge, the score's right end 0x24b keeps its margin (53 * 3 px). */
    v[0].sx = 3440.0f;
    v[1].sx = 3440.0f * 0x24b / 640.0f;
    CHECK(drvWideMapHud(&v[0].sx, sizeof v[0], 2, 3440, 1440, kDrvWideAnchorRight, 178, &lo, &hi) == 0);
    CHECK(v[0].sx == 3002.0f && fabsf(v[1].sx - (3002.0f - (640 - 0x24b) * 3.0f)) < 0.01f);
    CHECK(lo == 3002.0f - 1920.0f && hi == 3002.0f);
    /* hud_aspect 0 at 21:9: the screen's edges. */
    v[0].sx = 0.0f;
    drvWideMapHud(&v[0].sx, sizeof v[0], 1, 3440, 1440, kDrvWideAnchorLeft, 0, &lo, &hi);
    CHECK(v[0].sx == 0.0f);
    /* 16:9: left at 0, right at 1920, with the 4:3 scale (2.25 per virtual pixel). */
    v[0].sx = 1920.0f * 0x35 / 640.0f;
    drvWideMapHud(&v[0].sx, sizeof v[0], 1, 1920, 1080, kDrvWideAnchorLeft, 178, &lo, &hi);
    CHECK(fabsf(v[0].sx - 0x35 * 2.25f) < 0.01f);
    /* Centre and None are drvWideMap2D bit for bit, with the 4:3 area as the clip. */
    {
        Vertex a = {0}, b = {0};
        float lo2, hi2;

        a.sx = b.sx = 1234.5f;
        a.sy = b.sy = 77.0f;
        CHECK(drvWideMapHud(&a.sx, sizeof a, 1, 3440, 1440, kDrvWideAnchorCentre, 178, &lo, &hi) == 0);
        drvWideMap2D(&b.sx, sizeof b, 1, 3440, 1440);
        CHECK(sameBits(&a, &b, 8) && drvWideArea(3440, 1440, &lo2, &hi2) == 0 && lo == lo2 && hi == hi2);
        a.sx = 100.0f; /* 4:3: untouched */
        CHECK(drvWideMapHud(&a.sx, sizeof a, 1, 1024, 768, kDrvWideAnchorLeft, 178, &lo, &hi) == -1);
        CHECK(a.sx == 100.0f);
    }
}

int main(void)
{
    testHudAnchor();
    testHudPools();
    testGlyphRun();
    testMapHud();
    testViewWindow();
    testClassify();
    testMap();
    testClip();
    testPresent();
    testSharp();
    printf("bmxwidetest: %d checks, %d failures\n", gChecks, gFailures);
    return gFailures != 0;
}
