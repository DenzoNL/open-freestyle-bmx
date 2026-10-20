/* Tests for frontend_language.c. */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/zge/zprim.h"
#include "dt.h"
#include "dt_frontend.h"
#include "zge/zstring.h"

static void BMX_CDECL stubSetLanguage(uint32_t language)
{
    dtFrontLog(0x200, language, 0, 0, 0);
}

static void randomLanguageState(void)
{
    gLanguageTable[0] = (uint8_t)dt_range(0, 5);
    gLanguageTable[1] = (uint8_t)dt_range(0, 5);
    gLanguageTable[2] = (uint8_t)dt_range(0, 5);
    gLanguageCurrent = dt_u32() & 0xffffff07u;
    gLanguageSel = (uint16_t)(dt_u32() % 9 == 0 ? dt_range(3, 6) : dt_range(0, 2));
}

static void setup_init(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    dtFrontendStub(DT_FS_SCREENS);
    randomLanguageState();
    if (dt_u32() % 3 == 0) { /* the current language is not offered, or offered twice */
        gLanguageCurrent = 0x1234;
    }
    gFrontendStep = NULL;
}

static void setup_layout(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    static const uint32_t ids[] = {0xa0140003, 0xa0140004, 0xa0140005, 0xa0140006};
    dtFrontendElements(ids, 4);
    gFrontendStep = dtFrontStep;
}

static void setup_done(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    gFrontendStep = dtFrontStep;
    dt_stub(0x0043fb40, mainMenuEnter, (void *)dtFrontStep);
}

static void setup_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTickAll,
                               kGuiMsgTickAll,
                               kGuiMsgNavUp,
                               kGuiMsgNavUpAlt,
                               kGuiMsgNavDown,
                               kGuiMsgNavDownAlt,
                               kGuiMsgButton22,
                               kGuiMsgButton24,
                               0x2f,
                               0x31,
                               kGuiMsgButton25,
                               kGuiMsgIdle,
                               kGuiMsgDrawAll};
    dt_seed(c);
    dtFrontendReset();
    dtFrontendStub(DT_FS_SOUND | DT_FS_ZPRIM);
    dt_stub(0x0044dad0, frontendSetLanguage, (void *)stubSetLanguage);
    static const uint32_t ids[] = {0xa0140003, 0xa0140004, 0xa0140005, 0xa0140006};
    dtFrontendElements(ids, 4);
    randomLanguageState();
    gFrontendStep = dt_u32() % 3 == 0 ? NULL : dtFrontStep;
    gGuiIdleMode = dt_range(0, 1);
    gGuiPulse = dt_range(0x10, 0xf0);
    dtFrontendHookArgs(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
}

DT_CASES(frontend_language_cases) = {
    {"languageScreenInit", 0x0043cd80, languageScreenInit, 0, DT_RET_INT, setup_init, 100, 0},
    {"languageScreenLayout", 0x0043cdf0, languageScreenLayout, 0, DT_RET_VOID, setup_layout, 100, 0},
    {"languageScreenDone", 0x0043cf10, languageScreenDone, 0, DT_RET_VOID, setup_done, 20, 0},
    {"languageScreenHook", 0x0043cf40, languageScreenHook, 4, DT_RET_INT, setup_hook, 600, 0},
};
