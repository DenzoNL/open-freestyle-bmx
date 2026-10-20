/*
 * Game setup screen (screen 0xa009): rider, bike, style and level selection for one or two players. Each
 * stage keeps a carousel of three picture/text pairs; moving the choice slides the next preview in
 * (selSlideStep) and the stage's commit function then settles the selection.
 */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guilabel.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"
#include "zge/zstring.h"

#define EMPTY_TEXT ((const char *)&gGuiEmptyChar)

/* The carousel page `offset` places after the current one. */
static int selSlot(int offset)
{
    return menuWrap(gSelPage + offset, gSelPages);
}

static GuiElement *selPicture(int slot)
{
    return guiFindById(gSelPicIds[slot]);
}

static GuiElement *selText(int slot)
{
    return guiFindById(gSelTextIds[slot]);
}

/* Rider stage: moves the choice by `step` (wrapping over the unlocked riders) and starts the caption. */
BMX_ADDR(0x00444ae0)
void selRiderCommit(int step)
{
    const int player = gSelPlayer;
    gSelRider[player] += step;
    if (gSelRider[player] >= (int)gRiderCount) {
        gSelRider[player] = 0;
    }
    if (gSelRider[player] < 0) {
        gSelRider[player] = (int)gRiderCount - 1;
    }
    guiHide(selPicture(selSlot(1)));
    guiHide(selText(selSlot(1)));
    selStartCaption(gRiderCaptions[gSelRider[gSelPlayer]]);
}

/* Rider stage, left/right press: prepares the next preview (name and picture) and starts the slide. */
BMX_ADDR(0x00444bc0)
void selRiderPreview(int step)
{
    int next = gSelRider[gSelPlayer] + step;
    if (next >= (int)gRiderCount) {
        next = 0;
    } else if (next < 0) {
        next = (int)gRiderCount - 1;
    }
    gSelTick++;
    const char *name = zstringGet(gRiderNames[next]);
    guiSetText(selText(selSlot(1)), name);
    gSelSlide[selSlot(1)] = step << 7;
    GuiElement *picture = selPicture(selSlot(1));
    ZPrimAsset *sprite = picture->sprite;
    zprimCopyTexture(zprimLoadAsset(gRiderTextures[next]), sprite);
    gSelFlipTick = gSelTick;
    gSelSlideDir = -step;
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401bb8, selSlideStep);
    if (step != 0) {
        menuPlaySound(2000);
    }
}

/* Opens the rider stage once the screen has settled: loads the preview group and shows the first rider. */
BMX_ADDR(0x00444d20)
void selRiderOpen(void)
{
    if ((uint32_t)gSelTick < (uint32_t)gSelReadyTick) {
        return;
    }
    if (zprimGroupLoaded(3) == 0) {
        zprimLoadGroup(3, 0xe004);
        selSpriteSlot(0, 0, 0, 4, 0x10);
        selSpriteSlot(1, 0, 0, 4, 0x10);
        selSpriteSlot(2, 0, 0, 4, 0x10);
        selSpriteSlot(1, 0x80, 0xc0, 8, 0x100);
        selSpriteSlot(0, 0x80, 0xc0, 8, 0x100);
    }
    gSelPicturesOn = 1;
    menuShow(0xa0090002);
    menuShow(0xa0090010);
    menuSetText(0xa0090010, zstringGet(0xd6));
    menuShow(0xa0090011);
    menuSetText(0xa0090011, zstringGet(0xd7));
    gSelDimA = 0x80;
    gSelDimB = 0xc0;
    gSelRider[gSelPlayer]++;
    selRiderPreview(-1);
    gSelSlideDir = 1;
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401bb8, selSlideStep);
    gSelCommitFn = BMX_FNPTR(SelStepFn, 0x00401dc0, selRiderCommit);
    gSelPreviewFn = BMX_FNPTR(SelStepFn, 0x0040129e, selRiderPreview);
    menuHide(0xa0090010);
    menuHide(0xa0090011);
    selShowHeader();
    menuShow(0xa0090018);
    menuShow(0xa0090019);
    menuShow(0xa009001a);
    menuShow(0xa009001b);
    menuShow(0xa009001c);
    menuShow(0xa009001d);
    menuShow(0xa0090001);
    menuShow(0xa009001e);
}

/* Leaves the rider stage: greys the chosen rider's picture, resets the carousel and frees the group. */
BMX_ADDR(0x00444fc0)
void selRiderClose(void)
{
    ZPrimAsset *sprite = guiFindById(0xa009000d)->sprite;
    ZPrimAsset *asset = zprimLoadAsset(gRiderTextures[gSelRider[gSelPlayer]]);
    zprimReplaceVramGrey(asset, sprite, 8, 0xc0);
    selResetCarousel();
    gSelFlipTick = gSelTick;
    gSelPreviewFn = NULL;
    gSelCommitFn = NULL;
    selHideHeader();
    zprimFreeGroup(3);
    menuHide(0xa009001c);
    menuHide(0xa009001d);
    menuHide(0xa0090001);
    zmemdmmOptimize();
}

/* Address of level entry `index` (0x27 bytes) of a rider: {kind, variant, ...}. */
static const uint8_t *selLevelEntry(const RiderSave *rider, int index)
{
    return rider->data + index * 0x27;
}

/* Level stage, slide ended: applies the step and starts the caption of the level (or "locked"). */
BMX_ADDR(0x00445090)
void selLevelCommit(int step)
{
    gSelLevel += step;
    guiShow(selPicture(selSlot(0)));
    guiShow(selPicture(selSlot(1)));
    guiHide(selText(selSlot(1)));
    const SelCaption *caption;
    if (gPlayerCount == 2) {
        caption = gSelLevel < (int)gSelLevelCount ? NULL : gCaptionLocked;
    } else if (gSelLevel < 6) {
        if (gSelLevel >= (int)gSelLevelCount) {
            caption = gCaptionLocked;
        } else {
            const uint8_t *entry = selLevelEntry(&gRiders[gSelRider[gSelPlayer]], gSelLevel);
            const unsigned kind = entry[0];
            if (kind > 2) {
                gCaptionOn = 0;
                gCaptionLine = 3;
                return;
            }
            caption = gCaptionKinds[kind + (entry[1] == 4)];
        }
    } else if (gSelLevel >= (int)gSelLevelCount) {
        caption = gCaptionLocked;
    } else {
        const RiderSave *rider = &gRiders[gSelRider[gSelPlayer]];
        caption = gCaptionOrders[rider->data[0xe3 + gSelLevel]];
    }
    selStartCaption(caption);
    gCaptionOn = 0;
    gCaptionLine = 3;
}

/* Level stage, first draw: fills the preview for level gSelLevel+step and shows the pages. */
BMX_ADDR(0x00445290)
void selLevelRefresh(int step)
{
    const int index = gSelLevel + step;
    ZPrimAsset *sprite = selPicture(selSlot(1))->sprite;
    ZPrimAsset *asset = zprimLoadAsset((uint16_t)gLevelTextures[index]);
    if (index >= (int)gSelLevelCount) {
        zprimReplaceVramGrey(asset, sprite, 8, 0xc0);
    } else {
        zprimCopyTexture(asset, sprite);
    }
    gSelFlipTick = gSelTick;
    guiShow(selPicture(selSlot(0)));
    guiShow(selPicture(selSlot(1)));
    guiSetText(selText(menuWrap(gSelPage + step * 2, gSelPages)), EMPTY_TEXT);
    if (index >= (int)gSelLevelCount) {
        guiSetText(selText(selSlot(step)), zstringGet(0x3a));
    } else {
        guiSetText(selText(selSlot(step)), zstringGet((uint16_t)gLevelNames[index]));
    }
    gSelSlideDir = 0;
    gFrontendStep = NULL;
    gSelCommitFn = BMX_FNPTR(SelStepFn, 0x00401519, selLevelCommit);
}

