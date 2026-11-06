/*
 * RenderWare 3.1's Direct3D 7 driver, Im3D clipper (lib/d3d7driver): triangles and lines that are partly
 * outside the frustum are clipped plane by plane (Sutherland-Hodgman on a ring of 16 vertex indices), the new
 * vertices get TL vertices, and the pieces go out as a triangle fan or a line through the index functions.
 *
 * Camera space frustum: perspective 0 <= x, y <= z, parallel 0 <= x, y <= 1; near <= z <= far for both.
 * Float code runs in the x87 mode of the camera update (rwD3DRoundBegin, rw/rwd3d.h).
 */
#include "rw/rwd3dclip.h"
#include "rw/rwim.h"

/* The edge interpolators the clippers pass to the plane clippers (stored as the original's addresses). */
#define CLIP_FN(addr, fn) BMX_FNPTR(RwD3DClipInterpFunc, addr, fn)

static float asFloat(int32_t bits)
{
    float f;
    __builtin_memcpy(&f, &bits, 4);
    return f;
}

static int isNegative(float f)
{
    return rwD3DBits(f) < 0;
}

/*
 * The original's hand-written double to int conversion of a float it stored as double: truncates toward zero,
 * 0 below 1, and beyond 2^31 takes the shift count mod 32 like SHR does.
 */
static int32_t truncDouble(float value)
{
    const double d = value;
    uint64_t bits;
    uint32_t hi, lo, e, m;

    __builtin_memcpy(&bits, &d, 8);
    hi = (uint32_t)(bits >> 32);
    lo = (uint32_t)bits;
    e = (hi >> 20) & 0x7ff;
    if (e <= 0x3fe) {
        return 0;
    }
    m = (hi << 11 | lo >> 21) | 0x80000000u;
    m >>= (0x41e - e) & 31;
    return (int32_t)hi < 0 ? (int32_t)(0u - m) : (int32_t)m;
}

static float lerp(float out, float in, float t)
{
    return (in - out) * t + out;
}

/* Colour between two D3DCOLORs at f / 256 per channel; a negative difference spills into the other bytes. */
static uint32_t lerpColor(uint32_t out, uint32_t in, uint32_t f)
{
    uint32_t color = 0;

    for (int shift = 0; shift < 32; shift += 8) {
        const uint32_t o = out >> shift & 0xff, i = in >> shift & 0xff;
        color |= (((i - o) * f >> 8) + o) << shift;
    }
    return color;
}

/* The new TL vertex only gets its colour; the clipper fills in the rest. */
static void interpColor(RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL, float t)
{
    tl->color = lerpColor(outTL->color, inTL->color, (uint32_t)truncDouble(255.0f * t) & 0xff);
}

/*
 * The x flags of a new vertex: left when x is negative (as bits), else right beyond the plane x = z
 * (perspective) or x = 1 (parallel). The parallel near and far interpolators test !(1 >= x), the others x > 1
 * (only NaN differs).
 */
enum { kPersp, kParallelZ, kParallelY };

static uint8_t xFlags(const RwD3DCamVertex *v, int kind)
{
    int right;

    if (isNegative(v->x)) {
        return rwD3DCLIPLEFT;
    }
    switch (kind) {
    case kPersp:
        right = v->x > v->z;
        break;
    case kParallelZ:
        right = !(1.0f >= v->x);
        break;
    default:
        right = v->x > 1.0f;
        break;
    }
    return right ? rwD3DCLIPRIGHT : 0;
}

/* Clip flags of a vertex on the near or far plane: the plane, then y and x like xFlags. */
static void zPlaneFlags(RwD3DCamVertex *v, uint8_t plane, int kind)
{
    uint8_t clip = plane;

    if (isNegative(v->y)) {
        clip |= rwD3DCLIPTOP;
    } else if (kind == kPersp ? v->y > v->z : !(1.0f >= v->y)) {
        clip |= rwD3DCLIPBOTTOM;
    }
    v->clip = clip;
    v->clip |= xFlags(v, kind);
}

