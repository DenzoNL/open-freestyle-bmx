/*
 * Differential tests of replay.c. The static memory region and the game state it reads are stubbed (the
 * allocation hands out a random filled buffer); the replay table is filled with random records.
 */
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/prop.h"
#include "bmx/com/action/replay.h"
#include "dt.h"
#include "dt_gameplay.h"

/* Slots 0..7: the table has five entries, the handle check only looks at the low byte. */
static void randomReplays(int allowNull)
{
    uint32_t *table = (uint32_t *)&gReplays[0];

    for (int i = 0; i < 8; i++) {
        uint8_t *r = dt_alloc(0x80);
        dt_fill(r, 0x80);
        table[i] = (uint32_t)(uintptr_t)r;
        if (allowNull && i < 5 && (dt_u32() & 1) != 0) {
            table[i] = 0;
        }
    }
}

static uint32_t handleArg(void)
{
    /* Mostly valid handles, sometimes 5..7; the upper bits of the argument are not read. */
    const uint32_t h = (dt_u32() & 7) == 0 ? (uint32_t)dt_range(5, 7) : (uint32_t)dt_range(0, 4);
    return (dt_u32() & 1) != 0 ? (dt_u32() & 0xffffff00u) | h : h;
}

static void setupInit(DtCtx *c)
{
    dtgpBegin(c, 0);
    dt_fill(&gReplays, 0x20);
}

static void setupDestroyAll(DtCtx *c)
{
    dtgpBegin(c, DTGP_ZMEM | DTGP_DEBUG);
    randomReplays(1);
}

static void setupCreate(DtCtx *c)
{
    dtgpBegin(c, DTGP_ZMEM | DTGP_GAME | DTGP_DEBUG);
    ReplayParams *p = dt_alloc(sizeof *p);

    randomReplays(1);
    dt_fill(&gPropState, sizeof gPropState);
    gCurrentLevel = (uint8_t)((dt_u32() & 7) == 0 ? dt_u32() : (uint32_t)dt_range(0, 7));
    dtgp.region = dt_alloc(0x10000);
    dt_fill(dtgp.region, 0x10000);
    dtgp.extra[2] = 0x00700000u + dt_u32() % 0x1000 * 4;
    dtgp.mode = (dt_u32() & 3) == 0 ? (uint32_t)dt_range(1, 3) : 0;
    dtgp.subMode = (dt_u32() & 3) == 0 ? (uint32_t)dt_range(1, 3) : 0;
    dtgp.goalInfo = dt_alloc(0x27);
    dt_fill(dtgp.goalInfo, 0x27);
    if ((dt_u32() & 7) == 0) {
        dtgp.staticFail = dt_u32() & 0x1f;
    }
    p->flags = (uint8_t)dt_u32();
    p->index = (uint8_t)dt_u32();
    p->size = (dt_u32() & 3) == 0 ? 0 : (dt_u32() & 1) != 0 ? dt_range(1, 0x2000) : dt_range(0, 0x3000);
    DT_ARG_PTR(c, 0, p);
}

static void setupHandle(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    randomReplays(0);
    DT_ARG_INT(c, 0, handleArg());
}

static void setupDestroy(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG | DTGP_ZMEM);
    randomReplays(0);
    DT_ARG_INT(c, 0, handleArg());
}

