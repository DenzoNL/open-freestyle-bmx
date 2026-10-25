/*
 * zoverii: the motion rigs (rider and bike ragdoll), the limb target block and the overlay layer. Callees
 * that need a level or a renderer (point collision, edge queries, bone lengths, the glyph renderer) are
 * stubbed per iteration; the stubs log their arguments by value.
 */
#include <stddef.h>

#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "dt.h"
#include "dt_zactor.h"
#include "zge/zmem.h"

/* ---- log and stubs ----------------------------------------------------------------------------- */

#define ZLOG_SIZE 2048
static uint32_t zlog[ZLOG_SIZE];
static uint32_t zlogN;

static void lg(uint32_t v)
{
    if (zlogN < ZLOG_SIZE) {
        zlog[zlogN] = v;
    }
    zlogN++;
}

static void lgf(float v)
{
    uint32_t u;
    __builtin_memcpy(&u, &v, 4);
    lg(u);
}

/* Point collision: sometimes hits, nudging the point and filling the hit record. */
static int stubCollide(ZRigPoint *p, uint32_t *hitId, void *hit)
{
    const uint32_t r = dt_u32();

    lg(0xc0110000u);
    lg((uint32_t)(uintptr_t)p);
    if (r % 3 == 0) {
        uint32_t *rec = hit;

        *hitId = r >> 8;
        rec[0] = dt_u32();
        rec[1] = dt_u32();
        p->pos.x = (float)(p->pos.x + dt_float(-3.0f, 3.0f));
        p->pos.y = (float)(p->pos.y + dt_float(-3.0f, 3.0f));
        p->pos.z = (float)(p->pos.z + dt_float(-3.0f, 3.0f));
        return 1;
    }
    return 0;
}

static void stubResolve(ZRigPoint *p)
{
    lg(0x4e500000u);
    lg((uint32_t)(uintptr_t)p);
    p->pos.x = (float)(p->pos.x + dt_float(-2.0f, 2.0f));
    p->pos.y = (float)(p->pos.y + dt_float(-2.0f, 2.0f));
    p->pos.z = (float)(p->pos.z + dt_float(-2.0f, 2.0f));
}

static void stubPoll(void)
{
    lg(0x9011000u);
}

static void BMX_CDECL hitCallback(ZRig *rig, uint32_t index, uint32_t hitId, void *hit)
{
    const uint32_t *rec = hit;

    lg(0xca110000u);
    lg((uint32_t)(uintptr_t)rig);
    lg(index);
    lg(hitId);
    lg(rec[0]);
    lg(rec[1]);
    rig->points[index].pos.x = (float)(rig->points[index].pos.x + 0.5f);
}

static void **stubEdgesNear(const ZVec3 *p, float radius);
static void stubClosest(const int16_t *a, const int16_t *b, const ZVec3 *dir, const ZVec3 *p, ZVec3 *out);
static double stubPartLength(void *actor, int part);

static void **edgeList;
static ZVec3 closestOff;
static int closestHave;

static void **stubEdgesNear(const ZVec3 *p, float radius)
{
    lg(0xed9e0000u);
    lg((uint32_t)(uintptr_t)p);
    lgf(p->x);
    lgf(p->y);
    lgf(p->z);
    lgf(radius);
    return edgeList;
}

static void stubClosest(const int16_t *a, const int16_t *b, const ZVec3 *dir, const ZVec3 *p, ZVec3 *out)
{
    lg(0xc10e0000u);
    lg((uint32_t)(uintptr_t)a);
    lg((uint32_t)(uintptr_t)b);
    lg((uint32_t)(uintptr_t)dir);
    lg((uint32_t)(uintptr_t)p);
    if (closestHave && dt_u32() % 3 == 0) {
        /* the same distance from the other side: equal scores, different points */
        out->x = (float)(p->x - closestOff.x);
        out->y = (float)(p->y + closestOff.y);
        out->z = (float)(p->z + closestOff.z);
        return;
    }
    closestOff.x = dt_float(-90.0f, 90.0f);
    closestOff.y = dt_float(-90.0f, 90.0f);
    closestOff.z = dt_float(-90.0f, 90.0f);
    closestHave = 1;
    out->x = (float)(p->x + closestOff.x);
    out->y = (float)(p->y + closestOff.y);
    out->z = (float)(p->z + closestOff.z);
}

/* Returns a length with more precision than a float, as the x87 original does. */
static double stubPartLength(void *actor, int part)
{
    lg(0x1e460000u);
    lg((uint32_t)(uintptr_t)actor);
    lg((uint32_t)part);
    return (double)dt_float(0.5f, 40.0f) * (1.0 + (double)dt_float(0.0f, 1.0f) * 1e-6);
}

/* ---- fixtures ---------------------------------------------------------------------------------- */

static float rf(float lo, float hi)
{
    return dt_float(lo, hi);
}

static void rand_vec(ZVec3 *v, float range)
{
    v->x = rf(-range, range);
    v->y = rf(-range, range);
    v->z = rf(-range, range);
}

/* Rows of random floats; the unused words of the rows are random bits. */
static void rand_matrix(ZMatrix *m, float range)
{
    dt_fill(m, sizeof *m);
    rand_vec(&m->right, range);
    rand_vec(&m->up, range);
    rand_vec(&m->at, range);
    rand_vec(&m->pos, 50.0f);
}

static ZMatrix *new_matrix(float range)
{
    ZMatrix *m = dt_alloc(sizeof *m);
    rand_matrix(m, range);
    return m;
}

static ZVec3 *new_vec(float range)
{
    ZVec3 *v = dt_alloc(16);
    dt_fill(v, 16);
    rand_vec(v, range);
    return v;
}

/*
 * A rig with random points (mode, flags, basis, mass, positions), `constraints` constraints and `hinges`
 * hinges between random points (distinct where it matters), center and callback. Padding is random.
 */
static ZRig *make_rig(int points, int constraints, int hinges)
{
    ZRig *rig = dt_alloc(sizeof *rig);

    dt_fill(rig, sizeof *rig);
    rig->pointCount = (uint32_t)points;
    for (int i = 0; i < points; i++) {
        ZRigPoint *p = &rig->points[i];

        p->mode = (uint16_t)(dt_u32() % 4 == 0 ? 0 : (dt_u32() % 3 == 0 ? 2 : 1));
        p->flags = (uint16_t)(dt_u32() % 5 == 0 ? (dt_u32() & 0xffff) | 1 : (dt_u32() & 0xfffe));
        rand_matrix(&p->basis, 1.0f);
        p->mass = rf(0.1f, 4.0f);
        p->weight = rf(0.1f, 8.0f);
        p->friction = rf(0.0f, 1.0f);
        p->restitution = rf(0.0f, 1.0f);
        rand_vec(&p->pos, 60.0f);
        rand_vec(&p->prev, 60.0f);
        rand_vec(&p->accum, 100.0f);
        rand_vec(&p->vel, 4.0f);
    }
    rig->constraintCount = (uint32_t)constraints;
    for (int i = 0; i < constraints; i++) {
        ZRigConstraint *c = &rig->constraints[i];
        const uint32_t t = dt_u32() % 20;
        const int a = dt_range(0, points - 1);
        int b = dt_range(0, points - 1);

        if (b == a) {
            b = (a + 1) % points;
        }
        c->type = t < 10 ? kZoverStick : (t < 17 ? kZoverStickLinked : (t < 19 ? kZoverCorner : 3));
        c->stiffness = rf(0.0f, 24.0f);
        c->a = &rig->points[a];
        c->b = &rig->points[b];
        if (c->type == kZoverCorner) {
            c->u.corner.angle = (uint16_t)dt_u32();
        } else {
            c->u.stick.length = rf(1.0f, 60.0f);
            c->u.stick.c = &rig->points[dt_range(0, points - 1)];
            c->u.stick.d = &rig->points[dt_range(0, points - 1)];
            c->u.stick.alignGain = rf(0.0f, 5.0f);
        }
    }
    rig->hingeCount = (uint32_t)hinges;
    for (int i = 0; i < hinges; i++) {
        ZRigHinge *h = &rig->hinges[i];
        const uint32_t t = dt_u32() % 10;

        h->type = t < 3 ? kZoverHingeSwing : (t < 6 ? kZoverHingeTwist : (t < 9 ? kZoverHingeBone : 3));
        h->a = &rig->points[dt_range(0, points - 1)];
        h->b = &rig->points[dt_range(0, points - 1)];
        h->c = &rig->points[dt_range(0, points - 1)];
        h->refA = &rig->points[dt_range(0, points - 1)];
        h->refB = &rig->points[dt_range(0, points - 1)];
        h->sign = dt_range(0, 3);
        h->axisA = dt_range(0, 7);
        h->axisB = dt_range(0, 7);
        h->strength = rf(0.0f, 2.0f);
    }
    rand_vec(&rig->center, 20.0f);
    rig->onHit = dt_u32() % 3 == 0 ? NULL : hitCallback;
    rig->hingesOff = dt_u32() % 4 == 0;
    return rig;
}

