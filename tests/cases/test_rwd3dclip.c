/*
 * Tests for rw/d3dclip.c, the Im3D clipper of the D3D7 driver: edge interpolators, plane clippers, triangle
 * and line clippers. Camera space vertices are random with the clip flags the transform gives them
 * (perspective or parallel), so the clippers really clip; the index functions are logging stubs.
 *
 * Float code runs with the x87 control word of a camera update (24-bit precision, rounding down), which the
 * original's camera begin update sets: the wrappers below set it around both sides.
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwd3dclip.h"
#include "rw/rwim.h"

enum { kVerts = 32, kBuffer = kVerts + 160 };

static uint16_t cameraCw(void)
{
    uint16_t old, cw;
    __asm__ volatile("fnstcw %0" : "=m"(old));
    cw = (uint16_t)((old & 0xf0ff) | 0x0400);
    __asm__ volatile("fldcw %0" ::"m"(cw));
    return old;
}

static void restoreCw(uint16_t cw)
{
    __asm__ volatile("fldcw %0" ::"m"(cw));
}

static void BMX_CDECL stIndexTri(int32_t a, int32_t b, int32_t c)
{
    rwtLog(0xc4);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
    rwtLog((uint32_t)c);
}

static void BMX_CDECL stIndexLine(int32_t a, int32_t b)
{
    rwtLog(0xc5);
    rwtLog((uint32_t)a);
    rwtLog((uint32_t)b);
}

static void BMX_CDECL stInterp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                               RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    rwtLog(0xc0);
    rwtLogPtr(vertex);
    rwtLogPtr(out);
    rwtLogPtr(in);
    rwtLogPtr(tl);
    rwtLogPtr(outTL);
    rwtLogPtr(inTL);
    vertex->clip = (uint8_t)(out->clip ^ in->clip);
}

/* The transform's clip flags (rw/d3dim3d.c): x and y against z or 1, z against near and far, as bits. */
static uint8_t clipPersp(const RwD3DCamVertex *cam)
{
    const int32_t x = rwD3DBits(cam->x), y = rwD3DBits(cam->y), z = rwD3DBits(cam->z);
    const uint32_t xs = (uint32_t)x >> 31, ys = (uint32_t)y >> 31;
    uint32_t code = (uint32_t)(z > rwD3DFarZ) << 2;

    if (z >= rwD3DNearZ) {
        code = (((code + (uint32_t)(z < y)) * 2 + ys) * 2 + (uint32_t)(z < x)) * 2 + xs;
    } else if (z >= 0) {
        code = (((code + 2 + (uint32_t)(z < y)) * 2 + ys) * 2 + (uint32_t)(z < x)) * 2 + xs;
    } else {
        code = (((code + 2 + ((uint32_t)(z < y) ^ ys)) * 2 + ys) * 2 + ((uint32_t)(z < x) ^ xs)) * 2 + xs;
    }
    return (uint8_t)code;
}

static uint8_t clipParallel(const RwD3DCamVertex *cam)
{
    const int32_t x = rwD3DBits(cam->x), y = rwD3DBits(cam->y), z = rwD3DBits(cam->z);
    const int32_t one = rwD3DBits(1.0f);
    uint32_t code = (uint32_t)(z > rwD3DFarZ) * 2 | (uint32_t)(z < rwD3DNearZ);

    code = (code * 2 + (uint32_t)(one < y)) * 2 + ((uint32_t)y >> 31);
    return (uint8_t)((code * 2 + (uint32_t)(one < x)) * 2 + ((uint32_t)x >> 31));
}

static int gParallel;
static float gNear, gFar;

/*
 * A random camera space vertex with its clip flags: z in [-5, far * 1.3] (or [near, far] with `inZ`), x and y
 * around the frustum (0..z, or 0..1 for parallel cameras), random texture coordinates.
 */
static void randomVertex(RwD3DCamVertex *v, int inZ)
{
    const float z = inZ ? dt_float(gNear, gFar) : dt_float(-5.0f, gFar * 1.3f);
    const float scale = gParallel ? 1.0f : z;

    v->z = z;
    v->x = dt_float(-0.7f, 1.7f) * scale;
    v->y = dt_float(-0.7f, 1.7f) * scale;
    v->u = dt_float(-2.0f, 2.0f);
    v->v = dt_float(-2.0f, 2.0f);
    v->clip = gParallel ? clipParallel(v) : clipPersp(v);
}

