/*
 * Checks of the platform stand-ins (src/platform) against Windows itself, as a small hosted program (preset
 * native32, target bmxplatformtest; it does not load the game): time, critical sections, the winmm timer, the
 * file shims, DirectInput and DirectSound. Exit code 0 when all pass.
 */
#include <SDL3/SDL.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#include <windows.h>

#include <audiopolicy.h>
#include <mmdeviceapi.h>

#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

/* The mixer's state is at the original's addresses: ORIG_VAR in its BMX_HOOK form. */
#define BMX_HOOK 1
#include "bmx/pc/zge/sound/winchan.h"
#undef BMX_HOOK
#include "platform/input.h"
#include "platform/smoothmix.h"
#include "platform/sound.h"
#include "platform/standins.h"

static int failures, checks;

int testMusic(int *checks); /* test_music.c: the music override */

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

static void stick8Props(BmxDiAxisProps props[kDiAxes])
{
    for (int i = 0; i < kDiAxes; i++) {
        bmxDiAxisDefaults(&props[i]);
        props[i].min = -127;
        props[i].max = 127;
    }
}

static void stick8(int x, int y, int *outX, int *outY)
{
    BmxDiAxisProps props[kDiAxes];
    BmxPadSnapshot p = padWith(kBmxPadDualSense, 0);
    BmxDiJoyState s;

    stick8Props(props);
    p.leftX = (int16_t)x;
    p.leftY = (int16_t)y;
    bmxPadToJoyState(&p, props, &s);
    (void)bmxPadApplyStick8Way(&p, props, &s);
    *outX = s.axis[kDiAxisX];
    *outY = s.axis[kDiAxisY];
}

static int roundToInt(double v)
{
    return (int)(v < 0 ? v - 0.5 : v + 0.5);
}

static void testStick8Way(void)
{
    /* angle (degrees, 0 = right, 90 = down as SDL's +Y) at radius 20000 -> expected lX, lY; the sector
     * boundaries are at 22.5 + k * 45, tested 2.5 degrees to each side */
    static const struct {
        int deg10, ex, ey;
    } kCases[] = {
        {0, 127, 0},        {200, 127, 0},      {250, 127, 127},   {450, 127, 127},   {650, 127, 127},
        {700, 0, 127},      {900, 0, 127},      {1100, 0, 127},    {1150, -127, 127}, {1350, -127, 127},
        {1550, -127, 127},  {1600, -127, 0},    {1800, -127, 0},   {2000, -127, 0},   {2050, -127, -127},
        {2250, -127, -127}, {2450, -127, -127}, {2500, 0, -127},   {2700, 0, -127},   {2900, 0, -127},
        {2950, 127, -127},  {3150, 127, -127},  {3350, 127, -127}, {3400, 127, 0},
    };
    BmxDiAxisProps props[kDiAxes];
    BmxPadSnapshot p;
    BmxDiJoyState s, plain;
    int x, y;

    for (size_t i = 0; i < sizeof kCases / sizeof kCases[0]; i++) {
        const double a = kCases[i].deg10 / 10.0 * 3.14159265358979 / 180.0;

        stick8(roundToInt(20000 * cos(a)), roundToInt(20000 * sin(a)), &x, &y);
        CHECK(x == kCases[i].ex && y == kCases[i].ey, "8-way %.1f deg: %d %d", kCases[i].deg10 / 10.0, x, y);
    }
    /* radial dead zone: inside the radius is 0/0, just outside it a full deflection */
    stick8(3000, 0, &x, &y);
    CHECK(x == 0 && y == 0, "inside dead zone %d %d", x, y);
    stick8(2300, 2300, &x, &y);
    CHECK(x == 0 && y == 0, "diagonal inside the circle %d %d", x, y);
    stick8(3500, 0, &x, &y);
    CHECK(x == 127 && y == 0, "outside dead zone %d %d", x, y);
    /* the problem case: down with a little X (vanilla gives a diagonal) is a pure down */
    stick8(2500, 30000, &x, &y);
    CHECK(x == 0 && y == 127, "down with drift %d %d", x, y);
    stick8(-32768, 32767, &x, &y);
    CHECK(x == -127 && y == 127, "extremes %d %d", x, y);

    /* option off = the function is not applied: the state stays the vanilla cooked one */
    stick8Props(props);
    p = padWith(kBmxPadDualSense, 0);
    p.leftX = 5000;
    p.leftY = 30000;
    bmxPadToJoyState(&p, props, &plain);
    CHECK(plain.axis[kDiAxisX] > 0 && plain.axis[kDiAxisY] > 100 && plain.axis[kDiAxisY] < 127,
          "vanilla is an analog diagonal-ish value %d %d", (int)plain.axis[0], (int)plain.axis[1]);
    s = plain;
    CHECK(bmxPadApplyStick8Way(&p, props, &s) == 1 && s.axis[kDiAxisX] == 0 && s.axis[kDiAxisY] == 127,
          "snap changes it");
    /* a state already snapped reports no change */
    CHECK(bmxPadApplyStick8Way(&p, props, &s) == 0, "idempotent");
    /* d-pad wins (input.c skips the snap while a d-pad direction is held): the d-pad then acts as before */
    p = padWith(kBmxPadDualSense, 1ull << SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    p.leftY = 30000;
    bmxPadToJoyState(&p, props, &s);
    plain = s;
    CHECK(bmxPadDpad(&p) != 0 && bmxPadApplyDpad(bmxPadDpad(&p), props, &s) == 1 &&
              s.axis[kDiAxisX] == -127 && s.axis[kDiAxisY] == plain.axis[kDiAxisY],
          "d-pad press: X from the d-pad, Y vanilla");
    /* generic pad: the left stick is axes 0 / 1 */
    p = padWith(kBmxPadGeneric, 0);
    p.axisCount = 2;
    p.axes[0] = 2000;
    p.axes[1] = -30000;
    bmxPadToJoyState(&p, props, &s);
    (void)bmxPadApplyStick8Way(&p, props, &s);
    CHECK(s.axis[kDiAxisX] == 0 && s.axis[kDiAxisY] == -127, "generic %d %d", (int)s.axis[0], (int)s.axis[1]);
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

/* Keys held at focus gain (input.c): a fake OS probe stands in for GetAsyncKeyState. */
static uint8_t gProbeDown[256];

static int fakeProbe(uint8_t dik)
{
    return gProbeDown[dik];
}

static void testHeldKeys(void)
{
    uint8_t state[256];

    bmxInputReset();
    bmxInputSetKeyProbe(fakeProbe);
    memset(gProbeDown, 0, sizeof gProbeDown);

    /* Enter (DIK 0x1c) and right Ctrl (0x9d) are down when the window gains the focus; SDL knows neither. */
    gProbeDown[0x1c] = gProbeDown[0x9d] = 1;
    bmxInputFocusGained();
    memset(state, 0, sizeof state);
    bmxKeyboardApplySeed(state);
    CHECK(state[0x1c] == 0x80 && state[0x9d] == 0x80 && state[0x1e] == 0, "held keys read as down");
    memset(state, 0, sizeof state);
    bmxKeyboardApplySeed(state);
    CHECK(state[0x1c] == 0x80 && state[0x9d] == 0x80, "and keep reading as down while held");

    /* Ctrl is released (SDL never hears of it): up again, and it stays up when the probe says down again */
    gProbeDown[0x9d] = 0;
    memset(state, 0, sizeof state);
    bmxKeyboardApplySeed(state);
    CHECK(state[0x1c] == 0x80 && state[0x9d] == 0, "a released key reads as up");
    gProbeDown[0x9d] = 1;
    memset(state, 0, sizeof state);
    bmxKeyboardApplySeed(state);
    CHECK(state[0x9d] == 0, "a released key is not seeded again (that is a new press, SDL's)");

    /* SDL has seen Enter pressed (a new press): SDL's state takes over, the seed is gone */
    memset(state, 0, sizeof state);
    state[0x1c] = 0x80;
    bmxKeyboardApplySeed(state);
    CHECK(state[0x1c] == 0x80, "SDL's own report stays");
    memset(state, 0, sizeof state);
    bmxKeyboardApplySeed(state);
    CHECK(state[0x1c] == 0, "after SDL took over, its release is final");

    /* focus lost forgets; a key held at the next gain is seeded again; unknown keys never are */
    gProbeDown[0x1c] = 1;
    bmxInputFocusGained();
    bmxInputFocusLost();
    memset(state, 0, sizeof state);
    bmxKeyboardApplySeed(state);
    CHECK(state[0x1c] == 0, "focus lost forgets the held keys");
    memset(gProbeDown, 1, sizeof gProbeDown);
    bmxInputFocusGained();
    memset(state, 0, sizeof state);
    bmxKeyboardApplySeed(state);
    CHECK(state[0x1c] == 0x80 && state[0xc8] == 0x80 && state[0] == 0 && state[0x7f] == 0 && state[0xff] == 0,
          "every known key is seeded, no unknown one (DIK 0, 0x7f, 0xff)");

    /* a seed with nothing held changes nothing */
    memset(gProbeDown, 0, sizeof gProbeDown);
    bmxInputFocusGained();
    memset(state, 0, sizeof state);
    bmxKeyboardApplySeed(state);
    CHECK(state[0x1c] == 0, "nothing held, nothing seeded");

    /* the OS probe (GetAsyncKeyState on Windows) runs for every key; the keys are the user's, so no value is
     * asserted */
    bmxInputSetKeyProbe(NULL);
    bmxInputFocusGained();
    bmxInputFocusLost();
    bmxInputReset();
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

/* ---- DirectSound (S7) ---- */

/*
 * Against the real dsound.dll in this process, with a hidden window (never focused: buffers without
 * DSBCAPS_GLOBALFOCUS are silent on both sides) and this process's audio session muted. Data written to the
 * buffers is silence unless the session is muted.
 */

static const GUID kClsidDeviceEnumerator = {
    0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e}};
static const GUID kIidDeviceEnumerator = {
    0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6}};
static const GUID kIidSessionManager = {
    0xbfa971f1, 0x4d5e, 0x40bb, {0x93, 0x5e, 0x96, 0x70, 0x39, 0xbf, 0xbe, 0xe4}};
static const GUID kIidMeter = {0xc02216f6, 0x8c67, 0x4b5b, {0x9d, 0x00, 0xd0, 0x08, 0xe7, 0x3e, 0x00, 0x64}};

/* IAudioMeterInformation: QueryInterface, AddRef, Release, GetPeakValue, GetMeteringChannelCount,
 * GetChannelsPeakValues, QueryHardwareSupport. */
typedef struct Meter {
    struct {
        void *qi, *addRef, *release, *peak, *count;
        HRESULT(STDMETHODCALLTYPE *channelPeaks)(struct Meter *self, UINT32 count, float *peaks);
    } *vtbl;
} Meter;

static Meter *gMeter;

/* Mutes this process's audio session (like main.c's BMX_MUTE) and gets its peak meter; 0 when not muted. */
static int muteSession(void)
{
    IMMDeviceEnumerator *devices = NULL;
    IMMDevice *device = NULL;
    IAudioSessionManager *sessions = NULL;
    ISimpleAudioVolume *volume = NULL;
    IAudioSessionControl *control = NULL;
    BOOL muted = FALSE;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(CoCreateInstance(&kClsidDeviceEnumerator, NULL, CLSCTX_ALL, &kIidDeviceEnumerator,
                                (void **)&devices)) ||
        FAILED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(devices, eRender, eConsole, &device)) ||
        FAILED(IMMDevice_Activate(device, &kIidSessionManager, CLSCTX_ALL, NULL, (void **)&sessions)) ||
        FAILED(IAudioSessionManager_GetSimpleAudioVolume(sessions, NULL, FALSE, &volume)) ||
        FAILED(ISimpleAudioVolume_SetMute(volume, TRUE, NULL)) ||
        FAILED(ISimpleAudioVolume_GetMute(volume, &muted))) {
        return 0;
    }
    if (SUCCEEDED(IAudioSessionManager_GetAudioSessionControl(sessions, NULL, 0, &control))) {
        IAudioSessionControl_QueryInterface(control, &kIidMeter, (void **)&gMeter);
    }
    return muted;
}

