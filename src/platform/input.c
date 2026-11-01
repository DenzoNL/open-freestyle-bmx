/*
 * DirectInput on SDL3 (docs/PLATFORM.md 3.1, P3-P7, S5/S6): the stand-in for DINPUT.dll!DirectInputCreateEx
 * hands out an IDirectInput7A and IDirectInputDevice7A objects for the keyboard and the first pad, whose
 * GetDeviceState fills the 256-byte DIK state and DIJOYSTATE the way DirectInput does. pcinput.c is
 * unchanged.
 *
 * Vanilla: the first joystick SDL lists at start (no hot-plug); buttons numbered as DirectInput numbers them
 * for that pad family; lX / lY with the range the game sets (-127..127) and DirectInput's dead zone
 * arithmetic on the raw HID value; keys by physical position (DIK = scan code); foreground semantics (Acquire
 * fails and an acquired device reports DIERR_INPUTLOST while the window has no focus; "exclusive" only
 * shuts out other processes, so every device object of the game on the pad acquires and reads it).
 *
 * BMX_KEYS (src/native/keys.c) is a source of the same keyboard: the script's keys, focus ignored, no pad.
 * BMX_INPUT_CHECK=1 mirrors every call on the real DirectInput and compares (inputcheck.c).
 * Option `input.dpad_steering` (bmxnative.ini, P7): the d-pad also sets lX / lY.
 * Option `input.stick_8way` (P7-B): the left stick snaps to 8 directions (radial dead zone, full deflection).
 */
#include <SDL3/SDL.h>
#include <string.h>

#include "platform/input.h"
#include "platform/platform.h"
#include "platform/standins.h"

/* ---- DirectInput arithmetic ---- */

int32_t bmxDiMulDiv(int32_t a, int32_t b, int32_t c)
{
    const int64_t p = (int64_t)a * b;
    const int64_t ap = p < 0 ? -p : p, ac = c < 0 ? -(int64_t)c : c;
    int64_t q;

    if (c == 0) {
        return -1;
    }
    q = (ap + ac / 2) / ac;
    return (int32_t)(((p < 0) != (c < 0)) ? -q : q);
}

void bmxDiAxisDefaults(BmxDiAxisProps *props)
{
    props->min = 0;
    props->max = 65535;
    props->deadZone = 0;
    props->saturation = 10000;
}

int32_t bmxDiCookAxis(int32_t raw, int32_t logicalMax, const BmxDiAxisProps *props)
{
    const int32_t center = (logicalMax + 1) >> 1; /* 128 for 0..255 (measured; Wine uses 127) */
    const int32_t sum = props->min + props->max;
    const int32_t phyCenter =
        props->min == 0 ? props->max >> 1 : (sum >= 0 ? (sum + 1) / 2 : -((1 - sum) / 2));
    const int32_t t = raw - center;
    const int32_t dz = (int32_t)props->deadZone, sat = (int32_t)props->saturation;

    if (t <= 0) {
        const int32_t lo = bmxDiMulDiv(-center, sat, 10000), hi = bmxDiMulDiv(-center, dz, 10000);

        if (t <= lo) {
            return props->min;
        }
        if (t >= hi) {
            return phyCenter;
        }
        return props->min + bmxDiMulDiv(t - lo, phyCenter - props->min, hi - lo);
    }
    {
        const int32_t lo = bmxDiMulDiv(logicalMax - center, dz, 10000);
        const int32_t hi = bmxDiMulDiv(logicalMax - center, sat, 10000);

        if (t <= lo) {
            return phyCenter;
        }
        if (t >= hi) {
            return props->max;
        }
        return phyCenter + bmxDiMulDiv(t - lo, props->max - phyCenter, hi - lo);
    }
}

/* ---- pad snapshot -> DIJOYSTATE ---- */

#define BIT(b) (1ull << (b))

/* DirectInput's button index of each SDL gamepad button, per family (P3-A; L2/R2 come from the triggers). */
typedef struct ButtonIndex {
    uint8_t sdl, di;
} ButtonIndex;

static const ButtonIndex kPlayStationButtons[] = {
    {SDL_GAMEPAD_BUTTON_WEST, 0},          {SDL_GAMEPAD_BUTTON_SOUTH, 1},
    {SDL_GAMEPAD_BUTTON_EAST, 2},          {SDL_GAMEPAD_BUTTON_NORTH, 3},
    {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 4}, {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 5},
    {SDL_GAMEPAD_BUTTON_BACK, 8},          {SDL_GAMEPAD_BUTTON_START, 9},
    {SDL_GAMEPAD_BUTTON_LEFT_STICK, 10},   {SDL_GAMEPAD_BUTTON_RIGHT_STICK, 11},
    {SDL_GAMEPAD_BUTTON_GUIDE, 12},        {SDL_GAMEPAD_BUTTON_TOUCHPAD, 13},
    {SDL_GAMEPAD_BUTTON_MISC1, 14}, /* DualSense mute; a DualShock 4 has no MISC1 */
};

static const ButtonIndex kXboxButtons[] = {
    {SDL_GAMEPAD_BUTTON_SOUTH, 0},         {SDL_GAMEPAD_BUTTON_EAST, 1},
    {SDL_GAMEPAD_BUTTON_WEST, 2},          {SDL_GAMEPAD_BUTTON_NORTH, 3},
    {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 4}, {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 5},
    {SDL_GAMEPAD_BUTTON_BACK, 6},          {SDL_GAMEPAD_BUTTON_START, 7},
    {SDL_GAMEPAD_BUTTON_LEFT_STICK, 8},    {SDL_GAMEPAD_BUTTON_RIGHT_STICK, 9},
};

