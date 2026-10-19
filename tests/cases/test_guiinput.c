#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/menu/gui/guimain.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_gui.h"
#include "dt_zmem.h"

/* Pad index argument: pad 0 almost always, otherwise the out-of-range assert. */
static uint32_t pad_index(void)
{
    const uint32_t v = dt_u32() % 16;
    return v == 0 ? 1 : (v == 1 ? 0xffffffffu : (v == 2 ? (uint32_t)dt_range(2, 9) : 0));
}

static void setup_pad_get(DtCtx *c)
{
    dt_seed(c);
    dtGuiRandomPad();
    DT_ARG_INT(c, 0, pad_index());
}

static void setup_pad_test(DtCtx *c)
{
    dt_seed(c);
    dtGuiRandomPad();
    DT_ARG_INT(c, 0, pad_index());
}

static void setup_pad_state(DtCtx *c)
{
    dt_seed(c);
    dtGuiRandomPad();
}

static void setup_nop(DtCtx *c)
{
    dt_seed(c);
}

static void setup_velocity_sound(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32() % 4 == 0 ? dt_range(1, 3) : 0);
    switch (c->seed % 6) {
    case 0:
        DT_ARG_FLOAT(c, 2, 10.0f); /* the threshold itself does not play */
        break;
    case 1:
        DT_ARG_FLOAT(c, 2, dt_float(1e9f, 3e9f)); /* volume overflows the int conversion */
        break;
    case 2:
        DT_ARG_FLOAT(c, 2, dt_float(10.0f, 22.0f)); /* just under the volume clamp at 127 */
        break;
    default:
        DT_ARG_FLOAT(c, 2, dt_float_edgy(-5.0f, 60.0f));
    }
    DT_ARG_PTR(c, 3, dt_alloc(64));
}

/*
 * Controller device creation: the engine's input layer talks to DirectInput through a COM object.
 * A fake IDirectInput whose CreateDeviceEx always fails makes the creation fall back to a device record
 * without a hardware device, which is enough for the GUI side. The record comes from two free lists.
 */
static long BMX_STDCALL createDeviceFails(void *self, void *guid, void *iid, void **out, void *outer)
{
    return -1;
}

#define gDirectInput    ORIG_VAR(0x00674038, void *, gDirectInput)
#define gKeyboardDevice ORIG_VAR(0x0067404c, uint32_t, gKeyboardDevice)
#define gPadDeviceList  ORIG_VAR(0x0065e2ac, void *, gPadDeviceList)
#define gPadStateList   ORIG_VAR(0x0067402c, void *, gPadStateList)

static void install_fake_input(void)
{
    static void *vtable[32];
    static void *comObject;
    vtable[9] = (void *)createDeviceFails;
    comObject = (void *)vtable;
    gDirectInput = (void *)&comObject;
    gKeyboardDevice = 1;
    dt_zmem_env();
    for (int i = 0; i < 128; i++) {
        gZmemFreeListInUse[i] = 0;
    }
    gPadDeviceList = DT_ORIG(0x00485e80, void *(*)(uint32_t, uint32_t, void *))(0x74, 0x20, NULL);
    gPadStateList = DT_ORIG(0x00485e80, void *(*)(uint32_t, uint32_t, void *))(0x14, 0x20, NULL);
}

static void setup_pad_init(DtCtx *c)
{
    dt_seed(c);
    dtGuiRandomPad();
    gGuiDebugPrint = dtGuiDebugPrint;
    install_fake_input();
}

static void setup_pad_shutdown(DtCtx *c)
{
    setup_pad_init(c);
    gGuiPad.device = zpadCreateDevice(2);
}

