/*
 * Tests for the big NgIF collision queries: zbspFindEdgesNear, zbspFindFaceNear (NgIFzbsp.c),
 * zcollideFindEdgesBehind, zcollideRay and zcollideFillPropEdges (NgIFzcollide.c). The renderer's
 * collision queries are stubbed with prebuilt results.
 */
#include "bmx/com/action/physics.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "dt.h"
#include "dt_ngif.h"
#include "zge/com/zglrw/zrwcol.h"
#include "zge/com/zglrw/zrwmodelrender.h"
#include "zge/zgemods/ztaginf.h"
#include "zge/zlist.h"
#include "zge/zobj.h"

typedef ZMemFreeList *(*FreeListCreateFn)(uint32_t, uint32_t, ZMemDestroyFn);
typedef void *(*FreeListAllocFn)(ZMemFreeList *);
typedef ZList *(*ListCreateFn)(int);
typedef void (*ListInsertFn)(ZList *, void *);

/* ---- shared fixtures ------------------------------------------------------------------------------ */

/* Coordinate near the origin, with edge values; `span` sets the range. */
static float around(float span)
{
    return dt_float_edgy(-span, span);
}

static void around3(ZVec3 *v, float span)
{
    v->x = around(span);
    v->y = around(span);
    v->z = around(span);
}

static ZbspEdgeTag *newEdgeTag(void)
{
    ZbspEdgeTag *t = dt_alloc(sizeof *t);

    t->id = (uint16_t)dt_u32();
    t->kind = dt_u32() % 5 ? (uint8_t)dt_range(1, 3) : 0;
    return t;
}

/* A prop collision object whose flags sometimes exclude it (bits 6-9). */
static PhysPropObject *newPropObject(void)
{
    PhysPropObject *o = dt_alloc(sizeof *o);

    o->index = (uint16_t)dt_u32();
    o->flags = (uint16_t)(dt_u32() & (dt_u32() % 3 ? 0xfc3fu : 0xffffu));
    return o;
}

/* A roller list of `n` prop edges with int16 end points within `span` of the origin. */
static void makeRollerEdges(int n, int span)
{
    ZList *list = DT_ORIG(0x0048c260, ListCreateFn)(0x200);

    for (int i = 0; i < n; i++) {
        PhysEdge *e = dt_alloc(sizeof *e);

        dt_fill(e, sizeof *e);
        for (int k = 0; k < 3; k++) {
            e->a[k] = (int16_t)dt_range(-span, span);
            e->b[k] = dt_u32() % 9 == 0 ? e->a[k] : (int16_t)dt_range(-span, span);
        }
        e->prop = newPropObject();
        DT_ORIG(0x0048c460, ListInsertFn)(list, e);
    }
    gZrollerEdges = list;
}

/* Level tag record as the renderer's triangles point at it: id word, flag byte 3, link at +8. */
static uint8_t *newLevelTag(void)
{
    uint8_t *t = dt_alloc(12);

    dt_fill(t, 12);
    if (dt_u32() & 1) {
        t[1] = (uint8_t)((t[1] & 0x0f) | 0x10);
    }
    if (dt_u32() & 1) {
        *(uint32_t *)(t + 8) = 0;
    }
    return t;
}

/* zrwModelFindTriangle: NULL, a triangle without tag, or one with a tag. */
static ZrwRenderColHit gTri;
static uint8_t *gTriTag;

static ZrwRenderColHit *BMX_CDECL stubFindTriangle(const ZVec3 *point, float maxDistance)
{
    const uint32_t pick = dt_u32() % 4;

    ngLog(0x48e840);
    ngLogVec(point);
    ngLogFloat(maxDistance);
    if (pick == 0) {
        return NULL;
    }
    gTri.tag = pick == 1 ? 0 : (uint32_t)(uintptr_t)gTriTag;
    return &gTri;
}

/* Props whose nodes carry frames the frame lookup can return; objects are sometimes built on demand. */
enum { kMaxNodes = 6 };
static PhysPropFrame *gFrames[kMaxNodes];
static int gNodeCount;