uint8_t bmxPadDpad(const BmxPadSnapshot *pad)
{
    if (pad->family == kBmxPadGeneric) {
        return pad->hat;
    }
    return (uint8_t)(((pad->gamepadButtons & BIT(SDL_GAMEPAD_BUTTON_DPAD_UP)) ? SDL_HAT_UP : 0) |
                     ((pad->gamepadButtons & BIT(SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) ? SDL_HAT_RIGHT : 0) |
                     ((pad->gamepadButtons & BIT(SDL_GAMEPAD_BUTTON_DPAD_DOWN)) ? SDL_HAT_DOWN : 0) |
                     ((pad->gamepadButtons & BIT(SDL_GAMEPAD_BUTTON_DPAD_LEFT)) ? SDL_HAT_LEFT : 0));
}

/* The POV value of SDL_HAT_* bits: hundredths of a degree clockwise from up, 0xffffffff when centred. */
static uint32_t povFromHat(uint8_t hat)
{
    static const uint32_t kPov[16] = {
        0xffffffffu, 0,     9000,        4500, 18000, 0xffffffffu, 13500, 9000,
        27000,       31500, 0xffffffffu, 0,    22500, 27000,       18000, 0xffffffffu,
    };

    return kPov[hat & 15];
}

int32_t bmxPadRawStick(BmxPadFamily family, int16_t value, int32_t *logicalMax)
{
    if (family == kBmxPadPlayStation || family == kBmxPadDualSense) {
        /* SDL's HIDAPI driver scales the 8-bit report value r to r * 257 - 32768. */
        *logicalMax = 255;
        return ((int32_t)value + 32768 + 128) / 257;
    }
    *logicalMax = 65535;
    return (int32_t)value + 32768;
}

/* A trigger (0..32767) as the 8-bit value the HID report had. */
static int32_t rawTrigger(int16_t value)
{
    return ((int32_t)(value < 0 ? 0 : value) * 255 + 16383) / 32767;
}

static int32_t cookStick(BmxPadFamily family, int16_t value, const BmxDiAxisProps *props)
{
    int32_t logicalMax;
    const int32_t raw = bmxPadRawStick(family, value, &logicalMax);

    return bmxDiCookAxis(raw, logicalMax, props);
}

void bmxPadToJoyState(const BmxPadSnapshot *pad, const BmxDiAxisProps props[kDiAxes], BmxDiJoyState *out)
{
    memset(out, 0, sizeof *out);
    out->pov[0] = povFromHat(bmxPadDpad(pad));
    out->pov[1] = out->pov[2] = out->pov[3] = 0xffffffffu;
    switch (pad->family) {
    case kBmxPadPlayStation:
    case kBmxPadDualSense: {
        /* HID usages of the Sony pads: X/Y left stick, Z/Rz right stick, Rx/Ry the triggers (all 8-bit). */
        const size_t n = sizeof kPlayStationButtons / sizeof kPlayStationButtons[0] -
                         (pad->family == kBmxPadPlayStation ? 1 : 0);

        out->axis[kDiAxisX] = cookStick(pad->family, pad->leftX, &props[kDiAxisX]);
        out->axis[kDiAxisY] = cookStick(pad->family, pad->leftY, &props[kDiAxisY]);
        out->axis[kDiAxisZ] = cookStick(pad->family, pad->rightX, &props[kDiAxisZ]);
        out->axis[kDiAxisRz] = cookStick(pad->family, pad->rightY, &props[kDiAxisRz]);
        out->axis[kDiAxisRx] = bmxDiCookAxis(rawTrigger(pad->leftTrigger), 255, &props[kDiAxisRx]);
        out->axis[kDiAxisRy] = bmxDiCookAxis(rawTrigger(pad->rightTrigger), 255, &props[kDiAxisRy]);
        for (size_t i = 0; i < n; i++) {
            out->buttons[kPlayStationButtons[i].di] =
                (pad->gamepadButtons & BIT(kPlayStationButtons[i].sdl)) ? 0x80 : 0;
        }
        /* L2 / R2 also report a button (the pad's digital bit; assumed set as soon as the trigger moves). */
        out->buttons[6] = pad->leftTrigger > 0 ? 0x80 : 0;
        out->buttons[7] = pad->rightTrigger > 0 ? 0x80 : 0;
        break;
    }
    case kBmxPadXbox: {
        /* XInput pads under DirectInput: X/Y left stick, Rx/Ry right stick, both triggers on Z (LT up, RT
         * down, centred at rest), 10 buttons, no guide button. */
        const int32_t z = 32768 + ((int32_t)pad->leftTrigger - (int32_t)pad->rightTrigger);

        out->axis[kDiAxisX] = cookStick(pad->family, pad->leftX, &props[kDiAxisX]);
        out->axis[kDiAxisY] = cookStick(pad->family, pad->leftY, &props[kDiAxisY]);
        out->axis[kDiAxisRx] = cookStick(pad->family, pad->rightX, &props[kDiAxisRx]);
        out->axis[kDiAxisRy] = cookStick(pad->family, pad->rightY, &props[kDiAxisRy]);
        out->axis[kDiAxisZ] = bmxDiCookAxis(z > 65535 ? 65535 : z, 65535, &props[kDiAxisZ]);
        for (size_t i = 0; i < sizeof kXboxButtons / sizeof kXboxButtons[0]; i++) {
            out->buttons[kXboxButtons[i].di] = (pad->gamepadButtons & BIT(kXboxButtons[i].sdl)) ? 0x80 : 0;
        }
        break;
    }
    case kBmxPadGeneric:
        /* SDL's raw order: axes into X, Y, Z, Rx, Ry, Rz in turn, buttons by index. */
        for (int i = 0; i < pad->axisCount && i < kDiAxisRz + 1; i++) {
            out->axis[i] = cookStick(pad->family, pad->axes[i], &props[i]);
        }
        for (int i = 0; i < pad->buttonCount && i < kBmxPadMaxButtons; i++) {
            out->buttons[i] = pad->buttons[i] ? 0x80 : 0;
        }
        break;
    }
}

int bmxPadApplyDpad(uint8_t dpad, const BmxDiAxisProps props[kDiAxes], BmxDiJoyState *state)
{
    const BmxDiJoyState before = *state;

    if (dpad & SDL_HAT_LEFT) {
        state->axis[kDiAxisX] = props[kDiAxisX].min;
    } else if (dpad & SDL_HAT_RIGHT) {
        state->axis[kDiAxisX] = props[kDiAxisX].max;
    }
    if (dpad & SDL_HAT_UP) {
        state->axis[kDiAxisY] = props[kDiAxisY].min;
    } else if (dpad & SDL_HAT_DOWN) {
        state->axis[kDiAxisY] = props[kDiAxisY].max;
    }
    return memcmp(&before, state, sizeof before) != 0;
}

int bmxPadApplyStick8Way(const BmxPadSnapshot *pad, const BmxDiAxisProps props[kDiAxes], BmxDiJoyState *state)
{
    const BmxDiJoyState before = *state;
    int32_t x, y;

    if (pad->family == kBmxPadGeneric) {
        if (pad->axisCount < 2) {
            return 0;
        }
        x = pad->axes[0];
        y = pad->axes[1];
    } else {
        x = pad->leftX;
        y = pad->leftY;
    }
    /* The radial dead zone on the raw SDL vector (-32768..32767): 10% of the half range as a radius. */
    if ((int64_t)x * x + (int64_t)y * y <= (int64_t)kStick8WayDeadZone * kStick8WayDeadZone) {
        state->axis[kDiAxisX] = 0;
        state->axis[kDiAxisY] = 0;
    } else {
        /* An axis is active when its share of the vector is beyond tan(22.5 deg) = 0.4142 of the other. */
        const int64_t ax = x < 0 ? -(int64_t)x : x, ay = y < 0 ? -(int64_t)y : y;
        const int32_t sx = x < 0 ? props[kDiAxisX].min : props[kDiAxisX].max;
        const int32_t sy = y < 0 ? props[kDiAxisY].min : props[kDiAxisY].max;

        state->axis[kDiAxisX] = ax * 10000 > ay * 4142 ? sx : 0;
        state->axis[kDiAxisY] = ay * 10000 > ax * 4142 ? sy : 0;
    }
    return memcmp(&before, state, sizeof before) != 0;
}

/* ---- keyboard: SDL_Scancode (USB HID usage) -> DIK (set-1 scan code, + 0x80 for E0 keys) ---- */

static const uint8_t kDik[SDL_SCANCODE_COUNT] = {
    [SDL_SCANCODE_ESCAPE] = 0x01,
    [SDL_SCANCODE_1] = 0x02,
    [SDL_SCANCODE_2] = 0x03,
    [SDL_SCANCODE_3] = 0x04,
    [SDL_SCANCODE_4] = 0x05,
    [SDL_SCANCODE_5] = 0x06,
    [SDL_SCANCODE_6] = 0x07,
    [SDL_SCANCODE_7] = 0x08,
    [SDL_SCANCODE_8] = 0x09,
    [SDL_SCANCODE_9] = 0x0a,
    [SDL_SCANCODE_0] = 0x0b,
    [SDL_SCANCODE_MINUS] = 0x0c,
    [SDL_SCANCODE_EQUALS] = 0x0d,
    [SDL_SCANCODE_BACKSPACE] = 0x0e,
    [SDL_SCANCODE_TAB] = 0x0f,
    [SDL_SCANCODE_Q] = 0x10,
    [SDL_SCANCODE_W] = 0x11,
    [SDL_SCANCODE_E] = 0x12,
    [SDL_SCANCODE_R] = 0x13,
    [SDL_SCANCODE_T] = 0x14,
    [SDL_SCANCODE_Y] = 0x15,
    [SDL_SCANCODE_U] = 0x16,
    [SDL_SCANCODE_I] = 0x17,
    [SDL_SCANCODE_O] = 0x18,
    [SDL_SCANCODE_P] = 0x19,
    [SDL_SCANCODE_LEFTBRACKET] = 0x1a,
    [SDL_SCANCODE_RIGHTBRACKET] = 0x1b,
    [SDL_SCANCODE_RETURN] = 0x1c,
    [SDL_SCANCODE_LCTRL] = 0x1d,
    [SDL_SCANCODE_A] = 0x1e,
    [SDL_SCANCODE_S] = 0x1f,
    [SDL_SCANCODE_D] = 0x20,
    [SDL_SCANCODE_F] = 0x21,
    [SDL_SCANCODE_G] = 0x22,
    [SDL_SCANCODE_H] = 0x23,
    [SDL_SCANCODE_J] = 0x24,
    [SDL_SCANCODE_K] = 0x25,
    [SDL_SCANCODE_L] = 0x26,
    [SDL_SCANCODE_SEMICOLON] = 0x27,
    [SDL_SCANCODE_APOSTROPHE] = 0x28,
    [SDL_SCANCODE_GRAVE] = 0x29,
    [SDL_SCANCODE_LSHIFT] = 0x2a,
    [SDL_SCANCODE_BACKSLASH] = 0x2b,
    [SDL_SCANCODE_NONUSHASH] = 0x2b, /* the ISO key left of Enter has the same scan code */
    [SDL_SCANCODE_Z] = 0x2c,
    [SDL_SCANCODE_X] = 0x2d,
    [SDL_SCANCODE_C] = 0x2e,
    [SDL_SCANCODE_V] = 0x2f,
    [SDL_SCANCODE_B] = 0x30,
    [SDL_SCANCODE_N] = 0x31,
    [SDL_SCANCODE_M] = 0x32,
    [SDL_SCANCODE_COMMA] = 0x33,
    [SDL_SCANCODE_PERIOD] = 0x34,
    [SDL_SCANCODE_SLASH] = 0x35,
    [SDL_SCANCODE_RSHIFT] = 0x36,
    [SDL_SCANCODE_KP_MULTIPLY] = 0x37,
    [SDL_SCANCODE_LALT] = 0x38,
    [SDL_SCANCODE_SPACE] = 0x39,
    [SDL_SCANCODE_CAPSLOCK] = 0x3a,
    [SDL_SCANCODE_F1] = 0x3b,
    [SDL_SCANCODE_F2] = 0x3c,
    [SDL_SCANCODE_F3] = 0x3d,
    [SDL_SCANCODE_F4] = 0x3e,
    [SDL_SCANCODE_F5] = 0x3f,
    [SDL_SCANCODE_F6] = 0x40,
    [SDL_SCANCODE_F7] = 0x41,
    [SDL_SCANCODE_F8] = 0x42,
    [SDL_SCANCODE_F9] = 0x43,
    [SDL_SCANCODE_F10] = 0x44,
    [SDL_SCANCODE_NUMLOCKCLEAR] = 0x45,
    [SDL_SCANCODE_SCROLLLOCK] = 0x46,
    [SDL_SCANCODE_KP_7] = 0x47,
    [SDL_SCANCODE_KP_8] = 0x48,
    [SDL_SCANCODE_KP_9] = 0x49,
    [SDL_SCANCODE_KP_MINUS] = 0x4a,
    [SDL_SCANCODE_KP_4] = 0x4b,
    [SDL_SCANCODE_KP_5] = 0x4c,
    [SDL_SCANCODE_KP_6] = 0x4d,
    [SDL_SCANCODE_KP_PLUS] = 0x4e,
    [SDL_SCANCODE_KP_1] = 0x4f,
    [SDL_SCANCODE_KP_2] = 0x50,
    [SDL_SCANCODE_KP_3] = 0x51,
    [SDL_SCANCODE_KP_0] = 0x52,
    [SDL_SCANCODE_KP_PERIOD] = 0x53,
    [SDL_SCANCODE_NONUSBACKSLASH] = 0x56,
    [SDL_SCANCODE_F11] = 0x57,
    [SDL_SCANCODE_F12] = 0x58,
    [SDL_SCANCODE_F13] = 0x64,
    [SDL_SCANCODE_F14] = 0x65,
    [SDL_SCANCODE_F15] = 0x66,
    [SDL_SCANCODE_INTERNATIONAL2] = 0x70, /* kana */
    [SDL_SCANCODE_INTERNATIONAL1] = 0x73, /* ro / ABNT_C1 */
    [SDL_SCANCODE_INTERNATIONAL4] = 0x79, /* convert */
    [SDL_SCANCODE_INTERNATIONAL5] = 0x7b, /* no convert */
    [SDL_SCANCODE_INTERNATIONAL3] = 0x7d, /* yen */
    [SDL_SCANCODE_KP_EQUALS] = 0x8d,
    [SDL_SCANCODE_MEDIA_PREVIOUS_TRACK] = 0x90,
    [SDL_SCANCODE_MEDIA_NEXT_TRACK] = 0x99,
    [SDL_SCANCODE_KP_ENTER] = 0x9c,
    [SDL_SCANCODE_RCTRL] = 0x9d,
    [SDL_SCANCODE_MUTE] = 0xa0,
    [SDL_SCANCODE_MEDIA_PLAY_PAUSE] = 0xa2,
    [SDL_SCANCODE_MEDIA_STOP] = 0xa4,
    [SDL_SCANCODE_VOLUMEDOWN] = 0xae,
    [SDL_SCANCODE_VOLUMEUP] = 0xb0,
    [SDL_SCANCODE_KP_COMMA] = 0xb3,
    [SDL_SCANCODE_KP_DIVIDE] = 0xb5,
    [SDL_SCANCODE_PRINTSCREEN] = 0xb7,
    [SDL_SCANCODE_RALT] = 0xb8,
    [SDL_SCANCODE_PAUSE] = 0xc5,
    [SDL_SCANCODE_HOME] = 0xc7,
    [SDL_SCANCODE_UP] = 0xc8,
    [SDL_SCANCODE_PAGEUP] = 0xc9,
    [SDL_SCANCODE_LEFT] = 0xcb,
    [SDL_SCANCODE_RIGHT] = 0xcd,
    [SDL_SCANCODE_END] = 0xcf,
    [SDL_SCANCODE_DOWN] = 0xd0,
    [SDL_SCANCODE_PAGEDOWN] = 0xd1,
    [SDL_SCANCODE_INSERT] = 0xd2,
    [SDL_SCANCODE_DELETE] = 0xd3,
    [SDL_SCANCODE_LGUI] = 0xdb,
    [SDL_SCANCODE_RGUI] = 0xdc,
    [SDL_SCANCODE_APPLICATION] = 0xdd,
    [SDL_SCANCODE_POWER] = 0xde,
    [SDL_SCANCODE_SLEEP] = 0xdf,
};

uint8_t bmxDikFromScancode(int scancode)
{
    return scancode > 0 && scancode < SDL_SCANCODE_COUNT ? kDik[scancode] : 0;
}

/* ---- keys held at focus gain ---- */

static uint8_t gSeed[256]; /* DIK-indexed: 0x80 for a key held at focus gain that SDL has not seen */
static BmxKeyProbe gProbe;

#if defined(_WIN32)
typedef int16_t(BMX_WINAPI *GetAsyncKeyStateFn)(int vk);
typedef uint32_t(BMX_WINAPI *MapVirtualKeyFn)(uint32_t code, uint32_t mapType);

/*
 * Windows-only fallback: the physical state of the key through USER32's GetAsyncKeyState (SDL does not
 * resync its keyboard state when the window gains the focus). The DIK is a set-1 scan code (+ 0x80 for the
 * extended keys): MapVirtualKeyW(MAPVK_VSC_TO_VK_EX) gives the virtual key, left / right modifiers included.
 * USER32 is in the process anyway (SDL3 imports it); it is looked up at run time like the other dev aids.
 */
static int osKeyDown(uint8_t dik)
{
    static GetAsyncKeyStateFn getState;
    static MapVirtualKeyFn mapKey;
    static int looked;
    uint32_t vk;

    if (!looked) {
        looked = 1;
        getState = (GetAsyncKeyStateFn)bmxRealProc("USER32.dll", "GetAsyncKeyState");
        mapKey = (MapVirtualKeyFn)bmxRealProc("USER32.dll", "MapVirtualKeyW");
    }
    if (getState == NULL || mapKey == NULL) {
        return 0;
    }
    vk = mapKey((uint32_t)((dik & 0x80) != 0 ? 0xe000 : 0) | (dik & 0x7fu), 3 /* MAPVK_VSC_TO_VK_EX */);
    return vk != 0 && (getState((int)vk) & (int16_t)0x8000) != 0;
}
#else
static int osKeyDown(uint8_t dik)
{
    (void)dik;
    return 0;
}
#endif

static int probeKey(uint8_t dik)
{
    return gProbe != NULL ? gProbe(dik) : osKeyDown(dik);
}

void bmxInputSetKeyProbe(BmxKeyProbe probe)
{
    gProbe = probe;
}

void bmxInputFocusGained(void)
{
    memset(gSeed, 0, sizeof gSeed);
    for (int s = 1; s < SDL_SCANCODE_COUNT; s++) {
        const uint8_t dik = kDik[s];

        if (dik != 0 && probeKey(dik)) {
            gSeed[dik] = 0x80;
        }
    }
}

void bmxInputFocusLost(void)
{
    memset(gSeed, 0, sizeof gSeed);
}

void bmxKeyboardApplySeed(uint8_t state[256])
{
    for (int k = 1; k < 256; k++) {
        if (gSeed[k] == 0) {
            continue;
        }
        if (state[k] != 0 || !probeKey((uint8_t)k)) {
            gSeed[k] = 0; /* SDL has the key now, or it was released */
        } else {
            state[k] = 0x80;
        }
    }
}

/* ---- the SDL side: the pad, focus ---- */

static struct {
    SDL_Gamepad *gamepad;   /* for a gamepad family */
    SDL_Joystick *joystick; /* always (the gamepad's) */
    SDL_GUID guid;          /* to find it again after an unplug */
    BmxPadFamily family;
    char name[128];
    uint16_t vendor, product;
    int present;
} gPad;

static int gScripted, gDpadSteering, gStick8Way, gStarted;

static int focused(void)
{
    return gScripted || (SDL_WasInit(SDL_INIT_VIDEO) && SDL_GetKeyboardFocus() != NULL);
}

static void padClose(void)
{
    if (gPad.gamepad != NULL) {
        SDL_CloseGamepad(gPad.gamepad);
    } else if (gPad.joystick != NULL) {
        SDL_CloseJoystick(gPad.joystick);
    }
    gPad.gamepad = NULL;
    gPad.joystick = NULL;
}

/* Opens joystick `id`: as a gamepad when SDL knows it (button tables per family), else raw. */
static int padOpen(SDL_JoystickID id)
{
    gPad.gamepad = SDL_IsGamepad(id) ? SDL_OpenGamepad(id) : NULL;
    if (gPad.gamepad != NULL) {
        gPad.joystick = SDL_GetGamepadJoystick(gPad.gamepad);
        switch (SDL_GetGamepadType(gPad.gamepad)) {
        case SDL_GAMEPAD_TYPE_PS4:
            gPad.family = kBmxPadPlayStation;
            break;
        case SDL_GAMEPAD_TYPE_PS5:
            gPad.family = kBmxPadDualSense;
            break;
        case SDL_GAMEPAD_TYPE_XBOX360:
        case SDL_GAMEPAD_TYPE_XBOXONE:
            gPad.family = kBmxPadXbox;
            break;
        default:
            gPad.family = kBmxPadGeneric;
            break;
        }
    } else {
        gPad.joystick = SDL_OpenJoystick(id);
        gPad.family = kBmxPadGeneric;
    }
    return gPad.joystick != NULL;
}

/* After an unplug: the same pad (SDL GUID: bus, VID, PID) when it is back, as DirectInput re-acquires it. */
static int padConnected(void)
{
    int count = 0;
    SDL_JoystickID *ids;

    if (gPad.joystick != NULL && SDL_JoystickConnected(gPad.joystick)) {
        return 1;
    }
    if (!gPad.present) {
        return 0;
    }
    padClose();
    ids = SDL_GetJoysticks(&count);
    for (int i = 0; ids != NULL && i < count; i++) {
        if (memcmp(SDL_GetJoystickGUIDForID(ids[i]).data, gPad.guid.data, sizeof gPad.guid.data) == 0) {
            if (padOpen(ids[i])) {
                bmxLog("input: pad reconnected\n");
            }
            break;
        }
    }
    SDL_free(ids);
    return gPad.joystick != NULL;
}

static const char *familyName(BmxPadFamily family)
{
    static const char *const kNames[] = {"generic (SDL order)", "Xbox", "PlayStation 4", "DualSense"};

    return kNames[family];
}

/* The first joystick present now (P5-A: no hot-plug; DIEDFL_ATTACHEDONLY). */
static void padStart(void)
{
    int count = 0;
    SDL_JoystickID *ids;

    SDL_SetHint(SDL_HINT_JOYSTICK_ENHANCED_REPORTS, "0"); /* would switch a Bluetooth DualSense away from
                                                              the report DirectInput reads, until power off */
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
        bmxLog("input: SDL_InitSubSystem(GAMEPAD) failed: %s\n", SDL_GetError());
        return;
    }
    /* Nobody pumps SDL's events (the game's own message loop runs): the pad is updated by Poll. */
    SDL_SetJoystickEventsEnabled(false);
    SDL_SetGamepadEventsEnabled(false);
    SDL_UpdateJoysticks();
    ids = SDL_GetJoysticks(&count);
    if (ids != NULL && count > 0 && padOpen(ids[0])) {
        gPad.present = 1;
        gPad.guid = SDL_GetJoystickGUID(gPad.joystick);
        gPad.vendor = SDL_GetJoystickVendor(gPad.joystick);
        gPad.product = SDL_GetJoystickProduct(gPad.joystick);
        SDL_snprintf(gPad.name, sizeof gPad.name, "%s",
                     SDL_GetJoystickName(gPad.joystick) ? SDL_GetJoystickName(gPad.joystick) : "pad");
        bmxLog("input: pad \"%s\" (VID %04x PID %04x, %d joysticks), buttons numbered as %s\n", gPad.name,
               gPad.vendor, gPad.product, count, familyName(gPad.family));
    } else {
        bmxLog("input: no pad\n");
    }
    SDL_free(ids);
}

