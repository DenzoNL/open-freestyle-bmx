/*
 * Front-end menu screens (C:\zaxis\Bmx\src\com\menu\frontend.c and the screens built on it).
 *
 * Every screen is a GUI hook (GuiHookFn) plus a "step" function: gFrontendStep holds the function the
 * screen's tick runs once per frame (or NULL). The ids 0xa0140004 etc. are element ids of the screens'
 * resource blocks (screen id in the high half).
 */
#ifndef BMX_COM_MENU_FRONTEND_H
#define BMX_COM_MENU_FRONTEND_H

#include <stddef.h>
#include <stdint.h>

#include "bmx/com/action/comdata.h"
#include "bmx/com/menu/gui/gui.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/ngif/missingzge.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/zprim.h"
#include "compat/orig.h"

/* The step function the running screen executes from its tick message (0x57458c). */
typedef void(BMX_CDECL *FrontendStepFn)(void);
#define gFrontendStep ORIG_VAR(0x0057458c, FrontendStepFn, gFrontendStep)

/* Display list: the primitive buffer cursor (0x5d2510) and the ordering table pointer (0x68c3d0). */
#define gPrimCursor ORIG_VAR(0x005d2510, uint8_t *, gPrimCursor)

/*
 * Primitive of the 3x3 frame box (a 4 point textured polygon of the display list, 0xa4 bytes apart in the
 * primitive buffer). Vertex k has x, y, u, v (u/v in texture pixels) and a colour.
 */
typedef struct FramePoly {
    uint32_t tag; /* 0x00 ordering table link in the low 28 bits */
    uint8_t r0, g0, b0, code;
    uint16_t unk8;
    uint16_t unkA; /* always 0x20 */
    int16_t x0, y0, u0, v0;
    uint8_t r1, g1, b1, pad17;
    int16_t x1, y1, u1, v1;
    uint8_t r2, g2, b2, pad23;
    int16_t x2, y2, u2, v2;
    uint8_t r3, g3, b3, pad2f;
    int16_t x3, y3, u3, v3;
    uint32_t texture;
    uint32_t flags; /* 0x3c */
    uint8_t pad40[0x64];
} FramePoly;

_Static_assert(sizeof(FramePoly) == 0xa4, "FramePoly layout");
_Static_assert(offsetof(FramePoly, x3) == 0x30, "FramePoly layout");
_Static_assert(offsetof(FramePoly, flags) == 0x3c, "FramePoly layout");

/* Sprite holding the frame box texture (a copy of asset 0x40c4); created by menuFrameInit. */
#define gMenuFrameSprite ORIG_VAR(0x00574594, ZPrimAsset *, gMenuFrameSprite)

/* Slide offsets of the four rows that slide in/out (0x691f28, 0x691f24, 0x691f20, 0x691f10). */
#define gMenuSlide0 ORIG_VAR(0x00691f28, int, gMenuSlide0)
#define gMenuSlide1 ORIG_VAR(0x00691f24, int, gMenuSlide1)
#define gMenuSlide2 ORIG_VAR(0x00691f20, int, gMenuSlide2)
#define gMenuSlide3 ORIG_VAR(0x00691f10, int, gMenuSlide3)

/* ---- shared helpers (0x43be90..0x43cae0) ----------------------------------------------------- */
int menuWrap(int value, int count);
void menuFrameInit(void);
void menuFrameFree(void);
void menuDrawFrame(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color, int full);
int menuSlideOut(uint32_t id0, uint32_t id1, uint32_t id2, uint32_t id3);
void menuSlideReset(void);
int menuSlideIn(uint32_t id0, uint32_t id1, uint32_t id2, uint32_t id3);

/* ---- element helpers: the screens address elements by id ----------------------------------------- */
static inline void menuSetText(uint32_t id, const char *text)
{
    guiSetText(guiFindById(id), text);
}

static inline void menuSetStyle(uint32_t id, uint32_t style)
{
    guiSetTextStyle(guiFindById(id), style);
}

static inline void menuSetTint(uint32_t id, uint8_t r, uint8_t g, uint8_t b, uint32_t alpha)
{
    guiSetTint(guiFindById(id), r, g, b, alpha);
}

static inline void menuShow(uint32_t id)
{
    guiShow(guiFindById(id));
}

static inline void menuHide(uint32_t id)
{
    guiHide(guiFindById(id));
}

static inline void menuPlaySound(int id)
{
    bmxSoundPlayAt(id, NULL, 0, 0, 0);
}

/* ---- language screen (frontend_language.c) ------------------------------------------------- */
/* Language ids offered (0x55a358) and the one in use (0x696218, low byte). */
typedef uint8_t LanguageTable[3];
#define gLanguageTable   ORIG_VAR(0x0055a358, LanguageTable, gLanguageTable)
#define gLanguageCurrent ORIG_VAR(0x00696218, uint32_t, gLanguageCurrent)
#define gLanguageSel     ORIG_VAR(0x00691f04, uint16_t, gLanguageSel)

int languageScreenInit(void);
void languageScreenLayout(void);
void languageScreenDone(void);
int BMX_CDECL languageScreenHook(int msg, GuiElement *el, int arg3, int arg4);

/* Volumes of the two sound channels, one byte each. */
#define gSoundVolume0 ORIG_VAR(0x006955e4, uint8_t, gSoundVolume0)
#define gSoundVolume1 ORIG_VAR(0x006955e5, uint8_t, gSoundVolume1)

