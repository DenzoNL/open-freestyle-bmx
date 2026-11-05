/* Memory manager (C:\NgEngine\Zge\zmem.c): heap blocks, static region, pools and free lists. */
#include "zge/zmem.h"

#include "compat/crt.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zprop.h"
#include "zge/zstring.h"

static const char zmemFile[] = "C:\\NgEngine\\Zge\\zmem.c";

/* Rounds a payload size up (to 4 on 32-bit as the original; 8 on 64-bit). */
static uint32_t zmemAlignSize(uint32_t n)
{
    return (n + (kZmemAlign - 1)) & ~(uint32_t)(kZmemAlign - 1);
}

static uintptr_t zmemAlignAddress(uintptr_t a)
{
    return (a + (kZmemAlign - 1)) & ~(uintptr_t)(kZmemAlign - 1);
}

/* The header word of a heap or static payload. */
static uint32_t *zmemHeaderOf(void *payload)
{
    return (uint32_t *)payload - 1;
}

/* Replaces the size bits of a block header, keeping the origin bits. */
static uint32_t zmemHeaderWithSize(uint32_t header, uint32_t size)
{
    return (header & ~(uint32_t)kZmemSizeMask) | (size & kZmemSizeMask);
}

/* Returns 1: the original ends with MOV EAX,1 (Ghidra shows void) and its caller tests EAX. */
BMX_ADDR(0x004859f0)
int zmemInitialize(void)
{
    if (gZmem.initialized != 0) {
        zdebugFail("Already initialized", zmemFile, 0x202, 0);
    }
    zmemInstallHooks();
    zmemFill(&gZmem, 0, sizeof gZmem);
    gZmem.initialized = 1;
    return 1;
}

/* Allocates size bytes (rounded up to 4) from the redirect pool if set, else the renderer's malloc. */
BMX_ADDR(0x00485aa0)
void *zmemAlloc(uint32_t size)
{
    const uint32_t aligned = zmemAlignSize(size);
    uint8_t *base;
    if (gZmem.pool != NULL) {
        base = zmemPoolAlloc(gZmem.pool, aligned + 8);
    } else {
        base = gRwMalloc(aligned + 8);
    }
    if (base == NULL) {
        zdebugFail("Memory allocation failed", zmemFile, 0x26b, 0);
        return NULL;
    }
    uint8_t *const payload = base + kZmemHead;
    *zmemHeaderOf(payload) = ((uint32_t)kZmemOriginHeap << kZmemOriginShift) | (aligned & kZmemSizeMask);
    return payload;
}

BMX_ADDR(0x00485b20)
void zmemFree(void *block)
{
    void *base = (uint8_t *)block - kZmemHead;
    if (gZmem.pool != NULL) {
        zmemPoolFree(gZmem.pool, base);
        return;
    }
    gRwFree(base);
}

/*
 * Resizes the last static block or a heap block (not while a pool is active). Original quirk: the
 * static path compares lastStaticAlloc (a header address) with the payload address, so it only
 * succeeds if the two were made equal by hand.
 */
BMX_ADDR(0x00485b50)
void *zmemResize(void *block, uint32_t size)
{
    if (size == 0) {
        zdebugFail("Use zmemFree to resize block to 0", zmemFile, 0x2ac, 0);
    }
    const uint32_t aligned = zmemAlignSize(size);
    const uint32_t origin = *zmemHeaderOf(block) >> kZmemOriginShift;
    if (origin == kZmemOriginStatic) {
        if (gZmem.lastStaticAlloc != block) {
            zdebugFail("Can only resize last allocated block", zmemFile, 0x2bd, 0);
            return NULL;
        }
        const uintptr_t newTop = zmemAlignAddress((uintptr_t)block + aligned + 8);
        if (newTop >= gZmem.staticEnd) {
            zdebugFail("Cant resize", zmemFile, 0x2c7, 0);
            return NULL;
        }
        gZmem.staticTop = (uint32_t *)newTop;
    } else if (origin == kZmemOriginHeap) {
        if (gZmem.pool != NULL) {
            zdebugFail("Cannot resize memory when allocations are re-directed to a pool.", zmemFile, 0x2e2,
                       0);
            return NULL;
        }
        uint8_t *const base = gRwRealloc((uint8_t *)block - kZmemHead, aligned + 8);
        if (base == NULL) {
            zdebugFail("Cant resize", zmemFile, 0x2e8, 0);
            return NULL;
        }
        block = base + kZmemHead;
    } else {
        zdebugFail("Unknown pool", zmemFile, 0x2f7, 0);
        return NULL;
    }
    uint32_t *const header = zmemHeaderOf(block);
    *header = zmemHeaderWithSize(*header, aligned);
    return block;
}

