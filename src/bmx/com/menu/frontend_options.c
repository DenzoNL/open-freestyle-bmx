/*
 * Options screen (screen 0xa00c): a main list (see optionsMain*) that opens panels for the game options,
 * the controller display, the keyboard layout and the sound volumes. Panels slide in with the shared row
 * animation and hand the tick/draw work to their own hook.
 */
#include "bmx/com/action/memcard.h"
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/menu/gui/guilabel.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"
#include "compat/pktlink.h"
#include "zge/zstring.h"

enum {
    kOptPanel = 0xa00c0015, /* element the game options panel draws through */
    kOptMainRow0 = 0xa00c0005,
    kOptMainRow1 = 0xa00c0007,
    kOptMainRow2 = 0xa00c0009,
    kOptMainRow3 = 0xa00c000b,
};

/* Hides the eight option panel elements of the main list screen (names and values of the four rows). */
static void optionsHidePanelRows(void)
{
    menuHide(0xa00c000e);
    menuHide(0xa00c000d);
    menuHide(0xa00c0010);
    menuHide(0xa00c000f);
    menuHide(0xa00c0012);
    menuHide(0xa00c0011);
    menuHide(0xa00c0014);
    menuHide(0xa00c0013);
}

/* Builds the list of available options and opens the game options panel. */
BMX_ADDR(0x00440dc0)
void optionsGameOpen(void)
{
    const uint32_t avail = gOptionsAvail;
    for (int i = 0; i < 8; i++) {
        gOptSlide[i] = 0;
    }
    uint16_t count = 0;
    for (const OptionEntry *entry = gOptionTable; entry < gOptionTable + 8; entry++) {
        if ((avail & (1u << (entry->bit & 0x1f))) != 0) {
            gOptRows[count++] = entry;
        }
    }
    gOptCount = count;
    gOptSel = 0;
    menuShow(kOptPanel);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x004013b1, optionsGameHook));
    guiFindById(kOptPanel)->proc = BMX_FNPTR(GuiProcFn, 0x004013b1, optionsGameHook);
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401659, optionsGameSlideIn);
    menuHide(0xa00c0010);
    menuHide(0xa00c000f);
}

/* Back to the options main list: the four rows slide in again. */
BMX_ADDR(0x00440eb0)
void optionsGameClose(void)
{
    optionsHidePanelRows();
    menuSlideReset();
    menuSlideIn(kOptMainRow0, kOptMainRow1, kOptMainRow2, kOptMainRow3);
    menuHide(kOptPanel);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x00401e47, optionsMainHook));
    guiFindById(kOptPanel)->proc = BMX_FNPTR(GuiProcFn, 0x00401e4c, guiLabelProc);
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401dcf, optionsMainStep);
}

/* Slides the option rows in from the right (each row waits for the one before it to pass 0x104). */
BMX_ADDR(0x00440fd0)
void optionsGameSlideIn(void)
{
    gOptSlide[0] += (500 - gOptSlide[0]) / 5;
    if (gOptSlide[0] > 0x1cc) {
        gOptSlide[0] = 0x1cc;
    }
    const int count = gOptCount;
    for (int i = 1; i < count; i++) {
        if (gOptSlide[i - 1] > 0x104) {
            gOptSlide[i] += (500 - gOptSlide[i]) / 5;
        }
        if (gOptSlide[i] > 0x1cc) {
            gOptSlide[i] = 0x1cc;
        }
    }
    if (gOptSlide[count - 1] > 0x1cb) {
        gFrontendStep = NULL;
    }
}

/* Slides the rows back out; once the last one is gone the panel closes. */
BMX_ADDR(0x004410b0)
void optionsGameSlideOut(void)
{
    gOptSlide[0] += (500 - gOptSlide[0]) / -5;
    if (gOptSlide[0] < 0) {
        gOptSlide[0] = 0;
    }
    const int count = gOptCount;
    for (int i = 1; i < count; i++) {
        if (gOptSlide[i - 1] < 400) {
            gOptSlide[i] -= (500 - gOptSlide[i]) / 5;
        }
        if (gOptSlide[i] < 0) {
            gOptSlide[i] = 0;
        }
    }
    if (gOptSlide[count - 1] > 0) {
        return;
    }
    optionsGameClose();
}

/*
 * Hook and draw handler of the game options panel. It draws every row through the panel element: first
 * the option name, then its On/Off text, each positioned by the row's slide offset.
 */
