/* Language selection screen (screen 0xa014): three rows, the one in `gLanguageSel` highlighted. */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/zprim.h"
#include "zge/zstring.h"

enum { kLangTitle = 0xa0140003, kLangRow0 = 0xa0140004, kLangRow1 = 0xa0140005, kLangRow2 = 0xa0140006 };

/* Opens the screen with the current language selected. */
BMX_ADDR(0x0043cd80)
int languageScreenInit(void)
{
    guiRequestScreen(0xa014, BMX_FNPTR(GuiHookFn, 0x004022d9, languageScreenHook));
    gLanguageSel = 0;
    for (uint32_t i = 0; i < 3; i++) {
        if (gLanguageTable[i] == (uint8_t)gLanguageCurrent) {
            gLanguageSel = (uint16_t)i;
            break;
        }
    }
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401b27, languageScreenLayout);
    return 0;
}

/* Sets the row styles and texts once, right after the screen loaded. */
BMX_ADDR(0x0043cdf0)
void languageScreenLayout(void)
{
    menuSetStyle(kLangRow0, 2);
    menuSetStyle(kLangRow1, 2);
    menuSetStyle(kLangRow2, 2);
    menuSetText(kLangTitle, zstringGet(0x249));
    menuSetText(kLangRow0, zstringGet(0x24b));
    menuSetText(kLangRow1, zstringGet(0x24d));
    menuSetText(kLangRow2, zstringGet(0x24c));
    gFrontendStep = NULL;
}

/* Leaves the language screen for the title screen. */
BMX_ADDR(0x0043cf10)
void languageScreenDone(void)
{
    gFrontendStep = NULL;
    mainMenuEnter();
}

BMX_ADDR(0x0043cf40)
int BMX_CDECL languageScreenHook(int msg, GuiElement *el, int arg3, int arg4)
{
    int next;
    switch (msg) {
    case kGuiMsgTickAll:
        zprimSet(kZprimColor, 0x10101);
        zprimFillRect(0x28, 0, 0x1db, 200);
        if (gFrontendStep != NULL) {
            gFrontendStep();
        }
        menuSetTint(kLangRow0, 0xc0, 0xc0, 0xc0, 0x80);
        menuSetTint(kLangRow1, 0xc0, 0xc0, 0xc0, 0x80);
        menuSetTint(kLangRow2, 0xc0, 0xc0, 0xc0, 0x80);
        menuSetStyle(kLangTitle, 2);
        menuSetStyle(kLangRow0, gLanguageSel == 0);
        menuSetStyle(kLangRow1, gLanguageSel == 1);
        menuSetStyle(kLangRow2, gLanguageSel == 2);
        if (gLanguageSel == 0) {
            menuSetTint(kLangRow0, 0xc0, 0xc0, 0x80, (uint32_t)gGuiPulse + 0x20);
        } else if (gLanguageSel == 1) {
            menuSetTint(kLangRow1, 0xc0, 0xc0, 0x80, (uint32_t)gGuiPulse + 0x20);
        } else if (gLanguageSel == 2) {
            menuSetTint(kLangRow2, 0xc0, 0xc0, 0x80, (uint32_t)gGuiPulse + 0x20);
        }
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case kGuiMsgNavUp:
    case kGuiMsgNavUpAlt:
        menuPlaySound(2000);
        next = (int)gLanguageSel - 1;
        break;
    case kGuiMsgNavDown:
    case kGuiMsgNavDownAlt:
        menuPlaySound(2000);
        next = (int)gLanguageSel + 1;
        break;
    case kGuiMsgButton22:
    case kGuiMsgButton24:
    case 0x2f:
    case 0x31:
        gLanguageCurrent = (gLanguageCurrent & 0xffffff00u) | gLanguageTable[gLanguageSel];
        frontendSetLanguage(gLanguageCurrent & 0xff);
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402266, languageScreenDone);
        menuPlaySound(0x7d2);
        return 1;
    case kGuiMsgButton25:
    case kGuiMsgIdle:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402266, languageScreenDone);
        if (gGuiIdleMode == 0) {
            menuPlaySound(0x7d3);
        }
        return 1;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
    gLanguageSel = (uint16_t)menuWrap(next, 3);
    return 1;
}
