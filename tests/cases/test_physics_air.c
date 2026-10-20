/*
 * Tests for the airborne update and the actor pose sync (physics_air.c).
 *
 * Both functions call deep engine code (world raycasts, sound, actor skeletons and IK) that needs a
 * loaded level. Those callees are replaced per iteration with dt_stub() by stubs that log their
 * arguments and answer from the test's random stream; ported callees are stubbed on both sides. The
 * log is compared like any other memory.
 */
#include "bmx/com/action/physics_air.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_zactor.h"
#include "zge/zdebug.h"
#include "zge/zquat.h"

/* ---- call log ------------------------------------------------------------------------------------ */

static uint32_t gLog[4096];
static uint32_t gLogN;

static void logU(uint32_t v)
{
    if (gLogN < sizeof gLog / sizeof gLog[0]) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static void logF(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    logU(u);
}

static void logV(const ZVec3 *v)
{
    logF(v->x);
    logF(v->y);
    logF(v->z);
}

static void logBytes(const void *p, uint32_t n)
{
    const uint8_t *b = p;
    for (uint32_t i = 0; i < n; i += 4) {
        uint32_t u = 0;
        memcpy(&u, b + i, n - i < 4 ? n - i : 4);
        logU(u);
    }
}

/* ---- world fixtures ------------------------------------------------------------------------------ */

enum { kFaces = 6, kObjects = 3 };

typedef struct World {
    uint8_t *face[kFaces];
    uint8_t *object[kObjects];
    uint8_t *info;    /* game object descriptor (0x42eec0 argument) */
    uint8_t *objInfo; /* 0x4665c0 result */
    uint32_t *material[3];
    uint8_t *edge;
    int missPct;   /* raycast miss chance */
    int flatPct;   /* chance of a normal with y == 0 */
    int deadlyPct; /* chance of material 6 */
    int kindBias;  /* face kind to favour, or -1 */
    float noise;   /* normal noise */
    int exactUp;   /* every hit normal is exactly (0, 1, 0) */
    ZVec3 partTab[16];
} World;

static World gWorld;

static uint8_t *newFace(unsigned kind)
{
    uint8_t *f = dt_alloc(512);
    for (int i = 0; i < 512; i++) {
        f[i] = (uint8_t)dt_u32();
    }
    *(uint16_t *)(f + 0x16) = (uint16_t)(kind << 12 | (dt_u32() & 0xfff));
    return f;
}

static void worldInit(void)
{
    static const unsigned kinds[kFaces] = {1, 9, 10, 2, 5, 0};
    for (int i = 0; i < kFaces; i++) {
        gWorld.face[i] = newFace(kinds[i]);
    }
    gWorld.info = dt_alloc(16);
    gWorld.objInfo = dt_alloc(16);
    gWorld.objInfo[2] = (uint8_t)dt_range(0, 9);
    for (int i = 0; i < kObjects; i++) {
        uint8_t *o = newFace(0);
        *(void **)(o + 0x1ec) = (i == 0) ? NULL : gWorld.info;
        *(void **)(o + 0x1f0) = dt_alloc(64);
        gWorld.object[i] = o;
    }
    for (int i = 0; i < 3; i++) {
        gWorld.material[i] = dt_alloc(4);
        *gWorld.material[i] = dt_u32();
    }
    gWorld.edge = newFace(1);
    gWorld.missPct = dt_range(0, 60);
    gWorld.flatPct = dt_range(0, 8);
    gWorld.deadlyPct = dt_range(0, 6);
    gWorld.kindBias = (dt_u32() & 1) ? dt_range(0, 11) : -1;
    gWorld.noise = dt_float(0.0f, 0.8f);
    for (int i = 0; i < 16; i++) {
        gWorld.partTab[i].x = dt_float(-50.0f, 50.0f);
        gWorld.partTab[i].y = dt_float(-50.0f, 50.0f);
        gWorld.partTab[i].z = dt_float(-50.0f, 50.0f);
    }
}

static int pct(int p)
{
    return (int)(dt_u32() % 100) < p;
}

/* ---- stubs --------------------------------------------------------------------------------------- */

static int BMX_CDECL stubRaycast(const ZVec3 *from, const ZVec3 *to, PhysRayHit *hit)
{
    ZVec3 d;
    float len;
    float frac;

    logU(0x469b00);
    logV(from);
    logV(to);
    if (pct(gWorld.missPct)) {
        return 0;
    }
    memset(hit, 0, sizeof *hit);
    frac = dt_float(0.0f, 1.0f);
    hit->point.x = from->x + (to->x - from->x) * frac;
    hit->point.y = from->y + (to->y - from->y) * frac;
    hit->point.z = from->z + (to->z - from->z) * frac;
    d.x = from->x - to->x + dt_float(-1.0f, 1.0f) * gWorld.noise * 30.0f;
    d.y = from->y - to->y + dt_float(-1.0f, 1.0f) * gWorld.noise * 30.0f;
    d.z = from->z - to->z + dt_float(-1.0f, 1.0f) * gWorld.noise * 30.0f;
    if (pct(gWorld.flatPct)) {
        d.y = 0.0f;
    }
    len = __builtin_sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len < 1e-3f) {
        d.x = 0.0f;
        d.y = 1.0f;
        d.z = 0.0f;
        len = 1.0f;
    }
    hit->normal.x = d.x / len;
    hit->normal.y = d.y / len;
    hit->normal.z = d.z / len;
    if (gWorld.exactUp) {
        hit->normal.x = 0.0f;
        hit->normal.y = 1.0f;
        hit->normal.z = 0.0f;
    }
    hit->normalW = dt_float(-100.0f, 100.0f);
    hit->id = (int32_t)dt_u32();
    hit->object = (int32_t)(dt_u32() & 0xffff);
    switch (dt_u32() % 8) {
    case 0:
        hit->flags = 2;
        break;
    case 1:
        hit->flags = 0;
        break;
    case 2:
        hit->flags = 3;
        break;
    default:
        hit->flags = 1;
        break;
    }
    hit->flags |= (uint8_t)(dt_u32() & 0xf0);
    if (gWorld.kindBias >= 0 && (dt_u32() & 1)) {
        uint8_t *f = gWorld.face[dt_u32() % kFaces];
        *(uint16_t *)(f + 0x16) = (uint16_t)(gWorld.kindBias << 12 | (dt_u32() & 0xfff));
        hit->face = (PhysFace *)f;
    } else {
        hit->face = (dt_u32() % 16) == 0 ? NULL : (PhysFace *)gWorld.face[dt_u32() % kFaces];
    }
    if (hit->face == NULL) {
        hit->flags &= (uint8_t)~1u;
    }
    hit->material = (dt_u32() % 5) == 0 ? NULL : gWorld.material[dt_u32() % 3];
    if (hit->material != NULL) {
        *(uint32_t *)(uintptr_t)hit->material = pct(gWorld.deadlyPct) ? 6 : dt_u32();
    }
    hit->objectData = (dt_u32() % 6) == 0 ? NULL : (PhysCollObject *)gWorld.object[dt_u32() % kObjects];
    if (hit->objectData == NULL) {
        hit->flags &= (uint8_t)~2u; /* both functions dereference the object of an object hit */
    }
    return 1 + (int)(dt_u32() & 0xff);
}