static void padRead(BmxPadSnapshot *s)
{
    memset(s, 0, sizeof *s);
    s->family = gPad.family;
    if (gPad.joystick == NULL) {
        return;
    }
    if (gPad.family != kBmxPadGeneric) {
        SDL_Gamepad *const g = gPad.gamepad;

        s->leftX = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTX);
        s->leftY = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTY);
        s->rightX = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_RIGHTX);
        s->rightY = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_RIGHTY);
        s->leftTrigger = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        s->rightTrigger = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
        for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
            if (SDL_GetGamepadButton(g, (SDL_GamepadButton)b)) {
                s->gamepadButtons |= BIT(b);
            }
        }
        return;
    }
    s->axisCount = SDL_GetNumJoystickAxes(gPad.joystick);
    s->buttonCount = SDL_GetNumJoystickButtons(gPad.joystick);
    for (int i = 0; i < s->axisCount && i < kBmxPadMaxAxes; i++) {
        s->axes[i] = SDL_GetJoystickAxis(gPad.joystick, i);
    }
    for (int i = 0; i < s->buttonCount && i < kBmxPadMaxButtons; i++) {
        s->buttons[i] = SDL_GetJoystickButton(gPad.joystick, i) ? 1 : 0;
    }
    s->hat = SDL_GetNumJoystickHats(gPad.joystick) > 0 ? SDL_GetJoystickHat(gPad.joystick, 0) : 0;
}

