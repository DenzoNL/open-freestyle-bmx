/*
 * The winmm multimedia timer (docs/PLATFORM.md P11, S3): timeGetDevCaps, timeBeginPeriod / timeEndPeriod,
 * timeSetEvent and timeKillEvent on SDL threads.
 *
 * The game's sound pump (bmxSoundOpen: timeSetEvent(16, 1, zoverPollTimer, 0, TIME_PERIODIC)) runs
 * winchanPump on winmm's timer thread, concurrently with the game thread. Windows has one timer thread for
 * every event, at THREAD_PRIORITY_TIME_CRITICAL; here each event gets its own thread at
 * SDL_THREAD_PRIORITY_TIME_CRITICAL (the game has one event; the 8 slots are an arbitrary limit). A periodic
 * event keeps absolute deadlines (the next call is due one period after the previous one was due, not after
 * it finished) and resynchronises instead of firing a burst when it fell behind by more than a period.
 *
 * timeKillEvent returns after the event's callback is no longer running (it joins the thread), as winmm's
 * does for a callback on another thread. Called from the callback itself it only marks the event stopped.
 *
 * Resolution: every period request is accepted (the range is 1 ms .. 1000000 ms) and has no further effect,
 * the clocks here are always fine (kernel.c).
 *
 * BMX_TIMER_LOG=1 logs, when an event is killed, how it ran: calls, mean / min / max / standard deviation of
 * the interval between callback starts, calls started more than 2 ms late, and the mean callback duration.
 */
#include <SDL3/SDL.h>
#include <math.h>

#include "platform/standins.h"

enum { kMaxTimers = 8, kTimePeriodic = 1, kTimerrStruct = 129, kMmsyserrInvalParam = 11 };

typedef struct Timer {
    SDL_ThreadID threadId;
    /* statistics (written by the timer thread only; read after it ended) */
    uint64_t calls, late, lastStart;
    double sumInterval, sumSquares, minInterval, maxInterval, sumDuration;
    BmxTimeProc proc;
    SDL_Thread *thread;
    SDL_AtomicInt stop;
    int used, finished, detached;
    uint32_t id, delay, user, flags;
} Timer;

static Timer gTimers[kMaxTimers];
static SDL_Mutex *gLock;
static SDL_InitState gLockState;
static uint32_t gNextId;

static int logging(void)
{
    static int state = -1;

    if (state < 0) {
        const char *e = SDL_getenv("BMX_TIMER_LOG");

        state = e != NULL && *e != 0 && *e != '0';
    }
    return state;
}

static void lockInit(void)
{
    if (SDL_ShouldInit(&gLockState)) {
        gLock = SDL_CreateMutex();
        SDL_SetInitialized(&gLockState, true);
    }
}

static void logStatistics(const Timer *t)
{
    double mean, deviation;

    if (!logging() || t->calls < 2) {
        return;
    }
    mean = t->sumInterval / (double)(t->calls - 1);
    deviation = sqrt(fmax(0.0, t->sumSquares / (double)(t->calls - 1) - mean * mean));
    bmxLog(
        "timer %u (period %u ms): %llu calls, interval mean %.3f ms min %.3f max %.3f sd %.3f, %llu late by "
        "more than 2 ms, callback mean %.3f ms\n",
        (unsigned)t->id, (unsigned)t->delay, (unsigned long long)t->calls, mean, t->minInterval,
        t->maxInterval, deviation, (unsigned long long)t->late, t->sumDuration / (double)t->calls);
}

static void record(Timer *t, uint64_t start)
{
    if (t->calls > 0) {
        const double interval = (double)(start - t->lastStart) / 1e6;

        t->sumInterval += interval;
        t->sumSquares += interval * interval;
        t->minInterval = t->calls == 1 || interval < t->minInterval ? interval : t->minInterval;
        t->maxInterval = t->calls == 1 || interval > t->maxInterval ? interval : t->maxInterval;
        t->late += interval > (double)t->delay + 2.0;
    }
    t->lastStart = start;
    t->calls++;
}

