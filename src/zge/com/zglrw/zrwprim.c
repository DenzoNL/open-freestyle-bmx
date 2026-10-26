/* Renderer primitives (C:\NgEngine\Zge\Com\ZglRw\zrwPrim.c). */
#include "zge/com/zglrw/zrwprim.h"

#include "compat/crt.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kPrimFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwPrim.c";

/* RenderWare (statically linked, not ported): immediate mode and camera queries. */
ORIG_FN(0x004de610, float, RwCameraGetNearClipPlane, (void *camera));
ORIG_FN(0x004db800, float, RwIm2DGetNearScreenZ, (void));
ORIG_FN(0x004db920, int, RwIm2DRenderPrimitive, (int type, void *vertices, int count));
ORIG_FN(0x004df0b0, int, RwIm3DTransform, (void *vertices, int count, const ZMatrix *matrix, int flags));
ORIG_FN(0x004def70, int, RwIm3DRenderPrimitive, (int type));
ORIG_FN(0x004df090, int, RwIm3DEnd, (void));
ORIG_FN(0x004d3ae0, void *, RwV3dTransformVectors,
        (ZVec3 * out, const ZVec3 *in, int count, const ZMatrix *m));

/* The renderer's render state hook (0x561490): state bits and the raster to use. */
typedef void(BMX_CDECL *ZrwPrimStateHookFn)(uint32_t state, void *raster);
#define gZrwPrimStateHook ORIG_VAR(0x00561490, ZrwPrimStateHookFn, gZrwPrimStateHook)
#define gZrwPrimCamera    ORIG_VAR(0x006976f4, void *, gZrwPrimCamera)

enum { kPrimLineList = 1, kPrimTriFan = 5 };

/* RenderWare's 2D immediate mode vertex (RwIm2DVertex, 0x20 bytes). */
typedef struct ZrwIm2DVertex {
    float x, y, z, rhw;
    uint32_t color; /* 0x10 ARGB */
    uint32_t pad14;
    float u, v; /* 0x18 */
} ZrwIm2DVertex;

/* RenderWare's 3D immediate mode vertex (0x24 bytes); the normal is left as it is. */
typedef struct ZrwIm3DVertex {
    float x, y, z;
    float normal[3];
    uint8_t color[4]; /* 0x18 */
    float u, v;       /* 0x1c */
} ZrwIm3DVertex;

_Static_assert(sizeof(ZrwIm2DVertex) == 0x20, "2D vertex");
_Static_assert(sizeof(ZrwIm3DVertex) == 0x24, "3D vertex");

static void *align16(void *p)
{
    return (void *)(((uintptr_t)p + 0xf) & ~(uintptr_t)0xf);
}

static void primBeginCall(void)
{
    if (gZrwPrimFrame != gZrwFrames) {
        gZrwPrimFrame = gZrwFrames;
    }
    gZrwPrimDepthBias = 0.0f;
}

/* Screen depth of a 2D call: 1/near and the near screen z, each nudged by the bias, which then grows. */
static void primScreenDepth(float *rhw, float *z)
{
    *rhw = (float)(1.0 / (double)RwCameraGetNearClipPlane(gZrwPrimCamera) + (double)gZrwPrimDepthBias);
    *z = (float)((double)RwIm2DGetNearScreenZ() + (double)gZrwPrimDepthBias);
    gZrwPrimDepthBias = (float)((double)0x1.0624dep-10f + (double)gZrwPrimDepthBias);
}

/* Fills a 2D vertex: position as given (or transformed), colour bytes r g b a packed as ARGB. */
static void primFill2D(ZrwIm2DVertex *out, const ZrwPrimVertex *src, const ZMatrix *xform, float z, float rhw)
{
    if (xform != NULL) {
        ZVec3 p;

        zmatrixTransformPoint(&p, xform, (const ZVec3 *)src);
        out->x = p.x;
        out->y = p.y;
    } else {
        out->x = src->x;
        out->y = src->y;
    }
    out->z = z;
    out->rhw = rhw;
    out->u = src->u;
    out->v = src->v;
    out->color = ((uint32_t)src->color[3] << 24) | ((uint32_t)src->color[0] << 16) |
                 ((uint32_t)src->color[1] << 8) | src->color[2];
}

/*
 * Fills a 3D vertex: the game space point (transformed by `xform` first if given) scaled into RenderWare's
 * world (y and z mirrored about the world centre), or, `relative` to the camera, mirrored in x and y, rotated
 * and moved by the camera matrix.
 */
