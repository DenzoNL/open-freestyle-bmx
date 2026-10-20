/*
 * physEnterGrindIn (0x421190) and physEnterAir (0x422e50). Level queries, the ray test, trick lookups,
 * sound and skid marks are replaced per iteration with dt_stub() stubs that serve scenes built around
 * the rider's flight path and fold their arguments into bigLog, so both sides must make the same calls.
 */
#include <stdarg.h>

#include "bmx/com/action/physics.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "dt.h"
#include "dt_zactor.h"
#include "zge/zdebug.h"

/* ---- call log ------------------------------------------------------------------------------------- */

static uint32_t bigLog;
static uint32_t bigCalls;

static void logU32(uint32_t v)
{
    bigLog = (bigLog ^ v) * 16777619u + 0x9e3779b9u;
    bigCalls++;
}

static void logBytes(const void *p, uint32_t n)
{
    const uint8_t *b = p;
    for (uint32_t i = 0; i < n; i++) {
        logU32(b[i]);
    }
}

static void logVec(const ZVec3 *v)
{
    logBytes(v, sizeof *v);
}

static void logFloat(float f)
{
    logBytes(&f, sizeof f);
}

/* ---- scene served by the stubs -------------------------------------------------------------------- */

enum { kMaxPath = 40 };

static struct {
    void **edges;         /* zbspFindEdgesNear */
    void **props;         /* zbspFindPropsNear */
    ZbspFaceQuery *faces; /* zbspFindFaceNear */
    uint32_t faceMask;    /* bit k: the k-th face query finds anything */
    uint32_t faceCalls;
    uint32_t trickA, trickB, trickKind;
    uint32_t rayCalls;
    uint32_t rayHit[2];   /* return value of the k-th ray */
    PhysRayHit rayRec[2]; /* record of the k-th ray */
    PhysEdge *tagged[3];  /* zbspFindEdge results, NULL-terminated */
    uint32_t enterResult; /* the stub grind enter handler's state */
    uint8_t enterSurface; /* ... and the surface it leaves (0xff: unchanged) */
    ZVec3 path[kMaxPath]; /* where the grind-in search will probe */
} gScene;

static void **BMX_CDECL stubEdgesNear(const ZVec3 *p, float radius)
{
    logVec(p);
    logFloat(radius);
    return gScene.edges;
}

static void **BMX_CDECL stubPropsNear(int lo, int hi)
{
    logU32((uint32_t)lo);
    logU32((uint32_t)hi);
    return gScene.props;
}

static void *BMX_CDECL stubFaceNear(const ZVec3 *p, float radius, float cosLimit)
{
    const uint32_t k = gScene.faceCalls++;
    logVec(p);
    logFloat(radius);
    logFloat(cosLimit);
    return ((gScene.faceMask >> (k & 31)) & 1) ? gScene.faces : NULL;
}

static uint32_t BMX_CDECL stubTrickA(const void *r)
{
    logU32((uint32_t)(uintptr_t)r);
    return gScene.trickA;
}

static uint32_t BMX_CDECL stubTrickB(const void *r)
{
    logU32((uint32_t)(uintptr_t)r);
    return gScene.trickB;
}

static uint32_t BMX_CDECL stubTrickKind(const void *r)
{
    logU32((uint32_t)(uintptr_t)r);
    return gScene.trickKind;
}

static void BMX_CDECL stubSkidMark(const ZVec3 *from, const ZVec3 *to, int flags, const ZVec3 *normal)
{
    logVec(from);
    logVec(to);
    logU32((uint32_t)flags);
    logVec(normal);
}

static int BMX_CDECL stubRay(const ZVec3 *from, const ZVec3 *to, void *hit)
{
    const uint32_t k = gScene.rayCalls++ & 1;
    logVec(from);
    logVec(to);
    *(PhysRayHit *)hit = gScene.rayRec[k];
    return (int)gScene.rayHit[k];
}

static int BMX_CDECL stubSound(int id, const void *position, int a, int volume, int b)
{
    logU32((uint32_t)id);
    logVec(position);
    logU32((uint32_t)a);
    logU32((uint32_t)volume);
    logU32((uint32_t)b);
    return 0;
}

