#include "dt.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zmem.h"

/* Observable side effects of callbacks (compared by the harness: it diffs the DLL's data too). */
static uint32_t cb_log[4];

static void *BMX_CDECL null_alloc(size_t size)
{
    return NULL;
}

static void *BMX_CDECL null_realloc(void *block, size_t size)
{
    return NULL;
}

static void BMX_CDECL destroy_cb(void *payload)
{
    cb_log[0]++;
    cb_log[1] += (uint32_t)(uintptr_t)payload;
}

static void BMX_CDECL each_cb(void *payload, void *arg)
{
    cb_log[0]++;
    cb_log[1] += (uint32_t)(uintptr_t)payload;
    cb_log[2] += (uint32_t)(uintptr_t)arg;
}

/* ---- pools ------------------------------------------------------------------------------------ */

typedef struct PoolFixture {
    ZMemPool *pool;
    ZMemBlock *blocks[10];
    int count;
} PoolFixture;

/* Contiguous blocks with random sizes and free flags, closed by a used sentinel header. */
static PoolFixture make_pool(int blocks, int allUsed)
{
    PoolFixture f = {dt_alloc(sizeof(ZMemPool)), {0}, 0};
    blocks = blocks < 1 ? 1 : (blocks > 10 ? 10 : blocks);
    f.count = blocks;
    uint32_t sizes[10];
    uint32_t total = 8;
    for (int i = 0; i < blocks; i++) {
        sizes[i] = 4u * (uint32_t)dt_range(4, 40);
        total += 8 + sizes[i];
    }
    uint8_t *arena = dt_alloc(total);
    uint8_t *at = arena;
    for (int i = 0; i < blocks; i++) {
        f.blocks[i] = (ZMemBlock *)at;
        at += 8 + sizes[i];
    }
    ZMemBlock *sentinel = (ZMemBlock *)at;
    for (int i = 0; i < blocks; i++) {
        const int isFree = !allUsed && (dt_u32() & 1);
        f.blocks[i]->next = i + 1 < blocks ? f.blocks[i + 1] : sentinel;
        f.blocks[i]->prevAndFree = (i ? (uintptr_t)f.blocks[i - 1] : 0) | (uintptr_t)isFree;
    }
    sentinel->next = NULL;
    sentinel->prevAndFree = (uintptr_t)f.blocks[blocks - 1];
    f.pool->first = f.blocks[0];
    f.pool->end = (uintptr_t)arena + total;
    return f;
}

static ZMemBlock *pick_block(const PoolFixture *f, int wantFree)
{
    for (int i = 0; i < f->count; i++) {
        ZMemBlock *b = f->blocks[(i + (int)(dt_u32() % (uint32_t)f->count)) % f->count];
        if (((b->prevAndFree & 1) != 0) == wantFree) {
            return b;
        }
    }
    return f->blocks[0];
}

static void setup_pool_alloc(DtCtx *c)
{
    PoolFixture f;
    dt_seed(c);
    f = make_pool(dt_range(2, 9), c->seed % 9 == 8);
    DT_ARG_PTR(c, 0, f.pool);
    switch (c->seed % 4) {
    case 0:
        DT_ARG_INT(c, 1, dt_range(0, 0x20)); /* below the minimum size */
        break;
    case 1: /* an exact fit for some block's payload minus the 8 byte quirk */
        DT_ARG_INT(c, 1, (uint32_t)((uintptr_t)pick_block(&f, 1)->next - (uintptr_t)pick_block(&f, 1)) - 16);
        break;
    default:
        DT_ARG_INT(c, 1, dt_range(0, 0x120));
    }
}

static void setup_pool_free(DtCtx *c)
{
    PoolFixture f;
    dt_seed(c);
    f = make_pool(dt_range(2, 9), 0);
    DT_ARG_PTR(c, 0, f.pool);
    switch (c->seed % 12) {
    case 0: /* NULL is ignored */
        DT_ARG_PTR(c, 1, NULL);
        break;
    case 1: /* outside the pool */
        DT_ARG_PTR(c, 1, dt_alloc(32));
        break;
    case 2: /* already free */
        DT_ARG_PTR(c, 1, (uint8_t *)pick_block(&f, 1) + 8);
        break;
    default:
        DT_ARG_PTR(c, 1, (uint8_t *)pick_block(&f, 0) + 8);
    }
}

