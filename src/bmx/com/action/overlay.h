/*
 * In-game overlay (C:\zaxis\Bmx\Src\com\action\overlay.c): the HUD elements (score, timer, height and
 * balance meters, captions, hints) and the menu screens (pause, options, sound, trick list, goals, name
 * entry, scoreboards, saving).
 *
 * The overlay has one current screen (gOvlScreen: three callbacks and an item list) and a table of 17
 * HUD elements (gOvlElements). Texts and rectangles are overlays of the zoverii layer, addressed by
 * 16 bit handles (0xffff = none) kept in small arrays of globals; each screen owns a slice of them.
 */
#ifndef BMX_COM_ACTION_OVERLAY_H
#define BMX_COM_ACTION_OVERLAY_H

#include <stddef.h>
#include <stdint.h>

#include "bmx/com/action/physics_types.h"
#include "compat/orig.h"
#include "zge/zstring.h"

#define kOvlNoHandle 0xffffu

/* A HUD element. flags bit 0 set = hidden. Any callback may be NULL. */
typedef struct OvlElement {
    uint32_t flags;                           /* 0x00 */
    void(BMX_CDECL *init)(int create);        /* 0x04 create (1) or destroy (0) the element's overlays */
    void(BMX_CDECL *draw)(void);              /* 0x08 per frame */
    void(BMX_CDECL *show)(int show);          /* 0x0c */
    void(BMX_CDECL *setA)(const void *value); /* 0x10 */
    void(BMX_CDECL *setB)(void *value);       /* 0x14 */
} OvlElement;

/* A menu screen. */
typedef struct OvlScreen {
    void(BMX_CDECL *enter)(int enter); /* 0x00 create (1) or destroy (0) the screen's overlays */
    int(BMX_CDECL *update)(void);      /* 0x04 per frame input; non-zero when it handled a press */
    void(BMX_CDECL *draw)(void);       /* 0x08 per frame (NULL: the selected item pulses instead) */
    uint8_t itemCount;                 /* 0x0c */
    uint8_t selected;                  /* 0x0d */
    uint8_t pad0e[2];
} OvlScreen;

/* A font record as parsed from the font resources (only the fields the overlay reads). */
typedef struct OvlFont {
    void *image;       /* 0x00 pointer to the font's image reference (a u16 resource index) */
    uint8_t pad04;     /* 0x04 */
    uint8_t height;    /* 0x05 */
    int8_t firstChar;  /* 0x06 */
    uint8_t charCount; /* 0x07 */
    void *glyphs;      /* 0x08 */
} OvlFont;

/* An item of a dynamic menu: its action and the string id of its caption. */
typedef struct OvlMenuItem {
    uint16_t action;
    uint16_t text;
} OvlMenuItem;

/* A message with a frame countdown (hint, goal and flash texts); id 0xff = none. */
typedef struct OvlTimedMsg {
    uint8_t id;
    uint8_t arg;
    uint16_t timer;
} OvlTimedMsg;

/* The status message additionally carries a number. */
typedef struct OvlStatusMsg {
    OvlTimedMsg msg;
    uint32_t value;
} OvlStatusMsg;

/* The marker element: which marker (0..4) and a frame counter for its blinking. */
typedef struct OvlIndicator {
    uint8_t id;
    uint8_t pad1;
    uint16_t counter;
} OvlIndicator;

/* The full screen colour fade (element 16): current colour, per frame step, frames left. */
typedef struct OvlFade {
    uint8_t
        flags; /* 0x00 bit 0: fade out from the colour, else fade in from black; bit 1 selects the blend */
    uint8_t pad01[3];
    uint16_t ticks; /* 0x04 */
    uint8_t pad06[2];
    float r, g, b;    /* 0x08 */
    float dr, dg, db; /* 0x14 */
} OvlFade;

/* A page of the save screen: how many items it has and the string id of the first one. */
typedef struct OvlPageDef {
    uint8_t count;
    uint8_t pad01;
    uint16_t textBase;
} OvlPageDef;

/* Profile slot of a player (0x4c bytes each at 0x6952a0). */
typedef struct OvlProfile {
    uint8_t flags;      /* 0x00 bit 1 toggled by the second option of screen 2 */
    uint8_t controller; /* 0x01 controller index */
    uint8_t pad02;
    uint8_t character; /* 0x03 rider */
    uint8_t pad04[2];
    uint8_t option; /* 0x06 first option of screen 2 */
    char name[0x45];
} OvlProfile;