static BmxDs *createReal(void)
{
    const BmxDirectSoundCreateFn create =
        (BmxDirectSoundCreateFn)bmxRealProc("DSOUND.dll", "DirectSoundCreate");
    BmxDs *ds = NULL;

    return create != NULL && create(NULL, &ds, NULL) == 0 ? ds : NULL;
}

static BmxDs *createOurs(void)
{
    BmxDs *ds = NULL;

    return bmxStandDirectSoundCreate(NULL, (void **)&ds, NULL) == 0 ? ds : NULL;
}

static BmxDsBuffer *makeBuffer(BmxDs *ds, uint32_t rate, uint16_t bits, uint16_t channels, uint32_t bytes,
                               uint32_t flags)
{
    const uint16_t align = (uint16_t)(channels * bits / 8);
    const BmxWaveFormat f = {1, channels, rate, rate * align, align, bits, 0};
    BmxDsBufferDesc d = {sizeof d, flags, bytes, 0, &f, {0, 0, 0, {0}}};
    BmxDsBuffer *b = NULL;

    ds->vtbl->createSoundBuffer(ds, &d, &b, NULL);
    return b;
}

/* Fills the whole buffer with silence (or a pattern). */
static void fillBuffer(BmxDsBuffer *b, int bits, int16_t left, int16_t right)
{
    void *p1, *p2;
    uint32_t n1, n2;

    if (b->vtbl->lock(b, 0, 0, &p1, &n1, &p2, &n2, kDsbLockEntireBuffer) != 0) {
        return;
    }
    if (bits == 8) {
        memset(p1, 0x80 + (left >> 8), n1);
    } else {
        for (uint32_t i = 0; i + 4 <= n1; i += 4) {
            memcpy((uint8_t *)p1 + i, &left, 2);
            memcpy((uint8_t *)p1 + i + 2, &right, 2);
        }
    }
    b->vtbl->unlock(b, p1, n1, p2, n2);
}

typedef struct Record {
    int count;
    const char *label[400];
    int64_t value[400];
} Record;

static void rec(Record *r, const char *label, int64_t value)
{
    if (r->count < 400) {
        r->label[r->count] = label;
        r->value[r->count++] = value;
    }
}

/* Lock + Unlock, recording the result, the parts and the first part's offset. */
static void lockRec(Record *r, BmxDsBuffer *b, const uint8_t *base, uint32_t offset, uint32_t bytes,
                    uint32_t flags, const char *label)
{
    void *p1 = (void *)1, *p2 = (void *)1;
    uint32_t n1 = 77, n2 = 77;
    const BmxHresult hr = b->vtbl->lock(b, offset, bytes, &p1, &n1, &p2, &n2, flags);

    rec(r, label, hr);
    rec(r, "  p1 offset / null", p1 == NULL ? -1 : (int64_t)((const uint8_t *)p1 - base));
    rec(r, "  n1", n1);
    rec(r, "  p2 null", p2 == NULL);
    rec(r, "  n2", n2);
    if (hr == 0) {
        rec(r, "  unlock", b->vtbl->unlock(b, p1, n1, p2, n2));
    }
}

