/*
 * bmxphysUpdatePositionRoll needs a loaded level (collision, props, sound, effects, the state
 * machine). Those callees are replaced by scripted stubs: setup() patches a JMP at each original
 * entry point into the stubs below, which (while armed) log their arguments by value into DLL data
 * that the harness compares, and return scripted results. Disarmed, a stub forwards to a trampoline
 * holding the original prologue, so other test cases see the original functions unchanged. The
 * trampolines live in a non-writable section the harness does not snapshot, so they survive the
 * per-iteration memory restore just like the patched exe code.
 */
#include <stdarg.h>

#include "bmx/com/action/physics_roll.h"
#include "dt.h"
#include "zge/zdebug.h"
#include "zge/zquat.h"

/* ---- stubs ------------------------------------------------------------------------------------- */

enum {
    kStubSetState,
    kStubCollideLine,
    kStubQpEdge,
    kStubPropTouch,
    kStubPropHitFace,
    kStubBspGetInfo,
    kStubObjectValid,
    kStubBspObjectHit,
    kStubSkidMark,
    kStubSparks,
    kStubSound,
    kStubDrawLine,
    kStubCount,
};

#define MAX_RAYS 16
#define LOG_SIZE 1024

typedef struct RollRay {
    int32_t result;
    float frac; /* hit point = from + frac * (to - from) */
    PhysRayHit hit;
} RollRay;

typedef struct RollScript {
    RollRay ray[MAX_RAYS];
    uint32_t rayN;
    int32_t state[4]; /* physSetState results in call order; -1: the requested state */
    uint32_t stateN;
    PhysQpEdge *edge;
    int32_t propTouch;
    const uint8_t *bspInfo;
    int32_t objectValid;
} RollScript;

static int32_t armed;
static RollScript script;
static uint32_t logBuf[LOG_SIZE];
static uint32_t logN;

#pragma section(".dttramp", read, execute)
__declspec(allocate(".dttramp")) static uint8_t trampolines[kStubCount][16];

static void logU32(uint32_t v)
{
    if (logN < LOG_SIZE) {
        logBuf[logN] = v;
    }
    logN++;
}

static void logF(float f)
{
    uint32_t u;
    __builtin_memcpy(&u, &f, 4);
    logU32(u);
}

static void logVec(const ZVec3 *v)
{
    logF(v->x);
    logF(v->y);
    logF(v->z);
}

#define TRAMP(id, type) ((type)(void *)trampolines[id])

static int stubSetState(PhysRider *r, int state)
{
    int32_t res;
    if (!armed) {
        return TRAMP(kStubSetState, int (*)(PhysRider *, int))(r, state);
    }
    logU32(0x5e7a7e00u + (uint32_t)state);
    res = script.stateN < 4 ? script.state[script.stateN] : -1;
    script.stateN++;
    if (res < 0) {
        res = state;
    }
    r->state = (uint8_t)res;
    return res;
}

static int stubCollideLine(const ZVec3 *from, const ZVec3 *to, PhysRayHit *hit)
{
    const RollRay *ray;
    if (!armed) {
        return TRAMP(kStubCollideLine, int (*)(const ZVec3 *, const ZVec3 *, PhysRayHit *))(from, to, hit);
    }
    logU32(0xc0111de0u);
    logVec(from);
    logVec(to);
    if (script.rayN >= MAX_RAYS) {
        return 0;
    }
    ray = &script.ray[script.rayN++];
    *hit = ray->hit;
    hit->point.x = from->x + ray->frac * (to->x - from->x);
    hit->point.y = from->y + ray->frac * (to->y - from->y);
    hit->point.z = from->z + ray->frac * (to->z - from->z);
    return ray->result;
}

static PhysQpEdge *stubQpEdge(int section, int edgeId, const ZVec3 *pos)
{
    if (!armed) {
        return TRAMP(kStubQpEdge, PhysQpEdge * (*)(int, int, const ZVec3 *))(section, edgeId, pos);
    }
    logU32(0x0e06e000u);
    logU32((uint32_t)section);
    logU32((uint32_t)edgeId);
    logVec(pos);
    return script.edge;
}

