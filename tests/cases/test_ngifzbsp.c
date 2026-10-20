/*
 * Tests for NgIFzbsp.c: level geometry queries, prop collision objects and the debug-print hooks.
 */
#include "bmx/com/action/physics.h"
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "dt.h"
#include "dt_ngif.h"
#include "dt_zmem.h"
#include "zge/zgemods/ztaginf.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zobj.h"

/* ---- fixtures ------------------------------------------------------------------------------------ */

typedef ZMemFreeList *(*FreeListCreateFn)(uint32_t, uint32_t, ZMemDestroyFn);
typedef void *(*FreeListAllocFn)(ZMemFreeList *);
typedef ZList *(*ListCreateFn)(int);
typedef void (*ListInsertFn)(ZList *, void *);

static ZMemFreeList *makePool(uint32_t elem)
{
    return DT_ORIG(0x00485e80, FreeListCreateFn)(elem, 4, NULL);
}

static void *poolAlloc(ZMemFreeList *pool)
{
    return DT_ORIG(0x00486050, FreeListAllocFn)(pool);
}

/* A random quaternion that is not close to zero. */
static void randQuat(ZQuat *q)
{
    do {
        q->x = dt_float(-1.0f, 1.0f);
        q->y = dt_float(-1.0f, 1.0f);
        q->z = dt_float(-1.0f, 1.0f);
        q->w = dt_float(-1.0f, 1.0f);
    } while (q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w < 0.25f);
}

static PhysPropFrame *newFrame(void)
{
    PhysPropFrame *f = dt_alloc(sizeof *f);

    dt_fill(f, sizeof *f);
    ngRf3(&f->pos);
    randQuat(&f->rot);
    f->model = (void *)(uintptr_t)(dt_u32() | 0x1000);
    return f;
}

static PhysPropNode *newNode(void)
{
    PhysPropNode *n = dt_alloc(sizeof *n);

    dt_fill(n, sizeof *n);
    n->frame = newFrame();
    return n;
}

static PhysPropObject *newPropObject(void)
{
    PhysPropObject *o = dt_alloc(sizeof *o);

    dt_fill(o, sizeof *o);
    o->node = newNode();
    o->edges = (dt_u32() & 1) ? (PhysEdge *)(uintptr_t)(dt_u32() | 0x1000) : NULL;
    return o;
}

static ZAngle3 *newAngles(void)
{
    ZAngle3 *a = dt_alloc(sizeof *a);

    a->x = (ZAngle)dt_u32();
    a->y = (ZAngle)dt_u32();
    a->z = (ZAngle)dt_u32();
    return a;
}

/* ---- rider actors and prop movement --------------------------------------------------------------- */

static void BMX_CDECL stubSyncPose(void *actor)
{
    ngLog(0x4654b0);
    ngLogPtr(actor);
}

static void setup_sync_actors(DtCtx *c)
{
    ngBegin(c, 0);
    *(uint32_t *)0x0056b408 = (dt_u32() & ~0xffu) | (uint32_t)dt_range(0, 3);
    for (int i = 0; i < 4; i++) {
        gPhysRiders[i].actor = (void *)(uintptr_t)(dt_u32() | 0x1000);
        gPhysRiders[i].bikeObj = (void *)(uintptr_t)(dt_u32() | 0x1000);
    }
    dt_stub(0x004654b0, NULL, (void *)stubSyncPose);
}

static void BMX_CDECL stubSetRotation(void *object, const ZQuat *q)
{
    ngLog(0x4874d0);
    ngLogPtr(object);
    ngLogFloat(q->x);
    ngLogFloat(q->y);
    ngLogFloat(q->z);
    ngLogFloat(q->w);
}

static void BMX_CDECL stubSetPosition(void *object, const ZVec3 *pos)
{
    ngLog(0x487470);
    ngLogPtr(object);
    ngLogVec(pos);
}

static int BMX_CDECL stubFillEdges(PhysPropObject *object, PhysEdge *edges)
{
    ngLog(0x46a4d0);
    ngLogPtr(object);
    ngLogPtr(edges);
    return 0;
}

static void setup_prop_rotation(DtCtx *c)
{
    PhysPropObject *o;

    ngBegin(c, 1);
    dt_stub(0x004874d0, NULL, (void *)stubSetRotation);
    dt_stub(0x0046a4d0, NULL, (void *)stubFillEdges);
    o = newPropObject();
    o->flags = (uint16_t)dt_u32();
    DT_ARG_PTR(c, 0, o);
    DT_ARG_PTR(c, 1, newAngles());
}

