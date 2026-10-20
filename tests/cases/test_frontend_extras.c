/* Tests for frontend_extras.c: the extras hub, movie list, high scores, level information, credits and
 * start-up. */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_frontend.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

/* The exe's high score records (20 bytes each) the accessor hands out. */
typedef HiscoreEntry HiscoreTable[57];
#define gHiscoreTable ORIG_VAR(0x00551fd8, HiscoreTable, gHiscoreTable)

static void BMX_CDECL stubDrawFrame(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color, int full)
{
    dtFrontLog(0x210, (uint16_t)x0 | (uint32_t)(uint16_t)y0 << 16,
               (uint16_t)x1 | (uint32_t)(uint16_t)y1 << 16, color, (uint32_t)(full != 0));
}

static void BMX_CDECL stubMainMenuInit(int skipLoad)
{
    dtFrontLog(0x211, (uint32_t)skipLoad, 0, 0, 0);
}

static int BMX_CDECL stubSelScreenOpen(void)
{
    dtFrontLog(0x212, 0, 0, 0, 0);
    return 0;
}

static void BMX_CDECL stubAutoloadInit(void)
{
    dtFrontLog(0x213, 0, 0, 0, 0);
}

static void BMX_CDECL stubSetProp(uint16_t prop, int value)
{
    dtFrontLog(0x216, prop, (uint32_t)value, 0, 0);
}

static int BMX_CDECL stubLoadGroup(uint16_t slot, uint16_t group)
{
    dtFrontLog(0x217, slot, group, 0, 0);
    return 0;
}

static void BMX_CDECL stubFreeGroup(uint16_t slot)
{
    dtFrontLog(0x218, slot, 0, 0, 0);
}

enum {
    kUiStubs = 1u << 24, /* frame, zprimSet, group load/free */
    kUiGroups =
        DT_FS_SOUND | DT_FS_SCREENS | DT_FS_LABEL | DT_FS_ASSET | DT_FS_EXTRAS | DT_FS_FRAME | kUiStubs,
};

/* The elements of the hub screen, and the random state every case starts from. */
static void extrasBase(DtCtx *c, uint32_t groups)
{
    dt_seed(c);
    dtFrontendReset();
    dtFrontendStub(groups);
    if ((groups & kUiStubs) != 0) {
        dt_stub(0x0043bf30, menuDrawFrame, (void *)stubDrawFrame);
        dt_stub(0x00478050, zprimSet, (void *)stubSetProp);
        dt_stub(0x0047b400, zprimLoadGroup, (void *)stubLoadGroup);
        dt_stub(0x0047b5c0, zprimFreeGroup, (void *)stubFreeGroup);
    }
    dt_stub(0x0043fa20, mainMenuInit, (void *)stubMainMenuInit);
    dtFrontendElementRange(0xa00d0001, 0x17);
    gFrontendStep = dt_u32() % 3 == 0 ? NULL : dtFrontStep;
    gExtrasChoice = dt_u32() % 8 == 0 ? dt_range(-3, 6) : dt_range(-1, 3);
    gExtrasReady = (int)(dt_u32() % 2);
    gExtrasPulse = dt_range(-0x10, 0x50);
    gExtrasPulseStep = dt_u32() % 2 == 0 ? dt_range(1, 4) : -dt_range(1, 4);
    gExtrasWait = (uint16_t)(dt_u32() % 3 == 0 ? dt_range(1, 3) : 0);
    gExtrasWait2 = (uint16_t)(dt_u32() % 3 == 0 ? dt_range(1, 3) : 0);
    gMenuInstant = (int)(dt_u32() % 3 == 0 ? dt_u32() | 1 : 0);
    gMenuSlide0 = dt_range(-0x48, 0x30);
    gMenuSlide1 = dt_range(-0x48, 0x30);
    gMenuSlide2 = dt_range(-0x48, 0x30);
    gMenuSlide3 = dt_range(-0x48, 0x30);
    gGuiPulse = dt_range(0x10, 0xf0);
    gLanguageCurrent = dt_u32();
    gSoundVolume0 = (uint8_t)dt_u32();
    gSoundVolume1 = (uint8_t)dt_u32();
    gMenuFrameSprite = (ZPrimAsset *)(uintptr_t)(dt_u32() % 2 == 0 ? 0 : 0x44000000u + (dt_u32() & 0xff0));
    gExtrasTitles[0] = &gTitleIntro;
    gExtrasTitles[1] = &gTitleDave;
    gExtrasTitles[2] = &gTitleRyan;
    gExtrasTitles[3] = &gTitleAmish;
    gExtrasCount = (uint16_t)(dt_u32() % 10 == 0 ? dt_range(0, 4) : dt_range(1, 4));
    gExtrasSel = (uint16_t)(dt_u32() % 8 == 0 ? dt_range(0, 3)
                                              : dt_range(0, gExtrasCount > 0 ? (int)gExtrasCount - 1 : 0));
    gExtrasPlayed = dt_u32() % 2 == 0 ? 0 : dt_u32() | 1;
    gUnlockDave = (uint8_t)dt_u32();
    gUnlockRyan = (uint8_t)dt_u32();
    gUnlockAmish = (uint8_t)dt_u32();
}

