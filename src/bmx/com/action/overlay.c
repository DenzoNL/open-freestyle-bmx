/* In-game overlay: HUD elements and menu screens (C:\zaxis\Bmx\Src\com\action\overlay.c). */
#include "bmx/com/action/overlay.h"

#include <stdbool.h>

#include "bmx/com/action/action.h"
#include "bmx/com/action/camera.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/memcard.h"
#include "bmx/com/action/overlymp.h"
#include "bmx/com/action/replay.h"
#include "bmx/com/action/rider.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/missingzge.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzres.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/pc/zge/sound/winchan.h"
#include "compat/crt.h"
#include "zge/zasset.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char overlayFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\overlay.c";

/* Colours are 0xAARRGGBB-ish words of the overlay layer; these are the recurring ones. */
enum {
    kOvlColorWhite = 0xf0f0f0,
    kOvlColorItem = 0x13040e0,
    kOvlColorLabel = 0xd0d0d0,
    kOvlColorFill = 0x2d0d0d0,
};

/* Menu input and click sounds. */
enum {
    kOvlSoundMove = 2000,
    kOvlSoundSelect = 0x7d1,
    kOvlSoundConfirm = 0x7d2,
    kOvlSoundBack = 0x7d3,
    kOvlSoundDeny = 0x7d4,
    kOvlSoundDelete = 0x7d5,
};

/* Pad buttons as ovlPadPressed (contrlerPressed) masks. */
enum {
    kPadUp = 1,
    kPadDown = 2,
    kPadLeft = 4,
    kPadRight = 8,
    kPadCancel = 0x10010,
    kPadConfirm = 0x20080,
    kPadStart = 0x4000,
};

static void ovlSound(int id, int a, int b)
{
    bmxSoundPlayAt(id, (const void *)(intptr_t)a, b, 0, 0);
}

/* x that centres a text of width w around column `centre` (the screen is 640 wide). */
static int ovlCentreX(int centre, int16_t w)
{
    return centre - (int)((uint16_t)w >> 1);
}

/* Adds a text overlay centred on the screen's middle and returns its handle. */
static uint32_t ovlAddCentred(int layer, const OvlFont *font, const char *text, int y, uint32_t color,
                              int flags)
{
    const int16_t w = zoverTextWidth(text, font);
    return zoverAddText(layer, font, text, ovlCentreX(0x13f, w), y, color, flags);
}

/* Removes the overlay behind *handle (if any) and clears it. */
static void ovlDropHandle(uint16_t *handle)
{
    if (*handle != kOvlNoHandle) {
        zoverRemove(*handle);
        *handle = kOvlNoHandle;
    }
}

static void ovlDropHandles(uint16_t *first, const uint16_t *end)
{
    for (uint16_t *p = first; p < end; p++) {
        ovlDropHandle(p);
    }
}

/* Draws "PLAYER n:" and the characters typed so far for every player; advances *y per player. */
BMX_ADDR(0x00414580)
void ovlDrawPlayerNames(int *y)
{
    char buf[32];
    for (uint32_t i = 0; i < gPlayerCount; i++) {
        uint8_t *rec = gPlayerRecs[i];
        zstrFormat(buf, "%s %s:", zstringGet(0x1000), zstringGet((uint16_t)(0x100c + i)));
        zoverDrawText(&gOvlFontSmall, buf, 0x35, (uint16_t)*y, 0x40e0, 0);
        const uint32_t count = *(uint32_t *)(rec + kOvlPlayerNameCount);
        if (count != 0) {
            buf[1] = '\0';
            uint32_t x = 0x86;
            for (uint32_t c = 0; c < count; c++, x += 0x86) {
                buf[0] = gameGetNameTable()[c];
                uint32_t color = 0xd0d0d0;
                if (*(uint32_t *)(rec + kOvlPlayerReady) == 0 && c == count - 1 && (gOvlFrame & 1) != 0) {
                    color = 0x2c05050;
                }
                zoverDrawText(&gOvlFontSmall, buf, (int)(x / (count + 1) + 0x1b4), (uint16_t)*y, color, 0);
            }
        }
        *y += 0x1d;
    }
}

/* Update of screen 25 (the demo / replay pause menu). Item 1 returns 0 although it acts (original quirk). */
BMX_ADDR(0x004146f0)
int ovlScreen25Update(void)
{
    if (ovlPadConfirm(0) == 0) {
        return 0;
    }
    switch (gOvlScreen->selected) {
    case 0:
        ovlSetScreen(0);
        gameBigGameFunction(gameTestOption(1) == 0 ? 3 : 6);
        ovlSound(kOvlSoundConfirm, 0, 0);
        return 1;
    case 1:
        ovlSetScreen(0);
        actionSetMode(2);
        gDemoPlayer = gCurrentPlayer;
        ovlSound(kOvlSoundConfirm, 0, 0);
        return 0;
    case 2:
        gOvlReturnScreen = 0x19;
        ovlSetScreen(7);
        ovlSound(kOvlSoundConfirm, 0, 0);
        return 1;
    default:
        return 0;
    }
}

/* Creates the overlay layer, clears the handles, initialises every HUD element and hooks the draw callback.
 */
BMX_ADDR(0x004147e0)
void ovlInit(void)
{
    gOvlScreen = NULL;
    const uint32_t root = zoverAddRect(1, 0, 0, 0x280, 0x1e0, 0x11202020);
    gOvlBackdropRect = (uint16_t)root;
    zoverShow(root, 0);
    gOvlBackdrop = 0xff;
    ovlSetBackdrop(0);
    for (int i = 0; i < 24; i++) {
        gOvlHandles[i] = kOvlNoHandle;
    }
    for (int i = 0; i < 17; i++) {
        OvlElement *e = &gOvlElements[i];
        if (e->init != NULL) {
            e->init(1);
        }
        e->flags = 0;
        ovlSetElementVisible(i, 0);
    }
    gOvlScreenId = 0;
    zrollerAddDrawCallback(BMX_FNPTR(ZrollerDrawFn, 0x00401168, ovlDraw));
}

/* Leaves the current screen, destroys every overlay and element, and unregisters the string tables. */
BMX_ADDR(0x004148b0)
void ovlShutdown(void)
{
    ovlSetScreen(0);
    ovlDropHandles(gOvlHandles, gOvlHandles + 24);
    for (int i = 0; i < 17; i++) {
        if (gOvlElements[i].init != NULL) {
            gOvlElements[i].init(0);
        }
    }
    zoverRemove(gOvlBackdropRect);
    gOvlBackdropRect = kOvlNoHandle;
    zstringUnregister(gOvlStrings[0]);
    zstringUnregister(gOvlStrings[1]);
}

/* Reserves the image slots of the four fonts and loads and parses the font resources 0x7531..0x7535. */
static void ovlLoadFont(uint16_t resource, OvlFont *font, const char *missing, const char *parse, int line)
{
    if (zassetFind(gOvlLibrary, resource, 0) == 0) {
        zdebugFail(missing, overlayFile, line, 0);
    }
    const uint32_t size = zassetSize(gOvlLibrary);
    void *data = zmemStaticAlloc(size);
    if (data == NULL) {
        zdebugFail("no memory for font", overlayFile, line + 3, 0);
    }
    zassetRead(gOvlLibrary, data);
    if (zoverParseFont(data, font) != 0) {
        zdebugFail(parse, overlayFile, line + 6, 0);
    }
}

BMX_ADDR(0x00414970)
void ovlLoadFonts(void)
{
    gOvlImgSmall = (uint16_t)zrollerReserve(2);
    ngifzresRegister(0x30, 0, (uint16_t)(gOvlImgSmall + 1));
    ngifzresRegister(0x31, 0, gOvlImgSmall);
    ovlLoadFont(0x7531, &gOvlFontSmall, "missing font data (small)", "error parsing font (small)", 0x491);
    gOvlFontSmall.image = &gOvlImgSmall;

    gOvlImgTiny = (uint16_t)zrollerReserve(1);
    ngifzresRegister(0x32, 0, gOvlImgTiny);
    ovlLoadFont(0x7532, &gOvlFontTiny, "missing font data (tiny)", "error parsing font (tiny)", 0x4a5);
    gOvlFontTiny.image = &gOvlImgTiny;

    gOvlImgButton = (uint16_t)zrollerReserve(1);
    ngifzresRegister(0x33, 0, gOvlImgButton);
    ovlLoadFont(0x7534, &gOvlFontButton, "missing font data (button)", "error parsing font (button)", 0x4b9);
    gOvlFontButton.image = &gOvlImgButton;

    gOvlImgIcons = (uint16_t)zrollerReserve(2);
    ngifzresRegister(0x34, 0, (uint16_t)(gOvlImgIcons + 1));
    ngifzresRegister(0x35, 0, gOvlImgIcons);
    ovlLoadFont(0x7535, &gOvlFontIcons, "missing font data (icons)", "error parsing font (icons)", 0x4d5);
    gOvlFontIcons.image = &gOvlImgIcons;

    gOvlResA = (uint16_t)zrollerReserve(1);
    ngifzresRegister(0x39, 0, gOvlResA);
    gOvlResB = (uint16_t)zrollerReserve(1);
    ngifzresRegister(0x3a, 0, gOvlResB);
}

/* Loads the UI string tables of a language: the common one and the one-or-two player variant. */
BMX_ADDR(0x00414dd0)
void ovlLoadStrings(uint32_t language)
{
    int line;
    language &= 0xff;
    if (zassetFind(gOvlLibrary, (int)(language + 0x7d01), 0) == 0) {
        line = 0x522;
    } else {
        uint32_t size = zassetSize(gOvlLibrary);
        gOvlStrings[0] = zmemStaticAlloc(size);
        if (gOvlStrings[0] == NULL) {
            zdebugFail("no memory for string table", overlayFile, 0x50f, 0);
        }
        zassetRead(gOvlLibrary, (void *)gOvlStrings[0]);
        zstringRegister(gOvlStrings[0]);
        int id = (int)(language + 0x7d05);
        if (1 < gPlayerCount) {
            id = (int)(language + 0x7d09);
        }
        if (zassetFind(gOvlLibrary, id, 0) != 0) {
            size = zassetSize(gOvlLibrary);
            gOvlStrings[1] = zmemStaticAlloc(size);
            if (gOvlStrings[1] == NULL) {
                zdebugFail("no memory for string table", overlayFile, 0x51b, 0);
            }
            zassetRead(gOvlLibrary, (void *)gOvlStrings[1]);
            zstringRegister(gOvlStrings[1]);
            return;
        }
        line = 0x51f;
    }
    zdebugFail("string table not found - invalid language?", overlayFile, line, 0);
}

/* Per frame draw callback (registered with the renderer): the screen or the HUD elements, then flush. */
BMX_ADDR(0x00414f60)
int ovlDraw(void)
{
    if (gOvlScreen == NULL) {
        for (int i = 0; i < 17; i++) {
            const OvlElement *e = &gOvlElements[i];
            if ((e->flags & 1) == 0 && e->draw != NULL) {
                e->draw();
            }
        }
        zoverFlush();
        return 1;
    }
    if (gOvlScreen->draw != NULL) {
        gOvlScreen->draw();
    } else if (gOvlScreen->itemCount != 0) {
        zoverSetColor(gOvlHandles[gOvlScreen->selected], &gOvlFontSmall, ovlPulseColor(), 0);
    }
    if (gOvlSaving != 0) {
        uint32_t level = gOvlFrame;
        bool scaled = true;
        if (level > 6) {
            if (level < 0x34) {
                level = 0xc0;
                scaled = false;
            } else if (level >= 0x3c) {
                gOvlSaving = 0;
                zoverFlush();
                return 1;
            } else {
                level = 0x3c - level;
            }
        }
        if (scaled) {
            level = (uint8_t)bmxFtol((double)level * (double)0x1.b6db6ep+4f);
        }
        if (level != 0) {
            char text[32];
            zstrFormat(text, "%s", "SAVING GAME DATA...");
            const uint32_t c = level & 0xff;
            zoverDrawText(&gOvlFontTiny, text, 0x2d, 0x21, 0x3000000 | c << 16 | c << 8 | c, 0);
            const int16_t w = zoverTextWidth(text, &gOvlFontTiny);
            zoverDrawRect(0x28, 0x1e, w + 10, 0x1a, 0xc0b060);
        }
    }
    zoverFlush();
    return 1;
}

/* Highlights the selected caption in the idle colour (screens without a draw callback pulse it instead). */
static void ovlRestoreItemColor(void)
{
    if (gOvlScreen->draw == NULL) {
        zoverSetColor(gOvlHandles[gOvlScreen->selected], &gOvlFontSmall, kOvlColorItem, 0);
    }
}

/*
 * Per frame update: runs the screen's own input handler, then moves the selection with the d-pad of
 * every player (or only the current one).
 */
BMX_ADDR(0x00415140)
int ovlUpdate(void)
{
    if (gOvlScreen == NULL) {
        return 0;
    }
    gOvlPulse++;
    if (gOvlFrame >= gOvlDrawDelay) {
        const int handled = gOvlScreen->update();
        if (handled != 0) {
            return handled;
        }
        if (gOvlScreen == NULL) {
            return 0;
        }
        if (gOvlScreen->itemCount > 1) {
            for (uint32_t p = 0; p < gPlayerCount; p++) {
                if (gOvlSharedPad != 0 && p != gCurrentPlayer) {
                    continue;
                }
                if (contrlerPressed((int)p, kPadDown, 0) != 0) {
                    ovlRestoreItemColor();
                    const int last = gOvlScreen->itemCount - 1;
                    gOvlScreen->selected = gOvlScreen->selected < last ? gOvlScreen->selected + 1 : 0;
                } else if (contrlerPressed((int)p, kPadUp, 0) != 0) {
                    ovlRestoreItemColor();
                    const uint8_t at = gOvlScreen->selected;
                    gOvlScreen->selected = (uint8_t)((at != 0 ? at : gOvlScreen->itemCount) - 1);
                } else {
                    continue;
                }
                ovlSound(kOvlSoundMove, 0, 0);
                gOvlPulse = 0;
                return 1;
            }
        }
    }
    gOvlFrame++;
    return 0;
}

/* Shows (non-zero) or hides HUD element `index`, telling its show callback when the state changes. */
BMX_ADDR(0x00415300)
void ovlSetElementVisible(int index, uint32_t show)
{
    OvlElement *e = &gOvlElements[index];
    if (show != (~e->flags & 1)) {
        if (e->show != NULL) {
            e->show((int)show);
        }
        if (show != 0) {
            e->flags &= ~1u;
        } else {
            e->flags |= 1;
        }
    }
}

BMX_ADDR(0x00415370)
void ovlSetElementA(int index, const void *value)
{
    if (gOvlElements[index].setA != NULL) {
        gOvlElements[index].setA(value);
    }
}

BMX_ADDR(0x004153a0)
void ovlSetElementB(int index, void *value)
{
    if (gOvlElements[index].setB != NULL) {
        gOvlElements[index].setB(value);
    }
}

/* Id of the open screen; while the transient screen 7 is up, the screen it returns to. */
BMX_ADDR(0x004153d0)
uint32_t ovlGetScreenId(void)
{
    if (gOvlScreenId == 7) {
        return gOvlReturnScreen;
    }
    return gOvlScreenId;
}

/* Switches to screen `id` (0 closes the overlay, 0x1a reopens the last screen). */
BMX_ADDR(0x00415400)
void ovlSetScreen(uint32_t id)
{
    if (id == 0x1a) {
        id = gOvlLastScreen;
    } else if (id != 0) {
        gOvlLastScreen = (uint8_t)id;
    }
    if (gOvlScreen != NULL && gOvlScreen->enter != NULL) {
        gOvlScreen->enter(0);
    }
    gOvlDrawDelay = 0;
    gOvlSharedPad = 1;
    gOvlSaving = 0;
    ovlResetScreenState(0);
    if (id == 0) {
        ovlSound(1000, 2, 0);
        zoverSetLayerFlag(0);
        if (gOvlScreenId == 7) {
            gOvlScreenId = gOvlReturnScreen;
        }
        actionSetMode(0);
        gOvlScreenId = 0;
        gOvlScreen = gOvlScreens[0];
        return;
    }
    if (gOvlScreenId == 7 && id == gOvlReturnScreen) {
        gOvlScreen = gOvlScreens[id];
        gOvlScreenId = (uint8_t)id;
        gOvlScreen->selected = 0;
    } else {
        if (gOvlScreenId != 0 && id != 7) {
            actionSetMode((uint32_t)-1);
        }
        gOvlScreen = gOvlScreens[id];
        gOvlScreenId = (uint8_t)id;
        gOvlScreen->selected = 0;
        if (id != 7) {
            actionSetMode(1);
        }
    }
    switch (id) {
    case 1:
        gOvlScreen->itemCount = (gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel < 6) ? 6 : 5;
        break;
    case 10:
        gOvlDrawDelay = 0xf;
        if (gameGetMode() != 0) {
            gOvlSharedPad = 0;
        }
        break;
    case 0x16:
        gOvlSharedPad = 0;
        break;
    case 0xc:
    case 0x17:
        gOvlSharedPad = 0;
        gOvlDrawDelay = 0xf;
        break;
    case 0x11:
    case 0x19:
        gOvlDrawDelay = 0xf;
        break;
    case 0x18:
        gOvlScreen->itemCount = (gameGetMode() == 3 && gameTestOption(4) == 0) ? 3 : 2;
        gOvlDrawDelay = 0xf;
        break;
    default:
        break;
    }
    zoverSetLayerFlag(1);
    ovlDropHandles(gOvlHandles, gOvlHandles + 24);
    if (gOvlScreen->enter != NULL) {
        gOvlScreen->enter(1);
    }
    ovlSetBackdrop(0);
}

/* Draws text with the tiny font. */
BMX_ADDR(0x004156f0)
void ovlDrawTinyText(const char *text, int x, int y, uint32_t color)
{
    zoverDrawText(&gOvlFontTiny, text, x, y, color, 0);
}

