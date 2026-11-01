/*
 * The maths of the widescreen seams (docs/WIDESCREEN.md W2, W4-W7): view window, 2D classifier and
 * transforms, present rectangle. No SDL and no game state, so tests/platform/test_wide.c links it alone.
 */
#include <math.h>
#include <string.h>

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

/* ---- HUD anchoring (WS5) ---- */

int drvWideHudAnchorOf(int32_t x0, int32_t x1)
{
    const int32_t twiceMiddle = x0 + x1;

    if (twiceMiddle >= 640 - 16 && twiceMiddle <= 640 + 16) {
        return kDrvWideAnchorCentre;
    }
    return twiceMiddle < 640 ? kDrvWideAnchorLeft : kDrvWideAnchorRight;
}

static int16_t rdS16(const uint8_t *p, unsigned off)
{
    return (int16_t)(uint16_t)(p[off] | p[off + 1] << 8);
}

static uint32_t rdU32(const uint8_t *p, unsigned off)
{
    return (uint32_t)p[off] | (uint32_t)p[off + 1] << 8 | (uint32_t)p[off + 2] << 16 |
           (uint32_t)p[off + 3] << 24;
}

int drvWidePacketBox(const uint8_t *packet, int16_t box[4])
{
    /* The corner offsets and the flag word of each command, as ngifDrawPoly* / ngifDrawLine* read them. */
    static const struct {
        uint8_t code, flagOff, count, xy[4];
    } kinds[] = {
        {0x20, 0x18, 3, {8, 0xc, 0x10, 0}},       /* F3 */
        {0x28, 0x1c, 4, {8, 0xc, 0x10, 0x14}},    /* F4 */
        {0x2c, 0x34, 4, {8, 0x12, 0x1c, 0x26}},   /* FT4 */
        {0x38, 0x28, 4, {8, 0x10, 0x18, 0x20}},   /* G4 */
        {0x3c, 0x3c, 4, {0xc, 0x18, 0x24, 0x30}}, /* GT4 */
        {0x40, 0x14, 2, {8, 0xc, 0, 0}},          /* F2 */
        {0x50, 0x18, 2, {8, 0x10, 0, 0}},         /* G2 */
    };
    const uint8_t code = packet[7] & 0xfc;

    for (size_t k = 0; k < sizeof kinds / sizeof kinds[0]; k++) {
        if (kinds[k].code != code) {
            continue;
        }
        if ((rdU32(packet, kinds[k].flagOff) & 0x20) != 0) {
            return 0; /* float corners: the 3D list, not 2D */
        }
        box[0] = box[2] = rdS16(packet, kinds[k].xy[0]);
        box[1] = box[3] = rdS16(packet, kinds[k].xy[0] + 2u);
        for (int i = 1; i < kinds[k].count; i++) {
            const int16_t x = rdS16(packet, kinds[k].xy[i]), y = rdS16(packet, kinds[k].xy[i] + 2u);

            box[0] = x < box[0] ? x : box[0];
            box[2] = x > box[2] ? x : box[2];
            box[1] = y < box[1] ? y : box[1];
            box[3] = y > box[3] ? y : box[3];
        }
        return 1;
    }
    return 0;
}

enum {
    kItemKind = 0x9c,
    kItemNext = 0x9e,
    kKindGlyph = 5,
};

static uint32_t itemNext(const uint8_t *item)
{
    return (uint32_t)(item[kItemNext] | item[kItemNext + 1] << 8);
}

static void boxUnion(int16_t into[4], const int16_t box[4])
{
    into[0] = box[0] < into[0] ? box[0] : into[0];
    into[1] = box[1] < into[1] ? box[1] : into[1];
    into[2] = box[2] > into[2] ? box[2] : into[2];
    into[3] = box[3] > into[3] ? box[3] : into[3];
}

/* The anchor of `box`, or of the widest (and wider) text in `texts` that holds it on the same lines. */
static uint8_t anchorWithin(const int16_t box[4], int16_t (*texts)[4], const uint8_t *textAnchor, int32_t n,
                            int32_t self)
{
    int32_t best = -1;

    for (int32_t t = 0; t < n; t++) {
        const int16_t *const c = texts[t];

        if (t == self || c[0] > box[0] || c[2] < box[2] || c[1] > box[3] || c[3] < box[1] ||
            c[2] - c[0] <= box[2] - box[0]) {
            continue;
        }
        if (best < 0 || c[2] - c[0] > texts[best][2] - texts[best][0]) {
            best = t;
        }
    }
    return best >= 0 ? textAnchor[best] : (uint8_t)drvWideHudAnchorOf(box[0], box[2]);
}