/* A slide table: values around the thresholds of the animation. */
static void randomSlide(int16_t *slide, int top)
{
    for (int i = 0; i < 4; i++) {
        switch (dt_u32() % 4) {
        case 0:
            slide[i] = (int16_t)dt_range(-40, 20);
            break;
        case 1:
            slide[i] = (int16_t)(top - dt_range(0, 20));
            break;
        case 2:
            slide[i] = (int16_t)dt_range(0x100, 0x1d9);
            break;
        default:
            slide[i] = (int16_t)dt_range(-600, 600);
            break;
        }
    }
}

/* High score records: random scores (with ties), riders below 12 and short names. */
static void randomHiscores(void)
{
    static const uint32_t kScores[] = {0, 1, 5, 12, 100, 1000, 36000, 0x7fffffffu, 0x10000000u};
    for (int i = 0; i < 57; i++) {
        HiscoreEntry *e = &gHiscoreTable[i];
        e->score =
            dt_u32() % 4 == 0 ? kScores[dt_u32() % (sizeof kScores / sizeof kScores[0])] : dt_u32() % 100000u;
        e->rider = (uint8_t)dt_range(0, 11);
        e->pad5[0] = (uint8_t)dt_u32();
        e->pad5[1] = (uint8_t)dt_u32();
        const int length = dt_range(0, 12);
        for (int k = 0; k < 13; k++) {
            e->name[k] = k < length ? (char)dt_range('A', 'z') : '\0';
        }
    }
}

/* Gives the level titles a '-' to split at (sometimes first, sometimes none) in the string table. */
static void randomTitles(void)
{
    static const char *const kTitles[] = {"Big Air - Contest", "Park", "- Sub", "A - B - C",
                                          "Street Jam - Day 2"};
    const ZStringSet *set = gZstringSets[0];
    for (int level = 0; level < 12; level++) {
        if (dt_u32() % 3 == 0) {
            continue;
        }
        char *text = (char *)set + set->offsets[gHiscoreTitleIds[level].id];
        const char *title = kTitles[dt_u32() % (sizeof kTitles / sizeof kTitles[0])];
        size_t n = 0;
        for (; title[n] != '\0'; n++) {
            text[n] = title[n];
        }
        text[n] = '\0';
    }
}

/* An asset string set (registered, in a locked block) as the open functions leave it, or nothing. */
static void randomAssetSet(void)
{
    if (dt_u32() % 4 == 0) {
        gAssetSetData = NULL;
        gHiscoreHandle = 0;
        return;
    }
    uint32_t size = 0;
    const uint8_t *blob = dtFrontendAssetBlob(&size);
    gAssetSetSize = size;
    gHiscoreHandle = zmemdmmAlloc(size);
    gAssetSetData = zmemdmmLock(gHiscoreHandle);
    memcpy(gAssetSetData, blob, size);
    zstringRegister((const ZStringSet *)gAssetSetData);
}

static void hiscoreBase(DtCtx *c)
{
    extrasBase(c, kUiGroups);
    randomHiscores();
    randomTitles();
    gSelLevelSetting = (uint8_t)dt_range(0, 11);
    randomSlide(gHiscoreSlide, 0x1d6);
    gHiscoreBlink = (uint16_t)dt_u32();
    gHiscoreWait = (uint16_t)dt_range(0, 3);
    gHiscoreSprite = (ZPrimAsset *)(uintptr_t)(0x58000000u + (dt_u32() & 0xff0));
    gAssetLibrary = dt_range(0, 5);
    if (dt_u32() % 2 == 0) {
        const int sizes[2] = {0, 1};
        dtFrontQueue(DT_Q_ASSETSIZE, sizes, 1 + (int)(dt_u32() % 2));
    }
}

