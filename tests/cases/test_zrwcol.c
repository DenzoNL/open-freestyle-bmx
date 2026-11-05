/*
 * Tests for zrwCol.c: the collision system's state, the nearest hit bookkeeping, the object filters, the hit
 * records of the world and of objects, and the sphere query. RenderWare (frames, transforms, material
 * lookups, the intersection enumerators) is stubbed; the stubs log what they get and answer from the seeded
 * generator.
 */
#include "dt.h"
#include "dt_ngif.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwcol.h"
#include "zge/com/zglrw/zrwmodel.h"

static float rf(void)
{
    return dt_float_edgy(-1000.0f, 1000.0f);
}

/* ---- RenderWare stand-ins -------------------------------------------------------------------------- */

static ZMatrix gMatrix;
/* Set by the line query case: the intersection volume is a line of six floats. */
static int gLogLine;
static uint8_t gTable[64];

static void *BMX_CDECL stubWorldMaterial(void *world, uint32_t index)
{
    ngLog(0x4ba140);
    ngLogPtr(world);
    ngLog(index);
    return (dt_u32() % 5 == 0) ? NULL : (void *)(uintptr_t)(dt_u32() | 8u);
}

static void *BMX_CDECL stubMaterialData(void *material)
{
    ngLog(0x4b93d0);
    ngLogPtr(material);
    return (dt_u32() % 4 == 0) ? NULL : (void *)(uintptr_t)(dt_u32() | 8u);
}

static uint32_t BMX_CDECL stubMatUserData(const void *material)
{
    ngLog(0x497ae0);
    ngLogPtr(material);
    return dt_u32();
}

static uint32_t BMX_CDECL stubMdlUserData(const void *object)
{
    ngLog(0x488b80);
    ngLogPtr(object);
    return dt_u32();
}

static void *BMX_CDECL stubFrameLtm(void *frame)
{
    ngLog(0x4d2b10);
    ngLogPtr(frame);
    return &gMatrix;
}

static void BMX_CDECL stubTransformPoints(float *out, const void *in, int count, const void *matrix)
{
    ngLog(0x4d3a90);
    ngLogPtr(in);
    ngLog((uint32_t)count);
    ngLogPtr(matrix);
    for (int i = 0; i < 3; i++) {
        out[i] = rf();
    }
}

static void BMX_CDECL stubTransformVectors(float *out, const void *in, int count, const void *matrix)
{
    ngLog(0x4d3ae0);
    ngLogPtr(in);
    ngLog((uint32_t)count);
    ngLogPtr(matrix);
    for (int i = 0; i < 3; i++) {
        out[i] = rf();
    }
}

static uint8_t *BMX_CDECL stubModelTable(void *model)
{
    ngLog(0x4b46b0);
    ngLogPtr(model);
    return gTable;
}

static void *BMX_CDECL stubModelMaterial(void *model, uint32_t index)
{
    ngLog(0x4b47a0);
    ngLogPtr(model);
    ngLog(index);
    return (dt_u32() % 5 == 0) ? NULL : (void *)(uintptr_t)(dt_u32() | 8u);
}

static void BMX_CDECL stubIntersectTriangles(void *world, const ZrwColIntersection *volume, void *callback,
                                             void *arg)
{
    ngLog(0x4c1b10);
    ngLogPtr(world);
    ngLogFloat(volume->x);
    ngLogFloat(volume->y);
    ngLogFloat(volume->z);
    ngLogFloat(volume->radius);
    ngLog((uint32_t)volume->type);
    if (gLogLine) { /* a line: the end point continues past the radius slot */
        ngLog(volume->pad10[0]);
        ngLog(volume->pad10[1]);
    }
    ngLogPtr(callback);
    ngLogPtr(arg);
    if (dt_u32() % 2 == 0) { /* a hit */
        gZrwCol.found = 1;
        gZrwCol.count = (uint32_t)dt_range(1, 5);
    }
}

static void BMX_CDECL stubIntersectObjects(void *world, const ZrwColIntersection *volume, void *callback,
                                           const void *arg)
{
    ngLog(0x4c1c10);
    ngLogPtr(world);
    ngLogFloat(volume->x);
    ngLogFloat(volume->y);
    ngLogFloat(volume->z);
    ngLogFloat(volume->radius);
    ngLog((uint32_t)volume->type);
    if (gLogLine) { /* a line: the end point continues past the radius slot */
        ngLog(volume->pad10[0]);
        ngLog(volume->pad10[1]);
    }
    ngLogPtr(callback);
    ngLogPtr(arg);
    if (dt_u32() % 2 == 0) {
        gZrwCol.found = 1;
        gZrwCol.count = (uint32_t)dt_range(1, 5);
    }
}

