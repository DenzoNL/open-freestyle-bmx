/*
 * Save/load slot screen (screen 0xa012): "Save 1", "Save 2", "Load 1", "Load 2" and back. Confirming a
 * slot opens the memory card dialog on its lines (ids 0xa012000d..0x14) and returns here when it closes.
 */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/zprim.h"
#include "zge/zstring.h"

enum {
    kSlotTitle = 0xa0120007,
    kSlotRow0 = 0xa0120008, /* rows 0..4 are consecutive ids */
    kSlotRowCount = 5,
};

/* Opens the screen; the texts are put in by slotScreenLayout on the next tick. */
BMX_ADDR(0x00443e30)
int slotScreenInit(void)
{
    guiRequestScreen(0xa012, BMX_FNPTR(GuiHookFn, 0x004015af, slotScreenHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004014f1, slotScreenLayout);
    return 1;
}

/* Sets the texts and styles of the screen's rows (the card dialog lines get style 3). */
BMX_ADDR(0x00443e70)
void slotScreenLayout(void)
{
    gSlotSel = 0;
    for (uint32_t i = 0; i < kSlotRowCount; i++) {
        menuSetStyle(kSlotRow0 + i, 2);
    }
    menuSetText(kSlotTitle, zstringGet(0x28));
    menuSetText(kSlotRow0 + 0, zstringGet(0x24e));
    menuSetText(kSlotRow0 + 1, zstringGet(0x24f));
    menuSetText(kSlotRow0 + 2, zstringGet(0x250));
    menuSetText(kSlotRow0 + 3, zstringGet(0x251));
    menuSetText(kSlotRow0 + 4, zstringGet(0));
    for (uint32_t id = 0xa012000d; id <= 0xa0120014; id++) {
        menuSetStyle(id, 3);
    }
    gFrontendStep = NULL;
}

/* Leaves the screen through the stored return function. */
BMX_ADDR(0x00444110)
void slotScreenDone(void)
{
    gFrontendStep = NULL;
    gScreenReturnFn(gScreenReturnArg);
}

/* The card dialog closed: shows the rows again (or leaves when the dialog ended with a finished action). */
BMX_ADDR(0x00444140)
void slotScreenReturn(void)
{
    if (gMcState != 7 && gMcState != 0x102) {
        for (uint32_t i = 0; i < 6; i++) {
            menuShow(kSlotTitle + i);
        }
        menuSetText(kSlotTitle, zstringGet(0x28));
        menuSetText(kSlotRow0 + 0, zstringGet(0x24e));
        menuSetText(kSlotRow0 + 1, zstringGet(0x24f));
        menuSetText(kSlotRow0 + 2, zstringGet(0x250));
        menuSetText(kSlotRow0 + 3, zstringGet(0x251));
        menuSetText(kSlotRow0 + 4, zstringGet(0));
        gFrontendStep = NULL;
        guiSetHook(BMX_FNPTR(GuiHookFn, 0x004015af, slotScreenHook));
        return;
    }
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402293, slotScreenDone);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x004015af, slotScreenHook));
}

/* Hides the screen's own elements before the card dialog takes over the display. */
static void slotHideAll(void)
{
    menuHide(0xa0120002);
    menuHide(0xa0120001);
    menuHide(0xa0120004);
    menuHide(0xa0120003);
    menuHide(0xa0120006);
    menuHide(0xa0120005);
    for (uint32_t i = 0; i < 6; i++) {
        menuHide(kSlotTitle + i);
    }
}

BMX_ADDR(0x00444310)
int BMX_CDECL slotScreenHook(int msg, GuiElement *el, intptr_t arg3, intptr_t arg4)
{
    static const uint32_t dialogRows[8] = {0xa012000d, 0xa012000e, 0xa012000f, 0xa0120010,
                                           0xa0120011, 0xa0120012, 0xa0120013, 0xa0120014};
    switch (msg) {
    case kGuiMsgTickAll:
        zprimSet(kZprimColor, 0x10101);
        zprimFillRect(0x28, 0, 0x1db, 0xe8);
        if (gFrontendStep != NULL) {
            gFrontendStep();
        }
        for (uint32_t i = 0; i < kSlotRowCount; i++) {
            menuSetTint(kSlotRow0 + i, 0xc0, 0xc0, 0xc0, 0x80);
        }
        menuSetStyle(kSlotTitle, 2);
        for (uint32_t i = 0; i < kSlotRowCount; i++) {
            menuSetStyle(kSlotRow0 + i, gSlotSel == i);
        }
        if (gSlotSel < kSlotRowCount) {
            menuSetTint(kSlotRow0 + gSlotSel, 0xc0, 0xc0, 0x80, (uint32_t)gGuiPulse + 0x20);
        }
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case kGuiMsgNavUp:
    case kGuiMsgNavUpAlt:
        menuPlaySound(2000);
        gSlotSel = (uint16_t)menuWrap((int)gSlotSel - 1, 5);
        return 1;
    case kGuiMsgNavDown:
    case kGuiMsgNavDownAlt:
        menuPlaySound(2000);
        gSlotSel = (uint16_t)menuWrap((int)gSlotSel + 1, 5);
        return 1;
    case kGuiMsgButton22:
    case kGuiMsgButton24:
    case 0x2f:
    case 0x31:
        switch (gSlotSel) {
        case 0:
        case 1:
            slotHideAll();
            menuPlaySound(0x7d2);
            mcDialogOpen(1, gSlotSel, dialogRows, 1, BMX_FNPTR(McDoneFn, 0x00401fd7, slotScreenReturn));
            return 1;
        case 2:
        case 3:
            slotHideAll();
            menuPlaySound(0x7d2);
            mcDialogOpen(0, (uint16_t)(gSlotSel - 2), dialogRows, 1,
                         BMX_FNPTR(McDoneFn, 0x00401fd7, slotScreenReturn));
            return 1;
        case 4:
            break;
        default:
            return 1;
        }
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402293, slotScreenDone);
        menuPlaySound(0x7d2);
        return 1;
    case kGuiMsgButton25:
    case kGuiMsgIdle:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402293, slotScreenDone);
        if (gGuiIdleMode != 0) {
            return 1;
        }
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402293, slotScreenDone);
        menuPlaySound(0x7d2);
        return 1;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
}
