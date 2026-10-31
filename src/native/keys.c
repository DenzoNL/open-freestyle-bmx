/*
 * Scripted keyboard (dev aid, BMX_KEYS): the game reads its keyboard through DirectInput only (pcinput.c:
 * GetDeviceState on a c_dfDIKeyboard device, once per poll), and DirectInput is our stand-in
 * (src/platform/input.c). With BMX_KEYS its keyboard reports the script's keys at the current shown frame
 * (bmxKeysFillState), it lists no pad and ignores the focus: nothing real is read, the run is the same
 * whether the window has the focus or not, and keys the user presses in another program (or a game pad) never
 * reach it. The script's key edges also reach the window procedure as WM_KEYDOWN / WM_KEYUP, and modal
 * message boxes are answered without a window (both in src/platform/window.c).
 *
 * BMX_KEYS="f300:enter,+100:enter,+20:down*10,f900:left+e" is a comma separated list of events:
 *   f<N>       press at shown frame N (RwRasterShowRaster count, as BMX_SHOT_FRAME / BMX_QUIT_FRAMES)
 *   t<N>       press N milliseconds after the start (for movies, during which no frame is shown)
 *   +<N>       press N frames (milliseconds after a t event) after the previous event's start
 *   :<keys>    key names joined by '+': enter, esc, space, up, down, left, right, a..z, 0..9, or a DIK code
 *              as 0x<hex>
 *   *<H>       hold for H frames (default 2; for t events milliseconds, default 100)
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

enum { kMaxEvents = 256, kMaxEventKeys = 4, kDefaultHold = 2, kDefaultHoldMs = 100 };

typedef struct KeyEvent {
    int frame, hold, count; /* frame and hold in milliseconds when timed */
    int timed;
    uint8_t keys[kMaxEventKeys];
} KeyEvent;

static KeyEvent events[kMaxEvents];
static int eventCount;
static int nextLogged; /* first event not logged yet */
static int scripted;
static uint32_t startTime; /* bmxTimeGetTime at install, for t events */

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

/* Parses one event ("f300:enter*4"); `previous` is the event before it (NULL for the first). */
static int parseEvent(const char *s, size_t len, const KeyEvent *previous, KeyEvent *e)
{
    const char *const end = s + len;
    const char *p = s;
    char *num;

    if (p < end && (*p == 'f' || *p == 't' || *p == '+')) {
        const long n = strtol(p + 1, &num, 10);

        if (num == p + 1 || n < 0) {
            return -1;
        }
        e->timed = *p == 't' || (*p == '+' && previous != NULL && previous->timed);
        e->frame = (*p == '+') ? (previous != NULL ? previous->frame : 0) + (int)n : (int)n;
        p = num;
    } else {
        return -1;
    }
    if (p >= end || *p != ':') {
        return -1;
    }
    p++;
    e->hold = e->timed ? kDefaultHoldMs : kDefaultHold;
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

    if (env == NULL) {
        return 0;
    }
    snprintf(script, sizeof script, "%s", env);
    while (*s) {
        const char *comma = strchr(s, ',');
        const size_t len = comma != NULL ? (size_t)(comma - s) : strlen(s);

        if (len > 0) {
            if (eventCount == kMaxEvents ||
                parseEvent(s, len, eventCount > 0 ? &events[eventCount - 1] : NULL, &events[eventCount]) !=
                    0) {
                bmxLog("BMX_KEYS: cannot parse \"%.*s\" (or more than %d events)\n", (int)len, s, kMaxEvents);
                exit(1);
            }
            eventCount++;
        }
        s += len + (comma != NULL);
    }
    scripted = 1;
    startTime = bmxTimeGetTime();
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
    const int ms = (int)(bmxTimeGetTime() - startTime);

    for (int i = 0; i < eventCount; i++) {
        const KeyEvent *e = &events[i];
        const int now = e->timed ? ms : frame;

        if (now >= e->frame && now < e->frame + e->hold) {
            for (int k = 0; k < e->count; k++) {
                if (e->keys[k] < size) {
                    state[e->keys[k]] = 0x80;
                }
            }
            if (i >= nextLogged) {
                bmxLog("BMX_KEYS: frame %d (%d ms): event %d (%d keys, first DIK 0x%02x, %d %s)\n", frame, ms,
                       i, e->count, e->keys[0], e->hold, e->timed ? "ms" : "frames");
                nextLogged = i + 1;
            }
        }
    }
}
