/*
 * Tests for zrwModelRender.c (model rendering and the collision tree debug drawing). RenderWare and the
 * debug drawing hooks are stubbed; the stubs log their arguments (and the contents of the vertex and index
 * buffers handed to the immediate mode functions) and answer from fixtures. Frames and matrices are the same
 * memory in the fixtures (RwFrameGetLTM returns its argument). Assert paths end the harness run, which
 * compares the abort.
 */
#include <stddef.h>

#include "dt.h"
#include "dt_ngif.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwmodelrender.h"

#define gRenderFlagsT ORIG_VAR(0x00697778, uint32_t, gRenderFlagsT)
#define gRenderExtraT ORIG_VAR(0x0069777c, int, gRenderExtraT)
#define gCameraT      ORIG_VAR(0x006976f4, void *, gCameraT)
#define gLineHookT    ORIG_VAR(0x00560064, void *, gLineHookT)
#define gTriHookT     ORIG_VAR(0x00560068, void *, gTriHookT)
#define gSphereHookT  ORIG_VAR(0x0056006c, void *, gSphereHookT)
#define gRenderHookT  ORIG_VAR(0x00561490, void *, gRenderHookT)
#define gBeginHookT   ORIG_VAR(0x00561494, void *, gBeginHookT)
#define gEndHookT     ORIG_VAR(0x00561498, void *, gEndHookT)
#define gRegisterT    ORIG_VAR(0x00561470, void *, gRegisterT)
#define gUnregisterT  ORIG_VAR(0x00561474, void *, gUnregisterT)
#define gPluginOffset ORIG_VAR(0x00560e70, int, gPluginOffset)

typedef int(BMX_CDECL *InitFn)(void);

/* ---- fixtures and answers ------------------------------------------------------------------------- */

static ZMatrix *gCameraFrame;
static int gNumVerts, gNumTris;
static float *gVertices;
static int16_t *gTriangles;
static uint32_t *gTexCoords;
static int gTransformOk;
static float gFarPlane;

static void logColor(const uint32_t *color)
{
    ngLogPtr(color);
    ngLog(color != NULL ? *color : 0);
}

static void BMX_CDECL stubLine(const ZVec3 *a, const ZVec3 *b, const uint32_t *color, int flags)
{
    ngLog(0x47ff90);
    ngLogVec(a);
    ngLogVec(b);
    logColor(color);
    ngLog((uint32_t)flags);
}

static void BMX_CDECL stubTriangle(const ZVec3 *a, const ZVec3 *b, const ZVec3 *c, const uint32_t *color,
                                   int flags)
{
    ngLog(0x480070);
    ngLogVec(a);
    ngLogVec(b);
    ngLogVec(c);
    logColor(color);
    ngLog((uint32_t)flags);
}

static void BMX_CDECL stubSphere(const ZVec3 *center, float radius, const uint32_t *color, int flags)
{
    ngLog(0x480100);
    ngLogVec(center);
    ngLogFloat(radius);
    logColor(color);
    ngLog((uint32_t)flags);
}

static void BMX_CDECL stubDrawHook(void *object)
{
    ngLog(0xb4);
    ngLogPtr(object);
}

static void BMX_CDECL stubRenderHook(uint32_t flags, const void *raster)
{
    ngLog(0x561490);
    ngLog(flags);
    ngLogPtr(raster);
}

static void BMX_CDECL stubBeginHook(void)
{
    ngLog(0x561494);
}

static void BMX_CDECL stubEndHook(void)
{
    ngLog(0x561498);
}

static void *BMX_CDECL stubRegister(int type, void *fn, void *arg, int append)
{
    ngLog(0x561470);
    ngLog((uint32_t)type);
    ngLogPtr(fn);
    ngLogPtr(arg);
    ngLog((uint32_t)append);
    return (void *)(uintptr_t)0x6100;
}

static void BMX_CDECL stubUnregister(void *node)
{
    ngLog(0x561474);
    ngLogPtr(node);
}

static void *BMX_CDECL stubCameraFrame(void *camera)
{
    ngLog(0x4dea80);
    ngLogPtr(camera);
    return gCameraFrame;
}

static ZMatrix *BMX_CDECL stubFrameLtm(void *frame)
{
    ngLog(0x4d2b10);
    ngLogPtr(frame);
    return frame;
}

static ZMatrix *BMX_CDECL stubFrameMatrix(void *frame)
{
    ngLog(0x4d2b00);
    ngLogPtr(frame);
    return frame;
}

static void *BMX_CDECL stubTranslate(ZMatrix *m, const ZVec3 *t, int combine)
{
    ngLog(0x4d15a0);
    ngLogPtr(m);
    ngLogVec(t);
    ngLog((uint32_t)combine);
    m->pos = *t;
    return m;
}

