/*
 * Differential tests of the big game.c functions: the run phase driver, combos, jumps, hints, goals and the
 * judges. The shared fixtures of dt_game.c randomise the session and stub the engine; the overlay element
 * stubs and the callees ported in game.c are replaced here by stubs that log only bytes the callers
 * initialise (the original leaves parts of its messages unset).
 */
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_game.h"

/* ---- log ---------------------------------------------------------------------------------------- */

#define LOG_SIZE 2048

__declspec(dllexport) uint32_t dtBigLog[LOG_SIZE];
__declspec(dllexport) uint32_t dtBigLogN;

static void lg(uint32_t v)
{
    if (dtBigLogN < LOG_SIZE) {
        dtBigLog[dtBigLogN] = v;
    }
    dtBigLogN++;
}

static uint32_t rd32(const void *p)
{
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}

static uint16_t rd16(const void *p)
{
    uint16_t v;
    memcpy(&v, p, 2);
    return v;
}

/* Scripted answers of the stubs below. */
static struct {
    uint32_t respawnCalls;
    uint32_t respawnAfter; /* physFindRespawnFace succeeds from this call on */
    void *list[3];         /* zbsp finders: NULL or a hit list */
    uint8_t face[0x88];
} gBig;

/* ---- stubs -------------------------------------------------------------------------------------- */

static void BMX_CDECL stubElemA(int index, const void *value)
{
    const uint8_t *b = value;
    lg(0x0e1a0000u + (uint32_t)index);
    switch (index) {
    case 1:
        lg(rd32(b));
        lg(b[4]);
        break;
    case 2:
    case 5:
    case 6:
    case 7:
        lg(rd32(b));
        break;
    case 8:
        lg(b[0]);
        break;
    case 10:
    case 11:
        lg(b[0]);
        if (b[0] != 0xff) {
            lg(rd16(b + 2));
        }
        break;
    case 12:
        lg(b[0]);
        if (b[0] == 0xb || b[0] == 0xc) {
            lg(rd32(b + 4));
        }
        break;
    default:
        break;
    }
}

/* Elements 10..12 report the message on screen: its timer word is zero most of the time. */
static void BMX_CDECL stubElemB(int index, void *value)
{
    lg(0x0e1b0000u + (uint32_t)index);
    uint32_t w = dt_u32();
    if ((w & 3) != 0) {
        w &= 0xffff;
    }
    if (index == 10 || index == 11) {
        memcpy(value, &w, 4);
    } else if (index == 12) {
        const uint32_t v = dt_u32();
        memcpy(value, &w, 4);
        memcpy((uint8_t *)value + 4, &v, 4);
    }
}

static void BMX_CDECL stubSubmit(int player, uint32_t kind, uint32_t value, uint32_t sound, uint32_t flags)
{
    lg(0xe0300000u);
    lg((uint32_t)player);
    lg(kind);
    lg(value);
    lg(sound);
    lg(flags);
}

static void BMX_CDECL stubEvaluate(int player, int finished)
{
    lg(0xfc300000u);
    lg((uint32_t)player);
    lg((uint32_t)finished);
}

static void BMX_CDECL stubScoreCombo(RiderCombo *c)
{
    lg(0xfe400000u);
    lg(dt_canon((uint32_t)(uintptr_t)c));
}

static void BMX_CDECL stubEndCombo(int player, int landed)
{
    lg(0xdae00000u);
    lg((uint32_t)player);
    lg((uint32_t)landed);
}

static void BMX_CDECL stubResetRider(int player, int full)
{
    lg(0xc1900000u);
    lg((uint32_t)player);
    lg((uint32_t)full);
}

static void BMX_CDECL stubUnlocks(int player)
{
    lg(0x0c600000u);
    lg((uint32_t)player);
}

static int BMX_CDECL stubSave(void)
{
    lg(0x23400000u);
    return 0;
}

static uint8_t BMX_CDECL stubReplayRider(uint32_t handle, uint32_t rider)
{
    lg(0x0bf00000u);
    lg(handle & 0xff);
    lg(rider & 0xff);
    return (uint8_t)rider;
}

static void BMX_CDECL stubReplayMode(uint32_t handle, uint32_t mode)
{
    lg(0x04100000u);
    lg(handle & 0xff);
    lg(mode);
}