static void BMX_CDECL stubObjectTriangles(void *object, void *arg, void *callback, void *user)
{
    ngLog(0x4c1f30);
    ngLogPtr(object);
    ngLogPtr(arg);
    ngLogPtr(callback);
    ngLogPtr(user);
}

static void install_rw_stubs(void)
{
    dt_stub(0x004ba140, NULL, (void *)stubWorldMaterial);
    dt_stub(0x004b93d0, NULL, (void *)stubMaterialData);
    dt_stub(0x00497ae0, NULL, (void *)stubMatUserData);
    dt_stub(0x00488b80, NULL, (void *)stubMdlUserData);
    dt_stub(0x004d2b10, NULL, (void *)stubFrameLtm);
    dt_stub(0x004d3a90, NULL, (void *)stubTransformPoints);
    dt_stub(0x004d3ae0, NULL, (void *)stubTransformVectors);
    dt_stub(0x004b46b0, NULL, (void *)stubModelTable);
    dt_stub(0x004b47a0, NULL, (void *)stubModelMaterial);
    dt_stub(0x004c1b10, NULL, (void *)stubIntersectTriangles);
    dt_stub(0x004c1c10, NULL, (void *)stubIntersectObjects);
    dt_stub(0x004c1f30, NULL, (void *)stubObjectTriangles);
}

/* ---- fixtures -------------------------------------------------------------------------------------- */

static void begin(DtCtx *c)
{
    ngBegin(c, 0);
    install_rw_stubs();
    dt_fill(&gMatrix, sizeof gMatrix);
    dt_fill(gTable, sizeof gTable);
    gZrwWorld = (void *)(uintptr_t)(dt_u32() | 4u);
    gZrwWorldCenter = (ZVec3){rf(), rf(), rf()};
    gZrwScale = dt_float(0.25f, 4.0f);
    gZrwInvScale = dt_float(0.25f, 4.0f);
}

/* State with a record buffer of `capacity` records, `count` in use. */
static void random_state(int capacity, int count)
{
    ZrwColHit *hits = dt_zmemalloc((uint32_t)capacity * sizeof(ZrwColHit));

    dt_fill(&gZrwCol, sizeof gZrwCol);
    dt_fill(hits, (uint32_t)capacity * sizeof(ZrwColHit));
    gZrwCol.initialized = 1;
    gZrwCol.hits = hits;
    gZrwCol.capacity = (uint32_t)capacity;
    gZrwCol.count = (uint32_t)count;
    gZrwCol.cursor = hits + count;
    gZrwCol.found = (int)(dt_u32() & 1);
    gZrwCol.nearest = dt_float_edgy(0.0f, 20000.0f);
}

static void setup_system_init(DtCtx *c)
{
    ngBegin(c, 1);
    dt_fill(&gZrwCol, sizeof gZrwCol);
    if (dt_u32() & 1) {
        gZrwCol.initialized = 0;
    }
}

static void setup_system_exit(DtCtx *c)
{
    ngBegin(c, 0);
    random_state(dt_range(1, 4), 0);
    gZrwCol.initialized = (int)(dt_u32() & 1);
}

static ZrwColTriangle *random_triangle(void)
{
    ZrwColTriangle *t = dt_alloc(sizeof *t);

    dt_fill(t, sizeof *t);
    for (int i = 0; i < 3; i++) {
        float *v = dt_alloc(3 * sizeof *v);

        t->normal[i] = dt_float_edgy(-1.0f, 1.0f);
        v[0] = rf();
        v[1] = rf();
        v[2] = rf();
        t->vertex[i] = v;
    }
    t->index = dt_range(0, 7);
    return t;
}

static void setup_keep_sub(DtCtx *c)
{
    float distance;

    begin(c);
    distance = (dt_u32() % 3 == 0) ? 0.0f : dt_float_edgy(0.0f, 20000.0f);
    random_state(2, 0);
    if (dt_u32() % 3 == 0) {
        gZrwCol.nearest = distance;
    }
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, random_triangle());
    DT_ARG_FLOAT(c, 3, distance);
    DT_ARG_INT(c, 4, dt_u32());
}