static PhysPropFrame *BMX_CDECL stubObjectFrame(uint32_t objectData)
{
    const uint32_t pick = dt_u32() % (uint32_t)(gNodeCount + 1);

    ngLog(0x560ee0);
    ngLog(objectData);
    return pick < (uint32_t)gNodeCount ? gFrames[pick] : (PhysPropFrame *)(uintptr_t)0x1234;
}

/* zbspBuildCollObject: installs the object prepared in the node's spare word. */
static void BMX_CDECL stubBuildColl(PhysPropNode *node)
{
    ngLog(0x468bc0);
    ngLogPtr(node);
    node->object = *(PhysPropObject **)node->pad28;
}

static void makeProps(void)
{
    PhysPropNode *head = NULL;

    gNodeCount = dt_range(0, kMaxNodes);
    for (int i = 0; i < gNodeCount; i++) {
        PhysPropNode *n = dt_alloc(sizeof *n);
        PhysPropTag *tag = dt_alloc(sizeof *tag);
        PhysPropObject *o = newPropObject();

        gFrames[i] = dt_alloc(sizeof(PhysPropFrame));
        n->frame = gFrames[i];
        tag->range = dt_u32() % 3 ? 0 : (uint16_t)dt_u32();
        n->tag = dt_u32() % 4 ? tag : NULL;
        *(PhysPropObject **)n->pad28 = o;
        n->object = dt_u32() % 3 ? o : NULL;
        n->next = head;
        head = n;
    }
    *zobjPropList() = head;
    dt_stub(0x00468bc0, (void *)zbspBuildCollObject, (void *)stubBuildColl);
    gZbspObjectFrameFn = stubObjectFrame;
}

static void makeTriangleStub(void)
{
    dt_fill(&gTri, sizeof gTri);
    gTriTag = newLevelTag();
    dt_stub(0x0048e840, NULL, (void *)stubFindTriangle);
}

/* ---- zbspFindEdgesNear -------------------------------------------------------------------------- */

static ZMemFreeList *makePool(uint32_t elem)
{
    return DT_ORIG(0x00485e80, FreeListCreateFn)(elem, 4, NULL);
}

static void makeEdgeLevel(int n, ZMemFreeList *pool, float span)
{
    ZbspLevelFace *faces = n ? dt_alloc((uint32_t)n * sizeof *faces) : NULL;

    for (int i = 0; i < n; i++) {
        ZbspLevelFace *f = &faces[i];

        around3(&f->v0, span);
        around3(&f->v1, span);
        around3(&f->v2, span);
        for (int k = 0; k < 3; k++) {
            f->edgeTag[k] = dt_u32() % 4 ? newEdgeTag() : NULL;
            f->edge[k] =
                (pool != NULL && dt_u32() % 3 == 0) ? DT_ORIG(0x00486050, FreeListAllocFn)(pool) : NULL;
        }
    }
    gZtaginfFaces = faces;
    gZtaginfFaceCount = (uint32_t)n;
}

static void setup_edges_near(DtCtx *c)
{
    ZVec3 *p = dt_alloc(sizeof *p);
    const int overflow = c->seed % 23 == 5 ? 1 : c->seed % 23 == 11 ? 2 : 0;
    ZMemFreeList *pool;
    int faces;
    float radius;

    ngBegin(c, 1);
    pool = c->seed % 3 ? makePool(sizeof(PhysEdge)) : NULL;
    gZbspEdgePool = pool;
    faces = c->seed % 9 == 0 ? 0 : dt_range(1, 8);
    radius = dt_float_edgy(0.0f, 150.0f);
    if (c->seed % 13 == 0) {
        radius = -radius; /* the radius is squared: the sign does not matter */
    }
    if (overflow == 1) {
        faces = 86; /* 258 near level edges */
        radius = 1.0e6f;
    }
    makeEdgeLevel(faces, pool, 100.0f);
    if (overflow == 1) {
        for (int i = 0; i < faces; i++) {
            for (int k = 0; k < 3; k++) {
                ZbspEdgeTag *t = newEdgeTag();

                t->kind = 1;
                gZtaginfFaces[i].edgeTag[k] = t;
            }
        }
    }
    makeRollerEdges(overflow == 2 ? 258 : dt_range(0, 8), 120);
    if (overflow == 2) {
        radius = 1.0e6f;
        for (PhysEdge *e = zlistFirst(gZrollerEdges); e != NULL; e = zlistNext(gZrollerEdges)) {
            ((PhysPropObject *)e->prop)->flags = 0;
        }
    }
    dt_fill((void *)&gZbspHits[0], 16);
    around3(p, 100.0f);
    DT_ARG_PTR(c, 0, p);
    DT_ARG_FLOAT(c, 1, radius);
}

