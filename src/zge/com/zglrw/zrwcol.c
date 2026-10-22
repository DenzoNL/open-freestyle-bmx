/* Collision queries against the renderer's world (C:\NgEngine\Zge\Com\ZglRw\zrwCol.c). */
#include "zge/com/zglrw/zrwcol.h"

#include "zge/com/zglrw/zrwmodel.h"
#include "zge/com/zglrw/zrwramdisk.h"
#include "zge/com/zglrw/zrwtxr.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kColFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwCol.c";

/* The two queries the platform layer exposes through the pointers at 0x562068 (line) and 0x56206c (sphere).
 */
typedef int(BMX_CDECL *ZrwColLineFn)(const float *a, const float *b, int mode, int flags, float *out);
typedef int(BMX_CDECL *ZrwColSphereFn)(const float *center, float radius, uint32_t flags, const int *excluded,
                                       uint32_t *out);
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
    gZrwCol.hits = zmemAlloc(0x13000);
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
ZrwColTriangle *zrwcolKeepNearestSub(void *arg, uint32_t sub, ZrwColTriangle *triangle, float distance,
                                     uint32_t user)
{
    (void)arg;
    if (gZrwCol.found != 0 && distance > gZrwCol.nearest) {
        return triangle;
    }
    gZrwCol.found = 1;
    gZrwCol.bestUser = user;
    gZrwCol.bestSub = sub;
    gZrwCol.nearest = distance;
    zmemCopy(gZrwCol.best, triangle, sizeof gZrwCol.best);
    return triangle;
}

BMX_ADDR(0x00494310)
ZrwColTriangle *zrwcolKeepNearest(void *arg, ZrwColTriangle *triangle, float distance, uint32_t user)
{
    (void)arg;
    if (gZrwCol.found != 0 && distance > gZrwCol.nearest) {
        return triangle;
    }
    gZrwCol.found = 1;
    gZrwCol.bestUser = user;
    gZrwCol.bestSub = 0;
    gZrwCol.nearest = distance;
    zmemCopy(gZrwCol.best, triangle, sizeof gZrwCol.best);
    return triangle;
}

/*
 * Whether the query wants this object: bit 1 and 2 of the flags select solid and non solid objects (both set
 * takes everything), bit 3 turns on the list of ids to skip (ended by 0).
 */
static int objectWanted(const ZrwColPoly *poly, const int *excluded)
{
    const uint32_t flags = gZrwCol.flags;

    if ((flags & 6) != 6) {
        const int solid = (poly->flags & 0x80) != 0;

        if ((flags & 2) != 0 ? !solid : solid) {
            return 0;
        }
    }
    if ((flags & 8) != 0 && excluded != NULL && *excluded != 0) {
        for (const int *id = excluded; *id != 0; id++) {
            if (*id == poly->id) {
                return 0;
            }
        }
    }
    return 1;
}

BMX_ADDR(0x004942a0)
ZrwColPoly *zrwcolFilterNearest(void *arg, void *unused, ZrwColPoly *poly, void *unused2, const int *excluded)
{
    (void)unused;
    (void)unused2;
    if (objectWanted(poly, excluded)) {
        rwObjectIntersectTriangles(poly, arg, BMX_FNPTR(void *, 0x00494310, zrwcolKeepNearest), poly);
    }
    return poly;
}

BMX_ADDR(0x00494750)
ZrwColPoly *zrwcolFilterRecord(void *arg, void *unused, ZrwColPoly *poly, void *unused2, const int *excluded)
{
    (void)unused;
    (void)unused2;
    if (objectWanted(poly, excluded)) {
        rwObjectIntersectTriangles(poly, arg, BMX_FNPTR(void *, 0x004947c0, zrwcolRecordObjectHit), poly);
    }
    return poly;
}

