/* Model rendering and collision debugging (C:\NgEngine\Zge\Com\ZglRw\zrwModelRender.c). */
#include "zge/com/zglrw/zrwmodelrender.h"

#include <stddef.h>

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kZrwModelRenderFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwModelRender.c";

/* RenderWare library calls (statically linked in the exe, not ported). */
typedef void *(BMX_CDECL *RwFrameCallback)(void *frame, void *data);
typedef void *(BMX_CDECL *RpAtomicCallback)(void *atomic, void *data);
ORIG_FN(0x004d2b10, ZMatrix *, RwFrameGetLTM, (void *frame));
ORIG_FN(0x004d2b00, ZMatrix *, RwFrameGetMatrix, (void *frame));
ORIG_FN(0x004d2a80, void *, RwFrameUpdateObjects, (void *frame));
ORIG_FN(0x004d2eb0, void *, RwFrameForAllChildren, (void *frame, RwFrameCallback callback, void *data));
ORIG_FN(0x004dea80, void *, RwCameraGetFrame, (void *camera));
ORIG_FN(0x004de5a0, void *, RwCameraGetRaster, (void *camera));
ORIG_FN(0x004de650, float, RwCameraGetFarClipPlane, (void *camera));
ORIG_FN(0x004de620, void *, RwCameraSetFarClipPlane, (void *camera, float distance));
ORIG_FN(0x004de610, float, RwCameraGetNearClipPlane, (void *camera));
ORIG_FN(0x004dcc30, int, RwRasterGetWidth, (void *raster));
ORIG_FN(0x004dcc40, int, RwRasterGetHeight, (void *raster));
ORIG_FN(0x004d15a0, void *, RwMatrixTranslate, (ZMatrix * matrix, const ZVec3 *translation, int combine));
ORIG_FN(0x004d1040, void *, RwMatrixRotate, (ZMatrix * matrix, const ZVec3 *axis, float angle, int combine));
ORIG_FN(0x004b1990, void *, RpClumpForAllAtomics, (void *clump, RpAtomicCallback callback, void *data));
ORIG_FN(0x004b1940, void *, RpClumpRender, (void *clump));
ORIG_FN(0x004b4550, int, RpGeometryGetNumVertices, (void *geometry));
ORIG_FN(0x004b43e0, void *, RpGeometryGetMorphTarget, (void *geometry, int index));
ORIG_FN(0x004b45d0, const float *, RpMorphTargetGetVertices, (void *morphTarget));
ORIG_FN(0x004b46b0, const int16_t *, RpGeometryGetTriangles, (void *geometry));
ORIG_FN(0x004b44e0, const uint32_t *, RpGeometryGetTexCoords, (void *geometry));
ORIG_FN(0x004b4720, int, RpGeometryGetNumTriangles, (void *geometry));
ORIG_FN(0x004b47a0, void *, RpGeometryGetMaterial, (void *geometry, int index));
ORIG_FN(0x004b93d0, void *, RpMaterialGetTexture, (void *material));
ORIG_FN(0x004df650, void *, RwTextureGetRaster, (void *texture));
ORIG_FN(0x004db820, int, RwRenderStateSet, (int state, int value));
ORIG_FN(0x004df0b0, int, RwIm3DTransform, (void *vertices, int count, const ZMatrix *matrix, int flags));
ORIG_FN(0x004df090, int, RwIm3DEnd, (void));
ORIG_FN(0x004df020, int, RwIm3DRenderIndexedPrimitive, (int primitive, const uint16_t *indices, int count));
ORIG_FN(0x004defc0, int, RwIm3DRenderLine, (int from, int to));
ORIG_FN(0x00504490, void, zrwModelPostUpdate, (void *clump));
/* The collision search over the triangle tree (large, not ported). */
ORIG_FN(0x0048ee70, void, zrwColSearch,
        (int mode, ZrwRenderColNode *root, ZrwRenderColQuery *result, const void *a, const void *b));

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
#define gZrwRenderFlags     ORIG_VAR(0x00697778, uint32_t, gZrwRenderFlags)
#define gZrwRenderExtra     ORIG_VAR(0x0069777c, int, gZrwRenderExtra)
typedef void(BMX_CDECL *ZrwModelCallFn)(void *object);
#define gZrwRenderStateHook ORIG_VAR(0x00561490, ZrwRenderStateHookFn, gZrwRenderStateHook)
#define gZrwBeginHook       ORIG_VAR(0x00561494, ZrwRenderHookFn, gZrwBeginHook)
#define gZrwEndHook         ORIG_VAR(0x00561498, ZrwRenderHookFn, gZrwEndHook)
#define gZrwModelRegister   ORIG_VAR(0x00561470, ZrwModelRegisterFn, gZrwModelRegister)
#define gZrwModelUnregister ORIG_VAR(0x00561474, ZrwModelUnregisterFn, gZrwModelUnregister)
#define gZrwCamera          ORIG_VAR(0x006976f4, void *, gZrwCamera)
/* Colour handed to zrwModelSetColor after drawing a model (0x561818, 0x56181c). */
#define gZrwModelColor1 ORIG_VAR(0x00561818, uint32_t, gZrwModelColor1)
#define gZrwModelColor2 ORIG_VAR(0x0056181c, uint32_t, gZrwModelColor2)
/* The hit list the tree query fills (0x672908). */
#define gZrwColList ORIG_VAR(0x00672908, uint32_t, gZrwColList)