BMX_ADDR(0x00441180)
int BMX_CDECL optionsGameHook(int msg, GuiElement *el, int arg3, int arg4)
{
    switch (msg) {
    case kGuiMsgDraw:
        for (int i = 0; i < gOptCount; i++) {
            const OptionEntry *entry = gOptRows[i];
            guiSetText(el, zstringGet(entry->stringId));
            guiSetTextPos(el, (int16_t)(0x200 - gOptSlide[i]), (int16_t)(i * 0x23));
            guiSetTextStyle(el, gOptSel == i);
            guiLabelProc(msg, el, arg3, arg4);
            const int on = (gOptionsValue >> (entry->bit & 0x1f)) & 1;
            guiSetText(el, zstringGet(gOptStateStrings[on].stringId));
            guiSetTextPos(el, (int16_t)(0x336 - gOptSlide[i]), (int16_t)(i * 0x23));
            guiSetTextStyle(el, (gOptSel != i) + 1);
            guiLabelProc(msg, el, arg3, arg4);
        }
        return 0;
    case kGuiMsgTickAll:
        gOptTicks++;
        if (gFrontendStep != NULL) {
            gFrontendStep();
        }
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case kGuiMsgNavUp:
    case kGuiMsgNavUpAlt:
        if (gFrontendStep != NULL) {
            return 1;
        }
        gOptSel--;
        if (gOptSel < 0) {
            gOptSel = (int)gOptCount - 1;
        }
        break;
    case kGuiMsgNavDown:
    case kGuiMsgNavDownAlt:
        if (gFrontendStep != NULL) {
            return 1;
        }
        gOptSel++;
        if (gOptSel >= (int)gOptCount) {
            gOptSel = 0;
        }
        break;
    case kGuiMsgNavLeft:
    case kGuiMsgNavRight:
    case kGuiMsgNavLeftAlt:
    case kGuiMsgNavRightAlt:
        if (gFrontendStep != NULL) {
            return 1;
        }
        gOptionsValue ^= (uint16_t)(1u << (gOptRows[gOptSel]->bit & 0x1f));
        break;
    case kGuiMsgButton22:
    case kGuiMsgButton24:
    case 0x2f:
    case 0x31:
        return 1;
    case kGuiMsgButton25:
    case kGuiMsgIdle:
        if (gFrontendStep != NULL) {
            return 1;
        }
        if (gGuiIdleMode != 0) {
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402540, optionsGameClose);
            return 1;
        }
        menuPlaySound(0x7d2);
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402153, optionsGameSlideOut);
        return 1;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
    menuPlaySound(2000);
    return 1;
}

/* Appends one strip polygon to the display list. */
static void stripLink(StripPoly *poly)
{
    uint32_t *table = ngifGetOrderTable();
    poly->tag = (poly->tag & 0xf0000000u) | (table[0] & 0x0fffffffu);
    table = ngifGetOrderTable();
    table[0] = (table[0] & 0xf0000000u) | bmxPacketLink(poly);
}

static void stripBegin(StripPoly *poly)
{
    poly->tag = (poly->tag & 0x0fffffffu) | 0x80000000u;
    poly->code = 0x38;
    poly->flags = (poly->flags & 0xffffff80u) | 0x7f80u;
}

/*
 * Draws the eight vertical colour strips of the controller panel's backdrop (each a bright-to-dark pair of
 * polygons: top half scaled to six times the colour at the top edge, bottom half fading to black) and two
 * lines under them. Strip i spans x = scale0*scale1*i .. *(i+1), y 0..300 and 300..511.
 */
BMX_ADDR(0x004414d0)
void controlsDrawStrips(void)
{
    StripPoly *poly = (StripPoly *)gPrimCursor;
    for (int i = 0; i < 8; i++) {
        const uint8_t r = gStripColors[i][0];
        const uint8_t g = gStripColors[i][1];
        const uint8_t b = gStripColors[i][2];
        const float fi = (float)i;
        const float fn = (float)(i + 1);
        const double scale = (double)gStripScale0 * (double)gStripScale1;
        const int16_t xLeft = (int16_t)bmxFtol(scale * (double)fi);
        const int16_t xRight = (int16_t)bmxFtol(scale * (double)fn);

        stripBegin(poly);
        poly->r0 = (uint8_t)(r * 6);
        poly->g0 = (uint8_t)(g * 6);
        poly->b0 = (uint8_t)(b * 6);
        poly->r1 = (uint8_t)(r * 6);
        poly->g1 = (uint8_t)(g * 6);
        poly->b1 = (uint8_t)(b * 6);
        poly->r2 = r;
        poly->g2 = g;
        poly->b2 = b;
        poly->r3 = r;
        poly->g3 = g;
        poly->b3 = b;
        poly->x0 = xLeft;
        poly->y0 = 0;
        poly->x1 = xRight;
        poly->y1 = 0;
        poly->x2 = xLeft;
        poly->y2 = 0x12c;
        poly->x3 = xRight;
        poly->y3 = 0x12c;
        stripLink(poly);
        poly++;

        stripBegin(poly);
        poly->r0 = r;
        poly->g0 = g;
        poly->b0 = b;
        poly->r1 = r;
        poly->g1 = g;
        poly->b1 = b;
        poly->r2 = poly->g2 = poly->b2 = 0;
        poly->r3 = poly->g3 = poly->b3 = 0;
        poly->x0 = xLeft;
        poly->y0 = 0x12c;
        poly->x1 = xRight;
        poly->y1 = 0x12c;
        poly->x2 = xLeft;
        poly->y2 = 0x1ff;
        poly->x3 = xRight;
        poly->y3 = 0x1ff;
        stripLink(poly);
        poly++;
    }
    gPrimCursor = (uint8_t *)poly;
    ngifSetOrderTable(ngifGetOrderTable() - 1);
    zprimSet(kZprimColor, 0xeeeeee);
    const int16_t lineEnd = (int16_t)bmxFtol((double)gStripScale0);
    zprimDrawLine(0, 0xf0, lineEnd, 0xf0);
    zprimDrawLine(0, 0xf1, lineEnd, 0xf1);
}