/* Level stage, left/right press: shows the next level's preview sliding in (error sound at the ends). */
BMX_ADDR(0x00445490)
void selLevelPreview(int step)
{
    const int index = gSelLevel + step;
    if (index >= 12 || index < 0) {
        menuPlaySound(0x7d4);
        return;
    }
    gSelTick++;
    guiShow(selPicture(selSlot(0)));
    guiShow(selPicture(selSlot(1)));
    if (step > 0) {
        gSelSlide[selSlot(0)] = 0;
        gSelSlide[selSlot(1)] = 0xf0;
    } else {
        gSelSlide[selSlot(0)] = 0;
        gSelSlide[selSlot(1)] = -0xf0;
    }
    ZPrimAsset *sprite = selPicture(selSlot(1))->sprite;
    if (index >= (int)gSelLevelCount) {
        guiSetText(selText(selSlot(step)), zstringGet(0x3a));
    } else {
        guiSetText(selText(selSlot(step)), zstringGet((uint16_t)gLevelNames[index]));
    }
    ZPrimAsset *asset = zprimLoadAsset((uint16_t)gLevelTextures[index]);
    if (index >= (int)gSelLevelCount) {
        zprimReplaceVramGrey(asset, sprite, 8, 0xc0);
    } else {
        zprimCopyTexture(asset, sprite);
    }
    gSelFlipTick = gSelTick;
    gSelSlideDir = -step;
    gSelCommitFn = BMX_FNPTR(SelStepFn, 0x00401519, selLevelCommit);
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401bb8, selSlideStep);
    if (step != 0) {
        menuPlaySound(2000);
    }
}

/* Level stage: opens once the screen settled; the choice is the last level unlocked by any rider. */
BMX_ADDR(0x00445760)
void selLevelOpen(void)
{
    if ((uint32_t)gSelTick < (uint32_t)gSelReadyTick) {
        return;
    }
    zprimLoadGroup(3, 0xe003);
    gSelPicturesOn = 1;
    gSelPages = 2;
    gSelExtentC = 0xf0;
    gSelExtentA = 0x14;
    if (gPlayerCount == 1) {
        gSelLevelCount = gRiders[gSelRider[gSelPlayer]].levelCount;
    } else {
        uint8_t most = 1;
        for (int i = 0; i < 12; i++) {
            if (gRiders[i].levelCount > most) {
                most = gRiders[i].levelCount;
            }
        }
        gSelLevelCount = most;
    }
    menuShow(0xa0090010);
    menuSetText(0xa0090010, zstringGet(0xdc));
    menuShow(0xa0090011);
    menuSetText(0xa0090011, zstringGet(0xdd));
    selSpriteSlot(0, 0, 0, 8, 0x100);
    selSpriteSlot(1, 0, 0, 8, 0x100);
    selSpriteSlot(0, 0xf0, 0xb8, 8, 0x100);
    selSpriteSlot(1, 0xf0, 0xb8, 8, 0x100);
    gSelDimA = 0xf0;
    gSelDimB = 0xb4;
    selShowHeader();
    guiSetSpriteOffset(guiFindById(0xa009000d), 0, 0);
    guiSetSpriteOffset(guiFindById(0xa009000e), 0xf0, 0);
    menuShow(0xa009000d);
    menuShow(0xa009000e);
    menuHide(0xa0090010);
    menuHide(0xa0090011);
    const int next = gSelLevel + 1;
    gSelPreviewFn = BMX_FNPTR(SelStepFn, 0x00401852, selLevelPreview);
    gFrontendStep = NULL;
    gSelLevel = next;
    selLevelPreview(-1);
    gSelSlideDir = 1;
}

/* Leaves the level stage: greys the chosen level's picture and frees the preview group. */
BMX_ADDR(0x00445a10)
void selLevelClose(void)
{
    ZPrimAsset *sprite = guiFindById(0xa009000d)->sprite;
    ZPrimAsset *asset = zprimLoadAsset((uint16_t)gLevelTextures[gSelLevel]);
    zprimReplaceVramGrey(asset, sprite, 8, 0xc0);
    gSelFlipTick = gSelTick;
    selResetCarousel();
    menuShow(0xa009000d);
    menuHide(0xa009000e);
    guiSetSpriteOffset(guiFindById(0xa009000e), 0x78, 0x78);
    gSelPages = 2;
    gSelExtentC = 0x80;
    gSelExtentA = 0x10;
    gSelPreviewFn = NULL;
    selHideHeader();
    zprimFreeGroup(3);
}

/* Style stage, slide ended: applies the step. */
BMX_ADDR(0x00445b00)
void selStyleCommit(int step)
{
    gSelStyle[gSelPlayer] += step;
    menuShow(0xa0090005);
    guiHide(selPicture(selSlot(1)));
    guiHide(selText(selSlot(1)));
}

/* Style stage, left/right press: "<style string> n" under the next preview (error sound at the ends). */
BMX_ADDR(0x00445b90)
void selStylePreview(int step)
{
    const int index = gSelStyle[gSelPlayer] + step;
    if (index >= 5 || index < 0) {
        menuPlaySound(0x7d4);
        return;
    }
    char name[100];
    zstrFormat(name, "%s %d", zstringGet(0xdb), index + 1);
    gSelTick++;
    guiSetText(selText(selSlot(1)), name);
    gSelSlide[gSelPage ^ 1] = step << 7;
    ZPrimAsset *sprite = guiFindById(gSelPicIds[gSelPage ^ 1])->sprite;
    ZPrimAsset *asset = zprimLoadAsset(gStyleTextures[index + gSelRider[gSelPlayer] * 5]);
    if (index >= (int)gSelStyleCount) {
        zprimReplaceVramGrey(asset, sprite, 8, 0xc0);
        guiSetText(selText(selSlot(1)), zstringGet(0x3a));
    } else {
        zprimCopyTexture(asset, sprite);
    }
    gSelFlipTick = gSelTick;
    gSelSlideDir = -step;
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401bb8, selSlideStep);
    if (step != 0) {
        menuPlaySound(2000);
    }
}

/* Frees the style preview sprites. */
static void selFreeStyleSprites(void)
{
    for (int i = 0; i < 16; i++) {
        if (gStyleSprites[i] != NULL) {
            zprimFreeSprite(gStyleSprites[i]);
            gStyleSprites[i] = NULL;
        }
    }
}

/* Style stage: loads the rider's style group and makes a sprite for each extra picture of it. */
BMX_ADDR(0x00445d90)
void selStyleOpen(void)
{
    if ((uint32_t)gSelTick < (uint32_t)gSelReadyTick) {
        return;
    }
    if (zprimGroupLoaded(3) == 0 || gSelPlayer == 0 || (gSelPlayer == 1 && gSelRider[0] != gSelRider[1])) {
        selSpriteSlot(0, 0, 0, 4, 0x10);
        selSpriteSlot(1, 0, 0, 4, 0x10);
        selSpriteSlot(2, 0, 0, 4, 0x10);
        selSpriteSlot(1, 0x80, 0xc0, 8, 0x100);
        selSpriteSlot(0, 0x80, 0xc0, 8, 0x100);
        zprimFreeGroup(3);
        zprimLoadGroup(3, (uint16_t)(gStyleGroups[gSelRider[gSelPlayer]] | 0xe000));
    }
    const int player = gSelPlayer;
    gSelPicturesOn = 0;
    gSelStyleCount = gRiders[gSelRider[player]].styleCount;
    gSelStyle[player] = (int)gSelStyleCount - 1;
    menuHide(0xa0090010);
    menuHide(0xa0090011);
    menuShow(0xa0090002);
    menuSetText(0xa0090012, EMPTY_TEXT);
    menuSetText(0xa0090013, EMPTY_TEXT);
    menuSetText(0xa0090014, EMPTY_TEXT);
    selFreeStyleSprites();
    const int count = zprimGroupCount(3);
    gSelExtentB = (uint16_t)(count - 6);
    for (int i = 6; i < count; i++) {
        ZPrimAsset *sprite = zprimCreateSpriteSmall(0x54, 0x30, 4, 0x10);
        gStyleSprites[i - 6] = sprite;
        zprimCopyTexture(zprimGroupSelect(3, (uint32_t)i), sprite);
        gSelFlipTick = gSelTick;
    }
    gSelStyle[gSelPlayer]++;
    selStylePreview(-1);
    gSelSlideDir = 1;
    selShowHeader();
    menuHide(0xa0090018);
    menuHide(0xa0090019);
    menuHide(0xa009001a);
    menuHide(0xa009001b);
    menuHide(0xa009001c);
    menuHide(0xa009001d);
    menuHide(0xa0090001);
    menuHide(0xa009001e);
    menuShow(0xa0090004);
    gSelDimA = 0x80;
    gSelDimB = 0xc0;
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401bb8, selSlideStep);
    gSelPreviewFn = BMX_FNPTR(SelStepFn, 0x0040128a, selStylePreview);
    gSelCommitFn = BMX_FNPTR(SelStepFn, 0x00401a5a, selStyleCommit);
}

