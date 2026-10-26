/* Collision queries (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzcollide.c). */
#include "bmx/com/zge/ngif/ngifzcollide.h"

#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "compat/crt.h"
#include "zge/com/zglrw/zrwmodelrender.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zstring.h"

static const char kCollideFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFzcollide.c";

/* What every hook stub prints: "<file> <line> <name>". */
static void trace(int line, const char *what)
{
    gZdebugPrint("%s %d %s\n", kCollideFile, line, what);
}

/* A float truncated to int16 through _ftol. */
static int16_t toShort(double v)
{
    return (int16_t)bmxFtol(v);
}

static void toShort3(int16_t *out, const ZVec3 *v)
{
    out[0] = toShort(v->x);
    out[1] = toShort(v->y);
    out[2] = toShort(v->z);
}

/* zcollideFindEdgesBehind's buffers: the list, the edge pointer cells it points at, the edge records. */
typedef PhysEdge **ZcollideEdgeList[32];
typedef PhysEdge *ZcollideEdgeCells[34];
typedef PhysEdge ZcollideEdgeRecords[33];
#define gZcollideEdgeList    ORIG_VAR(0x005f8520, ZcollideEdgeList, gZcollideEdgeList)
#define gZcollideEdgeCells   ORIG_VAR(0x005f8ce0, ZcollideEdgeCells, gZcollideEdgeCells)
#define gZcollideEdgeRecords ORIG_VAR(0x005f85a8, ZcollideEdgeRecords, gZcollideEdgeRecords)

/* Lists edge `e` in slot n of the result (through its cell). */
static void listEdge(uint32_t n, PhysEdge *e)
{
    gZcollideEdgeList[n] = &gZcollideEdgeCells[n];
    gZcollideEdgeCells[n] = e;
}

/*
 * Edges near p with at least one end behind p along dir (dot(dir, p - end) > 0; NaN never is). First the
 * level edges the renderer's collision tree reports within `radius` (rebuilt as records like
 * zbspFindTaggedEdges does), then the roller's prop edges whose line passes within `radius` of p (NaN
 * counts as within). The list holds pointers to cells holding the edge pointers. Nothing bounds it: the
 * original clears 32 slots and relies on few edges being near.
 */
BMX_ADDR(0x00469560)
PhysEdge ***zcollideFindEdgesBehind(const ZVec3 *p, const ZVec3 *dir, float radius)
{
    const ZrwRenderColQuery *q;
    uint32_t n = 0;

    zmemFill((void *)gZcollideEdgeList, 0, sizeof gZcollideEdgeList);
    q = zrwModelQuery(p, radius);
    if (q != NULL) {
        for (int i = 0; i < (int)q->found; i++) {
            const ZrwRenderColHit *r = &((const ZrwRenderColHit *)q->list)[i];
            const ZbspEdgeTag *tag = (const ZbspEdgeTag *)(uintptr_t)r->tag;
            ZVec3 s = *p;
            ZVec3 t = *p;
            PhysEdge *e;
            ZVec3 d;
            ZVec3 nrm;
            const ZVec3 up = {0.0f, 1.0f, 0.0f};
            const ZVec3 zero = {0.0f, 0.0f, 0.0f};

            zvec3Sub(&s, &s, &r->v0);
            zvec3Sub(&t, &t, &r->v1);
            if (!(zvec3Dot(dir, &s) > 0.0) && !(zvec3Dot(dir, &t) > 0.0)) {
                continue;
            }
            e = &gZcollideEdgeRecords[n];
            e->side = 0;
            listEdge(n, e);
            e->kind = (int8_t)tag->kind;
            e->a[0] = toShort(r->v0.x);
            e->a[1] = toShort(r->v0.y);
            e->a[2] = toShort(r->v0.z);
            e->b[0] = toShort(r->v1.x);
            e->b[1] = toShort(r->v1.y);
            e->b[2] = toShort(r->v1.z);
            d = r->v1;
            zvec3Sub(&d, &d, &r->v0);
            e->length = toShort(zvec3Length(&d));
            e->tag = tag->id;
            zvec3NormalizeScaled(&d, &d);
            e->dir = d;
            zvec3Cross3(&nrm, &up, &zero, &e->dir);
            e->normal = nrm;
            e->d = (float)-zvec3Dot(&nrm, &r->v0);
            n++;
        }
    }
    for (PhysEdge *e = zlistFirst(gZrollerEdges); e != NULL; e = zlistNext(gZrollerEdges)) {
        const ZVec3 a = {(float)e->a[0], (float)e->a[1], (float)e->a[2]};
        const ZVec3 b = {(float)e->b[0], (float)e->b[1], (float)e->b[2]};
        ZVec3 s = *p;
        ZVec3 d = b;
        ZVec3 u = *p;
        ZVec3 w = *p;
        double h;

        zvec3Sub(&s, &s, &a);
        zvec3Sub(&d, &d, &a);
        zvec3NormalizeScaled(&d, &d);
        h = zvec3Dot(&s, &d);
        if (__builtin_sqrt((((double)s.z * s.z + (double)s.y * s.y) + (double)s.x * s.x) - h * h) > radius) {
            continue;
        }
        zvec3Sub(&u, &u, &a);
        zvec3Sub(&w, &w, &b);
        if (zvec3Dot(dir, &u) > 0.0 || zvec3Dot(dir, &w) > 0.0) {
            listEdge(n, e);
            n++;
        }
    }
    return gZcollideEdgeList;
}