/* Opens the controller panel: four device names, everything else of the list screen hidden. */
BMX_ADDR(0x00441860)
void controlsOpen(void)
{
    menuShow(0xa00c0001);
    menuShow(0xa00c0002);
    menuShow(0xa00c0003);
    menuShow(0xa00c0004);
    menuSetText(0xa00c0001, zstringGet(0x135));
    menuSetText(0xa00c0002, zstringGet(0x136));
    menuSetText(0xa00c0003, zstringGet(0x137));
    menuSetText(0xa00c0004, zstringGet(0x138));
    menuHide(0xa00c000e);
    menuHide(0xa00c000d);
    menuHide(0xa00c0010);
    menuHide(0xa00c000f);
    menuHide(0xa00c0014);
    menuHide(0xa00c0013);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x00401078, controlsHook));
    gFrontendStep = NULL;
}

/* Leaves the controller panel: names hidden, main list rows slide back in. */
BMX_ADDR(0x004419f0)
void controlsClose(void)
{
    menuHide(0xa00c0001);
    menuHide(0xa00c0002);
    menuHide(0xa00c0003);
    menuHide(0xa00c0004);
    for (uint32_t id = 0xa00c0005; id <= 0xa00c000c; id++) {
        menuShow(id);
    }
    optionsHidePanelRows();
    menuSlideReset();
    menuSlideIn(kOptMainRow0, kOptMainRow1, kOptMainRow2, kOptMainRow3);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x00401e47, optionsMainHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401dcf, optionsMainStep);
}

/* Hook of the controller panel: draws the gradient strips; back leaves, everything else is ignored. */
BMX_ADDR(0x00441bd0)
int BMX_CDECL controlsHook(int msg, GuiElement *el, int arg3, int arg4)
{
    switch (msg) {
    case kGuiMsgTickAll:
        if (gFrontendStep != NULL) {
            gFrontendStep();
        }
        controlsDrawStrips();
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case kGuiMsgNavUp:
    case kGuiMsgNavDown:
    case kGuiMsgNavLeft:
    case kGuiMsgNavRight:
    case kGuiMsgButton22:
    case kGuiMsgButton24:
    case kGuiMsgNavUpAlt:
    case kGuiMsgNavDownAlt:
    case kGuiMsgNavLeftAlt:
    case kGuiMsgNavRightAlt:
    case 0x2f:
    case 0x31:
        return 1;
    case kGuiMsgButton25:
    case kGuiMsgIdle:
        break;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
    if (gFrontendStep != NULL) {
        return 1;
    }
    if (gGuiIdleMode == 0) {
        menuPlaySound(0x7d2);
    }
    controlsClose();
    return 1;
}

/* ---- keyboard layout panel ---------------------------------------------------------------------- */

enum {
    kKbRowLabel = 0xa00c0021,  /* + row: action name, rows 0..10 */
    kKbRowAction = 0xa00c002c, /* + row: layout specific label, rows 0..10 */
    kKbRowKey = 0xa00c0037,    /* + row: the bound button, rows 0..13 */
    kKbTitle = 0xa00c0042,
    kKbRows = 11,
    kKbRowLayout = 11,
    kKbRowOk = 12,
    kKbRowCancel = 13,
};

/* Highlights (or not) a row of the panel: its two labels for action rows, the key text otherwise. */
static void keyboardStyleRow(int row, int on)
{
    if (row < kKbRows) {
        menuSetStyle(kKbRowLabel + (uint32_t)row, (uint32_t)on);
        menuSetStyle(kKbRowAction + (uint32_t)row, (uint32_t)on);
    }
    menuSetStyle(kKbRowKey + (uint32_t)row, on ? 2u : 0u);
}

/* Opens the panel for gKeyPlayer: copies the bindings to the working array and fills the rows. */
BMX_ADDR(0x00441cb0)
void keyboardOpen(void)
{
    for (uint32_t i = 0; i < 3; i++) {
        menuShow(0xa00c001e + i);
    }
    const PlayerSetup *profile = &gPlayers[gKeyPlayer];
    gKbState = 0;
    gKbCursor = kKbRowCancel;
    for (uint32_t row = 0; row < 14; row++) {
        const int highlighted = row == gKbCursor;
        if (row < kKbRows) {
            menuShow(kKbRowLabel + row);
            menuShow(kKbRowAction + row);
            menuShow(kKbRowKey + row);
            menuSetStyle(kKbRowLabel + row, (uint32_t)highlighted);
            menuSetStyle(kKbRowAction + row, (uint32_t)highlighted);
            menuSetStyle(kKbRowKey + row, highlighted ? 2u : 0u);
            const uint32_t bits = profile->bind[row];
            gKbWork[row] = bits;
            for (uint32_t b = 0; b < 0x12; b++) {
                if ((bits & (1u << b)) != 0) {
                    menuSetText(kKbRowKey + row, gKeyNames[b]);
                    break;
                }
            }
            menuSetText(kKbRowAction + row, gKbLayoutLabels[gKbLayout][row]);
        } else {
            menuShow(kKbRowKey + row);
            menuSetStyle(kKbRowKey + row, highlighted ? 2u : 0u);
        }
    }
    for (uint32_t i = 0; i < 2; i++) {
        menuShow(0xa00c0045 + i);
    }
    gKbLayout = profile->layout;
    menuSetText(kKbTitle, gKbLayoutTitles[gKbLayout]);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x004420a0, keyboardHook));
    gFrontendStep = NULL;
}