/* Leaves the style stage: greys the chosen style's picture and frees sprites and group. */
BMX_ADDR(0x00446170)
void selStyleClose(void)
{
    ZPrimAsset *sprite = guiFindById(0xa009000d)->sprite;
    ZPrimAsset *asset = zprimLoadAsset(gStyleTextures[gSelRider[gSelPlayer] * 5 + gSelStyle[gSelPlayer]]);
    zprimReplaceVramGrey(asset, sprite, 8, 0xc0);
    gSelFlipTick = gSelTick;
    selResetCarousel();
    menuHide(0xa0090005);
    selFreeStyleSprites();
    menuHide(0xa0090004);
    gSelPreviewFn = NULL;
    gSelCommitFn = NULL;
    selHideHeader();
    zprimFreeGroup(3);
    zmemdmmOptimize();
}

/* Bike stage tick: every fifth frame steps the preview's animation texture (greyed when the bike is locked).
 */
BMX_ADDR(0x00446280)
void selBikeIdle(void)
{
    if ((uint32_t)gSelTick % 5 != 2) {
        return;
    }
    gStyleFrame = menuWrap(gStyleFrame + 1, 0x20);
    ZPrimAsset *sprite = guiFindById(gSelPicIds[menuWrap(gSelPage, gSelPages)])->sprite;
    ZPrimAsset *asset = zprimLoadAsset((uint32_t)(gStyleFrame + 0x4096));
    const int player = gSelPlayer;
    if (gSelBike[player] >= (int)gSelBikeCount) {
        zprimReplaceVramGrey(asset, sprite, 4, 0xc0);
    } else {
        const int index = gSelBike[player] + gSelRider[player] * 5;
        zprimReplaceVramPalette(asset, sprite, 4, (uint8_t)(gBikePalettes[index] - 1));
    }
    gSelFlipTick = gSelTick;
}

/* Bike stage, slide ended: applies the step, starts the bike's caption and shows the animated picture. */
BMX_ADDR(0x00446390)
void selBikeCommit(int step)
{
    const int player = gSelPlayer;
    gSelBike[player] += step;
    guiHide(selPicture(selSlot(1)));
    guiHide(selText(selSlot(1)));
    const SelCaption *caption;
    if (gSelBike[gSelPlayer] >= (int)gSelBikeCount) {
        caption = gBikeLocked;
    } else {
        caption = gBikeCaptions[gSelBike[gSelPlayer] + gSelRider[gSelPlayer] * 5];
    }
    selStartCaption(caption);
    gCaptionOn = 0;
    gCaptionLine = 3;
    guiSetSpriteOffset(guiFindById(gSelPicIds[2]), 0, 0xa0);
    menuShow(gSelPicIds[2]);
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402248, selBikeIdle);
}

/* Bike stage, left/right press: shows the next bike's name and palette-tinted preview. */
BMX_ADDR(0x004464c0)
void selBikePreview(int step)
{
    const int limit = (int)gSelBikeCount > 4 ? (int)gSelBikeCount : 4;
    const int player = gSelPlayer;
    const int index = gSelBike[player] + step;
    if (index >= limit || index < 0) {
        menuPlaySound(0x7d4);
        return;
    }
    const int rider = gSelRider[player];
    guiSetText(selText(selSlot(1)), zstringGet((uint16_t)(gBikeNames[rider] + index)));
    gSelSlide[selSlot(1)] = step << 7;
    const uint16_t palette = (uint16_t)(gBikePalettes[index + gSelRider[gSelPlayer] * 5] - 1);
    guiSetPalette(selPicture(selSlot(1)), palette);
    ZPrimAsset *sprite = selPicture(selSlot(1))->sprite;
    ZPrimAsset *asset = zprimLoadAsset((uint32_t)(gStyleFrame + 0x4096));
    if (index >= (int)gSelBikeCount) {
        zprimReplaceVramGrey(asset, sprite, 4, 0xc0);
        guiSetText(selText(selSlot(1)), zstringGet(0x3a));
    } else {
        zprimReplaceVramPalette(asset, sprite, 4,
                                (uint8_t)(gBikePalettes[index + gSelRider[gSelPlayer] * 5] - 1));
    }
    gSelFlipTick = gSelTick;
    gSelSlideDir = -step;
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401bb8, selSlideStep);
    if (step != 0) {
        menuPlaySound(2000);
    }
}

/* Bike stage: loads the bike group and shows the rider's bikes. */
BMX_ADDR(0x00446730)
void selBikeOpen(void)
{
    if ((uint32_t)gSelTick < (uint32_t)gSelReadyTick) {
        return;
    }
    if (zprimGroupLoaded(3) == 0) {
        zprimLoadGroup(3, 0xe002);
        selSpriteSlot(0, 0, 0, 4, 0x10);
        selSpriteSlot(1, 0, 0, 4, 0x10);
        selSpriteSlot(2, 0, 0, 4, 0x10);
        selSpriteSlot(2, 0x80, 0x46, 4, 0x10);
        selSpriteSlot(1, 0x80, 0x80, 4, 0x100);
        selSpriteSlot(0, 0x80, 0x80, 4, 0x100);
    }
    const int player = gSelPlayer;
    gSelBikeCount = gRiders[gSelRider[player]].bikeCount;
    if ((int)gSelBikeCount - 1 < gSelBike[player]) {
        gSelBike[player] = (int)gSelBikeCount - 1;
    }
    gSelPicturesOn = 1;
    menuSetText(0xa0090012, EMPTY_TEXT);
    menuSetText(0xa0090013, EMPTY_TEXT);
    gSelDimA = 0x80;
    gSelDimB = 0x80;
    menuShow(0xa0090002);
    menuHide(0xa0090010);
    menuHide(0xa0090011);
    selResetCarousel();
    guiFindById(0xa0090003)->sprite = gSelSprites[1];
    menuShow(0xa0090003);
    ZPrimAsset *sprite = guiFindById(0xa0090003)->sprite;
    zprimCopyTexture(zprimLoadAsset(gBikeTextures[gSelRider[gSelPlayer]]), sprite);
    gSelFlipTick = gSelTick;
    gSelBike[gSelPlayer]++;
    selBikePreview(-1);
    gSelSlideDir = 1;
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401bb8, selSlideStep);
    gSelPreviewFn = BMX_FNPTR(SelStepFn, 0x00401b40, selBikePreview);
    gSelCommitFn = BMX_FNPTR(SelStepFn, 0x00401eba, selBikeCommit);
    selShowHeader();
    menuShow(0xa0090018);
    menuShow(0xa0090019);
    menuShow(0xa009001a);
    menuShow(0xa009001b);
    menuShow(0xa009001e);
}

