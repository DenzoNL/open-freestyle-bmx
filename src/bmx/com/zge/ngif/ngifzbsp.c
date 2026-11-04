/* Level geometry queries (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzbsp.c). */
#include "bmx/com/zge/ngif/ngifzbsp.h"

#include "bmx/com/action/physics.h"
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "compat/crt.h"
#include "zge/com/zglrw/zrwcol.h"
#include "zge/com/zglrw/zrwmodelrender.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zgemods/ztaginf.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zquat.h"
#include "zge/zstring.h"

static const char kBspFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFzbsp.c";

/* What every hook stub prints: "<file> <line> <name>". */
static void trace(int line, const char *what)
{
    gZdebugPrint("%s %d %s\n", kBspFile, line, what);
}

/* Reads pos from the frame like the original (a float truncated to int16 through _ftol). */
static int16_t toShort(double v)
{
    return (int16_t)bmxFtol(v);
}

/* x87 FCOM followed by `TEST AH,1`: true for a < b and for unordered operands (NaN). */
static int lessOrUnordered(double a, double b)
{
    return !(a >= b);
}

/* Sign extends to 32 bits and wraps like the original's integer arithmetic. */
#define U(x) ((uint32_t)(int32_t)(x))

/* Appends `item` to a hit list and keeps it NULL terminated. */
static void appendHit(void **list, uint32_t index, void *item)
{
    list[index] = item;
    list[index + 1] = NULL;
}

/* Sets a prop's orientation (Euler angles), tells the renderer and refreshes its collision edges. */
BMX_ADDR(0x004661c0)
void zbspPropSetRotation(PhysPropObject *object, const ZAngle3 *rot)
{
    PhysPropFrame *frame = object->node->frame;
    ZQuat q;

    zquatFromEuler(&q, rot, 0);
    zobjSetRotation(frame, &q);
    if (!((object->flags & kZbspObjFlagged) && (object->flags & kZbspObjSignalled))) {
        zcollidePropRotated(object, rot);
    }
}

/* Turns a prop by `delta` (16 bit wrapping add on the current Euler angles). */
BMX_ADDR(0x00466230)
void zbspPropRotate(PhysPropObject *object, const ZAngle3 *delta)
{
    ZAngle3 e;

    zquatToEuler(&e, &object->node->frame->rot, 0);
    e.x = (ZAngle)(e.x + delta->x);
    e.y = (ZAngle)(e.y + delta->y);
    e.z = (ZAngle)(e.z + delta->z);
    zbspPropSetRotation(object, &e);
}

/* Moves a prop: stores the int16 position, tells the renderer and refreshes the collision edges. */
BMX_ADDR(0x004662b0)
void zbspPropSetPosition(PhysPropObject *object, const ZVec3 *pos)
{
    object->pos[0] = toShort(pos->x);
    object->pos[1] = toShort(pos->y);
    object->pos[2] = toShort(pos->z);
    zobjSetPosition(object->node->frame, pos);
    zcollidePropMoved(object, pos);
}

/*
 * Builds the collision shape of a tree object (kept in obj->shape): the definition's kind 1..4 becomes
 * shape kind 2, 3, 1, 4, and the box is derived from the object's extents.
 */
BMX_ADDR(0x00466390)
void zbspBuildShape(ZbspTreeObject *obj)
{
    const ZbspObjectDef *def = obj->def;
    ZbspShape *s;

    if (def == NULL) {
        return;
    }
    s = zmemFill(zmemAlloc(sizeof *s), 0, sizeof *s);
    obj->shape = s;
    s->flags &= 0xfe00;
    s->byte2 = 0;
    s->word4 = 0;
    s->byte6 = 0;
    s->byte7 = 0;
    switch (def->kind) {
    case 1:
        s->kind = 2;
        break;
    case 2:
        s->kind = 3;
        break;
    case 3:
        s->kind = 1;
        break;
    case 4:
        s->kind = 4;
        break;
    default:
        s->kind = 0;
    }
    if (s->kind == 0) {
        s->box[0] = 0.0f;
        s->box[1] = 0.0f;
        s->box[2] = 0.0f;
        s->box[3] = 0.0f;
    } else if (s->kind < 5) {
        s->box[0] = (float)((double)obj->size[0] * 50.0f);
        s->box[1] = (float)((double)obj->size[1] * 100.0f);
        s->box[2] = (float)((double)obj->size[2] * 50.0f);
        s->box[3] = (float)((((double)obj->size[1] * 0.5f + obj->base) + obj->lift) * 100.0f);
    }
}

/* Position of a tree object in the list (building the shapes on the way), -1 when absent. */
BMX_ADDR(0x004664e0)
int zbspTreeObjectIndex(const ZbspTreeObject *obj)
{
    int index = 0;

    for (ZbspTreeNode *n = *zobjTreeObjectList(); n != NULL; n = n->next, index++) {
        if (n->object.shape == NULL) {
            zbspBuildShape(&n->object);
        }
        if (obj == &n->object) {
            return index;
        }
    }
    return -1;
}

/* The `index`-th tree object, building every shape up to it; NULL when the list is shorter. */
BMX_ADDR(0x00466550)
ZbspTreeObject *zbspTreeObjectAt(int index)
{
    int i = 0;

    for (ZbspTreeNode *n = *zobjTreeObjectList(); n != NULL; n = n->next, i++) {
        if (n->object.shape == NULL) {
            zbspBuildShape(&n->object);
        }
        if (i == index) {
            return &n->object;
        }
    }
    return NULL;
}

BMX_ADDR(0x004665c0)
ZbspShape *zbspGetShape(int index)
{
    ZbspTreeObject *obj = zbspTreeObjectAt((uint16_t)index);

    if (obj == NULL || obj->shape == NULL) {
        return NULL;
    }
    return obj->shape;
}

/* Pointer to the kind byte of a tree object's shape; a zeroed static byte when it has none. */
BMX_ADDR(0x00469530)
uint8_t *zbspGetShapeKind(int index)
{
    ZbspShape *shape = zbspGetShape(index);

    if (shape == NULL) {
        gZbspNoShapeKind = 0;
        return &gZbspNoShapeKind;
    }
    return &shape->kind;
}

BMX_ADDR(0x004665f0)
int zbspReturnZero(void)
{
    return 0;
}