static int stubPropTouch(int player, int propId)
{
    if (!armed) {
        return TRAMP(kStubPropTouch, int (*)(int, int))(player, propId);
    }
    logU32(0x9709e000u);
    logU32((uint32_t)player);
    logU32((uint32_t)propId);
    return script.propTouch;
}

static void stubPropHitFace(int player, PhysFace *face)
{
    if (!armed) {
        TRAMP(kStubPropHitFace, void (*)(int, PhysFace *))(player, face);
        return;
    }
    logU32(0x9709f000u);
    logU32((uint32_t)player);
    logU32((uint32_t)(uintptr_t)face);
}

static const uint8_t *stubBspGetInfo(int object)
{
    if (!armed) {
        return TRAMP(kStubBspGetInfo, const uint8_t *(*)(int))(object);
    }
    logU32(0xb5910000u);
    logU32((uint32_t)object);
    return script.bspInfo;
}

static int stubObjectValid(int object)
{
    if (!armed) {
        return TRAMP(kStubObjectValid, int (*)(int))(object);
    }
    logU32(0xb5920000u);
    logU32((uint32_t)object);
    return script.objectValid;
}

static void stubBspObjectHit(PhysCollObject *object, int a)
{
    if (!armed) {
        TRAMP(kStubBspObjectHit, void (*)(PhysCollObject *, int))(object, a);
        return;
    }
    logU32(0xb5930000u);
    logU32((uint32_t)(uintptr_t)object);
    logU32((uint32_t)a);
}

static void stubSkidMark(const ZVec3 *from, const ZVec3 *to, int flags, const ZVec3 *normal)
{
    if (!armed) {
        TRAMP(kStubSkidMark, void (*)(const ZVec3 *, const ZVec3 *, int, const ZVec3 *))(from, to, flags,
                                                                                         normal);
        return;
    }
    logU32(0x5c1d0000u);
    logVec(from);
    logVec(to);
    logU32((uint32_t)flags);
    logVec(normal);
}

static void stubSparks(const ZVec3 *pos, const ZVec3 *vel)
{
    if (!armed) {
        TRAMP(kStubSparks, void (*)(const ZVec3 *, const ZVec3 *))(pos, vel);
        return;
    }
    logU32(0x59a4c000u);
    logVec(pos);
    logVec(vel);
}

static int stubSound(int id, const void *position, int a, int volume, int b)
{
    if (!armed) {
        return TRAMP(kStubSound, int (*)(int, const void *, int, int, int))(id, position, a, volume, b);
    }
    logU32(0x50d00000u);
    logU32((uint32_t)id);
    logVec(position);
    logU32((uint32_t)a);
    logU32((uint32_t)volume);
    logU32((uint32_t)b);
    return 0;
}

static void stubDrawLine(int mode, const PhysDebugVertex *a, const PhysDebugVertex *b)
{
    if (!armed) {
        TRAMP(kStubDrawLine, void (*)(int, const PhysDebugVertex *, const PhysDebugVertex *))(mode, a, b);
        return;
    }
    logU32(0xd7a10000u + (uint32_t)mode);
    logVec(&a->pos);
    logU32((uint32_t)a->rgb[0] | (uint32_t)a->rgb[1] << 8 | (uint32_t)a->rgb[2] << 16);
    logVec(&b->pos);
    logU32((uint32_t)b->rgb[0] | (uint32_t)b->rgb[1] << 8 | (uint32_t)b->rgb[2] << 16);
}

/* Replaces the debug print pointer: logs the format text (by value) and two int arguments. */
static void stubPrint(const char *fmt, ...)
{
    va_list ap;
    uint32_t h = 2166136261u;
    unsigned args = 0;
    for (const char *p = fmt; *p; p++) {
        h = (h ^ (uint8_t)*p) * 16777619u;
        args += *p == '%';
    }
    logU32(0xdeb90000u);
    logU32(h);
    va_start(ap, fmt);
    while (args--) { /* only %d conversions occur here */
        logU32((uint32_t)va_arg(ap, int));
    }
    va_end(ap);
}

typedef struct StubPatch {
    uint32_t addr;
    uint32_t prologue; /* bytes of whole instructions copied to the trampoline */
    void *stub;
} StubPatch;