/* ---- the device objects ---- */

enum {
    kMaxDevices = 8,
    kDisclExclusive = 1,
    kDisclNonExclusive = 2,
    kDisclForeground = 4,
    kDisclBackground = 8
};

typedef struct Device {
    const BmxDiDeviceVtbl *vtbl; /* first: the object the game holds */
    int used, refs, keyboard, acquired;
    uint32_t dataSize, coopFlags; /* dataSize 0: no data format yet */
    BmxDiAxisProps props[kDiAxes];
    BmxCheckDevice *check;
} Device;

static Device gDevices[kMaxDevices];

/* DIERR_INPUTLOST once, when an acquired device lost the focus (or its pad): DirectInput unacquires it. */
static int lost(Device *d)
{
    if (d->acquired &&
        (((d->coopFlags & kDisclForeground) && !focused()) || (!d->keyboard && !padConnected()))) {
        d->acquired = 0;
        return 1;
    }
    return 0;
}

static BmxHresult BMX_WINAPI devQueryInterface(BmxDiDevice *self, const BmxGuid *iid, void **out)
{
    (void)self;
    (void)iid;
    *out = NULL;
    return kDiErrNoInterface;
}

static uint32_t BMX_WINAPI devAddRef(BmxDiDevice *self)
{
    return (uint32_t)++((Device *)self)->refs;
}