/* Applies the stored volume (times 7) of a sound channel through the sound layer's volume command. */
static inline void menuSetChannelVolume(int channel, int volume)
{
    bmxSoundPlayAt(0x3ea, (const void *)(uintptr_t)channel, volume, 0, 0);
}

/* ---- memory card dialog (frontend_memcard.c) -------------------------------------------------- */
typedef void(BMX_CDECL *McDoneFn)(void);
typedef uint32_t McRows[8];
/* Element ids of the dialog's lines (title, message lines, then the two buttons), copied from the caller. */
#define gMcRows      ORIG_VAR(0x00691ee0, McRows, gMcRows)
#define gMcState     ORIG_VAR(0x00691f00, uint16_t, gMcState)
#define gMcSlot      ORIG_VAR(0x00691f02, uint16_t, gMcSlot)
#define gMcCountdown ORIG_VAR(0x00691ed6, int16_t, gMcCountdown) /* frames to wait before the next step */
#define gMcChoice    ORIG_VAR(0x00691ec0, uint16_t, gMcChoice)   /* highlighted button: 0 first, 1 second */
#define gMcLoadOk    ORIG_VAR(0x00691ed4, uint16_t, gMcLoadOk)
#define gMcMode      ORIG_VAR(0x00691ed8, uint16_t, gMcMode)
#define gMcSaveOk    ORIG_VAR(0x00691eda, uint16_t, gMcSaveOk)
#define gMcDone      ORIG_VAR(0x0057459c, McDoneFn, gMcDone)

void mcDialogOpen(int16_t save, uint16_t slot, const uint32_t *rows, uint16_t mode, McDoneFn done);
int BMX_CDECL mcDialogHook(int msg, GuiElement *el, int arg3, int arg4);
void mcStepClose(void);
void mcStepLoadProbe(void);
void mcStepLoadRead(void);
void mcStepPrompt1(void);
void mcStepSaveFormat(void);
void mcStepMessage(void);
void mcStepPrompt3(void);
void mcStepPrompt4(void);
void mcStepSaveProbe(void);
void mcStepSaveWrite(void);
void mcShowLoadResult(int code);
void mcShowSaveResult(int code);

/* ---- options screen (screen 0xa00c, frontend_options.c) ----------------------------------------- */
/* One toggle of the game options panel: label string id and the bit it flips in gOptionsValue. */
typedef struct OptionEntry {
    uint16_t stringId;
    uint8_t bit;
    uint8_t pad3;
} OptionEntry;
typedef OptionEntry OptionTable[8];
typedef const OptionEntry *OptionRowTable[8];
typedef int OptionSlideTable[8];
#define gOptionTable ORIG_VAR(0x0055a3bc, OptionTable, gOptionTable)
/* Options offered (bit mask, 0x6955e0) and their values (0x6955e2). */
#define gOptionsAvail ORIG_VAR(0x006955e0, uint16_t, gOptionsAvail)
#define gOptionsValue ORIG_VAR(0x006955e2, uint16_t, gOptionsValue)
/* Entries shown (pointers into gOptionTable), their number, the selected row and the rows' slide offsets. */
#define gOptRows  ORIG_VAR(0x00691e80, OptionRowTable, gOptRows)
#define gOptCount ORIG_VAR(0x00691e10, uint16_t, gOptCount)
#define gOptSel   ORIG_VAR(0x00691ea0, int, gOptSel)
#define gOptSlide ORIG_VAR(0x00691e40, OptionSlideTable, gOptSlide)
#define gOptTicks ORIG_VAR(0x00691e1c, int, gOptTicks)
/* Strings for the two states of an option (off, on): ids at 0x55a3b0 and 0x55a3b4. */
typedef struct OptionStateString {
    uint16_t stringId;
    uint16_t pad2;
} OptionStateString;
typedef OptionStateString OptionStateTable[2];
#define gOptStateStrings ORIG_VAR(0x0055a3b0, OptionStateTable, gOptStateStrings)

/* Gradient strip primitive (a 4 point flat-coloured polygon, 0x90 bytes apart in the primitive buffer). */
typedef struct StripPoly {
    uint32_t tag;
    uint8_t r0, g0, b0, code;
    int16_t x0, y0;
    uint8_t r1, g1, b1, pad0f;
    int16_t x1, y1;
    uint8_t r2, g2, b2, pad17;
    int16_t x2, y2;
    uint8_t r3, g3, b3, pad1f;
    int16_t x3, y3;
    uint32_t pad24;
    uint32_t flags; /* 0x28 */
    uint8_t pad2c[0x64];
} StripPoly;

_Static_assert(sizeof(StripPoly) == 0x90, "StripPoly layout");
_Static_assert(offsetof(StripPoly, flags) == 0x28, "StripPoly layout");

/* Colours of the eight strips (0x55a3dc) and the two scale factors of their x positions. */
typedef uint8_t StripColorTable[8][3];
#define gStripColors ORIG_VAR(0x0055a3dc, StripColorTable, gStripColors)
#define gStripScale0 ORIG_VAR(0x0055c690, float, gStripScale0)
#define gStripScale1 ORIG_VAR(0x0054d220, float, gStripScale1)

/*
 * Setup of one player (0x6952a0, 0x4c bytes each, two players): the chosen rider, style and bike, and the
 * key bindings (11 action masks, bit n = pad button n) with the keyboard layout in the last byte.
 */
