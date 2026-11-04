/* Model rendering and collision debugging (C:\NgEngine\Zge\Com\ZglRw\zrwModelRender.c). */
#include "zge/com/zglrw/zrwmodelrender.h"

#include <stddef.h>

#include "compat/crt.h"
#include "rw/rwcamera.h"
#include "rw/rwclump.h"
#include "rw/rwcore.h"
#include "rw/rwim.h"
#include "rw/rwmath.h"
#include "rw/rwplugins.h"
#include "rw/rwtexture.h"
#include "rw/rwworld.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zgemods/ztaginf.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kZrwModelRenderFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwModelRender.c";

/*
 * The tree ztaginf.c builds (ZtaginfNode, from its free list, root at 0x6728c4) is the one walked here
 * (ZrwRenderColNode): the two views must agree wherever both read, on every build. Nothing here reads past
 * `triangles` (the free list's nodes end there).
 */
_Static_assert(offsetof(ZrwRenderColNode, type) == offsetof(ZtaginfNode, kind) &&
                   offsetof(ZrwRenderColNode, triangleCount) == offsetof(ZtaginfNode, faceCount) &&
                   offsetof(ZrwRenderColNode, below) == offsetof(ZtaginfNode, child[0]) &&
                   offsetof(ZrwRenderColNode, above) == offsetof(ZtaginfNode, child[1]) &&
                   offsetof(ZrwRenderColNode, triangles) == offsetof(ZtaginfNode, faces) &&
                   offsetof(ZrwRenderColNode, triangles) + sizeof(void *) == sizeof(ZtaginfNode),
               "ZrwRenderColNode and ZtaginfNode describe the same tree nodes");

/* RenderWare library calls (statically linked in the exe, not ported). */
typedef void *(BMX_CDECL *RwFrameCallback)(void *frame, void *data);
typedef void *(BMX_CDECL *RpAtomicCallback)(void *atomic, void *data);

/* Hooks into the renderer (function pointers in the exe's data). */
typedef void(BMX_CDECL *ZrwRenderStateHookFn)(uint32_t flags, const void *raster);
typedef void(BMX_CDECL *ZrwRenderHookFn)(void);
typedef int(BMX_CDECL *ZrwFrameUpdateFn)(void *arg);
typedef void *(BMX_CDECL *ZrwModelRegisterFn)(int type, ZrwFrameUpdateFn fn, void *arg, int append);
typedef void(BMX_CDECL *ZrwModelUnregisterFn)(void *node);
/* The debug drawing hooks of zrwdbg.c (0x560064..). */
typedef void(BMX_CDECL *ZrwDbgLineFn)(const ZVec3 *a, const ZVec3 *b, const uint32_t *color, int flags);
typedef void(BMX_CDECL *ZrwDbgTriangleFn)(const ZVec3 *a, const ZVec3 *b, const ZVec3 *c,
                                          const uint32_t *color, int flags);
typedef void(BMX_CDECL *ZrwDbgSphereFn)(const ZVec3 *center, float radius, const uint32_t *color, int flags);
#define gZrwDbgLineHook     ORIG_VAR(0x00560064, ZrwDbgLineFn, gZrwDbgLineHook)
#define gZrwDbgTriangleHook ORIG_VAR(0x00560068, ZrwDbgTriangleFn, gZrwDbgTriangleHook)
#define gZrwDbgSphereHook   ORIG_VAR(0x0056006c, ZrwDbgSphereFn, gZrwDbgSphereHook)
/* zrwzgl.h's render state (flags and the current raster); that header clashes with zrwmodel.h. */
#define gZrwRenderFlags     ORIG_VAR(0x00697778, uint32_t, gZrwRenderFlags)
#define gZrwRenderExtra     ORIG_VAR(0x0069777c, const void *, gZrwRenderExtra)
#define gZrwRenderStateHook ORIG_VAR(0x00561490, ZrwRenderStateHookFn, gZrwRenderStateHook)
#define gZrwBeginHook       ORIG_VAR(0x00561494, ZrwRenderHookFn, gZrwBeginHook)
#define gZrwEndHook         ORIG_VAR(0x00561498, ZrwRenderHookFn, gZrwEndHook)
#define gZrwModelRegister   ORIG_VAR(0x00561470, ZrwModelRegisterFn, gZrwModelRegister)
#define gZrwModelUnregister ORIG_VAR(0x00561474, ZrwModelUnregisterFn, gZrwModelUnregister)
#define gZrwCamera          ORIG_VAR(0x006976f4, void *, gZrwCamera)
/* Colour handed to zrwModelSetColor after drawing a model (0x561818, 0x56181c). */
#define gZrwModelColor1 ORIG_VAR(0x00561818, uint32_t, gZrwModelColor1)
#define gZrwModelColor2 ORIG_VAR(0x0056181c, uint32_t, gZrwModelColor2)
/* The hit list the tree query fills (0x672908, 0x40 records, followed by gZrwColHit). */
typedef ZrwRenderColHit ZrwRenderColList[0x40];
#define gZrwColList ORIG_VAR(0x00672908, ZrwRenderColList, gZrwColList)

