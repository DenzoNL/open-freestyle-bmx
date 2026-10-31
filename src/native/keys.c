/*
 * Scripted keyboard (dev aid, BMX_KEYS): the game reads its keyboard through DirectInput only (pcinput.c:
 * GetDeviceState on a c_dfDIKeyboard device, once per poll), so the script replaces DirectInput itself. The
 * import shim for DirectInputCreateEx hands out a stand-in IDirectInput7 that enumerates no joystick and
 * creates only the keyboard, a stand-in device whose state is the script's keys at the current shown frame.
 * Nothing real is read: the run is the same whether the window has the focus or not, and keys the user
 * presses in another program (or a game pad) never reach it. Without BMX_KEYS the shim forwards to the real
 * DirectInputCreateEx. Modal message boxes are answered by a shim as well (messageBoxA below).
 *
 * BMX_KEYS="f300:enter,+100:enter,+20:down*10,f900:left+e" is a comma separated list of events:
 *   f<N>       press at shown frame N (RwRasterShowRaster count, as BMX_SHOT_FRAME / BMX_QUIT_FRAMES)
 *   +<N>       press N frames after the previous event's start
 *   :<keys>    key names joined by '+': enter, esc, space, up, down, left, right, a..z, 0..9, or a DIK code
 *              as 0x<hex>
 *   *<H>       hold for H frames (default 2)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIRECTINPUT_VERSION 0x0700
#define CINTERFACE
#include <windows.h>

#include <dinput.h>

#include "driver/driver.h"
#include "native/native.h"
#include "platform/platform.h"

enum { kMaxEvents = 256, kMaxEventKeys = 4, kDefaultHold = 2 };

typedef struct KeyEvent {
    int frame, hold, count;
    uint8_t keys[kMaxEventKeys];
} KeyEvent;

static KeyEvent events[kMaxEvents];
static int eventCount;
static int nextLogged; /* first event not logged yet */
static int scripted;

static const struct {
    const char *name;
    uint8_t dik;
} kKeyNames[] = {
    {"enter", DIK_RETURN}, {"esc", DIK_ESCAPE}, {"space", DIK_SPACE}, {"up", DIK_UP},
    {"down", DIK_DOWN},    {"left", DIK_LEFT},  {"right", DIK_RIGHT},
};

/* Letters in DIK order (QWERTY rows) and digits 1..9, 0. */
static const char kDikRow1[] = "qwertyuiop", kDikRow2[] = "asdfghjkl", kDikRow3[] = "zxcvbnm";

/* DIK code of a letter in one of the rows, or -1. */
static int rowKey(const char *row, int first, char c)
{
    for (int i = 0; row[i] != 0; i++) {
        if (row[i] == c) {
            return first + i;
        }
    }
    return -1;
}

static int keyCode(const char *name, size_t len)
{
    int code;

    for (size_t i = 0; i < sizeof kKeyNames / sizeof kKeyNames[0]; i++) {
        if (strlen(kKeyNames[i].name) == len && strncmp(kKeyNames[i].name, name, len) == 0) {
            return kKeyNames[i].dik;
        }
    }
    if (len > 2 && name[0] == '0' && name[1] == 'x') {
        char *end;
        const unsigned long v = strtoul(name, &end, 16);

        return (end == name + len && v > 0 && v < 0x100) ? (int)v : -1;
    }
    if (len != 1) {
        return -1;
    }
    if (name[0] >= '1' && name[0] <= '9') {
        return DIK_1 + (name[0] - '1');
    }
    if (name[0] == '0') {
        return DIK_0;
    }
    code = rowKey(kDikRow1, DIK_Q, name[0]);
    if (code < 0) {
        code = rowKey(kDikRow2, DIK_A, name[0]);
    }
    if (code < 0) {
        code = rowKey(kDikRow3, DIK_Z, name[0]);
    }
    return code;
}