static void *BMX_CDECL stubRotate(ZMatrix *m, const ZVec3 *axis, float angle, int combine)
{
    ngLog(0x4d1040);
    ngLogPtr(m);
    ngLogVec(axis);
    ngLogFloat(angle);
    ngLog((uint32_t)combine);
    m->up.x = angle;
    return m;
}

static void *BMX_CDECL stubUpdateObjects(void *frame)
{
    ngLog(0x4d2a80);
    ngLogPtr(frame);
    return frame;
}

static void *BMX_CDECL stubForAllChildren(void *frame, void *cb, void *data)
{
    ngLog(0x4d2eb0);
    ngLogPtr(frame);
    ngLogPtr(cb);
    ngLogVec(data);
    return frame;
}

static void *BMX_CDECL stubForAllAtomics(void *clump, void *cb, void *data)
{
    ngLog(0x4b1990);
    ngLogPtr(clump);
    ngLogPtr(cb);
    ngLogPtr(data);
    return clump;
}

static void *BMX_CDECL stubClumpRender(void *clump)
{
    ngLog(0x4b1940);
    ngLogPtr(clump);
    return clump;
}

static void BMX_CDECL stubPostUpdate(void *clump)
{
    ngLog(0x504490);
    ngLogPtr(clump);
}

static void BMX_CDECL stubSetColor(void *model, const void *color)
{
    ngLog(0x488cc0);
    ngLogPtr(model);
    ngLogPtr(color);
    ngLog(*(const uint32_t *)color);
}

static int BMX_CDECL stubNumVertices(void *geometry)
{
    ngLog(0x4b4550);
    ngLogPtr(geometry);
    return gNumVerts;
}

static void *BMX_CDECL stubMorphTarget(void *geometry, int index)
{
    ngLog(0x4b43e0);
    ngLogPtr(geometry);
    ngLog((uint32_t)index);
    return (void *)(uintptr_t)0x8800;
}

static const float *BMX_CDECL stubMorphVertices(void *morph)
{
    ngLog(0x4b45d0);
    ngLogPtr(morph);
    return gVertices;
}

static const int16_t *BMX_CDECL stubTriangles(void *geometry)
{
    ngLog(0x4b46b0);
    ngLogPtr(geometry);
    return gTriangles;
}

static const uint32_t *BMX_CDECL stubTexCoords(void *geometry)
{
    ngLog(0x4b44e0);
    ngLogPtr(geometry);
    return gTexCoords;
}

static int BMX_CDECL stubNumTriangles(void *geometry)
{
    ngLog(0x4b4720);
    ngLogPtr(geometry);
    return gNumTris;
}

static void *BMX_CDECL stubGetMaterial(void *geometry, int index)
{
    ngLog(0x4b47a0);
    ngLogPtr(geometry);
    ngLog((uint32_t)index);
    return (void *)(uintptr_t)(0x9000 + (uint32_t)index * 0x10);
}

static void *BMX_CDECL stubMaterialTexture(void *material)
{
    ngLog(0x4b93d0);
    ngLogPtr(material);
    return (void *)((uintptr_t)material + 0x100);
}

static void *BMX_CDECL stubTextureRaster(void *texture)
{
    ngLog(0x4df650);
    ngLogPtr(texture);
    return (void *)((uintptr_t)texture + 0x1000);
}

static void *BMX_CDECL stubCameraRaster(void *camera)
{
    ngLog(0x4de5a0);
    ngLogPtr(camera);
    return (void *)(uintptr_t)0x7a00;
}

static int BMX_CDECL stubRasterSize(void *raster)
{
    ngLog(0x4dcc30);
    ngLogPtr(raster);
    return 640;
}

static float BMX_CDECL stubFarGet(void *camera)
{
    ngLog(0x4de650);
    ngLogPtr(camera);
    return gFarPlane;
}

static void *BMX_CDECL stubFarSet(void *camera, float distance)
{
    ngLog(0x4de620);
    ngLogPtr(camera);
    ngLogFloat(distance);
    return camera;
}

static float BMX_CDECL stubNearGet(void *camera)
{
    ngLog(0x4de610);
    ngLogPtr(camera);
    return 0.5f;
}

static int BMX_CDECL stubRenderState(int state, int value)
{
    ngLog(0x4db820);
    ngLog((uint32_t)state);
    ngLog((uint32_t)value);
    return 1;
}

static int BMX_CDECL stubTransform(void *vertices, int count, const ZMatrix *matrix, int flags)
{
    const uint8_t *v = vertices;

    ngLog(0x4df0b0);
    ngLog((uint32_t)count);
    ngLogPtr(matrix);
    if (matrix != NULL) {
        ngLogVec(&matrix->pos);
    }
    ngLog((uint32_t)flags);
    for (int i = 0; i < count; i++) {
        ngLog(*(const uint32_t *)(v + i * 0x24));
        ngLog(*(const uint32_t *)(v + i * 0x24 + 4));
        ngLog(*(const uint32_t *)(v + i * 0x24 + 8));
        ngLog(*(const uint32_t *)(v + i * 0x24 + 0x18));
        if (count > 2) {
            ngLog(*(const uint32_t *)(v + i * 0x24 + 0x1c));
            ngLog(*(const uint32_t *)(v + i * 0x24 + 0x20));
        }
    }
    return gTransformOk;
}

