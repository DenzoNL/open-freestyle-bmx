/*
 * Tests for rw/core.c: free lists, the error slot, object/frame linkage, the plugin registry and the
 * string table. The engine instance is a fixture (dt_rw.h) whose memory interface and free list entry
 * points are logging stubs.
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwcore.h"

/* ---- free lists ------------------------------------------------------------------------------------- */

#define ORIG_FREELIST_CREATE DT_ORIG(0x004dba90, RwFreeList *(*)(int32_t, int32_t, int32_t))

/* The chain of all free lists: the sentinel plus 0..2 other lists (plain link nodes). */
static void freelist_chain(void)
{
    const int others = dt_range(0, 2);
    RwLLLink *nodes[3];
    nodes[0] = &rwFreeListHead;
    for (int i = 1; i <= others; i++) {
        nodes[i] = dt_alloc(sizeof(RwLLLink));
    }
    for (int i = 0; i <= others; i++) {
        nodes[i]->next = nodes[(i + 1) % (others + 1)];
        nodes[i]->prev = nodes[(i + others) % (others + 1)];
    }
}

static int32_t pick_alignment(void)
{
    static const int32_t alignments[] = {0, 1, 2, 4, 8, 16, 32, 3, 64, 12};
    return alignments[dt_u32() % (sizeof alignments / sizeof alignments[0])];
}

static void setup_fl_create(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    freelist_chain();
    gRwtMallocFailAt = c->seed % 7 == 3 ? 1 : (c->seed % 7 == 5 ? 2 : 0);
    DT_ARG_INT(c, 0, dt_range(0, 300));
    DT_ARG_INT(c, 1, dt_range(0, 100));
    DT_ARG_INT(c, 2, pick_alignment());
    rwtEnd();
}

/* Adds `count` blocks with room for `perBlock` entries of the list's size. */
static void add_blocks(RwFreeList *fl, int count)
{
    for (int i = 0; i < count; i++) {
        RwFreeListBlock *block = dt_alloc((uint32_t)fl->blockSize + 16);
        block->next = fl->blocks;
        fl->blocks = block;
        fl->numBlocks++;
    }
}

static void setup_fl_destroy(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    freelist_chain();
    RwFreeList *other = ORIG_FREELIST_CREATE(16, 4, 4);
    RwFreeList *fl = ORIG_FREELIST_CREATE(dt_range(1, 100), dt_range(1, 20), pick_alignment());
    RwFreeList *last = ORIG_FREELIST_CREATE(8, 2, 0);
    (void)other;
    (void)last;
    add_blocks(fl, dt_range(0, 3));
    if (dt_u32() % 5 == 0) {
        fl->freeBegin = NULL;
    }
    DT_ARG_PTR(c, 0, fl);
    rwtEnd();
}

static uint32_t gUsedCalls;

static void BMX_CDECL used_cb(void *entry, void *data)
{
    rwtLogPtr(entry);
    rwtLogPtr(data);
    rwtLog(++gUsedCalls);
}

/* A hand-built free list: blocks of entries and a stack holding a random subset of them as free. */
static RwFreeList *make_used_list(int entrySize, int perBlock, int blocks)
{
    RwFreeList *fl = dt_alloc(sizeof *fl);
    fl->entrySize = entrySize;
    fl->entriesPerBlock = perBlock;
    fl->blockSize = 4 + entrySize * perBlock;
    uint8_t **entries = (uint8_t **)dt_alloc(4 * (uint32_t)(blocks * perBlock + 1));
    int total = 0;
    for (int b = 0; b < blocks; b++) {
        RwFreeListBlock *block = dt_alloc((uint32_t)fl->blockSize + 8);
        block->next = fl->blocks;
        fl->blocks = block;
        for (int i = 0; i < perBlock; i++) {
            entries[total++] = (uint8_t *)(block + 1) + i * entrySize;
        }
    }
    uint8_t **stack = (uint8_t **)dt_alloc(4 * (uint32_t)(total + 2));
    int free = 0;
    for (int i = 0; i < total; i++) {
        if (dt_u32() % 3 == 0) {
            stack[free++] = entries[i];
        }
    }
    for (int i = free - 1; i > 0; i--) { /* shuffle */
        const int j = dt_range(0, i);
        uint8_t *t = stack[i];
        stack[i] = stack[j];
        stack[j] = t;
    }
    fl->freeBegin = (void **)stack;
    fl->freeTop = (void **)stack + free - 1;
    return fl;
}

static void setup_fl_foreach(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    gUsedCalls = 0;
    RwFreeList *fl = make_used_list(4 * dt_range(1, 8), dt_range(0, 6), dt_range(0, 3));
    DT_ARG_PTR(c, 0, fl);
    DT_ARG_PTR(c, 1, used_cb);
    DT_ARG_PTR(c, 2, dt_alloc(8));
    rwtEnd();
}

