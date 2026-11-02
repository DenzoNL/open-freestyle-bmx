/*
 * Pointers kept in 32-bit slots (docs/PORTING.md X2c).
 *
 * A few pointers live in 4-byte slots whose layout is fixed: inside file images used in place (the zprim
 * font's glyph page table) and in records the original's code shares. Where pointers are 32 bits wide the
 * slot holds the pointer itself, as in the original (the oracle compares those bytes). With 64-bit pointers
 * it holds a handle: 0 for NULL, otherwise an index (plus one) into a table of pointers (compat/slot32.c).
 * Handles are released with bmxSlot32Drop when the pointer goes away.
 */
#ifndef BMX_COMPAT_SLOT32_H
#define BMX_COMPAT_SLOT32_H

#include <stdint.h>

#include "compat/layout.h"

#if BMX_LAYOUT32
static inline uint32_t bmxSlot32Put(const void *p)
{
    return (uint32_t)(uintptr_t)p;
}

static inline void *bmxSlot32Get(uint32_t slot)
{
    return (void *)(uintptr_t)slot;
}

static inline void bmxSlot32Drop(uint32_t slot)
{
    (void)slot;
}
#else
uint32_t bmxSlot32Put(const void *p);
void *bmxSlot32Get(uint32_t slot);
void bmxSlot32Drop(uint32_t slot);
#endif

#endif