/* Leaves the bike stage: greys the picture, drops its sprite and frees the group. */
BMX_ADDR(0x00446a20)
void selBikeClose(void)
{
    ZPrimAsset *sprite = guiFindById(0xa009000d)->sprite;
    zprimReplaceVramGrey(zprimLoadAsset((uint32_t)(gStyleFrame + 0x4096)), sprite, 4, 0xc0);
    gSelFlipTick = gSelTick;
    selResetCarousel();
    gSelPreviewFn = NULL;
    selHideHeader();
    guiFindById(0xa0090003)->sprite = NULL;
    menuHide(0xa0090003);
    zprimFreeGroup(3);
    zmemdmmOptimize();
}

/*
 * Button sequences typed on the setup screen (the d-pad directions and one button, as pad or alternate
 * messages) unlock riders, bikes, styles and levels. Each pattern is matched against the last 16 messages;
 * zeros at the end of a pattern match anything.
 */
static const int16_t kCheatPatterns[12][16] = {
    {0x1f, 0x1f, 0x20, 0x21, 0x1e, 0x1e, 0x23},
    {0x2c, 0x2c, 0x2d, 0x2e, 0x2b, 0x2b, 0x30},
    {0x1e, 0x1e, 0x1e, 0x1f, 0x1e, 0x1e, 0x1e, 0x21, 0x23},
    {0x2b, 0x2b, 0x2b, 0x2c, 0x2b, 0x2b, 0x2b, 0x2e, 0x30},
    {0x1e, 0x20, 0x1e, 0x1f, 0x1e, 0x21, 0x20, 0x21, 0x23},
    {0x2b, 0x2d, 0x2b, 0x2c, 0x2b, 0x2e, 0x2d, 0x2e, 0x30},
    {0x20, 0x1e, 0x21, 0x1f, 0x20, 0x1f, 0x21, 0x1e, 0x20, 0x23},
    {0x2d, 0x2b, 0x2e, 0x2c, 0x2d, 0x2c, 0x2e, 0x2b, 0x2d, 0x30},
    {0x1e, 0x1e, 0x1f, 0x1f, 0x20, 0x21, 0x20, 0x23},
    {0x2b, 0x2b, 0x2c, 0x2c, 0x2d, 0x2e, 0x2d, 0x30},
    {0x1f, 0x20, 0x1f, 0x20, 0x1f, 0x20, 0x1f, 0x23},
    {0x2c, 0x2d, 0x2c, 0x2d, 0x2c, 0x2d, 0x2c, 0x30},
};

/* Plays the unlock sound and forgets the typed messages. */
static void selCheatDone(void)
{
    menuPlaySound(0x7d2);
    for (int i = 0; i < 16; i++) {
        gCheatRing[i] = 0;
    }
}

/* The unlock a matched pattern pair performs; later kinds also try the unlocks below them (fall through). */
static void selCheatApply(int kind)
{
    RiderSave *rider = &gRiders[gSelRider[gSelPlayer]];
    switch (kind) {
    case 0:
        if (gRiderCount < 0xb && gSelStage == 0) {
            gRiderCount = 0xb;
            selCheatDone();
        }
        break;
    case 1:
        if (gRiderCount > 10 && gSelStage == 0) {
            gRiderCount = 0xc;
            break;
        }
        /* fall through */
    case 2:
        if (rider->bikeCount < 5 && gSelStage == 1) {
            rider->bikeCount = 5;
            gSelBikeCount = 5;
            selBikeCommit(0);
            selCheatDone();
        }
        /* fall through */
    case 3:
        if (rider->styleCount < 5 && gSelStage == 2) {
            rider->styleCount = 5;
            gSelStyleCount = 5;
            selCheatDone();
        }
        /* fall through */
    case 4:
        if (rider->levelCount < 0xc && gSelStage == 3) {
            rider->levelCount = 0xc;
            gSelLevelCount = 0xc;
            selCheatDone();
        }
        break;
    default:
        break;
    }
}

/* Records a message in the cheat ring and runs the unlock of every pattern it completes. */
BMX_ADDR(0x00446ac0)
void selCheatInput(uint16_t message)
{
    gCheatRing[gCheatPos] = message;
    gCheatPos = (uint16_t)((gCheatPos + 1) & 0xf);
    for (int kind = 0; kind < 6; kind++) {
        for (int variant = 0; variant < 2; variant++) {
            const int16_t *pattern = kCheatPatterns[kind * 2 + variant];
            for (uint32_t start = 0; start < 16; start++) {
                if ((int16_t)gCheatRing[start] != pattern[0]) {
                    continue;
                }
                int matched = 0;
                for (uint32_t j = 0; j < 16; j++) {
                    if (pattern[j] == (int16_t)gCheatRing[(start + j) & 0xf] || pattern[j] == 0) {
                        matched++;
                    }
                }
                if (matched == 16) {
                    selCheatApply(kind);
                    break;
                }
            }
        }
    }
}

/* Opens the setup screen for the mode picked on the main menu; the rest happens in selScreenLayout. */
BMX_ADDR(0x00447540)
int selScreenOpen(void)
{
    guiDestroyAll();
    gSelSprites[2] = NULL;
    gSelSprites[0] = NULL;
    gSelSprites[1] = NULL;
    switch (gGameModeSel) {
    case 0:
        guiLoadBackdrop(0xa00e, 3);
        break;
    case 1:
        guiLoadBackdrop(0xa010, 5);
        break;
    case 2:
        guiLoadBackdrop(0xa00f, 4);
        break;
    case 3:
        guiLoadBackdrop(0xa006, 9);
        break;
    default:
        break;
    }
    zprimLoadGroup(2, 0xe001);
    guiRequestScreen(0xa009, BMX_FNPTR(GuiHookFn, 0x00401aff, selScreenHook));
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402491, selScreenLayout);
    return 0;
}

/* Leaves the setup screen: frees the sprites and groups, then goes to the main menu or starts the game. */
BMX_ADDR(0x00447600)
void selScreenLeave(void)
{
    if ((uint32_t)gSelReadyTick > (uint32_t)gSelTick) {
        return;
    }
    if (gSelSprites[1] != NULL) {
        zprimFreeSprite(gSelSprites[1]);
    }
    if (gSelSprites[0] != NULL) {
        zprimFreeSprite(gSelSprites[0]);
    }
    if (gSelSprites[2] != NULL) {
        zprimFreeSprite(gSelSprites[2]);
    }
    menuFrameFree();
    zprimFreeGroup(2);
    zprimFreeGroup(3);
    gFrontendStep = NULL;
    guiDestroyAll();
    if (gSelStage == -1) {
        mainMenuInit(1);
    } else if (gSelStage == 4) {
        frontendRequestExit(0);
    }
}

/* Final stage: stores the picks into the player setups and level, then fades the header out. */
BMX_ADDR(0x004476c0)
void selCommitSetup(void)
{
    guiSetSpriteOffset(guiFindById(0xa009000d), 0, 0);
    guiSetSpriteOffset(guiFindById(0xa009000e), 0x78, 0);
    menuShow(0xa009000d);
    if ((uint32_t)gSelTick < (uint32_t)gSelReadyTick) {
        return;
    }
    guiSetSpriteOffset(guiFindById(0xa009000f), 0xf0, 0);
    menuShow(0xa0090006);
    gPlayers[0].rider = (uint8_t)gSelRider[0];
    gPlayers[1].rider = (uint8_t)gSelRider[1];
    gPlayers[0].style = (uint8_t)gSelStyle[0];
    gPlayers[1].style = (uint8_t)gSelStyle[1];
    gPlayers[0].bike = (uint8_t)gSelBike[0];
    gPlayers[1].bike = (uint8_t)gSelBike[1];
    gSelLevelSetting = (uint8_t)gSelLevel;
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004022b1, selLeaveStep);
}