/* Vertex of RenderWare's immediate mode (0x24 bytes). */
typedef struct ZrwImVertex {
    float x, y, z;
    uint8_t pad0c[0xc];
    uint8_t color[4]; /* 0x18 */
    uint32_t u;       /* 0x1c */
    float v;          /* 0x20 */
} ZrwImVertex;

_Static_assert(offsetof(ZrwImVertex, color) == 0x18, "immediate vertex");
_Static_assert(sizeof(ZrwImVertex) == 0x24, "immediate vertex");

/* Corner `index` of a collision triangle (v0, v1 and v2 are 16 bytes apart). The index is not range checked:
 * callers pass what the original passed. */
static const ZVec3 *colCorner(const ZrwRenderColTriangle *triangle, uint32_t index)
{
    return (const ZVec3 *)((const uint8_t *)&triangle->v0 + index * 0x10);
}

/* Draws the 12 edges of a node's bounding box. */
BMX_ADDR(0x0048def0)
void zrwModelDrawNodeBox(const ZrwRenderColNode *node, const uint32_t *color)
{
    ZVec3 corner[8];
    const ZVec3 *const lo = &node->boxMin;
    const ZVec3 *const hi = &node->boxMax;
    static const uint8_t edges[12][2] = {{0, 1}, {1, 3}, {2, 3}, {2, 0}, {4, 5}, {5, 7},
                                         {6, 7}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

    for (int i = 0; i < 8; i++) {
        corner[i].x = (i & 4) != 0 ? lo->x : hi->x;
        corner[i].y = (i & 2) != 0 ? lo->y : hi->y;
        corner[i].z = (i & 1) != 0 ? lo->z : hi->z;
    }
    if (color != NULL) {
        for (int i = 0; i < 12; i++) {
            gZrwDbgLineHook(&corner[edges[i][0]], &corner[edges[i][1]], color, 0);
        }
    }
}

/*
 * Draws a collision triangle moved 10 units along its normal: filled (and again reversed when it has a tag),
 * its marked edges, and its marked corners (moved back by the unscaled normal) as small spheres.
 */
BMX_ADDR(0x0048e0c0)
void zrwModelDrawTriangle(const ZrwRenderColTriangle *triangle, const uint32_t *pointColor,
                          const uint32_t *edgeColor, const uint32_t *fillColor)
{
    ZVec3 a, b, c;
    const double scaledX = (double)triangle->normal.x * 10.0f;
    const double scaledY = (double)triangle->normal.y * 10.0f;
    const double scaledZ = (double)triangle->normal.z * 10.0f;
    const float roundedY = (float)scaledY;
    const float roundedZ = (float)scaledZ;

    a.x = (float)(scaledX + triangle->v0.x);
    a.y = (float)(scaledY + triangle->v0.y);
    a.z = (float)(scaledZ + triangle->v0.z);
    b.x = (float)(scaledX + triangle->v1.x);
    b.y = (float)((double)roundedY + triangle->v1.y);
    b.z = (float)((double)roundedZ + triangle->v1.z);
    c.x = (float)(scaledX + triangle->v2.x);
    c.y = (float)((double)roundedY + triangle->v2.y);
    c.z = (float)((double)roundedZ + triangle->v2.z);
    if (fillColor != NULL) {
        if (triangle->tag != NULL) {
            gZrwDbgTriangleHook(&c, &b, &a, fillColor, 2);
        }
        gZrwDbgTriangleHook(&a, &b, &c, fillColor, 2);
    }
    if (edgeColor != NULL) {
        if (triangle->edgeTag[0] != NULL) {
            gZrwDbgLineHook(&a, &b, edgeColor, 0);
        }
        if (triangle->edgeTag[1] != NULL) {
            gZrwDbgLineHook(&b, &c, edgeColor, 0);
        }
        if (triangle->edgeTag[2] != NULL) {
            gZrwDbgLineHook(&c, &a, edgeColor, 0);
        }
    }
    if (pointColor != NULL) {
        const ZVec3 *const corners[3] = {&triangle->v0, &triangle->v1, &triangle->v2};

        for (int i = 0; i < 3; i++) {
            if (triangle->cornerTag[i] != NULL) {
                ZVec3 point;

                point.x = (float)((double)corners[i]->x - triangle->normal.x);
                point.y = (float)((double)corners[i]->y - triangle->normal.y);
                point.z = (float)((double)corners[i]->z - triangle->normal.z);
                gZrwDbgSphereHook(&point, 0.5f, pointColor, 0);
            }
        }
    }
}

/* Draws one edge of a collision triangle (moved by its normal) when it is marked. */
BMX_ADDR(0x0048e2b0)
void zrwModelDrawTriangleEdge(const ZrwRenderColTriangle *triangle, uint8_t edge, const uint32_t *color)
{
    const uint32_t first = edge;
    const uint32_t second = (first + 1) % 3;
    const ZVec3 *const from = colCorner(triangle, first);
    const ZVec3 *const to = colCorner(triangle, second);
    ZVec3 a, b;

    a.x = (float)((double)triangle->normal.x + from->x);
    a.y = (float)((double)from->y + triangle->normal.y);
    a.z = (float)((double)from->z + triangle->normal.z);
    b.x = (float)((double)triangle->normal.x + to->x);
    b.y = (float)((double)to->y + triangle->normal.y);
    b.z = (float)((double)to->z + triangle->normal.z);
    if (color != NULL && triangle->edgeTag[first] != NULL) {
        gZrwDbgLineHook(&a, &b, color, 2);
    }
}

/*
 * Walks the tree down to the leaf of `point`. Comparisons follow the x87 flags: an unordered compare counts
 * as "below" (types 2 and 8) or "at most" (type 4). An unknown node type never leaves the loop.
 */
BMX_ADDR(0x0048e400)
ZrwRenderColNode *zrwModelFindNode(const ZVec3 *point)
{
    ZrwRenderColNode *node = &gZrwColRoot;

    for (;;) {
        if (node->type == 1) {
            const uint32_t red = 0xff0000ff;

            if (gZrwColDebug != 0) {
                zrwModelDrawNodeBox(node, &red);
            }
            return node;
        }
        if (node->type == 2) {
            node = !(point->x >= node->split) ? node->below : node->above;
        } else if (node->type == 4) {
            node = !(point->y > node->split) ? node->above : node->below;
        } else if (node->type == 8) {
            node = !(point->z >= node->split) ? node->below : node->above;
        }
        if (node == NULL) {
            return NULL;
        }
    }
}

/* Whether the cross products of the corner pairs and the point all point along the normal (within 0.01). */
static int nearNormal(const ZVec3 *a, const ZVec3 *normal)
{
    const double epsilon = 0x1.47ae14p-7f;

    return !(__builtin_fabs((double)a->x - normal->x) >= epsilon) &&
           !(__builtin_fabs((double)a->y - normal->y) >= epsilon) &&
           !(__builtin_fabs((double)a->z - normal->z) >= epsilon);
}

BMX_ADDR(0x0048e4b0)
int zrwModelPointInTriangle(const ZVec3 *v0, const ZVec3 *v1, const ZVec3 *v2, const ZVec3 *normal,
                            const ZVec3 *point)
{
    ZVec3 n;

    zvec3Cross3(&n, v0, v1, point);
    zvec3NormalizeScaled(&n, &n);
    if (!nearNormal(&n, normal)) {
        return 0;
    }
    zvec3Cross3(&n, v1, v2, point);
    zvec3NormalizeScaled(&n, &n);
    if (!nearNormal(&n, normal)) {
        return 0;
    }
    zvec3Cross3(&n, v2, v0, point);
    zvec3NormalizeScaled(&n, &n);
    return nearNormal(&n, normal);
}

/*
 * Finds the triangle of the current leaf whose plane is closest to `point` (distance along the normal below
 * maxDistance) and that contains the projection of the point; the answer lives in gZrwColHit.
 */
BMX_ADDR(0x0048e840)
ZrwRenderColHit *zrwModelFindTriangle(const ZVec3 *point, float maxDistance)
{
    ZrwRenderColTriangle *best = NULL;
    float bestDistance = 0x1.387f34p+13f; /* 9999.9 */
    ZrwRenderColNode *node;

    gZrwColHit.valid = 0;
    gZrwColHit.tag = NULL;
    node = zrwModelFindNode(point);
    if (node == NULL) {
        return NULL;
    }
    if (gZrwColDebug != 0) {
        const uint32_t color = 0x200000ff;

        for (uint32_t i = 0; i < node->triangleCount; i++) {
            zrwModelDrawTriangle(node->triangles[i], NULL, NULL, &color);
        }
    }
    for (uint32_t i = 0; i < node->triangleCount; i++) {
        ZrwRenderColTriangle *const triangle = node->triangles[i];

        if (triangle->tag != NULL) {
            ZVec3 offset, scaled, projected;
            double dot;
            float distance;

            zvec3Sub(&offset, point, &triangle->v0);
            dot = __builtin_fabs(zvec3Dot(&offset, &triangle->normal));
            distance = (float)dot;
            if (!__builtin_isfinite(dot)) {
                gZdebugPrint("FUCKED TAG INFO NORMAL\n");
                distance = 99999.0f;
            }
            if (!(distance >= bestDistance)) {
                zvec3Scale(&scaled, &triangle->normal, distance);
                zvec3Sub(&projected, point, &scaled);
                if (zrwModelPointInTriangle(&triangle->v0, &triangle->v1, &triangle->v2, &triangle->normal,
                                            &projected) != 0) {
                    bestDistance = distance;
                    best = node->triangles[i];
                }
            }
        }
    }
    if (!(bestDistance > maxDistance)) {
        if (best != NULL && gZrwColDebug != 0) {
            const uint32_t color = 0x20ff0000;

            zrwModelDrawTriangle(best, NULL, NULL, &color);
        }
        gZrwColHit.valid = 1;
        gZrwColHit.tag = best->tag;
        gZrwColHit.v0 = best->v0;
        gZrwColHit.v1 = best->v1;
        gZrwColHit.v2 = best->v2;
        return &gZrwColHit;
    }
    return NULL;
}

/* Appends a hit of `kind` with the corners it has (NULL: left as is) to the list; the caller checks room. */
static void colListAdd(uint16_t *found, uint8_t kind, const ZVec3 *v0, const ZVec3 *v1, const ZVec3 *v2,
                       const void *tag)
{
    gZrwColList[*found].valid = kind;
    gZrwColList[*found].v0 = *v0;
    if (v1 != NULL) {
        gZrwColList[*found].v1 = *v1;
    }
    if (v2 != NULL) {
        gZrwColList[*found].v2 = *v2;
    }
    gZrwColList[*found].tag = tag;
    (*found)++;
}

static float bitsToFloat(uint32_t bits)
{
    float f;

    __builtin_memcpy(&f, &bits, 4);
    return f;
}

/*
 * Collects the collision features within `radius` of `point` into the hit list (at most 0x40, counted in
 * *found): mode 1 whole triangles (kind 1), mode 2 marked edges (kind 2, drawn green while gZrwColDebug is
 * set), mode 3 marked corners (kind 3). Split nodes closer to the point than the radius are searched on both
 * sides (the lower side recursively). Original quirks: the split coordinate lives in the `found` argument's
 * stack slot, so until the first split of type 2, 4 or 8 it is the pointer's bits; mode 1 measures three
 * times the corner at byte offset (those bits << 4) of the triangle instead of each corner.
 */
BMX_ADDR(0x0048ee70)
void zrwColSearch(int mode, const ZrwRenderColNode *node, uint16_t *found, const ZVec3 *point, float radius)
{
    uint32_t key = (uint32_t)(uintptr_t)found;

    while (node->type != 1) {
        double distance;

        switch (node->type) {
        case 2:
            __builtin_memcpy(&key, &point->x, 4);
            break;
        case 4:
            __builtin_memcpy(&key, &point->y, 4);
            break;
        case 8:
            __builtin_memcpy(&key, &point->z, 4);
            break;
        default:
            break;
        }
        distance = __builtin_fabs((double)bitsToFloat(key) - (double)node->split);
        if (!(distance >= (double)radius)) {
            zrwColSearch(mode, node->below, found, point, radius);
            node = node->above;
        } else if (!(bitsToFloat(key) >= node->split)) {
            node = node->below;
        } else {
            node = node->above;
        }
    }
    for (int i = 0; i < (int)node->triangleCount; i++) {
        const ZrwRenderColTriangle *tri = node->triangles[i];
        ZVec3 d;

        if (mode == 1) {
            if (tri->tag != NULL) {
                const ZVec3 *corner = (const ZVec3 *)((uintptr_t)tri + (uint32_t)(key << 4));
                float best = 9999.0f;

                for (int k = 0; k < 3; k++) {
                    double length;

                    zvec3Sub(&d, point, corner);
                    length = __builtin_fabs(zvec3Length(&d));
                    if (!(length >= (double)best)) {
                        best = (float)length;
                    }
                }
                if (!(best >= radius) && *found < 0x40) {
                    colListAdd(found, 1, &tri->v0, &tri->v1, &tri->v2, tri->tag);
                }
            }
        } else if (mode == 2) {
            for (int e = 0; e < 3; e++) {
                const ZVec3 *from = colCorner(tri, (uint32_t)e);
                const ZVec3 *to = colCorner(tri, (uint32_t)((e + 1) % 3));
                ZVec3 edge;
                float along;

                if (tri->edgeTag[e] == NULL) {
                    continue;
                }
                zvec3Sub(&d, point, from);
                zvec3Sub(&edge, to, from);
                zvec3NormalizeScaled(&edge, &edge);
                along = (float)zvec3Dot(&d, &edge);
                if (!(__builtin_sqrt(zvec3LengthSq(&d) - (double)along * along) >= (double)radius) &&
                    *found < 0x40) {
                    if (tri != NULL && gZrwColDebug != 0) {
                        const uint8_t green[4] = {0, 0xff, 0, 0xff};
                        uint32_t color;

                        __builtin_memcpy(&color, green, 4);
                        zrwModelDrawTriangleEdge(tri, (uint8_t)e, &color);
                    }
                    colListAdd(found, 2, from, to, NULL, tri->edgeTag[e]);
                }
            }
        } else if (mode == 3) {
            for (int k = 0; k < 3; k++) {
                const ZVec3 *corner = colCorner(tri, (uint32_t)k);

                if (tri->cornerTag[k] == NULL) {
                    continue;
                }
                zvec3Sub(&d, point, corner);
                if (!(__builtin_fabs(zvec3Length(&d)) >= (double)radius) && *found < 0x40) {
                    colListAdd(found, 3, corner, NULL, NULL, tri->cornerTag[k]);
                }
            }
        }
    }
}

BMX_ADDR(0x0048f320)
ZrwRenderColQuery *zrwModelQuery(const ZVec3 *point, float radius)
{
    gZrwColQuery.found = 0;
    gZrwColQuery.list = &gZrwColList;
    zrwColSearch(2, &gZrwColRoot, &gZrwColQuery.found, point, radius);
    return gZrwColQuery.found != 0 ? &gZrwColQuery : NULL;
}

/*
 * Atomic render callback that draws a model facing the camera: the atomic's world matrix is reset to a
 * rotation about y that undoes the camera's yaw, the atomic's original render callback draws it, and the
 * model colour is switched around that.
 */
BMX_ADDR(0x0048f3e0)
void *zrwModelRenderBillboard(void *object)
{
    ZrwModel *const model = zrwMdlGetUserData(object);
    const ZMatrix *cameraMatrix = RwFrameGetLTM(RwCameraGetFrame(gZrwCamera));
    ZMatrix *const frame = RwFrameGetLTM(RpAtomicGetFrame(object));
    uint32_t renderFlags;
    ZVec3 axis;
    ZVec3 position;
    ZAngle yaw;
    float angle;

    zrwModelSetColor(model, &model->color);
    gZrwBeginHook();
    renderFlags = (gZrwRenderFlags & 0xffff22ffu) | 0x2222;
    if (gZrwRenderFlags != renderFlags || gZrwRenderExtra != NULL) {
        gZrwRenderStateHook(renderFlags, NULL);
    }
    axis.x = 0;
    axis.y = -1.0f;
    axis.z = 0;
    position = frame->pos;
    yaw = zmathAtan2(cameraMatrix->at.x, cameraMatrix->at.z);
    angle = (float)((double)(int16_t)(-(int)yaw) * (double)0x1.68p-8f);
    frame->at.z = 1.0f;
    frame->up.y = 1.0f;
    frame->right.x = 1.0f;
    frame->up.x = 0;
    frame->right.z = 0;
    frame->right.y = 0;
    frame->at.y = 0;
    frame->at.x = 0;
    frame->up.z = 0;
    frame->pos.z = 0;
    frame->pos.y = 0;
    frame->pos.x = 0;
    frame->flags |= 0x20003;
    RwMatrixRotate(frame, &axis, angle, 0);
    frame->pos = position;
    model->def->atomicRender(object);
    gZrwEndHook();
    zrwModelSetColor(model, &gZrwModelColor1);
    return object;
}

/* The render callback zrwModelSetDrawColor installs: the atomic's original one, drawn in the model colour. */
BMX_ADDR(0x0048f530)
void *zrwModelRenderCallback(void *object)
{
    ZrwModel *const model = zrwMdlGetUserData(object);

    zrwModelSetColor(model, &model->color);
    model->def->atomicRender(object);
    zrwModelSetColor(model, &gZrwModelColor2);
    return object;
}

/* Sends the buffered triangles of one material to the renderer, switching the texture when needed. */
static void flushTriangles(RpGeometry *geometry, int material, int count)
{
    RpMaterial *const rpMaterial = RpGeometryGetMaterial(geometry, material);
    void *raster = RwTextureGetRaster(RpMaterialGetTexture(rpMaterial));

    /* the original fetches the raster a second time */
    if (raster != gZrwRenderExtra) {
        raster = RwTextureGetRaster(RpMaterialGetTexture(rpMaterial));
        gZrwRenderStateHook(gZrwRenderFlags, raster);
    }
    RwIm3DRenderIndexedPrimitive(3, gZrwModelRender.indices, count * 3);
}

/*
 * Draws an atomic's geometry unlit, on top of everything, moved 2 units down from the camera's position: the
 * vertices go through the immediate mode transform and the triangles are sent per material. The far clip
 * plane is pulled in to 300 meanwhile (and, when the atomic has no geometry, left there).
 */
BMX_ADDR(0x0048f720)
void *zrwModelRenderAtomic(void *atomic)
{
    ZrwImVertex local[0x100];
    ZMatrix *const frame = RwFrameGetLTM(RpAtomicGetFrame(atomic));
    RpGeometry *const geometry = ((const RpAtomic *)atomic)->geometry;
    const int vertexCount = RpGeometryGetNumVertices(geometry);
    const float *vertices = (const float *)RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geometry, 0));
    const int16_t *triangle = (const int16_t *)RpGeometryGetTriangles(geometry);
    const uint32_t *texCoords = (const uint32_t *)RpGeometryGetTexCoords(geometry);
    const int triangleCount = RpGeometryGetNumTriangles(geometry);
    void *const raster = RwCameraGetRaster(gZrwCamera);
    float farPlane;

    RwRasterGetWidth(raster);
    RwRasterGetHeight(raster);
    farPlane = RwCameraGetFarClipPlane(gZrwCamera);
    RwCameraSetFarClipPlane(gZrwCamera, 300.0f);
    if (geometry == NULL) {
        return atomic;
    }
    if (vertexCount > gZrwModelRender.vertexCapacity) {
        gZrwModelRender.vertices = zmemResize(gZrwModelRender.vertices, (uint32_t)vertexCount * 0x20 + 0x10);
        gZrwModelRender.buffer2 = zmemResize(gZrwModelRender.buffer2, (uint32_t)vertexCount * 12);
        gZrwModelRender.buffer3 = zmemResize(gZrwModelRender.buffer3, (uint32_t)vertexCount);
        gZrwModelRender.vertexCapacity = vertexCount;
    }
    if (triangleCount > gZrwModelRender.indexCapacity) {
        gZrwModelRender.indices = zmemResize(gZrwModelRender.indices, (uint32_t)triangleCount * 6);
        gZrwModelRender.indexCapacity = triangleCount;
    }
    RwMatrixTranslate(frame, &RwFrameGetLTM(RwCameraGetFrame(gZrwCamera))->pos, 0);
    frame->pos.y = (float)((double)frame->pos.y - 2.0f);
    for (int i = 0; i < vertexCount; i++) {
        ZrwImVertex *const out = &local[i];

        out->x = vertices[i * 3];
        out->y = vertices[i * 3 + 1];
        out->z = vertices[i * 3 + 2];
        out->color[0] = 0xff;
        out->color[1] = 0xff;
        out->color[2] = 0xff;
        out->color[3] = 0xff;
        out->u = texCoords[i * 2];
        out->v = ((const float *)texCoords)[i * 2 + 1];
    }
    {
        const uint32_t renderFlags = (gZrwRenderFlags & 0xffff12f5u) | 0x1220;

        if (gZrwRenderFlags != renderFlags) {
            gZrwRenderStateHook(renderFlags, gZrwRenderExtra);
        }
    }
    RwRenderStateSet(4, 0);
    RwRenderStateSet(6, 0);
    if (RwIm3DTransform(local, vertexCount, frame, 1) != 0) {
        uint16_t *cursor = gZrwModelRender.indices;
        int count = 0;
        int material = -1;

        RwCameraGetNearClipPlane(gZrwCamera);
        for (int left = triangleCount; left > 0; left--, triangle += 4) {
            if (triangle[3] != material) {
                if (count != 0) {
                    flushTriangles(geometry, material, count);
                    cursor = gZrwModelRender.indices;
                    count = 0;
                }
                material = triangle[3];
            }
            cursor[0] = (uint16_t)triangle[0];
            cursor[1] = (uint16_t)triangle[1];
            cursor[2] = (uint16_t)triangle[2];
            cursor += 3;
            count++;
        }
        if (count != 0) {
            flushTriangles(geometry, material, count);
        }
    }
    RwIm3DEnd();
    RwCameraSetFarClipPlane(gZrwCamera, farPlane);
    RwRenderStateSet(4, 1);
    RwRenderStateSet(6, 1);
    return atomic;
}

