/*
 * The large rider.c functions: riderUpdateAll (0x431000), riderStepAir (0x4322b0), riderStepBail
 * (0x433480) and the air trick sub state riderRollSub7 (0x434760). Pad, game and debug callees use the
 * shared game fixtures (dt_game.c); physics, actor, motion, rig and effect callees are replaced by the
 * stubs below, which log their arguments by value into rbLog and return scripted results.
 */
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/fx.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/rider.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/com/zge/zactor.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_game.h"
#include "zge/com/zglrw/zrwmat.h"

/* ---- call log ------------------------------------------------------------------------------------- */

enum { kLogSize = 512 };

__declspec(dllexport) uint32_t rbLog[kLogSize];
__declspec(dllexport) uint32_t rbLogN;

static void lg(uint32_t v)
{
    if (rbLogN < kLogSize) {
        rbLog[rbLogN] = v;
    }
    rbLogN++;
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

/* ---- scripted answers ----------------------------------------------------------------------------- */

static struct {
    uint32_t physState;
    uint32_t physSetResult; /* 0: the requested state */
    uint32_t probeHit;
    float probeY;
    uint32_t hands;
    float height;
    ZMotionDelta *played;
    ZMotionDelta **blended;
    uint32_t stepScript;
    uint32_t stepCalls;
    uint32_t stepModes; /* 3 bits per call: the mode a step switches to (7: none) */
} gRb;

static uint32_t pick(uint32_t n)
{
    return dt_u32() % n;
}

static float flt(float lo, float hi)
{
    if ((dt_u32() & 31) == 0) {
        return __builtin_nanf("");
    }
    return (dt_u32() & 7) == 0 ? dt_float_edgy(lo, hi) : dt_float(lo, hi);
}

/* ---- stubs ---------------------------------------------------------------------------------------- */

static uint32_t BMX_CDECL stubGetState(const PhysRider *r)
{
    lg(0x420c9000u);
    lgp(r);
    return gRb.physState;
}

static uint32_t BMX_CDECL stubSetState(PhysRider *r, int state)
{
    lg(0x420cb000u);
    lgp(r);
    lg((uint32_t)state);
    return gRb.physSetResult != 0 ? gRb.physSetResult - 1 : (uint32_t)state;
}

static void BMX_CDECL stubRespawn(PhysRider *r)
{
    lg(0x41fe1000u);
    lgp(r);
}

static int BMX_CDECL stubProbe(const ZVec3 *p, PhysGroundHit *out)
{
    lg(0x420d7000u);
    lgp(p);
    out->pos.y = gRb.probeY;
    return (int)gRb.probeHit;
}

static const float *gBounds;

static const float *BMX_CDECL stubBounds(void)
{
    lg(0x46ceb000u);
    return gBounds;
}

static ZMotionDelta *BMX_CDECL stubPlay(ZActor *a, const ZMotion *m, float speed, uint32_t flags)
{
    lg(0x4740c000u);
    lgp(a);
    lgp(m);
    lgf(speed);
    lg(flags);
    return gRb.played;
}

static ZMotionDelta **BMX_CDECL stubBlend(ZActor *a, const ZMotion *ma, const ZMotion *mb,
                                          const uint16_t *flags, float t)
{
    lg(0x476ae000u);
    lgp(a);
    lgp(ma);
    lgp(mb);
    lg(flags[0] | (uint32_t)flags[1] << 16);
    lgf(t);
    return gRb.blended;
}

static float BMX_CDECL stubHeight(const ZActor *a)
{
    lg(0x47628000u);
    lgp(a);
    return gRb.height;
}

static void BMX_CDECL stubCopyJoints(const ZActor *a, const ZActor *b)
{
    lg(0x47496000u);
    lgp(a);
    lgp(b);
}

static void BMX_CDECL stubSyncPose(void *actor)
{
    lg(0x4654b000u);
    lgp(actor);
}

static void BMX_CDECL stubMatUpdate(void *object)
{
    lg(0x4892a000u);
    lgp(object);
}

static void BMX_CDECL stubShow(ZActor *a)
{
    lg(0x75a30000u);
    lgp(a);
}

static void BMX_CDECL stubHide(ZActor *a)
{
    lg(0x75a60000u);
    lgp(a);
}

static void BMX_CDECL stubSplash(const uint8_t *obj, int unused, const float *offset)
{
    lg(0x096d0000u);
    lgp(obj);
    lg((uint32_t)unused);
    lgp(offset);
}

static int BMX_CDECL stubSound(int id, const void *pos, int a, int volume, int b)
{
    lg(0x4e300000u);
    lg((uint32_t)id);
    lgp(pos);
    lg((uint32_t)a);
    lg((uint32_t)volume);
    lg((uint32_t)b);
    return 0;
}

static void BMX_CDECL stubLimbs(void *actor, void *limbs)
{
    lg(0x55ef0000u);
    lgp(actor);
    lgp(limbs);
}

/* The rig calls: the original passes the rider actor too where the port's callee only reads the rig. */
#define RIG1(name, id)                                                                                       \
    static void BMX_CDECL name(ZRig *rig)                                                                    \
    {                                                                                                        \
        lg(id);                                                                                              \
        lgp(rig);                                                                                            \
    }
#define RIG2(name, id)                                                                                       \
    static void BMX_CDECL name(ZRig *rig, ZActor *rider)                                                     \
    {                                                                                                        \
        lg(id);                                                                                              \
        lgp(rig);                                                                                            \
        lgp(rider);                                                                                          \
    }
#define RIG3(name, id)                                                                                       \
    static void BMX_CDECL name(ZRig *rig, ZActor *rider, int16_t angle)                                      \
    {                                                                                                        \
        lg(id);                                                                                              \
        lgp(rig);                                                                                            \
        lgp(rider);                                                                                          \
        lg((uint16_t)angle);                                                                                 \
    }
RIG1(stubReleaseHands, 0x53890000u)
RIG1(stubEndLimp, 0x54140000u)
RIG1(stubEndStiff, 0x53b70000u)
RIG1(stubClearPitch, 0x546a0000u)
RIG1(stubClearYaw, 0x54790000u)
RIG2(stubGoLimp, 0x53f80000u)
RIG2(stubEase, 0x535d0000u)
RIG3(stubRequestPitch, 0x54660000u)
RIG3(stubRequestYaw, 0x54750000u)

static int BMX_CDECL stubHandsHolding(void)
{
    lg(0x53870000u);
    return (int)gRb.hands;
}

/* Every mode and sub state step: the script decides the result and whether the mode changes. */
static int BMX_CDECL stubStep(PhysRider *r)
{
    const uint32_t k = gRb.stepCalls++;
    lg(0x57e90000u);
    lgp(r);
    lg(r->stateLevel[0] | r->stateLevel[1] << 8 | r->stateLevel[2] << 16 | r->stateLevel[3] << 24);
    if (k >= 6) {
        return 1;
    }
    const uint32_t mode = (gRb.stepModes >> (k * 3)) & 7;
    if (mode != 7) {
        r->stateLevel[0] = (uint8_t)mode;
    }
    return (int)((gRb.stepScript >> k) & 1);
}

typedef struct RbStub {
    uint32_t orig;
    void *stub;
} RbStub;

static void installStubs(void)
{
    static const RbStub defs[] = {
        {0x00420c90, (void *)stubGetState},     {0x00420cb0, (void *)stubSetState},
        {0x0041fe10, (void *)stubRespawn},      {0x00420d70, (void *)stubProbe},
        {0x0046ceb0, (void *)stubBounds},       {0x004740c0, (void *)stubPlay},
        {0x00476ae0, (void *)stubBlend},        {0x00476280, (void *)stubHeight},
        {0x00474960, (void *)stubCopyJoints},   {0x004654b0, (void *)stubSyncPose},
        {0x004892a0, (void *)stubMatUpdate},    {0x00475a30, (void *)stubShow},
        {0x00475a60, (void *)stubHide},         {0x004096d0, (void *)stubSplash},
        {0x0044e300, (void *)stubSound},        {0x00455ef0, (void *)stubLimbs},
        {0x00453890, (void *)stubReleaseHands}, {0x00454140, (void *)stubEndLimp},
        {0x00453b70, (void *)stubEndStiff},     {0x004546a0, (void *)stubClearPitch},
        {0x00454790, (void *)stubClearYaw},     {0x00453f80, (void *)stubGoLimp},
        {0x004535d0, (void *)stubEase},         {0x00454660, (void *)stubRequestPitch},
        {0x00454750, (void *)stubRequestYaw},   {0x00453870, (void *)stubHandsHolding},
    };
    for (unsigned i = 0; i < sizeof defs / sizeof defs[0]; i++) {
        dt_stub(defs[i].orig, NULL, defs[i].stub);
    }
    for (int i = 0; i < 7; i++) {
        gRiderModeStep[i] = stubStep;
    }
    for (int i = 0; i < 9; i++) {
        gRiderRollStep[i] = stubStep;
    }
}

/* ---- fixtures ------------------------------------------------------------------------------------- */

static ZMotionDelta *randomDelta(void)
{
    ZMotionDelta *d = dt_alloc(sizeof *d);
    dt_fill(d, sizeof *d);
    d->delta = (ZVec3){flt(-20.0f, 20.0f), flt(-20.0f, 20.0f), flt(-20.0f, 20.0f)};
    return d;
}

/* An actor whose motion player answers motionPlaying (channel 1's key cursor). */
static ZActor *randomActor(void)
{
    ZActor *a = dt_alloc(sizeof *a);
    dt_fill(a, sizeof *a);
    ZMotionPlayer *pl = dt_alloc(sizeof *pl);
    dt_fill(pl, sizeof *pl);
    const uint32_t kind = pick(3);
    uint32_t *key = dt_alloc(4);
    *key = kind == 1 ? 0 : dt_u32() | 1;
    pl->ch[1].cursor = kind == 0 ? NULL : (const uint8_t *)key;
    a->player = pl;
    return a;
}

/* Begins an iteration: shared game state and pad script, then our stubs and scripts. */
static void begin(DtCtx *c)
{
    dtGameBegin(c, DTG_DEBUG | DTG_PAD | DTG_GAMEFN);
    rbLogN = 0;
    memset(&gRb, 0, sizeof gRb);
    /* most buttons up, so that the later input checks are reached too */
    for (int a = 0; a < 0x15; a++) {
        for (int s = 0; s < 3; s++) {
            dtGame.pressed[a][s] = (uint8_t)(pick(a == 0x10 || a == 0x11 ? 12 : 3) == 0);
        }
    }
    gRb.physState = (dt_u32() & 3) != 0 ? 1 : pick(8);
    gRb.physSetResult = pick(3) == 0 ? 0 : 1 + ((dt_u32() & 1) ? 3 + 2 * pick(2) : pick(8));
    gRb.probeHit = pick(4) != 0;
    gRb.hands = pick(2);
    gRb.height = flt(-50.0f, 50.0f);
    gRb.stepScript = dt_u32();
    gRb.stepModes = dt_u32();
    gRb.played = pick(4) == 0 ? NULL : randomDelta();
    if (pick(5) == 0) {
        gRb.blended = NULL;
    } else {
        gRb.blended = (ZMotionDelta **)dt_alloc(8);
        gRb.blended[0] = pick(6) == 0 ? NULL : randomDelta();
        gRb.blended[1] = pick(6) == 0 ? NULL : randomDelta();
    }
    installStubs();
}

/* Stick directions with a bias towards the pairs the code looks for (3 3, 4 4) and real rider tricks. */
static void randomDirections(PhysRider *r, uint32_t maxCount)
{
    r->dirCount = (uint8_t)pick(maxCount + 1);
    for (int i = 0; i < 4; i++) {
        r->dirList[i] = (uint8_t)(1 + pick(8));
    }
    if (r->dirCount >= 2 && (dt_u32() & 1) != 0) {
        const uint8_t d = (uint8_t)(3 + pick(2));
        r->dirList[r->dirCount - 1] = d;
        r->dirList[r->dirCount - 2] = d;
    }
}

static void randomAngles(ZAngle3 *a, int32_t range)
{
    a->x = (ZAngle)dt_range(-range, range);
    a->y = (ZAngle)((dt_u32() & 3) == 0 ? 0 : dt_range(-range, range));
    a->z = (ZAngle)((dt_u32() & 3) == 0 ? 0 : dt_range(-range, range));
}

static PhysRider *commonRider(void)
{
    PhysRider *r = &gPhysRiders[pick(2)];
    r->padIndex = (uint8_t)pick(2);
    r->flags = dt_u32();
    for (int i = 0; i < 4; i++) {
        r->stateTimer[i] = (int16_t)((dt_u32() & 3) != 0 ? pick(0x20) : dt_u32());
    }
    for (int i = 0; i < 7; i++) {
        r->stat[i] = (dt_u32() & 15) == 0 ? dt_float_edgy(0.0f, 3.0f) : dt_float(0.5f, 2.0f);
    }
    return r;
}

/* ---- riderUpdateAll ------------------------------------------------------------------------------- */

static void setupUpdate(DtCtx *c)
{
    begin(c);
    gPlayerCount = (uint8_t)pick(3);
    float *b = dt_alloc(0x20);
    for (int i = 0; i < 3; i++) {
        b[i] = dt_float(-2000.0f, 0.0f);
        b[i + 4] = dt_float(0.0f, 2000.0f);
    }
    b[3] = b[7] = 0.0f;
    gBounds = b;
    for (int k = 0; k < 2; k++) {
        PhysRider *r = &gPhysRiders[k];
        r->flags = dt_u32();
        if ((dt_u32() & 7) != 0) {
            r->flags |= 1;
        }
        if ((dt_u32() & 3) != 0) {
            r->flags |= 0x10;
        }
        r->inputBits = (dt_u32() & 1) != 0 ? dt_u32() : dt_u32() & 0xf;
        r->inputBits2 = (dt_u32() & 1) != 0 ? 0 : dt_u32();
        for (int i = 0; i < 4; i++) {
            const uint32_t kind = pick(4);
            r->stick[i] = kind == 0   ? flt(-0.6f, 0.6f)
                          : kind == 1 ? 0.5f * (float)(1 - 2 * (int)pick(2))
                                      : flt(-1.0f, 1.0f);
            r->axis[i] = kind == 2 ? r->stick[i] : flt(-1.0f, 1.0f);
        }
        randomDirections(r, (dt_u32() & 15) == 0 ? 6 : 4);
        float *pos = &r->pos.x;
        for (int i = 0; i < 3; i++) {
            const float lo = b[i] - 100.0f;
            const float hi = b[i + 4] + 100.0f;
            const uint32_t kind = pick(12);
            pos[i] = kind == 0   ? lo - dt_float(0.0f, 50.0f)
                     : kind == 1 ? hi + dt_float(0.0f, 50.0f)
                     : kind == 2 ? ((dt_u32() & 1) != 0 ? lo : hi)
                     : kind == 3 ? __builtin_nanf("")
                                 : dt_float(lo, hi);
        }
        r->stateLevel[0] = (uint8_t)pick(7);
        r->fieldBA4 = dt_u32();
        r->fieldB9C = (int16_t)pick(0x200);
        r->motionIndex = (uint16_t)pick(0x200);
        r->animSpeed = flt(0.0f, 2.0f);
        r->motionWeight = flt(0.0f, 1.0f);
        r->fieldBFC = (ZVec3){flt(-50.0f, 50.0f), flt(-50.0f, 50.0f), flt(-50.0f, 50.0f)};
        r->actor = randomActor();
        r->soundCooldown = (uint8_t)((dt_u32() & 1) != 0 ? 0 : dt_u32());
    }
    if (pick(40) == 0) {
        /* the "no update proc" assert (an abort in the harness); steps keep their mode so that the loop,
         * which does not check again, never calls the NULL entry */
        gRiderModeStep[gPhysRiders[pick(2)].stateLevel[0]] = NULL;
        gRb.stepModes = 0x3fffffffu;
    }
}

/* ---- riderStepAir --------------------------------------------------------------------------------- */

static void setupAir(DtCtx *c)
{
    begin(c);
    PhysRider *r = commonRider();
    r->stateLevel[0] = kRiderAir;
    r->stateLevel[1] = (uint8_t)pick(3);
    r->stateLevel[2] = (uint8_t)pick(6);
    randomDirections(r, 4);
    r->rot.y = (ZAngle)((dt_u32() & 3) == 0   ? 0
                        : (dt_u32() & 3) == 0 ? (int32_t)dt_u32()
                                              : dt_range(-0x900, 0x900));
    r->rot.z = (ZAngle)((dt_u32() & 3) == 0 ? dt_range(-200, 200) : (int32_t)dt_u32());
    r->orientARot.y = (ZAngle)dt_range(-0x1000, 0x1000);
    r->timer = (int16_t)((dt_u32() & 1) != 0 ? 0 : dt_range(1, 30));
    r->yawTrim = (int16_t)((dt_u32() & 1) != 0 ? dt_range(-0x1900, 0x1900) : (int32_t)dt_u32());
    r->stateTimer[3] = (int16_t)((dt_u32() & 1) != 0 ? 7 : dt_u32());
    r->spin = (int32_t)dt_u32();
    if ((dt_u32() & 1) != 0) {
        /* no turn button, and maybe no stick: the free turn and the damping */
        dtGame.pressed[0xa][1] = dtGame.pressed[0xb][1];
        dtGame.axes[0][0] = (dt_u32() & 1) != 0 ? 0.0f : dtGame.axes[0][0];
    }
    if ((dt_u32() & 3) == 0) {
        dtGame.axes[0][1] = (dt_u32() & 1) != 0 ? 0.75f : -0.75f;
    }
    DT_ARG_PTR(c, 0, r);
}

/* ---- riderStepBail -------------------------------------------------------------------------------- */

static void setupBail(DtCtx *c)
{
    begin(c);
    PhysRider *r = commonRider();
    r->stateLevel[0] = kRiderBail;
    r->stateLevel[1] = (uint8_t)((dt_u32() & 15) == 0 ? dt_u32() : pick(7));
    r->surface = (uint8_t)((dt_u32() & 3) == 0 ? 0xd : pick(16));
    r->pos = (ZVec3){flt(-500.0f, 500.0f), flt(-500.0f, 500.0f), flt(-500.0f, 500.0f)};
    gRb.probeY = (dt_u32() & 3) == 0 ? r->pos.y + (float)(int)pick(3) * 50.0f : flt(-1000.0f, 1000.0f);
    r->speed = (dt_u32() & 3) == 0 ? 3.0f : flt(0.0f, 6.0f);
    r->stateTimer[0] = (int16_t)((dt_u32() & 1) != 0 ? dt_range(0x90, 0x9a) : dt_range(0x126, 0x130));
    r->stateTimer[1] = (int16_t)((dt_u32() & 1) != 0 ? dt_range(0xe, 0x10) : dt_range(0x3a, 0x3c));
    if ((dt_u32() & 1) != 0) {
        r->flags &= ~0x1000006u;
    }
    if ((dt_u32() & 1) != 0) {
        r->flags &= ~0x4000000u;
    }
    r->actor = (void *)(uintptr_t)0x00300100;
    r->actor2 = (void *)(uintptr_t)0x00300200;
    r->bikeObj = (void *)(uintptr_t)0x00300300;
    DT_ARG_PTR(c, 0, r);
}

/* ---- riderRollSub7 -------------------------------------------------------------------------------- */

static void setupSub7(DtCtx *c)
{
    begin(c);
    PhysRider *r = commonRider();
    r->stateLevel[2] = (uint8_t)((dt_u32() & 15) == 0 ? dt_u32() : pick(5));
    randomDirections(r, 4);
    if (r->dirCount != 0 && (dt_u32() & 1) != 0) {
        /* end the directions with one of the rider's signature tricks */
        const uint8_t *sig = (const uint8_t *)(uintptr_t)0x00553840 + gProfiles[r->player].character * 0x18;
        const uint8_t *t = sig + pick(4) * 6;
        const uint32_t n = t[0];
        if (n != 0 && n <= 3) {
            r->dirCount = (uint8_t)(n + pick(5 - n));
            for (uint32_t i = 0; i < n; i++) {
                r->dirList[r->dirCount - n + i] = t[1 + i];
            }
        }
    }
    r->anim = (uint8_t)((dt_u32() & 7) == 0 ? dt_u32() : pick(0x1c));
    r->ikPose = (uint8_t)((dt_u32() & 3) == 0 ? 0xff : (dt_u32() & 1) != 0 ? 0 : pick(0x30));
    r->orientBRot = (ZAngle3){0, 0, 0};
    randomAngles(&r->orientBRot, (dt_u32() & 1) != 0 ? 0x400 : 0x7fff);
    static const int32_t caps[] = {0xd555, 0x1d555, 0x2d555, 0xeaaa, 0x1eaaa};
    const int32_t cap = caps[pick(5)] + dt_range(-0x800, 0x100);
    r->field54 = (dt_u32() & 3) == 0 ? (int32_t)dt_u32() : (dt_u32() & 1) != 0 ? cap : -cap;
    const uint32_t kind = pick(5);
    r->ikBlend = kind == 0 ? 0.0f : kind == 1 ? 1.0f : flt(-0.2f, 1.2f);
    if ((dt_u32() & 1) != 0) {
        r->stateTimer[1] = (int16_t)(r->stateTimer[1] & ~0xf);
    }
    DT_ARG_PTR(c, 0, r);
}

DT_CASES(rider_big_cases) = {
    {"riderUpdateAll", 0x00431000, riderUpdateAll, 0, DT_RET_VOID, setupUpdate, 2000, 0},
    {"riderStepAir", 0x004322b0, riderStepAir, 1, DT_RET_INT, setupAir, 3000, 0},
    {"riderStepBail", 0x00433480, riderStepBail, 1, DT_RET_INT, setupBail, 2000, 0},
    {"riderRollSub7", 0x00434760, riderRollSub7, 1, DT_RET_INT, setupSub7, 3000, 0},
};
