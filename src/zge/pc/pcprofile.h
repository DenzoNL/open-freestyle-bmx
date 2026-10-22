/*
 * PC timers and profile clock (C:\NgEngine\Zge\Pc\pcProfile.c): the high resolution counter the profiler
 * reads, the timer queue (a binary heap of timers ordered by due time, serviced by a per-frame update
 * callback) and the start-up hooks of the C runtime's floating point support that share the file.
 */
#ifndef ZGE_PC_PCPROFILE_H
#define ZGE_PC_PCPROFILE_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/com/zglrw/zrwzgl.h"

/* Heap slot: the key is the due time. Slots start at 1. */
typedef struct ZtimerHeapEntry {
    uint32_t key;
    void *value;
} ZtimerHeapEntry;

/* Priority queue (0x6764a4). descending: a max-heap instead of a min-heap. */
typedef struct ZtimerHeap {
    uint32_t pad00;
    int32_t count;            /* 0x04 */
    int32_t descending;       /* 0x08 */
    ZtimerHeapEntry *entries; /* 0x0c */
} ZtimerHeap;

_Static_assert(sizeof(ZtimerHeap) == 0x10, "timer heap");

enum { kZtimerScript = 3, kZtimerEvent = 2, kZtimerCallback = 1 };

/* A queued timer (the free list's nodes start with the list links). */
typedef struct ZtimerNode {
    void *next;   /* 0x00 */
    void *prev;   /* 0x04 */
    uint32_t due; /* 0x08 */
    uint32_t pad0c;
    uint32_t kind; /* 0x10 kZtimer* */
    uint32_t arg1; /* 0x14 */
    uint32_t arg2; /* 0x18 */
    union {
        void(BMX_CDECL *callback)(uint32_t now, uint32_t arg1, uint32_t arg2); /* kind 1 */
        uint32_t objectId;                                                     /* kind 2 */
    } u;                                                                       /* 0x1c */
} ZtimerNode;

_Static_assert(sizeof(ZtimerNode) == 0x20, "timer node");

/* State of the profile clock (0x6764d0, cleared at initialisation). */
typedef struct PcProfileClock {
    int32_t initialized; /* 0x00 */
    uint32_t pad04;
    int64_t frequency; /* 0x08 counts per second */
} PcProfileClock;

_Static_assert(sizeof(PcProfileClock) == 0x10, "profile clock");

#define gZtimerSlots    ORIG_VAR(0x006764a0, uint16_t, gZtimerSlots)
#define gZtimerHeap     ORIG_VAR(0x006764a4, ZtimerHeap, gZtimerHeap)
#define gZtimerFree     ORIG_VAR(0x006764b4, ZrwList, gZtimerFree)
#define gZtimerUpdate   ORIG_VAR(0x006764c0, ZrwUpdateNode *, gZtimerUpdate)
#define gZtimerReady    ORIG_VAR(0x006764c4, uint32_t, gZtimerReady)
#define gPcProfileClock ORIG_VAR(0x006764d0, PcProfileClock, gPcProfileClock)
/* The entry ztimerHeapPop took off the queue. */
#define gZtimerPopped ORIG_VAR(0x006764e0, ZtimerHeapEntry, gZtimerPopped)
/* Result of the C runtime's floating point probe. */
#define gCrtFpStatus ORIG_VAR(0x006764ec, uint32_t, gCrtFpStatus)

/* Frees an entry of the profiler's free list (the list's destroy callback). */
void pcProfileDestroyEntry(void *entry);
/* Registers the timer update callback; `slots` is stored (16 bits) but never used. Returns 0 on failure. */
int ztimerInitialize(int slots);
/* Update callback: runs every timer that is due. Returns 0 on a timer of an unknown kind. */
int ztimerUpdate(void);
/* Starts the profile clock (asserts when started twice). Returns 0 without a hardware counter. */
int pcProfileInitialize(void);
/* ticks / (frequency / scale) */
double pcProfileTicksToUnits(uint32_t ticks, float scale);
/* Counts elapsed since *start (low 32 bits). */
int32_t pcProfileElapsed(const int32_t *start);
/* Removes the first entry of the heap and returns it (a copy in gZtimerPopped), NULL when empty. */
ZtimerHeapEntry *ztimerHeapPop(ZtimerHeap *heap);
/* The first entry, NULL when empty. */
ZtimerHeapEntry *ztimerHeapPeek(const ZtimerHeap *heap);
/* Removes the entry with the given value; returns 1 when found (never for a heap of one entry). */
int ztimerHeapRemove(void *value, ZtimerHeap *heap);

/* C runtime start-up: installs the float conversion hooks and clears the FPU exceptions. */
void crtFpInit(void);
void crtNop(void);
void crtInstallFloatHooks(void);

#endif
