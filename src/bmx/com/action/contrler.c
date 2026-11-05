/* Controller handling (C:\zaxis\Bmx\Src\com\action\contrler.c). */
#include "bmx/com/action/contrler.h"

#include <stddef.h>

#include "bmx/com/action/overlay.h"
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guiinput.h"
#include "compat/trace.h"
#include "zge/zdebug.h"

static const char contrlerFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\contrler.c";

enum { kPadCount = 2 };

/* x87 `FCOMP thr; TEST AH,0x41; JNZ` falls through only for |v| > 0.5 (never for NaN). */
static int stickPast(float v)
{
    const double a = v < 0 ? -(double)v : (double)v;
    return a > 0.5;
}

/* Direction bit of a stick value: `neg` below zero (and for NaN), else `pos`. */
static uint32_t stickDir(float v, uint32_t neg, uint32_t pos)
{
    return 0.0 <= (double)v ? pos : neg;
}

/*
 * Creates the engine device of both pads (kind 2, else kind 0). A pad whose device is not connected
 * starts flagged as disconnected.
 */
BMX_ADDR(0x00408030)
void contrlerCreateDevices(void)
{
    for (int i = 0; i < kPadCount; i++) {
        ContrlerPad *const pad = &gContrlerPads[i];

        pad->device = (ContrlerDevice *)zpadCreateDevice(2);
        if (pad->device == NULL) {
            pad->device = (ContrlerDevice *)zpadCreateDevice(0);
        }
        pad->flags = 0;
        pad->stuck = 0;
        pad->flags2 = 0;
        if (pad->device == NULL) {
            zdebugFail("could not create controller device", contrlerFile, 0x1b8, 0);
        }
        if ((pad->device->status & 1) == 0) {
            pad->flags |= 1;
        }
    }
    gContrlerMode = 1;
}

BMX_ADDR(0x004080c0)
void contrlerFreeDevices(void)
{
    for (int i = 0; i < kPadCount; i++) {
        zpadDestroyDevice((GuiPadDevice *)gContrlerPads[i].device);
    }
}

/* Clears buttons and sticks of a pad (not its flags). */
static void clearState(ContrlerPad *pad)
{
    pad->state.buttons = 0;
    pad->state.pressed = 0;
    for (int k = 0; k < 4; k++) {
        pad->state.axis[k] = 0.0f;
        pad->state.prevAxis[k] = 0.0f;
    }
}

/* The button mask the device has to show for game button `bit` (the configured pad button of the actions). */
static uint32_t padMask(const PlayerSetup *setup, uint32_t bit)
{
    switch (bit) {
    case 0x80:
        return setup->bind[4];
    case 0x20:
        return setup->bind[5];
    case 0x40:
        return setup->bind[6];
    case 0x10:
        return setup->bind[7];
    case 0x100:
        return setup->bind[8];
    case 0x200:
        return setup->bind[9];
    case 0x4000:
        return setup->bind[10];
    default:
        return bit;
    }
}

/*
 * Polls both pads: remaps the device buttons through the player's bindings, derives the edges (buttons
 * held since a (re)connect are ignored until released) and shifts the sticks. Returns bit 0 when a pad
 * got disconnected and bit 1 when one got reconnected.
 */
BMX_ADDR(0x004080f0)
uint32_t contrlerPoll(void)
{
    uint32_t result = 0;

    BMX_TRACE_POLL();
    for (int i = 0; i < kPadCount; i++) {
        ContrlerPad *const pad = &gContrlerPads[i];
        const PlayerSetup *const setup = &gPlayers[i];
        uint32_t raw = 0;
        uint16_t flags = pad->flags;

        pad->flags = flags & 0xfffd;
        flags = pad->flags;
        if ((flags & 1) != 0) {
            if ((pad->device->status & 1) == 0) {
                pad->state.buttons = 0;
                pad->state.pressed = 0;
                pad->stuck = 0;
                for (int k = 0; k < 4; k++) {
                    pad->state.axis[k] = 0.0f;
                    pad->state.prevAxis[k] = 0.0f;
                }
            } else {
                result |= 2;
                pad->flags2 = 0;
                pad->flags = (flags & 0xfffe) | 2;
            }
        }
        if ((pad->flags & 1) != 0) {
            continue;
        }
        if ((pad->device->status & 1) == 0) {
            pad->flags |= 1;
            pad->state.buttons = 0;
            pad->state.pressed = 0;
            pad->stuck = 0;
            for (int k = 0; k < 4; k++) {
                pad->state.axis[k] = 0.0f;
                pad->state.prevAxis[k] = 0.0f;
            }
            result |= 1;
            continue;
        }
        for (uint32_t b = 0; b < 32; b++) {
            const uint32_t bit = 1u << b;
            if ((pad->device->held & padMask(setup, bit)) != 0) {
                raw |= bit;
            }
        }
        {
            const uint32_t changed = (pad->state.buttons ^ raw) & ~pad->stuck;
            pad->state.pressed = changed;
            pad->state.buttons ^= changed;
            pad->stuck &= raw;
        }
        for (int k = 0; k < 4; k++) {
            pad->state.prevAxis[k] = pad->state.axis[k];
            pad->state.axis[k] = 0.0f;
        }
        for (int k = 0; k < 4; k++) {
            pad->state.axis[k] = *(const float *)&pad->device->axis[k];
        }
    }
    return result;
}

BMX_ADDR(0x00408390)
void contrlerClear(void)
{
    for (int i = 0; i < kPadCount; i++) {
        ContrlerPad *const pad = &gContrlerPads[i];

        clearState(pad);
        pad->flags2 = 0;
    }
}

