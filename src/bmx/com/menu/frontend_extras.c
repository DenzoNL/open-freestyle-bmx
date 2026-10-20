/*
 * Extras menu (screen 0xa00d): the hub with its four entries, the movie list, the high score tables, the
 * level information pages and the credits. The sub screens reuse the hub's elements (ids 0xa00d0001..0x17):
 * 1/2 and 3/4 are the pages' drawing elements, 5/6 their two header lines, 7 the movie list, 8..0xf the
 * hub's four entries (texts and highlights), 0x10..0x17 the bars that are hidden while sliding.
 */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/menu/gui/guilabel.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"
#include "zge/zasset.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

typedef char CreditsMovieName[22];
#define gCreditsMovie ORIG_VAR(0x0055b2c8, CreditsMovieName, gCreditsMovie)

#define kHubLabel0     0xa00d0008u /* entries: 8/9, a/b, c/d, e/f (the second of each pair is its highlight) */
#define kHubLabel1     0xa00d000au
#define kHubLabel2     0xa00d000cu
#define kHubLabel3     0xa00d000eu
#define kHubHighlight0 0xa00d0009u
#define kHubHighlight1 0xa00d000bu
#define kHubHighlight2 0xa00d000du
#define kHubHighlight3 0xa00d000fu
#define kMovieList     0xa00d0007u
#define kHeader1       0xa00d0005u
#define kHeader2       0xa00d0006u

/* Ids 0xa00d0010..17: the bars that are hidden whenever the hub moves (listed in the order they are hidden).
 */
static void hubHideBars(void)
{
    menuHide(0xa00d0011);
    menuHide(0xa00d0010);
    menuHide(0xa00d0013);
    menuHide(0xa00d0012);
    menuHide(0xa00d0015);
    menuHide(0xa00d0014);
    menuHide(0xa00d0017);
    menuHide(0xa00d0016);
}

static void hubShowEntries(void)
{
    menuShow(kHubHighlight0);
    menuShow(kHubHighlight1);
    menuShow(kHubHighlight2);
    menuShow(kHubHighlight3);
    menuShow(kHubLabel0);
    menuShow(kHubLabel1);
    menuShow(kHubLabel2);
    menuShow(kHubLabel3);
}

static void hubHideEntries(void)
{
    menuHide(kHubHighlight0);
    menuHide(kHubHighlight1);
    menuHide(kHubHighlight2);
    menuHide(kHubHighlight3);
    menuHide(kHubLabel0);
    menuHide(kHubLabel1);
    menuHide(kHubLabel2);
    menuHide(kHubLabel3);
}

/* Puts the highlight elements of the four entries where the entries' slide offsets say. */
static void hubPlaceHighlights(void)
{
    guiSetTextPos(guiFindById(kHubHighlight0), (int16_t)((0x28 - gMenuSlide0) * 4), 0);
    guiSetTextPos(guiFindById(kHubHighlight1), (int16_t)((0x28 - gMenuSlide1) * 4), 0);
    guiSetTextPos(guiFindById(kHubHighlight2), (int16_t)((0x28 - gMenuSlide2) * 4), 0);
    guiSetTextPos(guiFindById(kHubHighlight3), (int16_t)((0x28 - gMenuSlide3) * 4), 0);
}

/* Slide step of the pages: (500 - v) / 4, rounded towards zero. */
static int slideQuarter(int16_t v)
{
    const int d = 500 - v;
    return (d + ((d >> 31) & 3)) >> 2;
}

/* The same as multiplication by 0x2aaaaaab (or its negative), as the compiler wrote it. */
static int slideSixth(int16_t v, int32_t magic)
{
    const int32_t d = 500 - v;
    const int32_t hi = (int32_t)(((int64_t)d * magic) >> 32);
    return hi + (int)((uint32_t)hi >> 31);
}

/* Plays the chosen title's movie, then comes back to the list. */
BMX_ADDR(0x0044a2d0)
void extrasPlay(void)
{
    MovieDesc *list[2];
    list[1] = NULL;
    list[0] = &gExtrasTitles[gExtrasSel]->movie;
    if (gExtrasWait2 != 0) {
        gExtrasWait2--;
        return;
    }
    guiModeShutdown();
    bmxSoundPlayAt(1000, NULL, 0, 0, 0);
    moviePlay(list);
    bmxSoundPlayAt(1000, (const void *)(uintptr_t)1, 0, 0, 0);
    menuSetChannelVolume(1, gSoundVolume1 * 7);
    menuSetChannelVolume(0, gSoundVolume0 * 7);
    guiModeInit();
    guiLoadBackdrop(0xa011, 8);
    guiRequestScreen(0xa00d, BMX_FNPTR(GuiHookFn, 0x00401eec, extrasHook));
    extrasMenuLayoutCopy();
    menuHide(kHubHighlight0);
    menuHide(kHubHighlight1);
    menuHide(kHubHighlight2);
    menuHide(kHubHighlight3);
    menuHide(kHubLabel0);
    menuHide(kHubLabel1);
    menuHide(kHubLabel2);
    menuHide(kHubLabel3);
    gExtrasChoice = 2;
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004012cb, extrasOpen);
}

