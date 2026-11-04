/* GUI controller input (C:\zaxis\Bmx\src\com\menu\gui\guiinput.c): pad polling and button tests. */
#include "bmx/com/menu/gui/guiinput.h"

#include "bmx/com/menu/gui/guilabel.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "zge/zdebug.h"

static const char guiinputFile[] = "C:\\zaxis\\Bmx\\src\\com\\menu\\gui\\guiinput.c";

/* The only controller GUI code looks at is pad 0; the 32 byte stride is the original's. */
BMX_ADDR(0x00437420)
GuiPad *guiPadGet(uint32_t index)
{
    if (index >= 1) {
        zdebugFail("controller number out of range", guiinputFile, 0x307, 0);
    }
    return (GuiPad *)((uint8_t *)&gGuiPad + (index << 5));
}

BMX_ADDR(0x00437200)
void guiPadInit(void)
{
    gGuiPad.device = zpadCreateDevice(2);
    if (gGuiPad.device == NULL) {
        gGuiPad.device = zpadCreateDevice(0);
    }
    gGuiPad.flags = 0;
    gGuiPad.flags2 = 0;
    if (gGuiPad.device == NULL) {
        zdebugFail("could not create controller device", guiinputFile, 0x267, 0);
    }
    if ((gGuiPad.device->status & 1) == 0) {
        gGuiPad.flags |= 1;
    }
}

BMX_ADDR(0x00437290)
void guiPadShutdown(void)
{
    zpadDestroyDevice(gGuiPad.device);
}

/*
 * Copies the device state into gGuiPad. Returns bit 0 when the device just disappeared and bit 1 when
 * it just came back. The analog block is dead: it tests the constant 0.0 against 0.5.
 */
BMX_ADDR(0x004372b0)
uint32_t guiPadPoll(void)
{
    static const double kAnalogProbe = 0.0; /* double at 0x54d210 */
    uint32_t result = 0;
    GuiPadDevice *dev = gGuiPad.device;

    if ((gGuiPad.flags & 1) != 0 && (dev->status & 1) != 0) {
        gGuiPad.flags &= 0xfffe;
        gGuiPad.flags2 = 0;
        result = 2;
    }
    if ((gGuiPad.flags & 1) == 0) {
        if ((dev->status & 1) == 0) {
            gGuiPad.flags |= 1;
            result |= 1;
        } else {
            gGuiPad.held = dev->held;
            gGuiPad.pressed = dev->pressed;
            gGuiPad.prevAxis[0] = gGuiPad.axis[0];
            gGuiPad.prevAxis[1] = gGuiPad.axis[1];
            gGuiPad.axis[0] = 0;
            gGuiPad.axis[1] = 0;
            gGuiPad.axis[2] = 0;
            gGuiPad.axis[3] = 0;
            if (gGuiAnalogPad == 1) {
                const double magnitude = kAnalogProbe < 0.0 ? -kAnalogProbe : kAnalogProbe;
                if (magnitude > 0.5f) {
                    gGuiPad.held |= 8;
                }
                if (magnitude > 0.5f) {
                    gGuiPad.held |= 2;
                }
            }
        }
    }
    return result;
}

BMX_ADDR(0x004373e0)
void guiPadReset(void)
{
    gGuiPad.held = 0;
    gGuiPad.pressed = 0;
    gGuiPad.flags2 = 0;
    gGuiPad.axis[0] = 0;
    gGuiPad.axis[1] = 0;
    gGuiPad.axis[2] = 0;
    gGuiPad.axis[3] = 0;
}

/*
 * One d-pad direction with auto-repeat: fires on the press, then after 20 frames of holding every 5th
 * frame. Pressing another direction restarts the repeat.
 */
