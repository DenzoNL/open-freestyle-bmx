/*
 * Fixtures of the game and rider tests: scripted stubs for everything the functions under test call
 * (see dt_game.h) and the randomisation of the game and rider state.
 */
#include "dt_game.h"

#include "bmx/com/action/camera.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/fx.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/prop.h"
#include "bmx/com/action/replay.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/ngiffx.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzres.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/com/zge/zactor.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zquat.h"
#include "zge/zstring.h"

DtGameScript dtGame;

/* ---- log ---------------------------------------------------------------------------------------- */

#define LOG_SIZE 4096

/* Exported so the compiler keeps the stores although nothing in the DLL reads the log. */
__declspec(dllexport) uint32_t dtGameLog[LOG_SIZE];
__declspec(dllexport) uint32_t dtGameLogN;

static void lg(uint32_t v)
{
    if (dtGameLogN < LOG_SIZE) {
        dtGameLog[dtGameLogN] = v;
    }
    dtGameLogN++;
}

static void lgf(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    lg(u);
}

static void lgp(const void *p)
{
    lg(dt_canon((uint32_t)(uintptr_t)p));
}

static void lgv(const ZVec3 *v)
{
    lgf(v->x);
    lgf(v->y);
    lgf(v->z);
}

static void lgVert(const NgifVert3 *v)
{
    lgf(v->x);
    lgf(v->y);
    lgf(v->z);
    lg(v->rgb[0] | v->rgb[1] << 8 | v->rgb[2] << 16);
}

static uint32_t hashText(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s != '\0') {
        h = (h ^ (uint8_t)*s++) * 16777619u;
    }
    return h;
}

/* ---- stubs -------------------------------------------------------------------------------------- */

static void BMX_CDECL stubFail(const char *why, const char *file, int line, int flags)
{
    lg(0xfa110000u);
    lg(hashText(why));
    lg(hashText(file));
    lg((uint32_t)line);
    lg((uint32_t)flags);
}

static void BMX_CDECL stubPrint(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    lg(0x9217a000u);
    for (const char *p = fmt; *p; p++) {
        if (*p == '%') {
            lg((uint32_t)va_arg(ap, int));
            p++;
            while (*p != '\0' && *p != 'd' && *p != 's') {
                p++;
            }
            if (*p == '\0') {
                break;
            }
        }
    }
    lg(hashText(fmt));
    va_end(ap);
}

static int BMX_CDECL stubSound(int id, int a, int b, int c, int d)
{
    lg(0x50d00000u);
    lg((uint32_t)id);
    lg(dt_canon((uint32_t)a));
    lg((uint32_t)b);
    lg((uint32_t)c);
    lg((uint32_t)d);
    return 0;
}

static void BMX_CDECL stubSetElementA(int index, const void *value)
{
    const uint8_t *b = value;
    lg(0x0e1a0000u + (uint32_t)index);
    switch (index) {
    case 2:
        lg(*(const uint32_t *)value);
        break;
    case 7:
    case 8:
        lg(b[0]);
        break;
    case 5:
        lg(*(const uint32_t *)value);
        lg(*(const uint16_t *)(b + 10));
        break;
    case 0xc:
        lg(*(const uint32_t *)value);
        lg(*(const uint32_t *)(b + 4));
        break;
    default:
        break;
    }
}

static void BMX_CDECL stubSetElementB(int index, void *value)
{
    lg(0x0e1b0000u + (uint32_t)index);
    if (index == 0xc) {
        memcpy(value, dtGame.statusMsg, 8);
    }
}

static void BMX_CDECL stubSetScreen(uint32_t id)
{
    lg(0x5c4e0000u);
    lg(id);
}

static uint32_t BMX_CDECL stubBigGame(int state)
{
    lg(0xb1600000u);
    lg((uint32_t)state);
    return 0;
}

static void BMX_CDECL stubUpdateRun(int player)
{
    lg(0x0d010000u);
    lg((uint32_t)player);
}

static void BMX_CDECL stubAwardGoals(int player)
{
    lg(0x0e380000u);
    lg((uint32_t)player);
}

static void BMX_CDECL stubEvaluateRun(int player)
{
    lg(0x0ec40000u);
    lg((uint32_t)player);
}

static void BMX_CDECL stubEndRun(int player, int a)
{
    lg(0x0dae0000u);
    lg((uint32_t)player);
    lg((uint32_t)a);
}

static void BMX_CDECL stubRiderTrick(int player, int a)
{
    lg(0xc3000000u);
    lg((uint32_t)player);
    lg((uint32_t)a);
}

static uint32_t BMX_CDECL stubGetLevelRecord(uint32_t key)
{
    lg(0x07d20000u);
    lg(key);
    return dtGame.recordMissing ? 0 : (uint32_t)(uintptr_t)dtGame.levelRecord;
}

static uint8_t *BMX_CDECL stubGoalInfo(int player, uint32_t level)
{
    lg(0x07fc0000u);
    lg((uint32_t)player);
    lg(level);
    return dtGame.goalInfo[player & 3][level % 12];
}

static void BMX_CDECL stubResetView(int player, int mode)
{
    lg(0x07760000u);
    lg((uint32_t)player);
    lg((uint32_t)mode);
}

static void BMX_CDECL stubSelectPlayer(uint32_t player)
{
    lg(0x07530000u);
    lg(player);
}

