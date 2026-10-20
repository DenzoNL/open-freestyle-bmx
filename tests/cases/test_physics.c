#include <stdarg.h>

#include "bmx/com/action/physics.h"
#include "dt.h"
#include "dt_zactor.h"
#include "zge/zquat.h"
#include "zge/zstring.h"

/* ---- fixtures ------------------------------------------------------------------------------------ */

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

/* A random rotation; every 8th one is axis aligned (up = -y), which hits the exact-compare edges. */
static void rand_frame(ZVec3 *a, ZVec3 *b, ZVec3 *c)
{
    ZMatrix m;
    ZQuat q;

    if ((dt_u32() & 7) == 0) {
        q.x = q.y = q.z = 0.0f;
        q.w = 1.0f;
    } else {
        do {
            q.x = rf(-1, 1);
            q.y = rf(-1, 1);
            q.z = rf(-1, 1);
            q.w = rf(-1, 1);
        } while (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w < 0.05f);
    }
    zquatToMatrix(&m, &q);
    *a = m.right;
    *b = m.up;
    *c = m.at;
}

static void zvec3NegateLocal(ZVec3 *v)
{
    v->x = -v->x;
    v->y = -v->y;
    v->z = -v->z;
}

static void rand_unit(ZVec3 *v)
{
    ZVec3 b, c;
    rand_frame(v, &b, &c);
}

static void rand_matrix(ZMatrix *m)
{
    dt_fill(m, sizeof *m);
    rand_frame(&m->right, &m->up, &m->at);
    rand_vec(&m->pos, 500.0f);
}

static void rand_angles(ZAngle3 *a, int range)
{
    a->x = (ZAngle)dt_range(-range, range);
    a->y = (ZAngle)dt_range(-range, range);
    a->z = (ZAngle)dt_range(-range, range);
}

static int16_t *rand_point16(const ZVec3 *near, int spread)
{
    int16_t *p = dt_alloc(8);
    p[0] = (int16_t)((int)near->x + dt_range(-spread, spread));
    p[1] = (int16_t)((int)near->y + dt_range(-spread, spread));
    p[2] = (int16_t)((int)near->z + dt_range(-spread, spread));
    return p;
}

/* Random edge record near p. Every 16th one is degenerate (a == b). */
static PhysEdge *rand_edge(const ZVec3 *p, int spread)
{
    PhysEdge *e = dt_alloc(500);
    float len;

    dt_fill(e, 500);
    for (int i = 0; i < 3; i++) {
        const float c = i == 0 ? p->x : i == 1 ? p->y : p->z;
        e->a[i] = (int16_t)((int)c + dt_range(-spread, spread));
        e->b[i] = (int16_t)((int)c + dt_range(-spread, spread));
    }
    if ((dt_u32() & 15) == 0) {
        e->b[0] = e->a[0];
        e->b[1] = e->a[1];
        e->b[2] = e->a[2];
    }
    e->dir.x = (float)(e->b[0] - e->a[0]);
    e->dir.y = (float)(e->b[1] - e->a[1]);
    e->dir.z = (float)(e->b[2] - e->a[2]);
    len = __builtin_sqrtf(e->dir.x * e->dir.x + e->dir.y * e->dir.y + e->dir.z * e->dir.z);
    if (len > 0.0f) {
        e->dir.x /= len;
        e->dir.y /= len;
        e->dir.z /= len;
    }
    rand_unit(&e->normal);
    e->d = -(e->normal.x * e->a[0] + e->normal.y * e->a[1] + e->normal.z * e->a[2]) + rf(-50, 50);
    e->kind = (int8_t)dt_range(-1, 3);
    e->length = (int16_t)dt_range(0, 3000);
    e->tag = (uint16_t)dt_u32();
    e->prop = NULL;
    return e;
}

/* Random face record (normal mostly facing up, i.e. -y). */
static PhysFace *rand_face(const ZVec3 *p)
{
    PhysFace *f = dt_alloc(500);

    dt_fill(f, 500);
    rand_unit(&f->normal);
    if (f->normal.y > 0.0f && (dt_u32() & 3)) {
        f->normal.y = -f->normal.y;
    }
    f->d = rf(-1000, 1000);
    f->flags = (uint8_t)dt_u32();
    f->tag = (uint16_t)dt_u32();
    for (int i = 0; i < 4; i++) {
        f->vert[i] = rand_point16(p, 300);
    }
    return f;
}

/* Contact record matching surface kind s. */
static void *rand_contact(int s, const ZVec3 *p)
{
    if (s == 1 || s == 2 || s == 3 || s == 9) {
        return rand_edge(p, 400);
    }
    if (s == 12) {
        uint8_t *rec = dt_alloc(500);
        dt_fill(rec, 500);
        *(void **)(rec + 0x1ec) = NULL;
        return rec;
    }
    return rand_face(p);
}

static void rand_wheel(PhysWheel *w)
{
    rand_vec(&w->anchor, 40.0f);
    w->radius = rf(5, 20);
    rand_vec(&w->vel, 10.0f);
    w->speed = rf(0, 40);
    rand_frame(&w->dir, &w->normal, &w->side);
    w->mode = (uint8_t)dt_range(0, 2);
    w->touch = (uint8_t)dt_u32();
    w->angle = (int16_t)dt_u32();
    w->spin = (dt_u32() & 3) ? dt_range(-3000, 3000) : (int32_t)dt_u32();
    w->travel = rf(-5, 5);
    w->travelVel = rf(-5, 5);
    w->material = dt_range(0, 12);
}

/* Motion object (rider/bike "actor2"): matrix at +8, velocity at +0x88, position at +0xe70. */
static uint8_t *rand_motion_obj(void)
{
    uint8_t *o = dt_alloc(0x1000);
    dt_fill(o, 0x1000);
    rand_matrix((ZMatrix *)(void *)(o + 8));
    rand_vec((ZVec3 *)(void *)(o + 0x88), 20.0f);
    rand_vec((ZVec3 *)(void *)(o + 0xe70), 1000.0f);
    return o;
}

static ZActor *riderActorOf(const PhysRider *r)
{
    return (ZActor *)r->actor;
}

static ZActor *bikeActorOf(const PhysRider *r)
{
    return (ZActor *)r->bikeObj;
}

