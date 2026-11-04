/*
 * The large x87 functions of zoverii.c: the level contact of a rig point (zoverRigCollidePoint 0x451390,
 * zoverRigResolvePoint 0x451d60), the rider rig step (zoverStepRiderMotion 0x4547c0) and the rider's limb
 * IK (zoverSolveLimbs 0x456120). Level queries, part positions and the per-point contact of the rig step are
 * stubbed; the stubs log their arguments by value and answer from fixtures built around the inputs, so that
 * the faces lie near the swept segments and the limb targets near their roots.
 */
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_zactor.h"

/* ---- log ------------------------------------------------------------------------------------------ */

enum { kLogSize = 1024 };

static uint32_t zbLog[kLogSize];
static uint32_t zbLogN;

static void lg(uint32_t v)
{
    if (zbLogN < kLogSize) {
        zbLog[zbLogN] = v;
    }
    zbLogN++;
}

static void lgp(const void *p)
{
    lg(dt_canon((uint32_t)(uintptr_t)p));
}

static void lgf(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    lg(u);
}

static void lgv(const ZVec3 *v)
{
    lgf(v->x);
    lgf(v->y);
    lgf(v->z);
}

static void begin(const DtCtx *c)
{
    dt_seed(c);
    zbLogN = 0;
}

/* ---- vectors -------------------------------------------------------------------------------------- */

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

/* A random direction (never zero), scaled to `len`. */
static void rand_dir(ZVec3 *v, float len)
{
    double l;

    do {
        rand_vec(v, 1.0f);
        l = __builtin_sqrt((double)v->x * v->x + (double)v->y * v->y + (double)v->z * v->z);
    } while (l < 0.1);
    v->x = (float)(v->x / l * len);
    v->y = (float)(v->y / l * len);
    v->z = (float)(v->z / l * len);
}

static void lerp(ZVec3 *out, const ZVec3 *a, const ZVec3 *b, float t)
{
    out->x = a->x + (b->x - a->x) * t;
    out->y = a->y + (b->y - a->y) * t;
    out->z = a->z + (b->z - a->z) * t;
}

static void add_scaled(ZVec3 *v, const ZVec3 *d, float s)
{
    v->x += d->x * s;
    v->y += d->y * s;
    v->z += d->z * s;
}

static void rand_matrix(ZMatrix *m)
{
    dt_fill(m, sizeof *m);
    rand_dir(&m->right, 1.0f);
    rand_dir(&m->up, 1.0f);
    rand_dir(&m->at, 1.0f);
    rand_vec(&m->pos, 50.0f);
}

/* ---- level faces ---------------------------------------------------------------------------------- */

static ZbspFaceQuery *faceQuery;
static uint32_t matRefs[4];

static void *stubFindFaceNear(const ZVec3 *p, float radius, float cosLimit)
{
    lg(0xfa0e0000u);
    lgv(p);
    lgf(radius);
    lgf(cosLimit);
    return faceQuery;
}

static const uint32_t *stubMatRef(const void *obj)
{
    const uint32_t r = dt_u32() % 5;

    lg(0x3a7e0000u);
    lgp(obj);
    return r == 4 ? NULL : &matRefs[r];
}

/*
 * Up to six faces around the segment from-to: planes through points of the segment, pushed along the
 * normal by up to 30 units (some far away, some degenerate, some pairs facing each other), with closest
 * points nearby. The plane distance d is stale: the code refreshes it from the owner.
 */
