/*
 * Differential tests of pcprofile.c (profile clock, timer queue, C runtime FP start-up hooks), the sound tick
 * rate / debug start-up of zdebug.c and zprimEndFrame. Registration callbacks, the free list and the clock
 * callers are stubbed and log their arguments; heaps are random sorted arrays (valid heaps).
 */
#include "bmx/com/zge/zprim.h"
#include "dt.h"
#include "dt_gameplay.h"
#include "zge/pc/pcprofile.h"
#include "zge/zdebug.h"
#include "zge/zprofile.h"

typedef ZrwUpdateNode *(BMX_CDECL *RegisterFn)(int type, ZrwUpdateFn fn, void *arg, int append);
typedef int(BMX_CDECL *StartFn)(void);
typedef void(BMX_CDECL *FailFn)(const char *why, const char *file, int line, int flags);

static void BMX_CDECL stubFailHandler(const char *why, const char *file, int line, int flags)
{
    dtgpLog(0xfa110001u);
    dtgpLog(dtgpHash(why));
    dtgpLog(dtgpHash(file));
    dtgpLog((uint32_t)line);
    dtgpLog((uint32_t)flags);
}

/* ---- stubs -------------------------------------------------------------------------------------- */

static uint32_t clockNow;

static uint32_t BMX_STDCALL stubTime(void)
{
    dtgpLog(0x71e00000u);
    return clockNow;
}

static void BMX_CDECL stubTimerCallback(uint32_t now, uint32_t a, uint32_t b)
{
    dtgpLog(0x71c80000u);
    dtgpLog(now);
    dtgpLog(a);
    dtgpLog(b);
}

static void BMX_CDECL stubListAdd(ZrwList *list, void *node)
{
    dtgpLog(0x11570000u);
    dtgpLogP(list);
    dtgpLogP(node);
}

static void BMX_CDECL stubFreeListFree(void *list, void *payload)
{
    dtgpLog(0xf4ee0001u);
    dtgpLogP(list);
    dtgpLogP(payload);
}

static ZrwUpdateNode *BMX_CDECL stubRegister(int type, ZrwUpdateFn fn, void *arg, int append)
{
    dtgpLog(0x4e610000u);
    dtgpLog((uint32_t)type);
    dtgpLogP((const void *)fn);
    dtgpLogP(arg);
    dtgpLog((uint32_t)append);
    return (dt_u32() & 3) == 0 ? NULL : (ZrwUpdateNode *)(uintptr_t)(0x00650000u + dt_range(0, 255) * 0x20u);
}

static uint32_t BMX_CDECL stubFpProbe(void)
{
    dtgpLog(0xf9400000u);
    return dt_u32();
}

static void BMX_CDECL stubFpSetup(void)
{
    dtgpLog(0xf9500000u);
}

static int BMX_CDECL stubDebugStart(void)
{
    dtgpLog(0xdeb00000u);
    return (int)(dt_u32() & 1);
}

/* ---- heaps ---------------------------------------------------------------------------------------- */

static int cmpAsc(const void *a, const void *b)
{
    const uint32_t x = *(const uint32_t *)a;
    const uint32_t y = *(const uint32_t *)b;

    return x < y ? -1 : x > y;
}

/* Fills the heap with n entries (values are 0x640000 + i * 0x20) sorted by key: a valid min or max heap. */
static void fillHeap(ZtimerHeap *h, int n, int descending)
{
    ZtimerHeapEntry *e = dt_alloc((uint32_t)(n + 3) * sizeof *e);
    uint32_t keys[10];
    const uint32_t range = (dt_u32() & 1) != 0 ? 20 : 0;

    for (int i = 0; i < n; i++) {
        keys[i] = range != 0 ? dt_u32() % range : dt_u32();
    }
    for (int i = 1; i < n; i++) {
        for (int j = i; j > 0 && cmpAsc(&keys[j - 1], &keys[j]) > 0; j--) {
            const uint32_t t = keys[j];
            keys[j] = keys[j - 1];
            keys[j - 1] = t;
        }
    }
    for (int i = 0; i < n; i++) {
        e[i + 1].key = descending ? keys[n - 1 - i] : keys[i];
        e[i + 1].value = (void *)(uintptr_t)(0x00640000u + (uint32_t)i * 0x20u);
    }
    e[0].key = dt_u32();
    e[n + 1].key = dt_u32();
    e[n + 1].value = (void *)(uintptr_t)dt_u32();
    h->count = n;
    h->descending = descending;
    h->entries = e;
}