static void fill_rider(PhysRider *r, int surface)
{
    dt_fill(r, sizeof *r);
    r->flags = dt_u32();
    r->player = (uint8_t)dt_range(0, 3);
    r->anim = (uint8_t)dt_range(0x23, 0x2d);
    r->field58 = (int32_t)dt_u32();
    r->impactTotal = rf(0, 100);
    r->state = (uint8_t)dt_range(0, 6);
    r->surface = (uint8_t)surface;
    r->timer = (int16_t)dt_range(0, 4);
    rand_frame(&r->fwd, &r->up, &r->side);
    rand_vec(&r->omega, 0.1f);
    rand_vec(&r->pos, 2000.0f);
    rand_vec(&r->step, 30.0f);
    r->groundGap = rf(-50, 50);
    r->landQuality = rf(0, 1);
    rand_vec(&r->vel, 30.0f);
    rand_vec(&r->dvel, 3.0f);
    rand_vec(&r->airVel, 10.0f);
    rand_angles(&r->rot, (dt_u32() & 1) ? 0x400 : 0x7fff);
    r->yawTrim = (int16_t)dt_range(-0x2000, 0x2000);
    r->speed = (dt_u32() & 7) == 0 ? 0.0f : rf(0, 60);
    r->contact = rand_contact(surface, &r->pos);
    rand_vec(&r->contactPos, 2000.0f);
    rand_frame(&r->contactAxis0, &r->contactAxis1, &r->contactAxis2);
    r->field6E0 = rf(0, 1);
    r->field6E4 = rf(0, 1);
    rand_matrix(&r->orientA);
    rand_angles(&r->orientARot, 0x800);
    rand_matrix(&r->orientB);
    rand_angles(&r->orientBRot, 0x800);
    r->snapA.surface = (uint8_t)dt_range(0, 14);
    r->snapA.contact = rand_contact(r->snapA.surface, &r->pos);
    r->snapB.surface = (uint8_t)dt_range(0, 14);
    r->snapB.contact = rand_contact(r->snapB.surface, &r->pos);
    rand_vec(&r->snapAPos, 2000.0f);
    rand_vec(&r->snapBPos, 2000.0f);
    r->actor = dt_rand_actor(kZactorTypePerson);
    r->actor2 = rand_motion_obj();
    r->fieldB9C = (int16_t)dt_range(0, 3);
    r->fieldBA4 = dt_u32();
    rand_vec(&r->fieldBFC, 30.0f);
    rand_vec(&r->center, 2000.0f);
    rand_matrix(&r->basis);
    r->fieldC60 = rf(-1, 1);
    r->fieldC64 = rf(-1, 1);
    for (int i = 0; i < 2; i++) {
        rand_wheel(&r->wheel[i]);
    }
    r->bikeObj = dt_rand_actor(kZactorTypePerson);
    r->bikeObj2 = rand_motion_obj();
    /* the rider's skeleton always has its root bone pair */
    riderActorOf(r)->player->ch[0].boneCount = 2;
    bikeActorOf(r)->player->ch[0].boneCount = 2;
}

static int rand_surface(void)
{
    return dt_range(0, 14);
}

static PhysRider *rand_rider(int surface)
{
    PhysRider *r = dt_alloc(sizeof *r);
    fill_rider(r, surface);
    return r;
}

/* ---- state handler stubs (installed in the exe's state tables; both sides call the same ones) ----- */

static int BMX_CDECL stubExit(PhysRider *r, int from, int to)
{
    r->fieldBA4 = r->fieldBA4 * 31u + (uint32_t)from * 7u + (uint32_t)to + 1u;
    return 0x55;
}

static int BMX_CDECL stubEnter(PhysRider *r, int from, int to)
{
    r->fieldBA4 = r->fieldBA4 * 37u + (uint32_t)from * 11u + (uint32_t)to + 3u;
    return (int)(0x300 + (r->fieldBA4 % 7u));
}

static void BMX_CDECL stubStepA(PhysRider *r)
{
    r->fieldBA4 = r->fieldBA4 * 41u + r->state + 5u;
}

static void BMX_CDECL stubStepB(PhysRider *r)
{
    r->fieldBA4 = r->fieldBA4 * 43u + r->state + 9u;
    if ((r->fieldBA4 & 0x30) == 0x30) {
        r->state = (uint8_t)(r->fieldBA4 % 7u); /* lets the update loop see state changes */
    }
}

static void install_stubs(void)
{
    for (int i = 0; i < kPhysStateCount; i++) {
        gPhysStateExit[i] = (dt_u32() & 3) ? stubExit : NULL;
        gPhysStateEnter[i] = stubEnter;
        gPhysStateOrient[i] = (dt_u32() & 3) ? stubStepA : NULL;
        gPhysStateVelocity[i] = (dt_u32() & 3) ? stubStepB : NULL;
        gPhysStateMove[i] = (dt_u32() & 3) ? ((dt_u32() & 1) ? stubStepB : stubStepA) : NULL;
        gPhysStateSync[i] = (dt_u32() & 3) ? stubStepB : NULL;
    }
    gPhysStateExit[2] = stubExit;
    gPhysStateMove[2] = stubStepB;
}

/* ---- pure and leaf functions ------------------------------------------------------------------- */

static void setup_none(DtCtx *c)
{
    dt_seed(c);
}

static void setup_rider(DtCtx *c)
{
    dt_seed(c);
    install_stubs();
    DT_ARG_PTR(c, 0, rand_rider(rand_surface()));
}

/* Rider on a grind edge (surface 9). */
static void setup_rider_edge(DtCtx *c)
{
    dt_seed(c);
    install_stubs();
    DT_ARG_PTR(c, 0, rand_rider(9));
}

static void setup_set_state(DtCtx *c)
{
    PhysRider *r;
    dt_seed(c);
    install_stubs();
    r = rand_rider(rand_surface());
    if (c->seed % 5 == 0) {
        r->state = kPhysStateNone;
    }
    DT_ARG_PTR(c, 0, r);
    DT_ARG_INT(c, 1, (c->seed % 3 == 0) ? r->state : dt_range(0, 6));
}

static void setup_transition(DtCtx *c)
{
    dt_seed(c);
    install_stubs();
    DT_ARG_PTR(c, 0, rand_rider(rand_surface()));
    DT_ARG_INT(c, 1, dt_range(0, 6));
    DT_ARG_INT(c, 2, dt_range(0, 6));
}

static void setup_add_velocity(DtCtx *c)
{
    ZVec3 *v = dt_alloc(sizeof *v);
    dt_seed(c);
    rand_vec(v, 20.0f);
    DT_ARG_PTR(c, 0, rand_rider(rand_surface()));
    DT_ARG_PTR(c, 1, v);
}

static void setup_tunables(void)
{
    gPhysGravity = (dt_u32() & 1) ? 1.58f : rf(0.5f, 3.0f);
    gPhysAirDrag = rf(0, 0.01f);
    gPhysGrindDrag = rf(0, 0.02f);
    gPhysAlignRate = (dt_u32() & 1) ? 0.15f : rf(0, 1);
    gPhysRollSteer = rf(0, 1);
    gPhysRollYawDamp = rf(0, 1);
    gPhysGrindYawDamp = rf(0, 1);
    gPhysGrindBalance = rf(0, 0.5f);
    gPhysRollSpinSpeed = (dt_u32() & 1) ? 12.0f : rf(1, 30);
}

static void setup_rider_tuned(DtCtx *c)
{
    setup_rider(c);
    setup_tunables();
}

static void setup_rider_edge_tuned(DtCtx *c)
{
    setup_rider_edge(c);
    setup_tunables();
}

/* Grind/stall orientation: surfaces 9-12, slow riders, tails and lips. */
static void setup_orient_grind(DtCtx *c)
{
    PhysRider *r;
    dt_seed(c);
    install_stubs();
    setup_tunables();
    r = rand_rider(dt_range(8, 13));
    if (dt_u32() & 1) {
        r->speed = rf(0, 45);
    }
    gPhysRailAngleLimit = (int16_t)((dt_u32() & 1) ? 0x2000 : dt_range(0, 0x3000));
    DT_ARG_PTR(c, 0, r);
}