static int guiPadRepeat(uint32_t index, uint32_t others, uint32_t bit)
{
    const GuiPad *pad = guiPadGet(index);
    if ((pad->flags & 1) == 0) {
        const uint32_t fresh = pad->pressed & pad->held;
        if ((fresh & others) == 0) {
            if ((fresh & bit) != 0) {
                gGuiPadRepeatCount = 0;
                gGuiPadRepeatHeld = pad->held;
                return 1;
            }
            if ((pad->held & gGuiPadRepeatHeld & bit) != 0) {
                gGuiPadRepeatCount++;
                if (gGuiPadRepeatCount > 0x14 && gGuiPadRepeatCount % 5 == 0) {
                    return 1;
                }
            }
        } else {
            gGuiPadRepeatCount = 0;
            gGuiPadRepeatHeld = pad->held;
        }
    }
    return 0;
}

BMX_ADDR(0x00436c20)
int guiPadRepeatBit1(uint32_t index)
{
    return guiPadRepeat(index, 0xe, 1);
}

BMX_ADDR(0x00436ce0)
int guiPadRepeatBit2(uint32_t index)
{
    return guiPadRepeat(index, 0xd, 2);
}

BMX_ADDR(0x00436da0)
int guiPadRepeatBit4(uint32_t index)
{
    return guiPadRepeat(index, 0xb, 4);
}

/* 0x436ee8 is the int3 padding after this function, not code. */
BMX_ADDR(0x00436e60)
int guiPadRepeatBit8(uint32_t index)
{
    return guiPadRepeat(index, 0x7, 8);
}

/*
 * Level test for a d-pad direction while no other direction is held (no repeat). The saved button
 * state follows the held buttons whenever another direction is down or on a hit.
 */
static int guiPadDirection(uint32_t index, uint32_t others, uint32_t bit)
{
    const GuiPad *pad = guiPadGet(index);
    if ((pad->flags & 1) != 0) {
        return 0;
    }
    const uint32_t held = pad->held;
    if ((held & others) != 0) {
        gGuiPadRepeatHeld = held;
        return 0;
    }
    if ((held & bit) != 0) {
        gGuiPadRepeatHeld = held;
        return 1;
    }
    return 0;
}

BMX_ADDR(0x00436f20)
int guiPadPressedBit4(uint32_t index)
{
    return guiPadDirection(index, 0xb, 4);
}

BMX_ADDR(0x00436f70)
int guiPadPressedBit8(uint32_t index)
{
    return guiPadDirection(index, 0x7, 8);
}

static int guiPadButton(uint32_t index, uint32_t mask)
{
    const GuiPad *pad = guiPadGet(index);
    return (pad->flags & 1) == 0 && (pad->pressed & pad->held & mask) != 0;
}

BMX_ADDR(0x00436fc0)
int guiPadPressedMask20080(uint32_t index)
{
    return guiPadButton(index, 0x20080);
}

BMX_ADDR(0x00437000)
int guiPadPressedMask40(uint32_t index)
{
    return guiPadButton(index, 0x40);
}

BMX_ADDR(0x00437040)
int guiPadPressedMask4000(uint32_t index)
{
    return guiPadButton(index, 0x4000);
}

BMX_ADDR(0x00437080)
int guiPadPressedMask10010(uint32_t index)
{
    return guiPadButton(index, 0x10010);
}

BMX_ADDR(0x004370c0)
int guiPadPressedMask100(uint32_t index)
{
    return guiPadButton(index, 0x100);
}

BMX_ADDR(0x00437100)
int guiPadPressedMask400(uint32_t index)
{
    return guiPadButton(index, 0x400);
}

BMX_ADDR(0x00437140)
int guiPadPressedMask200(uint32_t index)
{
    return guiPadButton(index, 0x200);
}

BMX_ADDR(0x00437180)
int guiPadPressedMask800(uint32_t index)
{
    return guiPadButton(index, 0x800);
}

BMX_ADDR(0x004371c0)
int guiPadPressedMask20(uint32_t index)
{
    return guiPadButton(index, 0x20);
}

BMX_ADDR(0x004364b0)
void guiPlayVelocitySound(struct ZRig *rig, uint32_t kind, float speed, struct ZRigHit *hit)
{
    (void)rig;
    if (kind == 0 && speed > 10.0f) {
        int volume = (int)((double)speed * 6.0f);
        if (volume >= 0x7f) {
            volume = 0x7f;
        }
        bmxSoundPlayAt(0xbbc, &hit->point, 0, volume, 0);
    }
}