static void begin(const DtCtx *c)
{
    dt_seed(c);
    zlogN = 0;
}

static void stubs_collide(void)
{
    dt_stub(0x00451390, NULL, stubCollide);
}

static void stubs_resolve(void)
{
    dt_stub(0x00451d60, NULL, stubResolve);
}

/* ---- construction ------------------------------------------------------------------------------- */

typedef uint32_t(BMX_STDCALL *TimeFn)(void);
#define tTimeGetTime ORIG_VAR(0x00699838, TimeFn, tTimeGetTime)
#define tLastPoll    ORIG_VAR(0x005746b0, uint32_t, tLastPoll)

static void setup_poll_timer(DtCtx *c)
{
    const uint32_t now = tTimeGetTime();

    begin(c);
    dt_stub(0x0046e010, NULL, stubPoll);
    tLastPoll = now - (uint32_t)dt_range(0, 8);
    for (int i = 0; i < 5; i++) {
        DT_ARG_INT(c, i, dt_u32());
    }
}

static void setup_rig_create(DtCtx *c)
{
    begin(c);
    dt_static_region(0x4000);
    dt_fill(gZmem.staticBase, 0x4000);
    gZmem.staticTop = (uint32_t *)((uint8_t *)gZmem.staticBase + 4u * (uint32_t)dt_range(0, 0x40));
}

static void setup_add_point(DtCtx *c)
{
    ZRigPointDef *def = dt_alloc(sizeof *def);
    ZRig *rig;

    begin(c);
    rig = make_rig(dt_range(0, 14), 0, 0);
    dt_fill(def, sizeof *def);
    def->mode = (uint16_t)(dt_u32() % 5 == 0 ? dt_u32() : dt_range(0, 2));
    rand_vec(&def->pos, 80.0f);
    def->mass = rf(0.1f, 4.0f);
    def->friction = rf(0.0f, 1.0f);
    def->restitution = rf(0.0f, 1.0f);
    DT_ARG_PTR(c, 0, rig);
    DT_ARG_PTR(c, 1, def);
}

static void setup_add_stick(DtCtx *c)
{
    const int n = dt_range(2, 14);
    ZRig *rig;

    begin(c);
    rig = make_rig(n, dt_range(0, 20), 0);
    DT_ARG_PTR(c, 0, rig);
    DT_ARG_INT(c, 1, dt_range(0, n - 1));
    DT_ARG_INT(c, 2, dt_range(0, n - 1));
    DT_ARG_FLOAT(c, 3, rf(0.0f, 24.0f));
}

static void setup_add_stick_linked(DtCtx *c)
{
    setup_add_stick(c);
    {
        const int n = (int)((ZRig *)(uintptr_t)c->args[0])->pointCount;

        DT_ARG_INT(c, 3, dt_range(0, n - 1));
        DT_ARG_INT(c, 4, dt_range(0, n - 1));
        DT_ARG_FLOAT(c, 5, rf(0.0f, 24.0f));
        DT_ARG_FLOAT(c, 6, rf(0.0f, 5.0f));
    }
}

static void setup_add_corner(DtCtx *c)
{
    const int n = dt_range(3, 14);
    ZRig *rig;

    begin(c);
    rig = make_rig(n, dt_range(0, 20), 0);
    DT_ARG_PTR(c, 0, rig);
    for (int i = 1; i < 6; i++) {
        DT_ARG_INT(c, i, dt_range(0, n - 1));
    }
    DT_ARG_FLOAT(c, 6, rf(0.0f, 24.0f));
}

static void setup_add_hinge(DtCtx *c)
{
    const int n = dt_range(2, 14);
    ZRig *rig;
    ZRigHingeDef *def = dt_alloc(sizeof *def);

    begin(c);
    rig = make_rig(n, 0, dt_range(0, 6));
    dt_fill(def, sizeof *def);
    def->axisA = dt_range(0, 7);
    def->axisB = dt_range(0, 7);
    def->a = &rig->points[dt_range(0, n - 1)];
    def->b = &rig->points[dt_range(0, n - 1)];
    def->c = &rig->points[dt_range(0, n - 1)];
    def->refA = &rig->points[dt_range(0, n - 1)];
    def->refB = &rig->points[dt_range(0, n - 1)];
    def->sign = dt_range(0, 3);
    def->strength = rf(0.0f, 2.0f);
    DT_ARG_PTR(c, 0, rig);
    DT_ARG_PTR(c, 1, def);
}

/* A hinge of the given type (or any when type < 0) inside a random rig. */
static void setup_hinge_of(DtCtx *c, int type)
{
    ZRig *rig;
    ZRigHinge *h;

    begin(c);
    rig = make_rig(dt_range(2, 14), 0, dt_range(1, 7));
    h = &rig->hinges[dt_range(0, (int)rig->hingeCount - 1)];
    if (type >= 0) {
        h->type = type;
    }
    h->sign = dt_range(0, 2) == 0 ? 0 : (int32_t)dt_range(1, 2);
    DT_ARG_PTR(c, 0, h);
}

static void setup_hinge_swing(DtCtx *c)
{
    setup_hinge_of(c, kZoverHingeSwing);
}

static void setup_hinge_twist(DtCtx *c)
{
    setup_hinge_of(c, kZoverHingeTwist);
}

static void setup_hinge_bone(DtCtx *c)
{
    setup_hinge_of(c, kZoverHingeBone);
}

static void setup_solve_hinges(DtCtx *c)
{
    begin(c);
    const int n = dt_range(0, 12);

    DT_ARG_PTR(c, 0, make_rig(n, 0, n == 0 ? 0 : dt_range(0, 7)));
}

static void setup_relax(DtCtx *c)
{
    int n = dt_range(0, 15);
    ZRig *rig;

    begin(c);
    stubs_collide();
    if (n == 1) {
        n = 2;
    }
    rig = make_rig(n, n == 0 ? 0 : dt_range(0, 22), n == 0 ? 0 : dt_range(0, 7));
    DT_ARG_PTR(c, 0, rig);
}

static void setup_set_hit_callback(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_rig(0, 0, 0));
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_build_frame(DtCtx *c)
{
    const int n = dt_range(3, 12);

    begin(c);
    DT_ARG_PTR(c, 0, make_rig(n, 0, 0));
    DT_ARG_INT(c, 1, dt_range(0, n - 1));
    DT_ARG_INT(c, 2, dt_range(0, n - 1));
    DT_ARG_INT(c, 3, dt_range(0, n - 1));
    DT_ARG_PTR(c, 4, new_matrix(3.0f));
}

static void setup_project(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, new_vec(30.0f));
    DT_ARG_PTR(c, 1, new_vec(30.0f));
    DT_ARG_FLOAT(c, 2, rf(-5.0f, 5.0f));
    DT_ARG_PTR(c, 3, dt_alloc(8));
    DT_ARG_PTR(c, 4, dt_alloc(8));
    DT_ARG_PTR(c, 5, new_vec(2.0f));
}