/* On a plane z = const: x, y, u, v at t = (const - out.z) / (in.z - out.z). */
static void interpZ(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                    RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL, int32_t z,
                    uint8_t plane, int kind)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    const float t = (asFloat(z) - out->z) / (in->z - out->z);

    vertex->x = lerp(out->x, in->x, t);
    vertex->y = lerp(out->y, in->y, t);
    __builtin_memcpy(&vertex->z, &z, 4);
    vertex->u = lerp(out->u, in->u, t);
    vertex->v = lerp(out->v, in->v, t);
    interpColor(tl, outTL, inTL, t);
    zPlaneFlags(vertex, plane, kind);
    rwD3DRoundEnd(mxcsr);
}

/* ---- edge interpolators: perspective ------------------------------------------------------------------- */

/* 0x4f3310: on the near plane. */
BMX_ADDR(0x004f3310)
void rwD3DInterpNearPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                          RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    interpZ(vertex, out, in, tl, outTL, inTL, rwD3DNearZ, rwD3DCLIPNEAR, kPersp);
}

/* 0x4f34e0: on the far plane. */
BMX_ADDR(0x004f34e0)
void rwD3DInterpFarPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                         RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    interpZ(vertex, out, in, tl, outTL, inTL, rwD3DFarZ, rwD3DCLIPFAR, kPersp);
}

/*
 * 0x4f36b0: on the plane y = 0. The new vertex keeps the outside vertex's near and far flags (the clipper
 * clipped those planes first), its own x flags are recomputed.
 */
BMX_ADDR(0x004f36b0)
void rwD3DInterpTopPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                         RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    const float t = out->y / (out->y - in->y);

    vertex->x = lerp(out->x, in->x, t);
    vertex->y = 0.0f;
    vertex->z = lerp(out->z, in->z, t);
    vertex->u = lerp(out->u, in->u, t);
    vertex->v = lerp(out->v, in->v, t);
    interpColor(tl, outTL, inTL, t);
    vertex->clip = (uint8_t)((out->clip & 0xf0) | rwD3DCLIPTOP);
    vertex->clip |= xFlags(vertex, kPersp);
    rwD3DRoundEnd(mxcsr);
}

/* 0x4f3850: on the plane y = z. */
BMX_ADDR(0x004f3850)
void rwD3DInterpBottomPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                            RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    const float d = out->y - out->z;
    const float t = d / ((in->z - in->y) + d);
    const float z = lerp(out->z, in->z, t);

    vertex->x = lerp(out->x, in->x, t);
    vertex->z = z;
    vertex->y = z;
    vertex->u = lerp(out->u, in->u, t);
    vertex->v = lerp(out->v, in->v, t);
    interpColor(tl, outTL, inTL, t);
    vertex->clip = (uint8_t)((out->clip & 0xf0) | rwD3DCLIPBOTTOM);
    vertex->clip |= xFlags(vertex, kPersp);
    rwD3DRoundEnd(mxcsr);
}

/* 0x4f3a00: on the plane x = 0 (the last but one plane: all other flags come from the outside vertex). */
BMX_ADDR(0x004f3a00)
void rwD3DInterpLeftPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                          RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    const float t = out->x / (out->x - in->x);

    vertex->y = lerp(out->y, in->y, t);
    vertex->x = 0.0f;
    vertex->z = lerp(out->z, in->z, t);
    vertex->u = lerp(out->u, in->u, t);
    vertex->v = lerp(out->v, in->v, t);
    interpColor(tl, outTL, inTL, t);
    vertex->clip = (uint8_t)((out->clip & 0xfc) | rwD3DCLIPLEFT);
    rwD3DRoundEnd(mxcsr);
}