BMX_LAYOUT32_ASSERT(sizeof(OvlElement) == 0x18, "OvlElement size");
BMX_LAYOUT32_ASSERT(sizeof(OvlScreen) == 0x10, "OvlScreen size");
_Static_assert(sizeof(OvlProfile) == 0x4c, "OvlProfile size");
_Static_assert(sizeof(OvlFade) == 0x20, "OvlFade size");
_Static_assert(sizeof(OvlTimedMsg) == 4, "OvlTimedMsg size");
_Static_assert(sizeof(OvlStatusMsg) == 8, "OvlStatusMsg size");
_Static_assert(sizeof(OvlPageDef) == 4, "OvlPageDef size");
BMX_LAYOUT32_ASSERT(offsetof(OvlScreen, itemCount) == 0xc && offsetof(OvlScreen, selected) == 0xd,
                    "OvlScreen fields");
BMX_LAYOUT32_ASSERT(offsetof(OvlElement, setB) == 0x14, "OvlElement fields");
BMX_LAYOUT32_ASSERT(offsetof(OvlFont, glyphs) == 8 && offsetof(OvlFont, height) == 5, "OvlFont fields");
_Static_assert(offsetof(OvlFade, ticks) == 4 && offsetof(OvlFade, db) == 0x1c, "OvlFade fields");
_Static_assert(offsetof(OvlProfile, name) == 7 && offsetof(OvlProfile, option) == 6, "OvlProfile fields");

/* Per rider unlock data at 0x6955e8, 0x104 bytes each. */
enum {
    kOvlRiderStride = 0x104,
    kOvlRiderTrickBits = 0xf7, /* bitmap of the tricks the rider has landed */
};

/*
 * The player records at 0x691f40 are the riders (PhysRider, gPhysRiders); the overlay reads them by these
 * byte offsets, all before the record's first pointer (asserted in physics_types.h), so they hold on every
 * build.
 */
enum {
    kOvlPlayerStride = sizeof(PhysRider),
    kOvlPlayerNameLen = 0x05,
    kOvlPlayerName = 0x06, /* button glyph codes */
    kOvlPlayerReady = 0x5c,
    kOvlPlayerScore = 0x60,
    kOvlPlayerNameCount = 0x68,
    kOvlPlayerUnlockMask = 0x3c0,
    kOvlPlayerRunTimes = 0x3c8, /* 12 bytes per run, six time cells first */
};

typedef uint16_t OvlHandles24[24];
typedef uint16_t OvlHandles21[21];
typedef uint16_t OvlHandles7[7];
typedef uint16_t OvlHandles6[6];
typedef uint16_t OvlHandles4[4];
typedef uint16_t OvlHandles3[3];
typedef uint16_t OvlHandleGrid[10][4];
typedef uint16_t OvlGoalRows[5][4];
typedef OvlMenuItem OvlMenuItems7[7];
typedef OvlMenuItem OvlMenuItems10[10];
typedef uint8_t OvlTrickList[0x54][2];
typedef const char *OvlStrTable[64];
typedef const char *OvlStrTable5[5];
typedef const char *OvlStrTable4[4];
typedef uint32_t OvlColors5[5];
typedef uint8_t OvlGoalEntry[2];
typedef OvlGoalEntry OvlGoalEntries[0x40];
typedef uint8_t OvlBytes16[16];
typedef uint8_t OvlBytes32[0x20];
typedef uint8_t OvlBytes128[0x80];
typedef uint8_t OvlBytes256[0x100];
typedef OvlElement OvlElementTable[17];
typedef OvlScreen *OvlScreenTable[0x1b];
typedef OvlPageDef OvlPageDefs[12];
typedef const ZStringSet *OvlStringSets[2];
typedef uint8_t OvlVolumes[4];
typedef OvlProfile OvlProfiles[2];
typedef uint8_t OvlPlayerRecs[2][kOvlPlayerStride];
typedef uint8_t OvlRiderData[12][kOvlRiderStride];
typedef uint8_t OvlCareerRec[3];
typedef OvlCareerRec OvlCareer[12];
typedef uint8_t OvlCareerUnlocked[3];
typedef uint16_t OvlTrickView[2];
typedef char OvlNameBuf[16];
typedef char OvlText8[8];
typedef char OvlText4[4];
typedef uint32_t OvlSize2[2];