static PhysFace *BMX_CDECL stubRespawn(const ZVec3 *pos, uint32_t flags)
{
    lg(0x00b00000u);
    lg(rd32(&pos->x));
    lg(rd32(&pos->y));
    lg(rd32(&pos->z));
    lg(flags);
    return ++gBig.respawnCalls >= gBig.respawnAfter ? (PhysFace *)(void *)gBig.face : NULL;
}

static void BMX_CDECL stubCentroid(const void *face, ZVec3 *out)
{
    lg(0x6c500000u);
    lg(dt_canon((uint32_t)(uintptr_t)face));
    out->x = 1.5f;
    out->y = -2.25f;
    out->z = 300.0f;
}

#define FINDER(name, id, slot)                                                                               \
    static void **BMX_CDECL name(int lo, int hi)                                                             \
    {                                                                                                        \
        lg((id));                                                                                            \
        lg((uint32_t)lo & 0xffff);                                                                           \
        lg((uint32_t)hi & 0xffff);                                                                           \
        return (void **)gBig.list[(slot)];                                                                   \
    }
FINDER(stubFaces, 0x8ef00000u, 0)
FINDER(stubEdges, 0x91c00000u, 1)
FINDER(stubProps, 0x8e300000u, 2)

enum {
    BIG_SUBMIT = 1u << 0,
    BIG_EVALUATE = 1u << 1,
    BIG_COMBO = 1u << 2,
    BIG_ENDCOMBO = 1u << 3,
    BIG_SESSION = 1u << 4, /* resets, unlocks, save, replays, respawn faces */
    BIG_FINDERS = 1u << 5,
};

/* Starts an iteration: shared fixtures, the overlay element stubs and the requested callee stubs. */
static void begin(DtCtx *c, uint32_t groups, uint32_t big)
{
    dtGameBegin(c, groups | DTG_OVL);
    dtBigLogN = 0;
    memset(&gBig, 0, sizeof gBig);
    dt_stub(0x00415370, NULL, (void *)stubElemA);
    dt_stub(0x004153a0, NULL, (void *)stubElemB);
    if ((big & BIG_SUBMIT) != 0) {
        dt_stub(0x0040e030, NULL, (void *)stubSubmit);
    }
    if ((big & BIG_EVALUATE) != 0) {
        dt_stub(0x0040fc30, NULL, (void *)stubEvaluate);
    }
    if ((big & BIG_COMBO) != 0) {
        dt_stub(0x0040fe40, NULL, (void *)stubScoreCombo);
    }
    if ((big & BIG_ENDCOMBO) != 0) {
        dt_stub(0x0040dae0, NULL, (void *)stubEndCombo);
    }
    if ((big & BIG_SESSION) != 0) {
        dt_stub(0x0040c190, NULL, (void *)stubResetRider);
        dt_stub(0x00410c60, NULL, (void *)stubUnlocks);
        dt_stub(0x00412340, NULL, (void *)stubSave);
        dt_stub(0x00430bf0, NULL, (void *)stubReplayRider);
        dt_stub(0x00430410, NULL, (void *)stubReplayMode);
        dt_stub(0x004200b0, NULL, (void *)stubRespawn);
        dt_stub(0x00466c50, NULL, (void *)stubCentroid);
        gBig.respawnAfter = (uint32_t)dt_range(1, 4);
    }
    if ((big & BIG_FINDERS) != 0) {
        dt_stub(0x00468ef0, NULL, (void *)stubFaces);
        dt_stub(0x004691c0, NULL, (void *)stubEdges);
        dt_stub(0x00468e30, NULL, (void *)stubProps);
        for (int i = 0; i < 3; i++) {
            if ((dt_u32() & 3) != 0) {
                void *list = dt_alloc(0x100);
                dt_fill(list, 0x100);
                gBig.list[i] = list;
            }
        }
    }
    gGameExtent[0] = (uint16_t)((dt_u32() & 7) == 0 ? dt_u32() | 1 : (uint32_t)dt_range(1, 1000));
    gGameExtent[1] = (uint16_t)((dt_u32() & 7) == 0 ? dt_u32() | 1 : (uint32_t)dt_range(1, 1000));
    for (int i = 0; i < 0x300; i += 8) {
        gGameRecords[i + 4] &= 1; /* rider indices in the records stay valid */
    }
}