static int BMX_CDECL stubTransformEnd(void)
{
    ngLog(0x4df090);
    return 1;
}

static int BMX_CDECL stubRenderIndexed(int primitive, const uint16_t *indices, int count)
{
    ngLog(0x4df020);
    ngLog((uint32_t)primitive);
    ngLog((uint32_t)count);
    for (int i = 0; i < count; i++) {
        ngLog(indices[i]);
    }
    return 1;
}

static int BMX_CDECL stubRenderLine(int from, int to)
{
    ngLog(0x4defc0);
    ngLog((uint32_t)from);
    ngLog((uint32_t)to);
    return 1;
}

static void BMX_CDECL stubColSearch(int mode, void *root, void *result, const void *a, const void *b)
{
    ngLog(0x48ee70);
    ngLog((uint32_t)mode);
    ngLogPtr(root);
    ngLogPtr(result);
    ngLogPtr(a);
    ngLogPtr(b);
    *(uint16_t *)result = (uint16_t)(dt_u32() % 3 == 0 ? 0 : dt_u32() | 1);
}

/* Set by the search case itself, which runs the real search. */
static int gRealColSearch;

static void stub_all(void)
{
    gLineHookT = (void *)stubLine;
    gTriHookT = (void *)stubTriangle;
    gSphereHookT = (void *)stubSphere;
    gRenderHookT = (void *)stubRenderHook;
    gBeginHookT = (void *)stubBeginHook;
    gEndHookT = (void *)stubEndHook;
    gRegisterT = (void *)stubRegister;
    gUnregisterT = (void *)stubUnregister;
    dt_stub(0x004dea80, NULL, (void *)stubCameraFrame);
    dt_stub(0x004d2b10, NULL, (void *)stubFrameLtm);
    dt_stub(0x004d2b00, NULL, (void *)stubFrameMatrix);
    dt_stub(0x004d15a0, NULL, (void *)stubTranslate);
    dt_stub(0x004d1040, NULL, (void *)stubRotate);
    dt_stub(0x004d2a80, NULL, (void *)stubUpdateObjects);
    dt_stub(0x004d2eb0, NULL, (void *)stubForAllChildren);
    dt_stub(0x004b1990, NULL, (void *)stubForAllAtomics);
    dt_stub(0x004b1940, NULL, (void *)stubClumpRender);
    dt_stub(0x00504490, NULL, (void *)stubPostUpdate);
    dt_stub(0x00488cc0, NULL, (void *)stubSetColor);
    dt_stub(0x004b4550, NULL, (void *)stubNumVertices);
    dt_stub(0x004b43e0, NULL, (void *)stubMorphTarget);
    dt_stub(0x004b45d0, NULL, (void *)stubMorphVertices);
    dt_stub(0x004b46b0, NULL, (void *)stubTriangles);
    dt_stub(0x004b44e0, NULL, (void *)stubTexCoords);
    dt_stub(0x004b4720, NULL, (void *)stubNumTriangles);
    dt_stub(0x004b47a0, NULL, (void *)stubGetMaterial);
    dt_stub(0x004b93d0, NULL, (void *)stubMaterialTexture);
    dt_stub(0x004df650, NULL, (void *)stubTextureRaster);
    dt_stub(0x004de5a0, NULL, (void *)stubCameraRaster);
    dt_stub(0x004dcc30, NULL, (void *)stubRasterSize);
    dt_stub(0x004dcc40, NULL, (void *)stubRasterSize);
    dt_stub(0x004de650, NULL, (void *)stubFarGet);
    dt_stub(0x004de620, NULL, (void *)stubFarSet);
    dt_stub(0x004de610, NULL, (void *)stubNearGet);
    dt_stub(0x004db820, NULL, (void *)stubRenderState);
    dt_stub(0x004df0b0, NULL, (void *)stubTransform);
    dt_stub(0x004df090, NULL, (void *)stubTransformEnd);
    dt_stub(0x004df020, NULL, (void *)stubRenderIndexed);
    dt_stub(0x004defc0, NULL, (void *)stubRenderLine);
    if (!gRealColSearch) {
        dt_stub(0x0048ee70, NULL, (void *)stubColSearch);
    }
}

static float rf(void)
{
    return dt_float_edgy(-100.0f, 100.0f);
}

static void begin(const DtCtx *c)
{
    ngBegin(c, 0);
    dt_zmem_env();
    stub_all();
    gRenderFlagsT = dt_u32();
    gRenderExtraT = dt_u32() % 3 == 0 ? (int)(0x9100 + 0x1100 + 0x10 * (dt_u32() % 3)) : (int)dt_u32();
    gCameraT = (void *)(uintptr_t)0x7000;
    gFarPlane = dt_float_edgy(1.0f, 5000.0f);
    gTransformOk = dt_u32() % 5 != 0;
}