/* Starts a fresh screen: sets the page, restarts the frame counters and selects the first item. */
BMX_ADDR(0x00415730)
void ovlResetScreenState(uint32_t page)
{
    gOvlPage = (uint8_t)page;
    gOvlFrame = 0;
    gOvlPulse = 0;
    if (gOvlScreen != NULL) {
        gOvlScreen->selected = 0;
    }
}

/* Shows (flag 0) or hides (flag != 0) the full-screen backdrop rectangle when the state changes. */
BMX_ADDR(0x00415770)
void ovlSetBackdrop(uint32_t flag)
{
    if (flag != gOvlBackdrop) {
        if (gOvlBackdrop == 0) {
            zoverShow(gOvlBackdropRect, 0);
        }
        if (flag == 0) {
            zoverShow(gOvlBackdropRect, 1);
        }
        gOvlBackdrop = (uint8_t)flag;
    }
}

static OvlProfile *ovlProfile(uint32_t player)
{
    return &gProfiles[0] + player;
}

/* Item list state shared by the dynamic menus: appends {action, caption} to the table at `items`. */
static void ovlAddMenuItem(OvlMenuItem *items, uint16_t action, uint16_t text)
{
    OvlMenuItem *item = &items[gOvlScreen->itemCount];
    item->action = action;
    item->text = text;
    gOvlScreen->itemCount++;
}

/* y of the top of a block of `height` pixels centred vertically (the screen is 480 high). */
static int ovlCentreY(int height)
{
    return 0xf0 - (height >> 1);
}

/* y of the first of `count` items that are `step` apart, centred vertically. */
static int ovlListTop(int count, int step)
{
    return ovlCentreY(count * step);
}

/* Pause menu (screen 1): title and a dynamic list of items. */
BMX_ADDR(0x004157e0)
void ovlScreen1Enter(int create)
{
    if (create == 0) {
        return;
    }
    const char *title = zstringGet(0x1079);
    gOvlHandles[23] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, title, 0x3c, kOvlColorWhite, 0);
    gOvlScreen->itemCount = 0;
    ovlAddMenuItem(gOvlMenuItems, 0, 0x107a);
    if (gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel < 6) {
        ovlAddMenuItem(gOvlMenuItems, 2, 0x107c);
    }
    for (uint16_t k = 0; k < 4; k++) {
        ovlAddMenuItem(gOvlMenuItems, (uint16_t)(k + 3), (uint16_t)(0x107d + k));
    }
    int y = ovlListTop(gOvlScreen->itemCount, 0x1d);
    for (uint32_t i = 0; i < gOvlScreen->itemCount; i++) {
        const char *text = zstringGet(gOvlMenuItems[i].text);
        gOvlHandles[i] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, text, y, kOvlColorItem, 0);
        y += 0x1d;
    }
}

BMX_ADDR(0x00415a00)
int ovlScreen1Update(void)
{
    uint32_t next;
    if (contrlerPressed(gCurrentPlayer, 0x14000, 0) != 0) {
        ovlSetScreen(0);
        ovlSound(1000, 2, 0);
        ovlSound(kOvlSoundConfirm, 0, 0);
        return 1;
    }
    if (contrlerPressed(gCurrentPlayer, kPadConfirm, 0) == 0) {
        return 0;
    }
    switch (gOvlMenuItems[gOvlScreen->selected].action) {
    case 0:
        ovlSetScreen(0);
        ovlSound(1000, 2, 0);
        goto confirm;
    case 1:
        gameStartMode((int)gameGetMode());
        gameBigGameFunction(3);
        goto confirm;
    case 2:
        next = 0xe;
        break;
    case 3:
        next = 2;
        break;
    case 4:
        next = 4;
        break;
    case 5:
        next = 0x12;
        break;
    case 6:
        gOvlReturnScreen = 1;
        next = 7;
        break;
    default:
        goto confirm;
    }
    ovlSetScreen(next);
confirm:
    ovlSound(kOvlSoundConfirm, 0, 0);
    return 1;
}

/* Options screen (2): title, two labelled values and a footer. */
BMX_ADDR(0x00415b70)
void ovlScreen2Enter(int create)
{
    static const uint16_t captions[2] = {0x1083, 0x1085};
    if (create == 0) {
        return;
    }
    gOvlHandles[3] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet(0x1082), 0x3c, kOvlColorWhite, 0);
    int y = ovlListTop(gOvlScreen->itemCount, 0x21);
    for (int i = 0; i < 2; i++) {
        char buf[32];
        zstrFormat(buf, "%s:", zstringGet(captions[i]));
        gOvlHandles[4 + i] = (uint16_t)zoverAddText(1, &gOvlFontSmall, buf, 0x56, y, kOvlColorItem, 0);
        y += 0x21;
    }
    gOvlHandles[2] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet(0x1086), y, kOvlColorItem, 0);
    ovlScreen2Refresh();
}

BMX_ADDR(0x00415cf0)
int ovlScreen2Update(void)
{
    OvlProfile *profile = ovlProfile(gCurrentPlayer);
    if (contrlerPressed(gCurrentPlayer, kPadRight, 0) != 0 ||
        contrlerPressed(gCurrentPlayer, kPadLeft, 0) != 0) {
        if (gOvlScreen->selected == 0) {
            const uint8_t limit = (gOptionFlags & 1) != 0 ? 3 : 2;
            if (contrlerPressed(gCurrentPlayer, kPadRight, 0) == 0) {
                profile->option = profile->option == 0 ? limit : (uint8_t)(profile->option - 1);
            } else {
                profile->option++;
                if (limit < profile->option) {
                    profile->option = 0;
                }
            }
            ovlScreen2Refresh();
            comdataSetViewMode(gCurrentPlayer, profile->option);
            ovlSound(kOvlSoundSelect, 0, 0);
            return 1;
        }
        if (gOvlScreen->selected == 1) {
            profile->flags ^= 2;
            const uint32_t arg = 0xff;
            ovlSetElementA(10, &arg);
            ovlScreen2Refresh();
            ovlSound(kOvlSoundSelect, 0, 0);
            return 1;
        }
    }
    if (contrlerPressed(gCurrentPlayer, kPadConfirm, 0) != 0) {
        ovlSetScreen(gOvlScreen->selected == 2 ? 3 : 1);
        ovlSound(kOvlSoundConfirm, 0, 0);
        return 1;
    }
    if (contrlerPressed(gCurrentPlayer, kPadStart, 0) != 0) {
        ovlSetScreen(1);
        ovlSound(kOvlSoundConfirm, 0, 0);
        return 1;
    }
    if (contrlerPressed(gCurrentPlayer, kPadCancel, 0) != 0) {
        ovlSetScreen(1);
        ovlSound(kOvlSoundBack, 0, 0);
        return 1;
    }
    if ((contrlerGet(profile->controller)[0xc] & 3) != 0) {
        profile->flags &= 0xfe;
        ovlScreen2Refresh();
        if ((contrlerGet(profile->controller)[0xc] & 2) != 0) {
            ovlSound(kOvlSoundSelect, 0, 0);
        }
    }
    return 0;
}

/* Rebuilds the two value captions of screen 2 (selected value highlighted, right aligned at x 553). */
BMX_ADDR(0x00416060)
void ovlScreen2Refresh(void)
{
    const OvlProfile *profile = ovlProfile(gCurrentPlayer);
    const int top = -((gOvlScreen->itemCount * 0x21) >> 1);
    const uint8_t selected = gOvlScreen->selected;

    const char *text = zstringGet((uint16_t)(0x1087 + profile->option));
    if (gOvlHandles[0] != kOvlNoHandle) {
        zoverRemove(gOvlHandles[0]);
    }
    uint32_t color = (selected != 0 ? 0xfe3f4ff0u : 0u) + 0x2f0f0f0;
    int16_t w = zoverTextWidth(text, &gOvlFontSmall);
    gOvlHandles[0] =
        (uint16_t)zoverAddText(1, &gOvlFontSmall, text, 0x229 - (uint16_t)w, top + 0xf0, color, 0);

    text = zstringGet((uint16_t)(0x101a + ((profile->flags >> 1) & 1)));
    if (gOvlHandles[1] != kOvlNoHandle) {
        zoverRemove(gOvlHandles[1]);
    }
    color = (selected != 1 ? 0xfe3f4ff0u : 0u) + 0x2f0f0f0;
    w = zoverTextWidth(text, &gOvlFontSmall);
    gOvlHandles[1] =
        (uint16_t)zoverAddText(1, &gOvlFontSmall, text, 0x229 - (uint16_t)w, top + 0x111, color, 0);
}

/* Sound options screen (3): title and the three option captions (music, effects, voice set). */
BMX_ADDR(0x004161f0)
void ovlScreen3Enter(int create)
{
    if (create == 0) {
        ovlDropHandles(gOvlVolHandles, gOvlVolHandles + 7);
        return;
    }
    for (int i = 0; i < 7; i++) {
        gOvlVolHandles[i] = kOvlNoHandle;
    }
    gSoundVolumes[2] = (uint8_t)bmxSoundPlayAt(0x3ec, NULL, 0, 0, 0);
    gOvlVolHandles[0] =
        (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet(0x108c), 0x3c, kOvlColorWhite, 0);
    int y = ovlCentreY(gOvlScreen->itemCount * 0x1d + 0x2c);
    for (uint32_t i = 0; i < gOvlScreen->itemCount; i++) {
        char buf[64];
        zstrFormat(buf, "%s:", zstringGet((uint16_t)(0x108d + i)));
        int x = 0x35;
        if (&gOvlVolHandles[1 + i] >= &gOvlVolHandles[3]) {
            x = ovlCentreX(0x13f, zoverTextWidth(buf, &gOvlFontSmall));
        }
        gOvlVolHandles[1 + i] = (uint16_t)zoverAddText(1, &gOvlFontSmall, buf, x, y, kOvlColorItem, 0);
        y += 0x21;
    }
    ovlScreen3Refresh();
}

/* Volume (0..0x12) to the 0..0x7f range of the sound layer. */
static uint32_t ovlVolumeLevel(uint8_t volume)
{
    const uint32_t level = (uint32_t)volume * 0x7f / 0x12;
    return level > 0x7e ? 0x7f : level;
}

BMX_ADDR(0x004163d0)
int ovlScreen3Update(void)
{
    int sound;
    int a;
    uint32_t b;
    if (contrlerPressed(gCurrentPlayer, kPadDown, 0) != 0) {
        const int last = gOvlScreen->itemCount - 1;
        gOvlScreen->selected = gOvlScreen->selected < last ? gOvlScreen->selected + 1 : 0;
        goto moved;
    }
    if (contrlerPressed(gCurrentPlayer, kPadUp, 0) != 0) {
        const uint8_t at = gOvlScreen->selected;
        gOvlScreen->selected = (uint8_t)((at != 0 ? at : gOvlScreen->itemCount) - 1);
        goto moved;
    }
    if (contrlerPressed(gCurrentPlayer, kPadRight, 0) == 0) {
        if (contrlerPressed(gCurrentPlayer, kPadLeft, 0) == 0) {
            if (contrlerPressed(gCurrentPlayer, kPadCancel, 0) == 0 &&
                contrlerPressed(gCurrentPlayer, kPadConfirm, 0) == 0 &&
                contrlerPressed(gCurrentPlayer, kPadStart, 0) == 0) {
                return 0;
            }
            ovlSetScreen(2);
            ovlSound(kOvlSoundConfirm + (contrlerPressed(gCurrentPlayer, kPadCancel, 0) != 0), 0, 0);
            return 1;
        }
        switch (gOvlScreen->selected) {
        case 0:
        case 1: {
            const int row = gOvlScreen->selected;
            if (gSoundVolumes[row] == 0) {
                break;
            }
            gSoundVolumes[row]--;
            sound = 0x3ea;
            a = row;
            b = ovlVolumeLevel(gSoundVolumes[row]);
            goto changed;
        }
        case 2:
            gSoundVolumes[2] = gSoundVolumes[2] == 0 ? 9 : (uint8_t)(gSoundVolumes[2] - 1);
            sound = 0x3ed;
            a = gSoundVolumes[2];
            b = 0;
            goto changed;
        default:
            goto refresh;
        }
    } else {
        switch (gOvlScreen->selected) {
        case 0:
        case 1: {
            const int row = gOvlScreen->selected;
            if (gSoundVolumes[row] >= 0x12) {
                break;
            }
            gSoundVolumes[row]++;
            sound = 0x3ea;
            a = row;
            b = ovlVolumeLevel(gSoundVolumes[row]);
            goto changed;
        }
        case 2:
            gSoundVolumes[2] = gSoundVolumes[2] < 9 ? (uint8_t)(gSoundVolumes[2] + 1) : 0;
            sound = 0x3ed;
            a = gSoundVolumes[2];
            b = 0;
            goto changed;
        default:
            goto refresh;
        }
    }
    ovlSound(kOvlSoundDeny, 0, 0);
refresh:
    ovlScreen3Refresh();
    return 1;
changed:
    ovlSound(sound, a, (int)b);
    ovlSound(kOvlSoundSelect, 0, 0);
    ovlScreen3Refresh();
    return 1;
moved:
    ovlSound(kOvlSoundMove, 0, 0);
    gOvlPulse = 0;
    ovlScreen3Refresh();
    return 1;
}

/* Rebuilds the selection bar and the voice set captions of screen 3. */
BMX_ADDR(0x00416810)
void ovlScreen3Refresh(void)
{
    ovlDropHandles(&gOvlVolHandles[4], &gOvlVolHandles[7]);
    int y = ovlCentreY(gOvlScreen->itemCount * 0x1d + 0x2c);
    for (uint32_t i = 0; i < gOvlScreen->itemCount; i++) {
        if (i == 2) {
            const uint32_t color = (gOvlScreen->selected != 2 ? 0xff6f6f70u : 0u) + 0x2d0d0d0;
            const char *text = zstringGet((uint16_t)(gSoundVolumes[2] * 4 + 0x102f));
            gOvlVolHandles[5] = (uint16_t)ovlAddCentred(1, &gOvlFontTiny, text, y + 0x21, color, 0);
            text = zstringGet((uint16_t)(gSoundVolumes[2] * 4 + 0x102e));
            gOvlVolHandles[6] = (uint16_t)ovlAddCentred(1, &gOvlFontTiny, text, y + 0x35, color, 0);
            y += 0x49;
        } else {
            if (i == gOvlScreen->selected) {
                gOvlVolHandles[4] = (uint16_t)zoverAddRect(1, 0x1d2, y - 3, 0x75, 0x12, 0x1000000);
            }
            y += 0x21;
        }
    }
}

/* Draws the volume bars (one 3x12 block per volume step) of the first two options. */
BMX_ADDR(0x00416a10)
void ovlScreen3Draw(void)
{
    int y = ovlCentreY(gOvlScreen->itemCount * 0x1d + 0x2c);
    for (uint32_t row = 0; row < gOvlScreen->itemCount; row++) {
        if (row < 2) {
            int x = 0x1d8;
            for (uint32_t n = gSoundVolumes[row]; n != 0; n--) {
                const uint32_t color = row == gOvlScreen->selected ? ovlPulseColor() : kOvlColorItem;
                zoverDrawRect(x, y, 3, 0xc, color);
                x += 6;
            }
            y += 0x21;
        }
    }
}

/* Trick list screens (4..6): a title and a list of the tricks the rider has landed or unlocked. */
BMX_ADDR(0x00416ae0)
void ovlScreen4Enter(int create)
{
    if (create == 0) {
        ovlDropHandles(gOvlTrickRows, gOvlTrickRows + 21);
        return;
    }
    gOvlHandles[0] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet(0x1090), 0x3c, kOvlColorWhite, 0);
    for (int i = 0; i < 21; i++) {
        gOvlTrickRows[i] = kOvlNoHandle;
    }
    gOvlTrickCount = 0;
    const uint8_t *landed = (const uint8_t *)gRiderData +
                            ovlProfile(gCurrentPlayer)->character * kOvlRiderStride + kOvlRiderTrickBits;
    for (uint32_t k = 0; k < 0x54; k++) {
        const uint8_t bit = (uint8_t)(1u << (k & 7));
        if ((bit & landed[k >> 3]) != 0 || ovlTrickAvailable(k) != 0) {
            uint8_t *entry = gOvlTrickList[gOvlTrickCount++];
            if (k < 0x2e) {
                entry[0] = 0;
                entry[1] = (uint8_t)k;
            } else {
                entry[0] = 1;
                entry[1] = (uint8_t)(k - 0x2e);
            }
            if ((landed[k >> 3] & bit) != 0) {
                entry[0] |= 0x80;
            }
        }
    }
    ovlScreen4Refresh();
}

BMX_ADDR(0x00416c70)
int ovlScreen4Update(void)
{
    uint32_t back;
    if (ovlPadConfirm(0) == 0) {
        for (uint32_t p = 0; p < gPlayerCount; p++) {
            if (gOvlSharedPad != 0 && p != gCurrentPlayer) {
                continue;
            }
            if (contrlerPressed((int)p, kPadCancel, 0) != 0) {
                goto leave;
            }
            if (contrlerPressed((int)p, kPadDown, 0) != 0) {
                if (gOvlTrickView[1] < gOvlTrickCount - 1U) {
                    if ((uint32_t)gOvlTrickView[0] + 10 <= (uint16_t)(gOvlTrickView[1] + 1)) {
                        gOvlTrickView[0]++;
                    }
                    gOvlTrickView[1]++;
                    gOvlPulse = 0;
                    ovlSound(kOvlSoundMove, 0, 0);
                    ovlScreen4Refresh();
                    return 1;
                }
                ovlSound(kOvlSoundDeny, 0, 0);
                return 1;
            }
            if (contrlerPressed((int)p, kPadUp, 0) != 0) {
                if (gOvlTrickView[1] != 0) {
                    if ((uint16_t)(gOvlTrickView[1] - 1) < gOvlTrickView[0]) {
                        gOvlTrickView[0]--;
                    }
                    gOvlTrickView[1]--;
                    gOvlPulse = 0;
                    ovlScreen4Refresh();
                    ovlSound(kOvlSoundMove, 0, 0);
                    return 1;
                }
                ovlSound(kOvlSoundDeny, 0, 0);
                return 1;
            }
        }
        return 0;
    }
    switch (gOvlScreenId) {
    case 4:
        back = 1;
        break;
    case 5:
        back = 9;
        break;
    case 6:
        back = 10;
        break;
    default:
        goto confirmed;
    }
    ovlSetScreen(back);
confirmed:
    ovlSound(kOvlSoundConfirm, 0, 0);
    return 1;
leave:
    switch (gOvlScreenId) {
    case 4:
        ovlSetScreen(1);
        break;
    case 5:
        ovlSetScreen(9);
        break;
    case 6:
        ovlSetScreen(10);
        break;
    default:
        break;
    }
    ovlSound(kOvlSoundBack, 0, 0);
    return 1;
}

