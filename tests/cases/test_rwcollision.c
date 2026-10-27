/*
 * Tests for rw/collision.c (and the vector / frame / world sector helpers it uses): the triangle-cube,
 * sphere- triangle and point-in-triangle kernels, the BSP walks and per-sector tests, and the three entry
 * points on a random world (a BSP of plane sectors over world sectors with polygon split trees, atomics with
 * morph targets tied to the sectors).
 *
 * The engine instance has real square root tables and a real point transform; the callbacks log what they
 * get (box queries leave the triangle normal uninitialised, so it is not logged for them). The register
 * convention internals are called through thunks. Coordinates stay positive and radii / box sizes clear of
 * zero: the walks compare coordinates as integers, and a split test with an empty box loops forever in the
 * original.
 */
#include "dt.h"
#include "dt_rwmath.h"
#include "rw/rwcollision.h"

/* ---- engine -------------------------------------------------------------------------------------------- */

RWT_FN uint32_t collBits(float f)
{
    uint32_t u;
    __builtin_memcpy(&u, &f, 4);
    return u;
}

RWT_FN float collFloat(uint32_t u)
{
    float f;
    __builtin_memcpy(&f, &u, 4);
    return f;
}

/* out = in * matrix (rows right, up, at, then pos), in place safe. */
static RwV3d *BMX_CDECL collTransformPoints(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *m)
{
    rwtLog(0x4d3a90);
    rwtLog((uint32_t)count);
    for (int32_t i = 0; i < count; i++) {
        const float x = in[i].x, y = in[i].y, z = in[i].z;
        out[i].x = x * m->right.x + y * m->up.x + z * m->at.x + m->pos.x;
        out[i].y = x * m->right.y + y * m->up.y + z * m->at.y + m->pos.y;
        out[i].z = x * m->right.z + y * m->up.z + z * m->at.z + m->pos.z;
    }
    return out;
}

/* The engine of the maths tests with the real square root tables (indexed by the top mantissa bits). */
static RwGlobals *collEnv(DtCtx *c)
{
    dt_seed(c);
    RwGlobals *g = mathEnv();
    RwMatrixModule *matrixModule = (RwMatrixModule *)((uint8_t *)g + kMathMatrixOffset);
    matrixModule->flagMask = 0x20000;
    RwV3dModule *v3d = (RwV3dModule *)((uint8_t *)g + kMathV3dOffset);
    uint32_t *sqrtTable = dt_alloc(4096 * 4);
    uint32_t *invTable = dt_alloc(4096 * 4);
    for (uint32_t i = 0; i < 4096; i++) {
        const uint32_t bits = 0x3f000000u + (i << 12) + 0x800;
        const float s = __builtin_sqrtf(collFloat(bits));
        sqrtTable[i] = collBits(s) - ((bits & 0x7f800000u) >> 1);
        invTable[i] = collBits(1.0f / s) - ((~bits & 0x7f800000u) >> 1);
    }
    v3d->sqrtTable = sqrtTable;
    v3d->invSqrtTable = invTable;
    v3d->transformPoints = collTransformPoints;
    g->lightFrame = (uint16_t)(dt_u32() % 4 == 0 ? 0xffff : dt_u32());
    return g;
}

/* A coordinate in the positive working range, now and then an exact repeat of a few fixed values. */
static float collCoord(void)
{
    static const float kFixed[] = {10.0f, 25.0f, 50.0f, 50.5f, 75.0f};
    if (dt_u32() % 8 == 0) {
        return kFixed[dt_u32() % 5];
    }
    return dt_float(1.0f, 99.0f);
}

static void collPoint(RwV3d *v)
{
    v->x = collCoord();
    v->y = collCoord();
    v->z = collCoord();
}

/* A point near another (within `spread`), clamped into the positive range. */
static void collNear(RwV3d *v, const RwV3d *around, float spread)
{
    v->x = around->x + dt_float(-spread, spread);
    v->y = around->y + dt_float(-spread, spread);
    v->z = around->z + dt_float(-spread, spread);
    if (dt_u32() % 10 == 0) {
        *v = *around;
    }
}

/* ---- callbacks ----------------------------------------------------------------------------------------- */

static int gCollCalls, gCollStopAt;

static int collStop(void)
{
    gCollCalls++;
    return gCollStopAt != 0 && gCollCalls == gCollStopAt;
}

static void collLogTriangle(const RpCollisionTriangle *t, int withNormal)
{
    if (withNormal) {
        rwtLogWords(&t->normal, 3);
    }
    rwtLogWords(&t->point, 3);
    rwtLog((uint32_t)t->index);
    for (int i = 0; i < 3; i++) {
        rwtLogPtr(t->vertices[i]);
        rwtLogWords(t->vertices[i], 3);
    }
}

static RpCollisionTriangle *BMX_CDECL collWorldTriangleCb(RpIntersection *intersection, RpWorldSector *sector,
                                                          RpCollisionTriangle *triangle, float distance,
                                                          void *data)
{
    rwtLog(0xc0111);
    rwtLogPtr(intersection);
    rwtLogPtr(sector);
    rwtLogPtr(triangle);
    collLogTriangle(triangle, intersection->type != rpINTERSECTBOX);
    rwtLogFloatBits(distance);
    rwtLogPtr(data);
    return collStop() ? NULL : triangle;
}

static RpAtomic *BMX_CDECL collAtomicCb(RpIntersection *intersection, RpWorldSector *sector, RpAtomic *atomic,
                                        float distance, void *data)
{
    rwtLog(0xc0112);
    rwtLogPtr(intersection);
    rwtLogPtr(sector);
    rwtLogPtr(atomic);
    rwtLogFloatBits(distance);
    rwtLogPtr(data);
    return collStop() ? NULL : atomic;
}

static RpCollisionTriangle *BMX_CDECL collGeometryTriangleCb(RpIntersection *intersection,
                                                             RpCollisionTriangle *triangle, float distance,
                                                             void *data)
{
    rwtLog(0xc0113);
    rwtLogPtr(intersection);
    rwtLogPtr(triangle);
    collLogTriangle(triangle, 1);
    rwtLogFloatBits(distance);
    rwtLogPtr(data);
    return collStop() ? NULL : triangle;
}

static RpWorldSector *BMX_CDECL collSectorCb(RpIntersection *intersection, RpWorldSector *sector,
                                             RpCollisionQuery *query)
{
    rwtLog(0xc0114);
    rwtLogPtr(intersection);
    rwtLogPtr(sector);
    rwtLogPtr(query);
    rwtLogWords(&query->t.line, 6);
    return collStop() ? NULL : sector;
}