/* 0x4f3b90: on the plane x = z. */
BMX_ADDR(0x004f3b90)
void rwD3DInterpRightPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                           RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    const float d = out->x - out->z;
    const float t = d / ((in->z - in->x) + d);
    const float z = lerp(out->z, in->z, t);

    vertex->y = lerp(out->y, in->y, t);
    vertex->z = z;
    vertex->x = z;
    vertex->u = lerp(out->u, in->u, t);
    vertex->v = lerp(out->v, in->v, t);
    interpColor(tl, outTL, inTL, t);
    vertex->clip = (uint8_t)((out->clip & 0xfc) | rwD3DCLIPRIGHT);
    rwD3DRoundEnd(mxcsr);
}

/* ---- edge interpolators: parallel -----------------------------------------------------------------------
 */

/* 0x4f43f0: on the near plane. */
BMX_ADDR(0x004f43f0)
void rwD3DInterpNearParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                             RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    interpZ(vertex, out, in, tl, outTL, inTL, rwD3DNearZ, rwD3DCLIPNEAR, kParallelZ);
}

/* 0x4f45d0: on the far plane. */
BMX_ADDR(0x004f45d0)
void rwD3DInterpFarParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                            RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    interpZ(vertex, out, in, tl, outTL, inTL, rwD3DFarZ, rwD3DCLIPFAR, kParallelZ);
}

/* On a plane y = 0 or 1: x, z, u, v at t, the plane's flag after the outside vertex's z flags, x flags. */
static void interpYParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                            RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL, float t,
                            float y, uint8_t plane)
{
    vertex->x = lerp(out->x, in->x, t);
    vertex->y = y;
    vertex->z = lerp(out->z, in->z, t);
    vertex->u = lerp(out->u, in->u, t);
    vertex->v = lerp(out->v, in->v, t);
    interpColor(tl, outTL, inTL, t);
    vertex->clip = (uint8_t)((out->clip & 0xf0) | plane);
    vertex->clip |= xFlags(vertex, kParallelY);
}

/* 0x4f47b0: on the plane y = 0. */
BMX_ADDR(0x004f47b0)
void rwD3DInterpTopParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                            RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    interpYParallel(vertex, out, in, tl, outTL, inTL, out->y / (out->y - in->y), 0.0f, rwD3DCLIPTOP);
    rwD3DRoundEnd(mxcsr);
}

/* 0x4f4960: on the plane y = 1. */
BMX_ADDR(0x004f4960)
void rwD3DInterpBottomParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                               RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    interpYParallel(vertex, out, in, tl, outTL, inTL, (1.0f - out->y) / (in->y - out->y), 1.0f,
                    rwD3DCLIPBOTTOM);
    rwD3DRoundEnd(mxcsr);
}

/* On a plane x = 0 or 1: y, z, u, v at t, the plane's flag after the outside vertex's other flags. */
static void interpXParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                            RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL, float t,
                            float x, uint8_t plane)
{
    vertex->y = lerp(out->y, in->y, t);
    vertex->x = x;
    vertex->z = lerp(out->z, in->z, t);
    vertex->u = lerp(out->u, in->u, t);
    vertex->v = lerp(out->v, in->v, t);
    interpColor(tl, outTL, inTL, t);
    vertex->clip = (uint8_t)((out->clip & 0xfc) | plane);
}

/* 0x4f4b10: on the plane x = 0. */
BMX_ADDR(0x004f4b10)
void rwD3DInterpLeftParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                             RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    interpXParallel(vertex, out, in, tl, outTL, inTL, out->x / (out->x - in->x), 0.0f, rwD3DCLIPLEFT);
    rwD3DRoundEnd(mxcsr);
}

/* 0x4f4ca0: on the plane x = 1. */
BMX_ADDR(0x004f4ca0)
void rwD3DInterpRightParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                              RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    interpXParallel(vertex, out, in, tl, outTL, inTL, (1.0f - out->x) / (in->x - out->x), 1.0f,
                    rwD3DCLIPRIGHT);
    rwD3DRoundEnd(mxcsr);
}