static uint32_t BMX_CDECL stubViewPlayer(void)
{
    lg(0x07600000u);
    return dtGame.viewPlayer;
}

static uint32_t BMX_CDECL stubPlayback(void)
{
    lg(0x056a0000u);
    return dtGame.playback;
}

static void BMX_CDECL stubPropsReset(void)
{
    lg(0x2ecf0000u);
}

static uint8_t *BMX_CDECL stubPropFind(uint32_t id)
{
    static uint8_t props[2][0x84];
    lg(0x2ed10000u);
    lg(id);
    if (dtGame.propMissing && (id & 1) != 0) {
        return NULL;
    }
    return props[id & 1];
}

static void BMX_CDECL stubPropCollect(uint32_t player, uint8_t *prop)
{
    lg(0x2ed80000u);
    lg(player);
    lgp(prop);
}

static void BMX_CDECL stubPropRelease(uint32_t player, uint8_t *prop)
{
    lg(0x2ee40000u);
    lg(player);
    lgp(prop);
}

static uint8_t BMX_CDECL stubReplayCreate(const ReplayParams *params)
{
    lg(0x30180000u);
    lg(params->flags);
    lg(params->index);
    lg((uint32_t)params->size);
    dtGame.replayCalls++;
    if (dtGame.replayFailAt != 0 && dtGame.replayCalls == dtGame.replayFailAt) {
        return 0xff;
    }
    return (uint8_t)(0x10 + dtGame.replayCalls);
}

static void BMX_CDECL stubReplayDestroy(uint32_t handle)
{
    lg(0x30380000u);
    lg(handle & 0xff);
}

static const float *BMX_CDECL stubBounds(void)
{
    lg(0x6ceb0000u);
    return dtGame.bounds;
}

static void BMX_CDECL stubDrawCallback(ZrollerDrawFn fn)
{
    lg(0x6d690000u);
    lg((uint32_t)(uintptr_t)fn);
}

static const uint8_t *BMX_CDECL stubRollerRecord(uint32_t index)
{
    lg(0x6c920000u);
    lg(index);
    return dtGame.rollerRecord;
}

static void BMX_CDECL stubGetStart(int index, ZVec3 *pos, ZAngle3 *rot)
{
    lg(0x6cf30000u);
    lg((uint32_t)index);
    *pos = dtGame.startPos;
    *rot = dtGame.startRot;
}

static void *BMX_CDECL stubCreateBike(const int32_t *lookId)
{
    lg(0x6c740000u);
    lg((uint32_t)*lookId);
    return (dtGame.actorNull & 2) != 0 ? NULL : (void *)0x00300001;
}

static const uint8_t *BMX_CDECL stubLevelInfo(uint32_t index)
{
    static uint8_t info[100];
    lg(0x6b990000u);
    lg(index);
    return info;
}

static void *BMX_CDECL stubCreateActor(const void *record)
{
    const uint8_t *b = record;
    lg(0x6c580000u);
    lgp(record);
    lg(b[0] | b[1] << 8);
    lg(b[2]);
    return (dtGame.actorNull & 1) != 0 ? NULL : (void *)0x00300002;
}

static ZAngle BMX_CDECL stubAsin(float s)
{
    lg(0x80e60000u);
    lgf(s);
    if (dtGame.asinResult != 0) {
        return (ZAngle)(dtGame.asinResult - 0x10000);
    }
    return (ZAngle)(int32_t)(s * dtGame.asinScale);
}

static ZAngle BMX_CDECL stubAcos(float c)
{
    lg(0x80ed0000u);
    lgf(c);
    if (dtGame.acosResult != 0) {
        return (ZAngle)(dtGame.acosResult - 0x10000);
    }
    return (ZAngle)(int32_t)((1.0f - c) * dtGame.acosScale);
}

static double BMX_CDECL stubDot(const ZVec3 *a, const ZVec3 *b)
{
    lg(0x81540000u);
    lgv(a);
    lgv(b);
    return (double)a->x * b->x + (double)a->y * b->y + (double)a->z * b->z;
}

static void BMX_CDECL stubZero(ZVec3 *v)
{
    lg(0x81120000u);
    v->x = 0.0f;
    v->y = 0.0f;
    v->z = 0.0f;
}

static void BMX_CDECL stubAdd(ZVec3 *out, const ZVec3 *a, const ZVec3 *b)
{
    lg(0x81130000u);
    lgp(out);
    lgv(a);
    lgv(b);
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
}

static void BMX_CDECL stubNegate(ZVec3 *out, const ZVec3 *in)
{
    lg(0x812f0000u);
    lgp(out);
    lgv(in);
    out->x = -in->x;
    out->y = -in->y;
    out->z = -in->z;
}

static void BMX_CDECL stubIdentity(ZMatrix *m)
{
    lg(0x81880000u);
    lgp(m);
    memset(m, 0, sizeof *m);
    m->right.x = 1.0f;
    m->up.y = 1.0f;
    m->at.z = 1.0f;
}

static ZMatrix *BMX_CDECL stubRotateEuler(ZMatrix *out, const ZMatrix *in, const ZAngle3 *angles)
{
    lg(0x81ba0000u);
    lgp(out);
    lgp(in);
    lg((uint16_t)angles->x | (uint32_t)(uint16_t)angles->y << 16);
    lg((uint16_t)angles->z);
    if (out != in) {
        memcpy(out, in, sizeof *out);
    }
    out->right.x += (float)angles->x;
    out->up.y += (float)angles->y * 0.5f;
    out->at.z -= (float)angles->z;
    return out;
}