void drvWideHudPools(const uint8_t *glyphs, int32_t nGlyphs, const uint8_t *shapes, int32_t nShapes,
                     uint8_t *glyphAnchor, uint8_t *shapeAnchor)
{
    static int16_t texts[kDrvWideMaxItems][4];
    static uint8_t textAnchor[kDrvWideMaxItems], own[kDrvWideMaxItems];
    static int32_t textOf[kDrvWideMaxItems];
    static uint8_t referenced[kDrvWideMaxItems];
    int32_t nTexts = 0;

    nGlyphs = nGlyphs > kDrvWideMaxItems ? kDrvWideMaxItems : nGlyphs;
    nShapes = nShapes > kDrvWideMaxItems ? kDrvWideMaxItems : nShapes;
    memset(referenced, 0, sizeof referenced);
    for (int32_t i = 0; i < nGlyphs; i++) {
        const uint8_t *const it = glyphs + (size_t)i * kDrvWideItemSize;
        const uint32_t next = itemNext(it);

        textOf[i] = -1;
        if (it[kItemKind] == kKindGlyph && next != 0xffff && (int32_t)(next & 0xfff) < nGlyphs) {
            referenced[next & 0xfff] = 1;
        }
    }
    /* Every chain from a glyph nobody points at is one text (zoverAddText's handle is its first glyph). */
    for (int32_t head = 0; head < nGlyphs; head++) {
        int16_t box[4] = {0, 0, 0, 0};
        int32_t i = head;
        int found = 0;

        if (glyphs[(size_t)head * kDrvWideItemSize + kItemKind] != kKindGlyph || referenced[head]) {
            continue;
        }
        for (int32_t steps = 0; steps < nGlyphs && i >= 0 && textOf[i] < 0; steps++) {
            const uint8_t *const it = glyphs + (size_t)i * kDrvWideItemSize;
            const uint32_t next = itemNext(it);
            int16_t g[4];

            if (it[kItemKind] != kKindGlyph) {
                break;
            }
            textOf[i] = nTexts;
            if (drvWidePacketBox(it, g)) {
                if (found) {
                    boxUnion(box, g);
                } else {
                    memcpy(box, g, sizeof box);
                    found = 1;
                }
            }
            i = next == 0xffff || (int32_t)(next & 0xfff) >= nGlyphs ? -1 : (int32_t)(next & 0xfff);
        }
        memcpy(texts[nTexts], box, sizeof box);
        textAnchor[nTexts] = (uint8_t)drvWideHudAnchorOf(box[0], box[2]);
        nTexts++;
    }
    /* A text inside a wider one on the same lines (a hint's button glyph) goes with it. */
    for (int32_t t = 0; t < nTexts; t++) {
        own[t] = anchorWithin(texts[t], texts, textAnchor, nTexts, t);
    }
    memcpy(textAnchor, own, (size_t)nTexts);
    for (int32_t i = 0; i < nGlyphs; i++) {
        const uint8_t *const it = glyphs + (size_t)i * kDrvWideItemSize;
        int16_t g[4];

        if (it[kItemKind] != kKindGlyph) {
            glyphAnchor[i] = kDrvWideAnchorNone;
        } else if (textOf[i] >= 0) {
            glyphAnchor[i] = textAnchor[textOf[i]];
        } else { /* a cycle without a head (zoverAddText never makes one) */
            glyphAnchor[i] = drvWidePacketBox(it, g) ? (uint8_t)drvWideHudAnchorOf(g[0], g[2])
                                                     : (uint8_t)kDrvWideAnchorCentre;
        }
    }
    for (int32_t i = 0; i < nShapes; i++) {
        const uint8_t *const it = shapes + (size_t)i * kDrvWideItemSize;
        int16_t g[4];

        if (it[kItemKind] == 0) {
            shapeAnchor[i] = kDrvWideAnchorNone;
        } else if (!drvWidePacketBox(it, g)) {
            shapeAnchor[i] = kDrvWideAnchorCentre;
        } else {
            shapeAnchor[i] = anchorWithin(g, texts, textAnchor, nTexts, -1);
        }
    }
}

