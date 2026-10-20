/*
 * Differential tests of game.c: session state, records, run phases, goal bookkeeping, goal markers and the
 * effect callbacks. Callees outside the function under test are stubbed (dt_game.c); ported helpers of the
 * same file run natively on both sides.
 */
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_game.h"

static uint8_t *randomBytes(uint32_t size)
{
    uint8_t *p = dt_alloc(size);
    dt_fill(p, size);
    return p;
}

static char *randomWord(int maxLen)
{
    const int len = dt_range(0, maxLen);
    char *s = dt_alloc((uint32_t)len + 1);
    for (int i = 0; i < len; i++) {
        s[i] = (char)('a' + dt_range(0, 25));
    }
    return s;
}

/* A key for gameGetRecord: every table kind, in range and not. */
static uint32_t randomKey(void)
{
    const uint32_t type = (dt_u32() & 15) == 0 ? dt_u32() & 0xff : (uint32_t)dt_range(0, 12);
    const uint32_t player = (uint32_t)dt_range(0, 3);
    const uint32_t group = (uint32_t)dt_range(0, 15);
    const uint32_t slot = (dt_u32() & 3) == 0 ? (uint32_t)dt_range(0, 63) : (uint32_t)dt_range(0, 1);
    const uint32_t low = (dt_u32() & 7) == 0 ? dt_u32() & 0xfff : (uint32_t)dt_range(0, 23);
    return type << 24 | player << 22 | slot << 16 | group << 12 | low;
}

/* ---- accessors ----------------------------------------------------------------------------------- */

static void setupMask(DtCtx *c)
{
    dtGameBegin(c, 0);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setupSetOption(DtCtx *c)
{
    dtGameBegin(c, 0);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32() & 1 ? dt_u32() : 0);
}

DTGAME_SETUP(setupNone, 0)

static void setupStartMode(DtCtx *c)
{
    dtGameBegin(c, DTG_GAMEFN);
    DT_ARG_INT(c, 0, (dt_u32() & 7) == 0 ? dt_range(-4, 40) : dt_range(0, 12));
}

