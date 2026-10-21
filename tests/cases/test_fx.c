/*
 * Differential tests of fx.c. The effect engine (node lookup, effect creation, name table handlers,
 * particle setup) is stubbed and logs the parameter blocks it receives; the random number generator runs
 * natively on both sides.
 */
#include "bmx/com/action/fx.h"
#include "bmx/com/action/physics.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_gameplay.h"
#include "zge/zmath.h"

static void *makeNode(void)
{
    uint8_t *node = dt_alloc(0x40);
    float *target = dt_alloc(0x20);

    dt_fill(node, 0x40);
    for (int i = 0; i < 8; i++) {
        target[i] = dt_float_edgy(-500.0f, 500.0f);
    }
    *(float **)(node + 0x30) = target;
    return node;
}

/* The original reads the node's transform before it checks the node, so only fxWaterSplash can see NULL. */
static void randomNode(int mayBeNull)
{
    dtgp.node = mayBeNull && (dt_u32() & 7) == 0 ? NULL : makeNode();
}

static ZVec3 *randomVec(float range)
{
    ZVec3 *v = dt_alloc(sizeof *v);
    v->x = dt_float_edgy(-range, range);
    v->y = dt_float_edgy(-range, range);
    v->z = dt_float_edgy(-range, range);
    return v;
}

static void setupNone(DtCtx *c)
{
    dtgpBegin(c, 0);
}

static void randomState(void)
{
    gFxActive = dt_u32();
    gFxUnused = dt_u32();
    gFxLightCallback = (FxLightFn)(uintptr_t)dt_u32();
    gFxListCallback = (FxListFn)(uintptr_t)dt_u32();
}

/* A rider record the impact sparks read the actor from. */
static void randomRider(void)
{
    uint8_t *rider = dt_alloc(0xc00);
    dt_fill(rider, 0xc00);
    gFxRider = (PhysRider *)rider;
}

static void setupRider(DtCtx *c)
{
    dtgpBegin(c, 0);
    randomState();
    DT_ARG_PTR(c, 0, (void *)(uintptr_t)(0x00300000u + dt_range(0, 0xff) * 0x10u));
}

static void setupInit(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX);
    randomState();
}

static void setupShutdown(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX);
    randomState();
}

static void setupMode(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setupSpawn(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX);
    dtgp.sparkWords = 13;
    randomNode(0);
    gZmathTables.randomSeed = dt_u32();
    DT_ARG_PTR(c, 0, (void *)(uintptr_t)(0x00300000u + dt_range(0, 0xff) * 0x10u));
    DT_ARG_INT(c, 1, dt_range(0, 3));
    DT_ARG_PTR(c, 2, randomVec(1000.0f));
    DT_ARG_INT(c, 3, dt_u32());
}

static void setupPuff(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX);
    dtgp.extra[0] = 6;
    DT_ARG_PTR(c, 0, randomVec(1000.0f));
}

static void setupImpact(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX);
    dtgp.sparkWords = 11;
    randomNode(0);
    randomRider();
    DT_ARG_PTR(c, 0, randomVec(1000.0f));
    DT_ARG_PTR(c, 1, randomVec(100.0f));
}

static void setupScatter(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX);
    uint32_t *sprites = dt_alloc(0x40);

    dtgp.extra[0] = 16;
    dtgp.extra[1] = 0xbbdf; /* the words the original leaves uninitialised are not logged */
    gZmathTables.randomSeed = dt_u32();
    for (int i = 0; i < 8; i++) {
        sprites[i] = dt_u32();
    }
    DT_ARG_PTR(c, 0, sprites);
    DT_ARG_INT(c, 1, dt_range(0, 9));
    DT_ARG_PTR(c, 2, randomVec(200.0f));
    DT_ARG_INT(c, 3, (dt_u32() & 1) != 0 ? dt_range(-0x8000, 0x7fff) : (int32_t)dt_u32());
    DT_ARG_PTR(c, 4, randomVec(200.0f));
}

static void setupEmit(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX | DTGP_DEBUG);
    dtgp.extra[0] = 8;
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, randomVec(1000.0f));
    DT_ARG_INT(c, 3, dt_u32());
    DT_ARG_INT(c, 4, (dt_u32() & 7) == 0 ? dt_range(6, 7) : dt_range(0, 5));
}