static void setup_prop_position(DtCtx *c)
{
    PhysPropObject *o;
    ZVec3 *pos = dt_alloc(sizeof *pos);

    ngBegin(c, 1);
    dt_stub(0x00487470, NULL, (void *)stubSetPosition);
    dt_stub(0x0046a4d0, NULL, (void *)stubFillEdges);
    o = newPropObject();
    ngRf3(pos);
    if (dt_u32() % 5 == 0) {
        pos->x *= 1.0e7f; /* beyond int16 (and sometimes int32): the truncation must match */
        pos->z *= 1.0e12f;
    }
    DT_ARG_PTR(c, 0, o);
    DT_ARG_PTR(c, 1, pos);
}

/* ---- tree objects and shapes ---------------------------------------------------------------------- */

static void setup_build_shape(DtCtx *c)
{
    ZbspTreeObject *obj = dt_alloc(sizeof *obj);
    ZbspObjectDef *def = dt_alloc(sizeof *def);

    ngBegin(c, 0);
    dt_fill(obj, sizeof *obj);
    obj->size[0] = dt_float_edgy(0.0f, 20.0f);
    obj->size[1] = dt_float_edgy(0.0f, 20.0f);
    obj->size[2] = dt_float_edgy(0.0f, 20.0f);
    obj->lift = dt_float_edgy(-5.0f, 5.0f);
    obj->base = dt_float_edgy(-5.0f, 5.0f);
    def->flags = (uint8_t)dt_u32();
    def->kind = (uint8_t)(dt_u32() % 7);
    obj->def = c->seed % 6 == 0 ? NULL : def;
    DT_ARG_PTR(c, 0, obj);
}

static void setup_tree_index(DtCtx *c)
{
    int n;
    ZbspTreeNode *node;
    int pick;

    ngBegin(c, 0);
    n = dt_range(0, 7);
    pick = dt_range(0, n + 1);
    node = ngMakeTree(n);
    for (; pick > 0 && node != NULL; pick--) {
        node = node->next;
    }
    DT_ARG_PTR(c, 0, node != NULL ? &node->object : (c->seed & 1 ? (void *)dt_alloc(0x60) : NULL));
}

static void setup_tree_at(DtCtx *c)
{
    int n;

    ngBegin(c, 0);
    n = dt_range(0, 7);
    ngMakeTree(n);
    DT_ARG_INT(c, 0, dt_range(-1, n + 1));
}

static void setup_shape_index(DtCtx *c)
{
    int n;

    ngBegin(c, 0);
    n = dt_range(0, 7);
    ngMakeTree(n);
    gZbspNoShapeKind = (uint8_t)dt_u32();
    DT_ARG_INT(c, 0, ((uint32_t)dt_range(-1, n + 1) & 0xffff) | (dt_u32() & 0xffff0000u));
}

/* ---- prop signals, material ----------------------------------------------------------------------- */

static void BMX_CDECL stubFlags(void *model, int set, int clear)
{
    ngLog(0x560ed8);
    ngLogPtr(model);
    ngLog((uint32_t)set);
    ngLog((uint32_t)clear);
}

static void BMX_CDECL stubVisible(int32_t object, int visible)
{
    ngLog(0x487600);
    ngLog((uint32_t)object);
    ngLog((uint32_t)visible);
}

static void setup_signal(DtCtx *c)
{
    PhysPropObject *o;

    ngBegin(c, 0);
    o = newPropObject();
    gZobjSetFlags = stubFlags;
    dt_stub(0x00487600, NULL, (void *)stubVisible);
    DT_ARG_PTR(c, 0, o);
    DT_ARG_INT(c, 1, dt_u32() % 8 == 0 ? dt_u32() : (uint32_t)dt_range(0, 7));
}

static uint32_t gMatWord;

static const uint32_t *BMX_CDECL stubMaterial(const void *material)
{
    ngLog(0x561038);
    ngLogPtr(material);
    return &gMatWord;
}

static void setup_mat_ref(DtCtx *c)
{
    uint8_t *obj = dt_alloc(0x100);
    uint8_t *holder = dt_alloc(0x60);

    ngBegin(c, 0);
    gMatWord = dt_u32();
    gZbspMatRefValue = dt_u32();
    gZbspMaterialFn = stubMaterial;
    *(uint8_t **)(obj + 0x84) = holder;
    *(uint32_t *)(holder + 0x48) = (c->seed % 3 == 0) ? 0 : (dt_u32() | 0x1000);
    DT_ARG_PTR(c, 0, obj);
}

/* ---- pure geometry ------------------------------------------------------------------------------ */

static void setup_centroid(DtCtx *c)
{
    PhysFace *f = dt_alloc(sizeof *f);
    ZVec3 *out = dt_alloc(16);

    ngBegin(c, 1);
    dt_fill(f, sizeof *f);
    ngRf3(&f->p0);
    ngRf3(&f->p1);
    ngRf3(&f->p2);
    dt_fill(out, 16);
    DT_ARG_PTR(c, 0, f);
    DT_ARG_PTR(c, 1, out);
}

