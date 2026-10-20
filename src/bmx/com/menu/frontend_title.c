/*
 * Title screens (screen 0xa005): the autoload check, "press start" and the main menu, a carousel of
 * seven entries (a..0x10) that scrolls through six menu strings.
 */
#include "bmx/com/action/memcard.h"
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"
#include "zge/zstring.h"

enum {
    kTitlePrompt = 0xa0050001,
    kAutoLine0 = 0xa0050002,
    kAutoLine1 = 0xa0050003,
    kAutoLine2 = 0xa0050004,
    kAutoLine3 = 0xa0050005,
    kMenuSlotFirst = 0xa005000a,
    kMenuSlotCount = 7,
    kMenuHighlight = 0xa005000d,
};

#define EMPTY_TEXT ((const char *)&gGuiEmptyChar)

/* Puts "<string 0x25d> <string 0x25c>" into a line. */
static void autoloadSetFoundText(uint32_t id)
{
    char buffer[100];
    zstrFormat(buffer, "%s %s", zstringGet(0x25d), zstringGet(0x25c));
    menuSetText(id, buffer);
}

/* Puts "Slot n" (string 0x269) into a line. */
static void autoloadSetSlotText(uint32_t id, int slot)
{
    char buffer[100];
    zstrFormat(buffer, zstringGet(0x269), slot);
    menuSetText(id, buffer);
}