static void make_faces(const ZVec3 *from, const ZVec3 *to)
{
    const uint32_t mode = dt_u32() % 16;
    const int n = mode == 0 ? 0 : dt_range(1, 6);
    ZbspFaceHit *hits = dt_alloc((uint32_t)(n + 1) * sizeof *hits);

    faceQuery = dt_alloc(sizeof *faceQuery);
    dt_fill(faceQuery, sizeof *faceQuery);
    faceQuery->hits = hits;
    if (mode == 1) {
        faceQuery = NULL;
    }
    for (int i = 0; i < n; i++) {
        PhysFace *f = dt_alloc(sizeof *f);
        uint8_t *owner = dt_alloc(0x50);
        ZVec3 *onPlane = (ZVec3 *)(owner + 0x10);
        ZVec3 s;
        const uint32_t kind = dt_u32() % 16;

        dt_fill(f, sizeof *f);
        dt_fill(owner, 0x50);
        dt_fill(&hits[i], sizeof hits[i]);
        if (i > 0 && (mode == 2 || kind == 0)) {
            /* the reverse of the previous face: the normals cancel */
            const PhysFace *prev = hits[i - 1].face;

            f->normal.x = -prev->normal.x;
            f->normal.y = -prev->normal.y;
            f->normal.z = -prev->normal.z;
        } else if (kind == 1) {
            f->normal.x = 0.0f;
            f->normal.y = 0.0f;
            f->normal.z = 0.0f;
        } else {
            rand_dir(&f->normal, kind == 2 ? rf(0.5f, 1.5f) : 1.0f);
        }
        f->d = rf(-500.0f, 500.0f);
        f->owner = (const struct ZrwColHit *)(const void *)owner;
        lerp(&s, from, to, rf(-0.2f, 1.2f));
        *onPlane = s;
        add_scaled(onPlane, &f->normal, kind == 3 ? rf(-400.0f, 400.0f) : rf(-30.0f, 30.0f));
        hits[i].face = f;
        hits[i].dist = rf(-50.0f, 50.0f);
        hits[i].point = s;
        add_scaled(&hits[i].point, &f->normal, rf(-5.0f, 5.0f));
        hits[i].point.x += rf(-3.0f, 3.0f);
        hits[i].point.z += rf(-3.0f, 3.0f);
        if (dt_u32() % 4 == 0) {
            uint16_t *object = dt_alloc(8);

            dt_fill(object, 8);
            hits[i].object = (PhysPropObject *)(void *)object;
        } else {
            hits[i].object = 0;
        }
    }
    hits[n].face = NULL;
}

/* Speed classes: over the 100 limit, below the slow limit, below 1, normal. */
static void rand_velocity(ZVec3 *v)
{
    switch (dt_u32() % 8) {
    case 0:
        rand_dir(v, rf(100.5f, 300.0f));
        break;
    case 1:
        rand_dir(v, rf(0.0f, 0.3f));
        break;
    case 2:
        rand_dir(v, rf(0.3f, 1.0f));
        break;
    default:
        rand_dir(v, rf(1.0f, 40.0f));
        break;
    }
}

static ZRigPoint *make_point(void)
{
    ZRigPoint *p = dt_alloc(sizeof *p);

    dt_fill(p, sizeof *p);
    p->mode = (uint16_t)dt_range(0, 2);
    p->flags = (uint16_t)dt_u32();
    p->mass = rf(0.1f, 4.0f);
    p->weight = rf(0.1f, 8.0f);
    p->friction = rf(0.0f, 2.0f);
    p->restitution = rf(0.0f, 1.0f);
    rand_vec(&p->prev, 300.0f);
    rand_velocity(&p->vel);
    return p;
}

static void stubs_faces(void)
{
    dt_stub(0x00468070, NULL, stubFindFaceNear);
    dt_stub(0x00466bb0, NULL, stubMatRef);
}

static void setup_collide(DtCtx *c)
{
    ZRigPoint *p;
    float *impact = dt_alloc(4);
    ZRigHit *hit = dt_alloc(sizeof *hit);
    ZVec3 to;

    begin(c);
    stubs_faces();
    p = make_point();
    /* the position moved from prev by about one frame's velocity */
    p->pos = p->prev;
    add_scaled(&p->pos, &p->vel, rf(0.0f, 1.5f));
    to = p->pos;
    add_scaled(&to, &p->vel, 1.0f);
    make_faces(&p->prev, &to);
    dt_fill(impact, 4);
    dt_fill(hit, sizeof *hit);
    DT_ARG_PTR(c, 0, p);
    DT_ARG_PTR(c, 1, impact);
    DT_ARG_PTR(c, 2, hit);
}

static void setup_resolve(DtCtx *c)
{
    ZRigPoint *p;
    ZVec3 move, to;

    begin(c);
    stubs_faces();
    p = make_point();
    rand_velocity(&move);
    p->pos = p->prev;
    add_scaled(&p->pos, &move, 1.0f);
    to = p->pos;
    add_scaled(&to, &move, 1.0f);
    make_faces(&p->prev, &to);
    if (dt_u32() % 4 != 0) {
        p->flags &= (uint16_t)~kZoverPointStopped;
    }
    DT_ARG_PTR(c, 0, p);
}

/* ---- rider rig step ------------------------------------------------------------------------------- */

/* Body template of the rider rig (see zoverStepRiderMotion), y up. */
static const float kPose[14][3] = {
    {0, 40, 0},   {-8, 35, 0}, {8, 35, 0},   {-8, 18, 4}, {8, 18, 4},    {-8, 0, 0},   {8, 0, 0},
    {-12, 65, 0}, {12, 65, 0}, {-14, 50, 8}, {14, 50, 8}, {-14, 38, 16}, {14, 38, 16}, {0, 78, 0},
};

