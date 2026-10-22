/*
 * Tests for zrwPrim.c: edge interpolation, polygon clipping and the primitive list wrappers. The drawing
 * routine and the camera lookup are stubbed; the draw stub logs every polygon it is given, vertex by vertex
 * (the clipping leaves z of a clipped vertex unset, so z is only logged when it carries the marker of the
 * test's own vertices).
 */
#include "dt.h"
#include "dt_ngif.h"
#include "zge/com/zglrw/zrwprim.h"

#define kZMarker 0xc0de0000u

static void BMX_CDECL stubDraw(uint32_t flags, const ZrwPrimPoly *polys, int count, uint32_t arg,
                               uint32_t arg2)
{
    ngLog(0x48d170);
    ngLog(flags);
    ngLog(dt_canon((uint32_t)(uintptr_t)polys));
    ngLog((uint32_t)count);
    ngLog(arg);
    ngLog(arg2);
    for (int i = 0; i < count; i++) {
        ngLog(polys[i].texture);
        ngLog(polys[i].count);
        ngLog(dt_canon((uint32_t)(uintptr_t)polys[i].vertices));
        for (uint32_t j = 0; j < polys[i].count && j < 40; j++) {
            const ZrwPrimVertex *v = polys[i].vertices[j];
            uint32_t zbits;

            ngLog(dt_canon((uint32_t)(uintptr_t)v));
            ngLogFloat(v->x);
            ngLogFloat(v->y);
            ngLogFloat(v->u);
            ngLogFloat(v->v);
            ngLog(*(const uint32_t *)v->color);
            __builtin_memcpy(&zbits, &v->z, 4);
            ngLog((zbits >> 16) == (kZMarker >> 16) ? zbits : 0);
        }
    }
}

static uint8_t *gCamera;

static uint8_t *BMX_CDECL stubCamera(void)
{
    ngLog(0x493070);
    return gCamera;
}

/*
 * Zeroes the stack below the setup's frame: the callee's uninitialised locals (the z of a clipped vertex)
 * would otherwise show what the setup left there, which is different for the original and for the port.
 */
static void scrub_stack(void)
{
    volatile uint8_t junk[8192];

    for (uint32_t i = 0; i < sizeof junk; i++) {
        junk[i] = 0;
    }
}

static void begin(DtCtx *c)
{
    ngBegin(c, 0);
    dt_stub(0x0048d170, NULL, (void *)stubDraw);
    dt_stub(0x00493070, NULL, (void *)stubCamera);
}

static ZrwPrimVertex *make_vertex(float lo, float hi)
{
    ZrwPrimVertex *v = dt_alloc(sizeof *v);
    uint32_t z = kZMarker | (dt_u32() & 0xffff);

    v->x = dt_float_edgy(lo, hi);
    v->y = dt_float_edgy(lo, hi);
    __builtin_memcpy(&v->z, &z, 4);
    v->pad0c = 0;
    v->u = dt_float(0.0f, 1.0f);
    v->v = dt_float(0.0f, 1.0f);
    for (int i = 0; i < 4; i++) {
        v->color[i] = (uint8_t)dt_u32();
    }
    return v;
}

/* ---- interpolation --------------------------------------------------------------------------------- */

static void setup_lerp(DtCtx *c)
{
    ZrwPrimVertex *a;
    ZrwPrimVertex *b;
    ZrwPrimVertex *out = dt_alloc(sizeof *out);

    begin(c);
    a = make_vertex(-100.0f, 100.0f);
    b = make_vertex(-100.0f, 100.0f);
    if (dt_u32() % 4 == 0) { /* the truncated coordinates of both ends agree */
        b->x = a->x + dt_float(-0.9f, 0.9f);
        b->y = a->y + dt_float(-0.9f, 0.9f);
    }
    dt_fill(out, sizeof *out);
    DT_ARG_FLOAT(c, 0, dt_float_edgy(-120.0f, 120.0f));
    DT_ARG_PTR(c, 1, a);
    DT_ARG_PTR(c, 2, b);
    DT_ARG_PTR(c, 3, out);
}

/* ---- clipping -------------------------------------------------------------------------------------- */