/*
 * The driver state: a projection, near and far planes, fog, screen mapping, vertex buffers with random camera
 * vertices (kVerts of them transformed, room for the clipper's), a level, and logging index functions.
 */
static void clipEnv(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    gParallel = dt_u32() % 3 == 0;
    gNear = dt_float(0.05f, 2.0f);
    gFar = dt_float(10.0f, 150.0f);
    const float fog = dt_float(0.0f, gFar * 0.9f);
    rwD3DNearZ = rwD3DBits(gNear);
    rwD3DFarZ = rwD3DBits(gFar);
    rwD3DFogStart = rwD3DBits(fog);
    rwD3DFogEnd = gFar;
    rwD3DFogScale = 255.0f / (gFar - fog);
    rwD3DZScale = dt_float(-2.0f, 2.0f);
    rwD3DZShift = dt_float(-1.0f, 1.0f);
    rwD3DXScale = (float)dt_range(1, 1600);
    rwD3DYScale = (float)dt_range(1, 1200);
    rwD3DXOffset = (float)dt_range(-64, 64);
    rwD3DYOffset = (float)dt_range(-64, 64);
    rwD3DNearScreenZ = dt_float(0.0f, 0.1f);
    rwD3DFarScreenZ = dt_float(0.9f, 1.0f);
    dt_fill((void *)rwD3DFogTable, 256 * 4);
    rwIm3DGouraud = (int32_t)(dt_u32() % 2);

    const int base = dt_range(0, 4);
    rwD3DTLBuffer = dt_alloc((kBuffer + base) * sizeof(RwD3DTLVertex));
    rwD3DCamBuffer = dt_alloc((kBuffer + base) * sizeof(RwD3DCamVertex));
    dt_fill(rwD3DTLBuffer, (kBuffer + base) * sizeof(RwD3DTLVertex));
    dt_fill(rwD3DCamBuffer, (kBuffer + base) * sizeof(RwD3DCamVertex));
    rwD3DIm3DLevel = &rwD3DIm3DLevels[dt_range(0, 9)];
    rwD3DIm3DLevel->base = base;
    rwD3DIm3DLevel->numVertices = kVerts;
    rwD3DIm3DLevel->numTL = kVerts + dt_range(0, 4);
    rwD3DTLBase = rwD3DTLBuffer + base;
    rwD3DCamBase = rwD3DCamBuffer + base;
    const int inZ = dt_u32() % 4 == 0;
    for (int i = 0; i < kVerts; i++) {
        randomVertex(&rwD3DCamBase[i], inZ);
    }
    rwD3DIndexTri = stIndexTri;
    rwD3DIndexLine = stIndexLine;
}

/* ---- edge interpolators ---- */

typedef void (*InterpFn)(RwD3DCamVertex *, const RwD3DCamVertex *, const RwD3DCamVertex *, RwD3DTLVertex *,
                         const RwD3DTLVertex *, const RwD3DTLVertex *);

static void interpCall(uint32_t orig, InterpFn impl, const uint32_t *args)
{
    const uint16_t cw = cameraCw();
    RwD3DCamVertex *vertex = (RwD3DCamVertex *)(uintptr_t)args[0];
    const RwD3DCamVertex *out = (const RwD3DCamVertex *)(uintptr_t)args[1];
    const RwD3DCamVertex *in = (const RwD3DCamVertex *)(uintptr_t)args[2];
    RwD3DTLVertex *tl = (RwD3DTLVertex *)(uintptr_t)args[3];
    const RwD3DTLVertex *outTL = (const RwD3DTLVertex *)(uintptr_t)args[4];
    const RwD3DTLVertex *inTL = (const RwD3DTLVertex *)(uintptr_t)args[5];

    if (impl == NULL) {
        ((RwD3DClipInterpFunc)(uintptr_t)orig)(vertex, out, in, tl, outTL, inTL);
    } else {
        impl(vertex, out, in, tl, outTL, inTL);
    }
    restoreCw(cw);
}

/* Orig and port wrappers with the camera control word, one pair per interpolator. */
#define INTERP_WRAP(name, addr, fn)                                                                          \
    static void name##Orig(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)     \
    {                                                                                                        \
        const uint32_t args[6] = {a0, a1, a2, a3, a4, a5};                                                   \
        interpCall(addr, NULL, args);                                                                        \
    }                                                                                                        \
    static void name##Impl(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)     \
    {                                                                                                        \
        const uint32_t args[6] = {a0, a1, a2, a3, a4, a5};                                                   \
        interpCall(0, fn, args);                                                                             \
    }

