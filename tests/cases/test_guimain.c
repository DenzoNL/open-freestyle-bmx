/* Tests for guimain.c: lifecycle, element list, screen update. */
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_gui.h"

static void setup_nop(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
}

static void setup_init(DtCtx *c)
{
    setup_nop(c);
    gGuiInitialized = (int)(dt_u32() & 1);
}

static void setup_asset_file(DtCtx *c)
{
    setup_nop(c);
    gGuiAssetFile = (const char *)(uintptr_t)dt_u32();
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_hook(DtCtx *c)
{
    setup_nop(c);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_abs(DtCtx *c)
{
    setup_nop(c);
    switch (c->seed % 4) {
    case 0:
        DT_ARG_FLOAT(c, 0, dt_float_edgy(-100.0f, 100.0f));
        break;
    case 1:
        DT_ARG_FLOAT(c, 0, -dt_float(0.0f, 65536.0f));
        break;
    default:
        DT_ARG_INT(c, 0, dt_u32());
    }
}

static void setup_pulse(DtCtx *c)
{
    static const int bounds[] = {0x10, 0x18, 0x20, 0xe8, 0xf0, 0xf8, 0x8, 0};
    setup_nop(c);
    gGuiPulse = c->seed % 3 == 0 ? (int)dt_u32() : bounds[dt_u32() % 8] + dt_range(-8, 8);
    gGuiPulseStep = c->seed % 7 == 0 ? dt_range(-3, 3) : (dt_u32() & 1 ? 8 : -8);
    gGuiPulseHold = c->seed % 5 == 0 ? (int)dt_u32() : 0;
}

static GuiElement *pick_element(void)
{
    return gGuiElementCount > 0 ? gGuiSorted[dt_u32() % (uint32_t)gGuiElementCount] : NULL;
}

static void setup_find(DtCtx *c)
{
    setup_nop(c);
    dtGuiRandomScreen(dt_range(0, 12));
    GuiElement *el = pick_element();
    DT_ARG_INT(c, 0, el != NULL && c->seed % 4 != 3 ? el->id : dt_u32());
}

static void setup_get_index(DtCtx *c)
{
    setup_nop(c);
    dtGuiRandomScreen(dt_range(0, 12));
    DT_ARG_INT(c, 0, c->seed % 5 == 0 ? dt_u32() : (uint32_t)dt_range(0, 14));
}

static void setup_compare(DtCtx *c)
{
    setup_nop(c);
    GuiElement *a = dtGuiRandomElement(0);
    GuiElement *b = dtGuiRandomElement(1);
    if (c->seed % 4 == 0) {
        b->order = a->order;
    }
    GuiElement **pa = (GuiElement **)dt_alloc(4);
    GuiElement **pb = (GuiElement **)dt_alloc(4);
    *pa = a;
    *pb = b;
    DT_ARG_PTR(c, 0, pa);
    DT_ARG_PTR(c, 1, pb);
}

static void setup_sort(DtCtx *c)
{
    setup_nop(c);
    dtGuiRandomScreen(dt_range(0, 16));
    for (int i = gGuiElementCount - 1; i > 0; i--) {
        const int j = (int)(dt_u32() % (uint32_t)(i + 1));
        GuiElement *t = gGuiSorted[i];
        gGuiSorted[i] = gGuiSorted[j];
        gGuiSorted[j] = t;
    }
    if (c->seed % 4 == 0 && gGuiElementCount > 1) { /* ties */
        gGuiSorted[1]->order = gGuiSorted[0]->order;
    }
}

static void setup_create(DtCtx *c)
{
    static const int16_t types[] = {1, 2, 3, 4, 5, 6, 7, 8, 0, 9, -1};
    setup_nop(c);
    const int existing = c->seed % 11 == 10 ? kGuiMaxElements : dt_range(0, 12);
    dtGuiRandomScreen(existing);
    DT_ARG_INT(c, 0, (uint32_t)(int32_t)types[dt_u32() % 11] | (dt_u32() & 0xffff0000u));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

static void give_handles(GuiElement *el)
{
    switch (dt_u32() % 3) {
    case 0:
        break;
    case 1:
        el->textHandle = dtGuiTextHandle(dtGuiRandomText(10, 0));
        el->textCapacity = 11;
        break;
    default:
        el->textHandle = dtGuiTextHandle("x");
        el->textCapacity = 2;
    }
    if (el->type == kGuiTypeToggle) {
        if (dt_u32() % 4 != 0) {
            dtGuiMakeToggleData(el, (int)(dt_u32() & 1), (int)(dt_u32() & 1));
        }
    } else if (dt_u32() % 4 == 0) {
        el->dataHandle = zmemdmmAlloc(16); /* freed as is */
    }
}

static void setup_destroy(DtCtx *c)
{
    setup_nop(c);
    dtGuiRandomScreen(dt_range(1, 10));
    GuiElement *el = pick_element();
    give_handles(el);
    if (c->seed % 6 == 5) { /* not in the list: reports an error */
        el = dtGuiRandomElement(20);
        give_handles(el);
    }
    if (c->seed % 13 == 12) {
        el = NULL;
    }
    DT_ARG_PTR(c, 0, el);
}

static void setup_destroy_all(DtCtx *c)
{
    setup_nop(c);
    dtGuiRandomScreen(dt_range(0, 10));
    for (int i = 0; i < gGuiElementCount; i++) {
        give_handles(gGuiSorted[i]);
    }
    gGuiScreen.hook = c->seed % 3 == 0 ? NULL : dtGuiHook;
    gGuiScreen.backdrop = (ZPrimAsset *)(uintptr_t)dt_u32();
    gGuiScreen.focusFont = (uint16_t)dt_u32();
    gGuiAssetsLoaded = (int)(dt_u32() & 1);
    if (c->seed % 7 == 6) { /* a list entry that is not a table slot survives */
        gGuiSorted[gGuiElementCount++] = (GuiElement *)dt_alloc(sizeof(GuiElement));
    }
}

static void setup_request(DtCtx *c)
{
    setup_nop(c);
    gGuiScreenLoaded = 1;
    gGuiReloadDelay = (int)dt_u32();
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
    if (c->seed % 4 == 3) { /* nothing on display: the screen (id 0, so no file access) loads at once */
        gGuiScreenLoaded = 0;
        dtGuiRandomScreen(dt_range(0, 4));
        gGuiScreen.hook = dtGuiHook;
        DT_ARG_INT(c, 0, dt_u32() & 0xffff0000u);
        DT_ARG_PTR(c, 1, c->seed % 8 == 3 ? NULL : (void *)dtGuiHook); /* the hook gets called */
    }
}

/* A screen part-way through its transition with the pad in a random state. */
static void setup_update(DtCtx *c)
{
    setup_nop(c);
    dtGuiRandomScreen(dt_range(0, 10));
    dtGuiRandomPad();
    gGuiModeReady = c->seed % 16 != 15;
    gGuiSkipUpdate = (int)(dt_u32() % 5 == 0);
    gGuiScreenLoaded = (int)(dt_u32() % 5 != 0);
    gGuiReloadPending = (int)(dt_u32() % 5 == 0);
    gGuiReloadDelay = dt_range(0, 3); /* stays under the delay that triggers a reload */
    gGuiSlideState = dt_range(0, 2);
    gGuiAssetsLoaded = (int)(dt_u32() & 1);
    gGuiIdleMode = (int)(dt_u32() % 6 == 0);
    gGuiPulse = dt_range(0x10, 0xf0);
    gGuiPulseStep = dt_u32() & 1 ? 8 : -8;
    gGuiPulseHold = 0;
    gGuiScreen.hook = c->seed % 3 == 0 ? NULL : dtGuiHook;
}

typedef uint32_t ZAssetGroups[4];
#define gZassetGroups ORIG_VAR(0x00678418, ZAssetGroups, gZassetGroups)

/* Mode init stops early when the app is not initialised or the mode is already up. */
static void setup_mode_init(DtCtx *c)
{
    setup_nop(c);
    gGuiInitialized = c->seed % 3 != 0;
    gGuiModeReady = c->seed % 3 == 1;
    gGuiScreen.backdrop = (ZPrimAsset *)(uintptr_t)dt_u32();
    for (int i = 0; i < 4; i++) {
        gGuiSorted[i] = (GuiElement *)(uintptr_t)dt_u32();
    }
    gGuiElements[3].type = 5;
}

static void setup_mode_shutdown(DtCtx *c)
{
    setup_nop(c);
    dtGuiRandomScreen(dt_range(0, 6));
    for (int i = 0; i < gGuiElementCount; i++) {
        give_handles(gGuiSorted[i]);
    }
    gGuiModeReady = c->seed % 4 != 3;
    gGuiScreen.backdrop = (ZPrimAsset *)dtGuiMakeSprite(0x123, 2);
    gGuiAssetsLoaded = (int)(dt_u32() & 1);
    gGuiScreen.hook = c->seed % 3 == 0 ? NULL : dtGuiHook;
}

static void setup_load_screen(DtCtx *c)
{
    setup_nop(c);
    dtGuiRandomScreen(dt_range(0, 8));
    for (int i = 0; i < gGuiElementCount; i++) {
        give_handles(gGuiSorted[i]);
    }
    gGuiDefaultHook = c->seed % 3 == 0 ? NULL : dtGuiHook;
    gGuiScreen.hook = dtGuiProc;
    gGuiScreen.backdrop = (ZPrimAsset *)(uintptr_t)dt_u32();
    gGuiAssetsLoaded = c->seed % 2;
    if (c->seed % 5 == 4) { /* a screen is requested but its asset group is "already loaded" */
        gGuiScreenId = (uint16_t)dt_range(1, 0xfff);
        gZassetGroups[1] = 1;
        gGuiAssetsLoaded = 0;
    } else {
        gGuiScreenId = 0;
    }
}

DT_CASES(guimain_cases) = {
    {"guiModeInit", 0x00437640, guiModeInit, 0, DT_RET_INT, setup_mode_init, 200, 0},
    {"guiModeShutdown", 0x004377d0, guiModeShutdown, 0, DT_RET_VOID, setup_mode_shutdown, 200, 0},
    {"guiLoadScreen", 0x00437980, guiLoadScreen, 0, DT_RET_VOID, setup_load_screen, 200, 0},
    {"guiUpdate", 0x00437c40, guiUpdate, 0, DT_RET_INT, setup_update, 400, 0},
    {"guiInit", 0x004375d0, guiInit, 0, DT_RET_INT, setup_init, 200, 0},
    {"guiExit", 0x00437610, guiExit, 0, DT_RET_VOID, setup_init, 200, 0},
    {"guiSetAssetFile", 0x00437850, guiSetAssetFile, 1, DT_RET_INT, setup_asset_file, 200, 0},
    {"guiSetHook", 0x004375a0, guiSetHook, 1, DT_RET_VOID, setup_hook, 200, 0},
    {"guiAbsF", 0x00437c20, guiAbsF, 1, DT_RET_DOUBLE, setup_abs, 200, 0},
    {"guiPulseUpdate", 0x004382f0, guiPulseUpdate, 0, DT_RET_VOID, setup_pulse, 300, 0},
    {"guiGetScreenState", 0x00438380, guiGetScreenState, 0, DT_RET_INT, setup_nop, 5, 0},
    {"guiFindById", 0x004383a0, guiFindById, 1, DT_RET_INT, setup_find, 200, 0},
    {"guiGetByIndex", 0x004383e0, guiGetByIndex, 1, DT_RET_INT, setup_get_index, 200, 0},
    {"guiCompareOrder", 0x00438410, guiCompareOrder, 2, DT_RET_INT, setup_compare, 200, 0},
    {"guiSortElements", 0x00438440, guiSortElements, 0, DT_RET_VOID, setup_sort, 200, 0},
    {"guiCreateElement", 0x00438470, guiCreateElement, 3, DT_RET_INT, setup_create, 300, 0},
    {"guiDestroyElement", 0x00438600, guiDestroyElement, 1, DT_RET_VOID, setup_destroy, 300, 0},
    {"guiDestroyAll", 0x00437870, guiDestroyAll, 0, DT_RET_VOID, setup_destroy_all, 200, 0},
    {"guiRequestScreen", 0x00437930, guiRequestScreen, 2, DT_RET_VOID, setup_request, 200, 0},
};
