/* Tests for frontend_options.c: the options panels (game options, controllers, keyboard, sound). */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/zprim.h"
#include "dt.h"
#include "dt_frontend.h"
#include "zge/zstring.h"

static GuiPadDevice fakePad;

/* All elements of the options screen and a random state for every global the panels use. */
static void optionsState(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    dtFrontendStub(DT_FS_SOUND | DT_FS_ZPRIM | DT_FS_PAD | DT_FS_LABEL | DT_FS_SCREENS | DT_FS_CARD |
                   DT_FS_FRAME);
    dtFrontRandomCardResults();
    dtFrontendElementRange(0xa00c0001, 0x4c);
    gFrontendStep = dt_u32() % 3 == 0 ? NULL : dtFrontStep;
    gGuiIdleMode = dt_range(0, 1);
    gGuiPulse = dt_range(0x10, 0xf0);

    /* game options */
    gOptionsAvail = (uint16_t)(dt_u32() % 4 == 0 ? dt_u32() : 0xffff);
    gOptionsValue = (uint16_t)dt_u32();
    gOptCount = (uint16_t)dt_range(0, 8);
    for (int i = 0; i < 8; i++) {
        gOptRows[i] = &gOptionTable[dt_u32() % 8];
        gOptSlide[i] = dt_range(0, 0x1e0);
    }
    if (dt_u32() % 3 == 0) { /* rows sliding in: the first rows already far out */
        for (int i = 0; i < 8; i++) {
            gOptSlide[i] = i < 3 ? dt_range(0x1c8, 0x1ce) : dt_range(0, 0x120);
        }
    }
    if (dt_u32() % 4 == 0) { /* rows sliding out */
        for (int i = 0; i < 8; i++) {
            gOptSlide[i] = i < 3 ? dt_range(0, 3) : dt_range(0, 420);
        }
    }
    gOptSel = dt_range(0, 7);
    gOptTicks = (int)dt_u32();
    gMcChoice = 0;

    /* keyboard panel */
    gKeyPlayer = dt_range(0, 3);
    for (int p = 0; p < 4; p++) {
        for (int i = 0; i < 11; i++) {
            gPlayers[p].bind[i] =
                dt_u32() % 5 == 0 ? 0 : (dt_u32() % 7 == 0 ? dt_u32() : 1u << (dt_u32() % 18));
        }
        gPlayers[p].layout = (uint8_t)(dt_u32() % 9 == 0 ? dt_u32() % 5 : dt_range(0, 4));
    }
    for (int i = 0; i < 11; i++) {
        gKbWork[i] = dt_u32();
    }
    gKbCursor = (uint8_t)dt_range(0, 13);
    gKbState = (uint8_t)(dt_u32() % 4 == 0 ? dt_range(0, 3) : dt_range(0, 2));
    gKbLayout = (uint8_t)dt_range(0, 4);
    fakePad.held = dt_u32() % 3 == 0 ? 0 : (dt_u32() % 2 ? 1u << (dt_u32() % 18) : dt_u32());
    dt_front_pad = &fakePad;

    /* sound panel */
    gSoundSlideA = dt_range(0, 0x1ce);
    gSoundSlideB = dt_range(0, 0x1ce);
    if (dt_u32() % 2 == 0) {
        gSoundSlideA = dt_range(0x100, 0x1ce);
        gSoundSlideB = dt_range(0x1c0, 0x1ce);
    }
    gSoundSel = dt_range(0, 1);
    for (uint32_t id = 0xa00c0049; id <= 0xa00c004c;
         id += 3) { /* the labels measured by the tick: text and a font */
        GuiElement *label = guiFindById(id);
        guiSetText(label, dtGuiRandomText(8, 0));
        label->font = dtGuiMakeFont((uint16_t)(0x190 + (id & 7)), (uint16_t)dt_range(6, 18));
    }
    gSoundLevel0 = dt_range(0, 0x12);
    gSoundLevel1 = dt_range(0, 0x12);
    gSoundVolume0 = (uint8_t)dt_range(0, 255);
    gSoundVolume1 = (uint8_t)dt_range(0, 255);

    /* strips */
    for (int i = 0; i < 8; i++) {
        for (int k = 0; k < 3; k++) {
            gStripColors[i][k] = (uint8_t)dt_u32();
        }
    }
    gStripScale0 = dt_float_edgy(0.0f, 64.0f);
    /* gStripScale1 is in .rdata (not restored between cases): left as the exe has it */
    uint8_t *prims = dt_alloc(17 * sizeof(StripPoly));
    for (uint32_t i = 0; i < 17 * sizeof(StripPoly); i++) {
        prims[i] = (uint8_t)dt_u32();
    }
    gPrimCursor = prims;
    uint32_t *table = dt_alloc(8 * 4);
    for (int i = 0; i < 8; i++) {
        table[i] = dt_u32();
    }
    gNgifOrderTable = table + dt_range(1, 6);
    gFrontendStep = dt_u32() % 3 == 0 ? NULL : dtFrontStep;

    /* options main list and the frame box it draws */
    gOptMainSel = dt_u32() % 8 == 0 ? dt_range(-1, 4) : dt_range(0, 2);
    gOptMainReady = (int)(dt_u32() % 2);
    gOptMainPulse = dt_range(-3, 0x48);
    gOptMainPulseStep = dt_range(-4, 4);
    gOptSaveDelay = dt_range(0, 3);
    gMenuSlide0 = dt_range(-0x48, 0x30);
    gMenuSlide1 = dt_range(-0x48, 0x30);
    gMenuSlide2 = dt_range(-0x48, 0x30);
    gMenuSlide3 = dt_range(-0x48, 0x30);
    if (dt_u32() % 3 == 0) {
        gOptionsAvail = 0;
    }
    uint8_t frameData[0x50];
    for (uint32_t i = 0; i < sizeof frameData; i++) {
        frameData[i] = (uint8_t)dt_u32();
    }
    gMenuFrameSprite = dtGuiMakeAsset(0x40c5, frameData, sizeof frameData);
}