static void setup_segment(DtCtx *c)
{
    ZVec3 *a = dt_alloc(sizeof *a), *b = dt_alloc(sizeof *b), *p = dt_alloc(sizeof *p), *out = dt_alloc(16);
    float u;

    ngBegin(c, 1);
    u = dt_float(-0.5f, 1.5f);
    ngRf3(a);
    ngRf3(b);
    if (c->seed % 13 == 0) {
        *b = *a; /* zero length direction */
    }
    /* p near the line a-b, before, inside and beyond the segment */
    p->x = a->x + (b->x - a->x) * u + dt_float(-30.0f, 30.0f);
    p->y = a->y + (b->y - a->y) * u + dt_float(-30.0f, 30.0f);
    p->z = a->z + (b->z - a->z) * u + dt_float(-30.0f, 30.0f);
    dt_fill(out, 16);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
    DT_ARG_PTR(c, 2, p);
    DT_ARG_PTR(c, 3, out);
}

static void setup_edge_side(DtCtx *c)
{
    uint8_t *e = dt_alloc(sizeof(PhysEdge));

    ngBegin(c, 0);
    e[0] = (uint8_t)dt_u32();
    DT_ARG_PTR(c, 0, e);
}

static int16_t *newVert(void)
{
    int16_t *v = dt_alloc(8);

    for (int i = 0; i < 3; i++) {
        v[i] = (int16_t)dt_range(-50, 50);
    }
    return v;
}

/* Vertex pointers hold int16 triples; the query point is mostly inside the polygon. */
static void setup_point_in_face(DtCtx *c)
{
    PhysFace *f = dt_alloc(sizeof *f);
    int axis;
    int16_t *tri[3];
    int16_t *quad;
    float n[3];
    float p[3];
    float u;
    float w;

    ngBegin(c, 0);
    axis = dt_range(0, 2);
    u = dt_float(0.0f, 1.0f);
    w = dt_float(0.0f, 1.0f);
    dt_fill(f, sizeof *f);
    for (int i = 0; i < 3; i++) {
        n[i] = dt_float_edgy(-0.9f, 0.9f);
    }
    n[axis] = (dt_u32() & 1 ? 1.0f : -1.0f) * dt_float(1.0f, 2.0f);
    if (c->seed % 3 == 0) {
        n[(axis + 1 + dt_range(0, 1)) % 3] = n[axis]; /* tie between two components */
    }
    if (c->seed % 17 == 0) {
        n[axis] = (dt_u32() & 1) ? 5.0e-6f : -5.0e-6f; /* inside the +-1e-5 dead zone */
        n[(axis + 1) % 3] = 0.0f;
        n[(axis + 2) % 3] = 0.0f;
    }
    f->normal.x = n[0];
    f->normal.y = n[1];
    f->normal.z = n[2];
    f->flags = (uint8_t)dt_u32();
    for (int i = 0; i < 3; i++) {
        tri[i] = newVert();
        f->vert[i] = tri[i];
    }
    /* Quad flag: a parallelogram A, B, B + C - A, C. Triangle otherwise: A, B, C. */
    quad = newVert();
    for (int k = 0; k < 3; k++) {
        quad[k] = (int16_t)(tri[1][k] + tri[2][k] - tri[0][k]);
    }
    if (f->flags & 1) {
        f->vert[2] = quad;
        f->vert[3] = tri[2];
    } else {
        f->vert[3] = newVert();
        if (u + w > 1.0f) {
            u = 1.0f - u;
            w = 1.0f - w;
        }
    }
    for (int k = 0; k < 3; k++) {
        p[k] = (float)tri[0][k] + u * (float)(tri[1][k] - tri[0][k]) + w * (float)(tri[2][k] - tri[0][k]);
    }
    if (dt_u32() % 4 == 0) {
        p[0] = (float)dt_range(-60, 60) + dt_float(0.0f, 0.99f); /* anywhere */
        p[1] = (float)dt_range(-60, 60) + dt_float(0.0f, 0.99f);
        p[2] = (float)dt_range(-60, 60) + dt_float(0.0f, 0.99f);
    }
    /* Half of the time make sure the winding accepts the point (the outcome depends on the axis). */
    if (!zbspPointInFace(f, p[0], p[1], p[2]) && (dt_u32() & 1)) {
        const int last = (f->flags & 1) ? 3 : 2;
        void *swap = f->vert[1];

        f->vert[1] = f->vert[last];
        f->vert[last] = swap;
    }
    DT_ARG_PTR(c, 0, f);
    DT_ARG_FLOAT(c, 1, p[0]);
    DT_ARG_FLOAT(c, 2, p[1]);
    DT_ARG_FLOAT(c, 3, p[2]);
}