/* Reacts to a prop event: 0, 1 set marker flags, 2 / 3 toggle a renderer flag, 4 / 5 hide / show. */
BMX_ADDR(0x00466610)
void zbspObjectSignal(PhysPropObject *object, int signal)
{
    PhysPropFrame *frame = object->node->frame;

    switch (signal) {
    case 0:
        object->flags |= kZbspObjSignalled;
        break;
    case 1:
        object->flags |= kZbspObjTouched;
        break;
    case 2:
        gZobjSetFlags(frame->model, 8, 0);
        object->flags |= kZbspObjFlagged;
        break;
    case 3:
        gZobjSetFlags(frame->model, 0, 8);
        object->flags &= (uint16_t)~kZbspObjFlagged;
        break;
    case 4:
        zobjSetVisible(frame, 0);
        object->flags |= kZbspObjHidden;
        break;
    case 5:
        zobjSetVisible(frame, 1);
        object->flags &= (uint16_t)~kZbspObjHidden;
        break;
    default:
        break;
    }
}

/* Static copy of the first word of a model's material data, NULL when it has none. */
BMX_ADDR(0x00466bb0)
const uint32_t *zbspGetMatRef(const void *obj)
{
    const void *material = ((const PhysFace *)obj)->owner->materialData;

    if (material == NULL) {
        return NULL;
    }
    gZbspMatRefValue = *gZbspMaterialFn(material);
    return &gZbspMatRefValue;
}

/* Centroid of a face record's three corners. */
BMX_ADDR(0x00466c50)
void zbspFaceCentroid(const void *face, ZVec3 *out)
{
    const PhysFace *f = face;
    const ZVec3 *corner[3] = {&f->p0, &f->p1, &f->p2};

    zvec3Zero(out);
    for (int i = 0; i < 3; i++) {
        zvec3Add(out, out, corner[i]);
    }
    zvec3Scale(out, out, 0x1.555556p-2f);
}

/* Point of the segment a-b closest to p: p's projection on the line, clamped to the end points. */
BMX_ADDR(0x00466d70)
void zbspClosestPointOnSegment(const ZVec3 *a, const ZVec3 *b, const ZVec3 *p, ZVec3 *out)
{
    ZVec3 d;
    double s;
    double t;

    d.x = (float)((double)b->x - a->x);
    d.y = (float)((double)b->y - a->y);
    d.z = (float)((double)b->z - a->z);
    zvec3NormalizeScaled(&d, &d);
    s = (((double)a->z - p->z) * d.z + ((double)a->y - p->y) * d.y) + ((double)a->x - p->x) * d.x;
    if (!lessOrUnordered(s, 0.0)) {
        *out = *a;
        return;
    }
    t = (((double)b->z - p->z) * d.z + ((double)b->y - p->y) * d.y) + ((double)b->x - p->x) * d.x;
    if (t > 0.0) {
        out->x = (float)((double)b->x - t * d.x);
        out->y = (float)((double)b->y - t * d.y);
        out->z = (float)((double)b->z - t * d.z);
        return;
    }
    *out = *b;
}

/* Which side of an edge is solid: 1 (bit 0), -1 (bit 1) or 0. */
BMX_ADDR(0x00467690)
int zbspEdgeSide(const void *edge)
{
    const uint8_t side = *(const uint8_t *)edge;

    if (side & 1) {
        return 1;
    }
    return -(int)((side & 2) != 0);
}

/*
 * Point (x, y, z) inside the face outline? The test works in the int16 projection along the normal's
 * dominant axis and walks the vertex list backwards (3 corners, 4 with the quad flag).
 */
BMX_ADDR(0x00467a90)
int zbspPointInFace(const PhysFace *face, float x, float y, float z)
{
    const int32_t ix = bmxFtol(x);
    const int32_t iy = bmxFtol(y);
    const int32_t iz = bmxFtol(z);
    const float ny = (float)__builtin_fabs(face->normal.y);
    const float n[3] = {face->normal.x, face->normal.y, face->normal.z};
    int axis;
    int32_t sign;
    int count;

    if (lessOrUnordered(__builtin_fabs(face->normal.x), ny)) {
        axis = __builtin_fabs(face->normal.z) > ny ? 2 : 1;
    } else {
        axis = lessOrUnordered(__builtin_fabs(face->normal.x), __builtin_fabs(face->normal.z)) ? 2 : 0;
    }
    if (n[axis] > 0x1.4f8b58p-17f) {
        sign = 1;
    } else if (!lessOrUnordered(n[axis], -0x1.4f8b58p-17f)) {
        sign = 0;
    } else {
        sign = -1;
    }
    sign = -sign;
    count = 3 + (face->flags & 1);
    {
        const int16_t *cur = face->vert[0];
        void *const *next = &face->vert[count - 1];

        for (int i = 0; i < count; i++, next--) {
            const int16_t *nxt = *next;
            int32_t cross;

            /* 32 bit wrapping arithmetic, like the original's IMUL / SUB. */
            if (axis == 0) {
                cross = (int32_t)((U(iy) - U(nxt[1])) * (U(cur[2]) - U(nxt[2])) -
                                  (U(iz) - U(nxt[2])) * (U(cur[1]) - U(nxt[1])));
            } else if (axis == 1) {
                cross = (int32_t)((U(cur[0]) - U(nxt[0])) * (U(iz) - U(nxt[2])) -
                                  (U(ix) - U(nxt[0])) * (U(cur[2]) - U(nxt[2])));
            } else {
                cross = (int32_t)((U(ix) - U(nxt[0])) * (U(cur[1]) - U(nxt[1])) -
                                  (U(cur[0]) - U(nxt[0])) * (U(iy) - U(nxt[1])));
            }
            if (cross != 0 && (cross ^ sign) < 0) {
                return 0;
            }
            cur = nxt;
        }
    }
    return 1;
}