static void BMX_CDECL stubCross3(ZVec3 *out, const ZVec3 *a, const ZVec3 *b, const ZVec3 *c)
{
    lg(0x815b0000u);
    lgp(out);
    lgv(a);
    lgv(b);
    lgv(c);
    out->x = a->y * c->z - a->z * c->y;
    out->y = a->z * c->x - a->x * c->z;
    out->z = a->x * c->y - a->y * c->x;
}

static void BMX_CDECL stubTransform(ZVec3 *out, const ZMatrix *m, const ZVec3 *v)
{
    lg(0x81640000u);
    lgv(&m->right);
    lgv(&m->up);
    lgv(&m->at);
    lgv(&m->pos);
    lgv(v);
    out->x = v->x * 1.5f + m->right.x;
    out->y = v->y * 0.5f - m->up.y;
    out->z = v->z + 3.0f + m->at.z;
}

static void BMX_CDECL stubQuatToMatrix(ZMatrix *m, ZQuat *q)
{
    lg(0x83470000u);
    lgp(m);
    lgf(q->x);
    lgf(q->y);
    lgf(q->z);
    lgf(q->w);
    memset(m, 0, sizeof *m);
    m->right = (ZVec3){q->x * 0.5f, q->y + 1.0f, q->z};
    m->up = (ZVec3){q->w, q->x, -q->y};
    m->at = (ZVec3){q->z, q->w * 2.0f, q->x};
}

static void BMX_CDECL stubPoly(int flags, int count, const NgifVert3 *v, int texture)
{
    lg(0x63130000u);
    lg((uint32_t)flags);
    lg((uint32_t)count);
    lg((uint32_t)texture);
    for (int i = 0; i < count; i++) {
        lgVert(&v[i]);
    }
}

static void BMX_CDECL stubLine(int flags, const NgifVert3 *a, const NgifVert3 *b)
{
    lg(0x63710000u);
    lg((uint32_t)flags);
    lgVert(a);
    lgVert(b);
}

static const uint8_t *BMX_CDECL stubCollideObject(uint32_t id)
{
    lg(0x69530000u);
    lg(id & 0xffff);
    return dtGame.object;
}

static const uint8_t *BMX_CDECL stubBspInfo(uint32_t id)
{
    lg(0x665c0000u);
    lg(id & 0xffff);
    return dtGame.objectInfo;
}

static uint32_t BMX_CDECL stubPhysGetState(const PhysRider *r)
{
    lg(0x420c9000u);
    lgp(r);
    return dtGame.physState;
}

static uint8_t BMX_CDECL stubPhysSetState(PhysRider *r, int state)
{
    lg(0x420cb000u);
    lgp(r);
    lg((uint32_t)state);
    return (uint8_t)(dtGame.physSetResult != 0 ? dtGame.physSetResult - 1 : (uint32_t)state);
}

static void BMX_CDECL stubPhysAddVelocity(PhysRider *r, const ZVec3 *dv)
{
    lg(0x420d3000u);
    lgp(r);
    lgv(dv);
}

static void BMX_CDECL stubPhysRespawn(PhysRider *r)
{
    lg(0x41fe1000u);
    lgp(r);
}

static void BMX_CDECL stubPhysUpdateRiders(void)
{
    lg(0x41fc4000u);
}

static uint8_t *BMX_CDECL stubPadGet(uint32_t index)
{
    lg(0x86f00000u);
    lg(index);
    return dtGame.padRecords[index & 1];
}

static int BMX_CDECL stubPadAction(uint32_t config, const void *state, uint32_t action, uint32_t status)
{
    lg(0x84b00000u);
    lg(config & 0xff);
    lgp(state);
    lg(action);
    lg(status);
    return (dtGame.pressed[action % 0x15][status % 3] & 1) != 0;
}

static const float *BMX_CDECL stubPadAxes(const void *state, uint32_t index)
{
    lg(0x86100000u);
    lgp(state);
    lg(index);
    return dtGame.axes[index & 1];
}

static void BMX_CDECL stubRumble(int player, int a, int b)
{
    lg(0x86d00000u);
    lg((uint32_t)player);
    lg((uint32_t)a);
    lg((uint32_t)b);
}

static void *BMX_CDECL stubRiderMotion(void *actor, uint32_t flags)
{
    lg(0x52e80000u);
    lgp(actor);
    lg(flags & 0xff);
    return (void *)0x00300003;
}

static void *BMX_CDECL stubBikeMotion(void)
{
    lg(0x526a0000u);
    return (void *)0x00300004;
}

static void BMX_CDECL stubHitCallback(void *motion, ZoverHitFn fn)
{
    lg(0x51280000u);
    lgp(motion);
    lg((uint32_t)(uintptr_t)fn);
}

static void BMX_CDECL stubMotionSetup(void *motion, void *actor)
{
    lg(0x54400000u);
    lgp(motion);
    lgp(actor);
}

static void BMX_CDECL stubMotionSetupB(void *motion, void *actor)
{
    lg(0x54430000u);
    lgp(motion);
    lgp(actor);
}

static void BMX_CDECL stubReadLimbs(void *actor, void *limbs)
{
    lg(0x55f10000u);
    lgp(actor);
    lgp(limbs);
}