static void setup_orient_roll(DtCtx *c)
{
    PhysRider *r;
    setup_rider_tuned(c);
    r = (PhysRider *)(uintptr_t)c->args[0];
    if (dt_u32() & 1) {
        /* flat ground, sideways tilt: makes the steering branch reachable */
        r->up.x = rf(-0.2f, 0.2f);
        r->up.y = rf(-1.0f, -0.6f);
        r->side.y = rf(-0.8f, 0.8f);
        r->speed = rf(0, 60);
    }
    if (dt_u32() & 1) {
        r->fieldB9C = 2;
    }
}

static void setup_orient_air(DtCtx *c)
{
    PhysRider *r;
    setup_rider_tuned(c);
    r = (PhysRider *)(uintptr_t)c->args[0];
    r->timer = (int16_t)dt_range(0, 2);
}

static void setup_edges_equal(DtCtx *c)
{
    ZVec3 p = {0, 0, 0};
    PhysEdge *a, *b;
    dt_seed(c);
    a = rand_edge(&p, 3);
    b = rand_edge(&p, 3);
    if (c->seed % 3 == 0) {
        for (int i = 0; i < 3; i++) {
            b->a[i] = a->a[i];
            b->b[i] = a->b[i];
        }
        if (c->seed % 2) {
            b->b[dt_range(0, 2)] ^= 1; /* one coordinate off */
        }
    }
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
}

static void setup_point_edge(DtCtx *c)
{
    ZVec3 *p = dt_alloc(sizeof *p);
    dt_seed(c);
    rand_vec(p, 2000.0f);
    DT_ARG_PTR(c, 0, p);
    DT_ARG_PTR(c, 1, rand_edge(p, 500));
    DT_ARG_PTR(c, 2, dt_alloc(sizeof(ZVec3)));
}

static void setup_plane_distance(DtCtx *c)
{
    ZVec3 *p = dt_alloc(sizeof *p);
    float *plane = dt_alloc(16);
    dt_seed(c);
    rand_vec(p, 3000.0f);
    rand_unit((ZVec3 *)(void *)plane);
    plane[3] = rf(-3000, 3000);
    DT_ARG_PTR(c, 0, p);
    DT_ARG_PTR(c, 1, plane);
}

static void setup_segment(DtCtx *c)
{
    ZVec3 p = {0, 0, 0};
    int16_t *a, *b, *q;
    dt_seed(c);
    a = rand_point16(&p, 30000);
    b = (c->seed % 9 == 0) ? a : rand_point16(&p, 30000);
    q = rand_point16(&p, 30000);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
    DT_ARG_PTR(c, 2, q);
    DT_ARG_PTR(c, 3, dt_alloc(sizeof(ZVec3)));
}

static void setup_snapshot(DtCtx *c)
{
    PhysContactSnap *s = dt_alloc(sizeof *s);
    ZVec3 p = {0, 0, 0};
    dt_seed(c);
    dt_fill(s, sizeof *s);
    s->surface = (uint8_t)dt_range(0, 16);
    s->contact = (c->seed % 6 == 0) ? NULL : rand_contact(s->surface, &p);
    DT_ARG_PTR(c, 0, s);
}

static void setup_contact_copy(DtCtx *c)
{
    PhysRider *r;
    dt_seed(c);
    r = rand_rider(dt_range(0, 16));
    if (c->seed % 6 == 0) {
        r->contact = NULL;
    }
    DT_ARG_PTR(c, 0, r);
}

static void setup_relative_angles(DtCtx *c)
{
    ZMatrix *a = dt_alloc(sizeof *a), *b = dt_alloc(sizeof *b);
    dt_seed(c);
    rand_matrix(a);
    rand_matrix(b);
    if (c->seed % 4 == 0) {
        *b = *a; /* no relative rotation */
    } else if (c->seed % 4 == 1) {
        /* gimbal case: b's x axis is a's z axis */
        b->right = a->at;
        if (dt_u32() & 1) {
            zvec3NegateLocal(&b->right);
        }
        b->up = a->up;
        b->at = a->right;
    }
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
    DT_ARG_INT(c, 2, dt_range(1, 3));
    DT_ARG_PTR(c, 3, dt_alloc(sizeof(ZAngle3)));
}

static void setup_quarter_pipe(DtCtx *c)
{
    PhysRider *r;
    dt_seed(c);
    setup_tunables();
    r = rand_rider(1);
    gPhysPipeAngleLimit = (int16_t)((dt_u32() & 1) ? 0x238e : dt_range(0, 0x8000));
    DT_ARG_PTR(c, 0, r);
    DT_ARG_PTR(c, 1, (c->seed % 7 == 0) ? NULL : rand_edge(&r->pos, 200));
    DT_ARG_INT(c, 2, dt_range(0, 1));
}

static void setup_snap_edge(DtCtx *c)
{
    PhysRider *r;
    dt_seed(c);
    r = rand_rider(9);
    if (dt_u32() & 1) {
        r->state = kPhysStateGrind;
    }
    DT_ARG_PTR(c, 0, r);
    DT_ARG_PTR(c, 1, rand_edge(&r->pos, 300));
}

static void setup_grind_in(DtCtx *c)
{
    PhysRider *r;
    setup_rider(c);
    r = (PhysRider *)(uintptr_t)c->args[0];
    r->timer = (int16_t)dt_range(0, 3);
    if (dt_u32() & 1) {
        r->state = (uint8_t)((dt_u32() & 1) ? kPhysStateGrindIn : kPhysStateStallIn);
    }
}

DT_CASES(physics_leaf_cases) = {
    {"physNop", 0x0041fa40, physNop, 0, DT_RET_VOID, setup_none, 1, 0},
    {"physTick", 0x0041fa00, physTick, 0, DT_RET_VOID, setup_none, 1, 0},
    {"physGetState", 0x00420c90, physGetState, 1, DT_RET_INT8, setup_rider, 50, 0},
    {"physSetState", 0x00420cb0, physSetState, 2, DT_RET_INT8, setup_set_state, 300, 0},
    {"physAddVelocity", 0x00420d30, physAddVelocity, 2, DT_RET_VOID, setup_add_velocity, 300, 0},
    {"physExitGrind", 0x00420f00, physExitGrind, 3, DT_RET_INT, setup_transition, 50, 0},
    {"physExitScripted", 0x004210a0, physExitScripted, 3, DT_RET_INT, setup_transition, 20, 0},
    {"physVelocityRoll", 0x00425740, physVelocityRoll, 1, DT_RET_VOID, setup_rider, 5, 0},
    {"physVelocityAir", 0x00425760, physVelocityAir, 1, DT_RET_VOID, setup_rider_tuned, 300, 0},
    {"physVelocityGrind", 0x004257e0, physVelocityGrind, 1, DT_RET_VOID, setup_rider_edge_tuned, 300, 0},
    {"physVelocityGrindIn", 0x00425910, physVelocityGrindIn, 1, DT_RET_VOID, setup_rider_tuned, 200, 0},
    {"physOrientToContact", 0x00425610, physOrientToContact, 1, DT_RET_VOID, setup_rider_tuned, 300, 0},
    {"physOrientRoll", 0x00424800, physOrientRoll, 1, DT_RET_VOID, setup_orient_roll, 500, 0},
    {"physOrientAir", 0x00424a10, physOrientAir, 1, DT_RET_VOID, setup_orient_air, 300, 0},
    {"physOrientGrind", 0x00424ea0, physOrientGrind, 1, DT_RET_VOID, setup_orient_grind, 800, 0},
    {"physEdgeEqual", 0x0042b570, physEdgeEqual, 2, DT_RET_INT, setup_edges_equal, 200, 0},
    {"physClosestPointOnEdge", 0x0042cca0, physClosestPointOnEdge, 3, DT_RET_VOID, setup_point_edge, 300, 0},
    {"physPlaneDistance", 0x0042d670, physPlaneDistance, 2, DT_RET_DOUBLE, setup_plane_distance, 300, 0},
    {"physClosestPointOnSegment", 0x0042d6e0, physClosestPointOnSegment, 4, DT_RET_VOID, setup_segment, 400,
     0},
    {"physContactSnapshotFix", 0x0042dce0, physContactSnapshotFix, 1, DT_RET_VOID, setup_snapshot, 100, 0},
    {"physContactCopy", 0x0042dda0, physContactCopy, 1, DT_RET_VOID, setup_contact_copy, 100, 0},
    {"physRelativeAngles", 0x0042d250, physRelativeAngles, 4, DT_RET_VOID, setup_relative_angles, 300, 0},
    {"physAlignToQuarterPipe", 0x0042d3c0, physAlignToQuarterPipe, 3, DT_RET_VOID, setup_quarter_pipe, 400,
     0},
    {"physSnapToEdge", 0x0042ca90, physSnapToEdge, 2, DT_RET_VOID, setup_snap_edge, 300, 0},
    {"physMoveGrindIn", 0x0042afd0, physMoveGrindIn, 1, DT_RET_VOID, setup_grind_in, 300, 0},
};

