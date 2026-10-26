/*
 * Gameplay trace for golden-trace recording on real Windows (hook build only). Switched on by a marker
 * file in the game's working directory (hook.h); writes bmxtrace-<tick>.bin next to it.
 *
 * The trace holds everything a later replay needs to drive the game deterministically, plus checkpoints
 * to compare against:
 *   - every timeGetTime result, in call order (the import slot is redirected);
 *   - per controller poll (one per gameplay frame): the raw pad devices the poll reads, the random seed,
 *     and the rider and camera state at the start of the frame.
 * Format: tools/trace_dump.py. All little-endian, no padding.
 */
#include <stddef.h>
#include <stdint.h>

#include "bmx/com/action/camera.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/physics.h"
#include "compat/trace.h"
#include "hook/hook.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zmath.h"

typedef int BOOL;
typedef unsigned long DWORD;
typedef void *HANDLE;

__declspec(dllimport) DWORD __stdcall GetFileAttributesA(const char *name);
__declspec(dllimport) HANDLE __stdcall CreateFileA(const char *name, DWORD access, DWORD share, void *sec,
                                                   DWORD disposition, DWORD flags, HANDLE templ);
__declspec(dllimport) BOOL __stdcall WriteFile(HANDLE file, const void *data, DWORD size, DWORD *written,
                                               void *overlapped);
__declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE handle);
__declspec(dllimport) DWORD __stdcall GetTickCount(void);
__declspec(dllimport) DWORD __stdcall GetCurrentThreadId(void);
__declspec(dllimport) void __stdcall Sleep(DWORD ms);
__declspec(dllimport) BOOL __stdcall VirtualProtect(void *addr, size_t size, DWORD prot, DWORD *old);

#define INVALID_FILE_ATTRIBUTES ((DWORD)0xffffffff)
#define INVALID_HANDLE_VALUE    ((HANDLE)(intptr_t)-1)
#define GENERIC_WRITE           0x40000000
#define FILE_SHARE_READ         1
#define CREATE_ALWAYS           2
#define FILE_ATTRIBUTE_NORMAL   0x80
#define PAGE_READWRITE          0x04

enum {
    kTraceVersion = 2,
    kTagTime = 'T',      /* timeGetTime on the game thread */
    kTagTimeOther = 't', /* timeGetTime on another thread (sound): not replayable, kept for reference */
    kTagFrame = 'F',
    kRiders = 4,
    kFlushEvery = 60, /* frames: at most ~1 s of trace is lost if the game crashes */
};

static HANDLE gFile = INVALID_HANDLE_VALUE;
static uint8_t gBuf[1 << 16];
static uint32_t gUsed;
static uint32_t gFrame;
static ZrwTimeFn gRealTime;
static DWORD gGameThread; /* the thread that polls the controllers */
static volatile long gLock;

static void lock(void)
{
    while (__atomic_exchange_n(&gLock, 1, __ATOMIC_ACQUIRE) != 0) {
        Sleep(0);
    }
}

static void unlock(void)
{
    __atomic_store_n(&gLock, 0, __ATOMIC_RELEASE);
}

static void flush(void)
{
    DWORD written;

    if (gUsed != 0) {
        WriteFile(gFile, gBuf, gUsed, &written, NULL);
        gUsed = 0;
    }
}

static void put(const void *data, uint32_t size)
{
    if (gUsed + size > sizeof gBuf) {
        flush();
    }
    __builtin_memcpy(gBuf + gUsed, data, size);
    gUsed += size;
}

static void put8(uint32_t v)
{
    const uint8_t b = (uint8_t)v;
    put(&b, 1);
}

static void put16(uint32_t v)
{
    const uint16_t h = (uint16_t)v;
    put(&h, 2);
}

static void put32(uint32_t v)
{
    put(&v, 4);
}