/* ---- zbspFindFaceNear ---------------------------------------------------------------------------- */

static ZbspSphereResult gSphere;

static int BMX_CDECL stubSphereTest(const float *center, float radius, uint32_t flags, const int *excluded,
                                    uint32_t *out)
{
    ngLog(0x494ca0);
    ngLogFloat(center[0]);
    ngLogFloat(center[1]);
    ngLogFloat(center[2]);
    ngLogFloat(radius);
    ngLog(flags);
    ngLogPtr(excluded);
    if (gSphere.hits == NULL) {
        return 0;
    }
    out[0] = (uint32_t)(uintptr_t)gSphere.hits;
    out[1] = gSphere.count;
    return 1;
}

/* A renderer triangle around p: its corners within `span` and its normal from the corners (or random). */
static void makeColHit(ZrwColHit *h, const ZVec3 *p, float span)
{
    ZVec3 e1;
    ZVec3 e2;
    ZVec3 n;
    double len;

    dt_fill(h, sizeof *h);
    for (int k = 0; k < 3; k++) {
        h->vertex[k][0] = p->x + around(span);
        h->vertex[k][1] = p->y - around(span) * 0.25f - 2.0f;
        h->vertex[k][2] = p->z + around(span);
    }
    e1.x = h->vertex[1][0] - h->vertex[0][0];
    e1.y = h->vertex[1][1] - h->vertex[0][1];
    e1.z = h->vertex[1][2] - h->vertex[0][2];
    e2.x = h->vertex[2][0] - h->vertex[0][0];
    e2.y = h->vertex[2][1] - h->vertex[0][1];
    e2.z = h->vertex[2][2] - h->vertex[0][2];
    n.x = e1.y * e2.z - e1.z * e2.y;
    n.y = e1.z * e2.x - e1.x * e2.z;
    n.z = e1.x * e2.y - e1.y * e2.x;
    len = __builtin_sqrt((double)n.x * n.x + (double)n.y * n.y + (double)n.z * n.z);
    if (len < 1e-3 || dt_u32() % 7 == 0) {
        n.x = dt_float(-1.0f, 1.0f);
        n.y = dt_float(-1.0f, 1.0f);
        n.z = dt_float(-1.0f, 1.0f);
        len = 1.0;
    }
    if (dt_u32() & 1) {
        len = -len; /* facing either way */
    }
    h->normal[0] = (float)(n.x / len);
    h->normal[1] = (float)(n.y / len);
    h->normal[2] = (float)(n.z / len);
    h->objectData = dt_u32() % 3 ? 0 : (dt_u32() | 1);
}