static void BMX_CDECL stubLimbsBegin(void *actor, void *limbs)
{
    lg(0x55ef0000u);
    lgp(actor);
    lgp(limbs);
}

#define POSE_STUB(name, id)                                                                                  \
    static void BMX_CDECL name(void *motion, void *actor)                                                    \
    {                                                                                                        \
        lg(id);                                                                                              \
        lgp(motion);                                                                                         \
        lgp(actor);                                                                                          \
    }
POSE_STUB(stubPoseA, 0x54140000u)
POSE_STUB(stubPoseB, 0x53b70000u)
POSE_STUB(stubPoseC, 0x546a0000u)
POSE_STUB(stubPoseD, 0x54790000u)
POSE_STUB(stubPoseE, 0x53890000u)

static void BMX_CDECL stubActorShow(ZActor *a)
{
    lg(0x75a30000u);
    lgp(a);
}

static void BMX_CDECL stubActorHide(ZActor *a)
{
    lg(0x75a60000u);
    lgp(a);
}

static ZVec3 *BMX_CDECL stubActorPos(ZActor *a)
{
    static ZVec3 pos;
    lg(0x72940000u);
    lgp(a);
    return &pos;
}

static void BMX_CDECL stubSparks(const ZVec3 *pos, const ZVec3 *vel)
{
    lg(0x08c90000u);
    lgv(pos);
    lgv(vel);
}

static void BMX_CDECL stubBurst(void)
{
    lg(0x08a80000u);
}

static void BMX_CDECL stubFxEmit(uint32_t a, uint32_t b, const ZVec3 *pos, uint32_t c, uint32_t type)
{
    lg(0x095a0000u);
    lg(a);
    lg(b & 0xffff);
    lgv(pos);
    lg(c);
    lg(type);
}

static void BMX_CDECL stubFxCreate(uint32_t type, const ZVec3 *pos, const void *arg)
{
    lg(0x09640000u);
    lg(type);
    lgv(pos);
    lgp(arg);
}

static const uint8_t *BMX_CDECL stubFxNode(const void *obj, int index)
{
    lg(0x5f480000u);
    lgp(obj);
    lg((uint32_t)index);
    return dtGame.node;
}

static void BMX_CDECL stubNameCall(const char *name, uint32_t a, uint32_t b)
{
    const uint32_t *w = (const uint32_t *)(uintptr_t)b;
    lg(0x82fc0000u);
    lg(hashText(name));
    lgp((const void *)(uintptr_t)a);
    for (int i = 0; i < 13; i++) {
        if (i == 3 && name[0] == 'r') {
            continue; /* the ripple block's word 3 is never written */
        }
        if (i >= 9 && name[0] == 'r') {
            break;
        }
        lg(w[i]);
    }
}

static void BMX_CDECL stubRiderReset(int player, int full)
{
    lg(0x31aa0000u);
    lg((uint32_t)player);
    lg((uint32_t)full);
}

static void BMX_CDECL stubUpdateInput(void)
{
    lg(0x31000000u);
}

/* Stands in for every mode and sub state step function: the script decides what a call returns and does. */
static int BMX_CDECL stubStep(PhysRider *r)
{
    const uint32_t k = dtGame.stepCalls++;
    lg(0x57e90000u);
    lgp(r);
    lg(r->stateLevel[0] | r->stateLevel[1] << 8 | r->stateLevel[2] << 16 | r->stateLevel[3] << 24);
    if (k >= 6) {
        return 1;
    }
    const uint32_t bits = (dtGame.stepScript >> (k * 3)) & 7;
    if ((bits & 2) != 0) {
        r->stateLevel[0] = (uint8_t)(r->stateLevel[0] + 1);
    }
    if ((bits & 4) != 0) {
        r->stateLevel[1] = 0;
    }
    return (int)(bits & 1);
}

void dtGameInstallStepTables(void)
{
    for (int i = 0; i < 9; i++) {
        gRiderRollStep[i] = stubStep;
    }
    gRiderGrindStep[0] = stubStep;
    gRiderLipStep[0] = stubStep;
    gRiderManualStep[0] = stubStep;
}

typedef struct StubDef {
    uint32_t orig;
    void *ours;
    void *stub;
    uint32_t group;
} StubDef;

#define STUB(addr, ours, fn, group) {addr, (void *)(ours), (void *)(fn), group}

