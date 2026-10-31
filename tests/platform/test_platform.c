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
    /* one-shot fires once */
    gCalls = 0;
    id = bmxStandTimeSetEvent(20, 1, tick, 7, 0);
    SDL_Delay(120);
    CHECK(gCalls == 1, "one-shot fired %ld times", (long)gCalls);
    bmxStandTimeKillEvent(id);
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
    testTime();
    testSections();
    testTimer();
    compareWithWinmm();
    testFiles();
    bmxTimerShutdown();
    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