/* ---- errors ----------------------------------------------------------------------------------------- */

static void setup_rwerror(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
}

static int32_t pick_code(void)
{
    switch (dt_u32() % 4) {
    case 0:
        return (int32_t)(0x80000000u | (dt_u32() & 0xff));
    case 1:
        return rwERRNONE;
    default:
        return (int32_t)(dt_u32() & 0x7fffffffu);
    }
}

static RwError *random_error(void)
{
    RwError *e = dt_alloc(sizeof *e);
    e->pluginID = (int32_t)(dt_u32() % 5);
    e->errorCode = pick_code();
    return e;
}

static void setup_error_set(DtCtx *c)
{
    dt_seed(c);
    RwGlobals *g = rwtEnv();
    if (dt_u32() % 3 != 0) {
        rwtErrorSlot(g)->pluginID = (int32_t)(dt_u32() % 3);
        rwtErrorSlot(g)->errorCode = dt_u32() % 2 ? rwERRNONE : pick_code();
    }
    DT_ARG_PTR(c, 0, random_error());
    rwtEnd();
}

static void setup_error_get(DtCtx *c)
{
    dt_seed(c);
    RwGlobals *g = rwtEnv();
    rwtErrorSlot(g)->pluginID = (int32_t)(dt_u32() % 5);
    rwtErrorSlot(g)->errorCode = pick_code();
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(RwError)));
    rwtEnd();
}

/* ---- object / frame linkage ------------------------------------------------------------------------- */

static void *BMX_CDECL stub_update_objects(void *frame)
{
    rwtLog(0x4d2a80);
    rwtLogPtr(frame);
    return frame;
}

static RwFrame *make_frame(RwObjectHasFrame **members, int n)
{
    RwFrame *f = dt_alloc(0x80);
    f->objectList.next = f->objectList.prev = &f->objectList;
    for (int i = 0; i < n; i++) {
        members[i] = dt_alloc(sizeof(RwObjectHasFrame));
        members[i]->object.parent = f;
        members[i]->lFrame.next = f->objectList.next;
        members[i]->lFrame.prev = &f->objectList;
        f->objectList.next->prev = &members[i]->lFrame;
        f->objectList.next = &members[i]->lFrame;
    }
    return f;
}

/* An object in the middle of an old frame's list (or in none), and a new frame with 0..2 objects. */
static void frame_fixture(DtCtx *c, int setNewFrame)
{
    RwObjectHasFrame *old[3];
    RwObjectHasFrame *fresh[3];
    dt_seed(c);
    rwtEnv();
    dt_stub(0x004d2a80, NULL, (void *)stub_update_objects);
    RwObjectHasFrame *object;
    if (dt_u32() % 3 == 0) {
        object = dt_alloc(sizeof *object);
    } else {
        const int members = dt_range(1, 3);
        make_frame(old, members);
        object = old[dt_range(0, members - 1)];
    }
    DT_ARG_PTR(c, 0, object);
    if (setNewFrame) {
        DT_ARG_PTR(c, 1, dt_u32() % 4 == 0 ? NULL : make_frame(fresh, dt_range(0, 2)));
    }
    rwtEnd();
}

static void setup_set_frame(DtCtx *c)
{
    frame_fixture(c, 1);
}

static void setup_release_frame(DtCtx *c)
{
    frame_fixture(c, 0);
}

/* ---- plugin registry -------------------------------------------------------------------------------- */

static void setup_registry_open(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    freelist_chain();
    gRwtMallocFailAt = c->seed % 5 == 2 ? 1 : (c->seed % 5 == 4 ? 2 : 0);
    rwPluginRegEntryFreeList = NULL;
    rwtEnd();
}

static void setup_registry_close(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    freelist_chain();
    RwFreeList *fl = c->seed % 6 == 0 ? NULL : ORIG_FREELIST_CREATE(sizeof(RwPluginRegEntry), 5, 0);
    rwPluginRegEntryFreeList = fl;
    if (fl != NULL) {
        RwPluginRegistry *regs[2] = {rwtRegistry(dt_range(0, 3), 0x100), rwtRegistry(dt_range(0, 2), 0x200)};
        /* a block of three entries, used ones parented to the registries */
        RwFreeListBlock *block = dt_alloc(4 + 3 * sizeof(RwPluginRegEntry) + 8);
        block->next = fl->blocks;
        fl->blocks = block;
        for (int i = 0; i < 3; i++) {
            RwPluginRegEntry *e = (RwPluginRegEntry *)((uint8_t *)(block + 1) + i * sizeof *e);
            e->parentRegistry = regs[dt_u32() % 2];
        }
        fl->entriesPerBlock = 3;
        fl->entrySize = sizeof(RwPluginRegEntry);
        fl->blockSize = 4 + 3 * (int32_t)sizeof(RwPluginRegEntry);
        void **stack = (void **)dt_alloc(16);
        stack[0] = (uint8_t *)(block + 1) + 2 * sizeof(RwPluginRegEntry);
        fl->freeBegin = stack;
        fl->freeTop = dt_u32() % 2 ? stack : stack - 1;
    }
    rwtEnd();
}