static ZRig *stepRigs[2];

static int collideHits;

static int stubCollide(ZRigPoint *p, float *impact, ZRigHit *hit)
{
    const uint32_t r = dt_u32();

    lg(0xc0110000u);
    lgp(p);
    if (r % 3 == 0) {
        *impact = (float)(r >> 12);
        memset(hit, 0, sizeof *hit);
        hit->kind = r & 3;
        hit->point = p->pos;
        p->pos.y = (float)(p->pos.y + dt_float(0.0f, 3.0f));
        collideHits++;
        return 1;
    }
    return 0;
}

static void stubResolve(ZRigPoint *p)
{
    lg(0x4e500000u);
    lgp(p);
    p->pos.y = (float)(p->pos.y + dt_float(0.0f, 2.0f));
}

static void BMX_CDECL hitCallback(ZRig *rig, uint32_t index, float impact, ZRigHit *hit)
{
    lg(0xca110000u);
    lgp(rig);
    lg(index);
    lgf(impact);
    lg(hit->kind);
    lgv(&hit->point);
}

static void **edgeList;

static void **stubEdgesNear(const ZVec3 *p, float radius)
{
    lg(0xed9e0000u);
    lgv(p);
    lgf(radius);
    return edgeList;
}

static void stubClosest(const int16_t *a, const int16_t *b, const ZVec3 *dir, const ZVec3 *p, ZVec3 *out)
{
    lg(0xc10e0000u);
    lgp(a);
    lgp(b);
    lgp(dir);
    lgv(p);
    out->x = (float)(p->x + dt_float(-70.0f, 70.0f));
    out->y = (float)(p->y + dt_float(-70.0f, 70.0f));
    out->z = (float)(p->z + dt_float(-70.0f, 70.0f));
}

/*
 * A 14 point rider rig in the template pose (turned, moved and jittered), moving a little, with random
 * sticks and hinges (the rest lengths roughly right) and random point attributes.
 */
static ZRig *make_body_rig(void)
{
    ZRig *rig = dt_alloc(sizeof *rig);
    static const float kTurn[8][2] = {{1, 0},  {0.8f, 0.6f},   {0, 1},  {-0.6f, 0.8f},
                                      {-1, 0}, {-0.8f, -0.6f}, {0, -1}, {0.6f, -0.8f}};
    const uint32_t turn = dt_u32() & 7;
    const float cy = kTurn[turn][0];
    const float sy = kTurn[turn][1];
    ZVec3 base;

    dt_fill(rig, sizeof *rig);
    rand_vec(&base, 500.0f);
    rig->pointCount = 14;
    for (int i = 0; i < 14; i++) {
        ZRigPoint *p = &rig->points[i];

        p->mode = (uint16_t)(dt_u32() % 6 == 0 ? 0 : dt_range(1, 2));
        p->flags = (uint16_t)(dt_u32() & 3);
        rand_matrix(&p->basis);
        p->mass = rf(0.1f, 1.0f);
        p->weight = rf(0.1f, 2.0f);
        p->friction = rf(0.0f, 2.0f);
        p->restitution = rf(0.0f, 1.0f);
        p->pos.x = base.x + kPose[i][0] * cy + kPose[i][2] * sy + rf(-3.0f, 3.0f);
        p->pos.y = base.y + kPose[i][1] + rf(-3.0f, 3.0f);
        p->pos.z = base.z - kPose[i][0] * sy + kPose[i][2] * cy + rf(-3.0f, 3.0f);
        rand_vec(&p->vel, 3.0f);
        p->prev = p->pos;
        add_scaled(&p->prev, &p->vel, -1.0f);
        rand_vec(&p->accum, 100.0f);
    }
    rig->constraintCount = (uint32_t)dt_range(11, 22);
    for (uint32_t i = 0; i < rig->constraintCount; i++) {
        ZRigConstraint *k = &rig->constraints[i];
        const int a = dt_range(0, 13);
        const int b = (a + dt_range(1, 13)) % 14;
        const float dx = kPose[a][0] - kPose[b][0];
        const float dy = kPose[a][1] - kPose[b][1];
        const float dz = kPose[a][2] - kPose[b][2];

        k->type = dt_u32() % 4 == 0 ? kZoverStickLinked : kZoverStick;
        k->stiffness = rf(0.0f, 24.0f);
        k->a = &rig->points[a];
        k->b = &rig->points[b];
        k->u.stick.length = __builtin_sqrtf(dx * dx + dy * dy + dz * dz) * rf(0.9f, 1.1f);
        k->u.stick.c = &rig->points[dt_range(0, 13)];
        k->u.stick.d = &rig->points[dt_range(0, 13)];
        k->u.stick.alignGain = rf(0.0f, 5.0f);
    }
    rig->hingeCount = (uint32_t)dt_range(6, 7);
    for (uint32_t i = 0; i < rig->hingeCount; i++) {
        ZRigHinge *h = &rig->hinges[i];
        const uint32_t t = dt_u32() % 3;

        h->type = t == 0 ? kZoverHingeSwing : (t == 1 ? kZoverHingeTwist : kZoverHingeBone);
        h->a = &rig->points[dt_range(0, 13)];
        h->b = &rig->points[dt_range(0, 13)];
        h->c = &rig->points[dt_range(0, 13)];
        h->refA = &rig->points[dt_range(0, 13)];
        h->refB = &rig->points[dt_range(0, 13)];
        h->sign = dt_range(0, 3);
        h->axisA = dt_range(1, 6);
        h->axisB = dt_range(1, 6);
        h->strength = rf(0.0f, 2.0f);
    }
    rig->center = rig->points[0].pos;
    rig->onHit = dt_u32() % 3 == 0 ? NULL : hitCallback;
    rig->hingesOff = dt_u32() % 3 == 0;
    return rig;
}