static ZMatrix *make_frame(void)
{
    ZMatrix *m = dt_alloc(0x80);
    float *f = (float *)m;

    for (int i = 0; i < 16; i++) {
        f[i] = rf();
    }
    m->flags = dt_u32() & 0xffff;
    return m;
}

static ZVec3 *make_vec(void)
{
    ZVec3 *v = dt_alloc(sizeof *v);

    v->x = rf();
    v->y = rf();
    v->z = rf();
    return v;
}

static uint32_t *make_color(void)
{
    uint32_t *c = dt_alloc(4);

    *c = dt_u32();
    return c;
}

/* ---- debug drawing -------------------------------------------------------------------------------- */

static ZrwRenderColTriangle *make_triangle(void)
{
    ZrwRenderColTriangle *t = dt_alloc(0x100);

    for (int i = 0; i < 16; i++) {
        ((float *)t)[i] = rf();
    }
    t->tag = (const uint16_t *)(uintptr_t)(dt_u32() % 4 != 0 ? dt_u32() | 1 : 0);
    for (int i = 0; i < 3; i++) {
        t->edgeTag[i] = (const ZbspEdgeTag *)(uintptr_t)(dt_u32() % 3 == 0);
        t->cornerTag[i] = (const void *)(uintptr_t)(dt_u32() % 3 == 0);
    }
    return t;
}

static void setup_node_box(DtCtx *c)
{
    ZrwRenderColNode *node = dt_alloc(sizeof *node);

    begin(c);
    for (uint32_t i = 0; i < 0x3c; i += 4) {
        ((float *)node)[i / 4] = rf();
    }
    DT_ARG_PTR(c, 0, node);
    DT_ARG_PTR(c, 1, dt_u32() % 4 == 0 ? NULL : make_color());
}

static void setup_draw_triangle(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_triangle());
    DT_ARG_PTR(c, 1, dt_u32() % 3 == 0 ? NULL : make_color());
    DT_ARG_PTR(c, 2, dt_u32() % 3 == 0 ? NULL : make_color());
    DT_ARG_PTR(c, 3, dt_u32() % 3 == 0 ? NULL : make_color());
}

static void setup_triangle_edge(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_triangle());
    DT_ARG_INT(c, 1, dt_u32() % 4 == 0 ? dt_u32() : (uint32_t)dt_range(0, 2) | (dt_u32() & 0xffffff00u));
    DT_ARG_PTR(c, 2, dt_u32() % 4 == 0 ? NULL : make_color());
}

/* ---- the collision tree --------------------------------------------------------------------------- */

static ZrwRenderColNode *make_node(int depth, const ZVec3 *p)
{
    ZrwRenderColNode *node = dt_alloc(sizeof *node);

    node->boxMin.x = rf();
    node->boxMin.y = rf();
    node->boxMin.z = rf();
    node->boxMax.x = rf();
    node->boxMax.y = rf();
    node->boxMax.z = rf();
    if (depth == 0 || dt_u32() % 3 == 0) {
        node->type = 1;
        return node;
    }
    node->type = (uint8_t[]){2, 4, 8}[dt_u32() % 3];
    node->split = node->type == 2 ? p->x : node->type == 4 ? p->y : p->z;
    switch (dt_u32() % 4) {
    case 0:
        node->split += 3.0f;
        break;
    case 1:
        node->split -= 3.0f;
        break;
    default:
        break; /* exactly on the point */
    }
    node->below = make_node(depth - 1, p);
    node->above = dt_u32() % 12 == 0 ? NULL : make_node(depth - 1, p);
    return node;
}

/* Gives every leaf triangles around p: planar, with the normal the in-triangle test expects (made by the
 * original code). */