BMX_ADDR(0x0043f0c0)
int BMX_CDECL autoloadScreenHook(int msg, GuiElement *el, int arg3, int arg4)
{
    if (msg != kGuiMsgTickAll) {
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
    zprimSet(kZprimColor, 0x10101);
    zprimFillRect(0x3c, 0x104, 0x1c7, 0x168);
    switch (gAutoTick) {
    case 0:
        menuShow(kAutoLine0);
        menuShow(kAutoLine1);
        menuSetText(kAutoLine0, zstringGet(0x259));
        menuSetText(kAutoLine1, EMPTY_TEXT);
        menuSetText(kAutoLine2, EMPTY_TEXT);
        menuSetText(kAutoLine3, EMPTY_TEXT);
        break;
    case 4: {
        /* Slot 0 first; an unformatted card is formatted and slot 1 tried next. */
        memcardBegin();
        memcardSelectSlot(0);
        int code = saveFileClose();
        if (code == 6) {
            gAutoSlot = 0;
            autoloadSetFoundText(kAutoLine0);
            autoloadSetSlotText(kAutoLine1, 1);
            menuSetText(kAutoLine2, EMPTY_TEXT);
            menuSetText(kAutoLine3, EMPTY_TEXT);
            memcardEnd();
            break;
        }
        if (code == 5) {
            memcardFormat();
        }
        memcardSelectSlot(1);
        code = saveFileClose();
        const char *line1;
        if (code == 5) {
            memcardFormat();
            menuSetText(kAutoLine0, zstringGet(0x262));
            line1 = EMPTY_TEXT;
        } else if (code == 6) {
            gAutoSlot = 1;
            autoloadSetFoundText(kAutoLine0);
            autoloadSetSlotText(kAutoLine1, 2);
            memcardEnd();
            break;
        } else {
            menuSetText(kAutoLine0, zstringGet(0x262));
            line1 = zstringGet(0x260);
        }
        menuSetText(kAutoLine1, line1);
        gAutoTick = 8;
        memcardEnd();
        break;
    }
    case 8: {
        memcardBegin();
        memcardSelectSlot(gAutoSlot);
        const int code = saveFileRead();
        uint32_t id;
        const char *text;
        if (code == 0) {
            frontendSetLanguage(gLanguageCurrent & 0xff);
            gAutoLoadOk = 1;
            menuSetText(kAutoLine0, zstringGet(0x25e));
            text = EMPTY_TEXT;
            id = kAutoLine1;
        } else {
            if (code == -1) {
                menuSetText(kAutoLine1, zstringGet(0x272));
            }
            text = zstringGet(0x261);
            id = kAutoLine0;
        }
        menuSetText(id, text);
        menuSetText(kAutoLine2, EMPTY_TEXT);
        menuSetText(kAutoLine3, EMPTY_TEXT);
        memcardEnd();
        break;
    }
    case 0x78:
        mainMenuSetHook();
        menuSetChannelVolume(1, gSoundVolume1 * 7);
        menuSetChannelVolume(0, gSoundVolume0 * 7);
        menuPlaySound(0x7d9);
        break;
    default:
        break;
    }
    gAutoTick++;
    return guiDefaultHookProc(msg, el, arg3, arg4);
}

/* Opens the title screen with the autoload hook; clears the screen state. */
BMX_ADDR(0x0043f730)
void autoloadScreenInit(void)
{
    guiDestroyAll();
    guiLoadBackdrop(0xa007, 1);
    guiRequestScreen(0xa005, BMX_FNPTR(GuiHookFn, 0x004018ac, autoloadScreenHook));
    gTitleAction = NULL;
    gTitleFrames = 0;
    gTitleDelay = 0;
    gTitleIdle = 0;
    gTitleSel = 0;
    gAutoLoadDone = 0;
}

/* The "press start" screen: blinks the prompt; start moves on to the main menu, long idling to the attract
 * mode. */
BMX_ADDR(0x0043f7a0)
int BMX_CDECL pressStartHook(int msg, GuiElement *el, int arg3, int arg4)
{
    if (msg == kGuiMsgTickAll) {
        gTitleIdle++;
        if (gTitleIdle > 0x708) {
            gTitleDelay = 0;
            gTitleIdle = 0;
            gTitleAction = BMX_FNPTR(TitleActionFn, 0x004015dc, mainMenuIdleExit);
        }
        if (gTitleStage == 0) {
            menuShow(kTitlePrompt);
            menuSetText(kTitlePrompt, zstringGet(0x23));
            menuSetStyle(kTitlePrompt, 1);
            guiSetTextFlags(guiFindById(kTitlePrompt), 0x2c);
            gTitleStage = 1;
        }
        if (gTitleAction != NULL) {
            const int waited = gTitleDelay++;
            if (waited > 2) {
                menuHide(kTitlePrompt);
                TitleActionFn action = gTitleAction;
                if (action == BMX_FNPTR(TitleActionFn, 0x0040137f, mainMenuInit)) {
                    mainMenuInit(1);
                } else {
                    action();
                }
                gTitleAction = NULL;
            }
        } else {
            const uint8_t level = (uint8_t)(gGuiPulse / 2 + 0x7f);
            menuSetTint(kTitlePrompt, level, level, level, (uint32_t)(int32_t)(int8_t)level);
        }
        gTitleFrames++;
    } else if (msg == kGuiMsgButton24 || msg == 0x31) {
        gTitleAction = BMX_FNPTR(TitleActionFn, 0x0040137f, mainMenuInit);
        gTitleIdle = 0;
        gTitleDelay = 0;
    }
    return guiDefaultHookProc(msg, el, arg3, arg4);
}

/* Opens the title screen with the "press start" hook. */
BMX_ADDR(0x0043f9b0)
void pressStartInit(void)
{
    guiDestroyAll();
    guiLoadBackdrop(0xa007, 1);
    guiRequestScreen(0xa005, BMX_FNPTR(GuiHookFn, 0x00402504, pressStartHook));
    gTitleAction = NULL;
    gTitleFrames = 0;
    gTitleDelay = 0;
    gTitleIdle = 0;
    gTitleSel = 0;
    gTitleStage = 0;
    gAutoLoadDone = 0;
}

/* Opens the main menu (screen 0xa005 with the menu hook). With skipLoad the saved game is not re-read. */
BMX_ADDR(0x0043fa20)
int mainMenuInit(int skipLoad)
{
    guiDestroyAll();
    guiLoadBackdrop(0xa007, 1);
    guiRequestScreen(0xa005, BMX_FNPTR(GuiHookFn, 0x00401488, mainMenuHook));
    gTitleAction = NULL;
    gTitleUnknown551fd4 = 0;
    gTitleFrames = 0;
    gTitleDelay = 0;
    gTitleIdle = 0;
    mainMenuSetHook();
    if (skipLoad == 0) {
        gTitleSel = 0;
        saveFileRead();
        menuSetChannelVolume(1, gSoundVolume1 * 7);
        menuSetChannelVolume(0, gSoundVolume0 * 7);
        menuPlaySound(0x7d9);
        return 0;
    }
    gGuiAssetsLoaded = 0;
    return 0;
}

/* Installs the menu hook on the current screen and schedules the carousel layout. */
BMX_ADDR(0x0043fb10)
void mainMenuSetHook(void)
{
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x00401488, mainMenuHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401cf3, mainMenuLayout);
}

/* Re-enters the main menu screen without touching the backdrop. */
BMX_ADDR(0x0043fb40)
void mainMenuEnter(void)
{
    guiRequestScreen(0xa005, BMX_FNPTR(GuiHookFn, 0x00401488, mainMenuHook));
    gTitleAction = NULL;
    gTitleFrames = 0;
    gTitleDelay = 0;
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401cf3, mainMenuLayout);
}