static void collCallbacks(void)
{
    gCollCalls = 0;
    gCollStopAt = dt_u32() % 3 == 0 ? dt_range(1, 6) : 0;
}

/* ---- world fixture ------------------------------------------------------------------------------------- */

enum { kCollMaxSectors = 16, kCollMaxAtomics = 5 };

typedef struct CollWorld {
    RpWorld *world;
    RpWorldSector *sectors[kCollMaxSectors];
    int numSectors;
    RpAtomic *atomics[kCollMaxAtomics];
    int numAtomics;
} CollWorld;

static void collListInit(RwLLLink *head)
{
    head->next = head->prev = head;
}

static void collListAdd(RwLLLink *head, RwLLLink *link)
{
    link->next = head->next;
    link->prev = head;
    head->next->prev = link;
    head->next = link;
}

/* Polygon split tree of a sector: a full tree of `depth` levels below node 1, 3 bytes per node. */
static const uint8_t *collSectorTree(int numVertices, int numPolygons)
{
    const int depth = dt_range(0, 2);
    const int nodes = 1 << (depth + 1);
    uint8_t *tree = dt_alloc((uint32_t)(nodes * 3 + 4));
    dt_fill(tree, 3);
    int next = 0;
    for (int node = 1; node < nodes; node++) {
        uint8_t *entry = tree + node * 3;
        const int count = numPolygons - next < 3 ? numPolygons - next : dt_range(0, 3);
        const int first = next;
        next += count;
        if (node < nodes / 2) {
            entry[0] = (uint8_t)(0x80 | (dt_range(0, 2) << 5) | count);
            entry[1] = (uint8_t)dt_range(0, numVertices - 1);
        } else {
            entry[0] = (uint8_t)count;
            entry[1] = (uint8_t)dt_u32();
        }
        entry[2] = (uint8_t)first;
    }
    return tree;
}

static RpWorldSector *collSector(void)
{
    RpWorldSector *sector = dt_alloc(sizeof *sector);
    dt_fill(sector, sizeof *sector);
    sector->type = -1 - dt_range(0, 3);
    const int numVertices = dt_range(3, 12);
    const int numPolygons = dt_range(0, 10);
    sector->numVertices = (uint16_t)numVertices;
    sector->numPolygons = (uint16_t)numPolygons;
    sector->vertices = dt_alloc((uint32_t)(numVertices * sizeof(RwV3d)));
    RwV3d centre;
    collPoint(&centre);
    for (int i = 0; i < numVertices; i++) {
        collNear(&sector->vertices[i], &centre, 20.0f);
    }
    if (numVertices > 4 && dt_u32() % 4 == 0) {
        /* collinear corners */
        sector->vertices[2].x = (sector->vertices[0].x + sector->vertices[1].x) * 0.5f;
        sector->vertices[2].y = (sector->vertices[0].y + sector->vertices[1].y) * 0.5f;
        sector->vertices[2].z = (sector->vertices[0].z + sector->vertices[1].z) * 0.5f;
    }
    sector->polygons = dt_alloc((uint32_t)(numPolygons * sizeof(RpPolygon) + 4));
    for (int i = 0; i < numPolygons; i++) {
        RpPolygon *p = &sector->polygons[i];
        p->matIndex = (uint8_t)dt_u32();
        for (int k = 0; k < 3; k++) {
            p->vertIndex[k] = (uint8_t)dt_range(0, numVertices - 1);
        }
        if (dt_u32() % 6 == 0) {
            p->vertIndex[1] = p->vertIndex[0];
        }
    }
    sector->colSectorRoot = dt_u32() % 8 == 0 ? NULL : collSectorTree(numVertices, numPolygons);
    collListInit(&sector->collAtomicsInWorldSector);
    return sector;
}

static RpSector *collBsp(CollWorld *w, int depth)
{
    if (depth == 0 || w->numSectors >= kCollMaxSectors - 1 || dt_u32() % 4 == 0) {
        RpWorldSector *sector = collSector();
        if (w->numSectors < kCollMaxSectors) {
            w->sectors[w->numSectors++] = sector;
        }
        return (RpSector *)sector;
    }
    RpPlaneSector *plane = dt_alloc(sizeof *plane);
    plane->type = 4 * dt_range(0, 2);
    const float split = dt_float(20.0f, 80.0f);
    const float overlap = dt_u32() % 4 == 0 ? 0.0f : dt_float(0.0f, 10.0f);
    plane->value = split;
    plane->leftValue = split + overlap;
    plane->rightValue = dt_u32() % 8 == 0 ? split + overlap + 1.0f : split - overlap;
    plane->leftSubTree = collBsp(w, depth - 1);
    plane->rightSubTree = collBsp(w, depth - 1);
    return (RpSector *)plane;
}

/* An LTM: a rotation about z, a uniform scale and a translation (orthonormal flags when unscaled). */
static void collLtm(RwMatrix *m)
{
    static const float kCos[] = {1.0f, 0.866025388f, 0.707106769f, 0.0f, -0.5446390f};
    static const float kSin[] = {0.0f, 0.5f, 0.707106769f, 1.0f, 0.8386706f};
    const int a = dt_range(0, 4);
    const float s = dt_u32() % 3 == 0 ? dt_float(0.5f, 2.0f) : 1.0f;
    __builtin_memset(m, 0, sizeof *m);
    m->right.x = kCos[a] * s;
    m->right.y = kSin[a] * s;
    m->up.x = -kSin[a] * s;
    m->up.y = kCos[a] * s;
    m->at.z = s;
    collPoint(&m->pos);
    m->flags = s == 1.0f ? 3u : 0u;
}

