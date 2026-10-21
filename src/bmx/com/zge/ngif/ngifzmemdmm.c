/* Handle based memory manager (C:\zaxis\Bmx\src\com\zge\NgIF\NgIfzmemDMM.c). */
#include "bmx/com/zge/ngif/ngifzmemdmm.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kDmmFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIfzmemDMM.c";

/* Value of freeHead when no block is free. */
#define kNoFree ((ZmemdmmBlock *)(uintptr_t)0xffffffffu)

/* Address of the record of a handle (no range check, like the original). */
static ZmemdmmHandle *record(int handle)
{
    return (ZmemdmmHandle *)(uintptr_t)((uint32_t)(uintptr_t)gZmemdmm.top - (uint32_t)handle * 0x10u);
}

/* Last block of the free list. */
static ZmemdmmBlock *lastFree(ZmemdmmBlock *from)
{
    ZmemdmmBlock *last = from;
    for (ZmemdmmBlock *n = from->next; n != NULL; n = n->next) {
        last = n;
    }
    return last;
}

/* highWater becomes the last free block (after the free list changed). */
static void updateHighWater(void)
{
    ZmemdmmBlock *last = lastFree(gZmemdmm.freeHead);
    gZmemdmm.highWater = (uint8_t *)last;
}

/* Fatal: reports through the debug print as well as the assert. */
static void failPrinted(const char *msg, int line)
{
    gZdebugPrint(msg);
    zdebugFail(msg, kDmmFile, line, 0);
}

BMX_ADDR(0x0046aca0)
int zmemdmmInit(int a, uint32_t size, uint32_t b)
{
    if (gZmemdmm.initialized != 0) {
        failPrinted("ZMEMDMM: Error - DMM already initialized\n", 0xc5);
        return 1;
    }
    gZmemdmm.staticMark = zmemStaticMark();
    gZmemdmm.size = (size + 3) & ~3u;
    gZmemdmm.base = zmemStaticAlloc(gZmemdmm.size);
    if (gZmemdmm.base == NULL) {
        failPrinted("ZMEMDMM: Failed to initialize DMM\n", 0xd4);
        return 1;
    }
    gZmemdmm.highWater = gZmemdmm.base;
    gZmemdmm.top = gZmemdmm.base + gZmemdmm.size;
    gZmemdmm.tableLow = gZmemdmm.top;
    gZmemdmm.lockCount = 0;
    gZmemdmm.tableGrew = 0;
    zmemFill(gZmemdmm.base, 0, gZmemdmm.size);
    gZmemdmm.freeHead = (ZmemdmmBlock *)gZmemdmm.base;
    gZmemdmm.freeHead->prev = NULL;
    gZmemdmm.freeHead->next = NULL;
    gZmemdmm.freeHead->size = gZmemdmm.size - 0x10;
    gZmemdmm.initialized = 1;
    return 0;
}

BMX_ADDR(0x0046ade0)
void zmemdmmShutdown(void)
{
    if (gZmemdmm.initialized == 0) {
        failPrinted("ZMEMDMM: DMM Shutdown without being initialized\n", 0x10c);
        return;
    }
    zmemStaticRelease(gZmemdmm.staticMark);
    gZmemdmm.initialized = 0;
}

/* Handle of the record whose payload is `data`, or 0. */
BMX_ADDR(0x0046b350)
int zmemdmmFindHandle(const void *data)
{
    int handle = 0;
    for (;;) {
        handle++;
        const ZmemdmmHandle *rec = record(handle);
        if (rec->used != 0 && rec->data == data) {
            return handle;
        }
        if ((uint8_t *)rec <= gZmemdmm.tableLow) {
            return 0;
        }
    }
}

/*
 * A handle number whose record is not in use. When every record up to the table's end is taken the table
 * grows by one record at the expense of the last free block (tableGrew remembers that, so a failed
 * allocation can give the record back).
 */
BMX_ADDR(0x0046b3a0)
int zmemdmmFindFreeHandle(void)
{
    int handle = 1;
    const ZmemdmmHandle *rec;

    gZmemdmm.tableGrew = 0;
    rec = record(1);
    while ((uint8_t *)rec >= gZmemdmm.tableLow) {
        if (rec->used != 1) {
            gZmemdmm.tableGrew = 0;
            return handle;
        }
        handle++;
        rec = record(handle);
    }
    if (gZmemdmm.tableLow - 0x10 <= gZmemdmm.highWater || gZmemdmm.freeHead == kNoFree) {
        return 0;
    }
    ZmemdmmBlock *last = lastFree(gZmemdmm.freeHead);
    if (last->size > 0x10) {
        last->size -= 0x10;
        gZmemdmm.tableGrew = 1;
        gZmemdmm.tableLow -= 0x10;
        return handle;
    }
    return 0;
}