static void rand_flags(int32_t *f, uint32_t oneIn)
{
    f[0] = dt_u32() % oneIn == 0;
    f[1] = dt_u32() % oneIn == 0;
}

static void setup_step_rider(DtCtx *c)
{
    ZActor *actor;
    ZRig *rig;

    begin(c);
    collideHits = 0;
    dt_stub(0x00451390, NULL, stubCollide);
    dt_stub(0x00451d60, NULL, stubResolve);
    dt_stub(0x00466ed0, NULL, stubEdgesNear);
    dt_stub(0x00468920, (void *)zbspClosestPointOnEdge, stubClosest);
    if (dt_u32() % 3 == 0) {
        edgeList = NULL;
    } else {
        const int n = dt_range(0, 3);

        edgeList = (void **)dt_alloc(4 * sizeof(void *));
        for (int i = 0; i < n; i++) {
            edgeList[i] = dt_alloc(0x40);
            dt_fill(edgeList[i], 0x40);
        }
        edgeList[n] = NULL;
    }
    stepRigs[0] = make_body_rig();
    stepRigs[1] = make_body_rig();
    gZoverRigs[0] = stepRigs[0];
    gZoverRigs[1] = stepRigs[1];
    gZoverRigIndex = (int32_t)(dt_u32() & 1);
    rand_flags(gZoverHandsGrabbed, 2);
    rand_flags(gZoverHandsHolding, 2);
    rand_flags(gZoverHandsEased, 3);
    rand_flags(gZoverLimp, 3);
    rand_flags(gZoverStiff, 3);
    rand_flags(gZoverRelaxed, 4);
    rand_flags(gZoverSticky, 3);
    rand_flags(gZoverPitchOn, 2);
    rand_flags(gZoverYawOn, 2);
    gZoverPitch[0] = (int16_t)dt_range(-0x800, 0x800);
    gZoverPitch[1] = (int16_t)dt_range(-0x800, 0x800);
    gZoverYaw[0] = (int16_t)dt_range(-0x800, 0x800);
    gZoverYaw[1] = (int16_t)dt_range(-0x800, 0x800);
    for (int i = 0; i < 6; i++) {
        gZoverHingeIdx[i] = dt_range(0, 5);
    }
    for (int i = 0; i < 8; i++) {
        gZoverStickIdx[i] = dt_range(0, 10);
    }
    for (int i = 0; i < 3; i++) {
        gZoverBackIdx[i] = dt_range(0, 10);
    }
    /* the frames the step keeps between calls */
    dt_fill((void *)(uintptr_t)0x00574794, 0xc0);
    dt_fill((void *)(uintptr_t)0x00574854, 0x40);
    dt_fill((void *)(uintptr_t)0x00574938, 0x40);
    dt_fill((void *)(uintptr_t)0x00574a54, 0x40);
    rig = dt_u32() % 8 == 0 ? make_body_rig() : stepRigs[dt_u32() & 1];
    actor = dt_rand_actor(kZactorTypePerson);
    actor->skeleton = (int32_t)(dt_u32() % 4 == 0);
    DT_ARG_PTR(c, 0, rig);
    DT_ARG_PTR(c, 1, actor);
}

/* ---- limb IK -------------------------------------------------------------------------------------- */

/* Part positions: the actor's position plus a fixed offset per part, the local point and some noise. */
static ZVec3 partBase[16];