/* Element and screen tables. */
#define gOvlElements ORIG_VAR(0x00556f00, OvlElementTable, gOvlElements)
#define gOvlScreens  ORIG_VAR(0x005571b8, OvlScreenTable, gOvlScreens)

/* Current screen state. */
#define gOvlScreen       ORIG_VAR(0x0056bf20, OvlScreen *, gOvlScreen)
#define gOvlScreenId     ORIG_VAR(0x0056beba, uint8_t, gOvlScreenId)
#define gOvlReturnScreen ORIG_VAR(0x0056be32, uint8_t, gOvlReturnScreen)
#define gOvlLastScreen   ORIG_VAR(0x0056bd60, uint8_t, gOvlLastScreen)
#define gOvlFrame        ORIG_VAR(0x0056be14, uint32_t, gOvlFrame)
#define gOvlPulse        ORIG_VAR(0x0056bf68, uint32_t, gOvlPulse)
#define gOvlDrawDelay    ORIG_VAR(0x0056bfa4, uint32_t, gOvlDrawDelay)
#define gOvlSharedPad    ORIG_VAR(0x0056be3c, uint32_t, gOvlSharedPad) /* 0: every pad drives the menu */
#define gOvlPage         ORIG_VAR(0x0056bf30, uint8_t, gOvlPage)
#define gOvlSaving       ORIG_VAR(0x006966dc, uint32_t, gOvlSaving)
#define gOvlSaveStep     ORIG_VAR(0x00556efc, int32_t, gOvlSaveStep)
#define gOvlSaveSlot     ORIG_VAR(0x0056c074, uint8_t, gOvlSaveSlot)
#define gOvlSavePages    ORIG_VAR(0x00557224, OvlPageDefs, gOvlSavePages)
#define gOvlSavedSubMode ORIG_VAR(0x00556ef8, int8_t, gOvlSavedSubMode)
#define gOvlBackdrop     ORIG_VAR(0x0056be2a, uint8_t, gOvlBackdrop)
#define gOvlBackdropRect ORIG_VAR(0x0056bef8, uint16_t, gOvlBackdropRect)
#define gOvlWarnLevel    ORIG_VAR(0x0056b617, uint8_t, gOvlWarnLevel)
#define gOvlStrings      ORIG_VAR(0x0056be34, OvlStringSets, gOvlStrings)
#define gOvlLibrary      ORIG_VAR(0x00678490, struct ZassetLibrary *, gOvlLibrary)
#define gOvlEmpty        ORIG_VAR(0x0056bd44, OvlText4, gOvlEmpty) /* "" */

/* Game state read by the overlay. */
#define gCurrentLevel    ORIG_VAR(0x0056b614, uint8_t, gCurrentLevel)
#define gCurrentPlayer   ORIG_VAR(0x0056b615, uint8_t, gCurrentPlayer)
#define gPlayerCount     ORIG_VAR(0x0069621c, uint8_t, gPlayerCount)
#define gDemoPlayer      ORIG_VAR(0x006962f5, uint8_t, gDemoPlayer)
#define gSoundVolumes    ORIG_VAR(0x006955e4, OvlVolumes, gSoundVolumes) /* music, effects, voice set */
#define gOptionFlags     ORIG_VAR(0x006955e0, uint8_t, gOptionFlags)
#define gProfiles        ORIG_VAR(0x006952a0, OvlProfiles, gProfiles)
#define gPlayerRecs      ORIG_VAR(0x00691f40, OvlPlayerRecs, gPlayerRecs)
#define gRiderData       ORIG_VAR(0x006955e8, OvlRiderData, gRiderData)
#define gCareer          ORIG_VAR(0x00695200, OvlCareer, gCareer)
#define gCareerUnlocked  ORIG_VAR(0x0069521e, OvlCareerUnlocked, gCareerUnlocked)
#define gScoreRecordSlot ORIG_VAR(0x00553658, OvlBytes16, gScoreRecordSlot)

