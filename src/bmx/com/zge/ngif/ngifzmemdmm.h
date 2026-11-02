/*
 * Handle based memory manager (C:\zaxis\Bmx\src\com\zge\NgIF\NgIfzmemDMM.c). One block taken from the zmem
 * static region holds both the data and the handle table: payloads are carved from the bottom, 16 byte
 * handle records grow down from the top, and the gaps between payloads form an address ordered list of
 * free blocks (each starting with a ZmemdmmBlock header). Handles are small integers (0 is invalid).
 * Locking a handle yields its payload and bumps a lock count, so every zmemdmmLock must be paired with a
 * zmemdmmUnlock; optimize only runs with nothing locked and slides payloads down to close the gaps.
 */
#ifndef BMX_COM_ZGE_NGIF_NGIFZMEMDMM_H
#define BMX_COM_ZGE_NGIF_NGIFZMEMDMM_H

#include <stdint.h>

#include "compat/orig.h"

/* Header of a free block. */
typedef struct ZmemdmmBlock {
    struct ZmemdmmBlock *prev;
    struct ZmemdmmBlock *next;
    uint32_t size; /* bytes, including this header */
} ZmemdmmBlock;

/* Handle record, found at `top - handle * sizeof(ZmemdmmHandle)`. */
typedef struct ZmemdmmHandle {
    uint8_t used;
    uint8_t locks;
    uint8_t pad02[2];
    void *data;
    uint32_t size; /* bytes of payload (rounded up to a pointer's size, at least a ZmemdmmBlock) */
    uint8_t pad0c[4];
} ZmemdmmHandle;

BMX_LAYOUT32_ASSERT(sizeof(ZmemdmmHandle) == 0x10, "DMM handle record");
BMX_LAYOUT32_ASSERT(sizeof(ZmemdmmBlock) == 0xc, "DMM free block header");

typedef struct ZmemdmmState {
    uint8_t *tableLow;      /* 0x5f8f30 lowest handle record (the data area ends below it) */
    int tableGrew;          /* 0x5f8f34 the last handle search took 16 bytes from the last free block */
    ZmemdmmBlock *freeHead; /* 0x5f8f38 first free block, -1 when nothing is free */
    uint32_t *staticMark;   /* 0x5f8f3c zmem static region mark taken at init */
    uint32_t size;          /* 0x5f8f40 total bytes */
    uint8_t *highWater;     /* 0x5f8f44 end of the highest allocation, or the last free block */
    int lockCount;          /* 0x5f8f48 locks held over all handles */
    uint8_t *top;           /* 0x5f8f4c end of the block; handle n lives at top - n * sizeof(ZmemdmmHandle) */
    uint8_t *base;          /* 0x5f8f50 */
    int initialized;        /* 0x5f8f54 */
} ZmemdmmState;

#define gZmemdmm ORIG_VAR(0x005f8f30, ZmemdmmState, gZmemdmm)

/* Sets up a manager of `size` bytes (rounded up to 4); returns 1 on failure, 0 on success. */
int zmemdmmInit(int a, uint32_t size, uint32_t b);
void zmemdmmShutdown(void);
/* Allocates a zeroed block of at least `size` bytes (minimum 12); returns its handle, 0 when out of memory.
 */
int zmemdmmAlloc(uint32_t size);
void zmemdmmFree(int handle);
void *zmemdmmLock(int handle);
void zmemdmmUnlock(int handle);
/* Compacts the pool; returns non-zero when blocks are locked (or on corruption). */
int zmemdmmOptimize(void);
/* Handle whose payload is `data`, or 0. */
int zmemdmmFindHandle(const void *data);
/* A handle number not in use (growing the handle table when needed); 0 when out of room. */
int zmemdmmFindFreeHandle(void);
/* Carves `size` bytes out of the free block `blk` and gives them a handle; 0 when no handle is left. */
int zmemdmmCarveBlock(ZmemdmmBlock *blk, uint32_t size);
/* Largest free block and handle count, computed and thrown away (the original keeps neither). */
void zmemdmmComputeStats(void);

#endif