static int SDLCALL timerThread(void *arg)
{
    Timer *const t = arg;
    const uint64_t period = (uint64_t)(t->delay ? t->delay : 1) * 1000000ull;
    uint64_t deadline;

    t->threadId = SDL_GetCurrentThreadID();
    SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_TIME_CRITICAL);
    deadline = SDL_GetTicksNS() + period;
    while (!SDL_GetAtomicInt(&t->stop)) {
        uint64_t now = SDL_GetTicksNS();
        uint64_t end;

        if (now < deadline) {
            SDL_DelayPrecise(deadline - now);
            if (SDL_GetAtomicInt(&t->stop)) {
                break;
            }
            now = SDL_GetTicksNS();
        }
        record(t, now);
        t->proc(t->id, 0, t->user, 0, 0);
        end = SDL_GetTicksNS();
        t->sumDuration += (double)(end - now) / 1e6;
        if (!(t->flags & kTimePeriodic)) {
            break;
        }
        deadline += period;
        if (end > deadline + period) {
            deadline = end;
        }
    }
    SDL_LockMutex(gLock);
    t->finished = 1;
    if (t->detached) {
        logStatistics(t);
        t->used = 0;
    }
    SDL_UnlockMutex(gLock);
    return 0;
}

uint32_t BMX_WINAPI bmxStandTimeGetDevCaps(void *caps, uint32_t size)
{
    uint32_t *const c = caps;

    if (size < 8 || c == NULL) {
        return kTimerrStruct;
    }
    c[0] = 1;
    c[1] = 1000000;
    return 0;
}

uint32_t BMX_WINAPI bmxStandTimeBeginPeriod(uint32_t period)
{
    (void)period;
    return 0;
}

uint32_t BMX_WINAPI bmxStandTimeEndPeriod(uint32_t period)
{
    (void)period;
    return 0;
}

/* Joins and frees the slots of finished one-shot events. Caller holds the lock. */
static void reap(void)
{
    for (int i = 0; i < kMaxTimers; i++) {
        Timer *const t = &gTimers[i];

        if (t->used && t->finished && !t->detached) {
            SDL_WaitThread(t->thread, NULL);
            t->used = 0;
        }
    }
}

uint32_t BMX_WINAPI bmxStandTimeSetEvent(uint32_t delay, uint32_t resolution, BmxTimeProc proc, uint32_t user,
                                         uint32_t flags)
{
    Timer *t = NULL;
    uint32_t id = 0;

    (void)resolution;
    lockInit();
    SDL_LockMutex(gLock);
    reap();
    for (int i = 0; i < kMaxTimers && t == NULL; i++) {
        t = gTimers[i].used ? NULL : &gTimers[i];
    }
    if (t != NULL && proc != NULL) {
        SDL_zerop(t);
        t->used = 1;
        t->id = id = ++gNextId;
        t->delay = delay;
        t->user = user;
        t->flags = flags;
        t->proc = proc;
        t->thread = SDL_CreateThread(timerThread, "bmx-timer", t);
        if (t->thread == NULL) {
            t->used = 0;
            id = 0;
        }
    }
    SDL_UnlockMutex(gLock);
    if (logging()) {
        bmxLog("timer %u: timeSetEvent(%u ms, flags %u)%s\n", (unsigned)id, (unsigned)delay, (unsigned)flags,
               id ? "" : " failed");
    }
    return id;
}

uint32_t BMX_WINAPI bmxStandTimeKillEvent(uint32_t id)
{
    Timer *t = NULL;
    SDL_Thread *thread;

    lockInit();
    SDL_LockMutex(gLock);
    for (int i = 0; i < kMaxTimers; i++) {
        if (gTimers[i].used && gTimers[i].id == id && !gTimers[i].detached) {
            t = &gTimers[i];
        }
    }
    if (t == NULL) {
        SDL_UnlockMutex(gLock);
        return kMmsyserrInvalParam;
    }
    SDL_SetAtomicInt(&t->stop, 1);
    thread = t->thread;
    if (t->threadId == SDL_GetCurrentThreadID()) {
        /* Killed from its own callback: the thread ends when the callback returns and frees the slot. */
        t->detached = 1;
        SDL_DetachThread(thread);
        SDL_UnlockMutex(gLock);
        return 0;
    }
    SDL_UnlockMutex(gLock);
    SDL_WaitThread(thread, NULL);
    SDL_LockMutex(gLock);
    logStatistics(t);
    t->used = 0;
    SDL_UnlockMutex(gLock);
    return 0;
}

void bmxTimerShutdown(void)
{
    uint32_t ids[kMaxTimers];
    int n = 0;

    lockInit();
    SDL_LockMutex(gLock);
    for (int i = 0; i < kMaxTimers; i++) {
        if (gTimers[i].used && !gTimers[i].detached) {
            ids[n++] = gTimers[i].id;
        }
    }
    SDL_UnlockMutex(gLock);
    for (int i = 0; i < n; i++) {
        (void)bmxStandTimeKillEvent(ids[i]);
    }
}