/* ---- plane clippers ---------------------------------------------------------------------------------------
 */

/*
 * 0x4f2f50 (ring in EAX, &start in EDX, &end in ECX, plane and interpolator as 4th and 5th stack word; the
 * first three stack words are not read): clips the polygon ring[start..end) against one plane. The result is
 * appended to the ring after end (indices mod 16) and becomes the new start..end; returns its vertex count.
 */
BMX_ADDR(0x004f2f50)
BMX_REGPARM(3)
int32_t rwD3DClipPolyPlane(int32_t *ring, int32_t *start, int32_t *end, int32_t unused0, int32_t unused1,
                           int32_t unused2, uint32_t plane, RwD3DClipInterpFunc interp)
{
    const int32_t last = *end;
    int32_t out = last, i = *start;
    int32_t prev = ring[(last - 1) & 15], cur = ring[i];
    RwD3DCamVertex *prevCam = &rwD3DCamBase[prev], *curCam = &rwD3DCamBase[cur];
    RwD3DTLVertex *prevTL = &rwD3DTLBase[prev], *curTL = &rwD3DTLBase[cur];

    do {
        if ((prevCam->clip & plane) == 0) {
            ring[out] = prev;
            out = (out + 1) & 15;
        }
        if (((prevCam->clip ^ curCam->clip) & plane) != 0) {
            const int32_t n = rwD3DIm3DLevel->numTL++;
            RwD3DTLVertex *tl = &rwD3DTLBase[n];
            RwD3DCamVertex *cam;

            ring[out] = n;
            out = (out + 1) & 15;
            cam = &rwD3DCamBase[n];
            if ((prevCam->clip & plane) != 0) {
                interp(cam, prevCam, curCam, tl, prevTL, curTL);
            } else {
                interp(cam, curCam, prevCam, tl, curTL, prevTL);
            }
        }
        i = (i + 1) & 15;
        prev = cur;
        prevCam = curCam;
        prevTL = curTL;
        cur = ring[i];
        curTL = &rwD3DTLBase[cur];
        curCam = &rwD3DCamBase[cur];
    } while (i != last);
    *start = i;
    *end = out;
    return (out - i) & 15;
}

/*
 * 0x4f3120 (same convention): clips the line ring[start], ring[start + 1] against one plane. The result (the
 * second vertex if inside, the crossing, the first if inside: the line flips) is appended after end.
 */
BMX_ADDR(0x004f3120)
BMX_REGPARM(3)
int32_t rwD3DClipLinePlane(int32_t *ring, int32_t *start, int32_t *end, int32_t unused0, int32_t unused1,
                           int32_t unused2, uint32_t plane, RwD3DClipInterpFunc interp)
{
    const int32_t a = ring[*start], b = ring[(*start + 1) & 15];
    const int32_t first = *end;
    int32_t out = first;
    RwD3DCamVertex *camA = &rwD3DCamBase[a], *camB = &rwD3DCamBase[b];
    RwD3DTLVertex *tlA = &rwD3DTLBase[a], *tlB = &rwD3DTLBase[b];

    if ((camB->clip & plane) == 0) {
        ring[out] = b;
        out = (out + 1) & 15;
    }
    if (((camA->clip ^ camB->clip) & plane) != 0) {
        const int32_t n = rwD3DIm3DLevel->numTL++;
        RwD3DCamVertex *cam;
        RwD3DTLVertex *tl;

        ring[out] = n;
        cam = &rwD3DCamBase[n];
        tl = &rwD3DTLBase[n];
        out = (out + 1) & 15;
        if ((camB->clip & plane) != 0) {
            interp(cam, camB, camA, tl, tlB, tlA);
        } else {
            interp(cam, camA, camB, tl, tlA, tlB);
        }
    }
    if ((camA->clip & plane) == 0) {
        ring[out] = a;
        out = (out + 1) & 15;
    }
    *start = *end;
    *end = out;
    return (out - first) & 15;
}

