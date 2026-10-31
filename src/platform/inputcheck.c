/*
 * BMX_INPUT_CHECK=1 (docs/PLATFORM.md S5): the real DirectInput opened alongside the stand-in. Every call the
 * game makes on a stand-in device (input.c) is repeated on a real device for the same keyboard / first
 * joystick, and the results are compared: HRESULTs, the 256-byte DIK state, DIJOYSTATE.
 *
 * What the game reads decides what is strict: the DIK bytes' 0x80, the buttons' 0x80 and the sign of lX / lY
 * (pcinput.c tests `> 0`; the device block's bits are a function of exactly these). A difference that lasts
 * one read only is counted as transient (the two read the device at slightly different times: a key event
 * reaches SDL through the message queue, DirectInput directly), one that is there in two reads in a row is
 * logged as a mismatch. The other axes and the POV are compared and reported, but are not strict. The stick
 * threshold is measured per direction: the raw HID value at which DirectInput's lX / lY first leaves 0, and
 * ours. Edges of real keys, buttons and the stick sign are logged as they happen, so a test session's log
 * reads like a protocol. bmxCheckSummary prints the totals (bmxPlatformShutdown).
 */
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

#include "platform/input.h"

enum { kMaxChecked = 8, kMaxLines = 3000, kRepeatLines = 3 };

/* An extreme of the stick threshold: per axis (X, Y) and side (-, +). */
typedef struct Threshold {
    int32_t zeroMax; /* largest |raw - centre| reported as 0 (-1: none seen) */
    int32_t onMin;   /* smallest |raw - centre| reported non-zero (-1: none seen) */
} Threshold;

struct BmxCheckDevice {
    uint64_t reads, bothOk, hresultMismatches, persistent, transient, infoMismatches;
    uint64_t stickDiffs; /* reads with |lX or lY difference| > 1 */
    BmxDiDevice *real;
    int used, keyboard, index;
    int32_t maxStickDiff;
    uint32_t infoSeen; /* info fields already logged once */
    int lastRealSign[2], lastOurSign[2];
    BmxHresult lastResult[2]; /* previous (ours, real) of GetDeviceState */
    Threshold realLimit[2][2], ourLimit[2][2];
    uint64_t stableReads, stableDiffs; /* lX / lY reads with raw value and DirectInput's value unchanged */
    const char *lastMethod;            /* the last HRESULT difference and how often it repeated */
    BmxHresult lastOurs, lastTheirs;
    int repeats;
    int32_t prevRaw[2], prevReal[2];
    int16_t stableValue[2][256]; /* 8-bit sticks: DirectInput's value per raw value on still reads */
    uint8_t stableSeen[2][256];
    uint8_t lastReal[256];      /* previous real keyboard state / buttons (edges) */
    uint8_t lastDiff[256 + 64]; /* per strict item: differed in the previous read */
};

static BmxCheckDevice gChecked[kMaxChecked];
static BmxDi *gReal;
static BmxGuid gRealJoystick;
static int gRealHasJoystick, gEnabled = -1, gLines, gDeviceCount;
static uint64_t gTotalHresult;

static void line(const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(1);

static void line(const char *fmt, ...)
{
    char text[512];
    va_list ap;

    if (gLines++ >= kMaxLines) {
        if (gLines == kMaxLines + 1) {
            bmxLog("inputcheck: more than %d lines, quiet until the summary\n", kMaxLines);
        }
        return;
    }
    va_start(ap, fmt);
    SDL_vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);
    bmxLog("inputcheck: [%7.3f s] %s\n", (double)SDL_GetTicks() / 1000.0, text);
}

void bmxCheckReset(void)
{
    for (int i = 0; i < kMaxChecked; i++) {
        bmxCheckRelease(&gChecked[i]);
    }
    memset(gChecked, 0, sizeof gChecked);
    if (gReal != NULL) {
        gReal->vtbl->release(gReal);
    }
    gReal = NULL;
    gRealHasJoystick = gLines = gDeviceCount = 0;
    gTotalHresult = 0;
    gEnabled = -1;
}

uint64_t bmxCheckDifferences(void)
{
    uint64_t n = gTotalHresult;

    for (int i = 0; i < kMaxChecked; i++) {
        n += gChecked[i].persistent;
    }
    return n;
}

int bmxCheckEnabled(void)
{
    if (gEnabled < 0) {
        const char *e = getenv("BMX_INPUT_CHECK");

        gEnabled = e != NULL && *e != 0 && *e != '0' && !bmxKeysScripted();
    }
    return gEnabled;
}

