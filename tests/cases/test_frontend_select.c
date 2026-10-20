/* Tests for frontend_select.c: the game setup screen (rider, bike, style and level stages). */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_frontend.h"
#include "zge/zstring.h"

enum { kSelElements = 0x22 };

/* All elements of the screen, fonts where text is measured, and a random state for every global. */
static void selState(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    dtFrontendStub(DT_FS_SOUND | DT_FS_ZPRIM | DT_FS_SCREENS | DT_FS_FRAME | DT_FS_ZASSET | DT_FS_QUIT);
    const int loaded[] = {0, 1, 1};
    dtFrontQueue(DT_Q_GROUPLOADED, loaded, dt_range(1, 3));
    const int counts[] = {dt_range(0, 9), dt_range(0, 9), dt_range(0, 9)};
    dtFrontQueue(DT_Q_GROUPCOUNT, counts, 3);
    dtFrontendElementRange(0xa0090001, kSelElements);
    GuiElement *measured = guiFindById(0xa0090017);
    guiSetText(measured, dtGuiRandomText(8, 0));
    measured->font = dtGuiMakeFont(0x1a0, (uint16_t)dt_range(6, 18));

    gFrontendStep = dt_u32() % 3 == 0 ? NULL : dtFrontStep;
    gGuiIdleMode = dt_range(0, 1);
    gGuiPulse = dt_range(0x10, 0xf0);
    gPlayerCount = (uint8_t)(dt_u32() % 8 == 0 ? dt_range(0, 3) : dt_range(1, 2));
    gSelPlayer = dt_range(0, 1);
    gSelStage = dt_range(-1, 4);
    gSelResume = (uint8_t)dt_range(0, 3);
    gGameModeSel = (uint8_t)dt_range(0, 4);
    gRiderCount = (uint8_t)dt_range(1, 12);
    for (int r = 0; r < 12; r++) {
        gRiders[r].levelCount = (uint8_t)dt_range(0, 12);
        gRiders[r].styleCount = (uint8_t)dt_range(0, 5);
        gRiders[r].bikeCount = (uint8_t)dt_range(0, 5);
        for (int i = 0; i < 0x100; i++) {
            gRiders[r].data[i] = (uint8_t)(dt_u32() % 5 == 0 ? dt_u32() : dt_range(0, 4));
        }
    }
    for (int p = 0; p < 2; p++) {
        gSelRider[p] = dt_range(0, 11);
        gSelBike[p] = dt_range(-1, 5);
        gSelStyle[p] = dt_range(-1, 5);
        gPlayers[p].rider = (uint8_t)dt_u32();
        gPlayers[p].style = (uint8_t)dt_u32();
        gPlayers[p].bike = (uint8_t)dt_u32();
    }
    gSelLevel = dt_range(-1, 12);
    gSelLevelSetting = (uint8_t)dt_u32();
    gSelBikeCount = (uint8_t)dt_range(0, 6);
    gSelStyleCount = (uint8_t)dt_range(0, 6);
    gSelLevelCount = (uint8_t)dt_range(0, 12);
    gSelTick = dt_u32() % 3 == 0 ? dt_range(0, 12) : (int)(dt_u32() % 0x40000);
    gSelReadyTick = dt_u32() % 2 ? gSelTick + dt_range(-2, 2) : dt_range(0, 12);
    gSelFlipTick = gSelTick - dt_range(0, 5);
    gSelPending = dt_u32() % 2 ? 0 : (dt_u32() % 2 ? 1 : -1);
    gSelPages = dt_u32() % 4 == 0 ? 3 : 2;
    gSelPage = dt_range(0, gSelPages - 1);
    for (int i = 0; i < 3; i++) {
        gSelSlide[i] = dt_range(-0x100, 0x100);
    }
    gSelSlideDir = dt_range(-1, 1);
    gSelExtentA = (uint16_t)dt_range(0x8, 0x20);
    gSelExtentB = (uint16_t)dt_u32();
    gSelExtentC = (uint16_t)dt_range(0x60, 0x100);
    gSelDimA = (uint16_t)dt_u32();
    gSelDimB = (uint16_t)dt_range(0x20, 0xc8);
    gSelPicturesOn = (int)dt_u32();
    gStyleFrame = dt_range(0, 0x1f);
    for (int i = 0; i < 16; i++) {
        gStyleSprites[i] =
            dt_u32() % 3 == 0 ? NULL : (ZPrimAsset *)(uintptr_t)(0x70000000u + (uint32_t)i * 0x40);
    }
    for (int i = 0; i < 3; i++) {
        gSelSprites[i] =
            dt_u32() % 3 == 0 ? NULL : (ZPrimAsset *)(uintptr_t)(0x71000000u + (uint32_t)i * 0x40);
    }
    gCaptionPtr = (const SelCaption *)(uintptr_t)gCaptionKinds;
    gCaptionOn = (uint16_t)dt_u32();
    gCaptionLine = (uint16_t)dt_u32();
    gCaptionTime = (uint16_t)dt_range(0, 20);
    gCaptionHold = (uint16_t)dt_range(0, 5);
    gCaptionAlpha = (uint16_t)dt_range(0, 0xb4);
    gSelPreviewFn = dt_u32() % 2 ? NULL : (SelStepFn)(uintptr_t)0x00401b40;
    gSelCommitFn = dt_u32() % 2 ? NULL : (SelStepFn)(uintptr_t)0x00401dc0;
    gMenuSlide0 = dt_range(-0x48, 0x30);
    gMenuSlide1 = dt_range(-0x48, 0x30);
    gMenuSlide2 = dt_range(-0x48, 0x30);
    gMenuSlide3 = dt_range(-0x48, 0x30);
    for (int i = 0; i < 16; i++) {
        gCheatRing[i] = (uint16_t)(dt_u32() % 3 == 0 ? 0 : dt_range(0x1e, 0x31));
    }
    gCheatPos = (uint16_t)dt_range(0, 15);
    uint8_t frameData[0x50];
    for (uint32_t i = 0; i < sizeof frameData; i++) {
        frameData[i] = (uint8_t)dt_u32();
    }
    gMenuFrameSprite = dtGuiMakeAsset(0x40c5, frameData, sizeof frameData);
}