static RpGeometry *collGeometry(void)
{
    RpGeometry *geometry = dt_alloc(sizeof *geometry);
    dt_fill(geometry, sizeof *geometry);
    const int numVertices = dt_range(3, 8);
    const int numTargets = dt_range(1, 3);
    geometry->numVertices = numVertices;
    geometry->numMorphTargets = numTargets;
    geometry->morphTarget = dt_alloc((uint32_t)(numTargets * sizeof(RpMorphTarget)));
    for (int t = 0; t < numTargets; t++) {
        RpMorphTarget *target = &geometry->morphTarget[t];
        target->parentGeom = geometry;
        target->boundingSphere.center.x = dt_float(-2.0f, 2.0f);
        target->boundingSphere.center.y = dt_float(-2.0f, 2.0f);
        target->boundingSphere.center.z = dt_float(-2.0f, 2.0f);
        target->boundingSphere.radius = dt_float(1.0f, 8.0f);
        target->verts = dt_alloc((uint32_t)(numVertices * sizeof(RwV3d)));
        target->normals = NULL;
        for (int i = 0; i < numVertices; i++) {
            target->verts[i].x = dt_float(-5.0f, 5.0f);
            target->verts[i].y = dt_float(-5.0f, 5.0f);
            target->verts[i].z = dt_float(-5.0f, 5.0f);
        }
    }
    const int numTriangles = dt_range(0, 6);
    geometry->numTriangles = numTriangles;
    geometry->triangles = dt_alloc((uint32_t)(numTriangles * sizeof(RpTriangle) + 4));
    for (int i = 0; i < numTriangles; i++) {
        for (int k = 0; k < 3; k++) {
            geometry->triangles[i].vertIndex[k] = (uint16_t)dt_range(0, numVertices - 1);
        }
        geometry->triangles[i].matIndex = (int16_t)dt_u32();
    }
    return geometry;
}

static RpAtomic *collAtomic(void)
{
    RpAtomic *atomic = dt_alloc(sizeof *atomic + 0x10);
    dt_fill(atomic, sizeof *atomic);
    RwFrame *frame = dt_alloc(sizeof(RwFrame) + 0x10);
    dt_fill(frame, sizeof(RwFrame));
    frame->root = frame;
    frame->object.privateFlags = 0;
    collLtm(&frame->ltm);
    atomic->object.object.parent = frame;
    atomic->object.object.privateFlags = (uint8_t)(dt_u32() & 0xff);
    RpGeometry *geometry = collGeometry();
    atomic->geometry = geometry;
    RpInterpolator *interpolator = &atomic->interpolator;
    interpolator->flags = dt_u32();
    interpolator->startMorphTarget = (int16_t)dt_range(0, geometry->numMorphTargets);
    interpolator->endMorphTarget = (int16_t)dt_range(0, geometry->numMorphTargets);
    interpolator->time = dt_float(0.5f, 2.0f);
    /* position * recipTime stays in 0..1: a blended radius stays positive (an empty box loops forever) */
    interpolator->recipTime = dt_float(0.5f, 1.0f);
    interpolator->position = dt_float(0.0f, 1.0f);
    atomic->boundingSphere.center.x = dt_float(-2.0f, 2.0f);
    atomic->boundingSphere.center.y = dt_float(-2.0f, 2.0f);
    atomic->boundingSphere.center.z = dt_float(-2.0f, 2.0f);
    atomic->boundingSphere.radius = dt_float(1.0f, 8.0f);
    collPoint(&atomic->worldBoundingSphere.center);
    atomic->worldBoundingSphere.radius = dt_float(1.0f, 15.0f);
    atomic->collisionTag =
        dt_u32() % 5 == 0 ? (uint16_t)(RwEngineInstance->lightFrame + 1) : (uint16_t)dt_u32();
    collListInit(&atomic->llWorldSectorsInAtomic);
    return atomic;
}

static void collTie(RpAtomic *atomic, RpWorldSector *sector)
{
    RpTie *tie = dt_alloc(sizeof *tie);
    tie->apAtom = dt_u32() % 12 == 0 ? NULL : atomic;
    tie->worldSector = sector;
    collListAdd(&sector->collAtomicsInWorldSector, &tie->lAtomicInWorldSector);
    collListAdd(&atomic->llWorldSectorsInAtomic, &tie->lWorldSectorInAtomic);
}

static void collWorld(CollWorld *w)
{
    __builtin_memset(w, 0, sizeof *w);
    w->world = dt_alloc(sizeof(RpWorld) + 0x10);
    dt_fill(w->world, sizeof(RpWorld));
    w->world->rootSector = collBsp(w, dt_range(0, 3));
    w->numAtomics = dt_range(0, kCollMaxAtomics);
    for (int i = 0; i < w->numAtomics; i++) {
        RpAtomic *atomic = collAtomic();
        w->atomics[i] = atomic;
        const int ties = dt_range(0, 2);
        for (int k = 0; k < ties; k++) {
            collTie(atomic, w->sectors[dt_range(0, w->numSectors - 1)]);
        }
    }
}

/* A volume of the given type around the world's content (a few of them reach below zero). */
static RpIntersection *collIntersection(const CollWorld *w, int32_t type)
{
    RpIntersection *is = dt_alloc(sizeof *is);
    dt_fill(is, sizeof *is);
    is->type = type;
    RwV3d anchor;
    if (w->numSectors > 0 && dt_u32() % 2 == 0) {
        const RpWorldSector *s = w->sectors[dt_range(0, w->numSectors - 1)];
        anchor = s->vertices[dt_range(0, s->numVertices - 1)];
    } else {
        collPoint(&anchor);
    }
    switch (type) {
    case rpINTERSECTLINE:
        collNear(&is->t.line.start, &anchor, 30.0f);
        collNear(&is->t.line.end, &anchor, 30.0f);
        if (dt_u32() % 8 == 0) {
            is->t.line.end = is->t.line.start;
            is->t.line.end.x += 25.0f;
        }
        break;
    case rpINTERSECTPOINT:
        collNear(&is->t.point, &anchor, 5.0f);
        break;
    case rpINTERSECTSPHERE:
        collNear(&is->t.sphere.center, &anchor, 10.0f);
        is->t.sphere.radius = dt_float(0.5f, 25.0f);
        break;
    case rpINTERSECTBOX:
        collNear(&is->t.box.inf, &anchor, 10.0f);
        is->t.box.sup.x = is->t.box.inf.x + dt_float(0.5f, 30.0f);
        is->t.box.sup.y = is->t.box.inf.y + dt_float(0.5f, 30.0f);
        is->t.box.sup.z = is->t.box.inf.z + dt_float(0.5f, 30.0f);
        if (dt_u32() % 10 == 0) {
            is->t.box.inf.y = -1.0f;
        }
        break;
    case rpINTERSECTATOMIC:
        is->t.object = w->numAtomics > 0 ? w->atomics[dt_range(0, w->numAtomics - 1)] : collAtomic();
        break;
    default:
        break;
    }
    return is;
}