/* Leaves the panel: hides its rows and brings the options main list back. */
BMX_ADDR(0x00441f80)
void keyboardClose(void)
{
    for (uint32_t i = 0; i < 3; i++) {
        menuHide(0xa00c001e + i);
    }
    for (uint32_t row = 0; row < 14; row++) {
        if (row < kKbRows) {
            menuHide(kKbRowLabel + row);
            menuHide(kKbRowAction + row);
        }
        menuHide(kKbRowKey + row);
    }
    for (uint32_t i = 0; i < 2; i++) {
        menuHide(0xa00c0045 + i);
    }
    menuSlideReset();
    menuSlideIn(kOptMainRow0, kOptMainRow1, kOptMainRow2, kOptMainRow3);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x00401e47, optionsMainHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401dcf, optionsMainStep);
}

/* Shows the layout title and the layout's action labels for the layout in gKbLayout. */
static void keyboardShowLayout(void)
{
    menuSetText(kKbTitle, gKbLayoutTitles[gKbLayout]);
    for (uint32_t i = 0; i < kKbRows; i++) {
        menuSetText(kKbRowAction + i, gKbLayoutLabels[gKbLayout][i]);
    }
}

/*
 * Hook of the keyboard panel. Browsing (state 0): up/down move the cursor, left/right change the layout
 * on its row, confirm starts a rebinding (state 1 waits until the confirm button is released, state 2
 * draws a box around the row and binds the first button pressed) or, on the OK/cancel rows, leaves.
 */