/* Tables (.data). */
#define gTrickTurnNames ORIG_VAR(0x00559390, OvlStrTable, gTrickTurnNames)
/* The barspin names are the turn table's entries from 46 on (0x00559448). */
#define gTrickBarspinNames (&gTrickTurnNames[46])
#define gTrickSpinNames    ORIG_VAR(0x005594e0, OvlStrTable, gTrickSpinNames)
#define gTrickDoubleNames  ORIG_VAR(0x00557308, OvlStrTable4, gTrickDoubleNames)
#define gTrickClassHi      ORIG_VAR(0x00558fa2, OvlBytes128, gTrickClassHi)
#define gTrickClassLo      ORIG_VAR(0x00558fb4, OvlBytes32, gTrickClassLo)
#define gGoalEntriesA      ORIG_VAR(0x005568b8, OvlGoalEntries, gGoalEntriesA)
#define gGoalEntriesB      ORIG_VAR(0x00556948, OvlGoalEntries, gGoalEntriesB)
#define gGoalEntriesC      ORIG_VAR(0x0055695c, OvlGoalEntries, gGoalEntriesC)
#define gGoalEntriesD      ORIG_VAR(0x00556968, OvlGoalEntries, gGoalEntriesD)
#define gGoalColumns       ORIG_VAR(0x00556998, OvlBytes256, gGoalColumns)
#define gButtonGlyphs      ORIG_VAR(0x005572cb, OvlBytes16, gButtonGlyphs)
#define gHintGlyphs        ORIG_VAR(0x005572bb, OvlBytes256, gHintGlyphs)
#define gIndicatorText     ORIG_VAR(0x005572d4, OvlStrTable5, gIndicatorText)
#define gIndicatorColor    ORIG_VAR(0x005572e8, OvlColors5, gIndicatorColor)
#define gOvlBarSize        ORIG_VAR(0x00557260, OvlSize2, gOvlBarSize)
#define gOvlFadeSize       ORIG_VAR(0x00557298, OvlSize2, gOvlFadeSize)

/* Fonts and their image references. */
#define gOvlFontSmall  ORIG_VAR(0x0056bf38, OvlFont, gOvlFontSmall)
#define gOvlFontTiny   ORIG_VAR(0x0056be98, OvlFont, gOvlFontTiny)
#define gOvlFontIcons  ORIG_VAR(0x0056bf98, OvlFont, gOvlFontIcons)
#define gOvlFontButton ORIG_VAR(0x0056c008, OvlFont, gOvlFontButton)
#define gOvlImgSmall   ORIG_VAR(0x0056bef4, uint16_t, gOvlImgSmall)
#define gOvlImgTiny    ORIG_VAR(0x0056be28, uint16_t, gOvlImgTiny)
#define gOvlImgIcons   ORIG_VAR(0x0056be92, uint16_t, gOvlImgIcons)
#define gOvlImgButton  ORIG_VAR(0x0056bffa, uint16_t, gOvlImgButton)
#define gOvlResA       ORIG_VAR(0x0056be30, uint16_t, gOvlResA)
#define gOvlResB       ORIG_VAR(0x006951e8, uint16_t, gOvlResB)

/* Overlay handles and state of the menu screens. */
#define gOvlHandles    ORIG_VAR(0x0056bebc, OvlHandles24, gOvlHandles)
#define gOvlMenuItems  ORIG_VAR(0x0056befc, OvlMenuItems7, gOvlMenuItems)
#define gOvlMenuItems2 ORIG_VAR(0x0056c028, OvlMenuItems10, gOvlMenuItems2)
#define gOvlVolHandles ORIG_VAR(0x0056bea4, OvlHandles7, gOvlVolHandles)
#define gOvlTrickList  ORIG_VAR(0x0056bd6c, OvlTrickList, gOvlTrickList)
#define gOvlTrickCount ORIG_VAR(0x0056bd68, uint32_t, gOvlTrickCount)
#define gOvlTrickRows  ORIG_VAR(0x0056bf6c, OvlHandles21, gOvlTrickRows)
#define gOvlTrickDesc  ORIG_VAR(0x0056bf94, uint16_t, gOvlTrickDesc)
#define gOvlTrickView  ORIG_VAR(0x0056c070, OvlTrickView, gOvlTrickView) /* first visible row, cursor */
#define gOvlGoalRows   ORIG_VAR(0x0056bec4, OvlGoalRows, gOvlGoalRows)
#define gOvlRunTimes   ORIG_VAR(0x0056be40, OvlHandleGrid, gOvlRunTimes)
#define gOvlNameBuf    ORIG_VAR(0x0056bfb4, OvlNameBuf, gOvlNameBuf)
#define gOvlNameCursor ORIG_VAR(0x0056bf64, uint8_t, gOvlNameCursor)