/* Replaces working sprite `slot` (0..2) by a new w x h sprite. */
BMX_ADDR(0x004477e0)
void selSpriteSlot(int slot, int w, int h, int bpp, int colors)
{
    static const int kSlotIndex[3] = {2, 0, 1};
    if (slot < 0 || slot > 2) {
        return;
    }
    const int index = kSlotIndex[slot];
    zprimAssetFree(gSelSprites[index]);
    gSelSprites[index] = zprimCreateSpriteSmall((uint16_t)w, (uint16_t)h, (uint16_t)bpp, (uint16_t)colors);
}

/* Puts the carousel back to its first page with its three pictures at rest (and no palette tint). */
BMX_ADDR(0x004478b0)
void selResetCarousel(void)
{
    gSelPage = 0;
    gSelSlide[2] = 0;
    gSelSlide[1] = 0;
    gSelSlide[0] = 0;
    menuShow(0xa009000d);
    menuHide(0xa009000e);
    menuHide(0xa009000f);
    guiSetPalette(guiFindById(0xa009000d), 0);
    guiSetPalette(guiFindById(0xa009000e), 0);
    guiSetPalette(guiFindById(0xa009000f), 0);
    guiSetSpriteOffset(guiFindById(0xa009000d), 0, 0);
    guiSetSpriteOffset(guiFindById(0xa009000e), 0, 0);
    guiSetSpriteOffset(guiFindById(0xa009000f), 0, 0);
}

/* Moves to the next (delta 1) or previous (delta -1) stage / player, closing the one that is left. */
BMX_ADDR(0x004479d0)
void selChangeStage(int delta)
{
    char label[40];
    selStartCaption(NULL);
    menuHide(0xa0090002);
    gSelReadyTick = gSelTick + 2;
    if (gSelStage == 3 || (gSelPlayer == 0 && delta < 0) ||
        (gSelPlayer == (int)gPlayerCount - 1 && delta > 0)) {
        switch ((uint32_t)gSelStage) {
        case 0:
            selRiderClose();
            break;
        case 1:
            selBikeClose();
            break;
        case 2:
            selStyleClose();
            break;
        case 3:
            selLevelClose();
            break;
        default:
            break;
        }
    }
    gSelPlayer = menuWrap(gSelPlayer + delta, gPlayerCount);
    if (gSelStage == 3 || (gSelPlayer == 0 && delta > 0) ||
        (gSelPlayer == (int)gPlayerCount - 1 && delta < 0)) {
        gSelStage += delta;
    }
    if (gPlayerCount > 1 && gSelStage < 3) {
        zstrFormat(label, zstringGet(0x3b), gSelPlayer + 1);
        menuSetText(0xa0090002, label);
    }
    gSelReadyTick = gSelTick + 2;
    switch (gSelStage) {
    case -1:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402315, selScreenLeave);
        break;
    case 0:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004025a9, selRiderOpen);
        break;
    case 1:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x0040244b, selBikeOpen);
        break;
    case 2:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004019f1, selStyleOpen);
        break;
    case 3:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x004020ea, selLevelOpen);
        break;
    case 4:
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00401da7, selCommitSetup);
        break;
    default:
        break;
    }
}

/* Starts the typewriter caption (NULL clears it). */
BMX_ADDR(0x00448040)
void selStartCaption(const SelCaption *caption)
{
    gCaptionOn = 1;
    gCaptionPtr = caption;
    gCaptionTime = 0;
    gCaptionLine = 0;
    gCaptionHold = 4;
    gCaptionAlpha = 0xb4;
}

/* Alpha of a header tab: bright for the current stage, dim for the others. */
static uint32_t selTabAlpha(int tab)
{
    return gSelStage != tab ? 0x50 : 0xff;
}

/* Shows the stage header tabs (the current stage lit) and hides the plain title. */
BMX_ADDR(0x004487b0)
void selShowHeader(void)
{
    menuShow(0xa0090016);
    menuHide(0xa0090006);
    menuSetTint(0xa009001f, 0xff, 0xff, 0xff, selTabAlpha(0));
    menuSetTint(0xa0090020, 0xff, 0xff, 0xff, selTabAlpha(1));
    menuSetTint(0xa0090021, 0xff, 0xff, 0xff, selTabAlpha(2));
    menuSetTint(0xa0090022, 0xff, 0xff, 0xff, selTabAlpha(3));
}

/* Hides the stage header: back to the plain title, tabs dimmed. */
BMX_ADDR(0x00448910)
void selHideHeader(void)
{
    menuHide(0xa0090016);
    menuShow(0xa0090006);
    menuHide(0xa0090007);
    menuHide(0xa0090008);
    menuHide(0xa0090009);
    menuHide(0xa009000a);
    menuHide(0xa009000b);
    menuHide(0xa009000c);
    menuSetTint(0xa009001f, 0xff, 0xff, 0xff, 0x50);
    menuSetTint(0xa0090020, 0xff, 0xff, 0xff, 0x50);
    menuSetTint(0xa0090021, 0xff, 0xff, 0xff, 0x50);
    menuSetTint(0xa0090022, 0xff, 0xff, 0xff, 0x50);
}

/* Slide tick: moves the pages by gSelExtentA, and once the front page is far enough ends the slide. */
BMX_ADDR(0x004496a0)
void selSlideStep(void)
{
    for (int i = 0; i < gSelPages; i++) {
        int delta = (int16_t)gSelExtentA;
        if (gSelSlideDir < 1) {
            delta = -delta;
        }
        const int slot = menuWrap(gSelPage + i, gSelPages);
        gSelSlide[slot] += delta;
        guiShow(selPicture(menuWrap(gSelPage + i, gSelPages)));
        const int16_t x = (int16_t)gSelSlide[menuWrap(gSelPage + i, gSelPages)];
        guiSetSpriteOffset(selPicture(menuWrap(gSelPage + i, gSelPages)), x, 0);
        guiShow(selText(menuWrap(gSelPage + i, gSelPages)));
        const int16_t x3 = (int16_t)(gSelSlide[menuWrap(gSelPage + i, gSelPages)] * 3);
        guiSetTextPos(selText(menuWrap(gSelPage + i, gSelPages)), x3, 0);
    }
    const int travelled = gSelSlide[menuWrap(gSelPage, gSelPages)];
    if ((travelled < 0 ? -travelled : travelled) >= (int16_t)gSelExtentC) {
        gFrontendStep = NULL;
        const int step = -gSelSlideDir;
        gSelPage = menuWrap(gSelPage + step, gSelPages);
        gSelSlideDir = 0;
        if (gSelCommitFn != NULL) {
            gSelCommitFn(step);
        }
    }
}

/* Intro of the setup screen: slides the headers in and then moves on to the stage the entry mode asks for. */
BMX_ADDR(0x004498d0)
void selIntroStep(void)
{
    guiSetTextPos(guiFindById(0xa0090010), (int16_t)(gMenuSlide0 * 6 - 0xf8), 0);
    guiSetTextPos(guiFindById(0xa0090011), (int16_t)(0xf8 - gMenuSlide0 * 6), 0);
    if (gSelResume == 1) {
        if (gSelDimB < 0x80) {
            gSelDimB += 8;
        }
    } else if (gSelResume == 3) {
        if (gSelDimB < 0xb4) {
            gSelDimB += 8;
        } else {
            gSelDimB = 0xb4;
        }
    } else if (gSelDimB < 0xc0) {
        gSelDimB += 8;
    }
    if (menuSlideIn(0xa009001f, 0xa0090020, 0xa0090021, 0xa0090022) == 0) {
        return;
    }
    switch (gSelResume) {
    case 1:
        selChangeStage(2);
        break;
    case 2:
        selChangeStage(3);
        break;
    case 3:
        selChangeStage(4);
        break;
    default:
        selChangeStage(1);
        break;
    }
    gSelResume = 0;
}