/* Draws a red line from the parent's position to this frame's, and on to its children. */
BMX_ADDR(0x0048fa90)
void *zrwFrameDrawLines(void *frame, const ZVec3 *parentPosition)
{
    const ZMatrix *const ltm = RwFrameGetLTM(frame);
    const ZVec3 position = ltm->pos;
    ZrwImVertex line[2];

    memset(line, 0, sizeof line);
    line[0].x = parentPosition->x;
    line[0].y = parentPosition->y;
    line[0].z = parentPosition->z;
    line[1].x = position.x;
    line[1].y = position.y;
    line[1].z = position.z;
    for (int i = 0; i < 2; i++) {
        line[i].color[0] = 0xff;
        line[i].color[1] = 0;
        line[i].color[2] = 0;
        line[i].color[3] = 0xff;
    }
    if (RwIm3DTransform(line, 2, NULL, 0) != 0) {
        RwIm3DRenderLine(0, 1);
        RwIm3DEnd();
    }
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0048fa90, zrwFrameDrawLines),
                          (void *)&position);
    return frame;
}

BMX_ADDR(0x0048fb70)
void zrwModelDrawFrames(const ZrwModel *model)
{
    if (model->clump != NULL) {
        RwFrame *const frame = RpClumpGetFrame(model->clump);

        if (frame != NULL) {
            const ZVec3 position = RwFrameGetLTM(frame)->pos;

            RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0048fa90, zrwFrameDrawLines),
                                  (void *)&position);
        }
    }
}

