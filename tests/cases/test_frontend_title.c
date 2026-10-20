/* Tests for frontend_title.c: autoload, press start, main menu and its carousel. */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/zge/zprim.h"
#include "dt.h"
#include "dt_frontend.h"
#include "zge/zstring.h"

static void BMX_CDECL stubSetLanguage(uint32_t language)
{
    dtFrontLog(0x200, language, 0, 0, 0);
}

/* The screen's elements: prompt, four autoload lines, entries 6..9 and the carousel slots a..0x10. */
static void titleElements(void)
{
    dtFrontendElementRange(0xa0050001, 16);
}

static void titleState(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    dtFrontendStub(DT_FS_SOUND | DT_FS_ZPRIM | DT_FS_CARD | DT_FS_QUIT | DT_FS_SCREENS);
    dt_stub(0x0044dad0, frontendSetLanguage, (void *)stubSetLanguage);
    dtFrontRandomCardResults();
    titleElements();
    gFrontendStep = dt_u32() % 3 == 0 ? NULL : dtFrontStep;
    gTitleAction = dt_u32() % 2 == 0 ? NULL : dtFrontStep;
    gTitleSel = (uint16_t)(dt_u32() % 8 == 0 ? dt_range(6, 9) : dt_range(0, 5));
    gAutoTick = (uint16_t)dt_range(0, 3);
    gTitleStage = (uint16_t)dt_range(0, 1);
    gAutoLoadOk = dt_u32();
    gAutoSlot = (uint16_t)dt_range(0, 1);
    gTitleIdle = (uint16_t)(dt_u32() % 5 == 0 ? dt_range(0x700, 0x709) : dt_range(0, 20));
    gTitleDelay = dt_range(-1, 4);
    gTitleFrames = (uint16_t)dt_u32();
    gScrollTick = (uint16_t)(dt_u32() % 4 == 0 ? dt_range(0, 0x3c) : dt_range(0, 0x30));
    gPlayerCount = (uint8_t)dt_u32();
    gGameModeSel = (uint8_t)dt_u32();
    gAutoLoadDone = dt_u32();
    gGuiPulse = dt_range(0x10, 0xf0);
    gLanguageCurrent = dt_u32();
    gSoundVolume0 = (uint8_t)dt_u32();
    gSoundVolume1 = (uint8_t)dt_u32();
    gGuiAssetsLoaded = (int)dt_u32();
    if (dt_u32() % 2 == 0) { /* the carousel tables with random values instead of the exe's */
        for (int i = 0; i < 7; i++) {
            gMenuSlotPos[i].x = (int16_t)dt_range(-300, 700);
            gMenuSlotPos[i].y = (int16_t)dt_range(-100, 500);
            gMenuSlotAlpha[i] = (int16_t)dt_range(-0x20, 0xff);
            gMenuSlotTint[i] = (int16_t)dt_range(-0x20, 0x1ff);
        }
        for (int i = 0; i < 6; i++) {
            gMenuEntryStrings[i] = (uint16_t)dt_range(0, 0x3ff);
        }
    }
}

static void setup_state(DtCtx *c)
{
    titleState(c);
}

static void setup_autoload_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTickAll, kGuiMsgTickAll, kGuiMsgTickAll,
                               kGuiMsgTickAll, kGuiMsgDrawAll, kGuiMsgButton24};
    titleState(c);
    static const uint16_t ticks[] = {0, 4, 4, 8, 8, 0x78, 1, 0x79, 0xffff};
    gAutoTick = ticks[dt_u32() % (sizeof ticks / sizeof ticks[0])];
    dtFrontendHookArgs(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
}

static void setup_press_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTickAll, kGuiMsgTickAll, kGuiMsgTickAll, kGuiMsgButton24,
                               0x31,           kGuiMsgDrawAll, kGuiMsgButton22};
    titleState(c);
    static const uint32_t actions[] = {0, 0, 0x40137f, 0x4015dc, 0};
    const uint32_t pick = actions[dt_u32() % (sizeof actions / sizeof actions[0])];
    if (pick != 0 && gTitleAction != NULL) {
        gTitleAction = (TitleActionFn)(uintptr_t)pick;
    }
    dtFrontendHookArgs(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
}

static void setup_menu_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTickAll,
                               kGuiMsgTickAll,
                               kGuiMsgNavUp,
                               kGuiMsgNavDown,
                               kGuiMsgNavUpAlt,
                               kGuiMsgNavDownAlt,
                               kGuiMsgNavLeft,
                               kGuiMsgNavRight,
                               0x23,
                               0x29,
                               kGuiMsgNavLeftAlt,
                               kGuiMsgNavRightAlt,
                               0x30,
                               0x36,
                               kGuiMsgButton22,
                               kGuiMsgButton22,
                               kGuiMsgButton24,
                               kGuiMsgButton24,
                               0x2f,
                               0x31,
                               kGuiMsgButton25,
                               kGuiMsgIdle,
                               kGuiMsgDrawAll};
    titleState(c);
    dtFrontendHookArgs(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
}

static void setup_blank_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTickAll, kGuiMsgTickAll, kGuiMsgDrawAll, kGuiMsgButton24};
    titleState(c);
    dtFrontendHookArgs(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
}

static void setup_main_menu_init(DtCtx *c)
{
    titleState(c);
    DT_ARG_INT(c, 0, c->seed % 2 == 0 ? 0 : (int)(dt_u32() | 1));
}

DT_CASES(frontend_title_cases) = {
    {"autoloadScreenHook", 0x0043f0c0, autoloadScreenHook, 4, DT_RET_INT, setup_autoload_hook, 800, 0},
    {"autoloadScreenInit", 0x0043f730, autoloadScreenInit, 0, DT_RET_VOID, setup_state, 50, 0},
    {"pressStartHook", 0x0043f7a0, pressStartHook, 4, DT_RET_INT, setup_press_hook, 800, 0},
    {"pressStartInit", 0x0043f9b0, pressStartInit, 0, DT_RET_VOID, setup_state, 50, 0},
    {"mainMenuInit", 0x0043fa20, mainMenuInit, 1, DT_RET_INT, setup_main_menu_init, 200, 0},
    {"mainMenuSetHook", 0x0043fb10, mainMenuSetHook, 0, DT_RET_VOID, setup_state, 50, 0},
    {"mainMenuEnter", 0x0043fb40, mainMenuEnter, 0, DT_RET_VOID, setup_state, 50, 0},
    {"mainMenuHook", 0x0043fb80, mainMenuHook, 4, DT_RET_INT, setup_menu_hook, 2000, 0},
    {"mainMenuScrollDown", 0x004400a0, mainMenuScrollDown, 0, DT_RET_VOID, setup_state, 400, 0},
    {"mainMenuScrollUp", 0x004402c0, mainMenuScrollUp, 0, DT_RET_VOID, setup_state, 400, 0},
    {"mainMenuLayout", 0x004404e0, mainMenuLayout, 0, DT_RET_VOID, setup_state, 200, 0},
    {"mainMenuIdleExit", 0x00440d30, mainMenuIdleExit, 0, DT_RET_INT, setup_state, 50, 0},
    {"blankScreenHook", 0x00440d80, blankScreenHook, 4, DT_RET_INT, setup_blank_hook, 200, 0},
};
