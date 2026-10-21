/*
 * Tests for the handle based memory manager (NgIFzmemDMM.c). Every function is run from many pool states
 * that the original code built with random alloc / free / lock / optimize scripts, and whole scripts are
 * replayed through the original and through the port (two wrapper "originals" living in this DLL, so a
 * sequence of calls is compared as one case: its result log and the whole pool must end up identical).
 */
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "dt.h"
#include "dt_ngif.h"
#include "dt_zactor.h"

typedef int(BMX_CDECL *InitFn)(int, uint32_t, uint32_t);
typedef void(BMX_CDECL *VoidFn)(void);
typedef int(BMX_CDECL *AllocFn)(uint32_t);
typedef void(BMX_CDECL *FreeFn)(int);
typedef void *(BMX_CDECL *LockFn)(int);
typedef int(BMX_CDECL *OptimizeFn)(void);
typedef int(BMX_CDECL *FindHandleFn)(const void *);
typedef int(BMX_CDECL *FindFreeFn)(void);

typedef struct DmmApi {
    InitFn init;
    VoidFn shutdown;
    AllocFn alloc;
    FreeFn free;
    LockFn lock;
    FreeFn unlock;
    OptimizeFn optimize;
    FindFreeFn findFree;
} DmmApi;

static const DmmApi kOrig = {
    DT_ORIG(0x0046aca0, InitFn),     DT_ORIG(0x0046ade0, VoidFn),     DT_ORIG(0x0046b030, AllocFn),
    DT_ORIG(0x0046b140, FreeFn),     DT_ORIG(0x0046b470, LockFn),     DT_ORIG(0x0046b4f0, FreeFn),
    DT_ORIG(0x0046b570, OptimizeFn), DT_ORIG(0x0046b3a0, FindFreeFn),
};

static int BMX_CDECL portInit(int a, uint32_t size, uint32_t b)
{
    return zmemdmmInit(a, size, b);
}
static void BMX_CDECL portShutdown(void)
{
    zmemdmmShutdown();
}
static int BMX_CDECL portAlloc(uint32_t size)
{
    return zmemdmmAlloc(size);
}
static void BMX_CDECL portFree(int h)
{
    zmemdmmFree(h);
}
static void *BMX_CDECL portLock(int h)
{
    return zmemdmmLock(h);
}
static void BMX_CDECL portUnlock(int h)
{
    zmemdmmUnlock(h);
}
static int BMX_CDECL portOptimize(void)
{
    return zmemdmmOptimize();
}

static int BMX_CDECL portFindFree(void)
{
    return zmemdmmFindFreeHandle();
}

static const DmmApi kPort = {portInit, portShutdown, portAlloc,    portFree,
                             portLock, portUnlock,   portOptimize, portFindFree};

/* ---- scripts ------------------------------------------------------------------------------------ */

enum { kSlots = 48 };

typedef struct Script {
    int handle[kSlots];
    int locks[kSlots];
    int logging; /* the script is the body of a case: failures are stubbed and logged */
} Script;

static Script gScript;

static int pick_slot(int wantLive, int wantUnlocked)
{
    for (int tries = 0; tries < 8; tries++) {
        const int s = dt_range(0, kSlots - 1);

        if (wantLive && gScript.handle[s] == 0) {
            continue;
        }
        if (wantUnlocked && gScript.locks[s] != 0) {
            continue;
        }
        return s;
    }
    return -1;
}

static uint32_t random_size(void)
{
    const uint32_t k = dt_u32() % 16;

    if (k == 0) {
        return (uint32_t)dt_range(1, 11); /* below the 12 byte minimum */
    }
    if (k == 1) {
        return (uint32_t)dt_range(300, 3000);
    }
    return (uint32_t)dt_range(12, 200);
}

/* Record of a handle. */
static const uint8_t *record_at(int handle)
{
    return gZmemdmm.top - (size_t)handle * 16;
}

/*
 * True when optimize can run without asserting in the original: every free block except the last must
 * end exactly where a live block starts. (Growing the handle table can leave tiny or odd free blocks that
 * break that, and an assert in the setup would cut it short.)
 */
static int optimize_is_safe(void)
{
    if (gZmemdmm.freeHead == (ZmemdmmBlock *)(uintptr_t)-1) {
        return 1;
    }
    for (ZmemdmmBlock *b = gZmemdmm.freeHead; b != NULL && b->next != NULL; b = b->next) {
        const uint8_t *end = (const uint8_t *)b + b->size;
        int found = 0;

        for (int h = 1; record_at(h) >= gZmemdmm.tableLow; h++) {
            const uint8_t *r = record_at(h);

            if (*r != 0 && *(const uint8_t *const *)(r + 4) == end) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return 0;
        }
    }
    return 1;
}