static void primFill3D(ZrwIm3DVertex *out, const ZrwPrimVertex *src, const ZMatrix *xform,
                       const ZMatrix *camera, int relative)
{
    const ZVec3 *p = (const ZVec3 *)src;
    ZVec3 moved;
    const double scale = gZrwInvScale;

    if (xform != NULL) {
        zmatrixTransformPoint(&moved, xform, p);
        p = &moved;
    }
    if (relative) {
        out->x = (float)-(scale * p->x);
        out->y = (float)-((double)p->y * scale);
        out->z = (float)((double)p->z * scale);
        RwV3dTransformVectors((ZVec3 *)out, (const ZVec3 *)out, 1, camera);
        out->x = (float)((double)camera->pos.x + (double)out->x);
        out->y = (float)((double)camera->pos.y + (double)out->y);
        out->z = (float)((double)camera->pos.z + (double)out->z);
    } else {
        out->x = (float)(scale * p->x + (double)gZrwOriginX);
        out->y = (float)((double)gZrwOriginY - (double)p->y * scale);
        out->z = (float)((double)gZrwOriginZ - (double)p->z * scale);
    }
    out->u = src->u;
    out->v = src->v;
    __builtin_memcpy(out->color, src->color, 4);
}

/*
 * Hands the render state and the texture's raster to the hook, unless both are what is set already. The
 * raster is looked up a second time for the hook.
 */
static void primSetTexture(uint32_t flags, uint32_t texture)
{
    const uint32_t state = flags & 0xffffff;
    ZrwMaterial *material = (ZrwMaterial *)(uintptr_t)texture;

    if (state == gZrwRenderFlags) {
        const void *raster = (material == NULL) ? NULL : zrwmatGetRaster(material, 0);

        if ((uint32_t)(uintptr_t)raster == (uint32_t)gZrwRenderExtra) {
            return;
        }
    }
    gZrwPrimStateHook(state, (material == NULL) ? NULL : zrwmatGetRaster(material, 0));
}

static const ZMatrix *primCameraMatrix(void)
{
    return zrwCameraGetWorldMatrix(zrwCameraGetCurrent());
}

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
    zrwprimDrawPolys(flags, &poly, 1, NULL);
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
    gZrwPrim.im3dMax = 0x40;
    gZrwPrim.im3d = zmemAlloc(0x90f);
    gZrwPrim.im2dMax = 0x40;
    gZrwPrim.im2d = zmemAlloc(0x80f);
    return 1;
}

BMX_ADDR(0x0048ccb0)
int zrwPrimSystemShutdown(void)
{
    if (gZrwPrim.initialized != 0) {
        if (gZrwPrim.im3d != NULL) {
            zmemFree(gZrwPrim.im3d);
        }
        if (gZrwPrim.im2d != NULL) {
            zmemFree(gZrwPrim.im2d);
        }
        gZrwPrim.initialized = 0;
    }
    return 1;
}

/* Draws `count` lines as 2 vertex line lists; the buffers only ever grow to 0x20 vertices here. */
BMX_ADDR(0x0048ccf0)
void zrwprimDrawLines(uint32_t flags, const ZrwPrimLine *lines, int count, const ZMatrix *xform)
{
    primBeginCall();
    if ((flags & 0x2000000) != 0) {
        float rhw;
        float z;

        primScreenDepth(&rhw, &z);
        do {
            ZrwIm2DVertex *base;

            if (gZrwPrim.im2dMax < 2) {
                gZrwPrim.im2dMax = 0x20;
                gZrwPrim.im2d = zmemResize(gZrwPrim.im2d, 0x40f);
            }
            base = align16(gZrwPrim.im2d);
            for (int i = 0; i < 2; i++) {
                primFill2D(&base[i], lines->vertices[i], xform, z, rhw);
            }
            primSetTexture(flags, lines->texture);
            RwIm2DRenderPrimitive(kPrimLineList, base, 2);
            lines++;
        } while (--count != 0);
        return;
    }
    {
        const int relative = (flags & 0x1000000) != 0;
        const ZMatrix *camera = relative ? primCameraMatrix() : NULL;

        do {
            ZrwIm3DVertex *base;

            if (gZrwPrim.im3dMax < 2) {
                gZrwPrim.im3dMax = 0x20;
                gZrwPrim.im3d = zmemResize(gZrwPrim.im3d, 0x48f);
            }
            base = align16(gZrwPrim.im3d);
            for (int i = 0; i < 2; i++) {
                primFill3D(&base[i], lines->vertices[i], xform, camera, relative);
            }
            if (RwIm3DTransform(base, 2, NULL, 1) != 0) {
                primSetTexture(flags, lines->texture);
                RwIm3DRenderPrimitive(kPrimLineList);
                RwIm3DEnd();
            }
            lines++;
        } while (--count != 0);
    }
}

