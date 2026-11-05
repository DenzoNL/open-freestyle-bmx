/* Collision queries against the renderer's world (C:\NgEngine\Zge\Com\ZglRw\zrwCol.c). */
#include "zge/com/zglrw/zrwcol.h"

#include "compat/crt.h"

#include "zge/com/zglrw/zrwmodel.h"
#include "zge/com/zglrw/zrwramdisk.h"
#include "zge/com/zglrw/zrwtxr.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zgemods/ztaginf.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kColFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwCol.c";

/* The two queries the platform layer exposes through the pointers at 0x562068 (line) and 0x56206c (sphere).
 */
typedef int(BMX_CDECL *ZrwColLineFn)(const float *a, const float *b, int mode, void *const *excluded,
                                     float *out);
typedef int(BMX_CDECL *ZrwColSphereFn)(const float *center, float radius, uint32_t flags,
                                       void *const *excluded, ZrwColSphereResult *out);
#define gZrwColLineFn   ORIG_VAR(0x00562068, ZrwColLineFn, gZrwColLineFn)
#define gZrwColSphereFn ORIG_VAR(0x0056206c, ZrwColSphereFn, gZrwColSphereFn)

BMX_ADDR(0x00493d80)
int zrwColSystemInitialize(void)
{
    if (gZrwCol.initialized != 0) {
        zdebugFail("Already initialized", kColFile, 0x1b9, 0);
    }
    zmemFill(&gZrwCol, 0, sizeof gZrwCol);
    gZrwCol.initialized = 1;
    gZrwCol.capacity = 0x400;
    gZrwCol.hits = zmemAlloc(gZrwCol.capacity * (uint32_t)sizeof(ZrwColHit));
    return 1;
}

BMX_ADDR(0x00493de0)
int zrwColSystemShutdown(void)
{
    if (gZrwCol.initialized != 0) {
        zmemFree(gZrwCol.hits);
        gZrwCol.hits = NULL;
        gZrwCol.initialized = 0;
    }
    return 1;
}

BMX_ADDR(0x00494240)
ZrwColTriangle *zrwcolKeepNearestSub(void *arg, const RpWorldSector *sub, ZrwColTriangle *triangle,
                                     float distance, RpAtomic *user)
{
    (void)arg;
    if (gZrwCol.found != 0 && distance > gZrwCol.nearest) {
        return triangle;
    }
    gZrwCol.found = 1;
    gZrwCol.bestUser = user;
    gZrwCol.bestSub = sub;
    gZrwCol.nearest = distance;
    zmemCopy(&gZrwCol.best, triangle, sizeof gZrwCol.best);
    return triangle;
}

BMX_ADDR(0x00494310)
ZrwColTriangle *zrwcolKeepNearest(void *arg, ZrwColTriangle *triangle, float distance, RpAtomic *user)
{
    (void)arg;
    if (gZrwCol.found != 0 && distance > gZrwCol.nearest) {
        return triangle;
    }
    gZrwCol.found = 1;
    gZrwCol.bestUser = user;
    gZrwCol.bestSub = NULL;
    gZrwCol.nearest = distance;
    zmemCopy(&gZrwCol.best, triangle, sizeof gZrwCol.best);
    return triangle;
}

/*
 * Whether the query wants this atomic: bit 1 and 2 of the flags select solid and non solid atomics (both set
 * takes everything), bit 3 turns on the list of ids to skip (ended by 0). An id is the atomic's clump
 * pointer.
 */
static int objectWanted(const RpAtomic *atomic, void *const *excluded)
{
    const uint32_t flags = gZrwCol.flags;

    if ((flags & 6) != 6) {
        const int solid = (atomic->object.object.flags & 0x80) != 0;

        if ((flags & 2) != 0 ? !solid : solid) {
            return 0;
        }
    }
    if ((flags & 8) != 0 && excluded != NULL && *excluded != NULL) {
        for (void *const *clump = excluded; *clump != NULL; clump++) {
            if (*clump == atomic->clump) {
                return 0;
            }
        }
    }
    return 1;
}