static void fill_leaves(ZrwRenderColNode *node, const ZVec3 *p, int withTriangles)
{
    typedef void(BMX_CDECL * Cross3Fn)(ZVec3 *, const ZVec3 *, const ZVec3 *, const ZVec3 *);
    typedef double(BMX_CDECL * NormalizeFn)(ZVec3 *, const ZVec3 *);

    if (node == NULL) {
        return;
    }
    if (node->type != 1) {
        fill_leaves(node->below, p, withTriangles);
        fill_leaves(node->above, p, withTriangles);
        return;
    }
    node->triangleCount = (uint16_t)(withTriangles ? dt_range(0, 3) : 0);
    node->triangles = (ZrwRenderColTriangle **)dt_alloc(16);
    for (int i = 0; i < node->triangleCount; i++) {
        ZrwRenderColTriangle *t = dt_alloc(sizeof *t + 0x10);
        const float h = p->z - dt_float_edgy(0.0f, 5.0f);
        const float a = dt_float_edgy(1.0f, 10.0f);
        const float b = dt_float_edgy(1.0f, 10.0f);
        ZVec3 centroid;

        t->v0 = (ZVec3){p->x - a, p->y - b, h};
        t->v1 = (ZVec3){p->x + 2.0f * a, p->y - b, h};
        t->v2 = (ZVec3){p->x - a, p->y + 2.0f * b, h};
        centroid = (ZVec3){p->x, p->y, h};
        DT_ORIG(0x004815b0, Cross3Fn)(&t->normal, &t->v0, &t->v1, &centroid);
        DT_ORIG(0x004811c0, NormalizeFn)(&t->normal, &t->normal);
        t->tag = (const uint16_t *)(uintptr_t)(dt_u32() % 6 == 0 ? 0 : dt_u32() | 1);
        for (int k = 0; k < 3; k++) {
            t->edgeTag[k] = (const ZbspEdgeTag *)(uintptr_t)(dt_u32() % 3 == 0);
            t->cornerTag[k] = (const void *)(uintptr_t)(dt_u32() % 3 == 0);
        }
        if (dt_u32() % 8 == 0) {
            t->normal.x = dt_float_edgy(-1.0f, 1.0f); /* a normal that does not match the plane */
        }
        node->triangles[i] = t;
    }
}

static void build_tree(const ZVec3 *p, int withTriangles)
{
    ZrwRenderColNode *root = make_node(3, p);

    fill_leaves(root, p, withTriangles);
    gZrwColRoot = *root;
    gZrwColDebug = (int)(dt_u32() & 1);
    gZrwColHit.valid = (uint8_t)dt_u32();
    gZrwColHit.tag = (const void *)(uintptr_t)dt_u32();
}

static ZVec3 *make_point(void)
{
    ZVec3 *p = make_vec();

    if (dt_u32() % 12 == 0) {
        p->x = __builtin_nanf("");
    }
    return p;
}

static void setup_find_node(DtCtx *c)
{
    ZVec3 *p = make_point();

    begin(c);
    build_tree(p, 0);
    DT_ARG_PTR(c, 0, p);
}

static void setup_find_triangle(DtCtx *c)
{
    ZVec3 *p = make_vec();

    begin(c);
    build_tree(p, 1);
    p->z += dt_float_edgy(0.0f, 8.0f);
    DT_ARG_PTR(c, 0, p);
    DT_ARG_FLOAT(c, 1, dt_float_edgy(0.0f, 9.0f));
}

static void setup_point_in_triangle(DtCtx *c)
{
    typedef void(BMX_CDECL * Cross3Fn)(ZVec3 *, const ZVec3 *, const ZVec3 *, const ZVec3 *);
    typedef double(BMX_CDECL * NormalizeFn)(ZVec3 *, const ZVec3 *);
    ZVec3 *v = dt_alloc(5 * sizeof(ZVec3));
    const float h = rf();

    begin(c);
    v[0] = (ZVec3){dt_float_edgy(-10, 0), dt_float_edgy(-10, 0), h};
    v[1] = (ZVec3){dt_float_edgy(5, 10), dt_float_edgy(-10, 0), h};
    v[2] = (ZVec3){dt_float_edgy(-10, 0), dt_float_edgy(5, 10), h};
    v[4] = (ZVec3){dt_float_edgy(-12, 12), dt_float_edgy(-12, 12), dt_u32() % 4 == 0 ? rf() : h};
    DT_ORIG(0x004815b0, Cross3Fn)(&v[3], &v[0], &v[1], &v[4]);
    DT_ORIG(0x004811c0, NormalizeFn)(&v[3], &v[3]);
    if (dt_u32() % 6 == 0) {
        v[3].z = -v[3].z;
    }
    if (dt_u32() % 8 == 0) {
        v[3].x += dt_float_edgy(-0.02f, 0.02f);
    }
    for (int i = 0; i < 5; i++) {
        DT_ARG_PTR(c, i, &v[i]);
    }
}

static void setup_query(DtCtx *c)
{
    begin(c);
    gZrwColQuery.found = (uint16_t)dt_u32();
    DT_ARG_PTR(c, 0, make_vec());
    DT_ARG_PTR(c, 1, make_vec());
}

/* ---- the tree search ------------------------------------------------------------------------------ */

typedef uint8_t ColListBytes[0xe00];
#define gColListT ORIG_VAR(0x00672908, ColListBytes, gColListT)

/*
 * A coordinate for mode 1 searches: the search measures the corner at byte offset (bits << 4) of the
 * coordinate the last split looked at, so the low 28 bits are 0, 1 or 2 (corner v0, v1, v2) and the top
 * nibble picks about +-2 or +-5e-10.
 */
static float search_coordinate(void)
{
    static const uint32_t kTop[4] = {0x3, 0x4, 0xb, 0xc};
    const uint32_t bits = (kTop[dt_u32() % 4] << 28) | (uint32_t)dt_range(0, 2);
    float f;

    __builtin_memcpy(&f, &bits, 4);
    return f;
}

