/*
 * Differential tests of contrler.c. The engine's device layer is replaced by fake devices (random status,
 * buttons and sticks) and the create / destroy calls are stubbed and logged.
 */
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/menu/frontend.h"
#include "dt.h"
#include "dt_gameplay.h"

static uint32_t floatBits(float f)
{
    uint32_t u;
    __builtin_memcpy(&u, &f, 4);
    return u;
}

/* Stick values around the 0.5 threshold and the full range, including non finite ones. */
static float stickValue(void)
{
    static const float special[] = {0.0f,         -0.0f,       0.5f,         -0.5f, 0.50000006f,
                                    -0.50000006f, 0.49999997f, -0.49999997f, 1.0f,  -1.0f};
    switch (dt_u32() & 7) {
    case 0:
    case 1:
        return special[dt_u32() % (sizeof special / sizeof special[0])];
    case 2:
        return (dt_u32() & 3) == 0 ? __builtin_nanf("") : dt_float_edgy(-1.0f, 1.0f);
    default:
        return dt_float(-1.0f, 1.0f);
    }
}

static ContrlerDevice *makeDevice(void)
{
    ContrlerDevice *d = dt_alloc(sizeof *d);
    dt_fill(d, sizeof *d);
    d->status = (uint8_t)((dt_u32() & 3) == 0 ? dt_u32() & 0xfe : dt_u32() | 1);
    for (int i = 0; i < 4; i++) {
        d->axis[i] = floatBits(stickValue());
    }
    return d;
}

/* A sparse random button mask. */
static uint32_t randomMask(void)
{
    const uint32_t a = dt_u32();
    const uint32_t b = dt_u32();
    const uint32_t c = dt_u32();
    return a & (b | c);
}

static void randomPads(void)
{
    dt_fill(&gContrlerPads, sizeof gContrlerPads);
    for (int i = 0; i < 2; i++) {
        ContrlerPad *p = &gContrlerPads[i];
        p->device = makeDevice();
        p->flags = (uint16_t)(dt_u32() & 3 ? dt_u32() & 3 : dt_u32());
        if ((dt_u32() & 1) != 0) {
            p->stuck = p->state.buttons;
        }
        for (int k = 0; k < 4; k++) {
            p->state.axis[k] = stickValue();
            p->state.prevAxis[k] = stickValue();
        }
    }
    gContrlerMode = (uint8_t)(dt_u32() & 3 ? dt_u32() & 1 : dt_u32());
    for (int i = 0; i < 2; i++) {
        static const uint32_t def[] = {0x80, 0x20, 0x40, 0x10, 0x100, 0x200, 0x4000};
        for (int k = 0; k < 7; k++) {
            gPlayers[i].bind[4 + k] = (dt_u32() & 1) != 0 ? def[k] : randomMask();
        }
    }
}

static void setupCreate(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG | DTGP_PAD);
    dt_fill(&gContrlerPads, sizeof gContrlerPads);
    dtgp.padResults[0] = (dt_u32() & 1) != 0 ? (uint32_t)(uintptr_t)makeDevice() : 0;
    dtgp.padResults[1] = (uint32_t)(uintptr_t)makeDevice();
    gContrlerMode = (uint8_t)dt_u32();
}

static void setupFree(DtCtx *c)
{
    dtgpBegin(c, DTGP_PAD);
    randomPads();
}

static void setupPoll(DtCtx *c)
{
    dtgpBegin(c, 0);
    randomPads();
}

static void setupSetMode(DtCtx *c)
{
    dtgpBegin(c, 0);
    randomPads();
    DT_ARG_INT(c, 0, (dt_u32() & 3) == 0 ? dt_u32() : (uint32_t)dt_range(0, 2));
}

static ContrlerState *randomState(void)
{
    ContrlerState *s = dt_alloc(0x40);
    dt_fill(s, sizeof *s);
    s->buttons = (dt_u32() & 1) != 0 ? dt_u32() & 0xf : dt_u32();
    s->pressed = (dt_u32() & 1) != 0 ? dt_u32() & 0xf : dt_u32();
    for (int k = 0; k < 4; k++) {
        s->axis[k] = stickValue();
        s->prevAxis[k] = stickValue();
    }
    return s;
}

static void setupAction(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    DT_ARG_INT(c, 0, (dt_u32() & 15) == 0 ? 3 : dt_range(0, 2));
    DT_ARG_PTR(c, 1, randomState());
    DT_ARG_INT(c, 2, (dt_u32() & 15) == 0 ? dt_range(0x15, 0x17) : dt_range(0, 0x14));
    DT_ARG_INT(c, 3, (dt_u32() & 15) == 0 ? 3 : dt_range(0, 2));
}

static void setupPressed(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    randomPads();
    for (int i = 0; i < 2; i++) {
        gProfiles[i].controller = (uint8_t)((dt_u32() & 15) == 0 ? dt_range(2, 3) : dt_range(0, 1));
    }
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_INT(c, 1, (dt_u32() & 3) == 0 ? dt_u32() : (dt_u32() & 0xf) | (dt_u32() & 0x3f00));
    DT_ARG_INT(c, 2, (dt_u32() & 15) == 0 ? dt_range(8, 9) : dt_range(0, 7));
}

static void setupAxes(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    gContrlerAxes[0] = stickValue();
    gContrlerAxes[1] = stickValue();
    DT_ARG_PTR(c, 0, randomState());
    DT_ARG_INT(c, 1, (dt_u32() & 15) == 0 ? 2 : dt_range(0, 1));
}

static void setupNone(DtCtx *c)
{
    dtgpBegin(c, 0);
}

static void setupGet(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    DT_ARG_INT(c, 0, (dt_u32() & 7) == 0 ? dt_range(2, 4) : dt_range(0, 1));
}

static void setupTest(DtCtx *c)
{
    dtgpBegin(c, 0);
    DT_ARG_PTR(c, 0, randomState());
    DT_ARG_INT(c, 1, (dt_u32() & 3) == 0 ? dt_u32() : (dt_u32() & 0xf) | (dt_u32() & 0x3f00));
    DT_ARG_INT(c, 2, dt_range(0, 8));
}

DT_CASES(contrler_cases) = {
    {"contrlerCreateDevices", 0x00408030, contrlerCreateDevices, 0, DT_RET_VOID, setupCreate, 200, 0},
    {"contrlerFreeDevices", 0x004080c0, contrlerFreeDevices, 0, DT_RET_VOID, setupFree, 20, 0},
    {"contrlerPoll", 0x004080f0, contrlerPoll, 0, DT_RET_INT, setupPoll, 1500, 0},
    {"contrlerClear", 0x00408390, contrlerClear, 0, DT_RET_VOID, setupPoll, 30, 0},
    {"contrlerSetMode", 0x004083e0, contrlerSetMode, 1, DT_RET_VOID, setupSetMode, 600, 0},
    {"contrlerAction", 0x004084b0, contrlerAction, 4, DT_RET_INT, setupAction, 1500, 0},
    {"contrlerPressed", 0x00408590, contrlerPressed, 3, DT_RET_INT, setupPressed, 1000, 0},
    {"contrlerAxes", 0x00408610, contrlerAxes, 2, DT_RET_INT, setupAxes, 500, 0},
    {"contrlerRumble", 0x004086d0, contrlerRumble, 3, DT_RET_VOID, setupNone, 1, 0},
    {"contrlerGet", 0x004086f0, contrlerGet, 1, DT_RET_INT, setupGet, 50, 0},
    {"contrlerTest", 0x00408740, contrlerTest, 3, DT_RET_INT, setupTest, 2000, 0},
};