/* Reserves size bytes with zmemAlloc to serve as the static region. */
BMX_ADDR(0x00485c80)
int zmemStaticInit(uint32_t size)
{
    gZmem.staticBase = zmemAlloc(size);
    if (gZmem.staticBase != NULL) {
        gZmem.staticTop = gZmem.staticBase;
        gZmem.staticEnd = (uintptr_t)gZmem.staticBase + size;
        return 1;
    }
    zdebugFail("Could not initialize static memory manager", zmemFile, 0x32b, 0);
    return 0;
}

/* Bump allocation from the static region; every block costs 8 bytes of overhead (the header word and 4
 * unused bytes; on 64-bit the head, plus the rounding). */
BMX_ADDR(0x00485d80)
void *zmemStaticAlloc(uint32_t size)
{
    const uint32_t aligned = zmemAlignSize(size);
    const uintptr_t newTop = zmemAlignAddress((uintptr_t)gZmem.staticTop + aligned + 8);
    if (newTop > gZmem.staticEnd) {
        zdebugFail("Memory allocation failed - Out of RAM", zmemFile, 0x360, 0);
        return NULL;
    }
    uint8_t *const payload = (uint8_t *)gZmem.staticTop + kZmemHead;
    uint32_t *const header = zmemHeaderOf(payload);
    gZmem.lastStaticAlloc = header;
    gZmem.staticTop = (uint32_t *)newTop;
    *header = aligned & kZmemSizeMask;
    return payload;
}

/* Rolls the static region back to a mark from zmemStaticMark. */
BMX_ADDR(0x00485e10)
void zmemStaticRelease(const uint32_t *mark)
{
    if (mark < gZmem.staticTop) {
        gZmem.staticTop = (uint32_t *)zmemAlignAddress((uintptr_t)mark);
    }
    gZmem.lastStaticAlloc = NULL;
}

BMX_ADDR(0x00485e40)
uint32_t *zmemStaticMark(void)
{
    return gZmem.staticTop;
}

/*
 * Creates a free list in the first unused slot of the global table. Chunks hold at least 32 nodes.
 * `destroy`, if any, runs on each live payload when the list is destroyed.
 */
BMX_ADDR(0x00485e80)
ZMemFreeList *zmemFreeListCreate(uint32_t elemSize, uint32_t count, ZMemDestroyFn destroy)
{
    int slot = 0;
    while (gZmemFreeListInUse[slot] != 0) {
        slot++;
        if (&gZmemFreeListInUse[slot] >= &gZmemFreeListInUse[128]) {
            zdebugFail("Free list pool has been exhausted.", zmemFile, 0x4dd, 0);
            return NULL;
        }
    }
    gZmemFreeListInUse[slot] = 1;
    ZMemFreeList *list = &gZmemFreeLists[slot];

    const uint32_t payloadSize = zmemAlignSize(elemSize);
    if (count < 0x20) {
        count = 0x20;
    }
    const uint32_t stride = payloadSize + sizeof(ZMemFreeNode);
    list->elemSize = payloadSize;
    list->destroy = destroy;
    list->chunkCount = count;
    ZMemChunk *chunk = zmemAlloc(zmemAlignSize(stride * count + kZmemChunkHeader));
    uint8_t *node = (uint8_t *)chunk + kZmemChunkHeader;
    for (uint32_t i = count - 1; i != 0; i--) {
        ZMemFreeNode *n = (ZMemFreeNode *)node;
        node += stride;
        n->next = (ZMemFreeNode *)node;
    }
    ((ZMemFreeNode *)node)->next = NULL;
    list->chunks = chunk;
    chunk->next = NULL;
    list->freeHead = (ZMemFreeNode *)((uint8_t *)list->chunks + kZmemChunkHeader);
    list->freeCount = count;
    list->usedHead = NULL;
    list->usedCount = 0;
    return list;
}

/* Runs the destroy callback on live payloads, frees every chunk and releases the table slot. */
BMX_ADDR(0x00485fb0)
int zmemFreeListDestroy(ZMemFreeList *list)
{
    ZMemChunk *chunk = list->chunks;
    ZMemFreeNode *node = list->usedHead;
    const ZMemDestroyFn destroy = list->destroy;
    if (destroy != NULL) {
        while (node != NULL) {
            void *payload = node + 1;
            node = node->next;
            destroy(payload);
        }
    }
    while (chunk != NULL) {
        ZMemChunk *next = chunk->next;
        zmemFree(chunk);
        chunk = next;
    }
    if ((uintptr_t)list < (uintptr_t)&gZmemFreeLists[0] ||
        (uintptr_t)list >= (uintptr_t)&gZmemFreeLists[128]) {
        zdebugFail("An invalid free list was destroyed.", zmemFile, 0x4eb, 0);
    }
    gZmemFreeListInUse[list - &gZmemFreeLists[0]] = 0;
    return 1;
}