/* The same calls on a DirectSound object, real or ours; every result into r. */
static void soundApiScript(BmxDs *ds, HWND window, Record *r)
{
    BmxDsBuffer *primary = NULL, *primary2 = NULL, *b, *nb, *dup = NULL;
    BmxWaveFormat f;
    BmxDsbCaps caps = {sizeof caps, 0, 0, 0, 0};
    uint32_t n = 0, status = 0, play = 0, write = 0, freq = 0;
    int32_t v = 0;
    void *p1, *p2, *q1, *q2;
    uint32_t n1, n2, m1, m2;
    const uint8_t *base;
    BmxDsBufferDesc pd = {sizeof pd, kDsbPrimary, 0, 0, NULL, {0, 0, 0, {0}}};

    rec(r, "coop NULL window", ds->vtbl->setCooperativeLevel(ds, NULL, 1));
    rec(r, "coop level 0", ds->vtbl->setCooperativeLevel(ds, window, 0));
    rec(r, "coop level 5", ds->vtbl->setCooperativeLevel(ds, window, 5));
    rec(r, "coop normal", ds->vtbl->setCooperativeLevel(ds, window, 1));
    /* the primary buffer as winchanInit uses it */
    pd.bufferBytes = 100;
    rec(r, "primary with bytes", ds->vtbl->createSoundBuffer(ds, &pd, &primary, NULL));
    rec(r, "  out NULL", primary == NULL);
    pd.bufferBytes = 0;
    rec(r, "primary", ds->vtbl->createSoundBuffer(ds, &pd, &primary, NULL));
    rec(r, "primary again", ds->vtbl->createSoundBuffer(ds, &pd, &primary2, NULL));
    rec(r, "  same object", primary == primary2);
    if (primary2 != NULL) {
        rec(r, "  release", primary2->vtbl->release(primary2));
    }
    if (primary == NULL) {
        return;
    }
    primary->vtbl->getCaps(primary, &caps);
    rec(r, "primary caps flags", caps.flags);
    rec(r, "primary caps bytes", caps.bufferBytes);
    memset(&f, 0, sizeof f);
    rec(r, "primary GetFormat", primary->vtbl->getFormat(primary, &f, sizeof f, &n));
    rec(r, "  written", n);
    rec(r, "  rate", f.samplesPerSec);
    rec(r, "  bits", f.bitsPerSample);
    rec(r, "  channels", f.channels);
    f = (BmxWaveFormat){1, 2, 22050, 88200, 4, 16, 0};
    rec(r, "SetFormat at normal", primary->vtbl->setFormat(primary, &f));
    rec(r, "coop priority", ds->vtbl->setCooperativeLevel(ds, window, 2));
    f.blockAlign = 3;
    rec(r, "SetFormat bad align", primary->vtbl->setFormat(primary, &f));
    f.blockAlign = 4;
    f.avgBytesPerSec = 1;
    rec(r, "SetFormat bad avg", primary->vtbl->setFormat(primary, &f));
    f.avgBytesPerSec = 88200;
    rec(r, "SetFormat 22050/16/2", primary->vtbl->setFormat(primary, &f));
    memset(&f, 0, sizeof f);
    primary->vtbl->getFormat(primary, &f, sizeof f, &n);
    rec(r, "  rate", f.samplesPerSec);
    rec(r, "  bits", f.bitsPerSample);
    rec(r, "coop normal", ds->vtbl->setCooperativeLevel(ds, window, 1));
    rec(r, "primary GetStatus", primary->vtbl->getStatus(primary, &status));
    rec(r, "  status", status);
    rec(r, "primary Play once", primary->vtbl->play(primary, 0, 0, 0));
    rec(r, "primary Play looping", primary->vtbl->play(primary, 0, 0, 1));
    primary->vtbl->getStatus(primary, &status);
    rec(r, "  status", status);
    rec(r, "primary GetCurrentPosition", primary->vtbl->getCurrentPosition(primary, &play, &write));
    rec(r, "primary Lock", primary->vtbl->lock(primary, 0, 4, &p1, &n1, &p2, &n2, 0));
    rec(r, "primary SetVolume", primary->vtbl->setVolume(primary, -100));
    rec(r, "primary SetCurrentPosition", primary->vtbl->setCurrentPosition(primary, 0));

    /* secondary buffers: invalid descriptions */
    rec(r, "create 3 bytes", makeBuffer(ds, 22050, 16, 2, 3, 0xe0) == NULL);
    nb = makeBuffer(ds, 22050, 16, 2, 6, 0xe0);
    rec(r, "create 6 bytes", nb != NULL);
    if (nb != NULL) {
        nb->vtbl->getCaps(nb, &caps);
        rec(r, "  rounded to", caps.bufferBytes);
        rec(r, "  release", nb->vtbl->release(nb));
    }
    rec(r, "create 0x10000000 bytes", makeBuffer(ds, 22050, 16, 2, 0x10000000, 0xe0) == NULL);
    rec(r, "create 99 Hz", makeBuffer(ds, 99, 16, 2, 8816, 0xe0) == NULL);
    rec(r, "create 200001 Hz", makeBuffer(ds, 200001, 16, 2, 8816, 0xe0) == NULL);
    rec(r, "create 6 channels", makeBuffer(ds, 22050, 16, 6, 8820, 0xe0) == NULL);
    {
        BmxDsBufferDesc d = {sizeof d, 0xe0, 8816, 0, NULL, {0, 0, 0, {0}}};
        const BmxWaveFormat bad = {1, 2, 22050, 1, 4, 16, 0};

        nb = (BmxDsBuffer *)1;
        rec(r, "create no format", ds->vtbl->createSoundBuffer(ds, &d, &nb, NULL));
        rec(r, "  out NULL", nb == NULL);
        d.format = &bad;
        rec(r, "create bad avg", ds->vtbl->createSoundBuffer(ds, &d, &nb, NULL));
        d.size = 0x10;
        rec(r, "create desc size 0x10", ds->vtbl->createSoundBuffer(ds, &d, &nb, NULL));
    }
    nb = makeBuffer(ds, 22050, 16, 2, 8816, 0);
    rec(r, "no-control SetVolume", nb->vtbl->setVolume(nb, -100));
    rec(r, "no-control SetPan", nb->vtbl->setPan(nb, 0));
    rec(r, "no-control SetFrequency", nb->vtbl->setFrequency(nb, 1000));
    rec(r, "no-control GetVolume", nb->vtbl->getVolume(nb, &v));
    nb->vtbl->getCaps(nb, &caps);
    rec(r, "no-control caps flags", caps.flags);
    rec(r, "  release", nb->vtbl->release(nb));

    /* the mixer's output buffer of winchanInit */
    b = makeBuffer(ds, 22050, 16, 2, 8816, 0xe0);
    b->vtbl->getCaps(b, &caps);
    rec(r, "caps flags", caps.flags);
    rec(r, "caps bytes", caps.bufferBytes);
    memset(&f, 0, sizeof f);
    rec(r, "GetFormat", b->vtbl->getFormat(b, &f, sizeof f, &n));
    rec(r, "  written", n);
    rec(r, "  rate / bits / ch / align",
        f.samplesPerSec * 1000000 + f.bitsPerSample * 1000 + f.channels * 10 + f.blockAlign);
    rec(r, "GetFormat size query", b->vtbl->getFormat(b, NULL, 0, &n));
    rec(r, "  written", n);
    rec(r, "GetFormat 16 bytes", b->vtbl->getFormat(b, &f, 16, &n));
    rec(r, "GetStatus", b->vtbl->getStatus(b, &status));
    rec(r, "  status", status);
    b->vtbl->lock(b, 0, 0, &p1, &n1, &p2, &n2, kDsbLockEntireBuffer);
    base = p1;
    rec(r, "lock entire: n1", n1);
    rec(r, "  unlock", b->vtbl->unlock(b, p1, n1, p2, n2));
    lockRec(r, b, base, 0, 8816, 0, "lock 0 8816");
    lockRec(r, b, base, 0, 0, 0, "lock 0 0");
    lockRec(r, b, base, 100, 0, kDsbLockEntireBuffer, "lock 100 entire");
    lockRec(r, b, base, 8000, 1000, 0, "lock 8000 1000 (wraps)");
    lockRec(r, b, base, 8816, 4, 0, "lock at the end");
    lockRec(r, b, base, 0, 8817, 0, "lock too long");
    lockRec(r, b, base, 1, 3, 0, "lock unaligned");
    lockRec(r, b, base, 8812, 8816, 0, "lock 8812 whole");
    lockRec(r, b, base, 2000, 100, kDsbLockFromWriteCursor, "lock from write cursor (stopped)");
    rec(r, "lock A", b->vtbl->lock(b, 0, 100, &p1, &n1, &p2, &n2, 0));
    rec(r, "lock B overlapping", b->vtbl->lock(b, 50, 100, &q1, &m1, &q2, &m2, 0));
    rec(r, "lock C apart", b->vtbl->lock(b, 200, 100, &q1, &m1, &q2, &m2, 0));
    rec(r, "unlock C", b->vtbl->unlock(b, q1, m1, q2, m2));
    rec(r, "unlock wrong pointer", b->vtbl->unlock(b, (uint8_t *)p1 + 4, 96, NULL, 0));
    rec(r, "unlock more bytes", b->vtbl->unlock(b, p1, 200, NULL, 0));
    rec(r, "unlock fewer bytes", b->vtbl->unlock(b, p1, 10, NULL, 0));
    rec(r, "unlock again", b->vtbl->unlock(b, p1, n1, NULL, 0));
    rec(r, "unlock NULL 0", b->vtbl->unlock(b, NULL, 0, NULL, 0));
    rec(r, "unlock NULL 1000", b->vtbl->unlock(b, NULL, 1000, NULL, 0));
    rec(r, "lock wrapping, no second pointer", b->vtbl->lock(b, 8000, 1000, &p1, &n1, NULL, NULL, 0));
    rec(r, "  n1", n1);
    rec(r, "  unlock without the second part", b->vtbl->unlock(b, p1, n1, NULL, 0));
    rec(r, "lock the second part's bytes", b->vtbl->lock(b, 0, 100, &q1, &m1, &q2, &m2, 0));
    rec(r, "unlock the second part", b->vtbl->unlock(b, (void *)base, 184, NULL, 0));
    rec(r, "lock them again", b->vtbl->lock(b, 0, 100, &q1, &m1, &q2, &m2, 0));
    rec(r, "  unlock", b->vtbl->unlock(b, q1, m1, q2, m2));
    rec(r, "lock 0 100", b->vtbl->lock(b, 0, 100, &p1, &n1, &p2, &n2, 0));
    rec(r, "unlock with a second part it does not have",
        b->vtbl->unlock(b, p1, n1, (uint8_t *)p1 + 1000, 50));
    rec(r, "unlock", b->vtbl->unlock(b, p1, n1, NULL, 0));
    rec(r, "SetCurrentPosition 101", b->vtbl->setCurrentPosition(b, 101));
    b->vtbl->getCurrentPosition(b, &play, &write);
    rec(r, "  play", play);
    rec(r, "  write", write);
    rec(r, "SetCurrentPosition 8816", b->vtbl->setCurrentPosition(b, 8816));
    rec(r, "SetCurrentPosition 4000", b->vtbl->setCurrentPosition(b, 4000));
    b->vtbl->getCurrentPosition(b, &play, &write);
    rec(r, "  play", play);
    rec(r, "  write", write);
    rec(r, "GetCurrentPosition NULL NULL", b->vtbl->getCurrentPosition(b, NULL, NULL));
    rec(r, "SetVolume 1", b->vtbl->setVolume(b, 1));
    rec(r, "SetVolume -10001", b->vtbl->setVolume(b, -10001));
    rec(r, "SetVolume -10000", b->vtbl->setVolume(b, -10000));
    b->vtbl->getVolume(b, &v);
    rec(r, "  read back", v);
    rec(r, "SetVolume -1234", b->vtbl->setVolume(b, -1234));
    b->vtbl->getVolume(b, &v);
    rec(r, "  read back", v);
    rec(r, "SetPan 10001", b->vtbl->setPan(b, 10001));
    rec(r, "SetPan -9906", b->vtbl->setPan(b, -9906));
    b->vtbl->getPan(b, &v);
    rec(r, "  read back", v);
    b->vtbl->getFrequency(b, &freq);
    rec(r, "GetFrequency", freq);
    rec(r, "SetFrequency 99", b->vtbl->setFrequency(b, 99));
    rec(r, "SetFrequency 100", b->vtbl->setFrequency(b, 100));
    rec(r, "SetFrequency 200001", b->vtbl->setFrequency(b, 200001));
    rec(r, "SetFrequency 200000", b->vtbl->setFrequency(b, 200000));
    b->vtbl->getFrequency(b, &freq);
    rec(r, "  read back", freq);
    rec(r, "SetFrequency 0", b->vtbl->setFrequency(b, 0));
    b->vtbl->getFrequency(b, &freq);
    rec(r, "  read back", freq);
    rec(r, "Restore", b->vtbl->restore(b));
    rec(r, "Initialize", b->vtbl->initialize(b, ds, NULL));
    rec(r, "Play reserved 1", b->vtbl->play(b, 1, 0, 1));
    rec(r, "Play priority 1", b->vtbl->play(b, 0, 1, 1));
    rec(r, "Play flags 8", b->vtbl->play(b, 0, 0, 8));
    rec(r, "Play looping", b->vtbl->play(b, 0, 0, 1));
    b->vtbl->getStatus(b, &status);
    rec(r, "  status", status);
    rec(r, "Play once while playing", b->vtbl->play(b, 0, 0, 0));
    b->vtbl->getStatus(b, &status);
    rec(r, "  status", status);
    rec(r, "Stop", b->vtbl->stop(b));
    rec(r, "Stop again", b->vtbl->stop(b));
    b->vtbl->getStatus(b, &status);
    rec(r, "  status", status);
    rec(r, "AddRef", b->vtbl->addRef(b));
    rec(r, "Release", b->vtbl->release(b));
    {
        static const BmxGuid kIidBuffer = {
            0x279afa85, 0x4981, 0x11ce, {0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60}};
        static const BmxGuid kIidOther = {1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 11}};
        void *out = NULL;

        rec(r, "QueryInterface buffer", b->vtbl->queryInterface(b, &kIidBuffer, &out));
        rec(r, "  same object", out == (void *)b);
        if (out != NULL) {
            rec(r, "  release", b->vtbl->release(b));
        }
        rec(r, "QueryInterface other", b->vtbl->queryInterface(b, &kIidOther, &out));
        rec(r, "  out NULL", out == NULL);
    }
    /* duplicates share the data, not the state */
    b->vtbl->setVolume(b, -500);
    b->vtbl->setCurrentPosition(b, 1000);
    rec(r, "DuplicateSoundBuffer", ds->vtbl->duplicateSoundBuffer(ds, b, &dup));
    if (dup != NULL) {
        dup->vtbl->getCurrentPosition(dup, &play, &write);
        rec(r, "  play", play);
        dup->vtbl->getVolume(dup, &v);
        rec(r, "  volume", v);
        dup->vtbl->getStatus(dup, &status);
        rec(r, "  status", status);
        rec(r, "  lock", dup->vtbl->lock(dup, 0, 4, &p1, &n1, &p2, &n2, 0));
        rec(r, "  same data", p1 == base);
        rec(r, "  original locks the same bytes", b->vtbl->lock(b, 0, 4, &q1, &m1, &q2, &m2, 0));
        dup->vtbl->unlock(dup, p1, n1, p2, n2);
        rec(r, "  release", dup->vtbl->release(dup));
    }
    rec(r, "DuplicateSoundBuffer primary", ds->vtbl->duplicateSoundBuffer(ds, primary, &dup));
    rec(r, "buffer Release", b->vtbl->release(b));
    rec(r, "primary Stop", primary->vtbl->stop(primary));
    primary->vtbl->getStatus(primary, &status);
    rec(r, "  status", status);
    rec(r, "primary Release", primary->vtbl->release(primary));
    rec(r, "device Release", ds->vtbl->release(ds));
}

static void testSoundApi(HWND window)
{
    static Record real, ours;
    BmxDs *ds = createReal();
    int differences = 0;

    CHECK(ds != NULL, "real DirectSoundCreate");
    if (ds == NULL) {
        return;
    }
    soundApiScript(ds, window, &real);
    ds = createOurs();
    soundApiScript(ds, window, &ours);
    CHECK(real.count == ours.count, "%d vs %d results", real.count, ours.count);
    for (int i = 0; i < real.count && i < ours.count; i++) {
        if (real.value[i] != ours.value[i]) {
            differences++;
            printf("dsound api: %-36s DirectSound %#llx, ours %#llx\n", real.label[i],
                   (unsigned long long)real.value[i], (unsigned long long)ours.value[i]);
        }
    }
    CHECK(differences == 0, "%d of %d results differ from dsound.dll", differences, real.count);
    printf("dsound api: %d results compared\n", real.count);
}

/* The cursor sequence of one play: the distinct (play, write) pairs in frames, real (polled) or ours (per
 * offline mix). */
typedef struct Scenario {
    const char *name;
    uint32_t rate;
    uint16_t bits, channels;
    uint32_t bytes, start, flags;
} Scenario;

enum { kPairs = 12 };

static int realCursorSequence(BmxDs *ds, const Scenario *s, uint32_t pairs[kPairs][3])
{
    BmxDsBuffer *b = makeBuffer(ds, s->rate, s->bits, s->channels, s->bytes, 0xe0);
    const uint32_t align = s->channels * s->bits / 8u;
    const uint64_t t0 = SDL_GetTicksNS();
    uint32_t p, w, st, lp = UINT32_MAX, lw = UINT32_MAX;
    int k = 0;

    fillBuffer(b, s->bits, 0, 0);
    b->vtbl->setCurrentPosition(b, s->start);
    b->vtbl->play(b, 0, 0, s->flags);
    while (k < kPairs && SDL_GetTicksNS() - t0 < 400000000ull) {
        b->vtbl->getCurrentPosition(b, &p, &w);
        b->vtbl->getStatus(b, &st);
        if (p != lp || w != lw) {
            pairs[k][0] = p / align;
            pairs[k][1] = w / align;
            pairs[k][2] = st;
            k++;
            lp = p;
            lw = w;
            if (st == 0) {
                /* dsound.dll resets the position a moment after the status: read it once settled */
                SDL_Delay(20);
                b->vtbl->getCurrentPosition(b, &p, &w);
                pairs[k - 1][0] = p / align;
                pairs[k - 1][1] = w / align;
                break;
            }
        }
    }
    b->vtbl->release(b);
    return k;
}

static int ourCursorSequence(BmxDs *ds, const Scenario *s, uint32_t pairs[kPairs][3])
{
    BmxDsBuffer *b = makeBuffer(ds, s->rate, s->bits, s->channels, s->bytes, 0xe0);
    const uint32_t align = s->channels * s->bits / 8u;
    uint32_t p, w, st, lp = UINT32_MAX, lw = UINT32_MAX;
    int k = 0;

    fillBuffer(b, s->bits, 0, 0);
    b->vtbl->setCurrentPosition(b, s->start);
    b->vtbl->play(b, 0, 0, s->flags);
    for (int mix = 0; mix < 40 && k < kPairs; mix++) {
        b->vtbl->getCurrentPosition(b, &p, &w);
        b->vtbl->getStatus(b, &st);
        if (p != lp || w != lw) {
            pairs[k][0] = p / align;
            pairs[k][1] = w / align;
            pairs[k][2] = st;
            k++;
            lp = p;
            lw = w;
            if (st == 0) {
                break;
            }
        }
        bmxSoundMixOffline(NULL);
    }
    b->vtbl->release(b);
    return k;
}