static uint32_t BMX_WINAPI devRelease(BmxDiDevice *self)
{
    Device *const d = (Device *)self;

    if (--d->refs > 0) {
        return (uint32_t)d->refs;
    }
    bmxCheckRelease(d->check);
    memset(d, 0, sizeof *d);
    return 0;
}

static BmxHresult setProperty(Device *d, uintptr_t property, const BmxDiPropHeader *h)
{
    int first = 0, last = kDiAxes - 1;

    if (h == NULL || h->headerSize != sizeof(BmxDiPropHeader)) {
        return kDiErrInvalidParam;
    }
    if (property != kDiPropRange && property != kDiPropDeadZone && property != kDiPropSaturation) {
        return kDiErrUnsupported;
    }
    if (d->keyboard) {
        return kDiErrNotFound; /* the keyboard has no axes */
    }
    if (property == kDiPropRange ? h->size != sizeof(BmxDiPropRange) : h->size != sizeof(BmxDiPropDword)) {
        return kDiErrInvalidParam;
    }
    if (h->how == kDiphByOffset) {
        if (h->obj % 4 != 0 || h->obj / 4 >= kDiAxes) {
            return kDiErrNotFound;
        }
        first = last = (int)(h->obj / 4);
    } else if (h->how != kDiphDevice || h->obj != 0) {
        return kDiErrInvalidParam;
    }
    if (property == kDiPropRange) {
        const BmxDiPropRange *r = (const BmxDiPropRange *)h;

        if (r->min >= r->max) {
            return kDiErrInvalidParam;
        }
        for (int i = first; i <= last; i++) {
            d->props[i].min = r->min;
            d->props[i].max = r->max;
        }
    } else {
        const uint32_t v = ((const BmxDiPropDword *)h)->data;

        if (v > 10000) {
            return kDiErrInvalidParam;
        }
        for (int i = first; i <= last; i++) {
            if (property == kDiPropDeadZone) {
                d->props[i].deadZone = v;
            } else {
                d->props[i].saturation = v;
            }
        }
    }
    return kDiOk;
}