BMX_ADDR(0x00416ec0)
void ovlScreen4Draw(void)
{
    ovlDrawMenuItems(8, 0);
}

/* Rebuilds the ten visible rows of the trick list, the "landed" marks and the description of the cursor row.
 */
BMX_ADDR(0x00416ee0)
void ovlScreen4Refresh(void)
{
    ovlDropHandles(gOvlTrickRows, gOvlTrickRows + 21);
    const int top = -((int)(gOvlFontButton.height + 0xce) >> 1);
    int markY = top + 0xf1;
    int y = top + 0xf0;
    int rowY = y;
    for (uint32_t row = 0; row < 10; row++) {
        rowY = y;
        const uint32_t at = (uint32_t)gOvlTrickView[0] + row;
        const uint8_t kind = gOvlTrickList[at][0];
        const uint8_t id = gOvlTrickList[at][1];
        const char *name;
        uint32_t color;
        if ((kind & 0x3f) == 1) {
            name = gTrickBarspinNames[id];
            color = 0x2a07010;
        } else {
            name = gTrickTurnNames[id];
            if (id < 0x1b) {
                color = 0x22030d0;
            } else if (id < 0x24) {
                color = 0x220a040;
            } else if (id < 0x2a) {
                color = 0x2703090;
            } else {
                color = (id < 0x2c ? 0x6fff90u : 0u) + 0x2200080;
            }
        }
        if (at == gOvlTrickView[1]) {
            color = ((kind & 0x40) != 0 ? 0xdfdfe0u : 0u) + 0x2a0a0a0;
        } else if ((kind & 0x40) != 0) {
            color = 0x2202020;
        }
        const int16_t w = zoverTextWidth(name, &gOvlFontTiny);
        gOvlTrickRows[row * 2] =
            (uint16_t)zoverAddText(1, &gOvlFontTiny, name, ovlCentreX(0x13f, w), rowY, color, 0);
        if ((gOvlTrickList[at][0] & 0x80) != 0) {
            gOvlTrickRows[row * 2 + 1] =
                (uint16_t)zoverAddText(1, &gOvlFontIcons, "c", 0x20e, markY, 0x2b04010, 0);
        }
        markY += 0x14;
        y = rowY + 0x14;
    }
    const uint8_t *cursor = gOvlTrickList[gOvlTrickView[1]];
    const char *desc =
        zstringGet((uint16_t)((cursor[0] & 0x3f) == 1 ? cursor[1] + 0x10e9 : cursor[1] + 0x10bb));
    gOvlTrickDesc = (uint16_t)ovlAddCentred(1, &gOvlFontButton, desc, rowY + 0x1a, kOvlColorLabel, 0);
}

/* True when the trick (index into the 0x2e + 0x26 trick table) can be used without having been landed. */
BMX_ADDR(0x00417190)
uint32_t ovlTrickAvailable(uint32_t trick)
{
    if (trick > 0x2d) {
        return ((uint8_t)~gTrickClassHi[trick] & 0x10) >> 4;
    }
    if (trick < 0x1b) {
        return ((uint8_t)~gTrickClassLo[trick] & 0x10) >> 4;
    }
    return 1;
}

/* Screen 12 (goal list of the level / mode): title and the goal texts. */
BMX_ADDR(0x004171e0)
void ovlScreen12Enter(int create)
{
    if (create == 0) {
        return;
    }
    int id = gameGetMode() == 0 ? gCurrentLevel + 0x101e : (int)gameGetMode() + 0x3001;
    gOvlHandles[5] =
        (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet((uint16_t)id), 0x3c, kOvlColorWhite, 0);
    int y = 0xbe;
    for (int k = 0; k < 5; k++) {
        if (gameGetMode() == 0) {
            id = gCurrentLevel < 0xb ? gCurrentLevel * 5 + 0x2000 + k : k + 0x2037;
        } else {
            id = k + ((int)gameGetMode() + 0x99d) * 5;
        }
        gOvlHandles[k] =
            (uint16_t)ovlAddCentred(1, &gOvlFontTiny, zstringGet((uint16_t)id), y, kOvlColorFill, 0);
        y += 0x14;
    }
}

BMX_ADDR(0x00417360)
int ovlScreen12Update(void)
{
    uint32_t next;
    if (ovlPadConfirm(0) != 0) {
        if (gOvlScreen->selected == 0) {
            if (gameGetMode() == 3) {
                next = 0x17;
            } else {
                ovlSetScreen(0);
                gameBigGameFunction(3);
                goto confirm;
            }
        } else if (gOvlScreen->selected == 1) {
            if (gameGetMode() == 0) {
                gOvlReturnScreen = 0xc;
                ovlSetScreen(7);
                gOvlWarnLevel = 3;
                goto confirm;
            }
            next = 0x16;
        } else {
            goto other;
        }
        ovlSetScreen(next);
    confirm:
        ovlSound(kOvlSoundConfirm, 0, 0);
        return 1;
    }
other:
    if (gameGetMode() != 0) {
        for (uint32_t p = 0; p < gPlayerCount; p++) {
            if ((gOvlSharedPad == 0 || p == gCurrentPlayer) && contrlerPressed((int)p, kPadCancel, 0) != 0) {
                ovlSetScreen(0x16);
                ovlSound(kOvlSoundBack, 0, 0);
                return 1;
            }
        }
    }
    return 0;
}

BMX_ADDR(0x004174a0)
void ovlScreen12Draw(void)
{
    ovlDrawMenuItems(0, 0);
}

/* Screens 13..15 (goal set summary, save or replay): the goal texts of the level. */
BMX_ADDR(0x004174c0)
void ovlScreen13Enter(int create)
{
    if (create == 0) {
        return;
    }
    const uint8_t *goals = comdataGetGoalInfo(gCurrentPlayer, gCurrentLevel);
    gOvlHandles[0] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet(0x203c), 0x3c, kOvlColorWhite, 0);
    uint32_t set = goals[1];
    if (goals[1] == 2 && goals[2] == 4) {
        set = 3;
    }
    char buf[128];
    zstrFormat(buf, "%s%s", zstringGet((uint16_t)(set + 0x203d)), set < 3 ? ":" : (const char *)gOvlEmpty);
    gOvlHandles[1] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, buf, 0x8e, kOvlColorItem, 0);
    int handle = 2;
    int y = 0xb3;
    if (set < 3) {
        int id = (gCurrentLevel * 3 + goals[1]) * 0xc + 0x2089;
        for (int row = 0; row < 4; row++) {
            buf[0] = '\0';
            for (int k = 0; k < 3; k++, id++) {
                const char *text = zstringGet((uint16_t)id);
                if (*text != '\0') {
                    zstrFormat(buf + strlen(buf), "%s%s", k == 0 ? (const char *)gOvlEmpty : " ", text);
                }
            }
            gOvlHandles[2 + row] = (uint16_t)ovlAddCentred(1, &gOvlFontTiny, buf, y, kOvlColorFill, 0);
            y += 0x16;
        }
        handle = 6;
    } else {
        y = 0x10b;
    }
    y += 0xc;
    int id = (int)(set + gCurrentLevel * 4) * 3 + 0x2041;
    for (int k = 0; k < 3; k++, id++) {
        gOvlHandles[handle + k] =
            (uint16_t)ovlAddCentred(1, &gOvlFontTiny, zstringGet((uint16_t)id), y, 0x2a07010, 0);
        y += 0x14;
    }
}

BMX_ADDR(0x00417820)
int ovlScreen13Update(void)
{
    if (ovlPadConfirm(0) == 0) {
        if (contrlerPressed(gCurrentPlayer, kPadCancel, 0) == 0 || gOvlScreenId == 0xd) {
            return 0;
        }
    }
    if (gOvlScreenId == 0xd) {
        ovlSetScreen(0);
        gameBigGameFunction(4);
    } else if (gOvlScreenId == 0xe) {
        ovlSetScreen(1);
    } else if (gOvlScreenId == 0xf) {
        saveFileWrite();
        ovlSetScreen(10);
        gOvlSaving = 1;
    }
    if (contrlerPressed(gCurrentPlayer, kPadCancel, 0) != 0) {
        ovlSound(kOvlSoundBack, 0, 0);
        return 1;
    }
    ovlSound(kOvlSoundConfirm, 0, 0);
    return 1;
}

/* Fades the four goal rows in one after another (a rising chirp per row) and draws a completion bar. */
BMX_ADDR(0x00417930)
void ovlScreen13Draw(void)
{
    static const uint8_t fade[16] = {0x20, 0x40, 0x60, 0x78, 0x90, 0xa8, 0xc0, 0xd0,
                                     0xe0, 0xf0, 0xf0, 0xc0, 0xa0, 0x98, 0x90, 0x88};
    const uint8_t *goals = comdataGetGoalInfo(gCurrentPlayer, gCurrentLevel);
    uint32_t set = goals[1];
    if (set == 2 && goals[2] == 4) {
        set = 3;
    }
    if (set < 3) {
        int y = 0xbf;
        for (uint32_t row = 0; row < 4; row++) {
            uint8_t level = 0x80;
            if (gOvlScreenId == 0xd) {
                if (gOvlFrame >> 4 < row) {
                    level = 0;
                } else if (gOvlFrame >> 4 == row) {
                    level = fade[gOvlFrame & 0xf];
                    if ((gOvlFrame & 0xf) == 1) {
                        ovlSound(0x1390, 0, 0);
                    }
                }
            }
            zoverSetColor(gOvlHandles[2 + row], &gOvlFontTiny,
                          0x2000000u | (uint32_t)level << 16 | (uint32_t)level << 8 | level, 0);
            const uint8_t goal = goals[(row + 1 + goals[1] * 4) * 3];
            if ((goal & 0x80) != 0) {
                const uint32_t width =
                    ((goal & 0x40) == 0 || 0xf < gOvlFrame) ? 0x1f5 : gOvlFrame * 0x1f5 >> 4;
                zoverDrawRect(0x45, y, (int)width, 1,
                              0x2000000u | (uint32_t)(uint8_t)((uint32_t)level * 0xc0 >> 8) << 16 |
                                  (uint32_t)(level >> 2) << 8 | (level >> 4));
            }
            y += 0x16;
        }
    }
    ovlDrawMenuItems(8, 0);
}

/* Screen 16 (goal reward / stats page): title; the page content is built by ovlScreen16Refresh. */
BMX_ADDR(0x00417b80)
void ovlScreen16Enter(int create)
{
    if (create != 0) {
        gOvlHandles[0] =
            (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet(0x2161), 0x3c, kOvlColorWhite, 0);
        ovlScreen16Refresh(0);
    }
}

/* Rebuilds screen 16's page (0 trick goals, 1 level goals, 2..4 rewards): captions plus the earned goals. */
BMX_ADDR(0x00417c00)
void ovlScreen16Refresh(uint32_t page)
{
    ovlResetScreenState(page);
    ovlDropHandles(&gOvlHandles[1], &gOvlHandles[24]);
    const uint8_t *player = gPlayerRecs[gCurrentPlayer];
    const uint32_t unlocked = *(const uint32_t *)(player + kOvlPlayerUnlockMask);
    int strBase;
    if (page == 2) {
        strBase = 0x2198;
    } else if (page < 3 || 4 < page) {
        if (gCurrentLevel < 6) {
            const uint8_t *goals = comdataGetGoalInfo(gCurrentPlayer, gCurrentLevel);
            uint32_t set = goals[1];
            if (goals[2] == 4) {
                set++;
            }
            strBase = page == 0 ? (int)(set * 3 + 0x215f)
                                : (int)((set + (uint32_t)(gCurrentLevel / 3) * 3) * 3 + 0x2171);
        } else {
            int found = 0;
            const uint8_t character = ovlProfile(gCurrentPlayer)->character;
            for (uint32_t k = 0; k < 3; k++) {
                if (gCareerUnlocked[k] == character || (character > 9 && gCareerUnlocked[k] == 5)) {
                    found = 3 - (int)k;
                    break;
                }
            }
            strBase = page == 0 ? found * 3 + 0x2168 : (found + (gCurrentLevel / 3) * 3) * 3 + 0x2171;
        }
    } else {
        strBase = (int)page * 3 + 0x2192;
    }
    int y = 0x6c;
    for (int i = 0; i < 3; i++) {
        gOvlHandles[1 + i] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet((uint16_t)(strBase + i)),
                                                     y, kOvlColorItem, 0);
        if (i < 2) {
            y += 0x1d;
        }
    }
    int earned = 0;
    switch (page) {
    case 0:
        for (uint32_t u = 0; u < 3; u++) {
            for (uint32_t v = 0; v < 2; v++) {
                if ((unlocked & 1u << ((u * 2 + v) & 31)) != 0 &&
                    gGoalEntriesA[(v + (u + gCurrentLevel * 3u) * 2)][0] != 0) {
                    earned++;
                }
            }
        }
        break;
    case 1:
        for (uint32_t u = 0; u < 3; u++) {
            if ((unlocked & 1u << ((u + 6) & 31)) != 0) {
                earned++;
            }
        }
        break;
    case 2:
    case 3:
    case 4:
        if ((unlocked & 1u << ((page + 7) & 31)) != 0) {
            earned = 1;
        }
        break;
    default:
        break;
    }
    int handle = 4;
    const uint32_t top = (uint32_t)y + 0x23;
    y = (int)(top + ((0x187u - top) >> 1) - ((uint32_t)earned * 0x17 & 0x7fffffff));
    switch (page) {
    case 0:
        for (uint32_t u = 0; u < 3; u++) {
            for (uint32_t v = 0; v < 2; v++) {
                if ((unlocked & 1u << ((u * 2 + v) & 31)) != 0) {
                    ovlAddGoalLine(gCurrentPlayer, gGoalEntriesA[v + (u + gCurrentLevel * 3u) * 2], &handle,
                                   &y, earned == 1);
                }
            }
        }
        break;
    case 1:
        for (uint32_t u = 0; u < 3; u++) {
            if ((unlocked & 1u << ((u + 6) & 31)) != 0) {
                ovlAddGoalLine(gCurrentPlayer, gGoalEntriesB[u + (gCurrentLevel / 3u) * 3], &handle, &y,
                               earned == 1);
            }
        }
        break;
    case 2:
    case 3:
    case 4:
        if ((unlocked & 1u << ((page + 7) & 31)) != 0) {
            ovlAddGoalLine(gCurrentPlayer, gGoalEntriesC[page], &handle, &y, earned == 1);
        }
        break;
    default:
        break;
    }
    ovlSound(0x1393, (int)page, 0);
}

/*
 * Adds one earned goal to screen 16: a two-layer icon (the goal kind's letter on the icon font) and the
 * two caption lines. Advances *handle past the overlays it makes and *y past the lines.
 */
BMX_ADDR(0x004180f0)
void ovlAddGoalLine(int player, const uint8_t *goal, int *handle, int *y, int centred)
{
    static const char icons[12] = {0x20, 0x42, 0x4f, 0x43, 0x4c, 0x4f, 0x54, 0x54, 0x4d, 0x54, 0x54, 0x54};
    const OvlProfile *profile = ovlProfile((uint32_t)player);
    int text;
    switch (goal[0]) {
    case 1:
        text = goal[1] * 2 + 0x21bb;
        break;
    case 2:
        if (profile->character < 10) {
            text = goal[1] * 2 + 0x21c5;
            break;
        }
        text = 0x21cf;
        goto draw;
    case 3:
        text = goal[1] * 2 + 0x21bd;
        break;
    case 4:
        text = goal[1] * 2 + 0x21a1;
        break;
    case 5:
        text = gGoalColumns[profile->character * 8 + goal[1]] * 2 + 0x21d5;
        break;
    case 6:
        text = goal[1] * 2 + 0x2243;
        break;
    case 7:
        text = (goal[1] + profile->character * 2) * 2 + 0x225f;
        break;
    case 8:
        text = goal[1] * 2 + 0x228f;
        break;
    case 9:
        ovlAddGoalLine(player, gGoalEntriesD[goal[1] + profile->character * 2], handle, y, centred);
        return;
    case 10:
        text = goal[1] * 2 + 0x2291;
        break;
    default:
        return;
    }
    if (text == -1) {
        return;
    }
draw:;
    const char icon[2] = {icons[goal[0]], '\0'};
    int x = 0x56;
    if (centred != 0) {
        uint32_t widest = 0;
        for (int k = 0; k < 2; k++) {
            const char *line = zstringGet((uint16_t)(text + k));
            if (widest <= (uint16_t)zoverTextWidth(line, &gOvlFontTiny)) {
                widest = (uint16_t)zoverTextWidth(line, &gOvlFontTiny);
            }
        }
        x = 0x13f - (int)((widest + 0x28) >> 1);
    }
    const int iconY = (*y - (gOvlFontIcons.height >> 1)) + 0x15;
    gOvlHandles[(*handle)++] = (uint16_t)zoverAddText(1, &gOvlFontIcons, icon, x, iconY, 0x2000000, 2);
    gOvlHandles[(*handle)++] = (uint16_t)zoverAddText(1, &gOvlFontIcons, icon, x, iconY, 0x3000000, 0);
    int last = *y;
    for (int k = 0; k < 2; k++) {
        const char *line = zstringGet((uint16_t)(text + k));
        gOvlHandles[(*handle)++] = (uint16_t)zoverAddText(1, &gOvlFontTiny, line, x + 0x28, *y, 0x2000000, 0);
        last = *y;
        *y = last + 0x14;
    }
    *y = last + 0x1a;
}