static float search_offset(float radius)
{
    switch (dt_u32() % 4) {
    case 0:
        return 0.0f;
    case 1:
        return dt_float(-radius, radius);
    default:
        return dt_float_edgy(-3.0f * radius - 1.0f, 3.0f * radius + 1.0f);
    }
}

static ZrwRenderColTriangle *search_triangle(const ZVec3 *p, float radius)
{
    ZrwRenderColTriangle *t = dt_alloc(sizeof *t + 0x40);

    dt_fill(t, sizeof *t + 0x40);
    for (int k = 0; k < 3; k++) {
        ZVec3 *v = (ZVec3 *)((uint8_t *)t + 0x10 * k);

        v->x = p->x + search_offset(radius);
        v->y = p->y + search_offset(radius);
        v->z = p->z + search_offset(radius);
        t->edgeTag[k] = (const ZbspEdgeTag *)(uintptr_t)((dt_u32() % 3 == 0) ? 0 : dt_u32());
        t->cornerTag[k] = (const void *)(uintptr_t)((dt_u32() % 3 == 0) ? 0 : dt_u32());
    }
    t->tag = (const uint16_t *)(uintptr_t)((dt_u32() % 4 == 0) ? 0 : dt_u32());
    return t;
}

/*
 * A subtree around p: leaves (type 1) with 0..4 triangles, splits on x / y / z (types 2, 4, 8) near the
 * point, and now and then a node of another type (which keeps the last split coordinate). `split` forces a
 * split of type 2, 4 or 8.
 */
static ZrwRenderColNode *search_node(int depth, const ZVec3 *p, float radius, int split)
{
    ZrwRenderColNode *node = dt_alloc(sizeof *node);

    dt_fill(node, sizeof *node);
    if (!split && (depth == 0 || dt_u32() % 3 == 0)) {
        node->type = 1;
        node->triangleCount = (uint16_t)dt_range(0, 4);
        node->triangles = (ZrwRenderColTriangle **)dt_alloc(4 * 4);
        for (int i = 0; i < node->triangleCount; i++) {
            node->triangles[i] = search_triangle(p, radius);
        }
        return node;
    }
    switch (dt_u32() % (split ? 3 : 4)) {
    case 0:
        node->type = 2;
        node->split = p->x + search_offset(radius);
        break;
    case 1:
        node->type = 4;
        node->split = p->y + search_offset(radius);
        break;
    case 2:
        node->type = 8;
        node->split = p->z + search_offset(radius);
        break;
    default:
        node->type = (uint8_t)((dt_u32() & 1) ? 0 : 0x10);
        node->split = dt_float_edgy(-10.0f, 10.0f);
        break;
    }
    node->below = search_node(depth - 1, p, radius, 0);
    node->above = search_node(depth - 1, p, radius, 0);
    return node;
}

static void setup_col_search(DtCtx *c)
{
    static const int kModes[8] = {1, 2, 3, 1, 2, 3, 0, 4};
    ZVec3 *p = dt_alloc(sizeof *p);
    uint16_t *found = dt_alloc(4);
    int mode;
    float radius;

    gRealColSearch = 1;
    begin(c);
    mode = kModes[dt_u32() % 8];
    if (mode == 1) {
        p->x = search_coordinate();
        p->y = search_coordinate();
        p->z = search_coordinate();
    } else {
        p->x = rf();
        p->y = rf();
        p->z = (dt_u32() % 16 == 0) ? __builtin_nanf("") : rf();
    }
    radius = dt_float_edgy(0.0f, 8.0f);
    if (dt_u32() % 16 == 0) {
        radius = (dt_u32() & 1) ? -1.0f : __builtin_nanf("");
    }
    *found = (uint16_t)((dt_u32() & 1) ? dt_range(0, 3) : dt_range(0x3c, 0x40));
    dt_fill(&gColListT, sizeof gColListT);
    gZrwColDebug = (int)(dt_u32() & 1);
    DT_ARG_INT(c, 0, mode);
    /* mode 1 measures with the bits of a split coordinate, so its root is a split */
    DT_ARG_PTR(c, 1, search_node(3, p, radius, mode == 1 || dt_u32() % 2 == 0));
    DT_ARG_PTR(c, 2, found);
    DT_ARG_PTR(c, 3, p);
    DT_ARG_FLOAT(c, 4, radius);
}

/* ---- models --------------------------------------------------------------------------------------- */

/* A model with a definition (flags at +8, draw hook at +0xb4) and a clump (a frame at +4, plugin data at
 * +0x40 whose slot points back at the model). */