static BmxHresult BMX_WINAPI devSetProperty(BmxDiDevice *self, uintptr_t property, const BmxDiPropHeader *h)
{
    Device *const d = (Device *)self;
    const BmxHresult r = setProperty(d, property, h);

    bmxCheckSetProperty(d->check, property, h, r);
    return r;
}

static BmxHresult acquire(Device *d)
{
    if (d->dataSize == 0) {
        return kDiErrInvalidParam;
    }
    if (d->acquired) {
        return kDiNoEffect;
    }
    if ((d->coopFlags & kDisclForeground) && !focused()) {
        return kDiErrOtherAppHasPrio;
    }
    /* Exclusive access only shuts out other processes: a second exclusive object on the same pad in this
     * process acquires it too (measured, docs/PLATFORM.md 4b; the game's in-level pad is such an object). */
    if (!d->keyboard && !padConnected()) {
        return kDiErrInputLost;
    }
    d->acquired = 1;
    return kDiOk;
}

static BmxHresult BMX_WINAPI devAcquire(BmxDiDevice *self)
{
    Device *const d = (Device *)self;
    const BmxHresult r = acquire(d);

    bmxCheckAcquire(d->check, r);
    return r;
}

static BmxHresult BMX_WINAPI devUnacquire(BmxDiDevice *self)
{
    Device *const d = (Device *)self;
    BmxHresult r = kDiNoEffect;

    if (d->acquired) {
        d->acquired = 0;
        r = kDiOk;
    }
    bmxCheckUnacquire(d->check, r);
    return r;
}