BMX_ADDR(0x004184b0)
int ovlScreen16Update(void)
{
    uint32_t rows = 0;
    if (gOvlGoalRows[0][0] != kOvlNoHandle) {
        const uint16_t *row = gOvlGoalRows[0];
        do {
            row += 4;
            rows++;
        } while (*row != kOvlNoHandle);
    }
    if (gOvlFrame >> 4 < rows) {
        return 0;
    }
    if (ovlPadConfirm(0) == 0) {
        return 0;
    }
    const uint32_t page = gOvlPage;
    const uint32_t unlocked = *(const uint32_t *)(gPlayerRecs[gCurrentPlayer] + kOvlPlayerUnlockMask);
    if (page == 0) {
        if ((unlocked & 0x1c0) != 0) {
            ovlScreen16Refresh(1);
            goto confirm;
        }
    } else if (page == 1) {
        if ((unlocked & 0x200) != 0) {
            ovlScreen16Refresh(2);
            goto confirm;
        }
    } else if (page == 2) {
        const uint8_t character = ovlProfile(gCurrentPlayer)->character;
        if (character != 10) {
            if (character == 11) {
                if ((unlocked & 0x800) != 0) {
                    ovlScreen16Refresh(4);
                    goto confirm;
                }
            } else if ((unlocked & 0x400) != 0) {
                ovlScreen16Refresh(3);
                goto confirm;
            }
        }
    }
    saveFileWrite();
    ovlSetScreen(10);
    gOvlSaving = 1;
confirm:
    ovlSound(kOvlSoundConfirm, 0, 0);
    return 1;
}

/* Fades the goal rows in one at a time; the row's glyph layers and its two lines share the fade level. */
BMX_ADDR(0x00418620)
void ovlScreen16Draw(void)
{
    uint32_t rows = 0;
    uint32_t frame = gOvlFrame;
    if (gOvlGoalRows[0][0] != kOvlNoHandle) {
        const uint16_t *row = gOvlGoalRows[0];
        do {
            const uint32_t step = frame >> 4;
            uint32_t level;
            bool draw = true;
            if (step < rows) {
                level = 0;
            } else if (step == rows) {
                const int ramp = bmxFtol((double)(int)(frame & 0xf) * (double)0x1.3333340000000p-1f) * 32;
                level = ramp > 0xf0 ? 0xf0 : (uint8_t)ramp;
                if ((frame & 0xf) == 1) {
                    ovlSound(0x1390, 0, 0);
                    frame = gOvlFrame;
                }
                draw = (uint8_t)level != 0xff;
            } else if (step == rows + 1 && (frame & 0xf) == 0) {
                level = 0xf0;
            } else {
                draw = false;
                level = 0;
            }
            if (draw) {
                zoverSetColor(row[0], &gOvlFontIcons, ((level | 0x400) << 8 | level) << 7 | level >> 1, 2);
                zoverSetColor(row[1], &gOvlFontIcons, ((level | 0xc00) << 8 | level) << 6 | level >> 2, 0);
                const uint32_t color = ((level | 0x200) << 8 | level) << 8 | level;
                zoverSetColor(row[2], &gOvlFontTiny, color, 0);
                zoverSetColor(row[3], &gOvlFontTiny, color, 0);
                frame = gOvlFrame;
            }
            rows++;
            row += 4;
        } while (*row != kOvlNoHandle);
    } else {
        frame = gOvlFrame;
    }
    if (frame >> 4 >= rows) {
        ovlDrawMenuItems(8, 0);
    }
}

/* Name entry screen (17): starts with the player's current name, cursor on its last letter. */
BMX_ADDR(0x00418800)
void ovlScreen17Enter(int create)
{
    if (create != 0) {
        zstrFormat(gOvlNameBuf, "%s", ovlProfile(gCurrentPlayer)->name);
        gOvlNameCursor = (uint8_t)(zstrlen(gOvlNameBuf) - 1);
    }
}

BMX_ADDR(0x00418870)
int ovlScreen17Update(void)
{
    int sound;
    for (uint32_t p = 0; p < gPlayerCount; p++) {
        if (gOvlSharedPad != 0 && p != gCurrentPlayer) {
            continue;
        }
        if (contrlerPressed((int)p, kPadStart, 0) != 0 || contrlerPressed((int)p, kPadConfirm, 0) != 0) {
            for (uint32_t k = 0; k < 10; k++) {
                uint8_t *record = gameGetRecord((((gameGetMode() << 2 | gameGetSubMode()) << 6) | k) << 16);
                if (record != NULL && (record[5] & 1) != 0) {
                    uint8_t *target = (uint8_t *)comdataGetLevelRecord(
                        (((gameGetMode() << 2 | gameGetSubMode()) << 6) | k) << 16);
                    memcpy(target, record, 4);
                    const uint8_t *rider = zrollerGetRecord(p);
                    target[4] = rider[4];
                    target[5] = rider[6];
                    target[6] = rider[7];
                    zstrFormat((char *)target + 7, "%s", gOvlNameBuf);
                    zstrFormat(ovlProfile(gCurrentPlayer)->name, "%s", gOvlNameBuf);
                }
            }
            ovlSetScreen(0x13);
            ovlSound(kOvlSoundConfirm, 0, 0);
            return 1;
        }
        char *at = &gOvlNameBuf[gOvlNameCursor];
        if (contrlerPressed((int)p, kPadLeft, 0) != 0 || contrlerPressed((int)p, 0x20, 0) != 0) {
            if (gOvlNameCursor != 0) {
                gOvlNameBuf[gOvlNameCursor] = 0;
                gOvlNameCursor--;
                sound = kOvlSoundDelete;
                goto done;
            }
            sound = kOvlSoundDeny;
            goto done;
        }
        if (contrlerPressed((int)p, kPadRight, 0) != 0 || contrlerPressed((int)p, kPadConfirm, 0) != 0) {
            if (gOvlNameCursor < 9) {
                gOvlNameBuf[gOvlNameCursor + 1] = gOvlNameBuf[gOvlNameCursor];
                gOvlNameCursor++;
                sound = kOvlSoundConfirm;
                goto done;
            }
            sound = kOvlSoundDeny;
            goto done;
        }
        if (contrlerPressed((int)p, kPadUp, 0) != 0) {
            if (*at == ' ') {
                *at = 'A';
            } else if (*at == 'Z') {
                *at = 'a';
            } else if (*at == 'z') {
                *at = ' ';
            } else {
                (*at)++;
            }
        } else if (contrlerPressed((int)p, kPadDown, 0) != 0) {
            if (*at == ' ') {
                *at = 'z';
            } else if (*at == 'A') {
                *at = ' ';
            } else if (*at == 'a') {
                *at = 'Z';
            } else {
                (*at)--;
            }
        } else {
            continue;
        }
        sound = kOvlSoundSelect;
    done:
        ovlSound(sound, 0, 0);
        gOvlPulse = 0;
        return 1;
    }
    return 0;
}

BMX_ADDR(0x00418be0)
void ovlScreen17Draw(void)
{
    char buf[64];
    const char *prompt = zstringGet(0x1091);
    int16_t w = zoverTextWidth(prompt, &gOvlFontSmall);
    zoverDrawText(&gOvlFontSmall, prompt, ovlCentreX(0x13f, w), 0x6c, kOvlColorItem, 0);
    zstrFormat(buf, "%s:", zstringGet(0x1092));
    w = zoverTextWidth(buf, &gOvlFontSmall);
    zoverDrawText(&gOvlFontSmall, buf, ovlCentreX(0x13f, w), 0x89, kOvlColorItem, 0);
    const uint32_t len = (uint32_t)zstrlen(gOvlNameBuf);
    uint32_t x = 0x13f - (len * 0x35 >> 1);
    for (uint32_t i = 0; i < len; i++) {
        zstrFormat(buf, "%c", (int)gOvlNameBuf[i]);
        const uint32_t color = i == gOvlNameCursor ? ovlPulseColor() : kOvlColorItem;
        w = zoverTextWidth(buf, &gOvlFontSmall);
        zoverDrawText(&gOvlFontSmall, buf, (int)((x - ((uint16_t)w >> 1)) + 0x1a), 0xe2, color, 0);
        x += 0x35;
    }
}

/* Screen 18 (high scores / run times table, one row per slot): title and the rows. */
BMX_ADDR(0x00418d70)
void ovlScreen18Enter(int create)
{
    char buf[32];
    if (create == 0) {
        for (int slot = 1; slot < 10; slot++) {
            for (int k = 0; k < 4; k++) {
                zoverRemove(gOvlRunTimes[slot][k]);
            }
        }
        return;
    }
    gOvlHandles[0] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet(0x1093), 0x3c, kOvlColorWhite, 0);
    int y = 0x8d;
    for (uint32_t slot = 1; slot < 10; slot++) {
        const uint32_t *record =
            comdataGetLevelRecord((((gameGetMode() << 2 | gameGetSubMode()) << 6) | slot) << 16);
        zstrFormat(buf, "%s:", zstringGet((uint16_t)(slot + 0x1094)));
        gOvlRunTimes[slot][0] = (uint16_t)zoverAddText(1, &gOvlFontTiny, buf, 0x35, y, 0x23040e0, 0);
        const uint8_t *rider = ngifzresGetRiderRecord((uint8_t)record[1] * 5u);
        gOvlRunTimes[slot][3] =
            (uint16_t)zoverAddText(1, &gOvlFontTiny, (const char *)rider + 8, 0xd0, y, 0x2d09010, 0);
        if (slot == 1) {
            const uint32_t mode = gameGetMode();
            if (mode < 6 || mode > 10) {
                zstrFormat(buf, "%ld", (long)record[0]);
            } else {
                ovlFormatFeetInches(buf, (float)record[0]);
            }
        } else if (slot < 4 || slot > 9) {
            zstrFormat(buf, "%ld", (long)record[0]);
        } else {
            ovlFormatFeetInches(buf, (float)record[0]);
        }
        int16_t w = zoverTextWidth(buf, &gOvlFontTiny);
        gOvlRunTimes[slot][1] =
            (uint16_t)zoverAddText(1, &gOvlFontTiny, buf, 0x24a - (uint16_t)w, y, kOvlColorFill, 0);
        zstrFormat(buf, "%s", (const char *)record + 7);
        w = zoverTextWidth(buf, &gOvlFontTiny);
        gOvlRunTimes[slot][2] =
            (uint16_t)zoverAddText(1, &gOvlFontTiny, buf, ovlCentreX(0x1af, w), y, kOvlColorFill, 0);
        y += 0x16;
    }
}

BMX_ADDR(0x00419080)
int ovlScreen18Update(void)
{
    uint32_t next;
    if (ovlPadConfirm(0) == 0) {
        for (uint32_t p = 0; p < gPlayerCount; p++) {
            if ((gOvlSharedPad == 0 || p == gCurrentPlayer) && contrlerPressed((int)p, kPadCancel, 0) != 0) {
                if (gOvlScreenId == 0x12) {
                    ovlSetScreen(1);
                } else if (gOvlScreenId == 0x14) {
                    ovlSetScreen(9);
                } else if (gOvlScreenId == 0x15) {
                    ovlSetScreen(10);
                }
                ovlSound(kOvlSoundBack, 0, 0);
                return 1;
            }
        }
        return 0;
    }
    switch (gOvlScreenId) {
    case 0x12:
        next = 1;
        break;
    case 0x13: {
        const uint32_t mode = gameGetMode();
        if (mode == 0) {
            if (gameGetSubMode() == 0) {
                if (gCurrentLevel > 5) {
                    ovlSetScreen(8);
                    goto confirm;
                }
                if (gameHasPendingUnlock(gCurrentPlayer) != 0) {
                    ovlSetScreen(0x10);
                    goto confirm;
                }
            }
        } else if (mode < 5 || mode > 10) {
            if (gameTestOption(1) == 0) {
                ovlSetScreen(0x19);
                goto confirm;
            }
        } else if (gameTestOption(1) == 0) {
            ovlSetScreen(0);
            gameBigGameFunction(3);
            ovlSound(kOvlSoundConfirm, 0, 0);
            return 1;
        }
        saveFileWrite();
        ovlSetScreen(10);
        gOvlSaving = 1;
        goto confirm;
    }
    case 0x14:
        next = 9;
        break;
    case 0x15:
        next = 10;
        break;
    default:
        goto confirm;
    }
    ovlSetScreen(next);
confirm:
    ovlSound(kOvlSoundConfirm, 0, 0);
    return 1;
}

/* Blinks the name and time of the row the player just set (screens other than 18's first page). */
BMX_ADDR(0x004192b0)
void ovlScreen18Draw(void)
{
    if (gOvlScreenId != 0x12) {
        for (uint32_t slot = 1; slot < 10; slot++) {
            const uint8_t *record =
                gameGetRecord((((gameGetMode() << 2 | gameGetSubMode()) << 6) | slot) << 16);
            if ((record[5] & 1) != 0) {
                for (int k = 1; k < 3; k++) {
                    zoverSetColor(gOvlRunTimes[slot][k], &gOvlFontTiny,
                                  ((gOvlFrame & 1) != 0 ? 0xffef7f80u : 0u) + 0x2d0d0d0, 0);
                }
            }
        }
    }
    ovlDrawMenuItems(8, 0);
}

/* Screen 7 (confirmation): title by run type and the item captions. */
BMX_ADDR(0x00419380)
void ovlScreen7Enter(int create)
{
    if (create == 0) {
        return;
    }
    const uint16_t title = gameGetRunType() == 4 ? 0x109f : 0x109e;
    gOvlHandles[23] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet(title), 0x3c, kOvlColorWhite, 0);
    int y = ovlListTop(gOvlScreen->itemCount, 0x1d);
    for (uint32_t i = 0; i < gOvlScreen->itemCount; i++) {
        gOvlHandles[i] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet((uint16_t)(0x10a0 + i)), y,
                                                 kOvlColorItem, 0);
        y += 0x1d;
    }
}

BMX_ADDR(0x004194c0)
int ovlScreen7Update(void)
{
    for (uint32_t p = 0; p < gPlayerCount; p++) {
        if (gOvlSharedPad != 0 && p != gCurrentPlayer) {
            continue;
        }
        if (contrlerPressed((int)p, kPadConfirm, 0) != 0 || contrlerPressed((int)p, kPadStart, 0) != 0) {
            if (gOvlScreen->selected == 0) {
                goto back;
            }
            if (gOvlScreen->selected != 1) {
                return 1;
            }
            switch (gameGetRunType()) {
            case 1:
            case 3:
            case 5:
            case 6:
                if (gameGetMode() == 0) {
                    ovlSetScreen(0);
                    if (gCurrentLevel < 6) {
                        gameResetGoals(gCurrentPlayer);
                    }
                } else {
                    if (gOvlReturnScreen != 0x16) {
                        for (uint32_t q = 0; q < gPlayerCount; q++) {
                            riderReset((int)q, 0);
                        }
                        cameraSelectPlayer(0);
                        comdataResetView(0, 4);
                        ovlSetScreen(0x16);
                        goto confirm;
                    }
                    ovlSetScreen(0);
                }
                break;
            case 4:
                gameLeaveRun();
                goto confirm;
            default:
                break;
            }
            zsystemSetMenuMode();
            goto confirm;
        }
        if (contrlerPressed((int)p, kPadCancel, 0) != 0) {
            goto back;
        }
    }
    return 0;
back:
    ovlSetScreen(gOvlReturnScreen);
    ovlSound(kOvlSoundBack, 0, 0);
    return 1;
confirm:
    ovlSound(kOvlSoundConfirm, 0, 0);
    return 1;
}

/* Screen 8 update: reward screen of a level; goes on to the unlock screen or saves. */
BMX_ADDR(0x00419ca0)
int ovlScreen8Update(void)
{
    if (ovlPadConfirm(0) == 0) {
        return 0;
    }
    uint8_t character = ovlProfile(gCurrentPlayer)->character;
    if (character > 9) {
        character = 5;
    }
    if (gameTestOption(1) == 0) {
        ovlSetScreen(9);
    } else {
        for (uint32_t k = 0; k < 3; k++) {
            if (gCareerUnlocked[k] == character) {
                ovlSetScreen(0x10);
                ovlSound(kOvlSoundConfirm, 0, 0);
                return 1;
            }
        }
        saveFileWrite();
        ovlSetScreen(10);
        gOvlSaving = 1;
    }
    ovlSound(kOvlSoundConfirm, 0, 0);
    return 1;
}

/* Screen 9 draw: scoreboard header, the run times table and the item captions. */
BMX_ADDR(0x00419d90)
void ovlScreen9Draw(void)
{
    gameGetRunCount();
    overlympDrawHeader(zstringGet(0x10a7));
    int y = 0x6c;
    ovlDrawRunTimes(&y);
    y = gOvlScreen->itemCount * -0x1d + 0x1a4;
    if (gOvlDrawDelay < gOvlFrame) {
        for (uint32_t i = 0; i < gOvlScreen->itemCount; i++) {
            const char *text = zstringGet((uint16_t)(0x10aa + i));
            const uint32_t color = i == gOvlScreen->selected ? ovlPulseColor() : kOvlColorItem;
            const int16_t w = zoverTextWidth(text, &gOvlFontSmall);
            zoverDrawText(&gOvlFontSmall, text, ovlCentreX(0x13f, w), y, color, 0);
            y += 0x1d;
        }
    }
}

/* "xx.y" for a run time byte scaled to 0..100 (the original keeps the int part in a float). */
static void ovlFormatPercent(char *buf, uint8_t value)
{
    const double scaled = (double)value * (double)0x1.919192p-2f;
    const int whole = bmxFtol(scaled);
    const float wholeF = (float)whole;
    const int tenths = bmxFtol((scaled - (double)wholeF) * (double)0x1.4p+3f);
    zstrFormat(buf, "%02d.%d", bmxFtol((double)wholeF), tenths);
}