/*
 * Per-frame callback of kZrwModelFollowCamera models: puts the clump at the camera's position and draws its
 * atomics. 0 for models without option bit 0.
 */
BMX_ADDR(0x0048fbc0)
int zrwModelRender(ZrwModel *model)
{
    RpClump *const clump = model->clump;

    if ((model->def->options & 1) == 0) {
        zdebugFail("Not coded", kZrwModelRenderFile, 0x312, 0);
        return 0;
    }
    {
        const ZMatrix *const camera = RwFrameGetLTM(RwCameraGetFrame(gZrwCamera));

        RwMatrixTranslate(RwFrameGetMatrix(RpClumpGetFrame(clump)), &camera->pos, 0);
    }
    RwFrameUpdateObjects(RpClumpGetFrame(clump));
    RpClumpForAllAtomics(model->clump, BMX_FNPTR(RpAtomicCallback, 0x0048f720, zrwModelRenderAtomic), NULL);
    return 1;
}

BMX_ADDR(0x0048fc50)
int zrwModelRenderInitialize(void)
{
    if (gZrwModelRender.initialized != 0) {
        zdebugFail("Already initialized", kZrwModelRenderFile, 0x32f, 0);
    }
    zmemFill(&gZrwModelRender, 0, sizeof gZrwModelRender);
    gZrwModelRender.initialized = 1;
    gZrwModelRender.vertexCapacity = 0x200;
    gZrwModelRender.vertices = zmemAlloc((uint32_t)gZrwModelRender.vertexCapacity * 0x20 + 0x10);
    gZrwModelRender.buffer2 = zmemAlloc((uint32_t)gZrwModelRender.vertexCapacity * 12);
    gZrwModelRender.buffer3 = zmemAlloc((uint32_t)gZrwModelRender.vertexCapacity);
    gZrwModelRender.indexCapacity = 0x100;
    gZrwModelRender.indices = zmemAlloc((uint32_t)gZrwModelRender.indexCapacity * 6);
    gZrwModelRender.pad84 = 0;
    gZrwModelRender.atomicCallback = NULL;
    return 1;
}