/* Point of the segment a-b (int16 end points, unit direction dir) closest to p. */
BMX_ADDR(0x00468920)
void zbspClosestPointOnEdge(const int16_t *a, const int16_t *b, const ZVec3 *dir, const ZVec3 *p, ZVec3 *out)
{
    int32_t ix = bmxFtol((double)a[0] - p->x);
    int32_t iy = bmxFtol((double)a[1] - p->y);
    int32_t iz = bmxFtol((double)a[2] - p->z);
    double s = ((double)ix * dir->x + (double)iz * dir->z) + (double)iy * dir->y;
    float sf;

    if (!lessOrUnordered(s, 0.0)) {
        out->x = (float)a[0];
        out->y = (float)a[1];
        out->z = (float)a[2];
        return;
    }
    ix = bmxFtol((double)b[0] - p->x);
    iy = bmxFtol((double)b[1] - p->y);
    iz = bmxFtol((double)b[2] - p->z);
    s = ((double)ix * dir->x + (double)iz * dir->z) + (double)iy * dir->y;
    sf = (float)s;
    if (s > 0.0) {
        /* The overshoot beyond b is truncated to whole units. */
        out->x = (float)(int32_t)(U(b[0]) - U(bmxFtol((double)sf * dir->x)));
        out->y = (float)(int32_t)(U(b[1]) - U(bmxFtol((double)sf * dir->y)));
        out->z = (float)(int32_t)(U(b[2]) - U(bmxFtol((double)sf * dir->z)));
        return;
    }
    out->x = (float)b[0];
    out->y = (float)b[1];
    out->z = (float)b[2];
}

/*
 * Builds the collision object of a prop node (500 bytes, kept in node->object): position, unit scale,
 * flags from the placement tag, orientation and the index of the source tree object. The node's destroy
 * callback becomes zbspFreeCollObject.
 */
BMX_ADDR(0x00468bc0)
void zbspBuildCollObject(PhysPropNode *node)
{
    const PhysPropTag *tag = node->tag;
    PhysPropObject *o = zmemFill(zmemAlloc(sizeof(PhysPropObject)), 0, sizeof(PhysPropObject));

    node->object = o;
    node->destroy = BMX_FNPTR(void(BMX_CDECL *)(PhysPropObject *), 0x00402414, zbspFreeCollObject);
    o->pos[0] = toShort(node->frame->pos.x);
    o->pos[1] = toShort(node->frame->pos.y);
    o->pos[2] = toShort(node->frame->pos.z);
    o->scale[0] = 1.0f;
    o->scale[1] = 1.0f;
    o->scale[2] = 1.0f;
    if (tag == NULL) {
        o->flags &= 0xe3e0;
    } else {
        o->flags = (uint16_t)((tag->group & kZbspObjGroupMask) | ((tag->flag6 & 1) << 10) |
                              ((tag->flag4 & 1) << 11) | ((tag->flag5 & 1) << 12));
    }
    o->edges = NULL;
    zquatToEuler(&o->rot, &node->frame->rot, 0);
    o->index = (uint16_t)zbspTreeObjectIndex(node->source);
    o->node = node;
}