static int32_t collType(int32_t first, int32_t last)
{
    if (dt_u32() % 16 == 0) {
        return dt_u32() % 2 == 0 ? 0 : 6;
    }
    return dt_range(first, last);
}

/* ---- entry points -------------------------------------------------------------------------------------- */

static void setup_world_triangles(DtCtx *c)
{
    collEnv(c);
    CollWorld w;
    collWorld(&w);
    collCallbacks();
    RpIntersection *is = collIntersection(&w, collType(1, 5));
    if (is->type == rpINTERSECTSPHERE && dt_u32() % 10 == 0) {
        is->t.sphere.center.z = is->t.sphere.radius * 0.5f;
    }
    DT_ARG_PTR(c, 0, w.world);
    DT_ARG_PTR(c, 1, is);
    DT_ARG_PTR(c, 2, collWorldTriangleCb);
    DT_ARG_PTR(c, 3, dt_alloc(4));
    rwtEnd();
}

static void setup_world_objects(DtCtx *c)
{
    collEnv(c);
    CollWorld w;
    collWorld(&w);
    collCallbacks();
    DT_ARG_PTR(c, 0, w.world);
    DT_ARG_PTR(c, 1, collIntersection(&w, collType(1, 5)));
    DT_ARG_PTR(c, 2, collAtomicCb);
    DT_ARG_PTR(c, 3, dt_alloc(4));
    rwtEnd();
}

static void setup_object_triangles(DtCtx *c)
{
    collEnv(c);
    CollWorld w;
    collWorld(&w);
    collCallbacks();
    RpAtomic *atomic = w.numAtomics > 0 ? w.atomics[0] : collAtomic();
    static const int32_t kTypes[] = {1, 1, 3, 3, 5, 2};
    RpIntersection *is = collIntersection(&w, dt_u32() % 16 == 0 ? 4 : kTypes[dt_u32() % 6]);
    if (is->type == rpINTERSECTLINE && dt_u32() % 2 == 0) {
        /* through the atomic */
        const RwV3d *p = &((RwFrame *)atomic->object.object.parent)->ltm.pos;
        collNear(&is->t.line.start, p, 8.0f);
        collNear(&is->t.line.end, p, 8.0f);
    } else if (is->type == rpINTERSECTSPHERE && dt_u32() % 2 == 0) {
        collNear(&is->t.sphere.center, &((RwFrame *)atomic->object.object.parent)->ltm.pos, 4.0f);
    }
    DT_ARG_PTR(c, 0, atomic);
    DT_ARG_PTR(c, 1, is);
    DT_ARG_PTR(c, 2, collGeometryTriangleCb);
    DT_ARG_PTR(c, 3, dt_alloc(4));
    rwtEnd();
}

static void setup_world_sphere(DtCtx *c)
{
    collEnv(c);
    RpAtomic *atomic = collAtomic();
    if (dt_u32() % 4 == 0) {
        atomic->object.object.parent = dt_alloc(sizeof(RwFrame));
        RwFrame *frame = atomic->object.object.parent;
        dt_fill(frame, sizeof *frame);
        frame->root = frame;
        frame->object.privateFlags = 0;
        collLtm(&frame->ltm);
    }
    if (dt_u32() % 6 == 0) {
        atomic->geometry = NULL;
    }
    DT_ARG_PTR(c, 0, atomic);
    rwtEnd();
}

/* ---- per-sector tests ---------------------------------------------------------------------------------- */

/* A world, one of its sectors (with atomics tied to it) and a query of the given volume type. */
static void sector_setup(DtCtx *c, int32_t type, void *callBack)
{
    collEnv(c);
    CollWorld w;
    collWorld(&w);
    collCallbacks();
    RpWorldSector *sector = w.sectors[dt_range(0, w.numSectors - 1)];
    for (int i = 0; i < w.numAtomics; i++) {
        if (dt_u32() % 2 == 0) {
            collTie(w.atomics[i], sector);
        }
    }
    if (w.numAtomics > 0 && dt_u32() % 4 == 0) {
        /* a NaN in a world sphere that stays as it is: the overlap tests let it through */
        RpAtomic *atomic = w.atomics[0];
        atomic->interpolator.flags &= ~2u;
        atomic->object.object.privateFlags &= 0xfe;
        atomic->worldBoundingSphere.center.y = collFloat(0xffc00000u);
    }
    RpIntersection *is = collIntersection(&w, type);
    if (type == rpINTERSECTBOX) {
        is->t.box.inf.y = is->t.box.sup.y - dt_float(0.5f, 30.0f);
    }
    RpCollisionQuery *query = dt_alloc(sizeof *query);
    dt_fill(query, sizeof *query);
    query->callBack = callBack;
    query->data = dt_alloc(4);
    __builtin_memcpy(&query->t, &is->t, sizeof query->t);
    query->exclude = w.numAtomics > 0 && dt_u32() % 3 == 0 ? w.atomics[dt_range(0, w.numAtomics - 1)] : NULL;
    DT_ARG_PTR(c, 0, is);
    DT_ARG_PTR(c, 1, sector);
    DT_ARG_PTR(c, 2, query);
    rwtEnd();
}

static void setup_sector_atomics_sphere(DtCtx *c)
{
    sector_setup(c, rpINTERSECTSPHERE, collAtomicCb);
}

static void setup_sector_atomics_box(DtCtx *c)
{
    sector_setup(c, rpINTERSECTBOX, collAtomicCb);
}

static void setup_sector_triangles_sphere(DtCtx *c)
{
    sector_setup(c, rpINTERSECTSPHERE, collWorldTriangleCb);
}

static void setup_sector_triangles_box(DtCtx *c)
{
    sector_setup(c, rpINTERSECTBOX, collWorldTriangleCb);
}

/* ---- register convention walkers, through thunks ------------------------------------------------------- */

typedef uint32_t(__attribute__((regparm(3))) * CollWalkFn)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                                                           uint32_t, uint32_t, uint32_t);

#define COLL_WALK_THUNKS(fn, addr)                                                                           \
    static uint32_t fn##_orig(uint32_t a, uint32_t b, uint32_t cb, uint32_t is, uint32_t data)               \
    {                                                                                                        \
        return ((CollWalkFn)(uintptr_t)(addr))(a, b, cb, 0, 0, 0, is, data);                                 \
    }                                                                                                        \
    static uint32_t fn##_impl(uint32_t a, uint32_t b, uint32_t cb, uint32_t is, uint32_t data)               \
    {                                                                                                        \
        return ((CollWalkFn)(uintptr_t)(fn))(a, b, cb, 0, 0, 0, is, data);                                   \
    }