/*
 * Switches the mode of the pads. Leaving mode 0 saves the buttons; entering it restores them. Buttons
 * the device holds across the switch are ignored until released.
 */
BMX_ADDR(0x004083e0)
void contrlerSetMode(uint32_t mode)
{
    if (mode == gContrlerMode) {
        return;
    }
    if (gContrlerMode == 0) {
        for (int i = 0; i < kPadCount; i++) {
            ContrlerPad *const pad = &gContrlerPads[i];

            pad->saved = pad->state.buttons;
            if ((pad->flags & 1) == 0 && (pad->device->status & 1) != 0) {
                pad->stuck = pad->device->held;
            } else {
                pad->stuck = 0;
            }
        }
    }
    if (mode == 0) {
        for (int i = 0; i < kPadCount; i++) {
            ContrlerPad *const pad = &gContrlerPads[i];
            const uint32_t saved = pad->saved;

            pad->state.buttons = saved;
            if ((pad->flags & 1) == 0 && (pad->device->status & 1) != 0) {
                pad->stuck = pad->device->held & ~saved;
            } else {
                pad->stuck = 0;
            }
        }
    }
    gContrlerMode = (uint8_t)mode;
}

BMX_ADDR(0x004084b0)
int contrlerAction(uint32_t config, const void *state, uint32_t action, uint32_t status)
{
    if (config > 2) {
        zdebugFail("controller config out of range", contrlerFile, 0x2e2, 0);
    }
    if (action > 0x14) {
        zdebugFail("action out of range", contrlerFile, 0x2e3, 0);
    }
    if (status > 2) {
        zdebugFail("actionStatus out of range", contrlerFile, 0x2e4, 0);
    }
    const ContrlerBinding *const binding =
        &gContrlerBindings[0][0][0] + (config * 0x15 + action) * 3 + status;
    return contrlerTest(state, binding->mask, binding->mode);
}

BMX_ADDR(0x00408590)
int contrlerPressed(int player, uint32_t mask, uint32_t status)
{
    if (status > 7) {
        zdebugFail("status out of range", contrlerFile, 0x2f8, 0);
    }
    return contrlerTest(&(&gContrlerPads[0])[gProfiles[player].controller].state, mask, status);
}

/*
 * Axes of stick 0 or 1 of a pad state in a static buffer; the d-pad overrides stick 0 with full
 * deflection (the first direction held wins per axis, up before down, left before right).
 */
BMX_ADDR(0x00408610)
const float *contrlerAxes(const void *state, uint32_t index)
{
    const ContrlerState *const s = state;

    if (index > 1) {
        zdebugFail("joystick number out of range", contrlerFile, 0x30b, 0);
    }
    /* The index is not checked after the assert (the original reads whatever follows the sticks). */
    const float *const sticks = (const float *)((const uint8_t *)s + offsetof(ContrlerState, axis));

    gContrlerAxes[0] = sticks[index * 2];
    gContrlerAxes[1] = sticks[index * 2 + 1];
    if (index == 0) {
        if ((s->buttons & 4) != 0) {
            gContrlerAxes[0] = -1.0f;
        } else if ((s->buttons & 8) != 0) {
            gContrlerAxes[0] = 1.0f;
        }
        if ((s->buttons & 1) != 0) {
            gContrlerAxes[1] = -1.0f;
            return gContrlerAxes;
        }
        if ((s->buttons & 2) != 0) {
            gContrlerAxes[1] = 1.0f;
        }
    }
    return gContrlerAxes;
}

/* Force feedback; empty in the PC build. */
BMX_ADDR(0x004086d0)
void contrlerRumble(int player, int a, int b) {}

BMX_ADDR(0x004086f0)
ContrlerPad *contrlerGet(uint32_t index)
{
    if (index > 1) {
        zdebugFail("controller number out of range", contrlerFile, 0x346, 0);
    }
    return &(&gContrlerPads[0])[index];
}

/*
 * Tests a pad state against button mask `mask`. The d-pad bits (1 up, 2 down, 4 left, 8 right) are
 * derived from the first stick: a direction is held while its axis is past 0.5, and "changed" when it
 * crossed that threshold since the previous poll (in either direction).
 */
BMX_ADDR(0x00408740)
int contrlerTest(const ContrlerState *state, uint32_t mask, uint32_t mode)
{
    uint32_t held = state->buttons;
    uint32_t changed = state->pressed;

    if (stickPast(state->axis[0])) {
        held |= stickDir(state->axis[0], 4, 8);
        if (!stickPast(state->prevAxis[0])) {
            changed |= stickDir(state->axis[0], 4, 8);
        }
    } else if (stickPast(state->prevAxis[0])) {
        changed |= stickDir(state->prevAxis[0], 4, 8);
    }
    if (stickPast(state->axis[1])) {
        held |= stickDir(state->axis[1], 1, 2);
        if (!stickPast(state->prevAxis[1])) {
            changed |= stickDir(state->axis[1], 1, 2);
        }
    } else if (stickPast(state->prevAxis[1])) {
        changed |= stickDir(state->prevAxis[1], 1, 2);
    }
    switch (mode) {
    case kContrlerNewPress:
        return (changed & held & mask) != 0;
    case kContrlerHeldChanged:
        return (held & mask) == mask && (mask & changed) != 0;
    case kContrlerAllHeld:
        return (held & mask) == mask;
    case kContrlerAnyHeld:
        return (held & mask) != 0;
    case kContrlerUpChanged:
        return (~held & mask) == mask && (mask & changed) != 0;
    case kContrlerNewRelease:
        return (~held & changed & mask) != 0;
    case kContrlerAlways:
        return 1;
    default:
        return 0;
    }
}