static int BMX_CDECL stubContactFromHit(PhysRider *r, PhysRayHit *hit)
{
    logU(0x42dad0);
    logU((uint32_t)(uintptr_t)r);
    logBytes(hit, sizeof *hit);
    return (dt_u32() % 3) != 0;
}

static void *BMX_CDECL stubFindEdge(int faceIndex, int id, const ZVec3 *pos)
{
    logU(0x42d950);
    logU((uint32_t)faceIndex);
    logU((uint32_t)id);
    logV(pos);
    return (dt_u32() & 1) ? gWorld.edge : NULL;
}

static void BMX_CDECL stubFaceTrigger(int player, void *face)
{
    logU(0x42ef60);
    logU((uint32_t)player);
    logU((uint32_t)(uintptr_t)face);
}

static int BMX_CDECL stubObjectTouch(int player, void *info)
{
    logU(0x42eec0);
    logU((uint32_t)player);
    logU((uint32_t)(uintptr_t)info);
    return (int)(dt_u32() & 1);
}

static uint8_t *BMX_CDECL stubObjectInfo(int id)
{
    logU(0x4665c0);
    logU((uint32_t)id);
    gWorld.objInfo[2] = pct(gWorld.deadlyPct) ? 6 : (uint8_t)dt_range(0, 10);
    return gWorld.objInfo;
}