/* Hides the entries once an action is pending, so they do not show while the next screen loads. */
static void mainMenuHideEntries(void)
{
    menuHide(0xa005000a);
    menuHide(0xa005000b);
    menuHide(0xa005000c);
    menuSetTint(kMenuHighlight, 0xff, 0xff, 0x96, 0x80);
    menuHide(0xa005000e);
    menuHide(0xa005000f);
    menuHide(0xa0050010);
    menuHide(0xa0050006);
    menuHide(0xa0050007);
    menuHide(0xa0050008);
    menuHide(0xa0050009);
}

BMX_ADDR(0x0043fb80)
int BMX_CDECL mainMenuHook(int msg, GuiElement *el, int arg3, int arg4)
{
    switch (msg) {
    case kGuiMsgTickAll:
        gTitleIdle++;
        if (gTitleIdle > 0x708) {
            gTitleDelay = 0;
            gTitleIdle = 0;
            gTitleAction = BMX_FNPTR(TitleActionFn, 0x004015dc, mainMenuIdleExit);
        }
        gMenuSlotAlpha[3] = (int16_t)gGuiPulse;
        if (gFrontendStep != NULL) {
            gFrontendStep();
        } else if (gTitleAction == NULL) {
            menuSetTint(kMenuHighlight, 0xff, 0xff, 0x96, (uint32_t)gGuiPulse);
        }
        if (gTitleAction != NULL) {
            const int waited = gTitleDelay++;
            if (waited > 2) {
                gTitleAction();
                gTitleAction = NULL;
            }
        }
        gTitleFrames++;
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case kGuiMsgNavUp:
    case kGuiMsgNavUpAlt:
        gTitleIdle = 0;
        if (gFrontendStep == NULL) {
            gScrollTick = 0;
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004011f4, mainMenuScrollUp);
        }
        return 1;
    case kGuiMsgNavDown:
    case kGuiMsgNavDownAlt:
        gTitleIdle = 0;
        if (gFrontendStep == NULL) {
            gScrollTick = 0;
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401375, mainMenuScrollDown);
        }
        return 1;
    case kGuiMsgNavLeft:
    case kGuiMsgNavRight:
    case 0x23:
    case 0x29:
    case kGuiMsgNavLeftAlt:
    case kGuiMsgNavRightAlt:
    case 0x30:
    case 0x36:
        gTitleIdle = 0;
        return 1;
    case kGuiMsgButton22:
    case kGuiMsgButton24:
    case 0x2f:
    case 0x31:
        gTitleIdle = 0;
        if (gFrontendStep != NULL) {
            return 1;
        }
        switch (gTitleSel) {
        case 0:
        case 1:
        case 2:
            if (msg != 0x2f && msg != 0x31) {
                gPlayerCount = 1;
                menuPlaySound(0x7d2);
                gTitleDelay = 0;
                gTitleAction = BMX_FNPTR(TitleActionFn, 0x004024a0, selScreenOpen);
                gGameModeSel = (uint8_t)gTitleSel;
            } else {
                menuPlaySound(0x7d4);
            }
            break;
        case 3:
            menuPlaySound(0x7d2);
            gTitleDelay = 0;
            gTitleAction = BMX_FNPTR(TitleActionFn, 0x0040210d, extrasMenuInit);
            break;
        case 4:
            menuPlaySound(0x7d2);
            gTitleDelay = 0;
            gTitleAction = BMX_FNPTR(TitleActionFn, 0x004020f9, optionsInit);
            break;
        case 5:
            if (zrwConfirmQuit() != 0) {
                frontendRequestExit(1);
                menuPlaySound(0x7d2);
            }
            break;
        default:
            menuPlaySound(0x7d4);
            break;
        }
        if (gTitleAction != NULL) {
            mainMenuHideEntries();
        }
        return 1;
    case kGuiMsgButton25:
    case kGuiMsgIdle:
        gTitleIdle = 0;
        return 0;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
}

/*
 * One tick of the carousel scroll: slot k moves between table entries k and k+1 as the tick runs from 0
 * to 0x3c (position, tint and alpha interpolated linearly, in steps of 1/60 of the distance). Scrolling
 * down moves slots 1..6 up into entries 0..5, scrolling up moves slots 0..5 down into entries 1..6.
 */
