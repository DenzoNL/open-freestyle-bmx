/*
 * The maths of the widescreen seams (docs/WIDESCREEN.md W2, W4-W7): view window, 2D classifier and
 * transforms, present rectangle. No SDL and no game state, so tests/platform/test_wide.c links it alone.
 */
#include <math.h>

#include "driver/wide.h"

int drvWideIs43(int32_t w, int32_t h)
{
    return (int64_t)w * 3 == (int64_t)h * 4;
}

int drvWideViewWindow(const DrvWideConfig *cfg, int32_t w, int32_t h, float window[2])
{
    double x = window[0], y = window[1];
    int changed = 0;

    if (w <= 0 || h <= 0) {
        return 0;
    }
    if (cfg->aspect == 1 && !drvWideIs43(w, h)) {
        if ((int64_t)w * 3 > (int64_t)h * 4) {
            x = y * w / h; /* hor+: the 4:3 view's height, wider */
        } else {
            y = x * h / w; /* vert+ (5:4): the 4:3 view's width, taller */
        }
        changed = 1;
    } else if (cfg->aspect == 2 && !drvWideIs43(w, h)) {
        y = x * h / w; /* vert-: the 4:3 view's width, cropped (or grown) vertically */
        changed = 1;
    }
    if (cfg->fovScale > 0 && cfg->fovScale != 100) {
        x = x * cfg->fovScale / 100.0;
        y = y * cfg->fovScale / 100.0;
        changed = 1;
    }
    if (cfg->maxHfov > 0 && cfg->maxHfov < 180) {
        const double cap = tan((double)cfg->maxHfov * 3.14159265358979323846 / 360.0);

        if (x > cap) {
            y = y * cap / x;
            x = cap;
            changed = 1;
        }
    }
    if (!changed) {
        return 0;
    }
    window[0] = (float)x;
    window[1] = (float)y;
    return 1;
}

int drvWideClassify(const float *xy, size_t stride, int32_t n, int textured, int32_t w, int32_t h)
{
    const double tol = 2.0 * w / 640.0;
    float lo, hi;

    (void)h;
    if (n <= 0) {
        return kDrvWideOther;
    }
    lo = hi = xy[0];
    for (int32_t i = 1; i < n; i++) {
        const float x = *(const float *)((const uint8_t *)xy + (size_t)i * stride);

        lo = x < lo ? x : lo;
        hi = x > hi ? x : hi;
    }
    if (lo > tol || hi < w - tol) {
        return kDrvWideOther;
    }
    return textured ? kDrvWideBackdrop : kDrvWideCover;
}

int drvWideMoves(const DrvWideConfig *cfg, int cls)
{
    switch (cls) {
    case kDrvWideCover:
        return 0;
    case kDrvWideBackdrop:
        return cfg->menus != 1;
    default:
        return cfg->hud != 2;
    }
}

void drvWideMap2D(float *xy, size_t stride, int32_t n, int32_t w, int32_t h)
{
    if (w <= 0 || h <= 0 || drvWideIs43(w, h)) {
        return;
    }
    if ((int64_t)w * 3 > (int64_t)h * 4) {
        const double inner = h * 4.0 / 3.0;
        const double off = (w - inner) / 2.0, s = inner / w;

        for (int32_t i = 0; i < n; i++) {
            float *const x = (float *)((uint8_t *)xy + (size_t)i * stride);

            *x = (float)(off + *x * s);
        }
    } else {
        const double inner = w * 3.0 / 4.0;
        const double off = (h - inner) / 2.0, s = inner / h;

        for (int32_t i = 0; i < n; i++) {
            float *const y = (float *)((uint8_t *)xy + (size_t)i * stride) + 1;

            *y = (float)(off + *y * s);
        }
    }
}

int drvWideArea(int32_t w, int32_t h, float *lo, float *hi)
{
    if (w <= 0 || h <= 0 || drvWideIs43(w, h)) {
        return -1;
    }
    if ((int64_t)w * 3 > (int64_t)h * 4) {
        const double inner = h * 4.0 / 3.0;

        *lo = (float)((w - inner) / 2.0);
        *hi = (float)((w - inner) / 2.0 + inner);
        return 0;
    }
    {
        const double inner = w * 3.0 / 4.0;

        *lo = (float)((h - inner) / 2.0);
        *hi = (float)((h - inner) / 2.0 + inner);
        return 1;
    }
}

static float coord(const DrvWideVertex *v, int axis)
{
    return axis == 0 ? v->sx : v->sy;
}

static uint32_t lerpColor(uint32_t a, uint32_t b, float t)
{
    uint32_t out = 0;

    for (int shift = 0; shift < 32; shift += 8) {
        const float ca = (float)((a >> shift) & 0xff), cb = (float)((b >> shift) & 0xff);
        const long c = lroundf(ca + (cb - ca) * t);

        out |= (uint32_t)(c < 0 ? 0 : c > 255 ? 255 : c) << shift;
    }
    return out;
}

