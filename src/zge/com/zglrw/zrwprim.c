/* Renderer primitives (C:\NgEngine\Zge\Com\ZglRw\zrwPrim.c). */
#include "zge/com/zglrw/zrwprim.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kPrimFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwPrim.c";

/*
 * Colour channel of the point where `t` of the way from b to a: the products stay in the x87 register
 * and _ftol truncates them (to an int64, of which only the low byte is stored).
 */
static uint8_t lerpChannel(uint8_t a, uint8_t b, double t)
{
    return (uint8_t)bmxFtol((double)((int32_t)a - (int32_t)b) * t + (double)(int32_t)b);
}

/* Everything of the interpolation except the coordinate that is being clipped against. */
static void lerpRest(const ZrwPrimVertex *a, const ZrwPrimVertex *b, ZrwPrimVertex *out, double t)
{
    out->u = (float)(((double)a->u - (double)b->u) * t + (double)b->u);
    out->v = (float)(((double)a->v - (double)b->v) * t + (double)b->v);
    for (int i = 0; i < 4; i++) {
        out->color[i] = lerpChannel(a->color[i], b->color[i], t);
    }
}

/*
 * The ratio is 1 when the truncated coordinates of both ends are equal (which avoids dividing by a tiny
 * difference), so a clipped vertex then lies on a.
 */
BMX_ADDR(0x0048c620)
void zrwprimLerpAtX(float x, const ZrwPrimVertex *a, const ZrwPrimVertex *b, ZrwPrimVertex *out)
{
    double t;

    if (bmxFtol((double)a->x) == bmxFtol((double)b->x)) {
        t = 1.0f;
    } else {
        t = ((double)x - (double)b->x) / ((double)a->x - (double)b->x);
    }
    out->x = x;
    out->y = (float)(((double)a->y - (double)b->y) * t + (double)b->y);
    lerpRest(a, b, out, t);
}

BMX_ADDR(0x0048c730)
void zrwprimLerpAtY(float y, const ZrwPrimVertex *a, const ZrwPrimVertex *b, ZrwPrimVertex *out)
{
    double t;

    if (bmxFtol((double)a->y) == bmxFtol((double)b->y)) {
        t = 1.0f;
    } else {
        t = ((double)y - (double)b->y) / ((double)a->y - (double)b->y);
    }
    out->y = y;
    out->x = (float)(((double)a->x - (double)b->x) * t + (double)b->x);
    lerpRest(a, b, out, t);
}

enum { kClipMaxVertices = 32, kClipPoolVertices = 64 };

typedef void (*ClipLerpFn)(float, const ZrwPrimVertex *, const ZrwPrimVertex *, ZrwPrimVertex *);

typedef struct ClipPass {
    int y;         /* clips along y (else x) */
    int keepBelow; /* keeps coordinates below the limit (else above) */
    float limit;
} ClipPass;

static float coordinate(const ZrwPrimVertex *v, int y)
{
    return y ? v->y : v->x;
}

/* A vertex on the wrong side of the limit; NaN counts as outside above and as inside below (x87 compare). */
static int outside(float c, const ClipPass *pass)
{
    return pass->keepBelow ? (pass->limit <= c) : !(c > pass->limit);
}

/* One Sutherland-Hodgman pass over a closed polygon; returns the number of vertices written to out. */
static uint32_t clipPass(ZrwPrimVertex *const *in, uint32_t count, ZrwPrimVertex **out, const ClipPass *pass,
                         ZrwPrimVertex **pool)
{
    const ClipLerpFn lerp = pass->y ? zrwprimLerpAtY : zrwprimLerpAtX;
    ZrwPrimVertex *prev = in[count - 1];
    uint32_t n = 0;

    for (uint32_t i = 0; i < count; i++) {
        ZrwPrimVertex *cur = in[i];
        const int prevOut = outside(coordinate(prev, pass->y), pass);
        const int curOut = outside(coordinate(cur, pass->y), pass);

        if (prevOut) {
            if (!curOut) {
                lerp(pass->limit, cur, prev, *pool);
                out[n++] = (*pool)++;
                out[n++] = cur;
            }
        } else if (curOut) {
            lerp(pass->limit, prev, cur, *pool);
            out[n++] = (*pool)++;
        } else {
            out[n++] = cur;
        }
        prev = cur;
    }
    return n;
}