/* ---- heap blocks and the static region -------------------------------------------------------- */

static void setup_init(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    for (uint32_t i = 0; i < 9; i++) {
        ((uint32_t *)&gZmem)[i] = i == 0 ? 0 : dt_u32();
    }
    gZmemHooksInstalled = 0;
    gZmemHooks.alloc = NULL;
    gZmemHooks.free = NULL;
    if (c->seed % 4 == 1) {
        gZmem.initialized = 1; /* assert: already initialized */
    }
    if (c->seed % 4 == 2) {
        gZmemHooksInstalled = 1; /* assert from zmemInstallHooks */
    }
}

static void setup_install_hooks(DtCtx *c)
{
    dt_seed(c);
    gZmemHooksInstalled = (int)(c->seed % 2);
    gZmemHooks.alloc = NULL;
    gZmemHooks.free = NULL;
}

static void setup_alloc(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    switch (c->seed % 6) {
    case 3: /* malloc fails */
        gRwMalloc = null_alloc;
        break;
    case 4:
        gZmem.pool = make_pool(dt_range(2, 8), 0).pool;
        break;
    case 5: /* pool without a free block */
        gZmem.pool = make_pool(dt_range(2, 4), 1).pool;
        break;
    default:
        break;
    }
    DT_ARG_INT(c, 0, dt_range(0, 200));
}

static void setup_free(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    switch (c->seed % 6) {
    case 0:
    case 1:
        DT_ARG_PTR(c, 0, dt_zmemalloc((uint32_t)dt_range(1, 100)));
        break;
    case 2: { /* pool, used block */
        PoolFixture f = make_pool(dt_range(2, 8), 0);
        gZmem.pool = f.pool;
        DT_ARG_PTR(c, 0, (uint8_t *)pick_block(&f, 0) + 12);
        break;
    }
    case 3: { /* pool, already free */
        PoolFixture f = make_pool(dt_range(2, 8), 0);
        gZmem.pool = f.pool;
        DT_ARG_PTR(c, 0, (uint8_t *)pick_block(&f, 1) + 12);
        break;
    }
    case 4: /* pool, foreign block */
        gZmem.pool = make_pool(dt_range(2, 8), 0).pool;
        DT_ARG_PTR(c, 0, (uint8_t *)dt_alloc(32) + 4);
        break;
    default: { /* pool with a free neighbour on both sides likely */
        PoolFixture f = make_pool(9, 0);
        gZmem.pool = f.pool;
        DT_ARG_PTR(c, 0, (uint8_t *)f.blocks[dt_range(1, 7)] + 12);
        break;
    }
    }
}

/* Static region of 0x400 bytes with the next free byte somewhere inside it. */
static uint32_t *make_static_region(void)
{
    uint8_t *base = dt_alloc(0x400);
    gZmem.staticBase = (uint32_t *)base;
    gZmem.staticEnd = (uintptr_t)base + 0x400;
    gZmem.staticTop = (uint32_t *)(base + 4u * (uint32_t)dt_range(0, 0xf0));
    gZmem.lastStaticAlloc = NULL;
    return (uint32_t *)base;
}