BMX_ADDR(0x004420a0)
int BMX_CDECL keyboardHook(int msg, GuiElement *el, int arg3, int arg4)
{
    PlayerSetup *profile = &gPlayers[gKeyPlayer];
    const GuiPadDevice *device = zpadGetDevice(gKeyPlayer);
    switch (gKbState) {
    case 0:
        break;
    case 1:
        if (msg == kGuiMsgTickAll && device->held == 0) {
            gKbState = 2;
        }
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case 2:
        if (msg == kGuiMsgTickAll || msg == kGuiMsgButton25 || msg == kGuiMsgIdle) {
            for (int side = 0; side < 4; side++) {
                const int top = gKbCursor * 25;
                int y0 = top + 0x26;
                int y1 = top + 0x26;
                if (side != 0) {
                    y1 = top + 0x3e;
                    if (side == 2) {
                        y0 = y1;
                    }
                }
                zprimDrawLine((int16_t)(side != 3 ? 0x152 : 0x19c), (int16_t)y0,
                              (int16_t)(side != 1 ? 0x19c : 0x152), (int16_t)y1);
            }
            if (device->held != 0) {
                const uint32_t row = gKbCursor;
                gKbWork[row] = 0;
                for (uint32_t b = 0; b < 0x12; b++) {
                    if ((device->held & (1u << b)) != 0) {
                        gKbWork[row] = 1u << b;
                        menuSetText(kKbRowKey + row, gKeyNames[b]);
                        menuShow(kKbRowKey + gKbCursor);
                        break;
                    }
                }
                gKbState = 0;
                menuPlaySound(0x7d2);
                return 1;
            }
        }
        return guiDefaultHookProc(msg, el, arg3, arg4);
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
    switch (msg) {
    case kGuiMsgNavUp:
    case kGuiMsgNavUpAlt:
    case kGuiMsgNavDown:
    case kGuiMsgNavDownAlt:
        keyboardStyleRow(gKbCursor, 0);
        if (msg == kGuiMsgNavUp || msg == kGuiMsgNavUpAlt) {
            gKbCursor = gKbCursor > 0 ? (uint8_t)(gKbCursor - 1) : (uint8_t)kKbRowCancel;
        } else {
            gKbCursor = gKbCursor < kKbRowCancel ? (uint8_t)(gKbCursor + 1) : 0;
        }
        keyboardStyleRow(gKbCursor, 1);
        menuPlaySound(2000);
        return 1;
    case kGuiMsgNavLeft:
    case kGuiMsgNavLeftAlt:
        if (gKbCursor == kKbRowLayout) {
            gKbLayout = gKbLayout > 0 ? (uint8_t)((gKbLayout - 1) % 5) : 4;
            keyboardShowLayout();
            return 1;
        }
        menuPlaySound(0x7d1);
        return 1;
    case kGuiMsgNavRight:
    case kGuiMsgNavRightAlt:
        if (gKbCursor == kKbRowLayout) {
            gKbLayout = (uint8_t)((gKbLayout + 1) % 5);
            keyboardShowLayout();
            return 1;
        }
        menuPlaySound(0x7d1);
        return 1;
    case kGuiMsgButton22:
    case kGuiMsgButton24:
    case 0x2f:
    case 0x31:
        if (gKbCursor < kKbRows) {
            gKbState = 1;
            menuHide(kKbRowKey + gKbCursor);
            menuPlaySound(0x7d1);
            return 1;
        }
        menuPlaySound(0x7d2);
        if (gKbCursor == kKbRowLayout) {
            return 1;
        }
        if (gKbCursor == kKbRowOk) {
            for (int i = 0; i < kKbRows; i++) {
                profile->bind[i] = gKbWork[i];
            }
            profile->layout = gKbLayout;
            menuPlaySound(0x7d2);
        } else if (gKbCursor == kKbRowCancel) {
            menuPlaySound(0x7d3);
        }
        keyboardClose();
        return 1;
    case kGuiMsgButton25:
    case kGuiMsgIdle:
        menuPlaySound(0x7d3);
        keyboardClose();
        return 1;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
}

/* ---- sound volume panel -------------------------------------------------------------------------- */

#define kSndBarMusic   0xa00c0048u
#define kSndBarSfx     0xa00c004bu
#define kSndLabelMusic 0xa00c0049u
#define kSndLabelSfx   0xa00c004cu

/* Opens the panel: two bars (music, effects) with their labels, taking the levels from the settings. */
BMX_ADDR(0x00442730)
void soundPanelOpen(void)
{
    gSoundLevel1 = gSoundVolume1;
    gSoundLevel0 = gSoundVolume0;
    gSoundSlideA = 0;
    gSoundSlideB = 0;
    gSoundSel = 0;
    menuSetText(kSndLabelMusic, zstringGet(0x147));
    menuSetText(kSndLabelSfx, zstringGet(0x148));
    menuShow(0xa00c0047);
    menuShow(kSndBarMusic);
    menuShow(0xa00c004a);
    menuShow(kSndBarSfx);
    menuShow(kSndLabelMusic);
    menuShow(kSndLabelSfx);
    menuSetStyle(kSndLabelMusic, 1);
    menuSetStyle(kSndLabelSfx, 1);
    guiSetSpriteOffset(guiFindById(0xa00c0047), 0x200, 0);
    guiSetSpriteOffset(guiFindById(kSndBarMusic), 0x200, 0);
    guiSetSpriteOffset(guiFindById(0xa00c004a), 0x200, 0);
    guiSetSpriteOffset(guiFindById(kSndBarSfx), 0x200, 0);
    guiSetTextPos(guiFindById(kSndLabelMusic), 0x200, 0);
    guiSetTextPos(guiFindById(kSndLabelSfx), 0x200, 0);
    guiFindById(kSndBarMusic)->proc = BMX_FNPTR(GuiProcFn, 0x00401c3a, soundPanelHook);
    guiFindById(kSndBarSfx)->proc = BMX_FNPTR(GuiProcFn, 0x00401c3a, soundPanelHook);
    menuHide(0xa00c0010);
    menuHide(0xa00c000f);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x00401c3a, soundPanelHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402063, soundPanelSlideIn);
}

/* Leaves the panel and brings the options main list back. */
BMX_ADDR(0x00442a00)
void soundPanelClose(void)
{
    optionsHidePanelRows();
    menuSlideReset();
    menuSlideIn(kOptMainRow0, kOptMainRow1, kOptMainRow2, kOptMainRow3);
    menuHide(0xa00c0047);
    menuHide(kSndBarMusic);
    menuHide(0xa00c004a);
    menuHide(kSndBarSfx);
    menuHide(kSndLabelMusic);
    menuHide(kSndLabelSfx);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x00401e47, optionsMainHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401dcf, optionsMainStep);
}