typedef struct PlayerSetup {
    uint8_t pad0[3];
    uint8_t rider; /* 0x03 */
    uint8_t style; /* 0x04 */
    uint8_t bike;  /* 0x05 */
    uint8_t pad6[14];
    uint32_t bind[11]; /* 0x14 */
    uint8_t pad40[11];
    uint8_t layout; /* 0x4b */
} PlayerSetup;

_Static_assert(sizeof(PlayerSetup) == 0x4c, "PlayerSetup layout");
_Static_assert(offsetof(PlayerSetup, bind) == 0x14, "PlayerSetup layout");
_Static_assert(offsetof(PlayerSetup, layout) == 0x4b, "PlayerSetup layout");

#define gKeyPlayer ORIG_VAR(0x00691e08, int, gKeyPlayer)
typedef PlayerSetup PlayerSetupTable[4];
#define gPlayers ORIG_VAR(0x006952a0, PlayerSetupTable, gPlayers)
/* Working copy of the player's bindings while editing (0x5745b4, 11 masks). */
typedef uint32_t KeyWork[11];
#define gKbWork ORIG_VAR(0x005745b4, KeyWork, gKbWork)
#define gKbCursor                                                                                            \
    ORIG_VAR(0x005745e0, uint8_t, gKbCursor) /* row 0..10 actions, 11 layout, 12 ok, 13 cancel */
#define gKbState                                                                                             \
    ORIG_VAR(0x005745e1, uint8_t, gKbState) /* 0 browsing, 1 wait for release, 2 press a button */
#define gKbLayout ORIG_VAR(0x005745e2, uint8_t, gKbLayout) /* 0..4 */
/* Text pointers: names of the 18 pad buttons, the 5 layout titles, and 5 x 18 action labels. */
typedef const char *KeyNameTable[18];
typedef const char *KeyTitleTable[5];
typedef const char *KeyLabelTable[5][18];
#define gKeyNames       ORIG_VAR(0x0055a3f4, KeyNameTable, gKeyNames)
#define gKbLayoutTitles ORIG_VAR(0x0055a43c, KeyTitleTable, gKbLayoutTitles)
#define gKbLayoutLabels ORIG_VAR(0x0055a450, KeyLabelTable, gKbLayoutLabels)

/* Sound volume panel: slide offsets of its two bars, selected bar and the two levels (0..18). */
#define gSoundSlideA ORIG_VAR(0x00691e00, int, gSoundSlideA)
#define gSoundSlideB ORIG_VAR(0x00691e04, int, gSoundSlideB)
#define gSoundSel    ORIG_VAR(0x00691e20, int, gSoundSel)
#define gSoundLevel0 ORIG_VAR(0x00691dd4, int, gSoundLevel0)
#define gSoundLevel1 ORIG_VAR(0x00691e64, int, gSoundLevel1)

void keyboardOpen(void);
void keyboardClose(void);
int BMX_CDECL keyboardHook(int msg, GuiElement *el, int arg3, int arg4);
void soundPanelOpen(void);
void soundPanelClose(void);
void soundPanelSlideIn(void);
void soundPanelSlideOut(void);
int BMX_CDECL soundPanelHook(int msg, GuiElement *el, int arg3, int arg4);

void controlsDrawStrips(void);
void optionsGameOpen(void);
void optionsGameClose(void);
void optionsGameSlideIn(void);
void optionsGameSlideOut(void);
int BMX_CDECL optionsGameHook(int msg, GuiElement *el, int arg3, int arg4);
void controlsOpen(void);
void controlsClose(void);
int BMX_CDECL controlsHook(int msg, GuiElement *el, int arg3, int arg4);

/* Options main list: selected row (0x691e60, -1 while leaving), "ready" flag, frame pulse, save delay. */
#define gOptMainSel       ORIG_VAR(0x00691e60, int, gOptMainSel)
#define gOptMainReady     ORIG_VAR(0x00691df8, int, gOptMainReady)
#define gOptMainPulse     ORIG_VAR(0x00574584, int, gOptMainPulse)
#define gOptMainPulseStep ORIG_VAR(0x0055a354, int, gOptMainPulseStep)
#define gOptSaveDelay     ORIG_VAR(0x00691e14, int, gOptSaveDelay)

int optionsInit(void);
int BMX_CDECL optionsMainHook(int msg, GuiElement *el, int arg3, int arg4);
void optionsMainHighlight(void);
void optionsMainSave(void);
void optionsMainSlideOut(void);
void optionsMainStep(void);
void optionsMainLayout(void);

/* ---- save/load slot screen (screen 0xa012, frontend_slots.c) ------------------------------------ */
/* Function the slot screen calls when it is left (0x691dc0) and its argument (0x691dd0); selected entry. */
typedef void(BMX_CDECL *ScreenReturnFn)(uint32_t arg);
#define gScreenReturnFn  ORIG_VAR(0x00691dc0, ScreenReturnFn, gScreenReturnFn)
#define gScreenReturnArg ORIG_VAR(0x00691dd0, uint32_t, gScreenReturnArg)
#define gSlotSel         ORIG_VAR(0x00691dce, uint16_t, gSlotSel)

int slotScreenInit(void);
void slotScreenLayout(void);
void slotScreenDone(void);
void slotScreenReturn(void);
int BMX_CDECL slotScreenHook(int msg, GuiElement *el, int arg3, int arg4);

/* ---- game setup screen (screen 0xa009, frontend_select.c) ---------------------------------------- */
/*
 * Picking rider, bike, style and level for one or two players. Each stage shows a carousel of three
 * picture/text element pairs (gSelPicIds / gSelTextIds) that flip as the choice changes; the previews are
 * textures copied into the pictures' sprites.
 */