static uint32_t __stdcall traceTime(void)
{
    const uint32_t t = gRealTime();
    const DWORD thread = GetCurrentThreadId();

    /* Once gameplay polls start, other threads' reads are dropped: the sound thread spins on
     * timeGetTime (~280M reads, 1.4 GB per minute on Windows 11) and they can't be replayed anyway. */
    if (gGameThread != 0 && thread != gGameThread) {
        return t;
    }
    lock();
    put8(thread == gGameThread ? kTagTime : kTagTimeOther);
    put32(t);
    unlock();
    return t;
}

static void putRider(const PhysRider *r)
{
    put8(r->state);
    put8(r->surface);
    put(r->stateLevel, sizeof r->stateLevel);
    put32(r->flags);
    put16((uint16_t)r->timer);
    put(&r->pos, sizeof r->pos);
    put(&r->vel, sizeof r->vel);
    put(&r->fwd, sizeof r->fwd);
    put(&r->up, sizeof r->up);
    put(&r->rot, sizeof r->rot);
    put32((uint32_t)r->spin);
}

static void tracePoll(void)
{
    const CameraState *cam = &gCameras[gCameraPlayer < 4 ? gCameraPlayer : 0];

    lock();
    gGameThread = GetCurrentThreadId();
    put8(kTagFrame);
    put32(gFrame);
    put32(gZmathTables.randomSeed);
    for (int i = 0; i < 2; i++) {
        const ContrlerPad *pad = &gContrlerPads[i];
        const ContrlerDevice *dev = pad->device;

        put16(pad->flags);
        put8(dev != NULL);
        put8(dev != NULL ? dev->status : 0);
        put32(dev != NULL ? dev->held : 0);
        for (int k = 0; k < 4; k++) {
            put32(dev != NULL ? dev->axis[k] : 0);
        }
    }
    put8(gCameraPlayer);
    put(&cam->eye, sizeof cam->eye);
    put(&cam->lookAt, sizeof cam->lookAt);
    for (int i = 0; i < kRiders; i++) {
        putRider(&gPhysRiders[i]);
    }
    if (++gFrame % kFlushEvery == 0) {
        flush();
    }
    unlock();
}

static char *putDecimal(char *p, uint32_t v)
{
    char digits[10];
    int n = 0;

    do {
        digits[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v != 0);
    while (n > 0) {
        *p++ = digits[--n];
    }
    return p;
}

int bmxTraceMode(void)
{
    if (GetFileAttributesA("bmxtrace.orig") != INVALID_FILE_ATTRIBUTES) {
        return kBmxTraceOriginal;
    }
    if (GetFileAttributesA("bmxtrace.on") != INVALID_FILE_ATTRIBUTES) {
        return kBmxTracePort;
    }
    return kBmxTraceOff;
}

void bmxTraceStart(int mode)
{
    char name[32] = "bmxtrace-";
    char *p;
    DWORD old;
    ZrwTimeFn *slot = &gTimeGetTime;

    if (mode == kBmxTraceOff) {
        return;
    }
    p = putDecimal(name + 9, GetTickCount());
    __builtin_memcpy(p, ".bin", 5);
    gFile =
        CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (gFile == INVALID_HANDLE_VALUE) {
        return;
    }
    put("BMXT", 4);
    put32(kTraceVersion);
    put32((uint32_t)mode); /* 1 original game, 2 our port */
    /* Record sizes, so the dumper can check it matches this build's layout. */
    put32(sizeof(ZVec3));
    put32(sizeof(ZAngle3));
    put32(kRiders);
    flush();

    gRealTime = *slot;
    VirtualProtect((void *)slot, sizeof *slot, PAGE_READWRITE, &old);
    *slot = traceTime;
    VirtualProtect((void *)slot, sizeof *slot, old, &old);
    gBmxTracePoll = tracePoll;
}

void bmxTraceStop(void)
{
    if (gFile != INVALID_HANDLE_VALUE) {
        gBmxTracePoll = NULL;
        lock();
        flush();
        CloseHandle(gFile);
        gFile = INVALID_HANDLE_VALUE;
        unlock();
    }
}