/* ---- triangle and line clippers ---------------------------------------------------------------------------
 */

typedef struct ClipRing {
    int32_t start;
    int32_t index[16];
    int32_t end;
} ClipRing;

/* The interpolators of a projection, in plane order: near, far, top, bottom, left, right. */
typedef struct ClipPlanes {
    RwD3DClipInterpFunc interp[6];
    int parallel;
} ClipPlanes;

static const ClipPlanes kPerspPlanes = {
    {CLIP_FN(0x004f3310, rwD3DInterpNearPersp), CLIP_FN(0x004f34e0, rwD3DInterpFarPersp),
     CLIP_FN(0x004f36b0, rwD3DInterpTopPersp), CLIP_FN(0x004f3850, rwD3DInterpBottomPersp),
     CLIP_FN(0x004f3a00, rwD3DInterpLeftPersp), CLIP_FN(0x004f3b90, rwD3DInterpRightPersp)},
    0,
};

static const ClipPlanes kParallelPlanes = {
    {CLIP_FN(0x004f43f0, rwD3DInterpNearParallel), CLIP_FN(0x004f45d0, rwD3DInterpFarParallel),
     CLIP_FN(0x004f47b0, rwD3DInterpTopParallel), CLIP_FN(0x004f4960, rwD3DInterpBottomParallel),
     CLIP_FN(0x004f4b10, rwD3DInterpLeftParallel), CLIP_FN(0x004f4ca0, rwD3DInterpRightParallel)},
    1,
};

static uint8_t clipOf(int32_t index)
{
    return rwD3DCamBase[index].clip;
}

/* Fog factor index of a camera space z: 0 before the fog plane (or NaN), 255 beyond the fog end. */
static int32_t fogIndex(float z)
{
    const float start = asFloat(rwD3DFogStart);

    if (!(z >= start)) {
        return 0;
    }
    if (!(rwD3DFogEnd >= z)) {
        return 0xff;
    }
    return truncDouble((z - start) * rwD3DFogScale);
}

/*
 * The TL vertex of a vertex the clipping left in the polygon (if it has any outcode: original vertices inside
 * have theirs). A coordinate on a screen edge gets the edge exactly: x for flags 1 or 2 alone (of 1 | 2), y
 * only when no x flag is set either (flags & 0xf of 4 or 8), z when near or far is its only flag (of 0x3f).
 */
static void fillTL(int32_t index, int parallel)
{
    const RwD3DCamVertex *cam = &rwD3DCamBase[index];
    RwD3DTLVertex *tl = &rwD3DTLBase[index];
    const uint8_t clip = cam->clip;
    const uint8_t flags = clip & 0x3f;
    float rhw = 1.0f;

    if (flags == 0) {
        return;
    }
    if (!parallel) {
        rhw = 1.0f / cam->z;
    }
    tl->specular = rwD3DFogTable[fogIndex(cam->z)];
    tl->rhw = rhw;
    if ((clip & 3) == rwD3DCLIPRIGHT) {
        tl->sx = rwD3DXOffset + rwD3DXScale;
    } else if ((clip & 3) == rwD3DCLIPLEFT) {
        tl->sx = rwD3DXOffset;
    } else {
        tl->sx = (parallel ? cam->x : cam->x * rhw) * rwD3DXScale + rwD3DXOffset;
    }
    if ((clip & 0xf) == rwD3DCLIPBOTTOM) {
        tl->sy = rwD3DYOffset + rwD3DYScale;
    } else if ((clip & 0xf) == rwD3DCLIPTOP) {
        tl->sy = rwD3DYOffset;
    } else {
        tl->sy = (parallel ? cam->y : cam->y * rhw) * rwD3DYScale + rwD3DYOffset;
    }
    if (flags == rwD3DCLIPFAR) {
        tl->sz = rwD3DFarScreenZ;
    } else if (flags == rwD3DCLIPNEAR) {
        tl->sz = rwD3DNearScreenZ;
    } else {
        tl->sz = (parallel ? cam->z : rhw) * rwD3DZScale + rwD3DZShift;
    }
    tl->u = cam->u;
    tl->v = cam->v;
}

