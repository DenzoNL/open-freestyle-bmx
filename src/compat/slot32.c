/* Pointers kept in 32-bit slots, 64-bit side: a table of pointers addressed by handle (compat/slot32.h). */
#include "compat/slot32.h"

#if !BMX_LAYOUT32
#include <stdlib.h>

#include "zge/zdebug.h"

/* Entry i holds handle i + 1's pointer; free entries are chained through gSlotNext. */
static void **gSlots;
static uint32_t *gSlotNext;
static uint32_t gSlotCount;
static uint32_t gSlotFree; /* first free handle, 0: none */

uint32_t bmxSlot32Put(const void *p)
{
    uint32_t slot;

    if (p == NULL) {
        return 0;
    }
    if (gSlotFree == 0) {
        const uint32_t count = gSlotCount == 0 ? 256 : gSlotCount * 2;
        void **slots = realloc(gSlots, count * sizeof *slots);
        uint32_t *next = realloc(gSlotNext, count * sizeof *next);

        if (slots == NULL || next == NULL) {
            zdebugFail("out of memory for 32-bit pointer slots", __FILE__, __LINE__, 0);
        }
        gSlots = slots;
        gSlotNext = next;
        for (uint32_t i = gSlotCount; i < count; i++) {
            gSlotNext[i] = i + 1 < count ? i + 2 : 0;
        }
        gSlotFree = gSlotCount + 1;
        gSlotCount = count;
    }
    slot = gSlotFree;
    gSlotFree = gSlotNext[slot - 1];
    gSlots[slot - 1] = (void *)p;
    return slot;
}

void *bmxSlot32Get(uint32_t slot)
{
    return slot == 0 ? NULL : gSlots[slot - 1];
}

void bmxSlot32Drop(uint32_t slot)
{
    if (slot != 0) {
        gSlots[slot - 1] = NULL;
        gSlotNext[slot - 1] = gSlotFree;
        gSlotFree = slot;
    }
}
#endif