/* Opens the movie list: the unlocked titles, the first one selected unless a movie was just played. */
BMX_ADDR(0x0044a4b0)
void extrasOpen(void)
{
    const uint32_t played = gExtrasPlayed;
    gExtrasSlide[0] = 0;
    gExtrasSlide[1] = 0;
    gExtrasSlide[2] = 0;
    gExtrasSlide[3] = 0;
    if (played == 0) {
        gExtrasSel = 0;
    }
    gExtrasWait2 = 0;
    gExtrasPlayed = 0;
    uint16_t count = 1;
    gExtrasTitles[0] = &gTitleIntro;
    gExtrasCount = 1;
    if ((gUnlockDave & 1) != 0) {
        count = 2;
        gExtrasTitles[1] = &gTitleDave;
        gExtrasCount = 2;
    }
    if ((gUnlockRyan & 1) != 0) {
        const uint16_t at = count;
        count++;
        gExtrasCount = count;
        gExtrasTitles[at] = &gTitleRyan;
    }
    if ((gUnlockAmish & 1) != 0) {
        gExtrasCount = (uint16_t)(count + 1);
        gExtrasTitles[count] = &gTitleAmish;
    }
    menuShow(kMovieList);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x00401eec, extrasHook));
    guiFindById(kMovieList)->proc = BMX_FNPTR(GuiProcFn, 0x004021d5, extrasListDraw);
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401d9d, extrasSlideIn);
    menuHide(0xa00d0015);
    menuHide(0xa00d0014);
    hubHideEntries();
}

/* Back to the hub: the entries slide in again and the list goes away. */
BMX_ADDR(0x0044a690)
void extrasShowMenu(void)
{
    menuSlideReset();
    menuSlideIn(kHubLabel0, kHubLabel1, kHubLabel2, kHubLabel3);
    menuHide(kMovieList);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x004018de, extrasMenuHook));
    guiFindById(kMovieList)->proc = BMX_FNPTR(GuiProcFn, 0x00401e4c, guiLabelProc);
    hubHideBars();
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004010e1, extrasMenuSlideIn);
}

/* The list's rows slide in one after the other. */
BMX_ADDR(0x0044a7b0)
void extrasSlideIn(void)
{
    int16_t v = gExtrasSlide[0];
    v = (int16_t)(v + slideQuarter(v));
    gExtrasSlide[0] = v;
    if (v > 0x1cc) {
        gExtrasSlide[0] = 0x1cc;
    }
    for (int i = 1; i < 4; i++) {
        if (gExtrasSlide[i - 1] > 0x104) {
            const int16_t w = gExtrasSlide[i];
            gExtrasSlide[i] = (int16_t)(slideQuarter(w) + w);
        }
        if (gExtrasSlide[i] > 0x1cc) {
            gExtrasSlide[i] = 0x1cc;
        }
    }
    if (gExtrasSlide[3] >= 0x1cc) {
        gFrontendStep = NULL;
    }
}

/* ... and out again; then the hub is back, or the chosen movie starts. */
BMX_ADDR(0x0044a870)
void extrasSlideOut(void)
{
    int16_t v = gExtrasSlide[0];
    v = (int16_t)(v - slideQuarter(v));
    gExtrasSlide[0] = v;
    if (v < 0) {
        gExtrasSlide[0] = 0;
    }
    for (int i = 1; i < 4; i++) {
        if (gExtrasSlide[i - 1] < 0x1a4) {
            const int16_t w = gExtrasSlide[i];
            gExtrasSlide[i] = (int16_t)(w - slideQuarter(w));
        }
        if (gExtrasSlide[i] < 0) {
            gExtrasSlide[i] = 0;
        }
    }
    if (gExtrasSlide[3] > 0) {
        return;
    }
    if (gExtrasPlayed == 0) {
        extrasShowMenu();
        return;
    }
    extrasShowMenu();
    gExtrasWait2 = 2;
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401311, extrasPlay);
}

/* Draws the movie list (the list element's own handler): every title as a row, the selected one lit. */
BMX_ADDR(0x0044a950)
int BMX_CDECL extrasListDraw(int msg, GuiElement *el, int arg3, int arg4)
{
    if (msg == 0xc && el->id == kMovieList && gExtrasCount != 0) {
        for (int i = 0; i < (int)gExtrasCount; i++) {
            guiSetText(el, (const char *)gExtrasTitles[i]);
            guiSetTextPos(el, (int16_t)(0x200 - gExtrasSlide[i]), (int16_t)(i * 0x23));
            guiSetTextStyle(el, gExtrasSel == (uint32_t)i);
            guiLabelProc(0xc, el, arg3, arg4);
        }
    }
    return 0;
}

/* Hook of the movie list screen. */
BMX_ADDR(0x0044aa30)
int BMX_CDECL extrasHook(int msg, GuiElement *el, int arg3, int arg4)
{
    switch (msg) {
    case 0x16:
        if (gFrontendStep != NULL) {
            gFrontendStep();
        }
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case 0x1e:
    case 0x2b:
        if (gExtrasSel == 0) {
            return 1;
        }
        gExtrasSel--;
        menuPlaySound(2000);
        return 1;
    case 0x1f:
    case 0x2c:
        if ((int)(gExtrasCount - 1) <= (int)gExtrasSel) {
            return 1;
        }
        gExtrasSel++;
        menuPlaySound(2000);
        return 1;
    case 0x20:
    case 0x21:
    case 0x2d:
    case 0x2e:
        return 1;
    case 0x22:
    case 0x24:
    case 0x2f:
    case 0x31:
        if (gFrontendStep != NULL) {
            return 1;
        }
        gExtrasPlayed = 1;
        menuPlaySound(0x7d2);
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402581, extrasSlideOut);
        return 1;
    case 0x25:
    case 0x32:
        if (gFrontendStep != NULL) {
            return 1;
        }
        if (gMenuInstant != 0) {
            if (gExtrasPlayed == 0) {
                extrasShowMenu();
                return 1;
            }
            extrasShowMenu();
            gExtrasWait2 = 2;
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401311, extrasPlay);
            return 1;
        }
        menuPlaySound(0x7d2);
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402581, extrasSlideOut);
        return 1;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
}