static void setup_reg_release(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 3), 0x100);
    RwPluginRegEntry *e = dt_alloc(sizeof *e);
    e->parentRegistry = r;
    DT_ARG_PTR(c, 0, e);
    DT_ARG_PTR(c, 1, dt_alloc(sizeof(RwFreeList)));
    rwtEnd();
}

/* Dummy non-null free list, since AddPlugin only checks for NULL. */
static void registry_env(DtCtx *c, int instances)
{
    rwtEnv();
    rwPluginRegEntryFreeList = c->seed % 11 == 1 ? NULL : dt_alloc(sizeof(RwFreeList));
    rwEngineInstanceCount = instances;
    gRwtEntryFailAt = c->seed % 13 == 3 ? 1 : 0;
}

static void setup_add_plugin(DtCtx *c)
{
    dt_seed(c);
    registry_env(c, c->seed % 7 == 2);
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 5), 0x100);
    const int n = dt_range(0, 5);
    int32_t id = dt_u32() % 2 ? 0x100 + dt_range(0, 5) : 0x300 + n; /* present, or new */
    DT_ARG_PTR(c, 0, r);
    DT_ARG_INT(c, 1, dt_u32() % 6 == 0 ? -dt_range(0, 9) : dt_range(0, 100));
    DT_ARG_INT(c, 2, id);
    DT_ARG_PTR(c, 3, rwtPickCtor());
    DT_ARG_PTR(c, 4, rwtPickDtor());
    DT_ARG_PTR(c, 5, rwtPickCopy());
    rwtEnd();
}

static void setup_init_object(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 5), 0x100);
    gRwtCtorCalls = 0;
    gRwtCtorFailAt = dt_u32() % 2 ? dt_range(1, 6) : 0;
    DT_ARG_PTR(c, 0, r);
    DT_ARG_PTR(c, 1, dt_alloc(0x40));
    rwtEnd();
}

static void setup_deinit_object(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    DT_ARG_PTR(c, 0, rwtRegistry(dt_range(0, 5), 0x100));
    DT_ARG_PTR(c, 1, dt_alloc(0x40));
    rwtEnd();
}

static void setup_add_stream(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 5), 0x100);
    DT_ARG_PTR(c, 0, r);
    DT_ARG_INT(c, 1, dt_u32() % 3 ? 0x100 + dt_range(0, 5) : 0x500);
    DT_ARG_PTR(c, 2, rwtPickRead());
    DT_ARG_PTR(c, 3, rwtPickWrite());
    DT_ARG_PTR(c, 4, rwtPickSize());
    rwtEnd();
}

/* A script for the extension chunk: a version, a total length and a list of (type, length) sub-chunks. */
static void chunk_script(DtCtx *c, RwPluginRegistry *r)
{
    rwtStreamStubs();
    const int n = dt_range(0, 4);
    uint32_t total = 0;
    for (int i = 0; i < n; i++) {
        gRwtChunks[i].type = dt_u32() % 4 ? (uint32_t)(0x100 + dt_range(0, 6)) : 0x900;
        gRwtChunks[i].length = (uint32_t)dt_range(0, 40);
        total += gRwtChunks[i].length + 12;
    }
    gRwtChunkN = n;
    gRwtFindLength = dt_u32() % 8 == 0 ? (uint32_t)dt_range(0, 30) : total;
    switch (dt_u32() % 8) {
    case 0:
        gRwtFindVersion = 0x300;
        break;
    case 1:
        gRwtFindVersion = 0x304;
        break;
    default:
        gRwtFindVersion = (uint32_t)dt_range(0x301, 0x303);
    }
    gRwtFindOk = dt_u32() % 9 != 0;
    gRwtHeaderFailAt = dt_u32() % 6 == 0 ? dt_range(1, 3) : 0;
    gRwtSkipFailAt = dt_u32() % 6 == 0 ? dt_range(1, 3) : 0;
    gRwtCallbackFails = dt_u32() % 5 == 0 ? (int)(1u << dt_range(0, 3)) : 0;
    (void)r;
    (void)c;
}

static void setup_read_chunks(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 5), 0x100);
    chunk_script(c, r);
    DT_ARG_PTR(c, 0, r);
    DT_ARG_PTR(c, 1, dt_alloc(16));
    DT_ARG_PTR(c, 2, dt_alloc(0x40));
    rwtEnd();
}