static void printPairs(const char *who, uint32_t pairs[kPairs][3], int n)
{
    printf("  %-11s", who);
    for (int i = 0; i < n; i++) {
        printf(" %u/%u%s", (unsigned)pairs[i][0], (unsigned)pairs[i][1], pairs[i][2] ? "" : "(stopped)");
    }
    printf("\n");
}

/* The cursor model against dsound.dll: the sequence of (play, write) after each 10 ms mix, exactly. */
static void testSoundCursors(HWND window, uint32_t rate)
{
    static const Scenario kScenarios[] = {
        {"winchan ring 22050/16/2 8816", 22050, 16, 2, 8816, 0, 1},
        {"same from 1000", 22050, 16, 2, 8816, 1000, 1},
        {"half the size", 22050, 16, 2, 4408, 0, 1},
        {"once from 1000", 22050, 16, 2, 4408, 1000, 0},
        {"device rate, small", 48000, 16, 2, 8816, 0, 1},
        {"stream 44100/8/1 64K", 44100, 8, 1, 65536, 0, 1},
        {"stream 44100/16/2 64K", 44100, 16, 2, 65536, 0, 1},
        {"tiny 2000", 22050, 16, 2, 2000, 0, 1},
        {"stream 11025/16/1 from 2", 11025, 16, 1, 65536, 2, 1},
        {"stream 16000/16/1", 16000, 16, 1, 65536, 0, 1},
        {"96000 (down)", 96000, 16, 2, 65536, 0, 1},
        {"8000/8/1", 8000, 8, 1, 65536, 0, 1},
    };
    BmxDs *real = createReal(), *ours;

    if (real == NULL) {
        CHECK(0, "real DirectSoundCreate");
        return;
    }
    real->vtbl->setCooperativeLevel(real, window, 1);
    bmxSoundReset();
    bmxSoundUseOffline(rate);
    ours = createOurs();
    ours->vtbl->setCooperativeLevel(ours, window, 1);
    for (size_t i = 0; i < sizeof kScenarios / sizeof kScenarios[0]; i++) {
        const Scenario *s = &kScenarios[i];
        uint32_t a[kPairs][3], b[kPairs][3];
        int na, nb, same;

        if (rate != 48000 && s->rate == 48000) {
            continue;
        }
        na = realCursorSequence(real, s, a);
        nb = ourCursorSequence(ours, s, b);
        same = na == nb && memcmp(a, b, sizeof a[0] * (size_t)na) == 0;
        CHECK(same, "cursor sequence of \"%s\" differs", s->name);
        printf("dsound cursors %s: %s\n", s->name, same ? "identical" : "DIFFERENT");
        if (!same) {
            printPairs("DirectSound", a, na);
            printPairs("ours", b, nb);
        }
    }
    /* SetCurrentPosition while playing: the cursors restart there, the write cursor without the lookahead. */
    {
        BmxDsBuffer *rb = makeBuffer(real, 22050, 16, 2, 8816, 0xe0),
                    *ob = makeBuffer(ours, 22050, 16, 2, 8816, 0xe0);
        uint32_t rp, rw, op, ow;

        rb->vtbl->play(rb, 0, 0, 1);
        ob->vtbl->play(ob, 0, 0, 1);
        SDL_Delay(55);
        for (int i = 0; i < 6; i++) {
            bmxSoundMixOffline(NULL);
        }
        rb->vtbl->setCurrentPosition(rb, 400);
        ob->vtbl->setCurrentPosition(ob, 400);
        rb->vtbl->getCurrentPosition(rb, &rp, &rw);
        ob->vtbl->getCurrentPosition(ob, &op, &ow);
        CHECK(rp == 400 && rw == 400 && op == 400 && ow == 400, "after SetCurrentPosition: %u/%u, ours %u/%u",
              (unsigned)rp, (unsigned)rw, (unsigned)op, (unsigned)ow);
        /* two of dsound.dll's mixes, two of ours */
        for (uint32_t last = rw, changes = 0, t = 0; changes < 2 && t < 100; t++) {
            rb->vtbl->getCurrentPosition(rb, &rp, &rw);
            if (rw != last) {
                changes++;
                last = rw;
            } else {
                SDL_Delay(1);
            }
        }
        bmxSoundMixOffline(NULL);
        bmxSoundMixOffline(NULL);
        ob->vtbl->getCurrentPosition(ob, &op, &ow);
        CHECK(rp == 400 && op == 400 && rw >= 400 + 4 * 440 && rw <= 400 + 4 * 443 && ow >= 400 + 4 * 440 &&
                  ow <= 400 + 4 * 443,
              "two mixes after SetCurrentPosition: %u/%u, ours %u/%u", (unsigned)rp, (unsigned)rw,
              (unsigned)op, (unsigned)ow);
        printf("dsound cursors after SetCurrentPosition while playing, two mixes: DirectSound %u/%u, ours "
               "%u/%u\n",
               (unsigned)rp, (unsigned)rw, (unsigned)op, (unsigned)ow);
        /* Stop keeps the play cursor, for both cursors */
        rb->vtbl->stop(rb);
        ob->vtbl->stop(ob);
        rb->vtbl->getCurrentPosition(rb, &rp, &rw);
        ob->vtbl->getCurrentPosition(ob, &op, &ow);
        CHECK(rp == rw && op == ow && op == 400, "after Stop: %u/%u, ours %u/%u", (unsigned)rp, (unsigned)rw,
              (unsigned)op, (unsigned)ow);
        rb->vtbl->release(rb);
        ob->vtbl->release(ob);
    }
    ours->vtbl->release(ours);
    real->vtbl->release(real);
}

/* What the mixer makes of the buffers (offline, at the device rate). */
static double mixMean(int channel, int quanta, float *out)
{
    double sum = 0;
    const uint32_t q = bmxSoundQuantum();

    for (int i = 0; i < quanta; i++) {
        bmxSoundMixOffline(out);
        for (uint32_t j = 0; j < q; j++) {
            sum += out[2 * j + channel];
        }
    }
    return sum / (quanta * (double)q);
}

static void testSoundMix(HWND window, uint32_t rate)
{
    float *out;
    BmxDs *ds;
    BmxDsBuffer *b;
    float l, r;

    bmxSoundReset();
    bmxSoundUseOffline(rate);
    ds = createOurs();
    ds->vtbl->setCooperativeLevel(ds, window, 1);
    out = calloc(2 * (size_t)bmxSoundQuantum(), sizeof(float));
    bmxSoundForceFocus(1);
    b = makeBuffer(ds, rate, 16, 2, rate * 4, 0xe0);
    fillBuffer(b, 16, 8192, -4096);
    b->vtbl->play(b, 0, 0, 1);
    CHECK(fabs(mixMean(0, 3, out) - 0.25) < 1e-6 && fabs(mixMean(1, 3, out) + 0.125) < 1e-6,
          "16-bit stereo: %f %f", mixMean(0, 1, out), mixMean(1, 1, out));
    b->vtbl->setVolume(b, -600);
    b->vtbl->setPan(b, 1000);
    bmxDsGains(-600, 1000, &l, &r);
    /* -16 dB and -6 dB as dsound.dll applies them: -15.9 dB and -5.9 dB, truncated to 1/65536 */
    CHECK(l == (float)(10507.0 / 65536) && r == (float)(33226.0 / 65536), "gains %f %f", (double)l,
          (double)r);
    CHECK(fabs(mixMean(0, 2, out) - 0.25 * l) < 1e-6 && fabs(mixMean(1, 2, out) + 0.125 * r) < 1e-6,
          "volume and pan: %f %f", mixMean(0, 1, out), mixMean(1, 1, out));
    b->vtbl->setVolume(b, -10000);
    CHECK(mixMean(0, 1, out) == 0 && mixMean(1, 1, out) == 0, "silent at -10000");
    b->vtbl->setVolume(b, 0);
    b->vtbl->setPan(b, 0);
    /* no focus: silent (DSBCAPS_GLOBALFOCUS would play on), the cursor runs */
    bmxSoundForceFocus(0);
    {
        uint32_t p0, p1, w;

        b->vtbl->getCurrentPosition(b, &p0, &w);
        CHECK(mixMean(0, 2, out) == 0, "unfocused buffer is silent");
        b->vtbl->getCurrentPosition(b, &p1, &w);
        CHECK(p1 == p0 + 2 * 4 * bmxSoundQuantum(), "cursor runs unfocused: %u -> %u", (unsigned)p0,
              (unsigned)p1);
    }
    b->vtbl->stop(b);
    b->vtbl->release(b);
    b = makeBuffer(ds, rate, 16, 2, rate * 4, 0xe0 | kDsbGlobalFocus);
    fillBuffer(b, 16, 8192, -4096);
    b->vtbl->play(b, 0, 0, 1);
    CHECK(fabs(mixMean(0, 3, out) - 0.25) < 1e-6, "global focus plays unfocused");
    b->vtbl->release(b);
    bmxSoundForceFocus(1);
    /* 8-bit mono resampled: both channels, value (v - 128) / 128 */
    b = makeBuffer(ds, 11025, 8, 1, 11025, 0xe0);
    fillBuffer(b, 8, 0x40 << 8, 0);
    b->vtbl->play(b, 0, 0, 1);
    mixMean(0, 2, out);
    CHECK(fabs(mixMean(0, 4, out) - 0.5) < 1e-3 && fabs(mixMean(1, 4, out) - 0.5) < 1e-3, "8-bit mono: %f %f",
          mixMean(0, 1, out), mixMean(1, 1, out));
    b->vtbl->release(b);
    /* a 1 kHz sine at 22050 Hz keeps its level through the resampler */
    {
        void *p1, *p2;
        uint32_t n1, n2;
        double sum = 0;
        const uint32_t q = bmxSoundQuantum();

        b = makeBuffer(ds, 22050, 16, 2, 22050 * 4, 0xe0);
        b->vtbl->lock(b, 0, 0, &p1, &n1, &p2, &n2, kDsbLockEntireBuffer);
        for (uint32_t i = 0; i < n1 / 4; i++) {
            const int16_t v = (int16_t)lrint(16384.0 * sin(2.0 * 3.14159265358979 * 1000.0 * i / 22050.0));

            memcpy((uint8_t *)p1 + 4 * i, &v, 2);
            memcpy((uint8_t *)p1 + 4 * i + 2, &v, 2);
        }
        b->vtbl->unlock(b, p1, n1, p2, n2);
        b->vtbl->play(b, 0, 0, 1);
        for (int k = 0; k < 3; k++) {
            bmxSoundMixOffline(out);
        }
        for (int k = 0; k < 20; k++) {
            bmxSoundMixOffline(out);
            for (uint32_t j = 0; j < q; j++) {
                sum += (double)out[2 * j] * out[2 * j];
            }
        }
        sum = sqrt(sum / (20.0 * q));
        CHECK(fabs(sum - 0.5 / sqrt(2.0)) < 0.005, "sine RMS %f (expected %f)", sum, 0.5 / sqrt(2.0));
        b->vtbl->release(b);
    }
    /* a non-looping buffer stops at the mix that reaches its end, at position 0, and is silent then */
    {
        uint32_t st = 1, p, w;
        int mixes = 0;

        b = makeBuffer(ds, 22050, 16, 2, 4408, 0xe0);
        fillBuffer(b, 16, 8192, 8192);
        b->vtbl->setCurrentPosition(b, 1000);
        b->vtbl->play(b, 0, 0, 0);
        while (st != 0 && mixes < 20) {
            bmxSoundMixOffline(out);
            mixes++;
            b->vtbl->getStatus(b, &st);
        }
        b->vtbl->getCurrentPosition(b, &p, &w);
        CHECK((rate != 48000 || mixes == 4) && p == 0 && w == 0, "non-looping end after %d mixes at %u/%u",
              mixes, (unsigned)p, (unsigned)w);
        CHECK(mixMean(0, 1, out) == 0, "silent after the end");
        b->vtbl->release(b);
    }
    bmxSoundForceFocus(-1);
    ds->vtbl->release(ds);
    free(out);
    bmxSoundReset();
}