/* Takes a node from the free list (growing it by a doubled chunk when empty) and returns its payload. */
BMX_ADDR(0x00486050)
void *zmemFreeListAlloc(ZMemFreeList *list)
{
    if (list->freeHead == NULL) {
        if (list->freeCount != 0) {
            zdebugFail("Free list should be empty, but free blocks still exist.", zmemFile, 0x56c, 0);
        }
        const uint32_t stride = list->elemSize + sizeof(ZMemFreeNode);
        uint32_t count = list->chunkCount * 2;
        list->chunkCount = count;
        ZMemChunk *chunk = zmemAlloc(zmemAlignSize(stride * count + kZmemChunkHeader));
        uint8_t *node = (uint8_t *)chunk + kZmemChunkHeader;
        ZMemFreeNode *first = (ZMemFreeNode *)node;
        for (count--; count != 0; count--) {
            ZMemFreeNode *n = (ZMemFreeNode *)node;
            node += stride;
            n->next = (ZMemFreeNode *)node;
        }
        ((ZMemFreeNode *)node)->next = NULL;
        chunk->next = list->chunks;
        list->freeHead = first;
        list->chunks = chunk;
        list->freeCount = list->chunkCount;
    }
    ZMemFreeNode *node = list->freeHead;
    ZMemFreeNode *used = list->usedHead;
    list->freeHead = node->next;
    if (used == NULL) {
        node->next = NULL;
        node->prev = NULL;
    } else {
        node->next = used;
        node->prev = NULL;
        used->prev = node;
    }
    list->usedHead = node;
    list->usedCount++;
    list->freeCount--;
    return node + 1;
}

/* Returns a payload to the free list; asserts it is on the used list. */
BMX_ADDR(0x00486170)
void zmemFreeListFree(ZMemFreeList *list, void *payload)
{
    ZMemFreeNode *node = (ZMemFreeNode *)payload - 1;
    ZMemFreeNode *cur = list->usedHead;
    for (;; cur = cur->next) {
        if (cur == NULL) {
            zdebugFail("Invalid block released from free list.", zmemFile, 0x5b1, 0);
            break;
        }
        if (cur == node) {
            break;
        }
    }
    if (node == list->usedHead) {
        if (node->prev != NULL) { // NOLINT(clang-analyzer-core.NullDereference)
            zdebugFail("Free list is corrupt: Head of used block list is not the sentinel.", zmemFile, 0x5ba,
                       0);
        }
        list->usedHead = node->next;
    }
    if (node->prev != NULL) {
        node->prev->next = node->next;
    }
    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    node->next = list->freeHead;
    list->usedCount--;
    list->freeCount++;
    list->freeHead = node;
}

/* Calls fn(payload, arg) for every live object, newest first. */
BMX_ADDR(0x00486210)
int zmemFreeListForEach(const ZMemFreeList *list, void(BMX_CDECL *fn)(void *payload, void *arg), void *arg)
{
    ZMemFreeNode *node = list->usedHead;
    if (fn != NULL && node != NULL) {
        do {
            void *payload = node + 1;
            node = node->next;
            fn(payload, arg);
        } while (node != NULL);
    }
    return 1;
}

BMX_ADDR(0x00486250)
ZMemFreeNode *zmemFreeListFirst(const ZMemFreeList *list)
{
    return list->usedHead;
}

BMX_ADDR(0x00486260)
ZMemFreeNode *zmemFreeNodeNext(const ZMemFreeNode *node)
{
    return node->next;
}

BMX_ADDR(0x00486270)
void *zmemFreeNodePayload(ZMemFreeNode *node)
{
    return node + 1;
}

BMX_ADDR(0x00486280)
ZMemFreeNode *zmemFreeNodeFromPayload(void *payload)
{
    return (ZMemFreeNode *)payload - 1;
}

BMX_ADDR(0x00486290)
uint32_t zmemFreeListUsedCount(const ZMemFreeList *list)
{
    return list->usedCount;
}

BMX_ADDR(0x00486300)
void zmemNop(void) {}

/* Publishes zmemAllocHook/zmemFree as the allocator entry points at 0x6718a8. */
BMX_ADDR(0x00486310)
void zmemInstallHooks(void)
{
    if (gZmemHooksInstalled != 0) {
        zdebugFail("Memory pool system has been previously initialized.", zmemFile, 0x814, 0);
    }
    gZmemHooks.alloc = BMX_FNPTR(ZMemAllocFn, 0x00486360, zmemAllocHook);
    gZmemHooks.free = BMX_FNPTR(ZMemFreeFn, 0x00485b20, zmemFree);
    gZmemHooksInstalled = 1;
}

/* Same as zmemAlloc. */
BMX_ADDR(0x00486360)
void *zmemAllocHook(uint32_t size)
{
    return zmemAlloc(size);
}