static void setup_keep(DtCtx *c)
{
    float distance;

    begin(c);
    distance = dt_float_edgy(0.0f, 20000.0f);
    random_state(2, 0);
    if (dt_u32() % 3 == 0) {
        gZrwCol.nearest = distance;
    }
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_PTR(c, 1, random_triangle());
    DT_ARG_FLOAT(c, 2, distance);
    DT_ARG_INT(c, 3, dt_u32());
}

/* An object as the filters see it: the flag byte and id, and a list of ids to skip. */
static void setup_filter(DtCtx *c)
{
    RpAtomic *poly = dt_alloc(sizeof *poly);
    int *excluded = dt_alloc(8 * sizeof *excluded);

    begin(c);
    random_state(2, 0);
    dt_fill(poly, sizeof *poly);
    gZrwCol.flags = (uint32_t)dt_range(0, 15);
    poly->clump = (RpClump *)(uintptr_t)dt_range(1, 6);
    for (int i = 0; i < 5; i++) {
        excluded[i] = dt_range(1, 6);
    }
    excluded[dt_range(0, 5)] = 0;
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_PTR(c, 1, dt_alloc(8));
    DT_ARG_PTR(c, 2, poly);
    DT_ARG_PTR(c, 3, dt_alloc(8));
    DT_ARG_PTR(c, 4, (dt_u32() % 5 == 0) ? NULL : excluded);
}

static void setup_record(DtCtx *c, int withObject)
{
    RpWorldSector *sector = dt_alloc(sizeof *sector);
    RpAtomic *object = dt_alloc(sizeof *object);
    int capacity;
    float distance;

    begin(c);
    capacity = dt_range(2, 5);
    distance = rf();
    random_state(capacity, (dt_u32() % 3 == 0) ? capacity : dt_range(0, capacity - 1));
    dt_fill(sector, sizeof *sector);
    sector->polygons = dt_alloc(64);
    dt_fill(sector->polygons, 64);
    dt_fill(object, sizeof *object);
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_PTR(c, 1, sector);
    DT_ARG_PTR(c, 2, random_triangle());
    DT_ARG_FLOAT(c, 3, distance);
    DT_ARG_PTR(c, 4, withObject ? object : NULL);
}

static void setup_record_world(DtCtx *c)
{
    setup_record(c, 0);
}

static void setup_record_object(DtCtx *c)
{
    setup_record(c, 1);
}

/* The same without the sector: 0x4947c0 passes NULL for it, and the object is always there. */
static void setup_record_object_short(DtCtx *c)
{
    RpAtomic *object = dt_alloc(sizeof *object);
    int capacity;

    begin(c);
    capacity = dt_range(2, 5);
    random_state(capacity, (dt_u32() % 3 == 0) ? capacity : dt_range(0, capacity - 1));
    dt_fill(object, sizeof *object);
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_PTR(c, 1, random_triangle());
    DT_ARG_FLOAT(c, 2, rf());
    DT_ARG_PTR(c, 3, object);
}

static void random_box(float *box)
{
    for (int i = 0; i < 3; i++) {
        box[i] = dt_float_edgy(-100.0f, 0.0f);
        box[4 + i] = dt_float_edgy(10.0f, 100.0f);
    }
    box[3] = 0.0f;
    box[7] = 0.0f;
}

static void setup_sphere_query(DtCtx *c)
{
    float *center = dt_alloc(3 * sizeof *center);
    uint32_t *out = dt_alloc(2 * sizeof *out);
    int *excluded = dt_alloc(4 * sizeof *excluded);

    begin(c);
    random_state(dt_range(2, 4), 0);
    random_box(gZrwColBox);
    for (int i = 0; i < 3; i++) {
        center[i] =
            (dt_u32() % 5 == 0) ? dt_float_edgy(-300.0f, 300.0f) : dt_float(gZrwColBox[i], gZrwColBox[4 + i]);
    }
    DT_ARG_PTR(c, 0, center);
    DT_ARG_FLOAT(c, 1, dt_float_edgy(0.0f, 50.0f));
    DT_ARG_INT(c, 2, dt_u32() & 15);
    DT_ARG_PTR(c, 3, excluded);
    DT_ARG_PTR(c, 4, out);
}