/* ---- engine environment: level geometry, renderer callbacks and debug output ------------------- */

/* Debug print and renderer callbacks are function pointers in the exe; the tests install their own. */
#define DT_DEBUG_PRINT  ORIG_VAR(0x00560074, void *, DT_DEBUG_PRINT)
#define DT_RW_BOUNDS    ORIG_VAR(0x00561488, void *, DT_RW_BOUNDS)
#define DT_RW_RAY       ORIG_VAR(0x00562068, void *, DT_RW_RAY)
#define DT_RW_SPHERE    ORIG_VAR(0x0056206c, void *, DT_RW_SPHERE)
#define DT_RW_OBJECT_ID ORIG_VAR(0x00560ee0, void *, DT_RW_OBJECT_ID)
#define DT_RW_OBJ_SYNC  ORIG_VAR(0x00560e98, void *, DT_RW_OBJ_SYNC)
#define DT_RW_OBJ_POINT ORIG_VAR(0x00560eb0, void *, DT_RW_OBJ_POINT)
/* Level polygons {3 vertices, face tag at +0x40, edge tags at +0x44, edge/face caches at +0x5c/+0x68}. */
#define DT_BSP_COUNT ORIG_VAR(0x006728bc, uint32_t, DT_BSP_COUNT)
#define DT_BSP_POLYS ORIG_VAR(0x006728c0, uint8_t *, DT_BSP_POLYS)
/* World sector tree root node (+0x24 node type, +0x26 polygon count) and its debug flag. */
#define DT_WORLD_ROOT  ORIG_VAR(0x006728c4, uint8_t, DT_WORLD_ROOT)
#define DT_WORLD_DEBUG ORIG_VAR(0x00672900, uint32_t, DT_WORLD_DEBUG)
#define DT_EDGE_POOL   ORIG_VAR(0x005f34b0, uint32_t, DT_EDGE_POOL)
#define DT_FACE_POOL   ORIG_VAR(0x005f34ac, uint32_t, DT_FACE_POOL)
#define DT_PROP_EDGES  ORIG_VAR(0x00678494, void *, DT_PROP_EDGES)
#define DT_PROP_FACES  ORIG_VAR(0x006722c4, void *, DT_PROP_FACES)
/* Display list: primitive write pointer and the ordering-table slot packets are linked into. */
#define DT_PRIM_PTR ORIG_VAR(0x0068ebe0, uint8_t *, DT_PRIM_PTR)
#define DT_PRIM_OT  ORIG_VAR(0x0055c698, uint32_t *, DT_PRIM_OT)
/* zactorGetPartPosition: current view, part tables (person / other) and their init flags. */
#define DT_VIEW            ORIG_VAR(0x0056b408, uint32_t, DT_VIEW)
#define DT_PARTS_READY     ORIG_VAR(0x005da240, uint32_t, DT_PARTS_READY)
#define DT_PARTS           ORIG_VAR(0x005da248, uint32_t, DT_PARTS)
#define DT_OBJ_PARTS_READY ORIG_VAR(0x005da1e8, uint32_t, DT_OBJ_PARTS_READY)
#define DT_OBJ_PARTS       ORIG_VAR(0x005da1f0, uint32_t, DT_OBJ_PARTS)
#define DT_ARM_HASHES      ORIG_VAR(0x005da2c8, uint32_t, DT_ARM_HASHES)
/* Effects: the current effect owner (+0x30: its frame) and the named effect handlers list. */
/* Props: a nonzero "props frozen" byte keeps propTouch out of the trick-scoring code. */
#define DT_PROPS_FROZEN ORIG_VAR(0x0056b701, uint8_t, DT_PROPS_FROZEN)
#define DT_FX_OWNER     ORIG_VAR(0x0068ff80, void *, DT_FX_OWNER)
#define DT_FX_NAMES     ORIG_VAR(0x00670524, void *, DT_FX_NAMES)
/* Debug draw callback slots of the level renderer. */
typedef void *DtDrawSlots[8];
#define DT_DRAW_SLOTS ORIG_VAR(0x005fb6c8, DtDrawSlots, DT_DRAW_SLOTS)

static uint32_t dtPrintCount;
static uint32_t dtPrintHash;

static void BMX_CDECL dtPrint(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dtPrintCount++;
    for (const char *p = fmt; *p; p++) {
        dtPrintHash = dtPrintHash * 31u + (uint8_t)*p;
        if (*p == '%') {
            dtPrintHash = dtPrintHash * 31u + (uint32_t)va_arg(ap, int);
        }
    }
    va_end(ap);
}

/* World box used by the ground probe. */
static float dtBoundsMin, dtBoundsMax;

static void BMX_CDECL dtRwBounds(int which, float *out, int z)
{
    out[0] = -5000.0f;
    out[1] = dtBoundsMin;
    out[2] = -5000.0f;
    out[4] = 5000.0f;
    out[5] = dtBoundsMax;
    out[6] = 5000.0f;
}

/* Ray stub: the world is the plane y = dtGroundY with normal dtGroundN (a triangle around the hit). */
static float dtGroundY;
static ZVec3 dtGroundN;
static int dtGroundOn;
static int32_t dtGroundObject; /* nonzero: the plane belongs to a prop (an edge hit) */

static int BMX_CDECL dtRwRay(const float *from, const float *to, int a, int b, float *rec)
{
    float t, hx, hz;

    if (!dtGroundOn || (from[1] - dtGroundY) * (to[1] - dtGroundY) > 0.0f || from[1] == to[1]) {
        return 0;
    }
    t = (dtGroundY - from[1]) / (to[1] - from[1]);
    hx = from[0] + (to[0] - from[0]) * t;
    hz = from[2] + (to[2] - from[2]) * t;
    for (int i = 0; i < 23; i++) {
        rec[i] = 0.0f;
    }
    rec[0] = dtGroundN.x;
    rec[1] = dtGroundN.y;
    rec[2] = dtGroundN.z;
    rec[4] = hx - 40.0f;
    rec[5] = dtGroundY;
    rec[6] = hz - 30.0f;
    rec[8] = hx + 50.0f;
    rec[9] = dtGroundY;
    rec[10] = hz - 20.0f;
    rec[12] = hx;
    rec[13] = dtGroundY;
    rec[14] = hz + 60.0f;
    rec[16] = t;
    __builtin_memcpy(&rec[17], &dtGroundObject, 4);
    rec[19] = hx;
    rec[20] = dtGroundY;
    rec[21] = hz;
    return 1;
}