/* Loads the string set of the language's page (asset `id`) into a locked block; remembers it for the close.
 */
static void loadAssetStrings(int id)
{
    gAssetSetData = NULL;
    zassetFind(gAssetLibrary, id, 0);
    gAssetSetSize = zassetSize(gAssetLibrary);
    if (gAssetSetSize != 0) {
        gHiscoreHandle = zmemdmmAlloc(gAssetSetSize);
        gAssetSetData = zmemdmmLock(gHiscoreHandle);
        zassetRead(gAssetLibrary, gAssetSetData);
        zstringRegister((const ZStringSet *)gAssetSetData);
    }
}

/* Frees what loadAssetStrings made. */
static void freeAssetStrings(void)
{
    zstringUnregister((const ZStringSet *)gAssetSetData);
    zmemdmmUnlock(gHiscoreHandle);
    zmemdmmFree(gHiscoreHandle);
    gHiscoreHandle = 0;
}

/* Opens the high score tables. */
BMX_ADDR(0x0044ac00)
void hiscoreOpen(void)
{
    if (gExtrasWait != 0) {
        gExtrasWait--;
        return;
    }
    zprimLoadGroup(3, 0xe00a);
    loadAssetStrings((int)(gLanguageCurrent & 0xff) + 0x7d01);
    gHiscoreSprite = zprimCreateSpriteSmall(0xa6, 0x80, 8, 0x100);
    gSelLevelSetting = 0;
    for (int i = 0; i < 8; i++) {
        HiscoreCell *cell = &gHiscoreLabels[i];
        cell->flags = (int16_t)0x8001;
        cell->kind = 0;
        cell->y = i * 0x16;
        zstrcpy(cell->text, zstringGet((uint16_t)(0x1096 + i)));
    }
    hiscoreBuild();
    gHiscoreSlide[0] = 0;
    gHiscoreSlide[1] = 0;
    gHiscoreSlide[2] = 0;
    gHiscoreSlide[3] = 0;
    gHiscoreWait = 2;
    menuSetStyle(kHeader1, 1);
    menuSetStyle(kHeader2, 1);
    guiFindById(0xa00d0003)->proc = BMX_FNPTR(GuiProcFn, 0x00401668, hiscoreHook);
    guiFindById(0xa00d0004)->proc = BMX_FNPTR(GuiProcFn, 0x00401668, hiscoreHook);
    gHiscoreBlink = 0;
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x00401668, hiscoreHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401631, hiscoreSlideIn);
}

/* Closes the tables and goes back to the hub. */
BMX_ADDR(0x0044ae30)
void hiscoreClose(void)
{
    zprimFreeSprite(gHiscoreSprite);
    if (gAssetSetData != NULL) {
        freeAssetStrings();
        gAssetSetData = NULL;
    }
    menuSlideReset();
    menuSlideIn(kHubLabel0, kHubLabel1, kHubLabel2, kHubLabel3);
    menuHide(kHeader1);
    menuHide(kHeader2);
    menuHide(0xa00d0003);
    menuHide(0xa00d0004);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x004018de, extrasMenuHook));
    zprimFreeGroup(3);
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004010e1, extrasMenuSlideIn);
}

/*
 * The title of the selected level splits at its first '-' into the two header lines; the level's picture
 * is copied into the page's sprite.
 */
static void pageHeader(ZPrimAsset *sprite)
{
    char title[100];
    zstrcpy(title, zstringGet(gHiscoreTitleIds[gSelLevelSetting].id));
    const char *second = title;
    for (int i = 0; title[i] != '\0'; i++) {
        if (title[i] == '-') {
            title[i] = '\0';
            second = title + i + 2;
            break;
        }
    }
    menuSetText(kHeader1, title);
    menuSetText(kHeader2, second);
    zprimCopyTexture(zprimLoadAsset(gHiscorePictureIds[gSelLevelSetting].id), sprite);
}

/* True when the entry is the level's best: no other level has a score (of the same table) that is as high. */
static int isBestScore(const HiscoreEntry *entry, uint32_t index)
{
    const uint8_t saved = gSelLevelSetting;
    int best = 1;
    for (int level = 0; level < 12; level++) {
        gSelLevelSetting = (uint8_t)level;
        const HiscoreEntry *other = hiscoreEntry(index);
        if (entry->score <= other->score && level != saved) {
            best = 0;
            break;
        }
    }
    gSelLevelSetting = saved;
    return best;
}

/* Fills the table's cells for the selected level (rows 0..1 are scores, 2..7 times with the best one lit). */
BMX_ADDR(0x0044af40)
void hiscoreBuild(void)
{
    if (gExtrasWait != 0) {
        gExtrasWait--;
        return;
    }
    pageHeader(gHiscoreSprite);
    for (int row = 0; row < 8; row++) {
        const int y = row * 0x16;
        const uint32_t index = (uint32_t)(2 + row) << 16;
        const HiscoreEntry *entry = hiscoreEntry(index);
        char text[32];
        if (y < 0x2c || y > 0x9a) {
            zstrFormat(text, "  %6ld", entry->score);
        } else {
            const uint32_t t =
                (uint32_t)bmxFtol((double)entry->score * (double)gScoreToTime + (double)gScoreRound);
            zstrFormat(text, " %3ld` %2ld~", t / 12, t % 12);
        }
        const int style = (isBestScore(entry, index) != 0 ? 8 : 0) + 2;
        HiscoreCell *time = &gHiscoreTimes[row];
        time->flags = (int16_t)0x8001;
        time->kind = 0;
        time->y = y;
        time->blinkStyle = 2;
        time->style = style;
        zstrcpy(time->text, text);
        HiscoreCell *name = &gHiscoreNames[row];
        name->flags = (int16_t)0x8001;
        name->kind = 0;
        name->y = y;
        name->blinkStyle = 2;
        name->style = style;
        zstrcpy(name->text, entry->name);
        HiscoreCell *rider = &gHiscoreRiders[row];
        rider->flags = (int16_t)0x8001;
        rider->kind = 2;
        rider->y = y;
        rider->blinkStyle = 0;
        rider->style = style;
        zstrcpy(rider->text, zstringGet(gRiderNameIds[entry->rider]));
    }
    gFrontendStep = NULL;
}