/* Frees a prop collision object with its edge array (and unlinks the edges from the roller's list). */
BMX_ADDR(0x00468d20)
void zbspFreeCollObject(PhysPropObject *object)
{
    int edges;

    if (object == NULL) {
        return;
    }
    /* The original also compares the 16 bit index against -1, which can never match. */
    switch (*zbspGetShapeKind(object->index)) {
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
    if (object->edges != NULL && edges > 0) {
        for (int i = 0; i < edges; i++) {
            void *e = zlistFirst(gZrollerEdges);

            while (e != NULL) {
                if (e == &object->edges[i]) {
                    zlistRemoveCurrent(gZrollerEdges);
                    break;
                }
                e = zlistNext(gZrollerEdges);
            }
        }
        zmemFree(object->edges);
    }
    zmemFree(object);
}

/* Collision objects of the props whose tag group lies in [lo, hi] (16 bit compare), NULL when none. */
BMX_ADDR(0x00468e30)
void **zbspFindPropsNear(int lo, int hi)
{
    uint32_t count = 0;

    for (PhysPropNode *n = *zobjPropList(); n != NULL; n = n->next) {
        if (n->object == NULL) {
            zbspBuildCollObject(n);
        }
        if (n->tag != NULL) {
            const uint16_t group = n->tag->group;

            if (group >= (uint16_t)lo && group <= (uint16_t)hi) {
                if (count >= 0x100) {
                    zdebugFail("prop list overflow", kBspFile, 0xf21, 0);
                }
                appendHit(gZbspHits, count, n->object);
                count++;
            }
        }
    }
    return count != 0 ? gZbspHits : NULL;
}

/* Vertex k of a level face (the vertices are 0x10 bytes apart). */
static const ZVec3 *faceVertex(const ZbspLevelFace *face, int k)
{
    return (const ZVec3 *)((const uint8_t *)face + (size_t)k * 0x10);
}

/*
 * Fills the static edge record for edge `k` of a level face: int16 end points, length, tag id, unit
 * direction, plane normal (direction x up) and plane distance. Shared by zbspFindEdge and
 * zbspFindTaggedEdges. The record's prop pointer (+0x34) is left alone.
 */
static void fillEdgeRecord(PhysEdge *e, const ZbspLevelFace *face, int k, const ZbspEdgeTag *tag)
{
    const ZVec3 *a = faceVertex(face, k);
    const ZVec3 *b = faceVertex(face, (k + 1) % 3);
    ZVec3 d = *b;
    ZVec3 up = {0.0f, 1.0f, 0.0f};
    ZVec3 zero = {0.0f, 0.0f, 0.0f};
    ZVec3 n;

    e->side = 0;
    e->kind = (int8_t)tag->kind;
    e->a[0] = toShort(a->x);
    e->a[1] = toShort(a->y);
    e->a[2] = toShort(a->z);
    e->b[0] = toShort(b->x);
    e->b[1] = toShort(b->y);
    e->b[2] = toShort(b->z);
    zvec3Sub(&d, &d, a);
    e->length = toShort(zvec3Length(&d));
    e->tag = tag->id;
    zvec3NormalizeScaled(&d, &d);
    e->dir = d;
    zvec3Cross3(&n, &up, &zero, &e->dir);
    e->normal = n;
    e->d = (float)-zvec3Dot(&n, a);
}

/* The `index`-th edge (counting in level order) tagged `id`; rebuilt in a static record. */
BMX_ADDR(0x00467410)
void *zbspFindEdge(int unused, int id, int index, int unused2)
{
    const uint32_t count = ztaginfGetFaceCount();
    ZbspLevelFace *face = ztaginfGetFaces();
    int found = 0;

    for (uint32_t f = 0; count != 0 && f < count; f++, face++) {
        for (int k = 0; k < 3; k++) {
            const ZbspEdgeTag *tag = face->edgeTag[k];

            if (tag != NULL && tag->id == (uint16_t)id) {
                if (found == index) {
                    fillEdgeRecord(&gZbspEdgeRecord, face, k, tag);
                    return &gZbspEdgeRecord;
                }
                found++;
            }
        }
    }
    return NULL;
}

/*
 * Builds the face record of a level triangle. The int16 corner copies are partly mixed up in the original:
 * q0 gets (p0.x, p2.y, p2.z), q1 and q2 only their x.
 */
BMX_ADDR(0x00469080)
void zbspBuildFace(ZbspLevelFace *src, const uint16_t *tag, PhysFace *out)
{
    ZVec3 n;

    out->flags = 0;
    out->tag = *tag;
    zvec3Cross3(&n, &src->v0, &src->v1, &src->v2);
    zvec3NormalizeScaled(&n, &n);
    out->normal = n;
    out->d = (float)zvec3Dot(&src->v0, &n);
    out->p0 = src->v0;
    out->p1 = src->v1;
    out->p2 = src->v2;
    out->vert[0] = out->q0;
    out->q0[0] = toShort(out->p0.x);
    out->vert[1] = out->q1;
    out->q1[0] = toShort(out->p1.x);
    out->vert[2] = out->q2;
    out->q2[0] = toShort(out->p2.x);
    out->q0[1] = toShort(out->p2.y);
    out->q0[2] = toShort(out->p2.z);
    out->vert[3] = src;
    out->kind = 0;
    out->object = NULL;
}

/*
 * Faces whose tag id (low 12 bits) lies in [lo, hi]: face records of the level triangles (built on first
 * use) followed by one record per prop in range, whose +0x2c names the prop's collision object. The
 * result lives in a static buffer.
 */
BMX_ADDR(0x00468ef0)
void **zbspFindTaggedFaces(int lo, int hi)
{
    uint32_t n = 0;
    uint32_t slot = 0;
    uint32_t count;
    ZbspLevelFace *face;

    gZbspHits[0] = NULL;
    count = ztaginfGetFaceCount();
    face = ztaginfGetFaces();
    if (count == 0) {
        return gZbspHits;
    }
    for (; count != 0; count--, face++) {
        const uint16_t *tag = face->tag;

        if (tag != NULL && lo <= (*tag & 0xfff) && (*tag & 0xfff) <= hi) {
            if (face->face == NULL) {
                face->face = zmemFreeListAlloc(gZbspFacePool);
            }
            n++;
            zbspBuildFace(face, tag, face->face);
            if (n >= 0x100) {
                zdebugFail("prop list overflow", kBspFile, 0xf61, 0);
            }
            appendHit(gZbspHits, slot, face->face);
            slot++;
        }
    }
    for (PhysPropNode *node = *zobjPropList(); node != NULL; node = node->next) {
        if (node->object == NULL) {
            zbspBuildCollObject(node);
        }
        if (node->tag != NULL) {
            const int id = node->tag->range & 0xfff;

            if (lo <= id && id <= hi) {
                PhysFace *rec = (PhysFace *)&gZbspPropFaces + gZbspPropFaceCount++;

                rec->object = node->object;
                appendHit(gZbspHits, slot, rec);
                slot++;
            }
        }
    }
    return gZbspHits;
}

/* Edge records of the level whose tag id lies in [lo, hi] (16 bit compare); cached per edge. */
BMX_ADDR(0x004691c0)
void **zbspFindTaggedEdges(int lo, int hi)
{
    uint32_t count = ztaginfGetFaceCount();
    ZbspLevelFace *face;
    uint32_t slot = 0;

    gZbspHits[0] = NULL;
    face = ztaginfGetFaces();
    if (count == 0) {
        return gZbspHits;
    }
    if (gZbspEdgePool == NULL) {
        gZbspEdgePool = zmemFreeListCreate(sizeof(PhysEdge), 0x100, NULL);
    }
    for (; count != 0; count--, face++) {
        for (int k = 0; k < 3; k++) {
            const ZbspEdgeTag *tag = face->edgeTag[k];

            if (tag != NULL && tag->kind != 0 && tag->id >= (uint16_t)lo && tag->id <= (uint16_t)hi) {
                PhysEdge *e;

                if (face->edge[k] == NULL) {
                    face->edge[k] = zmemFreeListAlloc(gZbspEdgePool);
                }
                e = face->edge[k];
                fillEdgeRecord(e, face, k, tag);
                e->prop = NULL;
                if (slot >= 0x100) {
                    zdebugFail("edge list overflow", kBspFile, 0xff6, 0);
                }
                appendHit(gZbspHits, slot, e);
                slot++;
            }
        }
    }
    return gZbspHits;
}

/*
 * Turns `q` into its offset from p (stored as floats) and returns the squared length. The z offset is
 * squared unrounded against its rounded copy, like the original's FST / FMUL.
 */
static double offsetLengthSq(ZVec3 *q, const ZVec3 *p)
{
    double dz;

    q->x = (float)((double)q->x - p->x);
    q->y = (float)((double)q->y - p->y);
    dz = (double)q->z - p->z;
    q->z = (float)dz;
    return (dz * q->z + (double)q->y * q->y) + (double)q->x * q->x;
}

/*
 * Edges within `radius` of p: first the tagged level edges (kind != 0; their records are cached per face
 * edge and rebuilt), then the roller's prop edges whose object is neither flagged, signalled, touched nor
 * hidden. The prop edges are only listed when the level has faces. A NaN distance counts as near.
 */
BMX_ADDR(0x00466ed0)
void **zbspFindEdgesNear(const ZVec3 *p, float radius)
{
    const float r2 = (float)((double)radius * radius);
    void **cursor = &gZbspHits[0];
    uint32_t count;
    ZbspLevelFace *face;

    gZbspHits[0] = NULL;
    count = ztaginfGetFaceCount();
    face = ztaginfGetFaces();
    if (count == 0) {
        return gZbspHits;
    }
    if (gZbspEdgePool == NULL) {
        gZbspEdgePool = zmemFreeListCreate(sizeof(PhysEdge), 0x100, NULL);
    }
    for (; count != 0; count--, face++) {
        for (int k = 0; k < 3; k++) {
            const ZbspEdgeTag *tag = face->edgeTag[k];
            ZVec3 q;
            PhysEdge *e;

            if (tag == NULL || tag->kind == 0) {
                continue;
            }
            zbspClosestPointOnSegment(faceVertex(face, k), faceVertex(face, (k + 1) % 3), p, &q);
            if (!lessOrUnordered(offsetLengthSq(&q, p), r2)) {
                continue;
            }
            if (face->edge[k] == NULL) {
                face->edge[k] = zmemFreeListAlloc(gZbspEdgePool);
            }
            e = face->edge[k];
            fillEdgeRecord(e, face, k, tag);
            e->prop = NULL;
            if (cursor >= &gZbspHits[0x100]) {
                zdebugFail("edge list overflow", kBspFile, 0x91f, 0);
            }
            cursor[0] = e;
            cursor[1] = NULL;
            cursor++;
        }
    }
    for (PhysEdge *e = zlistFirst(gZrollerEdges); e != NULL; e = zlistNext(gZrollerEdges)) {
        const uint16_t flags = ((const PhysPropObject *)e->prop)->flags;
        ZVec3 a;
        ZVec3 b;
        ZVec3 q;

        if (flags & (kZbspObjFlagged | kZbspObjSignalled | kZbspObjTouched | kZbspObjHidden)) {
            continue;
        }
        a.x = (float)e->a[0];
        a.y = (float)e->a[1];
        a.z = (float)e->a[2];
        b.x = (float)e->b[0];
        b.y = (float)e->b[1];
        b.z = (float)e->b[2];
        zbspClosestPointOnSegment(&a, &b, p, &q);
        if (lessOrUnordered(offsetLengthSq(&q, p), r2)) {
            if (cursor >= &gZbspHits[0x100]) {
                zdebugFail("edge list overflow", kBspFile, 0x940, 0);
            }
            cursor[0] = e;
            cursor[1] = NULL;
            cursor++;
        }
    }
    return gZbspHits;
}

/* Corner i of a renderer hit. */
static const ZVec3 *hitCorner(const ZrwColHit *hit, int i)
{
    return (const ZVec3 *)(const void *)hit->vertex[i];
}

/* Face flag bits from byte 3 of a level tag: bit 0 -> 4, bit 1 -> 3, bits 2-3 -> 5-6. */
static uint8_t tagFaceFlags(uint8_t b)
{
    return (uint8_t)(((((b & 1) << 1 | (b & 0xc)) << 1) | (b & 2)) << 2);
}

/* Tag id of a level tag: the low 12 bits for 0x1xxx tags without a link (+8), else the whole word. */
static uint16_t tagFaceId(const uint8_t *tag)
{
    uint16_t id = *(const uint16_t *)tag;

    if ((id & 0xf000) == 0x1000 && *(const uint32_t *)(tag + 8) == 0) {
        id &= 0xfff;
    }
    return id;
}

/*
 * Faces near p: the renderer's sphere test (radius + 1) lists triangles, last first; each becomes a face
 * record (normal, plane, int16 corners, tag and flags of the level triangle under its centroid). A face is
 * kept when |normal.y| <= cosLimit + 0.001, p lies less than `radius` above its plane (props' faces also
 * not below it), and the projection of p is inside it or within the remaining radius of a corner or an edge
 * (tested in whole units). The hit gets the plane distance, the closest point and the prop's collision
 * object. A rejected triangle still overwrites the (reused) record slot. NULL when nothing is kept.
 */
BMX_ADDR(0x00468070)
void *zbspFindFaceNear(const ZVec3 *p, float radius, float cosLimit)
{
    PhysFace *face = gZbspNearFaces;
    ZbspNearCorners *corners = gZbspNearCorners;
    ZbspFaceHit *out = gZbspNearHits;
    uint32_t count = 0;

    if (!zrwcolSphereTest(&p->x, (float)((double)radius + 1.0), 3, NULL, (uint32_t *)&gZbspSphere)) {
        return NULL;
    }
    zmemFill(&gZbspFaceQuery, 0, sizeof gZbspFaceQuery);
    for (uint32_t i = gZbspSphere.count; i != 0; i--) {
        const ZrwColHit *src = &gZbspSphere.hits[i - 1];
        PhysPropObject *object = NULL;
        const ZrwRenderColHit *tri;
        ZVec3 center;
        ZVec3 proj;
        ZVec3 point;
        double dist;
        float distf;
        float r2;
        int last;

        if (face >= &gZbspNearFaces[kZbspNearFaceMax]) {
            continue;
        }
        if (src->objectData != NULL) {
            const PhysPropFrame *frame = gZbspObjectFrameFn(src->objectData);

            for (PhysPropNode *n = *zobjPropList(); n != NULL; n = n->next) {
                if (n->object == NULL) {
                    zbspBuildCollObject(n);
                }
                if (n->frame == frame) {
                    object = n->object;
                    break;
                }
            }
        }
        face->flags = 0;
        face->kind = 0;
        face->tag = 0;
        center.x =
            (float)((((double)src->vertex[1][0] + src->vertex[2][0]) + src->vertex[0][0]) * 0x1.555556p-2f);
        center.y =
            (float)((((double)src->vertex[1][1] + src->vertex[2][1]) + src->vertex[0][1]) * 0x1.555556p-2f);
        center.z =
            (float)((((double)src->vertex[2][2] + src->vertex[0][2]) + src->vertex[1][2]) * 0x1.555556p-2f);
        tri = zrwModelFindTriangle(&center, 2.0f);
        if (tri != NULL) {
            const uint8_t *tag = (const uint8_t *)(uintptr_t)tri->tag;
            uint16_t id = 0;

            if (tag != NULL) {
                face->flags |= tagFaceFlags(tag[3]);
                id = tagFaceId(tag);
            }
            face->tag = id;
        }
        face->normal.x = src->normal[0];
        face->normal.y = src->normal[1];
        face->normal.z = src->normal[2];
        dist = zvec3Dot((const ZVec3 *)src->normal, hitCorner(src, 0));
        face->owner = src;
        face->d = (float)-dist;
        face->vert[0] = corners->v[0];
        face->vert[1] = corners->v[1];
        face->vert[2] = corners->v[2];
        for (int k = 0; k < 3; k++) {
            corners->v[k][0] = toShort(hitCorner(src, k)->x);
            corners->v[k][1] = toShort(hitCorner(src, k)->y);
            corners->v[k][2] = toShort(hitCorner(src, k)->z);
        }
        if (lessOrUnordered((double)cosLimit + 0.001, __builtin_fabs(face->normal.y))) {
            continue;
        }
        dist = (((double)p->x * face->normal.x + (double)p->z * face->normal.z) +
                (double)face->normal.y * p->y) +
               face->d;
        distf = (float)dist;
        if (lessOrUnordered(dist, 0.0) && object != 0) {
            continue;
        }
        if (!lessOrUnordered(distf, radius)) {
            continue;
        }
        r2 = (float)((double)radius * radius - (double)distf * distf);
        proj.x = (float)((double)p->x - (double)distf * src->normal[0]);
        proj.y = (float)((double)p->y - (double)distf * src->normal[1]);
        proj.z = (float)((double)p->z - (double)distf * src->normal[2]);
        if (zbspPointInFace(face, proj.x, proj.y, proj.z)) {
            point = proj;
            goto keep;
        }
        last = (face->flags & 1) + 2;
        {
            const int32_t ix = bmxFtol(proj.x);
            const int32_t iy = bmxFtol(proj.y);
            const int32_t iz = bmxFtol(proj.z);

            for (int k = last; k >= 0; k--) {
                const int16_t *v = face->vert[k];
                const uint32_t dx = U(v[0]) - U(ix);
                const uint32_t dy = U(v[1]) - U(iy);
                const uint32_t dz = U(v[2]) - U(iz);

                if (lessOrUnordered((double)(int32_t)(dz * dz + dy * dy + dx * dx), r2)) {
                    point = *hitCorner(src, k);
                    goto keep;
                }
            }
        }
        for (int k = last; k >= 0; k--) {
            const int16_t *va = face->vert[k];
            const int16_t *vb = face->vert[(k + 1) % 3];
            const ZVec3 a = {(float)va[0], (float)va[1], (float)va[2]};
            const ZVec3 b = {(float)vb[0], (float)vb[1], (float)vb[2]};
            ZVec3 q;

            zbspClosestPointOnSegment(&a, &b, &proj, &q);
            if (lessOrUnordered((double)bmxFtol(offsetLengthSq(&q, &proj)), r2)) {
                point.x = (float)((double)q.x + proj.x);
                point.y = (float)((double)q.y + proj.y);
                point.z = (float)((double)q.z + proj.z);
                goto keep;
            }
        }
        continue;
    keep:
        out->face = face;
        out->object = object;
        out->dist = distf;
        out->point = point;
        out++;
        face++;
        corners++;
        count++;
    }
    if (count == 0) {
        return NULL;
    }
    if (count >= 0x100) {
        zdebugFail("Buffer overflow", kBspFile, 0xde9, 0);
    }
    gZbspFaceQuery.hits = gZbspNearHits;
    gZbspNearHits[count].face = NULL;
    return &gZbspFaceQuery;
}

/* ---- debug-print stubs ----------------------------------------------------------------------------- */

BMX_ADDR(0x00465a50)
int zbspInit(void)
{
    trace(107, "bsp init");
    return 0;
}

BMX_ADDR(0x00465a80)
void zbspUpdate(void)
{
    trace(126, "bsp update");
}

BMX_ADDR(0x00465ab0)
int zbspShutdown(void)
{
    trace(142, "shutdown");
    return 0;
}

BMX_ADDR(0x00465ae0)
int zbspInitializeScene(void)
{
    trace(165, "initialize scene");
    return 0;
}

BMX_ADDR(0x00465b10)
int zbspLoadScene(void)
{
    trace(186, "load scene");
    return 0;
}

BMX_ADDR(0x00465b40)
void zbspLoadAndProcessMap(void)
{
    trace(204, "load and process map");
}

BMX_ADDR(0x00465b70)
int zbspAllocAndLoadMap(void)
{
    trace(221, "alloc and load map");
    return 0;
}

BMX_ADDR(0x00465ba0)
void zbspFreeMap(void)
{
    trace(238, "free map");
}

BMX_ADDR(0x00465bd0)
int zbspSetTreeVisible(void)
{
    trace(270, "set tree visible");
    return 0;
}

BMX_ADDR(0x00465c00)
void *zbspConvertTreeFromLoad(void *tree)
{
    trace(574, "convert tree from load");
    return tree;
}

BMX_ADDR(0x00465c40)
void zbspRelativeLodZ(void)
{
    trace(592, "relative lod z");
}

BMX_ADDR(0x00465c70)
void zbspPreProcessScene(void)
{
    trace(613, "preprocess scene");
}

BMX_ADDR(0x00465ca0)
void zbspPreProcessTerrainNode(void)
{
    trace(627, "preprocess terrain node");
}

BMX_ADDR(0x00465cd0)
void zbspPreProcessFaceList(void)
{
    trace(641, "preprocess face list");
}

BMX_ADDR(0x00465d00)
int zbspViewerInPosSide(void)
{
    trace(702, "viewer in pos side");
    return 1;
}

BMX_ADDR(0x00465d40)
void zbspSndRegister(void)
{
    trace(725, "snd register");
}

BMX_ADDR(0x00465d70)
void zbspSndSetState(void)
{
    trace(743, "snd set state");
}

BMX_ADDR(0x00465da0)
int zbspSndGetHandle(void)
{
    trace(760, "snd get handle");
    return 0;
}

BMX_ADDR(0x00465dd0)
int zbspDvliteHandle(void)
{
    trace(782, "dvlitehandle");
    return 0;
}

BMX_ADDR(0x00465e00)
int zbspDvliteGetColor(void)
{
    trace(800, "dvlitegetcolor");
    return 1;
}

BMX_ADDR(0x00465e40)
void zbspDvliteLite(void)
{
    trace(819, "dvlitelite");
}

BMX_ADDR(0x00465e70)
void zbspDvliteInitialize(void)
{
    trace(836, "dvliteinitialize");
}

BMX_ADDR(0x00465ea0)
void zbspDvliteAttachToProps(void)
{
    trace(852, "dvlite attach to props");
}

BMX_ADDR(0x00465ed0)
void zbspPropActivate(void)
{
    trace(875, "prop activate");
}

BMX_ADDR(0x00465f00)
void zbspPropDeactivate(void)
{
    trace(890, "prop deactivate");
}

BMX_ADDR(0x00465f30)
void zbspActivateTree(void)
{
    trace(904, "activate tree");
}

BMX_ADDR(0x00465f60)
void zbspDeactivateTree(void)
{
    trace(918, "deactivate tree");
}

BMX_ADDR(0x00465f90)
int zbspInitializeTreeRef(void)
{
    trace(937, "initialize tree ref");
    return 0;
}

BMX_ADDR(0x00465fc0)
int16_t zbspCreateTreeRef(void)
{
    trace(955, "create tree ref");
    return 0;
}

BMX_ADDR(0x00466000)
void zbspDestroyTreeRef(void)
{
    trace(972, "destroy tree ref");
}

BMX_ADDR(0x00466030)
void zbspPropAddTreeRef(void)
{
    trace(988, "prop add tree ref");
}

BMX_ADDR(0x00466060)
void zbspRemoveTreeRef(void)
{
    trace(1004, "remove tree ref");
}

BMX_ADDR(0x00466090)
void zbspUpdateTreeRef(void)
{
    trace(1020, "update tree ref");
}

BMX_ADDR(0x004660c0)
void zbspAddToTree(void)
{
    trace(1038, "add to tree");
}

BMX_ADDR(0x004660f0)
void zbspRemoveFromTree(void)
{
    trace(1055, "remove from tree");
}

BMX_ADDR(0x00466120)
int zbspTotallyWithinTree(void)
{
    trace(1073, "totally within tree");
    return 1;
}

BMX_ADDR(0x00466160)
int zbspOverlapsTree(void)
{
    trace(1092, "overlaps tree");
    return 0;
}

BMX_ADDR(0x00466190)
int zbspFindTagged(void)
{
    trace(1114, "find tagged");
    return 0;
}

BMX_ADDR(0x00466320)
void zbspPropMove(void)
{
    trace(1229, "zbspPropMove");
}

BMX_ADDR(0x00466350)
int zbspGetNumDefs(void)
{
    trace(1245, "get num defs");
    return 1;
}

BMX_ADDR(0x004664b0)
int zbspGetTypeFromHash(void)
{
    trace(1350, "get type from hash");
    return 0;
}

BMX_ADDR(0x004666e0)
void zbspFxCreateTree(void)
{
    trace(1525, "fx create tree");
}

BMX_ADDR(0x00466710)
void zbspDestroyTree(void)
{
    trace(1540, "destroy tree");
}

BMX_ADDR(0x00466740)
void zbspDrawTerrain(void)
{
    trace(1562, "draw terrain");
}

BMX_ADDR(0x00466770)
void zbspDrawTerrainNode(void)
{
    trace(1578, "draw terrain node");
}

BMX_ADDR(0x004667a0)
void zbspDrawFaceList(void)
{
    trace(1592, "draw face list");
}

BMX_ADDR(0x004667d0)
void zbspSetDebug(void)
{
    trace(1638, "set debug");
}

BMX_ADDR(0x00466800)
void zbspDrawLineZ(void)
{
    trace(1667, "draw line z");
}

BMX_ADDR(0x00466830)
void zbspDrawFaceNormal(void)
{
    trace(1680, "draw face normal");
}

BMX_ADDR(0x00466860)
void zbspDrawFaceFlat(void)
{
    trace(1698, "draw face flat");
}

BMX_ADDR(0x00466890)
void zbspShowBoundingBox(void)
{
    trace(1712, "show bounding box");
}

BMX_ADDR(0x004668c0)
void zbspShowEdges(void)
{
    trace(1729, "show edges");
}

BMX_ADDR(0x004668f0)
void zbspShowPropVolume(void)
{
    trace(1743, "show prop volume");
}

BMX_ADDR(0x00466920)
int zbspGetFaceNum(void)
{
    trace(1750, "get face num");
    return 1;
}

BMX_ADDR(0x00466960)
void zbspTrnDumpTree(void)
{
    trace(1764, "trn dump tree");
}

BMX_ADDR(0x00466990)
void zbspShowPartition(void)
{
    trace(1778, "show partition");
}

BMX_ADDR(0x004669c0)
void zbspShowPartition2(void)
{
    trace(1794, "show partition");
}

BMX_ADDR(0x004669f0)
void zbspDrawPartition(void)
{
    trace(1808, "draw partition");
}

BMX_ADDR(0x00466a20)
int zbspGetTerrainFaces(void)
{
    trace(1827, "get terrain faces");
    return 0;
}

BMX_ADDR(0x00466a50)
int zbspGetTreeObject(void)
{
    trace(1844, "get tree object");
    return 0;
}

BMX_ADDR(0x00466a80)
int zbspPointWithinTreeObject(void)
{
    trace(1862, "point within tree object");
    return 0;
}

BMX_ADDR(0x00466ab0)
int zbspGetTreeObjects(void)
{
    trace(1883, "get tree objects");
    return 0;
}

BMX_ADDR(0x00466ae0)
void zbspGetTerrainNode(void)
{
    trace(1903, "get terrain node");
}

BMX_ADDR(0x00466b10)
int zbspGetMaterialInfo(void)
{
    trace(1920, "get material info");
    return 0;
}

BMX_ADDR(0x00466b40)
int zbspGetMatRefData(void)
{
    trace(1937, "get mat ref data");
    return 0;
}

BMX_ADDR(0x00466b70)
int zbspGetMatIdx(void)
{
    trace(1954, "get mat idx");
    return 1;
}

BMX_ADDR(0x00466bf0)
int zbspMatRefList(void)
{
    trace(1996, "mat ref list");
    return 0;
}

BMX_ADDR(0x00466c20)
int zbspFindTagged2(void)
{
    trace(2017, "find tagged");
    return 0;
}

BMX_ADDR(0x00466ca0)
int zbspDoesVertExist(void)
{
    trace(2087, "does vert exist");
    return 0;
}

BMX_ADDR(0x00466cd0)
int zbspFacesAtSamePos(void)
{
    trace(2107, "faces at same pos");
    return 1;
}

BMX_ADDR(0x00466d10)
int zbspSceneSaveState(void)
{
    trace(2161, "scene save state");
    return 0;
}

BMX_ADDR(0x00466d40)
void zbspSceneRestoreState(void)
{
    trace(2175, "scene restore state");
}

BMX_ADDR(0x004676c0)
void zbspGetNormalVector(void)
{
    trace(2507, "get normal vector");
}

BMX_ADDR(0x004676f0)
int zbspGetSideVector(void)
{
    trace(2522, "get side vector");
    return 1;
}

BMX_ADDR(0x00467730)
int zbspCollide(void)
{
    trace(2556, "bspcollide");
    return 0;
}

BMX_ADDR(0x00467760)
void zbspCreateSpanPlanes(void)
{
    trace(2576, "create span planes");
}

BMX_ADDR(0x00467790)
void zbspAddToFacelist(void)
{
    trace(2593, "add to facelist");
}

BMX_ADDR(0x004677c0)
void zbspGetTerrainHeightinf(void)
{
    trace(2600, "get terrain heightinf");
}

BMX_ADDR(0x004677f0)
void zbspTestRayCollision(void)
{
    trace(2609, "test ray collision");
}

BMX_ADDR(0x00467820)
void zbspTest(void)
{
    trace(2624, "bsptest");
}

BMX_ADDR(0x00467850)
void zbspTestResetFaceColors(void)
{
    trace(2638, "testresetfacecolors");
}

BMX_ADDR(0x00467880)
void zbspHighlightNodeFaces(void)
{
    trace(2654, "hilightnodefaces");
}

BMX_ADDR(0x004678b0)
void zbspSetFaceRgb(void)
{
    trace(2670, "set face rgb");
}

BMX_ADDR(0x004678e0)
void zbspPrintFaceListInf(void)
{
    trace(2684, "print face list inf");
}

BMX_ADDR(0x00467910)
int zbspTrnTestRayCollision(void)
{
    trace(2704, "trntestraycollision");
    return 0;
}

BMX_ADDR(0x00467940)
void zbspCreateNodesHit(void)
{
    trace(2722, "createnodeshit");
}

BMX_ADDR(0x00467970)
void zbspAddToNodeslist(void)
{
    trace(2740, "add to nodeslist");
}

BMX_ADDR(0x004679a0)
void zbspAddToFacesList(void)
{
    trace(2758, "add to faces list");
}

BMX_ADDR(0x004679d0)
int zbspLineToNodes(void)
{
    trace(2774, "line to nodes");
    return 1;
}

BMX_ADDR(0x00467a10)
int zbspLineToFaces(void)
{
    trace(2792, "line to faces");
    return 1;
}

BMX_ADDR(0x00467a50)
int zbspLinePlaneIntersection(void)
{
    trace(2809, "lineplaneintersection");
    return 1;
}

BMX_ADDR(0x00467d30)
int zbspClipRayToObject(void)
{
    trace(2938, "clip ray to object");
    return 1;
}

BMX_ADDR(0x00467d70)
int zbspClipLineToProps(void)
{
    trace(2999, "clip line to props");
    return 1;
}

BMX_ADDR(0x00467db0)
int zbspClipLineToProp(void)
{
    trace(3016, "Cliplinetoprop");
    return 0;
}

BMX_ADDR(0x00467de0)
void zbspBuildRayBoundingBox(void)
{
    trace(3034, "buildrayboundingbox");
}

BMX_ADDR(0x00467e10)
uint8_t *zbspAddPropFaces(uint8_t *prop)
{
    trace(3051, "addpropfaces");
    return prop + 0x110;
}

BMX_ADDR(0x00467e50)
void zbspComputeFacePlaneEq(void)
{
    trace(3068, "zbspComputeFacePlaneEq");
}

BMX_ADDR(0x00467e80)
void zbspTagInitialize(void)
{
    trace(3089, "taginitialize");
}

BMX_ADDR(0x00467eb0)
void zbspTagRegisterProFunc(void)
{
    trace(3104, "tagregisterprofunc");
}

BMX_ADDR(0x00467ee0)
int zbspFaceTagRegister(void)
{
    trace(3120, "facetagregister");
    return 0;
}

BMX_ADDR(0x00467f10)
int zbspFaceTagPreProcess(void)
{
    trace(3138, "facetagpreprocess");
    return 0;
}

BMX_ADDR(0x00467f40)
int zbspFaceTagProcess(void)
{
    trace(3156, "facetagprocess");
    return 0;
}

BMX_ADDR(0x00467f70)
int zbspFaceTagFindDef(void)
{
    trace(3173, "facetagfinddef");
    return 0;
}

BMX_ADDR(0x00467fa0)
int zbspFaceTagRegRegister(void)
{
    trace(3192, "facetagregregister");
    return 0;
}

BMX_ADDR(0x00467fd0)
int zbspFaceTagPreAttachAsRef(void)
{
    trace(3214, "facetagpreattachasref");
    return 0;
}

BMX_ADDR(0x00468000)
int zbspFaceTagPreAttachSmPosAsRef(void)
{
    trace(3229, "facetagpreattachsmposasref");
    return 0;
}

BMX_ADDR(0x00468030)
int zbspFaceTagProSwapWindow(void)
{
    trace(3249, "facetagproswapwindow");
    return 1;
}

BMX_ADDR(0x00468870)
void zbspColSphereCreateNodesHit(void)
{
    trace(3582, "colspherecreatenodeshit");
}

BMX_ADDR(0x004688a0)
int zbspHitNodeslist(void)
{
    trace(3596, "hitnodeslist");
    return 1;
}

BMX_ADDR(0x004688e0)
int zbspTestWithFacelist(void)
{
    trace(3611, "testwithfacelist");
    return 1;
}

BMX_ADDR(0x00468b50)
void *zbspFaceBoundingBox(void *face, void *box)
{
    trace(3711, "faceboundingbox");
    return box;
}

BMX_ADDR(0x00468b90)
void zbspColSphereDraw(void)
{
    trace(3726, "zbspColSphereDraw");
}

BMX_ADDR(0x00469500)
int zbspFindFaceByType(void)
{
    trace(4108, "zbspFindFaceByType()");
    return 0;
}