static const StubPatch patches[kStubCount] = {
    [kStubSetState] = {0x00420cb0, 7, (void *)stubSetState},
    [kStubCollideLine] = {0x00469b00, 6, (void *)stubCollideLine},
    [kStubQpEdge] = {0x0042d950, 7, (void *)stubQpEdge},
    [kStubPropTouch] = {0x0042eec0, 7, (void *)stubPropTouch},
    [kStubPropHitFace] = {0x0042ef60, 5, (void *)stubPropHitFace},
    [kStubBspGetInfo] = {0x004665c0, 9, (void *)stubBspGetInfo},
    [kStubObjectValid] = {0x00469530, 5, (void *)stubObjectValid},
    [kStubBspObjectHit] = {0x00466610, 5, (void *)stubBspObjectHit},
    [kStubSkidMark] = {0x00409250, 6, (void *)stubSkidMark},
    [kStubSparks] = {0x00408c90, 8, (void *)stubSparks},
    [kStubSound] = {0x0044e300, 5, (void *)stubSound},
    [kStubDrawLine] = {0x00463710, 5, (void *)stubDrawLine},
};

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* Installs the JMP patches once per process (exe code is not part of the restored snapshot). */
static void install_stubs(void)
{
    for (unsigned i = 0; i < kStubCount; i++) {
        uint8_t *code = (uint8_t *)(uintptr_t)patches[i].addr;
        volatile uint8_t *t = trampolines[i];
        const uint32_t rel = (uint32_t)(uintptr_t)patches[i].stub - (patches[i].addr + 5);
        if (code[0] == 0xe9 && code[1] == (uint8_t)rel && code[2] == (uint8_t)(rel >> 8) &&
            code[3] == (uint8_t)(rel >> 16) && code[4] == (uint8_t)(rel >> 24)) {
            continue;
        }
        for (uint32_t k = 0; k < patches[i].prologue; k++) {
            t[k] = code[k];
        }
        t[patches[i].prologue] = 0x68; /* push imm32 ; ret */
        put32((uint8_t *)&t[patches[i].prologue + 1], patches[i].addr + patches[i].prologue);
        t[patches[i].prologue + 5] = 0xc3;
        code[0] = 0xe9;
        put32(code + 1, rel);
    }
}

/* ---- state ------------------------------------------------------------------------------------- */

static ZVec3 rand_vec(float range)
{
    ZVec3 v = {dt_float_edgy(-range, range), dt_float(-range, range), dt_float(-range, range)};
    return v;
}

static ZVec3 rand_unit(void)
{
    ZVec3 v;
    float l;
    do {
        v = rand_vec(1.0f);
        l = v.x * v.x + v.y * v.y + v.z * v.z;
    } while (l < 0.05f);
    l = __builtin_sqrtf(l);
    v.x /= l;
    v.y /= l;
    v.z /= l;
    return v;
}

static ZVec3 scaled(const ZVec3 *v, float s)
{
    ZVec3 o = {v->x * s, v->y * s, v->z * s};
    return o;
}

/* Bike basis: mostly upright with random yaw, sometimes fully random. */
static void rand_basis(ZMatrix *m, int tilt)
{
    ZQuat q;
    float l;
    q.x = dt_float(-0.2f, 0.2f) * (float)tilt;
    q.y = dt_float(-1.0f, 1.0f);
    q.z = dt_float(-0.2f, 0.2f) * (float)tilt;
    q.w = dt_float(-1.0f, 1.0f);
    l = __builtin_sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (l < 0.1f) {
        q.w = 1.0f;
        l = __builtin_sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + 1.0f);
    }
    q.x /= l;
    q.y /= l;
    q.z /= l;
    q.w /= l;
    zquatToMatrix(m, &q);
}

typedef enum Scenario {
    kScGround,  /* main line tests mostly hit level ground */
    kScWalls,   /* side probes find walls */
    kScAir,     /* most line tests miss */
    kScSurface, /* front wheel picks up special faces */
    kScSteer,   /* steering locks with one wheel airborne */
    kScDegenerate,
} Scenario;

static void *alloc_contact(void)
{
    return dt_alloc(512);
}