static ZrwModel *make_model(void)
{
    ZrwModel *model = dt_alloc(0x100);
    uint8_t *def = dt_alloc(0x100);
    uint8_t *clump = dt_alloc(0x100);
    uint8_t *plugin = dt_alloc(0x40);

    *(uint32_t *)(def + 8) = dt_u32() % 6 == 0 ? 0 : (dt_u32() & 3) | 1;
    *(uint32_t *)(def + 0xb4) = (uint32_t)(uintptr_t)stubDrawHook;
    model->def = (ZrwModelDef *)def;
    model->clump = (void *)clump;
    *(void **)(clump + 4) = make_frame();
    model->color = dt_u32();
    gPluginOffset = 0x10;
    *(void **)(clump + 0x40) = plugin;
    *(void **)(plugin + gPluginOffset) = model;
    return model;
}

static void setup_billboard(DtCtx *c)
{
    typedef int(BMX_CDECL * InitTablesFn)(void);
    ZrwModel *model;
    uint8_t *object;

    begin(c);
    DT_ORIG(0x00480d20, InitTablesFn)();
    model = make_model();
    object = (uint8_t *)model->clump;
    gCameraFrame = make_frame();
    gCameraFrame->at.x = dt_float_edgy(-1.0f, 1.0f);
    gCameraFrame->at.z = dt_float_edgy(-1.0f, 1.0f);
    DT_ARG_PTR(c, 0, object);
}

static void setup_render_callback(DtCtx *c)
{
    ZrwModel *model;

    begin(c);
    model = make_model();
    DT_ARG_PTR(c, 0, model->clump);
}

/* ---- the atomic renderer -------------------------------------------------------------------------- */

static void setup_atomic(DtCtx *c)
{
    uint8_t *atomic = dt_alloc(0x80);
    int materials = dt_range(1, 3);

    begin(c);
    DT_ORIG(0x0048fc50, InitFn)();
    gZrwModelRender.vertexCapacity = dt_range(0, 24);
    gZrwModelRender.indexCapacity = dt_range(0, 14);
    gCameraFrame = make_frame();
    *(void **)(atomic + 4) = make_frame();
    *(void **)(atomic + 0x1c) = dt_u32() % 12 == 0 ? NULL : (void *)(uintptr_t)0x8000;
    gNumVerts = dt_range(1, 20);
    gNumTris = dt_range(0, 12);
    gVertices = dt_alloc((uint32_t)gNumVerts * 12 + 16);
    gTexCoords = dt_alloc((uint32_t)gNumVerts * 8 + 16);
    gTriangles = dt_alloc((uint32_t)gNumTris * 8 + 16);
    for (int i = 0; i < gNumVerts * 3; i++) {
        gVertices[i] = rf();
    }
    for (int i = 0; i < gNumVerts * 2; i++) {
        gTexCoords[i] = dt_u32();
    }
    for (int i = 0; i < gNumTris; i++) {
        gTriangles[i * 4] = (int16_t)dt_range(0, gNumVerts - 1);
        gTriangles[i * 4 + 1] = (int16_t)dt_range(0, gNumVerts - 1);
        gTriangles[i * 4 + 2] = (int16_t)dt_range(0, gNumVerts - 1);
        gTriangles[i * 4 + 3] =
            (int16_t)(i > 0 && dt_u32() % 2 ? gTriangles[i * 4 - 1] : dt_range(0, materials - 1));
    }
    DT_ARG_PTR(c, 0, atomic);
}

static void setup_frame_lines(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_frame());
    DT_ARG_PTR(c, 1, make_vec());
}

static void setup_draw_frames(DtCtx *c)
{
    ZrwModel *model = dt_alloc(0x100);

    begin(c);
    if (dt_u32() % 5 != 0) {
        uint8_t *clump = dt_alloc(0x40);

        *(void **)(clump + 4) = dt_u32() % 5 == 0 ? NULL : make_frame();
        model->clump = (void *)clump;
    }
    DT_ARG_PTR(c, 0, model);
}

static void setup_model_render(DtCtx *c)
{
    ZrwModel *model;

    begin(c);
    model = make_model();
    gCameraFrame = make_frame();
    DT_ARG_PTR(c, 0, model);
}

static void setup_render_init(DtCtx *c)
{
    begin(c);
    gZrwModelRender.initialized = dt_u32() % 6 == 0;
}

static void setup_render_shutdown(DtCtx *c)
{
    begin(c);
    DT_ORIG(0x0048fc50, InitFn)();
    gZrwModelRender.initialized = dt_u32() % 5 != 0;
    gZrwModelRender.updateNode = dt_u32() & 1 ? (void *)(uintptr_t)0x6100 : NULL;
    if (dt_u32() % 4 == 0) {
        gZrwModelRender.vertices = NULL;
    }
    if (dt_u32() % 4 == 0) {
        gZrwModelRender.buffer3 = NULL;
    }
    if (dt_u32() % 4 == 0) {
        gZrwModelRender.indices = NULL;
    }
}