BMX_ADDR(0x004942a0)
RpAtomic *zrwcolFilterNearest(void *arg, void *unused, RpAtomic *atomic, void *unused2, void *const *excluded)
{
    (void)unused;
    (void)unused2;
    if (objectWanted(atomic, excluded)) {
        rwObjectIntersectTriangles(
            atomic, arg, BMX_FNPTR(RpIntersectionCallBackGeometryTriangle, 0x00494310, zrwcolKeepNearest),
            atomic);
    }
    return atomic;
}

BMX_ADDR(0x00494750)
RpAtomic *zrwcolFilterRecord(void *arg, void *unused, RpAtomic *atomic, void *unused2, void *const *excluded)
{
    (void)unused;
    (void)unused2;
    if (objectWanted(atomic, excluded)) {
        rwObjectIntersectTriangles(
            atomic, arg, BMX_FNPTR(RpIntersectionCallBackGeometryTriangle, 0x004947c0, zrwcolRecordObjectHit),
            atomic);
    }
    return atomic;
}

/* Corner of a triangle in game space: x as is, y and z mirrored. */
static void gameCorner(float *out, float x, float y, float z)
{
    const double inv = gZrwInvScale;

    out[0] = (float)(((double)x - gZrwWorldCenter.x) * inv);
    out[1] = -(float)(((double)y - gZrwWorldCenter.y) * inv);
    out[2] = -(float)(((double)z - gZrwWorldCenter.z) * inv);
}

/* User data of the texture of a material (what zrwMatGetUserData reads), NULL without a texture. */
static void *materialData(RpMaterial *material)
{
    void *data = RpMaterialGetTexture(material);

    return (data == NULL) ? NULL : zrwMatGetUserData(data);
}

/* Material of a world polygon: its material index is relative to the sector's window into the world list. */
static RpMaterial *polygonMaterial(const RpWorldSector *sector, int32_t index)
{
    return RpWorldGetMaterial(gZrwWorld, (uint16_t)((uint16_t)sector->polygons[(uint32_t)index].matIndex +
                                                    sector->matListWindowBase));
}

/* Material of a triangle of an atomic's geometry; the index is not range checked (a negative material is
 * read as the unsigned 16 bit value, as the original does). */
static RpMaterial *triangleMaterial(const RpAtomic *atomic, int32_t index)
{
    RpGeometry *geometry = atomic->geometry;

    return RpGeometryGetMaterial(geometry,
                                 (uint16_t)RpGeometryGetTriangles(geometry)[(uint32_t)index].matIndex);
}

BMX_ADDR(0x00494490)
ZrwColTriangle *zrwcolRecordHit(void *arg, const RpWorldSector *sector, ZrwColTriangle *triangle,
                                float distance, RpAtomic *atomic)
{
    ZrwColState *state = &gZrwCol;
    ZrwColHit *hit;

    (void)arg;
    if (state->count == state->capacity) {
        state->capacity += 0x10;
        state->hits = zmemResize(state->hits, state->capacity * sizeof(ZrwColHit));
        hit = state->hits + state->count;
    } else {
        hit = state->cursor;
    }
    state->cursor = hit + 1;
    state->count++;
    state->found = 1;
    if (atomic == NULL) {
        hit->normal[0] = triangle->normal[0];
        hit->normal[1] = -triangle->normal[1];
        hit->normal[2] = -triangle->normal[2];
        for (int i = 0; i < 3; i++) {
            const float *p = triangle->vertex[i];

            gameCorner(hit->vertex[i], p[0], p[1], p[2]);
        }
        hit->distance = distance;
        hit->objectData = NULL;
        hit->materialData = materialData(polygonMaterial(sector, triangle->index));
        return triangle;
    }
    {
        const void *matrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));
        float moved[3];
        float normal[3];

        for (int i = 0; i < 3; i++) {
            RwV3dTransformPoints((RwV3d *)moved, (const RwV3d *)triangle->vertex[i], 1, matrix);
            gameCorner(hit->vertex[i], moved[0], moved[1], moved[2]);
        }
        RwV3dTransformVectors((RwV3d *)normal, (const RwV3d *)triangle, 1, matrix);
        hit->normal[1] = -normal[1];
        hit->normal[2] = -normal[2];
        hit->normal[0] = normal[0];
        hit->objectData = zrwMdlGetUserData(atomic);
        hit->materialData = materialData(triangleMaterial(atomic, triangle->index));
    }
    return triangle;
}