static void setupSetRider(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    randomReplays(0);
    DT_ARG_INT(c, 0, handleArg());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setupBuffer(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    randomReplays(0);
    DT_ARG_INT(c, 0, handleArg());
    DT_ARG_PTR(c, 1, dt_alloc(4));
    DT_ARG_PTR(c, 2, dt_alloc(4));
}

/* replayUpdate: the controllers are random, replaySetMode is stubbed and logs. */
static void BMX_CDECL stubSetMode(uint32_t handle, uint32_t mode)
{
    dtgpLog(0x05e70000u);
    dtgpLog(handle);
    dtgpLog(mode);
}

static void setupUpdate(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    dt_stub(0x00430410, NULL, (void *)stubSetMode);
    dt_fill(&gPhysRiders, sizeof gPhysRiders);
    for (int p = 0; p < 2; p++) {
        ContrlerPad *pad = &gContrlerPads[p];

        dt_fill(pad, sizeof *pad);
        pad->flags = (dt_u32() & 7) == 0 ? (uint16_t)dt_u32() | 1 : (uint16_t)(dt_u32() & 0xfffe);
        for (int i = 0; i < 4; i++) {
            pad->state.axis[i] = dt_float_edgy(-1.3f, 1.3f);
        }
        gProfiles[p].controller = (dt_u32() & 15) == 0 ? (uint8_t)dt_u32() : (uint8_t)dt_range(0, 1);
    }
    for (int i = 0; i < 5; i++) {
        const uint32_t size = (uint32_t)dt_range(0x10, 0x60);
        Replay *r;
        uint32_t offset;

        if ((dt_u32() & 3) == 0) {
            gReplays[i] = NULL;
            continue;
        }
        r = dt_alloc(0x40 + size + 0x20);
        dt_fill(r, 0x40 + size + 0x20);
        offset = (dt_u32() & 3) == 0 ? 0x40 + size - 3 + (uint32_t)dt_range(-1, 3)
                                     : (uint32_t)dt_range(0x40, 0x40 + (int)size);
        r->state = (uint8_t)((dt_u32() & 7) == 0 ? dt_range(0, 3) : dt_range(1, 2));
        r->counter = (dt_u32() & 3) == 0 ? (uint8_t)dt_u32() : (uint8_t)dt_range(0x1d, 0x20);
        if (r->state == 2) {
            r->counter = (uint8_t)dt_range(0, 3);
        }
        r->rider = (dt_u32() & 7) == 0 ? (uint8_t)dt_range(2, 3) : (uint8_t)dt_range(0, 1);
        r->field08 = (uint32_t)dt_range(0, 5);
        r->field10 = (dt_u32() & 7) == 0 ? 0 : (uint32_t)dt_range(1, 200);
        if (r->state == 2 && (dt_u32() & 3) == 0) {
            r->field10 = r->field08 + 1;
        }
        r->headerSize = 0x40;
        r->size = size;
        r->cursor = (uint8_t *)r + offset;
        r->buttons = (dt_u32() & 1) != 0 ? gContrlerPads[0].state.buttons : dt_u32();
        for (int s = 0; s < 2; s++) {
            if ((dt_u32() & 1) != 0) {
                const ContrlerState *st = &gContrlerPads[0].state;
                const uint8_t hi = (uint8_t)(int)(st->axis[2 * s] * 127.0f);
                const uint8_t lo = (uint8_t)(int)(st->axis[2 * s + 1] * 127.0f);

                r->sticks[s] = (uint16_t)(hi << 8 | lo);
            }
        }
        gReplays[i] = r;
    }
}

DT_CASES(replay_cases) = {
    {"replayUpdate", 0x0042fd50, replayUpdate, 0, DT_RET_VOID, setupUpdate, 3000, 0},
    {"replayInit", 0x0042fce0, replayInit, 0, DT_RET_VOID, setupInit, 10, 0},
    {"replayDestroyAll", 0x0042fd10, replayDestroyAll, 0, DT_RET_VOID, setupDestroyAll, 100, 0},
    {"replayCreate", 0x00430180, replayCreate, 1, DT_RET_INT8, setupCreate, 1500, 0},
    {"replayDestroy", 0x00430380, replayDestroy, 1, DT_RET_VOID, setupDestroy, 300, 0},
    {"replayGetState", 0x00430b70, replayGetState, 1, DT_RET_INT, setupHandle, 300, 0},
    {"replaySetRider", 0x00430bf0, replaySetRider, 2, DT_RET_INT, setupSetRider, 300, 0},
    {"replayGetRider", 0x00430c70, replayGetRider, 1, DT_RET_INT, setupHandle, 300, 0},
    {"replayGetField08", 0x00430cf0, replayGetField08, 1, DT_RET_INT, setupHandle, 300, 0},
    {"replayGetBuffer", 0x00430d70, replayGetBuffer, 3, DT_RET_VOID, setupBuffer, 300, 0},
};
