/*
 * Checks of the platform stand-ins (src/platform) against Windows itself, as a small hosted program (preset
 * native32, target bmxplatformtest; it does not load the game): time, critical sections, the winmm timer and
 * the file shims. Exit code 0 when all pass.
 */
#include <SDL3/SDL.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

#include "platform/input.h"
#include "platform/standins.h"

static int failures, checks;

#define CHECK(cond, ...)                                                                                     \
    do {                                                                                                     \
        checks++;                                                                                            \
        if (!(cond)) {                                                                                       \
            failures++;                                                                                      \
            printf("FAIL %s:%d: %s  ", __FILE__, __LINE__, #cond);                                           \
            printf(__VA_ARGS__);                                                                             \
            printf("\n");                                                                                    \
        }                                                                                                    \
    } while (0)

/* Provided by the loader in the exe. */
void bmxLog(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

void *bmxRealProc(const char *dll, const char *name)
{
    return (void *)GetProcAddress(LoadLibraryA(dll), name);
}

/* Provided by keys.c in the exe: a scripted keyboard with DIK 0x1e (A) and 0xc8 (up) held. */
static int gScripted;

int bmxKeysScripted(void)
{
    return gScripted;
}

void bmxKeysFillState(uint8_t *state, uint32_t size)
{
    if (size == 256) {
        state[0x1e] = 0x80;
        state[0xc8] = 0x80;
    }
}

/* ---- time ---- */

static void testTime(void)
{
    uint32_t prev, first, lastTick;
    int64_t freq, counter, realFreq;
    uint32_t steps[8] = {0};
    int stepCount = 0;
    const DWORD realStart = GetTickCount();
    const uint32_t ourStart = bmxTimeGetTime();
    uint32_t maxGap = 0;

    bmxPlatformInit();
    /* base: non-zero, and the clocks agree to within one system tick */
    first = bmxTimeGetTime();
    CHECK(first != 0 && first < 0x01000000, "first %u", (unsigned)first);
    CHECK(bmxGetTickCount() <= bmxTimeGetTime() && bmxTimeGetTime() - bmxGetTickCount() <= 16,
          "tick vs time");
    /* monotonic with 1 ms resolution; GetTickCount moves in 15/16 ms steps */
    prev = bmxTimeGetTime();
    lastTick = bmxGetTickCount();
    for (int i = 0; i < 20000; i++) {
        const uint32_t t = bmxTimeGetTime(), k = bmxGetTickCount();

        CHECK(t - prev < 0x80000000u, "timeGetTime went back: %u -> %u", (unsigned)prev, (unsigned)t);
        CHECK(k - lastTick < 0x80000000u, "GetTickCount went back");
        CHECK(k <= t, "tick %u after time %u", (unsigned)k, (unsigned)t);
        if (k != lastTick) {
            const uint32_t d = k - lastTick;

            if (stepCount < 8) {
                steps[stepCount++] = d;
            }
            CHECK(d == 15 || d == 16 || d == 31 || d == 32, "tick step %u", (unsigned)d);
            lastTick = k;
        }
        prev = t;
        SDL_Delay(0);
        if (i % 100 == 0) {
            SDL_Delay(1);
        }
    }
    CHECK(stepCount >= 4, "GetTickCount moved only %d times", stepCount);
    /* against Windows over the same interval */
    {
        const int64_t ours = (int32_t)(bmxTimeGetTime() - ourStart);
        const int64_t real = (int32_t)(GetTickCount() - realStart);

        CHECK(llabs(ours - real) <= 32, "elapsed ours %lld real %lld", (long long)ours, (long long)real);
    }
    /* QueryPerformance*: same frequency and the same counter as the real functions */
    bmxStandQueryPerformanceFrequency(&freq);
    QueryPerformanceFrequency((LARGE_INTEGER *)&realFreq);
    CHECK(freq == realFreq && freq > 0, "freq %lld vs %lld", (long long)freq, (long long)realFreq);
    bmxStandQueryPerformanceCounter(&counter);
    {
        int64_t real;

        QueryPerformanceCounter((LARGE_INTEGER *)&real);
        CHECK(llabs(real - counter) < freq / 100, "counter %lld vs %lld", (long long)counter,
              (long long)real);
    }
    /* wrap: start 200 ms before 2^32 and run through zero */
    bmxTimeSetBase(0);
    prev = bmxTimeGetTime(); /* the milliseconds since the origin */
    bmxTimeSetBase(0u - prev - 200u);
    prev = bmxTimeGetTime();
    CHECK(prev >= 0xffffff37u && prev < 0xffffff80u, "base %#x", (unsigned)prev);
    {
        int wrapped = 0;
        const uint32_t startAt = prev;

        for (int i = 0; i < 400; i++) {
            const uint32_t t = bmxTimeGetTime();

            wrapped |= t < startAt;
            CHECK(t - prev < 0x80000000u, "wrap: %#x -> %#x", (unsigned)prev, (unsigned)t);
            maxGap = t - prev > maxGap ? t - prev : maxGap;
            prev = t;
            SDL_Delay(1);
        }
        CHECK(wrapped, "did not wrap in 400 ms (last %#x)", (unsigned)prev);
        CHECK(maxGap < 50, "gap %u", (unsigned)maxGap);
    }
    bmxTimeSetBase(0x0036ee80);
    /* Sleep: at least the asked time (1 ms resolution here), not wildly more */
    {
        const uint64_t a = SDL_GetTicksNS();

        bmxStandSleep(50);
        const uint64_t ms = (SDL_GetTicksNS() - a) / 1000000;

        CHECK(ms >= 49 && ms < 120, "Sleep(50) took %llu ms", (unsigned long long)ms);
    }
}

/* ---- critical sections ---- */

typedef struct Shared {
    uint8_t section[24];
    long counter;
} Shared;

static int SDLCALL countThread(void *arg)
{
    Shared *const s = arg;

    for (int i = 0; i < 200000; i++) {
        bmxStandEnterCriticalSection(s->section);
        bmxStandEnterCriticalSection(s->section); /* recursive */
        s->counter = s->counter + 1;
        bmxStandLeaveCriticalSection(s->section);
        bmxStandLeaveCriticalSection(s->section);
    }
    return 0;
}

static void testSections(void)
{
    Shared s;
    SDL_Thread *t[3];

    memset(&s, 0xaa, sizeof s);
    bmxStandInitializeCriticalSection(s.section);
    s.counter = 0;
    for (int i = 0; i < 3; i++) {
        t[i] = SDL_CreateThread(countThread, "count", &s);
    }
    for (int i = 0; i < 3; i++) {
        SDL_WaitThread(t[i], NULL);
    }
    CHECK(s.counter == 600000, "counter %ld", s.counter);
    bmxStandDeleteCriticalSection(s.section);
    {
        uint8_t zero[sizeof(void *)] = {0};

        CHECK(memcmp(s.section, zero, sizeof zero) == 0, "deleted section keeps a pointer");
    }
    /* an uninitialised (zeroed) section: Enter / Leave do nothing */
    bmxStandEnterCriticalSection(s.section);
    bmxStandLeaveCriticalSection(s.section);
}

/* ---- winmm timer ---- */

static volatile LONG gCalls, gWrongArgs, gOtherThread, gKillSelf;
static SDL_ThreadID gMain;
static uint32_t gSelfId;

static void BMX_WINAPI tick(uint32_t id, uint32_t msg, uint32_t user, uint32_t a, uint32_t b)
{
    InterlockedIncrement(&gCalls);
    if (id == 0 || msg != 0 || user != 7 || a != 0 || b != 0) {
        InterlockedIncrement(&gWrongArgs);
    }
    if (SDL_GetCurrentThreadID() != gMain) {
        InterlockedIncrement(&gOtherThread);
    }
    SDL_Delay(1); /* the real pump takes well under a millisecond */
}

static void BMX_WINAPI tickKillSelf(uint32_t id, uint32_t msg, uint32_t user, uint32_t a, uint32_t b)
{
    if (InterlockedIncrement(&gKillSelf) == 3) {
        bmxStandTimeKillEvent(gSelfId);
    }
}

static uint32_t gConcurrentId;

static int SDLCALL killThread(void *arg)
{
    (void)arg;
    return (int)bmxStandTimeKillEvent(gConcurrentId);
}

/* The interval statistics of a 16 ms periodic timer, ours and winmm's own, for two seconds each. */
typedef struct Intervals {
    uint64_t last;
    int n;
    double sum, sumSquares, min, max;
} Intervals;

static Intervals gStats;

static void BMX_WINAPI measure(uint32_t id, uint32_t msg, uint32_t user, uint32_t a, uint32_t b)
{
    const uint64_t now = SDL_GetTicksNS();

    if (gStats.last != 0) {
        const double ms = (double)(now - gStats.last) / 1e6;

        gStats.sum += ms;
        gStats.sumSquares += ms * ms;
        gStats.min = gStats.n == 0 || ms < gStats.min ? ms : gStats.min;
        gStats.max = gStats.n == 0 || ms > gStats.max ? ms : gStats.max;
        gStats.n++;
    }
    gStats.last = now;
}

static void compareWithWinmm(void)
{
    typedef uint32_t(WINAPI * SetEventFn)(uint32_t, uint32_t, void *, uintptr_t, uint32_t);
    typedef uint32_t(WINAPI * KillEventFn)(uint32_t);
    typedef uint32_t(WINAPI * PeriodFn)(uint32_t);
    const SetEventFn realSet = (SetEventFn)bmxRealProc("WINMM.dll", "timeSetEvent");
    const KillEventFn realKill = (KillEventFn)bmxRealProc("WINMM.dll", "timeKillEvent");
    const PeriodFn realBegin = (PeriodFn)bmxRealProc("WINMM.dll", "timeBeginPeriod");
    const PeriodFn realEnd = (PeriodFn)bmxRealProc("WINMM.dll", "timeEndPeriod");
    double mean[2], sd[2];

    for (int which = 0; which < 2; which++) {
        uint32_t id;

        memset(&gStats, 0, sizeof gStats);
        /* the game's own sequence: BeginPeriod, SetEvent, EndPeriod */
        if (which == 0) {
            realBegin(1);
            id = realSet(16, 1, (void *)measure, 0, 1);
            realEnd(1);
        } else {
            bmxStandTimeBeginPeriod(1);
            id = bmxStandTimeSetEvent(16, 1, measure, 0, 1);
            bmxStandTimeEndPeriod(1);
        }
        SDL_Delay(3000);
        if (which == 0) {
            realKill(id);
        } else {
            bmxStandTimeKillEvent(id);
        }
        mean[which] = gStats.sum / gStats.n;
        sd[which] = sqrt(gStats.sumSquares / gStats.n - mean[which] * mean[which]);
        printf("%s 16 ms timer: %d intervals, mean %.3f ms, sd %.3f, min %.3f, max %.3f\n",
               which == 0 ? "winmm   " : "stand-in", gStats.n, mean[which], sd[which], gStats.min,
               gStats.max);
    }
    CHECK(fabs(mean[0] - mean[1]) < 0.5 && mean[1] > 15.8 && mean[1] < 16.2, "mean %.3f vs winmm %.3f",
          mean[1], mean[0]);
}

static void testTimer(void)
{
    uint32_t caps[2] = {0, 0};
    uint32_t id;
    uint64_t start;
    long calls;

    gMain = SDL_GetCurrentThreadID();
    CHECK(bmxStandTimeGetDevCaps(caps, 8) == 0 && caps[0] == 1 && caps[1] == 1000000, "caps");
    CHECK(bmxStandTimeGetDevCaps(caps, 4) != 0, "short caps");
    CHECK(bmxStandTimeBeginPeriod(1) == 0 && bmxStandTimeEndPeriod(1) == 0, "period");
    /* 16 ms periodic for one second: 62 or 63 calls, on another thread, with the arguments winmm passes */
    start = SDL_GetTicksNS();
    id = bmxStandTimeSetEvent(16, 1, tick, 7, 1);
    CHECK(id != 0, "timeSetEvent");
    SDL_Delay(1000);
    CHECK(bmxStandTimeKillEvent(id) == 0, "kill");
    calls = gCalls;
    CHECK(calls >= 60 && calls <= 63, "%ld calls in %llu ms", calls,
          (unsigned long long)((SDL_GetTicksNS() - start) / 1000000));
    CHECK(gWrongArgs == 0 && gOtherThread == calls, "args %ld, other thread %ld of %ld", (long)gWrongArgs,
          (long)gOtherThread, calls);
    SDL_Delay(100);
    CHECK(gCalls == calls, "called after timeKillEvent returned (%ld -> %ld)", calls, (long)gCalls);
    CHECK(bmxStandTimeKillEvent(id) != 0, "second kill must fail");
    CHECK(bmxStandTimeKillEvent(9999) != 0, "unknown id");
    /* one-shot fires once; afterwards the event is gone for timeKillEvent, as in winmm */
    gCalls = 0;
    id = bmxStandTimeSetEvent(20, 1, tick, 7, 0);
    SDL_Delay(120);
    CHECK(gCalls == 1, "one-shot fired %ld times", (long)gCalls);
    {
        typedef uint32_t(WINAPI * SetEventFn)(uint32_t, uint32_t, void *, uintptr_t, uint32_t);
        typedef uint32_t(WINAPI * KillEventFn)(uint32_t);
        const SetEventFn realSet = (SetEventFn)bmxRealProc("WINMM.dll", "timeSetEvent");
        const KillEventFn realKill = (KillEventFn)bmxRealProc("WINMM.dll", "timeKillEvent");
        const uint32_t realId = realSet(20, 1, (void *)tick, 7, 0);
        uint32_t real;

        SDL_Delay(120);
        real = realKill(realId);
        CHECK(bmxStandTimeKillEvent(id) == real, "kill of a fired one-shot: winmm %u", (unsigned)real);
        real = realKill(realId);
        CHECK(bmxStandTimeKillEvent(id) == real, "second kill: winmm %u", (unsigned)real);
        real = realKill(9999);
        CHECK(bmxStandTimeKillEvent(9999) == real, "unknown id: winmm %u", (unsigned)real);
    }
    /* two threads killing the same event: one joins, the other gets an error, the slot is freed once */
    for (int i = 0; i < 20; i++) {
        SDL_Thread *other;
        int otherResult;
        uint32_t mine;

        gConcurrentId = bmxStandTimeSetEvent(2, 1, tick, 7, 1);
        SDL_Delay(5);
        other = SDL_CreateThread(killThread, "kill", NULL);
        mine = bmxStandTimeKillEvent(gConcurrentId);
        SDL_WaitThread(other, &otherResult);
        CHECK((mine == 0) != (otherResult == 0), "concurrent kill %d: %u and %d", i, (unsigned)mine,
              otherResult);
    }
    /* killing from the callback itself ends the event (and does not deadlock) */
    gKillSelf = 0;
    gSelfId = bmxStandTimeSetEvent(10, 1, tickKillSelf, 0, 1);
    SDL_Delay(150);
    CHECK(gKillSelf == 3, "self kill: %ld calls", (long)gKillSelf);
    /* the 8 slots are reusable */
    for (int i = 0; i < 20; i++) {
        const uint32_t e = bmxStandTimeSetEvent(5, 1, tick, 7, 1);

        CHECK(e != 0, "slot reuse %d", i);
        SDL_Delay(8);
        bmxStandTimeKillEvent(e);
    }
}

/* ---- input: DirectInput arithmetic, button tables, keyboard, the stand-in objects, the config file ---- */

static const BmxDiAxisProps kGameStick = {-127, 127, 1000, 10000}; /* what pcinput.c sets on lX / lY */

/*
 * lX / lY pairs {raw HID value, DirectInput's value} from the user's BMX_INPUT_CHECK runs with a DualSense
 * (USB), the game's range -127..127 and dead zone 10%. One logged pair, raw 146 -> 5, is left out: no integer
 * raw value gives 5 under this mapping (145 -> 4, 146 -> 6) and it was logged while the stick moved 9 steps
 * between two polls, so the two reads most likely saw different reports. The still-stick table of the next
 * check run settles it.
 */
static const int32_t kDualSenseMeasured[][2] = {
    {52, -70},  {54, -67},  {71, -49}, {73, -46}, {93, -24}, {94, -23}, {95, -22}, {99, -18},  {100, -17},
    {101, -15}, {105, -11}, {108, -8}, {113, -2}, {114, -1}, {115, 0},  {116, 0},  {119, 0},   {121, 0},
    {124, 0},   {129, 0},   {131, 0},  {132, 0},  {133, 0},  {134, 0},  {135, 0},  {136, 0},   {137, 0},
    {138, 0},   {139, 0},   {141, 0},  {142, 1},  {143, 2},  {144, 3},  {145, 4},  {148, 8},   {153, 13},
    {154, 14},  {158, 19},  {166, 28}, {176, 39}, {178, 41}, {179, 42}, {192, 57}, {235, 105}, {238, 108},
};

static void testDiArithmetic(void)
{
    uint32_t seed = 12345;
    int32_t previous = -128;

    /* MulDiv against kernel32's own */
    for (int i = 0; i < 200000; i++) {
        int32_t a, b, c;

        seed = seed * 1664525u + 1013904223u;
        a = (int32_t)(seed >> 8) % 70000 - 35000;
        seed = seed * 1664525u + 1013904223u;
        b = (int32_t)(seed >> 8) % 20001 - 10000;
        seed = seed * 1664525u + 1013904223u;
        c = (int32_t)(seed >> 8) % 70000 - 35000;
        if (c == 0) {
            c = 1;
        }
        CHECK(bmxDiMulDiv(a, b, c) == MulDiv(a, b, c), "MulDiv(%d, %d, %d) = %d, kernel32 %d", (int)a, (int)b,
              (int)c, (int)bmxDiMulDiv(a, b, c), (int)MulDiv(a, b, c));
    }
    CHECK(bmxDiMulDiv(5, 5, 0) == -1, "MulDiv by 0");
    /* an 8-bit stick (DualSense) with the game's range and dead zone: ends, centre, monotonic, thresholds */
    CHECK(bmxDiCookAxis(0, 255, &kGameStick) == -127 && bmxDiCookAxis(255, 255, &kGameStick) == 127, "ends");
    CHECK(bmxDiCookAxis(127, 255, &kGameStick) == 0 && bmxDiCookAxis(128, 255, &kGameStick) == 0, "centre");
    for (int raw = 0; raw <= 255; raw++) {
        const int32_t v = bmxDiCookAxis(raw, 255, &kGameStick);

        CHECK(v >= previous && v >= -127 && v <= 127, "monotonic at %d: %d after %d", raw, (int)v,
              (int)previous);
        previous = v;
    }
    /* The centre is 128; 10% of each half (12.7 / 12.8 raw steps, MulDiv-rounded to 13) is dead, the next
     * step is +-1. DirectInput's values measured with the user's DualSense (BMX_INPUT_CHECK, 2026-10-31). */
    CHECK(bmxDiCookAxis(141, 255, &kGameStick) == 0 && bmxDiCookAxis(142, 255, &kGameStick) == 1,
          "+ threshold: %d %d", (int)bmxDiCookAxis(141, 255, &kGameStick),
          (int)bmxDiCookAxis(142, 255, &kGameStick));
    CHECK(bmxDiCookAxis(115, 255, &kGameStick) == 0 && bmxDiCookAxis(114, 255, &kGameStick) == -1,
          "- threshold: %d %d", (int)bmxDiCookAxis(115, 255, &kGameStick),
          (int)bmxDiCookAxis(114, 255, &kGameStick));
    for (size_t i = 0; i < sizeof kDualSenseMeasured / sizeof kDualSenseMeasured[0]; i++) {
        const int32_t v = bmxDiCookAxis(kDualSenseMeasured[i][0], 255, &kGameStick);

        CHECK(v == kDualSenseMeasured[i][1], "raw %d: ours %d, DirectInput %d", (int)kDualSenseMeasured[i][0],
              (int)v, (int)kDualSenseMeasured[i][1]);
    }
    /* 16-bit (XInput pads): the same 10% */
    CHECK(bmxDiCookAxis(0, 65535, &kGameStick) == -127 && bmxDiCookAxis(65535, 65535, &kGameStick) == 127 &&
              bmxDiCookAxis(32767, 65535, &kGameStick) == 0,
          "16-bit ends");
    CHECK(bmxDiCookAxis(32767 + 3277, 65535, &kGameStick) == 0 &&
              bmxDiCookAxis(32767 + 3500, 65535, &kGameStick) > 0,
          "16-bit threshold");
    /* defaults: 0..65535, no dead zone */
    {
        BmxDiAxisProps d;

        bmxDiAxisDefaults(&d);
        CHECK(bmxDiCookAxis(0, 255, &d) == 0 && bmxDiCookAxis(255, 255, &d) == 65535, "default range");
    }
    /* SDL's HIDAPI value r * 257 - 32768 goes back to r */
    for (int r = 0; r <= 255; r++) {
        int32_t max;

        CHECK(bmxPadRawStick(kBmxPadDualSense, (int16_t)(r * 257 - 32768), &max) == r && max == 255, "raw %d",
              r);
    }
}

static BmxPadSnapshot padWith(BmxPadFamily family, uint64_t buttons)
{
    BmxPadSnapshot p;

    memset(&p, 0, sizeof p);
    p.family = family;
    p.gamepadButtons = buttons;
    return p;
}

static int pressedButtons(const BmxDiJoyState *s, char *out)
{
    int n = 0;

    out[0] = 0;
    for (int b = 0; b < 32; b++) {
        if (s->buttons[b] & 0x80) {
            sprintf(out + strlen(out), "%s%d", n++ ? "," : "", b);
        }
    }
    return n;
}

typedef struct ButtonCase {
    int sdl, di;
} ButtonCase;

static void testPadTables(void)
{
    /* DualSense: Square 0, Cross 1, Circle 2, Triangle 3, L1 4, R1 5, L2 6, R2 7, Create 8, Options 9, L3 10,
     * R3 11, PS 12, touchpad 13, mute 14 */
    static const ButtonCase kSony[] = {
        {SDL_GAMEPAD_BUTTON_WEST, 0},          {SDL_GAMEPAD_BUTTON_SOUTH, 1},
        {SDL_GAMEPAD_BUTTON_EAST, 2},          {SDL_GAMEPAD_BUTTON_NORTH, 3},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 4}, {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 5},
        {SDL_GAMEPAD_BUTTON_BACK, 8},          {SDL_GAMEPAD_BUTTON_START, 9},
        {SDL_GAMEPAD_BUTTON_LEFT_STICK, 10},   {SDL_GAMEPAD_BUTTON_RIGHT_STICK, 11},
        {SDL_GAMEPAD_BUTTON_GUIDE, 12},        {SDL_GAMEPAD_BUTTON_TOUCHPAD, 13},
        {SDL_GAMEPAD_BUTTON_MISC1, 14},
    };
    /* Xbox: A 0, B 1, X 2, Y 3, LB 4, RB 5, Back 6, Start 7, LS 8, RS 9; the guide button is not reported */
    static const ButtonCase kXbox[] = {
        {SDL_GAMEPAD_BUTTON_SOUTH, 0},         {SDL_GAMEPAD_BUTTON_EAST, 1},
        {SDL_GAMEPAD_BUTTON_WEST, 2},          {SDL_GAMEPAD_BUTTON_NORTH, 3},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 4}, {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 5},
        {SDL_GAMEPAD_BUTTON_BACK, 6},          {SDL_GAMEPAD_BUTTON_START, 7},
        {SDL_GAMEPAD_BUTTON_LEFT_STICK, 8},    {SDL_GAMEPAD_BUTTON_RIGHT_STICK, 9},
    };
    BmxDiAxisProps props[kDiAxes];
    BmxDiJoyState s;
    BmxPadSnapshot p;
    char got[128];

    for (int i = 0; i < kDiAxes; i++) {
        bmxDiAxisDefaults(&props[i]);
    }
    props[kDiAxisX] = props[kDiAxisY] = kGameStick;
    for (size_t i = 0; i < sizeof kSony / sizeof kSony[0]; i++) {
        p = padWith(kBmxPadDualSense, 1ull << kSony[i].sdl);
        bmxPadToJoyState(&p, props, &s);
        CHECK(pressedButtons(&s, got) == 1 && (int)strtol(got, NULL, 10) == kSony[i].di,
              "DualSense SDL %d -> %s", kSony[i].sdl, got);
    }
    p = padWith(kBmxPadPlayStation, 1ull << SDL_GAMEPAD_BUTTON_MISC1);
    bmxPadToJoyState(&p, props, &s);
    CHECK(pressedButtons(&s, got) == 0, "DualShock 4 has no mute button: %s", got);
    p = padWith(kBmxPadDualSense, 0);
    p.leftTrigger = 1;
    p.rightTrigger = 32767;
    bmxPadToJoyState(&p, props, &s);
    CHECK(pressedButtons(&s, got) == 2 && strcmp(got, "6,7") == 0, "triggers -> %s", got);
    CHECK(s.axis[kDiAxisRy] == 65535, "R2 axis %d", (int)s.axis[kDiAxisRy]);
    for (size_t i = 0; i < sizeof kXbox / sizeof kXbox[0]; i++) {
        p = padWith(kBmxPadXbox, 1ull << kXbox[i].sdl);
        bmxPadToJoyState(&p, props, &s);
        CHECK(pressedButtons(&s, got) == 1 && (int)strtol(got, NULL, 10) == kXbox[i].di, "Xbox SDL %d -> %s",
              kXbox[i].sdl, got);
    }
    p = padWith(kBmxPadXbox, 1ull << SDL_GAMEPAD_BUTTON_GUIDE);
    bmxPadToJoyState(&p, props, &s);
    CHECK(pressedButtons(&s, got) == 0, "Xbox guide: %s", got);
    /* generic: raw order, hat 0 */
    p = padWith(kBmxPadGeneric, 0);
    p.buttonCount = 12;
    p.buttons[0] = p.buttons[11] = 1;
    p.axisCount = 2;
    p.axes[0] = 32767;
    p.axes[1] = -32768;
    p.hat = SDL_HAT_RIGHTUP;
    bmxPadToJoyState(&p, props, &s);
    CHECK(pressedButtons(&s, got) == 2 && strcmp(got, "0,11") == 0, "generic buttons %s", got);
    CHECK(s.axis[kDiAxisX] == 127 && s.axis[kDiAxisY] == -127, "generic axes %d %d", (int)s.axis[0],
          (int)s.axis[1]);
    CHECK(s.pov[0] == 4500 && s.pov[1] == 0xffffffffu, "POV %u", (unsigned)s.pov[0]);
    /* the stick: centred 0, full deflection +-127; the d-pad is only the POV */
    p = padWith(kBmxPadDualSense, 1ull << SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    p.leftY = 32767;
    bmxPadToJoyState(&p, props, &s);
    CHECK(s.axis[kDiAxisX] == 0 && s.axis[kDiAxisY] == 127 && s.pov[0] == 27000, "stick %d %d pov %u",
          (int)s.axis[0], (int)s.axis[1], (unsigned)s.pov[0]);
    /* d-pad steering (P7): left wins over the centred X, Y keeps the stick; up + right on a resting stick */
    CHECK(bmxPadApplyDpad(bmxPadDpad(&p), props, &s) == 1 && s.axis[kDiAxisX] == -127 &&
              s.axis[kDiAxisY] == 127,
          "d-pad left: %d %d", (int)s.axis[0], (int)s.axis[1]);
    p = padWith(kBmxPadDualSense, 1ull << SDL_GAMEPAD_BUTTON_DPAD_UP | 1ull << SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    bmxPadToJoyState(&p, props, &s);
    CHECK(bmxPadApplyDpad(bmxPadDpad(&p), props, &s) == 1 && s.axis[kDiAxisX] == 127 &&
              s.axis[kDiAxisY] == -127,
          "d-pad up-right: %d %d", (int)s.axis[0], (int)s.axis[1]);
    p = padWith(kBmxPadDualSense, 0);
    bmxPadToJoyState(&p, props, &s);
    CHECK(bmxPadApplyDpad(bmxPadDpad(&p), props, &s) == 0, "no d-pad, no change");
}

typedef struct KeyCase {
    int scancode, vk;
} KeyCase;

static void testKeyTable(void)
{
    /* layout-independent keys: the scan code Windows gives for the virtual key (E0 prefix -> + 0x80). Windows
     * reports the navigation block (arrows, Home .. Delete) without the E0 prefix, as their keypad twins:
     * those are the extended keys, + 0x80 too. */
    static const KeyCase kKeys[] = {
        {SDL_SCANCODE_ESCAPE, VK_ESCAPE},
        {SDL_SCANCODE_RETURN, VK_RETURN},
        {SDL_SCANCODE_SPACE, VK_SPACE},
        {SDL_SCANCODE_TAB, VK_TAB},
        {SDL_SCANCODE_BACKSPACE, VK_BACK},
        {SDL_SCANCODE_LSHIFT, VK_LSHIFT},
        {SDL_SCANCODE_RSHIFT, VK_RSHIFT},
        {SDL_SCANCODE_LCTRL, VK_LCONTROL},
        {SDL_SCANCODE_RCTRL, VK_RCONTROL},
        {SDL_SCANCODE_LALT, VK_LMENU},
        {SDL_SCANCODE_RALT, VK_RMENU},
        {SDL_SCANCODE_UP, VK_UP},
        {SDL_SCANCODE_DOWN, VK_DOWN},
        {SDL_SCANCODE_LEFT, VK_LEFT},
        {SDL_SCANCODE_RIGHT, VK_RIGHT},
        {SDL_SCANCODE_HOME, VK_HOME},
        {SDL_SCANCODE_END, VK_END},
        {SDL_SCANCODE_PAGEUP, VK_PRIOR},
        {SDL_SCANCODE_PAGEDOWN, VK_NEXT},
        {SDL_SCANCODE_INSERT, VK_INSERT},
        {SDL_SCANCODE_DELETE, VK_DELETE},
        {SDL_SCANCODE_F1, VK_F1},
        {SDL_SCANCODE_F10, VK_F10},
        {SDL_SCANCODE_F11, VK_F11},
        {SDL_SCANCODE_F12, VK_F12},
        {SDL_SCANCODE_KP_0, VK_NUMPAD0},
        {SDL_SCANCODE_KP_9, VK_NUMPAD9},
        {SDL_SCANCODE_KP_DIVIDE, VK_DIVIDE},
        {SDL_SCANCODE_KP_MULTIPLY, VK_MULTIPLY},
        {SDL_SCANCODE_KP_MINUS, VK_SUBTRACT},
        {SDL_SCANCODE_KP_PLUS, VK_ADD},
        {SDL_SCANCODE_KP_PERIOD, VK_DECIMAL},
        {SDL_SCANCODE_LGUI, VK_LWIN},
        {SDL_SCANCODE_RGUI, VK_RWIN},
        {SDL_SCANCODE_APPLICATION, VK_APPS},
        {SDL_SCANCODE_CAPSLOCK, VK_CAPITAL},
        {SDL_SCANCODE_SCROLLLOCK, VK_SCROLL},
    };
    int seen[256] = {0};

    for (size_t i = 0; i < sizeof kKeys / sizeof kKeys[0]; i++) {
        const UINT sc = MapVirtualKeyA((UINT)kKeys[i].vk, MAPVK_VK_TO_VSC_EX);
        const int navigation = kKeys[i].vk >= VK_PRIOR && kKeys[i].vk <= VK_DELETE;
        const int dik = (int)(sc & 0xff) | ((sc >> 8) == 0xe0 || navigation ? 0x80 : 0);

        CHECK(bmxDikFromScancode(kKeys[i].scancode) == dik,
              "scancode %d (VK 0x%x): ours 0x%02x, Windows 0x%02x", kKeys[i].scancode, kKeys[i].vk,
              bmxDikFromScancode(kKeys[i].scancode), dik);
    }
    /* the game's own keys (QWERTY names, physical positions): E S F D W R 3 4, keypad Enter */
    CHECK(bmxDikFromScancode(SDL_SCANCODE_E) == 0x12 && bmxDikFromScancode(SDL_SCANCODE_S) == 0x1f &&
              bmxDikFromScancode(SDL_SCANCODE_F) == 0x21 && bmxDikFromScancode(SDL_SCANCODE_D) == 0x20 &&
              bmxDikFromScancode(SDL_SCANCODE_W) == 0x11 && bmxDikFromScancode(SDL_SCANCODE_R) == 0x13 &&
              bmxDikFromScancode(SDL_SCANCODE_3) == 0x04 && bmxDikFromScancode(SDL_SCANCODE_4) == 0x05 &&
              bmxDikFromScancode(SDL_SCANCODE_KP_ENTER) == 0x9c,
          "game keys");
    /* no two keys share a DIK, except the ISO key left of Enter (same scan code as backslash) */
    for (int s = 0; s < SDL_SCANCODE_COUNT; s++) {
        const int dik = bmxDikFromScancode(s);

        if (dik != 0 && s != SDL_SCANCODE_NONUSHASH) {
            CHECK(!seen[dik], "DIK 0x%02x twice (scancode %d)", dik, s);
            seen[dik] = 1;
        }
    }
}

static BmxDi *createDi(void)
{
    static const BmxGuid kIidDirectInput7 = {
        0x9a4cb684, 0x236d, 0x11d3, {0x8e, 0x9d, 0, 0xc0, 0x4f, 0x68, 0x44, 0xae}};
    void *di = NULL;

    CHECK(bmxStandDirectInputCreateEx(GetModuleHandleA(NULL), 0x700, &kIidDirectInput7, &di, NULL) == 0 &&
              di != NULL,
          "DirectInputCreateEx");
    return di;
}

static int gEnumCalls;

static uint32_t BMX_WINAPI countJoysticks(const BmxDiDeviceInstance *instance, void *ref)
{
    (void)instance;
    (void)ref;
    gEnumCalls++;
    return 0;
}

static void testInputObjects(void)
{
    /* c_dfDIKeyboard: only dwDataSize (256) is read */
    static const BmxDiDataFormat kKeyboardFormat = {24, 16, 2, 256, 0, NULL};
    static const BmxGuid kNoJoystick = {0};
    const BmxDiPropDword deadZone = {{20, 16, 0, 0}, 1000};
    uint8_t state[256];
    BmxDiDevice *kbd = NULL, *pad = NULL;
    BmxDi *di;

    /* Not focused (no window at all): the keyboard cannot be acquired, nothing is read, the buffer stays */
    gScripted = 0;
    bmxInputReset();
    di = createDi();
    CHECK(di->vtbl->createDeviceEx(di, &kBmxGuidSysKeyboard, NULL, &kbd, NULL) == 0 && kbd != NULL,
          "keyboard");
    if (kbd == NULL) {
        return;
    }
    CHECK(kbd->vtbl->acquire(kbd) == kDiErrInvalidParam, "acquire before the data format");
    CHECK(kbd->vtbl->setDataFormat(kbd, &kKeyboardFormat) == 0, "format");
    CHECK(kbd->vtbl->setCooperativeLevel(kbd, NULL, 6) == 0, "foreground, non-exclusive");
    CHECK(kbd->vtbl->setCooperativeLevel(kbd, NULL, 3) == kDiErrInvalidParam, "exclusive and non-exclusive");
    CHECK(kbd->vtbl->acquire(kbd) == kDiErrOtherAppHasPrio, "acquire without the focus");
    memset(state, 0x55, sizeof state);
    CHECK(kbd->vtbl->getDeviceState(kbd, 256, state) == kDiErrNotAcquired && state[0] == 0x55,
          "not acquired");
    CHECK(kbd->vtbl->unacquire(kbd) == kDiNoEffect, "unacquire");
    CHECK(kbd->vtbl->release(kbd) == 0, "release");
    gEnumCalls = 0;
    CHECK(di->vtbl->enumDevices(di, 4, countJoysticks, NULL, 1) == 0, "enum");
    printf("input: %d pad(s) listed\n", gEnumCalls);
    di->vtbl->release(di);

    /* BMX_KEYS: the script is the keyboard, focus ignored, no pad */
    gScripted = 1;
    bmxInputReset();
    di = createDi();
    gEnumCalls = 0;
    CHECK(di->vtbl->enumDevices(di, 4, countJoysticks, NULL, 1) == 0 && gEnumCalls == 0, "scripted: no pad");
    CHECK(di->vtbl->createDeviceEx(di, &kNoJoystick, NULL, &pad, NULL) == kDiErrDeviceNotReg && pad == NULL,
          "no joystick GUID");
    CHECK(di->vtbl->createDeviceEx(di, &kBmxGuidSysKeyboard, NULL, &kbd, NULL) == 0, "keyboard");
    CHECK(kbd->vtbl->setDataFormat(kbd, &kKeyboardFormat) == 0 &&
              kbd->vtbl->setCooperativeLevel(kbd, NULL, 6) == 0,
          "setup");
    CHECK(kbd->vtbl->getDeviceState(kbd, 256, state) == kDiErrNotAcquired, "read before acquire");
    CHECK(kbd->vtbl->acquire(kbd) == 0, "acquire");
    CHECK(kbd->vtbl->acquire(kbd) == kDiNoEffect, "acquire again");
    memset(state, 0x55, sizeof state);
    CHECK(kbd->vtbl->getDeviceState(kbd, 256, state) == 0 && state[0x1e] == 0x80 && state[0xc8] == 0x80 &&
              state[0] == 0 && state[0x1f] == 0,
          "scripted state");
    CHECK(kbd->vtbl->getDeviceState(kbd, 255, state) == kDiErrInvalidParam, "wrong size");
    CHECK(kbd->vtbl->setDataFormat(kbd, &kKeyboardFormat) == kDiErrAcquired, "format while acquired");
    CHECK(kbd->vtbl->setProperty(kbd, kDiPropDeadZone, &deadZone.header) == kDiErrNotFound,
          "keyboard has no axes");
    CHECK(kbd->vtbl->unacquire(kbd) == 0, "unacquire");
    CHECK(kbd->vtbl->release(kbd) == 0, "release");
    di->vtbl->release(di);
    gScripted = 0;
    bmxInputReset();
}

/*
 * BMX_INPUT_CHECK against the real dinput.dll, without the focus: the game's own call sequence (pcinput.c) on
 * the stand-in, with a hidden window. Every HRESULT must match (unfocused, Acquire fails on both sides); with
 * a pad attached this also covers the four SetProperty calls (the last one is the game's wrong one) and
 * GetDeviceInfo (the log shows DirectInput's product name and GUID next to ours).
 */
static GUID gGameJoystick;

static uint32_t BMX_WINAPI rememberJoystick(const BmxDiDeviceInstance *instance, void *ref)
{
    memcpy(ref, &instance->guidInstance, sizeof(GUID));
    return 0;
}

static void testInputCheck(void)
{
    static const BmxGuid kIidDevice7 = {
        0x57d7c6bc, 0x2356, 0x11d3, {0x8e, 0x9d, 0, 0xc0, 0x4f, 0x68, 0x44, 0xae}};
    const HWND window = CreateWindowExA(0, "STATIC", "bmxplatformtest", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64,
                                        NULL, NULL, GetModuleHandleA(NULL), NULL); /* never shown */
    BmxDiDevice *kbd = NULL, *pad = NULL;
    uint8_t keys[256];
    BmxDiJoyState joy;
    BmxDi *di;

    _putenv("BMX_INPUT_CHECK=1"); /* getenv reads the CRT's copy */
    gScripted = 0;
    bmxInputReset();
    di = createDi();
    CHECK(bmxCheckEnabled(), "check on");
    memset(&gGameJoystick, 0, sizeof gGameJoystick);
    di->vtbl->enumDevices(di, 4, rememberJoystick, &gGameJoystick, 1);
    CHECK(di->vtbl->createDeviceEx(di, &kBmxGuidSysKeyboard, &kIidDevice7, &kbd, NULL) == 0, "keyboard");
    kbd->vtbl->setDataFormat(kbd, (const BmxDiDataFormat *)&c_dfDIKeyboard);
    kbd->vtbl->setCooperativeLevel(kbd, window, 6);
    kbd->vtbl->acquire(kbd);
    kbd->vtbl->getDeviceState(kbd, sizeof keys, keys);
    if (di->vtbl->createDeviceEx(di, (const BmxGuid *)&gGameJoystick, &kIidDevice7, &pad, NULL) == 0) {
        BmxDiPropRange range = {{0x18, 0x10, 0, 1}, -127, 127};
        BmxDiPropDword deadZone = {{0x14, 0x10, 0, 0}, 1000};
        BmxDiDeviceInstance info;

        pad->vtbl->setDataFormat(pad, (const BmxDiDataFormat *)&c_dfDIJoystick);
        pad->vtbl->setCooperativeLevel(pad, window, 5);
        pad->vtbl->setProperty(pad, kDiPropRange, &range.header);
        range.header.obj = 4;
        pad->vtbl->setProperty(pad, kDiPropRange, &range.header);
        pad->vtbl->setProperty(pad, kDiPropDeadZone, &deadZone.header);
        deadZone.header.obj = 4;
        pad->vtbl->setProperty(pad, kDiPropRange, &deadZone.header);
        memset(&info, 0, sizeof info);
        info.size = sizeof info;
        pad->vtbl->getDeviceInfo(pad, &info);
        pad->vtbl->acquire(pad);
        pad->vtbl->poll(pad);
        pad->vtbl->getDeviceState(pad, sizeof joy, &joy);
        pad->vtbl->unacquire(pad);
        pad->vtbl->release(pad);
    } else {
        printf("inputcheck test: no pad attached, keyboard only\n");
    }
    kbd->vtbl->unacquire(kbd);
    kbd->vtbl->release(kbd);
    di->vtbl->release(di);
    CHECK(bmxCheckDifferences() == 0, "%llu differences from the real DirectInput",
          (unsigned long long)bmxCheckDifferences());
    bmxCheckSummary();
    _putenv("BMX_INPUT_CHECK=");
    bmxInputReset();
    DestroyWindow(window);
}

/*
 * Two exclusive device objects on the same pad, as the game creates them at a level load: both acquire
 * (DI_OK, then S_FALSE when asked again) and both read, against the real dinput.dll under BMX_INPUT_CHECK.
 * Background exclusive (9) instead of the game's foreground exclusive (5), so no focus is needed; exclusivity
 * is the same.
 */
static void testTwoPads(void)
{
    static const BmxGuid kIidDevice7 = {
        0x57d7c6bc, 0x2356, 0x11d3, {0x8e, 0x9d, 0, 0xc0, 0x4f, 0x68, 0x44, 0xae}};
    const HWND window = CreateWindowExA(0, "STATIC", "bmxplatformtest", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64,
                                        NULL, NULL, GetModuleHandleA(NULL), NULL); /* never shown */
    BmxDiDevice *pads[2] = {NULL, NULL};
    BmxDi *di;
    int created = 0;

    _putenv("BMX_INPUT_CHECK=1");
    gScripted = 0;
    bmxInputReset();
    di = createDi();
    memset(&gGameJoystick, 0, sizeof gGameJoystick);
    di->vtbl->enumDevices(di, 4, rememberJoystick, &gGameJoystick, 1);
    for (int i = 0; i < 2; i++) {
        if (di->vtbl->createDeviceEx(di, (const BmxGuid *)&gGameJoystick, &kIidDevice7, &pads[i], NULL) ==
            0) {
            BmxDiPropRange range = {{0x18, 0x10, 0, 1}, -127, 127};
            BmxDiPropDword deadZone = {{0x14, 0x10, 0, 0}, 1000};

            created++;
            pads[i]->vtbl->setDataFormat(pads[i], (const BmxDiDataFormat *)&c_dfDIJoystick);
            pads[i]->vtbl->setCooperativeLevel(pads[i], window, 9);
            pads[i]->vtbl->setProperty(pads[i], kDiPropRange, &range.header);
            range.header.obj = 4;
            pads[i]->vtbl->setProperty(pads[i], kDiPropRange, &range.header);
            pads[i]->vtbl->setProperty(pads[i], kDiPropDeadZone, &deadZone.header);
        }
    }
    if (created == 2) {
        for (int i = 0; i < 2; i++) {
            CHECK(pads[i]->vtbl->acquire(pads[i]) == kDiOk, "pad object %d: first Acquire", i);
        }
        for (int i = 0; i < 2; i++) {
            BmxDiJoyState joy;

            CHECK(pads[i]->vtbl->acquire(pads[i]) == kDiNoEffect, "pad object %d: Acquire again", i);
            pads[i]->vtbl->poll(pads[i]);
            CHECK(pads[i]->vtbl->getDeviceState(pads[i], sizeof joy, &joy) == kDiOk, "pad object %d reads",
                  i);
            pads[i]->vtbl->poll(pads[i]);
            CHECK(pads[i]->vtbl->getDeviceState(pads[i], sizeof joy, &joy) == kDiOk, "pad object %d reads",
                  i);
        }
        for (int i = 0; i < 2; i++) {
            pads[i]->vtbl->unacquire(pads[i]);
            pads[i]->vtbl->release(pads[i]);
        }
        CHECK(bmxCheckDifferences() == 0, "two pads: %llu differences from the real DirectInput",
              (unsigned long long)bmxCheckDifferences());
        bmxCheckSummary();
    } else {
        printf("two pads test: no pad attached, skipped\n");
        for (int i = 0; i < created; i++) {
            pads[i]->vtbl->release(pads[i]);
        }
    }
    di->vtbl->release(di);
    _putenv("BMX_INPUT_CHECK=");
    bmxInputReset();
    DestroyWindow(window);
}

static void testConfig(void)
{
    bmxConfigLoadText("# bmxnative.ini\n[input]\r\n  dpad_steering = 1 ; on\nnonsense\nunknown = 3\n",
                      "test");
    CHECK(bmxConfigInt("input.dpad_steering", 0) == 1, "dpad_steering");
    CHECK(bmxConfigInt("input.unknown", 7) == 7, "unknown keys are dropped");
    bmxConfigLoadText("dpad_steering = 1\n[input]\ndpad_steering = 0x0\n", "test");
    CHECK(bmxConfigInt("input.dpad_steering", 5) == 0, "hex, section prefix");
    bmxConfigLoadText("[input]\ndpad_steering = yes\n", "test");
    CHECK(bmxConfigInt("input.dpad_steering", 0) == 0, "not a number -> default");
    bmxConfigLoadText("", "test");
    CHECK(bmxConfigInt("input.dpad_steering", 0) == 0, "empty");
}

/* ---- files ---- */

static void testFiles(void)
{
    char path[MAX_PATH];
    FILE *f;
    uint8_t *data = malloc(300000), *got = malloc(300000);
    uint32_t seed = 12345;
    void *h;
    uint32_t read;
    uint32_t pos;

    GetTempPathA(sizeof path, path);
    SDL_strlcat(path, "bmxplatformtest.bin", sizeof path);
    for (int i = 0; i < 300000; i++) {
        seed = seed * 1103515245u + 12345u;
        data[i] = (uint8_t)(seed >> 16);
    }
    f = fopen(path, "wb");
    fwrite(data, 1, 300000, f);
    fclose(f);

    CHECK(bmxPathResolve(path) == path, "identity");
    h = bmxStandCreateFileA(path, 0x80000000u, 1, NULL, 3, 0x08000000 | 0x80, NULL);
    CHECK(h != (void *)(intptr_t)-1, "open");
    /* winload: size from FILE_END, back to the start, header, then the whole file */
    CHECK(bmxStandSetFilePointer(h, 0, NULL, 2) == 300000, "size");
    CHECK(bmxStandSetFilePointer(h, 0, NULL, 0) == 0, "rewind");
    CHECK(bmxStandReadFile(h, got, 16, &read, NULL) == 1 && read == 16 && memcmp(got, data, 16) == 0,
          "header");
    CHECK(bmxStandSetFilePointer(h, 0, NULL, 0) == 0, "rewind 2");
    CHECK(bmxStandReadFile(h, got, 300000, &read, NULL) == 1 && read == 300000 &&
              memcmp(got, data, 300000) == 0,
          "whole file");
    /* the end of the file: success with 0 bytes */
    CHECK(bmxStandReadFile(h, got, 10, &read, NULL) == 1 && read == 0, "read at end: %u", (unsigned)read);
    /* winstrm: seek to an offset, read a chunk, short read at the end, relative and negative seeks */
    pos = bmxStandSetFilePointer(h, 299990, NULL, 0);
    CHECK(pos == 299990, "seek");
    CHECK(bmxStandReadFile(h, got, 100, &read, NULL) == 1 && read == 10 &&
              memcmp(got, data + 299990, 10) == 0,
          "short read");
    CHECK(bmxStandSetFilePointer(h, -50, NULL, 1) == 299950, "relative");
    CHECK(bmxStandSetFilePointer(h, -20, NULL, 2) == 299980, "from end");
    {
        int32_t high = 0;

        CHECK(bmxStandSetFilePointer(h, 1000, &high, 0) == 1000 && high == 0, "high part");
    }
    CHECK(bmxStandSetFilePointer(h, -1, NULL, 0) == 0xffffffffu, "negative seek fails");
    CHECK(bmxStandCloseHandle(h) == 1, "close");
    CHECK(bmxStandCreateFileA("no\\such\\file.zsd", 0x80000000u, 1, NULL, 3, 0, NULL) == (void *)(intptr_t)-1,
          "missing file");
    CHECK(bmxStandCloseHandle((void *)(intptr_t)-1) == 0, "close invalid");
    DeleteFileA(path);
    free(data);
    free(got);
}

int main(void)
{
    SetEnvironmentVariableA("BMX_FILE_CHECK", "1"); /* every call above is also made through kernel32 */
    /* A pad the user is playing with elsewhere is only read (no HIDAPI driver: it would set the lights). */
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
    testTime();
    testSections();
    testTimer();
    compareWithWinmm();
    testFiles();
    testDiArithmetic();
    testPadTables();
    testKeyTable();
    testInputObjects();
    testInputCheck();
    testTwoPads();
    testConfig();
    bmxPlatformShutdown();
    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