typedef uint32_t SelIdTable[3];
#define gSelPicIds  ORIG_VAR(0x0054d224, SelIdTable, gSelPicIds)  /* 0xa009000d..f */
#define gSelTextIds ORIG_VAR(0x0054d230, SelIdTable, gSelTextIds) /* 0xa0090012..14 */
typedef uint16_t SelAssetTable[12];
typedef uint32_t SelWideTable[12];
#define gRiderTextures ORIG_VAR(0x0054d23c, SelAssetTable, gRiderTextures) /* preview asset per rider */
#define gRiderNames    ORIG_VAR(0x0054d254, SelAssetTable, gRiderNames)    /* string id per rider */
#define gLevelTextures ORIG_VAR(0x0054d35c, SelWideTable, gLevelTextures)  /* low 16 bits: asset per level */
#define gLevelNames    ORIG_VAR(0x0054d38c, SelWideTable, gLevelNames)     /* low 16 bits: string id */

/* A caption: the first of a zero-terminated list of string ids shown one after the other by the typewriter.
 */
typedef uint16_t SelCaption;
typedef const SelCaption *SelCaptionPtrTable[12];
#define gRiderCaptions ORIG_VAR(0x0055aaa0, SelCaptionPtrTable, gRiderCaptions)
typedef SelCaption SelCaptionKinds[4][4];
#define gCaptionKinds                                                                                        \
    ORIG_VAR(0x0055aad0, SelCaptionKinds, gCaptionKinds) /* by entry kind (+1 when variant is 4) */
typedef SelCaption SelCaptionOrders[3][4];
#define gCaptionOrders ORIG_VAR(0x0055aaf0, SelCaptionOrders, gCaptionOrders)
typedef SelCaption SelCaptionLocked[4];
#define gCaptionLocked ORIG_VAR(0x0055ab10, SelCaptionLocked, gCaptionLocked)

/* Per rider save data (0x6955ec, 0x104 bytes each): unlocked counts and the entries of the levels. */
typedef struct RiderSave {
    uint8_t levelCount; /* 0x00 levels unlocked (up to 12) */
    uint8_t styleCount; /* 0x01 */
    uint8_t bikeCount;  /* 0x02 (up to 5) */
    uint8_t pad3;
    uint8_t data[0x100]; /* 0x04: level entries of 0x27 bytes {kind, variant, ...}; order list at 0xe7 */
} RiderSave;

_Static_assert(sizeof(RiderSave) == 0x104, "RiderSave layout");
typedef RiderSave RiderSaveTable[12];
#define gRiders ORIG_VAR(0x006955ec, RiderSaveTable, gRiders)
/* Number of riders unlocked (0x6955e7). */
#define gRiderCount ORIG_VAR(0x006955e7, uint8_t, gRiderCount)

/* Selection state: player being edited, stage, per player choices. */
typedef int SelPlayerInts[2];
#define gSelPlayer ORIG_VAR(0x00691ca8, int, gSelPlayer)
#define gSelStage                                                                                            \
    ORIG_VAR(0x00691cb0, int, gSelStage) /* -1 leave, 0 rider, 1 bike, 2 style, 3 level, 4 start */
#define gSelRider      ORIG_VAR(0x00691ca0, SelPlayerInts, gSelRider)
#define gSelStyle      ORIG_VAR(0x00691d58, SelPlayerInts, gSelStyle)
#define gSelBike       ORIG_VAR(0x00691d98, SelPlayerInts, gSelBike)
#define gSelLevel      ORIG_VAR(0x00691d94, int, gSelLevel)
#define gSelBikeCount  ORIG_VAR(0x0069181f, uint8_t, gSelBikeCount) /* bikes unlocked for the rider */
#define gSelStyleCount ORIG_VAR(0x0069183c, uint8_t, gSelStyleCount)
#define gSelLevelCount ORIG_VAR(0x00691d08, uint8_t, gSelLevelCount)
/* Frame counter of the screen, the frame the next stage may start at, the frame the last preview changed. */
#define gSelTick      ORIG_VAR(0x00691704, int, gSelTick)
#define gSelReadyTick ORIG_VAR(0x0069182c, int, gSelReadyTick)
#define gSelFlipTick  ORIG_VAR(0x00691cf4, int, gSelFlipTick)
/* Carousel: page, number of pages, slide offsets of the pages, slide direction. */
#define gSelPage  ORIG_VAR(0x00691830, int, gSelPage)
#define gSelPages ORIG_VAR(0x00691ce4, int, gSelPages)
typedef int SelSlideTable[3];
#define gSelSlide    ORIG_VAR(0x00691820, SelSlideTable, gSelSlide)
#define gSelSlideDir ORIG_VAR(0x00691838, int, gSelSlideDir)
/* 0x691ce8: set while the stage's pictures are in use; 0x691cee / 0x691d04: grey levels of the previews. */
#define gSelPicturesOn ORIG_VAR(0x00691ce8, int, gSelPicturesOn)
#define gSelDimA       ORIG_VAR(0x00691cee, uint16_t, gSelDimA)
#define gSelDimB       ORIG_VAR(0x00691d04, uint16_t, gSelDimB)
/* Callbacks of the running stage: preview(step) for a left/right press, commit(step) when the slide ended. */
typedef void(BMX_CDECL *SelStepFn)(int step);
#define gSelPreviewFn ORIG_VAR(0x00574588, SelStepFn, gSelPreviewFn)
#define gSelCommitFn  ORIG_VAR(0x00574590, SelStepFn, gSelCommitFn)