static ZtimerHeap *randomHeap(int maxCount)
{
    ZtimerHeap *h = dt_alloc(sizeof *h);

    fillHeap(h, dt_range(0, maxCount), (dt_u32() & 3) == 0);
    h->pad00 = dt_u32();
    return h;
}

static void setupPop(DtCtx *c)
{
    dtgpBegin(c, 0);
    dt_fill(&gZtimerPopped, sizeof gZtimerPopped);
    DT_ARG_PTR(c, 0, randomHeap(9));
}

static void setupPeek(DtCtx *c)
{
    dtgpBegin(c, 0);
    DT_ARG_PTR(c, 0, randomHeap(5));
}

static void setupRemove(DtCtx *c)
{
    ZtimerHeap *h;

    dtgpBegin(c, 0);
    h = randomHeap(9);
    DT_ARG_PTR(c, 0,
               h->count > 0 && (dt_u32() & 7) != 0 ? h->entries[dt_range(1, h->count)].value
                                                   : (void *)(uintptr_t)dt_u32());
    DT_ARG_PTR(c, 1, h);
}

/* ---- timers --------------------------------------------------------------------------------------- */

static void setupUpdate(DtCtx *c)
{
    static const uint32_t kinds[] = {1, 1, 2, 3, 1, 2, 3, 0, 4};
    const int n = dt_range(0, 6);
    ZtimerNode *nodes = dt_alloc(8 * sizeof *nodes);

    dtgpBegin(c, DTGP_DEBUG);
    clockNow = (dt_u32() & 1) != 0 ? dt_u32() : 1000u + (uint32_t)dt_range(0, 20);
    gTimeGetTime = stubTime;
    dt_stub(0x0048c150, NULL, (void *)stubListAdd);
    fillHeap(&gZtimerHeap, n, 0);
    for (int i = 1; i <= n; i++) {
        ZtimerNode *t = &nodes[i];

        dt_fill(t, sizeof *t);
        t->due = (dt_u32() & 1) != 0 ? gZtimerHeap.entries[i].key : clockNow + (uint32_t)dt_range(-3, 3);
        t->kind = (dt_u32() & 7) == 0 ? kinds[7 + (dt_u32() & 1)] : kinds[dt_u32() % 7];
        if (t->kind == kZtimerCallback) {
            t->u.callback = stubTimerCallback;
        }
        gZtimerHeap.entries[i].value = t;
        gZtimerHeap.entries[i].key = t->due;
    }
    /* keep the array a heap: sort by due time */
    for (int i = 1; i <= n; i++) {
        for (int j = i; j > 1 && gZtimerHeap.entries[j - 1].key > gZtimerHeap.entries[j].key; j--) {
            const ZtimerHeapEntry t = gZtimerHeap.entries[j];
            gZtimerHeap.entries[j] = gZtimerHeap.entries[j - 1];
            gZtimerHeap.entries[j - 1] = t;
        }
    }
    dt_fill(&gZtimerPopped, sizeof gZtimerPopped);
}

static void setupTimerInit(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    ORIG_VAR(0x00561470, RegisterFn, gTimerRegister) = stubRegister;
    dt_fill((void *)&gZtimerUpdate, sizeof gZtimerUpdate);
    DT_ARG_INT(c, 0, dt_u32());
}

/* ---- profile clock -------------------------------------------------------------------------------- */

static void setupProfileInit(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    dt_fill(&gPcProfileClock, sizeof gPcProfileClock);
    gPcProfileClock.initialized = (dt_u32() & 3) == 0;
}

static void setupTicks(DtCtx *c)
{
    dtgpBegin(c, 0);
    /* The frequency stays below 2^53: a double holds it exactly, as the x87 does after FILD. */
    gPcProfileClock.frequency =
        (int64_t)(((uint64_t)dt_u32() << 21 | (dt_u32() & 0x1fffffu)) >> (dt_u32() % 40));
    DT_ARG_INT(c, 0, (dt_u32() & 1) != 0 ? dt_u32() : dt_range(0, 100000));
    DT_ARG_FLOAT(c, 1, dt_float_edgy(-100.0f, 100000.0f));
}

