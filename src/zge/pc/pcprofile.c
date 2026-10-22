/* PC timers and profile clock (see pcprofile.h). */
#include "zge/pc/pcprofile.h"

#include "zge/zdebug.h"
#include "zge/zmem.h"
#include "zge/zprofile.h"
#include "zge/zstring.h"

/* Statically linked C runtime routines of the floating point start-up. */
ORIG_FN(0x004a6803, void, crtCfltcvt, (void));
ORIG_FN(0x004a6498, void, crtFp6498, (void));
ORIG_FN(0x004a64fe, void, crtFp64fe, (void));
ORIG_FN(0x004a643e, void, crtFp643e, (void));
ORIG_FN(0x004a64e6, void, crtFp64e6, (void));
ORIG_FN(0x004a6415, uint32_t, crtFpProbe, (void));
ORIG_FN(0x004a63c5, void, crtFpSetup, (void));

static const char kPcProfileFile[] = "C:\\NgEngine\\Zge\\Pc\\pcProfile.c";

/* The CRT's float conversion hook table (0x563ad0). */
typedef void *CrtFloatHooks[6];
#define gCrtFloatHooks ORIG_VAR(0x00563ad0, CrtFloatHooks, gCrtFloatHooks)

/* The renderer's update function registration (a pointer in the exe's data, zrwRegisterUpdateFunction). */
typedef ZrwUpdateNode *(BMX_CDECL *ZtimerRegisterFn)(int type, ZrwUpdateFn fn, void *arg, int append);
#define gTimerRegister ORIG_VAR(0x00561470, ZtimerRegisterFn, gTimerRegister)

/* kernel32's QueryPerformanceFrequency / Counter imports. */
typedef int(BMX_STDCALL *QueryCounterFn)(int64_t *out);
#define gQueryPerformanceFrequency ORIG_VAR(0x00699600, QueryCounterFn, gQueryPerformanceFrequency)
#define gQueryPerformanceCounter   ORIG_VAR(0x00699604, QueryCounterFn, gQueryPerformanceCounter)

BMX_ADDR(0x004a4080)
void pcProfileDestroyEntry(void *entry)
{
    zmemFreeListFree((ZMemFreeList *)gZProfile.entries, entry);
}

BMX_ADDR(0x004a4230)
int ztimerInitialize(int slots)
{
    gZtimerReady = 1;
    gZtimerSlots = (uint16_t)slots;
    gZtimerUpdate = gTimerRegister(8, BMX_FNPTR(ZrwUpdateFn, 0x004a4280, ztimerUpdate), NULL, 0);
    if (gZtimerUpdate == NULL) {
        gZdebugPrint("ZTIMER assert failed : unable to register update function\n");
        return 0;
    }
    return 1;
}

BMX_ADDR(0x004a4280)
int ztimerUpdate(void)
{
    const uint32_t now = gTimeGetTime();

    for (;;) {
        const ZtimerHeapEntry *top = ztimerHeapPeek(&gZtimerHeap);
        ZtimerNode *timer;

        if (top == NULL) {
            return 1;
        }
        timer = top->value;
        if (timer->due > now) {
            return 1;
        }
        ztimerHeapPop(&gZtimerHeap);
        switch (timer->kind) {
        case kZtimerCallback:
            timer->u.callback(now, timer->arg1, timer->arg2);
            break;
        case kZtimerEvent:
            gZdebugPrint("Event type timer for obj %d\n", timer->u.objectId);
            break;
        case kZtimerScript:
            gZdebugPrint("Script type timer\n");
            break;
        default:
            return 0;
        }
        zrwListAdd(&gZtimerFree, timer);
    }
}

BMX_ADDR(0x004a45e0)
int pcProfileInitialize(void)
{
    if (gPcProfileClock.initialized != 0) {
        zdebugFail("Already initialized", kPcProfileFile, 0x4a, 0);
    }
    zmemFill(&gPcProfileClock, 0, sizeof gPcProfileClock);
    if (gQueryPerformanceFrequency(&gPcProfileClock.frequency) == 0) {
        gZdebugPrint("PROFILE: No hardware counter available\n");
        return 0;
    }
    gPcProfileClock.initialized = 1;
    return 1;
}