/* Clips the ring against a pair of planes (those of the pair `clip` has); 0 when too little is left. */
static int clipPair(ClipRing *ring, uint8_t clip, uint8_t plane0, uint8_t plane1, RwD3DClipInterpFunc interp0,
                    RwD3DClipInterpFunc interp1, int line)
{
    const uint8_t plane[2] = {plane0, plane1};
    const RwD3DClipInterpFunc interp[2] = {interp0, interp1};

    for (int k = 0; k < 2; k++) {
        if ((clip & plane[k]) == 0) {
            continue;
        }
        if (line) {
            if (rwD3DClipLinePlane(ring->index, &ring->start, &ring->end, 0, 0, 0, plane[k], interp[k]) < 2) {
                return 0;
            }
        } else if (rwD3DClipPolyPlane(ring->index, &ring->start, &ring->end, 0, 0, 0, plane[k], interp[k]) <
                   3) {
            return 0;
        }
    }
    return 1;
}

/* The clip flags of the polygon in the ring, or'ed. */
static uint8_t ringClip(const ClipRing *ring)
{
    int32_t k = (ring->start + 1) & 15;
    uint8_t clip = clipOf(ring->index[ring->start]);

    do {
        clip |= clipOf(ring->index[k]);
        k = (k + 1) & 15;
    } while (k != ring->end);
    return clip;
}

static void fillRingTL(const ClipRing *ring, int parallel)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    int32_t k = ring->start;

    do {
        fillTL(ring->index[k], parallel);
        k = (k + 1) & 15;
    } while (k != ring->end);
    rwD3DRoundEnd(mxcsr);
}

/*
 * Clips a triangle near and far, top and bottom, left and right, gives the remaining vertices TL vertices and
 * emits the polygon as a fan. With flat shading the fan must start at a vertex with the triangle's colour
 * (that of a): the ring is rotated to a, or to the first new vertex, which then gets a's colour.
 */
static void clipTri(int32_t a, int32_t b, int32_t c, const ClipPlanes *planes)
{
    const RwD3DTLVertex *tlBase = rwD3DTLBase;
    ClipRing ring;
    uint8_t clip;
    int32_t first, k;

    ring.index[0] = a;
    ring.index[1] = b;
    ring.index[2] = c;
    ring.start = 0;
    ring.end = 3;
    clip = clipOf(b) | clipOf(a) | clipOf(c);
    if ((clip & (rwD3DCLIPNEAR | rwD3DCLIPFAR)) != 0) {
        if (!clipPair(&ring, clip, rwD3DCLIPNEAR, rwD3DCLIPFAR, planes->interp[0], planes->interp[1], 0)) {
            return;
        }
        clip = ringClip(&ring);
    }
    if ((clip & (rwD3DCLIPTOP | rwD3DCLIPBOTTOM)) != 0) {
        if (!clipPair(&ring, clip, rwD3DCLIPTOP, rwD3DCLIPBOTTOM, planes->interp[2], planes->interp[3], 0)) {
            return;
        }
        clip = ringClip(&ring);
    }
    if ((clip & (rwD3DCLIPLEFT | rwD3DCLIPRIGHT)) != 0 &&
        !clipPair(&ring, clip, rwD3DCLIPLEFT, rwD3DCLIPRIGHT, planes->interp[4], planes->interp[5], 0)) {
        return;
    }
    fillRingTL(&ring, planes->parallel);

    first = ring.index[ring.start];
    if (rwIm3DGouraud == 0 && first != a) {
        const RwD3DIm3DLevel *level = rwD3DIm3DLevel;
        for (;;) {
            if (first >= level->numVertices) {
                rwD3DTLBase[first].color = tlBase[a].color;
                break;
            }
            ring.index[ring.end] = first;
            ring.start = (ring.start + 1) & 15;
            ring.end = (ring.end + 1) & 15;
            first = ring.index[ring.start];
            if (first == a) {
                break;
            }
        }
    }
    k = (ring.start + 1) & 15;
    for (int32_t j = (ring.start + 2) & 15; j != ring.end; j = (j + 1) & 15) {
        rwD3DIndexTri(first, ring.index[k], ring.index[j]);
        k = j;
    }
}