/* The table's four parts slide in. */
BMX_ADDR(0x0044b340)
void hiscoreSlideIn(void)
{
    menuShow(kHeader1);
    menuShow(kHeader2);
    menuShow(0xa00d0003);
    menuShow(0xa00d0004);
    int16_t v = gHiscoreSlide[0];
    v = (int16_t)(v + slideSixth(v, 0x2aaaaaab));
    gHiscoreSlide[0] = v;
    if (v > 0x1d6) {
        gHiscoreSlide[0] = 0x1d6;
    }
    for (int i = 1; i < 4; i++) {
        if (gHiscoreSlide[i - 1] > 0x122) {
            const int16_t w = gHiscoreSlide[i];
            gHiscoreSlide[i] = (int16_t)(slideSixth(w, 0x2aaaaaab) + w);
        }
        if (gHiscoreSlide[i] > 0x1d6) {
            gHiscoreSlide[i] = 0x1d6;
        }
    }
    if (gHiscoreSlide[3] >= 0x1d6) {
        gFrontendStep = NULL;
    }
}

/* ... and out; then the screen closes. */
BMX_ADDR(0x0044b460)
void hiscoreSlideOut(void)
{
    int16_t v = gHiscoreSlide[0];
    v = (int16_t)(v + slideSixth(v, (int32_t)0xd5555555));
    gHiscoreSlide[0] = v;
    if (v < 0) {
        gHiscoreSlide[0] = 0;
    }
    for (int i = 1; i < 4; i++) {
        if (gHiscoreSlide[i - 1] < 0x1a4) {
            const int16_t w = gHiscoreSlide[i];
            gHiscoreSlide[i] = (int16_t)(w - slideSixth(w, 0x2aaaaaab));
        }
        if (gHiscoreSlide[i] < 0) {
            gHiscoreSlide[i] = 0;
        }
    }
    if (gHiscoreSlide[3] <= 0) {
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401ba9, hiscoreClose);
    }
    menuHide(0xa00d0017);
    menuHide(0xa00d0016);
    menuHide(0xa00d0015);
    menuHide(0xa00d0014);
}

/* Style of a lit row: 3 and 2 alternate with the blink bit. */
static uint32_t litStyle(void)
{
    return (uint32_t)((((uint8_t)~gHiscoreBlink & 4) | 8) >> 2);
}

/* Draws the title and sprite of the table (element 3) or the best score and the table's cells (element 4). */
static void hiscoreDraw(int msg, GuiElement *el, int arg3, int arg4)
{
    if (el->id == 0xa00d0003) {
        guiSetText(el, zstringGet(0x1094));
        guiSetTextPos(el, (int16_t)(0x2c8 - gHiscoreSlide[1]), 0x19);
        guiSetTextStyle(el, 0);
        guiLabelProc(msg, el, arg3, arg4);
        zprimSetPicture(gHiscoreSprite);
        zprimSet(0xe, 0);
        zprimDrawSprite(0, (int16_t)(gHiscoreSlide[0] - 0x1a2), -0x30);
        for (int i = 0; i < 8; i++) {
            guiSetText(el, gHiscoreLabels[i].text);
            guiSetTextPos(el, (int16_t)(0x200 - gHiscoreSlide[2]), (int16_t)(gHiscoreLabels[i].y + 0x6e));
            guiLabelProc(msg, el, arg3, arg4);
        }
        return;
    }
    if (el->id != 0xa00d0004) {
        return;
    }
    const HiscoreEntry *entry = hiscoreEntry(0);
    const int best = isBestScore(entry, 0);
    char text[32];
    guiSetText(el, entry->name);
    guiSetTextPos(el, (int16_t)(0x2c8 - gHiscoreSlide[1]), 0x2d);
    guiSetTextStyle(el, best != 0 ? litStyle() : 2);
    guiLabelProc(msg, el, arg3, arg4);
    guiSetText(el, zstringGet(gRiderNameIds[entry->rider]));
    guiSetTextPos(el, (int16_t)(0x2c8 - gHiscoreSlide[1]), 0x41);
    guiSetTextStyle(el, best != 0 ? litStyle() : 2);
    guiLabelProc(msg, el, arg3, arg4);
    zstrFormat(text, "%ld", entry->score);
    guiSetTextFlags(el, 0x84);
    guiSetText(el, text);
    guiSetTextPos(el, (int16_t)(0x3b3 - gHiscoreSlide[1]), 0x19);
    guiSetTextStyle(el, best != 0 ? litStyle() : 2);
    guiLabelProc(msg, el, arg3, arg4);
    guiSetTextFlags(el, 4);
    for (int i = 0; i < 8; i++) {
        const uint8_t blink = (uint8_t)gHiscoreBlink;
        zprimSet(0xe, ((blink >> 2) & 1) * 8 + 2);
        guiSetText(el, gHiscoreNames[i].text);
        guiSetTextPos(el, (int16_t)(0x27c - gHiscoreSlide[1]), (int16_t)(gHiscoreNames[i].y + 0x6e));
        guiSetTextStyle(el,
                        (uint32_t)((blink & 4) != 0 ? gHiscoreNames[i].blinkStyle : gHiscoreNames[i].style));
        guiLabelProc(msg, el, arg3, arg4);
        guiSetText(el, gHiscoreRiders[i].text);
        guiSetTextPos(el, (int16_t)(0x2ec - gHiscoreSlide[1]), (int16_t)(gHiscoreRiders[i].y + 0x6e));
        guiSetTextStyle(
            el, (uint32_t)((blink & 4) != 0 ? gHiscoreRiders[i].blinkStyle : gHiscoreRiders[i].style));
        guiLabelProc(msg, el, arg3, arg4);
        guiSetText(el, gHiscoreTimes[i].text);
        guiSetTextPos(el, (int16_t)(0x37c - gHiscoreSlide[1]), (int16_t)(gHiscoreTimes[i].y + 0x6e));
        guiSetTextStyle(el,
                        (uint32_t)((blink & 4) != 0 ? gHiscoreTimes[i].blinkStyle : gHiscoreTimes[i].style));
        guiLabelProc(msg, el, arg3, arg4);
    }
}