/* HUD element state. */
#define gOvlFade            ORIG_VAR(0x0056c050, OvlFade, gOvlFade)
#define gOvlScoreHandle     ORIG_VAR(0x0056be2c, uint16_t, gOvlScoreHandle)
#define gOvlScoreShown      ORIG_VAR(0x0056bd50, uint32_t, gOvlScoreShown)
#define gOvlScoreTarget     ORIG_VAR(0x0056bd58, uint32_t, gOvlScoreTarget)
#define gOvlScoreFlags      ORIG_VAR(0x0056bd5c, uint8_t, gOvlScoreFlags)
#define gOvlTimerHandle     ORIG_VAR(0x0056c000, uint16_t, gOvlTimerHandle)
#define gOvlTimerText       ORIG_VAR(0x0056bfac, OvlText8, gOvlTimerText)
#define gOvlTimerTicks      ORIG_VAR(0x0056bf58, uint32_t, gOvlTimerTicks)
#define gOvlTimerShownTicks ORIG_VAR(0x0056bf54, uint32_t, gOvlTimerShownTicks)
#define gOvlHeightHandles   ORIG_VAR(0x0056bf24, OvlHandles6, gOvlHeightHandles)
#define gOvlHeightValue     ORIG_VAR(0x0056bfc0, float, gOvlHeightValue)
#define gOvlHeightTarget    ORIG_VAR(0x0056bfc4, float, gOvlHeightTarget)
#define gOvlHeightExtra     ORIG_VAR(0x0056bfc8, uint32_t, gOvlHeightExtra)
#define gOvlHeightKind      ORIG_VAR(0x0056bfca, uint16_t, gOvlHeightKind)
#define gOvlHeightScroll    ORIG_VAR(0x0056bd48, float, gOvlHeightScroll)
#define gOvlBalanceHandles  ORIG_VAR(0x0056bfec, OvlHandles7, gOvlBalanceHandles)
#define gOvlBalanceValue    ORIG_VAR(0x0056bf18, float, gOvlBalanceValue)
#define gOvlCaptionHandle   ORIG_VAR(0x0056bffc, uint16_t, gOvlCaptionHandle)
#define gOvlCaptionCurrent  ORIG_VAR(0x0056beec, uint8_t, gOvlCaptionCurrent)
#define gOvlCaptionWanted   ORIG_VAR(0x0056beed, uint8_t, gOvlCaptionWanted)
#define gOvlElement8Value   ORIG_VAR(0x0056bfd9, uint8_t, gOvlElement8Value)
#define gOvlIndicator       ORIG_VAR(0x0056bf5c, OvlIndicator, gOvlIndicator)
#define gOvlHint            ORIG_VAR(0x0056bf44, OvlTimedMsg, gOvlHint)
#define gOvlHintHandles     ORIG_VAR(0x0056bfdc, OvlHandles7, gOvlHintHandles)
#define gOvlGoalMsg         ORIG_VAR(0x0056bf4c, OvlTimedMsg, gOvlGoalMsg)
#define gOvlGoalHandles     ORIG_VAR(0x0056beb4, OvlHandles3, gOvlGoalHandles)
#define gOvlStatus          ORIG_VAR(0x0056c018, OvlStatusMsg, gOvlStatus)
#define gOvlStatusHandle    ORIG_VAR(0x0056bfa8, uint16_t, gOvlStatusHandle)
#define gOvlFlash           ORIG_VAR(0x0056be18, OvlTimedMsg, gOvlFlash)
#define gOvlFlashHandles    ORIG_VAR(0x0056be20, OvlHandles4, gOvlFlashHandles)
#define gOvlIconHandle      ORIG_VAR(0x0056be90, uint16_t, gOvlIconHandle)

/* Setup, shutdown and per frame entry points. */
void ovlInit(void);
void ovlShutdown(void);
void ovlLoadFonts(void);
void ovlLoadStrings(uint32_t language);
int ovlDraw(void);
int ovlUpdate(void);

/* Screens and elements. */
void ovlSetElementVisible(int index, uint32_t show);
void ovlSetElementA(int index, const void *value);
void ovlSetElementB(int index, void *value);
uint32_t ovlGetScreenId(void);
void ovlSetScreen(uint32_t id);
void ovlDrawTinyText(const char *text, int x, int y, uint32_t color);
void ovlResetScreenState(uint32_t page);
void ovlSetBackdrop(uint32_t flag);
void ovlDrawMenuItems(uint32_t flags, int offset);
int ovlPadConfirm(uint32_t minItem);
uint32_t ovlPulseColor(void);
void ovlFormatFeetInches(char *dst, float feet);
void ovlFormatTrickName(char *out, const uint8_t *trick, int withExtras);