/* A contact record of the given surface: face or edge with a tag, edges with or without a prop. */
static void *randomContact(uint8_t surface)
{
    uint8_t *rec = dt_alloc(0x40);
    dt_fill(rec, 0x40);
    const uint16_t tag = (uint16_t)((dt_u32() & 3) == 0 ? dt_u32() : (uint32_t)dt_range(0, 24));
    memcpy(rec + 0x16, &tag, 2);
    const uint16_t group = (uint16_t)((dt_u32() & 3) == 0 ? dt_u32() : (uint32_t)dt_range(0, 24));
    memcpy(rec + 2, &group, 2);
    void *prop = NULL;
    if (surface == 9 && (dt_u32() & 1) != 0) {
        uint8_t *p = dt_alloc(8);
        dt_fill(p, 8);
        p[2] = (uint8_t)dt_range(0, 40);
        prop = p;
    }
    memcpy(rec + 0x34, (const void *)&prop, 4);
    return rec;
}

static uint8_t randomSurface(void)
{
    static const uint8_t common[] = {0, 1, 1, 2, 3, 9, 9, 12, 12, 4};
    return (dt_u32() & 7) == 0 ? (uint8_t)dt_range(0, 14) : common[dt_range(0, 9)];
}

/* Valid contacts for the current contact and both snapshots. */
static void riderContacts(PhysRider *r)
{
    r->surface = randomSurface();
    r->contact = randomContact(r->surface);
    r->snapA.surface = randomSurface();
    r->snapA.contact = randomContact(r->snapA.surface);
    r->snapB.surface = randomSurface();
    r->snapB.contact = randomContact(r->snapB.surface);
}

static void randomTricks(RiderCombo *c)
{
    for (int i = 0; i < 8; i++) {
        RiderTrick *t = &c->trick[i];
        t->trick = (uint8_t)((dt_u32() & 7) == 0    ? 0xff
                             : (dt_u32() & 15) == 0 ? dt_u32()
                                                    : (uint32_t)dt_range(0, 0x2f));
        t->style = (uint8_t)((dt_u32() & 3) == 0    ? 0xff
                             : (dt_u32() & 15) == 0 ? dt_u32()
                                                    : (uint32_t)dt_range(0, 0x27));
        t->tier = (uint8_t)dt_range(0, 9);
        t->barspin = (uint8_t)dt_range(0, 3);
        t->flags = (uint8_t)dt_u32();
        t->ticks = (uint16_t)((dt_u32() & 1) ? dt_range(0, 0x80) : (int32_t)dt_u32());
        t->duration = dt_float_edgy(0.0f, 40.0f);
    }
}

/* ---- setups ------------------------------------------------------------------------------------- */

static void setupJudge(DtCtx *c)
{
    begin(c, DTG_DEBUG, 0);
    const int player = dt_range(0, 1);
    PhysRider *r = &gPhysRiders[player];
    if ((dt_u32() & 7) != 0) {
        gGameMode = 0;
        gGameSubMode = 0;
        gCurrentLevel = (uint8_t)dt_range(6, 11);
    }
    gGameRunCount = (uint8_t)((dt_u32() & 7) == 0 ? dt_range(2, 3) : dt_range(0, 1));
    r->combo.score = (dt_u32() & 7) == 0 ? 0 : (dt_u32() & 1) ? dt_u32() : (uint32_t)dt_range(0, 90000);
    for (int i = 0; i < 17; i++) {
        uint16_t *w = &r->area[i].tricks;
        for (int k = 0; k < 7; k++) {
            w[k] = (uint16_t)((dt_u32() & 3) == 0   ? 0
                              : (dt_u32() & 7) == 0 ? dt_u32()
                                                    : (uint32_t)dt_range(0, 30));
        }
    }
    if ((dt_u32() & 3) == 0) {
        memset((uint8_t *)r + 0xef, 0, 0x118); /* few tricks: most of the grid empty */
        for (int k = dt_range(0, 12); k > 0; k--) {
            ((uint8_t *)r + 0xef)[dt_range(0, 0x117)] = (uint8_t)(1 << (dt_range(0, 3) * 2));
        }
    }
    gProfiles[player].character = (uint8_t)dt_range(0, 11);
    if ((dt_u32() & 7) == 0) {
        /* nothing but falls: the judge score goes negative and the marks saturate */
        memset((uint8_t *)r + 0xef, 0, 0x118);
        memset(r->area, 0, sizeof r->area + sizeof r->statTotal);
        r->statTotal.falls = (uint16_t)dt_range(1, 5);
        r->combo.score = (uint32_t)dt_range(1, 100);
    }
    DT_ARG_INT(c, 0, player);
}