/* Tables of the style/bike/level previews, indexed by rider*5 + choice unless noted. */
typedef uint16_t SelAsset60[60];
typedef const SelCaption *SelCaptionPtr60[60];
#define gStyleTextures ORIG_VAR(0x0054d3bc, SelAsset60, gStyleTextures)
#define gStyleGroups                                                                                         \
    ORIG_VAR(0x0054d434, SelAssetTable, gStyleGroups) /* by rider: asset group of the styles */
#define gBikePalettes ORIG_VAR(0x0054d82c, SelAsset60, gBikePalettes) /* palette index + 1 */
#define gBikeNames                                                                                           \
    ORIG_VAR(0x0055ab18, SelAssetTable, gBikeNames) /* by rider: first string id of the bikes */
#define gBikeTextures ORIG_VAR(0x0055ab30, SelAssetTable, gBikeTextures) /* by rider */
#define gBikeCaptions ORIG_VAR(0x0055ad30, SelCaptionPtr60, gBikeCaptions)
#define gBikeLocked   ORIG_VAR(0x0055ab48, SelCaptionLocked, gBikeLocked)
/* Working sprites: 0x691cf8, 0x691cfc, 0x691d00 (slots 1, 2, 0 of selSpriteSlot); style preview sprites and
 * frame. */
typedef ZPrimAsset *SelSpriteTable[3];
#define gSelSprites ORIG_VAR(0x00691cf8, SelSpriteTable, gSelSprites)
typedef ZPrimAsset *SelStyleSprites[16];
#define gStyleSprites ORIG_VAR(0x005745ec, SelStyleSprites, gStyleSprites)
#define gStyleFrame   ORIG_VAR(0x0057462c, int, gStyleFrame) /* animation frame of the bike preview texture */
/* Extent words of the stages (0x691834, 0x691836, 0x691cec). */
#define gSelExtentA ORIG_VAR(0x00691834, uint16_t, gSelExtentA)
#define gSelExtentB ORIG_VAR(0x00691836, uint16_t, gSelExtentB)
#define gSelExtentC ORIG_VAR(0x00691cec, uint16_t, gSelExtentC)

/* Caption typewriter lines: 5 slots (0x691840, 0xe0 bytes) of two text lines with their font ids, per slot
 * widths and colours. */
typedef struct CaptionSlot {
    uint32_t x1, y1;
    uint16_t font1; /* 0x08 asset id of the font of line 1 (0x8001) */
    char text1[100];
    uint8_t pad6e[2];
    uint32_t x2, y2; /* 0x70 */
    uint16_t font2;  /* 0x78 */
    char text2[100];
    uint8_t pade2[2];
} CaptionSlot;

_Static_assert(sizeof(CaptionSlot) == 0xe0, "CaptionSlot layout");
_Static_assert(offsetof(CaptionSlot, text2) == 0x7a, "CaptionSlot layout");
typedef CaptionSlot CaptionSlotTable[5];
#define gCaptionSlots ORIG_VAR(0x00691840, CaptionSlotTable, gCaptionSlots)
typedef uint16_t CaptionWidths[5];
#define gCaptionWidths ORIG_VAR(0x00691db0, CaptionWidths, gCaptionWidths) /* width of line 1 */
typedef struct CaptionColor {
    uint16_t line1, line2; /* zprim colour property of the two lines */
} CaptionColor;
typedef CaptionColor CaptionColors[5];
#define gCaptionColors ORIG_VAR(0x00691d80, CaptionColors, gCaptionColors)

/* Rider stat bars: bar widths (0x54d26c), per style layer sprite indices (0x54d44c), layer offsets, bike
 * stats. */
typedef uint32_t StatWidthTable[60];
#define gStatWidths ORIG_VAR(0x0054d26c, StatWidthTable, gStatWidths)
typedef int16_t StatLayerTable[60][8];
#define gStatLayers ORIG_VAR(0x0054d44c, StatLayerTable, gStatLayers)
typedef int16_t StatPosTable[8][2];
#define gStatPositions ORIG_VAR(0x0054d80c, StatPosTable, gStatPositions)
typedef uint8_t BikeStatTable[300];
#define gBikeStats ORIG_VAR(0x0054d8a4, BikeStatTable, gBikeStats)

/* Typed button messages (0x691cc0, 16 entries) and the next slot; pending stage change (0x691dbc). */
typedef uint16_t CheatRing[16];
#define gCheatRing  ORIG_VAR(0x00691cc0, CheatRing, gCheatRing)
#define gCheatPos   ORIG_VAR(0x00691cf0, uint16_t, gCheatPos)
#define gSelPending ORIG_VAR(0x00691dbc, int, gSelPending)
/* Stage to resume at when the screen is entered from a later point (0x56b617) and the level setting
 * (0x56b614). */
#define gSelResume       ORIG_VAR(0x0056b617, uint8_t, gSelResume)
#define gSelLevelSetting ORIG_VAR(0x0056b614, uint8_t, gSelLevelSetting)
#define gSelUnknown60    ORIG_VAR(0x00691d60, uint16_t, gSelUnknown60)
#define gSelUnknownA0    ORIG_VAR(0x00691da0, uint32_t, gSelUnknownA0)