static void installStubs(uint32_t groups)
{
    static const StubDef defs[] = {
        STUB(0x0047ed10, NULL, stubFail, DTG_DEBUG),
        STUB(0x0044e300, NULL, stubSound, DTG_SOUND),
        STUB(0x00415370, ovlSetElementA, stubSetElementA, DTG_OVL),
        STUB(0x004153a0, ovlSetElementB, stubSetElementB, DTG_OVL),
        STUB(0x00415400, ovlSetScreen, stubSetScreen, DTG_OVL),
        STUB(0x0040aa50, NULL, stubBigGame, DTG_GAMEFN),
        STUB(0x0040d010, NULL, stubUpdateRun, DTG_GAMEFN),
        STUB(0x0040e380, NULL, stubAwardGoals, DTG_GAMEFN),
        STUB(0x0040ec40, NULL, stubEvaluateRun, DTG_GAMEFN),
        STUB(0x0040dae0, NULL, stubEndRun, DTG_GAMEFN),
        STUB(0x0040c300, NULL, stubRiderTrick, DTG_GAMEFN),
        STUB(0x00407d20, NULL, stubGetLevelRecord, DTG_COMDATA),
        STUB(0x00407fc0, NULL, stubGoalInfo, DTG_COMDATA),
        STUB(0x00407760, NULL, stubResetView, DTG_COMDATA),
        STUB(0x00407530, NULL, stubSelectPlayer, DTG_COMDATA),
        STUB(0x00407600, NULL, stubViewPlayer, DTG_COMDATA),
        STUB(0x004056a0, NULL, stubPlayback, DTG_COMDATA),
        STUB(0x0042ecf0, NULL, stubPropsReset, DTG_PROP),
        STUB(0x0042ed10, NULL, stubPropFind, DTG_PROP),
        STUB(0x0042ed80, NULL, stubPropCollect, DTG_PROP),
        STUB(0x0042ee40, NULL, stubPropRelease, DTG_PROP),
        STUB(0x00430180, NULL, stubReplayCreate, DTG_REPLAY),
        STUB(0x00430380, NULL, stubReplayDestroy, DTG_REPLAY),
        STUB(0x0046ceb0, NULL, stubBounds, DTG_ROLLER),
        STUB(0x0046d690, NULL, stubDrawCallback, DTG_ROLLER),
        STUB(0x0046c920, NULL, stubRollerRecord, DTG_ROLLER),
        STUB(0x0046cf30, NULL, stubGetStart, DTG_ROLLER),
        STUB(0x0046c740, NULL, stubCreateBike, DTG_ROLLER),
        STUB(0x0046b990, NULL, stubLevelInfo, DTG_ROLLER),
        STUB(0x0046c580, NULL, stubCreateActor, DTG_ROLLER),
        STUB(0x00480e60, NULL, stubAsin, DTG_MATH),
        STUB(0x00480ed0, NULL, stubAcos, DTG_MATH),
        STUB(0x00481540, NULL, stubDot, DTG_MATH),
        STUB(0x00481120, NULL, stubZero, DTG_MATH),
        STUB(0x00481130, NULL, stubAdd, DTG_MATH),
        STUB(0x004812f0, NULL, stubNegate, DTG_MATH),
        STUB(0x00481880, NULL, stubIdentity, DTG_MATH),
        STUB(0x00481ba0, NULL, stubRotateEuler, DTG_MATH),
        STUB(0x004815b0, NULL, stubCross3, DTG_MATH),
        STUB(0x00481640, NULL, stubTransform, DTG_MATH),
        STUB(0x00483470, zquatToMatrix, stubQuatToMatrix, DTG_MATH),
        STUB(0x00463130, NULL, stubPoly, DTG_DRAW),
        STUB(0x00463710, NULL, stubLine, DTG_DRAW),
        STUB(0x00469530, NULL, stubCollideObject, DTG_DRAW),
        STUB(0x004665c0, NULL, stubBspInfo, DTG_DRAW),
        STUB(0x00420c90, physGetState, stubPhysGetState, DTG_PHYS),
        STUB(0x00420cb0, physSetState, stubPhysSetState, DTG_PHYS),
        STUB(0x00420d30, physAddVelocity, stubPhysAddVelocity, DTG_PHYS),
        STUB(0x0041fe10, physRespawn, stubPhysRespawn, DTG_PHYS),
        STUB(0x0041fc40, physUpdateRiders, stubPhysUpdateRiders, DTG_PHYS),
        STUB(0x004086f0, NULL, stubPadGet, DTG_PAD),
        STUB(0x004084b0, NULL, stubPadAction, DTG_PAD),
        STUB(0x00408610, NULL, stubPadAxes, DTG_PAD),
        STUB(0x004086d0, NULL, stubRumble, DTG_PAD),
        STUB(0x00452e80, NULL, stubRiderMotion, DTG_ZOVER),
        STUB(0x004526a0, NULL, stubBikeMotion, DTG_ZOVER),
        STUB(0x00451280, NULL, stubHitCallback, DTG_ZOVER),
        STUB(0x00454400, NULL, stubMotionSetup, DTG_ZOVER),
        STUB(0x00454430, NULL, stubMotionSetupB, DTG_ZOVER),
        STUB(0x00455f10, NULL, stubReadLimbs, DTG_ZOVER),
        STUB(0x00455ef0, NULL, stubLimbsBegin, DTG_ZOVER),
        STUB(0x00454140, NULL, stubPoseA, DTG_ZOVER),
        STUB(0x00453b70, NULL, stubPoseB, DTG_ZOVER),
        STUB(0x004546a0, NULL, stubPoseC, DTG_ZOVER),
        STUB(0x00454790, NULL, stubPoseD, DTG_ZOVER),
        STUB(0x00453890, NULL, stubPoseE, DTG_ZOVER),
        STUB(0x00475a30, zactorShow, stubActorShow, DTG_ZOVER),
        STUB(0x00475a60, zactorHide, stubActorHide, DTG_ZOVER),
        STUB(0x00472940, zactorGetPos, stubActorPos, DTG_ZOVER),
        STUB(0x00408c90, NULL, stubSparks, DTG_FX),
        STUB(0x00408a80, NULL, stubBurst, DTG_FX),
        STUB(0x004095a0, NULL, stubFxEmit, DTG_FX),
        STUB(0x00409640, NULL, stubFxCreate, DTG_FX),
        STUB(0x0045f480, NULL, stubFxNode, DTG_FX),
        STUB(0x00482fc0, zstrNameTable2Call, stubNameCall, DTG_FX),
        STUB(0x00431aa0, riderReset, stubRiderReset, DTG_RIDERFN),
        STUB(0x00431000, NULL, stubUpdateInput, DTG_RIDERFN),
    };
    for (unsigned i = 0; i < sizeof defs / sizeof defs[0]; i++) {
        if ((defs[i].group & groups) != 0) {
            dt_stub(defs[i].orig, defs[i].ours, defs[i].stub);
        }
    }
    if ((groups & DTG_DEBUG) != 0) {
        gZdebugPrint = stubPrint;
    }
}