static uint32_t BMX_WINAPI firstJoystick(const BmxDiDeviceInstance *instance, void *ref)
{
    (void)ref;
    gRealJoystick = instance->guidInstance;
    gRealHasJoystick = 1;
    bmxLog("inputcheck: DirectInput's first joystick: \"%s\", product GUID %08x-%04x-%04x\n",
           instance->productName, (unsigned)instance->guidProduct.data1, instance->guidProduct.data2,
           instance->guidProduct.data3);
    return 0;
}

void bmxCheckStart(void *instance, uint32_t version, const BmxGuid *iid)
{
    const BmxDirectInputCreateExFn create =
        (BmxDirectInputCreateExFn)bmxRealProc("DINPUT.dll", "DirectInputCreateEx");

    if (create == NULL || create(instance, version, iid, &gReal, NULL) < 0) {
        bmxLog("inputcheck: the real DirectInputCreateEx is not available; no check\n");
        gReal = NULL;
        gEnabled = 0;
        return;
    }
    gReal->vtbl->enumDevices(gReal, 4, firstJoystick, NULL, 1);
    if (!gRealHasJoystick) {
        bmxLog("inputcheck: DirectInput lists no joystick\n");
    }
    bmxLog("inputcheck: on; strict = DIK 0x80, buttons 0x80, sign of lX / lY; a difference in two reads in a "
           "row is a mismatch\n");
}

BmxCheckDevice *bmxCheckOpen(int keyboard, const BmxGuid *iid)
{
    BmxCheckDevice *c = NULL;
    BmxDiDevice *real = NULL;
    BmxHresult r;

    if (gReal == NULL) {
        return NULL;
    }
    if (!keyboard && !gRealHasJoystick) {
        line("our pad device has no real counterpart (DirectInput lists no joystick)");
        return NULL;
    }
    r = gReal->vtbl->createDeviceEx(gReal, keyboard ? &kBmxGuidSysKeyboard : &gRealJoystick, iid, &real,
                                    NULL);
    if (r < 0) {
        line("real CreateDeviceEx(%s) failed: 0x%08x", keyboard ? "keyboard" : "joystick", (unsigned)r);
        return NULL;
    }
    for (int i = 0; i < kMaxChecked && c == NULL; i++) {
        c = gChecked[i].used ? NULL : &gChecked[i];
    }
    if (c == NULL) {
        real->vtbl->release(real);
        return NULL;
    }
    memset(c, 0, sizeof *c);
    c->used = 1;
    c->real = real;
    c->keyboard = keyboard;
    c->index = gDeviceCount++;
    for (int a = 0; a < 2; a++) {
        for (int s = 0; s < 2; s++) {
            c->realLimit[a][s] = c->ourLimit[a][s] = (Threshold){-1, -1};
        }
    }
    line("device %d (%s) mirrored", c->index, keyboard ? "keyboard" : "pad");
    return c;
}

static const char *name(const BmxCheckDevice *c)
{
    return c->keyboard ? "keyboard" : "pad";
}

static void compareResult(BmxCheckDevice *c, const char *method, BmxHresult ours, BmxHresult real)
{
    if (ours != real) {
        c->hresultMismatches++;
        gTotalHresult++;
        /* the same difference again and again (a call the game repeats every poll): the first few only */
        if (method == c->lastMethod && ours == c->lastOurs && real == c->lastTheirs) {
            c->repeats++;
        } else {
            c->repeats = 0;
        }
        c->lastMethod = method;
        c->lastOurs = ours;
        c->lastTheirs = real;
        if (c->repeats < kRepeatLines) {
            line("device %d (%s) %s: ours 0x%08x, DirectInput 0x%08x%s", c->index, name(c), method,
                 (unsigned)ours, (unsigned)real,
                 c->repeats == kRepeatLines - 1 ? " (repeats not logged)" : "");
        }
    }
}

void bmxCheckSetDataFormat(BmxCheckDevice *c, const BmxDiDataFormat *format, BmxHresult ours)
{
    if (c != NULL) {
        compareResult(c, "SetDataFormat", ours, c->real->vtbl->setDataFormat(c->real, format));
    }
}

void bmxCheckSetCooperativeLevel(BmxCheckDevice *c, void *window, uint32_t flags, BmxHresult ours)
{
    if (c != NULL) {
        compareResult(c, "SetCooperativeLevel", ours,
                      c->real->vtbl->setCooperativeLevel(c->real, window, flags));
    }
}