static void setup_resize(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    switch (c->seed % 8) {
    case 0:
    case 1: /* heap block */
        DT_ARG_PTR(c, 0, dt_zmemalloc((uint32_t)dt_range(1, 100)));
        DT_ARG_INT(c, 1, dt_range(1, 200));
        break;
    case 2: /* size 0 asserts */
        DT_ARG_PTR(c, 0, dt_zmemalloc(16));
        DT_ARG_INT(c, 1, 0);
        break;
    case 3:
    case 4: { /* static block that is the last allocation (needs payload == lastStaticAlloc by hand) */
        uint32_t *base = make_static_region();
        uint32_t *block = base + 4;
        base[3] = (uint32_t)dt_range(1, 20) * 4;
        gZmem.lastStaticAlloc = block;
        gZmem.staticTop = block + 8;
        DT_ARG_PTR(c, 0, block);
        DT_ARG_INT(c, 1, c->seed % 8 == 3 ? dt_range(1, 0x300) : 0x3f0);
        break;
    }
    case 5: { /* static block that is not the last one */
        uint32_t *base = make_static_region();
        base[3] = 8;
        DT_ARG_PTR(c, 0, base + 4);
        DT_ARG_INT(c, 1, dt_range(1, 64));
        break;
    }
    case 6: { /* heap block while a pool is active */
        void *block = dt_zmemalloc(32);
        gZmem.pool = make_pool(3, 0).pool;
        DT_ARG_PTR(c, 0, block);
        DT_ARG_INT(c, 1, 64);
        break;
    }
    default: { /* unknown origin, or a failing realloc */
        uint32_t *block = dt_zmemalloc(32);
        if (c->seed % 16 == 7) {
            block[-1] |= 0x40000000;
        } else {
            gRwRealloc = null_realloc;
        }
        DT_ARG_PTR(c, 0, block);
        DT_ARG_INT(c, 1, 64);
    }
    }
}

static void setup_static_init(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    if (c->seed % 5 == 4) {
        gRwMalloc = null_alloc; /* the first assert ends the run before "Could not initialize" */
    }
    DT_ARG_INT(c, 0, dt_range(0, 0x800));
}

static void setup_static_alloc(DtCtx *c)
{
    dt_seed(c);
    make_static_region();
    gZmem.staticTop = (uint32_t *)((uint8_t *)gZmem.staticBase + 4u * (uint32_t)dt_range(0, 0x100));
    DT_ARG_INT(c, 0, c->seed % 4 == 3 ? dt_range(0x100, 0x400) : dt_range(0, 0x80));
}

static void setup_static_release(DtCtx *c)
{
    uint32_t *base;
    dt_seed(c);
    base = make_static_region();
    gZmem.lastStaticAlloc = base + 1;
    DT_ARG_PTR(c, 0, (uint8_t *)base + dt_range(0, 0x200) + (c->seed % 3 == 0 ? 0 : 0x80));
}

static void setup_static_mark(DtCtx *c)
{
    dt_seed(c);
    make_static_region();
}

static void setup_nop(DtCtx *c)
{
    dt_seed(c);
}

/* ---- free lists ------------------------------------------------------------------------------- */

static ZMemFreeList *make_flist(uint32_t elem, uint32_t count, ZMemDestroyFn destroy)
{
    dt_zmem_env();
    for (int i = 0; i < 128; i++) {
        gZmemFreeListInUse[i] = 0;
    }
    return DT_ORIG(0x00485e80, ZMemFreeList * (*)(uint32_t, uint32_t, ZMemDestroyFn))(elem, count, destroy);
}

static void *flist_alloc(ZMemFreeList *list)
{
    return DT_ORIG(0x00486050, void *(*)(ZMemFreeList *))(list);
}

/* A list with `used` live objects; returns the live payloads in `payloads` (newest first). */
static ZMemFreeList *populated_flist(int used, void **payloads, ZMemDestroyFn destroy)
{
    ZMemFreeList *list = make_flist((uint32_t)dt_range(1, 40), (uint32_t)dt_range(0, 40), destroy);
    for (int i = 0; i < used; i++) {
        payloads[used - 1 - i] = flist_alloc(list);
    }
    return list;
}

static void setup_flist_create(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    for (int i = 0; i < 128; i++) {
        gZmemFreeListInUse[i] = c->seed % 10 == 9 ? 1 : (uint32_t)(i < dt_range(0, 6));
    }
    DT_ARG_INT(c, 0, dt_range(0, 64));
    DT_ARG_INT(c, 1, c->seed % 3 == 0 ? (uint32_t)dt_range(0, 0x20) : (uint32_t)dt_range(0x20, 100));
    DT_ARG_PTR(c, 2, c->seed % 2 ? (void *)destroy_cb : NULL);
}