/* Flag bits of a level tag's byte 3 as the face record keeps them: bit 0 -> 4, bit 1 -> 3, bits 2-3 -> 5-6.
 */
static uint8_t tagFaceFlags(uint8_t b)
{
    return (uint8_t)(((((b & 1) << 1 | (b & 0xc)) << 1) | (b & 2)) << 2);
}

/*
 * Casts a ray from -> to. The renderer's line test runs forward from one unit before `from`, then back
 * from one unit beyond the forward hit (or `to`) to `from`; the forward hit wins. The hit record is
 * cleared, then filled: point, distance, plane, and a face record built in place. Terrain hits (kind 1) take
 * the level tag of the triangle under the point and the material through the renderer; prop hits (kind 2)
 * name the prop's collision object and become kind 1 when the prop's placement tag carries a surface.
 * Asserts on a NULL record and on a distance outside (-0.01, 1.01).
 */
BMX_ADDR(0x00469b00)
int zcollideRay(const ZVec3 *from, const ZVec3 *to, void *out)
{
    PhysRayHit *hit = out;
    PhysFace *f;
    ZcollideLineHit fwd;
    ZcollideLineHit rev;
    ZcollideLineHit best;
    ZVec3 d;
    ZVec3 start;
    ZVec3 back;
    ZVec3 end;
    int fwdHit;
    int revHit;
    int found = 0;

    d.x = (float)((double)to->x - from->x);
    d.y = (float)((double)to->y - from->y);
    d.z = (float)((double)to->z - from->z);
    zvec3NormalizeScaled(&d, &d);
    start.x = (float)((double)from->x - d.x);
    start.y = (float)((double)from->y - d.y);
    start.z = (float)((double)from->z - d.z);
    fwdHit = zrwcolLineTest(&start.x, &to->x, 3, 0, (float *)&fwd);
    back = *from;
    end = fwdHit ? fwd.point : *to;
    d.x = (float)((double)back.x - end.x);
    d.y = (float)((double)back.y - end.y);
    d.z = (float)((double)back.z - end.z);
    zvec3NormalizeScaled(&d, &d);
    start.x = (float)((double)end.x - d.x);
    start.y = (float)((double)end.y - d.y);
    start.z = (float)((double)end.z - d.z);
    revHit = zrwcolLineTest(&start.x, &back.x, 3, 0, (float *)&rev);
    if (fwdHit || revHit) {
        found = 1;
        best = fwdHit ? fwd : rev;
    }
    if (hit == NULL) {
        zdebugFail("Invalid parameter.", kCollideFile, 0x16f, 0);
    }
    /* After the assert the original clears the record anyway (and crashes on NULL). */
    zmemFill(hit, 0, sizeof *hit);
    if (!found) {
        return 0;
    }
    hit->point = best.point; // NOLINT(clang-analyzer-core.NullDereference)
    if (!(best.hit.distance > -0x1.47ae14p-7f && best.hit.distance < 0x1.028f5cp+0f)) {
        zdebugFail("Degenerate collision.", kCollideFile, 0x17c, 0);
    }
    hit->distance = best.hit.distance;
    hit->normal.x = best.hit.normal[0];
    hit->normal.y = best.hit.normal[1];
    hit->normal.z = best.hit.normal[2];
    hit->normalW = (float)-zvec3Dot((const ZVec3 *)best.hit.normal, (const ZVec3 *)best.hit.vertex[0]);
    f = &hit->faceRecord;
    if (best.hit.objectData == 0) {
        const ZrwRenderColHit *tri = zrwModelFindTriangle(&hit->point, 2.0f);
        const uint8_t *tag = tri != NULL ? (const uint8_t *)(uintptr_t)tri->tag : NULL;
        uint16_t id = 0;

        if (tag != NULL) {
            id = *(const uint16_t *)tag;
            if ((id & 0xf000) == 0x1000 && *(const uint32_t *)(tag + 8) == 0) {
                id &= 0xfff;
            }
        }
        hit->kind = 1;
        hit->onFace = 1;
        f->normal = hit->normal;
        f->d = hit->normalW;
        f->kind = 0x88;
        if (tag != NULL) {
            f->flags = tagFaceFlags(tag[3]);
        }
        f->tag = id;
        f->p0 = *(const ZVec3 *)best.hit.vertex[0];
        f->p1 = *(const ZVec3 *)best.hit.vertex[1];
        f->p2 = *(const ZVec3 *)best.hit.vertex[2];
        if (best.hit.materialData != 0) {
            hit->materialWord = *gZbspMaterialFn((const void *)(uintptr_t)best.hit.materialData);
        }
        hit->face = f;
        hit->material = &hit->materialWord;
        return found;
    }
    {
        const PhysPropFrame *frame = gZbspObjectFrameFn(best.hit.objectData);
        PhysPropNode *n;

        for (n = *zobjPropList(); n != NULL; n = n->next) {
            if (n->object == NULL) {
                zbspBuildCollObject(n);
            }
            if (n->frame == frame) {
                if (n->object != NULL) {
                    hit->kind = 2;
                    hit->objectData = (PhysCollObject *)n->object;
                    hit->object = n->object->index;
                }
                break;
            }
        }
        hit->propCopy = hit->objectData;
        hit->kind = 2;
        f->normal = hit->normal;
        f->d = hit->normalW;
        f->kind = 0x88;
        hit->face = f;
        hit->material = &hit->materialWord;
        if (n != NULL && n->tag != NULL && n->tag->range != 0) {
            hit->kind = 1;
            hit->onFace = 1;
            f->tag = n->tag->range;
        }
    }
    return found;
}