static int32_t BMX_CDECL dtRwObjectId(int32_t object)
{
    return object ^ 0x1234;
}

static int BMX_CDECL dtRwSphere(const void *a, float r, int b, int c, void *out)
{
    return 0;
}

static void BMX_CDECL dtRwObjSync(int handle)
{
    dtPrintHash = dtPrintHash * 7u + (uint32_t)handle;
}

static void BMX_CDECL dtRwObjPoint(int handle, int part, const float *in, float *out, int flag)
{
    out[0] = in[0] * 0.5f + (float)(part * 3 + handle % 17);
    out[1] = in[1] - (float)part;
    out[2] = in[2] + in[0] * 0.25f + (float)(handle % 5);
}

static void dt_env(void)
{
    static const uint32_t emptyList[4] = {0, 0, 0, 0};

    dtPrintCount = 0;
    dtPrintHash = 0;
    DT_DEBUG_PRINT = (void *)dtPrint;
    DT_RW_BOUNDS = (void *)dtRwBounds;
    DT_RW_RAY = (void *)dtRwRay;
    DT_RW_SPHERE = (void *)dtRwSphere;
    DT_RW_OBJ_SYNC = (void *)dtRwObjSync;
    DT_RW_OBJ_POINT = (void *)dtRwObjPoint;
    dtBoundsMin = rf(-3000, -1000);
    dtBoundsMax = rf(1000, 3000);
    dtGroundOn = 1;
    dtGroundY = rf(-500, 500);
    rand_unit(&dtGroundN);
    if (dtGroundN.y > 0.0f) {
        dtGroundN.y = -dtGroundN.y;
    }
    if (dt_u32() & 1) {
        /* mostly flat */
        float len;
        dtGroundN.x *= 0.3f;
        dtGroundN.y = -1.0f;
        dtGroundN.z *= 0.3f;
        len = __builtin_sqrtf(dtGroundN.x * dtGroundN.x + 1.0f + dtGroundN.z * dtGroundN.z);
        dtGroundN.x /= len;
        dtGroundN.y /= len;
        dtGroundN.z /= len;
    } else if (dt_u32() & 1) {
        dtGroundN.x = 0.0f;
        dtGroundN.y = -1.0f;
        dtGroundN.z = 0.0f;
    }
    /*
     * World tree: a single leaf. Mostly it holds one huge sector polygon in the ground plane, whose tag
     * record gives ray hits their surface kind.
     */
    (&DT_WORLD_ROOT)[0x24] = 1;
    *(uint16_t *)(void *)(&DT_WORLD_ROOT + 0x26) = 0;
    if (dt_u32() & 3) {
        float *poly = dt_alloc(0x50);
        uint8_t *tag = dt_alloc(0x10);
        void **polys = (void **)dt_alloc(sizeof(void *));

        poly[0] = -20000.0f;
        poly[1] = dtGroundY;
        poly[2] = -20000.0f;
        poly[4] = 20000.0f;
        poly[5] = dtGroundY;
        poly[6] = -20000.0f;
        poly[8] = 0.0f;
        poly[9] = dtGroundY;
        poly[10] = 30000.0f;
        poly[13] = -1.0f;
        *(uint16_t *)(void *)tag = (uint16_t)(dt_range(0, 15) << 12 | dt_range(0, 0xfff));
        tag[3] = (uint8_t)dt_u32();
        *(uint32_t *)(void *)(tag + 8) = dt_u32() & 1;
        *(void **)(void *)(poly + 0x10) = tag;
        polys[0] = poly;
        *(uint16_t *)(void *)(&DT_WORLD_ROOT + 0x26) = 1;
        *(void ***)(void *)(&DT_WORLD_ROOT + 0x30) = polys;
    }
    DT_WORLD_DEBUG = 0;
    DT_BSP_COUNT = 0;
    DT_EDGE_POOL = 1; /* never used: every polygon comes with its caches */
    DT_FACE_POOL = 1;
    {
        uint32_t *list = dt_alloc(sizeof emptyList);
        DT_PROP_EDGES = list;
    }
    DT_RW_OBJECT_ID = (void *)dtRwObjectId;
    DT_PROP_FACES = NULL;
    dtGroundObject = 0;
    if ((dt_u32() & 7) == 0) {
        /* a prop owns the ground: prop list node {next, ..., [7] object id, [9] face, [11] edge} */
        uint32_t *node = dt_alloc(0x40);
        uint16_t *edge = dt_alloc(0x40);
        dtGroundObject = dt_range(1, 1000);
        node[7] = (uint32_t)(dtGroundObject ^ 0x1234);
        node[11] = (uint32_t)(uintptr_t)edge;
        edge[0] = (uint16_t)dt_u32();
        DT_PROP_FACES = node;
    }
    DT_PROPS_FROZEN = 1;
    {
        uint8_t *owner = dt_alloc(0x40);
        float *frame = dt_alloc(0x20);
        rand_vec((ZVec3 *)(void *)(frame + 2), 1000.0f);
        *(void **)(owner + 0x30) = frame;
        DT_FX_OWNER = owner;
        DT_FX_NAMES = dt_alloc(16);
    }
}

/* Owner record of a grind edge (or a kind-12 contact) whose +0x1ec slot holds a prop, or nothing. */
static uint8_t *dt_prop_owner(void)
{
    uint8_t *owner = dt_alloc(0x200);
    if (dt_u32() & 1) {
        uint8_t *prop = dt_alloc(0x80);
        dt_fill(prop, 0x80);
        prop[0] = 0;
        prop[2] = (uint8_t)dt_range(0, 15);
        *(void **)(owner + 0x1ec) = prop;
    } else {
        *(void **)(owner + 0x1ec) = NULL;
    }
    return owner;
}

/* Builds n level polygons near p. Edge tags use ids idBase..idBase+2, face tags 0xfa0..0xfa5. */
static void dt_world(const ZVec3 *p, int n, int idBase)
{
    uint8_t *polys = dt_alloc((uint32_t)n * 0x6c + 4);

    for (int i = 0; i < n; i++) {
        uint8_t *poly = polys + i * 0x6c;
        float *v = (float *)(void *)poly;

        dt_fill(poly, 0x6c);
        for (int k = 0; k < 3; k++) {
            v[k * 4 + 0] = p->x + rf(-300, 300);
            v[k * 4 + 1] = p->y + rf(-100, 100);
            v[k * 4 + 2] = p->z + rf(-300, 300);
        }
        if (dt_u32() & 3) {
            uint16_t *tag = dt_alloc(4);
            tag[0] = (uint16_t)(dt_range(0xfa0, 0xfa5) | (dt_range(0, 2) << 12));
            *(void **)(poly + 0x40) = tag;
        } else {
            *(void **)(poly + 0x40) = NULL;
        }
        for (int k = 0; k < 3; k++) {
            if (dt_u32() & 3) {
                uint8_t *tag = dt_alloc(4);
                *(uint16_t *)(void *)tag = (uint16_t)(idBase + dt_range(0, 2));
                tag[2] = (uint8_t)dt_range(0, 3);
                *(void **)(poly + 0x44 + 4 * k) = tag;
            } else {
                *(void **)(poly + 0x44 + 4 * k) = NULL;
            }
            *(void **)(poly + 0x5c + 4 * k) = dt_alloc(0x40);
        }
        *(void **)(poly + 0x68) = dt_alloc(0x90);
    }
    DT_BSP_COUNT = (uint32_t)n;
    DT_BSP_POLYS = polys;
}