/* A constraint of the given type inside a random rig. */
static void setup_constraint_of(DtCtx *c, int type)
{
    ZRig *rig;
    ZRigConstraint *k;

    begin(c);
    rig = make_rig(dt_range(2, 14), dt_range(1, 22), 0);
    k = &rig->constraints[dt_range(0, (int)rig->constraintCount - 1)];
    k->type = type;
    k->u.stick.length = rf(1.0f, 60.0f);
    k->u.stick.c = &rig->points[dt_range(0, (int)rig->pointCount - 1)];
    k->u.stick.d = &rig->points[dt_range(0, (int)rig->pointCount - 1)];
    k->u.stick.alignGain = rf(0.0f, 5.0f);
    if (type == kZoverStickLinked && dt_u32() % 2 == 0) {
        /* make the alignment term positive more often: rows of c.up and d.right point against each other */
        k->u.stick.c->basis.up = (ZVec3){0.0f, 1.0f, 0.0f};
        k->u.stick.d->basis.right = (ZVec3){0.0f, -rf(0.1f, 1.0f), 0.0f};
    }
    DT_ARG_PTR(c, 0, k);
}

static void setup_stick(DtCtx *c)
{
    setup_constraint_of(c, kZoverStick);
}

static void setup_stick_linked(DtCtx *c)
{
    setup_constraint_of(c, kZoverStickLinked);
}

static void setup_corner(DtCtx *c)
{
    setup_constraint_of(c, kZoverCorner);
}

static void setup_triangle(DtCtx *c)
{
    setup_rig_create(c);
}

/* ---- bike and rider motion ---------------------------------------------------------------------- */

static ZActor *rand_actor(void)
{
    ZActor *a = dt_rand_actor(kZactorTypePerson);

    a->skeleton = (int32_t)(dt_u32() % 4 == 0);
    return a;
}

static void setup_start_bike(DtCtx *c)
{
    ZRig *rig;

    begin(c);
    rig = make_rig(dt_range(0, 3), 0, 0);
    DT_ARG_PTR(c, 0, rig);
    DT_ARG_PTR(c, 1, rand_actor());
    DT_ARG_PTR(c, 2, new_matrix(1.0f));
    DT_ARG_PTR(c, 3, new_vec(5.0f));
    {
        ZAngle3 *rot = dt_alloc(8);

        rot->x = (ZAngle)dt_range(-0x4000, 0x4000);
        rot->y = (ZAngle)dt_range(-0x8000, 0x7fff);
        rot->z = (ZAngle)dt_range(-0x4000, 0x4000);
        DT_ARG_PTR(c, 4, rot);
    }
}

static void setup_step_bike(DtCtx *c)
{
    ZRig *rig;

    begin(c);
    stubs_collide();
    rig = make_rig(3, dt_range(0, 6), dt_range(0, 3));
    DT_ARG_PTR(c, 0, rig);
    DT_ARG_PTR(c, 1, rand_actor());
}

static void setup_zero_angles(DtCtx *c)
{
    ZAngle3 *a = dt_alloc(8);

    begin(c);
    dt_fill(a, 8);
    DT_ARG_PTR(c, 0, a);
}

static void setup_load_pose(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_rig(dt_range(0, 15), 0, 0));
    DT_ARG_PTR(c, 1, rand_actor());
}

static void setup_create_rider(DtCtx *c)
{
    setup_rig_create(c);
    gZoverRigNext = dt_range(0, 1);
    DT_ARG_PTR(c, 0, rand_actor());
    DT_ARG_INT(c, 1, dt_u32());
}

/* ---- rider state -------------------------------------------------------------------------------- */

static ZRig *stateRigs[2];

static void rand_flags(int32_t *f)
{
    f[0] = (int32_t)(dt_u32() & 1);
    f[1] = (int32_t)(dt_u32() & 1);
}

/*
 * Two rigs in the rig table with the numbers the state functions index with, and random request and latch
 * flags. The selected rig index starts at a random value; the rig passed in is mostly one of the two.
 */
static ZRig *state_begin(DtCtx *c, int points)
{
    const int pts = points;
    ZRig *rig;

    begin(c);
    stateRigs[0] = make_rig(pts, pts < 2 ? 0 : dt_range(11, 22), pts < 2 ? 0 : dt_range(6, 7));
    stateRigs[1] = make_rig(pts, pts < 2 ? 0 : dt_range(11, 22), pts < 2 ? 0 : dt_range(6, 7));
    gZoverRigs[0] = stateRigs[0];
    gZoverRigs[1] = stateRigs[1];
    gZoverRigIndex = (int32_t)(dt_u32() & 1);
    rand_flags(gZoverHandsGrabbed);
    rand_flags(gZoverHandsHolding);
    rand_flags(gZoverHandsEased);
    rand_flags(gZoverLimp);
    rand_flags(gZoverStiff);
    rand_flags(gZoverRelaxed);
    rand_flags(gZoverSticky);
    rand_flags(gZoverPitchOn);
    rand_flags(gZoverYawOn);
    gZoverPitch[0] = (int16_t)dt_u32();
    gZoverPitch[1] = (int16_t)dt_u32();
    gZoverYaw[0] = (int16_t)dt_u32();
    gZoverYaw[1] = (int16_t)dt_u32();
    for (int i = 0; i < 6; i++) {
        gZoverHingeIdx[i] = dt_range(0, 5);
    }
    for (int i = 0; i < 8; i++) {
        gZoverStickIdx[i] = dt_range(0, 10);
    }
    for (int i = 0; i < 3; i++) {
        gZoverBackIdx[i] = dt_range(0, 10);
    }
    rig = dt_u32() % 8 == 0 ? make_rig(pts, 0, 0) : stateRigs[dt_u32() & 1];
    return rig;
}

static void setup_start_rider(DtCtx *c)
{
    ZRig *rig = state_begin(c, dt_range(0, 15));

    DT_ARG_PTR(c, 0, rig);
    DT_ARG_PTR(c, 1, rand_actor());
    DT_ARG_PTR(c, 2, new_matrix(1.0f));
    DT_ARG_PTR(c, 3, new_vec(5.0f));
    DT_ARG_PTR(c, 4, new_vec(5.0f));
}

static void setup_select(DtCtx *c)
{
    DT_ARG_PTR(c, 0, state_begin(c, 3));
}

static void setup_rig_actor(DtCtx *c)
{
    ZRig *rig = state_begin(c, dt_range(2, 14));

    DT_ARG_PTR(c, 0, rig);
    DT_ARG_PTR(c, 1, rand_actor());
}

static void setup_rig_only(DtCtx *c)
{
    DT_ARG_PTR(c, 0, state_begin(c, dt_range(2, 14)));
}

static void setup_ease(DtCtx *c)
{
    setup_rig_actor(c);
}

static void setup_grab_edges(DtCtx *c)
{
    ZRig *rig = state_begin(c, dt_range(12, 14));
    const int n = dt_range(0, 4);

    stubs_collide();
    closestHave = 0;
    dt_stub(0x00466ed0, NULL, stubEdgesNear);
    dt_stub(0x00468920, (void *)zbspClosestPointOnEdge, stubClosest);
    if (dt_u32() % 5 == 0) {
        edgeList = NULL;
    } else {
        edgeList = (void **)dt_alloc(8 * sizeof(void *));
        for (int i = 0; i < n; i++) {
            edgeList[i] = dt_alloc(0x40);
        }
        edgeList[n] = NULL;
    }
    /* mostly free hands: the function does nothing while they hold */
    if (dt_u32() % 4 != 0) {
        gZoverHandsHolding[0] = gZoverHandsHolding[1] = 0;
        gZoverHandsGrabbed[0] = gZoverHandsGrabbed[1] = 0;
    }
    DT_ARG_PTR(c, 0, rig);
}

static void setup_no_args(DtCtx *c)
{
    (void)state_begin(c, 3);
}

static void setup_blend_vec(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, new_vec(50.0f));
    DT_ARG_PTR(c, 1, new_vec(50.0f));
}