/* Material word of terrain probe hits (0x5f8518, the dword after it is cleared too). */
typedef struct ZcollideMaterial {
    uint32_t flags;
    uint32_t extra;
} ZcollideMaterial;
#define gZcollideProbeMaterial ORIG_VAR(0x005f8518, ZcollideMaterial, gZcollideProbeMaterial)

/*
 * Drops a vertical line (500 units up to 500 down) through p onto the terrain. On a hit, fills the
 * one-point face record `out` and returns the height the renderer reported; on a miss returns 10000.
 */
BMX_ADDR(0x0046a160)
double zcollideProbeHeight(const ZVec3 *p, ZcollideProbe *out)
{
    float top[4] = {p->x, 500.0f, p->z, 0.0f};
    float bottom[4] = {p->x, -500.0f, p->z, 0.0f};
    float hit[0x16];

    if (zrwcolLineTest(bottom, top, 3, 0, hit) == 0) {
        return 10000.0f;
    }
    out->kind = 2;
    out->points = &out->point;
    out->pad04 = 0;
    out->point.x = hit[0];
    out->point.y = hit[1];
    out->point.z = hit[2];
    out->pad3c = 0;
    out->count = 1;
    out->byte55 = 0;
    out->point2.x = hit[0];
    out->point2.y = hit[1];
    out->byte54 = 0;
    out->word56 = 0;
    out->point2.z = hit[2];
    out->pad4c = 0;
    out->pad50 = 0;
    out->points2 = &out->point2;
    out->pad14 = 0;
    gZcollideProbeMaterial.flags &= 0x3fe00000;
    gZcollideProbeMaterial.extra = 0;
    out->word20 = 0;
    out->pad24 = 0;
    out->pad28 = 0;
    out->pad2c = 0;
    out->material = &gZcollideProbeMaterial.flags;
    out->height = hit[0x14];
    return hit[0x14];
}