static void setupCreate(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX | DTGP_DEBUG);
    dtgp.extra[0] = 8;
    DT_ARG_INT(c, 0, (dt_u32() & 7) == 0 ? dt_range(6, 7) : dt_range(0, 5));
    DT_ARG_PTR(c, 1, randomVec(1000.0f));
    DT_ARG_PTR(c, 2, dt_alloc(0x40));
}

static uint8_t *randomBytes(uint32_t size)
{
    uint8_t *p = dt_alloc(size);
    dt_fill(p, size);
    return p;
}

static void setupSplash(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX);
    uint8_t *obj = randomBytes(0x40);
    float *offset = dt_alloc(12);

    randomNode(0);
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

static void setupCallbackEmit(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX | DTGP_DEBUG);
    dtgp.extra[0] = 8;
    DT_ARG_PTR(c, 0, randomBytes(0x40));
    DT_ARG_INT(c, 1, dt_u32() & 7);
}

static void setupLight(DtCtx *c)
{
    dtgpBegin(c, 0);
    DT_ARG_PTR(c, 0, randomBytes(0x40));
    DT_ARG_INT(c, 1, dt_range(0, 2));
}

static void setupList(DtCtx *c)
{
    dtgpBegin(c, DTGP_FX | DTGP_DEBUG);
    uint8_t *list = randomBytes(4 + 16 * 6);
    const uint32_t count = (uint32_t)dt_range(0, 6);

    dtgp.extra[0] = 8;
    for (uint32_t i = 0; i < count; i++) {
        list[4 + i * 16 + 6] = (dt_u32() & 3) ? 7 : (uint8_t)dt_u32();
        list[4 + i * 16 + 7] = (uint8_t)((dt_u32() & 7) == 0 ? dt_range(6, 9) : dt_range(0, 5));
    }
    DT_ARG_PTR(c, 0, list);
    DT_ARG_INT(c, 1, count);
}

DT_CASES(fx_cases) = {
    {"fxNop1", 0x00408960, fxNop1, 0, DT_RET_VOID, setupNone, 1, 0},
    {"fxNop2", 0x00408980, fxNop2, 0, DT_RET_VOID, setupNone, 1, 0},
    {"fxSetRider", 0x004089a0, fxSetRider, 1, DT_RET_VOID, setupRider, 30, 0},
    {"fxInit", 0x004089c0, fxInit, 0, DT_RET_INT, setupInit, 30, 0},
    {"fxShutdown", 0x00408a10, fxShutdown, 0, DT_RET_VOID, setupShutdown, 30, 0},
    {"fxStopAll", 0x00408a40, fxStopAll, 0, DT_RET_VOID, setupInit, 10, 0},
    {"fxStopAllMode", 0x00408a60, fxStopAllMode, 1, DT_RET_VOID, setupMode, 30, 0},
    {"fxBurst", 0x00408a80, fxBurst, 0, DT_RET_VOID, setupInit, 10, 0},
    {"fxSpawn", 0x00408aa0, fxSpawn, 4, DT_RET_VOID, setupSpawn, 600, 0},
    {"fxPuff", 0x00408c50, fxPuff, 1, DT_RET_VOID, setupPuff, 200, 0},
    {"fxImpactSparks", 0x00408c90, fxImpactSparks, 2, DT_RET_VOID, setupImpact, 600, 0},
    {"fxScatter", 0x00408db0, fxScatter, 5, DT_RET_VOID, setupScatter, 400, 0},
    {"fxEmit", 0x004095a0, fxEmit, 5, DT_RET_VOID, setupEmit, 300, 0},
    {"fxCreate", 0x00409640, fxCreate, 3, DT_RET_VOID, setupCreate, 300, 0},
    {"fxWaterSplash", 0x004096d0, fxWaterSplash, 3, DT_RET_VOID, setupSplash, 400, 0},
    {"fxNop", 0x00409860, fxNop, 0, DT_RET_VOID, setupNone, 1, 0},
    {"fxCallbackEmit", 0x00409880, fxCallbackEmit, 2, DT_RET_VOID, setupCallbackEmit, 200, 0},
    {"fxCallbackLight", 0x004098b0, fxCallbackLight, 2, DT_RET_INT, setupLight, 50, 0},
    {"fxCreateFromList", 0x004098f0, fxCreateFromList, 2, DT_RET_INT, setupList, 400, 0},
};