COLL_WALK_THUNKS(_rpCollWorldAtomicsPoint, 0x004be160)
COLL_WALK_THUNKS(_rpCollWorldAtomicsLine, 0x004be260)
COLL_WALK_THUNKS(_rpCollWorldAtomicsSphere, 0x004be2c0)
COLL_WALK_THUNKS(_rpCollWorldAtomicsBox, 0x004c0b00)
COLL_WALK_THUNKS(_rpCollWorldAtomicsAtomic, 0x004be480)
COLL_WALK_THUNKS(_rpCollWorldTrianglesLine, 0x004c05d0)
COLL_WALK_THUNKS(_rpCollWorldTrianglesSphere, 0x004c0650)
COLL_WALK_THUNKS(_rpCollWorldTrianglesBox, 0x004c0810)
COLL_WALK_THUNKS(_rpCollWorldTrianglesAtomic, 0x004c0c60)
COLL_WALK_THUNKS(_rpCollAtomicTrianglesLine, 0x004c0d40)
COLL_WALK_THUNKS(_rpCollAtomicTrianglesSphere, 0x004c1600)
COLL_WALK_THUNKS(_rpCollWorldForAllLineSectors, 0x004bd400)

/* A walker's arguments: world (or atomic), the volume inside a random intersection, callback, data. */
static void walk_setup(DtCtx *c, int32_t type, void *callBack, int onAtomic)
{
    collEnv(c);
    CollWorld w;
    collWorld(&w);
    collCallbacks();
    RpIntersection *is = collIntersection(&w, type);
    RpAtomic *atomic = w.numAtomics > 0 ? w.atomics[dt_range(0, w.numAtomics - 1)] : collAtomic();
    if (onAtomic && type == rpINTERSECTLINE) {
        collNear(&is->t.line.start, &((RwFrame *)atomic->object.object.parent)->ltm.pos, 8.0f);
        collNear(&is->t.line.end, &((RwFrame *)atomic->object.object.parent)->ltm.pos, 8.0f);
    }
    DT_ARG_PTR(c, 0, onAtomic ? (void *)atomic : (void *)w.world);
    DT_ARG_PTR(c, 1, type == rpINTERSECTATOMIC ? (void *)is->t.object : (void *)&is->t);
    DT_ARG_PTR(c, 2, callBack);
    DT_ARG_PTR(c, 3, is);
    DT_ARG_PTR(c, 4, dt_alloc(4));
    rwtEnd();
}

static void setup_walk_atomics_point(DtCtx *c)
{
    walk_setup(c, rpINTERSECTPOINT, collAtomicCb, 0);
}

static void setup_walk_atomics_line(DtCtx *c)
{
    walk_setup(c, rpINTERSECTLINE, collAtomicCb, 0);
}

static void setup_walk_atomics_sphere(DtCtx *c)
{
    walk_setup(c, rpINTERSECTSPHERE, collAtomicCb, 0);
}

static void setup_walk_atomics_box(DtCtx *c)
{
    walk_setup(c, rpINTERSECTBOX, collAtomicCb, 0);
}

static void setup_walk_atomics_atomic(DtCtx *c)
{
    walk_setup(c, rpINTERSECTATOMIC, collAtomicCb, 0);
}

static void setup_walk_triangles_line(DtCtx *c)
{
    walk_setup(c, rpINTERSECTLINE, collWorldTriangleCb, 0);
}

static void setup_walk_triangles_sphere(DtCtx *c)
{
    walk_setup(c, rpINTERSECTSPHERE, collWorldTriangleCb, 0);
}

static void setup_walk_triangles_box(DtCtx *c)
{
    walk_setup(c, rpINTERSECTBOX, collWorldTriangleCb, 0);
}

static void setup_walk_triangles_atomic(DtCtx *c)
{
    walk_setup(c, rpINTERSECTATOMIC, collWorldTriangleCb, 0);
}

static void setup_atomic_triangles_line(DtCtx *c)
{
    walk_setup(c, rpINTERSECTLINE, collGeometryTriangleCb, 1);
}

static void setup_atomic_triangles_sphere(DtCtx *c)
{
    walk_setup(c, rpINTERSECTSPHERE, collGeometryTriangleCb, 1);
}

/* The line walk: arguments world, line, sector callback, intersection, query. */
static void setup_line_sectors(DtCtx *c)
{
    walk_setup(c, rpINTERSECTLINE, collSectorCb, 0);
    RpCollisionQuery *query = dt_alloc(sizeof *query);
    dt_fill(query, sizeof *query);
    DT_ARG_PTR(c, 4, query);
}

/* ---- kernels ------------------------------------------------------------------------------------------- */

typedef int32_t(__attribute__((regparm(3))) * CollPointTriFn)(const RwV3d *, const RwV3d *, const RwV3d *,
                                                              uint32_t, uint32_t, uint32_t, const RwV3d *);

static int32_t point_tri_orig(const RwV3d *p, const RwV3d *a, const RwV3d *b, const RwV3d *c)
{
    return ((CollPointTriFn)(uintptr_t)0x004bc750)(p, a, b, 0, 0, 0, c);
}

static int32_t point_tri_impl(const RwV3d *p, const RwV3d *a, const RwV3d *b, const RwV3d *c)
{
    return _rpCollPointOutsideTriangle(p, a, b, 0, 0, 0, c);
}

/* A corner coordinate in the unit cube range with exact repeats, now and then a NaN. */
static float cube_coord(void)
{
    static const float kFixed[] = {0.0f, -0.0f, 0.5f, -0.5f, 0.25f, 1.0f, 0x1.a36e2ep-14f};
    const uint32_t r = dt_u32() % 16;
    if (r == 0) {
        return kFixed[dt_u32() % 7];
    }
    if (r == 1 && dt_u32() % 4 == 0) {
        return collFloat(0xffc00000u);
    }
    return dt_float(-1.0f, 1.0f);
}

static RwV3d *cube_point(void)
{
    RwV3d *v = dt_alloc(sizeof *v);
    v->x = cube_coord();
    v->y = cube_coord();
    v->z = cube_coord();
    return v;
}