BMX_ADDR(0x0048fd00)
int zrwModelRenderShutdown(void)
{
    if (gZrwModelRender.initialized != 0) {
        if (gZrwModelRender.updateNode != NULL) {
            gZrwModelUnregister(gZrwModelRender.updateNode);
            gZrwModelRender.updateNode = NULL;
        }
        if (gZrwModelRender.vertices != NULL) {
            zmemFree(gZrwModelRender.vertices);
            gZrwModelRender.vertices = NULL;
        }
        if (gZrwModelRender.buffer2 != NULL) {
            zmemFree(gZrwModelRender.buffer2);
            gZrwModelRender.buffer2 = NULL;
        }
        if (gZrwModelRender.buffer3 != NULL) {
            zmemFree(gZrwModelRender.buffer3);
            gZrwModelRender.buffer3 = NULL;
        }
        if (gZrwModelRender.indices != NULL) {
            zmemFree(gZrwModelRender.indices);
            gZrwModelRender.indices = NULL;
        }
        gZrwModelRender.initialized = 0;
    }
    return 1;
}

/* Draws the clump right away at the camera's position (as zrwModelRender), updating a skinned model first. */
BMX_ADDR(0x0048fda0)
void zrwModelRenderDirect(ZrwModel *model)
{
    RpClump *const clump = model->clump;

    if ((model->def->options & 1) == 0) {
        zdebugFail("Not coded", kZrwModelRenderFile, 900, 0);
        return;
    }
    {
        const ZMatrix *const camera = RwFrameGetLTM(RwCameraGetFrame(gZrwCamera));

        RwMatrixTranslate(RwFrameGetMatrix(RpClumpGetFrame(clump)), &camera->pos, 0);
    }
    RwFrameUpdateObjects(RpClumpGetFrame(clump));
    if ((model->def->options & kZrwModelSkinned) != 0) {
        RpSkinClumpUpdate(clump);
    }
    RpClumpRender(clump);
}