/* Real time, our SDL device against dsound.dll: the cursor steps every 10 ms, and (when the session meter is
 * there) the level of a GLOBALFOCUS tone through each with a volume and pan. Muted session only. */
static void playTone(BmxDs *ds, int32_t volume, int32_t pan, float peaks[2], double *stepMs, uint32_t *lead)
{
    BmxDsBuffer *b = makeBuffer(ds, 22050, 16, 2, 22050 * 4, 0xe0 | kDsbGlobalFocus);
    void *p1, *p2;
    uint32_t n1, n2, p, w, lp = UINT32_MAX;
    uint64_t t0, first = 0, last = 0;
    int steps = 0;

    b->vtbl->lock(b, 0, 0, &p1, &n1, &p2, &n2, kDsbLockEntireBuffer);
    for (uint32_t i = 0; i < n1 / 4; i++) {
        const int16_t v = (int16_t)lrint(3276.8 * sin(2.0 * 3.14159265358979 * 441.0 * i / 22050.0));

        memcpy((uint8_t *)p1 + 4 * i, &v, 2);
        memcpy((uint8_t *)p1 + 4 * i + 2, &v, 2);
    }
    b->vtbl->unlock(b, p1, n1, p2, n2);
    b->vtbl->setVolume(b, volume);
    b->vtbl->setPan(b, pan);
    peaks[0] = peaks[1] = 0;
    *lead = 0;
    b->vtbl->play(b, 0, 0, 1);
    t0 = SDL_GetTicksNS();
    while (SDL_GetTicksNS() - t0 < 600000000ull) {
        b->vtbl->getCurrentPosition(b, &p, &w);
        if (p != lp) {
            const uint64_t t = SDL_GetTicksNS();

            if (lp != UINT32_MAX && p != 0) {
                if (first == 0) {
                    first = t;
                }
                last = t;
                steps++;
                *lead = (w + 22050 * 4 - p) % (22050 * 4);
            }
            lp = p;
        }
        if (gMeter != NULL && SDL_GetTicksNS() - t0 > 100000000ull) {
            float now[2] = {0, 0};

            gMeter->vtbl->channelPeaks(gMeter, 2, now);
            peaks[0] = now[0] > peaks[0] ? now[0] : peaks[0];
            peaks[1] = now[1] > peaks[1] ? now[1] : peaks[1];
        }
        SDL_Delay(1);
    }
    b->vtbl->stop(b);
    b->vtbl->release(b);
    *stepMs = steps > 1 ? (double)(last - first) / 1e6 / (steps - 1) : 0;
    SDL_Delay(100);
}

static void testSoundRealTime(HWND window)
{
    static const int32_t kSettings[][2] = {{0, 0}, {-600, 0}, {-300, 2000}, {-1000, -4000}};
    BmxDs *real = createReal(), *ours;

    bmxSoundReset();
    bmxSoundUseOffline(0); /* the SDL audio device, as in the game */
    ours = createOurs();
    if (real == NULL || ours == NULL) {
        CHECK(0, "DirectSoundCreate");
        return;
    }
    real->vtbl->setCooperativeLevel(real, window, 1);
    ours->vtbl->setCooperativeLevel(ours, window, 1);
    for (size_t i = 0; i < sizeof kSettings / sizeof kSettings[0]; i++) {
        float rp[2], op[2];
        double rs, os;
        uint32_t rl, ol;

        playTone(real, kSettings[i][0], kSettings[i][1], rp, &rs, &rl);
        playTone(ours, kSettings[i][0], kSettings[i][1], op, &os, &ol);
        printf("dsound real time vol %d pan %d: step every %.2f ms, lead %u bytes, peaks %.4f %.4f; ours "
               "%.2f ms, "
               "%u bytes, %.4f %.4f\n",
               (int)kSettings[i][0], (int)kSettings[i][1], rs, (unsigned)rl, (double)rp[0], (double)rp[1], os,
               (unsigned)ol, (double)op[0], (double)op[1]);
        CHECK(fabs(rs - 10.0) < 0.5 && fabs(os - 10.0) < 0.5, "10 ms steps: %.2f / %.2f", rs, os);
        CHECK(rl == ol, "steady lead %u / %u", (unsigned)rl, (unsigned)ol);
        if (gMeter != NULL && rp[0] > 0) {
            CHECK(fabs(op[0] / rp[0] - 1) < 0.002 && fabs(op[1] / rp[1] - 1) < 0.002,
                  "levels within 0.2%%: %.4f %.4f / %.4f %.4f", (double)op[0], (double)op[1], (double)rp[0],
                  (double)rp[1]);
        }
    }
    ours->vtbl->release(ours);
    real->vtbl->release(real);
    bmxSoundReset();
}

static void testSound(void)
{
    const HWND window = CreateWindowExA(0, "STATIC", "bmxplatformtest sound", WS_OVERLAPPEDWINDOW, 0, 0, 64,
                                        64, NULL, NULL, GetModuleHandleA(NULL), NULL); /* never shown */
    SDL_AudioSpec spec = {SDL_AUDIO_F32, 2, 48000};
    int frames = 0;

    if (!muteSession()) {
        printf("dsound: could not mute this process's audio session; sound tests skipped\n");
        DestroyWindow(window);
        return;
    }
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &frames);
    printf("dsound: device rate %d Hz (%d-frame periods), session muted, meter %s\n", spec.freq, frames,
           gMeter != NULL ? "yes" : "no");
    {
        void *module = bmxStandLoadLibraryA("dsound.dll");

        CHECK(module != NULL &&
                  bmxStandGetProcAddress(module, "DirectSoundCreate") == (void *)bmxStandDirectSoundCreate,
              "LoadLibraryA / GetProcAddress");
        CHECK(bmxStandGetProcAddress(module, "DirectSoundEnumerateA") == NULL, "other names");
        CHECK(bmxStandLoadLibraryA("ddraw.dll") == NULL && bmxStandFreeLibrary(module) == 1,
              "other DLLs, free");
    }
    bmxSoundReset();
    bmxSoundUseOffline((uint32_t)spec.freq);
    testSoundApi(window);
    testSoundCursors(window, (uint32_t)spec.freq);
    testSoundMix(window, (uint32_t)spec.freq);
    testSoundRealTime(window);
    DestroyWindow(window);
}

/* ---- smooth resampling mixer (src/platform/smoothmix.c) against the real winchanMix ---- */

/* winchan.c's two loader calls; the mixer never makes them (the test sets the channels up by hand). */
ZsdSample *winloadGetSample(uint16_t file, uint32_t index)
{
    return NULL;
}

WinloadSampleFile *winloadGetSampleFile(uint16_t file, uint32_t index)
{
    return NULL;
}

enum { kSmokeFrames = 600 };

/* The mixer's state lives at the original's addresses: clear it all. */
static void smoothReset(void)
{
    free(gWinchanEchoBuffer);
    memset((void *)(uintptr_t)0x400000, 0, 0x2a0000);
    gWinchanRate = 22050;
    gWinchanEffectMaxFrames = 1000;
    gWinchanEchoBuffer = (WinchanFrame *)calloc(1000, sizeof(WinchanFrame));
}

/* Channel 0 plays `sample` (voice v): length, step (20.12), start position and fraction. */
static void smoothChannel(const ZsdSample *sample, const ZsdVoice *v, uint32_t length, uint32_t step,
                          uint32_t position, uint32_t fraction)
{
    WinchanChannel *ch = &gWinchanChannels[0];

    memset(ch, 0, sizeof *ch);
    ch->state = kWinchanPlaying;
    ch->voice = v;
    ch->sample = sample;
    ch->length = length;
    ch->step = step;
    ch->position = position;
    ch->fraction = fraction;
    ch->gainLeft = 2.0f; /* output = sample value */
    ch->gainRight = 1.5f;
    ch->level = 1.0f;
}

/* Renders frames with the real winchanMix (mode < 0) or the smooth one, from a saved state. */
typedef struct SmokeState {
    WinchanChannel ch;
    int32_t flangerPos, echoPos;
} SmokeState;

static void smokeRender(int mode, const SmokeState *from, int16_t *out, uint32_t frames)
{
    gWinchanChannels[0] = *&from->ch;
    gWinchanFlangerPos = from->flangerPos;
    gWinchanEchoPos = from->echoPos;
    memset(gWinchanEchoBuffer, 0, 1000 * sizeof(WinchanFrame));
    memset(gWinchanFlangerLine, 0, sizeof gWinchanFlangerLine);
    gWinchanFlangerPhase = 0;
    gWinchanFlangerDelay = 0;
    if (mode < 0) {
        winchanMix((uint8_t *)out, frames * 4);
    } else {
        bmxSmoothMixMode((uint8_t *)out, frames * 4, mode);
    }
}

/* One channel, effects off or on: the smooth mixer in `mode` against the original. */
static void smoothIdentical(const char *what, const ZsdSample *sample, const ZsdVoice *v, uint32_t length,
                            uint32_t step, uint32_t fraction, int mode, int withEffects)
{
    static int16_t a[kSmokeFrames * 2], b[kSmokeFrames * 2];
    SmokeState s;

    smoothChannel(sample, v, length, step, 1, fraction);
    gWinchanEffectAFrames = withEffects ? 37 : 0;
    gWinchanEffectAGain = 0.5f;
    gWinchanEchoFeedback = 0.25f;
    gWinchanEffectBTailStart = withEffects ? 3 : 0;
    gWinchanEffectBTail = 0x40000;
    gWinchanEffectBFrames = 5;
    gWinchanEffectBGain = 0.5f;
    for (int i = 0; i < 4096; i++) {
        gWinchanLfoTable[i] = (uint16_t)(i < 2048 ? i * 2 : (4095 - i) * 2);
    }
    gWinchanChannels[0].gainLeft = 3.0f;
    s.ch = gWinchanChannels[0];
    s.flangerPos = 7;
    s.echoPos = 11;
    smokeRender(-1, &s, a, kSmokeFrames);
    smokeRender(mode, &s, b, kSmokeFrames);
    CHECK(memcmp(a, b, sizeof a) == 0, "%s mode %d step %#x frac %u effects %d", what, mode, (unsigned)step,
          (unsigned)fraction, withEffects);
}