static void *BMX_CDECL stubObjectState(int id)
{
    logU(0x469530);
    logU((uint32_t)id);
    return (dt_u32() & 3) ? gWorld.info : NULL;
}

static int BMX_CDECL stubSetState(PhysRider *r, int state)
{
    logU(0x420cb0);
    logU((uint32_t)(uintptr_t)r);
    logU((uint32_t)state);
    return state;
}

static void BMX_CDECL stubDebugLine(int kind, const PhysDebugVertex *a, const PhysDebugVertex *b)
{
    logU(0x463710);
    logU((uint32_t)kind);
    logV(&a->pos);
    logBytes(a->rgb, 3);
    logV(&b->pos);
    logBytes(b->rgb, 3);
}

static int BMX_CDECL stubSound(int id, const void *position, int flags, int volume, int b)
{
    logU(0x44e300);
    logU((uint32_t)id);
    logV(position);
    logU((uint32_t)flags);
    logU((uint32_t)volume);
    logU((uint32_t)b);
    return 0;
}

static void BMX_CDECL stubPlaceActors(PhysRider *r)
{
    logU(0x42d050);
    logU((uint32_t)(uintptr_t)r);
}

static void BMX_CDECL stubPartPosition(void *actor, int part, const ZVec3 *offset, ZVec3 *out)
{
    const ZVec3 *base = &gWorld.partTab[part & 15];
    logU(0x464b50);
    logU((uint32_t)(uintptr_t)actor);
    logU((uint32_t)part);
    logV(offset);
    /* Some rotation of the offset: every component depends on it. */
    out->x = base->x + offset->x * 0.6f - offset->y * 0.8f;
    out->y = base->y + offset->x * 0.8f + offset->y * 0.6f - offset->z * 0.3f;
    out->z = base->z - offset->x * 0.7f + offset->z;
}

static void BMX_CDECL stubApplyIk(void *actor, void *ik)
{
    logU(0x456120);
    logU((uint32_t)(uintptr_t)actor);
    logU((uint32_t)(uintptr_t)ik);
}

/* Debug print: logs a checksum of the format and its integer arguments. */
static void BMX_CDECL stubPrint(const char *fmt, ...)
{
    va_list ap;
    uint32_t sum = 0;
    int args = 0;
    logU(0x480240);
    for (const char *p = fmt; *p; p++) {
        sum = sum * 31 + (uint8_t)*p;
        args += *p == '%';
    }
    logU(sum);
    va_start(ap, fmt);
    for (int i = 0; i < args; i++) {
        logU((uint32_t)va_arg(ap, int));
    }
    va_end(ap);
}

static void hooksOn(void)
{
    dt_stub(0x00469b00, NULL, (void *)stubRaycast);
    dt_stub(0x0042d950, (void *)physFindNearestTaggedEdge, (void *)stubFindEdge);
    dt_stub(0x0042dad0, (void *)physTakeContact, (void *)stubContactFromHit);
    dt_stub(0x0042ef60, NULL, (void *)stubFaceTrigger);
    dt_stub(0x0042eec0, NULL, (void *)stubObjectTouch);
    dt_stub(0x004665c0, (void *)zbspGetShape, (void *)stubObjectInfo);
    dt_stub(0x00469530, (void *)zbspGetShapeKind, (void *)stubObjectState);
    dt_stub(0x00420cb0, (void *)physSetState, (void *)stubSetState);
    dt_stub(0x00463710, NULL, (void *)stubDebugLine);
    dt_stub(0x0044e300, NULL, (void *)stubSound);
    dt_stub(0x0042d050, (void *)physPoseActorsGrind, (void *)stubPlaceActors);
    dt_stub(0x00464b50, NULL, (void *)stubPartPosition);
    dt_stub(0x00456120, NULL, (void *)stubApplyIk);
    gZdebugPrint = stubPrint;
    worldInit();
}