BMX_ADDR(0x00442b70)
void soundPanelSlideIn(void)
{
    gSoundSlideA += (500 - gSoundSlideA) / 5;
    if (gSoundSlideA > 0x1cc) {
        gSoundSlideA = 0x1cc;
    }
    if (gSoundSlideA > 0x104) {
        gSoundSlideB += (500 - gSoundSlideB) / 5;
    }
    if (gSoundSlideB > 0x1cc) {
        gSoundSlideB = 0x1cc;
    }
    if (gSoundSlideB > 0x1cb) {
        gFrontendStep = NULL;
    }
}

BMX_ADDR(0x00442c40)
void soundPanelSlideOut(void)
{
    gSoundSlideA += (500 - gSoundSlideA) / -5;
    if (gSoundSlideA < 0) {
        gSoundSlideA = 0;
    }
    if (gSoundSlideA < 400) {
        gSoundSlideB -= (500 - gSoundSlideB) / 5;
    }
    if (gSoundSlideB < 0) {
        gSoundSlideB = 0;
    }
    if (gSoundSlideB <= 0) {
        soundPanelClose();
    }
}

/* The master level changed: the sound layer gets both channels at the master level, then the effects
 * channel back at its own. */
static void soundPanelApplyMaster(void)
{
    menuSetChannelVolume(1, gSoundLevel1 * 7);
    menuSetChannelVolume(0, gSoundLevel1 * 7);
    menuPlaySound(2000);
    menuSetChannelVolume(0, gSoundLevel0 * 7);
}

/* Stores the levels as the settings' two volume bytes. */
static void soundPanelStoreLevels(void)
{
    gSoundVolume1 = (uint8_t)gSoundLevel1;
    gSoundVolume0 = (uint8_t)gSoundLevel0;
}

BMX_ADDR(0x00442ce0)
int BMX_CDECL soundPanelHook(int msg, GuiElement *el, int arg3, int arg4)
{
    switch (msg) {
    case kGuiMsgDraw:
        if (el->id == kSndBarMusic) {
            const int x = 0x2bf - gSoundSlideA;
            zprimSetClip(x, 0, x + gSoundLevel1 * 6, 0x1df);
        } else if (el->id == kSndBarSfx) {
            const int x = 0x2bf - gSoundSlideB;
            zprimSetClip(x, 0, x + gSoundLevel0 * 6, 0x1df);
        } else {
            return 1;
        }
        guiLabelProc(msg, el, arg3, arg4);
        zprimSetClip(0, 0, 0x27f, 0x1df);
        return 1;
    case kGuiMsgTickAll: {
        if (gFrontendStep != NULL) {
            gFrontendStep();
        }
        guiSetFrame(guiFindById(kSndBarMusic), (uint16_t)((gSoundSel != 0) + 1));
        guiSetFrame(guiFindById(kSndBarSfx), (uint16_t)((gSoundSel != 1) + 1));
        menuSetStyle(kSndLabelMusic, gSoundSel == 0);
        menuSetStyle(kSndLabelSfx, gSoundSel == 1);
        const int musicX = 0xa0 - (guiGetTextWidth(guiFindById(kSndLabelMusic)) & 0xffff);
        const int sfxX = 0xa0 - (guiGetTextWidth(guiFindById(kSndLabelSfx)) & 0xffff);
        guiSetSpriteOffset(guiFindById(0xa00c0047), (int16_t)(0x200 - gSoundSlideA), 0);
        guiSetSpriteOffset(guiFindById(kSndBarMusic), (int16_t)(0x200 - gSoundSlideA), 0);
        guiSetTextPos(guiFindById(kSndLabelMusic), (int16_t)(musicX - gSoundSlideA + 0x200), 0);
        guiSetSpriteOffset(guiFindById(0xa00c004a), (int16_t)(0x200 - gSoundSlideB), 0);
        guiSetSpriteOffset(guiFindById(kSndBarSfx), (int16_t)(0x200 - gSoundSlideB), 0);
        guiSetTextPos(guiFindById(kSndLabelSfx), (int16_t)(sfxX - gSoundSlideB + 0x200), 0);
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
    case kGuiMsgNavUp:
    case kGuiMsgNavUpAlt:
        gSoundSel--;
        if (gSoundSel < 0) {
            gSoundSel = 1;
        }
        menuPlaySound(2000);
        return 1;
    case kGuiMsgNavDown:
    case kGuiMsgNavDownAlt:
        gSoundSel++;
        if (gSoundSel > 1) {
            gSoundSel = 0;
        }
        menuPlaySound(2000);
        return 1;
    case kGuiMsgNavLeft:
    case kGuiMsgNavLeftAlt:
        if (gSoundSel != 0) {
            if (gSoundSel == 1 && gSoundLevel0 > 0) {
                gSoundLevel0--;
                menuSetChannelVolume(0, gSoundLevel0 * 7);
                menuPlaySound(2000);
            }
        } else if (gSoundLevel1 > 0) {
            gSoundLevel1--;
            soundPanelApplyMaster();
        }
        soundPanelStoreLevels();
        return 1;
    case kGuiMsgNavRight:
    case kGuiMsgNavRightAlt:
        if (gSoundSel != 0) {
            if (gSoundSel == 1 && gSoundLevel0 < 0x12) {
                gSoundLevel0++;
                menuSetChannelVolume(0, gSoundLevel0 * 7);
                menuPlaySound(2000);
            }
        } else if (gSoundLevel1 < 0x12) {
            gSoundLevel1++;
            soundPanelApplyMaster();
        }
        soundPanelStoreLevels();
        return 1;
    case kGuiMsgButton22:
    case kGuiMsgButton24:
    case 0x2f:
    case 0x31:
        return 1;
    case kGuiMsgButton25:
    case kGuiMsgIdle:
        if (gFrontendStep == NULL) {
            if (gGuiIdleMode != 0) {
                gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x0040193d, soundPanelClose);
                return 1;
            }
            menuPlaySound(0x7d2);
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004021f3, soundPanelSlideOut);
        }
        return 1;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
}