/* Shared pieces of the stages (see frontend_select.c). */
void selSpriteSlot(int slot, int w, int h, int bpp, int colors);
void selResetCarousel(void);
void selStartCaption(const SelCaption *caption);
void selShowHeader(void);
void selHideHeader(void);
void selSlideStep(void);

void selCheatInput(uint16_t message);
int selScreenOpen(void);
void selScreenLeave(void);
void selCommitSetup(void);
void selChangeStage(int delta);
void selLeaveStep(void);
void selIntroStep(void);
void selScreenLayout(void);
int BMX_CDECL selScreenHook(int msg, GuiElement *el, int arg3, int arg4);
int BMX_CDECL selScreenDraw(int msg, GuiElement *el, int arg3, int arg4);
void selCaptionFeed(int count, int offset);

void selRiderPreview(int step);
void selRiderCommit(int step);
void selRiderOpen(void);
void selRiderClose(void);
void selLevelCommit(int step);
void selLevelRefresh(int step);
void selLevelPreview(int step);
void selLevelOpen(void);
void selLevelClose(void);
void selStyleCommit(int step);
void selStylePreview(int step);
void selStyleOpen(void);
void selStyleClose(void);
void selBikeIdle(void);
void selBikeCommit(int step);
void selBikePreview(int step);
void selBikeOpen(void);
void selBikeClose(void);

/* Caption typewriter state: running flag (0x691ce0), the caption (0x691cac), its progress. */
#define gCaptionOn    ORIG_VAR(0x00691ce0, uint16_t, gCaptionOn)
#define gCaptionPtr   ORIG_VAR(0x00691cac, const SelCaption *, gCaptionPtr)
#define gCaptionLine  ORIG_VAR(0x00691d06, uint16_t, gCaptionLine)
#define gCaptionHold  ORIG_VAR(0x00691dba, uint16_t, gCaptionHold)
#define gCaptionTime  ORIG_VAR(0x00691da4, uint16_t, gCaptionTime)
#define gCaptionAlpha ORIG_VAR(0x00691da6, uint16_t, gCaptionAlpha)

/* ---- title screens: autoload, "press start", main menu (frontend_title.c) ---------------------- */
typedef void(BMX_CDECL *TitleActionFn)(void);
/* Action to run once the screen's delay has passed (0x5745a4), selected entry of the main menu (0x5745a0). */
#define gTitleAction ORIG_VAR(0x005745a4, TitleActionFn, gTitleAction)
#define gTitleSel    ORIG_VAR(0x005745a0, uint16_t, gTitleSel)
/* Autoload screen: ticks (0x5745ac), "press start" blink stage (0x5745ae), result flag (0x5745a8), slot
 * found. */
#define gAutoTick   ORIG_VAR(0x005745ac, uint16_t, gAutoTick)
#define gTitleStage ORIG_VAR(0x005745ae, uint16_t, gTitleStage)
#define gAutoLoadOk ORIG_VAR(0x005745a8, uint32_t, gAutoLoadOk)
#define gAutoSlot   ORIG_VAR(0x00691ebc, uint16_t, gAutoSlot)
/* Frames without input (0x691eb4), frames since a delayed action was queued (0x691eb8), frames on screen. */
#define gTitleIdle   ORIG_VAR(0x00691eb4, uint16_t, gTitleIdle)
#define gTitleDelay  ORIG_VAR(0x00691eb8, int, gTitleDelay)
#define gTitleFrames ORIG_VAR(0x00691ebe, uint16_t, gTitleFrames)
/* Progress of the main menu's scroll animation, 6 per tick up to 0x3c. */
#define gScrollTick ORIG_VAR(0x00691ea8, uint16_t, gScrollTick)
/* Number of players of the game being set up (0x69621c: 1 or 2, 0 outside of it); selected game mode entry
 * 0..2 (0x56b616). */
#define gPlayerCount        ORIG_VAR(0x0069621c, uint8_t, gPlayerCount)
#define gGameModeSel        ORIG_VAR(0x0056b616, uint8_t, gGameModeSel)
#define gAutoLoadDone       ORIG_VAR(0x0056b618, uint32_t, gAutoLoadDone)
#define gTitleUnknown551fd4 ORIG_VAR(0x00551fd4, uint32_t, gTitleUnknown551fd4)

/* Main menu carousel: 7 slots with a position, alpha and tint; the string ids of the 6 entries. */
typedef struct MenuSlotPos {
    int16_t x, y;
} MenuSlotPos;
typedef MenuSlotPos MenuSlotPosTable[7];
typedef int16_t MenuSlotShortTable[7];
typedef uint16_t MenuEntryStrings[6];
#define gMenuSlotPos      ORIG_VAR(0x0055a368, MenuSlotPosTable, gMenuSlotPos)
#define gMenuSlotAlpha    ORIG_VAR(0x0055a384, MenuSlotShortTable, gMenuSlotAlpha)
#define gMenuSlotTint     ORIG_VAR(0x0055a394, MenuSlotShortTable, gMenuSlotTint)
#define gMenuEntryStrings ORIG_VAR(0x0055a35c, MenuEntryStrings, gMenuEntryStrings)