static void setup_closest_on_edge(DtCtx *c)
{
    int16_t *a, *b;
    ZVec3 *dir = dt_alloc(sizeof *dir), *p = dt_alloc(sizeof *p), *out = dt_alloc(16);

    ngBegin(c, 0);
    a = newVert();
    b = newVert();
    dir->x = (float)(b[0] - a[0]);
    dir->y = (float)(b[1] - a[1]);
    dir->z = (float)(b[2] - a[2]);
    {
        const float len = __builtin_sqrtf(dir->x * dir->x + dir->y * dir->y + dir->z * dir->z);

        if (len > 0.0f) {
            dir->x /= len;
            dir->y /= len;
            dir->z /= len;
        }
    }
    if (c->seed % 7 == 0) {
        ngRf3(dir); /* arbitrary direction */
    }
    p->x = (float)dt_range(-70, 70) + dt_float(0.0f, 0.99f);
    p->y = (float)dt_range(-70, 70) + dt_float(0.0f, 0.99f);
    p->z = (float)dt_range(-70, 70) + dt_float(0.0f, 0.99f);
    dt_fill(out, 16);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
    DT_ARG_PTR(c, 2, dir);
    DT_ARG_PTR(c, 3, p);
    DT_ARG_PTR(c, 4, out);
}

/* ---- level faces and edges ------------------------------------------------------------------------ */

static ZbspEdgeTag *newEdgeTag(void)
{
    ZbspEdgeTag *t = dt_alloc(sizeof *t);

    t->id = (uint16_t)dt_range(0x10, 0x14);
    t->kind = dt_u32() % 5 ? (uint8_t)dt_range(1, 3) : 0;
    return t;
}

static uint16_t *newFaceTag(void)
{
    uint16_t *t = dt_alloc(4);

    *t = (uint16_t)(dt_range(0xfa0, 0xfa5) | (dt_u32() & 0xf000));
    return t;
}

/*
 * Level of `n` triangles installed as the tag tables' face array. Tags and edge tags are mostly present,
 * the cached face / edge records are mostly absent (some come from the real pools).
 */
static ZbspLevelFace *makeLevel(int n, ZMemFreeList *facePool, ZMemFreeList *edgePool)
{
    ZbspLevelFace *faces = n ? dt_alloc((uint32_t)n * sizeof *faces) : NULL;

    for (int i = 0; i < n; i++) {
        ZbspLevelFace *f = &faces[i];

        ngRf3(&f->v0);
        ngRf3(&f->v1);
        ngRf3(&f->v2);
        f->tag = dt_u32() % 6 ? newFaceTag() : NULL;
        for (int k = 0; k < 3; k++) {
            f->edgeTag[k] = dt_u32() % 4 ? newEdgeTag() : NULL;
            f->edge[k] = (edgePool != NULL && dt_u32() % 3 == 0) ? poolAlloc(edgePool) : NULL;
        }
        f->face = (facePool != NULL && dt_u32() % 3 == 0) ? poolAlloc(facePool) : NULL;
    }
    gZtaginfFaces = faces;
    gZtaginfFaceCount = (uint32_t)n;
    return faces;
}

static void setup_find_edge(DtCtx *c)
{
    ngBegin(c, 1);
    makeLevel(c->seed % 9 == 0 ? 0 : dt_range(1, 6), NULL, NULL);
    dt_fill(&gZbspEdgeRecord, sizeof gZbspEdgeRecord);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, (uint32_t)dt_range(0x0f, 0x15) | (dt_u32() & 0xffff0000u));
    DT_ARG_INT(c, 2, dt_range(0, 5));
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_build_face(DtCtx *c)
{
    ZbspLevelFace *src = dt_alloc(sizeof *src);
    PhysFace *out = dt_alloc(sizeof *out);

    ngBegin(c, 1);
    dt_fill(out, sizeof *out);
    ngRf3(&src->v0);
    ngRf3(&src->v1);
    ngRf3(&src->v2);
    DT_ARG_PTR(c, 0, src);
    DT_ARG_PTR(c, 1, newFaceTag());
    DT_ARG_PTR(c, 2, out);
}