static BmxHresult BMX_WINAPI devPoll(BmxDiDevice *self)
{
    Device *const d = (Device *)self;
    BmxHresult r;

    if (!d->keyboard && !gScripted) {
        SDL_UpdateJoysticks();
    }
    if (!d->acquired) {
        r = kDiErrNotAcquired;
    } else if (lost(d)) {
        r = kDiErrInputLost;
    } else {
        r = kDiNoEffect; /* nothing to poll: HID devices report by themselves */
    }
    bmxCheckPoll(d->check, r);
    return r;
}

static BmxHresult keyboardState(Device *d, uint32_t size, uint8_t *state)
{
    if (!d->acquired) {
        return kDiErrNotAcquired;
    }
    if (lost(d)) {
        return kDiErrInputLost;
    }
    if (size != d->dataSize || size != 256) {
        return kDiErrInvalidParam;
    }
    memset(state, 0, size);
    if (gScripted) {
        bmxKeysFillState(state, size);
    } else {
        int n = 0;
        const bool *keys = SDL_GetKeyboardState(&n);

        for (int s = 0; s < n; s++) {
            if (keys[s] && bmxDikFromScancode(s) != 0) {
                state[bmxDikFromScancode(s)] = 0x80;
            }
        }
        bmxKeyboardApplySeed(state);
    }
    return kDiOk;
}

static BmxHresult joyState(Device *d, uint32_t size, BmxDiJoyState *state, BmxPadSnapshot *pad)
{
    if (!d->acquired) {
        return kDiErrNotAcquired;
    }
    if (lost(d)) {
        return kDiErrInputLost;
    }
    if (size != d->dataSize || size != sizeof(BmxDiJoyState)) {
        return kDiErrInvalidParam;
    }
    padRead(pad);
    bmxPadToJoyState(pad, d->props, state);
    return kDiOk;
}

/* The state is written only on success (on failure the game keeps reading its old buffer, as with DI). */
static BmxHresult BMX_WINAPI devGetDeviceState(BmxDiDevice *self, uint32_t size, void *data)
{
    Device *const d = (Device *)self;
    BmxHresult r;

    if (d->keyboard) {
        uint8_t state[256];

        r = keyboardState(d, size, state);
        bmxCheckKeyboardState(d->check, r, r == kDiOk ? state : NULL);
        if (r == kDiOk) {
            memcpy(data, state, size);
        }
    } else {
        BmxDiJoyState state;
        BmxPadSnapshot pad;

        r = joyState(d, size, &state, &pad);
        bmxCheckJoyState(d->check, r, r == kDiOk ? &state : NULL, &pad);
        if (r == kDiOk) {
            const uint8_t dpadNow = gDpadSteering ? bmxPadDpad(&pad) : 0;

            if (gStick8Way && dpadNow == 0) {
                (void)bmxPadApplyStick8Way(&pad, d->props, &state);
            }
            if (gDpadSteering) {
                static uint8_t lastDpad;
                static int lines;
                const uint8_t dpad = dpadNow;

                (void)bmxPadApplyDpad(dpad, d->props, &state);
                if (dpad != lastDpad && lines < 200) {
                    lines++;
                    bmxLog("input: d-pad 0x%x -> lX %d, lY %d\n", dpad, (int)state.axis[kDiAxisX],
                           (int)state.axis[kDiAxisY]);
                }
                lastDpad = dpad;
            }
            memcpy(data, &state, size);
        }
    }
    return r;
}

static BmxHresult BMX_WINAPI devSetDataFormat(BmxDiDevice *self, const BmxDiDataFormat *format)
{
    Device *const d = (Device *)self;
    BmxHresult r = kDiOk;

    if (format == NULL) {
        r = kDiErrInvalidParam;
    } else if (d->acquired) {
        r = kDiErrAcquired;
    } else {
        d->dataSize = format->dataSize;
    }
    bmxCheckSetDataFormat(d->check, format, r);
    return r;
}

static BmxHresult BMX_WINAPI devSetCooperativeLevel(BmxDiDevice *self, void *window, uint32_t flags)
{
    Device *const d = (Device *)self;
    const int exclusive = (flags & (kDisclExclusive | kDisclNonExclusive)) == kDisclExclusive ||
                          (flags & (kDisclExclusive | kDisclNonExclusive)) == kDisclNonExclusive;
    const int ground = (flags & (kDisclForeground | kDisclBackground)) == kDisclForeground ||
                       (flags & (kDisclForeground | kDisclBackground)) == kDisclBackground;
    BmxHresult r = kDiOk;

    if (!exclusive || !ground || (d->keyboard && (flags & kDisclExclusive) && (flags & kDisclBackground))) {
        r = kDiErrInvalidParam;
    } else {
        d->coopFlags = flags;
    }
    bmxCheckSetCooperativeLevel(d->check, window, flags, r);
    return r;
}

/* {PIDVID-0000-0000-0000-504944564944}: DirectInput's product GUID of a HID device. */
static BmxGuid productGuid(uint16_t vendor, uint16_t product)
{
    const BmxGuid g = {(uint32_t)product << 16 | vendor, 0, 0, {0, 0, 'P', 'I', 'D', 'V', 'I', 'D'}};

    return g;
}

static const BmxGuid kPadInstance = {0x62d4f0a0, 0xb0e5, 0x11f0, {'b', 'm', 'x', 'p', 'a', 'd', 0, 1}};
const BmxGuid kBmxGuidSysKeyboard = {0x6f1d2b61, 0xd5a0, 0x11cf, {0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0, 0}};

static void padInstance(BmxDiDeviceInstance *info)
{
    memset(info, 0, sizeof *info);
    info->size = sizeof *info;
    info->guidInstance = kPadInstance;
    info->guidProduct = productGuid(gPad.vendor, gPad.product);
    /* DIDEVTYPE_JOYSTICK | DIDEVTYPEJOYSTICK_GAMEPAD (4 in DirectInput 7) << 8 | DIDEVTYPE_HID, as
     * DirectInput reports a DualSense; HID usage page 1, usage 5 */
    info->devType = 0x10404;
    SDL_snprintf(info->instanceName, sizeof info->instanceName, "%s", gPad.name);
    SDL_snprintf(info->productName, sizeof info->productName, "%s", gPad.name);
    info->usagePage = 1;
    info->usage = 5;
}