static void *BMX_CDECL stubFindEdge(int unused, int id, int index, int unused2)
{
    logU32((uint32_t)id);
    logU32((uint32_t)index);
    return index >= 0 && index < 3 ? gScene.tagged[index] : NULL;
}

static void BMX_CDECL stubPrint(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    for (const char *p = fmt; *p; p++) {
        logU32((uint8_t)*p);
        if (*p == '%') {
            logU32((uint32_t)va_arg(ap, int));
        }
    }
    va_end(ap);
}

static int BMX_CDECL stubExit(PhysRider *r, int from, int to)
{
    logU32((uint32_t)from * 7u + (uint32_t)to);
    return 0;
}

static int BMX_CDECL stubEnter(PhysRider *r, int from, int to)
{
    logU32((uint32_t)from * 11u + (uint32_t)to);
    if (gScene.enterSurface != 0xff) {
        r->surface = gScene.enterSurface;
    }
    return (int)gScene.enterResult;
}

static void install_stubs(void)
{
    gScene.faceCalls = 0;
    gScene.rayCalls = 0;
    bigLog = 0;
    bigCalls = 0;
    dt_stub(0x00466ed0, NULL, (void *)stubEdgesNear);
    dt_stub(0x00468e30, (void *)zbspFindPropsNear, (void *)stubPropsNear);
    dt_stub(0x00468070, NULL, (void *)stubFaceNear);
    dt_stub(0x00435ea0, NULL, (void *)stubTrickA);
    dt_stub(0x00435ef0, NULL, (void *)stubTrickB);
    dt_stub(0x00435f40, NULL, (void *)stubTrickKind);
    dt_stub(0x00409250, NULL, (void *)stubSkidMark);
    dt_stub(0x00469b00, NULL, (void *)stubRay);
    dt_stub(0x0044e300, NULL, (void *)stubSound);
    dt_stub(0x00467410, (void *)zbspFindEdge, (void *)stubFindEdge);
    gZdebugPrint = stubPrint;
    for (int i = 0; i < kPhysStateCount; i++) {
        gPhysStateExit[i] = (dt_u32() & 3) ? stubExit : NULL;
        gPhysStateEnter[i] = stubEnter;
    }
    gPhysStateExit[2] = stubExit;
}

/* ---- fixtures -------------------------------------------------------------------------------------- */

static float rf(float lo, float hi)
{
    return dt_float(lo, hi);
}

static void rand_vec(ZVec3 *v, float range)
{
    v->x = dt_float_edgy(-range, range);
    v->y = dt_float_edgy(-range, range);
    v->z = dt_float_edgy(-range, range);
}

static void normalize(ZVec3 *v)
{
    const float len = __builtin_sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
    if (len > 0.0f) {
        v->x /= len;
        v->y /= len;
        v->z /= len;
    }
}