/* A button inside a small screen; some elements share its id block so the nav bytes can hit them. */
static void setup_button(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTick,    kGuiMsgDraw,     kGuiMsgDraw,       kGuiMsgFocus,
                               kGuiMsgFocus,   kGuiMsgBlur,     kGuiMsgBlur,       kGuiMsgNavUp,
                               kGuiMsgNavDown, kGuiMsgNavUpAlt, kGuiMsgNavDownAlt, kGuiMsgNavLeft,
                               kGuiMsgCreate};
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    dtGuiRandomScreen(dt_range(0, 4));
    GuiElement *el = dtGuiRandomElement(10);
    el->type = kGuiTypeButton;
    el->flags &= (uint16_t)~kGuiFlagHidden;
    el->id = 0x700;
    el->nav[0] = (uint8_t)dt_range(0, 3);
    el->nav[1] = (uint8_t)dt_range(0, 3);
    gGuiSorted[gGuiElementCount++] = el;
    for (int i = 0; i + 1 < gGuiElementCount; i++) {
        if (dt_u32() % 2) {
            gGuiSorted[i]->id = 0x700u + (uint32_t)dt_range(0, 3);
        }
    }
    const int frames = dt_range(1, 3);
    el->sprite = c->seed % 3 == 0 ? NULL : dtGuiMakeSprite(0x190, frames);
    el->spriteFocused = c->seed % 4 == 0 ? dtGuiMakeSprite(0x191, frames) : NULL;
    el->frame = (uint16_t)dt_range(0, frames);
    el->font = dtGuiMakeFont(0x192, (uint16_t)dt_range(6, 18));
    el->animFlags = (uint16_t)dt_u32() & (uint16_t)~kGuiAnimPlaying;
    el->textFlags = (uint16_t)dt_u32();
    if (dt_u32() % 2) {
        el->textFlags &= (uint16_t)~kGuiTextDisabled;
        el->textHandle = dtGuiTextHandle(dtGuiRandomText(12, 1));
        el->textCapacity = 13;
    }
    dtGuiMakeFont(0x193, 8);
    dtGuiMakeFont(0x194, 9);
    gGuiScreen.focusFont = c->seed % 3 == 0 ? 0 : 0x193;
    gGuiScreen.normalFont = c->seed % 4 == 0 ? 0 : 0x194;
    for (int i = 0; i < 3; i++) {
        gGuiScreen.focusRgb[i] = (uint8_t)dt_u32();
        gGuiScreen.normalRgb[i] = (uint8_t)dt_u32();
    }
    gGuiPulse = dt_range(0x10, 0xf0);
    const int msg = msgs[dt_u32() % (sizeof msgs / sizeof msgs[0])];
    const int navigation =
        msg == kGuiMsgNavUp || msg == kGuiMsgNavDown || msg == kGuiMsgNavUpAlt || msg == kGuiMsgNavDownAlt;
    DT_ARG_INT(c, 0, msg);
    DT_ARG_PTR(c, 1, navigation && c->seed % 7 == 6 ? NULL : el); /* only navigation checks for NULL */
    DT_ARG_INT(c, 2, dt_u32() % 3 != 0);
    DT_ARG_INT(c, 3, dt_u32());
}