/* ---- rider fixture ------------------------------------------------------------------------------- */

static void randUnit(ZVec3 *v)
{
    float len;
    do {
        v->x = dt_float(-1.0f, 1.0f);
        v->y = dt_float(-1.0f, 1.0f);
        v->z = dt_float(-1.0f, 1.0f);
        len = v->x * v->x + v->y * v->y + v->z * v->z;
    } while (len < 0.05f || len > 1.0f);
    len = __builtin_sqrtf(len);
    v->x /= len;
    v->y /= len;
    v->z /= len;
}

static void randRotation(ZMatrix *m)
{
    ZQuat q;
    float len;
    do {
        q.x = dt_float(-1.0f, 1.0f);
        q.y = dt_float(-1.0f, 1.0f);
        q.z = dt_float(-1.0f, 1.0f);
        q.w = dt_float(-1.0f, 1.0f);
        len = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    } while (len < 0.05f);
    if (dt_u32() % 3 == 0) { /* upright, as in normal riding */
        q.x = q.z = 0.0f;
    }
    zquatToMatrix(m, &q);
}

static void randVec(ZVec3 *v, float r)
{
    v->x = dt_float(-r, r);
    v->y = dt_float(-r, r);
    v->z = dt_float(-r, r);
}

static PhysRider *randRider(void)
{
    PhysRider *r = dt_alloc(sizeof *r);
    ZMatrix rot;
    const float vmax = (dt_u32() & 3) ? 15.0f : 60.0f;

    randRotation(&rot);
    r->fwd = rot.right;
    r->up = rot.up;
    r->side = rot.at;
    randVec(&r->omega, (dt_u32() & 1) ? 0.0f : 5.0f);
    r->flags = dt_u32();
    r->player = (uint8_t)dt_range(0, 3);
    for (int i = 0; i < 4; i++) {
        ((uint8_t *)r)[0x3d + i] = (uint8_t)dt_range(0, 3);
    }
    r->stateTimer[1] = (int16_t)dt_u32();
    r->anim = (uint8_t)dt_u32();
    r->ikPose = (uint8_t)dt_range(0, 15);
    r->combo.count = (uint8_t)dt_range(0, 3);
    r->state = (uint8_t)dt_range(0, 6);
    r->timer = (dt_u32() & 1) ? 0 : (int16_t)dt_range(1, 20);
    r->yawTrim = (dt_u32() & 1) ? 0 : (int16_t)dt_u32();
    randVec(&r->pos, 1000.0f);
    randVec(&r->step, 10.0f);
    randVec(&r->vel, vmax);
    randVec(&r->dvel, 5.0f);
    r->speed = dt_float(0.0f, 60.0f);
    r->stat[4] = dt_float(0.0f, 1.2f);
    randVec(&r->fieldBFC, 30.0f);
    r->ikBlend = dt_float_edgy(-0.5f, 1.5f);
    r->center.x = r->pos.x + dt_float(-20.0f, 20.0f);
    r->center.y = r->pos.y + dt_float(-20.0f, 20.0f);
    r->center.z = r->pos.z + dt_float(-20.0f, 20.0f);
    if (dt_u32() % 6 == 0) {
        /* Huge position cancelled by the step: sums only come out right with the original grouping. */
        float *pos = &r->pos.x;
        float *step = &r->step.x;
        float *centre = &r->center.x;
        for (int i = 0; i < 3; i++) {
            pos[i] = ((dt_u32() & 1) ? 1.0f : -1.0f) * dt_float(1e7f, 1e9f);
            step[i] = -pos[i] + dt_float(-10.0f, 10.0f);
            centre[i] = pos[i] + dt_float(-200.0f, 200.0f);
        }
    }
    randRotation(&r->basis);
    r->fieldC6C = dt_float(0.5f, 4.0f);
    r->fieldC70 = dt_float(0.5f, 4.0f);
    r->travelMin = dt_float(-15.0f, -2.0f);
    r->travelMax = dt_float(2.0f, 15.0f);
    r->fieldC7C = dt_float(0.0f, 2.0f);
    r->fieldC80 = dt_float(0.0f, 1.0f);
    r->fieldC84 = dt_float(0.0f, 1.0f);
    for (int i = 0; i < 2; i++) {
        PhysWheel *w = &r->wheel[i];
        w->anchor.x = (i == 0 ? 1.0f : -1.0f) * dt_float(20.0f, 60.0f);
        w->anchor.y = dt_float(-10.0f, 25.0f);
        w->anchor.z = dt_float(-3.0f, 3.0f);
        w->radius = dt_float(15.0f, 40.0f);
        randVec(&w->vel, 10.0f);
        w->speed = dt_float(0.0f, 20.0f);
        randUnit(&w->dir);
        randUnit(&w->normal);
        if (dt_u32() & 1) {
            w->normal = r->up;
        }
        randUnit(&w->side);
        /*
         * Never airborne on entry: with one wheel landing on airborne-marked state and the other airborne,
         * the original reads an uninitialised up vector (see bmxphysUpdatePositionAir).
         */
        w->mode = (uint8_t)dt_range(0, 1);
        w->spin = (dt_u32() & 3) ? (int32_t)(int16_t)dt_u32() : (int32_t)dt_u32();
        w->travel = dt_float(-25.0f, 25.0f);
        w->travelVel = dt_float(-8.0f, 8.0f);
    }
    if (dt_u32() % 16 == 0) { /* identical wheel anchors: the X orientation assert */
        r->wheel[1].anchor = r->wheel[0].anchor;
    }
    r->actor = dt_rand_actor(kZactorTypePerson);
    r->bikeObj = dt_rand_actor(kZactorTypePerson);
    return r;
}