/* RpClumpForAllAtomics callback: installs the render callback and bumps the atomic's stamp. */
BMX_ADDR(0x0048fea0)
void *zrwAtomicSetRenderCallback(void *atomic, void *callback)
{
    RpAtomic *const rpAtomic = atomic;

    rpAtomic->renderCallBack = (RpAtomicRenderCallBack)(uintptr_t)callback;
    rpAtomic->stamp++;
    return atomic;
}

BMX_ADDR(0x0048fec0)
void zrwModelSetRenderCallback(void *model, ZrwObjectCallback callback)
{
    if (callback != NULL) {
        RpClumpForAllAtomics(((ZrwModel *)model)->clump,
                             BMX_FNPTR(RpAtomicCallback, 0x0048fea0, zrwAtomicSetRenderCallback),
                             (void *)(uintptr_t)callback);
    }
}

BMX_ADDR(0x0048fee0)
void zrwModelSetDefaultRenderCallback(ZrwModel *model)
{
    if (gZrwModelRender.atomicCallback != NULL) {
        RpClumpForAllAtomics(model->clump,
                             BMX_FNPTR(RpAtomicCallback, 0x0048fea0, zrwAtomicSetRenderCallback),
                             gZrwModelRender.atomicCallback);
    }
}

BMX_ADDR(0x0048ff00)
void zrwModelRenderRegister(void *arg)
{
    if (gZrwModelRender.updateNode == NULL) {
        gZrwModelRender.updateNode =
            gZrwModelRegister(1, BMX_FNPTR(ZrwFrameUpdateFn, 0x0048fbc0, zrwModelRender), arg, 0);
    }
}

BMX_ADDR(0x0048ff30)
void zrwModelRenderUnregister(void)
{
    if (gZrwModelRender.updateNode != NULL) {
        gZrwModelUnregister(gZrwModelRender.updateNode);
        gZrwModelRender.updateNode = NULL;
    }
}