static BmxHresult BMX_WINAPI devGetDeviceInfo(BmxDiDevice *self, BmxDiDeviceInstance *info)
{
    Device *const d = (Device *)self;
    BmxHresult r = kDiOk;

    if (info == NULL || info->size != sizeof *info) {
        r = kDiErrInvalidParam;
    } else if (d->keyboard) {
        memset(info, 0, sizeof *info);
        info->size = sizeof *info;
        info->guidInstance = kBmxGuidSysKeyboard;
        info->guidProduct = kBmxGuidSysKeyboard;
        info->devType = 0x403; /* DIDEVTYPE_KEYBOARD | DIDEVTYPEKEYBOARD_PCENH << 8 */
        SDL_snprintf(info->instanceName, sizeof info->instanceName, "Keyboard");
        SDL_snprintf(info->productName, sizeof info->productName, "Keyboard");
    } else {
        padInstance(info);
    }
    bmxCheckDeviceInfo(d->check, r == kDiOk ? info : NULL, r);
    return r;
}

/* The methods pcinput.c calls; the rest stay NULL (a call would fault at 0 and be reported by the trap
 * handler). */
static const BmxDiDeviceVtbl kDeviceVtbl = {
    .queryInterface = devQueryInterface,
    .addRef = devAddRef,
    .release = devRelease,
    .setProperty = devSetProperty,
    .acquire = devAcquire,
    .unacquire = devUnacquire,
    .getDeviceState = devGetDeviceState,
    .setDataFormat = devSetDataFormat,
    .setCooperativeLevel = devSetCooperativeLevel,
    .getDeviceInfo = devGetDeviceInfo,
    .poll = devPoll,
};

/* ---- IDirectInput7A ---- */

static int gDiRefs;

static BmxHresult BMX_WINAPI diQueryInterface(BmxDi *self, const BmxGuid *iid, void **out)
{
    (void)self;
    (void)iid;
    *out = NULL;
    return kDiErrNoInterface;
}

static uint32_t BMX_WINAPI diAddRef(BmxDi *self)
{
    (void)self;
    return (uint32_t)++gDiRefs;
}

static uint32_t BMX_WINAPI diRelease(BmxDi *self)
{
    (void)self;
    return (uint32_t)(gDiRefs > 0 ? --gDiRefs : 0);
}

enum { kDiDevTypeJoystick = 4, kDiEnumContinue = 1 };

/* Joysticks: the pad chosen at start, if any (the game asks for DIDEVTYPE_JOYSTICK, attached only). */
static BmxHresult BMX_WINAPI diEnumDevices(BmxDi *self, uint32_t type, BmxDiEnumCallback callback, void *ref,
                                           uint32_t flags)
{
    (void)self;
    (void)flags;
    if (callback == NULL) {
        return kDiErrInvalidParam;
    }
    if ((type == 0 || type == kDiDevTypeJoystick) && gPad.present && padConnected()) {
        BmxDiDeviceInstance info;

        padInstance(&info);
        (void)callback(&info, ref);
    }
    return kDiOk;
}

static BmxHresult BMX_WINAPI diCreateDeviceEx(BmxDi *self, const BmxGuid *guid, const BmxGuid *iid,
                                              BmxDiDevice **out, void *outer)
{
    const int keyboard = memcmp(guid, &kBmxGuidSysKeyboard, sizeof *guid) == 0;
    Device *d = NULL;

    (void)self;
    (void)outer;
    *out = NULL;
    if (!keyboard && (!gPad.present || memcmp(guid, &kPadInstance, sizeof *guid) != 0)) {
        return kDiErrDeviceNotReg;
    }
    for (int i = 0; i < kMaxDevices && d == NULL; i++) {
        d = gDevices[i].used ? NULL : &gDevices[i];
    }
    if (d == NULL) {
        return kDiErrInvalidParam;
    }
    memset(d, 0, sizeof *d);
    d->vtbl = &kDeviceVtbl;
    d->used = 1;
    d->refs = 1;
    d->keyboard = keyboard;
    for (int i = 0; i < kDiAxes; i++) {
        bmxDiAxisDefaults(&d->props[i]);
    }
    d->check = bmxCheckOpen(keyboard, iid);
    *out = (BmxDiDevice *)d;
    return kDiOk;
}

static const BmxDiVtbl kDiVtbl = {
    .queryInterface = diQueryInterface,
    .addRef = diAddRef,
    .release = diRelease,
    .enumDevices = diEnumDevices,
    .createDeviceEx = diCreateDeviceEx,
};

static BmxDi gDirectInput = {&kDiVtbl};

int32_t BMX_WINAPI bmxStandDirectInputCreateEx(void *instance, uint32_t version, const void *iid, void **out,
                                               void *outer)
{
    (void)outer;
    if (!gStarted) {
        gStarted = 1;
        gScripted = bmxKeysScripted();
        /* On by default (the user's call, PLAN.md Decisions): input conveniences; 0 gives vanilla. */
        gDpadSteering = bmxConfigOption("input.dpad_steering", 1, 0);
        gStick8Way = bmxConfigOption("input.stick_8way", 1, 0);
        if (gScripted) {
            bmxLog("input: scripted keyboard (BMX_KEYS), no pad, focus ignored\n");
        } else {
            padStart();
        }
        if (gDpadSteering) {
            bmxLog("input: d-pad steering on (the d-pad also sets lX / lY)\n");
        }
        if (gStick8Way) {
            bmxLog("input: stick 8-way snapping on\n");
        }
        if (bmxCheckEnabled()) {
            bmxCheckStart(instance, version, iid);
        }
    }
    gDiRefs++;
    *out = &gDirectInput;
    return kDiOk;
}

void bmxInputReset(void)
{
    for (int i = 0; i < kMaxDevices; i++) {
        bmxCheckRelease(gDevices[i].check);
    }
    bmxCheckReset();
    padClose();
    memset(&gPad, 0, sizeof gPad);
    memset(gDevices, 0, sizeof gDevices);
    memset(gSeed, 0, sizeof gSeed);
    gProbe = NULL;
    gStarted = gScripted = gDpadSteering = gStick8Way = gDiRefs = 0;
}
