/*
 * Differential tests of rider.c: creation and reset, the state level bookkeeping, the mode steps and their
 * sub states, trick lookups and the collision callback. Everything the code calls outside rider.c is
 * stubbed (dt_game.c); the step tables point at one logging stub so no original state code runs.
 */
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/rider.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_game.h"

static PhysRider *anyRider(void)
{
    return &gPhysRiders[dt_range(0, 1)];
}

/* A rider with state levels in the ranges the sub states handle. */
static PhysRider *stateRider(int roll)
{
    PhysRider *r = anyRider();
    r->stateLevel[1] = (uint8_t)(roll ? dt_range(0, 8) : 0);
    r->stateLevel[2] = (uint8_t)((dt_u32() & 7) == 0 ? dt_range(0, 255) : dt_range(0, 5));
    if ((dt_u32() & 3) != 0) {
        r->stateTimer[2] = (int16_t)dt_range(0, 40);
        r->stateTimer[1] = (int16_t)dt_range(0, 40);
    }
    r->dirCount = (uint8_t)dt_range(0, 4);
    return r;
}

DTGAME_SETUP(setupNone, 0)

static void setupReadPads(DtCtx *c)
{
    dtGameBegin(c, DTG_PAD);
    gPlayerCount = (uint8_t)dt_range(0, 2);
}

static void setupCreate(DtCtx *c)
{
    dtGameBegin(c, DTG_ROLLER | DTG_ZOVER | DTG_DEBUG | DTG_COMDATA);
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_INT(c, 1, dt_range(0, 11));
    DT_ARG_INT(c, 2, dt_range(0, 3));
    DT_ARG_INT(c, 3, (dt_u32() & 3) == 0 ? (uint32_t)dt_range(0, 0x30) : (uint32_t)dt_range(0, 5));
}

static void setupReset(DtCtx *c)
{
    dtGameBegin(c, DTG_ROLLER | DTG_MATH | DTG_ZOVER | DTG_PHYS);
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_INT(c, 1, dt_u32() & 1);
}

static void setupSetLevel(DtCtx *c)
{
    dtGameBegin(c, 0);
    DT_ARG_PTR(c, 0, anyRider());
    DT_ARG_INT(c, 1, dt_range(0, 3));
    DT_ARG_INT(c, 2, (dt_u32() & 7) == 0 ? dt_u32() : (uint32_t)dt_range(0, 7));
}

static void setupGetLevel(DtCtx *c)
{
    dtGameBegin(c, DTG_DEBUG);
    DT_ARG_PTR(c, 0, anyRider());
    DT_ARG_INT(c, 1, (dt_u32() & 7) == 0 ? dt_range(0, 9) : dt_range(0, 3));
}

static void setupAddScore(DtCtx *c)
{
    dtGameBegin(c, 0);
    PhysRider *r = anyRider();
    r->field58 = (int32_t)dt_u32();
    DT_ARG_PTR(c, 0, r);
    DT_ARG_INT(c, 1, (dt_u32() & 3) == 0 ? dt_range(0, 15) : (dt_u32() & 1) ? 13 : 0);
    DT_ARG_FLOAT(c, 2, (dt_u32() & 15) == 0 ? dt_float_edgy(-1e12f, 1e12f) : dt_float_edgy(-40.0f, 200.0f));
}

static void setupPlain(DtCtx *c)
{
    dtGameBegin(c, 0);
    PhysRider *r = anyRider();
    r->flags &= (dt_u32() & 1) ? 0xffffffffu : ~0x10u;
    DT_ARG_PTR(c, 0, r);
    DT_ARG_INT(c, 1, dt_u32() & 1);
}

static void setupTrick(DtCtx *c)
{
    dtGameBegin(c, 0);
    PhysRider *r = anyRider();
    r->dirCount = (uint8_t)dt_range(0, 4);
    for (int i = 0; i < 4; i++) {
        r->dirList[i] = (uint8_t)((dt_u32() & 7) == 0 ? dt_range(0, 11) : dt_range(0, 8));
    }
    if ((dt_u32() & 1) != 0) {
        r->inputBits = dt_u32() & 0xf;
    }
    DT_ARG_PTR(c, 0, r);
}