static void setup_aim_hands(DtCtx *c)
{
    ZRig *rig = state_begin(c, dt_range(9, 14));

    DT_ARG_PTR(c, 0, rig);
}

static void setup_blend_to_pose(DtCtx *c)
{
    ZRig *rig = state_begin(c, dt_range(9, 14));

    DT_ARG_PTR(c, 0, rig);
}

static void setup_apply_rotation(DtCtx *c)
{
    ZRig *rig = state_begin(c, dt_range(13, 14));

    stubs_resolve();
    DT_ARG_PTR(c, 0, rig);
    DT_ARG_PTR(c, 1, rand_actor());
    DT_ARG_PTR(c, 2, new_matrix(1.0f));
}

static void setup_apply_euler(DtCtx *c)
{
    ZRig *rig = state_begin(c, dt_range(13, 14));

    stubs_resolve();
    DT_ARG_PTR(c, 0, rig);
    DT_ARG_PTR(c, 1, rand_actor());
}

static void setup_request_angle(DtCtx *c)
{
    ZRig *rig = state_begin(c, dt_range(2, 14));

    DT_ARG_PTR(c, 0, rig);
    DT_ARG_PTR(c, 1, rand_actor());
    DT_ARG_INT(c, 2, dt_u32());
}

/* ---- frames ------------------------------------------------------------------------------------- */

static void setup_vec_sub(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, new_vec(100.0f));
    DT_ARG_PTR(c, 1, new_vec(100.0f));
    DT_ARG_PTR(c, 2, new_vec(1.0f));
}

static void setup_cross_neg(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, new_vec(10.0f));
    DT_ARG_PTR(c, 1, new_vec(10.0f));
    DT_ARG_PTR(c, 2, new_vec(1.0f));
}

static void setup_transpose(DtCtx *c)
{
    ZMatrix *in = new_matrix(2.0f);

    begin(c);
    DT_ARG_PTR(c, 0, in);
    DT_ARG_PTR(c, 1, dt_u32() % 4 == 0 ? in : new_matrix(2.0f));
}

static void setup_frame_from_points(DtCtx *c)
{
    const int n = dt_range(4, 14);

    begin(c);
    DT_ARG_PTR(c, 0, make_rig(n, 0, 0));
    for (int i = 1; i < 5; i++) {
        DT_ARG_INT(c, i, dt_range(0, n - 1));
    }
    DT_ARG_PTR(c, 5, new_matrix(1.0f));
}

static void setup_frame_ref(DtCtx *c)
{
    const int n = dt_range(2, 14);
    ZRig *rig;
    ZMatrix *ref;
    int ia, ib;

    begin(c);
    rig = make_rig(n, 0, 0);
    ia = dt_range(0, n - 1);
    ib = dt_range(0, n - 1);
    ref = new_matrix(dt_u32() % 3 == 0 ? 1.0f : 2.0f);
    /*
     * Now and then one of the reference rows is orthogonal to the segment ia - ib (which then runs along x),
     * so that its dot product is exactly zero: the edge of the sign tests.
     */
    if (dt_u32() % 4 == 0 && ia != ib) {
        rig->points[ib].pos.y = rig->points[ia].pos.y;
        rig->points[ib].pos.z = rig->points[ia].pos.z;
        rig->points[ib].pos.x = (float)(rig->points[ia].pos.x - rf(1.0f, 30.0f));
        switch (dt_u32() % 3) {
        case 0:
            ref->right.x = 0.0f;
            break;
        case 1:
            ref->up.x = dt_u32() & 1 ? 0.0f : -0.0f;
            break;
        default:
            ref->at.x = 0.0f;
            break;
        }
    }
    DT_ARG_PTR(c, 0, rig);
    DT_ARG_INT(c, 1, ia);
    DT_ARG_INT(c, 2, ib);
    DT_ARG_PTR(c, 3, ref);
    DT_ARG_PTR(c, 4, new_matrix(1.0f));
}

static void setup_frame_reflect(DtCtx *c)
{
    setup_frame_ref(c);
    DT_ARG_INT(c, 5, dt_u32() % 3 == 0 ? dt_u32() : (dt_u32() & 1));
}

static void setup_frame_from_up(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_rig(2, 0, 0));
    DT_ARG_PTR(c, 1, new_vec(5.0f));
    DT_ARG_PTR(c, 2, new_matrix(1.5f));
    DT_ARG_PTR(c, 3, new_matrix(1.0f));
}

static void setup_mul3(DtCtx *c)
{
    ZMatrix *a = new_matrix(2.0f);
    ZMatrix *b = new_matrix(2.0f);
    ZMatrix *out = new_matrix(1.0f);

    begin(c);
    switch (dt_u32() % 5) {
    case 0:
        out = a;
        break;
    case 1:
        out = b;
        break;
    default:
        break;
    }
    DT_ARG_PTR(c, 0, out);
    DT_ARG_PTR(c, 1, a);
    DT_ARG_PTR(c, 2, b);
}

static void setup_matrix_to_angles(DtCtx *c)
{
    ZMatrix *m = new_matrix(1.0f);
    ZAngle3 *out = dt_alloc(8);

    begin(c);
    dt_fill(out, 8);
    if (dt_u32() % 3 == 0) {
        /* gimbal lock: the first row's xy length is below 0.01 */
        m->right.x = rf(-0.004f, 0.004f);
        m->right.y = rf(-0.004f, 0.004f);
    }
    DT_ARG_PTR(c, 0, m);
    DT_ARG_PTR(c, 1, out);
}

static void setup_angles_from_frame(DtCtx *c)
{
    ZAngle3 *out = dt_alloc(8);

    begin(c);
    dt_fill(out, 8);
    DT_ARG_PTR(c, 0, rand_actor());
    DT_ARG_PTR(c, 1, new_matrix(1.0f));
    DT_ARG_PTR(c, 2, out);
}

/* ---- limbs -------------------------------------------------------------------------------------- */

static ZLimbs *new_limbs(void)
{
    ZLimbs *l = dt_alloc(sizeof *l);

    dt_fill(l, sizeof *l);
    return l;
}

static void setup_limb_blend(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, new_limbs());
    DT_ARG_FLOAT(c, 1, rf(0.0f, 1.0f));
    DT_ARG_FLOAT(c, 2, rf(0.0f, 0.1f));
}

static void setup_limbs_only(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, new_limbs());
}

static void setup_limbs_begin(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_PTR(c, 1, new_limbs());
}

static void setup_limbs_init(DtCtx *c)
{
    begin(c);
    dt_stub(0x00464ea0, NULL, stubPartLength);
    DT_ARG_PTR(c, 0, rand_actor());
    DT_ARG_PTR(c, 1, new_limbs());
}

static void setup_combine3(DtCtx *c)
{
    float *w = dt_alloc(16);

    begin(c);
    w[0] = rf(-2.0f, 2.0f);
    w[1] = rf(-2.0f, 2.0f);
    w[2] = rf(-2.0f, 2.0f);
    DT_ARG_PTR(c, 0, w);
    DT_ARG_PTR(c, 1, new_vec(20.0f));
    DT_ARG_PTR(c, 2, new_vec(20.0f));
    DT_ARG_PTR(c, 3, new_vec(20.0f));
    DT_ARG_PTR(c, 4, dt_u32() % 5 == 0 ? (ZVec3 *)(uintptr_t)c->args[1] : new_vec(1.0f));
}

static void setup_limb_targets(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, new_limbs());
    DT_ARG_PTR(c, 1, new_vec(30.0f));
    DT_ARG_PTR(c, 2, new_vec(30.0f));
    DT_ARG_INT(c, 3, dt_u32() % 4 == 0 ? dt_u32() : (uint32_t)dt_range(0, 15));
}

/* ---- overlay ------------------------------------------------------------------------------------ */