BMX_ADDR(0x004947c0)
ZrwColTriangle *zrwcolRecordObjectHit(void *arg, ZrwColTriangle *triangle, float distance, RpAtomic *atomic)
{
    return zrwcolRecordHit(arg, NULL, triangle, distance, atomic);
}

/* Game space to RenderWare space: scaled, y and z mirrored about the world centre. */
static void worldPoint(float *out, const float *p)
{
    const double scale = gZrwScale;

    out[0] = (float)(scale * p[0] + gZrwWorldCenter.x);
    out[1] = (float)(gZrwWorldCenter.y - (double)p[1] * scale);
    out[2] = (float)(gZrwWorldCenter.z - (double)p[2] * scale);
}

/*
 * Original quirk: the world box is widened by 1e-5 on every call, before the optional clip, so it keeps
 * growing. Without a nearest object or sector it asserts and still fills in the distance and the point.
 */
BMX_ADDR(0x00493e20)
int zrwcolQueryLine(float *from, float *to, uint32_t flags, void *const *excluded, ZrwColLineHit *out)
{
    ZrwColLineVolume line;
    const ZrwColTriangle *best = &gZrwCol.best;
    ZVec3 d;

    zvec3SubScalar((ZVec3 *)&gZrwColBox[0], (const ZVec3 *)&gZrwColBox[0], 0x1.4f8b58p-17f);
    zvec3AddScalar((ZVec3 *)&gZrwColBox[4], (const ZVec3 *)&gZrwColBox[4], 0x1.4f8b58p-17f);
    if ((flags & 0x20) != 0 && zrwBBoxClipSegment(from, to, gZrwColBox) == 0) {
        return 0;
    }
    line.type = 1;
    worldPoint(line.start, from);
    worldPoint(line.end, to);
    gZrwCol.found = 0;
    gZrwCol.flags = flags;
    if ((flags & 1) != 0) {
        rwWorldIntersectTriangles(
            gZrwWorld, (RpIntersection *)&line,
            BMX_FNPTR(RpIntersectionCallBackWorldTriangle, 0x00494240, zrwcolKeepNearestSub), NULL);
    }
    if ((flags & 6) != 0) {
        rwWorldIntersectObjects(gZrwWorld, (RpIntersection *)&line,
                                BMX_FNPTR(RpIntersectionCallBackAtomic, 0x004942a0, zrwcolFilterNearest),
                                (void *)excluded);
    }
    if (gZrwCol.found == 0) {
        return 0;
    }
    if (gZrwCol.bestUser != NULL) {
        RpAtomic *atomic = gZrwCol.bestUser;
        const void *matrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));
        float moved[3];

        for (int i = 0; i < 3; i++) {
            RwV3dTransformPoints((RwV3d *)moved, (const RwV3d *)best->vertex[i], 1, matrix);
            gameCorner(out->hit.vertex[i], moved[0], moved[1], moved[2]);
        }
        RwV3dTransformVectors((RwV3d *)moved, (const RwV3d *)best, 1, matrix);
        out->hit.normal[1] = -moved[1];
        out->hit.normal[2] = -moved[2];
        out->hit.normal[0] = moved[0];
        out->hit.objectData = zrwMdlGetUserData(atomic);
        out->hit.materialData = materialData(triangleMaterial(atomic, best->index));
    } else if (gZrwCol.bestSub != NULL) {
        const RpWorldSector *sector = gZrwCol.bestSub;

        out->hit.normal[0] = best->normal[0];
        out->hit.normal[1] = -best->normal[1];
        out->hit.normal[2] = -best->normal[2];
        for (int i = 0; i < 3; i++) {
            const float *p = best->vertex[i];

            gameCorner(out->hit.vertex[i], p[0], p[1], p[2]);
        }
        out->hit.objectData = NULL;
        out->hit.materialData = materialData(polygonMaterial(sector, best->index));
    } else {
        zdebugFail("Error", kColFile, 0x25b, 0);
    }
    out->hit.distance = gZrwCol.nearest;
    zvec3Sub(&d, (const ZVec3 *)to, (const ZVec3 *)from);
    zvec3Scale(&d, &d, gZrwCol.nearest);
    zvec3Add((ZVec3 *)out->point, (const ZVec3 *)from, &d);
    return 1;
}