/* Hook (and drawing handler of elements 3 and 4) of the high score screen. */
BMX_ADDR(0x0044b580)
int BMX_CDECL hiscoreHook(int msg, GuiElement *el, int arg3, int arg4)
{
    switch (msg) {
    case 0xc:
        hiscoreDraw(msg, el, arg3, arg4);
        return 0;
    case 0x16:
        if (gFrontendStep != NULL) {
            gFrontendStep();
        }
        guiSetTextPos(guiFindById(kHeader1), (int16_t)(0x2c8 - gHiscoreSlide[0]), -0x32);
        guiSetTextPos(guiFindById(kHeader2), (int16_t)(0x2c8 - gHiscoreSlide[0]), -0x32);
        menuDrawFrame((int16_t)(gHiscoreSlide[0] - 0x1ae), 0x3c, (int16_t)(gHiscoreSlide[0] - 0xf0), 0xd4,
                      0x80, 1);
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case 0x1e:
    case 0x1f:
    case 0x22:
    case 0x24:
    case 0x2b:
    case 0x2c:
    case 0x2f:
    case 0x31:
        return 1;
    case 0x20:
    case 0x2d:
        if (gFrontendStep == NULL) {
            gSelLevelSetting = (uint8_t)menuWrap(gSelLevelSetting - 1, 0xc);
            gExtrasWait = 2;
            menuPlaySound(2000);
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401a91, hiscoreBuild);
            return 1;
        }
        return 0;
    case 0x21:
    case 0x2e:
        if (gFrontendStep == NULL) {
            gSelLevelSetting = (uint8_t)menuWrap(gSelLevelSetting + 1, 0xc);
            gExtrasWait = 2;
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401a91, hiscoreBuild);
            menuPlaySound(2000);
            return 1;
        }
        return 0;
    case 0x25:
    case 0x32:
        if (gFrontendStep != NULL) {
            return 1;
        }
        gHiscoreWait = 2;
        if (gMenuInstant == 0) {
            menuPlaySound(0x7d2);
        }
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401e6f, hiscoreSlideOut);
        return 1;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
}

/* ---- level information pages ------------------------------------------------------------------ */

/* Opens the level information pages. */
BMX_ADDR(0x0044bcd0)
void infoOpen(void)
{
    if (gExtrasWait != 0) {
        gExtrasWait--;
        return;
    }
    zprimLoadGroup(3, 0xe00a);
    loadAssetStrings((int)(gLanguageCurrent & 0xff) + 0x7d05);
    gInfoSprite = zprimCreateSpriteSmall(0xa6, 0x80, 8, 0x100);
    gSelLevelSetting = 0;
    gInfoPage = 0;
    infoBuild();
    gInfoSlide[0] = 0;
    gInfoSlide[1] = 0;
    gInfoSlide[2] = 0;
    gInfoSlide[3] = 0;
    gInfoWait = 2;
    menuSetStyle(kHeader1, 1);
    menuSetStyle(kHeader2, 1);
    guiFindById(0xa00d0001)->proc = BMX_FNPTR(GuiProcFn, 0x004014c4, infoHook);
    guiFindById(0xa00d0002)->proc = BMX_FNPTR(GuiProcFn, 0x004014c4, infoHook);
    menuHide(0xa00d0013);
    menuHide(0xa00d0012);
    gInfoTicks = 0;
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x004014c4, infoHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x0040248c, infoSlideIn);
}

/* Closes the pages and goes back to the hub. */
BMX_ADDR(0x0044bea0)
void infoClose(void)
{
    if (gExtrasWait != 0) {
        gExtrasWait--;
        return;
    }
    zprimFreeSprite(gInfoSprite);
    if (gAssetSetData != NULL) {
        freeAssetStrings();
    }
    menuSlideReset();
    menuSlideIn(kHubLabel0, kHubLabel1, kHubLabel2, kHubLabel3);
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x004018de, extrasMenuHook));
    zprimFreeGroup(3);
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004010e1, extrasMenuSlideIn);
}

/* Sets the header lines and picture of the selected level. */
BMX_ADDR(0x0044bf70)
void infoBuild(void)
{
    if (gExtrasWait != 0) {
        gExtrasWait--;
        return;
    }
    pageHeader(gInfoSprite);
    gFrontendStep = NULL;
}