/* Fade level (0..0xf0) of the cell/row that appears on frame `step * 16`: a ramp over its 16 frames. */
static uint8_t ovlRunTimeFade(uint32_t frame, uint32_t cell)
{
    const uint32_t row = frame >> 4;
    if (row < cell) {
        return 0;
    }
    if (row != cell) {
        return 0xf0;
    }
    const int ramp = bmxFtol((double)(frame & 0xf) * (double)0x1.333334p+0f);
    const uint8_t level = (uint8_t)((ramp > 0xf ? 0xf : ramp) << 4);
    if ((frame & 0xf) == 1) {
        ovlSound(0x1390, 0, 0);
    }
    return level;
}

/* Draws the six time columns of every run of the player's record plus the best-of-runs line. */
BMX_ADDR(0x00419eb0)
void ovlDrawRunTimes(int *y)
{
    char buf[32];
    const char *text = zstringGet(0x10a8);
    int16_t w = zoverTextWidth(text, &gOvlFontTiny);
    zoverDrawText(&gOvlFontTiny, text, 0xf0 - ((uint16_t)w >> 1), (uint16_t)*y, kOvlColorFill, 0);
    text = zstringGet(0x10a9);
    w = zoverTextWidth(text, &gOvlFontTiny);
    zoverDrawText(&gOvlFontTiny, text, 0x222 - ((uint16_t)w >> 1), (uint16_t)*y, kOvlColorFill, 0);
    *y += 0x17;
    if ((int)(gameGetRunCount() + 1) == 0) {
        return;
    }
    const uint8_t *times = gPlayerRecs[gCurrentPlayer] + kOvlPlayerRunTimes;
    uint32_t run = 0;
    do {
        int x = 0x53;
        for (uint32_t k = 0; k < 6; k++) {
            ovlFormatPercent(buf, times[k]);
            uint8_t level;
            if (run == gameGetRunCount()) {
                level = ovlRunTimeFade(gOvlFrame, k);
            } else {
                level = 0xf0;
            }
            const uint32_t c = level;
            const uint32_t color = ((c | 0x300) << 8 | c) << 8 | c;
            w = zoverTextWidth(buf, &gOvlFontTiny);
            zoverDrawText(&gOvlFontTiny, buf, x - ((uint16_t)w >> 1) + 1, (uint16_t)*y, color, 0);
            if (level != 0) {
                zoverDrawRect(x - 0x15, (uint16_t)*y, 0x2b, 0x16, (k < 5 ? 0x309090u : 0u) + 0x903030);
            }
            x += (k < 4 ? 1 : 2) * 0x4a;
        }
        run++;
        times += 0xc;
        *y += 0x17;
    } while (run < gameGetRunCount() + 1U);
    if (run > 1) {
        uint8_t character = ovlProfile(gCurrentPlayer)->character;
        if (character >= 10) {
            character = 5;
        }
        ovlFormatPercent(buf, gCareer[character][2]);
        uint8_t level;
        if ((gOvlFrame & 0xfffffff0) < 0x60) {
            level = 0;
        } else if ((gOvlFrame & 0xfffffff0) == 0x60) {
            const uint32_t low = gOvlFrame & 0xf;
            level = (uint8_t)((low > 0xc ? 0xc : low) << 4);
            if (low == 1) {
                ovlSound(0x1390, 0, 0);
            }
        } else {
            level = 0xc0;
        }
        const uint32_t c = level;
        const uint32_t color = ((c | 0x300) << 8 | c) << 8 | c;
        w = zoverTextWidth(buf, &gOvlFontTiny);
        zoverDrawText(&gOvlFontTiny, buf, 0x210 - ((uint16_t)w >> 1), (uint16_t)*y, color, 0);
        if (level != 0) {
            zoverDrawRect(0x1fa, (uint16_t)*y, 0x2b, 0x16, 0xa07010);
        }
        *y += 0x17;
    }
}

BMX_ADDR(0x0041a360)
int ovlScreen9Update(void)
{
    if (contrlerPressed(gCurrentPlayer, kPadConfirm, 0) == 0 &&
        contrlerPressed(gCurrentPlayer, kPadStart, 0) == 0) {
        return 0;
    }
    switch (gOvlScreen->selected) {
    case 0:
        ovlSetScreen(0);
        gameBigGameFunction(3);
        break;
    case 1:
        ovlSetScreen(0);
        actionSetMode(2);
        gDemoPlayer = gCurrentPlayer;
        break;
    case 2:
        ovlSetScreen(5);
        break;
    case 3:
        ovlSetScreen(0x14);
        break;
    case 4:
        gOvlReturnScreen = 9;
        ovlSetScreen(7);
        gOvlWarnLevel = 0;
        break;
    default:
        break;
    }
    ovlSound(kOvlSoundConfirm, 0, 0);
    return 1;
}

/* Screen 10 (end of run menu): the title depends on the mode, the items on what the rider has unlocked. */
BMX_ADDR(0x0041a480)
void ovlScreen10Enter(int create)
{
    if (create == 0) {
        return;
    }
    uint16_t title = 0x10af;
    if (gameGetMode() != 0) {
        title = gameTestOption(8) == 0 ? 0x3052 : 0x1019;
    }
    gOvlHandles[23] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet(title), 0x3c, kOvlColorWhite, 0);
    gOvlScreen->itemCount = 0;
    if (gameGetMode() == 0) {
        const uint8_t *rider =
            (const uint8_t *)gRiderData + ovlProfile(gCurrentPlayer)->character * kOvlRiderStride;
        const uint32_t flags = *(const uint32_t *)rider;
        ovlAddMenuItem(gOvlMenuItems2, 0, 0x10b0);
        if (gameGetSubMode() == 0) {
            if (1 < rider[4]) {
                ovlAddMenuItem(gOvlMenuItems2, 1, 0x10b2);
            }
            if ((flags & 0x40) != 0) {
                ovlAddMenuItem(gOvlMenuItems2, 2, 0x10b3);
            } else if ((flags & 0x80) != 0) {
                ovlAddMenuItem(gOvlMenuItems2, 3, 0x10b3);
            }
        } else {
            if (1 < rider[4]) {
                ovlAddMenuItem(gOvlMenuItems2, 1, 0x10b2);
            }
            if (rider[6] < 2) {
                if (rider[5] >= 2) {
                    ovlAddMenuItem(gOvlMenuItems2, 3, 0x10b3);
                }
            } else {
                ovlAddMenuItem(gOvlMenuItems2, 2, 0x10b3);
            }
        }
        ovlAddMenuItem(gOvlMenuItems2, 5, 0x10b5);
    } else {
        ovlAddMenuItem(gOvlMenuItems2, 0, 0x10b1);
        if (gameTestOption(8) == 0) {
            ovlAddMenuItem(gOvlMenuItems2, 5, 0x10b5);
        }
    }
    ovlAddMenuItem(gOvlMenuItems2, 6, 0x10b6);
    ovlAddMenuItem(gOvlMenuItems2, 7, 0x10b7);
    gOvlMenuItems2[gOvlScreen->itemCount].action = 8;
    gOvlMenuItems2[gOvlScreen->itemCount].text = gameGetMode() != 0 ? 0x10b9 : 0x10b8;
    gOvlScreen->itemCount++;
}

/* Screen 10 draw: the scores of every player (or the run times of the career) above the item list. */
BMX_ADDR(0x0041a830)
void ovlScreen10Draw(void)
{
    int y;
    uint32_t best;
    char buf[32];
    switch (gameGetMode()) {
    case 2:
        overlympDrawHeader(zstringGet(gameTestOption(8) == 0 ? 0x3052 : 0x1019));
        overlympDrawBody(0x89);
        break;
    case 3:
        overlympDrawHeader(zstringGet(0x3052));
        y = 0x6c;
        ovlDrawPlayerNames(&y);
        break;
    default: {
        const uint8_t *record = gameGetRecord(
            (((gameGetMode() << 2 | gameGetSubMode()) << 6 | gScoreRecordSlot[gameGetMode()]) << 16) |
            0x1000);
        if (record == NULL) {
            zdebugFail(gOvlEmpty, overlayFile, 0x125d, 0);
        }
        best = record[4];
        if (gameGetMode() != 0 && gameTestOption(8) != 0) {
            best = 0xffffffff;
        }
        y = 0x6c;
        const uint8_t *top = gameGetRecord(
            ((gameGetMode() << 2 | gameGetSubMode()) << 6 | gScoreRecordSlot[gameGetMode()]) << 16);
        if (top == NULL) {
            zdebugFail(gOvlEmpty, overlayFile, 0x126a, 0);
        }
        if (gameGetMode() == 0 && gameGetSubMode() == 2) {
            break;
        }
        if (gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel > 5) {
            ovlDrawRunTimes(&y);
            break;
        }
        for (uint32_t i = 0; i < gPlayerCount; i++) {
            if (gameGetMode() == 0) {
                zstrFormat(buf, "%s:", zstringGet(0x1002));
            } else {
                zstrFormat(buf, "%s %s:", zstringGet(0x1000), zstringGet((uint16_t)(0x100c + i)));
            }
            zoverDrawText(&gOvlFontSmall, buf, 0x35, y, kOvlColorItem, 0);
            const uint32_t score = *(const uint32_t *)(gPlayerRecs[i] + kOvlPlayerScore);
            const uint32_t mode = gameGetMode();
            if (mode < 6 || 10 < mode) {
                zstrFormat(buf, "%ld", (long)score);
            } else {
                ovlFormatFeetInches(buf, (float)score);
            }
            uint32_t color = (i != best ? 0xfff03090u : 0u) + 0xe0a040;
            if ((top[5] & 1) != 0 && top[4] == i && (gOvlFrame & 1) != 0) {
                color = 0x2c05050;
            }
            const int16_t w = zoverTextWidth(buf, &gOvlFontSmall);
            zoverDrawText(&gOvlFontSmall, buf, 0x24a - (uint16_t)w, y, color, 0);
            y += 0x1d;
        }
        break;
    }
    }
    y = gOvlScreen->itemCount * -0x1d + 0x1a4;
    if (gOvlDrawDelay < gOvlFrame) {
        for (uint32_t i = 0; i < gOvlScreen->itemCount; i++) {
            const char *text = zstringGet(gOvlMenuItems2[i].text);
            const uint32_t color = i == gOvlScreen->selected ? ovlPulseColor() : kOvlColorItem;
            const int16_t w = zoverTextWidth(text, &gOvlFontSmall);
            zoverDrawText(&gOvlFontSmall, text, ovlCentreX(0x13f, w), y, color, 0);
            y += 0x1d;
        }
    }
}

/* Screen 10 update: the "save replay" shortcut and the item actions. */
BMX_ADDR(0x0041ad00)
int ovlScreen10Update(void)
{
    for (uint32_t p = 0; p < gPlayerCount; p++) {
        if (contrlerPressed(gCurrentPlayer, 0x100, 2) != 0 &&
            contrlerPressed(gCurrentPlayer, kPadStart, 0) != 0) {
            uint8_t *data;
            int size;
            char path[64];
            replayGetBuffer(gameGetModeByte(), &data, &size);
            zstrFormat(path, "s:\\bmx\\assets\\%s\\replay\\demo%02d%02d.rpy", "PC", gCurrentLevel,
                       ovlProfile(gCurrentPlayer)->character);
            void *file = zfileOpen(path, 2, 0);
            if (file != NULL && zfileWrite(file, data, size, 0) == size) {
                ovlSound(kOvlSoundConfirm, 0, 0);
                return 1;
            }
            ovlSound(kOvlSoundDeny, 0, 0);
            return 1;
        }
        if (contrlerPressed((int)p, kPadConfirm, 0) != 0 || contrlerPressed((int)p, kPadStart, 0) != 0) {
            switch (gOvlMenuItems2[gOvlScreen->selected].action) {
            case 0:
                ovlSetScreen(0);
                if (gameGetMode() != 0) {
                    gameSetSubMode((uint32_t)(int32_t)gOvlSavedSubMode);
                }
                gameStartMode((int)gameGetMode());
                gameBigGameFunction(3);
                break;
            case 1:
                gOvlReturnScreen = 10;
                ovlSetScreen(7);
                gOvlWarnLevel = 3;
                gOvlSharedPad = 0;
                break;
            case 2:
                gOvlReturnScreen = 10;
                ovlSetScreen(7);
                gOvlWarnLevel = 1;
                gOvlSharedPad = 0;
                break;
            case 3:
                gOvlReturnScreen = 10;
                ovlSetScreen(7);
                gOvlWarnLevel = 2;
                gOvlSharedPad = 0;
                break;
            case 4:
                ovlSetScreen(0xb);
                break;
            case 5:
                ovlSetScreen(0);
                actionSetMode(2);
                gDemoPlayer = (uint8_t)p;
                break;
            case 6:
                ovlSetScreen(6);
                break;
            case 7:
                ovlSetScreen(0x15);
                break;
            case 8:
                if (gameGetMode() == 0) {
                    gOvlReturnScreen = 10;
                    ovlSetScreen(7);
                    gOvlWarnLevel = 0;
                } else {
                    for (uint32_t q = 0; q < gPlayerCount; q++) {
                        riderReset((int)q, 0);
                    }
                    cameraSelectPlayer(0);
                    comdataResetView(0, 4);
                    ovlSetScreen(0x16);
                }
                gOvlSharedPad = 0;
                break;
            default:
                break;
            }
            ovlSound(kOvlSoundConfirm, 0, 0);
            return 1;
        }
    }
    return 0;
}

/* Save screen (11): plays the saving jingle and builds the first page. */
BMX_ADDR(0x0041b0a0)
void ovlScreen11Enter(int create)
{
    if (create == 0) {
        winchanEnd();
        ovlSound(1000, 2, 0);
        return;
    }
    ovlSound(1000, 3, 0);
    gOvlHandles[0] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, zstringGet(0x110f), 0x3c, kOvlColorWhite, 0);
    gOvlPage = 0;
    ovlScreen11Refresh();
    winchanBegin();
}

/* Records the step reported by the save thread (callback of winchan). */
BMX_ADDR(0x0041bc70)
void ovlSetSaveStep(int a, int step)
{
    gOvlSaveStep = step;
}

BMX_ADDR(0x0041b170)
int ovlScreen11Update(void)
{
    uint32_t next;
    int result;
    const OvlPageDef *def = &gOvlSavePages[gOvlPage];
    for (uint32_t p = 0; p < gPlayerCount; p++) {
        if (gOvlSharedPad != 0 && p != gCurrentPlayer) {
            continue;
        }
        if (contrlerPressed((int)p, kPadConfirm, 0) != 0 && gOvlScreen->selected == 0) {
            switch (gOvlPage) {
            case 0:
                next = 9;
                break;
            case 1:
            case 4:
            case 8:
                next = 0;
                break;
            case 2:
                next = 10;
                break;
            case 3:
                next = 5;
                break;
            case 5:
            case 6:
                next = 0xb;
                break;
            case 7:
                ovlSetScreen(10);
                goto confirm;
            default:
                goto other;
            }
            ovlResetScreenState(next);
            ovlScreen11Refresh();
        confirm:
            ovlSound(kOvlSoundConfirm, 0, 0);
            return 1;
        }
        if (contrlerPressed((int)p, kPadCancel, 0) != 0 ||
            (contrlerPressed((int)p, kPadConfirm, 0) != 0 && gOvlScreen->selected == 1)) {
            switch (gOvlPage) {
            case 0:
                ovlSetScreen(10);
                break;
            case 1:
            case 2:
            case 4:
            case 5:
            case 6:
            case 8:
                ovlResetScreenState(0);
                ovlScreen11Refresh();
                break;
            default:
                goto other;
            }
            ovlSound(kOvlSoundConfirm + (contrlerPressed((int)p, kPadCancel, 0) != 0), 0, 0);
            return 1;
        }
    other:
        if ((contrlerPressed((int)p, kPadLeft, 0) != 0 || contrlerPressed((int)p, kPadRight, 0) != 0) &&
            gOvlPage == 0) {
            gOvlSaveSlot = contrlerPressed((int)p, kPadLeft, 0) == 0;
            ovlScreen11Refresh();
            ovlSound(kOvlSoundSelect, 0, 0);
            gOvlPulse = 0;
            return 1;
        }
        if (def->count > 1 &&
            (contrlerPressed((int)p, kPadDown, 0) != 0 || contrlerPressed((int)p, kPadUp, 0) != 0)) {
            zoverSetColor(gOvlHandles[5 + gOvlScreen->selected], &gOvlFontSmall, kOvlColorItem, 0);
            if (contrlerPressed((int)p, kPadDown, 0) == 0) {
                const uint8_t at = gOvlScreen->selected;
                gOvlScreen->selected = (uint8_t)((at != 0 ? at : def->count) - 1);
            } else if (gOvlScreen->selected < def->count - 1) {
                gOvlScreen->selected++;
            } else {
                gOvlScreen->selected = 0;
            }
            ovlSound(kOvlSoundMove, 0, 0);
            gOvlPulse = 0;
            return 1;
        }
    }
    switch (gOvlPage) {
    case 9:
        if (gOvlFrame == 1) {
            winchanStart(gOvlSaveSlot);
            gOvlSaveStep = -1;
            winchanSetCallback(BMX_FNPTR(WinchanCallback, 0x0041bc70, ovlSetSaveStep));
            return 0;
        }
        if (1 < gOvlFrame) {
            if ((uint32_t)gOvlSaveStep < 4) {
                if (gOvlSaveStep == 3 || gOvlSaveStep == 0) {
                    switch (saveFileClose()) {
                    case 0:
                    case 5:
                        goto ready;
                    case 4:
                        next = 2;
                        goto failed;
                    case 6:
                        next = 6;
                        goto failed;
                    default:
                        goto fallback;
                    }
                }
                if (gOvlSaveStep == 1) {
                    ovlResetScreenState(1);
                    ovlScreen11Refresh();
                    return 0;
                }
                goto fallback;
            }
            if (gOvlSaveStep == 4) {
                ovlResetScreenState(2);
                gOvlScreen->selected = 1;
                ovlScreen11Refresh();
            } else {
                if (gOvlSaveStep != -1) {
                    goto fallback;
                }
                if (0x96 < gOvlFrame) {
                    gOvlSaveStep = 9;
                    return 0;
                }
            }
        }
        return 0;
    case 10:
        if (gOvlFrame != 1) {
            return 0;
        }
        switch (saveFileClose()) {
        case 0:
        case 5:
        ready:
            result = memcardFormat();
            if (result == 0) {
                ovlResetScreenState(5);
                ovlScreen11Refresh();
                return 0;
            }
            if (result == 7) {
                ovlResetScreenState(4);
                ovlScreen11Refresh();
                return 0;
            }
            break;
        case 1:
            goto back1;
        case 3:
            goto back9;
        case 4:
            if (winchanQuery() == 0) {
                ovlResetScreenState(0xb);
                ovlScreen11Refresh();
                return 0;
            }
            break;
        case 6:
            next = 6;
            goto failed;
        default:
            break;
        }
        break;
    case 0xb:
        if (gOvlFrame != 2) {
            return 0;
        }
        if (gCurrentLevel < 6) {
            gameResetGoals(gCurrentPlayer);
        }
        switch (saveFileClose()) {
        case 1:
        back1:
            ovlResetScreenState(1);
            ovlScreen11Refresh();
            return 0;
        case 3:
        back9:
            ovlResetScreenState(9);
            ovlScreen11Refresh();
            return 0;
        case 4:
            next = 2;
        failed:
            ovlResetScreenState(next);
            gOvlScreen->selected = 1;
            ovlScreen11Refresh();
            return 0;
        case 5:
            result = memcardFormat();
            if (result != 0) {
                if (result == 7) {
                    ovlResetScreenState(4);
                    ovlScreen11Refresh();
                    return 0;
                }
                break;
            }
            /* fall through */
        case 0:
        case 6:
            if (saveFileWrite() == 0) {
                ovlResetScreenState(7);
                ovlScreen11Refresh();
                return 0;
            }
            break;
        default:
            break;
        }
        break;
    default:
        return 0;
    }
fallback:
    ovlResetScreenState(8);
    ovlScreen11Refresh();
    return 0;
}