BMX_ADDR(0x0048c840)
uint32_t zrwprimClipPoly(uint32_t flags, uint32_t texture, ZrwPrimVertex *const *vertices, uint32_t count,
                         const ZMatrix *xform, const float *rect)
{
    ZrwPrimVertex storage[kClipPoolVertices];
    ZrwPrimVertex *pool = storage;
    ZrwPrimVertex *listA[kClipMaxVertices];
    ZrwPrimVertex *listB[kClipMaxVertices];
    ZrwPrimVertex *const *in;
    ZrwPrimVertex **out;
    ZrwPrimPoly poly;
    const ClipPass passes[4] = {
        {1, 0, rect[1]}, /* below the top edge */
        {0, 0, rect[0]}, /* right of the left edge */
        {1, 1, rect[3]}, /* above the bottom edge */
        {0, 1, rect[2]}, /* left of the right edge */
    };
    uint32_t n = count;

    if (count == 0) {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        const ZrwPrimVertex *src = vertices[i];

        pool->u = src->u;
        pool->v = src->v;
        __builtin_memcpy(pool->color, src->color, 4);
        if (xform == NULL) {
            pool->x = src->x;
            pool->y = src->y;
            pool->z = src->z;
        } else {
            zmatrixTransformPoint((ZVec3 *)pool, xform, (const ZVec3 *)src);
        }
        listA[i] = pool++;
    }
    in = listA;
    out = listB;
    for (int p = 0; p < 4; p++) {
        n = clipPass(in, n, out, &passes[p], &pool);
        if (n < 3) {
            return 0;
        }
        in = out;
        out = (out == listB) ? listA : listB;
    }
    poly.texture = texture;
    poly.vertices = in;
    poly.count = n;
    zrwprimDrawPolys(flags, &poly, 1, 0, 0);
    return n;
}

BMX_ADDR(0x0048cc30)
int zrwPrimSystemInitialize(void)
{
    if (gZrwPrim.initialized != 0) {
        zdebugFail("Already initialized", kPrimFile, 0x248, 0);
    }
    zmemFill(&gZrwPrim, 0, sizeof gZrwPrim);
    gZrwPrim.initialized = 1;
    gZrwPrim.vertexSize = 0x24;
    gZrwPrim.vertexMax = 0x40;
    gZrwPrim.vertices = zmemAlloc(0x90f);
    gZrwPrim.indexMax = 0x40;
    gZrwPrim.indices = zmemAlloc(0x80f);
    return 1;
}

BMX_ADDR(0x0048ccb0)
int zrwPrimSystemShutdown(void)
{
    if (gZrwPrim.initialized != 0) {
        if (gZrwPrim.vertices != NULL) {
            zmemFree(gZrwPrim.vertices);
        }
        if (gZrwPrim.indices != NULL) {
            zmemFree(gZrwPrim.indices);
        }
        gZrwPrim.initialized = 0;
    }
    return 1;
}

BMX_ADDR(0x0048d120)
void zrwprimDrawTriangles(uint32_t flags, const ZrwPrimTriangle *triangles, int count, uint32_t arg)
{
    do {
        ZrwPrimPoly poly = {triangles->texture, triangles->vertices, 3};

        zrwprimDrawPolys(flags, &poly, 1, arg, 0);
        triangles++;
        count--;
    } while (count != 0);
}

BMX_ADDR(0x0048d5a0)
void zrwprimDrawStrips(uint32_t flags, const ZrwPrimPoly *strips, int count, uint32_t arg)
{
    do {
        ZrwPrimVertex *other[3];
        ZrwPrimVertex *const *cursor = strips->vertices;
        int32_t a = (int32_t)strips->count - 2;
        int32_t b = (int32_t)strips->count - 3;
        const ZrwPrimPoly second = {strips->texture, other, 3};

        while (a > 0) {
            const ZrwPrimPoly first = {strips->texture, cursor, 3};

            zrwprimDrawPolys(flags, &first, 1, arg, 0);
            a = b - 1;
            if (b > 0) {
                other[0] = cursor[3];
                other[1] = cursor[2];
                other[2] = cursor[1];
                zrwprimDrawPolys(flags, &second, 1, arg, 0);
            }
            cursor += 2;
            b -= 2;
        }
        strips++;
        count--;
    } while (count != 0);
}

BMX_ADDR(0x0048d660)
void zrwprimDrawQuads(uint32_t flags, const ZrwPrimQuad *quads, int count, uint32_t arg)
{
    if ((flags & 0x2000000) != 0) {
        const uint8_t *camera = zrwGetClipCamera();

        if ((camera[8] & 1) != 0) {
            zrwprimClipPoly(flags, quads->texture, quads->vertices, 4, (const ZMatrix *)(uintptr_t)arg,
                            (const float *)(camera + 0x94));
            return;
        }
    }
    do {
        ZrwPrimPoly poly = {quads->texture, quads->vertices, 4};

        zrwprimDrawPolys(flags, &poly, 1, arg, 0);
        quads++;
        count--;
    } while (count != 0);
}