static void setup_point_tri(DtCtx *c)
{
    collEnv(c);
    RwV3d *a = cube_point(), *b = cube_point(), *t = cube_point();
    RwV3d *p = cube_point();
    switch (dt_u32() % 6) {
    case 0: /* a point of the triangle's plane: a blend of the corners */
    {
        const float u = dt_float(-0.2f, 1.0f), v = dt_float(-0.2f, 1.0f - u);
        p->x = a->x + u * (b->x - a->x) + v * (t->x - a->x);
        p->y = a->y + u * (b->y - a->y) + v * (t->y - a->y);
        p->z = a->z + u * (b->z - a->z) + v * (t->z - a->z);
        break;
    }
    case 1:
        *p = *b;
        break;
    case 2:
        *t = *a;
        break;
    default:
        break;
    }
    DT_ARG_PTR(c, 0, p);
    DT_ARG_PTR(c, 1, a);
    DT_ARG_PTR(c, 2, b);
    DT_ARG_PTR(c, 3, t);
    rwtEnd();
}

static uint32_t bevel2d_orig(const RwV3d *p)
{
    return ((uint32_t(__attribute__((regparm(1))) *)(const RwV3d *))(uintptr_t)0x004bc3c0)(p);
}

static uint32_t bevel2d_impl(const RwV3d *p)
{
    return _rpCollBevel2d(p);
}

static uint32_t bevel3d_orig(const RwV3d *p)
{
    return ((uint32_t(__attribute__((regparm(1))) *)(const RwV3d *))(uintptr_t)0x004bc4c0)(p);
}

static uint32_t bevel3d_impl(const RwV3d *p)
{
    return _rpCollBevel3d(p);
}

static void setup_bevel(DtCtx *c)
{
    collEnv(c);
    RwV3d *p = cube_point();
    p->x *= 2.0f;
    p->y *= 2.0f;
    DT_ARG_PTR(c, 0, p);
    rwtEnd();
}

/* check_point: EAX = p1, EDX = p2, ECX = mask, alpha in ST0. */
static uint32_t check_point_call(uint32_t target, const RwV3d *p1, const RwV3d *p2, uint32_t mask,
                                 const double *alpha)
{
    uint32_t eax = (uint32_t)(uintptr_t)p1, edx = (uint32_t)(uintptr_t)p2, ecx = mask;
    __asm__ volatile("fldl (%3)\n\tcall *%4"
                     : "+a"(eax), "+d"(edx), "+c"(ecx)
                     : "r"(alpha), "r"(target)
                     : "memory", "cc", "st");
    return eax;
}

static uint32_t check_point_orig(const RwV3d *p1, const RwV3d *p2, uint32_t mask, const double *alpha)
{
    return check_point_call(0x004bc570, p1, p2, mask, alpha);
}

static uint32_t check_point_impl(const RwV3d *p1, const RwV3d *p2, uint32_t mask, const double *alpha)
{
    return check_point_call((uint32_t)(uintptr_t)_rpCollCheckPoint, p1, p2, mask, alpha);
}

static void setup_check_point(DtCtx *c)
{
    collEnv(c);
    double *alpha = dt_alloc(8);
    *alpha = dt_u32() % 10 == 0 ? (dt_u32() % 2 ? 0.0 : 1.0) : (double)dt_float(-0.5f, 1.5f) / 3.0;
    DT_ARG_PTR(c, 0, cube_point());
    DT_ARG_PTR(c, 1, cube_point());
    DT_ARG_INT(c, 2, dt_u32() % 4 == 0 ? dt_u32() : 0x3f);
    DT_ARG_PTR(c, 3, alpha);
    rwtEnd();
}

static int32_t check_line_orig(const RwV3d *p1, const RwV3d *p2, uint32_t outcodes)
{
    return ((int32_t(__attribute__((regparm(3))) *)(const RwV3d *, const RwV3d *, uint32_t))(
        uintptr_t)0x004bc630)(p1, p2, outcodes);
}

static int32_t check_line_impl(const RwV3d *p1, const RwV3d *p2, uint32_t outcodes)
{
    return _rpCollCheckLine(p1, p2, outcodes);
}

static void setup_check_line(DtCtx *c)
{
    collEnv(c);
    RwV3d *p1 = cube_point(), *p2 = cube_point();
    if (dt_u32() % 8 == 0) {
        p2->x = p1->x;
    }
    DT_ARG_PTR(c, 0, p1);
    DT_ARG_PTR(c, 1, p2);
    DT_ARG_INT(c, 2, dt_u32() % 3 == 0 ? dt_u32() : (dt_u32() & 0x3f));
    rwtEnd();
}

typedef int32_t(__attribute__((regparm(3))) * CollBoxTriFn)(const RwBBox *, RwV3d *, const RwV3d *, uint32_t,
                                                            uint32_t, uint32_t, const RwV3d *, const RwV3d *,
                                                            const float *, const RwMatrix *);

static int32_t box_tri_orig(const RwBBox *box, RwV3d *normal, const RwV3d *v0, const RwV3d *v1,
                            const RwV3d *v2, const float *distance, const RwMatrix *matrix)
{
    return ((CollBoxTriFn)(uintptr_t)0x004bff70)(box, normal, v0, 0, 0, 0, v1, v2, distance, matrix);
}

static int32_t box_tri_impl(const RwBBox *box, RwV3d *normal, const RwV3d *v0, const RwV3d *v1,
                            const RwV3d *v2, const float *distance, const RwMatrix *matrix)
{
    return _rpCollTriangleBoxIntersect(box, normal, v0, 0, 0, 0, v1, v2, distance, matrix);
}

static RwV3d *box_corner(const RwBBox *box)
{
    RwV3d *v = dt_alloc(sizeof *v);
    v->x = box->inf.x + (box->sup.x - box->inf.x) * dt_float(-1.0f, 2.0f);
    v->y = box->inf.y + (box->sup.y - box->inf.y) * dt_float(-1.0f, 2.0f);
    v->z = box->inf.z + (box->sup.z - box->inf.z) * dt_float(-1.0f, 2.0f);
    return v;
}

