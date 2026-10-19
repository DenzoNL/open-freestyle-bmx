/* GUI core (C:\zaxis\Bmx\src\com\menu\gui\guimain.c): element list, messaging, screen update. */
#include "bmx/com/menu/gui/guimain.h"

#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/menu/gui/guilabel.h"
#include "bmx/com/menu/gui/guisfile.h"
#include "bmx/com/menu/gui/guitoggl.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"

/* Leftover of the console port (0x45a1a0): an empty function called every update. */
ORIG_FN(0x0045a1a0, void, guiPlatformUpdate, (void));

static void guiDebug(const char *message)
{
    (*gGuiDebugPrint)(message);
}

/* ---- lifecycle -------------------------------------------------------------------------------- */

BMX_ADDR(0x004375d0)
int guiInit(void)
{
    if (gGuiInitialized != 0) {
        guiDebug("GUIMAIN: Error - reinitializing app\n");
        return 0;
    }
    gGuiInitialized = 1;
    return 0;
}

BMX_ADDR(0x00437610)
void guiExit(void)
{
    if (gGuiInitialized != 0) {
        gGuiInitialized = 0;
    }
}

/* Clears the element table, screen state and hook, then brings up the memory manager and sprite layer. */
BMX_ADDR(0x00437640)
int guiModeInit(void)
{
    memset(&gGuiElements, 0, sizeof gGuiElements);
    memset(&gGuiSorted, 0, sizeof gGuiSorted);
    memset(&gGuiScreen, 0, sizeof gGuiScreen);
    gGuiAssetsLoaded = 0;
    gGuiScreen.hook = gGuiDefaultHook;
    if (gGuiInitialized == 0) {
        guiDebug("GUIMAIN: Error - initializing menu mode before app\n");
        return 0;
    }
    if (gGuiModeReady != 0) {
        guiDebug("GUIMAIN: Error - reinitializing menu mode\n");
        return 0;
    }
    if (zmemdmmInit(0, 0x300000, 0x40) != 0) {
        guiDebug("GUIMAIN: Failed to initialize ZMEMDMM\n");
        zprimShutdown();
        return 1;
    }
    if (zprimInit() != 0) {
        guiDebug("GUIMAIN: Failed to initialize ZPRIM\n");
        return 1;
    }
    zprimOpenLibrary(gGuiAssetFile);
    if (zprimLoadGroup(0, 0xe000) != 0) {
        guiDebug("GUIMAIN: Failed to load permanent screen assets\n");
        return 1;
    }
    if (guiHelpInit() != 0) {
        guiDebug("GUIMAIN: Failed to init help system\n");
        return 1;
    }
    gGuiScreenLoaded = 0;
    gGuiReloadPending = 0;
    gGuiSkipUpdate = 0;
    gGuiPulseHold = 0;
    gGuiModeReady = 1;
    return 0;
}

BMX_ADDR(0x004377d0)
void guiModeShutdown(void)
{
    if (gGuiModeReady != 0) {
        for (int i = 0; i < kGuiMaxElements; i++) {
            if (gGuiElements[i].type != 0) {
                guiDestroyElement(&gGuiElements[i]);
            }
        }
        zprimAssetFree(gGuiScreen.backdrop);
        guiDestroyAll();
        zprimFreeGroup(0);
        zprimShutdown();
        zmemdmmShutdown();
        gGuiModeReady = 0;
    }
}

/* Replaces the asset file name; returns the previous one. */
BMX_ADDR(0x00437850)
const char *guiSetAssetFile(const char *name)
{
    const char *previous = gGuiAssetFile;
    gGuiAssetFile = name;
    return previous;
}

/* Destroys every element and the screen state (the backdrop survives) and unloads the screen assets. */
BMX_ADDR(0x00437870)
void guiDestroyAll(void)
{
    guiBroadcast(kGuiMsgScreenClosing, NULL, 0, 0);
    for (int i = 0; i < kGuiMaxElements; i++) {
        if (gGuiElements[i].type != 0) {
            guiDestroyElement(&gGuiElements[i]);
        }
    }
    if (gGuiElementCount != 0) {
        guiDebug("GUIMAIN: Failed to destroy all elements\n");
        return;
    }
    ZPrimAsset *const backdrop = gGuiScreen.backdrop;
    memset(&gGuiScreen, 0, sizeof gGuiScreen);
    gGuiScreen.backdrop = backdrop;
    if (gGuiAssetsLoaded != 0) {
        zprimFreeGroup(1);
        gGuiAssetsLoaded = 0;
    }
    zmemdmmOptimize();
}