/* The id gameScoreJump reads from a contact snapshot. */
static uint16_t jumpId(const PhysContactSnap *snap)
{
    const uint8_t *contact = snap->contact;
    switch (snap->surface) {
    case 1:
    case 2:
    case 3:
        return rd16(contact + 0x16);
    case 9: {
        const uint8_t *prop;
        memcpy((void *)&prop, contact + 0x34, 4);
        return prop != NULL ? prop[2] & 0x1f : rd16(contact + 0x16);
    }
    case 12:
        return contact[2] & 0x1f;
    default:
        return rd16(contact + 0x16) & 0xfff;
    }
}

static void setupJump(DtCtx *c)
{
    begin(c, DTG_DEBUG | DTG_COMDATA | DTG_PROP, BIG_SUBMIT | BIG_EVALUATE);
    const int player = dt_range(0, 1);
    PhysRider *r = &gPhysRiders[player];
    riderContacts(r);
    static const uint8_t kinds[] = {0, 1, 2, 3, 9, 12};
    r->snapA.surface = kinds[dt_range(0, 5)];
    r->snapB.surface = (dt_u32() & 1) ? r->snapA.surface : kinds[dt_range(0, 5)];
    if ((dt_u32() & 15) == 0) {
        r->snapB.surface = (uint8_t)dt_range(10, 14);
    }
    r->snapA.contact = randomContact(r->snapA.surface);
    r->snapB.contact = randomContact(r->snapB.surface);
    const float span = (dt_u32() & 1) ? 600.0f : 50.0f;
    for (int i = 0; i < 3; i++) {
        (&r->snapAPos.x)[i] = dt_float_edgy(-span, span);
        (&r->snapBPos.x)[i] = dt_float_edgy(-span, span);
    }
    if ((dt_u32() & 7) == 0) {
        /* a jump right at the transfer distance of 400 */
        r->snapAPos = (ZVec3){(float)dt_range(-50, 50), (float)dt_range(-50, 50), (float)dt_range(-50, 50)};
        r->snapBPos = r->snapAPos;
        r->snapBPos.x += (float)dt_range(399, 401);
    }
    r->combo.count = (uint8_t)((dt_u32() & 1) ? 0 : dt_range(0, 8));
    randomTricks(&r->combo);
    if ((dt_u32() & 7) != 0) {
        gGameMode = 0;
        gGameSubMode = 0;
        gCurrentLevel = (uint8_t)dt_range(0, 5);
    }
    /* jump goals around the two contacts */
    uint8_t *info = dtGame.goalInfo[player][gCurrentLevel % 12];
    const uint16_t idA = jumpId(&r->snapA);
    const uint16_t idB = jumpId(&r->snapB);
    for (int row = 0; row < 4; row++) {
        GameGoalRow *gr = &gGameGoals[(gCurrentLevel * 3 + info[1]) % 18][row];
        for (int g = 0; g < 4; g++) {
            GameGoal *goal = &gr->goal[g];
            goal->type = (uint16_t)dt_range(0xd, 0x13);
            const int swap = (int)(dt_u32() & 1);
            goal->min = (uint16_t)((swap ? idB : idA) + dt_range(-6, 6));
            goal->max = (uint16_t)((swap ? idA : idB) + dt_range(-6, 6));
            goal->arg3 = (uint16_t)dt_range(0, 9);
            if ((goal->type == 0x10 || goal->type == 0x11) && (dt_u32() & 1) != 0) {
                goal->min = swap ? idB : idA;
                goal->max = swap ? idA : idB;
            }
            if (goal->type == 0x12 || (goal->type == 0x13 && (dt_u32() & 1))) {
                goal->arg3 = 0xffff; /* type 0x12: the original compares an uninitialised word with it */
            }
        }
    }
    DT_ARG_INT(c, 0, player);
    DT_ARG_INT(c, 1, (dt_u32() & 7) == 0 ? 0 : 1);
}