static void part_position(int sync, void *actor, int part, const ZVec3 *local, ZVec3 *out)
{
    lg(sync ? 0x9a570000u : 0x9a570001u);
    lgp(actor);
    lg((uint32_t)part);
    lgp(local);
    lgv(local);
    lgp(out);
    out->x = (float)(partBase[part & 15].x + local->x * 10.0f + dt_float(-0.5f, 0.5f));
    out->y = (float)(partBase[part & 15].y + local->y * 10.0f + dt_float(-0.5f, 0.5f));
    out->z = (float)(partBase[part & 15].z + local->z * 10.0f + dt_float(-0.5f, 0.5f));
}

static void stubPartPosition(void *actor, int part, const ZVec3 *local, ZVec3 *out)
{
    part_position(1, actor, part, local, out);
}

static void stubPartPositionNoSync(void *actor, int part, const ZVec3 *local, ZVec3 *out)
{
    part_position(0, actor, part, local, out);
}

/* Root part and bone length slots of each limb. */
static const int kLimbRoot[4] = {3, 6, 9, 0xc};
static const int kLimbLen[4][2] = {{0, 1}, {4, 5}, {3, 2}, {7, 6}};

static void setup_solve_limbs(DtCtx *c)
{
    ZActor *actor;
    ZLimbs *limbs = dt_alloc(sizeof *limbs);
    ZVec3 center;

    begin(c);
    dt_stub(0x00464b50, NULL, stubPartPosition);
    dt_stub(0x00464d10, NULL, stubPartPositionNoSync);
    actor = dt_rand_actor(kZactorTypePerson);
    rand_vec(&center, 500.0f);
    for (int i = 0; i < 16; i++) {
        partBase[i] = center;
        add_scaled(&partBase[i], &(ZVec3){rf(-40.0f, 40.0f), rf(-40.0f, 40.0f), rf(-40.0f, 40.0f)}, 1.0f);
    }
    /* the whole solver state, including the local point of the part queries */
    dt_fill((void *)(uintptr_t)0x005746c0, 0x00574d10 - 0x005746c0);
    gZoverRigs[0] = NULL;
    gZoverRigs[1] = NULL;
    rand_vec((ZVec3 *)(uintptr_t)0x00574cf8, 1.0f);

    dt_fill(limbs, sizeof *limbs);
    limbs->mask = dt_u32() % 8 == 0 ? dt_u32() : (dt_u32() & 15);
    for (int i = 0; i < 8; i++) {
        limbs->length[i] = rf(8.0f, 30.0f);
    }
    for (int k = 0; k < 4; k++) {
        const float sum = limbs->length[kLimbLen[k][0]] + limbs->length[kLimbLen[k][1]];
        const ZVec3 *root = &partBase[kLimbRoot[k]];
        ZLimbTarget *t = &limbs->target[k];
        const uint32_t mode = dt_u32() % 16;

        limbs->reachSum[k] = k < 2 ? sum - 1.0f : sum;
        t->a = *root;
        if (mode != 0) { /* else on the root: (almost) no reach */
            ZVec3 d;

            rand_dir(&d, sum * rf(0.1f, mode == 1 ? 3.0f : 1.1f));
            add_scaled(&t->a, &d, 1.0f);
        }
        if (mode == 3 || mode == 4) {
            /* NaN in the limb data (a target or an upper bone length): NaN signs must survive the arms'
             * negation */
            const uint32_t nan = 0x7fc00000u | (dt_u32() & 0x803fffffu);

            memcpy(mode == 3 ? &t->a.y : &limbs->length[kLimbLen[k][0]], &nan, 4);
        }
        if (mode == 2) {
            /* bend direction along the reach */
            t->b.x = t->a.x - root->x;
            t->b.y = t->a.y - root->y;
            t->b.z = t->a.z - root->z;
        } else {
            rand_dir(&t->b, rf(0.2f, 2.0f));
        }
    }
    DT_ARG_PTR(c, 0, actor);
    DT_ARG_PTR(c, 1, limbs);
}

DT_CASES(zoverii_big_cases) = {
    {"zoverRigCollidePoint", 0x00451390, zoverRigCollidePoint, 3, DT_RET_INT, setup_collide, 600, 0},
    {"zoverRigResolvePoint", 0x00451d60, zoverRigResolvePoint, 1, DT_RET_VOID, setup_resolve, 600, 0},
    {"zoverStepRiderMotion", 0x004547c0, zoverStepRiderMotion, 2, DT_RET_VOID, setup_step_rider, 300, 0},
    {"zoverSolveLimbs", 0x00456120, zoverSolveLimbs, 2, DT_RET_VOID, setup_solve_limbs, 600, 0},
};