/* ---- state -------------------------------------------------------------------------------------- */

static uint32_t pick(uint32_t n)
{
    return dt_u32() % n;
}

void dtGameRandomRider(PhysRider *r)
{
    dt_fill(r, sizeof *r);
    r->flags = dt_u32();
    for (int i = 0; i < 4; i++) {
        r->stateLevel[i] = (uint8_t)pick(4);
        r->stateTimer[i] = (int16_t)(dt_u32() & 3 ? pick(0x20) : dt_u32());
    }
    r->combo.count = (uint8_t)pick(9);
    r->combo.runState = (uint8_t)pick(5);
    r->anim = (dt_u32() & 3) ? (uint8_t)pick(0x30) : (uint8_t)dt_u32();
    r->ikPose = (dt_u32() & 3) ? (uint8_t)pick(0x10) : (uint8_t)dt_u32();
    r->dirCount = (uint8_t)pick(5);
    for (int i = 0; i < 4; i++) {
        r->dirList[i] = (uint8_t)pick(10);
    }
    r->surface = (uint8_t)pick(6);
    r->speed = dt_float_edgy(-20.0f, 60.0f);
    r->up = (ZVec3){dt_float_edgy(-1.0f, 1.0f), dt_float_edgy(-1.0f, 1.0f), dt_float_edgy(-1.0f, 1.0f)};
    r->fwd = (ZVec3){dt_float_edgy(-1.0f, 1.0f), dt_float_edgy(-1.0f, 1.0f), dt_float_edgy(-1.0f, 1.0f)};
    r->vel =
        (ZVec3){dt_float_edgy(-50.0f, 50.0f), dt_float_edgy(-50.0f, 50.0f), dt_float_edgy(-50.0f, 50.0f)};
    r->rot.x = (ZAngle)dt_u32();
    r->rot.y = (ZAngle)(dt_u32() & 1 ? dt_range(-0x500, 0x500) : (int32_t)dt_u32());
    r->rot.z = (ZAngle)dt_u32();
    r->timer = (int16_t)(dt_u32() & 7 ? dt_range(1, 60) : (int32_t)dt_u32());
    for (int i = 0; i < 4; i++) {
        r->stick[i] = dt_float_edgy(-1.0f, 1.0f);
        r->axis[i] = dt_float_edgy(-1.0f, 1.0f);
    }
    for (int i = 0; i < 7; i++) {
        r->stat[i] = dt_float_edgy(0.0f, 2.0f);
    }
    for (int i = 0; i < 2; i++) {
        r->wheel[i].mode = (uint8_t)pick(3);
        r->wheel[i].speed = dt_float_edgy(-2.0f, 3.0f);
    }
    r->fieldBA0 = (dt_u32() & 1) ? (void *)0x00300010 : NULL;
    r->motionSample = (dt_u32() & 1) ? (void *)0x00300020 : NULL;
    r->actor = (void *)(0x00300100u + pick(4) * 0x100);
    r->actor2 = (void *)(0x00300200u + pick(4) * 0x100);
    r->bikeObj = (void *)(0x00300300u + pick(4) * 0x100);
    r->bikeObj2 = (void *)(0x00300400u + pick(4) * 0x100);
    r->impactTotal = dt_float_edgy(0.0f, 100.0f);
    r->snapB.surface = (dt_u32() & 3) ? 0xc : (uint8_t)pick(16);
    r->snapB.contact = dtGame.object != NULL ? dtGame.object : (void *)r->snapB.data;
}

static void randomGoals(void)
{
    for (uint32_t set = 0; set < 18; set++) {
        for (uint32_t row = 0; row < 4; row++) {
            GameGoalRow *g = &gGameGoals[set][row];
            dt_fill(g, sizeof *g);
            g->count = (uint8_t)pick(5);
            for (int k = 0; k < 3; k++) {
                g->prop[k] = (dt_u32() & 3) ? (uint8_t)pick(8) : 0xff;
            }
            for (int i = 0; i < 4; i++) {
                static const uint8_t types[] = {0,  0,  1,  2,  3,  6,  7,  8,  9,  10, 11,
                                                12, 13, 14, 15, 16, 17, 18, 19, 20, 21};
                g->goal[i].type = (dt_u32() & 3) ? types[pick(sizeof types)] : (uint16_t)pick(30);
                g->goal[i].min = (dt_u32() & 1) ? (uint16_t)pick(8) : (uint16_t)(dt_u32() & 0xff);
                g->goal[i].max = (uint16_t)(g->goal[i].min + pick(6));
                for (int k = 0; k < 4; k++) {
                    g->goal[i].prop[k] = (dt_u32() & 3) ? (uint8_t)pick(8) : 0xff;
                }
            }
        }
    }
}