BMX_ADDR(0x004a4660)
double pcProfileTicksToUnits(uint32_t ticks, float scale)
{
    return (double)ticks / ((double)gPcProfileClock.frequency / (double)scale);
}

BMX_ADDR(0x004a46a0)
int32_t pcProfileElapsed(const int32_t *start)
{
    int64_t now;

    gQueryPerformanceCounter(&now);
    return (int32_t)now - *start;
}

BMX_ADDR(0x004a47d0)
ZtimerHeapEntry *ztimerHeapPop(ZtimerHeap *heap)
{
    ZtimerHeapEntry *e;
    ZtimerHeapEntry last;
    int i = 1;
    int n;

    if (heap->count == 0) {
        return NULL;
    }
    gZtimerPopped = heap->entries[1];
    last = heap->entries[heap->count];
    n = heap->count - 1;
    heap->count = n;
    if (n >= 2) {
        for (;;) {
            int j = 2 * i;

            e = heap->entries;
            if (heap->descending == 0) {
                if (j != n && e[j + 1].key < e[j].key) {
                    j++;
                }
                if (last.key <= e[j].key) {
                    break;
                }
            } else {
                if (j != n && e[j].key < e[j + 1].key) {
                    j++;
                }
                if (e[j].key <= last.key) {
                    break;
                }
            }
            e[i] = e[j];
            i = j;
            if (2 * i > heap->count) {
                break;
            }
        }
    }
    heap->entries[i] = last;
    return &gZtimerPopped;
}

BMX_ADDR(0x004a48c0)
ZtimerHeapEntry *ztimerHeapPeek(const ZtimerHeap *heap)
{
    if (heap->count == 0) {
        return NULL;
    }
    return &heap->entries[1];
}

BMX_ADDR(0x004a4920)
int ztimerHeapRemove(void *value, ZtimerHeap *heap)
{
    ZtimerHeapEntry *e;
    ZtimerHeapEntry last;
    int i = 1;
    int n = heap->count;

    if (n == 0) {
        return 0;
    }
    if (n < 2) {
        return 0;
    }
    e = heap->entries;
    while (e[i].value != value) {
        i++;
        if (n <= i) {
            return 0;
        }
    }
    last = e[n];
    n--;
    heap->count = n;
    if (2 * i <= n) {
        for (;;) {
            int j = 2 * i;

            e = heap->entries;
            if (heap->descending == 0) {
                if (j != n && e[j + 1].key < e[j].key) {
                    j++;
                }
                if (last.key <= e[j].key) {
                    break;
                }
            } else {
                if (j != n && e[j].key < e[j + 1].key) {
                    j++;
                }
                if (e[j].key <= last.key) {
                    break;
                }
            }
            e[i] = e[j];
            i = j;
            if (2 * i > heap->count) {
                break;
            }
        }
    }
    heap->entries[i] = last;
    return 1;
}

BMX_ADDR(0x004a4a20)
void crtFpInit(void)
{
    crtInstallFloatHooks();
    gCrtFpStatus = crtFpProbe();
    crtFpSetup();
    __asm__ volatile("fnclex");
}

BMX_ADDR(0x004a4a37)
void crtNop(void) {}

BMX_ADDR(0x004a4a38)
void crtInstallFloatHooks(void)
{
    gCrtFloatHooks[1] = (void *)crtFp6498;
    gCrtFloatHooks[0] = (void *)crtCfltcvt;
    gCrtFloatHooks[2] = (void *)crtFp64fe;
    gCrtFloatHooks[3] = (void *)crtFp643e;
    gCrtFloatHooks[4] = (void *)crtFp64e6;
    gCrtFloatHooks[5] = (void *)crtCfltcvt;
}