/* Normal for a scripted hit: ground-like, wall-like or random. */
static ZVec3 rand_normal(const ZMatrix *b, int wallBias)
{
    const uint32_t k = dt_u32() % 10;
    ZVec3 n;
    if ((int)k < wallBias) {
        const ZVec3 *axis = (dt_u32() & 1) ? &b->right : &b->at;
        n = scaled(axis, (dt_u32() & 1) ? 1.0f : -1.0f);
        if (dt_u32() & 1) {
            n.y = 0.0f;
        }
    } else if (k < 8) {
        n = b->up;
        n.x += dt_float(-0.3f, 0.3f);
        n.z += dt_float(-0.3f, 0.3f);
        if (dt_u32() % 3 == 0) {
            n.x = 0.0f;
            n.y = 1.0f;
            n.z = 0.0f;
        }
    } else {
        n = rand_unit();
    }
    return n;
}

static void rand_hit(PhysRayHit *h, const ZMatrix *b, int wallBias, int faceBias)
{
    static const uint16_t faceTypes[] = {0, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 15};
    uint32_t *mat;
    uint16_t type;

    switch (dt_u32() % 16) {
    case 0:
    case 1:
        h->kind = 2;
        break;
    case 2:
        h->kind = 3;
        break;
    case 3:
        h->kind = 4; /* neither a face nor an object */
        break;
    default:
        h->kind = 1;
        break;
    }
    h->id = dt_range(0, 40);
    h->normal = rand_normal(b, wallBias);
    h->normalW = dt_float(-1.0f, 1.0f);
    h->face = alloc_contact();
    type = faceBias ? faceTypes[dt_u32() % (sizeof faceTypes / sizeof faceTypes[0])] : (dt_u32() % 6 ? 0 : 9);
    if (dt_u32() % 12 == 0) {
        type = 9; /* prop face */
    }
    h->face->info = (uint16_t)(type << 12 | (dt_u32() & 0xfff));
    if (h->kind != 1 || dt_u32() % 5) { /* object hits always carry a material (hazards read it) */
        mat = alloc_contact();
        *mat = dt_u32() & 0x18000f;
        h->material = mat;
    }
    h->object = dt_range(1, 1000);
    h->objectData = alloc_contact();
    h->objectData->propId = dt_u32() % 3 ? 0 : dt_range(1, 99);
}