static void setup_tagged_edges(DtCtx *c)
{
    ZMemFreeList *pool;
    int n;

    ngBegin(c, 1);
    pool = (c->seed % 3) ? makePool(sizeof(PhysEdge)) : NULL;
    n = dt_range(0, 7);
    if (c->seed % 41 == 0) {
        n = 90; /* more than 256 matching edges: the overflow assert */
    }
    gZbspEdgePool = pool;
    makeLevel(n, NULL, pool);
    dt_fill((void *)&gZbspHits[0], 16);
    DT_ARG_INT(c, 0, (uint32_t)dt_range(0x0f, 0x13) | (dt_u32() & 0xffff0000u));
    DT_ARG_INT(c, 1, (uint32_t)dt_range(0x12, 0x15) | (dt_u32() & 0xffff0000u));
    if (c->seed % 41 == 0) {
        /* every edge matches */
        for (int i = 0; i < n; i++) {
            for (int k = 0; k < 3; k++) {
                ZbspEdgeTag *t = newEdgeTag();

                t->kind = 1;
                t->id = 0x11;
                gZtaginfFaces[i].edgeTag[k] = t;
            }
        }
        DT_ARG_INT(c, 0, 0x10);
        DT_ARG_INT(c, 1, 0x12);
    }
}

/* ---- props: collision objects and lists --------------------------------------------------------- */

static void BMX_CDECL stubBuildColl(PhysPropNode *node)
{
    ngLog(0x468bc0);
    ngLogPtr(node);
    node->object = dt_alloc(sizeof(PhysPropObject));
    node->object->index = (uint16_t)dt_u32();
}

static ZbspTreeObject gSource;

static void setup_build_coll(DtCtx *c)
{
    int n;
    PhysPropNode *node;
    ZbspTreeNode *t;
    PhysPropTag *tag = dt_alloc(sizeof *tag);
    int pick;

    ngBegin(c, 1);
    n = dt_range(0, 5);
    pick = dt_range(0, n);
    node = newNode();
    ngMakeTree(n);
    t = *zobjTreeObjectList();
    for (; pick > 0 && t != NULL; pick--) {
        t = t->next;
    }
    node->source = t != NULL && c->seed % 4 ? &t->object : &gSource;
    dt_fill(tag, sizeof *tag);
    node->tag = c->seed % 5 == 0 ? NULL : tag;
    DT_ARG_PTR(c, 0, node);
}

typedef void *(*ZmemAllocFn)(uint32_t);
typedef ZList *(*ZlistCreateFn)(int);
typedef void (*ZlistInsertFn)(ZList *, void *);

static void setup_free_coll(DtCtx *c)
{
    int n;
    PhysPropObject *o = NULL;

    ngBegin(c, 1);
    n = dt_range(0, 5);
    ngMakeTree(n);
    gZrollerEdges = DT_ORIG(0x0048c260, ZlistCreateFn)(0x80);
    if (c->seed % 8 != 0) {
        o = dt_zmemalloc(500);
        dt_fill(o, 500);
        o->index = (uint16_t)dt_range(0, n + 1);
        o->edges = (c->seed % 5 == 0) ? NULL : dt_zmemalloc(4 * sizeof(PhysEdge));
        for (int i = 0; i < 6; i++) {
            if (o->edges != NULL && (dt_u32() & 1)) {
                DT_ORIG(0x0048c460, ZlistInsertFn)(gZrollerEdges, &o->edges[dt_range(0, 3)]);
            } else {
                DT_ORIG(0x0048c460, ZlistInsertFn)(gZrollerEdges, (void *)(uintptr_t)(0x3000 + i * 0x38));
            }
        }
    }
    DT_ARG_PTR(c, 0, o);
}

static void setup_props_near(DtCtx *c)
{
    PhysPropNode *head = NULL;
    int n;

    ngBegin(c, 1);
    n = dt_range(0, 7);
    if (c->seed % 37 == 0) {
        n = 270; /* all in range: the list overflows */
    }
    for (int i = 0; i < n; i++) {
        PhysPropNode *node = dt_alloc(sizeof *node);
        PhysPropTag *tag = dt_alloc(sizeof *tag);

        tag->group = (uint8_t)(c->seed % 37 == 0 ? 11 : dt_range(0, 20));
        node->tag = (c->seed % 37 == 0 || dt_u32() % 5) ? tag : NULL;
        node->object = (dt_u32() & 1) ? dt_alloc(sizeof(PhysPropObject)) : NULL;
        node->next = head;
        head = node;
    }
    *zobjPropList() = head;
    dt_stub(0x00468bc0, (void *)zbspBuildCollObject, (void *)stubBuildColl);
    DT_ARG_INT(c, 0, (uint32_t)dt_range(0, 12) | (dt_u32() & 0xffff0000u));
    DT_ARG_INT(c, 1, (uint32_t)dt_range(8, 20) | (dt_u32() & 0xffff0000u));
}