static void setup_state(DtCtx *c)
{
    selState(c);
}

static void setup_step(DtCtx *c)
{
    selState(c);
    gSelTick = (int)(dt_u32() % 2 ? gSelReadyTick + dt_range(0, 3) : gSelReadyTick - dt_range(0, 3));
    if (dt_u32() % 2) {
        gSelStage = dt_range(0, 3);
    }
}

static void setup_step_arg(DtCtx *c)
{
    setup_step(c);
    DT_ARG_INT(c, 0, dt_range(-1, 1));
}

static void setup_cheat(DtCtx *c)
{
    static const int lengths[12] = {7, 7, 9, 9, 9, 9, 10, 10, 8, 8, 8, 8};
    static const int16_t sample[12][10] = {
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
    selState(c);
    int message = dt_range(0x1e, 0x37);
    if (dt_u32() % 3 != 0) { /* a pattern one message short of complete */
        const int p = dt_range(0, 11);
        const int start = dt_range(0, 15);
        for (int j = 0; j < lengths[p] - 1; j++) {
            gCheatRing[(start + j) & 15] = (uint16_t)sample[p][j];
        }
        gCheatPos = (uint16_t)((start + lengths[p] - 1) & 15);
        message = sample[p][lengths[p] - 1];
        gSelStage = dt_u32() % 4 != 0 ? (p < 2 ? 0 : p / 2 - 1) : dt_range(0, 3);
        if (dt_u32() % 2) {
            gRiderCount = (uint8_t)dt_range(9, 12);
            gRiders[gSelRider[gSelPlayer < 0 ? 0 : gSelPlayer & 1]].bikeCount = (uint8_t)dt_range(3, 5);
        }
    }
    DT_ARG_INT(c, 0, message);
}

static void setup_slot(DtCtx *c)
{
    selState(c);
    DT_ARG_INT(c, 0, dt_range(-1, 3));
    DT_ARG_INT(c, 1, dt_range(0, 300));
    DT_ARG_INT(c, 2, dt_range(0, 300));
    DT_ARG_INT(c, 3, dt_range(1, 8));
    DT_ARG_INT(c, 4, dt_range(1, 256));
}

static void setup_caption(DtCtx *c)
{
    selState(c);
    DT_ARG_PTR(c, 0, dt_u32() % 3 == 0 ? NULL : (void *)gCaptionKinds);
}

static void setup_change(DtCtx *c)
{
    selState(c);
    DT_ARG_INT(c, 0, dt_u32() % 2 ? 1 : -1);
    if (dt_u32() % 4 != 0) {
        gSelStage = dt_range(0, 3);
    }
}

static void setup_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTickAll,
                               kGuiMsgTickAll,
                               kGuiMsgTickAll,
                               kGuiMsgNavUp,
                               kGuiMsgNavDown,
                               kGuiMsgNavLeft,
                               kGuiMsgNavRight,
                               kGuiMsgNavUpAlt,
                               kGuiMsgNavDownAlt,
                               kGuiMsgNavLeftAlt,
                               kGuiMsgNavRightAlt,
                               kGuiMsgButton22,
                               kGuiMsgButton24,
                               0x2f,
                               0x31,
                               kGuiMsgButton25,
                               kGuiMsgIdle,
                               0x23,
                               0x27,
                               0x2a,
                               0x30,
                               0x33,
                               0x26,
                               0x34,
                               kGuiMsgDrawAll};
    selState(c);
    gSelPending = dt_u32() % 2 ? 0 : (dt_u32() % 2 ? 1 : -1);
    dtFrontendHookArgs(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
}