/*
 * Best fit allocation from a pool of contiguous blocks; an exact fit ends the search, a block with 0x28
 * or more spare bytes is split. Quirk: a block fits when its payload is at least size + 8 (the block
 * header's size: 16 on 64-bit).
 * Returns the payload or NULL.
 */
BMX_ADDR(0x004865b0)
void *zmemPoolAlloc(ZMemPool *pool, uint32_t size)
{
    ZMemBlock *best = NULL;
    uint32_t bestSize = 0xffffffffu;
    const uint32_t aligned = size < 0x20 ? 0x20 : zmemAlignSize(size);
    const uint32_t needed = aligned + sizeof(ZMemBlock);
    for (ZMemBlock *block = pool->first; block != NULL; block = block->next) {
        if ((block->prevAndFree & 1) == 0) {
            continue;
        }
        const uint32_t avail = (uint32_t)((uintptr_t)block->next - (uintptr_t)block) - sizeof(ZMemBlock);
        if (avail < bestSize && avail >= needed) {
            best = block;
            bestSize = avail;
            if (avail == needed) {
                break;
            }
        }
    }
    if (best == NULL) {
        return NULL;
    }
    if (bestSize - needed >= 0x28) {
        ZMemBlock *next = best->next;
        ZMemBlock *rest = (ZMemBlock *)((uint8_t *)best + aligned + sizeof(ZMemBlock));
        rest->next = next;
        /* The pool is assumed to end with a sentinel block, so next is never NULL (as in the original). */
        next->prevAndFree =
            (next->prevAndFree & 1) | (uintptr_t)rest; // NOLINT(clang-analyzer-core.NullDereference)
        best->next = rest;
        rest->prevAndFree = (uintptr_t)best | 1;
    }
    best->prevAndFree &= ~(uintptr_t)1;
    return best + 1;
}

/* Frees a pool payload and merges it with free neighbours. NULL is ignored. */
BMX_ADDR(0x00486820)
void zmemPoolFree(ZMemPool *pool, void *payload)
{
    if (payload == NULL) {
        return;
    }
    const uintptr_t addr = (uintptr_t)payload;
    if (addr < (uintptr_t)(pool->first + 1) || addr >= pool->end) {
        zdebugFail("Memory being released is not from this pool.", zmemFile, 0x951, 0);
    }
    ZMemBlock *block = (ZMemBlock *)payload - 1;
    if ((block->prevAndFree & 1) != 0) {
        zdebugFail("zmemPoolFree(): Memory has been previously freed.", zmemFile, 0x955, 0);
    }
    block->prevAndFree |= 1;
    ZMemBlock *prev = (ZMemBlock *)(block->prevAndFree & ~(uintptr_t)1);
    ZMemBlock *next = block->next;
    if ((next->prevAndFree & 1) != 0) {
        next = next->next;
        block->next = next;
        next->prevAndFree = (next->prevAndFree & 1) | (uintptr_t)block;
    }
    if (prev != NULL && (prev->prevAndFree & 1) != 0) {
        prev->next = next;
        next->prevAndFree = (next->prevAndFree & 1) | (uintptr_t)prev;
    }
}

/*
 * TODO(file): the two deferred release lists below probably belong to the resource module around
 * 0x490ff0/0x4913d0 (their only callers), not zmem.c.
 */

/* zprop's lists: the prop definitions (B) and the placed props (C). */
#define gDeferredB ORIG_VAR(0x006722b8, ZrwList, gDeferredB)
#define gDeferredC ORIG_VAR(0x006722c4, ZrwList, gDeferredC)

BMX_ADDR(0x00486910)
int zmemDeferredInit(void)
{
    zrwListInit(&gDeferredB);
    zrwListInit(&gDeferredC);
    return 1;
}

/* A resource released through `release`, or zmemFree when there is no release function. */
static void zmemReleaseResource(void *ptr, ZMemDestroyFn release)
{
    if (release != NULL) {
        release(ptr);
    } else if (ptr != NULL) {
        zmemFree(ptr);
    }
}

/* Unlinks and releases every node (both resources, then the node itself) of lists C and B. */
BMX_ADDR(0x00486940)
void zmemDeferredFlush(void)
{
    ZPropInst *c;
    while ((c = gDeferredC.head) != NULL) {
        zrwListRemove(&gDeferredC, c);
        zmemReleaseResource((void *)c->tag, c->tagRelease);
        zmemReleaseResource(c->object, (ZMemDestroyFn)c->destroy);
        zmemFree(c);
    }
    ZPropDef *b;
    while ((b = gDeferredB.head) != NULL) {
        zrwListRemove(&gDeferredB, b);
        zmemReleaseResource((void *)b->object.def, b->object.defRelease);
        zmemReleaseResource(b->object.shape, b->object.shapeRelease);
        zmemFree(b);
    }
}