/* Carves `size` bytes out of the free block `blk` and gives them a handle; 0 when no handle is left. */
BMX_ADDR(0x0046ae50)
int zmemdmmCarveBlock(ZmemdmmBlock *blk, uint32_t size)
{
    const int handle = zmemdmmFindFreeHandle();
    ZmemdmmHandle *rec;
    uint8_t *end;

    if (handle == 0) {
        zdebugFail("ZMEM: Out of DMM handles", kDmmFile, 0x139, 0);
        return 0;
    }
    rec = record(handle);
    end = (uint8_t *)blk + size;
    if (end + 0x10 <= gZmemdmm.tableLow) {
        gZmemdmm.tableGrew = 0;
        rec->used = 1;
        rec->locks = 0;
        rec->pad02[0] = 0;
        rec->data = blk;
        rec->size = size;
        if (gZmemdmm.highWater < end) {
            gZmemdmm.highWater = end;
        }
        ZmemdmmBlock *prev = blk->prev;
        ZmemdmmBlock *next = blk->next;
        const uint32_t blockSize = blk->size;
        if (gZmemdmm.highWater == gZmemdmm.tableLow) {
            gZmemdmm.freeHead = kNoFree;
        } else {
            ZmemdmmBlock *rest = (ZmemdmmBlock *)((uint8_t *)rec->data + size);
            if ((uint8_t *)blk + blockSize < (uint8_t *)rest + sizeof(ZmemdmmBlock)) {
                /* too little left over for a free block: the allocation takes it all */
                rec->size = blockSize;
                if (prev == NULL) {
                    if (next == NULL) {
                        gZmemdmm.freeHead = kNoFree;
                    } else {
                        gZmemdmm.freeHead = next;
                        next->prev = NULL;
                    }
                } else if (next == NULL) {
                    prev->next = NULL;
                } else {
                    prev->next = next;
                    next->prev = prev;
                }
            } else {
                rest->prev = prev;
                rest->next = next;
                rest->size = blockSize - size;
                if (prev != NULL) {
                    prev->next = rest;
                }
                if (rest->next != NULL) {
                    rest->next->prev = rest;
                }
                if (rest->prev == NULL) {
                    gZmemdmm.freeHead = rest;
                }
            }
        }
        zmemFill(rec->data, 0, rec->size);
        return handle;
    }
    if (gZmemdmm.tableGrew == 1) {
        /* give back the record the handle search took */
        if (gZmemdmm.freeHead == kNoFree) {
            return 0;
        }
        lastFree(gZmemdmm.freeHead)->size += 0x10;
        gZmemdmm.tableLow += 0x10;
        gZmemdmm.tableGrew = 0;
    }
    return 0;
}

BMX_ADDR(0x0046b030)
int zmemdmmAlloc(uint32_t size)
{
    if (gZmemdmm.initialized == 0) {
        zdebugFail("ZMEMDMM: DMM used without being initialized\n", kDmmFile, 0x1ea, 0);
        return 0;
    }
    if (size == 0) {
        failPrinted("ZMEMDMM: DMM allocation size = 0\n", 0x1f1);
        return 0;
    }
    if (gZmemdmm.freeHead == kNoFree) {
        zdebugFail("ZMEM: Out of RAM", kDmmFile, 0x204, 0);
        return 0;
    }
    if (size < 0xc) {
        size = 0xc;
    }
    size = (size + 3) & ~3u;
    for (ZmemdmmBlock *blk = gZmemdmm.freeHead; blk != NULL; blk = blk->next) {
        if (size <= blk->size) {
            return zmemdmmCarveBlock(blk, size);
        }
    }
    zdebugFail("ZMEMDMM: DMM failed to allocate", kDmmFile, 0x245, 0);
    return 0;
}

/* Returns the block to the address ordered free list and merges it with free neighbours. */
BMX_ADDR(0x0046b140)
void zmemdmmFree(int handle)
{
    ZmemdmmHandle *rec = record(handle);
    ZmemdmmBlock *blk, *p;

    if (handle == 0 || (uint8_t *)rec < gZmemdmm.tableLow || rec->used == 0) {
        gZdebugPrint("ZMEMDMM: invalid handle passed to zmemDMMDealloc\n");
        return;
    }
    if (rec->locks != 0) {
        zdebugFail("Locked", kDmmFile, 0x277, 0);
    }
    blk = rec->data;
    rec->used = 0;
    zmemFill(blk, 0, rec->size);
    blk->size = rec->size;
    if (blk < gZmemdmm.freeHead) {
        if (gZmemdmm.freeHead == kNoFree) {
            blk->next = NULL;
            blk->prev = NULL;
        } else {
            gZmemdmm.freeHead->prev = blk;
            blk->next = gZmemdmm.freeHead;
            blk->prev = NULL;
        }
        gZmemdmm.freeHead = blk;
    } else {
        ZmemdmmBlock *scan = gZmemdmm.freeHead;
        for (;;) {
            p = scan;
            if (blk <= p) {
                break;
            }
            scan = p->next;
            if (p->next == NULL) {
                break;
            }
        }
        if (p->next == NULL && p < blk) {
            p->next = blk;
            blk->prev = p;
            blk->next = NULL;
        } else {
            ZmemdmmBlock *before = p->prev;
            blk->next = p;
            blk->prev = before;
            p->prev->next = blk;
            p->prev = blk;
        }
    }
    ZmemdmmBlock *next = blk->next;
    if (next != NULL && (uint8_t *)blk + blk->size == (uint8_t *)next) {
        blk->size += next->size;
        if (next->next == NULL) {
            zmemFill(next, 0, sizeof(ZmemdmmBlock));
            blk->next = NULL;
        } else {
            next->next->prev = blk;
            blk->next = next->next;
            zmemFill(next, 0, sizeof(ZmemdmmBlock));
        }
    }
    ZmemdmmBlock *prev = blk->prev;
    if (prev != NULL && (uint8_t *)prev + prev->size == (uint8_t *)blk) {
        ZmemdmmBlock *after = blk->next;
        prev->size += blk->size;
        blk->prev->next = blk->next;
        if (after != NULL) {
            blk->next->prev = blk->prev;
        }
        zmemFill(blk, 0, sizeof(ZmemdmmBlock));
    }
    updateHighWater();
}