void autoloadScreenInit(void);
int BMX_CDECL autoloadScreenHook(int msg, GuiElement *el, int arg3, int arg4);
void pressStartInit(void);
int BMX_CDECL pressStartHook(int msg, GuiElement *el, int arg3, int arg4);
int mainMenuInit(int skipLoad);
void mainMenuSetHook(void);
void mainMenuEnter(void);
int BMX_CDECL mainMenuHook(int msg, GuiElement *el, int arg3, int arg4);
void mainMenuScrollDown(void);
void mainMenuScrollUp(void);
void mainMenuLayout(void);
int mainMenuIdleExit(void);
int BMX_CDECL blankScreenHook(int msg, GuiElement *el, int arg3, int arg4);

/* Belongs to bmxsound.c: stores a quit/transition request (flag at 0x574654, code at 0x574658). */
ORIG_FN(0x0044dd10, void, frontendRequestExit, (int code));
/* Belongs to zrwzgl.c: releases the display and shows a Yes/No message box; 1 when the player answered yes.
 */
ORIG_FN(0x0048a9b0, int, zrwConfirmQuit, (void));

/* ---- extras menu: hub, movies, high scores, level info, credits (frontend_extras.c) ---------------- */
/* The Extras hub (screen 0xa00d) has four entries (0x691214: -1 leaving, 0..3) that open the screens below.
 */
#define gExtrasChoice ORIG_VAR(0x00691214, int, gExtrasChoice)
#define gExtrasReady  ORIG_VAR(0x00691228, int, gExtrasReady) /* hub is on screen and takes input */
/* Pulse of the selection frame (0x574584) and its step (0x55a354). */
#define gExtrasPulse     ORIG_VAR(0x00574584, int, gExtrasPulse)
#define gExtrasPulseStep ORIG_VAR(0x0055a354, int, gExtrasPulseStep)
/* Frames to wait before the next step runs (0x691700). */
#define gExtrasWait ORIG_VAR(0x00691700, uint16_t, gExtrasWait)
/* Skips the slide animations when non-zero (0x5ab474). */
#define gMenuInstant ORIG_VAR(0x005ab474, int, gMenuInstant)

/*
 * Movie list entry: a description the movie player takes through a pointer to a pointer to it. The
 * extras titles (0x55af40 ...) are 100 character names followed by the description of their movie.
 */
typedef struct MovieDesc {
    const char *file;
    int32_t width, height;
    int32_t unk0c, unk10, end;
} MovieDesc;

typedef struct ExtrasTitle {
    char name[100];
    MovieDesc movie;
} ExtrasTitle;

_Static_assert(sizeof(ExtrasTitle) == 0x7c, "ExtrasTitle layout");
/* Plays the movie(s) of the descriptor; the exe's own name for it is not known. */
void moviePlay(MovieDesc *const *list);

/* Titles of the movie list (the first is always there, the others when unlocked). */
#define gTitleIntro ORIG_VAR(0x0055af40, ExtrasTitle, gTitleIntro)
#define gTitleDave  ORIG_VAR(0x0055afbc, ExtrasTitle, gTitleDave)
#define gTitleRyan  ORIG_VAR(0x0055b038, ExtrasTitle, gTitleRyan)
#define gTitleAmish ORIG_VAR(0x0055b0b4, ExtrasTitle, gTitleAmish)

typedef ExtrasTitle *ExtrasTitleList[4];
#define gExtrasTitles ORIG_VAR(0x0068fff0, ExtrasTitleList, gExtrasTitles)
#define gExtrasCount  ORIG_VAR(0x006916f0, uint16_t, gExtrasCount)
#define gExtrasSel    ORIG_VAR(0x00574644, uint16_t, gExtrasSel)
#define gExtrasPlayed                                                                                        \
    ORIG_VAR(0x00574648, uint32_t, gExtrasPlayed) /* a movie was chosen: play it after sliding out */
#define gExtrasWait2 ORIG_VAR(0x00690896, uint16_t, gExtrasWait2)
typedef int16_t SlideTable[4];
#define gExtrasSlide ORIG_VAR(0x00691230, SlideTable, gExtrasSlide)

/* Unlock flags of the movies (bit 0): Dave Mirra, Ryan Nyquist, Amish Boy. */
#define gUnlockDave  ORIG_VAR(0x006955e8, uint8_t, gUnlockDave)
#define gUnlockRyan  ORIG_VAR(0x006956ec, uint8_t, gUnlockRyan)
#define gUnlockAmish ORIG_VAR(0x00696114, uint8_t, gUnlockAmish)

/* Cells of the high score table (one text each, 0x78 bytes): position, style and the text. */
typedef struct HiscoreCell {
    int32_t kind;  /* 0x00 */
    int32_t y;     /* 0x04 */
    int16_t flags; /* 0x08 always 0x8001 */
    int16_t pad0a;
    int32_t blinkStyle; /* 0x0c style while the blink bit is set */
    int32_t style;      /* 0x10 */
    char text[100];     /* 0x14 */
} HiscoreCell;

_Static_assert(sizeof(HiscoreCell) == 0x78, "HiscoreCell layout");
typedef HiscoreCell HiscoreColumn[8];
#define gHiscoreTimes  ORIG_VAR(0x006904d0, HiscoreColumn, gHiscoreTimes)
#define gHiscoreLabels ORIG_VAR(0x00690990, HiscoreColumn, gHiscoreLabels)
#define gHiscoreRiders ORIG_VAR(0x00690e50, HiscoreColumn, gHiscoreRiders)
#define gHiscoreNames  ORIG_VAR(0x00691330, HiscoreColumn, gHiscoreNames)

/* A high score record (20 bytes in the exe's tables): score, rider and the player's name. */
typedef struct HiscoreEntry {
    uint32_t score;
    uint8_t rider;
    uint8_t pad5[2];
    char name[13];
} HiscoreEntry;