static void setup_box_tri(DtCtx *c)
{
    collEnv(c);
    RwBBox *box = dt_alloc(sizeof *box);
    collPoint(&box->inf);
    box->sup.x = box->inf.x + dt_float(0.5f, 20.0f);
    box->sup.y = box->inf.y + dt_float(0.5f, 20.0f);
    box->sup.z = box->inf.z + dt_float(0.5f, 20.0f);
    RwV3d *v0 = box_corner(box), *v1 = box_corner(box), *v2 = box_corner(box);
    if (dt_u32() % 8 == 0) {
        *v2 = *v1;
    }
    RwMatrix *m = dt_alloc(sizeof *m);
    __builtin_memset(m, 0, sizeof *m);
    m->right.x = 1.0f / (box->sup.x - box->inf.x);
    m->up.y = 1.0f / (box->sup.y - box->inf.y);
    m->at.z = 1.0f / (box->sup.z - box->inf.z);
    m->pos.x = -(box->inf.x + box->sup.x) * 0.5f * m->right.x;
    m->pos.y = -(box->inf.y + box->sup.y) * 0.5f * m->up.y;
    m->pos.z = -(box->inf.z + box->sup.z) * 0.5f * m->at.z;
    RwV3d *normal = dt_alloc(sizeof *normal);
    float *distance = dt_alloc(4);
    dt_fill(normal, sizeof *normal);
    dt_fill(distance, 4);
    DT_ARG_PTR(c, 0, box);
    DT_ARG_PTR(c, 1, normal);
    DT_ARG_PTR(c, 2, v0);
    DT_ARG_PTR(c, 3, v1);
    DT_ARG_PTR(c, 4, v2);
    DT_ARG_PTR(c, 5, distance);
    DT_ARG_PTR(c, 6, m);
    rwtEnd();
}

typedef int32_t(__attribute__((regparm(3))) * CollSphereTriFn)(const RwSphere *, RwV3d *, const RwV3d *,
                                                               uint32_t, uint32_t, uint32_t, const RwV3d *,
                                                               const RwV3d *, float *);

static int32_t sphere_tri_orig(const RwSphere *sphere, RwV3d *normal, const RwV3d *v0, const RwV3d *v1,
                               const RwV3d *v2, float *distance)
{
    return ((CollSphereTriFn)(uintptr_t)0x004bf0b0)(sphere, normal, v0, 0, 0, 0, v1, v2, distance);
}

static int32_t sphere_tri_impl(const RwSphere *sphere, RwV3d *normal, const RwV3d *v0, const RwV3d *v1,
                               const RwV3d *v2, float *distance)
{
    return _rpCollSphereTriangleIntersect(sphere, normal, v0, 0, 0, 0, v1, v2, distance);
}

static void setup_sphere_tri(DtCtx *c)
{
    collEnv(c);
    RwSphere *sphere = dt_alloc(sizeof *sphere);
    collPoint(&sphere->center);
    sphere->radius = dt_u32() % 10 == 0 ? 0.0f : dt_float(0.1f, 10.0f);
    const float spread = dt_u32() % 2 == 0 ? sphere->radius * 2.0f : 25.0f;
    RwV3d *v[3];
    for (int i = 0; i < 3; i++) {
        v[i] = dt_alloc(sizeof(RwV3d));
        collNear(v[i], &sphere->center, spread);
    }
    switch (dt_u32() % 8) {
    case 0:
        *v[2] = *v[0];
        break;
    case 1: /* all three corners on one side of an axis */
        v[0]->x = v[1]->x = v[2]->x = sphere->center.x + sphere->radius + dt_float(0.0f, 1.0f);
        break;
    case 2:
        v[0]->z = sphere->center.z - sphere->radius - 1.0f;
        v[1]->z = v[0]->z - 2.0f;
        v[2]->z = v[0]->z;
        break;
    default:
        break;
    }
    RwV3d *normal = dt_alloc(sizeof *normal);
    float *distance = dt_alloc(4);
    dt_fill(normal, sizeof *normal);
    dt_fill(distance, 4);
    DT_ARG_PTR(c, 0, sphere);
    DT_ARG_PTR(c, 1, normal);
    DT_ARG_PTR(c, 2, v[0]);
    DT_ARG_PTR(c, 3, v[1]);
    DT_ARG_PTR(c, 4, v[2]);
    DT_ARG_PTR(c, 5, distance);
    rwtEnd();
}

/* ---- vector, frame and sector helpers ------------------------------------------------------------------ */

static void setup_fast_sqrt(DtCtx *c)
{
    collEnv(c);
    const uint32_t r = dt_u32() % 8;
    DT_ARG_INT(c, 0, r == 0 ? 0u : (r == 1 ? dt_u32() : collBits(dt_float(0.0f, 1000.0f))));
    rwtEnd();
}

static void setup_normalize(DtCtx *c)
{
    collEnv(c);
    RwV3d *in = dt_alloc(sizeof *in);
    in->x = dt_float_edgy(-100.0f, 100.0f);
    in->y = dt_float_edgy(-100.0f, 100.0f);
    in->z = dt_float_edgy(-100.0f, 100.0f);
    if (dt_u32() % 8 == 0) {
        in->x = in->y = in->z = 0.0f;
    }
    RwV3d *out = dt_alloc(sizeof *out);
    dt_fill(out, sizeof *out);
    DT_ARG_PTR(c, 0, dt_u32() % 3 == 0 ? in : out);
    DT_ARG_PTR(c, 1, in);
    rwtEnd();
}

static void setup_frame_dirty(DtCtx *c)
{
    collEnv(c);
    RwFrame *frame = dt_alloc(sizeof *frame);
    RwFrame *root = dt_alloc(sizeof *root);
    dt_fill(frame, sizeof *frame);
    dt_fill(root, sizeof *root);
    frame->root = dt_u32() % 2 == 0 ? frame : root;
    DT_ARG_PTR(c, 0, frame);
    rwtEnd();
}

static void setup_sector_counts(DtCtx *c)
{
    collEnv(c);
    RpWorldSector *sector = dt_alloc(sizeof *sector);
    dt_fill(sector, sizeof *sector);
    DT_ARG_PTR(c, 0, sector);
    rwtEnd();
}