INTERP_WRAP(nearPersp, 0x004f3310, rwD3DInterpNearPersp)
INTERP_WRAP(farPersp, 0x004f34e0, rwD3DInterpFarPersp)
INTERP_WRAP(topPersp, 0x004f36b0, rwD3DInterpTopPersp)
INTERP_WRAP(bottomPersp, 0x004f3850, rwD3DInterpBottomPersp)
INTERP_WRAP(leftPersp, 0x004f3a00, rwD3DInterpLeftPersp)
INTERP_WRAP(rightPersp, 0x004f3b90, rwD3DInterpRightPersp)
INTERP_WRAP(nearParallel, 0x004f43f0, rwD3DInterpNearParallel)
INTERP_WRAP(farParallel, 0x004f45d0, rwD3DInterpFarParallel)
INTERP_WRAP(topParallel, 0x004f47b0, rwD3DInterpTopParallel)
INTERP_WRAP(bottomParallel, 0x004f4960, rwD3DInterpBottomParallel)
INTERP_WRAP(leftParallel, 0x004f4b10, rwD3DInterpLeftParallel)
INTERP_WRAP(rightParallel, 0x004f4ca0, rwD3DInterpRightParallel)

/*
 * An edge across `plane` (outside vertex first, as the plane clippers pass it): vertices in [near, far] for
 * the x and y planes, where the clipper only gets after clipping z.
 */
static void setupInterp(DtCtx *c, int parallel, uint8_t plane)
{
    clipEnv(c);
    gParallel = parallel;
    RwD3DCamVertex *out = &rwD3DCamBase[0], *in = &rwD3DCamBase[1];
    const int inZ = plane < rwD3DCLIPNEAR;
    for (int tries = 0; tries < 1000; tries++) {
        randomVertex(out, inZ);
        randomVertex(in, inZ);
        if ((out->clip & plane) != 0 && (in->clip & plane) == 0) {
            break;
        }
    }
    if (dt_u32() % 4 == 0) {
        out->clip |= (uint8_t)(dt_u32() & 0xc0);
    }
    const int n = rwD3DIm3DLevel->numTL;
    DT_ARG_PTR(c, 0, &rwD3DCamBase[n]);
    DT_ARG_PTR(c, 1, out);
    DT_ARG_PTR(c, 2, in);
    DT_ARG_PTR(c, 3, &rwD3DTLBase[n]);
    DT_ARG_PTR(c, 4, &rwD3DTLBase[0]);
    DT_ARG_PTR(c, 5, &rwD3DTLBase[1]);
    rwtEnd();
}

#define INTERP_SETUP(name, parallel, plane)                                                                  \
    static void setup_##name(DtCtx *c)                                                                       \
    {                                                                                                        \
        setupInterp(c, parallel, plane);                                                                     \
    }

INTERP_SETUP(nearPersp, 0, rwD3DCLIPNEAR)
INTERP_SETUP(farPersp, 0, rwD3DCLIPFAR)
INTERP_SETUP(topPersp, 0, rwD3DCLIPTOP)
INTERP_SETUP(bottomPersp, 0, rwD3DCLIPBOTTOM)
INTERP_SETUP(leftPersp, 0, rwD3DCLIPLEFT)
INTERP_SETUP(rightPersp, 0, rwD3DCLIPRIGHT)
INTERP_SETUP(nearParallel, 1, rwD3DCLIPNEAR)
INTERP_SETUP(farParallel, 1, rwD3DCLIPFAR)
INTERP_SETUP(topParallel, 1, rwD3DCLIPTOP)
INTERP_SETUP(bottomParallel, 1, rwD3DCLIPBOTTOM)
INTERP_SETUP(leftParallel, 1, rwD3DCLIPLEFT)
INTERP_SETUP(rightParallel, 1, rwD3DCLIPRIGHT)

/* ---- plane clippers ---- */

static const uint32_t kInterps[2][6] = {
    {0x004f3310, 0x004f34e0, 0x004f36b0, 0x004f3850, 0x004f3a00, 0x004f3b90},
    {0x004f43f0, 0x004f45d0, 0x004f47b0, 0x004f4960, 0x004f4b10, 0x004f4ca0},
};