/* Remembers the screen to show and loads it now, or after a short delay if one is on display. */
BMX_ADDR(0x00437930)
void guiRequestScreen(uint16_t screenId, GuiHookFn hook)
{
    gGuiScreenId = screenId;
    gGuiDefaultHook = hook;
    if (gGuiScreenLoaded == 0) {
        guiLoadScreen();
        return;
    }
    gGuiReloadPending = 1;
    gGuiReloadDelay = 0;
}

BMX_ADDR(0x00437c20)
double guiAbsF(float x)
{
    return __builtin_fabs((double)x);
}

/*
 * Tears down the current screen, loads gGuiScreenId and prepares the slide-in: every element starts at
 * its slide offset and waits (largest offset minus its own) so all of them arrive together.
 */
BMX_ADDR(0x00437980)
void guiLoadScreen(void)
{
    guiScreenLoadHook();
    guiDestroyAll();
    zprimFlush();
    gGuiScreen.hook = gGuiDefaultHook;
    if (gGuiScreenId != 0) {
        if (zprimLoadGroup(1, (uint32_t)(gGuiScreenId & 0xfff) | 0xe000) != 0) {
            guiDebug("GUISFILE: Failed to load screen assets\n");
            return;
        }
        gGuiAssetsLoaded = 1;
        if (guiBuildScreen(gGuiScreenId) != 0) {
            guiDebug("GUIMAIN: Error - failed to load screen\n");
            return;
        }
    }
    gGuiSlideState = 1;
    gGuiSlideMax = 0;
    for (int i = 0; i < kGuiMaxElements; i++) {
        GuiElement *el = &gGuiElements[i];
        if (el->type != 0) {
            el->slideX = el->slideStartX;
            el->slideY = el->slideStartY;
            if ((double)gGuiSlideMax < guiAbsF((float)el->slideStartX)) {
                gGuiSlideMax = (int)guiAbsF((float)el->slideStartX);
            }
            if ((double)gGuiSlideMax < guiAbsF((float)el->slideStartY)) {
                gGuiSlideMax = (int)guiAbsF((float)el->slideStartY);
            }
        }
    }
    for (int i = 0; i < kGuiMaxElements; i++) {
        GuiElement *el = &gGuiElements[i];
        if (el->type != 0) {
            if (el->slideStartX == 0) {
                el->slideDelayX = 0;
            } else {
                el->slideDelayX =
                    (uint16_t)(int)((double)(float)gGuiSlideMax - guiAbsF((float)el->slideStartX));
            }
            if (el->slideStartY == 0) {
                el->slideDelayY = 0;
            } else {
                el->slideDelayY =
                    (uint16_t)(int)((double)(float)gGuiSlideMax - guiAbsF((float)el->slideStartY));
            }
        }
    }
    guiBroadcast(kGuiMsgScreenLoaded, NULL, 0, 0);
    gGuiSkipUpdate = 1;
    gGuiScreenLoaded = 1;
}

/* Moves a slide-out offset towards zero by `step`, never overshooting. */
static void guiSlideOut(int16_t *offset, int step)
{
    int16_t v = *offset;
    if (v > 0) {
        v = (int16_t)(v - step);
        *offset = v;
        if (v < 0) {
            *offset = 0;
        }
    } else if (v < 0) {
        v = (int16_t)(v + step);
        *offset = v;
        if (v > 0) {
            *offset = 0;
        }
    }
}

/* Moves a slide-in offset towards its target by `step` once the delay has run down. */
static void guiSlideIn(int16_t *offset, int16_t target, uint16_t *delay, int step)
{
    if (*delay == 0) {
        int16_t v = *offset;
        if (v > target) {
            v = (int16_t)(v - step);
            *offset = v;
            if (v < target) {
                *offset = target;
            }
        } else if (v < target) {
            v = (int16_t)(v + step);
            *offset = v;
            if (v > target) {
                *offset = target;
            }
        }
    } else if (*delay > step) {
        *delay = (uint16_t)(*delay - step);
    } else {
        *delay = 0;
    }
}