static void stubFillBar(uint32_t color, uint32_t x, int y, uint32_t width, const void *rect)
{
    const ZOverRectSize *r = rect;

    lg(0xfb4a0000u);
    lg(color);
    lg(x);
    lg((uint32_t)y);
    lg(width);
    lg((uint32_t)(uintptr_t)rect);
    lg(r->w);
    lg(r->h);
}

static void stubGlyph(uint32_t x, int y, const void *record)
{
    const ZOverGlyphRecord *r = record;

    lg(0x61790000u);
    lg(x);
    lg((uint32_t)y);
    lg((uint32_t)(uintptr_t)record);
    lg(r->image);
    lg(r->x);
    lg(r->y);
    lg(r->width);
    lg(r->height);
    lg(r->color);
    lg(r->flags);
}

static ZOverItem *pool_items(uint32_t pool, uint32_t frame, uint32_t *count)
{
    switch (pool) {
    case 0:
        *count = 16;
        return gZoverPool0[frame];
    case 1:
        *count = 128;
        return gZoverPool1[frame];
    case 2:
        *count = 28;
        return gZoverPool2[frame];
    default:
        *count = 400;
        return gZoverPool3[frame];
    }
}

/* Pool number of a (hi, lo) pair: hi * 2 + lo. */
static uint32_t handle_for(uint32_t pool, uint32_t index)
{
    return (pool >> 1) << 14 | (pool & 1) << 12 | index;
}

/*
 * Random pools in a state the overlay functions can run on: kinds and flags per item, the glyph pools (odd
 * pools) built from proper chains, `fullPct` percent of the items free.
 */
static void ov_pools(int freePct)
{
    dt_fill(&gZoverPool0, sizeof(ZOverPool0));
    dt_fill(&gZoverPool1, sizeof(ZOverPool1));
    dt_fill(&gZoverPool2, sizeof(ZOverPool2));
    dt_fill(&gZoverPool3, sizeof(ZOverPool3));
    for (uint32_t pool = 0; pool < 4; pool++) {
        for (uint32_t frame = 0; frame < 2; frame++) {
            uint32_t count;
            ZOverItem *items = pool_items(pool, frame, &count);
            uint32_t order[400];
            uint32_t glyphs = 0;

            for (uint32_t i = 0; i < count; i++) {
                const int isFree = dt_range(0, 99) < freePct;
                uint8_t kind;

                if (isFree) {
                    kind = 0;
                } else if (pool & 1) {
                    kind = kZoverKindGlyph;
                } else {
                    kind = (uint8_t)dt_range(2, 4);
                }
                items[i].b[kZoverItemKind] = kind;
                items[i].b[kZoverItemFlags] = (uint8_t)dt_u32();
                items[i].h[kZoverItemNext] = 0xffff;
                if (kind == kZoverKindGlyph) {
                    order[glyphs++] = i;
                }
            }
            /* shuffle the glyph items and chain them in runs of one to six */
            for (uint32_t i = glyphs; i > 1; i--) {
                const uint32_t j = (uint32_t)dt_range(0, (int)i - 1);
                const uint32_t t = order[i - 1];

                order[i - 1] = order[j];
                order[j] = t;
            }
            for (uint32_t i = 0; i < glyphs;) {
                const uint32_t len = (uint32_t)dt_range(1, 6);
                uint32_t k = 0;

                while (k + 1 < len && i + k + 1 < glyphs) {
                    items[order[i + k]].h[kZoverItemNext] = (uint16_t)handle_for(pool, order[i + k + 1]);
                    k++;
                }
                i += k + 1;
            }
        }
    }
    dt_fill(&gZoverSprites, 64 * sizeof(ZOverSprite));
}

/* A handle of an item of some pool of the current frame (mostly an in-use one). */
static uint32_t ov_handle(void)
{
    const uint32_t pool = (uint32_t)dt_range(0, 3);
    uint32_t count;
    ZOverItem *items = pool_items(pool, gZoverFrame, &count);
    uint32_t idx = (uint32_t)dt_range(0, (int)count - 1);

    for (int tries = 0; tries < 8 && items[idx].b[kZoverItemKind] == 0; tries++) {
        idx = (uint32_t)dt_range(0, (int)count - 1);
    }
    return handle_for(pool, idx);
}

static uint32_t ov_handle_in(uint32_t pool)
{
    uint32_t count;
    ZOverItem *items = pool_items(pool, gZoverFrame, &count);
    uint32_t idx = (uint32_t)dt_range(0, (int)count - 1);

    for (int tries = 0; tries < 8 && items[idx].b[kZoverItemKind] == 0; tries++) {
        idx = (uint32_t)dt_range(0, (int)count - 1);
    }
    return handle_for(pool, idx);
}

/* Handle word as a caller would pass it: the upper half is garbage now and then. */
static uint32_t ov_garbage(uint32_t h)
{
    return dt_u32() % 3 == 0 ? h | (dt_u32() & 0xffff0000u) : h;
}

static void ov_lists(uint32_t target)
{
    gZoverFreeCount = (uint32_t)dt_range(0, 10);
    gZoverDirtyCount = (uint32_t)dt_range(0, 10);
    for (uint32_t i = 0; i < gZoverFreeCount; i++) {
        gZoverFreeList[i] = (int16_t)(dt_u32() % 3 == 0 ? target : ov_handle());
    }
    for (uint32_t i = 0; i < gZoverDirtyCount; i++) {
        gZoverDirtyList[i] = (int16_t)(dt_u32() % 3 == 0 ? target : ov_handle());
    }
    if (dt_u32() % 12 == 0) {
        gZoverDirtyCount = 0x23b; /* the next push trips the overflow assert */
    }
}

static void ov_begin(const DtCtx *c, int freePct)
{
    begin(c);
    ov_pools(freePct);
    gZoverFrame = (uint8_t)(dt_u32() & 1);
    gZoverLayer = (uint8_t)dt_range(0, 2);
    gZoverBusy = (uint32_t)dt_range(0, 3);
    gZoverFontCount = (uint8_t)dt_range(0, 9);
    gZoverOt = dt_alloc(8);
    *gZoverOt = dt_u32();
    dt_fill(&gZoverTerminators, sizeof(ZOverTerminators));
    ov_lists(0xffff);
}

static void setup_ov_init(DtCtx *c)
{
    ov_begin(c, 30);
}

static void setup_ov_flush(DtCtx *c)
{
    ov_begin(c, 30);
    gZoverBusy = dt_u32() % 8 == 0 ? (uint32_t)dt_range(1, 3) : 0;
    gZoverFreeCount = (uint32_t)dt_range(0, 20);
    gZoverDirtyCount = (uint32_t)dt_range(0, 20);
    for (uint32_t i = 0; i < gZoverFreeCount; i++) {
        gZoverFreeList[i] = (int16_t)ov_handle();
    }
    for (uint32_t i = 0; i < gZoverDirtyCount; i++) {
        gZoverDirtyList[i] = (int16_t)ov_handle();
    }
    if (dt_u32() % 20 == 0) {
        gZoverFreeCount = 0x23c + (dt_u32() & 1);
    }
    if (dt_u32() % 20 == 0) {
        gZoverDirtyCount = 0x23c;
    }
}