/* Magnitude of the DFT of the left channel at `hz` (rate 22050). */
static double smoothMagnitude(const int16_t *out, uint32_t frames, double hz)
{
    double re = 0, im = 0;

    for (uint32_t i = 0; i < frames; i++) {
        const double ph = 2.0 * 3.14159265358979323846 * hz * (double)i / 22050.0;

        re += (double)out[i * 2] * cos(ph);
        im += (double)out[i * 2] * sin(ph);
    }
    return sqrt(re * re + im * im) / (double)frames;
}

static void smoothCheckBoundaries(void)
{
    ZsdVoice v;
    ZsdSample s;
    WinchanChannel ch;
    int16_t d[8];
    int16_t out[2];
    int16_t stereo[16];
    uint8_t bytes[8];

    memset(&v, 0, sizeof v);
    memset(&s, 0, sizeof s);
    memset(&ch, 0, sizeof ch);
    for (int i = 0; i < 8; i++) {
        d[i] = (int16_t)(100 * (i + 1) * (i % 2 ? -1 : 1));
        stereo[i * 2] = (int16_t)(40 * (i + 1));
        stereo[i * 2 + 1] = (int16_t)(-80 * (i + 1));
        bytes[i] = (uint8_t)(0x80 + 16 * (i + 1));
    }
    s.data = (const uint8_t *)d;
    ch.sample = &s;
    ch.length = 8;
    s.loopStart = 2;

    /* One shot: clamped at both ends. */
    s.format = 0;
    CHECK(bmxSmoothNeighbour(&ch, 8) == 7 && bmxSmoothNeighbour(&ch, 9) == 7, "one shot clamps at the end");
    CHECK(bmxSmoothNeighbour(&ch, -1) == 0 && bmxSmoothNeighbour(&ch, 3) == 3, "one shot clamps at 0");
    /* Loop: wraps by length - loopStart. */
    s.format = 1;
    CHECK(bmxSmoothNeighbour(&ch, 8) == 2 && bmxSmoothNeighbour(&ch, 9) == 3, "loop wraps to the loop start");
    CHECK(bmxSmoothNeighbour(&ch, 1) == 1 && bmxSmoothNeighbour(&ch, 0) == 0, "loop: before the loop");
    s.loopStart = 8; /* a loop of nothing: stays on the last frame */
    CHECK(bmxSmoothNeighbour(&ch, 9) == 7, "empty loop clamps");
    s.loopStart = 2;
    /* Ping-pong: mirrors at length - 1, and at the loop start when running backwards. */
    s.format = 1 | 8;
    ch.step = 0x1000;
    CHECK(bmxSmoothNeighbour(&ch, 8) == 6 && bmxSmoothNeighbour(&ch, 9) == 5, "ping-pong mirrors at the end");
    CHECK(bmxSmoothNeighbour(&ch, 1) == 1, "ping-pong forward: before the loop start is real data");
    ch.step = (uint32_t)-0x1000;
    CHECK(bmxSmoothNeighbour(&ch, 1) == 3 && bmxSmoothNeighbour(&ch, 0) == 4, "ping-pong backwards mirrors");
    CHECK(bmxSmoothNeighbour(&ch, 2) == 2, "ping-pong backwards: loop start itself");

    /* Through the mixer (gain 2: the output is the interpolated value). */
#define ONE(fmt, pos, frac, mode, expectL, expectR)                                                          \
    do {                                                                                                     \
        const int32_t el = (expectL), er = (expectR);                                                        \
        s.format = (fmt);                                                                                    \
        smoothChannel(&s, &v, 8, 0, (pos), (frac));                                                          \
        gWinchanChannels[0].gainRight = 2.0f;                                                                \
        bmxSmoothMixMode((uint8_t *)out, 4, (mode));                                                         \
        CHECK(out[0] == el >> 1 && out[1] == er >> 1,                                                        \
              "mixer format %d mode %d at %d: %d %d, expected %d %d", (fmt), (mode), (pos), out[0], out[1],  \
              el >> 1, er >> 1);                                                                             \
    } while (0)
    /* d: 100 -200 300 -400 500 -600 700 -800. Loop: frame 7 and half way to frame 2 (300). */
    ONE(1, 7, 0x800, 1, -800 + 300, -800 + 300);
    /* Ping-pong at the end: the neighbour is frame 6 (700). */
    ONE(1 | 8, 7, 0x800, 1, -800 + 700, -800 + 700);
    /* One shot at the end: held. */
    ONE(0, 7, 0x800, 1, -800 * 2, -800 * 2);
    {
        int16_t ramp[8] = {0, 100, 200, 300, 400, 500, 600, 700};

        s.data = (const uint8_t *)ramp;
        /* A line is interpolated exactly by both. */
        ONE(0, 3, 0x400, 2, 2 * 325, 2 * 325);
        ONE(0, 3, 0x400, 1, 2 * 325, 2 * 325);
        s.data = (const uint8_t *)d;
    }
    /* Stereo 16 bit: left 40 x (i+1), right -80 x (i+1); frame 3 and a quarter. */
    s.data = (const uint8_t *)stereo;
    ONE(2, 3, 0x400, 1, 2 * 170, 2 * -340);
    /* 8 bit mono: byte 0x80 + 16 x (i+1) is 256 x 16 x (i+1); frame 0 and half. */
    s.data = bytes;
    ONE(4, 0, 0x800, 1, 2 * 24 * 256, 2 * 24 * 256);
    /* 8 bit stereo: frames (16, 32) and (48, 64) (x 256). */
    ONE(4 | 2, 0, 0x800, 1, 2 * 32 * 256, 2 * 48 * 256);
#undef ONE
}

static void testSmoothMix(void)
{
    ZsdVoice v;
    ZsdSample s;
    uint32_t count = 0;
    static int16_t data[2205];
    static int16_t outOrig[4410 * 2], outLin[4410 * 2], outCub[4410 * 2];
    SmokeState st;
    DWORD oldProtect;
    /* The exe owns the range (src/native/range.S, as bmxnative does): make it writable. */
    void *range = VirtualProtect((void *)(uintptr_t)0x400000, 0x2a0000, PAGE_READWRITE, &oldProtect)
                      ? (void *)(uintptr_t)0x400000
                      : NULL;

    /* The option: off unless set to 1 or 2 (above 2 counts as cubic). */
    bmxConfigLoadText("", "test");
    (void)bmxSmoothJumps(&count);
    CHECK(count == 1, "smooth_resample unset: cubic by default, the swap is installed");
    bmxConfigLoadText("[audio]\nsmooth_resample = 0\n", "test");
    (void)bmxSmoothJumps(&count);
    CHECK(count == 0, "smooth_resample = 0: no swap");
    for (int mode = 1; mode <= 2; mode++) {
        const BmxHook *jumps;
        char text[64];

        snprintf(text, sizeof text, "[audio]\nsmooth_resample = %d\n", mode);
        bmxConfigLoadText(text, "test");
        jumps = bmxSmoothJumps(&count);
        CHECK(count == 1 && jumps[0].impl == (void *)bmxSmoothMix &&
                  jumps[0].orig == (uint32_t)(uintptr_t)winchanMix,
              "smooth_resample = %d installs the swap over winchanMix", mode);
    }
    bmxConfigLoadText("[audio]\nsmooth_resample = 9\n", "test");
    (void)bmxSmoothJumps(&count);
    CHECK(count == 1, "smooth_resample above 2 counts as 2");
    bmxConfigLoadText("", "test");

    CHECK(range != NULL, "the original's address range belongs to the test exe");
    if (range == NULL) {
        return;
    }
    smoothReset();

    /* Fraction 0 at step 0x1000 (and mode 0 at any step): identical to the original winchanMix, for every
     * sample kind, with and without the echo and the flanger. */
    {
        static const struct {
            const char *what;
            uint16_t format;
            uint32_t length;
        } kinds[] = {
            {"16 bit mono one shot", 0, 700},     {"16 bit mono loop", 1, 90},
            {"16 bit mono ping-pong", 1 | 8, 90}, {"16 bit stereo loop", 1 | 2, 90},
            {"8 bit mono loop", 1 | 4, 90},       {"8 bit stereo ping-pong", 1 | 2 | 4 | 8, 90},
        };
        static int16_t noise[2000];
        uint32_t seed = 12345;

        for (int i = 0; i < 2000; i++) {
            seed = seed * 1664525u + 1013904223u;
            noise[i] = (int16_t)(seed >> 16);
        }
        memset(&v, 0, sizeof v);
        memset(&s, 0, sizeof s);
        v.fade = 0x7f;
        s.data = (const uint8_t *)noise;
        s.loopStart = 20;
        for (size_t k = 0; k < sizeof kinds / sizeof kinds[0]; k++) {
            s.format = kinds[k].format;
            for (int fx = 0; fx < 2; fx++) {
                for (int mode = 0; mode <= 2; mode++) {
                    smoothIdentical(kinds[k].what, &s, &v, kinds[k].length, 0x1000, 0, mode, fx);
                }
                smoothIdentical(kinds[k].what, &s, &v, kinds[k].length, 0x1800, 0x3c0, 0, fx);
                smoothIdentical(kinds[k].what, &s, &v, kinds[k].length, 0x2000, 0, 2, fx);
            }
        }
        /* An attack envelope (the level changes every frame). */
        smoothChannel(&s, &v, 90, 0x1000, 1, 0);
        gWinchanChannels[0].phase = kWinchanPhaseAttack;
        gWinchanChannels[0].levelStep = 0.02f;
        gWinchanChannels[0].phaseFrames = 40;
        st.ch = gWinchanChannels[0];
        st.flangerPos = 0;
        st.echoPos = 0;
        {
            static int16_t a[kSmokeFrames * 2], b[kSmokeFrames * 2];

            gWinchanEffectAFrames = 0;
            gWinchanEffectBTailStart = 0;
            smokeRender(-1, &st, a, kSmokeFrames);
            smokeRender(2, &st, b, kSmokeFrames);
            CHECK(memcmp(a, b, sizeof a) == 0, "envelope: same as the original");
        }
    }

    /* A sine at 1 kHz in an 11,025 Hz sample, played at 22,050 Hz (step 0x800): the zero-order hold images it
     * at 11025 - 1000 Hz; interpolation attenuates the image. */
    smoothReset();
    for (int i = 0; i < 2205; i++) {
        data[i] = (int16_t)lrint(12000.0 * sin(2.0 * 3.14159265358979323846 * 1000.0 * i / 11025.0));
    }
    memset(&v, 0, sizeof v);
    memset(&s, 0, sizeof s);
    v.fade = 0x7f;
    s.format = 1;
    s.data = (const uint8_t *)data;
    smoothChannel(&s, &v, 2205, 0x800, 0, 0);
    st.ch = gWinchanChannels[0];
    st.flangerPos = 0;
    st.echoPos = 0;
    smokeRender(-1, &st, outOrig, 4410);
    smokeRender(1, &st, outLin, 4410);
    smokeRender(2, &st, outCub, 4410);
    {
        const double f0 = 1000.0, image = 11025.0 - 1000.0;
        const double o0 = smoothMagnitude(outOrig, 4410, f0), o1 = smoothMagnitude(outOrig, 4410, image);
        const double l0 = smoothMagnitude(outLin, 4410, f0), l1 = smoothMagnitude(outLin, 4410, image);
        const double c0 = smoothMagnitude(outCub, 4410, f0), c1 = smoothMagnitude(outCub, 4410, image);

        printf("smooth: 1 kHz tone / image at %.0f Hz: original %.1f / %.1f, linear %.1f / %.1f, cubic %.1f "
               "/ %.1f\n",
               image, o0, o1, l0, l1, c0, c1);
        CHECK(o1 > 0.1 * o0, "the original has an image");
        CHECK(l1 < 0.5 * o1 && c1 < 0.5 * l1, "image energy: original %.1f, linear %.1f, cubic %.1f", o1, l1,
              c1);
        CHECK(l0 > 0.95 * o0 && c0 > 0.95 * o0, "the tone itself keeps its level");
    }

    smoothReset();
    smoothCheckBoundaries();
    free(gWinchanEchoBuffer);
    gWinchanEchoBuffer = NULL;
    VirtualProtect(range, 0x2a0000, oldProtect, &oldProtect);
}