static ZMatrix *random_matrix(void)
{
    ZMatrix *m = dt_alloc(sizeof *m);
    float *f = (float *)m;

    dt_fill(m, sizeof *m);
    for (int i = 0; i < 16; i++) {
        f[i] = dt_float(-2.0f, 2.0f);
    }
    return m;
}

static float *random_rect(void)
{
    float *r = dt_alloc(4 * sizeof *r);

    r[0] = dt_float(0.0f, 60.0f);
    r[1] = dt_float(0.0f, 60.0f);
    r[2] = r[0] + dt_float(1.0f, 80.0f);
    r[3] = r[1] + dt_float(1.0f, 80.0f);
    return r;
}

static ZrwPrimVertex **random_vertices(int n, float lo, float hi)
{
    ZrwPrimVertex **list = (ZrwPrimVertex **)dt_alloc((uint32_t)(n > 0 ? n : 1) * sizeof *list);

    for (int i = 0; i < n; i++) {
        list[i] = make_vertex(lo, hi);
    }
    return list;
}

/*
 * A convex polygon: points of a 16-gon in order, stretched and moved. The original keeps its clip vertices in
 * a 15 vertex stack buffer, which a self intersecting polygon would overflow. With `snap`, one vertex is
 * moved onto an edge of the clip rectangle.
 */
static ZrwPrimVertex **convex_polygon(int n, const float *rect, int snap)
{
    static const float kUnit[16][2] = {
        {1.0f, 0.0f},
        {0.92387953f, 0.38268343f},
        {0.70710678f, 0.70710678f},
        {0.38268343f, 0.92387953f},
        {0.0f, 1.0f},
        {-0.38268343f, 0.92387953f},
        {-0.70710678f, 0.70710678f},
        {-0.92387953f, 0.38268343f},
        {-1.0f, 0.0f},
        {-0.92387953f, -0.38268343f},
        {-0.70710678f, -0.70710678f},
        {-0.38268343f, -0.92387953f},
        {0.0f, -1.0f},
        {0.38268343f, -0.92387953f},
        {0.70710678f, -0.70710678f},
        {0.92387953f, -0.38268343f},
    };
    ZrwPrimVertex **list = random_vertices(n, 0.0f, 1.0f);
    const float cx = dt_float(-20.0f, 120.0f);
    const float cy = dt_float(-20.0f, 120.0f);
    const float rx = dt_float(5.0f, 80.0f);
    const float ry = dt_float(5.0f, 80.0f);
    int taken[16] = {0};
    int chosen[8];
    const int direction = (dt_u32() & 1) ? 1 : -1;

    for (int i = 0; i < n; i++) {
        int k;

        do {
            k = dt_range(0, 15);
        } while (taken[k]);
        taken[k] = 1;
        chosen[i] = k;
    }
    for (int i = 0; i < n; i++) { /* sort the indices */
        for (int j = i + 1; j < n; j++) {
            if (chosen[j] < chosen[i]) {
                const int t = chosen[i];

                chosen[i] = chosen[j];
                chosen[j] = t;
            }
        }
    }
    for (int i = 0; i < n; i++) {
        const int k = chosen[direction > 0 ? i : n - 1 - i];

        list[i]->x = cx + rx * kUnit[k][0];
        list[i]->y = cy + ry * kUnit[k][1];
    }
    if (snap && n > 0) {
        const int v = dt_range(0, n - 1);

        if (dt_u32() & 1) {
            list[v]->y = rect[(dt_u32() & 1) ? 1 : 3];
        } else {
            list[v]->x = rect[(dt_u32() & 1) ? 0 : 2];
        }
    }
    return list;
}

static void setup_clip(DtCtx *c)
{
    int n;
    float *rect;

    begin(c);
    n = (dt_u32() % 8 == 0) ? dt_range(0, 2) : dt_range(3, 6);
    rect = random_rect();
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, convex_polygon(n, rect, n <= 4 && dt_u32() % 5 == 0));
    DT_ARG_INT(c, 3, n);
    DT_ARG_PTR(c, 4, (dt_u32() % 3 == 0) ? NULL : random_matrix());
    DT_ARG_PTR(c, 5, rect);
    scrub_stack();
}

/* ---- initialisation -------------------------------------------------------------------------------- */