/* Draws the two slot frames of the first page and pulses the selected item. */
BMX_ADDR(0x0041b880)
void ovlScreen11Draw(void)
{
    const OvlPageDef *def = &gOvlSavePages[gOvlPage];
    if (def->count != 0) {
        zoverSetColor(gOvlHandles[5 + gOvlScreen->selected], &gOvlFontSmall, ovlPulseColor(), 0);
    }
    if (gOvlPage == 0) {
        uint32_t anchor = 0x215;
        for (uint32_t slot = 0; slot < 2; slot++, anchor += 0x42a) {
            const uint32_t left = anchor >> 2;
            uint32_t color = gOvlSaveSlot == slot ? ovlPulseColor() : 0x23040e0;
            zoverSetColor(gOvlHandles[5 + def->count + slot], &gOvlFontTiny, color, 0);
            for (uint32_t side = 0; side < 4; side++) {
                color = gOvlSaveSlot == slot ? ovlPulseColor() : kOvlColorItem;
                int x;
                int y;
                if (side == 2) {
                    y = 0x138;
                    x = (int)left - 0x1b;
                } else {
                    y = 0x120;
                    x = side == 3 ? (int)left + 0x83 : (int)left - 0x1b;
                }
                zoverDrawRect(x, y, (side & 1) != 0 ? 2 : 0xa0, (side & 1) != 0 ? 0x18 : 1, color);
            }
        }
    }
}

/* Rebuilds the captions of the save screen's current page and its item list. */
BMX_ADDR(0x0041ba10)
void ovlScreen11Refresh(void)
{
    char buf[64];
    ovlDropHandles(&gOvlHandles[1], &gOvlHandles[24]);
    const OvlPageDef *def = &gOvlSavePages[gOvlPage];
    int y = 0xb6;
    for (uint32_t i = 0; i < 4; i++) {
        zstrFormat(buf, zstringGet((uint16_t)(i + 0x1112 + gOvlPage * 4u)), (gOvlSaveSlot & 0xff) + 1);
        gOvlHandles[1 + i] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, buf, y, kOvlColorItem, 0);
        y += 0x1d;
    }
    y = def->count * -0x1d + 0x1a4;
    for (uint32_t i = 0; i < def->count; i++) {
        const char *text = zstringGet((uint16_t)(def->textBase + i));
        gOvlHandles[5 + i] = (uint16_t)ovlAddCentred(1, &gOvlFontSmall, text, y, kOvlColorItem, 0);
        y += 0x1d;
    }
    if (gOvlPage == 0) {
        uint32_t anchor = 0x215;
        for (uint32_t slot = 0; slot < 2; slot++, anchor += 0x42a) {
            const char *text = zstringGet((uint16_t)(0x1110 + slot));
            const int16_t w = zoverTextWidth(text, &gOvlFontTiny);
            gOvlHandles[5 + def->count + slot] = (uint16_t)zoverAddText(
                1, &gOvlFontTiny, text, (int)((anchor >> 2) - ((uint16_t)w >> 1) + 0x35), 0x121,
                kOvlColorItem, 0);
        }
    }
}

/* Draws the screen's items (Resume / Restart-like list of strings 0x1006..): flags 1 and 2 drop item 1 and 0,
 * 8 shows only the first. */
BMX_ADDR(0x0041e7f0)
void ovlDrawMenuItems(uint32_t flags, int offset)
{
    if (gOvlDrawDelay >= gOvlFrame) {
        return;
    }
    int count;
    if ((flags & 8) == 0) {
        count = (flags & 1) == 0 ? 2 : 3;
        if ((flags & 2) != 0) {
            count--;
        }
    } else {
        count = 1;
    }
    int y = count * -0x1d + 0x1a4;
    int drawn = 0;
    uint32_t i = 0;
    do {
        if (i == 0 ? (flags & 2) == 0 : (i != 1 || (flags & 1) != 0)) {
            const char *text = zstringGet((uint16_t)(i + 0x1006));
            const uint32_t color = (uint32_t)gOvlScreen->selected - (uint32_t)offset == (uint32_t)drawn
                                       ? ovlPulseColor()
                                       : kOvlColorItem;
            const int16_t w = zoverTextWidth(text, &gOvlFontSmall);
            zoverDrawText(&gOvlFontSmall, text, ovlCentreX(0x13f, w), y, color, 0);
            y += 0x1d;
            drawn++;
        }
    } while ((flags & 8) == 0 && ++i < 3);
}

/* True when a pad that may drive the menu confirms (or starts) with item `minItem` or later selected. */
BMX_ADDR(0x0041e920)
int ovlPadConfirm(uint32_t minItem)
{
    for (uint32_t p = 0; p < gPlayerCount; p++) {
        if ((gOvlSharedPad == 0 || p == gCurrentPlayer) &&
            (contrlerPressed((int)p, kPadConfirm, 0) != 0 || contrlerPressed((int)p, kPadStart, 0) != 0) &&
            minItem <= gOvlScreen->selected) {
            return 1;
        }
    }
    return 0;
}

/* Colour of the selected item: a sine pulse of the red/green/blue level around 0x40 (the same in all three).
 */
BMX_ADDR(0x0041e9c0)
uint32_t ovlPulseColor(void)
{
    const double wave = zmathSinTable(gOvlPulse * 0x888u);
    const uint8_t level =
        (uint8_t)(0x40 - (uint8_t)bmxFtol((wave + (double)0x1p+0f) * (double)0x1p-1f * (double)-0x1.7ep+7f));
    return 0x2000000u | (uint32_t)level << 16 | (uint32_t)level << 8 | level;
}

/* Formats a length in feet as feet and inches (rounded). */
BMX_ADDR(0x0041ea40)
void ovlFormatFeetInches(char *dst, float feet)
{
    const uint32_t inches = (uint32_t)bmxFtol((double)feet * (double)0x1.93264cp-2f + (double)0x1p-1f);
    zstrFormat(dst, "%0ld' %ld\"", (long)(inches / 12), (long)(inches % 12));
}

/* Appends `word` to `buf`, separated by a space unless the buffer is still empty. */
static void ovlAppendWord(char *buf, const char *word)
{
    const size_t len = strlen(buf);
    zstrFormat(buf + len, "%s%s", len == 0 ? (const char *)gOvlEmpty : " ", word);
}

/* Spin part of a trick name: "half-cab" and the 180/360/... spin, then the "opposite" and "alley-oop"
 * prefixes. */
static void ovlAppendSpin(char *buf, const uint8_t *trick, int withExtras)
{
    if (trick[4] != 0) {
        if (withExtras == 0 || (trick[0] & 4) == 0) {
            ovlAppendWord(buf, gTrickSpinNames[trick[4]]);
        } else {
            ovlAppendWord(buf, "half-cab");
            if (trick[4] >= 2) {
                ovlAppendWord(buf, gTrickSpinNames[trick[4]]);
            }
        }
    }
    if (withExtras != 0) {
        if ((trick[0] & 1) != 0) {
            ovlAppendWord(buf, "opposite");
        }
        if ((trick[0] & 2) != 0) {
            ovlAppendWord(buf, "alley-oop");
        }
    }
}

/* Barspin part of a trick name: the "double"/"triple"/... multiplier and the barspin kind. */
static void ovlAppendBarspin(char *buf, const uint8_t *trick)
{
    if (trick[3] > 1) {
        const int multiplier = trick[3] - 2;
        ovlAppendWord(buf, gTrickDoubleNames[multiplier < 4 ? multiplier : 3]);
    }
    ovlAppendWord(buf, gTrickBarspinNames[trick[2]]);
}

/*
 * TODO(file): probably belongs to a trick naming module; it is shared with overlymp.c and 0x41c590.
 *
 * Builds the display name of a trick record {flags, turn, barspin, multiplier, spin} into `out`;
 * withExtras adds the half-cab / opposite / alley-oop prefixes. Combos that have their own names
 * ("flair", "limo", "truck driver", ...) rebuild the text. Leaves `out` untouched ("" at least) for
 * the combinations the switch below does not handle.
 */
BMX_ADDR(0x0041eab0)
void ovlFormatTrickName(char *out, const uint8_t *trick, int withExtras)
{
    const char *last;
    out[0] = '\0';
    ovlAppendSpin(out, trick, withExtras);
    if (trick[2] != 0xff) {
        ovlAppendBarspin(out, trick);
    }
    if (trick[1] == 0xff) {
        if (trick[4] != 0 && trick[2] == 0xff) {
            zstrFormat(out + strlen(out), " %s", "spin");
        }
    } else {
        ovlAppendWord(out, gTrickTurnNames[trick[1]]);
    }
    switch (trick[1]) {
    case 1:
        if (trick[2] != 1) {
            return;
        }
        out[0] = '\0';
        ovlAppendSpin(out, trick, withExtras);
        ovlAppendWord(out, gTrickBarspinNames[7]);
        last = gTrickTurnNames[trick[1]];
        break;
    case 3:
        if (trick[4] == 1) {
            out[0] = '\0';
            if (trick[2] != 0xff) {
                ovlAppendBarspin(out, trick);
            }
            last = "flair";
        } else {
            if (trick[2] != 0) {
                return;
            }
            out[0] = '\0';
            switch (trick[3]) {
            case 0:
            case 1:
                last = "flip";
                break;
            case 2:
                last = "limo";
                break;
            case 3:
                last = "triple flip";
                break;
            default:
                last = "vette";
                break;
            }
            ovlAppendWord(out, last);
            last = " driver";
        }
        break;
    case 4:
        if (trick[2] != 6) {
            return;
        }
        out[0] = '\0';
        ovlAppendSpin(out, trick, withExtras);
        ovlAppendWord(out, gTrickTurnNames[trick[1]]);
        last = gTrickBarspinNames[trick[2]];
        break;
    case 10:
        if (trick[4] != 2) {
            return;
        }
        out[0] = '\0';
        if (trick[2] != 0xff) {
            ovlAppendBarspin(out, trick);
        }
        last = "flail";
        break;
    case 0xff:
        if (trick[2] != 0 || trick[4] < 2) {
            return;
        }
        out[0] = '\0';
        if (withExtras != 0) {
            if ((trick[0] & 4) != 0) {
                ovlAppendWord(out, "half-cab");
            }
            if ((trick[0] & 1) != 0) {
                ovlAppendWord(out, "opposite");
            }
            if ((trick[0] & 2) != 0) {
                ovlAppendWord(out, "alley-oop");
            }
        }
        if (trick[4] != 2) {
            ovlAppendWord(out, gTrickSpinNames[trick[4]]);
        }
        if (trick[3] > 1) {
            const int multiplier = trick[3] - 2;
            ovlAppendWord(out, gTrickDoubleNames[multiplier < 4 ? multiplier : 3]);
        }
        last = "truck driver";
        break;
    default:
        return;
    }
    ovlAppendWord(out, last);
}

/* ---- HUD elements ------------------------------------------------------------------------------- */

/* Element 0: the letterbox bars above and below the picture. */
BMX_ADDR(0x0041bc90)
void ovlLetterboxDraw(void)
{
    ngifFillScreenBar(0, 0, 0, 0x200, gOvlBarSize);
    ngifFillScreenBar(0, 0, 0x1a4, 0x200, gOvlBarSize);
}

/* Element 16: a full screen colour fade that steps r, g and b towards the target every frame. */
BMX_ADDR(0x0041bce0)
void ovlFadeDraw(void)
{
    if (gOvlFade.ticks != 0) {
        gOvlFade.r = (float)((double)gOvlFade.r + (double)gOvlFade.dr);
        const uint32_t mode = (uint32_t)(~gOvlFade.flags & 2);
        gOvlFade.g = (float)((double)gOvlFade.g + (double)gOvlFade.dg);
        gOvlFade.b = (float)((double)gOvlFade.b + (double)gOvlFade.db);
        uint32_t color = (mode | 4) << 7;
        color |= (uint32_t)bmxFtol(gOvlFade.r);
        color <<= 8;
        color |= (uint32_t)bmxFtol(gOvlFade.g);
        color <<= 8;
        color |= (uint32_t)bmxFtol(gOvlFade.b);
        ngifFillScreenBar(color, 0, 0, 0x200, gOvlFadeSize);
        gOvlFade.ticks--;
    }
}

/* Starts a fade from {flags, r, g, b, ticks(u16)}: bit 0 fades out from the colour, otherwise in from black.
 */
BMX_ADDR(0x0041bdc0)
void ovlFadeSet(const uint8_t *fade)
{
    const uint16_t ticks = (uint16_t)(fade[4] | fade[5] << 8);
    gOvlFade.ticks = ticks;
    if (ticks == 0) {
        return;
    }
    const double count = (double)ticks;
    gOvlFade.dr = (float)((double)fade[1] / count);
    gOvlFade.dg = (float)((double)fade[2] / count);
    gOvlFade.db = (float)((double)fade[3] / count);
    gOvlFade.flags = fade[0];
    if ((gOvlFade.flags & 1) != 0) {
        gOvlFade.r = (float)fade[1];
        gOvlFade.g = (float)fade[2];
        gOvlFade.b = (float)fade[3];
        gOvlFade.dr = -gOvlFade.dr;
        gOvlFade.dg = -gOvlFade.dg;
        gOvlFade.db = -gOvlFade.db;
    } else {
        gOvlFade.r = 0.0f;
        gOvlFade.g = 0.0f;
        gOvlFade.b = 0.0f;
    }
}

/* Element 1 (score): creates or removes the right aligned score text. */
BMX_ADDR(0x0041bf00)
void ovlScoreInit(int create)
{
    if (create == 0) {
        zoverRemove(gOvlScoreHandle);
        gOvlScoreHandle = kOvlNoHandle;
        return;
    }
    char buf[16];
    zstrFormat(buf, "%ul", gOvlScoreShown);
    const int16_t w = zoverTextWidth(buf, &gOvlFontSmall);
    const uint32_t handle = zoverAddText(0, &gOvlFontSmall, buf, 0x24b - (uint16_t)w, 400, kOvlColorLabel, 0);
    gOvlScoreHandle = (uint16_t)handle;
    zoverShow(handle, 0);
}

/* Counts the displayed score up towards the target (a tenth of the gap plus 10 per frame). */
BMX_ADDR(0x0041bfc0)
void ovlScoreDraw(void)
{
    char buf[16];
    const uint32_t mode = gameGetMode();
    switch (mode) {
    case 3:
        return;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        if ((gOvlScoreFlags & 1) == 0 && gOvlScoreTarget == gOvlScoreShown) {
            return;
        }
        gOvlScoreShown = gOvlScoreTarget;
        ovlFormatFeetInches(buf, (float)gOvlScoreTarget);
        break;
    default:
        if (mode == 0 && gameGetSubMode() == 2) {
            return;
        }
        if ((gOvlScoreFlags & 1) == 0) {
            const uint32_t shown = gOvlScoreShown;
            const uint32_t target = gOvlScoreTarget;
            if (target <= shown) {
                return;
            }
            const int step = bmxFtol((double)(target - shown) * (double)-0x1.99999ap-4f);
            gOvlScoreShown = shown - (uint32_t)step + 10;
            if (target <= gOvlScoreShown) {
                gOvlScoreShown = target;
            }
        } else {
            gOvlScoreFlags &= 0xfe;
            gOvlScoreShown = gOvlScoreTarget;
        }
        zstrFormat(buf, "%ld", (long)gOvlScoreShown);
        break;
    }
    zoverRemove(gOvlScoreHandle);
    const int16_t w = zoverTextWidth(buf, &gOvlFontSmall);
    gOvlScoreHandle =
        (uint16_t)zoverAddText(0, &gOvlFontSmall, buf, 0x24b - (uint16_t)w, 400, kOvlColorLabel, 0);
}