/* ---- cases --------------------------------------------------------------------------------------- */

static void setup_air(DtCtx *c)
{
    PhysRider *r;
    dt_seed(c);
    hooksOn();
    r = randRider();
    gPhysDebugDraw = (dt_u32() % 8 == 0);
    if (c->seed % 29 == 7) {
        /*
         * Axis-aligned rider, wheels one above the other, no up blending and both wheels touching while
         * moving away from the ground: the new forward axis comes out exactly parallel to up (Z assert).
         */
        static const ZVec3 ex = {1.0f, 0.0f, 0.0f}, ey = {0.0f, 1.0f, 0.0f}, ez = {0.0f, 0.0f, 1.0f};
        r->fwd = ex;
        r->up = ey;
        r->side = ez;
        r->yawTrim = 0;
        r->wheel[1].anchor = r->wheel[0].anchor;
        r->wheel[1].anchor.y += 20.0f;
        r->vel = ey;
        r->vel.y = 30.0f;
        for (int i = 0; i < 4; i++) {
            ((uint8_t *)r)[0x3d + i] = 0;
        }
        gPhysLandUpBlend = 0.0f;
        gWorld.exactUp = 1;
        gWorld.missPct = 0;
        gWorld.flatPct = 0;
    }
    DT_ARG_PTR(c, 0, r);
}

static void setup_pose(DtCtx *c)
{
    PhysRider *r;
    dt_seed(c);
    hooksOn();
    r = randRider();
    gPhysForceStep = (int32_t)(dt_u32() & 1);
    if (dt_u32() & 1) {
        r->flags |= 0x400;
    }
    gPhysIkLimbMasks[r->ikPose] = (dt_u32() & 7) ? (dt_u32() & 0x7ffff) | (dt_u32() & 0x80000000u) : 0;
    DT_ARG_PTR(c, 0, r);
}

DT_CASES(physics_air_cases) = {
    {"bmxphysUpdatePositionAir", 0x00428df0, bmxphysUpdatePositionAir, 1, DT_RET_VOID, setup_air, 600, 0},
    {"bmxphysUpdatePose", 0x0042ba50, bmxphysUpdatePose, 1, DT_RET_VOID, setup_pose, 400, 0},
};