typedef int32_t(__attribute__((regparm(3))) * PlaneFn)(int32_t *, int32_t *, int32_t *, int32_t, int32_t,
                                                       int32_t, uint32_t, RwD3DClipInterpFunc);

static int32_t planeCall(PlaneFn fn, int32_t *ring, int32_t *start, int32_t *end, uint32_t plane,
                         RwD3DClipInterpFunc interp)
{
    const uint16_t cw = cameraCw();
    const int32_t r = fn(ring, start, end, 0x11, 0x22, 0x33, plane, interp);
    restoreCw(cw);
    return r;
}

static int32_t polyOrig(int32_t *ring, int32_t *start, int32_t *end, uint32_t plane,
                        RwD3DClipInterpFunc interp)
{
    return planeCall((PlaneFn)(uintptr_t)0x004f2f50, ring, start, end, plane, interp);
}

static int32_t polyImpl(int32_t *ring, int32_t *start, int32_t *end, uint32_t plane,
                        RwD3DClipInterpFunc interp)
{
    return planeCall(rwD3DClipPolyPlane, ring, start, end, plane, interp);
}

static int32_t lineOrig(int32_t *ring, int32_t *start, int32_t *end, uint32_t plane,
                        RwD3DClipInterpFunc interp)
{
    return planeCall((PlaneFn)(uintptr_t)0x004f3120, ring, start, end, plane, interp);
}

static int32_t lineImpl(int32_t *ring, int32_t *start, int32_t *end, uint32_t plane,
                        RwD3DClipInterpFunc interp)
{
    return planeCall(rwD3DClipLinePlane, ring, start, end, plane, interp);
}

/* A ring of random vertices (n of them from start), one plane, the real interpolator or a logging one. */
static void setupPlane(DtCtx *c, int n)
{
    clipEnv(c);
    const int k = dt_range(0, 5);
    const uint8_t plane = (uint8_t)(k < 2 ? 0x10 << k : k < 4 ? 4 << (k - 2) : 1 << (k - 4));
    if (k >= 2) {
        for (int i = 0; i < kVerts; i++) {
            randomVertex(&rwD3DCamBase[i], 1);
        }
    }
    int32_t *ring = dt_alloc(16 * 4);
    int32_t *start = dt_alloc(4), *end = dt_alloc(4);
    for (int i = 0; i < 16; i++) {
        ring[i] = dt_range(0, kVerts - 1);
    }
    *start = dt_range(0, 15);
    *end = (*start + n) & 15;
    DT_ARG_PTR(c, 0, ring);
    DT_ARG_PTR(c, 1, start);
    DT_ARG_PTR(c, 2, end);
    DT_ARG_INT(c, 3, plane);
    DT_ARG_INT(c, 4, dt_u32() % 3 == 0 ? (uint32_t)(uintptr_t)stInterp : kInterps[gParallel][k]);
    rwtEnd();
}

static void setup_poly(DtCtx *c)
{
    setupPlane(c, dt_range(3, 8));
}

static void setup_line(DtCtx *c)
{
    setupPlane(c, 2);
}

/* ---- triangle and line clippers ---- */

static void triOrigAt(uint32_t addr, int32_t a, int32_t b, int32_t c)
{
    const uint16_t cw = cameraCw();
    ((RwD3DTriVoidFunc)(uintptr_t)addr)(a, b, c);
    restoreCw(cw);
}

static void clipTriPerspOrig(int32_t a, int32_t b, int32_t c)
{
    triOrigAt(0x004f3e40, a, b, c);
}

static void clipTriPerspImpl(int32_t a, int32_t b, int32_t c)
{
    const uint16_t cw = cameraCw();
    rwD3DClipTriPersp(a, b, c);
    restoreCw(cw);
}

static void clipTriParallelOrig(int32_t a, int32_t b, int32_t c)
{
    triOrigAt(0x004f4ea0, a, b, c);
}

static void clipTriParallelImpl(int32_t a, int32_t b, int32_t c)
{
    const uint16_t cw = cameraCw();
    rwD3DClipTriParallel(a, b, c);
    restoreCw(cw);
}

static void lineOrigAt(uint32_t addr, int32_t a, int32_t b)
{
    const uint16_t cw = cameraCw();
    ((RwD3DLineFunc)(uintptr_t)addr)(a, b);
    restoreCw(cw);
}

static void clipLinePerspOrig(int32_t a, int32_t b)
{
    lineOrigAt(0x004f5450, a, b);
}