int32_t drvWideGlyphRun(const uint8_t *const *packets, int32_t n)
{
    int16_t first[4], prev[4];
    int32_t i;

    if (n <= 0) {
        return 0;
    }
    if ((packets[0][7] & 0xfc) != 0x2c || !drvWidePacketBox(packets[0], first)) {
        return 1;
    }
    memcpy(prev, first, sizeof prev);
    for (i = 1; i < n; i++) {
        const uint8_t *const p = packets[i];
        int16_t box[4];

        if ((p[7] & 0xfc) != 0x2c || !drvWidePacketBox(p, box) || box[1] != first[1] || box[3] != first[3]) {
            break;
        }
        /* The glyph before it in the text (draw order is reversed), or after it. */
        if (!(p == packets[i - 1] - 0x9c && (uint16_t)(box[2] + 1) == (uint16_t)prev[0]) &&
            !(p == packets[i - 1] + 0x9c && (uint16_t)(prev[2] + 1) == (uint16_t)box[0])) {
            break;
        }
        memcpy(prev, box, sizeof prev);
    }
    return i;
}

int drvWideHudArea(int32_t w, int32_t h, int hudAspect, float *lo, float *hi)
{
    double inner, area;
    int32_t left;

    if (w <= 0 || h <= 0 || (int64_t)w * 3 <= (int64_t)h * 4) {
        return 0;
    }
    inner = h * 4.0 / 3.0;
    area = hudAspect <= 0 ? (double)w : h * (double)hudAspect / 100.0;
    area = area < inner ? inner : area > w ? (double)w : area;
    left = (w - (int32_t)lround(area)) / 2;
    *lo = (float)left;
    *hi = (float)(w - left);
    return 1;
}

int drvWideMapHud(float *xy, size_t stride, int32_t n, int32_t w, int32_t h, int anchor, int hudAspect,
                  float *lo, float *hi)
{
    float areaLo, areaHi;
    double inner, off, s;

    if ((anchor != kDrvWideAnchorLeft && anchor != kDrvWideAnchorRight) ||
        !drvWideHudArea(w, h, hudAspect, &areaLo, &areaHi)) {
        drvWideMap2D(xy, stride, n, w, h);
        return drvWideArea(w, h, lo, hi);
    }
    inner = h * 4.0 / 3.0;
    s = inner / w;
    off = anchor == kDrvWideAnchorLeft ? (double)areaLo : (double)areaHi - inner;
    for (int32_t i = 0; i < n; i++) {
        float *const x = (float *)((uint8_t *)xy + (size_t)i * stride);

        *x = (float)(off + *x * s);
    }
    *lo = (float)off;
    *hi = (float)(off + inner);
    return 0;
}

int drvWideBars(const DrvWideConfig *cfg, int cls, int32_t w, int32_t h, float rects[2][4])
{
    float lo = 0.0f, hi = 0.0f;
    const int axis = drvWideArea(w, h, &lo, &hi);

    if (cls != kDrvWideBackdrop || cfg->menus == 1 || axis < 0) {
        return 0;
    }
    if (axis == 0) {
        const float r0[4] = {0.0f, 0.0f, lo, (float)h}, r1[4] = {hi, 0.0f, (float)w, (float)h};

        memcpy(rects[0], r0, sizeof r0);
        memcpy(rects[1], r1, sizeof r1);
    } else {
        const float r0[4] = {0.0f, 0.0f, (float)w, lo}, r1[4] = {0.0f, hi, (float)w, (float)h};

        memcpy(rects[0], r0, sizeof r0);
        memcpy(rects[1], r1, sizeof r1);
    }
    return 2;
}

int drvWideBlurFill(const DrvWideConfig *cfg, int cls)
{
    return cls == kDrvWideBackdrop && cfg->menus == 2;
}

float drvWideSharpCoord(float t, float s)
{
    const float centre = floorf(t) + 0.5f;
    float x;

    s = s < 1.0f ? 1.0f : s;
    x = (t - centre) * s;
    return centre + (x < -0.5f ? -0.5f : x > 0.5f ? 0.5f : x);
}