static void cross(ZVec3 *out, const ZVec3 *a, const ZVec3 *b)
{
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

/* Random orthonormal frame; every 8th is axis aligned (up = -y). */
static void rand_frame(ZVec3 *fwd, ZVec3 *up, ZVec3 *side)
{
    if ((dt_u32() & 7) == 0) {
        *fwd = (ZVec3){0.0f, 0.0f, 1.0f};
        *up = (ZVec3){0.0f, -1.0f, 0.0f};
    } else {
        rand_vec(fwd, 1.0f);
        fwd->x += 0.01f;
        normalize(fwd);
        rand_vec(up, 1.0f);
        if (dt_u32() & 1) {
            up->y = -1.5f; /* mostly upright */
        }
        cross(side, fwd, up);
        normalize(side);
        cross(up, side, fwd);
        normalize(up);
    }
    cross(side, fwd, up);
}

static void rand_unit(ZVec3 *v)
{
    ZVec3 a, b;
    rand_frame(v, &a, &b);
}

/* Edge record from a to b with a random plane through it. */
static PhysEdge *make_edge(const ZVec3 *p, float spread)
{
    PhysEdge *e = dt_alloc(500);
    ZVec3 d;

    dt_fill(e, 500);
    *(uint8_t *)e = (uint8_t)dt_range(0, 3); /* solid-side flags read by zbspEdgeSide */
    for (int i = 0; i < 3; i++) {
        const float c = i == 0 ? p->x : i == 1 ? p->y : p->z;
        e->a[i] = (int16_t)(c + rf(-spread, spread));
        e->b[i] = (int16_t)(c + rf(-spread, spread));
    }
    if ((dt_u32() & 15) == 0) {
        e->b[1] = e->a[1]; /* level edge */
    }
    d.x = (float)(e->b[0] - e->a[0]);
    d.y = (float)(e->b[1] - e->a[1]);
    d.z = (float)(e->b[2] - e->a[2]);
    normalize(&d);
    e->dir = d;
    rand_unit(&e->normal);
    e->d = -(e->normal.x * e->a[0] + e->normal.y * e->a[1] + e->normal.z * e->a[2]) + rf(-60, 60);
    e->kind = (int8_t)dt_range(0, 2);
    e->length = (int16_t)dt_range(0, 3000);
    e->tag = (uint16_t)dt_u32();
    e->prop = NULL;
    return e;
}

static PhysFace *make_face(void)
{
    PhysFace *f = dt_alloc(500);

    dt_fill(f, 500);
    rand_unit(&f->normal);
    f->d = rf(-1000, 1000);
    f->flags = (uint8_t)((dt_u32() & 3) ? 0 : dt_u32());
    f->tag = (uint16_t)dt_u32();
    return f;
}

static PhysPropObject *make_prop(const ZVec3 *p, float spread)
{
    PhysPropObject *o = dt_alloc(sizeof *o);
    PhysPropInfo *info = dt_alloc(sizeof *info);
    PhysPropNode *node = dt_alloc(sizeof *node);
    PhysPropFrame *frame = dt_alloc(sizeof *frame);
    ZQuat q;
    float len;

    dt_fill(o, sizeof *o);
    o->rot.x = (ZAngle)dt_range(-0x800, 0x800);
    o->rot.y = (ZAngle)dt_u32();
    o->rot.z = (ZAngle)dt_range(-0x800, 0x800);
    o->pos[0] = (int16_t)(p->x + rf(-spread, spread));
    o->pos[1] = (int16_t)(p->y + rf(-spread, spread));
    o->pos[2] = (int16_t)(p->z + rf(-spread, spread));
    rand_unit(&info->facing);
    q.x = rf(-1, 1);
    q.y = rf(-1, 1);
    q.z = rf(-1, 1);
    q.w = rf(-1, 1);
    len = __builtin_sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len < 0.1f) {
        q = (ZQuat){0.0f, 0.0f, 0.0f, 1.0f};
        len = 1.0f;
    }
    frame->rot = (ZQuat){q.x / len, q.y / len, q.z / len, q.w / len};
    node->frame = frame;
    o->node = node;
    o->prop = (dt_u32() & 7) ? info : NULL;
    return o;
}

static void tunables(void)
{
    gPhysGravity = (dt_u32() & 3) ? 1.58f : rf(0.3f, 4.0f);
    gPhysGrindMinSpeed = (dt_u32() & 1) ? 6.0f : rf(0, 30);
    gPhysGrindInRadius = (dt_u32() & 3) ? 150.0f : rf(0, 400);
    gPhysEdgeSearchRadius = (dt_u32() & 1) ? 30.0f : rf(0, 80);
    gPhysGrindInSteps = (dt_u32() & 7) ? 12u : (uint32_t)dt_range(0, 8);
    gPhysStallInSteps = (dt_u32() & 7) ? 24u : (uint32_t)dt_range(0, 8);
    gPhysPipeAngleLimit = (int16_t)((dt_u32() & 3) ? 0x238e : dt_range(0, 0x4000));
    gPhysPipeTiltLimit = (int16_t)((dt_u32() & 3) ? 0x271c : dt_range(0, 0x4000));
    gPhysAirAlignFrames = (dt_u32() & 3) ? 15 : dt_range(1, 40);
    gPhysAirSpeedCap = (dt_u32() & 1) ? 16.0f : rf(0, 40);
    gPhysPipeLaunchScale = (dt_u32() & 1) ? 0.85f : rf(0, 2);
}