/* Vertex of RenderWare's immediate mode (0x24 bytes). */
typedef struct ZrwImVertex {
    float x, y, z;
    uint8_t pad0c[0xc];
    uint8_t color[4]; /* 0x18 */
    uint32_t u;       /* 0x1c */
    float v;          /* 0x20 */
} ZrwImVertex;

_Static_assert(sizeof(ZrwImVertex) == 0x24, "immediate vertex");

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
        if (triangle->tag != 0) {
            gZrwDbgTriangleHook(&c, &b, &a, fillColor, 2);
        }
        gZrwDbgTriangleHook(&a, &b, &c, fillColor, 2);
    }
    if (edgeColor != NULL) {
        if (triangle->drawEdge[0] != 0) {
            gZrwDbgLineHook(&a, &b, edgeColor, 0);
        }
        if (triangle->drawEdge[1] != 0) {
            gZrwDbgLineHook(&b, &c, edgeColor, 0);
        }
        if (triangle->drawEdge[2] != 0) {
            gZrwDbgLineHook(&c, &a, edgeColor, 0);
        }
    }
    if (pointColor != NULL) {
        const ZVec3 *const corners[3] = {&triangle->v0, &triangle->v1, &triangle->v2};

        for (int i = 0; i < 3; i++) {
            if (triangle->drawPoint[i] != 0) {
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
    /* The corner index is not range checked. */
    const ZVec3 *const from = (const ZVec3 *)((const uint8_t *)triangle + first * 0x10);
    const ZVec3 *const to = (const ZVec3 *)((const uint8_t *)triangle + second * 0x10);
    ZVec3 a, b;

    a.x = (float)((double)triangle->normal.x + from->x);
    a.y = (float)((double)from->y + triangle->normal.y);
    a.z = (float)((double)from->z + triangle->normal.z);
    b.x = (float)((double)triangle->normal.x + to->x);
    b.y = (float)((double)to->y + triangle->normal.y);
    b.z = (float)((double)to->z + triangle->normal.z);
    if (color != NULL && *(const uint32_t *)((const uint8_t *)triangle + 0x44 + first * 4) != 0) {
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
    gZrwColHit.tag = 0;
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

        if (triangle->tag != 0) {
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

BMX_ADDR(0x0048f320)
ZrwRenderColQuery *zrwModelQuery(const void *a, const void *b)
{
    gZrwColQuery.found = 0;
    gZrwColQuery.list = &gZrwColList;
    zrwColSearch(2, &gZrwColRoot, &gZrwColQuery, a, b);
    return gZrwColQuery.found != 0 ? &gZrwColQuery : NULL;
}

/*
 * Draws a model facing the camera: its frame is reset to a rotation about y that undoes the camera's yaw,
 * the model's own draw hook (+0xb4 of its definition) is called, and the model colour is switched around it.
 */
BMX_ADDR(0x0048f3e0)
void *zrwModelRenderBillboard(void *object)
{
    ZrwModel *const model = (ZrwModel *)(uintptr_t)zrwMdlGetUserData(object);
    const ZMatrix *cameraMatrix = RwFrameGetLTM(RwCameraGetFrame(gZrwCamera));
    ZMatrix *const frame = RwFrameGetLTM(*(void **)((uint8_t *)object + 4));
    uint32_t renderFlags;
    ZVec3 axis;
    ZVec3 position;
    ZAngle yaw;
    float angle;

    zrwModelSetColor(model, &model->onUpdateArg);
    gZrwBeginHook();
    renderFlags = (gZrwRenderFlags & 0xffff22ffu) | 0x2222;
    if (gZrwRenderFlags != renderFlags || gZrwRenderExtra != 0) {
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
    (*(ZrwModelCallFn *)((uint8_t *)model->def + 0xb4))(object);
    gZrwEndHook();
    zrwModelSetColor(model, &gZrwModelColor1);
    return object;
}

/* The render callback of a model's atomics: its definition's draw hook between two colour changes. */
BMX_ADDR(0x0048f530)
void *zrwModelRenderCallback(void *object)
{
    ZrwModel *const model = (ZrwModel *)(uintptr_t)zrwMdlGetUserData(object);

    zrwModelSetColor(model, &model->onUpdateArg);
    (*(ZrwModelCallFn *)((uint8_t *)model->def + 0xb4))(object);
    zrwModelSetColor(model, &gZrwModelColor2);
    return object;
}

/* Sends the buffered triangles of one material to the renderer, switching the texture when needed. */
static void flushTriangles(void *geometry, int material, int count)
{
    void *const materialPtr = RpGeometryGetMaterial(geometry, material);
    void *raster = RwTextureGetRaster(RpMaterialGetTexture(materialPtr));

    if ((int)(uintptr_t)raster != gZrwRenderExtra) {
        raster = RwTextureGetRaster(RpMaterialGetTexture(materialPtr));
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
    ZMatrix *const frame = RwFrameGetLTM(*(void **)((uint8_t *)atomic + 4));
    void *const geometry = *(void **)((uint8_t *)atomic + 0x1c);
    const int vertexCount = RpGeometryGetNumVertices(geometry);
    const float *vertices = RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geometry, 0));
    const int16_t *triangle = RpGeometryGetTriangles(geometry);
    const uint32_t *texCoords = RpGeometryGetTexCoords(geometry);
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
            gZrwRenderStateHook(renderFlags, (const void *)(uintptr_t)gZrwRenderExtra);
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
        void *const frame = *(void **)((uint8_t *)model->clump + 4);

        if (frame != NULL) {
            const ZVec3 position = RwFrameGetLTM(frame)->pos;

            RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0048fa90, zrwFrameDrawLines),
                                  (void *)&position);
        }
    }
}

/* Per-frame update: puts the clump at the camera's position and draws its atomics. 0 for unsupported models.
 */
BMX_ADDR(0x0048fbc0)
int zrwModelRender(ZrwModel *model)
{
    void *const clump = model->clump;

    if ((model->def->flags2 & 1) == 0) {
        zdebugFail("Not coded", kZrwModelRenderFile, 0x312, 0);
        return 0;
    }
    {
        const ZMatrix *const camera = RwFrameGetLTM(RwCameraGetFrame(gZrwCamera));

        RwMatrixTranslate(RwFrameGetMatrix(*(void **)((uint8_t *)clump + 4)), &camera->pos, 0);
    }
    RwFrameUpdateObjects(*(void **)((uint8_t *)clump + 4));
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
    gZrwModelRender.vertices = zmemAlloc(0x4010);
    gZrwModelRender.buffer2 = zmemAlloc(0x1800);
    gZrwModelRender.buffer3 = zmemAlloc(0x200);
    gZrwModelRender.indexCapacity = 0x100;
    gZrwModelRender.indices = zmemAlloc(0x600);
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

/* Draws the model right away (the same positioning as zrwModelRender) with the model's own post-update hook.
 */
BMX_ADDR(0x0048fda0)
void zrwModelRenderDirect(ZrwModel *model)
{
    void *const clump = model->clump;

    if ((model->def->flags2 & 1) == 0) {
        zdebugFail("Not coded", kZrwModelRenderFile, 900, 0);
        return;
    }
    {
        const ZMatrix *const camera = RwFrameGetLTM(RwCameraGetFrame(gZrwCamera));

        RwMatrixTranslate(RwFrameGetMatrix(*(void **)((uint8_t *)clump + 4)), &camera->pos, 0);
    }
    RwFrameUpdateObjects(*(void **)((uint8_t *)clump + 4));
    if ((model->def->flags2 & 2) != 0) {
        zrwModelPostUpdate(clump);
    }
    RpClumpRender(clump);
}

/* Per-atomic callback: stores the render callback and counts the change. */
BMX_ADDR(0x0048fea0)
void *zrwAtomicSetRenderCallback(void *atomic, void *callback)
{
    *(void **)((uint8_t *)atomic + 0x4c) = callback;
    (*(uint8_t *)((uint8_t *)atomic + 0x14))++;
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