/* A handle record is in use (the original takes any non-zero flag byte for that). */
/*
 * True when an allocation of `size` bytes cannot hit an assert (which would abort the setup): there is a
 * free block big enough and a handle record to be had.
 */
static int can_alloc(const DmmApi *api, uint32_t size)
{
    const uint32_t asz = (((size < 12) ? 12 : size) + 3) & ~3u;

    if (gZmemdmm.initialized == 0 || gZmemdmm.freeHead == (ZmemdmmBlock *)(uintptr_t)-1) {
        return 0;
    }
    /*
     * Growing the handle table takes 16 bytes from the last free block; the original corrupts the pool
     * when that leaves a block smaller than its 12 byte header, so such an allocation is not attempted.
     */
    {
        int free_record = 0;

        for (int h = 1; record_at(h) >= gZmemdmm.tableLow; h++) {
            if (*record_at(h) != 1) {
                free_record = 1;
                break;
            }
        }
        if (!free_record) {
            ZmemdmmBlock *last = gZmemdmm.freeHead;

            while (last != NULL && last->next != NULL) {
                last = last->next;
            }
            if (gZmemdmm.freeHead == (ZmemdmmBlock *)(uintptr_t)-1 || last->size < 0x1c) {
                return 0;
            }
        }
    }
    /* the handle search may take 16 bytes from the last free block, so do it before looking at sizes */
    if (api->findFree() == 0 || gZmemdmm.freeHead == (ZmemdmmBlock *)(uintptr_t)-1) {
        return 0;
    }
    for (ZmemdmmBlock *b = gZmemdmm.freeHead; b != NULL; b = b->next) {
        if (asz <= b->size) {
            return 1;
        }
    }
    return 0;
}

/* One random step; with `logging` set the observable results go to the compared log. */
static void script_step(const DmmApi *api, uint8_t *base)
{
    const uint32_t op = dt_u32() % 14;
    int s;

    if (op <= 4) {
        const uint32_t size = random_size();
        /* inside a case a tenth of the allocations is left unguarded (a failure asserts and ends the run) */
        const int h =
            ((gScript.logging && dt_u32() % 10 == 0) || can_alloc(api, size)) ? api->alloc(size) : 0;

        if (gScript.logging) {
            ngLog(0xa0 + (uint32_t)(h != 0));
        }
        s = pick_slot(0, 0);
        if (h != 0 && s >= 0 && gScript.handle[s] == 0) {
            gScript.handle[s] = h;
            gScript.locks[s] = 0;
            if (gScript.logging) {
                ngLog((uint32_t)h);
            }
        } else if (h != 0) {
            api->free(h); /* no slot to keep it in */
        }
    } else if (op <= 7) {
        s = pick_slot(1, 1);
        if (s >= 0 && gScript.handle[s] != 0) {
            api->free(gScript.handle[s]);
            gScript.handle[s] = 0;
        }
    } else if (op == 8) {
        s = pick_slot(1, 0);
        if (s >= 0 && gScript.handle[s] != 0) {
            uint8_t *p = api->lock(gScript.handle[s]);

            gScript.locks[s]++;
            if (gScript.logging) {
                ngLog((uint32_t)(p - base));
            }
            if (p != NULL) {
                p[0] = (uint8_t)dt_u32();
                p[11] = (uint8_t)dt_u32();
            }
        }
    } else if (op == 9) {
        s = pick_slot(1, 0);
        if (s >= 0 && gScript.locks[s] != 0) {
            api->unlock(gScript.handle[s]);
            gScript.locks[s]--;
        }
    } else if (op <= 12) {
        /* some odd pool layouts make the original's optimize assert */
        int r = 1;

        if (optimize_is_safe()) {
            r = api->optimize();
        }

        if (gScript.logging) {
            ngLog(0xb0 + (uint32_t)r);
        }
    } else {
        /* fill a live block with data so that moves are visible */
        s = pick_slot(1, 0);
        if (s >= 0 && gScript.handle[s] != 0) {
            uint8_t *p = api->lock(gScript.handle[s]);

            if (p != NULL) {
                p[1] = (uint8_t)dt_u32();
                p[2] = (uint8_t)dt_u32();
            }
            api->unlock(gScript.handle[s]);
        }
    }
}

