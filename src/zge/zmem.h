/* Memory manager (C:\NgEngine\Zge\zmem.c): heap blocks, static region, pools and free lists. */
#ifndef ZGE_ZMEM_H
#define ZGE_ZMEM_H

#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"

/*
 * Every zmem allocation is preceded by a 32 bit header word: the low 30 bits hold the (4 byte aligned)
 * size, the top two bits say where the block came from.
 */
enum {
    kZmemSizeMask = 0x3fffffff,
    kZmemOriginShift = 30,
    kZmemOriginStatic = 0, /* bump allocated from the static region */
    kZmemOriginHeap = 2,   /* malloc, or a pool when redirected */
};

/* Block in a pool: payload (and the next block header) follow. Blocks tile the pool contiguously. */
typedef struct ZMemBlock {
    struct ZMemBlock *next;
    uintptr_t prevAndFree; /* previous block address; bit 0 set when this block is free */
} ZMemBlock;

typedef struct ZMemPool {
    ZMemBlock *first;
    uintptr_t end;
} ZMemPool;

/* Node of a free list; the payload follows the header. Free nodes only use `next`. */
typedef struct ZMemFreeNode {
    struct ZMemFreeNode *next;
    struct ZMemFreeNode *prev;
} ZMemFreeNode;

typedef void(BMX_CDECL *ZMemDestroyFn)(void *payload);

/* Chunk of a free list: link to the previous chunk, then kZmemChunkHeader bytes in the first node. */
typedef struct ZMemChunk {
    struct ZMemChunk *next;
} ZMemChunk;
enum { kZmemChunkHeader = 4 };

/*
 * Fixed size object allocator. Memory comes in chunks ({next chunk, nodes...}); the chunk size doubles
 * whenever the free nodes run out. Live objects sit on the doubly linked `usedHead` list.
 */
typedef struct ZMemFreeList {
    ZMemFreeNode *freeHead;
    ZMemFreeNode *usedHead;
    ZMemChunk *chunks;
    uint32_t elemSize;   /* payload size, 4 byte aligned */
    uint32_t chunkCount; /* nodes in the newest chunk */
    uint32_t usedCount;
    uint32_t freeCount;
    ZMemDestroyFn destroy; /* called on every live payload when the list is destroyed */
} ZMemFreeList;

typedef void *(BMX_CDECL *ZMemAllocFn)(size_t size);
typedef void(BMX_CDECL *ZMemFreeFn)(void *block);
typedef void *(BMX_CDECL *ZMemReallocFn)(void *block, size_t size);

/* Allocator entry points published for the renderer (0x6718a8). */
typedef struct ZMemHooks {
    ZMemAllocFn alloc;
    ZMemFreeFn free;
} ZMemHooks;

typedef struct ZMemState {
    int initialized;
    uint32_t *staticBase;
    ZMemPool *pool; /* when set, heap allocations are carved from this pool instead of malloc */
    uint32_t *staticTop;
    uint32_t unknown890;
    uint32_t *lastStaticAlloc; /* header of the newest static block */
    uintptr_t staticEnd;
    uint32_t unknown89c[2];
} ZMemState;

typedef uint32_t ZMemInUseTable[128];
typedef ZMemFreeList ZMemFreeListTable[128];

#define gZmem               ORIG_VAR(0x00670880, ZMemState, gZmem)
#define gZmemFreeListInUse  ORIG_VAR(0x00670680, ZMemInUseTable, gZmemFreeListInUse)
#define gZmemFreeLists      ORIG_VAR(0x006708a8, ZMemFreeListTable, gZmemFreeLists)
#define gZmemHooks          ORIG_VAR(0x006718a8, ZMemHooks, gZmemHooks)
#define gZmemHooksInstalled ORIG_VAR(0x006722b0, int, gZmemHooksInstalled)
/* The renderer's memory interface, copied in by InitApp. */
#define gRwMalloc  ORIG_VAR(0x0056149c, ZMemAllocFn, gRwMalloc)
#define gRwFree    ORIG_VAR(0x005614a0, ZMemFreeFn, gRwFree)
#define gRwRealloc ORIG_VAR(0x005614a4, ZMemReallocFn, gRwRealloc)

int zmemInitialize(void);
void *zmemAlloc(uint32_t size);
void zmemFree(void *block);
void *zmemResize(void *block, uint32_t size);
int zmemStaticInit(uint32_t size);
void *zmemStaticAlloc(uint32_t size);
void zmemStaticRelease(const uint32_t *mark);
uint32_t *zmemStaticMark(void);

ZMemFreeList *zmemFreeListCreate(uint32_t elemSize, uint32_t count, ZMemDestroyFn destroy);
int zmemFreeListDestroy(ZMemFreeList *list);
void *zmemFreeListAlloc(ZMemFreeList *list);
void zmemFreeListFree(ZMemFreeList *list, void *payload);
int zmemFreeListForEach(const ZMemFreeList *list, void(BMX_CDECL *fn)(void *payload, void *arg), void *arg);
ZMemFreeNode *zmemFreeListFirst(const ZMemFreeList *list);
ZMemFreeNode *zmemFreeNodeNext(const ZMemFreeNode *node);
void *zmemFreeNodePayload(ZMemFreeNode *node);
ZMemFreeNode *zmemFreeNodeFromPayload(void *payload);
uint32_t zmemFreeListUsedCount(const ZMemFreeList *list);

void zmemNop(void);
void zmemInstallHooks(void);
void *zmemAllocHook(uint32_t size);
void *zmemPoolAlloc(ZMemPool *pool, uint32_t size);
void zmemPoolFree(ZMemPool *pool, void *payload);

int zmemDeferredInit(void);
void zmemDeferredFlush(void);

#endif