_Static_assert(sizeof(HiscoreEntry) == 20, "HiscoreEntry layout");
/* Belongs to the hiscore code: entry `index` (record number in the low half, table in the high half). */
static inline const HiscoreEntry *hiscoreEntry(uint32_t index)
{
    return (const HiscoreEntry *)comdataGetLevelRecord(index);
}

/* Per level: string id of the title and picture of its high score, and per rider the rider's name. */
typedef struct LevelId {
    uint16_t id;
    uint16_t pad2;
} LevelId;
typedef LevelId LevelIdTable[12];
#define gHiscoreTitleIds   ORIG_VAR(0x0054d38c, LevelIdTable, gHiscoreTitleIds)
#define gHiscorePictureIds ORIG_VAR(0x0054db58, LevelIdTable, gHiscorePictureIds)
typedef uint16_t RiderNameTable[12];
#define gRiderNameIds  ORIG_VAR(0x0054d254, RiderNameTable, gRiderNameIds)
#define gHiscoreHandle ORIG_VAR(0x00691238, int, gHiscoreHandle) /* asset string set (zmemdmm handle) */
#define gHiscoreSprite ORIG_VAR(0x0069123c, ZPrimAsset *, gHiscoreSprite)
#define gHiscoreSlide  ORIG_VAR(0x006916f8, SlideTable, gHiscoreSlide)
#define gHiscoreBlink  ORIG_VAR(0x006916f2, uint16_t, gHiscoreBlink)
#define gHiscoreWait   ORIG_VAR(0x00690894, uint16_t, gHiscoreWait)
#define gAssetSetSize  ORIG_VAR(0x00690890, uint32_t, gAssetSetSize)
#define gAssetSetData  ORIG_VAR(0x0057464c, void *, gAssetSetData)
#define gAssetLibrary  ORIG_VAR(0x00678490, int, gAssetLibrary)
/* Factor and rounding term that turn a score into a time (floats in the exe). */
#define gScoreToTime ORIG_VAR(0x0054d0ec, float, gScoreToTime)
#define gScoreRound  ORIG_VAR(0x0054d074, float, gScoreRound)

/* Level info screen (series with 3 pages per level and 4 lines per page). */
typedef uint16_t InfoHeadTable[4];
typedef uint16_t InfoLineTable[6][3][4];
#define gInfoHeadIds ORIG_VAR(0x0055b130, InfoHeadTable, gInfoHeadIds)
#define gInfoLineIds ORIG_VAR(0x0055b138, InfoLineTable, gInfoLineIds)
#define gInfoSprite  ORIG_VAR(0x00691210, ZPrimAsset *, gInfoSprite)
#define gInfoPage    ORIG_VAR(0x00691218, uint16_t, gInfoPage)
#define gInfoWait    ORIG_VAR(0x00690898, uint16_t, gInfoWait)
#define gInfoSlide   ORIG_VAR(0x00691220, SlideTable, gInfoSlide)
#define gInfoTicks   ORIG_VAR(0x0069122c, uint16_t, gInfoTicks)

int extrasMenuInit(void);
int BMX_CDECL extrasMenuHook(int msg, GuiElement *el, int arg3, int arg4);
void extrasMenuExit(void);
void extrasMenuHighlight(void);
void extrasMenuSlideIn(void);
void creditsPlay(void);
void extrasMenuSlideOut(void);
void extrasMenuLayout(void);
void extrasMenuLayoutCopy(void);
void extrasPlay(void);
void extrasOpen(void);
void extrasShowMenu(void);
void extrasSlideIn(void);
void extrasSlideOut(void);
int BMX_CDECL extrasListDraw(int msg, GuiElement *el, int arg3, int arg4);
int BMX_CDECL extrasHook(int msg, GuiElement *el, int arg3, int arg4);
void hiscoreOpen(void);
void hiscoreClose(void);
void hiscoreBuild(void);
void hiscoreSlideIn(void);
void hiscoreSlideOut(void);
int BMX_CDECL hiscoreHook(int msg, GuiElement *el, int arg3, int arg4);
void infoOpen(void);
void infoClose(void);
void infoBuild(void);
void infoSlideIn(void);
void infoSlideOut(void);
int BMX_CDECL infoHook(int msg, GuiElement *el, int arg3, int arg4);

/* ---- start-up (frontend_extras.c) ----------------------------------------------------------------- */
/* The macro string set (0x57465c: 16 KiB buffer, 0x574660: size of the set in it). */
#define gMacroBuf  ORIG_VAR(0x0057465c, void *, gMacroBuf)
#define gMacroSize ORIG_VAR(0x00574660, uint32_t, gMacroSize)
#define gExitFlag  ORIG_VAR(0x00574654, int, gExitFlag)
#define gExitCode  ORIG_VAR(0x00574658, int, gExitCode)
/* Other start-up calls (engine side, not ported yet). */
ORIG_FN(0x0046cbc0, void, appInitCardA, (void));
ORIG_FN(0x0046cc50, void, appInitCardB, (int arg));
ORIG_FN(0x0046c310, void, appInitCardC, (void));
ORIG_FN(0x0046d3a0, void, appInitCardD, (int arg));
ORIG_FN(0x0046d3c0, void, appInitCardE, (int arg));

int frontendAllocMacros(void);
int frontendInitApp(void);
int frontendSetLanguage(uint32_t language);

#endif