/* ---- options main list --------------------------------------------------------------------------- */

/* Opens the options screen; the list is laid out by the step on the next tick. */
BMX_ADDR(0x004433e0)
int optionsInit(void)
{
    guiDestroyAll();
    guiLoadBackdrop(0xa015, 7);
    guiRequestScreen(0xa00c, BMX_FNPTR(GuiHookFn, 0x00401e47, optionsMainHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401181, optionsMainLayout);
    return 0;
}

BMX_ADDR(0x00443420)
int BMX_CDECL optionsMainHook(int msg, GuiElement *el, int arg3, int arg4)
{
    switch (msg) {
    case kGuiMsgTickAll:
        if (gFrontendStep != NULL) {
            gFrontendStep();
        }
        if (gOptMainReady != 0) {
            /* the frame around the selected row pulses between colours 0x40 and 0x80 */
            if (gOptMainPulse < 0 || gOptMainPulse > 0x40) {
                gOptMainPulseStep = -gOptMainPulseStep;
            }
            gOptMainPulse += gOptMainPulseStep;
            menuDrawFrame(0x1f, (int16_t)(gOptMainSel * 0x54 + 0x46), 0x71,
                          (int16_t)(gOptMainSel * 0x54 + 0xa3), (uint8_t)(0x80 - gOptMainPulse), 1);
        }
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case kGuiMsgNavUp:
    case kGuiMsgNavUpAlt:
        if (gFrontendStep != NULL) {
            return 0;
        }
        gOptMainSel--;
        if (gOptMainSel < 0) {
            gOptMainSel = 2;
        }
        break;
    case kGuiMsgNavDown:
    case kGuiMsgNavDownAlt:
        if (gFrontendStep != NULL) {
            return 0;
        }
        gOptMainSel++;
        if (gOptMainSel > 2) {
            gOptMainSel = 0;
        }
        break;
    case kGuiMsgButton22:
    case kGuiMsgButton24:
    case 0x2f:
    case 0x31:
        if (gFrontendStep != NULL) {
            return 1;
        }
        if (gOptMainSel == 1 && gOptionsAvail == 0) {
            menuPlaySound(0x7d4);
            return 1;
        }
        gKeyPlayer = (msg == kGuiMsgButton22 || msg == kGuiMsgButton24) ? 0 : 1;
        gOptMainReady = 0;
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402590, optionsMainSlideOut);
        menuPlaySound(0x7d2);
        return 1;
    case kGuiMsgButton25:
    case kGuiMsgIdle:
        if (gFrontendStep != NULL) {
            return 1;
        }
        gOptMainSel = -1;
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402590, optionsMainSlideOut);
        if (gGuiIdleMode != 0) {
            gOptMainReady = 0;
            gOptMainSel = -1;
            return 1;
        }
        gOptMainReady = 0;
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402590, optionsMainSlideOut);
        menuPlaySound(0x7d2);
        return 1;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
    optionsMainHighlight();
    menuPlaySound(2000);
    return 0;
}