/*
 * The line query. The intersection stubs report a hit now and then; the nearest triangle, object and sector
 * are set up beforehand, as the triangle callbacks would have left them.
 */
static void setup_line_query(DtCtx *c)
{
    float *from = dt_alloc(3 * sizeof *from);
    float *to = dt_alloc(3 * sizeof *to);
    ZrwColLineHit *out = dt_alloc(sizeof *out);
    int *excluded = dt_alloc(4 * sizeof *excluded);
    ZrwColTriangle *best;
    RpWorldSector *sector = dt_alloc(sizeof *sector);
    RpAtomic *object = dt_alloc(sizeof *object);

    begin(c);
    dt_stub(0x0047ed10, NULL, (void *)ngFail);
    gLogLine = 1;
    random_state(2, 0);
    random_box(gZrwColBox);
    for (int i = 0; i < 3; i++) {
        from[i] =
            (dt_u32() % 4 == 0) ? dt_float_edgy(-300.0f, 300.0f) : dt_float(gZrwColBox[i], gZrwColBox[4 + i]);
        to[i] =
            (dt_u32() % 4 == 0) ? dt_float_edgy(-300.0f, 300.0f) : dt_float(gZrwColBox[i], gZrwColBox[4 + i]);
    }
    best = random_triangle();
    __builtin_memcpy(&gZrwCol.best, best, sizeof gZrwCol.best);
    dt_fill(sector, sizeof *sector);
    sector->polygons = dt_alloc(64);
    dt_fill(sector->polygons, 64);
    dt_fill(object, sizeof *object);
    switch (dt_u32() % 5) {
    case 0:
        gZrwCol.bestUser = NULL;
        gZrwCol.bestSub = NULL;
        break;
    case 1:
    case 2:
        gZrwCol.bestUser = object;
        break;
    default:
        gZrwCol.bestUser = NULL;
        gZrwCol.bestSub = sector;
        break;
    }
    gZrwCol.nearest = dt_float_edgy(0.0f, 1.0f);
    dt_fill(out, sizeof *out);
    for (int i = 0; i < 4; i++) {
        excluded[i] = dt_range(0, 6);
    }
    DT_ARG_PTR(c, 0, from);
    DT_ARG_PTR(c, 1, to);
    DT_ARG_INT(c, 2, dt_u32() & 0x3f);
    DT_ARG_PTR(c, 3, excluded);
    DT_ARG_PTR(c, 4, out);
}

/* A corner coordinate: random, or (to hit the degenerate cases) one shared with another corner. */
static float color_coordinate(const float *other)
{
    return (dt_u32() % 6 == 0) ? *other : dt_float_edgy(-200.0f, 200.0f);
}

/* The colour at a line hit: the nearest triangle of a sector with prelit colours, or no sector. */
static void setup_hit_color(DtCtx *c)
{
    ZrwColLineHit *hit = dt_alloc(sizeof *hit);
    RpWorldSector *sector = dt_alloc(sizeof *sector);
    uint8_t *map = dt_alloc(32);
    uint8_t *prelit = dt_alloc(64);
    ZrwColTriangle *best;

    begin(c);
    random_state(2, 0);
    best = random_triangle();
    best->index = dt_range(0, 7);
    __builtin_memcpy(&gZrwCol.best, best, sizeof gZrwCol.best);
    dt_fill(sector, sizeof *sector);
    dt_fill(map, 32);
    dt_fill(prelit, 64);
    for (int i = 0; i < 32; i++) {
        if (i % 4 != 0) {
            map[i] = (uint8_t)(map[i] % 16);
        }
    }
    sector->polygons = (RpPolygon *)map;
    sector->preLitLum = (uint32_t *)prelit;
    gZrwCol.bestSub = (dt_u32() % 8 == 0) ? NULL : sector;
    dt_fill(hit, sizeof *hit);
    for (int i = 0; i < 3; i++) {
        float *v = hit->hit.vertex[i];

        v[0] = color_coordinate(&hit->hit.vertex[(i + 2) % 3][0]);
        v[1] = dt_float_edgy(-200.0f, 200.0f);
        v[2] = color_coordinate(&hit->hit.vertex[(i + 2) % 3][2]);
    }
    for (int i = 0; i < 3; i++) {
        /* inside the triangle's box mostly, sometimes on a corner */
        const float *corner = hit->hit.vertex[dt_range(0, 2)];

        hit->point[i] = (dt_u32() % 5 == 0) ? corner[i] : dt_float(-200.0f, 200.0f);
    }
    DT_ARG_PTR(c, 0, hit);
}