/* ---- waveOut (the movies' sound) against the real winmm ---- */

typedef struct WaveApi {
    const char *name;
    MMRESULT(WINAPI *open)(HWAVEOUT *, UINT, const WAVEFORMATEX *, DWORD_PTR, DWORD_PTR, DWORD);
    MMRESULT(WINAPI *prepare)(HWAVEOUT, WAVEHDR *, UINT);
    MMRESULT(WINAPI *unprepare)(HWAVEOUT, WAVEHDR *, UINT);
    MMRESULT(WINAPI *write)(HWAVEOUT, WAVEHDR *, UINT);
    MMRESULT(WINAPI *pause)(HWAVEOUT);
    MMRESULT(WINAPI *restart)(HWAVEOUT);
    MMRESULT(WINAPI *reset)(HWAVEOUT);
    MMRESULT(WINAPI *close)(HWAVEOUT);
    BOOL(WINAPI *peek)(MSG *, HWND, UINT, UINT, UINT); /* where the device's messages arrive */
} WaveApi;

enum { kWaveHeaders = 6, kWaveTrace = 96 };

/* Waits up to `ms` for window messages from the device; each MM_WOM_* goes into the trace as message * 16 +
 * header index (+15: no header) when wParam is the device. Returns how many MM_WOM_DONE came. */
static int wavePump(const WaveApi *api, HWND window, HWAVEOUT device, WAVEHDR *headers, int ms, int stopAfter,
                    uint32_t *trace, int *n)
{
    const uint64_t end = SDL_GetTicks() + (uint64_t)ms;
    int done = 0;

    while (SDL_GetTicks() < end && (stopAfter == 0 || done < stopAfter)) {
        MSG msg;

        if (!api->peek(&msg, window, MM_WOM_OPEN, MM_WOM_DONE, PM_REMOVE)) {
            SDL_Delay(1);
            continue;
        }
        if ((HWAVEOUT)msg.wParam != device) {
            trace[(*n)++] = 0xbad;
            continue;
        }
        int index = 15;

        for (int i = 0; i < kWaveHeaders; i++) {
            if ((WAVEHDR *)msg.lParam == &headers[i]) {
                index = i;
            }
        }
        trace[(*n)++] = msg.message * 16 + (uint32_t)index;
        done += msg.message == MM_WOM_DONE;
    }
    return done;
}

/* The movie's protocol and its error cases on one device; the results, the headers' flags and the messages go
 * into the trace. */
static int waveScript(const WaveApi *api, HWND window, uint32_t *trace)
{
    static int16_t data[kWaveHeaders][37800];
    WAVEFORMATEX format = {WAVE_FORMAT_PCM, 1, 37800, 75600, 2, 16, 0};
    WAVEHDR headers[kWaveHeaders];
    HWAVEOUT device = NULL;
    int n = 0;

    memset(headers, 0, sizeof headers);
    for (int i = 0; i < kWaveHeaders; i++) {
        headers[i].lpData = (LPSTR)data[i];
        headers[i].dwBufferLength = i < 4 ? 2000 : 75600; /* 26 ms, then 1 s */
    }
    trace[n++] = api->open(&device, WAVE_MAPPER, &format, (DWORD_PTR)window, 0, CALLBACK_WINDOW);
    if (trace[0] != 0) {
        return n;
    }
    wavePump(api, window, device, headers, 100, 0, trace, &n);
    trace[n++] = api->write(device, &headers[0], sizeof headers[0]); /* not prepared */
    for (int i = 0; i < kWaveHeaders; i++) {
        trace[n++] = api->prepare(device, &headers[i], sizeof headers[i]);
        trace[n++] = headers[i].dwFlags;
    }
    trace[n++] = api->prepare(device, &headers[0], sizeof headers[0]); /* again */
    trace[n++] = api->pause(device);
    for (int i = 0; i < 4; i++) {
        trace[n++] = api->write(device, &headers[i], sizeof headers[i]);
        trace[n++] = headers[i].dwFlags;
    }
    trace[n++] = api->write(device, &headers[0], sizeof headers[0]); /* queued already */
    {
        uint32_t ignored[kWaveTrace];
        int none = 0;

        trace[n++] =
            (uint32_t)wavePump(api, window, device, headers, 150, 0, ignored, &none); /* paused: none */
    }
    trace[n++] = api->restart(device);
    trace[n++] = (uint32_t)wavePump(api, window, device, headers, 3000, 4, trace, &n);
    for (int i = 0; i < 4; i++) {
        trace[n++] = headers[i].dwFlags;
    }
    trace[n++] = api->write(device, &headers[4], sizeof headers[4]);
    trace[n++] = api->write(device, &headers[5], sizeof headers[5]);
    trace[n++] = api->unprepare(device, &headers[4], sizeof headers[4]); /* still queued */
    trace[n++] = api->close(device);                                     /* still playing */
    trace[n++] = api->reset(device);
    trace[n++] = (uint32_t)wavePump(api, window, device, headers, 1000, 2, trace, &n);
    trace[n++] = headers[4].dwFlags;
    trace[n++] = headers[5].dwFlags;
    for (int i = 0; i < kWaveHeaders; i++) {
        trace[n++] = api->unprepare(device, &headers[i], sizeof headers[i]);
        trace[n++] = headers[i].dwFlags;
    }
    trace[n++] = api->close(device);
    wavePump(api, window, device, headers, 200, 0, trace, &n);
    return n;
}

static void testWaveOut(void)
{
    const HWND window = CreateWindowExA(0, "STATIC", "bmxplatformtest waveout", WS_OVERLAPPEDWINDOW, 0, 0, 64,
                                        64, NULL, NULL, GetModuleHandleA(NULL), NULL); /* never shown */
    const WaveApi real = {
        "winmm",
        (void *)bmxRealProc("WINMM.dll", "waveOutOpen"),
        (void *)bmxRealProc("WINMM.dll", "waveOutPrepareHeader"),
        (void *)bmxRealProc("WINMM.dll", "waveOutUnprepareHeader"),
        (void *)bmxRealProc("WINMM.dll", "waveOutWrite"),
        (void *)bmxRealProc("WINMM.dll", "waveOutPause"),
        (void *)bmxRealProc("WINMM.dll", "waveOutRestart"),
        (void *)bmxRealProc("WINMM.dll", "waveOutReset"),
        (void *)bmxRealProc("WINMM.dll", "waveOutClose"),
        PeekMessageA,
    };
    const WaveApi ours = {
        "ours",
        (void *)bmxStandWaveOutOpen,
        (void *)bmxStandWaveOutPrepareHeader,
        (void *)bmxStandWaveOutUnprepareHeader,
        (void *)bmxStandWaveOutWrite,
        (void *)bmxStandWaveOutPause,
        (void *)bmxStandWaveOutRestart,
        (void *)bmxStandWaveOutReset,
        (void *)bmxStandWaveOutClose,
        (void *)bmxStandPeekMessageA, /* our message queue (window.c) */
    };
    uint32_t a[kWaveTrace], b[kWaveTrace];
    int na, nb;

    if (gMeter == NULL && !muteSession()) {
        printf("waveout: could not mute this process's audio session; skipped\n");
        DestroyWindow(window);
        return;
    }
    memset(a, 0, sizeof a);
    memset(b, 0, sizeof b);
    na = waveScript(&real, window, a);
    nb = waveScript(&ours, window, b);
    CHECK(na == nb && memcmp(a, b, (size_t)na * sizeof a[0]) == 0, "waveOut script: %d vs %d entries", na,
          nb);
    printf("waveout: %d results, flags and messages, %s\n", na,
           na == nb && memcmp(a, b, (size_t)na * sizeof a[0]) == 0 ? "equal to winmm's" : "DIFFERENT:");
    if (na != nb || memcmp(a, b, (size_t)na * sizeof a[0]) != 0) {
        for (int i = 0; i < SDL_max(na, nb); i++) {
            printf("  %2d: winmm %5x ours %5x%s\n", i, a[i], b[i], a[i] != b[i] ? "  <--" : "");
        }
    }
    CHECK(bmxStandSndPlaySoundA(NULL, 0) == 1, "sndPlaySoundA(NULL, 0)");
    DestroyWindow(window);
}

/* ---- the window, the message queue and message boxes (window.c) ---- */

enum { kMaxRecorded = 64 };

static struct {
    uint32_t message[kMaxRecorded];
    uintptr_t wParam[kMaxRecorded], lParam[kMaxRecorded];
    int n;
} gProcLog;

/* A window procedure shaped like the game's: WM_GETMINMAXINFO locks the size, WM_DESTROY quits, the rest goes
 * to DefWindowProc. */
static intptr_t BMX_WINAPI testProc(void *window, uint32_t message, uintptr_t wParam, uintptr_t lParam)
{
    if (gProcLog.n < kMaxRecorded) {
        gProcLog.message[gProcLog.n] = message;
        gProcLog.wParam[gProcLog.n] = wParam;
        gProcLog.lParam[gProcLog.n] = lParam;
        gProcLog.n++;
    }
    switch (message) {
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lParam)->ptMaxTrackSize.x = 640;
        ((MINMAXINFO *)lParam)->ptMaxTrackSize.y = 480;
        return 0;
    case WM_DESTROY:
        bmxStandClipCursor(NULL);
        bmxStandPostQuitMessage(0);
        return 0;
    default:
        return bmxStandDefWindowProcA(window, message, wParam, lParam);
    }
}

static int procSaw(uint32_t message, uintptr_t wParam)
{
    for (int i = 0; i < gProcLog.n; i++) {
        if (gProcLog.message[i] == message && gProcLog.wParam[i] == wParam) {
            return i + 1;
        }
    }
    return 0;
}

/* Message boxes are "shown" through this hook, which answers like a user (BMX_KEYS style) and records them.
 */
static struct {
    int shown, answer;
    char title[64], text[128];
    int buttons, ids[3], defaultId, escapeId;
    SDL_MessageBoxFlags flags;
} gBox;

static int boxHook(const SDL_MessageBoxData *box)
{
    gBox.shown++;
    SDL_strlcpy(gBox.title, box->title, sizeof gBox.title);
    SDL_strlcpy(gBox.text, box->message, sizeof gBox.text);
    gBox.buttons = box->numbuttons;
    gBox.flags = box->flags;
    gBox.defaultId = gBox.escapeId = 0;
    for (int i = 0; i < box->numbuttons && i < 3; i++) {
        gBox.ids[i] = box->buttons[i].buttonID;
        if ((box->buttons[i].flags & SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT) != 0) {
            gBox.defaultId = box->buttons[i].buttonID;
        }
        if ((box->buttons[i].flags & SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT) != 0) {
            gBox.escapeId = box->buttons[i].buttonID;
        }
    }
    return gBox.answer;
}