/* Parses one event ("f300:enter*4"); `previous` is the start frame of the event before it. */
static int parseEvent(const char *s, size_t len, int previous, KeyEvent *e)
{
    const char *const end = s + len;
    const char *p = s;
    char *num;

    if (p < end && (*p == 'f' || *p == '+')) {
        const long n = strtol(p + 1, &num, 10);

        if (num == p + 1 || n < 0) {
            return -1;
        }
        e->frame = (*p == 'f') ? (int)n : previous + (int)n;
        p = num;
    } else {
        return -1;
    }
    if (p >= end || *p != ':') {
        return -1;
    }
    p++;
    e->hold = kDefaultHold;
    e->count = 0;
    while (p < end) {
        const char *k = p;
        int code;

        while (p < end && *p != '+' && *p != '*') {
            p++;
        }
        code = keyCode(k, (size_t)(p - k));
        if (code < 0 || e->count == kMaxEventKeys) {
            return -1;
        }
        e->keys[e->count++] = (uint8_t)code;
        if (p < end && *p == '*') {
            const long h = strtol(p + 1, &num, 10);

            if (num != end || h <= 0) {
                return -1;
            }
            e->hold = (int)h;
            p = end;
        } else if (p < end) {
            p++;
        }
    }
    return e->count > 0 ? 0 : -1;
}

int bmxKeysInstall(void)
{
    static char script[8192];
    const char *env = getenv("BMX_KEYS");
    const char *s = script;
    int previous = 0;

    if (env == NULL) {
        return 0;
    }
    snprintf(script, sizeof script, "%s", env);
    while (*s) {
        const char *comma = strchr(s, ',');
        const size_t len = comma != NULL ? (size_t)(comma - s) : strlen(s);

        if (len > 0) {
            if (eventCount == kMaxEvents || parseEvent(s, len, previous, &events[eventCount]) != 0) {
                bmxLog("BMX_KEYS: cannot parse \"%.*s\" (or more than %d events)\n", (int)len, s, kMaxEvents);
                exit(1);
            }
            previous = events[eventCount++].frame;
        }
        s += len + (comma != NULL);
    }
    scripted = 1;
    bmxLog("BMX_KEYS: %d scripted key events; the real keyboard and joysticks are not read\n", eventCount);
    return 1;
}

/* ---- stand-in DirectInput keyboard ---- */