DT_CASES(guiinput_cases) = {
    {"guiPadInit", 0x00437200, guiPadInit, 0, DT_RET_VOID, setup_pad_init, 200, 0},
    {"guiPadShutdown", 0x00437290, guiPadShutdown, 0, DT_RET_VOID, setup_pad_shutdown, 200, 0},
    {"guiButtonProc", 0x00436520, guiButtonProc, 4, DT_RET_INT, setup_button, 600, 0},
    {"guiPadGet", 0x00437420, guiPadGet, 1, DT_RET_INT, setup_pad_get, 200, 0},
    {"guiPadPoll", 0x004372b0, guiPadPoll, 0, DT_RET_INT, setup_pad_state, 300, 0},
    {"guiPadReset", 0x004373e0, guiPadReset, 0, DT_RET_VOID, setup_pad_state, 200, 0},
    {"guiPadRepeatBit1", 0x00436c20, guiPadRepeatBit1, 1, DT_RET_INT, setup_pad_test, 400, 0},
    {"guiPadRepeatBit2", 0x00436ce0, guiPadRepeatBit2, 1, DT_RET_INT, setup_pad_test, 400, 0},
    {"guiPadRepeatBit4", 0x00436da0, guiPadRepeatBit4, 1, DT_RET_INT, setup_pad_test, 400, 0},
    {"guiPadRepeatBit8", 0x00436e60, guiPadRepeatBit8, 1, DT_RET_INT, setup_pad_test, 400, 0},
    {"guiPadPressedBit4", 0x00436f20, guiPadPressedBit4, 1, DT_RET_INT, setup_pad_test, 300, 0},
    {"guiPadPressedBit8", 0x00436f70, guiPadPressedBit8, 1, DT_RET_INT, setup_pad_test, 300, 0},
    {"guiPadPressedMask20080", 0x00436fc0, guiPadPressedMask20080, 1, DT_RET_INT, setup_pad_test, 300, 0},
    {"guiPadPressedMask40", 0x00437000, guiPadPressedMask40, 1, DT_RET_INT, setup_pad_test, 300, 0},
    {"guiPadPressedMask4000", 0x00437040, guiPadPressedMask4000, 1, DT_RET_INT, setup_pad_test, 300, 0},
    {"guiPadPressedMask10010", 0x00437080, guiPadPressedMask10010, 1, DT_RET_INT, setup_pad_test, 300, 0},
    {"guiPadPressedMask100", 0x004370c0, guiPadPressedMask100, 1, DT_RET_INT, setup_pad_test, 300, 0},
    {"guiPadPressedMask400", 0x00437100, guiPadPressedMask400, 1, DT_RET_INT, setup_pad_test, 300, 0},
    {"guiPadPressedMask200", 0x00437140, guiPadPressedMask200, 1, DT_RET_INT, setup_pad_test, 300, 0},
    {"guiPadPressedMask800", 0x00437180, guiPadPressedMask800, 1, DT_RET_INT, setup_pad_test, 300, 0},
    {"guiPadPressedMask20", 0x004371c0, guiPadPressedMask20, 1, DT_RET_INT, setup_pad_test, 300, 0},
    {"guiPlayVelocitySound", 0x004364b0, guiPlayVelocitySound, 4, DT_RET_VOID, setup_velocity_sound, 300, 0},
    {"guiHelpInit", 0x00436ac0, guiHelpInit, 0, DT_RET_INT, setup_nop, 1, 0},
    {"guiNullHandler", 0x00436ae0, guiNullHandler, 0, DT_RET_INT, setup_nop, 1, 0},
    {"guiPadMultiMode", 0x00436b00, guiPadMultiMode, 0, DT_RET_INT, setup_nop, 1, 0},
    {"guiStub436b20", 0x00436b20, guiStub436b20, 0, DT_RET_INT, setup_nop, 1, 0},
    {"guiStub436b40", 0x00436b40, guiStub436b40, 0, DT_RET_VOID, setup_nop, 1, 0},
    {"guiScreenLoadHook", 0x00436b60, guiScreenLoadHook, 0, DT_RET_VOID, setup_nop, 1, 0},
    {"guiStub436b80", 0x00436b80, guiStub436b80, 0, DT_RET_VOID, setup_nop, 1, 0},
    {"guiPadMultiPress", 0x00436ba0, guiPadMultiPress, 1, DT_RET_VOID, setup_nop, 1, 0},
    {"guiPadMultiEnd", 0x00436bc0, guiPadMultiEnd, 0, DT_RET_VOID, setup_nop, 1, 0},
    {"guiStub436be0", 0x00436be0, guiStub436be0, 0, DT_RET_VOID, setup_nop, 1, 0},
    {"guiStub436c00", 0x00436c00, guiStub436c00, 0, DT_RET_INT, setup_nop, 1, 0},
};