static void setupEndCombo(DtCtx *c)
{
    begin(c, DTG_DEBUG | DTG_COMDATA | DTG_ROLLER | DTG_SOUND, BIG_SUBMIT);
    const int player = dt_range(0, 1);
    PhysRider *r = &gPhysRiders[player];
    RiderCombo *combo = &r->combo;
    combo->count = (uint8_t)((dt_u32() & 7) == 0 ? dt_range(0, 8) : dt_range(1, 6));
    randomTricks(combo);
    r->lipHeight = dt_float_edgy(0.0f, 50.0f);
    r->pos.x = dt_float_edgy(-2500.0f, 2500.0f);
    r->pos.z = dt_float_edgy(-2500.0f, 2500.0f);
    if ((dt_u32() & 3) == 0) {
        gGameMode = 3;
    }
    gGameHorseCount = (uint8_t)dt_range(0, 3);
    for (int i = 0; i < 3; i++) {
        RiderTrick *h = &gGameHorseTricks[i];
        memcpy(h, &combo->trick[i], sizeof *h);
        if ((dt_u32() & 3) == 0) {
            h->trick = (uint8_t)((dt_u32() & 1) ? 0xff : dt_range(0, 0x2f));
        }
        if ((dt_u32() & 3) == 0) {
            h->style = (uint8_t)((dt_u32() & 1) ? 0xff : dt_range(0, 0x27));
        }
        if ((dt_u32() & 3) == 0) {
            h->tier = (uint8_t)dt_range(0, 9);
        }
    }
    DT_ARG_INT(c, 0, player);
    DT_ARG_INT(c, 1, (dt_u32() & 3) == 0 ? 0 : 1);
}

static void setupUpdateCombo(DtCtx *c)
{
    begin(c, DTG_DEBUG | DTG_COMDATA | DTG_PHYS | DTG_SOUND,
          BIG_SUBMIT | BIG_EVALUATE | BIG_COMBO | BIG_ENDCOMBO);
    const int player = dt_range(0, 1);
    PhysRider *r = &gPhysRiders[player];
    RiderCombo *combo = &r->combo;
    static const uint8_t modes[] = {0, 0, 0, 5, 1, 2, 3, 4, 6, 8, 10, 11};
    gGameMode = modes[dt_range(0, 11)];
    combo->runState = (uint8_t)((dt_u32() & 15) == 0 ? dt_range(4, 6) : dt_range(0, 3));
    combo->count = (uint8_t)((dt_u32() & 7) == 0 ? 7 : dt_range(0, 6));
    combo->timer = (uint8_t)((dt_u32() & 1) ? 1 : dt_u32());
    randomTricks(combo);
    for (int i = 0; i < 3; i++) {
        r->stateLevel[i] = (uint8_t)((dt_u32() & 3) == 0 ? dt_range(0, 6) : dt_range(0, 4));
    }
    r->flags = dt_u32() & ((dt_u32() & 1) ? 0x380030a0u : 0xffffffffu);
    if ((dt_u32() & 3) == 0) {
        r->spin = 0;
        r->anim = 0xff;
        r->ikPose = 0xff;
    } else {
        r->spin = (dt_u32() & 3) == 0 ? (int32_t)dt_u32() : dt_range(-0x50000, 0x50000);
        r->anim = (uint8_t)((dt_u32() & 7) == 0 ? 0xff : dt_range(0, 0x2f));
        r->ikPose = (uint8_t)((dt_u32() & 7) == 0 ? 0xff : dt_range(0, 0x27));
    }
    r->field54 = (dt_u32() & 3) == 0 ? (int32_t)dt_u32() : dt_range(-0x40000, 0x40000);
    r->field58 = dt_range(-1000, 1000);
    r->speed = dt_float_edgy(-5.0f, 30.0f);
    r->groundGap = dt_float_edgy(-5.0f, 30.0f);
    r->lipHeight = dt_float_edgy(-5.0f, 30.0f);
    r->impactTotal = (dt_u32() & 1) ? 0.0f : dt_float_edgy(-50.0f, 500.0f);
    r->pos.y = dt_float_edgy(-100.0f, 100.0f);
    r->snapAPos.y = dt_float_edgy(-100.0f, 100.0f);
    r->vel.y = dt_float_edgy(-10.0f, 10.0f);
    r->surface = (uint8_t)((dt_u32() & 1) ? 1 : dt_range(0, 12));
    dtGame.physState = (uint32_t)((dt_u32() & 1) ? 2 : dt_range(0, 7));
    if (combo->count != 0 && (dt_u32() & 3) == 0) {
        combo->trick[combo->count - 1].trick = (uint8_t)dt_range(8, 11);
        combo->trick[combo->count - 1].barspin = (uint8_t)dt_range(0, 2);
        if ((dt_u32() & 1) != 0) {
            combo->runState = 2;
            r->stateLevel[0] = 0;
        }
    }
    DT_ARG_INT(c, 0, player);
}