static void setup_step(DtCtx *c)
{
    optionsState(c);
}

static void hookMessages(DtCtx *c, const int *msgs, int count)
{
    optionsState(c);
    dtFrontendHookArgs(c, msgs, count);
}

static void setup_options_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgDraw,
                               kGuiMsgDraw,
                               kGuiMsgTickAll,
                               kGuiMsgNavUp,
                               kGuiMsgNavDown,
                               kGuiMsgNavUpAlt,
                               kGuiMsgNavDownAlt,
                               kGuiMsgNavLeft,
                               kGuiMsgNavRight,
                               kGuiMsgNavLeftAlt,
                               kGuiMsgNavRightAlt,
                               kGuiMsgButton22,
                               kGuiMsgButton24,
                               0x2f,
                               0x31,
                               kGuiMsgButton25,
                               kGuiMsgIdle,
                               kGuiMsgDrawAll};
    hookMessages(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
}

static void setup_main_hook(DtCtx *c)
{
    static const int msgs[] = {
        kGuiMsgTickAll,    kGuiMsgTickAll,  kGuiMsgNavUp,    kGuiMsgNavDown, kGuiMsgNavUpAlt,
        kGuiMsgNavDownAlt, kGuiMsgButton22, kGuiMsgButton24, 0x2f,           0x31,
        kGuiMsgButton25,   kGuiMsgIdle,     kGuiMsgNavLeft,  kGuiMsgDrawAll};
    hookMessages(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
}

static void setup_controls_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTickAll,
                               kGuiMsgTickAll,
                               kGuiMsgNavUp,
                               kGuiMsgNavDown,
                               kGuiMsgNavLeft,
                               kGuiMsgNavRight,
                               kGuiMsgButton22,
                               kGuiMsgButton24,
                               kGuiMsgNavUpAlt,
                               kGuiMsgNavDownAlt,
                               kGuiMsgNavLeftAlt,
                               kGuiMsgNavRightAlt,
                               0x2f,
                               0x31,
                               kGuiMsgButton25,
                               kGuiMsgIdle,
                               kGuiMsgDrawAll};
    hookMessages(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
}

static void setup_keyboard_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTickAll,
                               kGuiMsgTickAll,
                               kGuiMsgNavUp,
                               kGuiMsgNavDown,
                               kGuiMsgNavUpAlt,
                               kGuiMsgNavDownAlt,
                               kGuiMsgNavLeft,
                               kGuiMsgNavRight,
                               kGuiMsgNavLeftAlt,
                               kGuiMsgNavRightAlt,
                               kGuiMsgButton22,
                               kGuiMsgButton24,
                               0x2f,
                               0x31,
                               kGuiMsgButton25,
                               kGuiMsgIdle,
                               kGuiMsgDrawAll};
    hookMessages(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
    if (dt_u32() % 3 != 0) {
        gKbCursor = (uint8_t)(dt_u32() % 2 ? dt_range(10, 13) : gKbCursor);
    }
    if (gKbState != 0) { /* rebinding starts on an action row */
        gKbCursor = (uint8_t)dt_range(0, 10);
    }
}