BMX_ADDR(0x0041c190)
void ovlScoreShow(int show)
{
    const uint32_t mode = gameGetMode();
    if (mode == 0) {
        if (gameGetSubMode() == 2) {
            zoverShow(gOvlScoreHandle, 0);
            return;
        }
    } else if (mode == 3) {
        zoverShow(gOvlScoreHandle, 0);
        return;
    }
    zoverShow(gOvlScoreHandle, show);
}

/* Sets the score target {u32 score; u8 flags} (flag 1: jump to it without counting). */
BMX_ADDR(0x0041c210)
void ovlScoreSet(const uint32_t *value)
{
    gOvlScoreTarget = value[0];
    gOvlScoreFlags |= *(const uint8_t *)&value[1];
    uint32_t visible = ~gOvlElements[1].flags & 1;
    const uint32_t mode = gameGetMode();
    if (mode == 0) {
        if (gameGetSubMode() == 2) {
            visible = 0;
        }
    } else if (mode == 3) {
        return;
    }
    zoverShow(gOvlScoreHandle, (int)visible);
}

/* Element 2 (timer): creates or removes the M:SS text at the left. */
BMX_ADDR(0x0041c290)
void ovlTimerInit(int create)
{
    gOvlTimerText[0] = '\0';
    if (create == 0) {
        zoverRemove(gOvlTimerHandle);
        gOvlTimerHandle = kOvlNoHandle;
        return;
    }
    const uint32_t handle = zoverAddText(0, &gOvlFontSmall, gOvlTimerText, 0x35, 400, kOvlColorLabel, 0);
    gOvlTimerHandle = (uint16_t)handle;
    zoverShow(handle, 0);
}

/* Redraws the timer text (M, a blinking colon and SS from the frame count) when it changed. */
BMX_ADDR(0x0041c310)
void ovlTimerDraw(void)
{
    char buf[16];
    if (gameGetMode() == 0 && gameGetSubMode() == 2) {
        return;
    }
    uint32_t minutes = 0;
    uint32_t seconds = 0;
    uint32_t left = gOvlTimerTicks + 0x1d;
    if (0x708 < left) {
        minutes = (gOvlTimerTicks + 0x1c) / 0x708;
        left -= minutes * 0x708;
    }
    if (0x1e < left) {
        seconds = (left - 1) / 0x1e;
        left -= seconds * 0x1e;
    }
    zstrFormat(buf, "%1ld%c%02ld", (long)minutes, left < 0xf ? ' ' : ':', (long)seconds);
    if (strcmp(buf, gOvlTimerText) != 0) {
        zoverRemove(gOvlTimerHandle);
        gOvlTimerHandle = (uint16_t)zoverAddText(0, &gOvlFontSmall, buf, 0x35, 400,
                                                 (0x97 < gOvlTimerTicks ? 0x108080u : 0u) + 0xc05050, 0);
        gOvlTimerShownTicks = gOvlTimerTicks;
        memcpy(gOvlTimerText, buf, strlen(buf) + 1);
    }
}

BMX_ADDR(0x0041c4a0)
void ovlTimerShow(int show)
{
    if (gameGetMode() == 0 && gameGetSubMode() == 2) {
        return;
    }
    zoverShow(gOvlTimerHandle, show);
}

BMX_ADDR(0x0041c4e0)
void ovlTimerSet(const uint32_t *ticks)
{
    gOvlTimerTicks = *ticks;
}

/* Element 3: the current player's name typed with the button glyph font, top left. */
BMX_ADDR(0x0041c500)
void ovlNameDraw(void)
{
    char name[0x102];
    const uint8_t *rec = gPlayerRecs[gCurrentPlayer];
    uint32_t n = 0;
    if (rec[kOvlPlayerNameLen] != 0) {
        do {
            name[n] = (char)gButtonGlyphs[rec[kOvlPlayerName + n]];
            n++;
        } while (n < rec[kOvlPlayerNameLen]);
    }
    name[n] = '\0';
    zoverDrawText(&gOvlFontButton, name, 0x35, 0x3c, kOvlColorLabel, 0);
}

/* Fields of the player record that the combo list reads. */
enum {
    kOvlPlayerComboScore = 0x64,
    kOvlPlayerCombo = 0x6c,      /* RiderTrick[], 16 bytes each; the u16 at +8 of an entry is its age */
    kOvlPlayerComboCount = 0xed, /* entries in use */
};

/* Position a combo line has scrolled to after `ticks` frames: the cosine eases it up from the bottom. */
static uint32_t ovlComboAge(uint16_t ticks)
{
    const uint32_t v = ticks * 0x16cu;
    return v > 0x4000 ? 0x4000 : v;
}

/*
 * Element 4 (combo list): the tricks of the player's combo scroll up the screen, one line each with a "TO"
 * between them; names fade in at the bottom and out at the top. Lines that are too wide are split at the
 * first blank after the middle. The score is shown below when the mode has one.
 */
BMX_ADDR(0x0041c590)
void ovlComboDraw(void)
{
    const uint8_t *rec = (const uint8_t *)&gPlayerRecs + gCurrentPlayer * kOvlPlayerStride;
    const RiderTrick *tricks = (const RiderTrick *)(rec + kOvlPlayerCombo);
    char buf[256];
    int32_t y = 0;

    for (int i = 0; i < rec[kOvlPlayerComboCount]; i++) {
        const uint32_t age = ovlComboAge(tricks[i].ticks);
        char *second;
        uint32_t color;
        uint32_t flags;
        uint32_t level;
        int16_t pos;
        int16_t w;

        if (i > 0) {
            const int32_t rise = bmxFtol((double)0x1.9p+7f - zmathCos((ZAngle)age) * (double)0x1.9p+7f);
            const int32_t step = 0x168 - rise - (int16_t)y;

            y += bmxFtol((double)step * (double)0x1.570a3ep-1f);
            pos = (int16_t)y;
            if (pos >= 0xf0) {
                if (pos < 0x110) {
                    const int32_t d = pos - 0xf0;
                    level = (d & ~1) >= 0x18 ? 0xc : (uint32_t)(d >> 1);
                    color = ((level * 0x50 | 0x20000000) >> 4) | level * 0xb00 | level * 0xd0000;
                } else {
                    color = 0xd0b050;
                }
                zstrFormat(buf, "TO");
                w = zoverTextWidth(buf, &gOvlFontSmall);
                zoverDrawText(&gOvlFontSmall, buf, ovlCentreX(0x140, w), y, color, 5);
            }
        }

        const int32_t rise = bmxFtol(zmathCos((ZAngle)age) * (double)-0x1.9p+7f + (double)0x1.9p+7f);
        y = 0x168 - rise;
        pos = (int16_t)y;
        if (pos < 0xd2) {
            continue;
        }
        ovlFormatTrickName(buf, &tricks[i].flags, 1);
        if (pos < 0xf2) {
            const int32_t d = pos - 0xd2;
            color = 0x2000000;
            level = (d & ~3) >= 0x28 ? 10 : (uint32_t)(d >> 2);
        } else {
            color = 0;
            level = 0xc;
        }
        if (tricks[i].trick == 0xff || tricks[i].style == 0xff) {
            flags = 0;
            color |= ((level << 8 | level) << 8 | level) << 4;
        } else {
            const int32_t scaled = bmxFtol((double)level * (double)0x1.8p+0f);
            const uint32_t bright = scaled < 0xf ? (uint8_t)scaled : 0xf;

            zstrupr(buf);
            flags = 3;
            color |= (level << 6 >> 4) | bright << 10 | bright * 0xd0000;
        }
        second = NULL;
        w = (int16_t)(zoverTextWidth(buf, &gOvlFontSmall) & 0xffff);
        if ((uint16_t)w < 0x1e0) {
            y -= 0xe;
        } else {
            char *p = buf + (strlen(buf) >> 1);

            while (*p != '\0') {
                if (*p == ' ') {
                    *p++ = '\0';
                    break;
                }
                p++;
            }
            second = p;
            w = (int16_t)(zoverTextWidth(buf, &gOvlFontSmall) & 0xffff);
        }
        zoverDrawText(&gOvlFontSmall, buf, 0x140 - (int)((uint16_t)w >> 1), y, color, (int)flags);
        if (second != NULL) {
            y += 0x1d;
            w = zoverTextWidth(second, &gOvlFontSmall);
            zoverDrawText(&gOvlFontSmall, second, ovlCentreX(0x140, w), y, color, (int)flags);
        }
    }
    if (rec[kOvlPlayerComboCount] != 0) {
        const uint32_t mode = gameGetMode();

        if (mode == 0) {
            if (gameGetSubMode() == 2) {
                return;
            }
        } else if (mode == 3 || (mode >= 5 && mode <= 10)) {
            return;
        }
        zstrFormat(buf, "%ld", (long)*(const uint32_t *)(rec + kOvlPlayerComboScore));
        const int16_t w = zoverTextWidth(buf, &gOvlFontSmall);
        zoverDrawText(&gOvlFontSmall, buf, ovlCentreX(0x140, w), 400, kOvlColorLabel, 0);
    }
}

/* Element 5 (height meter): the frame (four bars) and the marker. */
BMX_ADDR(0x0041cae0)
void ovlHeightInit(int create)
{
    if (create == 0) {
        ovlDropHandles(gOvlHeightHandles, gOvlHeightHandles + 6);
        return;
    }
    for (int i = 0; i < 6; i++) {
        gOvlHeightHandles[i] = kOvlNoHandle;
    }
    uint32_t handle = zoverAddRect(0, 0x224, 0x51, 0x26, 0x12a, 0x1b0b040);
    gOvlHeightHandles[0] = (uint16_t)handle;
    zoverShow(handle, 0);
    for (uint32_t i = 0; i < 4; i++) {
        handle = zoverAddRect(0, i != 1 ? 0x223 : 0x24a, i != 3 ? 0x50 : 0x17b, (i & 2) != 0 ? 0x28 : 1,
                              (i & 2) != 0 ? 1 : 300, 0x1000000);
        gOvlHeightHandles[1 + i] = (uint16_t)handle;
        zoverShow(handle, 0);
    }
    gOvlHeightHandles[5] = (uint16_t)zoverAddRect(0, 0x213, 0x17a, 0x10, 2, 0xffffff);
    gOvlHeightValue = 0.0f;
    gOvlHeightScroll = 0.0f;
}

/* Draws the height scale: a tick every 5 ft (a number every 25 ft) scrolling with the rider's height. */
BMX_ADDR(0x0041cc60)
void ovlHeightDraw(void)
{
    char text[32];
    if (!(gOvlHeightValue < 0.0f || gOvlHeightValue > 0.0f)) {
        return;
    }
    float tickY = 379.0f;
    const double whole = (double)bmxFtol((double)gOvlHeightScroll);
    uint32_t feet = (uint32_t)bmxFtol(whole);
    const double frac = (double)gOvlHeightScroll - whole;
    if (frac < 0.0 || frac > 0.0) { /* an ordered, non-zero fraction (a NaN skips it, like the FCOM test) */
        tickY = (float)(379.0 - ((double)1.0f - frac) * (double)0x1.de6666p+3f);
        feet++;
    }
    uint32_t phase = feet % 5;
    if (phase != 0) {
        phase = 5 - phase;
    }
    for (int row = 0; row < 20; row++) {
        const uint16_t y = (uint16_t)bmxFtol((double)tickY);
        zoverDrawRect(0x224, (int16_t)y, phase != 0 ? 0x13 : 0x26, 1,
                      (phase != 0 ? 0x202020u : 0u) + 0x1202020);
        if (phase == 0 && bmxFtol((double)tickY) < 0x17a - gOvlFontTiny.height) {
            zstrFormat(text, "%ldft", (long)feet);
            zoverDrawText(&gOvlFontTiny, feet >= 100 ? text + 1 : text, 0x226, (int16_t)(y + 2),
                          (feet < 100 ? 0xcfd0d1u : 0u) + 0x300ffff, 0);
        }
        tickY = (float)((double)tickY - (double)0x1.de6666p+3f);
        feet++;
        phase = phase == 0 ? 4 : phase - 1;
    }
    if (phase == 0 && bmxFtol((double)tickY) > 0x4e) {
        zstrFormat(text, "%ldft", (long)feet);
        zoverDrawText(&gOvlFontTiny, feet >= 100 ? text + 1 : text, 0x226, bmxFtol((double)tickY) + 2,
                      (feet < 100 ? 0xcfd0d1u : 0u) + 0x300ffff, 0);
    }
    const uint8_t *rec = gameGetRecord(((gameGetMode() << 2 | gameGetSubMode()) << 6 | gOvlHeightKind) << 16);
    if (rec == NULL) {
        zdebugFail("invalid record type in display setup data", overlayFile, 0x1897, 0);
    }
    const float goal = (float)((double)*(const uint32_t *)rec * (double)0x1.0cc434p-5f);
    if ((double)gOvlHeightScroll + (double)0x1.4p+4f > (double)goal &&
        (double)goal >= (double)gOvlHeightScroll) {
        zoverDrawRect(0x224,
                      0x17b - bmxFtol(((double)goal - (double)gOvlHeightScroll) * (double)0x1.de6666p+3f),
                      0x26, 2, 0x703020);
    }
}

BMX_ADDR(0x0041cfa0)
void ovlHeightShow(int show)
{
    for (int i = 0; i < 6; i++) {
        zoverShow(gOvlHeightHandles[i], show != 0 && (gOvlHeightValue < 0.0f || gOvlHeightValue > 0.0f));
    }
}

/*
 * Sets {float height; float; u32 packed}: 0 hides the element, otherwise scrolls the scale and places
 * the marker. The u16 at +10 selects the record that holds the goal height.
 */
BMX_ADDR(0x0041d010)
void ovlHeightSet(const float *value)
{
    if (!(value[0] < gOvlHeightValue || value[0] > gOvlHeightValue)) {
        return;
    }
    if (!(value[0] < 0.0f || value[0] > 0.0f)) {
        for (int i = 0; i < 6; i++) {
            zoverShow(gOvlHeightHandles[i], 0);
        }
        gOvlHeightScroll = 0.0f;
        gOvlHeightValue = value[0];
        gOvlHeightTarget = value[1];
        memcpy(&gOvlHeightExtra, &value[2], 4);
        return;
    }
    if (!(gOvlHeightValue < 0.0f || gOvlHeightValue > 0.0f) && (gOvlElements[5].flags & 1) == 0) {
        for (int i = 0; i < 6; i++) {
            zoverShow(gOvlHeightHandles[i], 1);
        }
    }
    gOvlHeightValue = value[0];
    const double scaled = (double)gOvlHeightValue * (double)0x1.0cc434p-5f;
    gOvlHeightTarget = (float)scaled;
    memcpy(&gOvlHeightExtra, &value[2], 4);
    if (scaled < (double)0x1.ep+3f || scaled != scaled) {
        gOvlHeightScroll = 0.0f;
    } else {
        gOvlHeightScroll = (float)((double)gOvlHeightTarget - (double)0x1.ep+3f);
    }
    zoverSetRect(gOvlHeightHandles[5], 0x213,
                 0x17b -
                     bmxFtol(((double)gOvlHeightTarget - (double)gOvlHeightScroll) * (double)0x1.de6666p+3f),
                 0x10, 2);
    gOvlHeightKind = *(const uint16_t *)((const uint8_t *)value + 10);
}

/* Element 6 (balance meter): two gradient bars, a frame and the marker. */
BMX_ADDR(0x0041d1c0)
void ovlBalanceInit(int create)
{
    if (create == 0) {
        ovlDropHandles(gOvlBalanceHandles, gOvlBalanceHandles + 7);
        return;
    }
    for (int i = 0; i < 7; i++) {
        gOvlBalanceHandles[i] = kOvlNoHandle;
    }
    for (uint32_t i = 0; i < 2; i++) {
        const uint32_t handle =
            zoverAddGradient(0, 0xd6, (int)((i * 0x9e >> 1) + 0xa1), 0x1c, 0x4f,
                             (i != 0 ? 0xff709000u : 0u) + 0x1b02010, (i != 0 ? 0x8f7000u : 0u) + 0x120b010);
        gOvlBalanceHandles[i] = (uint16_t)handle;
        zoverShow(handle, 0);
    }
    for (uint32_t i = 0; i < 4; i++) {
        const uint32_t handle = zoverAddRect(0, i != 1 ? 0xd5 : 0xf2, i != 3 ? 0xa0 : 0x13f,
                                             (i & 2) != 0 ? 0x1e : 1, (i & 2) != 0 ? 1 : 0xa0, 0x1000000);
        gOvlBalanceHandles[2 + i] = (uint16_t)handle;
        zoverShow(handle, 0);
    }
    gOvlBalanceHandles[6] = (uint16_t)zoverAddRect(0, 0xd6, 0xee, 0x1c, 2, 0xffffff);
    gOvlBalanceValue = 0.0f;
}

BMX_ADDR(0x0041d380)
void ovlBalanceDraw(void) {}

BMX_ADDR(0x0041d3a0)
void ovlBalanceShow(int show)
{
    for (int i = 0; i < 7; i++) {
        zoverShow(gOvlBalanceHandles[i], show != 0 && (gOvlBalanceValue < 0.0f || gOvlBalanceValue > 0.0f));
    }
}

/* Sets the balance in -1..1 (0 hides the meter) and moves the marker up to 80 pixels either way. */
BMX_ADDR(0x0041d410)
void ovlBalanceSet(const float *value)
{
    if (!(value[0] < gOvlBalanceValue || value[0] > gOvlBalanceValue)) {
        return;
    }
    if (!(value[0] < 0.0f || value[0] > 0.0f)) {
        for (int i = 0; i < 7; i++) {
            zoverShow(gOvlBalanceHandles[i], 0);
        }
        gOvlBalanceValue = value[0];
        return;
    }
    if (!(gOvlBalanceValue < 0.0f || gOvlBalanceValue > 0.0f) && (gOvlElements[6].flags & 1) == 0) {
        for (int i = 0; i < 7; i++) {
            zoverShow(gOvlBalanceHandles[i], 1);
        }
    }
    gOvlBalanceValue = value[0];
    int offset = bmxFtol((double)gOvlBalanceValue * (double)0x1.6924p+6f);
    if (offset > 0x50) {
        offset = 0x50;
    } else if (offset < -0x50) {
        offset = -0x50;
    }
    zoverSetRect(gOvlBalanceHandles[6], 0xd6, offset + 0xee, 0x1c, 2);
}