static void setup_face_near(DtCtx *c)
{
    ZVec3 *p = dt_alloc(sizeof *p);
    const int full = c->seed % 29 == 3;
    float radius;
    float cosLimit;

    ngBegin(c, 1);
    around3(p, 200.0f);
    radius = dt_float_edgy(0.0f, 60.0f);
    cosLimit = dt_u32() % 3 ? 1.0f : dt_float_edgy(-0.5f, 1.0f);
    gSphere.count = (uint32_t)dt_range(0, 7);
    if (full) {
        gSphere.count = 262; /* more than the 255 face records */
    }
    gSphere.hits =
        c->seed % 11 == 0 ? NULL : dt_alloc((gSphere.count ? gSphere.count : 1) * sizeof(ZrwColHit));
    for (uint32_t i = 0; gSphere.hits != NULL && i < gSphere.count; i++) {
        ZrwColHit *h = &gSphere.hits[i];

        makeColHit(h, p, full ? 0.0f : 40.0f);
        if (full) {
            /* a flat triangle right under p */
            h->vertex[0][0] = p->x - 50.0f;
            h->vertex[0][2] = p->z - 50.0f;
            h->vertex[1][0] = p->x + 50.0f;
            h->vertex[1][2] = p->z - 50.0f;
            h->vertex[2][0] = p->x;
            h->vertex[2][2] = p->z + 50.0f;
            for (int k = 0; k < 3; k++) {
                h->vertex[k][1] = p->y - 1.0f;
            }
            h->normal[0] = 0.0f;
            h->normal[1] = 1.0f;
            h->normal[2] = 0.0f;
            h->objectData = 0;
        }
    }
    if (full) {
        radius = 10.0f;
        cosLimit = 1.0f;
    }
    makeProps();
    makeTriangleStub();
    dt_stub(0x00494ca0, NULL, (void *)stubSphereTest);
    dt_fill(&gZbspSphere, sizeof gZbspSphere);
    dt_fill(&gZbspFaceQuery, sizeof gZbspFaceQuery);
    dt_fill(&gZbspNearFaces[0], 4 * sizeof(PhysFace));
    dt_fill(&gZbspNearHits[0], 8 * sizeof(ZbspFaceHit));
    DT_ARG_PTR(c, 0, p);
    DT_ARG_FLOAT(c, 1, radius);
    DT_ARG_FLOAT(c, 2, cosLimit);
}

/* ---- zcollideFindEdgesBehind --------------------------------------------------------------------- */

static ZrwRenderColQuery gQuery;

static ZrwRenderColQuery *BMX_CDECL stubModelQuery(const void *a, const void *b)
{
    ngLog(0x48f320);
    ngLogVec(a);
    ngLog((uint32_t)(uintptr_t)b);
    return gQuery.list != NULL ? &gQuery : NULL;
}

static void setup_edges_behind(DtCtx *c)
{
    ZVec3 *p = dt_alloc(sizeof *p);
    ZVec3 *dir = dt_alloc(sizeof *dir);
    ZrwRenderColHit *list;
    /* Exact mode: small integer, axis-aligned geometry, so distances hit the radius and dots hit 0. */
    const int exact = c->seed % 3 == 1;
    const int span = exact ? 20 : 150;
    float radius = dt_float_edgy(0.0f, 200.0f);

    ngBegin(c, 0);
    around3(p, 100.0f);
    if (exact) {
        p->x = (float)dt_range(-span, span);
        p->y = (float)dt_range(-span, span);
        p->z = (float)dt_range(-span, span);
        dir->x = (float)dt_range(-1, 1);
        dir->y = (float)dt_range(-1, 1);
        dir->z = (float)dt_range(-1, 1);
        radius = (float)dt_range(0, 30);
    } else if (c->seed % 5) {
        dir->x = dt_float(-1.0f, 1.0f);
        dir->y = dt_float(-1.0f, 1.0f);
        dir->z = dt_float(-1.0f, 1.0f);
    } else {
        around3(dir, 2.0f);
    }
    gQuery.found = (uint16_t)dt_range(0, 12);
    list = dt_alloc(12 * sizeof *list);
    for (int i = 0; i < 12; i++) {
        dt_fill(&list[i], sizeof list[i]);
        around3(&list[i].v0, (float)span);
        around3(&list[i].v1, (float)span);
        if (exact) {
            list[i].v0.x = (float)(int)list[i].v0.x;
            list[i].v0.y = (float)(int)list[i].v0.y;
            list[i].v0.z = (float)(int)list[i].v0.z;
            list[i].v1 = list[i].v0;
            list[i].v1.y += (float)dt_range(-span, span);
        }
        list[i].tag = (uint32_t)(uintptr_t)newEdgeTag();
    }
    gQuery.list = c->seed % 7 == 0 ? NULL : list;
    dt_stub(0x0048f320, NULL, (void *)stubModelQuery);
    makeRollerEdges(dt_range(0, 12), span);
    if (exact) {
        int axis = 0;

        for (PhysEdge *e = zlistFirst(gZrollerEdges); e != NULL; e = zlistNext(gZrollerEdges), axis++) {
            const float *pc = &p->x;
            const int k1 = (axis + 1) % 3;
            const int k2 = (axis + 2) % 3;

            /* a power-of-two length normalises exactly; one offset is 0, the distance a whole number */
            e->a[k1] = (int16_t)pc[k1];
            for (int k = 0; k < 3; k++) {
                e->b[k] = e->a[k];
            }
            e->b[axis % 3] = (int16_t)(e->a[axis % 3] + (dt_u32() & 1 ? 1 : -1) * (1 << dt_range(0, 4)));
            if (dt_u32() % 3 == 0) {
                radius = __builtin_fabsf(pc[k2] - (float)e->a[k2]);
            }
        }
    }
    dt_fill((void *)0x005f8520, 0x80);
    dt_fill((void *)0x005f85a8, 0x38 * 4);
    dt_fill((void *)0x005f8ce0, 0x40);
    DT_ARG_PTR(c, 0, p);
    DT_ARG_PTR(c, 1, dir);
    DT_ARG_FLOAT(c, 2, radius);
}