DT_CASES(rwcollision_cases) = {
    {"RpAtomicGetWorldBoundingSphere", 0x004b1280, RpAtomicGetWorldBoundingSphere, 1, DT_RET_INT,
     setup_world_sphere, 600, 0},
    {"_rpCollBevel2d", (uint32_t)(uintptr_t)bevel2d_orig, bevel2d_impl, 1, DT_RET_INT, setup_bevel, 600, 0},
    {"_rpCollBevel3d", (uint32_t)(uintptr_t)bevel3d_orig, bevel3d_impl, 1, DT_RET_INT, setup_bevel, 600, 0},
    {"_rpCollCheckPoint", (uint32_t)(uintptr_t)check_point_orig, check_point_impl, 4, DT_RET_INT,
     setup_check_point, 800, 0},
    {"_rpCollCheckLine", (uint32_t)(uintptr_t)check_line_orig, check_line_impl, 3, DT_RET_INT,
     setup_check_line, 800, 0},
    {"_rpCollPointOutsideTriangle", (uint32_t)(uintptr_t)point_tri_orig, point_tri_impl, 4, DT_RET_INT,
     setup_point_tri, 2000, 0},
    {"_rpCollWorldForAllLineSectors", (uint32_t)(uintptr_t)_rpCollWorldForAllLineSectors_orig,
     _rpCollWorldForAllLineSectors_impl, 5, DT_RET_INT, setup_line_sectors, 800, 0},
    {"_rpCollSectorAtomicsSphere", 0x004be000, _rpCollSectorAtomicsSphere, 3, DT_RET_INT,
     setup_sector_atomics_sphere, 500, 0},
    {"_rpCollWorldAtomicsPoint", (uint32_t)(uintptr_t)_rpCollWorldAtomicsPoint_orig,
     _rpCollWorldAtomicsPoint_impl, 5, DT_RET_INT, setup_walk_atomics_point, 400, 0},
    {"_rpCollWorldAtomicsLine", (uint32_t)(uintptr_t)_rpCollWorldAtomicsLine_orig,
     _rpCollWorldAtomicsLine_impl, 5, DT_RET_INT, setup_walk_atomics_line, 400, 0},
    {"_rpCollWorldAtomicsSphere", (uint32_t)(uintptr_t)_rpCollWorldAtomicsSphere_orig,
     _rpCollWorldAtomicsSphere_impl, 5, DT_RET_INT, setup_walk_atomics_sphere, 400, 0},
    {"_rpCollWorldAtomicsAtomic", (uint32_t)(uintptr_t)_rpCollWorldAtomicsAtomic_orig,
     _rpCollWorldAtomicsAtomic_impl, 5, DT_RET_INT, setup_walk_atomics_atomic, 400, 0},
    {"_rpCollSectorTrianglesSphere", 0x004bed60, _rpCollSectorTrianglesSphere, 3, DT_RET_INT,
     setup_sector_triangles_sphere, 800, 0},
    {"_rpCollSphereTriangleIntersect", (uint32_t)(uintptr_t)sphere_tri_orig, sphere_tri_impl, 6, DT_RET_INT,
     setup_sphere_tri, 3000, 0},
    {"_rpCollSectorTrianglesBox", 0x004bfb20, _rpCollSectorTrianglesBox, 3, DT_RET_INT,
     setup_sector_triangles_box, 800, 0},
    {"_rpCollTriangleBoxIntersect", (uint32_t)(uintptr_t)box_tri_orig, box_tri_impl, 7, DT_RET_INT,
     setup_box_tri, 3000, 0},
    {"_rpCollWorldTrianglesLine", (uint32_t)(uintptr_t)_rpCollWorldTrianglesLine_orig,
     _rpCollWorldTrianglesLine_impl, 5, DT_RET_INT, setup_walk_triangles_line, 400, 0},
    {"_rpCollWorldTrianglesSphere", (uint32_t)(uintptr_t)_rpCollWorldTrianglesSphere_orig,
     _rpCollWorldTrianglesSphere_impl, 5, DT_RET_INT, setup_walk_triangles_sphere, 400, 0},
    {"_rpCollWorldTrianglesBox", (uint32_t)(uintptr_t)_rpCollWorldTrianglesBox_orig,
     _rpCollWorldTrianglesBox_impl, 5, DT_RET_INT, setup_walk_triangles_box, 400, 0},
    {"_rpCollSectorAtomicsBox", 0x004c0970, _rpCollSectorAtomicsBox, 3, DT_RET_INT, setup_sector_atomics_box,
     500, 0},
    {"_rpCollWorldAtomicsBox", (uint32_t)(uintptr_t)_rpCollWorldAtomicsBox_orig, _rpCollWorldAtomicsBox_impl,
     5, DT_RET_INT, setup_walk_atomics_box, 400, 0},
    {"_rpCollWorldTrianglesAtomic", (uint32_t)(uintptr_t)_rpCollWorldTrianglesAtomic_orig,
     _rpCollWorldTrianglesAtomic_impl, 5, DT_RET_INT, setup_walk_triangles_atomic, 400, 0},
    {"_rpCollAtomicTrianglesLine", (uint32_t)(uintptr_t)_rpCollAtomicTrianglesLine_orig,
     _rpCollAtomicTrianglesLine_impl, 5, DT_RET_INT, setup_atomic_triangles_line, 1000, 0},
    {"_rpCollAtomicTrianglesSphere", (uint32_t)(uintptr_t)_rpCollAtomicTrianglesSphere_orig,
     _rpCollAtomicTrianglesSphere_impl, 5, DT_RET_INT, setup_atomic_triangles_sphere, 1000, 0},
    {"rwWorldIntersectTriangles", 0x004c1b10, rwWorldIntersectTriangles, 4, DT_RET_INT, setup_world_triangles,
     1500, 0},
    {"rwWorldIntersectObjects", 0x004c1c10, rwWorldIntersectObjects, 4, DT_RET_INT, setup_world_objects, 1000,
     0},
    {"rwObjectIntersectTriangles", 0x004c1f30, rwObjectIntersectTriangles, 4, DT_RET_INT,
     setup_object_triangles, 1500, 0},
    {"RpWorldSectorGetNumVertices", 0x004c2050, RpWorldSectorGetNumVertices, 1, DT_RET_INT,
     setup_sector_counts, 20, 0},
    {"RpWorldSectorGetNumPolygons", 0x004c2060, RpWorldSectorGetNumPolygons, 1, DT_RET_INT,
     setup_sector_counts, 20, 0},
    {"RwFrameDirty", 0x004d2840, RwFrameDirty, 1, DT_RET_INT, setup_frame_dirty, 50, 0},
    {"_rwV3dNormalize", 0x004d3650, _rwV3dNormalize, 2, DT_RET_DOUBLE, setup_normalize, 500, 0},
    {"RwV3dNormalize", 0x004d36e0, RwV3dNormalize, 2, DT_RET_DOUBLE, setup_normalize, 500, 0},
    {"_rwFastSqrt", 0x004d3880, _rwFastSqrt, 1, DT_RET_DOUBLE, setup_fast_sqrt, 500, 0},
};