static void setup_tagged_faces(DtCtx *c)
{
    PhysPropNode *head = NULL;
    int n;
    const int overflow = c->seed % 53 == 0;
    int nodes;

    ngBegin(c, 1);
    n = dt_range(0, 6);
    nodes = dt_range(0, 4);
    gZbspFacePool = makePool(sizeof(PhysFace));
    if (overflow) {
        n = 262;
    }
    makeLevel(n, gZbspFacePool, NULL);
    for (int i = 0; i < nodes; i++) {
        PhysPropNode *node = dt_alloc(sizeof *node);
        PhysPropTag *tag = dt_alloc(sizeof *tag);

        tag->range = (uint16_t)(dt_range(0xfa0, 0xfa5) | (dt_u32() & 0xf000));
        node->tag = dt_u32() % 4 ? tag : NULL;
        node->object = (dt_u32() & 1) ? dt_alloc(sizeof(PhysPropObject)) : NULL;
        node->next = head;
        head = node;
    }
    *zobjPropList() = head;
    gZbspPropFaceCount = (uint32_t)dt_range(0, 9);
    dt_stub(0x00468bc0, (void *)zbspBuildCollObject, (void *)stubBuildColl);
    DT_ARG_INT(c, 0, dt_range(0xfa0, 0xfa3));
    DT_ARG_INT(c, 1, dt_range(0xfa2, 0xfa6));
    if (overflow) {
        for (int i = 0; i < n; i++) {
            uint16_t *t = newFaceTag();

            *t = 0xfa1;
            gZtaginfFaces[i].tag = t;
        }
        DT_ARG_INT(c, 0, 0xfa0);
        DT_ARG_INT(c, 1, 0xfa2);
    }
}

/* ---- cases ------------------------------------------------------------------------------------- */

DT_CASES(bsp_cases) = {
    {"zactorSyncRiderActors", 0x004659e0, zactorSyncRiderActors, 0, DT_RET_VOID, setup_sync_actors, 20, 0},
    {"zbspPropSetRotation", 0x004661c0, zbspPropSetRotation, 2, DT_RET_VOID, setup_prop_rotation, 400, 0},
    {"zbspPropRotate", 0x00466230, zbspPropRotate, 2, DT_RET_VOID, setup_prop_rotation, 400, 0},
    {"zbspPropSetPosition", 0x004662b0, zbspPropSetPosition, 2, DT_RET_VOID, setup_prop_position, 300, 0},
    {"zbspBuildShape", 0x00466390, zbspBuildShape, 1, DT_RET_VOID, setup_build_shape, 300, 0},
    {"zbspTreeObjectIndex", 0x004664e0, zbspTreeObjectIndex, 1, DT_RET_INT, setup_tree_index, 200, 0},
    {"zbspTreeObjectAt", 0x00466550, zbspTreeObjectAt, 1, DT_RET_INT, setup_tree_at, 200, 0},
    {"zbspGetShape", 0x004665c0, zbspGetShape, 1, DT_RET_INT, setup_shape_index, 200, 0},
    {"zbspReturnZero", 0x004665f0, zbspReturnZero, 0, DT_RET_INT, ngSetupTrace, 2, 0},
    {"zbspObjectSignal", 0x00466610, zbspObjectSignal, 2, DT_RET_VOID, setup_signal, 200, 0},
    {"zbspGetMatRef", 0x00466bb0, zbspGetMatRef, 1, DT_RET_INT, setup_mat_ref, 60, 0},
    {"zbspFaceCentroid", 0x00466c50, zbspFaceCentroid, 2, DT_RET_VOID, setup_centroid, 300, 0},
    {"zbspClosestPointOnSegment", 0x00466d70, zbspClosestPointOnSegment, 4, DT_RET_VOID, setup_segment, 600,
     0},
    {"zbspEdgeSide", 0x00467690, zbspEdgeSide, 1, DT_RET_INT, setup_edge_side, 60, 0},
    {"zbspFindEdge", 0x00467410, zbspFindEdge, 4, DT_RET_INT, setup_find_edge, 300, 0},
    {"zbspPointInFace", 0x00467a90, zbspPointInFace, 4, DT_RET_INT, setup_point_in_face, 800, 0},
    {"zbspClosestPointOnEdge", 0x00468920, zbspClosestPointOnEdge, 5, DT_RET_VOID, setup_closest_on_edge, 600,
     0},
    {"zbspBuildCollObject", 0x00468bc0, zbspBuildCollObject, 1, DT_RET_VOID, setup_build_coll, 200, 0},
    {"zbspFreeCollObject", 0x00468d20, zbspFreeCollObject, 1, DT_RET_VOID, setup_free_coll, 200, 0},
    {"zbspFindPropsNear", 0x00468e30, zbspFindPropsNear, 2, DT_RET_INT, setup_props_near, 200, 0},
    {"zbspFindTaggedFaces", 0x00468ef0, zbspFindTaggedFaces, 2, DT_RET_INT, setup_tagged_faces, 200, 0},
    {"zbspBuildFace", 0x00469080, zbspBuildFace, 3, DT_RET_VOID, setup_build_face, 300, 0},
    {"zbspFindTaggedEdges", 0x004691c0, zbspFindTaggedEdges, 2, DT_RET_INT, setup_tagged_edges, 200, 0},
    {"zbspGetShapeKind", 0x00469530, zbspGetShapeKind, 1, DT_RET_INT, setup_shape_index, 200, 0},
};