/* A rider in flight or on the ground; contact matches `surface`. */
static PhysRider *make_rider(int surface)
{
    PhysRider *r = dt_alloc(sizeof *r);

    dt_fill(r, sizeof *r);
    r->flags = dt_u32();
    r->player = (uint8_t)dt_range(0, 3);
    r->anim = (uint8_t)dt_range(0x23, 0x2d);
    r->state = (uint8_t)dt_range(0, 6);
    r->surface = (uint8_t)surface;
    r->timer = (int16_t)dt_range(0, 4);
    rand_frame(&r->fwd, &r->up, &r->side);
    rand_vec(&r->omega, 0.1f);
    rand_vec(&r->pos, 2000.0f);
    rand_vec(&r->step, 30.0f);
    r->groundGap = rf(-50, 50);
    rand_vec(&r->vel, 30.0f);
    rand_vec(&r->dvel, 3.0f);
    rand_vec(&r->airVel, 10.0f);
    r->speed = rf(0, 60);
    rand_vec(&r->contactPos, 2000.0f);
    r->center.x = r->pos.x + rf(-20, 20);
    r->center.y = r->pos.y + rf(-60, 20);
    r->center.z = r->pos.z + rf(-20, 20);
    r->fieldBA4 = dt_u32();
    for (int i = 0; i < 2; i++) {
        r->wheel[i].radius = rf(5, 20);
    }
    if ((surface >= 1 && surface <= 3) || surface == 9) {
        r->contact = make_edge(&r->pos, 300.0f);
    } else if (surface == 12) {
        r->contact = make_prop(&r->pos, 100.0f);
    } else {
        r->contact = make_face();
    }
    return r;
}

/* Probe positions of the grind-in search (approximately; placement only). */
static void plan_path(const PhysRider *r)
{
    ZVec3 p = r->center, v = r->vel;

    for (int k = 0; k < kMaxPath; k++) {
        gScene.path[k] = p;
        v.y += gPhysGravity;
        p.x += v.x;
        p.y += v.y;
        p.z += v.z;
    }
}

/*
 * A probe position of the search; one in 8 is mirrored ~30000 units away, so squared distances
 * overflow 32 bits as in the original.
 */
static const ZVec3 *path_point(uint32_t steps)
{
    static ZVec3 far;
    const uint32_t n = steps < 1 ? 1 : steps > kMaxPath ? kMaxPath : steps;
    const ZVec3 *p = &gScene.path[dt_range(0, (int32_t)n - 1)];

    if ((dt_u32() & 7) == 0) {
        far.x = p->x > 0.0f ? -30000.0f : 30000.0f;
        far.y = p->y > 0.0f ? -30000.0f : 30000.0f;
        far.z = p->z > 0.0f ? -30000.0f : 30000.0f;
        return &far;
    }
    return p;
}

/* ---- physEnterGrindIn ---------------------------------------------------------------------------- */

static void scene_edges(const PhysRider *r, uint32_t steps)
{
    const int n = dt_range(0, 8);
    void **list = (void **)dt_alloc((uint32_t)(n + 1) * sizeof(void *));

    for (int i = 0; i < n; i++) {
        PhysEdge *e = make_edge(path_point(steps), (dt_u32() & 1) ? 40.0f : 150.0f);
        if (dt_u32() & 1) {
            /* lined up with the bike */
            e->dir = r->fwd;
            if (dt_u32() & 1) {
                e->dir.y = 0.0f;
                normalize(&e->dir);
            }
        }
        list[i] = e;
    }
    list[n] = NULL;
    gScene.edges = (dt_u32() & 15) ? list : NULL;
}

static void scene_props(const PhysRider *r, uint32_t steps)
{
    const int n = dt_range(0, 4);
    void **list = (void **)dt_alloc((uint32_t)(n + 1) * sizeof(void *));

    for (int i = 0; i < n; i++) {
        PhysPropObject *o = make_prop(path_point(steps), 60.0f);
        if (o->prop != NULL && (dt_u32() & 1)) {
            /* facing the rider */
            PhysPropInfo *info = (PhysPropInfo *)o->prop;
            info->facing.x = -r->vel.x;
            info->facing.y = 0.0f;
            info->facing.z = -r->vel.z;
            normalize(&info->facing);
        }
        list[i] = o;
    }
    list[n] = NULL;
    gScene.props = (dt_u32() & 7) ? list : NULL;
}

