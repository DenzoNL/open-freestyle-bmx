/* Tests for frontend_slots.c: the save/load slot screen. */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/zge/zprim.h"
#include "dt.h"
#include "dt_frontend.h"
#include "zge/zstring.h"

static void BMX_CDECL stubDialogOpen(int16_t save, uint16_t slot, const uint32_t *rows, uint16_t mode,
                                     McDoneFn done)
{
    uint32_t hash = 0;
    for (int i = 0; i < 8; i++) {
        hash = hash * 31 + rows[i];
    }
    dtFrontLog(0x300, (uint32_t)(uint16_t)save, slot, hash, (uint32_t)mode ^ (uint32_t)(uintptr_t)done);
}

static void BMX_CDECL returnProbe(uint32_t arg)
{
    dtFrontLog(0x310, arg, 0, 0, 0);
}

static void slotState(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    dtFrontendStub(DT_FS_SOUND | DT_FS_ZPRIM | DT_FS_SCREENS);
    dt_stub(0x0043d320, mcDialogOpen, (void *)stubDialogOpen);
    dtFrontendElementRange(0xa0120001, 0x14);
    gFrontendStep = dt_u32() % 3 == 0 ? NULL : dtFrontStep;
    gSlotSel = (uint16_t)(dt_u32() % 8 == 0 ? dt_range(5, 9) : dt_range(0, 4));
    gMcState = (uint16_t)(dt_u32() % 2 ? (dt_u32() % 2 ? 7 : 0x102) : dt_range(0, 0x105));
    gGuiPulse = dt_range(0x10, 0xf0);
    gGuiIdleMode = dt_range(0, 1);
    gScreenReturnFn = returnProbe;
    gScreenReturnArg = dt_u32();
}

static void setup_state(DtCtx *c)
{
    slotState(c);
}

static void setup_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTickAll,
                               kGuiMsgTickAll,
                               kGuiMsgNavUp,
                               kGuiMsgNavDown,
                               kGuiMsgNavUpAlt,
                               kGuiMsgNavDownAlt,
                               kGuiMsgButton22,
                               kGuiMsgButton22,
                               kGuiMsgButton24,
                               0x2f,
                               0x31,
                               kGuiMsgButton25,
                               kGuiMsgIdle,
                               kGuiMsgDrawAll};
    slotState(c);
    dtFrontendHookArgs(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
}

DT_CASES(frontend_slots_cases) = {
    {"slotScreenInit", 0x00443e30, slotScreenInit, 0, DT_RET_INT, setup_state, 50, 0},
    {"slotScreenLayout", 0x00443e70, slotScreenLayout, 0, DT_RET_VOID, setup_state, 100, 0},
    {"slotScreenDone", 0x00444110, slotScreenDone, 0, DT_RET_VOID, setup_state, 50, 0},
    {"slotScreenReturn", 0x00444140, slotScreenReturn, 0, DT_RET_VOID, setup_state, 200, 0},
    {"slotScreenHook", 0x00444310, slotScreenHook, 4, DT_RET_INT, setup_hook, 1000, 0},
};