/* ---- debug-print hooks ----------------------------------------------------------------------- */

DT_CASES(zbsp_trace_cases) = {
    {"zbspInit", 0x00465a50, zbspInit, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspUpdate", 0x00465a80, zbspUpdate, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspShutdown", 0x00465ab0, zbspShutdown, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspInitializeScene", 0x00465ae0, zbspInitializeScene, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspLoadScene", 0x00465b10, zbspLoadScene, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspLoadAndProcessMap", 0x00465b40, zbspLoadAndProcessMap, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspAllocAndLoadMap", 0x00465b70, zbspAllocAndLoadMap, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFreeMap", 0x00465ba0, zbspFreeMap, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspSetTreeVisible", 0x00465bd0, zbspSetTreeVisible, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspConvertTreeFromLoad", 0x00465c00, zbspConvertTreeFromLoad, 1, DT_RET_INT, ngSetupTrace1, 4, 0},
    {"zbspRelativeLodZ", 0x00465c40, zbspRelativeLodZ, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspPreProcessScene", 0x00465c70, zbspPreProcessScene, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspPreProcessTerrainNode", 0x00465ca0, zbspPreProcessTerrainNode, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspPreProcessFaceList", 0x00465cd0, zbspPreProcessFaceList, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspViewerInPosSide", 0x00465d00, zbspViewerInPosSide, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspSndRegister", 0x00465d40, zbspSndRegister, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspSndSetState", 0x00465d70, zbspSndSetState, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspSndGetHandle", 0x00465da0, zbspSndGetHandle, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspDvliteHandle", 0x00465dd0, zbspDvliteHandle, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspDvliteGetColor", 0x00465e00, zbspDvliteGetColor, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspDvliteLite", 0x00465e40, zbspDvliteLite, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspDvliteInitialize", 0x00465e70, zbspDvliteInitialize, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspDvliteAttachToProps", 0x00465ea0, zbspDvliteAttachToProps, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspPropActivate", 0x00465ed0, zbspPropActivate, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspPropDeactivate", 0x00465f00, zbspPropDeactivate, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspActivateTree", 0x00465f30, zbspActivateTree, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspDeactivateTree", 0x00465f60, zbspDeactivateTree, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspInitializeTreeRef", 0x00465f90, zbspInitializeTreeRef, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspCreateTreeRef", 0x00465fc0, zbspCreateTreeRef, 0, DT_RET_INT16, ngSetupTrace, 4, 0},
    {"zbspDestroyTreeRef", 0x00466000, zbspDestroyTreeRef, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspPropAddTreeRef", 0x00466030, zbspPropAddTreeRef, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspRemoveTreeRef", 0x00466060, zbspRemoveTreeRef, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspUpdateTreeRef", 0x00466090, zbspUpdateTreeRef, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspAddToTree", 0x004660c0, zbspAddToTree, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspRemoveFromTree", 0x004660f0, zbspRemoveFromTree, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspTotallyWithinTree", 0x00466120, zbspTotallyWithinTree, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspOverlapsTree", 0x00466160, zbspOverlapsTree, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFindTagged", 0x00466190, zbspFindTagged, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspPropMove", 0x00466320, zbspPropMove, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspGetNumDefs", 0x00466350, zbspGetNumDefs, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspGetTypeFromHash", 0x004664b0, zbspGetTypeFromHash, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFxCreateTree", 0x004666e0, zbspFxCreateTree, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspDestroyTree", 0x00466710, zbspDestroyTree, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspDrawTerrain", 0x00466740, zbspDrawTerrain, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspDrawTerrainNode", 0x00466770, zbspDrawTerrainNode, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspDrawFaceList", 0x004667a0, zbspDrawFaceList, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspSetDebug", 0x004667d0, zbspSetDebug, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspDrawLineZ", 0x00466800, zbspDrawLineZ, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspDrawFaceNormal", 0x00466830, zbspDrawFaceNormal, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspDrawFaceFlat", 0x00466860, zbspDrawFaceFlat, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspShowBoundingBox", 0x00466890, zbspShowBoundingBox, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspShowEdges", 0x004668c0, zbspShowEdges, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspShowPropVolume", 0x004668f0, zbspShowPropVolume, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspGetFaceNum", 0x00466920, zbspGetFaceNum, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspTrnDumpTree", 0x00466960, zbspTrnDumpTree, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspShowPartition", 0x00466990, zbspShowPartition, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspShowPartition2", 0x004669c0, zbspShowPartition2, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspDrawPartition", 0x004669f0, zbspDrawPartition, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspGetTerrainFaces", 0x00466a20, zbspGetTerrainFaces, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspGetTreeObject", 0x00466a50, zbspGetTreeObject, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspPointWithinTreeObject", 0x00466a80, zbspPointWithinTreeObject, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspGetTreeObjects", 0x00466ab0, zbspGetTreeObjects, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspGetTerrainNode", 0x00466ae0, zbspGetTerrainNode, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspGetMaterialInfo", 0x00466b10, zbspGetMaterialInfo, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspGetMatRefData", 0x00466b40, zbspGetMatRefData, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspGetMatIdx", 0x00466b70, zbspGetMatIdx, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspMatRefList", 0x00466bf0, zbspMatRefList, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFindTagged2", 0x00466c20, zbspFindTagged2, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspDoesVertExist", 0x00466ca0, zbspDoesVertExist, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFacesAtSamePos", 0x00466cd0, zbspFacesAtSamePos, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspSceneSaveState", 0x00466d10, zbspSceneSaveState, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspSceneRestoreState", 0x00466d40, zbspSceneRestoreState, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspGetNormalVector", 0x004676c0, zbspGetNormalVector, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspGetSideVector", 0x004676f0, zbspGetSideVector, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspCollide", 0x00467730, zbspCollide, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspCreateSpanPlanes", 0x00467760, zbspCreateSpanPlanes, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspAddToFacelist", 0x00467790, zbspAddToFacelist, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspGetTerrainHeightinf", 0x004677c0, zbspGetTerrainHeightinf, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspTestRayCollision", 0x004677f0, zbspTestRayCollision, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspTest", 0x00467820, zbspTest, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspTestResetFaceColors", 0x00467850, zbspTestResetFaceColors, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspHighlightNodeFaces", 0x00467880, zbspHighlightNodeFaces, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspSetFaceRgb", 0x004678b0, zbspSetFaceRgb, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspPrintFaceListInf", 0x004678e0, zbspPrintFaceListInf, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspTrnTestRayCollision", 0x00467910, zbspTrnTestRayCollision, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspCreateNodesHit", 0x00467940, zbspCreateNodesHit, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspAddToNodeslist", 0x00467970, zbspAddToNodeslist, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspAddToFacesList", 0x004679a0, zbspAddToFacesList, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspLineToNodes", 0x004679d0, zbspLineToNodes, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspLineToFaces", 0x00467a10, zbspLineToFaces, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspLinePlaneIntersection", 0x00467a50, zbspLinePlaneIntersection, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspClipRayToObject", 0x00467d30, zbspClipRayToObject, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspClipLineToProps", 0x00467d70, zbspClipLineToProps, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspClipLineToProp", 0x00467db0, zbspClipLineToProp, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspBuildRayBoundingBox", 0x00467de0, zbspBuildRayBoundingBox, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspAddPropFaces", 0x00467e10, zbspAddPropFaces, 1, DT_RET_INT, ngSetupTrace1, 4, 0},
    {"zbspComputeFacePlaneEq", 0x00467e50, zbspComputeFacePlaneEq, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspTagInitialize", 0x00467e80, zbspTagInitialize, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspTagRegisterProFunc", 0x00467eb0, zbspTagRegisterProFunc, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspFaceTagRegister", 0x00467ee0, zbspFaceTagRegister, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFaceTagPreProcess", 0x00467f10, zbspFaceTagPreProcess, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFaceTagProcess", 0x00467f40, zbspFaceTagProcess, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFaceTagFindDef", 0x00467f70, zbspFaceTagFindDef, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFaceTagRegRegister", 0x00467fa0, zbspFaceTagRegRegister, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFaceTagPreAttachAsRef", 0x00467fd0, zbspFaceTagPreAttachAsRef, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFaceTagPreAttachSmPosAsRef", 0x00468000, zbspFaceTagPreAttachSmPosAsRef, 0, DT_RET_INT,
     ngSetupTrace, 4, 0},
    {"zbspFaceTagProSwapWindow", 0x00468030, zbspFaceTagProSwapWindow, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspColSphereCreateNodesHit", 0x00468870, zbspColSphereCreateNodesHit, 0, DT_RET_VOID, ngSetupTrace, 4,
     0},
    {"zbspHitNodeslist", 0x004688a0, zbspHitNodeslist, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspTestWithFacelist", 0x004688e0, zbspTestWithFacelist, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zbspFaceBoundingBox", 0x00468b50, zbspFaceBoundingBox, 2, DT_RET_INT, ngSetupTrace2, 4, 0},
    {"zbspColSphereDraw", 0x00468b90, zbspColSphereDraw, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zbspFindFaceByType", 0x00469500, zbspFindFaceByType, 0, DT_RET_INT, ngSetupTrace, 4, 0},
};