/* A step function: pad, physics state, steering and the sub state table. */
static void setupStep(DtCtx *c)
{
    dtGameBegin(c, DTG_PAD | DTG_PHYS | DTG_MATH);
    dtGameInstallStepTables();
    PhysRider *r = stateRider(1);
    if ((dt_u32() & 1) != 0) {
        r->flags |= 0x400000;
    }
    r->speed = (dt_u32() & 3) == 0 ? dt_float_edgy(-5.0f, 5.0f) : dt_float_edgy(0.0f, 60.0f);
    r->padIndex = (uint8_t)dt_range(0, 1);
    if ((dt_u32() & 3) == 0) {
        r->stateLevel[1] = 1;
        r->stateTimer[1] = (int16_t)dt_range(5, 8);
    }
    if ((dt_u32() & 1) != 0) {
        /* no steering: the yaw settles back, and snaps to zero below a small threshold */
        dtGame.axes[0][0] = 0.0f;
        r->rot.y = (ZAngle)dt_range(-200, 200);
    }
    DT_ARG_PTR(c, 0, r);
}

static void setupStepSingle(DtCtx *c)
{
    setupStep(c);
    ((PhysRider *)c->args[0])->stateLevel[1] = 0;
}

/* A sub state: the pad, the angles, physics and the game's trick hook. */
static void setupSub(DtCtx *c)
{
    dtGameBegin(c, DTG_PAD | DTG_PHYS | DTG_MATH | DTG_GAMEFN);
    PhysRider *r = stateRider(0);
    if ((dt_u32() & 1) != 0) {
        r->flags |= 0x200;
    }
    r->padIndex = (uint8_t)dt_range(0, 2);
    r->speed = (dt_u32() & 7) == 0 ? 0.0f : dt_float_edgy(-5.0f, 60.0f);
    r->rot.y = (dt_u32() & 3) == 0 ? 0 : r->rot.y;
    r->stat[3] = dt_float_edgy(0.5f, 2.0f);
    DT_ARG_PTR(c, 0, r);
}

static void setupCollide(DtCtx *c)
{
    dtGameBegin(c, DTG_DEBUG | DTG_SOUND | DTG_PAD | DTG_DRAW | DTG_MATH | DTG_FX | DTG_ZOVER);
    const uint32_t hitSize = 0x48;
    uint32_t *hit = dt_alloc(hitSize * 4);
    dt_fill(hit, hitSize * 4);
    hit[0] = (dt_u32() & 3) == 0 ? dt_u32() : (dt_u32() & 3);
    uint32_t *flagsWord = dt_alloc(8);
    flagsWord[0] = dt_u32();
    hit[0xe] =
        ((hit[0] & 1) != 0 && (dt_u32() & 7) == 0) ? 0 : (uint32_t)(uintptr_t)flagsWord; /* kind 6 reads it */
    hit[0xf] = dt_u32();
    uint8_t *holder = dt_alloc(0x200);
    uint8_t *node = dt_alloc(0x40);
    dt_fill(node, 0x40);
    for (int i = 0x1c; i < 0x28; i += 4) {
        float f = dt_float_edgy(-5.0f, 5.0f);
        memcpy(node + i, &f, 4);
    }
    const uint32_t nodePtr = (dt_u32() & 3) == 0 ? 0 : (uint32_t)(uintptr_t)node;
    memcpy(holder + 0x1ec, &nodePtr, 4);
    hit[0x10] = (uint32_t)(uintptr_t)holder;
    for (int i = 7; i < 10; i++) {
        float f = dt_float_edgy(-10.0f, 10.0f);
        memcpy(&hit[i], &f, 4);
    }
    uint8_t *object = dtGame.object;
    object[2] = (uint8_t)dt_range(0, 12);
    uint8_t *info = dtGame.objectInfo;
    info[2] = (uint8_t)((dt_u32() & 1) ? dt_range(4, 11) : dt_range(0, 13));
    PhysRider *r = anyRider();
    r->actor2 = dt_alloc(0x88 + 17 * 0xb4 + 0x20);
    dt_fill(r->actor2, 0x88 + 17 * 0xb4 + 0x20);
    r->soundCooldown = (uint8_t)((dt_u32() & 1) ? 0 : dt_range(1, 45));
    float impact = (dt_u32() & 7) == 0 ? dt_float_edgy(-5.0f, 8.0f) : dt_float_edgy(0.0f, 70.0f);
    if ((dt_u32() & 7) == 0) {
        static const float specials[] = {5.0f, 10.0f, 25.0f, 50.0f, 20.0f, 30.0f, 4.99f, 5.01f};
        impact = specials[dt_range(0, 7)];
    }
    DT_ARG_INT(c, 0, (uint32_t)(uintptr_t)r->actor2);
    DT_ARG_INT(c, 1,
               (dt_u32() & 7) == 0 ? dt_range(0, 16)
               : (dt_u32() & 1)    ? dt_range(0, 2)
                                   : dt_range(5, 13));
    DT_ARG_FLOAT(c, 2, impact);
    DT_ARG_PTR(c, 3, hit);
}