/* zobj stand-in: position +4, rotation +0x14, method table +0x28 (all empty), handle +0x2c. */
static void *dt_zobj(void)
{
    uint8_t *o = dt_alloc(0x40);
    *(void **)(o + 0x28) = dt_alloc(0x20);
    *(int32_t *)(void *)(o + 0x2c) = dt_range(0, 1000);
    return o;
}

/* Gives a person actor what zactorGetPartPosition needs: no bones to fix up and two scene objects. */
static void dt_actor_objects(ZActor *a)
{
    a->skeleton = (dt_u32() & 1) ? 0 : 1;
    *(uint32_t *)(void *)((uint8_t *)a + 0x274) = 0;
    a->object = (int32_t)(uintptr_t)dt_zobj();
    *(void **)(void *)((uint8_t *)a + 0x1d8) = dt_zobj();
}

static void dt_parts(void)
{
    DT_VIEW = 0;
    DT_ARM_HASHES = 1;
    DT_PARTS_READY = 1;
    DT_OBJ_PARTS_READY = 1;
    for (int i = 0; i < 10; i++) {
        (&DT_PARTS)[i] = (dt_u32() & 7) ? (uint32_t)dt_range(1, 50) : 0;
        (&DT_OBJ_PARTS)[i] = (dt_u32() & 7) ? (uint32_t)dt_range(1, 50) : 0;
    }
}

/* ---- functions that need the environment ------------------------------------------------------- */

static void setup_exit_air(DtCtx *c)
{
    dt_seed(c);
    install_stubs();
    DT_ARG_PTR(c, 0, rand_rider(rand_surface()));
    DT_ARG_INT(c, 1, dt_range(0, 6));
    DT_ARG_INT(c, 2, (dt_u32() & 1) ? dt_range(0, 2) : dt_range(0, 6));
}

static void setup_enter_roll(DtCtx *c)
{
    dt_seed(c);
    install_stubs();
    DT_ARG_PTR(c, 0, rand_rider(rand_surface()));
    DT_ARG_INT(c, 1, (dt_u32() & 1) ? kPhysStateGrind : dt_range(0, 6));
    DT_ARG_INT(c, 2, 0);
}

/* Rider props: edge records point at a prop whose +0x1ec slot is empty (the prop module is not set up). */
static void setup_enter_grind(DtCtx *c)
{
    PhysRider *r;
    PhysEdge *e;
    uint8_t *rb;

    dt_seed(c);
    install_stubs();
    setup_tunables();
    dt_env();
    r = rand_rider((dt_u32() & 3) ? dt_range(1, 3) : dt_range(0, 9));
    rb = (uint8_t *)r;
    rb[5] = (uint8_t)dt_range(0, 3); /* trick lookup: rider[5] selects rider[5 + n] */
    for (int i = 6; i < 10; i++) {
        rb[i] = (uint8_t)dt_range(0, 12);
    }
    e = (r->surface >= 1 && r->surface <= 3) ? r->contact : rand_edge(&r->pos, 300);
    if (r->surface < 1 || r->surface > 3) {
        r->contact = e;
    }
    /* close to the edge most of the time */
    if (dt_u32() & 3) {
        r->pos.x = (float)e->a[0] + rf(-20, 20);
        r->pos.y = (float)e->a[1] + rf(-20, 20);
        r->pos.z = (float)e->a[2] + rf(-20, 20);
        r->center.x = r->pos.x + rf(-60, 60);
        r->center.y = r->pos.y + rf(-120, 40);
        r->center.z = r->pos.z + rf(-60, 60);
    }
    if (dt_u32() & 1) {
        e->kind = (int8_t)dt_range(1, 2);
    }
    if (dt_u32() & 1) {
        e->prop = dt_prop_owner();
    }
    gPhysGrindMinSpeed = (dt_u32() & 1) ? 6.0f : rf(0, 40);
    DT_ARG_PTR(c, 0, r);
    DT_ARG_INT(c, 1, (dt_u32() & 3) ? 0 : dt_range(1, 6));
    DT_ARG_INT(c, 2, (dt_u32() & 1) ? kPhysStateGrind : kPhysStateStall);
}

static void setup_enter_stall(DtCtx *c)
{
    PhysRider *r;
    dt_seed(c);
    install_stubs();
    dt_env();
    r = rand_rider(dt_range(8, 13));
    if (r->surface == 9 && (dt_u32() & 1)) {
        ((PhysEdge *)r->contact)->prop = dt_prop_owner();
    }
    if (r->surface == 12) {
        r->contact = dt_prop_owner();
    }
    dtGroundY = r->contactPos.y + r->contactAxis1.y * 100.0f + rf(-50, 5000);
    DT_ARG_PTR(c, 0, r);
    DT_ARG_INT(c, 1, dt_range(0, 6));
    DT_ARG_INT(c, 2, dt_range(0, 6));
}

/* A zover motion object the start/step helpers accept: no ragdoll nodes. */
static uint8_t *dt_motion_obj_empty(void)
{
    uint8_t *o = rand_motion_obj();
    *(int32_t *)(void *)o = 0;           /* ragdoll nodes */
    *(int32_t *)(void *)(o + 0xa90) = 0; /* constraints */
    return o;
}

static void setup_enter_scripted(DtCtx *c)
{
    PhysRider *r;
    dt_seed(c);
    r = rand_rider(rand_surface());
    r->actor2 = dt_motion_obj_empty();
    r->bikeObj2 = dt_motion_obj_empty();
    DT_ARG_PTR(c, 0, r);
    DT_ARG_INT(c, 1, dt_range(0, 6));
    DT_ARG_INT(c, 2, kPhysStateScripted);
}

/* Rider grinding edge `contact`, inside or past its end points; the level has edges around. */
static void setup_move_grind(DtCtx *c)
{
    PhysRider *r;
    PhysEdge *e;

    dt_seed(c);
    install_stubs();
    setup_tunables();
    dt_env();
    r = rand_rider(9);
    e = r->contact;
    if (dt_u32() & 1) {
        r->pos.x = ((float)e->a[0] + e->b[0]) * 0.5f + rf(-5, 5);
        r->pos.y = ((float)e->a[1] + e->b[1]) * 0.5f + rf(-5, 5);
        r->pos.z = ((float)e->a[2] + e->b[2]) * 0.5f + rf(-5, 5);
    }
    if (dt_u32() & 1) {
        r->state = kPhysStateGrind;
    }
    dt_world(&r->pos, dt_range(0, 6), dt_range(0, 5));
    gPhysEdgeSearchRadius = (dt_u32() & 1) ? 30.0f : rf(10, 400);
    DT_ARG_PTR(c, 0, r);
}

