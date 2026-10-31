/*
 * Time and the small kernel32 functions (docs/PLATFORM.md 3.6, S2): timeGetTime, GetTickCount, Sleep,
 * QueryPerformanceFrequency / Counter, OutputDebugStringA, DebugBreak and the critical sections.
 *
 * Time. Windows reports both timeGetTime and GetTickCount as milliseconds since boot, as a 32-bit value that
 * wraps after 49.7 days; they share one base. Ours count from the start of the process plus a fixed non-zero
 * base (the original's frame limiter treats a last-tick value of 0 as "unset"), and wrap the same way.
 * Resolution: timeGetTime has 1 ms (what winmm gives while its timer holds the system timer at 1 ms, which
 * the game's sound timer does); GetTickCount only moves on the system tick, 15.625 ms (64 Hz), i.e. it
 * reports floor(k * 15.625). QueryPerformanceCounter is SDL's, which on Windows is the real
 * QueryPerformanceCounter (same base and 10 MHz frequency).
 */
#include <SDL3/SDL.h>
#include <string.h>

#include "platform/input.h"
#include "platform/platform.h"
#include "platform/sound.h"

enum {
    kDefaultBase = 0x0036ee80, /* one hour of "uptime" */
    kTickNs = 15625000,        /* the 64 Hz system tick */
};

static uint64_t gOrigin, gFrequency;
static uint32_t gBase = kDefaultBase;
static volatile int gInitDone;

void bmxTimeSetBase(uint32_t base)
{
    gBase = base;
}

void bmxPlatformInit(void)
{
    const char *base;

    if (gInitDone) {
        return;
    }
    gFrequency = SDL_GetPerformanceFrequency();
    gOrigin = SDL_GetPerformanceCounter();
    base = SDL_getenv("BMX_TIME_BASE");
    if (base != NULL && *base != 0) {
        gBase = (uint32_t)SDL_strtoul(base, NULL, 0);
        bmxLog("platform: time base %#x (BMX_TIME_BASE)\n", (unsigned)gBase);
    }
    gInitDone = 1;
}

static uint64_t elapsedNs(void)
{
    uint64_t c;

    bmxPlatformInit();
    c = SDL_GetPerformanceCounter() - gOrigin;
    return (c / gFrequency) * 1000000000ull + (c % gFrequency) * 1000000000ull / gFrequency;
}

uint64_t bmxTimeNs(void)
{
    return elapsedNs();
}

uint32_t bmxTimeMsAt(uint64_t ns)
{
    return gBase + (uint32_t)(ns / 1000000ull);
}

uint32_t bmxTimeGetTime(void)
{
    return bmxTimeMsAt(elapsedNs());
}

uint32_t bmxGetTickCount(void)
{
    const uint64_t ticks = elapsedNs() / kTickNs;

    return gBase + (uint32_t)(ticks * kTickNs / 1000000ull);
}

uint32_t BMX_WINAPI bmxStandTimeGetTime(void)
{
    return bmxTimeGetTime();
}

uint32_t BMX_WINAPI bmxStandGetTickCount(void)
{
    return bmxGetTickCount();
}

int32_t BMX_WINAPI bmxStandQueryPerformanceFrequency(void *out)
{
    const int64_t f = (int64_t)SDL_GetPerformanceFrequency();

    memcpy(out, &f, sizeof f);
    return 1;
}

int32_t BMX_WINAPI bmxStandQueryPerformanceCounter(void *out)
{
    const int64_t c = (int64_t)SDL_GetPerformanceCounter();

    memcpy(out, &c, sizeof c);
    return 1;
}

void BMX_WINAPI bmxStandSleep(uint32_t ms)
{
    SDL_Delay(ms);
}

/*
 * The original's debug output (zrwDebugOutput: asset loading, the movie player's every audio tick...) goes to
 * a debugger, and nowhere without one. Here it is dropped too, unless BMX_DEBUG_OUTPUT=1 sends it to the log.
 */
void BMX_WINAPI bmxStandOutputDebugStringA(const char *text)
{
    static int enabled = -1;
    const size_t n = text != NULL ? strlen(text) : 0;

    if (enabled < 0) {
        const char *e = SDL_getenv("BMX_DEBUG_OUTPUT");

        enabled = e != NULL && *e != 0 && *e != '0';
    }
    if (!enabled) {
        return;
    }
    bmxLog("OutputDebugString: %s%s", text != NULL ? text : "", (n > 0 && text[n - 1] == '\n') ? "" : "\n");
}

void BMX_WINAPI bmxStandDebugBreak(void)
{
    SDL_TriggerBreakpoint();
}

/*
 * Critical sections. The game owns the 24-byte CRITICAL_SECTION in its data; the first word of it holds our
 * SDL mutex (recursive, like a critical section). An uninitialised or deleted section has no mutex: Enter and
 * Leave then do nothing (Windows would crash).
 */
typedef struct Section {
    SDL_Mutex *mutex;
    uint8_t rest[20];
} Section;

void BMX_WINAPI bmxStandInitializeCriticalSection(void *section)
{
    Section *const s = section;

    memset(s, 0, 24);
    s->mutex = SDL_CreateMutex();
}

void BMX_WINAPI bmxStandEnterCriticalSection(void *section)
{
    SDL_LockMutex(((Section *)section)->mutex);
}

void BMX_WINAPI bmxStandLeaveCriticalSection(void *section)
{
    SDL_UnlockMutex(((Section *)section)->mutex);
}

void BMX_WINAPI bmxStandDeleteCriticalSection(void *section)
{
    Section *const s = section;
    SDL_Mutex *const m = s->mutex;

    s->mutex = NULL;
    SDL_DestroyMutex(m);
}

void bmxPlatformShutdown(void)
{
    bmxTimerShutdown();
    bmxSoundShutdown();
    bmxCheckSummary();
}
