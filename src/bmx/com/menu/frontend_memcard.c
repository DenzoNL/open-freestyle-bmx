/*
 * Memory card dialog: a box of up to 8 text lines (ids copied from the caller) that walks through the
 * steps of loading or saving the game file. gMcState picks the step the tick runs once the countdown
 * has run out; the steps talk to the card and put the result or the question into the lines.
 */
#include "bmx/com/action/memcard.h"
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"
#include "zge/zstring.h"

enum {
    kMcRowButton0 = 6,
    kMcRowButton1 = 7,
};

enum McState {
    kMcSaveProbe = 0,
    kMcPrompt1 = 1,
    kMcPrompt3 = 3,
    kMcPrompt4 = 4,
    kMcSaveFormat = 5,
    kMcSaveWrite = 6,
    kMcClose = 0xb,
    kMcLoadProbe = 0x100,
    kMcLoadRead = 0x101,
};

static void mcClearRows(int first)
{
    for (int i = first; i < 8; i++) {
        guiSetText(guiFindById(gMcRows[i]), (const char *)&gGuiEmptyChar);
    }
}

static void mcSetRow(int row, const char *text)
{
    guiSetText(guiFindById(gMcRows[row]), text);
}

/* Puts "Slot n" (string 0x269 with the slot number) into a row. */
static void mcSetSlotRow(int row)
{
    char buffer[128];
    zstrFormat(buffer, zstringGet(0x269), (int)gMcSlot + 1);
    mcSetRow(row, buffer);
}

/*
 * Opens the dialog on `slot`: copies the row ids, clears and shows the rows and sets the first lines.
 * Starts the save flow when `save` is non-zero, the load flow otherwise. `done` runs when the dialog
 * closes.
 */
BMX_ADDR(0x0043d320)
void mcDialogOpen(int16_t save, uint16_t slot, const uint32_t *rows, uint16_t mode, McDoneFn done)
{
    gMcSlot = slot;
    gMcLoadOk = 0;
    gMcSaveOk = 0;
    gMcDone = done;
    gMcMode = mode;
    for (int i = 0; i < 8; i++) {
        gMcRows[i] = rows[i];
    }
    for (int i = 0; i < 8; i++) {
        guiSetText(guiFindById(gMcRows[i]), (const char *)&gGuiEmptyChar);
        guiShow(guiFindById(gMcRows[i]));
    }
    mcSetRow(0, zstringGet(0x26e));
    mcSetSlotRow(1);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x004021e4, mcDialogHook));
    gFrontendStep = NULL;
    gMcCountdown = 4;
    gMcState = save != 0 ? kMcSaveProbe : kMcLoadProbe;
}

/* Picks the step function that matches the dialog state. */
static FrontendStepFn mcStepForState(void)
{
    switch (gMcState) {
    case kMcSaveProbe:
        return BMX_FNPTR(FrontendStepFn, 0x0040198d, mcStepSaveProbe);
    case kMcPrompt1:
        return BMX_FNPTR(FrontendStepFn, 0x0040211c, mcStepPrompt1);
    case kMcPrompt3:
        return BMX_FNPTR(FrontendStepFn, 0x00401127, mcStepPrompt3);
    case kMcPrompt4:
        return BMX_FNPTR(FrontendStepFn, 0x00401e7e, mcStepPrompt4);
    case kMcSaveFormat:
        return BMX_FNPTR(FrontendStepFn, 0x004016e5, mcStepSaveFormat);
    case kMcSaveWrite:
        return BMX_FNPTR(FrontendStepFn, 0x00401d16, mcStepSaveWrite);
    case 7:
    case 8:
    case 10:
    case 0x102:
    case 0x103:
        return BMX_FNPTR(FrontendStepFn, 0x004020cc, mcStepMessage);
    case kMcClose:
        return BMX_FNPTR(FrontendStepFn, 0x004019dd, mcStepClose);
    case kMcLoadProbe:
        return BMX_FNPTR(FrontendStepFn, 0x004025e5, mcStepLoadProbe);
    case kMcLoadRead:
        return BMX_FNPTR(FrontendStepFn, 0x004021df, mcStepLoadRead);
    default:
        return gFrontendStep;
    }
}

/* A prompt was answered "yes": replaces the lines by a status text for the slot and moves to `next`. */
static void mcAnswerYes(uint16_t title, uint16_t next)
{
    mcClearRows(0);
    mcSetRow(0, zstringGet(title));
    mcSetRow(1, zstringGet(0x25c));
    mcSetSlotRow(2);
    gMcState = next;
    gMcCountdown = 3;
}