static void infoBase(DtCtx *c)
{
    extrasBase(c, kUiGroups);
    randomHiscores();
    randomTitles();
    gSelLevelSetting = (uint8_t)dt_range(0, 5);
    gInfoPage = (uint16_t)dt_range(0, 2);
    randomSlide(gInfoSlide, 0x1d6);
    gInfoSprite = (ZPrimAsset *)(uintptr_t)(0x59000000u + (dt_u32() & 0xff0));
    gInfoTicks = (uint16_t)dt_u32();
    gAssetLibrary = dt_range(0, 5);
    if (dt_u32() % 2 == 0) {
        const int sizes[2] = {0, 1};
        dtFrontQueue(DT_Q_ASSETSIZE, sizes, 1 + (int)(dt_u32() % 2));
    }
}

/* Hook arguments: a message from the list; draw messages go to one of the given elements most of the time. */
static void pickHookArgs(DtCtx *c, const int *msgs, int count, const uint32_t *drawIds, int drawCount)
{
    int msg = msgs[dt_u32() % (uint32_t)count];
    if (dt_u32() % 10 == 0) {
        msg = dt_range(0, 0x40);
    }
    GuiElement *el = guiFindById(0xa00d0001 + dt_u32() % 0x17);
    if (msg == 0xc && dt_u32() % 8 != 0) {
        el = guiFindById(drawIds[dt_u32() % (uint32_t)drawCount]);
    }
    DT_ARG_INT(c, 0, msg);
    DT_ARG_PTR(c, 1, el);
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

static const int kNavMsgs[] = {0x1e, 0x1f, 0x20, 0x21, 0x2b, 0x2c, 0x2d, 0x2e,
                               0x22, 0x24, 0x2f, 0x31, 0x25, 0x32, 0x16, 0xc};

/* ---- movie list ---------------------------------------------------------------------------------- */

static void setup_state(DtCtx *c)
{
    extrasBase(c, kUiGroups);
}

static void setup_slides(DtCtx *c)
{
    extrasBase(c, kUiGroups);
    randomSlide(gExtrasSlide, 0x1cc);
}

static void setup_list_draw(DtCtx *c)
{
    static const uint32_t ids[] = {0xa00d0007, 0xa00d0007, 0xa00d0007, 0xa00d0003};
    static const int msgs[] = {0xc, 0xc, 0xc, 0xd, 0x16};
    extrasBase(c, kUiGroups);
    randomSlide(gExtrasSlide, 0x1cc);
    pickHookArgs(c, msgs, (int)(sizeof msgs / sizeof msgs[0]), ids, 4);
}

static void setup_extras_hook(DtCtx *c)
{
    static const uint32_t ids[] = {0xa00d0007};
    extrasBase(c, kUiGroups);
    pickHookArgs(c, kNavMsgs, (int)(sizeof kNavMsgs / sizeof kNavMsgs[0]), ids, 1);
}

/* ---- high scores ---------------------------------------------------------------------------------- */

static void setup_hiscore_open(DtCtx *c)
{
    hiscoreBase(c);
    gExtrasWait = (uint16_t)(dt_u32() % 4 == 0 ? dt_range(1, 3) : 0);
}

static void setup_hiscore_close(DtCtx *c)
{
    hiscoreBase(c);
    randomAssetSet();
}

static void setup_hiscore_hook(DtCtx *c)
{
    static const uint32_t ids[] = {0xa00d0003, 0xa00d0004, 0xa00d0004};
    hiscoreBase(c);
    pickHookArgs(c, kNavMsgs, (int)(sizeof kNavMsgs / sizeof kNavMsgs[0]), ids, 3);
}

static void setup_hiscore_state(DtCtx *c)
{
    hiscoreBase(c);
}

static void setup_info_state(DtCtx *c)
{
    infoBase(c);
}

static void setup_info_close(DtCtx *c)
{
    infoBase(c);
    randomAssetSet();
}

static void setup_info_hook(DtCtx *c)
{
    static const uint32_t ids[] = {0xa00d0001, 0xa00d0002};
    infoBase(c);
    pickHookArgs(c, kNavMsgs, (int)(sizeof kNavMsgs / sizeof kNavMsgs[0]), ids, 2);
}

/* ---- hub and start-up ------------------------------------------------------------------------------ */

static void setup_hub_hook(DtCtx *c)
{
    static const uint32_t ids[] = {0xa00d0008};
    extrasBase(c, kUiGroups);
    pickHookArgs(c, kNavMsgs, (int)(sizeof kNavMsgs / sizeof kNavMsgs[0]), ids, 1);
}

/* A static memory region big enough for the macro buffer. */
static void staticRegion(void)
{
    uint8_t *base = dt_alloc(0x6000);
    gZmem.staticBase = (uint32_t *)base;
    gZmem.staticEnd = (uintptr_t)base + 0x6000;
    gZmem.staticTop = (uint32_t *)(base + 4u * (uint32_t)dt_range(0, 0x100));
    gZmem.lastStaticAlloc = NULL;
}

/* The macro buffer, maybe holding a registered string set already. */
static void macroState(int allowBuffer)
{
    uint32_t size = 0;
    const uint8_t *blob = dtFrontendAssetBlob(&size);
    gMacroSize = 0;
    gMacroBuf = NULL;
    if (allowBuffer && dt_u32() % 2 == 0) {
        uint8_t *buffer = dt_alloc(0x4000);
        gMacroBuf = buffer;
        if (dt_u32() % 2 == 0) { /* a macro set is loaded and registered */
            memcpy(buffer, blob, size);
            zstringRegister((const ZStringSet *)buffer);
            gMacroSize = size;
        }
    }
}

static void setup_alloc_macros(DtCtx *c)
{
    extrasBase(c, kUiGroups);
    staticRegion();
    gMacroBuf = dt_u32() % 2 == 0 ? NULL : (void *)(uintptr_t)(0x60000000u + (dt_u32() & 0xff0));
}

static void setup_init_app(DtCtx *c)
{
    extrasBase(c, DT_FS_SOUND | DT_FS_INIT | DT_FS_EXTRAS | DT_FS_ASSET);
    dt_stub(0x00447540, selScreenOpen, (void *)stubSelScreenOpen);
    dt_stub(0x0043f730, autoloadScreenInit, (void *)stubAutoloadInit);
    static const int results[4] = {0, 0, 1, 0};
    dtFrontQueue(DT_Q_GUIINIT, results + dt_range(0, 2), 1);
    dtFrontQueue(DT_Q_MODEINIT, results + dt_range(0, 2), 1);
    const int sizes[1] = {1}; /* the language data exists (a missing one aborts, see the Abort case) */
    dtFrontQueue(DT_Q_ASSETSIZE, sizes, 1);
    staticRegion();
    macroState(1);
    gAssetLibrary = dt_range(0, 5);
    gSelResume = (uint8_t)(dt_u32() % 2 == 0 ? dt_range(0, 5) : 0);
    gAutoLoadDone = dt_u32() % 2 == 0 ? 0 : dt_u32();
}

/* The language's data is missing: both sides report the same fatal error. */
static void setup_init_app_abort(DtCtx *c)
{
    setup_init_app(c);
    const int found[1] = {0};
    dtFrontQueue(DT_Q_ASSETFIND, found, 1);
    const int zero[1] = {0};
    dtFrontQueue(DT_Q_GUIINIT, zero, 1);
}

static void setup_set_language(DtCtx *c)
{
    extrasBase(c, kUiGroups);
    macroState(0);
    uint8_t *buffer = dt_alloc(0x4000);
    gMacroBuf = buffer;
    uint32_t size = 0;
    const uint8_t *blob = dtFrontendAssetBlob(&size);
    if (dt_u32() % 2 == 0) { /* a macro set is loaded and registered */
        memcpy(buffer, blob, size);
        zstringRegister((const ZStringSet *)buffer);
        gMacroSize = size;
    }
    gAssetLibrary = dt_range(0, 5);
    /* the data of every language exists; its size is valid (the failure paths abort) */
    const int sizes[1] = {1};
    dtFrontQueue(DT_Q_ASSETSIZE, sizes, 1);
    DT_ARG_INT(c, 0, dt_u32() % 4 == 0 ? dt_u32() % 40 : dt_range(0, 6));
}

/* Missing data or a size of 0: the fatal error is the same on both sides. */
static void setup_set_language_abort(DtCtx *c)
{
    setup_set_language(c);
    const int none[1] = {0};
    if (dt_u32() % 2 == 0) {
        dtFrontQueue(DT_Q_ASSETFIND, none, 1);
    } else {
        dtFrontQueue(DT_Q_ASSETSIZE, none, 1);
    }
}

DT_CASES(frontend_extras_cases) = {
    {"extrasPlay", 0x0044a2d0, extrasPlay, 0, DT_RET_VOID, setup_state, 150, 0},
    {"extrasOpen", 0x0044a4b0, extrasOpen, 0, DT_RET_VOID, setup_state, 150, 0},
    {"extrasShowMenu", 0x0044a690, extrasShowMenu, 0, DT_RET_VOID, setup_state, 100, 0},
    {"extrasSlideIn", 0x0044a7b0, extrasSlideIn, 0, DT_RET_VOID, setup_slides, 300, 0},
    {"extrasSlideOut", 0x0044a870, extrasSlideOut, 0, DT_RET_VOID, setup_slides, 300, 0},
    {"extrasListDraw", 0x0044a950, extrasListDraw, 4, DT_RET_INT, setup_list_draw, 300, 0},
    {"extrasHook", 0x0044aa30, extrasHook, 4, DT_RET_INT, setup_extras_hook, 600, 0},
    {"hiscoreOpen", 0x0044ac00, hiscoreOpen, 0, DT_RET_VOID, setup_hiscore_open, 150, 0},
    {"hiscoreClose", 0x0044ae30, hiscoreClose, 0, DT_RET_VOID, setup_hiscore_close, 150, 0},
    {"hiscoreBuild", 0x0044af40, hiscoreBuild, 0, DT_RET_VOID, setup_hiscore_state, 300, 0},
    {"hiscoreSlideIn", 0x0044b340, hiscoreSlideIn, 0, DT_RET_VOID, setup_hiscore_state, 300, 0},
    {"hiscoreSlideOut", 0x0044b460, hiscoreSlideOut, 0, DT_RET_VOID, setup_hiscore_state, 300, 0},
    {"hiscoreHook", 0x0044b580, hiscoreHook, 4, DT_RET_INT, setup_hiscore_hook, 600, 0},
    {"infoOpen", 0x0044bcd0, infoOpen, 0, DT_RET_VOID, setup_info_state, 150, 0},
    {"infoClose", 0x0044bea0, infoClose, 0, DT_RET_VOID, setup_info_close, 150, 0},
    {"infoBuild", 0x0044bf70, infoBuild, 0, DT_RET_VOID, setup_info_state, 300, 0},
    {"infoSlideIn", 0x0044c0a0, infoSlideIn, 0, DT_RET_VOID, setup_info_state, 300, 0},
    {"infoSlideOut", 0x0044c1c0, infoSlideOut, 0, DT_RET_VOID, setup_info_state, 300, 0},
    {"infoHook", 0x0044c390, infoHook, 4, DT_RET_INT, setup_info_hook, 600, 0},
    {"extrasMenuInit", 0x0044ca00, extrasMenuInit, 0, DT_RET_INT, setup_state, 50, 0},
    {"extrasMenuHook", 0x0044ca40, extrasMenuHook, 4, DT_RET_INT, setup_hub_hook, 600, 0},
    {"extrasMenuExit", 0x0044cc70, extrasMenuExit, 0, DT_RET_VOID, setup_state, 100, 0},
    {"extrasMenuHighlight", 0x0044cca0, extrasMenuHighlight, 0, DT_RET_VOID, setup_state, 100, 0},
    {"extrasMenuSlideIn", 0x0044ce80, extrasMenuSlideIn, 0, DT_RET_VOID, setup_state, 200, 0},
    {"creditsPlay", 0x0044d080, creditsPlay, 0, DT_RET_VOID, setup_state, 100, 0},
    {"extrasMenuSlideOut", 0x0044d2a0, extrasMenuSlideOut, 0, DT_RET_VOID, setup_state, 300, 0},
    {"extrasMenuLayout", 0x0044d5a0, extrasMenuLayout, 0, DT_RET_VOID, setup_state, 100, 0},
    {"extrasMenuLayoutCopy", 0x0044d770, extrasMenuLayoutCopy, 0, DT_RET_VOID, setup_state, 100, 0},
    {"frontendAllocMacros", 0x0044d940, frontendAllocMacros, 0, DT_RET_INT, setup_alloc_macros, 50, 0},
    {"frontendInitApp", 0x0044d990, frontendInitApp, 0, DT_RET_INT, setup_init_app, 200, 0},
    {"frontendInitAppAbort", 0x0044d990, frontendInitApp, 0, DT_RET_INT, setup_init_app_abort, 20, 0},
    {"frontendSetLanguage", 0x0044dad0, frontendSetLanguage, 1, DT_RET_INT, setup_set_language, 100, 0},
    {"frontendSetLanguageAbort", 0x0044dad0, frontendSetLanguage, 1, DT_RET_INT, setup_set_language_abort, 20,
     0},
};