/* Slides the headers out and then leaves the screen. */
BMX_ADDR(0x00449a30)
void selLeaveStep(void)
{
    if (menuSlideOut(0xa009001f, 0xa0090020, 0xa0090021, 0xa0090022) != 0) {
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402315, selScreenLeave);
        gSelReadyTick = gSelTick + 2;
    }
}

BMX_ADDR(0x00447bf0)
int BMX_CDECL selScreenHook(int msg, GuiElement *el, int arg3, int arg4)
{
    if ((msg == kGuiMsgButton25 || msg == kGuiMsgIdle) && gGuiIdleMode != 0) {
        gSelStage = -1;
        gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x00402315, selScreenLeave);
        return 1;
    }
    /* Two players share the pad: the first one owns the alternate messages, the second the plain ones. */
    if (gPlayerCount == 2) {
        if (gSelPlayer == 0) {
            if ((msg >= 0x2b && msg <= 0x32) || (msg >= 0x34 && msg <= 0x37)) {
                return 1;
            }
        } else if (gSelPlayer == 1) {
            if ((msg >= 0x1e && msg <= 0x25) || (msg >= 0x27 && msg <= 0x2a)) {
                return 1;
            }
        }
    } else {
        switch (msg) {
        case 0x2b:
        case 0x2c:
        case 0x2d:
        case 0x2e:
        case 0x30:
        case 0x32:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
            return 1;
        case 0x2f:
        case 0x31:
            menuPlaySound(0x7d4);
            return 1;
        default:
            break;
        }
    }
    switch (msg) {
    case kGuiMsgTickAll: {
        gSelTick++;
        if ((uint32_t)gSelTick % 0x34bc0 == 0) {
            gSelTick = 0;
            gSelReadyTick = 0;
            gSelFlipTick = 0;
            selStartCaption(gCaptionPtr);
        }
        if (gFrontendStep != NULL) {
            gFrontendStep();
        }
        if (gSelPending != 0 && (uint32_t)(gSelTick - gSelFlipTick) > 2) {
            selChangeStage(gSelPending);
            gSelPending = 0;
        }
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
    case kGuiMsgNavUp:
    case kGuiMsgNavUpAlt:
    case kGuiMsgNavDown:
    case 0x23:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
    case kGuiMsgNavDownAlt:
    case 0x30:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
        selCheatInput((uint16_t)msg);
        return 1;
    case kGuiMsgNavLeft:
    case kGuiMsgNavLeftAlt:
        if (gSelSlideDir == 0 && gSelPreviewFn != NULL) {
            gSelPreviewFn(-1);
            selCheatInput((uint16_t)msg);
        }
        return 1;
    case kGuiMsgNavRight:
    case kGuiMsgNavRightAlt:
        if (gSelSlideDir == 0 && gSelPreviewFn != NULL) {
            gSelPreviewFn(1);
            selCheatInput((uint16_t)msg);
        }
        return 1;
    case kGuiMsgButton22:
    case kGuiMsgButton24:
    case 0x2f:
    case 0x31: {
        FrontendStepFn idle = BMX_FNPTR(FrontendStepFn, 0x00402248, selBikeIdle);
        if (gFrontendStep != NULL && gFrontendStep != idle) {
            return 1;
        }
        if ((uint32_t)gSelTick <= (uint32_t)gSelReadyTick) {
            return 1;
        }
        switch (gSelStage) {
        case 1:
            if (gSelBike[gSelPlayer] >= (int)gSelBikeCount) {
                menuPlaySound(0x7d4);
                return 1;
            }
            break;
        case 2:
            if (gSelStyle[gSelPlayer] >= (int)gSelStyleCount) {
                menuPlaySound(0x7d4);
                return 1;
            }
            break;
        case 3:
            if (gSelLevel >= (int)gSelLevelCount) {
                menuPlaySound(0x7d4);
                return 1;
            }
            break;
        default:
            break;
        }
        gSelPending = 1;
        menuPlaySound(0x7d2);
        return 1;
    }
    case kGuiMsgButton25:
    case kGuiMsgIdle: {
        FrontendStepFn idle = BMX_FNPTR(FrontendStepFn, 0x00402248, selBikeIdle);
        if (gFrontendStep == NULL || gFrontendStep == idle) {
            gSelPending = -1;
            menuPlaySound(0x7d2);
        }
        return 1;
    }
    default:
        return guiDefaultHookProc(msg, el, arg3, arg4);
    }
}

/* First tick of the setup screen: resets or restores the picks, sets the texts and creates the sprites. */
BMX_ADDR(0x00449a80)
void selScreenLayout(void)
{
    gCaptionLine = 0;
    gCaptionOn = 1;
    if (gSelResume == 0) {
        gSelRider[0] = 0;
        gSelRider[1] = 0;
        gSelBike[0] = 0;
        gSelBike[1] = 0;
        gSelStyle[0] = 0;
        gSelStyle[1] = 0;
        gSelLevel = 0;
    } else {
        gSelRider[0] = gPlayers[0].rider;
        gSelRider[1] = gPlayers[1].rider;
        gSelBike[0] = gPlayers[0].bike;
        gSelStyle[0] = gPlayers[0].style;
        gSelBike[1] = gPlayers[1].bike;
        gSelStyle[1] = gPlayers[1].style;
        gSelLevel = gSelLevelSetting;
    }
    for (int i = 0; i < 16; i++) {
        gCheatRing[i] = 0;
    }
    gSelFlipTick = 0;
    gCheatPos = 0;
    gSelSprites[2] = NULL;
    gSelSprites[0] = NULL;
    gSelSlideDir = 0;
    gSelStage = -1;
    gSelPending = 0;
    gSelUnknown60 = 4;
    gSelTick = 0;
    gSelReadyTick = 0;
    gSelUnknownA0 = 0;
    gSelExtentA = 0x10;
    gSelPages = 2;
    gSelPicturesOn = 1;
    gSelPlayer = -1;
    menuSetText(0xa0090006, zstringGet(0x3c));
    menuSetText(0xa0090008, zstringGet(0x3e));
    menuSetText(0xa0090018, zstringGet(0xd1));
    menuSetText(0xa0090019, zstringGet(0xd2));
    menuSetText(0xa009001a, zstringGet(0xd3));
    menuSetText(0xa009001b, zstringGet(0xd4));
    menuSetText(0xa009001c, zstringGet(0x2e));
    menuSetText(0xa009001d, zstringGet(0x2d));
    uint16_t titleId;
    uint16_t subtitleId;
    switch (gSelResume) {
    case 1:
        gSelExtentC = 0x80;
        gSelDimA = 0x80;
        gSelDimB = 0x80;
        titleId = 0xd8;
        subtitleId = 0xd9;
        break;
    case 2:
        gSelExtentC = 0x80;
        gSelDimA = 0x80;
        gSelDimB = 0xc0;
        titleId = 0xda;
        subtitleId = 0xdb;
        break;
    case 3:
        gSelExtentC = 0x78;
        gSelDimA = 0xf0;
        gSelDimB = 0xb4;
        titleId = 0xdc;
        subtitleId = 0xdd;
        break;
    default:
        gSelExtentC = 0x80;
        gSelDimA = 0x80;
        gSelDimB = 0x20;
        titleId = 0xd6;
        subtitleId = 0xd7;
        break;
    }
    menuSetText(0xa0090010, zstringGet(titleId));
    menuSetText(0xa0090011, zstringGet(subtitleId));
    char line[32];
    zstrcpy(line, zstringGet(0xd6));
    zstrcat(line, " ");
    zstrcat(line, zstringGet(0xd7));
    menuSetText(0xa0090012, line);
    menuSetText(0xa0090014, EMPTY_TEXT);
    for (uint32_t id = 0xa0090010; id <= 0xa0090014; id++) {
        menuSetStyle(id, 1);
    }
    static const uint32_t kDrawn[10] = {0xa009000d, 0xa009000e, 0xa009000f, 0xa0090015, 0xa0090016,
                                        0xa0090017, 0xa009001e, 0xa0090005, 0xa0090004, 0xa0090001};
    for (int i = 0; i < 10; i++) {
        guiFindById(kDrawn[i])->proc = BMX_FNPTR(GuiProcFn, 0x00401f4b, selScreenDraw);
    }
    gSelSprites[2] = zprimCreateSpriteSmall(0, 0, 8, 0x100);
    gSelSprites[0] = zprimCreateSpriteSmall(0, 0, 8, 0x100);
    gSelSprites[1] = zprimCreateSpriteSmall(0, 0, 8, 0x100);
    guiSetSpritePtr(guiFindById(0xa009000d), gSelSprites[2]);
    guiSetSpritePtr(guiFindById(0xa009000e), gSelSprites[0]);
    guiSetSpritePtr(guiFindById(0xa009000f), gSelSprites[1]);
    selResetCarousel();
    menuHide(0xa009000f);
    menuHide(0xa009000e);
    menuHide(0xa009000d);
    menuFrameInit();
    menuSlideReset();
    menuSlideIn(0xa009001f, 0xa0090020, 0xa0090021, 0xa0090022);
    guiSetTextPos(guiFindById(0xa0090010), (int16_t)(gMenuSlide0 * 6 - 0xf8), 0);
    guiSetTextPos(guiFindById(0xa0090011), (int16_t)(0xf8 - gMenuSlide0 * 6), 0);
    selStartCaption(NULL);
    gFrontendStep = BMX_FNPTR(FrontendStepFn, 0x0040149c, selIntroStep);
}