/*
 * The same for a line. With flat shading the line must start at a vertex with a's colour: a itself, else the
 * new vertex at the other end of b, else the first vertex, which gets a's colour.
 */
static void clipLine(int32_t a, int32_t b, const ClipPlanes *planes)
{
    const RwD3DTLVertex *tlBase = rwD3DTLBase;
    ClipRing ring;
    uint8_t clip;
    int32_t v0, v1;

    ring.index[0] = a;
    ring.index[1] = b;
    ring.start = 0;
    ring.end = 2;
    clip = clipOf(b) | clipOf(a);
    if ((clip & (rwD3DCLIPNEAR | rwD3DCLIPFAR)) != 0) {
        if (!clipPair(&ring, clip, rwD3DCLIPNEAR, rwD3DCLIPFAR, planes->interp[0], planes->interp[1], 1)) {
            return;
        }
        clip = clipOf(ring.index[(ring.start + 1) & 15]) | clipOf(ring.index[ring.start]);
    }
    if ((clip & (rwD3DCLIPTOP | rwD3DCLIPBOTTOM)) != 0) {
        if (!clipPair(&ring, clip, rwD3DCLIPTOP, rwD3DCLIPBOTTOM, planes->interp[2], planes->interp[3], 1)) {
            return;
        }
        clip = clipOf(ring.index[(ring.start + 1) & 15]) | clipOf(ring.index[ring.start]);
    }
    if ((clip & (rwD3DCLIPLEFT | rwD3DCLIPRIGHT)) != 0 &&
        !clipPair(&ring, clip, rwD3DCLIPLEFT, rwD3DCLIPRIGHT, planes->interp[4], planes->interp[5], 1)) {
        return;
    }
    fillRingTL(&ring, planes->parallel);

    /* The original reads ring[start + 1] without wrapping; start is at most 12 here (6 planes, 2 each). */
    v0 = ring.index[ring.start];
    v1 = ring.index[ring.start + 1];
    if (rwIm3DGouraud != 0 || v0 == a) {
        rwD3DIndexLine(v0, v1);
    } else if (v1 == a) {
        rwD3DIndexLine(v1, v0);
    } else if (v0 == b) {
        rwD3DTLBase[v1].color = tlBase[a].color;
        rwD3DIndexLine(v1, v0);
    } else {
        rwD3DTLBase[v0].color = tlBase[a].color;
        rwD3DIndexLine(v0, v1);
    }
}

/* 0x4f3e40 (rwD3DClipTri of perspective cameras). */
BMX_ADDR(0x004f3e40)
void rwD3DClipTriPersp(int32_t a, int32_t b, int32_t c)
{
    clipTri(a, b, c, &kPerspPlanes);
}

/* 0x4f4ea0 (rwD3DClipTri of parallel cameras). */
BMX_ADDR(0x004f4ea0)
void rwD3DClipTriParallel(int32_t a, int32_t b, int32_t c)
{
    clipTri(a, b, c, &kParallelPlanes);
}

/* 0x4f5450 (rwD3DClipLine of perspective cameras). */
BMX_ADDR(0x004f5450)
void rwD3DClipLinePersp(int32_t a, int32_t b)
{
    clipLine(a, b, &kPerspPlanes);
}

/* 0x4f59b0 (rwD3DClipLine of parallel cameras). */
BMX_ADDR(0x004f59b0)
void rwD3DClipLineParallel(int32_t a, int32_t b)
{
    clipLine(a, b, &kParallelPlanes);
}