static void setupHints(DtCtx *c)
{
    begin(c, DTG_DEBUG, 0);
    const int player = dt_range(0, 1);
    PhysRider *r = &gPhysRiders[player];
    if ((dt_u32() & 7) != 0) {
        gProfiles[player].flags |= 2;
    }
    const uint32_t bits = (uint32_t)dt_range(0, 3);
    r->hintMask = 0;
    for (uint32_t k = 0; k < bits; k++) {
        r->hintMask |= 1u << dt_range(0, 21);
    }
    if ((dt_u32() & 7) == 0) {
        r->hintMask = dt_u32();
    }
    for (int i = 0; i < 2; i++) {
        r->stateLevel[i] = (uint8_t)((dt_u32() & 3) == 0 ? dt_range(0, 6) : dt_range(0, 2));
    }
    if ((dt_u32() & 7) == 0) {
        r->stateLevel[0] = 6;
    }
    r->combo.count = (uint8_t)((dt_u32() & 1) ? 0 : dt_range(1, 7));
    randomTricks(&r->combo);
    r->vel.y = (dt_u32() & 1) ? -1.0f : dt_float_edgy(-5.0f, 5.0f);
    r->speed = (dt_u32() & 1) ? 0.0f : dt_float_edgy(-5.0f, 5.0f);
    r->surface = (uint8_t)((dt_u32() & 1) ? 1 : dt_range(0, 12));
    r->bailReason = (uint8_t)dt_range(6, 9);
    if ((dt_u32() & 1) != 0) {
        r->hintMask = 1u << ((dt_u32() & 1) ? dt_range(12, 21) : dt_range(0, 21));
        r->stateLevel[0] = (uint8_t)((dt_u32() & 3) == 0 ? dt_range(2, 6) : dt_range(0, 1));
        r->stateLevel[1] = (uint8_t)dt_range(0, 4);
    }
    DT_ARG_INT(c, 0, player);
}

/* Makes a goal's parameters match the rider's current trick, contact and state (the types' conditions). */
static void fitGoal(PhysRider *r, GameGoal *goal)
{
    const RiderCombo *combo = &r->combo;
    const RiderTrick *cur = combo->count != 0 ? &combo->trick[combo->count - 1] : &combo->trick[0];
    const uint8_t *contact = r->contact;
    const uint16_t tag = rd16(contact + 0x16);
    const uint16_t group = rd16(contact + 2) & 0x1f;
    const uint16_t level = r->stateLevel[0];
    goal->area[0] = -0x7fff;
    goal->area[1] = -0x7fff;
    goal->area[2] = 0x7fff;
    goal->area[3] = 0x7fff;
    switch (goal->type) {
    case 0:
    case 1:
    case 2:
    case 3: {
        goal->min = (dt_u32() & 3) == 0 ? 0xff : cur->trick;
        goal->max = (uint16_t)(cur->tier - (dt_u32() & 1));
        uint16_t want =
            (uint16_t)((r->surface >= 1 && r->surface <= 3) || r->surface == 9 ? tag : tag & 0xfff);
        if (goal->type == 2) {
            const uint8_t *object = r->snapA.contact;
            if (r->snapA.surface == 9) {
                memcpy((void *)&object, (const uint8_t *)r->snapA.contact + 0x34, 4);
            }
            want = object != NULL ? object[2] & 0x1f : 0;
        }
        goal->arg3 = (uint16_t)(want + dt_range(-6, 6) * (goal->type == 1));
        if (goal->type == 3) {
            goal->arg3 = (uint16_t)(tag - (dt_u32() & 1));
        }
        break;
    }
    case 4:
        goal->min = (uint16_t)dt_range(0, 3);
        break;
    case 5:
    case 6:
    case 7:
        if (goal->type == 6 && (dt_u32() & 1) != 0) {
            r->surface = (uint8_t)dt_range(0, 3);
        }
        goal->min = level;
        goal->max = (uint16_t)((goal->type == 7   ? group
                                : r->surface == 0 ? tag & 0xfff
                                                  : tag) -
                               (dt_u32() & 1));
        goal->arg3 = (uint16_t)(goal->max + dt_range(0, 2));
        break;
    case 8:
    case 9: {
        r->surface = 9;
        r->stateLevel[0] = (uint8_t)dt_range(2, 3);
        dtGame.physState = r->stateLevel[0] == 2 ? 4 : 6;
        uint16_t v = tag;
        if (goal->type == 9) {
            const uint8_t *prop;
            memcpy((void *)&prop, contact + 0x34, 4);
            v = prop != NULL ? prop[2] & 0x1f : 0;
        }
        goal->min = (uint16_t)(v - (dt_u32() & 1));
        goal->max = (uint16_t)(v + (dt_u32() & 1));
        goal->arg3 = (uint16_t)(dt_u32() & 1);
        break;
    }
    case 10:
    case 11:
        goal->min = (uint16_t)(group - (dt_u32() & 1));
        goal->max = (uint16_t)(group + (dt_u32() & 1));
        goal->arg3 = (dt_u32() & 1) ? combo->trick[0].trick : cur->trick;
        if ((dt_u32() & 1) != 0) {
            r->snapA.surface = (dt_u32() & 1) ? 0xc : 9;
            uint8_t *object = r->snapA.contact;
            if (r->snapA.surface == 9) {
                uint8_t *edge = object;
                memcpy((void *)&object, edge + 0x34, 4);
                if (object == NULL) {
                    object = dt_alloc(8);
                    dt_fill(object, 8);
                    memcpy(edge + 0x34, (const void *)&object, 4);
                }
            }
            if (object != NULL) {
                object[2] = (uint8_t)(group + dt_range(-1, 1));
            }
        }
        break;
    default:
        break;
    }
}