/* The point of a..b where the axis coordinate is `at` (2D: screen-linear attributes). */
static void cut(const DrvWideVertex *a, const DrvWideVertex *b, int axis, float at, DrvWideVertex *out)
{
    const float t = (at - coord(a, axis)) / (coord(b, axis) - coord(a, axis));
    DrvWideVertex r; /* out may be a or b */

    r.sx = axis == 0 ? at : a->sx + (b->sx - a->sx) * t;
    r.sy = axis == 1 ? at : a->sy + (b->sy - a->sy) * t;
    r.sz = a->sz + (b->sz - a->sz) * t;
    r.rhw = a->rhw + (b->rhw - a->rhw) * t;
    r.color = lerpColor(a->color, b->color, t);
    r.specular = lerpColor(a->specular, b->specular, t);
    r.u = a->u + (b->u - a->u) * t;
    r.v = a->v + (b->v - a->v) * t;
    *out = r;
}

/* One Sutherland-Hodgman pass of a convex polygon against coord >= at (side +1) or <= at (side -1). */
static int32_t clipPlane(const DrvWideVertex *in, int32_t n, int axis, float at, int side, DrvWideVertex *out)
{
    int32_t m = 0;

    for (int32_t i = 0; i < n; i++) {
        const DrvWideVertex *const a = &in[i], *const b = &in[(i + 1) % n];
        const int aIn = side > 0 ? coord(a, axis) >= at : coord(a, axis) <= at;
        const int bIn = side > 0 ? coord(b, axis) >= at : coord(b, axis) <= at;

        if (aIn) {
            out[m++] = *a;
        }
        if (aIn != bIn) {
            cut(a, b, axis, at, &out[m++]);
        }
    }
    return m;
}

int32_t drvWideClipFan(const DrvWideVertex *in, int32_t n, int axis, float lo, float hi, DrvWideVertex *out,
                       DrvWideVertex *scratch)
{
    int32_t m;

    if (n < 3) {
        return 0;
    }
    m = clipPlane(in, n, axis, lo, 1, scratch);
    return m < 3 ? 0 : clipPlane(scratch, m, axis, hi, -1, out);
}

int32_t drvWideClipLines(const DrvWideVertex *in, int32_t n, int axis, float lo, float hi, DrvWideVertex *out)
{
    int32_t m = 0;

    for (int32_t i = 0; i + 1 < n; i += 2) {
        DrvWideVertex a = in[i], b = in[i + 1];

        if ((coord(&a, axis) < lo && coord(&b, axis) < lo) ||
            (coord(&a, axis) > hi && coord(&b, axis) > hi)) {
            continue;
        }
        if (coord(&a, axis) < lo) {
            cut(&a, &b, axis, lo, &a);
        } else if (coord(&b, axis) < lo) {
            cut(&b, &a, axis, lo, &b);
        }
        if (coord(&a, axis) > hi) {
            cut(&a, &b, axis, hi, &a);
        } else if (coord(&b, axis) > hi) {
            cut(&b, &a, axis, hi, &b);
        }
        out[m++] = a;
        out[m++] = b;
    }
    return m;
}

int drvWideOutside(const DrvWideVertex *v, int32_t n, int axis, float lo, float hi)
{
    for (int32_t i = 0; i < n; i++) {
        if (coord(&v[i], axis) < lo || coord(&v[i], axis) > hi) {
            return 1;
        }
    }
    return 0;
}

void drvWideMapRect(int32_t rect[4], int32_t w, int32_t h)
{
    float edges[2][2] = {{(float)rect[0], (float)rect[1]},
                         {(float)(rect[0] + rect[2]), (float)(rect[1] + rect[3])}};

    if (w <= 0 || h <= 0 || drvWideIs43(w, h)) {
        return;
    }
    drvWideMap2D(&edges[0][0], sizeof edges[0], 2, w, h);
    rect[0] = (int32_t)lround(edges[0][0]);
    rect[1] = (int32_t)lround(edges[0][1]);
    rect[2] = (int32_t)lround(edges[1][0]) - rect[0];
    rect[3] = (int32_t)lround(edges[1][1]) - rect[1];
}

int drvWidePresentRect(int filter, int32_t width, int32_t height, uint32_t w, uint32_t h, uint32_t out[4])
{
    if (filter == 2 && width > 0 && height > 0) {
        const uint32_t fx = w / (uint32_t)width, fy = h / (uint32_t)height;
        const uint32_t f = fx < fy ? fx : fy;

        if (f >= 1) {
            out[2] = (uint32_t)width * f;
            out[3] = (uint32_t)height * f;
            out[0] = (w - out[2]) / 2;
            out[1] = (h - out[3]) / 2;
            return 1;
        }
    }
    {
        /* Letterbox (as gpu.c always did): the largest scale that fits, rounded, centred. */
        const double sx = (double)w / width, sy = (double)h / height;
        const double scale = sx < sy ? sx : sy;
        const uint32_t dw = (uint32_t)(width * scale + 0.5), dh = (uint32_t)(height * scale + 0.5);

        out[0] = (w - dw) / 2;
        out[1] = (h - dh) / 2;
        out[2] = dw;
        out[3] = dh;
    }
    return filter == 1;
}