static void setup_init(DtCtx *c)
{
    ngBegin(c, 1);
    dt_fill(&gZrwPrim, sizeof gZrwPrim);
    if (dt_u32() & 1) {
        gZrwPrim.initialized = 0;
    }
}

static void setup_shutdown(DtCtx *c)
{
    ngBegin(c, 0);
    dt_fill(&gZrwPrim, sizeof gZrwPrim);
    gZrwPrim.initialized = (dt_u32() & 1);
    gZrwPrim.vertices = (dt_u32() % 3 == 0) ? NULL : dt_zmemalloc(0x40);
    gZrwPrim.indices = (dt_u32() % 3 == 0) ? NULL : dt_zmemalloc(0x40);
}

/* ---- lists ----------------------------------------------------------------------------------------- */

static void setup_triangles(DtCtx *c)
{
    int n;
    ZrwPrimTriangle *tris;

    begin(c);
    n = dt_range(1, 4);
    tris = dt_alloc((uint32_t)n * sizeof *tris);
    for (int i = 0; i < n; i++) {
        tris[i].texture = dt_u32();
        for (int j = 0; j < 3; j++) {
            tris[i].vertices[j] = make_vertex(0.0f, 100.0f);
        }
    }
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_PTR(c, 1, tris);
    DT_ARG_INT(c, 2, n);
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_strips(DtCtx *c)
{
    int n;
    ZrwPrimPoly *strips;

    begin(c);
    n = dt_range(1, 3);
    strips = dt_alloc((uint32_t)n * sizeof *strips);
    for (int i = 0; i < n; i++) {
        const int count = dt_range(0, 10);

        strips[i].texture = dt_u32();
        strips[i].vertices = random_vertices(count, 0.0f, 100.0f);
        strips[i].count = (uint32_t)count;
    }
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_PTR(c, 1, strips);
    DT_ARG_INT(c, 2, n);
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_quads(DtCtx *c)
{
    const int n = dt_range(1, 3);
    ZrwPrimQuad *quads = dt_alloc((uint32_t)n * sizeof *quads);
    uint32_t flags = dt_u32();

    float *rect = random_rect();

    begin(c);
    for (int i = 0; i < n; i++) {
        ZrwPrimVertex **polygon = convex_polygon(4, rect, dt_u32() % 5 == 0);

        quads[i].texture = dt_u32();
        for (int j = 0; j < 4; j++) {
            quads[i].vertices[j] = polygon[j];
        }
    }
    gCamera = dt_alloc(0xa8);
    dt_fill(gCamera, 0x94);
    __builtin_memcpy(gCamera + 0x94, rect, 16);
    if (dt_u32() % 3 == 0) {
        flags &= ~0x2000000u;
    } else {
        flags |= 0x2000000u;
    }
    DT_ARG_INT(c, 0, flags);
    DT_ARG_PTR(c, 1, quads);
    DT_ARG_INT(c, 2, n);
    DT_ARG_PTR(c, 3, (dt_u32() % 3 == 0) ? NULL : random_matrix());
    scrub_stack();
}

DT_CASES(zrwprim_cases) = {
    {"zrwprimLerpAtX", 0x0048c620, zrwprimLerpAtX, 4, DT_RET_VOID, setup_lerp, 2000, 0},
    {"zrwprimLerpAtY", 0x0048c730, zrwprimLerpAtY, 4, DT_RET_VOID, setup_lerp, 2000, 0},
    {"zrwprimClipPoly", 0x0048c840, zrwprimClipPoly, 6, DT_RET_INT, setup_clip, 3000, 0},
    {"zrwPrimSystemInitialize", 0x0048cc30, zrwPrimSystemInitialize, 0, DT_RET_INT, setup_init, 50, 0},
    {"zrwPrimSystemShutdown", 0x0048ccb0, zrwPrimSystemShutdown, 0, DT_RET_INT, setup_shutdown, 50, 0},
    {"zrwprimDrawTriangles", 0x0048d120, zrwprimDrawTriangles, 4, DT_RET_VOID, setup_triangles, 100, 0},
    {"zrwprimDrawStrips", 0x0048d5a0, zrwprimDrawStrips, 4, DT_RET_VOID, setup_strips, 300, 0},
    {"zrwprimDrawQuads", 0x0048d660, zrwprimDrawQuads, 4, DT_RET_VOID, setup_quads, 500, 0},
};