/* The pages' four parts slide in. */
BMX_ADDR(0x0044c0a0)
void infoSlideIn(void)
{
    menuShow(kHeader1);
    menuShow(kHeader2);
    menuShow(0xa00d0002);
    menuShow(0xa00d0001);
    int16_t v = gInfoSlide[0];
    v = (int16_t)(v + slideSixth(v, 0x2aaaaaab));
    gInfoSlide[0] = v;
    if (v > 0x1d6) {
        gInfoSlide[0] = 0x1d6;
    }
    for (int i = 1; i < 4; i++) {
        if (gInfoSlide[i - 1] > 0x122) {
            const int16_t w = gInfoSlide[i];
            gInfoSlide[i] = (int16_t)(slideSixth(w, 0x2aaaaaab) + w);
        }
        if (gInfoSlide[i] > 0x1d6) {
            gInfoSlide[i] = 0x1d6;
        }
    }
    if (gInfoSlide[3] >= 0x1d6) {
        gFrontendStep = NULL;
    }
}

/* ... and out; then the screen closes (at once when slides are skipped). */
BMX_ADDR(0x0044c1c0)
void infoSlideOut(void)
{
    int16_t v = gInfoSlide[0];
    v = (int16_t)(v + slideSixth(v, (int32_t)0xd5555555));
    gInfoSlide[0] = v;
    if (v < 0) {
        gInfoSlide[0] = 0;
    }
    for (int i = 1; i < 4; i++) {
        if (gInfoSlide[i - 1] < 0x1a4) {
            const int16_t w = gInfoSlide[i];
            gInfoSlide[i] = (int16_t)(w - slideSixth(w, 0x2aaaaaab));
        }
        if (gInfoSlide[i] < 0) {
            gInfoSlide[i] = 0;
        }
    }
    if (gMenuInstant != 0 || gInfoSlide[3] <= 0) {
        menuHide(kHeader1);
        menuHide(kHeader2);
        menuHide(0xa00d0002);
        menuHide(0xa00d0001);
        gExtrasWait = 2;
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004016d6, infoClose);
    }
    hubHideBars();
}

/* One line of the page: three consecutive strings of the table, each followed by a space. */
static void infoLine(char *line, int index)
{
    const uint16_t id = gInfoLineIds[gSelLevelSetting][gInfoPage][index];
    zstrcpy(line, zstringGet(id));
    zstrcat(line, " ");
    zstrcat(line, zstringGet((uint16_t)(id + 1)));
    zstrcat(line, " ");
    zstrcat(line, zstringGet((uint16_t)(id + 2)));
    zstrcat(line, " ");
}

/* Hook (and drawing handler of elements 1 and 2) of the information screen. */
BMX_ADDR(0x0044c390)
int BMX_CDECL infoHook(int msg, GuiElement *el, int arg3, int arg4)
{
    switch (msg) {
    case 0xc:
        if (el->id == 0xa00d0002) {
            guiSetText(el, zstringGet(gInfoHeadIds[gInfoPage]));
            guiSetTextPos(el, (int16_t)(0x200 - gInfoSlide[1]), 100);
            guiSetTextStyle(el, 0);
            guiLabelProc(msg, el, arg3, arg4);
        } else if (el->id == 0xa00d0001) {
            zprimSetPicture(gInfoSprite);
            zprimSet(0xe, 0);
            zprimDrawSprite(0, (int16_t)(gInfoSlide[0] - 0x1a2), -0x30);
            for (int i = 0; i < 4; i++) {
                char line[200];
                infoLine(line, i);
                guiSetText(el, line);
                guiSetTextPos(el, (int16_t)(0x200 - gInfoSlide[1]), (int16_t)((i * 5 + 0x23) * 4));
                guiSetTextStyle(el, 2);
                guiLabelProc(msg, el, arg3, arg4);
            }
        }
        return 0;
    case 0x16:
        if (gFrontendStep != NULL) {
            gFrontendStep();
        }
        gInfoTicks++;
        guiSetTextPos(guiFindById(kHeader1), (int16_t)(0x2c8 - gInfoSlide[0]), -0x32);
        guiSetTextPos(guiFindById(kHeader2), (int16_t)(0x2c8 - gInfoSlide[0]), -0x32);
        menuDrawFrame((int16_t)(gInfoSlide[0] - 0x1ae), 0x3c, (int16_t)(gInfoSlide[0] - 0xf0), 0xd4, 0x80, 1);
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case 0x1e:
    case 0x2b:
        if (gFrontendStep == NULL) {
            gExtrasWait = 2;
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004012d5, infoBuild);
            gInfoPage = (uint16_t)menuWrap(gInfoPage - 1, 3);
            menuPlaySound(2000);
            return 1;
        }
        return 0;
    case 0x1f:
    case 0x2c:
        if (gFrontendStep == NULL) {
            gExtrasWait = 2;
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004012d5, infoBuild);
            gInfoPage = (uint16_t)menuWrap(gInfoPage + 1, 3);
            menuPlaySound(2000);
            return 1;
        }
        return 0;
    case 0x20:
    case 0x2d:
        if (gFrontendStep == NULL) {
            gSelLevelSetting = (uint8_t)menuWrap(gSelLevelSetting - 1, 6);
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004012d5, infoBuild);
            gExtrasWait = 2;
            menuPlaySound(2000);
            return 1;
        }
        return 0;
    case 0x21:
    case 0x2e:
        if (gFrontendStep == NULL) {
            gSelLevelSetting = (uint8_t)menuWrap(gSelLevelSetting + 1, 6);
            gExtrasWait = 2;
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004012d5, infoBuild);
            menuPlaySound(2000);
            return 1;
        }
        return 0;
    case 0x22:
    case 0x24:
    case 0x2f:
    case 0x31:
        return 1;
    case 0x25:
    case 0x32:
        if (gFrontendStep != NULL) {
            return 1;
        }
        if (gMenuInstant == 0) {
            menuPlaySound(0x7d2);
        }
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401cf8, infoSlideOut);
        return 1;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
}