static HRESULT STDMETHODCALLTYPE kbdQueryInterface(IDirectInputDevice7A *self, REFIID riid, void **out)
{
    (void)self;
    (void)riid;
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE kbdAddRef(IDirectInputDevice7A *self)
{
    (void)self;
    return 1;
}

static HRESULT STDMETHODCALLTYPE kbdOk(IDirectInputDevice7A *self)
{
    (void)self;
    return DI_OK;
}

static HRESULT STDMETHODCALLTYPE kbdSetDataFormat(IDirectInputDevice7A *self, LPCDIDATAFORMAT format)
{
    (void)self;
    (void)format;
    return DI_OK;
}

static HRESULT STDMETHODCALLTYPE kbdSetCooperativeLevel(IDirectInputDevice7A *self, HWND window, DWORD flags)
{
    (void)self;
    (void)window;
    (void)flags;
    return DI_OK;
}

/* The c_dfDIKeyboard state: 0x80 for each scripted key held at the current shown frame. */
static HRESULT STDMETHODCALLTYPE kbdGetDeviceState(IDirectInputDevice7A *self, DWORD size, void *data)
{
    const int frame = drvGpuShownFrames();
    uint8_t *const state = data;

    (void)self;
    memset(state, 0, size);
    for (int i = 0; i < eventCount; i++) {
        const KeyEvent *e = &events[i];

        if (frame >= e->frame && frame < e->frame + e->hold) {
            for (int k = 0; k < e->count; k++) {
                if (e->keys[k] < size) {
                    state[e->keys[k]] = 0x80;
                }
            }
            if (i >= nextLogged) {
                bmxLog("BMX_KEYS: frame %d: event %d (%d keys, first DIK 0x%02x, %d frames)\n", frame, i,
                       e->count, e->keys[0], e->hold);
                nextLogged = i + 1;
            }
        }
    }
    return DI_OK;
}

/* The game calls Release, Acquire, Unacquire, GetDeviceState, SetDataFormat and SetCooperativeLevel on the
 * keyboard (pcinput.c); the other entries stay NULL. */
static IDirectInputDevice7AVtbl kbdVtbl = {
    .QueryInterface = kbdQueryInterface,
    .AddRef = kbdAddRef,
    .Release = kbdAddRef,
    .Acquire = kbdOk,
    .Unacquire = kbdOk,
    .GetDeviceState = kbdGetDeviceState,
    .SetDataFormat = kbdSetDataFormat,
    .SetCooperativeLevel = kbdSetCooperativeLevel,
};

static IDirectInputDevice7A keyboard = {&kbdVtbl};

/* ---- stand-in IDirectInput7 ---- */

/* GUID_SysKeyboard (the game passes its own copy, 0x54f2f8). */
static const GUID kSysKeyboard = {
    0x6f1d2b61, 0xd5a0, 0x11cf, {0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

static HRESULT STDMETHODCALLTYPE diQueryInterface(IDirectInput7A *self, REFIID riid, void **out)
{
    (void)self;
    (void)riid;
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE diAddRef(IDirectInput7A *self)
{
    (void)self;
    return 1;
}

/* No joystick: the game's pad device falls back to the keyboard. */
static HRESULT STDMETHODCALLTYPE diEnumDevices(IDirectInput7A *self, DWORD type,
                                               LPDIENUMDEVICESCALLBACKA callback, void *ref, DWORD flags)
{
    (void)self;
    (void)type;
    (void)callback;
    (void)ref;
    (void)flags;
    return DI_OK;
}

static HRESULT STDMETHODCALLTYPE diCreateDeviceEx(IDirectInput7A *self, REFGUID guid, REFIID riid, void **out,
                                                  IUnknown *outer)
{
    (void)self;
    (void)riid;
    (void)outer;
    if (IsEqualGUID(guid, &kSysKeyboard)) {
        *out = &keyboard;
        return DI_OK;
    }
    *out = NULL;
    return DIERR_DEVICENOTREG;
}

/* The game calls Release, EnumDevices and CreateDeviceEx (pcinput.c); the other entries stay NULL. */
static IDirectInput7AVtbl diVtbl = {
    .QueryInterface = diQueryInterface,
    .AddRef = diAddRef,
    .Release = diAddRef,
    .EnumDevices = diEnumDevices,
    .CreateDeviceEx = diCreateDeviceEx,
};

static IDirectInput7A directInput = {&diVtbl};

typedef HRESULT(WINAPI *DirectInputCreateExFn)(HINSTANCE, DWORD, REFIID, void **, IUnknown *);

/* The import table's stand-in for DINPUT.dll!DirectInputCreateEx (src/platform/imports.c). */
HRESULT WINAPI bmxKeysDirectInputCreateEx(HINSTANCE instance, DWORD version, REFIID iid, void **out,
                                          IUnknown *outer)
{
    DirectInputCreateExFn real;

    if (scripted) {
        *out = &directInput;
        return DI_OK;
    }
    /* The DINPUT.dll the import fill loaded (the game folder's hook DLL forwards to the system one). */
    real = (DirectInputCreateExFn)bmxRealProc("DINPUT.dll", "DirectInputCreateEx");
    return real != NULL ? real(instance, version, iid, out, outer) : DIERR_GENERIC;
}

/*
 * A scripted run cannot answer a modal message box (the quit prompt, an assert, an application error), and
 * the quit prompt's MB_SETFOREGROUND would take the focus. So with BMX_KEYS each box is logged and answered
 * at once: Yes for Yes/No (the quit prompt quits), Cancel for Yes/No/Cancel (an assert terminates), else OK.
 */
/* The import table's stand-in for USER32.dll!MessageBoxA. */
int WINAPI bmxKeysMessageBoxA(HWND owner, const char *text, const char *caption, UINT type)
{
    const UINT buttons = type & MB_TYPEMASK;
    const int answer = buttons == MB_YESNO ? IDYES : buttons == MB_YESNOCANCEL ? IDCANCEL : IDOK;

    if (!scripted) {
        return MessageBoxA(owner, text, caption, type);
    }
    bmxLog("BMX_KEYS: message box \"%s\" answered %d\n", text != NULL ? text : "", answer);
    return answer;
}

/* The import table entries of this file (src/platform/imports.c looks them up with the stand-ins of its own).
 */
const BmxStandIn bmxKeysStandIns[] = {
    {"DINPUT.dll", "DirectInputCreateEx", (void *)bmxKeysDirectInputCreateEx},
    {"USER32.dll", "MessageBoxA", (void *)bmxKeysMessageBoxA},
};
const size_t bmxKeysStandInCount = sizeof bmxKeysStandIns / sizeof bmxKeysStandIns[0];