static void setup_write_chunks(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 5), 0x100);
    chunk_script(c, r);
    DT_ARG_PTR(c, 0, r);
    DT_ARG_PTR(c, 1, dt_alloc(16));
    DT_ARG_PTR(c, 2, dt_alloc(0x40));
    rwtEnd();
}

static void setup_skip_chunks(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 5), 0x100);
    chunk_script(c, r);
    DT_ARG_PTR(c, 0, r);
    DT_ARG_PTR(c, 1, dt_alloc(16));
    rwtEnd();
}

/* ---- strings ---------------------------------------------------------------------------------------- */

static void setup_string_open(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    dt_fill(&RwEngineInstance->stringFuncs, sizeof(RwStringFunctions));
    rwtEnd();
}

static void setup_default_cb(DtCtx *c)
{
    dt_seed(c);
    for (int i = 0; i < 4; i++) {
        DT_ARG_INT(c, i, dt_u32());
    }
}

static void setup_nothing(DtCtx *c)
{
    dt_seed(c);
}

DT_CASES(rwcore_cases) = {
    {"RwFreeListCreate", 0x004dba90, RwFreeListCreate, 3, DT_RET_INT, setup_fl_create, 300, 0},
    {"RwFreeListDestroy", 0x004dbbc0, RwFreeListDestroy, 1, DT_RET_INT, setup_fl_destroy, 150, 0},
    {"RwFreeListForAllUsed", 0x004dbf70, RwFreeListForAllUsed, 3, DT_RET_INT, setup_fl_foreach, 300, 0},
    {"_rwerror", 0x004ded80, _rwerror, 2, DT_RET_INT, setup_rwerror, 20, 0},
    {"RwErrorGet", 0x004ded90, RwErrorGet, 1, DT_RET_VOID, setup_error_get, 100, 0},
    {"RwErrorSet", 0x004dedd0, RwErrorSet, 1, DT_RET_VOID, setup_error_set, 300, 0},
    {"_rwObjectHasFrameSetFrame", 0x004e8740, _rwObjectHasFrameSetFrame, 2, DT_RET_VOID, setup_set_frame, 200,
     0},
    {"_rwObjectHasFrameReleaseFrame", 0x004e87a0, _rwObjectHasFrameReleaseFrame, 1, DT_RET_VOID,
     setup_release_frame, 100, 0},
    {"_rwPluginRegistryOpen", 0x004e87c0, _rwPluginRegistryOpen, 0, DT_RET_INT, setup_registry_open, 60, 0},
    {"_rwPluginRegistryClose", 0x004e8840, _rwPluginRegistryClose, 0, DT_RET_INT, setup_registry_close, 150,
     0},
    {"rwPluginRegEntryRelease", 0x004e8800, rwPluginRegEntryRelease, 2, DT_RET_VOID, setup_reg_release, 60,
     0},
    {"_rwPluginRegistryAddPlugin", 0x004e88d0, _rwPluginRegistryAddPlugin, 6, DT_RET_INT, setup_add_plugin,
     500, 0},
    {"_rwPluginRegistryDeInitObject", 0x004e8ab0, _rwPluginRegistryDeInitObject, 2, DT_RET_INT,
     setup_deinit_object, 150, 0},
    {"_rwPluginRegistryInitObject", 0x004e8af0, _rwPluginRegistryInitObject, 2, DT_RET_INT, setup_init_object,
     300, 0},
    {"_rwPluginRegistryAddPluginStream", 0x004e8b90, _rwPluginRegistryAddPluginStream, 5, DT_RET_INT,
     setup_add_stream, 300, 0},
    {"rwPluginDefaultCopy", 0x004e8b60, rwPluginDefaultCopy, 4, DT_RET_INT, setup_default_cb, 20, 0},
    {"rwPluginDefaultDestructor", 0x004e8b70, rwPluginDefaultDestructor, 3, DT_RET_INT, setup_default_cb, 20,
     0},
    {"rwPluginDefaultConstructor", 0x004e8b80, rwPluginDefaultConstructor, 3, DT_RET_INT, setup_default_cb,
     20, 0},
    {"_rwPluginRegistryReadDataChunks", 0x004e8bf0, _rwPluginRegistryReadDataChunks, 3, DT_RET_INT,
     setup_read_chunks, 600, 0},
    {"_rwPluginRegistryWriteDataChunks", 0x004e8db0, _rwPluginRegistryWriteDataChunks, 3, DT_RET_INT,
     setup_write_chunks, 400, 0},
    {"_rwPluginRegistrySkipDataChunks", 0x004e8ed0, _rwPluginRegistrySkipDataChunks, 2, DT_RET_INT,
     setup_skip_chunks, 300, 0},
    {"_rwStringOpen", 0x004ee300, _rwStringOpen, 0, DT_RET_INT, setup_string_open, 10, 0},
    {"_rwStringClose", 0x004ee400, _rwStringClose, 0, DT_RET_VOID, setup_nothing, 1, 0},
};