/* ---- the hub ------------------------------------------------------------------------------------- */

/* Opens the hub screen; its layout follows on the next tick. */
BMX_ADDR(0x0044ca00)
int extrasMenuInit(void)
{
    guiDestroyAll();
    guiLoadBackdrop(0xa011, 8);
    guiRequestScreen(0xa00d, BMX_FNPTR(GuiHookFn, 0x004018de, extrasMenuHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x0040171c, extrasMenuLayout);
    return 0;
}

/* Hook of the hub: the pulsing selection frame, up/down to choose, confirm to open, back to leave. */
BMX_ADDR(0x0044ca40)
int BMX_CDECL extrasMenuHook(int msg, GuiElement *el, int arg3, int arg4)
{
    switch (msg) {
    case 0x16:
        if (gFrontendStep != NULL) {
            gFrontendStep();
        }
        if (gExtrasReady != 0) {
            int pulse = gExtrasPulse;
            if (pulse < 0 || pulse > 0x40) {
                gExtrasPulseStep = -gExtrasPulseStep;
            }
            pulse += gExtrasPulseStep;
            gExtrasPulse = pulse;
            menuDrawFrame(0x1f, (int16_t)(gExtrasChoice * 0x54 + 0x46), 0x71,
                          (int16_t)(gExtrasChoice * 0x54 + 0xa3), (uint8_t)(0x80 - pulse), 1);
        }
        return guiDefaultHookProc(msg, el, arg3, arg4);
    case 0x1e:
    case 0x2b:
        if (gFrontendStep != NULL) {
            return 0;
        }
        gExtrasChoice--;
        if (gExtrasChoice < 0) {
            gExtrasChoice = 3;
        }
        extrasMenuHighlight();
        menuPlaySound(2000);
        return 0;
    case 0x1f:
    case 0x2c:
        if (gFrontendStep != NULL) {
            return 0;
        }
        gExtrasChoice++;
        if (gExtrasChoice > 3) {
            gExtrasChoice = 0;
        }
        extrasMenuHighlight();
        menuPlaySound(2000);
        return 0;
    case 0x22:
    case 0x24:
    case 0x2f:
    case 0x31:
        if (gFrontendStep == NULL) {
            gExtrasReady = 0;
            gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401c35, extrasMenuSlideOut);
            menuPlaySound(0x7d2);
        }
        return 1;
    case 0x25:
    case 0x32:
        if (gFrontendStep != NULL) {
            return 0;
        }
        gExtrasReady = 0;
        gExtrasChoice = -1;
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401c35, extrasMenuSlideOut);
        if (gMenuInstant == 0) {
            menuPlaySound(0x7d2);
        }
        return 1;
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
}

/* Leaves the hub for the main menu (after the wait). */
BMX_ADDR(0x0044cc70)
void extrasMenuExit(void)
{
    if (gExtrasWait != 0) {
        gExtrasWait--;
        return;
    }
    menuFrameFree();
    mainMenuInit(1);
}

/* Dims the entries that are not selected and lights the selected one's highlight. */
BMX_ADDR(0x0044cca0)
void extrasMenuHighlight(void)
{
    static const uint32_t kLabels[4] = {kHubLabel0, kHubLabel1, kHubLabel2, kHubLabel3};
    static const uint32_t kLights[4] = {kHubHighlight0, kHubHighlight1, kHubHighlight2, kHubHighlight3};
    for (int i = 0; i < 4; i++) {
        menuSetTint(kLabels[i], 0xff, 0xff, 0xff, gExtrasChoice != i ? 0x50 : 0xff);
    }
    for (int i = 0; i < 4; i++) {
        menuSetStyle(kLights[i], gExtrasChoice == i);
    }
}

/* The entries slide in; when they are there the hub takes input. */
BMX_ADDR(0x0044ce80)
void extrasMenuSlideIn(void)
{
    hubShowEntries();
    if (gMenuInstant != 0 || menuSlideIn(kHubLabel0, kHubLabel1, kHubLabel2, kHubLabel3) != 0) {
        menuHide(0xa00d0017);
        menuHide(0xa00d0016);
        extrasMenuHighlight();
        gExtrasReady = 1;
        gFrontendStep = NULL;
    }
    hubPlaceHighlights();
}

/* Plays the credits movie and comes back to the hub. */
BMX_ADDR(0x0044d080)
void creditsPlay(void)
{
    MovieDesc desc = {gCreditsMovie, 0x140, 0xf0, 0xd15, 0x18, 0};
    MovieDesc *list = &desc;
    if (gExtrasWait != 0) {
        gExtrasWait--;
        return;
    }
    guiDestroyAll();
    guiModeShutdown();
    bmxSoundPlayAt(1000, NULL, 0, 0, 0);
    moviePlay(&list);
    bmxSoundPlayAt(1000, (const void *)(uintptr_t)1, 0, 0, 0);
    menuSetChannelVolume(1, gSoundVolume1 * 7);
    menuSetChannelVolume(0, gSoundVolume0 * 7);
    guiModeInit();
    guiLoadBackdrop(0xa011, 8);
    guiRequestScreen(0xa00d, BMX_FNPTR(GuiHookFn, 0x004018de, extrasMenuHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x0040171c, extrasMenuLayout);
    extrasMenuLayoutCopy();
    guiSetHook(BMX_FNPTR(GuiHookFn, 0x004018de, extrasMenuHook));
    hubHideEntries();
    gExtrasChoice = 3;
}

/* The entries slide out; then the chosen screen is opened. */
BMX_ADDR(0x0044d2a0)
void extrasMenuSlideOut(void)
{
    hubHideBars();
    hubPlaceHighlights();
    if (gMenuInstant == 0 && menuSlideOut(kHubLabel0, kHubLabel1, kHubLabel2, kHubLabel3) == 0) {
        return;
    }
    hubHideEntries();
    gExtrasWait = 2;
    switch (gExtrasChoice) {
    case -1:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004022cf, extrasMenuExit);
        break;
    case 0:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401ab4, infoOpen);
        break;
    case 1:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401244, hiscoreOpen);
        break;
    case 2:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004012cb, extrasOpen);
        break;
    case 3:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401f78, creditsPlay);
        break;
    default:
        break;
    }
}