static void setupGoals(DtCtx *c)
{
    begin(c, DTG_DEBUG | DTG_COMDATA | DTG_PROP | DTG_PHYS | DTG_SOUND, BIG_FINDERS);
    const int player = dt_range(0, 1);
    PhysRider *r = &gPhysRiders[player];
    if ((dt_u32() & 7) != 0) {
        gGameMode = 0;
        gGameSubMode = 0;
        gCurrentLevel = (uint8_t)dt_range(0, 5);
    }
    riderContacts(r);
    r->combo.count = (uint8_t)((dt_u32() & 3) == 0 ? 0 : dt_range(1, 7));
    randomTricks(&r->combo);
    for (int i = 0; i < 3; i++) {
        r->stateLevel[i] = (uint8_t)((dt_u32() & 3) == 0 ? dt_range(0, 6) : dt_range(0, 3));
    }
    r->pos.x = dt_float_edgy(-100.0f, 100.0f);
    r->pos.z = dt_float_edgy(-100.0f, 100.0f);
    const uint8_t *info = dtGame.goalInfo[player][gCurrentLevel % 12];
    for (int row = 0; row < 4; row++) {
        GameGoalRow *gr = &gGameGoals[(gCurrentLevel * 3 + info[1]) % 18][row];
        for (int g = 0; g < 4; g++) {
            GameGoal *goal = &gr->goal[g];
            goal->type = (uint16_t)((dt_u32() & 15) == 0 ? dt_range(0, 0x18) : dt_range(0, 0x15));
            goal->min = (uint16_t)((dt_u32() & 7) == 0 ? 0xff : dt_range(0, 12));
            goal->max = (uint16_t)dt_range(0, 12);
            goal->arg3 = (uint16_t)((dt_u32() & 3) == 0 ? 0 : dt_range(0, 24));
            if ((dt_u32() & 3) != 0) {
                goal->area[0] = (int16_t)dt_range(-120, 0);
                goal->area[1] = (int16_t)dt_range(-120, 0);
                goal->area[2] = (int16_t)dt_range(0, 120);
                goal->area[3] = (int16_t)dt_range(0, 120);
            }
        }
        if ((dt_u32() & 3) == 0) {
            gr->count = 1; /* one goal: collecting it finishes the row */
        }
        for (int g = 0; g < 4; g++) {
            if ((dt_u32() & 1) != 0) {
                fitGoal(r, &gr->goal[g]);
            }
        }
        if ((dt_u32() & 3) == 0) {
            /* active goals with their flag bit set */
            uint8_t *status = dtGame.goalInfo[player][gCurrentLevel % 12];
            status += 4 + (row + status[1] * 4) * 3;
            status[-1] &= 0x7f;
            status[0] = (uint8_t)(0x88 | dt_range(1, 2) << 4 | dt_range(1, 2));
            status[1] = (uint8_t)(0x88 | dt_range(1, 2) << 4 | dt_range(1, 2));
        }
    }
    DT_ARG_INT(c, 0, player);
}