static void setup_roll(DtCtx *c, Scenario sc)
{
    PhysRider *r;
    uint8_t *info;
    int hitPct;

    dt_seed(c);
    install_stubs();
    armed = 1;
    logN = 0;
    gZdebugPrint = (ZDebugPrintFn)stubPrint;
    gPhysDebugDraw = (int32_t)(dt_u32() & 1);

    r = dt_alloc(sizeof *r);
    r->player = (uint8_t)dt_range(0, 3);
    r->stateLevel[0] = (uint8_t)dt_range(0, 2);
    r->stateLevel[1] = (uint8_t)(dt_u32() % 3 ? 2 : dt_range(0, 3));
    r->flags = dt_u32() & 0xfe4000u;
    if (sc == kScSteer) {
        r->flags |= 0x100000u << (dt_u32() & 1);
    }
    r->fieldBA4 = dt_u32() & 8;
    r->state = (uint8_t)dt_range(0, 6);
    r->surface = (uint8_t)(dt_u32() % 3 ? 0 : dt_range(1, 3));
    r->field6E0 = dt_u32() % 4 ? dt_float(0.2f, 2.0f) : dt_float(0.0f, 0.1f);
    r->impactTotal = dt_float(0.0f, 50.0f);
    r->field58 = dt_range(0, 1000);
    rand_basis(&r->basis, dt_u32() % 4 ? 1 : 5);
    r->center = rand_vec(200.0f);
    __builtin_memcpy(&r->fwd, &r->basis, sizeof(ZMatrix));
    r->vel = rand_vec(4.0f);
    r->dvel = rand_vec(1.0f);
    r->rot.x = (ZAngle)dt_u32();
    switch (dt_u32() % 6) {
    case 0:
        r->rot.y = 0;
        break;
    case 1:
        r->rot.y = (ZAngle)dt_u32();
        break;
    case 2:
    case 3:
        /* Multiples of 20: rot.y * 0.15f lands next to an integer before truncation. */
        r->rot.y = (ZAngle)(dt_range(-150, 150) * 20);
        break;
    default:
        r->rot.y = (ZAngle)dt_range(-3000, 3000);
        break;
    }
    r->rot.z = (ZAngle)dt_range(-4000, 4000);
    r->speed = dt_float(0.0f, 40.0f);
    r->actor = alloc_contact();
    r->bikeObj = alloc_contact();
    r->fieldC60 = dt_float(-0.5f, 0.5f);
    r->fieldC64 = dt_float(0.0f, 0.1f);
    r->fieldC68 = dt_float(0.0f, 10.0f);
    r->fieldC6C = dt_float(0.5f, 5.0f);
    r->fieldC70 = dt_float(0.5f, 5.0f);
    r->travelMin = -dt_float(0.0f, 3.0f);
    r->travelMax = dt_float(0.0f, 3.0f);
    r->fieldC7C = dt_float(0.0f, 1.0f);
    r->fieldC80 = dt_float(0.0f, 1.0f);
    r->fieldC84 = dt_float(0.0f, 1.0f);
    for (unsigned k = 0; k < 2; k++) {
        PhysWheel *wh = &r->wheel[k];
        wh->anchor.x = k == 0 ? dt_float(12.0f, 25.0f) : -dt_float(12.0f, 25.0f);
        wh->anchor.y = dt_float(-3.0f, 3.0f);
        wh->anchor.z = dt_float(-1.0f, 1.0f);
        wh->radius = dt_float(4.0f, 10.0f);
        wh->vel = rand_vec(dt_u32() & 1 ? 1.0f : (sc == kScWalls && dt_u32() % 3 == 0 ? 90.0f : 6.0f));
        wh->speed = dt_u32() % 5 ? dt_float(0.0f, 30.0f) : dt_float(30.0f, 300.0f);
        wh->dir = r->basis.right;
        wh->normal = dt_u32() % 4 ? r->basis.up : rand_unit();
        wh->side = r->basis.at;
        wh->mode = (uint8_t)(dt_u32() % 4 == 0 ? 2 : dt_range(0, 1));
        wh->travel = dt_float(r->travelMin - 1.0f, r->travelMax + 1.0f);
        wh->travelVel = dt_float(-2.0f, 2.0f);
        wh->spin = dt_range(-30000, 30000);
        wh->angle = (int16_t)dt_u32();
        wh->material = dt_range(0, 11);
    }
    if (sc == kScSteer) {
        r->wheel[dt_u32() & 1].mode = 2;
    }

    /* Scripted callee results. */
    script.edge = dt_u32() % 4 ? alloc_contact() : NULL;
    if (script.edge) {
        script.edge->dir = dt_u32() & 1 ? r->basis.right : rand_unit();
    }
    hitPct = sc == kScAir ? 15 : (sc == kScGround || sc == kScSurface ? 75 : 50);
    for (unsigned k = 0; k < MAX_RAYS; k++) {
        RollRay *ray = &script.ray[k];
        ray->result = dt_range(0, 99) < hitPct;
        ray->frac = dt_u32() % 8 ? dt_float(0.0f, 1.0f) : dt_float(-0.5f, 1.5f);
        rand_hit(&ray->hit, &r->basis, sc == kScWalls ? 6 : 1, sc == kScSurface);
    }
    if (sc == kScWalls) {
        /* Head-on walls against the wheel velocity. */
        for (unsigned k = 0; k < 4; k++) {
            if (dt_u32() & 1) {
                PhysWheel *wh = &r->wheel[dt_u32() & 1];
                const float l =
                    __builtin_sqrtf(wh->vel.x * wh->vel.x + wh->vel.y * wh->vel.y + wh->vel.z * wh->vel.z);
                if (l > 0.01f) {
                    script.ray[k].hit.normal = scaled(&wh->vel, -1.0f / l);
                }
            }
        }
    }
    if (sc == kScSteer && dt_u32() % 3 == 0) {
        /* Ceiling-like faces under the rolling wheel: the slope test's second branch. */
        r->vel.y = -__builtin_fabsf(r->vel.y) - 0.5f;
        for (unsigned k = 0; k < 2; k++) {
            r->wheel[k].normal.x = dt_float(-0.2f, 0.2f);
            r->wheel[k].normal.y = -1.0f;
            r->wheel[k].normal.z = dt_float(-0.2f, 0.2f);
        }
        for (unsigned k = 0; k < MAX_RAYS; k++) {
            script.ray[k].hit.normal = r->wheel[k & 1].normal;
        }
    }
    if (sc == kScSurface && dt_u32() % 3 == 0) {
        /* Upside-down rider frame lined up with the quarterpipe edge: the edge sound. */
        r->up.x = 0.0f;
        r->up.y = -1.0f;
        r->up.z = 0.0f;
        if (script.edge) {
            script.edge->dir = r->fwd;
        }
    }
    if (sc == kScGround || sc == kScSurface) {
        /* The final crash test between the wheels mostly misses. */
        script.ray[MAX_RAYS - 1].result = 0;
    }
    script.rayN = 0;
    for (unsigned k = 0; k < 4; k++) {
        script.state[k] = dt_u32() % 3 ? -1 : (int32_t)(dt_u32() % 3 == 0 ? 6 : dt_range(1, 5));
    }
    script.stateN = 0;
    script.propTouch = (int32_t)(dt_u32() & 1);
    info = alloc_contact();
    info[2] = (uint8_t)dt_range(0, 11);
    script.bspInfo = info;
    script.objectValid = (int32_t)(dt_u32() & 1);

    if (sc == kScDegenerate) {
        /* Identical wheels: the X axis assert, or (stacked along up) the Z axis assert. */
        r->wheel[1] = r->wheel[0];
        if (dt_u32() & 1) {
            __builtin_memset(&r->basis, 0, sizeof r->basis);
            r->basis.right.x = 1.0f;
            r->basis.up.y = 1.0f;
            r->basis.at.z = 1.0f;
            r->wheel[1].anchor.y += 5.0f;
        }
        r->flags &= ~0x300000u;
        for (unsigned k = 0; k < MAX_RAYS; k++) {
            script.ray[k].result = 0;
        }
        if (c->seed % 3 == 2) {
            /* Non-finite ground normals. */
            r->wheel[1].anchor.x -= 30.0f;
            for (unsigned k = 0; k < MAX_RAYS - 1; k++) {
                script.ray[k].result = 1;
                script.ray[k].hit.normal.x = __builtin_inff();
            }
        }
    }
    DT_ARG_PTR(c, 0, r);
}