static void swapCorners(float *a, float *b)
{
    for (int i = 0; i < 3; i++) {
        const float t = a[i];

        a[i] = b[i];
        b[i] = t;
    }
}

/* Channel `t` of the way from lo towards hi (truncated through _ftol). */
static uint8_t blendChannel(uint8_t lo, uint8_t hi, double t)
{
    return (uint8_t)bmxFtol((double)lo - (double)((int32_t)lo - (int32_t)hi) * t);
}

/*
 * Where the point lies on an edge from `a` with the extent (dx, dz): measured along x when the edge spans
 * more x (as a float) than z, else along z.
 */
static double edgeRatio(const float *a, float dxRounded, double dz, const float *point)
{
    if (!(__builtin_fabs(dz) >= __builtin_fabs((double)dxRounded))) {
        return ((double)a[0] - (double)point[0]) / (double)dxRounded;
    }
    return ((double)a[2] - (double)point[2]) / dz;
}

/*
 * The corners are sorted by height, but their colours stay with the triangle's vertex order (original
 * quirk). The colour is blended along the edge the point's height falls on, along the long edge v0-v2, and
 * then between those two points by x (or z when the x gap is the smaller one; 1 when the z gap is 0 or NaN).
 */
BMX_ADDR(0x004947e0)
uint8_t *zrwcolHitColor(ZrwColLineHit *hit)
{
    const ZrwColTriangle *best = &gZrwCol.best;
    const RpWorldSector *sector;
    const RpPolygon *polygon;
    uint8_t color[3][3];
    float *v0 = hit->hit.vertex[0];
    float *v1 = hit->hit.vertex[1];
    float *v2 = hit->hit.vertex[2];
    const float *point = hit->point;
    const float *lo;
    const float *hi;
    const uint8_t *cLo;
    const uint8_t *cHi;
    uint8_t near[3];
    uint8_t far[3];
    double t;
    double dx;
    double dz;
    float dxRounded;
    float x1;
    double z1;
    float x2;
    float z2;
    float wx;
    float wz;

    gZrwColColor[0] = 0;
    gZrwColColor[1] = 0;
    gZrwColColor[2] = 0;
    gZrwColColor[3] = 0xff;
    if (gZrwCol.bestSub == NULL) {
        return gZrwColColor;
    }
    sector = gZrwCol.bestSub;
    polygon = &sector->polygons[(uint32_t)best->index];
    for (int k = 0; k < 3; k++) {
        /* The first three bytes (r g b) of the vertex's prelit colour. */
        zmemCopy(color[k], &sector->preLitLum[polygon->vertIndex[k]], 3);
    }
    if (!(v1[2] >= v0[2])) {
        swapCorners(v0, v1);
    }
    if (!(v2[2] >= v1[2])) {
        swapCorners(v1, v2);
    }
    if (!(v1[2] >= v0[2])) {
        swapCorners(v0, v1);
    }
    if (point[2] > v1[2]) {
        lo = v1;
        hi = v2;
        cLo = color[1];
        cHi = color[2];
    } else {
        lo = v0;
        hi = v1;
        cLo = color[0];
        cHi = color[1];
    }
    dx = (double)lo[0] - (double)hi[0];
    dxRounded = (float)dx;
    dz = (double)lo[2] - (double)hi[2];
    t = edgeRatio(lo, dxRounded, dz, point);
    for (int ch = 0; ch < 3; ch++) {
        near[ch] = blendChannel(cLo[ch], cHi[ch], t);
    }
    x1 = (float)((double)lo[0] - dx * t);
    z1 = (double)lo[2] - dz * t;

    dxRounded = (float)((double)v0[0] - (double)v2[0]);
    dz = (double)v0[2] - (double)v2[2];
    t = edgeRatio(v0, dxRounded, dz, point);
    for (int ch = 0; ch < 3; ch++) {
        far[ch] = blendChannel(color[0][ch], color[2][ch], t);
    }
    x2 = (float)((double)v0[0] - (double)dxRounded * t);
    z2 = (float)((double)v0[2] - dz * t);

    wx = (float)((double)x2 - (double)x1);
    wz = (float)((double)z2 - z1);
    if (!(__builtin_fabs((double)wz) >= __builtin_fabs((double)wx))) {
        t = ((double)x2 - (double)point[0]) / (double)wx;
    } else if (wz < 0.0f || wz > 0.0f) {
        t = ((double)z2 - (double)point[2]) / (double)wz;
    } else {
        t = 1.0;
    }
    for (int ch = 0; ch < 3; ch++) {
        gZrwColColor[ch] = blendChannel(far[ch], near[ch], t);
    }
    gZrwColColor[3] = 0;
    return gZrwColColor;
}