void bmxCheckSetProperty(BmxCheckDevice *c, uintptr_t property, const BmxDiPropHeader *header,
                         BmxHresult ours)
{
    if (c != NULL) {
        compareResult(c, "SetProperty", ours, c->real->vtbl->setProperty(c->real, property, header));
    }
}

void bmxCheckAcquire(BmxCheckDevice *c, BmxHresult ours)
{
    if (c != NULL) {
        compareResult(c, "Acquire", ours, c->real->vtbl->acquire(c->real));
    }
}

void bmxCheckUnacquire(BmxCheckDevice *c, BmxHresult ours)
{
    if (c != NULL) {
        compareResult(c, "Unacquire", ours, c->real->vtbl->unacquire(c->real));
    }
}

void bmxCheckPoll(BmxCheckDevice *c, BmxHresult ours)
{
    if (c != NULL) {
        compareResult(c, "Poll", ours, c->real->vtbl->poll(c->real));
    }
}

static void printGuid(char *out, size_t size, const BmxGuid *g)
{
    SDL_snprintf(out, size, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", (unsigned)g->data1, g->data2,
                 g->data3, g->data4[0], g->data4[1], g->data4[2], g->data4[3], g->data4[4], g->data4[5],
                 g->data4[6], g->data4[7]);
}

void bmxCheckDeviceInfo(BmxCheckDevice *c, const BmxDiDeviceInstance *ours, BmxHresult result)
{
    BmxDiDeviceInstance real;
    BmxHresult r;
    char a[48], b[48];

    if (c == NULL) {
        return;
    }
    memset(&real, 0, sizeof real);
    real.size = sizeof real;
    r = c->real->vtbl->getDeviceInfo(c->real, &real);
    compareResult(c, "GetDeviceInfo", result, r);
    if (ours == NULL || r < 0) {
        return;
    }
    printGuid(a, sizeof a, &ours->guidProduct);
    printGuid(b, sizeof b, &real.guidProduct);
    line("device %d info: product \"%s\" / DirectInput \"%s\"; product GUID %s / %s (%s); type 0x%x / 0x%x",
         c->index, ours->productName, real.productName, a, b, strcmp(a, b) == 0 ? "same" : "DIFFERENT",
         (unsigned)ours->devType, (unsigned)real.devType);
}

/* One strict item: counts and logs a difference that is there in two reads in a row. */
static void strict(BmxCheckDevice *c, int item, int differs, const char *what, int ours, int real)
{
    if (differs) {
        if (c->lastDiff[item]) {
            c->persistent++;
            line("MISMATCH device %d (%s) %s: ours %d, DirectInput %d", c->index, name(c), what, ours, real);
        } else {
            c->transient++;
        }
    }
    c->lastDiff[item] = (uint8_t)differs;
}

static void compareStateResult(BmxCheckDevice *c, BmxHresult ours, BmxHresult real)
{
    c->reads++;
    if (ours != real && (ours != c->lastResult[0] || real != c->lastResult[1])) {
        compareResult(c, "GetDeviceState", ours, real);
    } else if (ours != real) {
        c->hresultMismatches++;
        gTotalHresult++;
    }
    c->lastResult[0] = ours;
    c->lastResult[1] = real;
}

void bmxCheckKeyboardState(BmxCheckDevice *c, BmxHresult ours, const uint8_t *state)
{
    uint8_t real[256];
    BmxHresult r;

    if (c == NULL) {
        return;
    }
    r = c->real->vtbl->getDeviceState(c->real, sizeof real, real);
    compareStateResult(c, ours, r);
    if (state == NULL || r < 0) {
        return;
    }
    c->bothOk++;
    for (int k = 0; k < 256; k++) {
        const int o = (state[k] & 0x80) != 0, d = (real[k] & 0x80) != 0;
        char what[32];

        if (d != ((c->lastReal[k] & 0x80) != 0)) {
            line("key DIK 0x%02x %s (ours: %s)", k, d ? "down" : "up", o ? "down" : "up");
        }
        SDL_snprintf(what, sizeof what, "DIK 0x%02x", k);
        strict(c, k, o != d, what, o, d);
        c->lastReal[k] = real[k];
    }
}

static int sign(int32_t v)
{
    return (v > 0) - (v < 0);
}