/* Pad input with the console port's single pad: turns button edges into GUI messages. */
static void guiPollSinglePad(void)
{
    if (guiPadPressedMask4000(0)) {
        guiPostToVisible(kGuiMsgButton24, 0, 0);
    }
    if (guiPadPressedMask20080(0)) {
        guiPostToVisible(kGuiMsgButton22, 0, 0);
    }
    if (guiPadPressedMask40(0)) {
        guiPostToVisible(kGuiMsgButton23, 0, 0);
    }
    if (guiPadPressedMask10010(0)) {
        guiPostToVisible(kGuiMsgButton25, 0, 0);
    }
    if (guiPadRepeatBit1(0)) {
        guiPostToVisible(kGuiMsgNavUp, 0, 0);
    }
    if (guiPadRepeatBit2(0)) {
        guiPostToVisible(kGuiMsgNavDown, 0, 0);
    }
    if (guiPadRepeatBit4(0)) {
        guiPostToVisible(kGuiMsgNavLeft, 0, 0);
    }
    if (guiPadRepeatBit8(0)) {
        guiPostToVisible(kGuiMsgNavRight, 0, 0);
    }
    if (guiPadPressedBit4(0)) {
        guiPostToVisible(kGuiMsgButton3e, 0, 0);
    }
    if (guiPadPressedBit8(0)) {
        guiPostToVisible(kGuiMsgButton3f, 0, 0);
    }
    if (guiPadPressedMask100(0)) {
        guiPostToVisible(kGuiMsgButton27, 0, 0);
    }
    if (guiPadPressedMask200(0)) {
        guiPostToVisible(kGuiMsgButton28, 0, 0);
    }
    if (guiPadPressedMask400(0)) {
        guiPostToVisible(kGuiMsgButton29, 0, 0);
    }
    if (guiPadPressedMask800(0)) {
        guiPostToVisible(kGuiMsgButton2a, 0, 0);
    }
}

/* Same for the multi pad mode (never entered: guiPadMultiMode always returns 0). */
static void guiPollMultiPad(void)
{
    if (guiPadPressedMask4000(0) || guiPadPressedMask4000(1)) {
        guiPadMultiPress(kGuiMsgButton24);
    }
    if (guiPadPressedMask20080(0) || guiPadPressedMask20080(1)) {
        guiPadMultiPress(kGuiMsgButton22);
    }
    if (guiPadRepeatBit4(0) || guiPadRepeatBit4(1)) {
        guiPadMultiPress(kGuiMsgNavLeft);
    }
    if (guiPadRepeatBit8(0) || guiPadRepeatBit8(1)) {
        guiPadMultiPress(kGuiMsgNavRight);
    }
    if (guiPadRepeatBit1(0) || guiPadRepeatBit1(1)) {
        guiPadMultiPress(kGuiMsgNavUp);
    }
    if (guiPadRepeatBit2(0) || guiPadRepeatBit2(1)) {
        guiPadMultiPress(kGuiMsgNavDown);
    }
    if (guiPadPressedMask20(0) || guiPadPressedMask20(1)) {
        guiPadMultiPress(kGuiMsgButton25);
    }
    if (guiPadPressedMask10010(0) || guiPadPressedMask10010(1)) {
        guiPadMultiPress(kGuiMsgButton26);
    }
}

