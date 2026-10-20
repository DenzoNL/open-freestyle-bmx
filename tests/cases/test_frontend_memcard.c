/* Tests for frontend_memcard.c: the memory card dialog, its steps and result texts. */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/zge/zprim.h"
#include "dt.h"
#include "dt_frontend.h"
#include "zge/zstring.h"

static const uint32_t kRows[8] = {0xa0070001, 0xa0070002, 0xa0070003, 0xa0070004,
                                  0xa0070005, 0xa0070006, 0xa0070007, 0xa0070008};

static void BMX_CDECL stubSetLanguage(uint32_t language)
{
    dtFrontLog(0x200, language, 0, 0, 0);
}

static void BMX_CDECL doneProbe(void)
{
    dtFrontLog(0x210, 0, 0, 0, 0);
}

/* The dialog's rows as elements, a random dialog state, and the stubs of the card layer. */
static void dialogState(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    dtFrontendStub(DT_FS_SOUND | DT_FS_ZPRIM | DT_FS_CARD);
    dt_stub(0x0044dad0, frontendSetLanguage, (void *)stubSetLanguage);
    dtFrontRandomCardResults();
    dtFrontendElements(kRows, 8);
    for (int i = 0; i < 8; i++) {
        gMcRows[i] = kRows[i];
    }
    gMcSlot = (uint16_t)dt_range(0, 3);
    gMcState = (uint16_t)dt_range(0, 13);
    gMcCountdown = (int16_t)dt_range(-2, 4);
    gMcChoice = (uint16_t)dt_range(0, 1);
    gMcLoadOk = (uint16_t)dt_u32();
    gMcSaveOk = (uint16_t)dt_u32();
    gMcMode = (uint16_t)dt_u32();
    gMcDone = doneProbe;
    gFrontendStep = dt_u32() % 2 ? NULL : dtFrontStep;
    gGuiPulse = dt_range(0x10, 0xf0);
    gGuiIdleMode = dt_range(0, 1);
    gLanguageCurrent = dt_u32();
    gSoundVolume0 = (uint8_t)dt_u32();
    gSoundVolume1 = (uint8_t)dt_u32();
}

static void setup_open(DtCtx *c)
{
    dialogState(c);
    dtFrontendStub(DT_FS_SCREENS);
    uint32_t *rows = dt_alloc(8 * 4);
    for (int i = 0; i < 8; i++) {
        rows[i] = kRows[i];
    }
    for (int i = 0; i < 8; i++) {
        gMcRows[i] = dt_u32(); /* the previous dialog's ids, replaced by the copy */
    }
    DT_ARG_INT(c, 0, c->seed % 3 == 0 ? 0 : (int)(dt_u32() | 1));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, rows);
    DT_ARG_INT(c, 3, dt_u32());
    DT_ARG_PTR(c, 4, c->seed % 5 == 0 ? NULL : (void *)doneProbe);
    gMcSlot = (uint16_t)dt_range(0, 3);
    DT_ARG_INT(c, 1, gMcSlot);
}

static void setup_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTickAll,
                               kGuiMsgTickAll,
                               kGuiMsgTickAll,
                               kGuiMsgNavUp,
                               kGuiMsgNavDown,
                               kGuiMsgNavLeft,
                               kGuiMsgNavRight,
                               kGuiMsgNavLeftAlt,
                               kGuiMsgNavRightAlt,
                               kGuiMsgButton22,
                               kGuiMsgButton24,
                               0x2f,
                               0x31,
                               kGuiMsgButton25,
                               kGuiMsgIdle,
                               0x27,
                               0x29,
                               0x34,
                               0x36,
                               kGuiMsgDrawAll};
    dialogState(c);
    if (dt_u32() % 3 != 0) { /* mostly the states the steps and buttons care about */
        static const uint16_t states[] = {0,   1,     3,     4,     5,     6, 7, 8,  10,
                                          0xb, 0x100, 0x101, 0x102, 0x103, 2, 9, 0xc};
        gMcState = states[dt_u32() % (sizeof states / sizeof states[0])];
    }
    dtFrontendHookArgs(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
}

static void setup_step(DtCtx *c)
{
    dialogState(c);
}

static void setup_code(DtCtx *c)
{
    static const int codes[] = {0, 1, 2, 3, 4, 5, 6, 7, -1};
    dialogState(c);
    DT_ARG_INT(c, 0, c->seed % 7 == 0 ? (int)dt_u32() : codes[dt_u32() % 9]);
}

DT_CASES(frontend_memcard_cases) = {
    {"mcDialogOpen", 0x0043d320, mcDialogOpen, 5, DT_RET_VOID, setup_open, 200, 0},
    {"mcDialogHook", 0x0043d4a0, mcDialogHook, 4, DT_RET_INT, setup_hook, 1500, 0},
    {"mcStepClose", 0x0043db20, mcStepClose, 0, DT_RET_VOID, setup_step, 50, 0},
    {"mcStepLoadProbe", 0x0043db90, mcStepLoadProbe, 0, DT_RET_VOID, setup_step, 200, 0},
    {"mcStepLoadRead", 0x0043dd00, mcStepLoadRead, 0, DT_RET_VOID, setup_step, 200, 0},
    {"mcStepPrompt1", 0x0043de10, mcStepPrompt1, 0, DT_RET_VOID, setup_step, 100, 0},
    {"mcStepSaveFormat", 0x0043e010, mcStepSaveFormat, 0, DT_RET_VOID, setup_step, 400, 0},
    {"mcStepMessage", 0x0043e3b0, mcStepMessage, 0, DT_RET_VOID, setup_step, 100, 0},
    {"mcStepPrompt3", 0x0043e440, mcStepPrompt3, 0, DT_RET_VOID, setup_step, 100, 0},
    {"mcStepPrompt4", 0x0043e670, mcStepPrompt4, 0, DT_RET_VOID, setup_step, 100, 0},
    {"mcStepSaveProbe", 0x0043e8a0, mcStepSaveProbe, 0, DT_RET_VOID, setup_step, 400, 0},
    {"mcStepSaveWrite", 0x0043ea10, mcStepSaveWrite, 0, DT_RET_VOID, setup_step, 400, 0},
    {"mcShowLoadResult", 0x0043eb70, mcShowLoadResult, 1, DT_RET_VOID, setup_code, 300, 0},
    {"mcShowSaveResult", 0x0043edf0, mcShowSaveResult, 1, DT_RET_VOID, setup_code, 300, 0},
};