/* The raw HID value (and its centre) of the stick axis `a` (0 X, 1 Y) in our snapshot. */
static int32_t snapshotRaw(const BmxPadSnapshot *pad, int a, int32_t *center)
{
    int32_t logicalMax;
    const int16_t v = pad->family == kBmxPadGeneric ? pad->axes[a] : (a == 0 ? pad->leftX : pad->leftY);
    const int32_t raw = bmxPadRawStick(pad->family, v, &logicalMax);

    *center = (logicalMax + 1) >> 1;
    return raw;
}

static void threshold(Threshold *t, int32_t distance, int32_t value)
{
    if (value == 0) {
        t->zeroMax = distance > t->zeroMax ? distance : t->zeroMax;
    } else if (t->onMin < 0 || distance < t->onMin) {
        t->onMin = distance;
    }
}

void bmxCheckJoyState(BmxCheckDevice *c, BmxHresult ours, const BmxDiJoyState *state,
                      const BmxPadSnapshot *pad)
{
    static const char *const kAxis[kDiAxes] = {"lX", "lY", "lZ", "lRx", "lRy", "lRz", "slider0", "slider1"};
    BmxDiJoyState real;
    BmxHresult r;

    if (c == NULL) {
        return;
    }
    r = c->real->vtbl->getDeviceState(c->real, sizeof real, &real);
    compareStateResult(c, ours, r);
    if (state == NULL || r < 0) {
        return;
    }
    c->bothOk++;
    /* strict: buttons and the stick's sign */
    for (int b = 0; b < 32; b++) {
        const int o = (state->buttons[b] & 0x80) != 0, d = (real.buttons[b] & 0x80) != 0;
        char what[32];

        if (d != ((c->lastReal[b] & 0x80) != 0) || o != ((c->lastReal[32 + b] & 0x80) != 0)) {
            line("pad button %d: DirectInput %s, ours %s", b, d ? "down" : "up", o ? "down" : "up");
        }
        SDL_snprintf(what, sizeof what, "button %d", b);
        strict(c, b, o != d, what, o, d);
        c->lastReal[b] = real.buttons[b];
        c->lastReal[32 + b] = state->buttons[b];
    }
    for (int a = 0; a < 2; a++) {
        int32_t center;
        const int32_t raw = snapshotRaw(pad, a, &center);
        const int side = raw >= center;
        const int32_t distance = raw >= center ? raw - center : center - raw;
        const int32_t diff = state->axis[a] - real.axis[a];

        if (sign(real.axis[a]) != c->lastRealSign[a] || sign(state->axis[a]) != c->lastOurSign[a]) {
            line("stick %s: DirectInput %d, ours %d (our raw HID value %d, centre %d)", kAxis[a],
                 (int)real.axis[a], (int)state->axis[a], (int)raw, (int)center);
        }
        c->lastRealSign[a] = sign(real.axis[a]);
        c->lastOurSign[a] = sign(state->axis[a]);
        strict(c, 256 + a, sign(state->axis[a]) != sign(real.axis[a]), kAxis[a], (int)state->axis[a],
               (int)real.axis[a]);
        threshold(&c->realLimit[a][side], distance, real.axis[a]);
        threshold(&c->ourLimit[a][side], distance, state->axis[a]);
        if (diff > 1 || diff < -1) {
            c->stickDiffs++;
        }
        if ((diff < 0 ? -diff : diff) > c->maxStickDiff) {
            c->maxStickDiff = diff < 0 ? -diff : diff;
        }
        /* A still stick (our raw value and DirectInput's value both as in the previous read): the two read
         * the same HID value, so the values must be equal. These pairs are the exact calibration data. */
        if (c->bothOk > 1 && raw == c->prevRaw[a] && real.axis[a] == c->prevReal[a]) {
            const int eightBit = center == 128 && raw >= 0 && raw < 256;

            c->stableReads++;
            if (state->axis[a] != real.axis[a]) {
                c->stableDiffs++;
                if (!eightBit || !c->stableSeen[a][raw]) {
                    line("still stick %s at raw %d: DirectInput %d, ours %d", kAxis[a], (int)raw,
                         (int)real.axis[a], (int)state->axis[a]);
                }
            }
            if (eightBit) {
                c->stableValue[a][raw] = (int16_t)real.axis[a];
                c->stableSeen[a][raw] = 1;
            }
        }
        c->prevRaw[a] = raw;
        c->prevReal[a] = real.axis[a];
    }
    /* informational: the other axes (within 1% of 65535) and the POVs */
    for (int a = 2; a < kDiAxes; a++) {
        const int32_t diff = state->axis[a] - real.axis[a];

        if (diff > 655 || diff < -655) {
            c->infoMismatches++;
            if (!(c->infoSeen & (1u << a))) {
                c->infoSeen |= 1u << a;
                line("info: %s ours %d, DirectInput %d (logged once)", kAxis[a], (int)state->axis[a],
                     (int)real.axis[a]);
            }
        }
    }
    for (int p = 0; p < 4; p++) {
        if (state->pov[p] != real.pov[p]) {
            c->infoMismatches++;
            if (!(c->infoSeen & (1u << (8 + p)))) {
                c->infoSeen |= 1u << (8 + p);
                line("info: POV %d ours %u, DirectInput %u (logged once)", p, (unsigned)state->pov[p],
                     (unsigned)real.pov[p]);
            }
        }
    }
}

