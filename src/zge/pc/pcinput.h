/*
 * Controller devices of the PC platform layer (C:\NgEngine\Zge\PC\pcinput.c). A device is a 0x74 byte block
 * from a free list: the kind at +0, the button state the polling fills in, and the DirectInput object at
 * +0x70 (see GuiPadDevice for the part the GUI reads).
 */
#ifndef ZGE_PC_PCINPUT_H
#define ZGE_PC_PCINPUT_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmem.h"

struct GuiPadDevice;

enum { kZpadDeviceBytes = 0x74, kZpadDirectInputSlot = 0x70 };

/* Free list of devices (0x65e2ac). */
#define gZpadDevices ORIG_VAR(0x0065e2ac, ZMemFreeList *, gZpadDevices)

/* Creates a device of `kind` (2 keyboard, 0 pad); NULL when DirectInput cannot set it up. */
struct GuiPadDevice *zpadCreateDevice(int kind);
/* Releases the DirectInput object and the block; returns 1. */
int zpadDestroyDevice(struct GuiPadDevice *device);
/* The n-th device of the controller list, or NULL (used by the key binding screen). */
struct GuiPadDevice *zpadGetDevice(int index);
/* Polls all devices (a jump to the platform's update at 0x493910); returns what it returns. */
int zpadPoll(void);

/* Platform side, not ported yet. */
ORIG_FN(0x00493310, int, zpadSetupDirectInput, (int kind, void *device));
ORIG_FN(0x004937f0, void, zpadReleaseDirectInput, (void *device));
ORIG_FN(0x00493910, int, zpadPollDevices, (void));

#endif