/* ---- zcollideRay --------------------------------------------------------------------------------- */

static ZcollideLineHit gLine[2];
static int gLineCalls;

static int BMX_CDECL stubLineTest(const float *a, const float *b, int mode, int flags, float *out)
{
    const int hit = dt_u32() % 3 != 0;
    const ZcollideLineHit *src = &gLine[gLineCalls++ & 1];

    ngLog(0x494c70);
    for (int i = 0; i < 3; i++) {
        ngLogFloat(a[i]);
        ngLogFloat(b[i]);
    }
    ngLog((uint32_t)mode);
    ngLog((uint32_t)flags);
    if (hit) {
        memcpy(out, src, sizeof *src);
    }
    return hit;
}

static uint32_t gMatWord;

static const uint32_t *BMX_CDECL stubMaterial(const void *material)
{
    ngLog(0x561038);
    ngLogPtr(material);
    return &gMatWord;
}

static void setup_ray(DtCtx *c)
{
    ZVec3 *from = dt_alloc(sizeof *from);
    ZVec3 *to = dt_alloc(sizeof *to);
    void *hit = dt_alloc(sizeof(PhysRayHit));

    ngBegin(c, 1);
    around3(from, 500.0f);
    around3(to, 500.0f);
    if (c->seed % 17 == 0) {
        *to = *from; /* zero length: the direction normalises to garbage */
    }
    for (int i = 0; i < 2; i++) {
        ZcollideLineHit *h = &gLine[i];

        dt_fill(h, sizeof *h);
        around3((ZVec3 *)h->hit.normal, 1.0f);
        for (int k = 0; k < 3; k++) {
            around3((ZVec3 *)h->hit.vertex[k], 500.0f);
        }
        around3(&h->point, 500.0f);
        h->hit.distance = dt_u32() % 8 ? dt_float(-0.0099f, 1.0099f) : dt_float_edgy(-2.0f, 2.0f);
        h->hit.objectData = dt_u32() % 2 ? 0 : (dt_u32() | 1);
        h->hit.materialData = dt_u32() % 3 ? (dt_u32() | 1) : 0;
    }
    gLineCalls = 0;
    gMatWord = dt_u32();
    gZbspMaterialFn = stubMaterial;
    dt_stub(0x00494c70, NULL, (void *)stubLineTest);
    makeProps();
    makeTriangleStub();
    dt_fill(hit, sizeof(PhysRayHit));
    DT_ARG_PTR(c, 0, from);
    DT_ARG_PTR(c, 1, to);
    DT_ARG_PTR(c, 2, hit);
}