static void setupSubMode(DtCtx *c)
{
    dtGameBegin(c, 0);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setupRound(DtCtx *c)
{
    dtGameBegin(c, DTG_DEBUG);
    DT_ARG_INT(c, 0, (dt_u32() & 7) == 0 ? dt_u32() : (uint32_t)dt_range(0, 12));
    DT_ARG_INT(c, 1, (dt_u32() & 3) == 0 ? (uint32_t)dt_range(0, 1) : dt_u32() & 3);
}

static void setupKey(DtCtx *c)
{
    dtGameBegin(c, 0);
    DT_ARG_INT(c, 0, randomKey());
}

static void setupHorse(DtCtx *c)
{
    dtGameBegin(c, DTG_DEBUG);
    DT_ARG_PTR(c, 0, randomWord(10));
}

static void setupLeaveRun(DtCtx *c)
{
    dtGameBegin(c, DTG_OVL | DTG_COMDATA | DTG_GAMEFN | DTG_SOUND);
}

static void setupInit(DtCtx *c)
{
    dtGameBegin(c, DTG_REPLAY | DTG_ROLLER | DTG_DEBUG | DTG_COMDATA);
    gPlayerCount = (uint8_t)dt_range(0, 2);
}

static void setupShutdown(DtCtx *c)
{
    dtGameBegin(c, DTG_REPLAY);
    gPlayerCount = (uint8_t)dt_range(0, 4);
}

static void setupTick(DtCtx *c)
{
    dtGameBegin(c, DTG_OVL | DTG_GAMEFN | DTG_COMDATA | DTG_SOUND | DTG_PHYS | DTG_ROLLER);
    static const uint8_t modes[] = {2, 3, 4, 5, 6, 11, 1};
    gGameMode = (uint8_t)((dt_u32() & 1) ? 0 : modes[dt_range(0, 6)]);
    gGameRunType = (uint8_t)((dt_u32() & 7) != 0 ? 4 : dt_range(0, 7));
    gGamePhase = (uint8_t)dt_range(0, 5);
    if ((dt_u32() & 3) == 0) {
        gGamePhase = 4;
        gGamePhaseTicks = (uint8_t)(0x3a + dt_range(0, 2));
    }
    if ((dt_u32() & 1) == 0) {
        gGameTimer = (uint32_t)dt_range(0, 11) * 15 + (uint32_t)dt_range(0, 1);
        gGameFrozen = 0;
        dtGame.playback = 0;
    }
}

/* ---- goals --------------------------------------------------------------------------------------- */

/* Makes goal 0 of row 0 of the current level and player a goal of the given type and status. */
static void plantGoal(int player, uint16_t type, uint16_t min, uint16_t max, uint8_t status)
{
    uint8_t *info = dtGame.goalInfo[player & 3][gCurrentLevel % 12];
    info[1] = (uint8_t)dt_range(0, 2);
    GameGoalRow *row = &gGameGoals[gCurrentLevel * 3 + info[1]][0];
    row->count = 1;
    row->goal[0].type = type;
    row->goal[0].min = min;
    row->goal[0].max = max;
    info[3 + info[1] * 12] &= 0x7f;
    info[4 + info[1] * 12] = (uint8_t)((info[4 + info[1] * 12] & 0xf0) | status);
}

static void setupGoalStatus(DtCtx *c)
{
    dtGameBegin(c, DTG_COMDATA | DTG_DEBUG);
    DT_ARG_INT(c, 0, dt_range(0, 3));
    DT_ARG_INT(c, 1, dt_range(0, 3));
    DT_ARG_INT(c, 2, (dt_u32() & 7) == 0 ? dt_u32() & 7 : (uint32_t)dt_range(0, 3));
}

static void setupSetGoalStatus(DtCtx *c)
{
    dtGameBegin(c, DTG_COMDATA | DTG_DEBUG | DTG_PROP);
    DT_ARG_INT(c, 0, dt_range(0, 3));
    DT_ARG_INT(c, 1, dt_range(0, 3));
    DT_ARG_INT(c, 2, (dt_u32() & 7) == 0 ? dt_u32() & 7 : (uint32_t)dt_range(0, 3));
    DT_ARG_INT(c, 3, (dt_u32() & 3) == 0 ? dt_range(0, 7) : dt_range(4, 5));
}

static void setupGoalFlag(DtCtx *c)
{
    dtGameBegin(c, DTG_COMDATA | DTG_DEBUG);
    DT_ARG_INT(c, 0, dt_range(0, 3));
    DT_ARG_INT(c, 1, dt_range(0, 3));
    DT_ARG_INT(c, 2, (dt_u32() & 7) == 0 ? dt_u32() & 7 : (uint32_t)dt_range(0, 3));
    DT_ARG_INT(c, 3, dt_u32());
}

static void setupSetGoalFlag(DtCtx *c)
{
    dtGameBegin(c, DTG_COMDATA | DTG_DEBUG);
    DT_ARG_INT(c, 0, dt_range(0, 3));
    DT_ARG_INT(c, 1, dt_range(0, 3));
    DT_ARG_INT(c, 2, (dt_u32() & 7) == 0 ? dt_u32() & 7 : (uint32_t)dt_range(0, 3));
    DT_ARG_INT(c, 3, (dt_u32() & 1) ? dt_u32() : 1);
    DT_ARG_INT(c, 4, dt_u32() & 1);
}

static void setupResetGoals(DtCtx *c)
{
    dtGameBegin(c, DTG_COMDATA | DTG_PROP);
    gCurrentLevel = (uint8_t)dt_range(0, 5);
    DT_ARG_INT(c, 0, dt_range(0, 3));
    if ((dt_u32() & 1) != 0) {
        /* every row locked: the set counter runs on to the end */
        uint8_t *info = dtGame.goalInfo[c->args[0] & 3][gCurrentLevel];
        info[1] = (uint8_t)dt_range(0, 2);
        for (int r = 0; r < 12; r++) {
            info[3 + r * 3] |= 0x80;
        }
    }
}

static void setupPlayerGoal(DtCtx *c)
{
    dtGameBegin(c, DTG_COMDATA);
    if ((dt_u32() & 3) != 0) {
        gGameMode = 0;
        gGameSubMode = 0;
    }
    DT_ARG_INT(c, 0, dt_range(0, 3));
}

static void setupPropGoal(DtCtx *c)
{
    dtGameBegin(c, DTG_COMDATA | DTG_PROP | DTG_FX | DTG_DEBUG);
    if ((dt_u32() & 7) != 0) {
        gGameMode = 0;
        gGameSubMode = 0;
    }
    uint8_t *prop = randomBytes(0x84);
    uint8_t *info = randomBytes(0x10);
    memcpy(prop + 8, (const void *)&info, 4);
    DT_ARG_INT(c, 0, dt_range(0, 3));
    DT_ARG_PTR(c, 1, prop);
    uint16_t kind = (uint16_t)(dt_u32() & 0x1f);
    memcpy(info + 2, &kind, 2);
}

static void setupGrindGoal(DtCtx *c)
{
    dtGameBegin(c, DTG_COMDATA | DTG_DEBUG);
    if ((dt_u32() & 7) != 0) {
        gGameMode = 0;
        gGameSubMode = 0;
    }
    PhysEdge *edge = (PhysEdge *)randomBytes(sizeof(PhysEdge));
    edge->tag = (uint16_t)(dt_u32() & 1 ? dt_range(0, 12) : (int32_t)dt_u32());
    DT_ARG_INT(c, 0, dt_range(0, 3));
    DT_ARG_PTR(c, 1, edge);
}

static void setupSubmitScore(DtCtx *c)
{
    dtGameBegin(c, DTG_OVL | DTG_ROLLER | DTG_COMDATA | DTG_SOUND | DTG_DEBUG);
    if ((dt_u32() & 3) != 0) {
        gGameMode = 0;
    }
    DT_ARG_INT(c, 0, dt_range(0, 3));
    DT_ARG_INT(c, 1, (dt_u32() & 7) == 0 ? dt_range(0, 20) : dt_range(0, 5));
    DT_ARG_INT(c, 2, (dt_u32() & 7) == 0 ? dt_u32() : (uint32_t)dt_range(0, 400));
    DT_ARG_INT(c, 3, dt_u32());
    DT_ARG_INT(c, 4, dt_u32());
    if (gCurrentLevel < 6 && (dt_u32() & 1) != 0) {
        gGameMode = 0;
        gGameSubMode = 0;
        plantGoal((int)c->args[0], 0x14, (uint16_t)c->args[1], (uint16_t)dt_range(0, 400),
                  (uint8_t)dt_range(0, 3));
    }
    dtGame.rollerRecord[7] = (uint8_t)dt_range(0, 5);
    if ((dt_u32() & 1) != 0) {
        /* make the stored record low so the new value beats it */
        const uint32_t mode = gGameMode;
        const uint32_t sub = gGameSubMode;
        const uint32_t kind = c->args[1];
        for (uint32_t slot = 0; slot < 4; slot++) {
            uint8_t *rec = gameGetRecord((((mode << 2 | sub) << 6 | kind) << 4 | slot) << 12);
            if (rec != NULL && (dt_u32() & 3) != 0) {
                *(uint32_t *)rec = (uint32_t)dt_range(0, 100);
            }
        }
    }
}

static void setupBusy(DtCtx *c)
{
    dtGameBegin(c, 0);
    PhysRider *r = &gPhysRiders[0];
    r->spin = (dt_u32() & 3) == 0 ? (int32_t)dt_u32() : dt_range(-0x9000, 0x9000);
    if ((dt_u32() & 3) == 0) {
        r->spin = (dt_u32() & 1 ? 1 : -1) * (0x4000 + dt_range(-1, 1));
        r->flags &= ~0x1000u;
    }
    if ((dt_u32() & 3) == 0) {
        r->anim = 0xff;
        r->ikPose = 0xff;
        r->flags &= ~0x80u;
    }
    DT_ARG_PTR(c, 0, r);
}

static void setupEvaluateGoals(DtCtx *c)
{
    dtGameBegin(c, DTG_COMDATA | DTG_PROP | DTG_DEBUG);
    if ((dt_u32() & 7) != 0) {
        gGameMode = 0;
        gGameSubMode = 0;
    }
    DT_ARG_INT(c, 0, dt_range(0, 3));
    DT_ARG_INT(c, 1, dt_u32() & 1);
    if (gCurrentLevel < 6 && (dt_u32() & 1) != 0) {
        static const uint16_t types[] = {0, 10, 11, 12, 5};
        gGameMode = 0;
        gGameSubMode = 0;
        plantGoal((int)c->args[0], types[dt_range(0, 4)], (uint16_t)dt_range(0, 12),
                  (uint16_t)dt_range(0, 12), 3);
        gPhysRiders[c->args[0] & 1].combo.count = (uint8_t)dt_range(0, 3);
        gPhysRiders[c->args[0] & 1].snapB.surface = 0xc;
    }
}

static void setupCompleteGoal(DtCtx *c)
{
    dtGameBegin(c, DTG_DEBUG);
    uint8_t *reward = dt_alloc(2);
    reward[0] = (uint8_t)((dt_u32() & 15) == 0 ? dt_u32() : (uint32_t)dt_range(0, 11));
    reward[1] = (uint8_t)((dt_u32() & 7) == 0 ? dt_u32() : (uint32_t)dt_range(0, 11));
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_PTR(c, 1, reward);
}

static void setupUnlocks(DtCtx *c)
{
    dtGameBegin(c, DTG_COMDATA | DTG_DEBUG);
    if ((dt_u32() & 7) != 0) {
        gGameMode = 0;
        gGameSubMode = 0;
    }
    gCurrentLevel = (uint8_t)((dt_u32() & 1) ? dt_range(6, 11) : dt_range(0, 5));
    DT_ARG_INT(c, 0, dt_range(0, 1));
    for (int p = 0; p < 4; p++) {
        for (int l = 0; l < 12; l++) {
            uint8_t *info = dtGame.goalInfo[p][l];
            if ((dt_u32() & 1) != 0) {
                info[2] = 4;
                info[1] = (uint8_t)dt_range(0, 2);
            }
        }
    }
    for (int i = 0; i < 12; i++) {
        if ((dt_u32() & 3) == 0) {
            gGameRiders[i].flags |= 2;
        }
    }
    if ((dt_u32() & 3) == 0) {
        /* everything finished up to the last set: the final unlocks fire */
        gGameMode = 0;
        gGameSubMode = 0;
        for (int p = 0; p < 4; p++) {
            for (int l = 0; l < 12; l++) {
                uint8_t *info = dtGame.goalInfo[p][l];
                info[1] = 2;
                info[2] = 4;
                info[3 + 2 * 12] |= 0x40;
            }
        }
        for (int i = 0; i < 12; i++) {
            for (int k = 0; k < 12; k++) {
                gGameRiders[i].levelDone[k] = 3;
            }
            if (i < 10) {
                gGameRiders[i].flags |= (dt_u32() & 7) == 0 ? 0 : 2;
            }
        }
        gProfiles[c->args[0] & 1].character = (uint8_t)dt_range(6, 11);
    }
}

static void setupResetRider(DtCtx *c)
{
    dtGameBegin(c, DTG_RIDERFN | DTG_PROP | DTG_ROLLER | DTG_MATH | DTG_PHYS | DTG_COMDATA);
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_INT(c, 1, dt_u32() & 1);
    if ((dt_u32() & 3) == 0) {
        gGameMode = 4;
    }
}

/* ---- trick scoring ------------------------------------------------------------------------------- */

static void setupCombo(DtCtx *c)
{
    dtGameBegin(c, DTG_OVL);
    PhysRider *r = &gPhysRiders[dt_range(0, 1)];
    r->combo.count = (uint8_t)dt_range(0, 8);
    for (int i = 0; i < 8; i++) {
        RiderTrick *t = &r->combo.trick[i];
        t->flags = (uint8_t)dt_u32();
        t->trick = (uint8_t)((dt_u32() & 7) == 0   ? 0xff
                             : (dt_u32() & 3) == 0 ? dt_range(0x17, 0x30)
                                                   : dt_range(0, 0x1a));
        t->style = (uint8_t)((dt_u32() & 7) == 0 ? 0xff : dt_range(0, 20));
        t->spin = (uint8_t)((dt_u32() & 7) == 0 ? dt_u32() : (uint32_t)dt_range(0, 7));
        t->tier = (uint8_t)((dt_u32() & 7) == 0 ? dt_u32() : (uint32_t)dt_range(0, 5));
        t->count = (int16_t)dt_range(0, 6);
        t->duration = dt_float_edgy(0.0f, 5.0f);
        if (i > 0 && (dt_u32() & 3) == 0) {
            t->trick = r->combo.trick[i - 1].trick;
            if ((dt_u32() & 1) != 0) {
                t->style = r->combo.trick[i - 1].style;
            }
        }
    }
    if ((dt_u32() & 3) == 0) {
        gGameMode = (uint8_t)((dt_u32() & 1) ? 3 : dt_range(5, 10));
    } else {
        gGameMode = (uint8_t)dt_range(0, 12);
    }
    DT_ARG_PTR(c, 0, &r->combo);
}

/* ---- markers ------------------------------------------------------------------------------------- */

static void *objectMarker(void)
{
    uint8_t *m = randomBytes(0x200);
    uint8_t *node = randomBytes(0x40);
    uint8_t *frame = randomBytes(0x40);
    for (int i = 0; i < 4; i++) {
        float f = dt_float_edgy(-1.0f, 1.0f);
        memcpy(frame + 0x14 + i * 4, &f, 4);
    }
    memcpy(node + 0x1c, (const void *)&frame, 4);
    memcpy(m + 0x1f0, (const void *)&node, 4);
    for (int i = 4; i < 0x10; i += 2) {
        int16_t v = (int16_t)dt_range(-300, 300);
        memcpy(m + i, &v, 2);
    }
    return m;
}

static void setupPlaneMarker(DtCtx *c)
{
    dtGameBegin(c, DTG_DRAW);
    uint8_t *m = randomBytes(0x80);
    for (int i = 0; i < 0x80; i += 4) {
        float f = dt_float_edgy(-100.0f, 100.0f);
        memcpy(m + i, &f, 4);
    }
    gGameFrame = dt_u32();
    DT_ARG_PTR(c, 0, m);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setupBoxMarker(DtCtx *c)
{
    dtGameBegin(c, DTG_DRAW | DTG_MATH);
    uint8_t *m = randomBytes(0x80);
    for (int i = 0x18; i < 0x30; i += 4) {
        float f = dt_float_edgy(-100.0f, 100.0f);
        memcpy(m + i, &f, 4);
    }
    gGameFrame = dt_u32();
    DT_ARG_PTR(c, 0, m);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setupObjectMarker(DtCtx *c)
{
    dtGameBegin(c, DTG_DRAW | DTG_MATH);
    gGameFrame = dt_u32();
    DT_ARG_PTR(c, 0, objectMarker());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setupGoalMarkers(DtCtx *c)
{
    dtGameBegin(c, DTG_DRAW | DTG_MATH | DTG_COMDATA | DTG_DEBUG);
    if ((dt_u32() & 7) != 0) {
        gGameMode = 0;
        gGameSubMode = 0;
    }
    dtGame.playback = (dt_u32() & 7) == 0;
    for (int i = 0; i < 3; i++) {
        gGameMarkersA[i] = (dt_u32() & 3) ? objectMarker() : NULL;
        gGameMarkersB[i] = (dt_u32() & 3) ? objectMarker() : NULL;
        if (gGameMarkersA[i] == NULL) {
            gGameMarkersA[i + 1] = NULL;
        }
        void *edge = (dt_u32() & 1) ? objectMarker() : NULL;
        if (gGameMarkersA[i] != NULL) {
            memcpy((uint8_t *)gGameMarkersA[i] + 0x2c, (const void *)&edge, 4);
        }
    }
    gGameMarkersA[3] = NULL;
    gGameMarkersB[3] = NULL;
    for (int p = 0; p < 4; p++) {
        for (int l = 0; l < 12; l++) {
            uint8_t *info = dtGame.goalInfo[p][l];
            for (int r = 0; r < 12; r++) {
                info[3 + r * 3] &= 0x7f;
            }
        }
    }
}

/* ---- effects ------------------------------------------------------------------------------------- */

static void setupSplash(DtCtx *c)
{
    dtGameBegin(c, DTG_FX);
    uint8_t *obj = randomBytes(0x40);
    float *offset = dt_alloc(12);
    for (int i = 0; i < 0x40; i += 4) {
        float f = dt_float_edgy(-500.0f, 500.0f);
        memcpy(obj + i, &f, 4);
    }
    for (int i = 0; i < 3; i++) {
        offset[i] = dt_float_edgy(-50.0f, 50.0f);
    }
    DT_ARG_PTR(c, 0, obj);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, offset);
}

static void setupEmit(DtCtx *c)
{
    dtGameBegin(c, DTG_FX);
    DT_ARG_PTR(c, 0, randomBytes(0x40));
    DT_ARG_INT(c, 1, dt_u32() & 7);
}

static void setupLight(DtCtx *c)
{
    dtGameBegin(c, DTG_FX);
    DT_ARG_PTR(c, 0, randomBytes(0x40));
    DT_ARG_INT(c, 1, dt_range(0, 2));
}

static void setupEffectList(DtCtx *c)
{
    dtGameBegin(c, DTG_FX | DTG_DEBUG);
    uint8_t *list = randomBytes(4 + 16 * 6);
    const uint32_t count = (uint32_t)dt_range(0, 6);
    for (uint32_t i = 0; i < count; i++) {
        list[4 + i * 16 + 6] = (dt_u32() & 3) ? 7 : (uint8_t)dt_u32();
    }
    DT_ARG_PTR(c, 0, list);
    DT_ARG_INT(c, 1, count);
}

DT_CASES(game_cases) = {
    {"gameTestOption", 0x0040a890, gameTestOption, 1, DT_RET_INT, setupMask, 200, 0},
    {"gameSetOption", 0x0040a850, gameSetOption, 2, DT_RET_VOID, setupSetOption, 200, 0},
    {"gameStartMode", 0x0040a8c0, gameStartMode, 1, DT_RET_INT, setupStartMode, 400, 0},
    {"gameGetMode", 0x0040a9f0, gameGetMode, 0, DT_RET_INT, setupNone, 20, 0},
    {"gameSetSubMode", 0x0040aa10, gameSetSubMode, 1, DT_RET_INT, setupSubMode, 100, 0},
    {"gameGetSubMode", 0x0040aa30, gameGetSubMode, 0, DT_RET_INT, setupNone, 20, 0},
    {"gameGetRunType", 0x0040bd50, gameGetRunType, 0, DT_RET_INT, setupNone, 20, 0},
    {"gameGetRoundCount", 0x0040bd70, gameGetRoundCount, 0, DT_RET_INT, setupNone, 300, 0},
    {"gameGetRunCount", 0x0040bdc0, gameGetRunCount, 0, DT_RET_INT, setupNone, 20, 0},
    {"gameGetStartPlayer", 0x0040bde0, gameGetStartPlayer, 0, DT_RET_INT, setupNone, 20, 0},
    {"gameGetRoundState", 0x0040be00, gameGetRoundState, 2, DT_RET_INT, setupRound, 400, 0},
    {"gameGetRecord", 0x0040be50, gameGetRecord, 1, DT_RET_INT, setupKey, 600, 0},
    {"gameSetHorseWord", 0x0040bfd0, gameSetHorseWord, 1, DT_RET_INT, setupHorse, 200, 0},
    {"gameGetNameTable", 0x0040c050, gameGetNameTable, 0, DT_RET_INT, setupNone, 10, 0},
    {"gameGetModeByte", 0x0040c070, gameGetModeByte, 0, DT_RET_INT8, setupNone, 100, 0},
    {"gameLeaveRun", 0x0040c0b0, gameLeaveRun, 0, DT_RET_VOID, setupLeaveRun, 300, 0},
    {"gameNop", 0x0040cf50, gameNop, 0, DT_RET_VOID, setupNone, 1, 0},
    {"gameInit", 0x004099c0, gameInit, 0, DT_RET_VOID, setupInit, 400, 0},
    {"gameShutdown", 0x00409cd0, gameShutdown, 0, DT_RET_VOID, setupShutdown, 100, 0},
    {"gameTick", 0x00409d20, gameTick, 0, DT_RET_VOID, setupTick, 1500, 0},
    {"gameGoalStatus", 0x004102c0, gameGoalStatus, 3, DT_RET_INT8, setupGoalStatus, 400, 0},
    {"gameSetGoalStatus", 0x00410350, gameSetGoalStatus, 4, DT_RET_VOID, setupSetGoalStatus, 500, 0},
    {"gameGoalFlag", 0x00410480, gameGoalFlag, 4, DT_RET_INT8, setupGoalFlag, 400, 0},
    {"gameSetGoalFlag", 0x00410510, gameSetGoalFlag, 5, DT_RET_VOID, setupSetGoalFlag, 400, 0},
    {"gameResetGoals", 0x0040cd40, gameResetGoals, 1, DT_RET_VOID, setupResetGoals, 400, 0},
    {"gameHasPendingUnlock", 0x0040cf70, gameHasPendingUnlock, 1, DT_RET_INT, setupPlayerGoal, 400, 0},
    {"gameCheckPropGoal", 0x0040ca60, gameCheckPropGoal, 2, DT_RET_INT, setupPropGoal, 500, 0},
    {"gameCheckGrindGoal", 0x0040cbe0, gameCheckGrindGoal, 2, DT_RET_INT, setupGrindGoal, 500, 0},
    {"gameSubmitScore", 0x0040e030, gameSubmitScore, 5, DT_RET_VOID, setupSubmitScore, 800, 0},
    {"gameRiderBusy", 0x0040e2f0, gameRiderBusy, 1, DT_RET_INT8, setupBusy, 300, 0},
    {"gameEvaluateGoals", 0x0040fc30, gameEvaluateGoals, 2, DT_RET_VOID, setupEvaluateGoals, 500, 0},
    {"gameCompleteGoal", 0x00411090, gameCompleteGoal, 2, DT_RET_VOID, setupCompleteGoal, 500, 0},
    {"gameUpdateUnlocks", 0x00410c60, gameUpdateUnlocks, 1, DT_RET_VOID, setupUnlocks, 1000, 0},
    {"gameResetRider", 0x0040c190, gameResetRider, 2, DT_RET_VOID, setupResetRider, 400, 0},
    {"gameScoreCombo", 0x0040fe40, gameScoreCombo, 1, DT_RET_VOID, setupCombo, 1000, 0},
    {"gameDrawPlaneMarker", 0x00411270, gameDrawPlaneMarker, 2, DT_RET_VOID, setupPlaneMarker, 300, 0},
    {"gameDrawBoxMarker", 0x00411440, gameDrawBoxMarker, 2, DT_RET_VOID, setupBoxMarker, 300, 0},
    {"gameDrawPropMarker", 0x004117b0, gameDrawPropMarker, 2, DT_RET_VOID, setupObjectMarker, 300, 0},
    {"gameDrawEdgeMarker", 0x00411c50, gameDrawEdgeMarker, 2, DT_RET_VOID, setupObjectMarker, 300, 0},
    {"gameDrawGoalMarkers", 0x0040a400, gameDrawGoalMarkers, 0, DT_RET_INT, setupGoalMarkers, 400, 0},
    {"fxWaterSplash", 0x004096d0, fxWaterSplash, 3, DT_RET_VOID, setupSplash, 200, 0},
    {"fxNop", 0x00409860, fxNop, 0, DT_RET_VOID, setupNone, 1, 0},
    {"fxCallbackEmit", 0x00409880, fxCallbackEmit, 2, DT_RET_VOID, setupEmit, 100, 0},
    {"fxCallbackLight", 0x004098b0, fxCallbackLight, 2, DT_RET_INT, setupLight, 50, 0},
    {"fxCreateFromList", 0x004098f0, fxCreateFromList, 2, DT_RET_INT, setupEffectList, 300, 0},
};