/* Clears a caption line and stores `text` (may be NULL for an empty line) with its font and colour. */
static void selCaptionLine(char *dest, uint16_t *font, uint32_t *x, uint32_t *y, const char *text)
{
    memset(dest, 0, 100);
    if (text != NULL) {
        memcpy(dest, text, strlen(text) + 1);
    }
    *font = 0x8001;
    *x = 0;
    *y = 0;
}

/*
 * Prepares `count` caption entries from the typewriter's position (+ `offset` ring slots ahead): looks up the
 * entry's string and splits it at '#' markers: "#Y"/"#b" start the first line (colour 1/0), "#B"/"#y" the
 * second (colour 0/1). The lines and the first line's measured width go into the 5 slot ring.
 */
BMX_ADDR(0x00448090)
void selCaptionFeed(int count, int offset)
{
    int entries = 0;
    while (gCaptionPtr[entries] != 0) {
        entries++;
    }
    entries++;
    const uint16_t *ids = gCaptionPtr;
    if (count < 1) {
        gCaptionTime = (uint16_t)(gCaptionTime + count);
        return;
    }
    for (int i = 0; i < count; i++) {
        char text[100];
        const uint16_t id = ids[menuWrap(gCaptionTime + i, entries)];
        if (id == 0) {
            text[0] = (char)gGuiEmptyChar;
        } else {
            const char *source = zstringGet(id);
            memcpy(text, source, strlen(source) + 1);
        }
        char *line1 = NULL;
        char *line2 = NULL;
        uint16_t color1 = 1;
        uint16_t color2 = 0;
        if (text[0] != '\0') {
            char *p = text;
            char *marker = text + 1;
            char *start = text + 2;
            char next;
            do {
                if (*p == '#') {
                    *p = '\0';
                    const char kind = *marker;
                    if (kind == 'Y') {
                        line1 = start;
                        color1 = 1;
                    }
                    if (kind == 'b') {
                        line1 = start;
                        color1 = 0;
                    }
                    if (kind == 'B') {
                        line2 = start;
                        color2 = 0;
                    }
                    if (kind == 'y') {
                        line2 = start;
                        color2 = 1;
                    }
                    marker++;
                    start++;
                    p++;
                }
                next = p[1];
                marker++;
                start++;
                p++;
            } while (next != '\0');
        }
        const int slot = menuWrap(gCaptionTime + i + offset, 5);
        CaptionSlot *entry = &gCaptionSlots[slot];
        if (line1 == NULL) {
            memset(entry->text1, 0, 100);
            entry->font1 = 0x8001;
            entry->x1 = 0;
            entry->y1 = 0;
            gCaptionWidths[slot] = 0;
            gCaptionColors[slot].line1 = color1;
        } else {
            selCaptionLine(entry->text1, &entry->font1, &entry->x1, &entry->y1, line1);
            guiSetText(guiFindById(0xa0090017), line1);
            gCaptionWidths[slot] = guiGetTextWidth(guiFindById(0xa0090017));
            gCaptionColors[slot].line1 = color1;
        }
        if (line2 == NULL) {
            memset(entry->text2, 0, 100);
        } else {
            selCaptionLine(entry->text2, &entry->font2, &entry->x2, &entry->y2, line2);
        }
        entry->font2 = 0x8001;
        entry->x2 = 0;
        entry->y2 = 0;
        gCaptionColors[slot].line2 = color2;
    }
    gCaptionTime = (uint16_t)(gCaptionTime + count);
}

/* The typewriter: counts the hold and scroll timers, feeds new lines and draws the five visible ones. */
static void selDrawCaption(void)
{
    if (gCaptionPtr == NULL) {
        return;
    }
    if (gFrontendStep != NULL && gFrontendStep != BMX_FNPTR(FrontendStepFn, 0x00402248, selBikeIdle)) {
        return;
    }
    if (gCaptionHold != 0) {
        if ((uint32_t)gSelTick % 5 == 0) {
            gCaptionHold--;
        }
        if (gCaptionHold == 0) {
            selCaptionFeed(5, 0);
        }
    }
    if (gCaptionAlpha == 0) {
        if ((uint32_t)gSelTick % 5 == 0) {
            gCaptionLine = (uint16_t)(gCaptionLine + gCaptionOn);
            if (gCaptionLine > 0x13) {
                gCaptionLine = 0;
                selCaptionFeed(1, 5);
            }
        }
    } else {
        gCaptionAlpha--;
    }
    if (gCaptionHold != 0) {
        return;
    }
    zprimSetClip(0x82, 0x146, 0x1f4, 0x18a);
    for (int i = 0; i < 5; i++) {
        const int16_t y = (int16_t)(i * 20 - (int)gCaptionLine + 0xd0);
        CaptionSlot *slot = &gCaptionSlots[menuWrap(gCaptionTime + i, 5)];
        zprimSetFont(zprimLoadAsset(slot->font1));
        zprimSet(0x17, gCaptionColors[menuWrap(gCaptionTime + i, 5)].line1);
        zprimDrawText((const uint8_t *)gCaptionSlots[menuWrap(gCaptionTime + i, 5)].text1, 0x82, y);
        slot = &gCaptionSlots[menuWrap(gCaptionTime + i, 5)];
        zprimSetFont(zprimLoadAsset(slot->font2));
        zprimSet(0x17, gCaptionColors[menuWrap(gCaptionTime + i, 5)].line2);
        const uint16_t x = (uint16_t)(gCaptionWidths[menuWrap(gCaptionTime + i, 5)] + 0x82);
        zprimDrawText((const uint8_t *)gCaptionSlots[menuWrap(gCaptionTime + i, 5)].text2, x, y);
    }
    zprimSetClip(0, 0, 0x27f, 0x1df);
}

/*
 * The four stat bars of the rider (and, while previewing bikes, the bike's addition drawn over them in
 * the palette 1 tint). The clip rectangle cuts each bar to its width; the 32 bit values mirror the
 * original's register use, where the high half of an address-like value rides along into the clip.
 */