BMX_ADDR(0x0048d120)
void zrwprimDrawTriangles(uint32_t flags, const ZrwPrimTriangle *triangles, int count, const ZMatrix *xform)
{
    do {
        ZrwPrimPoly poly = {triangles->texture, triangles->vertices, 3};

        zrwprimDrawPolys(flags, &poly, 1, xform);
        triangles++;
        count--;
    } while (count != 0);
}

/* Draws `count` polygons as triangle fans of `count` vertices each (which must not be 0). */
BMX_ADDR(0x0048d170)
void zrwprimDrawPolys(uint32_t flags, const ZrwPrimPoly *polys, int count, const ZMatrix *xform)
{
    primBeginCall();
    if ((flags & 0x2000000) != 0) {
        float rhw;
        float z;

        primScreenDepth(&rhw, &z);
        do {
            ZrwIm2DVertex *base;
            uint32_t n;

            if (polys->count > gZrwPrim.im2dMax) {
                gZrwPrim.im2dMax = (polys->count & ~0x1fu) + 0x20;
                gZrwPrim.im2d = zmemResize(gZrwPrim.im2d, gZrwPrim.im2dMax * 0x20 + 0xf);
            }
            base = align16(gZrwPrim.im2d);
            n = polys->count;
            for (uint32_t i = 0;; i++) {
                primFill2D(&base[i], polys->vertices[i], xform, z, rhw);
                if (--n == 0) {
                    break;
                }
            }
            primSetTexture(flags, polys->texture);
            RwIm2DRenderPrimitive(kPrimTriFan, base, (int)polys->count);
            polys++;
        } while (--count != 0);
        return;
    }
    {
        const int relative = (flags & 0x1000000) != 0;
        const ZMatrix *camera = relative ? primCameraMatrix() : NULL;

        do {
            ZrwIm3DVertex *base;
            uint32_t n;

            if (polys->count > gZrwPrim.im3dMax) {
                gZrwPrim.im3dMax = (polys->count & ~0x1fu) + 0x20;
                gZrwPrim.im3d = zmemResize(gZrwPrim.im3d, gZrwPrim.im3dMax * 0x24 + 0xf);
            }
            base = align16(gZrwPrim.im3d);
            n = polys->count;
            for (uint32_t i = 0;; i++) {
                primFill3D(&base[i], polys->vertices[i], xform, camera, relative);
                if (--n == 0) {
                    break;
                }
            }
            if (RwIm3DTransform(base, (int)polys->count, NULL, 1) != 0) {
                primSetTexture(flags, polys->texture);
                RwIm3DRenderPrimitive(kPrimTriFan);
                RwIm3DEnd();
            }
            polys++;
        } while (--count != 0);
    }
}

BMX_ADDR(0x0048d5a0)
void zrwprimDrawStrips(uint32_t flags, const ZrwPrimPoly *strips, int count, const ZMatrix *xform)
{
    do {
        ZrwPrimVertex *other[3];
        ZrwPrimVertex *const *cursor = strips->vertices;
        int32_t a = (int32_t)strips->count - 2;
        int32_t b = (int32_t)strips->count - 3;
        const ZrwPrimPoly second = {strips->texture, other, 3};

        while (a > 0) {
            const ZrwPrimPoly first = {strips->texture, cursor, 3};

            zrwprimDrawPolys(flags, &first, 1, xform);
            a = b - 1;
            if (b > 0) {
                other[0] = cursor[3];
                other[1] = cursor[2];
                other[2] = cursor[1];
                zrwprimDrawPolys(flags, &second, 1, xform);
            }
            cursor += 2;
            b -= 2;
        }
        strips++;
        count--;
    } while (count != 0);
}

BMX_ADDR(0x0048d660)
void zrwprimDrawQuads(uint32_t flags, const ZrwPrimQuad *quads, int count, const ZMatrix *xform)
{
    if ((flags & 0x2000000) != 0) {
        const uint8_t *camera = zrwGetClipCamera();

        if ((camera[8] & 1) != 0) {
            zrwprimClipPoly(flags, quads->texture, quads->vertices, 4, xform, (const float *)(camera + 0x94));
            return;
        }
    }
    do {
        ZrwPrimPoly poly = {quads->texture, quads->vertices, 4};

        zrwprimDrawPolys(flags, &poly, 1, xform);
        quads++;
        count--;
    } while (count != 0);
}