static void mainMenuScroll(int down)
{
    gScrollTick = (uint16_t)(gScrollTick + 6);
    const uint32_t t = gScrollTick;
    for (int k = 0; k < 6; k++) {
        const uint32_t id = kMenuSlotFirst + (uint32_t)k + (down ? 1u : 0u);
        const int32_t qx = (int32_t)((uint32_t)(gMenuSlotPos[k + 1].x - gMenuSlotPos[k].x) * t) / 60;
        const int32_t qy = (int32_t)((uint32_t)(gMenuSlotPos[k + 1].y - gMenuSlotPos[k].y) * t) / 60;
        const int32_t qc = (int32_t)((uint32_t)(gMenuSlotTint[k + 1] - gMenuSlotTint[k]) * t) / 60;
        const int32_t qa = (int32_t)((uint32_t)(gMenuSlotAlpha[k + 1] - gMenuSlotAlpha[k]) * t) / 60;
        int16_t x, y, alpha;
        uint8_t level;
        if (down) {
            x = (int16_t)(gMenuSlotPos[k + 1].x - qx);
            y = (int16_t)(gMenuSlotPos[k + 1].y - qy);
            level = (uint8_t)(gMenuSlotTint[k + 1] - qc);
            alpha = (int16_t)(gMenuSlotAlpha[k + 1] - qa);
        } else {
            x = (int16_t)(gMenuSlotPos[k].x + qx);
            y = (int16_t)(gMenuSlotPos[k].y + qy);
            level = (uint8_t)(gMenuSlotTint[k] + qc);
            alpha = (int16_t)(gMenuSlotAlpha[k] + qa);
        }
        guiSetTextPos(guiFindById(id), x, (int16_t)(y - 0x8c));
        guiSetTint(guiFindById(id), level, level, level, (uint32_t)(int32_t)alpha);
    }
    if (gScrollTick >= 0x3c) {
        gTitleSel = (uint16_t)menuWrap((int)gTitleSel + (down ? 1 : -1), 6);
        mainMenuLayout();
        if (down) {
            gFrontendStep = NULL;
        }
        menuPlaySound(2000);
    }
}

/* Scroll step for "down": the entries move up one place. */
BMX_ADDR(0x004400a0)
void mainMenuScrollDown(void)
{
    mainMenuScroll(1);
}

/* Scroll step for "up": the entries move down one place. */
BMX_ADDR(0x004402c0)
void mainMenuScrollUp(void)
{
    mainMenuScroll(0);
}

/* Lays out the carousel: entry texts around the selected one, slot positions, styles and tints. */
BMX_ADDR(0x004404e0)
void mainMenuLayout(void)
{
    menuHide(kAutoLine0);
    menuHide(kAutoLine1);
    for (int k = 0; k < kMenuSlotCount; k++) {
        menuShow(kMenuSlotFirst + (uint32_t)k);
    }
    for (int k = 0; k < kMenuSlotCount; k++) {
        const int entry = menuWrap((int)gTitleSel + k - 3, 6);
        menuSetText(kMenuSlotFirst + (uint32_t)k, zstringGet(gMenuEntryStrings[entry]));
    }
    for (int k = 0; k < kMenuSlotCount; k++) {
        guiSetTextPos(guiFindById(kMenuSlotFirst + (uint32_t)k), gMenuSlotPos[k].x,
                      (int16_t)(gMenuSlotPos[k].y - 0x8c));
    }
    for (int k = 0; k < kMenuSlotCount; k++) {
        menuSetStyle(kMenuSlotFirst + (uint32_t)k, 1);
    }
    for (int k = 0; k < kMenuSlotCount; k++) {
        guiSetFlags(guiFindById(kMenuSlotFirst + (uint32_t)k), 0x400);
    }
    for (int k = 0; k < kMenuSlotCount; k++) {
        guiSetTextFlags(guiFindById(kMenuSlotFirst + (uint32_t)k), 0xc);
    }
    for (int k = 0; k < kMenuSlotCount; k++) {
        const uint8_t level = (uint8_t)gMenuSlotTint[k];
        menuSetTint(kMenuSlotFirst + (uint32_t)k, level, level, level, (uint32_t)(int32_t)gMenuSlotAlpha[k]);
    }
    gFrontendStep = NULL;
}

/* Attract mode: leaves the menu for the blank screen that ends the idle demo. */
BMX_ADDR(0x00440d30)
int mainMenuIdleExit(void)
{
    gPlayerCount = 0;
    guiDestroyAll();
    guiRequestScreen(0, BMX_FNPTR(GuiHookFn, 0x00401573, blankScreenHook));
    gFrontendStep = NULL;
    gTitleDelay = 0;
    return 1;
}

/* Hook of the blank screen: after a few frames asks for the transition with code 0. */
BMX_ADDR(0x00440d80)
int BMX_CDECL blankScreenHook(int msg, GuiElement *el, int arg3, int arg4)
{
    if (msg == kGuiMsgTickAll) {
        const int waited = gTitleDelay++;
        if (waited > 4) {
            frontendRequestExit(0);
        }
    }
    return 1;
}