static void scene_faces(const PhysRider *r, uint32_t steps)
{
    const int n = dt_range(0, 6);
    ZbspFaceQuery *q = dt_alloc(sizeof *q);
    ZbspFaceHit *h = dt_alloc((uint32_t)(n + 1) * sizeof *h);

    dt_fill(q, sizeof *q);
    for (int i = 0; i < n; i++) {
        PhysFace *f = make_face();
        const ZVec3 *p = path_point(steps);

        dt_fill(&h[i], sizeof h[i]);
        if (dt_u32() & 3) {
            /* a steep lip face turned against the flight */
            f->normal.x = -r->vel.x + rf(-3, 3);
            f->normal.y = rf(-0.3f, 0.3f) * (__builtin_fabsf(r->vel.x) + __builtin_fabsf(r->vel.z) + 1.0f);
            f->normal.z = -r->vel.z + rf(-3, 3);
            normalize(&f->normal);
        }
        h[i].face = f;
        h[i].dist = (dt_u32() & 7) ? rf(0, 50) : rf(-50, 0);
        h[i].point.x = p->x + rf(-60, 60);
        h[i].point.y = p->y + rf(-60, 60);
        h[i].point.z = p->z + rf(-60, 60);
        h[i].object = (dt_u32() & 7) ? 0 : (int32_t)dt_u32();
    }
    h[n].face = NULL;
    q->hits = h;
    gScene.faces = q;
    gScene.faceMask = (dt_u32() & 3) ? 0xffffffffu : dt_u32();
}

static uint32_t rand_trick(void)
{
    switch (dt_u32() & 7) {
    case 0:
    case 1:
        return (uint32_t)dt_range(0x27, 0x29);
    case 2:
        return 0x2a;
    case 3:
    case 4:
        return 0x2b;
    default:
        return (uint32_t)dt_range(0, 0x40);
    }
}

static void setup_grind_in(DtCtx *c)
{
    PhysRider *r;
    uint32_t steps;
    int to;

    dt_seed(c);
    install_stubs();
    tunables();
    r = make_rider(dt_range(0, 14));
    if (dt_u32() & 1) {
        r->state = kPhysStateAir;
    }
    if (dt_u32() & 1) {
        /* a typical jump: up and forwards */
        r->vel.x = rf(-15, 15);
        r->vel.y = rf(-25, 5);
        r->vel.z = rf(-15, 15);
    }
    to = (dt_u32() & 1) ? kPhysStateGrindIn : kPhysStateStallIn;
    gScene.trickA = rand_trick();
    gScene.trickB = rand_trick();
    gScene.trickKind = (dt_u32() & 1) ? (uint32_t)dt_range(0x2a, 0x2b) : rand_trick();
    steps = to == kPhysStateGrindIn ? gPhysGrindInSteps : gPhysStallInSteps;
    plan_path(r);
    scene_edges(r, steps);
    scene_props(r, steps);
    scene_faces(r, steps);
    DT_ARG_PTR(c, 0, r);
    DT_ARG_INT(c, 1, (dt_u32() & 1) ? kPhysStateRoll : dt_range(0, 6));
    DT_ARG_INT(c, 2, (dt_u32() & 15) ? to : dt_range(0, 6));
}

/* ---- physEnterAir --------------------------------------------------------------------------------- */

static void make_ray(PhysRayHit *h, const PhysRider *r)
{
    dt_fill(h, sizeof *h);
    h->point.x = r->pos.x + rf(-50, 50);
    h->point.y = r->pos.y + rf(-50, 400);
    h->point.z = r->pos.z + rf(-50, 50);
    h->id = dt_range(0, 50);
    switch (dt_u32() & 7) {
    case 0:
        h->kind = 0;
        break;
    case 1:
        h->kind = 1;
        h->face = NULL;
        break;
    case 2:
        h->kind = 2;
        h->objectData = (dt_u32() & 3) ? (PhysCollObject *)(void *)make_prop(&r->pos, 50.0f) : NULL;
        break;
    default: {
        PhysFace *f = make_face();
        h->kind = 1 | (dt_u32() & 2);
        f->tag = (uint16_t)((dt_u32() & 1) ? dt_range(0x1000, 0x3fff) : dt_u32());
        h->face = f;
        break;
    }
    }
}