static void setup_ground(DtCtx *c)
{
    setup_roll(c, kScGround);
}

static void setup_walls(DtCtx *c)
{
    setup_roll(c, kScWalls);
}

static void setup_air(DtCtx *c)
{
    setup_roll(c, kScAir);
}

static void setup_surface(DtCtx *c)
{
    setup_roll(c, kScSurface);
}

static void setup_steer(DtCtx *c)
{
    setup_roll(c, kScSteer);
}

static void setup_degenerate(DtCtx *c)
{
    setup_roll(c, kScDegenerate);
}

DT_CASES(physics_roll_cases) = {
    {"bmxphysUpdatePositionRoll/ground", 0x00425940, bmxphysUpdatePositionRoll, 1, DT_RET_VOID, setup_ground,
     600, 0},
    {"bmxphysUpdatePositionRoll/walls", 0x00425940, bmxphysUpdatePositionRoll, 1, DT_RET_VOID, setup_walls,
     600, 0},
    {"bmxphysUpdatePositionRoll/air", 0x00425940, bmxphysUpdatePositionRoll, 1, DT_RET_VOID, setup_air, 300,
     0},
    {"bmxphysUpdatePositionRoll/surface", 0x00425940, bmxphysUpdatePositionRoll, 1, DT_RET_VOID,
     setup_surface, 600, 0},
    {"bmxphysUpdatePositionRoll/steer", 0x00425940, bmxphysUpdatePositionRoll, 1, DT_RET_VOID, setup_steer,
     1000, 0},
    {"bmxphysUpdatePositionRoll/degenerate", 0x00425940, bmxphysUpdatePositionRoll, 1, DT_RET_VOID,
     setup_degenerate, 40, 0},
};