static void setup_feed(DtCtx *c)
{
    selState(c);
    static const uint16_t lists[][6] = {
        {0x301, 0x302, 0x303, 0}, {0x300, 0x304, 0}, {0x305, 0}, {0x301, 0x307, 0x308, 0x30a, 0x30b, 0}, {0}};
    uint16_t *list = dt_alloc(sizeof lists[0]);
    memcpy(list, lists[dt_u32() % (sizeof lists / sizeof lists[0])], sizeof lists[0]);
    gCaptionPtr = list;
    gCaptionTime = (uint16_t)(dt_u32() % 7 == 0 ? dt_u32() : (uint32_t)dt_range(0, 12));
    for (int i = 0; i < 5; i++) {
        memset(&gCaptionSlots[i], (int)dt_u32() & 0xff, sizeof gCaptionSlots[i]);
        gCaptionWidths[i] = (uint16_t)dt_u32();
        gCaptionColors[i].line1 = (uint16_t)dt_u32();
        gCaptionColors[i].line2 = (uint16_t)dt_u32();
    }
    DT_ARG_INT(c, 0, dt_range(-1, 6));
    DT_ARG_INT(c, 1, dt_range(0, 6));
}

static void setup_draw(DtCtx *c)
{
    static const uint32_t ids[] = {0xa0090001, 0xa0090004, 0xa0090005, 0xa009000d, 0xa009000e,
                                   0xa009000f, 0xa0090015, 0xa0090016, 0xa0090017, 0xa0090017,
                                   0xa009001e, 0xa009001e, 0xa009001e, 0xa0090002, 0xa0090010};
    selState(c);
    dtFrontendStub(DT_FS_LABEL);
    for (int i = 0; i < 16; i++) { /* a plausible caption: the typewriter needs its slots */
        ((uint8_t *)gCaptionSlots)[i] = (uint8_t)dt_u32();
    }
    for (int i = 0; i < 5; i++) {
        memset(gCaptionSlots[i].text1, 'a' + i, 6);
        gCaptionSlots[i].text1[6] = '\0';
        memset(gCaptionSlots[i].text2, 'k' + i, 4);
        gCaptionSlots[i].text2[4] = '\0';
        gCaptionSlots[i].font1 = (uint16_t)dt_u32();
        gCaptionSlots[i].font2 = (uint16_t)dt_u32();
        gCaptionWidths[i] = (uint16_t)dt_range(0, 200);
        gCaptionColors[i].line1 = (uint16_t)dt_range(0, 1);
        gCaptionColors[i].line2 = (uint16_t)dt_range(0, 1);
    }
    gCaptionHold = (uint16_t)(dt_u32() % 3 == 0 ? dt_range(0, 3) : 0);
    gCaptionAlpha = (uint16_t)(dt_u32() % 3 == 0 ? dt_range(0, 3) : 0);
    gCaptionLine = (uint16_t)dt_range(0, 0x14);
    gCaptionOn = (uint16_t)dt_range(0, 3);
    gSelPreviewFn = dt_u32() % 2 ? (SelStepFn)(uintptr_t)0x00401b40 : NULL;
    if (dt_u32() % 2) {
        gSelStage = 1;
    }
    const uint32_t id = ids[dt_u32() % (sizeof ids / sizeof ids[0])];
    DT_ARG_INT(c, 0, dt_u32() % 12 == 0 ? dt_range(0, 0x20) : 0xc);
    DT_ARG_PTR(c, 1, guiFindById(id));
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

DT_CASES(frontend_select_cases) = {
    {"selRiderCommit", 0x00444ae0, selRiderCommit, 1, DT_RET_VOID, setup_step_arg, 200, 0},
    {"selRiderPreview", 0x00444bc0, selRiderPreview, 1, DT_RET_VOID, setup_step_arg, 200, 0},
    {"selRiderOpen", 0x00444d20, selRiderOpen, 0, DT_RET_VOID, setup_step, 300, 0},
    {"selRiderClose", 0x00444fc0, selRiderClose, 0, DT_RET_VOID, setup_step, 100, 0},
    {"selLevelCommit", 0x00445090, selLevelCommit, 1, DT_RET_VOID, setup_step_arg, 400, 0},
    {"selLevelRefresh", 0x00445290, selLevelRefresh, 1, DT_RET_VOID, setup_step_arg, 300, 0},
    {"selLevelPreview", 0x00445490, selLevelPreview, 1, DT_RET_VOID, setup_step_arg, 400, 0},
    {"selLevelOpen", 0x00445760, selLevelOpen, 0, DT_RET_VOID, setup_step, 300, 0},
    {"selLevelClose", 0x00445a10, selLevelClose, 0, DT_RET_VOID, setup_step, 100, 0},
    {"selStyleCommit", 0x00445b00, selStyleCommit, 1, DT_RET_VOID, setup_step_arg, 100, 0},
    {"selStylePreview", 0x00445b90, selStylePreview, 1, DT_RET_VOID, setup_step_arg, 400, 0},
    {"selStyleOpen", 0x00445d90, selStyleOpen, 0, DT_RET_VOID, setup_step, 400, 0},
    {"selStyleClose", 0x00446170, selStyleClose, 0, DT_RET_VOID, setup_step, 100, 0},
    {"selBikeIdle", 0x00446280, selBikeIdle, 0, DT_RET_VOID, setup_step, 300, 0},
    {"selBikeCommit", 0x00446390, selBikeCommit, 1, DT_RET_VOID, setup_step_arg, 300, 0},
    {"selBikePreview", 0x004464c0, selBikePreview, 1, DT_RET_VOID, setup_step_arg, 400, 0},
    {"selBikeOpen", 0x00446730, selBikeOpen, 0, DT_RET_VOID, setup_step, 300, 0},
    {"selBikeClose", 0x00446a20, selBikeClose, 0, DT_RET_VOID, setup_step, 100, 0},
    {"selCheatInput", 0x00446ac0, selCheatInput, 1, DT_RET_VOID, setup_cheat, 1500, 0},
    {"selScreenOpen", 0x00447540, selScreenOpen, 0, DT_RET_INT, setup_state, 100, 0},
    {"selScreenLeave", 0x00447600, selScreenLeave, 0, DT_RET_VOID, setup_step, 200, 0},
    {"selCommitSetup", 0x004476c0, selCommitSetup, 0, DT_RET_VOID, setup_step, 200, 0},
    {"selSpriteSlot", 0x004477e0, selSpriteSlot, 5, DT_RET_VOID, setup_slot, 200, 0},
    {"selResetCarousel", 0x004478b0, selResetCarousel, 0, DT_RET_VOID, setup_state, 100, 0},
    {"selChangeStage", 0x004479d0, selChangeStage, 1, DT_RET_VOID, setup_change, 600, 0},
    {"selScreenHook", 0x00447bf0, selScreenHook, 4, DT_RET_INT, setup_hook, 2500, 0},
    {"selStartCaption", 0x00448040, selStartCaption, 1, DT_RET_VOID, setup_caption, 50, 0},
    {"selShowHeader", 0x004487b0, selShowHeader, 0, DT_RET_VOID, setup_state, 100, 0},
    {"selHideHeader", 0x00448910, selHideHeader, 0, DT_RET_VOID, setup_state, 100, 0},
    {"selSlideStep", 0x004496a0, selSlideStep, 0, DT_RET_VOID, setup_step, 400, 0},
    {"selIntroStep", 0x004498d0, selIntroStep, 0, DT_RET_VOID, setup_step, 400, 0},
    {"selLeaveStep", 0x00449a30, selLeaveStep, 0, DT_RET_VOID, setup_step, 300, 0},
    {"selScreenLayout", 0x00449a80, selScreenLayout, 0, DT_RET_VOID, setup_step, 200, 0},
    {"selCaptionFeed", 0x00448090, selCaptionFeed, 2, DT_RET_VOID, setup_feed, 600, 0},
    {"selScreenDraw", 0x00448a80, selScreenDraw, 4, DT_RET_INT, setup_draw, 2500, 0},
};