static void setup_sound_hook(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgDraw,
                               kGuiMsgDraw,
                               kGuiMsgTickAll,
                               kGuiMsgNavUp,
                               kGuiMsgNavDown,
                               kGuiMsgNavUpAlt,
                               kGuiMsgNavDownAlt,
                               kGuiMsgNavLeft,
                               kGuiMsgNavLeft,
                               kGuiMsgNavRight,
                               kGuiMsgNavRight,
                               kGuiMsgNavLeftAlt,
                               kGuiMsgNavRightAlt,
                               kGuiMsgButton22,
                               kGuiMsgButton24,
                               0x2f,
                               0x31,
                               kGuiMsgButton25,
                               kGuiMsgIdle,
                               kGuiMsgDrawAll};
    hookMessages(c, msgs, (int)(sizeof msgs / sizeof msgs[0]));
    if (dt_u32() % 2 == 0) { /* draw: the bar elements */
        const uint32_t id = dt_u32() % 3 == 0 ? 0xa00c0048 : (dt_u32() % 2 ? 0xa00c004b : 0xa00c0049);
        DT_ARG_PTR(c, 1, guiFindById(id));
    }
    if (dt_u32() % 8 == 0) { /* levels at their limits */
        gSoundLevel0 = dt_u32() % 2 ? 0 : 0x12;
        gSoundLevel1 = dt_u32() % 2 ? 0 : 0x12;
    }
}

DT_CASES(frontend_options_cases) = {
    {"optionsGameOpen", 0x00440dc0, optionsGameOpen, 0, DT_RET_VOID, setup_step, 200, 0},
    {"optionsGameClose", 0x00440eb0, optionsGameClose, 0, DT_RET_VOID, setup_step, 100, 0},
    {"optionsGameSlideIn", 0x00440fd0, optionsGameSlideIn, 0, DT_RET_VOID, setup_step, 400, 0},
    {"optionsGameSlideOut", 0x004410b0, optionsGameSlideOut, 0, DT_RET_VOID, setup_step, 400, 0},
    {"optionsGameHook", 0x00441180, optionsGameHook, 4, DT_RET_INT, setup_options_hook, 1500, 0},
    {"controlsDrawStrips", 0x004414d0, controlsDrawStrips, 0, DT_RET_VOID, setup_step, 400, 0},
    {"controlsOpen", 0x00441860, controlsOpen, 0, DT_RET_VOID, setup_step, 100, 0},
    {"controlsClose", 0x004419f0, controlsClose, 0, DT_RET_VOID, setup_step, 100, 0},
    {"controlsHook", 0x00441bd0, controlsHook, 4, DT_RET_INT, setup_controls_hook, 800, 0},
    {"keyboardOpen", 0x00441cb0, keyboardOpen, 0, DT_RET_VOID, setup_step, 200, 0},
    {"keyboardClose", 0x00441f80, keyboardClose, 0, DT_RET_VOID, setup_step, 100, 0},
    {"keyboardHook", 0x004420a0, keyboardHook, 4, DT_RET_INT, setup_keyboard_hook, 2500, 0},
    {"soundPanelOpen", 0x00442730, soundPanelOpen, 0, DT_RET_VOID, setup_step, 100, 0},
    {"soundPanelClose", 0x00442a00, soundPanelClose, 0, DT_RET_VOID, setup_step, 100, 0},
    {"soundPanelSlideIn", 0x00442b70, soundPanelSlideIn, 0, DT_RET_VOID, setup_step, 300, 0},
    {"soundPanelSlideOut", 0x00442c40, soundPanelSlideOut, 0, DT_RET_VOID, setup_step, 300, 0},
    {"soundPanelHook", 0x00442ce0, soundPanelHook, 4, DT_RET_INT, setup_sound_hook, 2000, 0},
    {"optionsInit", 0x004433e0, optionsInit, 0, DT_RET_INT, setup_step, 50, 0},
    {"optionsMainHook", 0x00443420, optionsMainHook, 4, DT_RET_INT, setup_main_hook, 1500, 0},
    {"optionsMainHighlight", 0x004436c0, optionsMainHighlight, 0, DT_RET_VOID, setup_step, 100, 0},
    {"optionsMainSave", 0x00443830, optionsMainSave, 0, DT_RET_VOID, setup_step, 100, 0},
    {"optionsMainSlideOut", 0x00443870, optionsMainSlideOut, 0, DT_RET_VOID, setup_step, 400, 0},
    {"optionsMainStep", 0x00443af0, optionsMainStep, 0, DT_RET_VOID, setup_step, 400, 0},
    {"optionsMainLayout", 0x00443cb0, optionsMainLayout, 0, DT_RET_VOID, setup_step, 100, 0},
};