static int BMX_CDECL stubTagInfo(void)
{
    ngLog(0x48d6f0);
    return (int)(dt_u32() & 1);
}

static void setup_enable(DtCtx *c)
{
    ngBegin(c, 0);
    gZrwColEnabled = dt_u32();
    dt_stub(0x0048d6f0, NULL, (void *)stubTagInfo);
}

/* The line and sphere tests go through the pointers 0x562068 / 0x56206c to the stand-ins below. */
static int BMX_CDECL stubLine(uint32_t a, uint32_t b, uint32_t d, uint32_t e, uint32_t f)
{
    ngLog(0x493e20);
    ngLog(a);
    ngLog(b);
    ngLog(d);
    ngLog(e);
    ngLog(f);
    return (int)dt_u32();
}

static int BMX_CDECL stubSphere(uint32_t a, uint32_t b, uint32_t d, uint32_t e, uint32_t f)
{
    ngLog(0x494370);
    ngLog(a);
    ngLog(b);
    ngLog(d);
    ngLog(e);
    ngLog(f);
    return (int)(dt_u32() & 1);
}

static void setup_line_test(DtCtx *c)
{
    ngBegin(c, 0);
    dt_stub(0x00493e20, NULL, (void *)stubLine);
    for (int i = 0; i < 5; i++) {
        DT_ARG_INT(c, i, dt_u32());
    }
}

static void setup_sphere_test(DtCtx *c)
{
    ngBegin(c, 0);
    dt_stub(0x00494370, NULL, (void *)stubSphere);
    for (int i = 0; i < 5; i++) {
        DT_ARG_INT(c, i, dt_u32());
    }
}

DT_CASES(zrwcol_cases) = {
    {"zrwColSystemInitialize", 0x00493d80, zrwColSystemInitialize, 0, DT_RET_INT, setup_system_init, 50, 0},
    {"zrwColSystemShutdown", 0x00493de0, zrwColSystemShutdown, 0, DT_RET_INT, setup_system_exit, 50, 0},
    {"zrwcolKeepNearestSub", 0x00494240, zrwcolKeepNearestSub, 5, DT_RET_INT, setup_keep_sub, 500, 0},
    {"zrwcolFilterNearest", 0x004942a0, zrwcolFilterNearest, 5, DT_RET_INT, setup_filter, 500, 0},
    {"zrwcolKeepNearest", 0x00494310, zrwcolKeepNearest, 4, DT_RET_INT, setup_keep, 500, 0},
    {"zrwcolQuerySphere", 0x00494370, zrwcolQuerySphere, 5, DT_RET_INT, setup_sphere_query, 1000, 0},
    {"zrwcolRecordHit(world)", 0x00494490, zrwcolRecordHit, 5, DT_RET_INT, setup_record_world, 1000, 0},
    {"zrwcolRecordHit(object)", 0x00494490, zrwcolRecordHit, 5, DT_RET_INT, setup_record_object, 1000, 0},
    {"zrwcolFilterRecord", 0x00494750, zrwcolFilterRecord, 5, DT_RET_INT, setup_filter, 500, 0},
    {"zrwcolRecordObjectHit", 0x004947c0, zrwcolRecordObjectHit, 4, DT_RET_INT, setup_record_object_short,
     500, 0},
    {"zrwcolQueryLine", 0x00493e20, zrwcolQueryLine, 5, DT_RET_INT, setup_line_query, 2000, 0},
    {"zrwcolHitColor", 0x004947e0, zrwcolHitColor, 1, DT_RET_INT, setup_hit_color, 3000, 0},
    {"zrwcolEnableIfTagInfo", 0x00494c20, zrwcolEnableIfTagInfo, 0, DT_RET_INT, setup_enable, 50, 0},
    {"zrwcolLineTest", 0x00494c70, zrwcolLineTest, 5, DT_RET_INT, setup_line_test, 100, 0},
    {"zrwcolSphereTest", 0x00494ca0, zrwcolSphereTest, 5, DT_RET_INT, setup_sphere_test, 100, 0},
};