BMX_ADDR(0x00436ac0)
int guiHelpInit(void)
{
    return 0;
}

/* Type 6 element handler: ignores every message. */
BMX_ADDR(0x00436ae0)
int guiNullHandler(void)
{
    return 0;
}

BMX_ADDR(0x00436b00)
int guiPadMultiMode(void)
{
    return 0;
}

BMX_ADDR(0x00436b20)
int guiStub436b20(void)
{
    return 0;
}

BMX_ADDR(0x00436b40)
void guiStub436b40(void) {}

BMX_ADDR(0x00436b60)
void guiScreenLoadHook(void) {}

BMX_ADDR(0x00436b80)
void guiStub436b80(void) {}

BMX_ADDR(0x00436ba0)
void guiPadMultiPress(int msg) {}

BMX_ADDR(0x00436bc0)
void guiPadMultiEnd(void) {}

BMX_ADDR(0x00436be0)
void guiStub436be0(void) {}

BMX_ADDR(0x00436c00)
int guiStub436c00(void)
{
    return 1;
}

/* Reads the three bytes of a focus/normal colour pair and applies them as the element's text colour. */
static void guiApplyTextColor(GuiElement *el, void (*getRgb)(uint8_t *, uint8_t *, uint8_t *))
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    getRgb(&r, &g, &b);
    guiSetTextRgb(el, r, g, b);
}

/*
 * Button element handler. Draw/tick go to the label handler (a focused button draws as selected).
 * Focus unfocuses every other element, highlights this one and switches to the focus font/colour; blur
 * undoes that. Up/down (and their alt ids) move the focus to the neighbour named in nav[0]/nav[1].
 * The original is one function whose switch cases sit at 0x436543 (draw/tick), 0x436565, 0x4365a9
 * (focus), 0x436621 (blur), 0x436689, 0x4366b8 (navigation) and 0x4366e7 (default).
 */
BMX_ADDR(0x00436520)
int BMX_CDECL guiButtonProc(int msg, GuiElement *el, intptr_t arg3, intptr_t arg4)
{
    switch (msg) {
    case kGuiMsgTick:
        guiLabelProc(msg, el, arg3, arg4);
        return 1;
    case kGuiMsgDraw:
        if ((el->flags & kGuiFlagFocused) != 0) {
            guiLabelProc(kGuiMsgDraw, el, 1, arg4);
            return 1;
        }
        guiLabelProc(msg, el, arg3, arg4);
        return 1;
    case kGuiMsgFocus: {
        GuiElement *other;
        for (uint32_t i = 0; (other = guiGetByIndex(i)) != NULL; i++) {
            if (other != el) {
                guiSend(kGuiMsgBlur, other, 0, 0);
            }
        }
        el->flags |= kGuiFlagFocused;
        guiSetHighlight(el, 1);
        const uint16_t font = guiGetFocusFont();
        if (font != 0) {
            guiSetFont(el, font);
        }
        if ((el->textFlags & kGuiTextColored) != 0) {
            guiApplyTextColor(el, guiGetFocusRgb);
        }
        return 1;
    }
    case kGuiMsgBlur: {
        el->flags &= (uint16_t)~kGuiFlagFocused;
        guiSetHighlight(el, 0);
        const uint16_t font = guiGetNormalFont();
        if (font != 0) {
            guiSetFont(el, font);
        }
        if ((el->textFlags & kGuiTextColored) != 0) {
            guiApplyTextColor(el, guiGetNormalRgb);
        }
        return 1;
    }
    case kGuiMsgNavUp:
    case kGuiMsgNavUpAlt:
        if (el != NULL && el->nav[0] != 0) {
            guiFocusById((el->id & 0xffffff00u) | el->nav[0]);
            return 1;
        }
        return 0;
    case kGuiMsgNavDown:
    case kGuiMsgNavDownAlt:
        if (el != NULL && el->nav[1] != 0) {
            guiFocusById((el->id & 0xffffff00u) | el->nav[1]);
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}