static void begin_pool(DtCtx *c, int stubFail)
{
    ngBegin(c, stubFail);
    dt_static_region(0x20000);
    memset(&gScript, 0, sizeof gScript);
}

/* A pool of 0x600..0x2800 bytes brought to a random state by `steps` original operations. */
static void build_pool(uint32_t steps, int allowLocks)
{
    uint8_t *base;

    kOrig.init(0, (uint32_t)dt_range(0x600, 0x2800), 0);
    base = gZmemdmm.base;
    gScript.logging = 0;
    for (uint32_t i = 0; i < steps; i++) {
        script_step(&kOrig, base);
    }
    if (!allowLocks) {
        for (int s = 0; s < kSlots; s++) {
            while (gScript.locks[s] != 0) {
                kOrig.unlock(gScript.handle[s]);
                gScript.locks[s]--;
            }
        }
    }
}

static int BMX_CDECL scriptOrig(uint32_t unused)
{
    uint8_t *base;
    uint32_t steps = (uint32_t)dt_range(40, 160);

    gNgLogN = 0;
    kOrig.init(0, (uint32_t)dt_range(0x600, 0x3000), 0);
    base = gZmemdmm.base;
    gScript.logging = 1;
    for (uint32_t i = 0; i < steps; i++) {
        script_step(&kOrig, base);
    }
    return (int)gNgLogN;
}

static int BMX_CDECL scriptPort(uint32_t unused)
{
    uint8_t *base;
    uint32_t steps = (uint32_t)dt_range(40, 160);

    gNgLogN = 0;
    kPort.init(0, (uint32_t)dt_range(0x600, 0x3000), 0);
    base = gZmemdmm.base;
    gScript.logging = 1;
    for (uint32_t i = 0; i < steps; i++) {
        script_step(&kPort, base);
    }
    return (int)gNgLogN;
}

static void setup_script(DtCtx *c)
{
    begin_pool(c, 1);
    DT_ARG_INT(c, 0, 0);
}

/* ---- single function cases ------------------------------------------------------------------------ */

static int random_live_handle(void)
{
    const int s = pick_slot(1, 0);

    return s >= 0 ? gScript.handle[s] : 0;
}

/* A handle: live, dead, zero, or garbage. */
static int random_handle(void)
{
    const uint32_t k = dt_u32() % 8;

    if (k < 4) {
        return random_live_handle();
    }
    if (k == 4) {
        return 0;
    }
    if (k == 5) {
        return dt_range(1, 40);
    }
    /* records just outside the table (the pool is at most 0x2800 bytes): never mapped garbage */
    return dt_range(-2, (int)(gZmemdmm.size / 16) + 2);
}

static void setup_pool_args(DtCtx *c, int stubFail, int allowLocks)
{
    begin_pool(c, stubFail);
    build_pool((uint32_t)dt_range(0, 120), allowLocks);
}

static void setup_init(DtCtx *c)
{
    begin_pool(c, 1);
    if (c->seed & 1) {
        build_pool((uint32_t)dt_range(0, 20), 1); /* already initialised: asserts */
    }
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, (c->seed % 7 == 0) ? 0x7ffffff0u : (uint32_t)dt_range(0, 0x4000));
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_shutdown(DtCtx *c)
{
    begin_pool(c, 1);
    if (c->seed & 1) {
        build_pool((uint32_t)dt_range(0, 20), 1);
    }
}

static void setup_alloc(DtCtx *c)
{
    setup_pool_args(c, 1, 1);
    if (c->seed % 11 == 0) {
        gZmemdmm.initialized = 0;
    }
    DT_ARG_INT(c, 0,
               c->seed % 13 == 0 ? 0 : (c->seed % 5 == 0 ? (uint32_t)dt_range(1, 11) : random_size() * 2));
}

static void setup_free(DtCtx *c)
{
    setup_pool_args(c, 1, 1);
    DT_ARG_INT(c, 0, random_handle());
}

/* The shape the GUI leaves: many 12 byte blocks, then a bigger one that is freed next to the free tail. */
static void setup_free_tail(DtCtx *c)
{
    begin_pool(c, 1);
    kOrig.init(0, 0x8000, 0);
    for (int i = 0; i < dt_range(1, 24); i++) {
        kOrig.alloc(12);
    }
    DT_ARG_INT(c, 0, kOrig.alloc((uint32_t)dt_range(100, 6000)));
}

static void setup_lock(DtCtx *c)
{
    setup_pool_args(c, 1, 1);
    DT_ARG_INT(c, 0, random_handle());
}