static void randomGoalInfos(void)
{
    for (int p = 0; p < 4; p++) {
        for (int l = 0; l < 12; l++) {
            uint8_t *info = dt_alloc(0x80);
            dt_fill(info, 0x80);
            info[1] = (dt_u32() & 7) ? (uint8_t)pick(3) : (uint8_t)pick(5);
            info[2] = (uint8_t)pick(5);
            for (int r = 0; r < 12; r++) {
                info[3 + r * 3] = (uint8_t)((dt_u32() & 1) ? (dt_u32() & 0xc0) : pick(4) << 6);
                for (int b = 1; b < 3; b++) {
                    const uint8_t lo = (uint8_t)(pick(6) | (dt_u32() & 8));
                    const uint8_t hi = (uint8_t)(pick(6) | (dt_u32() & 8));
                    info[3 + r * 3 + b] = (uint8_t)(lo | hi << 4);
                }
            }
            dtGame.goalInfo[p][l] = info;
        }
    }
}

static void *randomMarker(void)
{
    uint8_t *m = dt_alloc(0x200);
    dt_fill(m, 0x200);
    for (int i = 0; i < 0x100; i += 4) {
        float f = dt_float_edgy(-50.0f, 50.0f);
        memcpy(m + i, &f, 4);
    }
    int16_t angles[8];
    for (int i = 0; i < 8; i++) {
        angles[i] = (int16_t)dt_u32();
        memcpy(m + 2 + i * 2, &angles[i], 2);
    }
    uint16_t id = (uint16_t)dt_u32();
    memcpy(m, &id, 2);
    return m;
}

static void randomState(void)
{
    gGameOptions = (uint8_t)dt_u32();
    gGameMode = (uint8_t)((dt_u32() & 1) ? 0 : (dt_u32() & 7) == 0 ? dt_u32() : pick(12));
    gGameSubMode = (uint8_t)((dt_u32() & 3) ? pick(3) : dt_u32());
    gGameRunCount = (uint8_t)pick(4);
    gGameRunType = (uint8_t)((dt_u32() & 1) ? 4 : pick(8));
    gGamePhase = (uint8_t)pick(7);
    gGamePhaseTicks = (uint8_t)((dt_u32() & 1) ? 0x3b + pick(2) : pick(256));
    gGameStartPlayer = (uint8_t)pick(4);
    gGameTimer = (dt_u32() & 3) ? pick(180) : ((dt_u32() & 1) ? 0 : dt_u32());
    gGameFrozen = pick(2);
    gGameFrame = dt_u32();
    gGameReplayAll = (uint8_t)dt_u32();
    gGameReplayShared = (uint8_t)dt_u32();
    for (int i = 0; i < 4; i++) {
        gGameReplays[i] = (uint8_t)dt_u32();
    }
    for (int i = 0; i < 3; i++) {
        gGameStartPos[i] = dt_float_edgy(-500.0f, 500.0f);
    }
    dt_fill(gGameRecords, 0x300);
    dt_fill(gGameRoundStates, sizeof(GameRoundStates));
    dt_fill(gGameHorseWord, sizeof(GameHorseWord));
    gGameDefaultWord = dt_u32();
    for (int i = 0; i < 6; i++) {
        gGameMarkersA[i] = (i < 3 && (dt_u32() & 3) != 0) ? randomMarker() : NULL;
        gGameMarkersB[i] = (i < 3 && (dt_u32() & 3) != 0) ? randomMarker() : NULL;
        if (gGameMarkersA[i] == NULL) {
            for (int j = i; j < 6; j++) {
                gGameMarkersA[j] = NULL;
            }
        }
    }
    for (int i = 0; i < 6; i++) {
        if (gGameMarkersA[i] != NULL) {
            void *edge = (dt_u32() & 1) ? randomMarker() : NULL;
            memcpy((uint8_t *)gGameMarkersA[i] + 0x2c, (const void *)&edge, 4);
        }
    }
    for (int i = 0; i < 3; i++) {
        gGameMarkerColors[i] = dt_u32() & 0xffffff;
    }
    randomGoals();
    dt_fill(gGameRiders, sizeof(GameRiderDataTable));
    for (int i = 0; i < 12; i++) {
        gGameRiders[i].flags &= (dt_u32() & 1) ? 0xffffffffu : 0xffffff3fu;
        for (int k = 0; k < 12; k++) {
            gGameRiders[i].levelDone[k] = (uint8_t)pick(5);
        }
        gGameRiders[i].levelsA = (uint8_t)pick(8);
        gGameRiders[i].levelsB = (uint8_t)pick(8);
        gGameRiders[i].levelsC = (uint8_t)pick(8);
    }
    gGameUnlockBits = (uint16_t)dt_u32();
    gCurrentLevel = (uint8_t)((dt_u32() & 7) == 0 ? pick(12) : pick(6));
    gCurrentPlayer = (uint8_t)pick(2);
    gPlayerCount = (uint8_t)pick(3);
    for (int i = 0; i < 2; i++) {
        gProfiles[i].character = (uint8_t)pick(12);
        gProfiles[i].controller = (uint8_t)pick(2);
        gProfiles[i].pad02 = (uint8_t)pick(2);
        gProfiles[i].flags = (uint8_t)dt_u32();
    }
    for (int i = 0; i < 4; i++) {
        gSoundVolumes[i] = (uint8_t)pick(12);
    }
    for (int i = 0; i < 3; i++) {
        gCareerUnlocked[i] = (uint8_t)((dt_u32() & 1) ? pick(12) : 5);
    }
    for (int i = 0; i < 0x40; i++) {
        gGoalEntriesA[i][0] = (uint8_t)pick(12);
        gGoalEntriesA[i][1] = (uint8_t)((dt_u32() & 7) == 0 ? pick(30) : pick(12));
        gGoalEntriesB[i][0] = (uint8_t)pick(12);
        gGoalEntriesB[i][1] = (uint8_t)((dt_u32() & 7) == 0 ? pick(30) : pick(12));
        gGoalEntriesD[i][0] = (uint8_t)pick(12);
        gGoalEntriesD[i][1] = (uint8_t)((dt_u32() & 7) == 0 ? pick(30) : pick(12));
    }
    gPhysSyncFlags = (uint8_t)dt_u32();
    for (int i = 0; i < 2; i++) { /* only two riders exist: the array of four overlaps the profile tables */
        dtGameRandomRider(&gPhysRiders[i]);
        gPhysRiders[i].player = (uint8_t)i;
    }
}