void bmxCheckRelease(BmxCheckDevice *c)
{
    if (c != NULL && c->real != NULL) {
        c->real->vtbl->unacquire(c->real);
        c->real->vtbl->release(c->real);
        c->real = NULL;
    }
}

static void summaryThreshold(const char *axis, const char *side, const Threshold *real, const Threshold *ours)
{
    bmxLog("inputcheck:   %s%s threshold (|raw - centre|): DirectInput 0 up to %d, non-zero from %d; ours 0 "
           "up to "
           "%d, non-zero from %d\n",
           axis, side, (int)real->zeroMax, (int)real->onMin, (int)ours->zeroMax, (int)ours->onMin);
}

/* DirectInput's value per raw value on still reads (8-bit sticks), 16 per line: raw:value. */
static void summaryTable(const BmxCheckDevice *c, int a, const char *axis)
{
    char text[400];
    int n = 0;

    text[0] = 0;
    for (int raw = 0; raw < 256; raw++) {
        if (c->stableSeen[a][raw]) {
            const size_t len = SDL_strlen(text);

            SDL_snprintf(text + len, sizeof text - len, " %d:%d", raw, (int)c->stableValue[a][raw]);
            if (++n % 16 == 0) {
                bmxLog("inputcheck:   %s still raw:DirectInput%s\n", axis, text);
                text[0] = 0;
            }
        }
    }
    if (text[0] != 0) {
        bmxLog("inputcheck:   %s still raw:DirectInput%s\n", axis, text);
    }
}

void bmxCheckSummary(void)
{
    uint64_t persistent = 0;

    if (gReal == NULL) {
        return;
    }
    for (int i = 0; i < kMaxChecked; i++) {
        const BmxCheckDevice *c = &gChecked[i];

        if (!c->used) {
            continue;
        }
        persistent += c->persistent;
        bmxLog("inputcheck: device %d (%s): %llu reads, %llu read by both; %llu mismatches (two reads in a "
               "row), "
               "%llu one-read differences, %llu HRESULT differences, %llu informational differences\n",
               c->index, name(c), (unsigned long long)c->reads, (unsigned long long)c->bothOk,
               (unsigned long long)c->persistent, (unsigned long long)c->transient,
               (unsigned long long)c->hresultMismatches, (unsigned long long)c->infoMismatches);
        if (!c->keyboard) {
            bmxLog("inputcheck:   lX / lY: %llu reads differ by more than 1, largest difference %d\n",
                   (unsigned long long)c->stickDiffs, (int)c->maxStickDiff);
            summaryThreshold("X", "-", &c->realLimit[0][0], &c->ourLimit[0][0]);
            summaryThreshold("X", "+", &c->realLimit[0][1], &c->ourLimit[0][1]);
            summaryThreshold("Y", "-", &c->realLimit[1][0], &c->ourLimit[1][0]);
            summaryThreshold("Y", "+", &c->realLimit[1][1], &c->ourLimit[1][1]);
            bmxLog("inputcheck:   still stick: %llu reads, %llu with a different lX / lY\n",
                   (unsigned long long)c->stableReads, (unsigned long long)c->stableDiffs);
            summaryTable(c, 0, "lX");
            summaryTable(c, 1, "lY");
        }
    }
    bmxLog("inputcheck: RESULT %s: %llu mismatches, %llu HRESULT differences\n",
           persistent == 0 ? "clean" : "MISMATCHES", (unsigned long long)persistent,
           (unsigned long long)gTotalHresult);
}