BMX_ADDR(0x00494370)
int zrwcolQuerySphere(const float *center, float radius, uint32_t flags, void *const *excluded,
                      ZrwColSphereResult *out)
{
    ZrwColIntersection volume = {0};

    if (zrwBBoxContainsPoint(gZrwColBox, center) == 0) {
        return 0;
    }
    gZrwCol.count = 0;
    gZrwCol.cursor = gZrwCol.hits;
    volume.type = 3;
    volume.x = (float)((double)gZrwScale * center[0] + gZrwWorldCenter.x);
    volume.y = (float)(gZrwWorldCenter.y - (double)center[1] * gZrwScale);
    gZrwCol.found = 0;
    gZrwCol.nearest = 9999.0f;
    gZrwCol.flags = flags;
    volume.z = (float)(gZrwWorldCenter.z - (double)center[2] * gZrwScale);
    volume.radius = (float)((double)gZrwScale * radius);
    if ((flags & 1) != 0) {
        rwWorldIntersectTriangles(gZrwWorld, (RpIntersection *)&volume,
                                  BMX_FNPTR(RpIntersectionCallBackWorldTriangle, 0x00494490, zrwcolRecordHit),
                                  NULL);
    }
    if ((flags & 6) != 0) {
        rwWorldIntersectObjects(gZrwWorld, (RpIntersection *)&volume,
                                BMX_FNPTR(RpIntersectionCallBackAtomic, 0x00494750, zrwcolFilterRecord),
                                (void *)excluded);
    }
    if (gZrwCol.found != 0) {
        out->hits = gZrwCol.hits;
        out->count = gZrwCol.count;
        return 1;
    }
    return 0;
}

BMX_ADDR(0x00494c20)
int zrwcolEnableIfTagInfo(void)
{
    if (ztaginfInitialize() != 0) {
        gZrwColEnabled = 1;
        return 1;
    }
    return 0;
}

BMX_ADDR(0x00494c70)
int zrwcolLineTest(const float *a, const float *b, int mode, void *const *excluded, float *out)
{
    return gZrwColLineFn(a, b, mode, excluded, out) != 0;
}

BMX_ADDR(0x00494ca0)
int zrwcolSphereTest(const float *center, float radius, uint32_t flags, void *const *excluded,
                     ZrwColSphereResult *out)
{
    return gZrwColSphereFn(center, radius, flags, excluded, out) != 0;
}