static void setup_find_nearest_edge(DtCtx *c)
{
    PhysRider *r;
    ZVec3 *p = dt_alloc(sizeof *p);

    dt_seed(c);
    dt_env();
    r = rand_rider((dt_u32() & 1) ? 9 : rand_surface());
    if (dt_u32() & 1) {
        r->state = kPhysStateGrind;
    }
    *p = r->pos;
    dt_world(p, dt_range(0, 8), 0);
    if (DT_BSP_COUNT != 0 && r->surface == 9 && (dt_u32() & 1)) {
        /* the edge being ground is one of the candidates */
        const float *v = (const float *)(void *)DT_BSP_POLYS;
        PhysEdge *e = r->contact;
        for (int i = 0; i < 3; i++) {
            e->a[i] = (int16_t)(int32_t)v[i];
            e->b[i] = (int16_t)(int32_t)v[4 + i];
        }
    }
    DT_ARG_PTR(c, 0, r);
    DT_ARG_PTR(c, 1, p);
    DT_ARG_FLOAT(c, 2, (dt_u32() & 1) ? 30.0f : rf(1, 600));
    DT_ARG_PTR(c, 3, dt_alloc(sizeof(ZVec3)));
}

static void setup_tagged_edge(DtCtx *c)
{
    ZVec3 *p = dt_alloc(sizeof *p);

    dt_seed(c);
    dt_env();
    rand_vec(p, 1500.0f);
    dt_world(p, dt_range(0, 8), 7);
    DT_ARG_INT(c, 0, (int32_t)dt_u32());
    DT_ARG_INT(c, 1, dt_range(6, 10));
    DT_ARG_PTR(c, 2, p);
}

/* Ray hit with a face (kinds 0-15) or a kind-12 contact; quarter-pipe faces resolve through the level. */
static void setup_take_contact(DtCtx *c)
{
    PhysRider *r;
    PhysRayHit *hit = dt_alloc(sizeof *hit);
    PhysFace *face;

    dt_seed(c);
    dt_env();
    setup_tunables();
    gPhysPipeAngleLimit = (int16_t)dt_range(0, 0x8000);
    r = rand_rider(rand_surface());
    dt_fill(hit, sizeof *hit);
    hit->flags = dt_u32() & 3;
    face = rand_face(&r->pos);
    face->tag = (uint16_t)((dt_range(0, 15) << 12) | dt_range(5, 9));
    hit->face = (dt_u32() & 7) ? face : NULL;
    hit->edge = (dt_u32() & 3) ? rand_contact(12, &r->pos) : NULL;
    dt_world(&r->pos, dt_range(0, 6), 5);
    DT_ARG_PTR(c, 0, r);
    DT_ARG_PTR(c, 1, hit);
}

static void setup_probe_ground(DtCtx *c)
{
    ZVec3 *p = dt_alloc(sizeof *p);
    dt_seed(c);
    dt_env();
    rand_vec(p, 3000.0f);
    if (c->seed % 5 == 0) {
        dtGroundOn = 0;
    }
    if (c->seed % 7 == 1) {
        dtGroundN.y = -dtGroundN.y; /* hit from below */
    }
    DT_ARG_PTR(c, 0, p);
    DT_ARG_PTR(c, 1, dt_alloc(sizeof(PhysGroundHit)));
}

static void setup_respawn_face(DtCtx *c)
{
    ZVec3 *p = dt_alloc(sizeof *p);
    dt_seed(c);
    dt_env();
    rand_vec(p, 1500.0f);
    switch (c->seed % 6) {
    case 0:
        dtGroundOn = 0; /* no ground: tagged faces only */
        break;
    case 1:
        dtGroundN.x = 0.8f; /* too steep */
        dtGroundN.y = -0.6f;
        dtGroundN.z = 0.0f;
        break;
    case 2:
        dtGroundY = p->y + rf(-600, 600); /* height limit (flag 2) */
        break;
    default:
        break;
    }
    dt_world(p, dt_range(0, 6), 1);
    DT_ARG_PTR(c, 0, p);
    DT_ARG_INT(c, 1, dt_range(0, 3));
}

static void setup_respawn(DtCtx *c)
{
    PhysRider *r;
    dt_seed(c);
    dt_env();
    r = rand_rider(rand_surface());
    if (c->seed % 4 == 0) {
        r->fwd = r->up; /* fwd parallel to the face normal */
    }
    if (c->seed % 9 == 0) {
        dtGroundOn = 0; /* tagged faces or the assert */
        dt_world(&r->pos, dt_range(0, 3), 1);
    }
    dtGroundY = r->pos.y + rf(-300, 300);
    dt_actor_objects(r->actor);
    DT_ARG_PTR(c, 0, r);
}

static void setup_limbs(DtCtx *c)
{
    PhysRider *r;
    dt_seed(c);
    dt_env();
    dt_parts();
    r = rand_rider(rand_surface());
    dt_actor_objects(r->actor);
    dt_actor_objects(r->bikeObj);
    DT_ARG_PTR(c, 0, r);
}

static void setup_pose(DtCtx *c)
{
    PhysRider *r;
    setup_limbs(c);
    setup_tunables();
    r = (PhysRider *)(uintptr_t)c->args[0];
    if (dt_u32() & 1) {
        r->speed = rf(0, 14);
        r->rot.y = (ZAngle)dt_range(-0x6000, 0x6000);
    }
}

static void setup_sync(DtCtx *c)
{
    PhysRider *r;
    uint8_t *rb;
    setup_pose(c);
    install_stubs();
    r = (PhysRider *)(uintptr_t)c->args[0];
    rb = (uint8_t *)r;
    for (int i = 0x3d; i < 0x41; i++) {
        rb[i] = (uint8_t)dt_range(0, 4);
    }
    if (dt_u32() & 1) {
        r->state = kPhysStateGrind;
        r->surface = 9;
        r->contact = rand_contact(9, &r->pos);
    }
}

static void setup_move_scripted(DtCtx *c)
{
    static const uint8_t motion[16] = {0};
    PhysRider *r;

    setup_rider(c);
    r = (PhysRider *)(uintptr_t)c->args[0];
    r->actor2 = dt_motion_obj_empty();
    r->bikeObj2 = dt_motion_obj_empty();
    r->motionIndex = 0;
    gPhysMotions[0] = motion;
    /*
     * zactorSampleScaled returns a pointer into its own stack frame when it samples, which differs between
     * the two sides; without the restart bit and with no key stream it returns NULL instead.
     */
    r->motionFlags &= (uint16_t)~2u;
    if (dt_u32() & 1) {
        r->surface = 13;
    }
}

static void setup_hud_show(DtCtx *c)
{
    dt_seed(c);
    /* only valid handles: kinds 2 and 3 make the overlay lookup use an uninitialised pointer */
    gPhysHudHandle =
        (uint16_t)((c->seed % 4 == 0) ? 0xffff
                                      : dt_range(0, 1) << 14 | dt_range(0, 1) << 12 | dt_range(0, 15));
    DT_ARG_INT(c, 0, dt_range(0, 1));
}