/* Allocates and lists the collision edges of a prop: 4 for shape kind 2, 1 for kinds 3 and 4. */
BMX_ADDR(0x0046a3f0)
int zcollideBuildPropEdges(PhysPropObject *object, PhysEdge **out)
{
    int count;
    int edges;

    switch (*zbspGetShapeKind(object->index)) {
    case 1:
        zdebugFail("Not coded", kCollideFile, 0x4da, 0);
        edges = 0;
        break;
    case 2:
        edges = 4;
        break;
    case 3:
    case 4:
        edges = 1;
        break;
    default:
        edges = 0;
    }
    object->edges =
        zmemFill(zmemAlloc((uint32_t)edges * sizeof(PhysEdge)), 0, (uint32_t)edges * sizeof(PhysEdge));
    count = zcollideFillPropEdges(object, object->edges);
    for (int i = 0; i < count; i++) {
        out[i] = &object->edges[i];
    }
    return count;
}

/*
 * Fills a prop's collision edges from its tree object's box: 8 corners (half width / depth, height and a
 * base cut from the source sizes), scaled, rotated and moved like the prop. Shape kind 2 gives the four
 * bottom edges of the table at 0x55d4f8 (all solid on the positive side), kind 3 one edge through the middle
 * of two sides, kind 4 the single edge at 0x55d504. Kind 1 asserts "Not coded". Returns the edge count.
 */
BMX_ADDR(0x0046a4d0)
int zcollideFillPropEdges(PhysPropObject *object, PhysEdge *edges)
{
    const ZbspShape *shape = zbspGetShape(object->index);
    const ZbspTreeObject *src = object->node->source;
    const double hx = (double)src->size[0] * 50.0f;
    const float hy = (float)((double)src->size[1] * 100.0f);
    const float hz = (float)((double)src->size[2] * 50.0f);
    const float top = (float)(((double)src->size[1] * 0.5f + src->lift) * 100.0f);
    const double bot = src->base > 0x1.4f8b58p-17f ? (double)hy - (double)src->base * 10000.0f : 0.0;
    const float lowY = (float)(bot - hy);
    ZVec3 c[8] = {
        {(float)-hx, -hy, -hz},      {(float)-hx, -hy, hz},        {(float)-hx, 0.0f, hz},
        {(float)-hx, 0.0f, -hz},     {(float)hx, lowY, -hz},       {(float)hx, lowY, hz},
        {(float)hx, (float)bot, hz}, {(float)hx, (float)bot, -hz},
    };
    ZVec3 mid[2];
    int16_t a[3];
    int16_t b[3];
    uint32_t side;

    for (int i = 0; i < 8; i++) {
        c[i].y = (float)((double)top + c[i].y);
    }
    for (int i = 0; i < 8; i++) {
        zvec3ScaleXYZ(&c[i], &c[i], object->scale[0], object->scale[1], object->scale[2]);
        zvec3RotateEuler(&c[i], &c[i], &object->rot);
        c[i].x = (float)((double)object->pos[0] + c[i].x);
        c[i].y = (float)((double)object->pos[1] + c[i].y);
        c[i].z = (float)((double)object->pos[2] + c[i].z);
    }
    mid[0].x = c[0].x;
    mid[0].y = c[0].y;
    mid[0].z = (float)(((double)c[1].z + c[0].z) * 0.5f);
    mid[1].x = c[4].x;
    mid[1].y = c[4].y;
    mid[1].z = (float)(((double)c[5].z + c[4].z) * 0.5f);
    switch (*zbspGetShapeKind(object->index)) {
    case 1:
        zdebugFail("Not coded", kCollideFile, 0x5ee, 0);
        return 0;
    case 2:
        for (int i = 0; i < 4; i++, edges++) {
            const ZcollideBoxEdge *t = &gZcollideBoxEdges[i];

            toShort3(a, &c[t->a]);
            toShort3(b, &c[t->b]);
            zcollideInitEdge(edges, shape->byte2, a, b, t->side);
            edges->prop = object;
        }
        return 4;
    case 3:
        toShort3(a, &mid[0]);
        toShort3(b, &mid[1]);
        side = 0;
        break;
    case 4:
        toShort3(a, &c[gZcollideBarEdge.a]);
        toShort3(b, &c[gZcollideBarEdge.b]);
        side = gZcollideBarEdge.side;
        break;
    default:
        return 0;
    }
    zcollideInitEdge(edges, shape->byte2, a, b, (int)side);
    edges->prop = object;
    return 1;
}