void ovlDrawPlayerNames(int *y);
void ovlDrawRunTimes(int *y);
void ovlAddGoalLine(int player, const uint8_t *goal, int *handle, int *y, int centred);
uint32_t ovlTrickAvailable(uint32_t trick);
void ovlSetSaveStep(int a, int step);

void ovlScreen1Enter(int create);
int ovlScreen1Update(void);
void ovlScreen2Enter(int create);
int ovlScreen2Update(void);
void ovlScreen2Refresh(void);
void ovlScreen3Enter(int create);
int ovlScreen3Update(void);
void ovlScreen3Refresh(void);
void ovlScreen3Draw(void);
void ovlScreen4Enter(int create);
int ovlScreen4Update(void);
void ovlScreen4Draw(void);
void ovlScreen4Refresh(void);
void ovlScreen7Enter(int create);
int ovlScreen7Update(void);
/* Screen 8 draw: the competition results (judges' scores of each run). */
void ovlScreen8Draw(void);
int ovlScreen8Update(void);
void ovlScreen9Draw(void);
int ovlScreen9Update(void);
void ovlScreen10Enter(int create);
void ovlScreen10Draw(void);
int ovlScreen10Update(void);
void ovlScreen11Enter(int create);
int ovlScreen11Update(void);
void ovlScreen11Draw(void);
void ovlScreen11Refresh(void);
void ovlScreen12Enter(int create);
int ovlScreen12Update(void);
void ovlScreen12Draw(void);
void ovlScreen13Enter(int create);
int ovlScreen13Update(void);
void ovlScreen13Draw(void);
void ovlScreen16Enter(int create);
void ovlScreen16Refresh(uint32_t page);
int ovlScreen16Update(void);
void ovlScreen16Draw(void);
void ovlScreen17Enter(int create);
int ovlScreen17Update(void);
void ovlScreen17Draw(void);
void ovlScreen18Enter(int create);
int ovlScreen18Update(void);
void ovlScreen18Draw(void);
int ovlScreen25Update(void);

void ovlLetterboxDraw(void);
void ovlFadeDraw(void);
void ovlFadeSet(const uint8_t *fade);
void ovlScoreInit(int create);
void ovlScoreDraw(void);
void ovlScoreShow(int show);
void ovlScoreSet(const uint32_t *value);
void ovlTimerInit(int create);
void ovlTimerDraw(void);
void ovlTimerShow(int show);
void ovlTimerSet(const uint32_t *ticks);
void ovlNameDraw(void);
void ovlComboDraw(void);
void ovlHeightInit(int create);
void ovlHeightDraw(void);
void ovlHeightShow(int show);
void ovlHeightSet(const float *value);
void ovlBalanceInit(int create);
void ovlBalanceDraw(void);
void ovlBalanceShow(int show);
void ovlBalanceSet(const float *value);
void ovlCaptionInit(int create);
void ovlCaptionDraw(void);
void ovlCaptionShow(int show);
void ovlCaptionSet(const uint8_t *id);
void ovlElement8Init(int create);
void ovlElement8Draw(void);
void ovlElement8Show(int show);
void ovlElement8Set(const uint8_t *value);
void ovlIndicatorInit(int create);
void ovlIndicatorDraw(void);
void ovlIndicatorShow(int show);
void ovlIndicatorSet(const uint32_t *value);
void ovlHintInit(int create);
void ovlHintDraw(void);
void ovlHintShow(int show);
void ovlHintSet(const uint32_t *value);
void ovlHintGet(uint32_t *out);
void ovlGoalMsgInit(int create);
void ovlGoalMsgDraw(void);
void ovlGoalMsgShow(int show);
void ovlGoalMsgSet(const uint32_t *value);
void ovlGoalMsgGet(uint32_t *out);
void ovlStatusMsgInit(int create);
void ovlStatusMsgDraw(void);
void ovlStatusMsgShow(int show);
void ovlStatusMsgSet(const uint32_t *value);
void ovlStatusMsgGet(uint32_t *out);
void ovlFlashInit(int create);
void ovlFlashDraw(void);
void ovlFlashShow(int show);
void ovlFlashSet(const uint32_t *value);
void ovlIconInit(int create);
void ovlIconShow(int show);

#endif
