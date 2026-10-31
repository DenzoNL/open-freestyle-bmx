/*
 * Internals of the DirectInput stand-in (input.c): the pure conversions from SDL's view of a pad and keyboard
 * to what DirectInput reports (unit-tested in tests/platform), and the BMX_INPUT_CHECK hooks (inputcheck.c).
 */
#ifndef BMX_PLATFORM_INPUT_H
#define BMX_PLATFORM_INPUT_H

#include <stdint.h>

#include "platform/diface.h"

/* ---- DirectInput's axis arithmetic ---- */

/* MulDiv: a * b / c with a 64-bit product, rounded half away from zero; -1 when c is 0. */
int32_t bmxDiMulDiv(int32_t a, int32_t b, int32_t c);

/* The per-axis properties a device keeps (DIPROP_RANGE, DIPROP_DEADZONE, DIPROP_SATURATION). */
typedef struct BmxDiAxisProps {
    int32_t min, max;
    uint32_t deadZone, saturation;
} BmxDiAxisProps;

/* The defaults of a fresh device: range 0..65535, no dead zone, saturation 10000. */
void bmxDiAxisDefaults(BmxDiAxisProps *props);

/*
 * The value DirectInput reports for a raw HID axis value in 0..logicalMax (the logical centre is
 * (logicalMax + 1) / 2: 128 for an 8-bit axis, fitted to DirectInput's values with a DualSense): the dead
 * zone and saturation are fractions (of 10000) of each half, the part between them is scaled linearly onto
 * the half of the range, with MulDiv's rounding.
 */
int32_t bmxDiCookAxis(int32_t raw, int32_t logicalMax, const BmxDiAxisProps *props);

/* ---- the pad as SDL sees it ---- */

/* How DirectInput numbers the device's buttons and axes (docs/PLATFORM.md P3). */
typedef enum BmxPadFamily {
    kBmxPadGeneric,     /* SDL_Joystick raw order (plain HID pads, like DirectInput) */
    kBmxPadXbox,        /* XInput-class pads under DirectInput */
    kBmxPadPlayStation, /* DualShock 4 / DualSense */
    kBmxPadDualSense,   /* as PlayStation, plus the mute button */
} BmxPadFamily;

/* SDL gamepad buttons used by the tables (SDL_GamepadButton values, so input.h needs no SDL header). */
enum { kBmxPadMaxAxes = 8, kBmxPadMaxButtons = 32 };

/* One read of the pad. For a gamepad family the SDL_Gamepad values, else the raw SDL_Joystick values. */
typedef struct BmxPadSnapshot {
    BmxPadFamily family;
    int16_t leftX, leftY, rightX, rightY; /* -32768..32767 */
    int16_t leftTrigger, rightTrigger;    /* 0..32767 */
    uint64_t gamepadButtons;              /* bit n = SDL_GamepadButton n pressed */
    /* kBmxPadGeneric: the raw joystick */
    int axisCount, buttonCount;
    int16_t axes[kBmxPadMaxAxes];
    uint8_t buttons[kBmxPadMaxButtons];
    uint8_t hat; /* SDL_HAT_* bits of hat 0 (generic) */
} BmxPadSnapshot;

/* The d-pad of a snapshot as SDL_HAT_* bits (UP 1, RIGHT 2, DOWN 4, LEFT 8). */
uint8_t bmxPadDpad(const BmxPadSnapshot *pad);

/* The 8-bit / 16-bit raw HID value DirectInput saw for an SDL stick value of this family. */
int32_t bmxPadRawStick(BmxPadFamily family, int16_t value, int32_t *logicalMax);

/* DIJOYSTATE as DirectInput fills it for this device (props: the device's per-axis properties). */
void bmxPadToJoyState(const BmxPadSnapshot *pad, const BmxDiAxisProps props[kDiAxes], BmxDiJoyState *out);

/* The d-pad steering option (docs/PLATFORM.md P7): a pressed d-pad direction sets lX / lY to the end of the
 * axis range, over the stick. Returns 1 when it changed the state. */
int bmxPadApplyDpad(uint8_t dpad, const BmxDiAxisProps props[kDiAxes], BmxDiJoyState *state);

/* The stick 8-way option (docs/PLATFORM.md P7-B): the left stick (raw SDL values) inside a radial dead zone
 * of kStick8WayDeadZone gives lX = lY = 0, outside it the angle snaps to the nearest of 8 directions (45
 * degree sectors centred on the axes and diagonals) and lX / lY are the axis ends (props min / max) or 0.
 * Returns 1 when it changed the state. */
enum { kStick8WayDeadZone = 3277 }; /* 10% of 32767 */
int bmxPadApplyStick8Way(const BmxPadSnapshot *pad, const BmxDiAxisProps props[kDiAxes],
                         BmxDiJoyState *state);

/* Forgets the pad and every device (tests). */
void bmxInputReset(void);

/* ---- keyboard ---- */

/* The DIK code (PC set-1 scan code, + 0x80 for extended keys) of an SDL_Scancode, 0 for none. */
uint8_t bmxDikFromScancode(int scancode);

/* ---- BMX_INPUT_CHECK (inputcheck.c): the same calls on the real DirectInput, compared ---- */

typedef struct BmxCheckDevice BmxCheckDevice;

/* 1 when BMX_INPUT_CHECK is set (and the run is not scripted). */
int bmxCheckEnabled(void);
/* Creates the real IDirectInput7 and finds its first joystick. */
void bmxCheckStart(void *instance, uint32_t version, const BmxGuid *iid);
/* The real device mirroring one of ours (the keyboard or the first joystick); NULL when there is none. */
BmxCheckDevice *bmxCheckOpen(int keyboard, const BmxGuid *iid);
void bmxCheckSetDataFormat(BmxCheckDevice *c, const BmxDiDataFormat *format, BmxHresult ours);
void bmxCheckSetCooperativeLevel(BmxCheckDevice *c, void *window, uint32_t flags, BmxHresult ours);
void bmxCheckSetProperty(BmxCheckDevice *c, uintptr_t property, const BmxDiPropHeader *header,
                         BmxHresult ours);
void bmxCheckAcquire(BmxCheckDevice *c, BmxHresult ours);
void bmxCheckUnacquire(BmxCheckDevice *c, BmxHresult ours);
void bmxCheckPoll(BmxCheckDevice *c, BmxHresult ours);
void bmxCheckDeviceInfo(BmxCheckDevice *c, const BmxDiDeviceInstance *ours, BmxHresult result);
/* ours: our result and state (state NULL on failure); pad: the snapshot it came from (joystick only). */
void bmxCheckKeyboardState(BmxCheckDevice *c, BmxHresult ours, const uint8_t *state);
void bmxCheckJoyState(BmxCheckDevice *c, BmxHresult ours, const BmxDiJoyState *state,
                      const BmxPadSnapshot *pad);
void bmxCheckRelease(BmxCheckDevice *c);
/* Logs the totals. */
void bmxCheckSummary(void);
/* HRESULT differences plus mismatches so far (tests); bmxCheckReset forgets everything (tests). */
uint64_t bmxCheckDifferences(void);
void bmxCheckReset(void);

#endif