static void clipLinePerspImpl(int32_t a, int32_t b)
{
    const uint16_t cw = cameraCw();
    rwD3DClipLinePersp(a, b);
    restoreCw(cw);
}

static void clipLineParallelOrig(int32_t a, int32_t b)
{
    lineOrigAt(0x004f59b0, a, b);
}

static void clipLineParallelImpl(int32_t a, int32_t b)
{
    const uint16_t cw = cameraCw();
    rwD3DClipLineParallel(a, b);
    restoreCw(cw);
}

/* Three different vertices, at least one of them outside (the clipper only gets those). */
static void setupClip(DtCtx *c, int parallel, int n)
{
    clipEnv(c);
    gParallel = parallel;
    for (int i = 0; i < kVerts; i++) {
        randomVertex(&rwD3DCamBase[i], 0);
    }
    int32_t v[3];
    for (int tries = 0; tries < 100; tries++) {
        v[0] = dt_range(0, kVerts - 1);
        v[1] = (v[0] + dt_range(1, kVerts - 1)) % kVerts;
        do {
            v[2] = dt_range(0, kVerts - 1);
        } while (v[2] == v[0] || v[2] == v[1]);
        uint8_t orClip = 0, andClip = 0xff;
        for (int i = 0; i < n; i++) {
            orClip |= rwD3DCamBase[v[i]].clip;
            andClip &= rwD3DCamBase[v[i]].clip;
        }
        if (orClip != 0 && andClip == 0) {
            break;
        }
    }
    for (int i = 0; i < n; i++) {
        DT_ARG_INT(c, i, v[i]);
    }
    rwtEnd();
}

static void setup_triPersp(DtCtx *c)
{
    setupClip(c, 0, 3);
}

static void setup_triParallel(DtCtx *c)
{
    setupClip(c, 1, 3);
}

static void setup_linePersp(DtCtx *c)
{
    setupClip(c, 0, 2);
}

static void setup_lineParallel(DtCtx *c)
{
    setupClip(c, 1, 2);
}

#define INTERP_CASE(name, fn)                                                                                \
    {#fn, (uint32_t)(uintptr_t)name##Orig, name##Impl, 6, DT_RET_VOID, setup_##name, 1000, 0}

DT_CASES(rwd3dclip_cases) = {
    INTERP_CASE(nearPersp, rwD3DInterpNearPersp),
    INTERP_CASE(farPersp, rwD3DInterpFarPersp),
    INTERP_CASE(topPersp, rwD3DInterpTopPersp),
    INTERP_CASE(bottomPersp, rwD3DInterpBottomPersp),
    INTERP_CASE(leftPersp, rwD3DInterpLeftPersp),
    INTERP_CASE(rightPersp, rwD3DInterpRightPersp),
    INTERP_CASE(nearParallel, rwD3DInterpNearParallel),
    INTERP_CASE(farParallel, rwD3DInterpFarParallel),
    INTERP_CASE(topParallel, rwD3DInterpTopParallel),
    INTERP_CASE(bottomParallel, rwD3DInterpBottomParallel),
    INTERP_CASE(leftParallel, rwD3DInterpLeftParallel),
    INTERP_CASE(rightParallel, rwD3DInterpRightParallel),
    {"rwD3DClipPolyPlane", (uint32_t)(uintptr_t)polyOrig, polyImpl, 5, DT_RET_INT, setup_poly, 1000, 0},
    {"rwD3DClipLinePlane", (uint32_t)(uintptr_t)lineOrig, lineImpl, 5, DT_RET_INT, setup_line, 1000, 0},
    {"rwD3DClipTriPersp", (uint32_t)(uintptr_t)clipTriPerspOrig, clipTriPerspImpl, 3, DT_RET_VOID,
     setup_triPersp, 1000, 0},
    {"rwD3DClipTriParallel", (uint32_t)(uintptr_t)clipTriParallelOrig, clipTriParallelImpl, 3, DT_RET_VOID,
     setup_triParallel, 1000, 0},
    {"rwD3DClipLinePersp", (uint32_t)(uintptr_t)clipLinePerspOrig, clipLinePerspImpl, 2, DT_RET_VOID,
     setup_linePersp, 1000, 0},
    {"rwD3DClipLineParallel", (uint32_t)(uintptr_t)clipLineParallelOrig, clipLineParallelImpl, 2, DT_RET_VOID,
     setup_lineParallel, 1000, 0},
};
