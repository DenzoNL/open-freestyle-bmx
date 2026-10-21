/* Tests for MissingZGE.c: the console-port stand-ins print through the debug print and set the mode byte. */
#include "bmx/com/zge/ngif/missingzge.h"
#include "dt.h"
#include "dt_ngif.h"

static void setup_plain(DtCtx *c)
{
    ngBegin(c, 0);
    gZsystemMode = (uint8_t)dt_u32();
}

static void setup_one(DtCtx *c)
{
    setup_plain(c);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_two(DtCtx *c)
{
    setup_plain(c);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
}

DT_CASES(missingzge_cases) = {
    {"zsystemEndFrame", 0x0045a1a0, zsystemEndFrame, 0, DT_RET_VOID, setup_plain, 10, 0},
    {"zconsoleLog", 0x0045a1c0, zconsoleLog, 1, DT_RET_VOID, setup_one, 20, 0},
    {"zglEnableHorizon", 0x0045a1f0, zglEnableHorizon, 0, DT_RET_VOID, setup_plain, 10, 0},
    {"zglDisableHorizon", 0x0045a220, zglDisableHorizon, 0, DT_RET_VOID, setup_plain, 10, 0},
    {"zsystemSetActionMode", 0x0045a250, zsystemSetActionMode, 0, DT_RET_VOID, setup_plain, 10, 0},
    {"zsystemSetMenuMode", 0x0045a290, zsystemSetMenuMode, 0, DT_RET_VOID, setup_plain, 10, 0},
    {"zfxDestroyAll", 0x0045a2d0, zfxDestroyAll, 1, DT_RET_VOID, setup_one, 20, 0},
    {"zfxRegisterFuncTable", 0x0045a300, zfxRegisterFuncTable, 2, DT_RET_INT, setup_two, 20, 0},
};