/*
 * Initialises an edge of a prop's collision box: int16 end points, rounded length, unit direction and the
 * normal (direction x up). `side` > 0 marks the solid side with flag 1, < 0 with flag 2.
 */
BMX_ADDR(0x0046aa50)
void zcollideInitEdge(PhysEdge *edge, uint32_t kind, const int16_t *a, const int16_t *b, int side)
{
    const ZVec3 down = {0.0f, -1.0f, 0.0f};
    ZVec3 n;
    uint32_t sum;
    int32_t dx, dy, dz;
    int16_t length;

    edge->side = 0;
    edge->tag = 0;
    edge->kind = (int8_t)kind;
    edge->a[0] = a[0];
    edge->a[1] = a[1];
    edge->a[2] = a[2];
    edge->b[0] = b[0];
    edge->b[1] = b[1];
    edge->b[2] = b[2];
    dx = b[0] - edge->a[0];
    dy = b[1] - edge->a[1];
    dz = b[2] - edge->a[2];
    /* The squared length is summed in 32 bits (wrapping) before the square root. */
    sum = (uint32_t)dz * (uint32_t)dz + (uint32_t)dy * (uint32_t)dy + (uint32_t)dx * (uint32_t)dx;
    length = (int16_t)bmxFtol(__builtin_sqrt((double)(int32_t)sum));
    edge->length = length;
    edge->dir.x = (float)((double)(int16_t)dx / (double)length);
    edge->dir.y = (float)((double)(int16_t)dy / (double)length);
    edge->dir.z = (float)((double)(int16_t)dz / (double)length);
    zvec3Cross3(&n, &edge->dir, &gZmathOrigin, &down);
    edge->normal = n;
    edge->d = 0.0f;
    if (side > 0) {
        edge->side |= 1;
    } else if (side < 0) {
        edge->side |= 2;
    }
}

/* Refreshes the collision edges of a prop after it turned. */
BMX_ADDR(0x0046ac40)
void zcollidePropRotated(PhysPropObject *object, const void *unused)
{
    if (object->edges != NULL) {
        zcollideFillPropEdges(object, object->edges);
    }
}

/* Refreshes the collision edges of a prop after it moved. */
BMX_ADDR(0x0046ac70)
void zcollidePropMoved(PhysPropObject *object, const void *unused)
{
    if (object->edges != NULL) {
        zcollideFillPropEdges(object, object->edges);
    }
}

/* ---- debug-print stubs ----------------------------------------------------------------------------- */

BMX_ADDR(0x0046a120)
int zcolBboxCollision(void)
{
    trace(900, "zcolbboxcollision");
    return 1;
}

BMX_ADDR(0x0046a2c0)
int zcollideGetTerrainFace(void)
{
    trace(995, "Get terrain face");
    return 0;
}

BMX_ADDR(0x0046a2f0)
int zcollidePropHitBox(void)
{
    trace(1056, "prop hit box");
    return 0;
}

BMX_ADDR(0x0046a320)
double zcollideGetPlaneHeight(void)
{
    trace(1074, "GetPlaneHeight");
    return 0.0;
}

BMX_ADDR(0x0046a360)
void zcollideGenerateBoundBox(void)
{
    trace(1167, "Generate bound box");
}

BMX_ADDR(0x0046a390)
void zcollideInitPropFacesList(void)
{
    trace(1183, "Init Prop faces list");
}

BMX_ADDR(0x0046a3c0)
void zcollideGeneratePropBoundingVolume(void)
{
    trace(1199, "Generate prop bounding volume");
}

BMX_ADDR(0x0046ac10)
void zcollidePropMove(void)
{
    trace(1592, "prop move");
}