/* Sets the hub up: frame, entries sliding in, their texts. (0x44d770 is an identical copy.) */
BMX_ADDR(0x0044d5a0)
void extrasMenuLayout(void)
{
    gExtrasChoice = 0;
    gExtrasReady = 0;
    menuFrameInit();
    menuSlideReset();
    menuSlideIn(kHubLabel0, kHubLabel1, kHubLabel2, kHubLabel3);
    hubPlaceHighlights();
    menuSetText(kHubHighlight0, zstringGet(0x2f));
    menuSetText(kHubHighlight1, zstringGet(0x33));
    menuSetText(kHubHighlight2, zstringGet(0x34));
    menuSetText(kHubHighlight3, zstringGet(0x35));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004010e1, extrasMenuSlideIn);
}

BMX_ADDR(0x0044d770)
void extrasMenuLayoutCopy(void)
{
    gExtrasChoice = 0;
    gExtrasReady = 0;
    menuFrameInit();
    menuSlideReset();
    menuSlideIn(kHubLabel0, kHubLabel1, kHubLabel2, kHubLabel3);
    hubPlaceHighlights();
    menuSetText(kHubHighlight0, zstringGet(0x2f));
    menuSetText(kHubHighlight1, zstringGet(0x33));
    menuSetText(kHubHighlight2, zstringGet(0x34));
    menuSetText(kHubHighlight3, zstringGet(0x35));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004010e1, extrasMenuSlideIn);
}

/* ---- start-up --------------------------------------------------------------------------------------- */

/* Allocates the buffer for the macro string set (once). Returns 1 on failure. */
BMX_ADDR(0x0044d940)
int frontendAllocMacros(void)
{
    if (gMacroBuf == NULL) {
        gMacroBuf = zmemStaticAlloc(0x4000);
        if (gMacroBuf == NULL) {
            gZdebugPrint("FRONTEND: Failed to alloc memory\n");
            return 1;
        }
    }
    return 0;
}

/* Starts the front end: engine pieces, the macro strings and the first screen. Returns 1 on failure. */
BMX_ADDR(0x0044d990)
int frontendInitApp(void)
{
    appLog("APP: Init MENU\n");
    appInitCardA();
    appInitCardB(0);
    frontendAllocMacros();
    if (guiInit() != 0) {
        gZdebugPrint("frontendInitApp: Failed to guiInitializeApp\n");
        return 1;
    }
    guiPadInit();
    gExitFlag = 0;
    gExitCode = 0;
    appInitCardC();
    appInitCardD(0);
    appInitCardE(0);
    if (frontendSetLanguage(gLanguageCurrent & 0xff) != 0) {
        gZdebugPrint("FRONTEND: Failed to load frontend macros\n");
        return 1;
    }
    bmxSoundPlayAt(1000, (const void *)(uintptr_t)1, 0, 0, 0);
    if (guiModeInit() != 0) {
        gZdebugPrint("FRONTEND: Failed to guiInitializeMenuMode\n");
        return 1;
    }
    if (gSelResume != 0 && gSelResume <= 3) {
        selScreenOpen();
        return 0;
    }
    if (gAutoLoadDone != 0) {
        autoloadScreenInit();
        return 0;
    }
    mainMenuInit(0);
    return 0;
}

/* Loads the macro string set of the language (asset 0x7919 + language) into the buffer and registers it. */
BMX_ADDR(0x0044dad0)
int frontendSetLanguage(uint32_t language)
{
    static const char kFile[] = "C:\\zaxis\\Bmx\\src\\com\\menu\\frontend.c";
    if (gMacroSize != 0) {
        zstringUnregister((const ZStringSet *)gMacroBuf);
        gMacroSize = 0;
    }
    if (zassetFind(gAssetLibrary, (int)language + 0x7919, 0) == 0) {
        gZdebugPrint("FRONTEND: Could not find macro data\n");
        zdebugFail("FRONTEND: Could not find macro data\n\n", kFile, 0xdd, 0);
        return 1;
    }
    gMacroSize = zassetSize(gAssetLibrary);
    if (gMacroSize != 0 && gMacroSize < 0x4000) {
        zassetRead(gAssetLibrary, gMacroBuf);
        if (zstringRegister((const ZStringSet *)gMacroBuf) == 0) {
            gZdebugPrint("FRONTEND: Failed to load macro string set\n");
            zdebugFail("FRONTEND: Failed to load macro string set\n\n", kFile, 0xed, 0);
            return 1;
        }
        return 0;
    }
    gZdebugPrint("FRONTEND: Macro data too big or too small\n");
    zdebugFail("FRONTEND: Macro data too big or too small\n\n", kFile, 0xe4, 0);
    return 1;
}