static void setup_flist_destroy(DtCtx *c)
{
    void *payloads[40];
    dt_seed(c);
    if (c->seed % 12 == 11) { /* not a table entry */
        ZMemFreeList *fake = dt_alloc(sizeof *fake);
        dt_zmem_env();
        DT_ARG_PTR(c, 0, fake);
        return;
    }
    DT_ARG_PTR(c, 0, populated_flist(dt_range(0, 39), payloads, c->seed % 2 ? destroy_cb : NULL));
}

static void setup_flist_alloc(DtCtx *c)
{
    void *payloads[100];
    ZMemFreeList *list;
    dt_seed(c);
    list = make_flist((uint32_t)dt_range(1, 40), (uint32_t)dt_range(0, 0x30), NULL);
    switch (c->seed % 4) {
    case 0: /* fresh list */
        break;
    case 1: /* exhausted: the next allocation grows the list (twice for the second round) */
    case 2:
        for (uint32_t i = 0; i < list->freeCount + (c->seed % 4 == 2 ? 70u : 0u) && i < 100; i++) {
            payloads[i] = flist_alloc(list);
        }
        break;
    default: /* corrupt: no free node but a non-zero free count */
        list->freeHead = NULL;
        list->freeCount = 3;
    }
    DT_ARG_PTR(c, 0, list);
}

static void setup_flist_free(DtCtx *c)
{
    void *payloads[40];
    ZMemFreeList *list;
    int used;
    dt_seed(c);
    used = dt_range(1, 39);
    list = populated_flist(used, payloads, NULL);
    DT_ARG_PTR(c, 0, list);
    switch (c->seed % 8) {
    case 5: /* not on the used list */
        DT_ARG_PTR(c, 1, (uint8_t *)dt_alloc(64) + 8);
        break;
    case 6: /* head with a predecessor */
        list->usedHead->prev = list->usedHead->next;
        DT_ARG_PTR(c, 1, payloads[0]);
        break;
    case 7: /* head */
        DT_ARG_PTR(c, 1, payloads[0]);
        break;
    default:
        DT_ARG_PTR(c, 1, payloads[dt_range(0, used - 1)]);
    }
}

