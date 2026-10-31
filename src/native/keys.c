/*
 * Scripted keyboard (dev aid, BMX_KEYS): the game reads its keyboard through DirectInput only (pcinput.c:
 * GetDeviceState on a c_dfDIKeyboard device, once per poll), and DirectInput is our stand-in
 * (src/platform/input.c). With BMX_KEYS its keyboard reports the script's keys at the current shown frame
 * (bmxKeysFillState), it lists no pad and ignores the focus: nothing real is read, the run is the same
 * whether the window has the focus or not, and keys the user presses in another program (or a game pad) never
 * reach it. Modal message boxes are answered by a shim as well (messageBoxA below).
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

int bmxKeysScripted(void)
{
    return scripted;
}

/* The script's keys held at the current shown frame (0x80 each) into the DIK state of the DirectInput
 * stand-in's keyboard (src/platform/input.c), which zeroed it. */
void bmxKeysFillState(uint8_t *state, uint32_t size)
{
    const int frame = drvGpuShownFrames();

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
    {"USER32.dll", "MessageBoxA", (void *)bmxKeysMessageBoxA},
};
const size_t bmxKeysStandInCount = sizeof bmxKeysStandIns / sizeof bmxKeysStandIns[0];