/* Brightens the selected row's label and gives it the highlight style; the others are dimmed. */
BMX_ADDR(0x004436c0)
void optionsMainHighlight(void)
{
    menuSetTint(0xa00c0005, 0xff, 0xff, 0xff, gOptMainSel != 0 ? 0x50 : 0xff);
    menuSetTint(0xa00c0007, 0xff, 0xff, 0xff, gOptMainSel != 1 ? 0x50 : 0xff);
    menuSetTint(0xa00c0009, 0xff, 0xff, 0xff, gOptMainSel != 2 ? 0x50 : 0xff);
    menuSetStyle(0xa00c0006, gOptMainSel == 0);
    menuSetStyle(0xa00c0008, gOptMainSel == 1);
    menuSetStyle(0xa00c000a, gOptMainSel == 2);
}

/* Leaving the options: after a short delay frees the frame sprite, saves and returns to the main menu. */
BMX_ADDR(0x00443830)
void optionsMainSave(void)
{
    if (gOptSaveDelay != 0) {
        gOptSaveDelay--;
        return;
    }
    menuFrameFree();
    saveFileWrite();
    mainMenuInit(1);
}

/* Moves the list rows out; when they are gone starts the chosen panel (or the exit when sel is -1). */
BMX_ADDR(0x00443870)
void optionsMainSlideOut(void)
{
    optionsHidePanelRows();
    guiSetTextPos(guiFindById(0xa00c0006), (int16_t)((0x28 - gMenuSlide0) * 4), 0);
    guiSetTextPos(guiFindById(0xa00c0008), (int16_t)((0x28 - gMenuSlide1) * 4), 0);
    guiSetTextPos(guiFindById(0xa00c000a), (int16_t)((0x28 - gMenuSlide2) * 4), 0);
    if (gGuiIdleMode == 0 && menuSlideOut(kOptMainRow0, kOptMainRow1, kOptMainRow2, kOptMainRow3) == 0) {
        return;
    }
    menuHide(0xa00c0006);
    menuHide(0xa00c0008);
    menuHide(0xa00c000a);
    menuHide(0xa00c000c);
    menuHide(0xa00c0005);
    menuHide(0xa00c0007);
    menuHide(0xa00c0009);
    menuHide(0xa00c000b);
    switch (gOptMainSel) {
    case 0:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x0040170d, keyboardOpen);
        break;
    case 1:
        optionsGameOpen();
        break;
    case 2:
        soundPanelOpen();
        break;
    case -1:
        gOptSaveDelay = 2;
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004016c2, optionsMainSave);
        break;
    default:
        break;
    }
}

/* Slides the list rows in; once they are there the highlight frame starts and the step ends. */
BMX_ADDR(0x00443af0)
void optionsMainStep(void)
{
    menuShow(0xa00c0006);
    menuShow(0xa00c0008);
    menuShow(0xa00c000a);
    menuShow(0xa00c0005);
    menuShow(0xa00c0007);
    menuShow(0xa00c0009);
    menuHide(0xa00c000b);
    if (gGuiIdleMode != 0 || menuSlideIn(kOptMainRow0, kOptMainRow1, kOptMainRow2, kOptMainRow3) != 0) {
        menuHide(0xa00c0014);
        menuHide(0xa00c0013);
        optionsMainHighlight();
        gOptMainReady = 1;
        gFrontendStep = NULL;
    }
    guiSetTextPos(guiFindById(0xa00c0006), (int16_t)((0x28 - gMenuSlide0) * 4), 0);
    guiSetTextPos(guiFindById(0xa00c0008), (int16_t)((0x28 - gMenuSlide1) * 4), 0);
    guiSetTextPos(guiFindById(0xa00c000a), (int16_t)((0x28 - gMenuSlide2) * 4), 0);
}

/* First tick of the options screen: creates the frame sprite and slides the three rows in. */
BMX_ADDR(0x00443cb0)
void optionsMainLayout(void)
{
    gOptMainSel = 0;
    gOptMainReady = 0;
    menuFrameInit();
    menuSlideReset();
    menuSlideIn(kOptMainRow0, kOptMainRow1, kOptMainRow2, kOptMainRow3);
    guiSetTextPos(guiFindById(0xa00c0006), (int16_t)((0x28 - gMenuSlide0) * 4), 0);
    guiSetTextPos(guiFindById(0xa00c0008), (int16_t)((0x28 - gMenuSlide1) * 4), 0);
    guiSetTextPos(guiFindById(0xa00c000a), (int16_t)((0x28 - gMenuSlide2) * 4), 0);
    menuSetText(0xa00c0006, zstringGet(0x132));
    menuSetText(0xa00c0008, zstringGet(0x133));
    menuSetText(0xa00c000a, zstringGet(0x139));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401dcf, optionsMainStep);
}