BMX_ADDR(0x0046b470)
void *zmemdmmLock(int handle)
{
    ZmemdmmHandle *rec = record(handle);

    if (handle == 0) {
        zdebugFail("ZMEMDMM: Trying to lock handle 0\n", kDmmFile, 0x370, 0);
    } else if ((uint8_t *)rec >= gZmemdmm.tableLow && rec->used != 0) {
        gZmemdmm.lockCount++;
        rec->locks++;
        return rec->data;
    }
    return NULL;
}

BMX_ADDR(0x0046b4f0)
void zmemdmmUnlock(int handle)
{
    ZmemdmmHandle *rec = record(handle);

    if (handle == 0) {
        zdebugFail("ZMEMDMM: Trying to unlock handle 0\n", kDmmFile, 0x3a5, 0);
        return;
    }
    if ((uint8_t *)rec >= gZmemdmm.tableLow && rec->used != 0 && rec->locks != 0) {
        gZmemdmm.lockCount--;
        rec->locks--;
    }
}

/*
 * Slides every payload down over the free block in front of it, front to back, so the free space ends up
 * as one block at the top. The free block header is rewritten behind each moved payload.
 */
BMX_ADDR(0x0046b570)
int zmemdmmOptimize(void)
{
    if (gZmemdmm.lockCount > 0) {
        return 1;
    }
    if (gZmemdmm.freeHead == kNoFree ||
        (gZmemdmm.freeHead->prev == NULL && gZmemdmm.freeHead->next == NULL)) {
        return 0;
    }
    while (gZmemdmm.freeHead->next != NULL) {
        ZmemdmmBlock *hole = gZmemdmm.freeHead;
        const uint32_t holeSize = hole->size;
        ZmemdmmBlock *next = hole->next;
        uint32_t *src = (uint32_t *)((uint8_t *)hole + holeSize);
        const int handle = zmemdmmFindHandle(src);
        ZmemdmmHandle *rec;

        if (handle == 0) {
            failPrinted("ZMEMDMM: DMM optimize found unrecognized pointer, memory corruption likely\n",
                        0x404);
            return 1;
        }
        rec = record(handle);
        if ((uint8_t *)src < (uint8_t *)hole + rec->size) {
            /* the move overlaps: word copy forwards */
            uint32_t *dst = (uint32_t *)hole;
            for (uint32_t words = rec->size >> 2; words != 0; words--) {
                *dst++ = *src++;
            }
        } else {
            zmemCopy(hole, src, rec->size);
        }
        rec->data = hole;
        gZmemdmm.freeHead = (ZmemdmmBlock *)((uint8_t *)hole + rec->size);
        gZmemdmm.freeHead->prev = NULL;
        gZmemdmm.freeHead->next = next;
        gZmemdmm.freeHead->size = holeSize;
        if (next != NULL) {
            ZmemdmmBlock *head = gZmemdmm.freeHead;

            next->prev = head;
            if ((uint8_t *)head + head->size == (uint8_t *)head->next) {
                head->size += head->next->size;
                if (head->next->next == NULL) {
                    head->next = NULL;
                } else {
                    head->next->next->prev = head;
                    head->next = head->next->next;
                }
            }
        }
    }
    updateHighWater();
    return 0;
}

BMX_ADDR(0x0046b730)
void zmemdmmComputeStats(void)
{
    uint32_t largest = 0;
    int records = 0;

    if (gZmemdmm.freeHead != kNoFree) {
        for (ZmemdmmBlock *b = gZmemdmm.freeHead; b != NULL; b = b->next) {
            if (largest < b->size) {
                largest = b->size;
            }
        }
    }
    if (gZmemdmm.tableLow != gZmemdmm.top) {
        do {
            records++;
        } while (gZmemdmm.tableLow < gZmemdmm.top - (size_t)records * 0x10);
    }
    (void)largest;
    (void)records;
}