static void setupElapsed(DtCtx *c)
{
    int32_t *start = dt_alloc(4);

    dtgpBegin(c, 0);
    *start = (int32_t)dt_u32();
    DT_ARG_PTR(c, 0, start);
}

static void setupDestroyEntry(DtCtx *c)
{
    dtgpBegin(c, 0);
    gZProfile.entries = (void *)(uintptr_t)(0x00660000u + dt_range(0, 255) * 0x10u);
    dt_stub(0x00486170, NULL, (void *)stubFreeListFree);
    DT_ARG_PTR(c, 0, (void *)(uintptr_t)(0x00670000u + dt_range(0, 255) * 0x10u));
}

/* ---- C runtime hooks, debug, prim ------------------------------------------------------------------- */

static void setupFpInit(DtCtx *c)
{
    dtgpBegin(c, 0);
    dt_fill(&gCrtFpStatus, sizeof gCrtFpStatus);
    dt_stub(0x004a6415, NULL, (void *)stubFpProbe);
    dt_stub(0x004a63c5, NULL, (void *)stubFpSetup);
}

static void setupHooks(DtCtx *c)
{
    dtgpBegin(c, 0);
    dt_fill((void *)(uintptr_t)0x00563ad0, 24);
}

static void setupNone(DtCtx *c)
{
    dtgpBegin(c, 0);
}

static void setupTickRate(DtCtx *c)
{
    dtgpBegin(c, 0);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setupDebugCreate(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    ORIG_VAR(0x00650170, int, gZdebugCreated) = (dt_u32() & 3) == 0;
    ORIG_VAR(0x00560030, StartFn, gZdebugStart) = stubDebugStart;
    ORIG_VAR(0x0056147c, FailFn, gZdebugFailHandler) = stubFailHandler;
}

static void setupEndFrame(DtCtx *c)
{
    dtgpBegin(c, 0);
    ORIG_VAR(0x005d2504, uint32_t, gZprimFrameEnded) = dt_u32();
}

DT_CASES(pcprofile_cases) = {
    {"pcProfileDestroyEntry", 0x004a4080, pcProfileDestroyEntry, 1, DT_RET_VOID, setupDestroyEntry, 20, 0},
    {"ztimerInitialize", 0x004a4230, ztimerInitialize, 1, DT_RET_INT, setupTimerInit, 100, 0},
    {"ztimerUpdate", 0x004a4280, ztimerUpdate, 0, DT_RET_INT, setupUpdate, 1500, 0},
    {"pcProfileInitialize", 0x004a45e0, pcProfileInitialize, 0, DT_RET_INT, setupProfileInit, 50, 0},
    {"pcProfileTicksToUnits", 0x004a4660, pcProfileTicksToUnits, 2, DT_RET_DOUBLE, setupTicks, 1000, 0},
    {"pcProfileElapsed", 0x004a46a0, pcProfileElapsed, 1, DT_RET_INT, setupElapsed, 50, 0},
    {"ztimerHeapPop", 0x004a47d0, ztimerHeapPop, 1, DT_RET_INT, setupPop, 1000, 0},
    {"ztimerHeapPeek", 0x004a48c0, ztimerHeapPeek, 1, DT_RET_INT, setupPeek, 100, 0},
    {"ztimerHeapRemove", 0x004a4920, ztimerHeapRemove, 2, DT_RET_INT, setupRemove, 1500, 0},
    {"crtFpInit", 0x004a4a20, crtFpInit, 0, DT_RET_VOID, setupFpInit, 50, 0},
    {"crtNop", 0x004a4a37, crtNop, 0, DT_RET_VOID, setupNone, 5, 0},
    {"crtInstallFloatHooks", 0x004a4a38, crtInstallFloatHooks, 0, DT_RET_VOID, setupHooks, 20, 0},
    {"zsndSetTickRate", 0x0047eba0, zsndSetTickRate, 1, DT_RET_INT, setupTickRate, 50, 0},
    {"zsndNop", 0x0047ebc0, zsndNop, 0, DT_RET_INT, setupNone, 5, 0},
    {"zdebugCreate", 0x0047ec40, zdebugCreate, 0, DT_RET_INT, setupDebugCreate, 100, 0},
    {"zprimEndFrame", 0x00477d00, zprimEndFrame, 0, DT_RET_VOID, setupEndFrame, 20, 0},
};
