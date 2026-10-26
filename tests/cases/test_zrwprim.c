/*
 * Tests for zrwPrim.c: edge interpolation, polygon clipping and the primitive list wrappers. The drawing
 * routine and the camera lookup are stubbed; the draw stub logs every polygon it is given, vertex by vertex
 * (the clipping leaves z of a clipped vertex unset, so z is only logged when it carries the marker of the
 * test's own vertices).
 */
#include "dt.h"
#include "dt_ngif.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/com/zglrw/zrwprim.h"

#define kZMarker 0xc0de0000u

static void BMX_CDECL stubDraw(uint32_t flags, const ZrwPrimPoly *polys, int count, uint32_t arg)
{
    ngLog(0x48d170);
    ngLog(flags);
    ngLog(dt_canon((uint32_t)(uintptr_t)polys));
    ngLog((uint32_t)count);
    ngLog(arg);
    /* the fifth word the original callers push (0) is never read by the routine */
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
    gZrwPrim.im3d = (dt_u32() % 3 == 0) ? NULL : dt_zmemalloc(0x40);
    gZrwPrim.im2d = (dt_u32() % 3 == 0) ? NULL : dt_zmemalloc(0x40);
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

/* ---- drawing routines ------------------------------------------------------------------------------ */

static float gNear;
static float gNearZ;
static int gTransformOk;
static ZrwCamera *gPrimCamera;

static float BMX_CDECL stubNear(void *camera)
{
    ngLog(0x4de610);
    ngLogPtr(camera);
    return gNear;
}

static float BMX_CDECL stubNearZ(void)
{
    ngLog(0x4db800);
    return gNearZ;
}

static int BMX_CDECL stubIm2DRender(int type, const float *vertices, int count)
{
    ngLog(0x4db920);
    ngLog((uint32_t)type);
    ngLogPtr(vertices);
    ngLog((uint32_t)count);
    for (int i = 0; i < count && i < 64; i++) {
        const float *v = vertices + 8 * i;

        for (int k = 0; k < 8; k++) {
            if (k != 5) { /* the word after the colour is never written */
                ngLogFloat(v[k]);
            }
        }
    }
    return 1;
}

static int BMX_CDECL stubIm3DTransform(const float *vertices, int count, const void *matrix, int flags)
{
    ngLog(0x4df0b0);
    ngLogPtr(vertices);
    ngLog((uint32_t)count);
    ngLogPtr(matrix);
    ngLog((uint32_t)flags);
    for (int i = 0; i < count && i < 64; i++) {
        const float *v = vertices + 9 * i;

        ngLogFloat(v[0]);
        ngLogFloat(v[1]);
        ngLogFloat(v[2]);
        ngLogFloat(v[6]);
        ngLogFloat(v[7]);
        ngLogFloat(v[8]);
    }
    return gTransformOk;
}

static int BMX_CDECL stubIm3DRender(int type)
{
    ngLog(0x4def70);
    ngLog((uint32_t)type);
    return 1;
}

static int BMX_CDECL stubIm3DEnd(void)
{
    ngLog(0x4df090);
    return 1;
}

/* Stand-in for RenderWare's vector transform: a fixed permutation and offset of each vector. */
static void *BMX_CDECL stubTransformVectors(ZVec3 *out, const ZVec3 *in, int count, const ZMatrix *m)
{
    ngLog(0x4d3ae0);
    ngLogPtr(out);
    ngLogPtr(in);
    ngLog((uint32_t)count);
    ngLogPtr(m);
    for (int i = 0; i < count; i++) {
        const ZVec3 t = in[i];

        ngLogVec(&t);
        out[i].x = t.y * 0.5f + m->right.x;
        out[i].y = t.z - m->up.y;
        out[i].z = t.x;
    }
    return out;
}

/* The raster of a test material is the word it points to. */
static void *BMX_CDECL stubGetRaster(const uint32_t *material, uint32_t index)
{
    ngLog(0x489b40);
    ngLogPtr(material);
    ngLog(index);
    return (void *)(uintptr_t)*material;
}

static void BMX_CDECL stubStateHook(uint32_t state, void *raster)
{
    ngLog(0x561490);
    ngLog(state);
    ngLogPtr(raster);
}

static ZrwCamera *BMX_CDECL stubCurrentCamera(void)
{
    ngLog(0x493070);
    return gPrimCamera;
}

#define gTestFrames      ORIG_VAR(0x00697744, int, gTestFrames)
#define gTestScale       ORIG_VAR(0x00697720, float, gTestScale)
#define gTestOrigin      ORIG_VAR(0x006976cc, ZVec3, gTestOrigin)
#define gTestRwCamera    ORIG_VAR(0x006976f4, void *, gTestRwCamera)
#define gTestRenderFlags ORIG_VAR(0x00697778, uint32_t, gTestRenderFlags)
#define gTestRenderExtra ORIG_VAR(0x0069777c, uint32_t, gTestRenderExtra)
#define gTestStateHook   ORIG_VAR(0x00561490, void *, gTestStateHook)

static ZrwPrimVertex *draw_vertex(void)
{
    ZrwPrimVertex *v = dt_alloc(sizeof *v);

    v->x = dt_float_edgy(-1000.0f, 1000.0f);
    v->y = dt_float_edgy(-1000.0f, 1000.0f);
    v->z = dt_float_edgy(-1000.0f, 1000.0f);
    v->pad0c = 0;
    v->u = dt_float_edgy(-2.0f, 2.0f);
    v->v = dt_float_edgy(-2.0f, 2.0f);
    for (int i = 0; i < 4; i++) {
        v->color[i] = (uint8_t)dt_u32();
    }
    return v;
}

/* A texture word: none, or a material whose raster is `raster` (or another one). */
static uint32_t draw_texture(uint32_t raster)
{
    uint32_t *material;

    if (dt_u32() % 4 == 0) {
        return 0;
    }
    material = dt_alloc(4);
    *material = (dt_u32() & 1) ? raster : dt_u32();
    return (uint32_t)(uintptr_t)material;
}

/* A zmem buffer of `max` vertices of `size` bytes plus the alignment slack (0 to 40, so some calls grow it).
 */
static void draw_buffer(void **buffer, uint32_t *max, uint32_t size)
{
    *max = (uint32_t)dt_range(0, 40);
    *buffer = dt_zmemalloc(*max * size + 0xf);
    dt_fill(*buffer, *max * size + 0xf);
}

/* Engine state and stubs of a draw call; returns the flags (2D, world or camera relative). */
static uint32_t draw_state(DtCtx *c, uint32_t *raster)
{
    uint32_t flags;

    ngBegin(c, 0);
    dt_stub(0x004de610, NULL, (void *)stubNear);
    dt_stub(0x004db800, NULL, (void *)stubNearZ);
    dt_stub(0x004db920, NULL, (void *)stubIm2DRender);
    dt_stub(0x004df0b0, NULL, (void *)stubIm3DTransform);
    dt_stub(0x004def70, NULL, (void *)stubIm3DRender);
    dt_stub(0x004df090, NULL, (void *)stubIm3DEnd);
    dt_stub(0x004d3ae0, NULL, (void *)stubTransformVectors);
    dt_stub(0x00489b40, NULL, (void *)stubGetRaster);
    dt_stub(0x00493070, NULL, (void *)stubCurrentCamera);
    gTestStateHook = (void *)stubStateHook;
    flags = dt_u32();
    switch (dt_u32() % 3) {
    case 0:
        flags |= 0x2000000u;
        break;
    case 1:
        flags &= ~0x3000000u;
        break;
    default:
        flags = (flags & ~0x2000000u) | 0x1000000u;
        break;
    }
    *raster = dt_u32();
    gTestFrames = (int)dt_u32();
    gZrwPrimFrame = (dt_u32() & 1) ? gTestFrames : (int)dt_u32();
    gZrwPrimDepthBias = dt_float(-1.0f, 1.0f);
    gTestScale = dt_float_edgy(-4.0f, 4.0f);
    gTestOrigin.x = dt_float_edgy(-500.0f, 500.0f);
    gTestOrigin.y = dt_float_edgy(-500.0f, 500.0f);
    gTestOrigin.z = dt_float_edgy(-500.0f, 500.0f);
    gTestRwCamera = dt_alloc(4);
    gTestRenderFlags = (dt_u32() & 1) ? (flags & 0xffffff) : dt_u32();
    gTestRenderExtra = (dt_u32() & 1) ? *raster : ((dt_u32() & 1) ? 0 : dt_u32());
    gNear = dt_float_edgy(0.0f, 10.0f);
    gNearZ = dt_float_edgy(-1.0f, 1.0f);
    gTransformOk = (dt_u32() % 4) != 0;
    gPrimCamera = dt_alloc(sizeof *gPrimCamera);
    dt_fill(gPrimCamera, sizeof *gPrimCamera);
    for (int i = 0; i < 16; i++) {
        ((float *)&gPrimCamera->world)[i] = dt_float_edgy(-100.0f, 100.0f);
    }
    draw_buffer(&gZrwPrim.im3d, &gZrwPrim.im3dMax, 0x24);
    draw_buffer(&gZrwPrim.im2d, &gZrwPrim.im2dMax, 0x20);
    return flags;
}

static void setup_lines(DtCtx *c)
{
    uint32_t raster;
    const uint32_t flags = draw_state(c, &raster);
    const int n = dt_range(1, 4);
    ZrwPrimLine *lines = dt_alloc((uint32_t)n * sizeof *lines);

    for (int i = 0; i < n; i++) {
        lines[i].texture = draw_texture(raster);
        lines[i].vertices[0] = draw_vertex();
        lines[i].vertices[1] = (dt_u32() % 5 == 0) ? lines[i].vertices[0] : draw_vertex();
    }
    DT_ARG_INT(c, 0, flags);
    DT_ARG_PTR(c, 1, lines);
    DT_ARG_INT(c, 2, n);
    DT_ARG_PTR(c, 3, (dt_u32() % 3 == 0) ? random_matrix() : NULL);
}

static void setup_polys(DtCtx *c)
{
    uint32_t raster;
    const uint32_t flags = draw_state(c, &raster);
    const int n = dt_range(1, 3);
    ZrwPrimPoly *polys = dt_alloc((uint32_t)n * sizeof *polys);

    for (int i = 0; i < n; i++) {
        const int count = (dt_u32() % 4 == 0) ? dt_range(33, 70) : dt_range(1, 8);
        ZrwPrimVertex **list = (ZrwPrimVertex **)dt_alloc((uint32_t)count * sizeof *list);

        for (int k = 0; k < count; k++) {
            list[k] = draw_vertex();
        }
        polys[i].texture = draw_texture(raster);
        polys[i].vertices = list;
        polys[i].count = (uint32_t)count;
    }
    DT_ARG_INT(c, 0, flags);
    DT_ARG_PTR(c, 1, polys);
    DT_ARG_INT(c, 2, n);
    DT_ARG_PTR(c, 3, (dt_u32() % 3 == 0) ? random_matrix() : NULL);
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
    {"zrwprimDrawLines", 0x0048ccf0, zrwprimDrawLines, 4, DT_RET_VOID, setup_lines, 1000, 0},
    {"zrwprimDrawPolys", 0x0048d170, zrwprimDrawPolys, 4, DT_RET_VOID, setup_polys, 1000, 0},
};