BMX_ADDR(0x0043d4a0)
int BMX_CDECL mcDialogHook(int msg, GuiElement *el, intptr_t arg3, intptr_t arg4)
{
    switch (msg) {
    case kGuiMsgTickAll:
        zprimSet(kZprimColor, 0x10101);
        zprimFillRect(0x28, 0, 0x1db, 0xe8);
        if (gMcCountdown >= 0) {
            gMcCountdown--;
        } else {
            gFrontendStep = mcStepForState();
            if (gFrontendStep != NULL) {
                gFrontendStep();
            }
        }
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case kGuiMsgNavUp:
    case kGuiMsgNavDown:
    case 0x27:
    case 0x29:
    case kGuiMsgNavUpAlt:
    case kGuiMsgNavDownAlt:
    case 0x34:
    case 0x36:
        return 1;
    case kGuiMsgNavLeft:
    case kGuiMsgNavRight:
    case kGuiMsgNavLeftAlt:
    case kGuiMsgNavRightAlt:
        if (gMcState == 1 || (gMcState > 2 && gMcState < 5)) {
            gMcChoice ^= 1;
            menuPlaySound(2000);
        }
        return 0;
    case kGuiMsgButton22:
    case kGuiMsgButton24:
    case 0x2f:
    case 0x31:
        if (gMcState < 0x106) {
            if (gMcState < 0x102) {
                switch (gMcState) {
                case 1:
                    if (gMcChoice != 0) {
                        mcAnswerYes(0x26f, kMcSaveFormat);
                        return 0;
                    }
                    break;
                case 3:
                case 4:
                    if (gMcChoice != 0) {
                        mcAnswerYes(0x265, kMcSaveWrite);
                        return 0;
                    }
                    break;
                case 7:
                case 8:
                case 10:
                    break;
                default:
                    return 0;
                }
            }
            gMcState = kMcClose;
            gMcCountdown = 3;
        }
        return 0;
    case kGuiMsgButton25:
    case kGuiMsgIdle:
        if (gGuiIdleMode == 0) {
            if (gMcState < 7) {
                if (gMcState > 4 || gMcState == 0) {
                    return 0;
                }
            } else if (gMcState >= 0x100 && gMcState < 0x102) {
                return 0;
            }
        }
        gMcCountdown = 3;
        gMcState = kMcClose;
        return 1;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
}

/* Final step: plays the confirm sound, hides the lines, tells the caller and parks the dialog. */
BMX_ADDR(0x0043db20)
void mcStepClose(void)
{
    menuPlaySound(0x7d2);
    for (int i = 0; i < 8; i++) {
        guiHide(guiFindById(gMcRows[i]));
    }
    gMcDone();
    gMcState = 0xc;
}

/* Load flow, first step: checks the card; a readable one moves on to reading, anything else shows why not. */
BMX_ADDR(0x0043db90)
void mcStepLoadProbe(void)
{
    memcardBegin();
    memcardSelectSlot(gMcSlot);
    const int code = saveFileClose();
    mcShowLoadResult(code);
    gMcCountdown = 3;
    if (code != 6) {
        gMcState = 0x103;
        memcardEnd();
        return;
    }
    mcClearRows(0);
    mcSetRow(0, zstringGet(0x25d));
    mcSetRow(1, zstringGet(0x25c));
    mcSetSlotRow(2);
    gMcState = kMcLoadRead;
    memcardEnd();
}

/* Load flow, second step: reads the file, re-applies the language and the stored sound volumes. */
BMX_ADDR(0x0043dd00)
void mcStepLoadRead(void)
{
    menuPlaySound(0x7d8);
    memcardBegin();
    memcardSelectSlot(gMcSlot);
    const int code = saveFileRead();
    gMcState = (uint16_t)((code != 0) + 0x102);
    memcardEnd();
    frontendSetLanguage(gLanguageCurrent & 0xff);
    menuSetChannelVolume(1, gSoundVolume1 * 7);
    menuSetChannelVolume(0, gSoundVolume0 * 7);
    menuPlaySound(0x7d9);
    mcShowLoadResult(code);
    gFrontendStep = NULL;
}

/* Confirm-overwrite prompt: Yes/No buttons with the choice highlighted. */
BMX_ADDR(0x0043de10)
void mcStepPrompt1(void)
{
    mcSetRow(5, zstringGet(0x26d));
    guiSetTextStyle(guiFindById(gMcRows[6]), 0);
    guiSetTextStyle(guiFindById(gMcRows[7]), 0);
    guiSetTint(guiFindById(gMcRows[6]), 0xc0, 0xc0, 0xc0, 0xc0);
    guiSetTint(guiFindById(gMcRows[7]), 0xc0, 0xc0, 0xc0, 0xc0);
    const uint32_t chosen = gMcChoice == 0 ? gMcRows[6] : gMcRows[7];
    guiSetTextStyle(guiFindById(chosen), 1);
    guiSetTint(guiFindById(chosen), 0xc0, 0xc0, 0x80, (uint32_t)gGuiPulse + 0x20);
    mcSetRow(6, zstringGet(0x25b));
    mcSetRow(7, zstringGet(0x25a));
}

/* Save flow, formatting step: checks the card again and reports what it found. */
BMX_ADDR(0x0043e010)
void mcStepSaveFormat(void)
{
    mcClearRows(0);
    mcSetRow(0, zstringGet(0x26f));
    mcSetRow(1, zstringGet(0x25c));
    mcSetSlotRow(2);
    memcardBegin();
    menuPlaySound(0x7d8);
    memcardSelectSlot(gMcSlot);
    int code = saveFileClose();
    switch (code) {
    case 0:
    case 6:
        gMcState = 4;
        gMcCountdown = 3;
        gMcChoice = 0;
        mcSetRow(0, zstringGet(0x253));
        goto done;
    case 4:
        memcardSelectSlot(gMcSlot);
        code = memcardQuery();
        if (code == 0) {
            gMcCountdown = 3;
            gMcState = kMcSaveWrite;
            mcSetRow(0, zstringGet(0x265));
            mcSetRow(1, zstringGet(0x25c));
            mcSetSlotRow(2);
            goto done;
        }
        mcShowSaveResult(code);
        mcSetRow(0, zstringGet(0x271));
        break;
    case 5:
        memcardSelectSlot(gMcSlot);
        code = memcardFormat();
        if (code == 0) {
            mcClearRows(0);
            gMcChoice = 0;
            gMcState = 3;
            gMcCountdown = 3;
            goto done;
        }
        /* fall through */
    default:
        mcShowSaveResult(code);
    }
    gMcState = 10;
    gMcCountdown = 3;
done:
    memcardEnd();
    menuPlaySound(0x7d9);
    gFrontendStep = NULL;
}

/* One-button message step ("OK"). */
BMX_ADDR(0x0043e3b0)
void mcStepMessage(void)
{
    mcSetRow(kMcRowButton0, zstringGet(0x258));
    guiSetTextStyle(guiFindById(gMcRows[kMcRowButton0]), 1);
    guiSetTint(guiFindById(gMcRows[kMcRowButton0]), 0xc0, 0xc0, 0x80, (uint32_t)gGuiPulse + 0x20);
}

/* Yes/No prompt for state 3 (title string 0x254). */
BMX_ADDR(0x0043e440)
void mcStepPrompt3(void)
{
    mcClearRows(1);
    mcSetRow(0, zstringGet(0x254));
    mcSetRow(6, zstringGet(0x25b));
    mcSetRow(7, zstringGet(0x25a));
    guiSetTextStyle(guiFindById(gMcRows[6]), 0);
    guiSetTextStyle(guiFindById(gMcRows[7]), 0);
    guiSetTint(guiFindById(gMcRows[6]), 0xc0, 0xc0, 0xc0, 0xc0);
    guiSetTint(guiFindById(gMcRows[7]), 0xc0, 0xc0, 0xc0, 0xc0);
    const uint32_t chosen = gMcChoice == 0 ? gMcRows[6] : gMcRows[7];
    guiSetTextStyle(guiFindById(chosen), 1);
    guiSetTint(guiFindById(chosen), 0xc0, 0xc0, 0x80, (uint32_t)gGuiPulse + 0x20);
}

/* Yes/No prompt for state 4 (the question goes into row 5, string 0x268). */
BMX_ADDR(0x0043e670)
void mcStepPrompt4(void)
{
    mcClearRows(1);
    mcSetRow(5, zstringGet(0x268));
    mcSetRow(6, zstringGet(0x25b));
    mcSetRow(7, zstringGet(0x25a));
    guiSetTextStyle(guiFindById(gMcRows[6]), 0);
    guiSetTextStyle(guiFindById(gMcRows[7]), 0);
    guiSetTint(guiFindById(gMcRows[6]), 0xc0, 0xc0, 0xc0, 0xc0);
    guiSetTint(guiFindById(gMcRows[7]), 0xc0, 0xc0, 0xc0, 0xc0);
    const uint32_t chosen = gMcChoice == 0 ? gMcRows[6] : gMcRows[7];
    guiSetTextStyle(guiFindById(chosen), 1);
    guiSetTint(guiFindById(chosen), 0xc0, 0xc0, 0x80, (uint32_t)gGuiPulse + 0x20);
}

/* Save flow, first step: checks the card and picks the prompt (format, overwrite) or the error. */
BMX_ADDR(0x0043e8a0)
void mcStepSaveProbe(void)
{
    memcardBegin();
    memcardSelectSlot(gMcSlot);
    const int code = saveFileClose();
    mcShowSaveResult(code);
    switch (code) {
    case 0:
    case 5: {
        memcardSelectSlot(gMcSlot);
        const int format = memcardFormat();
        if (format == 0) {
            mcClearRows(0);
            gMcState = 3;
            gMcChoice = 0;
            break;
        }
        mcShowSaveResult(format);
        gMcState = 8;
        break;
    }
    default:
        gMcState = 8;
        break;
    case 4:
        gMcState = 1;
        gMcChoice = 0;
        break;
    case 6:
        mcSetRow(0, zstringGet(0x253));
        gMcState = 4;
        gMcChoice = 0;
    }
    memcardEnd();
    gFrontendStep = NULL;
    gMcCountdown = 3;
}

/* Save flow, writing step: writes the file and shows the outcome. */
BMX_ADDR(0x0043ea10)
void mcStepSaveWrite(void)
{
    menuPlaySound(0x7d8);
    memcardBegin();
    memcardBegin();
    memcardSelectSlot(gMcSlot);
    int code = saveFileClose();
    switch (code) {
    default:
    fail:
        mcShowSaveResult(code);
        gMcState = 8;
        break;
    case 4:
        mcShowSaveResult(4);
        gMcState = 1;
        gMcChoice = 0;
        break;
    case 5:
        memcardSelectSlot(gMcSlot);
        code = memcardFormat();
        if (code != 0) {
            goto fail;
        }
        /* fall through */
    case 0:
    case 6:
        memcardSelectSlot(gMcSlot);
        code = saveFileWrite();
        mcShowSaveResult(code);
        gMcState = (uint16_t)((code != 0) + 7);
    }
    gMcCountdown = 3;
    memcardEnd();
    menuPlaySound(0x7d9);
    gFrontendStep = NULL;
}

/* Fills the lines for a load status code (0 ok, 1/2/4/5/-1 problems). */
BMX_ADDR(0x0043eb70)
void mcShowLoadResult(int code)
{
    mcClearRows(0);
    mcSetRow(0, zstringGet(code == 0 ? 0x25e : 0x261));
    switch (code) {
    case 0:
        gMcLoadOk = 1;
        return;
    case 1:
        mcSetSlotRow(1);
        mcSetRow(2, zstringGet(0x263));
        break;
    case 2:
        mcSetSlotRow(1);
        mcSetRow(2, zstringGet(0x264));
        break;
    case 4:
        mcSetSlotRow(1);
        mcSetRow(2, zstringGet(0x26c));
        break;
    case 5:
        mcSetRow(1, zstringGet(0x262));
        break;
    case -1:
        mcSetRow(1, zstringGet(0x272));
        break;
    default:
        break;
    }
}

/* Fills the lines for a save status code (0 ok, 1/2/4/7 problems). */
BMX_ADDR(0x0043edf0)
void mcShowSaveResult(int code)
{
    mcClearRows(0);
    mcSetRow(0, zstringGet(code == 0 ? 0x266 : 0x267));
    switch (code) {
    case 0:
        gMcSaveOk = 1;
        return;
    case 1:
        mcSetSlotRow(1);
        mcSetRow(2, zstringGet(0x263));
        break;
    case 2:
        mcSetSlotRow(1);
        mcSetRow(2, zstringGet(0x264));
        break;
    case 4:
        mcSetSlotRow(1);
        mcSetRow(2, zstringGet(0x26c));
        break;
    case 7:
        mcSetSlotRow(1);
        mcSetRow(2, zstringGet(0x26a));
        mcSetRow(3, zstringGet(0x26b));
        break;
    default:
        break;
    }
}