static void setup_ov_layer_flag(DtCtx *c)
{
    ov_begin(c, 30);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_ov_get_pool(DtCtx *c)
{
    uint32_t **out = (uint32_t **)dt_alloc(3 * sizeof(void *));

    ov_begin(c, 30);
    DT_ARG_INT(c, 0, dt_u32() % 2 ? dt_range(0, 3) : (int)(dt_u32() & 0xffffff01u));
    DT_ARG_INT(c, 1, dt_u32() % 2 ? dt_range(0, 3) : (int)(dt_u32() & 0xffffff03u));
    DT_ARG_INT(c, 2, dt_u32() % 2 ? dt_range(0, 1) : (int)(dt_u32() & 0xffffff01u));
    for (int i = 0; i < 3; i++) {
        out[i] = dt_alloc(4);
        *out[i] = dt_u32();
    }
    DT_ARG_PTR(c, 3, out[0]);
    DT_ARG_PTR(c, 4, out[1]);
    DT_ARG_PTR(c, 5, out[2]);
}

static void setup_ov_alloc(DtCtx *c)
{
    ov_begin(c, dt_range(0, 100) < 10 ? 100 : (dt_range(0, 100) < 10 ? 0 : 40));
    DT_ARG_INT(c, 0, dt_u32() % 3 ? dt_range(0, 1) : (int)(dt_u32() & 0xffffff01u));
    DT_ARG_INT(c, 1, dt_u32() % 3 ? dt_range(0, 1) : (int)(dt_u32() & 0xffffff01u));
}

static uint32_t ov_target(int allowNone)
{
    const uint32_t r = dt_u32() % 16;
    uint32_t count;

    if (r == 0 && allowNone) {
        return 0xffff;
    }
    if (r == 1) { /* index past the pool's end */
        const uint32_t pool = (uint32_t)dt_range(0, 3);

        (void)pool_items(pool, 0, &count);
        return handle_for(pool, count + (uint32_t)dt_range(0, 30));
    }
    return ov_handle();
}

static void setup_ov_remove(DtCtx *c)
{
    uint32_t h;

    ov_begin(c, 25);
    h = ov_target(1);
    ov_lists(h);
    DT_ARG_INT(c, 0, ov_garbage(h));
}

static void setup_ov_show(DtCtx *c)
{
    uint32_t h;

    ov_begin(c, 25);
    h = dt_u32() % 12 == 0 ? 0xffff : ov_handle();
    ov_lists(h);
    DT_ARG_INT(c, 0, ov_garbage(h));
    DT_ARG_INT(c, 1, dt_u32() % 2 ? (int)(dt_u32() & 3) : (int)dt_u32());
}

static void setup_ov_add_rect(DtCtx *c)
{
    ov_begin(c, dt_u32() % 8 == 0 ? 0 : 40);
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_INT(c, 1, dt_u32() % 4 ? dt_range(-100, 700) : (int)dt_u32());
    DT_ARG_INT(c, 2, dt_u32() % 4 ? dt_range(-100, 500) : (int)dt_u32());
    DT_ARG_INT(c, 3, dt_u32() % 4 ? dt_range(0, 600) : (int)dt_u32());
    DT_ARG_INT(c, 4, dt_u32() % 4 ? dt_range(0, 400) : (int)dt_u32());
    DT_ARG_INT(c, 5, dt_u32());
}

static void setup_ov_set_rect(DtCtx *c)
{
    uint32_t h;

    ov_begin(c, 25);
    h = ov_target(0);
    ov_lists(h);
    DT_ARG_INT(c, 0, ov_garbage(h));
    for (int i = 1; i < 5; i++) {
        DT_ARG_INT(c, i, dt_u32() % 4 ? dt_range(-100, 700) : (int)dt_u32());
    }
}

static void setup_ov_set_rect_color(DtCtx *c)
{
    uint32_t h;

    ov_begin(c, 25);
    h = ov_target(0);
    ov_lists(h);
    DT_ARG_INT(c, 0, ov_garbage(h));
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_ov_draw_rect(DtCtx *c)
{
    ov_begin(c, 25);
    dt_stub(0x004623a0, NULL, stubFillBar);
    for (int i = 0; i < 4; i++) {
        DT_ARG_INT(c, i, dt_u32() % 4 ? dt_range(-300, 700) : (int)dt_u32());
    }
    DT_ARG_INT(c, 4, dt_u32());
}

static void setup_ov_add_gradient(DtCtx *c)
{
    setup_ov_add_rect(c);
    DT_ARG_INT(c, 5, dt_u32());
    DT_ARG_INT(c, 6, dt_u32());
}

static void setup_ov_add_sprite(DtCtx *c)
{
    ov_begin(c, dt_u32() % 8 == 0 ? 0 : 40);
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_INT(c, 1, dt_u32() % 3 ? dt_range(0, 63) : (int)(dt_u32() & 0xffff0000u) | dt_range(0, 63));
    DT_ARG_INT(c, 2, dt_u32() % 4 ? dt_range(-100, 700) : (int)dt_u32());
    DT_ARG_INT(c, 3, dt_u32() % 4 ? dt_range(-100, 500) : (int)dt_u32());
    DT_ARG_INT(c, 4, dt_u32());
}

static ZOverFont *new_font(void)
{
    ZOverFont *f = dt_alloc(sizeof *f);
    uint16_t *image = dt_alloc(4);
    const uint32_t count = (uint32_t)dt_range(1, 60);
    ZOverGlyph *glyphs = dt_alloc(count * sizeof(ZOverGlyph) + 8);

    *image = (uint16_t)dt_range(0, 63);
    f->image = image;
    f->pad04 = (uint8_t)dt_u32();
    f->height = (uint8_t)dt_range(4, 30);
    f->firstChar = (int8_t)(dt_u32() % 8 == 0 ? dt_range(-128, -100) : dt_range(0, 100));
    f->charCount = (uint8_t)count;
    for (uint32_t i = 0; i < count; i++) {
        glyphs[i].x = (uint16_t)dt_range(0, 255);
        glyphs[i].y = (uint16_t)dt_range(0, 255);
        glyphs[i].width = (uint16_t)dt_range(0, 30);
        glyphs[i].pad06 = (uint16_t)dt_u32();
    }
    f->glyphs = glyphs;
    return f;
}

/* Non-zero characters around the font's range (some outside it, some above 0x7f). */
static char *new_text(const ZOverFont *f, int maxLen)
{
    const int len = dt_range(0, maxLen);
    char *s = dt_alloc((uint32_t)len + 1);

    for (int i = 0; i < len; i++) {
        int ch = (int)(uint8_t)f->firstChar + dt_range(-3, f->charCount + 3);

        ch &= 0xff;
        s[i] = (char)(ch == 0 ? 0x41 : ch);
    }
    s[len] = 0;
    return s;
}

static void setup_ov_add_text(DtCtx *c)
{
    ZOverFont *f = new_font();

    ov_begin(c, dt_u32() % 8 == 0 ? 90 : 40);
    if (dt_u32() % 8 == 0) { /* few free items: the text runs out of pool */
        ov_pools(97);
    }
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_PTR(c, 1, dt_u32() % 25 == 0 ? NULL : f);
    DT_ARG_PTR(c, 2, new_text(f, 14));
    DT_ARG_INT(c, 3, dt_u32() % 4 ? dt_range(-50, 600) : (int)dt_u32());
    DT_ARG_INT(c, 4, dt_u32() % 4 ? dt_range(-50, 400) : (int)dt_u32());
    DT_ARG_INT(c, 5, dt_u32());
    DT_ARG_INT(c, 6, dt_u32() % 4 ? dt_range(0, 8) : (int)dt_u32());
}

/* A chain head: the first item of a glyph chain (or any item). */
static uint32_t ov_chain_head(void)
{
    const uint32_t pool = dt_u32() & 1 ? 3 : 1;

    return ov_handle_in(pool);
}

static void setup_ov_set_color(DtCtx *c)
{
    ZOverFont *f = new_font();
    uint32_t h;

    ov_begin(c, 25);
    h = dt_u32() % 10 == 0 ? ov_target(0) : ov_chain_head();
    ov_lists(h);
    DT_ARG_INT(c, 0, ov_garbage(h));
    DT_ARG_PTR(c, 1, f);
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32() % 4 ? dt_range(0, 8) : (int)dt_u32());
}

static void setup_ov_move_text(DtCtx *c)
{
    uint32_t h;

    ov_begin(c, 25);
    h = dt_u32() % 10 == 0 ? ov_target(0) : ov_chain_head();
    ov_lists(h);
    DT_ARG_INT(c, 0, ov_garbage(h));
    DT_ARG_INT(c, 1, dt_u32() % 4 ? dt_range(-50, 600) : (int)dt_u32());
    DT_ARG_INT(c, 2, dt_u32() % 4 ? dt_range(-50, 400) : (int)dt_u32());
}

static void setup_ov_draw_text(DtCtx *c)
{
    ZOverFont *f = new_font();

    ov_begin(c, 25);
    dt_stub(0x00462940, NULL, stubGlyph);
    DT_ARG_PTR(c, 0, dt_u32() % 25 == 0 ? NULL : f);
    DT_ARG_PTR(c, 1, new_text(f, 12));
    DT_ARG_INT(c, 2, dt_u32() % 4 ? dt_range(-50, 600) : (int)dt_u32());
    DT_ARG_INT(c, 3, dt_u32() % 4 ? dt_range(-50, 400) : (int)dt_u32());
    DT_ARG_INT(c, 4, dt_u32());
    DT_ARG_INT(c, 5, dt_u32());
}

static void setup_ov_text_width(DtCtx *c)
{
    ZOverFont *f = new_font();

    ov_begin(c, 25);
    DT_ARG_PTR(c, 0, new_text(f, 20));
    DT_ARG_PTR(c, 1, dt_u32() % 25 == 0 ? NULL : f);
}

static void setup_ov_parse_font(DtCtx *c)
{
    const int n = dt_range(0, 24);
    uint8_t *data = dt_alloc(0xc + 13 * 25);
    ZOverFont *f = dt_alloc(sizeof *f);
    const int base = dt_range(0, 200);

    ov_begin(c, 25);
    dt_static_region(0x2000);
    dt_fill(gZmem.staticBase, 0x2000);
    if (dt_u32() % 10 == 0) {
        /* too small a static region: the glyph table does not fit */
        gZmem.staticEnd = (uintptr_t)gZmem.staticTop + 16;
    }
    dt_fill(data, 0xc + 13 * 25);
    dt_fill(f, sizeof *f);
    data[0] = (uint8_t)n;
    data[4] = (uint8_t)dt_range(4, 40);
    for (int i = 0; i < n; i++) {
        data[0xc + 13 * i] = (uint8_t)(base + dt_range(0, 40));
    }
    if (dt_u32() % 8 == 0) { /* codes spread over the whole byte range */
        for (int i = 0; i < n; i++) {
            data[0xc + 13 * i] = (uint8_t)dt_u32();
        }
    }
    DT_ARG_PTR(c, 0, data);
    DT_ARG_PTR(c, 1, dt_u32() % 25 == 0 ? NULL : f);
}

/* ---- cases -------------------------------------------------------------------------------------- */

DT_CASES(zoverii_cases) = {
    {"zoverPollTimer", 0x00450020, zoverPollTimer, 5, DT_RET_INT, setup_poll_timer, 100, 0},
    {"zoverRigCreate", 0x00450060, zoverRigCreate, 0, DT_RET_INT, setup_rig_create, 40, 0},
    {"zoverRigAddPoint", 0x004500a0, zoverRigAddPoint, 2, DT_RET_INT, setup_add_point, 300, 0},
    {"zoverRigAddStick", 0x00450140, zoverRigAddStick, 4, DT_RET_INT, setup_add_stick, 300, 0},
    {"zoverRigAddStickLinked", 0x00450210, zoverRigAddStickLinked, 7, DT_RET_VOID, setup_add_stick_linked,
     300, 0},
    {"zoverRigAddCorner", 0x00450300, zoverRigAddCorner, 7, DT_RET_VOID, setup_add_corner, 300, 0},
    {"zoverRigAddHingeSwing", 0x00450430, zoverRigAddHingeSwing, 2, DT_RET_INT, setup_add_hinge, 100, 0},
    {"zoverRigAddHingeTwist", 0x004504c0, zoverRigAddHingeTwist, 2, DT_RET_INT, setup_add_hinge, 100, 0},
    {"zoverRigAddBone", 0x00450550, zoverRigAddBone, 2, DT_RET_VOID, setup_add_hinge, 300, 0},
    {"zoverRigSolveHingeSwing", 0x004505f0, zoverRigSolveHingeSwing, 1, DT_RET_VOID, setup_hinge_swing, 800,
     0},
    {"zoverRigSolveHingeTwist", 0x00450990, zoverRigSolveHingeTwist, 1, DT_RET_VOID, setup_hinge_twist, 800,
     0},
    {"zoverRigSolveBone", 0x00450d30, zoverRigSolveBone, 1, DT_RET_VOID, setup_hinge_bone, 300, 0},
    {"zoverRigSolveHinges", 0x00450e00, zoverRigSolveHinges, 1, DT_RET_VOID, setup_solve_hinges, 300, 0},
    {"zoverRigRelax", 0x00450e80, zoverRigRelax, 1, DT_RET_VOID, setup_relax, 500, 0},
    {"zoverRigSetHitCallback", 0x00451280, zoverRigSetHitCallback, 2, DT_RET_VOID, setup_set_hit_callback, 20,
     0},
    {"zoverRigBuildFrame", 0x004512a0, zoverRigBuildFrame, 5, DT_RET_VOID, setup_build_frame, 300, 0},
    {"zoverRigProject", 0x00451c80, zoverRigProject, 6, DT_RET_VOID, setup_project, 300, 0},
    {"zoverRigSolveStick", 0x00452370, zoverRigSolveStick, 1, DT_RET_VOID, setup_stick, 300, 0},
    {"zoverRigSolveStickLinked", 0x004524d0, zoverRigSolveStickLinked, 1, DT_RET_VOID, setup_stick_linked,
     500, 0},
    {"zoverRigSolveCorner", 0x00452680, zoverRigSolveCorner, 1, DT_RET_VOID, setup_corner, 10, 0},
    {"zoverRigCreateTriangle", 0x004526a0, zoverRigCreateTriangle, 0, DT_RET_INT, setup_triangle, 40, 0},
    {"zoverStartBikeMotion", 0x004527c0, zoverStartBikeMotion, 5, DT_RET_VOID, setup_start_bike, 300, 0},
    {"zoverStepBikeMotion", 0x00452a40, zoverStepBikeMotion, 2, DT_RET_VOID, setup_step_bike, 300, 0},
    {"zoverRigLoadPose", 0x00452b80, zoverRigLoadPose, 2, DT_RET_VOID, setup_load_pose, 200, 0},
    {"zoverRigCreateRider", 0x00452e80, zoverRigCreateRider, 2, DT_RET_INT, setup_create_rider, 300, 0},
    {"zoverRigSelect", 0x00453460, zoverRigSelect, 1, DT_RET_VOID, setup_select, 100, 0},
    {"zoverStartRiderMotion", 0x004534a0, zoverStartRiderMotion, 5, DT_RET_VOID, setup_start_rider, 200, 0},
    {"zoverRigEase", 0x004535d0, zoverRigEase, 2, DT_RET_VOID, setup_ease, 300, 0},
    {"zoverRigGrabEdges", 0x004536a0, zoverRigGrabEdges, 1, DT_RET_VOID, setup_grab_edges, 400, 0},
    {"zoverRigHandsHolding", 0x00453870, zoverRigHandsHolding, 0, DT_RET_INT, setup_no_args, 20, 0},
    {"zoverRigReleaseHands", 0x00453890, zoverRigReleaseHands, 1, DT_RET_VOID, setup_rig_only, 100, 0},
    {"zoverBlendVec", 0x004538e0, zoverBlendVec, 2, DT_RET_VOID, setup_blend_vec, 200, 0},
    {"zoverRigAimHands", 0x00453920, zoverRigAimHands, 1, DT_RET_VOID, setup_aim_hands, 200, 0},
    {"zoverRigGoStiff", 0x00453ab0, zoverRigGoStiff, 2, DT_RET_VOID, setup_rig_actor, 200, 0},
    {"zoverRigEndStiff", 0x00453b70, zoverRigEndStiff, 1, DT_RET_VOID, setup_rig_only, 100, 0},
    {"zoverRigBlendToPose", 0x00453c30, zoverRigBlendToPose, 1, DT_RET_VOID, setup_blend_to_pose, 200, 0},
    {"zoverRigGoLimp", 0x00453f80, zoverRigGoLimp, 2, DT_RET_VOID, setup_rig_actor, 300, 0},
    {"zoverRigEndLimp", 0x00454140, zoverRigEndLimp, 1, DT_RET_VOID, setup_rig_only, 100, 0},
    {"zoverRigClearHinges", 0x004542d0, zoverRigClearHinges, 1, DT_RET_VOID, setup_rig_only, 20, 0},
    {"zoverRigRequestSticky", 0x004542f0, zoverRigRequestSticky, 1, DT_RET_VOID, setup_rig_only, 20, 0},
    {"zoverRigClearSticky", 0x00454320, zoverRigClearSticky, 1, DT_RET_VOID, setup_rig_only, 100, 0},
    {"zoverRigFreezeFlagged", 0x004543b0, zoverRigFreezeFlagged, 1, DT_RET_VOID, setup_rig_only, 100, 0},
    {"zoverRigRequestRelaxed", 0x00454400, zoverRigRequestRelaxed, 1, DT_RET_VOID, setup_rig_only, 20, 0},
    {"zoverRigClearRelaxed", 0x00454430, zoverRigClearRelaxed, 1, DT_RET_VOID, setup_rig_only, 100, 0},
    {"zoverRigApplyRotation", 0x004544b0, zoverRigApplyRotation, 3, DT_RET_VOID, setup_apply_rotation, 200,
     0},
    {"zoverRigApplyPitch", 0x004545e0, zoverRigApplyPitch, 2, DT_RET_VOID, setup_apply_euler, 200, 0},
    {"zoverRigRequestPitch", 0x00454660, zoverRigRequestPitch, 3, DT_RET_VOID, setup_request_angle, 100, 0},
    {"zoverRigClearPitch", 0x004546a0, zoverRigClearPitch, 1, DT_RET_VOID, setup_rig_only, 20, 0},
    {"zoverRigApplyYaw", 0x004546d0, zoverRigApplyYaw, 2, DT_RET_VOID, setup_apply_euler, 200, 0},
    {"zoverRigRequestYaw", 0x00454750, zoverRigRequestYaw, 3, DT_RET_VOID, setup_request_angle, 100, 0},
    {"zoverRigClearYaw", 0x00454790, zoverRigClearYaw, 1, DT_RET_VOID, setup_rig_only, 20, 0},
    {"zoverFrameFromPoints", 0x00455080, zoverFrameFromPoints, 6, DT_RET_VOID, setup_frame_from_points, 300,
     0},
    {"zoverFrameFollow", 0x00455140, zoverFrameFollow, 5, DT_RET_VOID, setup_frame_ref, 500, 0},
    {"zoverFrameSnap", 0x00455430, zoverFrameSnap, 5, DT_RET_VOID, setup_frame_ref, 500, 0},
    {"zoverFrameFollowFlipped", 0x00455590, zoverFrameFollowFlipped, 5, DT_RET_VOID, setup_frame_ref, 500, 0},
    {"zoverFrameReflect", 0x00455880, zoverFrameReflect, 6, DT_RET_VOID, setup_frame_reflect, 400, 0},
    {"zoverFrameFromUp", 0x004559c0, zoverFrameFromUp, 4, DT_RET_VOID, setup_frame_from_up, 400, 0},
    {"zoverVecSub", 0x00455b20, zoverVecSub, 3, DT_RET_VOID, setup_vec_sub, 200, 0},
    {"zoverCrossNeg", 0x00455b60, zoverCrossNeg, 3, DT_RET_VOID, setup_cross_neg, 200, 0},
    {"zoverZeroAngles", 0x00455bf0, zoverZeroAngles, 1, DT_RET_VOID, setup_zero_angles, 20, 0},
    {"zoverTransposeRotation", 0x00455c20, zoverTransposeRotation, 2, DT_RET_VOID, setup_transpose, 100, 0},
    {"zoverMul3", 0x00455c60, zoverMul3, 3, DT_RET_VOID, setup_mul3, 400, 0},
    {"zoverMatrixToAngles", 0x00455cf0, zoverMatrixToAngles, 2, DT_RET_VOID, setup_matrix_to_angles, 400, 0},
    {"zoverActorAnglesFromFrame", 0x00455dd0, zoverActorAnglesFromFrame, 3, DT_RET_VOID,
     setup_angles_from_frame, 200, 0},
    {"zoverSetLimbBlend", 0x00455e60, zoverSetLimbBlend, 3, DT_RET_VOID, setup_limb_blend, 100, 0},
    {"zoverLimbsSnap", 0x00455e90, zoverLimbsSnap, 1, DT_RET_VOID, setup_limbs_only, 20, 0},
    {"zoverResetLimbs", 0x00455ec0, zoverResetLimbs, 1, DT_RET_VOID, setup_limbs_only, 20, 0},
    {"zoverLimbsBegin", 0x00455ef0, zoverLimbsBegin, 2, DT_RET_VOID, setup_limbs_begin, 10, 0},
    {"zoverLimbsInit", 0x00455f10, zoverLimbsInit, 2, DT_RET_VOID, setup_limbs_init, 300, 0},
    {"zoverCombine3", 0x00456030, zoverCombine3, 5, DT_RET_VOID, setup_combine3, 300, 0},
    {"zoverSetLimbTargets", 0x00457790, zoverSetLimbTargets, 4, DT_RET_VOID, setup_limb_targets, 200, 0},
    {"zoverInit", 0x004578f0, zoverInit, 0, DT_RET_INT, setup_ov_init, 40, 0},
    {"zoverFlush", 0x00457b00, zoverFlush, 0, DT_RET_INT, setup_ov_flush, 400, 0},
    {"zoverSetLayerFlag", 0x00457f50, zoverSetLayerFlag, 1, DT_RET_INT, setup_ov_layer_flag, 20, 0},
    {"zoverGetPool", 0x00457f70, zoverGetPool, 6, DT_RET_INT, setup_ov_get_pool, 200, 0},
    {"zoverAllocItem", 0x004580e0, zoverAllocItem, 2, DT_RET_INT16, setup_ov_alloc, 200, 0},
    {"zoverRemove", 0x004581a0, zoverRemove, 1, DT_RET_VOID, setup_ov_remove, 400, 0},
    {"zoverShow", 0x00458400, zoverShow, 2, DT_RET_INT, setup_ov_show, 500, 0},
    {"zoverAddRect", 0x004588d0, zoverAddRect, 6, DT_RET_INT, setup_ov_add_rect, 400, 0},
    {"zoverSetRect", 0x00458b00, zoverSetRect, 5, DT_RET_INT, setup_ov_set_rect, 300, 0},
    {"zoverSetRectColor", 0x00458cc0, zoverSetRectColor, 2, DT_RET_INT, setup_ov_set_rect_color, 300, 0},
    {"zoverDrawRect", 0x00458ea0, zoverDrawRect, 5, DT_RET_VOID, setup_ov_draw_rect, 100, 0},
    {"zoverAddGradient", 0x00458f20, zoverAddGradient, 7, DT_RET_INT, setup_ov_add_gradient, 400, 0},
    {"zoverAddSprite", 0x00459180, zoverAddSprite, 5, DT_RET_INT, setup_ov_add_sprite, 400, 0},
    {"zoverAddText", 0x004594c0, zoverAddText, 7, DT_RET_INT16, setup_ov_add_text, 500, 0},
    {"zoverSetColor", 0x00459940, zoverSetColor, 4, DT_RET_INT, setup_ov_set_color, 400, 0},
    {"zoverMoveText", 0x00459be0, zoverMoveText, 3, DT_RET_INT, setup_ov_move_text, 300, 0},
    {"zoverDrawText", 0x00459df0, zoverDrawText, 6, DT_RET_VOID, setup_ov_draw_text, 400, 0},
    {"zoverTextWidth", 0x00459f30, zoverTextWidth, 2, DT_RET_INT16, setup_ov_text_width, 300, 0},
    {"zoverParseFont", 0x00459fc0, zoverParseFont, 2, DT_RET_INT, setup_ov_parse_font, 300, 0},
};
