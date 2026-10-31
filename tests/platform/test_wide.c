/*
 * bmxwidetest: the widescreen maths (src/driver/widemath.c, docs/WIDESCREEN.md W10.3) on synthetic input:
 * the view window (hor+, vert+, vert-, the 4:3 identity, fov_scale, max_hfov), the 2D classifier (fade, bars,
 * dim rect, backdrop, glyphs, clipped quads), the 2D and rectangle maps, the clip to the 4:3 area and the
 * present rectangle.
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

int main(void)
{
    testViewWindow();
    testClassify();
    testMap();
    testClip();
    testPresent();
    printf("bmxwidetest: %d checks, %d failures\n", gChecks, gFailures);
    return gFailures != 0;
}