static void setupBigGame(DtCtx *c)
{
    begin(c, DTG_DEBUG | DTG_COMDATA | DTG_ROLLER | DTG_SOUND | DTG_RIDERFN | DTG_PROP, BIG_SESSION);
    static const uint8_t modes[] = {0, 0, 0, 1, 2, 3, 3, 4, 5, 6, 8, 10, 11};
    gGameMode = (uint8_t)((dt_u32() & 15) == 0 ? dt_range(0, 14) : modes[dt_range(0, 12)]);
    gGameSubMode = (uint8_t)dt_range(0, 3); /* larger ones make record keys that name no table */
    gPlayerCount = (uint8_t)dt_range(1, 2);
    gCurrentPlayer = (uint8_t)dt_range(0, gPlayerCount - 1);
    gGameStartPlayer = (uint8_t)((dt_u32() & 1) ? gCurrentPlayer : dt_range(0, gPlayerCount - 1));
    gGameRunType = (uint8_t)((dt_u32() & 1) ? 5 : dt_range(0, 6));
    gGameRunCount = (uint8_t)dt_range(0, 5);
    if ((dt_u32() & 1) != 0) {
        gCurrentLevel = (uint8_t)dt_range(6, 11);
    }
    const int len = dt_range(0, 8);
    for (int i = 0; i < 12; i++) {
        gGameHorseWord[i] = i < len ? (char)('A' + dt_range(0, 25)) : '\0';
    }
    for (int i = 0; i < 2; i++) {
        RiderCombo *combo = &gPhysRiders[i].combo;
        combo->score = (dt_u32() & 3) == 0 ? 0 : (uint32_t)dt_range(0, 5000);
        combo->total = (dt_u32() & 3) == 0 ? (int32_t)combo->score : dt_range(0, 9000);
        combo->landed = dt_range(0, 9);
    }
    if ((dt_u32() & 3) == 0) {
        gPhysRiders[1].combo.score = gPhysRiders[0].combo.score;
        gPhysRiders[1].combo.total = gPhysRiders[0].combo.total;
        gPhysRiders[1].combo.landed = gPhysRiders[0].combo.landed;
    }
    for (int i = 0; i < 0x300; i += 8) {
        if ((dt_u32() & 1) != 0) {
            const uint32_t v = (uint32_t)dt_range(0, 6000);
            memcpy(&gGameRecords[i], &v, 4);
        }
    }
    if ((dt_u32() & 1) != 0) {
        for (int i = 5; i < 0x300; i += 8) {
            gGameRecords[i] &= 0xfe; /* no new record: the end screens past the record screen */
        }
    }
    if ((dt_u32() & 3) == 0) {
        /* a tie with the best total of the session */
        const uint32_t key =
            ((uint32_t)(gGameMode << 2 | gGameSubMode) << 6 | gScoreRecordSlot[gGameMode]) << 16 | 0x1000;
        const uint8_t *rec = gameGetRecord(key);
        if (rec != NULL) {
            gPhysRiders[dt_range(0, 1)].combo.total = (int32_t)rd32(rec);
        }
    }
    if ((dt_u32() & 3) == 0) {
        /* a finished set with a reward pending (gameHasPendingUnlock) */
        uint8_t *info = dtGame.goalInfo[gCurrentPlayer][gCurrentLevel % 12];
        info[2] = 4;
        info[3 + (info[1] * 4 + dt_range(0, 3)) * 3] |= 0x40;
    }
    static const uint8_t types[] = {1, 3, 3, 4, 5, 5, 5, 6, 0, 2, 7};
    DT_ARG_INT(c, 0, types[dt_range(0, 10)]);
}

DT_CASES(game_big_cases) = {
    {"gameJudgeRun", 0x004105d0, gameJudgeRun, 1, DT_RET_VOID, setupJudge, 600, 0},
    {"gameScoreJump", 0x0040c300, gameScoreJump, 2, DT_RET_VOID, setupJump, 1000, 0},
    {"gameEndCombo", 0x0040dae0, gameEndCombo, 2, DT_RET_VOID, setupEndCombo, 1000, 0},
    {"gameUpdateCombo", 0x0040d010, gameUpdateCombo, 1, DT_RET_VOID, setupUpdateCombo, 2000, 0},
    {"gameUpdateHints", 0x0040e380, gameUpdateHints, 1, DT_RET_VOID, setupHints, 3000, 0},
    {"gameUpdateGoals", 0x0040ec40, gameUpdateGoals, 1, DT_RET_VOID, setupGoals, 3000, 0},
    {"gameBigGameFunction", 0x0040aa50, gameBigGameFunction, 1, DT_RET_INT, setupBigGame, 2000, 0},
};