DT_CASES(rider_cases) = {
    {"riderNop0", 0x00430e30, riderNop0, 0, DT_RET_VOID, setupNone, 1, 0},
    {"riderNop1", 0x00430e50, riderNop1, 0, DT_RET_VOID, setupNone, 1, 0},
    {"riderReadPads", 0x00430e70, riderReadPads, 0, DT_RET_VOID, setupReadPads, 400, 0},
    {"riderCreate", 0x004316c0, riderCreate, 4, DT_RET_INT, setupCreate, 400, 0},
    {"riderReset", 0x00431aa0, riderReset, 2, DT_RET_VOID, setupReset, 300, 0},
    {"riderNop2", 0x00431c80, riderNop2, 0, DT_RET_VOID, setupNone, 1, 0},
    {"riderSetStateLevel", 0x00431ca0, riderSetStateLevel, 3, DT_RET_INT, setupSetLevel, 600, 0},
    {"riderGetStateLevel", 0x00431dd0, riderGetStateLevel, 2, DT_RET_INT, setupGetLevel, 200, 0},
    {"riderAddScore", 0x00431e10, riderAddScore, 3, DT_RET_VOID, setupAddScore, 600, 0},
    {"riderLatchDirection", 0x00435db0, riderLatchDirection, 2, DT_RET_VOID, setupPlain, 400, 0},
    {"riderStickDirection", 0x00435f90, riderStickDirection, 1, DT_RET_INT, setupTrick, 400, 0},
    {"riderGetTrickA", 0x00435ea0, riderGetTrickA, 1, DT_RET_INT, setupTrick, 400, 0},
    {"riderGetTrickB", 0x00435ef0, riderGetTrickB, 1, DT_RET_INT, setupTrick, 400, 0},
    {"riderGetTrickKind", 0x00435f40, riderGetTrickKind, 1, DT_RET_INT, setupTrick, 400, 0},
    {"riderStepRoll", 0x00431ea0, riderStepRoll, 1, DT_RET_INT, setupStep, 1500, 0},
    {"riderStepGrind", 0x00432d50, riderStepGrind, 1, DT_RET_INT, setupStepSingle, 400, 0},
    {"riderStepLip", 0x00432e40, riderStepLip, 1, DT_RET_INT, setupStepSingle, 400, 0},
    {"riderStepManual", 0x00432f30, riderStepManual, 1, DT_RET_INT, setupStepSingle, 1000, 0},
    {"riderStepMode5", 0x004331d0, riderStepMode5, 1, DT_RET_INT, setupStep, 1000, 0},
    {"riderRollSub0", 0x004339d0, riderRollSub0, 1, DT_RET_INT, setupSub, 1500, 0},
    {"riderRollSub1", 0x00433d10, riderRollSub1, 1, DT_RET_INT, setupSub, 1200, 0},
    {"riderRollSub2", 0x00433f90, riderRollSub2, 1, DT_RET_INT, setupSub, 600, 0},
    {"riderRollSub3", 0x004340a0, riderRollSub3, 1, DT_RET_INT, setupSub, 1200, 0},
    {"riderRollSub4", 0x004342f0, riderRollSub4, 1, DT_RET_INT, setupSub, 1500, 0},
    {"riderRollSub5", 0x004346a0, riderRollSub5, 1, DT_RET_INT, setupSub, 5, 0},
    {"riderRollSub6", 0x004346c0, riderRollSub6, 1, DT_RET_INT, setupSub, 300, 0},
    {"riderRollSub8", 0x004350f0, riderRollSub8, 1, DT_RET_INT, setupSub, 1500, 0},
    {"riderGrindSub0", 0x004353a0, riderGrindSub0, 1, DT_RET_INT, setupSub, 1500, 0},
    {"riderLipSub0", 0x00435790, riderLipSub0, 1, DT_RET_INT, setupSub, 1500, 0},
    {"riderManualSub0", 0x00435af0, riderManualSub0, 1, DT_RET_INT, setupSub, 1500, 0},
    {"riderCollide", 0x00436020, riderCollide, 4, DT_RET_VOID, setupCollide, 1500, 0},
};