/* Rider leaving a lip or quarter-pipe edge: rolling up it, up close to the edge normal. */
static void lip_launch(PhysRider *r)
{
    PhysEdge *e = r->contact;
    ZVec3 n = e->normal;
    float d;

    if (dt_u32() & 1) {
        n.y = rf(-0.3f, 0.3f);
        normalize(&n);
        e->normal = n;
    }
    d = rf(-150, 400);
    e->d = -(n.x * e->a[0] + n.y * e->a[1] + n.z * e->a[2]);
    r->pos.x = (float)e->a[0] + n.x * d + rf(-10, 10);
    r->pos.y = (float)e->a[1] + rf(-300, 100);
    r->pos.z = (float)e->a[2] + n.z * d + rf(-10, 10);
    if (dt_u32() & 1) {
        r->up = n;
        if (dt_u32() & 1) {
            r->up.x = -r->up.x;
            r->up.z = -r->up.z;
        }
        cross(&r->side, &r->fwd, &r->up);
        normalize(&r->side);
        cross(&r->fwd, &r->up, &r->side);
        normalize(&r->fwd);
    }
    if (dt_u32() & 1) {
        r->side.y = rf(-0.4f, 0.4f);
    }
    r->vel.y = (dt_u32() & 7) ? rf(-40, 0) : rf(-5, 5);
}

static void setup_enter_air(DtCtx *c)
{
    PhysRider *r;
    int from;

    dt_seed(c);
    install_stubs();
    tunables();
    switch (dt_u32() & 3) {
    case 0:
        from = kPhysStateRoll;
        break;
    case 1:
        from = kPhysStateGrind;
        break;
    case 2:
        from = kPhysStateStall;
        break;
    default:
        from = (dt_u32() & 1) ? kPhysStateRoll : dt_range(0, 6);
        break;
    }
    if (from == kPhysStateRoll && (dt_u32() & 1)) {
        r = make_rider(dt_range(1, 3));
        lip_launch(r);
    } else if (from == kPhysStateGrind || from == kPhysStateStall) {
        r = make_rider((dt_u32() & 1) ? 9 : dt_range(8, 13));
    } else {
        r = make_rider(dt_range(0, 14));
    }
    if (r->surface == 9 && (dt_u32() & 1)) {
        ((PhysEdge *)r->contact)->kind = 1;
    }
    r->flags &= ~(uint32_t)kPhysGrindRequest;
    if ((dt_u32() & 3) == 0) {
        r->flags |= kPhysGrindRequest;
    }
    if (dt_u32() & 1) {
        r->flags |= kPhysNoPipeAlign;
    }
    for (int k = 0; k < 2; k++) {
        gScene.rayHit[k] = k == 0 ? (dt_u32() & 7) != 0 : dt_u32() & 1;
        make_ray(&gScene.rayRec[k], r);
    }
    for (int k = 0; k < 3; k++) {
        gScene.tagged[k] = (dt_u32() & 3) ? make_edge(&r->pos, 300.0f) : NULL;
    }
    switch (dt_u32() & 3) {
    case 0:
        gScene.enterResult = kPhysStateGrind;
        break;
    case 1:
        gScene.enterResult = kPhysStateStall;
        break;
    default:
        gScene.enterResult = (uint32_t)dt_range(0, 6);
        break;
    }
    gScene.enterSurface = (dt_u32() & 3) ? 0xff : (uint8_t)dt_range(0, 12);
    DT_ARG_PTR(c, 0, r);
    DT_ARG_INT(c, 1, from);
    DT_ARG_INT(c, 2, kPhysStateAir);
}

DT_CASES(physics_big_cases) = {
    {"physEnterGrindIn", 0x00421190, physEnterGrindIn, 3, DT_RET_INT, setup_grind_in, 3000, 0},
    {"physEnterAir", 0x00422e50, physEnterAir, 3, DT_RET_INT, setup_enter_air, 3000, 0},
};