/* Per-frame update: input, screen transition animation, then the tick and draw broadcasts. */
BMX_ADDR(0x00437c40)
int guiUpdate(void)
{
    guiPulseUpdate();
    if (gGuiModeReady == 0) {
        guiDebug("GUIMAIN: Error - calling update before initialization\n");
        return 1;
    }
    if (gGuiSkipUpdate != 0) {
        gGuiSkipUpdate = 0;
    } else if (gGuiReloadPending != 0 && gGuiSlideState != 2) {
        gGuiReloadDelay++;
        if (gGuiReloadDelay <= 5) {
            return 0;
        }
        gGuiReloadPending = 0;
        gGuiScreenLoaded = 0;
        guiLoadScreen();
    }
    if (gGuiScreen.backdrop != NULL) {
        zprimSet(1, (int)(intptr_t)gGuiScreen.backdrop);
    } else {
        zprimSet(4, 0);
    }
    zprimClear();
    if (gGuiScreenLoaded != 0 && gGuiSkipUpdate == 0 && gGuiReloadPending == 0) {
        if (gGuiIdleMode != 0) {
            guiPostToVisible(kGuiMsgIdle, 0, 0);
        } else if (guiPadMultiMode() == 0) {
            guiPollSinglePad();
        } else {
            guiPollMultiPad();
        }
    }

    int moving = 0;
    for (int i = 0; i < gGuiElementCount; i++) {
        GuiElement *el = gGuiSorted[i];
        if (el->type == 0) {
            continue;
        }
        if (gGuiSlideState == 1) {
            guiSlideOut(&el->slideX, 4);
            guiSlideOut(&el->slideY, 3);
            if (el->slideX != 0 || el->slideY != 0) {
                moving++;
            }
        } else if (gGuiSlideState == 2) {
            guiSlideIn(&el->slideX, el->slideStartX, &el->slideDelayX, 4);
            guiSlideIn(&el->slideY, el->slideStartY, &el->slideDelayY, 3);
            if (el->slideX != el->slideStartX || el->slideY != el->slideStartY || el->slideDelayX != 0 ||
                el->slideDelayY != 0) {
                moving++;
            }
        }
    }
    if (moving == 0) {
        if (gGuiSlideState == 1) {
            gGuiSlideState = 0;
            guiBroadcast(kGuiMsgSlideDone, NULL, 0, 0);
        }
        if (gGuiSlideState == 2) {
            gGuiSlideState = 0;
        }
    }
    guiBroadcast(kGuiMsgTickAll, NULL, 0, 0);
    guiBroadcast(kGuiMsgDrawAll, NULL, 0, 0);
    if (guiPadMultiMode() != 0) {
        guiPadMultiEnd();
    }
    zprimEndFrame();
    if (gGuiAssetsLoaded != 0) {
        guiPlatformUpdate();
    }
    return 0;
}

/* Bounces the focus highlight brightness between 0x10 and 0xf0 (the hold flag stays 0). */
BMX_ADDR(0x004382f0)
void guiPulseUpdate(void)
{
    if (gGuiPulseHold == 0) {
        int step = gGuiPulseStep;
        if (step > 0) {
            gGuiPulse += 8;
            if (gGuiPulse > 0xf0) {
                step = -step;
                gGuiPulse = 0xf0;
                gGuiPulseStep = step;
                gGuiPulseHold = 0;
                return;
            }
        } else {
            gGuiPulse -= 8;
            if (gGuiPulse < 0x10) {
                step = -step;
                gGuiPulse = 0x10;
                gGuiPulseStep = step;
            }
        }
        gGuiPulseHold = 0;
    }
}

BMX_ADDR(0x004375a0)
void guiSetHook(GuiHookFn hook)
{
    gGuiAssetsLoaded = 1;
    gGuiScreen.hook = hook;
}

/* ---- element list ----------------------------------------------------------------------------- */

BMX_ADDR(0x00438380)
GuiScreenState *guiGetScreenState(void)
{
    return &gGuiScreen;
}

BMX_ADDR(0x004383a0)
GuiElement *guiFindById(uint32_t id)
{
    const int count = gGuiElementCount;
    for (int i = 0; i < count; i++) {
        if (gGuiSorted[i]->id == id) {
            return gGuiSorted[i];
        }
    }
    return NULL;
}

BMX_ADDR(0x004383e0)
GuiElement *guiGetByIndex(uint32_t index)
{
    if (index >= (uint32_t)gGuiElementCount) {
        return NULL;
    }
    return gGuiSorted[index];
}

/* qsort comparator: higher `order` first. */
BMX_ADDR(0x00438410)
int BMX_CDECL guiCompareOrder(const void *a, const void *b)
{
    const GuiElement *ea = *(GuiElement *const *)a;
    const GuiElement *eb = *(GuiElement *const *)b;
    return (int)eb->order - (int)ea->order;
}