static void selDrawStatBars(GuiElement *el, int arg3, int arg4)
{
    int riderDelta = 0;
    int bikeDelta = 0;
    if (gSelStage == 0 && gSelSlideDir != 0) {
        riderDelta = -gSelSlideDir;
    } else if (gSelStage == 1 && gSelSlideDir != 0) {
        bikeDelta = -gSelSlideDir;
    }
    for (int i = 0; i < 4; i++) {
        const int raw = gSelRider[gSelPlayer];
        const uint32_t high = (uint32_t)(raw + riderDelta) & 0xffff0000u;
        const int shown = menuWrap(raw + riderDelta, gRiderCount);
        const uint32_t barEnd = (high | (uint16_t)(gStatWidths[shown * 5 + i] << 3)) + 0x17c;
        zprimSetClip(0x17c, 0x40, (int)barEnd, 0x1df);
        guiSetSpriteOffset(el, 0, (int16_t)(i * 25));
        guiSetPalette(el, 0);
        guiLabelProc(kGuiMsgDraw, el, arg3, arg4);
        if (gSelStage == 1 && gSelPreviewFn == BMX_FNPTR(SelStepFn, 0x00401b40, selBikePreview) &&
            gSelBike[gSelPlayer] + bikeDelta < (int)gSelBikeCount) {
            const int player = gSelPlayer;
            const int rawRider = gSelRider[player];
            const int bikeShown = menuWrap(rawRider + riderDelta, gRiderCount);
            const int bike = gSelBike[player];
            const int bikeIndex = bikeShown * 5 + bike + bikeDelta;
            const uint32_t stat = gBikeStats[bikeIndex * 5 + i];
            const uint32_t extra =
                ((uint32_t)bike & 0xffff0000u) | (uint16_t)(stat + (uint16_t)gStatWidths[rawRider * 5 + i]);
            const uint32_t end = extra * 8 + 0x17c;
            const int again = menuWrap(rawRider + riderDelta, gRiderCount);
            const uint32_t start =
                ((end & 0xffff0000u) | (uint16_t)(gStatWidths[again * 5 + i] << 3)) + 0x17c;
            zprimSetClip((int)start, 0x40, (int)end, 0x1df);
            guiSetPalette(el, 1);
            guiSetSpriteOffset(el, (int16_t)(gStatWidths[(gSelRider[gSelPlayer] + riderDelta) * 5 + i] << 3),
                               (int16_t)(i * 25));
            guiLabelProc(kGuiMsgDraw, el, arg3, arg4);
        }
    }
    zprimSetClip(0, 0, 0x27f, 0x1df);
}

/* Draws the connecting lines of the style's four stat callouts (a layer entry of -1 has none). */
static void selDrawCallouts(const int16_t *layers)
{
    if (layers[0] != -1) {
        zprimDrawLine(0xf0, 0, 0x14a, 0);
        zprimDrawLine(0xf0, 1, 0x14a, 1);
        zprimDrawLine(0xdc, 0xf, 0xf0, 0);
        zprimDrawLine(0xdc, 0x10, 0xf0, 1);
    }
    if (layers[3] != -1) {
        zprimDrawLine(0x104, 0x32, 0x14a, 0x32);
        zprimDrawLine(0x104, 0x33, 0x14a, 0x33);
        zprimDrawLine(0xf0, 0x41, 0x104, 0x32);
        zprimDrawLine(0xf0, 0x42, 0x104, 0x33);
    }
    if (layers[1] != -1) {
        zprimDrawLine(0xe6, 0x64, 0x14a, 0x64);
        zprimDrawLine(0xe6, 0x65, 0x14a, 0x65);
    }
    if (layers[2] != -1) {
        zprimDrawLine(0x14a, 0x96, 0xf0, 0x96);
        zprimDrawLine(0x14a, 0x97, 0xf0, 0x97);
    }
}

/*
 * Draw handler (message 0xc) of the setup screen's elements, by element id: the rider's skill icons
 * (0xa0090001), the style callout lines (4), the style layers (5), the picture previews (0xd..0xf, clipped
 * to the preview window), the two frames (0x15 around the previews, 0x16 around the current tab), the
 * typewriter caption (0x17) and the stat bars (0x1e). Other ids draw as plain labels.
 */
BMX_ADDR(0x00448a80)
int BMX_CDECL selScreenDraw(int msg, GuiElement *el, int arg3, int arg4)
{
    if (msg != kGuiMsgDraw) {
        return 0;
    }
    switch (el->id) {
    case 0xa0090001: {
        int riderDelta = 0;
        if (gSelStage == 0 && gSelSlideDir != 0) {
            riderDelta = -gSelSlideDir;
        }
        for (int i = 0; i < 6; i++) {
            const int offset = i * 0x27;
            guiSetSpriteOffset(el, (int16_t)(i * 17), 0);
            int shown = menuWrap(gSelRider[gSelPlayer] + riderDelta, gRiderCount);
            const uint8_t *entry = (const uint8_t *)&gRiders[shown] + 4 + offset;
            switch (entry[0]) {
            case 0:
                guiSetFrame(el, 0);
                break;
            case 1:
                guiSetFrame(el, 1);
                break;
            case 2:
                shown = menuWrap(gSelRider[gSelPlayer] + riderDelta, gRiderCount);
                entry = (const uint8_t *)&gRiders[shown] + 4 + offset;
                guiSetFrame(el, entry[1] == 4 ? 3 : 2);
                break;
            default:
                break;
            }
            guiLabelProc(kGuiMsgDraw, el, arg3, arg4);
            guiSetSpriteOffset(el, (int16_t)(i * 17), 0x32);
            shown = menuWrap(gSelRider[gSelPlayer] + riderDelta, gRiderCount);
            guiSetFrame(el, ((const uint8_t *)&gRiders[shown])[0xed + i]);
            guiLabelProc(kGuiMsgDraw, el, arg3, arg4);
        }
        return 1;
    }
    case 0xa0090004:
        if (gFrontendStep == NULL && gSelStyle[gSelPlayer] < (int)gSelStyleCount) {
            zprimSet(kZprimColor, 0xdddd);
            selDrawCallouts(gStatLayers[gSelRider[gSelPlayer] * 5 + gSelStyle[gSelPlayer]]);
        }
        return 1;
    case 0xa0090005:
        if (gFrontendStep == NULL && gSelStyle[gSelPlayer] < (int)gSelStyleCount) {
            for (int layer = 0; layer < 8; layer++) {
                const int16_t sprite = gStatLayers[gSelRider[gSelPlayer] * 5 + gSelStyle[gSelPlayer]][layer];
                if (sprite != -1) {
                    guiSetSpritePtr(el, gStyleSprites[sprite]);
                    guiSetSpriteOffset(el, gStatPositions[layer][0], gStatPositions[layer][1]);
                    guiLabelProc(kGuiMsgDraw, el, arg3, arg4);
                }
            }
        }
        return 1;
    case 0xa009000d:
    case 0xa009000e:
    case 0xa009000f:
        zprimSetClip(0x8c, 0x64, (uint16_t)(gSelDimA + 0x8c), (uint16_t)(gSelDimB + 0x64));
        guiLabelProc(kGuiMsgDraw, el, arg3, arg4);
        zprimSetClip(0, 0, 0x27f, 0x1df);
        return 1;
    case 0xa0090015:
        menuDrawFrame(0x80, 0x58, (int16_t)(gSelDimA + 0x98), (int16_t)(gSelDimB + 0x70), 0x80,
                      gSelPicturesOn);
        return 1;
    case 0xa0090016:
        if (gOptMainPulse < 0 || gOptMainPulse > 0x40) {
            gOptMainPulseStep = -gOptMainPulseStep;
        }
        gOptMainPulse += gOptMainPulseStep;
        menuDrawFrame(0x1f, (int16_t)(gSelStage * 0x54 + 0x5a), 0x71, (int16_t)(gSelStage * 0x54 + 0xb7),
                      (uint8_t)(0x80 - gOptMainPulse), 1);
        return 1;
    case 0xa0090017:
        selDrawCaption();
        return 1;
    case 0xa009001e:
        selDrawStatBars(el, arg3, arg4);
        return 1;
    default:
        guiLabelProc(kGuiMsgDraw, el, arg3, arg4);
        return 1;
    }
}