/* ---- zcollideFillPropEdges ----------------------------------------------------------------------- */

static void setup_fill_prop_edges(DtCtx *c)
{
    PhysPropObject *o = dt_alloc(sizeof *o);
    PhysPropNode *node = dt_alloc(sizeof *node);
    ZbspTreeObject *src = dt_alloc(sizeof *src);
    PhysEdge *edges = dt_alloc(4 * sizeof *edges);
    ZbspTreeNode *t;
    int n;

    ngBegin(c, 1);
    n = dt_range(0, 6);
    t = ngMakeTree(n);
    for (; t != NULL; t = t->next) {
        if (t->object.shape != NULL) {
            t->object.shape->byte2 = (uint8_t)dt_u32();
            if (dt_u32() % 3) {
                t->object.shape->kind = (uint8_t)dt_range(2, 4);
            }
        }
    }
    dt_fill(o, sizeof *o);
    dt_fill(edges, 4 * sizeof *edges);
    o->index = (uint16_t)dt_range(0, n);
    for (int k = 0; k < 3; k++) {
        o->scale[k] = dt_u32() % 3 ? 1.0f : dt_float_edgy(-3.0f, 3.0f);
        o->pos[k] = (int16_t)(dt_u32() % 9 ? dt_range(-2000, 2000) : (int32_t)dt_u32());
    }
    o->rot.x = (ZAngle)dt_u32();
    o->rot.y = (ZAngle)dt_u32();
    o->rot.z = (ZAngle)dt_u32();
    src->size[0] = dt_float_edgy(0.0f, 20.0f);
    src->size[1] = dt_float_edgy(0.0f, 20.0f);
    src->size[2] = dt_float_edgy(0.0f, 20.0f);
    src->lift = dt_float_edgy(-5.0f, 5.0f);
    src->base = dt_u32() % 3 ? 0.0f : dt_float_edgy(-0.01f, 0.05f);
    if (c->seed % 2 == 1) {
        /*
         * Sizes in hundredths, no rotation, unit scale: the corners land on or next to whole units, so
         * rounding differences survive the truncation to int16.
         */
        src->size[0] = (float)dt_range(0, 2000) * 0.01f;
        src->size[1] = (float)dt_range(0, 2000) * 0.01f;
        src->size[2] = (float)dt_range(0, 2000) * 0.01f;
        src->lift = (float)dt_range(-500, 500) * 0.01f;
        src->base = dt_u32() & 1 ? 0.0f : (float)dt_range(0, 5) * 0.0001f;
        o->rot.x = 0;
        o->rot.y = 0;
        o->rot.z = 0;
        for (int k = 0; k < 3; k++) {
            o->scale[k] = 1.0f;
        }
    }
    node->source = src;
    o->node = node;
    DT_ARG_PTR(c, 0, o);
    DT_ARG_PTR(c, 1, edges);
}

/* ---- cases ------------------------------------------------------------------------------------- */

DT_CASES(collide_big_cases) = {
    {"zbspFindEdgesNear", 0x00466ed0, zbspFindEdgesNear, 2, DT_RET_INT, setup_edges_near, 300, 0},
    {"zbspFindFaceNear", 0x00468070, zbspFindFaceNear, 3, DT_RET_INT, setup_face_near, 400, 0},
    {"zcollideFindEdgesBehind", 0x00469560, zcollideFindEdgesBehind, 3, DT_RET_INT, setup_edges_behind, 300,
     0},
    {"zcollideRay", 0x00469b00, zcollideRay, 3, DT_RET_INT, setup_ray, 400, 0},
    {"zcollideFillPropEdges", 0x0046a4d0, zcollideFillPropEdges, 2, DT_RET_INT, setup_fill_prop_edges, 400,
     0},
};