BMX_ADDR(0x00438440)
void guiSortElements(void)
{
    if (gGuiElementCount > 1) {
        qsort(&gGuiSorted, (size_t)gGuiElementCount, 4,
              BMX_FNPTR(int (*)(const void *, const void *), 0x00402072, guiCompareOrder));
    }
}

/*
 * Claims the first free slot, installs the type handler and announces it with kGuiMsgCreate. The type
 * switch's jump table sits at 0x43858c (data, not code).
 */
BMX_ADDR(0x00438470)
GuiElement *guiCreateElement(int16_t type, int16_t order, uint32_t id)
{
    GuiElement *el = gGuiElements;
    int slot = 0;
    while (el->type != 0) {
        el++;
        slot++;
        if (slot >= kGuiMaxElements) {
            guiDebug("GUIMAIN: Error - failed to allocate element\n");
            return NULL;
        }
    }
    memset(el, 0, sizeof *el);
    el->type = type;
    el->order = (uint16_t)order;
    el->id = id;
    switch ((uint16_t)type) {
    case kGuiTypeLabel:
        el->defaultProc = BMX_FNPTR(GuiProcFn, 0x00401e4c, guiLabelProc);
        break;
    case kGuiTypeButton:
        el->defaultProc = BMX_FNPTR(GuiProcFn, 0x00401a78, guiButtonProc);
        break;
    case kGuiTypeToggle: {
        el->defaultProc = BMX_FNPTR(GuiProcFn, 0x00401c4e, guiToggleProc);
        el->dataHandle = zmemdmmAlloc(sizeof(GuiToggleData));
        memset(zmemdmmLock(el->dataHandle), 0, sizeof(GuiToggleData));
        zmemdmmUnlock(el->dataHandle);
        break;
    }
    case kGuiTypeLabel5:
        el->defaultProc = BMX_FNPTR(GuiProcFn, 0x00401e4c, guiLabelProc);
        el->animTicks = 0;
        el->frame = 0;
        break;
    case kGuiTypeNone:
        el->defaultProc = BMX_FNPTR(GuiProcFn, 0x00402509, guiNullHandler);
        break;
    case kGuiTypeRect:
        el->defaultProc = BMX_FNPTR(GuiProcFn, 0x00401834, guiRectProc);
        break;
    case kGuiTypeBox:
        el->defaultProc = BMX_FNPTR(GuiProcFn, 0x00401e83, guiBoxProc);
        break;
    default:
        break;
    }
    gGuiSorted[gGuiElementCount] = el;
    gGuiElementCount++;
    guiSortElements();
    guiSend(kGuiMsgCreate, el, 0, 0);
    return el;
}

/* Sends kGuiMsgDestroy, frees the element's text and data, clears the slot and unlinks it. */
BMX_ADDR(0x00438600)
void guiDestroyElement(GuiElement *el)
{
    if (el == NULL) {
        return;
    }
    guiSend(kGuiMsgDestroy, el, 0, 0);
    if (el->textHandle != 0) {
        zmemdmmFree(el->textHandle);
        el->textHandle = 0;
        el->textCapacity = 0;
    }
    if (el->dataHandle != 0) {
        if (el->type == kGuiTypeToggle) {
            GuiToggleData *data = zmemdmmLock(el->dataHandle);
            if (data->choicesHandle != 0) {
                zmemdmmFree(data->choicesHandle);
                data->choicesHandle = 0;
            }
            if (data->textHandle != 0) {
                zmemdmmFree(data->textHandle);
                data->textHandle = 0;
            }
            zmemdmmUnlock(el->dataHandle);
        }
        zmemdmmFree(el->dataHandle);
        el->dataHandle = 0;
    }
    memset(el, 0, sizeof *el);
    const int last = gGuiElementCount - 1;
    for (int i = last; i >= 0; i--) {
        if (el == gGuiSorted[i]) {
            /* The last entry fills the hole (a no-op when el was the last one). */
            GuiElement *moved = gGuiSorted[last];
            gGuiElementCount = last;
            gGuiSorted[i] = moved;
            el->type = 0;
            guiSortElements();
            return;
        }
    }
    guiDebug("GUIMAIN: Error - freeing element not in list\n");
}