static void setup_flist_each(DtCtx *c)
{
    void *payloads[40];
    dt_seed(c);
    DT_ARG_PTR(c, 0, populated_flist(dt_range(0, 20), payloads, NULL));
    DT_ARG_PTR(c, 1, c->seed % 4 == 3 ? NULL : (void *)each_cb);
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_flist_query(DtCtx *c)
{
    void *payloads[40];
    dt_seed(c);
    DT_ARG_PTR(c, 0, populated_flist(dt_range(0, 20), payloads, NULL));
}

static void setup_node_query(DtCtx *c)
{
    uint32_t *node;
    dt_seed(c);
    node = dt_alloc(32);
    for (int i = 0; i < 8; i++) {
        node[i] = dt_u32();
    }
    DT_ARG_PTR(c, 0, node + 4);
}

/* ---- deferred release lists ------------------------------------------------------------------- */

static void setup_deferred_init(DtCtx *c)
{
    uint32_t *lists = (uint32_t *)&ORIG_VAR(0x006722b8, ZrwList, gDeferredB);
    dt_seed(c);
    for (int i = 0; i < 6; i++) {
        lists[i] = dt_u32();
    }
}

/* Fills one resource slot: a zmem block (or nothing) plus an optional release callback. */
static void fill_resource(uint32_t *slot)
{
    slot[0] = dt_u32() % 4 == 0 ? 0 : (uint32_t)(uintptr_t)dt_zmemalloc(16);
    slot[1] = dt_u32() % 2 ? (uint32_t)(uintptr_t)destroy_cb : 0;
}

static void build_deferred(ZrwList *list, uint32_t resourceOffset)
{
    uint8_t *prev = NULL;
    const int n = dt_range(0, 6);
    list->head = NULL;
    list->tail = NULL;
    list->count = n;
    for (int i = 0; i < n; i++) {
        uint8_t *node = dt_zmemalloc(0x40);
        fill_resource((uint32_t *)(node + resourceOffset));
        fill_resource((uint32_t *)(node + resourceOffset + 8));
        ((void **)node)[1] = prev;
        if (prev) {
            ((void **)prev)[0] = node;
        } else {
            list->head = node;
        }
        list->tail = node;
        prev = node;
    }
}

static void setup_deferred_flush(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    build_deferred(&ORIG_VAR(0x006722b8, ZrwList, gDeferredB), 0x28);
    build_deferred(&ORIG_VAR(0x006722c4, ZrwList, gDeferredC), 0x24);
}

DT_CASES(zmem_cases) = {
    {"zmemInitialize", 0x004859f0, zmemInitialize, 0, DT_RET_VOID, setup_init, 40, 0},
    {"zmemAlloc", 0x00485aa0, zmemAlloc, 1, DT_RET_INT, setup_alloc, 300, 0},
    {"zmemFree", 0x00485b20, zmemFree, 1, DT_RET_VOID, setup_free, 300, 0},
    {"zmemResize", 0x00485b50, zmemResize, 2, DT_RET_INT, setup_resize, 400, 0},
    {"zmemStaticInit", 0x00485c80, zmemStaticInit, 1, DT_RET_INT, setup_static_init, 50, 0},
    {"zmemStaticAlloc", 0x00485d80, zmemStaticAlloc, 1, DT_RET_INT, setup_static_alloc, 200, 0},
    {"zmemStaticRelease", 0x00485e10, zmemStaticRelease, 1, DT_RET_VOID, setup_static_release, 200, 0},
    {"zmemStaticMark", 0x00485e40, zmemStaticMark, 0, DT_RET_INT, setup_static_mark, 20, 0},
    {"zmemFreeListCreate", 0x00485e80, zmemFreeListCreate, 3, DT_RET_INT, setup_flist_create, 200, 0},
    {"zmemFreeListDestroy", 0x00485fb0, zmemFreeListDestroy, 1, DT_RET_INT, setup_flist_destroy, 120, 0},
    {"zmemFreeListAlloc", 0x00486050, zmemFreeListAlloc, 1, DT_RET_INT, setup_flist_alloc, 200, 0},
    {"zmemFreeListFree", 0x00486170, zmemFreeListFree, 2, DT_RET_VOID, setup_flist_free, 300, 0},
    {"zmemFreeListForEach", 0x00486210, zmemFreeListForEach, 3, DT_RET_INT, setup_flist_each, 100, 0},
    {"zmemFreeListFirst", 0x00486250, zmemFreeListFirst, 1, DT_RET_INT, setup_flist_query, 50, 0},
    {"zmemFreeNodeNext", 0x00486260, zmemFreeNodeNext, 1, DT_RET_INT, setup_node_query, 20, 0},
    {"zmemFreeNodePayload", 0x00486270, zmemFreeNodePayload, 1, DT_RET_INT, setup_node_query, 20, 0},
    {"zmemFreeNodeFromPayload", 0x00486280, zmemFreeNodeFromPayload, 1, DT_RET_INT, setup_node_query, 20, 0},
    {"zmemFreeListUsedCount", 0x00486290, zmemFreeListUsedCount, 1, DT_RET_INT, setup_flist_query, 50, 0},
    {"zmemNop", 0x00486300, zmemNop, 0, DT_RET_VOID, setup_nop, 1, 0},
    {"zmemInstallHooks", 0x00486310, zmemInstallHooks, 0, DT_RET_VOID, setup_install_hooks, 20, 0},
    {"zmemAllocHook", 0x00486360, zmemAllocHook, 1, DT_RET_INT, setup_alloc, 300, 0},
    {"zmemPoolAlloc", 0x004865b0, zmemPoolAlloc, 2, DT_RET_INT, setup_pool_alloc, 400, 0},
    {"zmemPoolFree", 0x00486820, zmemPoolFree, 2, DT_RET_VOID, setup_pool_free, 400, 0},
    {"zmemDeferredInit", 0x00486910, zmemDeferredInit, 0, DT_RET_INT, setup_deferred_init, 20, 0},
    {"zmemDeferredFlush", 0x00486940, zmemDeferredFlush, 0, DT_RET_VOID, setup_deferred_flush, 100, 0},
};