/* Corner of a triangle in game space: x as is, y and z mirrored. */
static void gameCorner(float *out, float x, float y, float z)
{
    const double inv = gZrwInvScale;

    out[0] = (float)(((double)x - gZrwWorldCenter.x) * inv);
    out[1] = -(float)(((double)y - gZrwWorldCenter.y) * inv);
    out[2] = -(float)(((double)z - gZrwWorldCenter.z) * inv);
}

/* Material user data of the material behind a handle of the world or of a model. */
static uint32_t materialData(void *material)
{
    void *data = rwMaterialGetData(material);

    return (data == NULL) ? 0 : zrwMatGetUserData(data);
}

BMX_ADDR(0x00494490)
ZrwColTriangle *zrwcolRecordHit(void *arg, const ZrwColSector *sector, ZrwColTriangle *triangle,
                                float distance, const ZrwColObject *object)
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
    if (object == NULL) {
        hit->normal[0] = triangle->normal[0];
        hit->normal[1] = -triangle->normal[1];
        hit->normal[2] = -triangle->normal[2];
        for (int i = 0; i < 3; i++) {
            const float *p = triangle->vertex[i];

            gameCorner(hit->vertex[i], p[0], p[1], p[2]);
        }
        hit->distance = distance;
        hit->objectData = 0;
        hit->materialData = materialData(rwWorldGetMaterial(
            gZrwWorld,
            (uint16_t)((uint16_t)sector->map[(uint32_t)triangle->index * 4] + (uint16_t)sector->base)));
        return triangle;
    }
    {
        const void *matrix = rwFrameGetLTM(object->frame);
        float moved[3];
        float normal[3];

        for (int i = 0; i < 3; i++) {
            rwV3dTransformPoints(moved, triangle->vertex[i], 1, matrix);
            gameCorner(hit->vertex[i], moved[0], moved[1], moved[2]);
        }
        rwV3dTransformVectors(normal, triangle, 1, matrix);
        hit->normal[1] = -normal[1];
        hit->normal[2] = -normal[2];
        hit->normal[0] = normal[0];
        hit->objectData = zrwMdlGetUserData(object);
        {
            void *model = object->model;
            const uint8_t *table = rwModelGetMaterialTable(model);

            hit->materialData = materialData(
                rwModelGetMaterial(model, *(const uint16_t *)(table + (uint32_t)triangle->index * 8 + 6)));
        }
    }
    return triangle;
}

BMX_ADDR(0x004947c0)
ZrwColTriangle *zrwcolRecordObjectHit(void *arg, ZrwColTriangle *triangle, float distance,
                                      const ZrwColObject *object)
{
    return zrwcolRecordHit(arg, NULL, triangle, distance, object);
}

BMX_ADDR(0x00494370)
int zrwcolQuerySphere(const float *center, float radius, uint32_t flags, const int *excluded, uint32_t *out)
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
        rwWorldIntersectTriangles(gZrwWorld, &volume, BMX_FNPTR(void *, 0x00494490, zrwcolRecordHit), NULL);
    }
    if ((flags & 6) != 0) {
        rwWorldIntersectObjects(gZrwWorld, &volume, BMX_FNPTR(void *, 0x00494750, zrwcolFilterRecord),
                                excluded);
    }
    if (gZrwCol.found != 0) {
        out[0] = (uint32_t)(uintptr_t)gZrwCol.hits;
        out[1] = gZrwCol.count;
        return 1;
    }
    return 0;
}

BMX_ADDR(0x00494c20)
void zrwcolEnableIfTagInfo(void)
{
    if (ztaginfInitialize() != 0) {
        gZrwColEnabled = 1;
    }
}

BMX_ADDR(0x00494c70)
int zrwcolLineTest(const float *a, const float *b, int mode, int flags, float *out)
{
    return gZrwColLineFn(a, b, mode, flags, out) != 0;
}

BMX_ADDR(0x00494ca0)
int zrwcolSphereTest(const float *center, float radius, uint32_t flags, const int *excluded, uint32_t *out)
{
    return gZrwColSphereFn(center, radius, flags, excluded, out) != 0;
}
