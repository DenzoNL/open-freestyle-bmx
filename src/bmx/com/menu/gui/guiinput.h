/* GUI controller input (C:\zaxis\Bmx\src\com\menu\gui\guiinput.c): pad polling and button tests. */
#ifndef BMX_COM_MENU_GUI_GUIINPUT_H
#define BMX_COM_MENU_GUI_GUIINPUT_H

#include <stddef.h>
#include <stdint.h>

#include "bmx/com/menu/gui/gui.h"
#include "compat/orig.h"
#include "zge/pc/pcinput.h"

/* Device returned by the engine's controller layer (0x480370). Only the fields the GUI reads. */
typedef struct GuiPadDevice {
    uint32_t unknown0;
    uint8_t status; /* bit 0: connected */
    uint8_t pad5[3];
    uint32_t held;    /* buttons currently down */
    uint32_t pressed; /* buttons that went down this poll */
} GuiPadDevice;

/*
 * GUI view of controller 0. guiPadGet indexes this block in 32 byte steps although it is 40 bytes
 * long, so only pad 0 is valid (anything else asserts).
 */
typedef struct GuiPad {
    GuiPadDevice *device;
    uint32_t held;
    uint32_t pressed;
    uint16_t flags; /* bit 0: device is disconnected */
    uint16_t flags2;
    uint32_t axis[4];
    uint32_t prevAxis[2];
} GuiPad;

BMX_LAYOUT32_ASSERT(sizeof(GuiPad) == 0x28, "GuiPad layout");

#define gGuiPad ORIG_VAR(0x0056d298, GuiPad, gGuiPad)
/* Last button state and repeat count for the auto-repeating d-pad tests. */
#define gGuiPadRepeatHeld  ORIG_VAR(0x00691f2c, uint32_t, gGuiPadRepeatHeld)
#define gGuiPadRepeatCount ORIG_VAR(0x00691f3c, uint32_t, gGuiPadRepeatCount)
/* Set when the analog stick should also drive the d-pad bits. */
#define gGuiAnalogPad ORIG_VAR(0x005ab3a0, uint8_t, gGuiAnalogPad)

/* Controller layer: zpadCreateDevice, zpadDestroyDevice and zpadGetDevice (zge/pc/pcinput.h). */

GuiPad *guiPadGet(uint32_t index);
void guiPadInit(void);
void guiPadShutdown(void);
uint32_t guiPadPoll(void);
void guiPadReset(void);

/* d-pad direction tests with auto-repeat; 1 when the direction fires this frame. */
int guiPadRepeatBit1(uint32_t index);
int guiPadRepeatBit2(uint32_t index);
int guiPadRepeatBit4(uint32_t index);
int guiPadRepeatBit8(uint32_t index);
/* Edge triggered direction tests without repeat. */
int guiPadPressedBit4(uint32_t index);
int guiPadPressedBit8(uint32_t index);
/* Button tests: 1 when a button of the mask went down. The GUI turns them into messages 0x22..0x2a. */
int guiPadPressedMask20080(uint32_t index);
int guiPadPressedMask40(uint32_t index);
int guiPadPressedMask4000(uint32_t index);
int guiPadPressedMask10010(uint32_t index);
int guiPadPressedMask100(uint32_t index);
int guiPadPressedMask400(uint32_t index);
int guiPadPressedMask200(uint32_t index);
int guiPadPressedMask800(uint32_t index);
int guiPadPressedMask20(uint32_t index);

/*
 * Plays sound 0xbbc at the emitter's position (emitter + 8) when kind is 0 and speed exceeds 10; the
 * volume is speed * 6, at most 127. Called through a pointer by the engine.
 */
struct ZRig;
struct ZRigHit;
void guiPlayVelocitySound(struct ZRig *rig, uint32_t kind, float speed, struct ZRigHit *hit);

/* Button element handler (type 2). */
int BMX_CDECL guiButtonProc(int msg, GuiElement *el, int arg3, int arg4);

/* Leftovers of the console port: empty or constant functions kept for their addresses. */
int guiHelpInit(void);
int guiNullHandler(void);
int guiPadMultiMode(void);
int guiStub436b20(void);
void guiStub436b40(void);
void guiScreenLoadHook(void);
void guiStub436b80(void);
void guiPadMultiPress(int msg);
void guiPadMultiEnd(void);
void guiStub436be0(void);
int guiStub436c00(void);

#endif