/* String set 1 (ids 0x10b0..0x10cf) holding a random lower-case text for 0x10ba, and a small font. */
static void setup_hud_set(DtCtx *c)
{
    enum { kFirst = 0x10b0, kCount = 0x20 };
    uint8_t *blob = dt_alloc(8 + 2 * kCount + 64);
    ZStringSet *set = (ZStringSet *)(void *)blob;
    char *text = (char *)blob + 8 + 2 * kCount;
    const int len = dt_range(0, 40);
    uint8_t *font = &gPhysHudFont;
    int16_t *glyphs = dt_alloc(8 * 26);
    uint16_t *style = dt_alloc(4);

    setup_hud_show(c);
    __builtin_memcpy(set->magic, "STRS", 4);
    set->firstId = kFirst;
    set->count = kCount;
    for (int i = 0; i < kCount; i++) {
        set->offsets[i] = (uint16_t)(8 + 2 * kCount + (i & 1));
    }
    for (int i = 0; i < len; i++) {
        text[i] = (char)dt_range('a' - 2, 'z' + 2);
    }
    text[len] = 0;
    set->offsets[0x10ba - kFirst] = (uint16_t)(8 + 2 * kCount);
    gZstringSets[1] = set;
    for (int i = 0; i < 26 * 4; i++) {
        glyphs[i] = (int16_t)dt_range(0, 20);
    }
    *style = 0;
    *(void **)(void *)font = style;
    font[5] = (uint8_t)dt_range(4, 16);
    font[6] = 'a';
    font[7] = (uint8_t)((c->seed % 3 == 0) ? 0 : dt_range(1, 26));
    *(void **)(void *)(font + 8) = glyphs;
}

static void setup_register_draw(DtCtx *c)
{
    dt_seed(c);
    for (int i = 0; i < 8; i++) {
        DT_DRAW_SLOTS[i] = (c->seed % 5 == 0 || (dt_u32() & 1)) ? (void *)(uintptr_t)dt_u32() : NULL;
    }
}

static void setup_update_riders(DtCtx *c)
{
    dt_seed(c);
    install_stubs();
    gPhysPlayerCount = (uint8_t)dt_range(0, 3);
    gPhysSyncFlags = (uint8_t)dt_u32();
    gPhysDebugEdges = (PhysEdge **)(uintptr_t)dt_u32();
    for (int i = 0; i < 4; i++) {
        fill_rider(&gPhysRiders[i], rand_surface());
    }
}

static uint8_t *dt_prims(int compared)
{
    uint32_t *ot = dt_alloc(4);
    *ot = 0;
    DT_PRIM_OT = ot;
    /* Line packets copy a stack byte the original never initialises; those go to unchecked stack memory. */
    DT_PRIM_PTR = compared ? dt_alloc(0x8000) : (uint8_t *)(uintptr_t)0x00110000;
    return DT_PRIM_PTR;
}

static void setup_debug_draw(DtCtx *c, int contacts)
{
    dt_seed(c);
    gPhysPlayerCount = (uint8_t)dt_range(0, 2);
    gPhysDebugFrame = dt_u32();
    gPhysDebugTexture = 0;
    gPhysDebugContact = contacts;
    for (int i = 0; i < 3; i++) {
        fill_rider(&gPhysRiders[i], rand_surface());
        gPhysRiders[i].flags = dt_u32() & 7;
    }
    if (contacts && (dt_u32() & 1)) {
        ZVec3 p = {0, 0, 0};
        PhysEdge **list = (PhysEdge **)dt_alloc(5 * sizeof *list);
        for (int i = 0; i < 4; i++) {
            list[i] = rand_edge(&p, 2000);
        }
        list[dt_range(0, 4)] = NULL;
        list[4] = NULL;
        gPhysDebugEdges = list;
    } else {
        gPhysDebugEdges = NULL;
    }
    dt_prims(!contacts);
}

static void setup_debug_halo(DtCtx *c)
{
    setup_debug_draw(c, 0);
    for (int i = 0; i < 3; i++) {
        gPhysRiders[i].flags = (dt_u32() & 1) ? 5 : (dt_u32() & 3);
    }
}

static void setup_debug_contacts(DtCtx *c)
{
    setup_debug_draw(c, 1);
}

DT_CASES(physics_env_cases) = {
    {"physExitAir", 0x00420f30, physExitAir, 3, DT_RET_INT, setup_exit_air, 200, 0},
    {"physEnterRoll", 0x004210c0, physEnterRoll, 3, DT_RET_INT, setup_enter_roll, 200, 0},
    {"physEnterGrind", 0x00422830, physEnterGrind, 3, DT_RET_INT, setup_enter_grind, 600, 0},
    {"physEnterStall", 0x004243b0, physEnterStall, 3, DT_RET_INT, setup_enter_stall, 400, 0},
    {"physEnterScripted", 0x00424740, physEnterScripted, 3, DT_RET_INT, setup_enter_scripted, 100, 0},
    {"physMoveGrind", 0x0042b0a0, physMoveGrind, 1, DT_RET_VOID, setup_move_grind, 500, 0},
    {"physFindNearestEdge", 0x0042b3f0, physFindNearestEdge, 4, DT_RET_INT, setup_find_nearest_edge, 400, 0},
    {"physFindNearestTaggedEdge", 0x0042d950, physFindNearestTaggedEdge, 3, DT_RET_INT, setup_tagged_edge,
     300, 0},
    {"physTakeContact", 0x0042dad0, physTakeContact, 2, DT_RET_INT, setup_take_contact, 500, 0},
    {"physProbeGround", 0x00420d70, physProbeGround, 2, DT_RET_INT, setup_probe_ground, 300, 0},
    {"physFindRespawnFace", 0x004200b0, physFindRespawnFace, 2, DT_RET_INT, setup_respawn_face, 300, 0},
    {"physRespawn", 0x0041fe10, physRespawn, 1, DT_RET_VOID, setup_respawn, 200, 0},
    {"physUpdateLimbTargets", 0x0041fa60, physUpdateLimbTargets, 1, DT_RET_VOID, setup_limbs, 100, 0},
    {"physPoseActorsRoll", 0x0042cce0, physPoseActorsRoll, 1, DT_RET_VOID, setup_pose, 300, 0},
    {"physPoseActorsGrind", 0x0042d050, physPoseActorsGrind, 1, DT_RET_VOID, setup_pose, 300, 0},
    {"physSyncRoll", 0x0042b830, physSyncRoll, 1, DT_RET_VOID, setup_sync, 300, 0},
    {"physSyncGrind", 0x0042c7e0, physSyncGrind, 1, DT_RET_VOID, setup_sync, 300, 0},
    {"physMoveScripted", 0x0042b5e0, physMoveScripted, 1, DT_RET_VOID, setup_move_scripted, 100, 0},
    {"physHudTextShow", 0x0041f930, physHudTextShow, 1, DT_RET_VOID, setup_hud_show, 50, 0},
    {"physHudTextSet", 0x0041f960, physHudTextSet, 1, DT_RET_VOID, setup_hud_set, 100, 0},
    {"physRegisterDebugDraw", 0x0041fa20, physRegisterDebugDraw, 0, DT_RET_VOID, setup_register_draw, 40, 0},
    {"physUpdateRiders", 0x0041fc40, physUpdateRiders, 0, DT_RET_VOID, setup_update_riders, 200, 0},
    {"physDebugDrawHalo", 0x004203a0, physDebugDraw, 0, DT_RET_INT, setup_debug_halo, 100, 0},
    {"physDebugDrawContacts", 0x004203a0, physDebugDraw, 0, DT_RET_INT, setup_debug_contacts, 200, 0},
};