static void randomScript(void)
{
    memset(&dtGame, 0, sizeof dtGame);
    dt_fill(dtGame.pressed, sizeof dtGame.pressed);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            static const float vals[] = {0.0f, 1.0f, -1.0f, 0.95f, 0.96f, -0.5f, -0.6f, 0.25f, 0.2f};
            dtGame.axes[i][j] =
                (dt_u32() & 1) ? vals[pick(sizeof vals / sizeof vals[0])] : dt_float_edgy(-1.0f, 1.0f);
        }
    }
    dtGame.playback = pick(4) == 0;
    dtGame.viewPlayer = pick(2);
    dtGame.physState = (dt_u32() & 3) ? pick(8) : 0;
    dtGame.physSetResult = pick(3) == 0 ? 1 + pick(8) : 0;
    dtGame.replayFailAt = pick(6) == 0 ? 1 + pick(4) : 0;
    dtGame.propMissing = pick(3) == 0;
    dtGame.recordMissing = pick(8) == 0;
    dtGame.asinScale = (dt_u32() & 1) ? 0x6000 : dt_float(0x1000, 0x8000);
    dtGame.acosScale = (dt_u32() & 1) ? 0x4000 : dt_float(0x1000, 0x8000);
    dtGame.asinResult = pick(4) == 0 ? 0x10000u + (dt_u32() & 0x7fff) : 0;
    dtGame.acosResult = pick(4) == 0 ? 0x10000u + (dt_u32() & 0x7fff) : 0;
    dtGame.actorNull = pick(6) == 0 ? 1 + pick(3) : 0;
    uint8_t *node = dt_alloc(0x80);
    float *target = dt_alloc(0x40);
    for (int i = 0; i < 16; i++) {
        target[i] = dt_float_edgy(-100.0f, 100.0f);
    }
    memcpy(node + 0x30, (const void *)&target, 4);
    dtGame.node = node; /* the original dereferences the node before checking it */
    uint8_t *object = dt_alloc(0x40);
    dt_fill(object, 0x40);
    object[0] = (uint8_t)pick(6);
    for (int i = 4; i < 0x14; i += 4) {
        float f = dt_float_edgy(-30.0f, 30.0f);
        memcpy(object + i, &f, 4);
    }
    dtGame.object = object;
    uint8_t *info = dt_alloc(0x10);
    dt_fill(info, 0x10);
    dtGame.objectInfo = info;
    dtGame.bounds = dt_alloc(0x20);
    for (int i = 0; i < 8; i++) {
        dtGame.bounds[i] = dt_float_edgy(-2000.0f, 2000.0f);
    }
    dtGame.rollerRecord = dt_alloc(0x10);
    dt_fill(dtGame.rollerRecord, 0x10);
    dtGame.levelRecord = dt_alloc(0x10);
    dt_fill(dtGame.levelRecord, 0x10);
    randomGoalInfos();
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 0x40; j += 4) {
            static const float specials[] = {0.0f, 1.0f, -1.0f, 1e10f, -1e10f, 0.5f, 0.999f, -0.999f};
            const float f = (dt_u32() & 7) == 0 ? specials[pick(8)] : dt_float_edgy(-1.2f, 1.2f);
            memcpy(&dtGame.padRecords[i][j], &f, 4);
        }
    }
    dtGame.stepScript = dt_u32();
    dt_fill(dtGame.statusMsg, sizeof dtGame.statusMsg);
    if (dt_u32() & 1) {
        dtGame.statusMsg[0] &=
            0xffffu; /* timer field (bits 16..31 of the word) cleared: no message showing */
    }
    dtGame.startPos = (ZVec3){dt_float_edgy(-100.0f, 100.0f), dt_float_edgy(-100.0f, 100.0f),
                              dt_float_edgy(-100.0f, 100.0f)};
    dtGame.startRot = (ZAngle3){(ZAngle)dt_u32(), (ZAngle)dt_u32(), (ZAngle)dt_u32()};
}

void dtGameBegin(const DtCtx *ctx, uint32_t groups)
{
    dt_seed(ctx);
    dtGameLogN = 0;
    randomScript();
    randomState();
    installStubs(groups);
}