static void testMessageBoxes(void)
{
    static const char quit[] = "Do you want to quit the game?";
    static const char title[] = "Dave Mirra Freestyle BMX";

    bmxWindowSetMessageBoxHook(boxHook);
    /* [ui] confirm_quit off (the default): the quit prompt is answered Yes, no box. */
    bmxConfigLoadText("", "test");
    memset(&gBox, 0, sizeof gBox);
    CHECK(bmxStandMessageBoxA(NULL, quit, title, 0x10124) == IDYES, "quit prompt answered Yes");
    CHECK(gBox.shown == 0, "no box for the quit prompt with confirm_quit = 0");
    /* Other boxes still show: the same text with another style, another text with the same style, an assert.
     */
    gBox.answer = IDNO;
    CHECK(bmxStandMessageBoxA(NULL, quit, title, 4 /* MB_YESNO */) == IDNO && gBox.shown == 1,
          "other style shown");
    CHECK(bmxStandMessageBoxA(NULL, "Quit?", title, 0x10124) == IDNO && gBox.shown == 2, "other text shown");
    gBox.answer = IDCANCEL;
    CHECK(bmxStandMessageBoxA(NULL, "***ASSERT***", "Error", 0x42013) == IDCANCEL && gBox.shown == 3,
          "assert box shown");
    CHECK(gBox.buttons == 3 && gBox.ids[0] == IDYES && gBox.ids[1] == IDNO && gBox.ids[2] == IDCANCEL &&
              gBox.defaultId == IDYES && gBox.escapeId == IDCANCEL &&
              (gBox.flags & SDL_MESSAGEBOX_ERROR) != 0,
          "assert box: Yes / No / Cancel, default Yes, Esc Cancel, error icon");
    /* [ui] confirm_quit = 1: the box as in the original. */
    bmxConfigLoadText("[ui]\nconfirm_quit = 1\n", "test");
    memset(&gBox, 0, sizeof gBox);
    gBox.answer = IDYES;
    CHECK(bmxStandMessageBoxA(NULL, quit, title, 0x10124) == IDYES && gBox.shown == 1, "quit prompt shown");
    CHECK(strcmp(gBox.text, quit) == 0 && strcmp(gBox.title, title) == 0, "quit prompt text \"%s\" / \"%s\"",
          gBox.text, gBox.title);
    CHECK(gBox.buttons == 2 && gBox.ids[0] == IDYES && gBox.ids[1] == IDNO && gBox.defaultId == IDNO &&
              gBox.escapeId == 0 && (gBox.flags & SDL_MESSAGEBOX_INFORMATION) != 0,
          "quit prompt: Yes / No, default No (MB_DEFBUTTON2), no Esc button");
    gBox.answer = IDNO;
    CHECK(bmxStandMessageBoxA(NULL, quit, title, 0x10124) == IDNO, "quit prompt: No");
    gBox.answer = -1;
    CHECK(bmxStandMessageBoxA(NULL, quit, title, 0x10124) == IDNO, "quit prompt closed: the default, No");
    gBox.answer = -1;
    CHECK(bmxStandMessageBoxA(NULL, "x", "y", 1 /* MB_OKCANCEL */) == IDCANCEL, "OK / Cancel closed: Cancel");
    /* A scripted run (BMX_KEYS) answers without a box: Yes / No -> Yes, Yes / No / Cancel -> Cancel, else OK.
     */
    gScripted = 1;
    memset(&gBox, 0, sizeof gBox);
    CHECK(bmxStandMessageBoxA(NULL, quit, title, 0x10124) == IDYES, "scripted quit prompt");
    CHECK(bmxStandMessageBoxA(NULL, "***ASSERT***", "Error", 0x42013) == IDCANCEL, "scripted assert");
    CHECK(bmxStandMessageBoxA(NULL, "error", "Error", 0x42010) == IDOK, "scripted error");
    CHECK(gBox.shown == 0, "no box in a scripted run");
    gScripted = 0;
    bmxConfigLoadText("", "test");
    bmxWindowSetMessageBoxHook(NULL);
    printf("message boxes: quit prompt skipped by default, shown with confirm_quit = 1\n");
}

/* The queue on its own (no window): filters, order, WM_QUIT last, posting from another thread. */
static int SDLCALL postFromThread(void *data)
{
    for (int i = 0; i < 100; i++) {
        bmxWindowPost(data, MM_WOM_DONE, 1, (uintptr_t)i);
    }
    return 0;
}

static void testMessageQueue(void)
{
    void *const token = (void *)(uintptr_t)0x1234;
    BmxMsg msg;
    SDL_Thread *thread;
    int got = 0, inOrder = 1;

    while (bmxStandPeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
    }
    CHECK(!bmxStandPeekMessageA(&msg, NULL, 0, 0, PM_REMOVE), "empty");
    bmxWindowPost(token, 0x400, 7, 8);
    bmxWindowPost(token, MM_WOM_OPEN, 1, 0);
    CHECK(bmxStandPeekMessageA(&msg, token, MM_WOM_OPEN, MM_WOM_DONE, PM_NOREMOVE) &&
              msg.message == MM_WOM_OPEN,
          "filtered peek: 0x%x", msg.message);
    CHECK(bmxStandPeekMessageA(&msg, (void *)(uintptr_t)0x999, 0, 0, PM_REMOVE) == 0, "other window: none");
    CHECK(bmxStandPeekMessageA(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == 0x400 && msg.window == token &&
              msg.wParam == 7 && msg.lParam == 8,
          "first in, first out");
    bmxStandPostQuitMessage(5);
    CHECK(bmxStandPeekMessageA(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == MM_WOM_OPEN, "then the second");
    CHECK(bmxStandPeekMessageA(&msg, token, MM_WOM_OPEN, MM_WOM_DONE, PM_REMOVE) && msg.message == WM_QUIT &&
              msg.wParam == 5,
          "WM_QUIT once the queue is empty (any filter)");
    CHECK(!bmxStandPeekMessageA(&msg, NULL, 0, 0, PM_REMOVE), "WM_QUIT only once");
    thread = SDL_CreateThread(postFromThread, "post", token);
    while (got < 100) {
        if (bmxStandPeekMessageA(&msg, token, MM_WOM_DONE, MM_WOM_DONE, PM_REMOVE)) {
            inOrder &= msg.lParam == (uintptr_t)got;
            got++;
        }
    }
    SDL_WaitThread(thread, NULL);
    CHECK(inOrder, "messages from another thread in order");
    printf("message queue: ok\n");
}

/* The window on SDL (never shown): creation messages, the title, the instance, the cursor counter, a focus
 * change, the script's keys as WM_KEYDOWN, close -> WM_CLOSE -> WM_DESTROY -> WM_QUIT. */
static void testWindow(void)
{
    void *window;
    BmxMsg msg;
    SDL_Event e;
    int keys = 0, quit = 0;
    int maxW = 0, maxH = 0;

    memset(&gProcLog, 0, sizeof gProcLog);
    window = bmxWindowCreate("RenderWare Application", 640, 480, testProc, 1);
    CHECK(window != NULL, "window");
    if (window == NULL) {
        return;
    }
    CHECK(gProcLog.n >= 2 && gProcLog.message[0] == WM_GETMINMAXINFO && gProcLog.message[1] == WM_CREATE,
          "WM_GETMINMAXINFO, WM_CREATE");
    SDL_GetWindowMaximumSize(bmxWindowSdl(window), &maxW, &maxH);
    CHECK(maxW == 640 && maxH == 480, "size lock %dx%d", maxW, maxH);
    CHECK(bmxWindowNative(window) != NULL && bmxWindowNative(window) != window, "HWND behind the token");
    CHECK(bmxStandSetWindowTextA(window, "Dave Mirra Freestyle BMX") == 1 &&
              strcmp(SDL_GetWindowTitle(bmxWindowSdl(window)), "Dave Mirra Freestyle BMX") == 0,
          "title");
    bmxWindowSetInstance((void *)GetModuleHandleA(NULL));
    CHECK(bmxStandGetWindowLongA(window, GWL_HINSTANCE) == (intptr_t)GetModuleHandleA(NULL), "GWL_HINSTANCE");
    CHECK(bmxStandShowCursor(0) == -1 && bmxStandShowCursor(1) == 0, "ShowCursor counter");
    {
        int32_t rect[4] = {0, 0, 640, 480};

        CHECK(bmxStandAdjustWindowRect(rect, 0xcf0000, 0) == 1 && rect[2] == 640 && rect[3] == 480,
              "AdjustWindowRect: client sizes");
    }
    /* Focus gained: WM_ACTIVATE WA_ACTIVE, sent inside PeekMessage. The script's held keys (DIK 0x1e and
     * 0xc8, see bmxKeysFillState above) come as posted WM_KEYDOWN with their scan codes. */
    gScripted = 1;
    SDL_zero(e);
    e.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    e.window.windowID = SDL_GetWindowID(bmxWindowSdl(window));
    SDL_PushEvent(&e);
    SDL_Delay(2);
    while (bmxStandPeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_KEYDOWN && msg.window == window) {
            keys |= msg.lParam == 0x001e0001 ? 1 : msg.lParam == 0x01480001 ? 2 : 4;
            bmxStandTranslateMessage(&msg);
            bmxStandDispatchMessageA(&msg);
        }
    }
    gScripted = 0;
    CHECK(procSaw(WM_ACTIVATE, WA_ACTIVE), "WM_ACTIVATE WA_ACTIVE");
    CHECK(keys == 3, "script keys as WM_KEYDOWN (A: 0x001e0001, Up: 0x01480001): mask %d", keys);
    CHECK(procSaw(WM_KEYDOWN, 0), "WM_KEYDOWN dispatched to the procedure");
    /* Close: WM_CLOSE, whose default destroys the window: WM_ACTIVATE inactive, WM_DESTROY, WM_NCDESTROY;
     * the procedure's PostQuitMessage gives WM_QUIT. */
    SDL_zero(e);
    e.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    e.window.windowID = SDL_GetWindowID(bmxWindowSdl(window));
    SDL_PushEvent(&e);
    SDL_Delay(2);
    for (int i = 0; i < 10 && !quit; i++) {
        quit = bmxStandPeekMessageA(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_QUIT;
    }
    {
        const int close = procSaw(WM_CLOSE, 0), inactive = procSaw(WM_ACTIVATE, WA_INACTIVE);
        const int destroy = procSaw(WM_DESTROY, 0), ncdestroy = procSaw(WM_NCDESTROY, 0);

        CHECK(close && inactive > close && destroy > inactive && ncdestroy > destroy,
              "close order: WM_CLOSE %d, WM_ACTIVATE 0 %d, WM_DESTROY %d, WM_NCDESTROY %d", close, inactive,
              destroy, ncdestroy);
    }
    CHECK(quit, "WM_QUIT after the close");
    gProcLog.n = 0;
    bmxWindowPost(window, 0x400, 0, 0);
    CHECK(!bmxStandPeekMessageA(&msg, NULL, 0, 0, PM_REMOVE) && gProcLog.n == 0,
          "a destroyed window: nothing");
    printf("window: creation, activation, script keys and close as Windows sends them\n");
}

int main(int argc, char **argv)
{
    /* `bmxplatformtest music`: only the music override's tests (offline, no devices or windows). */
    if (argc > 1 && strcmp(argv[1], "music") == 0) {
        failures = testMusic(&checks);
        printf("%d checks, %d failures\n", checks, failures);
        return failures != 0;
    }
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
    testStick8Way();
    testKeyTable();
    testHeldKeys();
    testInputObjects();
    testInputCheck();
    testTwoPads();
    testConfig();
    testSound();
    testSmoothMix();
    failures += testMusic(&checks); /* test_music.c */
    testWaveOut();
    testMessageBoxes();
    testMessageQueue();
    testWindow();
    bmxPlatformShutdown();
    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