static void setup_set_callback(DtCtx *c)
{
    ZrwModel *model = dt_alloc(0x100);

    begin(c);
    model->clump = dt_alloc(0x40);
    gZrwModelRender.atomicCallback = dt_u32() & 1 ? (void *)(uintptr_t)0x5100 : NULL;
    DT_ARG_PTR(c, 0, model);
    DT_ARG_PTR(c, 1, dt_u32() % 4 == 0 ? NULL : (void *)(uintptr_t)0x5200);
}

static void setup_atomic_callback(DtCtx *c)
{
    uint8_t *atomic = dt_alloc(0x80);

    begin(c);
    atomic[0x14] = (uint8_t)dt_u32();
    DT_ARG_PTR(c, 0, atomic);
    DT_ARG_PTR(c, 1, (void *)(uintptr_t)dt_u32());
}

static void setup_register(DtCtx *c)
{
    begin(c);
    gZrwModelRender.updateNode = dt_u32() & 1 ? (void *)(uintptr_t)0x6100 : NULL;
    DT_ARG_PTR(c, 0, (void *)(uintptr_t)dt_u32());
}

static void setup_none(DtCtx *c)
{
    begin(c);
    gZrwModelRender.updateNode = dt_u32() & 1 ? (void *)(uintptr_t)0x6100 : NULL;
}

DT_CASES(zrwmodelrender_cases) = {
    {"zrwModelDrawNodeBox", 0x0048def0, zrwModelDrawNodeBox, 2, DT_RET_VOID, setup_node_box, 100, 0},
    {"zrwModelDrawTriangle", 0x0048e0c0, zrwModelDrawTriangle, 4, DT_RET_VOID, setup_draw_triangle, 1500, 0},
    {"zrwModelDrawTriangleEdge", 0x0048e2b0, zrwModelDrawTriangleEdge, 3, DT_RET_VOID, setup_triangle_edge,
     800, 0},
    {"zrwModelFindNode", 0x0048e400, zrwModelFindNode, 1, DT_RET_INT, setup_find_node, 600, 0},
    {"zrwModelPointInTriangle", 0x0048e4b0, zrwModelPointInTriangle, 5, DT_RET_INT, setup_point_in_triangle,
     1500, 0},
    {"zrwModelFindTriangle", 0x0048e840, zrwModelFindTriangle, 2, DT_RET_INT, setup_find_triangle, 1500, 0},
    {"zrwModelQuery", 0x0048f320, zrwModelQuery, 2, DT_RET_INT, setup_query, 100, 0},
    {"zrwColSearch", 0x0048ee70, zrwColSearch, 5, DT_RET_VOID, setup_col_search, 2000, 0},
    {"zrwModelRenderBillboard", 0x0048f3e0, zrwModelRenderBillboard, 1, DT_RET_INT, setup_billboard, 200, 0},
    {"zrwModelRenderCallback", 0x0048f530, zrwModelRenderCallback, 1, DT_RET_INT, setup_render_callback, 100,
     0},
    {"zrwModelRenderAtomic", 0x0048f720, zrwModelRenderAtomic, 1, DT_RET_INT, setup_atomic, 800, 0},
    {"zrwFrameDrawLines", 0x0048fa90, zrwFrameDrawLines, 2, DT_RET_INT, setup_frame_lines, 100, 0},
    {"zrwModelDrawFrames", 0x0048fb70, zrwModelDrawFrames, 1, DT_RET_VOID, setup_draw_frames, 100, 0},
    {"zrwModelRender", 0x0048fbc0, zrwModelRender, 1, DT_RET_INT, setup_model_render, 200, 0},
    {"zrwModelRenderInitialize", 0x0048fc50, zrwModelRenderInitialize, 0, DT_RET_INT, setup_render_init, 100,
     0},
    {"zrwModelRenderShutdown", 0x0048fd00, zrwModelRenderShutdown, 0, DT_RET_INT, setup_render_shutdown, 200,
     0},
    {"zrwModelRenderDirect", 0x0048fda0, zrwModelRenderDirect, 1, DT_RET_VOID, setup_model_render, 200, 0},
    {"zrwAtomicSetRenderCallback", 0x0048fea0, zrwAtomicSetRenderCallback, 2, DT_RET_INT,
     setup_atomic_callback, 100, 0},
    {"zrwModelSetRenderCallback", 0x0048fec0, zrwModelSetRenderCallback, 2, DT_RET_VOID, setup_set_callback,
     100, 0},
    {"zrwModelSetDefaultRenderCallback", 0x0048fee0, zrwModelSetDefaultRenderCallback, 1, DT_RET_VOID,
     setup_set_callback, 100, 0},
    {"zrwModelRenderRegister", 0x0048ff00, zrwModelRenderRegister, 1, DT_RET_VOID, setup_register, 50, 0},
    {"zrwModelRenderUnregister", 0x0048ff30, zrwModelRenderUnregister, 0, DT_RET_VOID, setup_none, 50, 0},
};