static void setup_optimize(DtCtx *c)
{
    setup_pool_args(c, 1, c->seed % 5 == 0);
}

static void setup_find_handle(DtCtx *c)
{
    setup_pool_args(c, 1, 1);
    if (dt_u32() & 1) {
        DT_ARG_PTR(c, 0, gZmemdmm.base + 4 * dt_range(0, 200));
    } else if (dt_u32() & 1) {
        const int h = random_live_handle();

        DT_ARG_PTR(c, 0, h ? (void *)kOrig.lock(h) : NULL);
        if (h) {
            kOrig.unlock(h);
        }
    } else {
        DT_ARG_PTR(c, 0, NULL);
    }
}

static void setup_find_free(DtCtx *c)
{
    setup_pool_args(c, 1, 1);
}

/* A free block that can hold the 12 byte header (growing the handle table may leave 4 and 8 byte ones). */
static ZmemdmmBlock *pick_free_block(int skip)
{
    ZmemdmmBlock *pick = NULL;

    if (gZmemdmm.freeHead == (ZmemdmmBlock *)(uintptr_t)-1) {
        return NULL;
    }
    for (ZmemdmmBlock *b = gZmemdmm.freeHead; b != NULL; b = b->next) {
        if (b->size >= 12) {
            pick = b;
            if (skip-- <= 0) {
                break;
            }
        }
    }
    return pick;
}

static void setup_carve(DtCtx *c)
{
    ZmemdmmBlock *b;

    setup_pool_args(c, 1, 1);
    b = pick_free_block(dt_range(0, 3));
    if (b == NULL) {
        const int slot = pick_slot(1, 1); /* (freeing a locked block would assert during setup) */

        if (slot >= 0) {
            kOrig.free(gScript.handle[slot]);
            gScript.handle[slot] = 0;
        }
        b = pick_free_block(dt_range(0, 3));
    }
    if (b == NULL) {
        /* nothing usable: a fresh pool is one big free block */
        memset(&gScript, 0, sizeof gScript);
        gZmem.staticTop = gZmem.staticBase;
        gZmemdmm.initialized = 0;
        kOrig.init(0, (uint32_t)dt_range(0x600, 0x2800), 0);
        b = gZmemdmm.freeHead;
    }
    DT_ARG_PTR(c, 0, b);
    DT_ARG_INT(c, 1, (uint32_t)(dt_range(3, (int)b->size / 4) * 4));
}

DT_CASES(zmemdmm_cases) = {
    {"zmemdmmInit", 0x0046aca0, zmemdmmInit, 3, DT_RET_INT, setup_init, 300, 0},
    {"zmemdmmShutdown", 0x0046ade0, zmemdmmShutdown, 0, DT_RET_VOID, setup_shutdown, 100, 0},
    {"zmemdmmAlloc", 0x0046b030, zmemdmmAlloc, 1, DT_RET_INT, setup_alloc, 2000, 0},
    {"zmemdmmFree", 0x0046b140, zmemdmmFree, 1, DT_RET_VOID, setup_free, 2000, 0},
    {"zmemdmmFree_tail", 0x0046b140, zmemdmmFree, 1, DT_RET_VOID, setup_free_tail, 50, 0},
    {"zmemdmmLock", 0x0046b470, zmemdmmLock, 1, DT_RET_INT, setup_lock, 1000, 0},
    {"zmemdmmUnlock", 0x0046b4f0, zmemdmmUnlock, 1, DT_RET_VOID, setup_lock, 1000, 0},
    {"zmemdmmOptimize", 0x0046b570, zmemdmmOptimize, 0, DT_RET_INT, setup_optimize, 2000, 0},
    {"zmemdmmFindHandle", 0x0046b350, zmemdmmFindHandle, 1, DT_RET_INT, setup_find_handle, 500, 0},
    {"zmemdmmFindFreeHandle", 0x0046b3a0, zmemdmmFindFreeHandle, 0, DT_RET_INT, setup_find_free, 1000, 0},
    {"zmemdmmCarveBlock", 0x0046ae50, zmemdmmCarveBlock, 2, DT_RET_INT, setup_carve, 2000, 0},
    {"zmemdmmComputeStats", 0x0046b730, zmemdmmComputeStats, 0, DT_RET_VOID, setup_find_free, 500, 0},
    {"zmemdmmScript", (uint32_t)(uintptr_t)scriptOrig, scriptPort, 1, DT_RET_INT, setup_script, 2000, 0},
};