/* Element 7 (caption): a text centred at the bottom that follows gOvlCaptionWant. */
BMX_ADDR(0x0041d530)
void ovlCaptionInit(int create)
{
    if (create != 0) {
        gOvlCaptionHandle = (uint16_t)zoverAddText(0, &gOvlFontSmall, gOvlEmpty, 0x140, 0x132, 0xc09020, 0);
        gOvlTimerShownTicks = 0xff;
        return;
    }
    zoverRemove(gOvlCaptionHandle);
    gOvlCaptionHandle = kOvlNoHandle;
    gOvlTimerShownTicks = 0xff;
}

BMX_ADDR(0x0041d5c0)
void ovlCaptionDraw(void)
{
    if (gOvlCaptionCurrent != gOvlCaptionWanted) {
        gOvlCaptionCurrent = gOvlCaptionWanted;
        if (gOvlCaptionWanted != 0xff) {
            const char *text = zstringGet((uint16_t)(gOvlCaptionWanted + 0x1056));
            zoverRemove(gOvlCaptionHandle);
            const int16_t w = zoverTextWidth(text, &gOvlFontSmall);
            gOvlCaptionHandle =
                (uint16_t)zoverAddText(0, &gOvlFontSmall, text, ovlCentreX(0x140, w), 0x132, 0xc09020, 0);
            return;
        }
        zoverShow(gOvlCaptionHandle, 0);
    }
}

BMX_ADDR(0x0041d680)
void ovlCaptionShow(int show)
{
    if (show != 0 && gOvlCaptionCurrent != 0xff) {
        zoverShow(gOvlCaptionHandle, 1);
        return;
    }
    zoverShow(gOvlCaptionHandle, 0);
}

BMX_ADDR(0x0041d6d0)
void ovlCaptionSet(const uint8_t *id)
{
    gOvlCaptionWanted = *id;
}

/* Elements 8 and 9 have no overlays of their own. */
BMX_ADDR(0x0041d6f0)
void ovlElement8Init(int create) {}

BMX_ADDR(0x0041d710)
void ovlElement8Draw(void) {}

BMX_ADDR(0x0041d730)
void ovlElement8Show(int show) {}

BMX_ADDR(0x0041d750)
void ovlElement8Set(const uint8_t *value)
{
    gOvlElement8Value = *value;
}

BMX_ADDR(0x0041d770)
void ovlIndicatorInit(int create)
{
    gOvlIndicator.id = 0;
}

/* Element 9: a one letter marker (R, P, p) drawn at the top right while the counter's bit 4 is clear. */
BMX_ADDR(0x0041d790)
void ovlIndicatorDraw(void)
{
    const char *text = gIndicatorText[gOvlIndicator.id];
    if (*text != '\0' && (gOvlIndicator.counter & 0x10) == 0) {
        const int16_t w = zoverTextWidth(text, &gOvlFontSmall);
        zoverDrawText(&gOvlFontIcons, text, 0x240 - ((uint16_t)w >> 1), 0x30,
                      gIndicatorColor[gOvlIndicator.id], 0);
    }
    gOvlIndicator.counter++;
}

BMX_ADDR(0x0041d810)
void ovlIndicatorShow(int show) {}

BMX_ADDR(0x0041d830)
void ovlIndicatorSet(const uint32_t *value)
{
    *(uint32_t *)&gOvlIndicator = *value & 0xffff;
}

/* Element 10 (hint): up to three lines of text with button glyphs, fading in and out over 90 frames. */
BMX_ADDR(0x0041d860)
void ovlHintInit(int create)
{
    if (create != 0) {
        gOvlHint.timer = 0;
        for (int i = 0; i < 7; i++) {
            gOvlHintHandles[i] = kOvlNoHandle;
        }
        return;
    }
    ovlDropHandles(gOvlHintHandles, gOvlHintHandles + 7);
    gOvlHint.timer = 0;
}

/* A fade level (0..10, any int in the original's byte arithmetic) as the colour level in steps of 16. */
static uint8_t ovlShade(int level)
{
    return (uint8_t)((uint32_t)level << 4);
}

/* Fade level of a message timer: ramps up while the timer runs through the fade-in and fade-out windows. */
static int ovlFadeStep(int steps)
{
    int level = bmxFtol((double)steps * (double)0x1.333334p+0f);
    return level > 10 ? 10 : level;
}

BMX_ADDR(0x0041d8e0)
void ovlHintDraw(void)
{
    if (gOvlHint.timer == 0) {
        return;
    }
    gOvlHint.timer--;
    if (gOvlHint.timer == 0) {
        ovlDropHandles(gOvlHintHandles, gOvlHintHandles + 7);
        return;
    }
    int level;
    if (gOvlHint.timer >= 0x3c) {
        level = ovlFadeStep((int16_t)(0x5a - gOvlHint.timer));
    } else {
        if (gOvlHint.timer < 0x1e) {
            return;
        }
        level = ovlFadeStep((int16_t)(gOvlHint.timer - 0x1e));
    }
    const uint8_t shade = ovlShade(level);
    if (shade == 0xff) {
        return;
    }
    const uint32_t s = shade;
    const uint32_t plain = ((s | 0x200) << 8 | s) << 8 | s;
    for (int i = 0; i < 7; i++) {
        if (i < 3) {
            zoverSetColor(gOvlHintHandles[i], &gOvlFontSmall,
                          (s * 0x50 >> 8) | s * 13 << 12 | s * 11 << 4 | 0x2000000, 5);
        } else {
            zoverSetColor(gOvlHintHandles[i], &gOvlFontButton, shade >= 0x40 ? kOvlColorLabel : plain, 0);
        }
    }
}

BMX_ADDR(0x0041da90)
void ovlHintShow(int show)
{
    for (int i = 0; i < 7; i++) {
        if (gOvlHintHandles[i] != kOvlNoHandle) {
            zoverShow(gOvlHintHandles[i], show);
        }
    }
}

/*
 * Shows hint string `id` (3 lines of 0x114a + id * 3): capital letters A..Z in the text are replaced by
 * blanks of the right width and a button glyph overlay is placed over each. id 0xff clears the hint.
 */
BMX_ADDR(0x0041dad0)
void ovlHintSet(const uint32_t *value)
{
    memcpy(&gOvlHint, value, 4);
    ovlDropHandles(gOvlHintHandles, gOvlHintHandles + 7);
    if (gOvlHint.id == 0xff) {
        gOvlHint.timer = 0;
        return;
    }
    gOvlHint.timer = 0x5a;
    int y = 0x83;
    int glyphs = 0;
    for (int line = 0; line < 3; line++, y += 0x1d) {
        const char *text = zstringGet((uint16_t)((gOvlHint.id & 0x7f) * 3 + 0x114a + line));
        if (*text == '\0') {
            continue;
        }
        char buf[64];
        char *out = buf;
        for (const char *s = text; *s != '\0'; s++) {
            if (*s < 'A' || 'Z' < *s) {
                *out++ = *s;
            } else {
                *out++ = ' ';
                *out++ = ' ';
                *out++ = ' ';
                if ('D' < *s) {
                    *out++ = ' ';
                }
            }
        }
        *out = '\0';
        const int x = 0x140 - ((uint16_t)zoverTextWidth(buf, &gOvlFontSmall) >> 1);
        out = buf;
        for (const char *s = text; *s != '\0'; s++) {
            if (*s < 'A' || 'Z' < *s) {
                *out++ = *s;
                continue;
            }
            const uint16_t glyph = gHintGlyphs[(uint8_t)*s];
            *out = '\0';
            const uint16_t before = (uint16_t)zoverTextWidth(buf, &gOvlFontSmall);
            *out++ = ' ';
            *out++ = ' ';
            *out++ = ' ';
            if ('D' < *s) {
                *out++ = ' ';
            }
            if (&gOvlHintHandles[3 + glyphs] > &gOvlHintHandles[3] + 3) {
                zdebugFail("too many buttons in hint string, increase allocation", overlayFile, 0x1b06, 0);
            }
            gOvlHintHandles[3 + glyphs] =
                (uint16_t)zoverAddText(0, &gOvlFontButton, (const char *)&glyph, before + x, y, 0x2000000, 0);
            glyphs++;
        }
        *out = '\0';
        gOvlHintHandles[line] = (uint16_t)zoverAddText(0, &gOvlFontSmall, buf, x, y, 0x2000000, 5);
    }
}

BMX_ADDR(0x0041ddc0)
void ovlHintGet(uint32_t *out)
{
    memcpy(out, &gOvlHint, 4);
}

/* Element 11 (goal message): the three goal text lines of the current level, shown for 120 frames. */
BMX_ADDR(0x0041dde0)
void ovlGoalMsgInit(int create)
{
    if (create != 0) {
        gOvlGoalMsg.timer = 0;
        for (int i = 0; i < 3; i++) {
            gOvlGoalHandles[i] = kOvlNoHandle;
        }
        return;
    }
    ovlDropHandles(gOvlGoalHandles, gOvlGoalHandles + 3);
    gOvlGoalMsg.timer = 0;
}

BMX_ADDR(0x0041de60)
void ovlGoalMsgDraw(void)
{
    if (gOvlGoalMsg.timer == 0) {
        return;
    }
    gOvlGoalMsg.timer--;
    if (gOvlGoalMsg.timer == 0) {
        ovlDropHandles(gOvlGoalHandles, gOvlGoalHandles + 3);
        return;
    }
    int level;
    if (gOvlGoalMsg.timer >= 0x5a) {
        level = ovlFadeStep((int16_t)(0x78 - gOvlGoalMsg.timer));
    } else {
        if (gOvlGoalMsg.timer >= 0x3c || gOvlGoalMsg.timer < 0x1e) {
            return;
        }
        level = ovlFadeStep((int16_t)(gOvlGoalMsg.timer - 0x1e));
    }
    const uint8_t shade = ovlShade(level);
    if (shade == 0xff) {
        return;
    }
    const uint32_t s = shade;
    for (int i = 0; i < 3; i++) {
        zoverSetColor(gOvlGoalHandles[i], &gOvlFontSmall,
                      ((s * 3 << 5) >> 8) | s << 14 | s * 13 << 4 | 0x2000000, 6);
    }
}

BMX_ADDR(0x0041dfc0)
void ovlGoalMsgShow(int show)
{
    for (int i = 0; i < 3; i++) {
        if (gOvlGoalHandles[i] != kOvlNoHandle) {
            zoverShow(gOvlGoalHandles[i], show);
        }
    }
}

BMX_ADDR(0x0041e000)
void ovlGoalMsgSet(const uint32_t *value)
{
    memcpy(&gOvlGoalMsg, value, 4);
    ovlDropHandles(gOvlGoalHandles, gOvlGoalHandles + 3);
    if (gOvlGoalMsg.id == 0xff) {
        gOvlGoalMsg.timer = 0;
        return;
    }
    gOvlGoalMsg.timer = 0x78;
    int y = 0x28;
    for (int line = 0; line < 3; line++, y += 0x1d) {
        const char *text =
            zstringGet((uint16_t)(((gOvlGoalMsg.id & 0x7f) + gCurrentLevel * 0xc) * 3 + 0x2089 + line));
        if (*text != '\0') {
            const int16_t w = zoverTextWidth(text, &gOvlFontSmall);
            gOvlGoalHandles[line] =
                (uint16_t)zoverAddText(0, &gOvlFontSmall, text, ovlCentreX(0x140, w), y, 0x2000000, 6);
        }
    }
}

BMX_ADDR(0x0041e130)
void ovlGoalMsgGet(uint32_t *out)
{
    memcpy(out, &gOvlGoalMsg, 4);
}

/* Element 12 (status message): one line of text (and for some kinds a distance) shown for 90 frames. */
BMX_ADDR(0x0041e150)
void ovlStatusMsgInit(int create)
{
    if (create == 0) {
        if (gOvlStatusHandle == kOvlNoHandle) {
            gOvlStatus.msg.timer = 0;
            return;
        }
        zoverRemove(gOvlStatusHandle);
    }
    gOvlStatusHandle = kOvlNoHandle;
    gOvlStatus.msg.timer = 0;
}

BMX_ADDR(0x0041e190)
void ovlStatusMsgDraw(void)
{
    if (gOvlStatus.msg.timer == 0) {
        return;
    }
    gOvlStatus.msg.timer--;
    if (gOvlStatus.msg.timer == 0) {
        if (gOvlStatusHandle != kOvlNoHandle) {
            zoverRemove(gOvlStatusHandle);
            gOvlStatusHandle = kOvlNoHandle;
        }
        return;
    }
    uint32_t shade;
    if (gOvlStatus.msg.timer < 0x3c) {
        int level = bmxFtol((double)(int16_t)gOvlStatus.msg.timer * (double)0x1.333334p-2f);
        if (level > 0xf) {
            level = 0xf;
        }
        shade = ovlShade(level);
    } else {
        shade = 0xf0;
    }
    zoverSetColor(gOvlStatusHandle, &gOvlFontSmall,
                  ((shade * 13 << 4) >> 8) | shade << 14 | shade * 5 << 4 | 0x2000000, 4);
}

BMX_ADDR(0x0041e280)
void ovlStatusMsgShow(int show)
{
    if (gOvlStatusHandle != kOvlNoHandle) {
        zoverShow(gOvlStatusHandle, show);
    }
}

BMX_ADDR(0x0041e2b0)
void ovlStatusMsgSet(const uint32_t *value)
{
    char buf[64];
    memcpy(&gOvlStatus, value, 8);
    if (gOvlStatusHandle != kOvlNoHandle) {
        zoverRemove(gOvlStatusHandle);
        gOvlStatusHandle = kOvlNoHandle;
    }
    const uint8_t id = gOvlStatus.msg.id;
    if (id == 0xff) {
        gOvlStatus.msg.timer = 0;
        return;
    }
    gOvlStatus.msg.timer = 0x5a;
    if (id == 0xc) {
        zstrFormat(buf, "%ld-%s", (long)gOvlStatus.value, zstringGet(0x106f));
    } else {
        zstrFormat(buf, "%s", zstringGet((uint16_t)(id + 0x1063)));
    }
    if (0xf < id && id < 0x16) {
        zstrFormat(buf + strlen(buf), ": ");
        ovlFormatFeetInches(buf + strlen(buf), (float)gOvlStatus.value);
    }
    const int16_t w = zoverTextWidth(buf, &gOvlFontSmall);
    gOvlStatusHandle =
        (uint16_t)zoverAddText(0, &gOvlFontSmall, buf, ovlCentreX(0x140, w), 0xe2, 0x2000000, 4);
}

BMX_ADDR(0x0041e470)
void ovlStatusMsgGet(uint32_t *out)
{
    memcpy(out, &gOvlStatus, 8);
}

/* Element 13 (flash message): four lines at the left edge that fade in and out over 360 frames. */
BMX_ADDR(0x0041e4a0)
void ovlFlashInit(int create)
{
    gOvlFlash.timer = 0;
    if (create != 0) {
        for (int i = 0; i < 4; i++) {
            gOvlFlashHandles[i] = kOvlNoHandle;
        }
        return;
    }
    ovlDropHandles(gOvlFlashHandles, gOvlFlashHandles + 4);
}

BMX_ADDR(0x0041e510)
void ovlFlashDraw(void)
{
    if (gOvlFlash.timer == 0) {
        return;
    }
    uint32_t shade = 0x80;
    const uint16_t t = gOvlFlash.timer;
    if (t < 0x1e) {
        if ((t & 0xfffe) < 0x11) {
            shade = (uint8_t)((uint8_t)(t >> 1) << 4);
        }
    } else if (t >= 0x14a && (int)((0x168 - t) & 0xfffffffe) < 0x11) {
        shade = (uint8_t)((uint8_t)((0x168 - t) >> 1) << 4);
    }
    for (int i = 0; i < 4; i++) {
        zoverSetColor(gOvlFlashHandles[i], &gOvlFontTiny, ((shade | 0x200) << 8 | shade) << 8 | shade, 0);
    }
    gOvlFlash.timer--;
}

BMX_ADDR(0x0041e620)
void ovlFlashShow(int show)
{
    for (int i = 0; i < 4; i++) {
        zoverShow(gOvlFlashHandles[i], show);
    }
}

BMX_ADDR(0x0041e660)
void ovlFlashSet(const uint32_t *value)
{
    memcpy(&gOvlFlash, value, 4);
    if (gOvlFlash.id != 0xff) {
        int y = 0x150;
        for (int i = 0; i < 4; i++, y += 0x14) {
            if (gOvlFlashHandles[i] != kOvlNoHandle) {
                zoverRemove(gOvlFlashHandles[i]);
            }
            gOvlFlashHandles[i] = (uint16_t)zoverAddText(
                0, &gOvlFontTiny, zstringGet((uint16_t)(i + gOvlFlash.id * 4 + 0x102e)), 0x19, y, 0x2000000,
                0);
        }
        gOvlFlash.timer = 0x168;
        return;
    }
    ovlDropHandles(gOvlFlashHandles, gOvlFlashHandles + 4);
    gOvlFlash.timer = 0;
}

/* Element 14: one sprite overlay (the picture resource reserved at startup). */
BMX_ADDR(0x0041e750)
void ovlIconInit(int create)
{
    if (create != 0) {
        gOvlIconHandle = (uint16_t)zoverAddSprite(0, gOvlResA, 0x1b1, 0x148, 0x1808080);
        return;
    }
    zoverRemove(gOvlIconHandle);
    gOvlIconHandle = kOvlNoHandle;
}

BMX_ADDR(0x0041e7c0)
void ovlIconShow(int show)
{
    zoverShow(gOvlIconHandle, show);
}
