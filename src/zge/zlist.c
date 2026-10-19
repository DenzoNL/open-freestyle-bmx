/* Pointer lists with an iteration cursor (C:\NgEngine\Zge\zList.c). */
#include "zge/zlist.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char zlistFile[] = "C:\\NgEngine\\Zge\\zList.c";

BMX_ADDR(0x0048c260)
ZList *zlistCreate(int capacity)
{
    ZList *list = zmemAlloc(sizeof *list);
    list->cursor = -1;
    list->items = (void **)zmemAlloc((uint32_t)capacity * 4);
    zmemFill((void *)list->items, 0, (uint32_t)capacity * 4);
    list->capacity = capacity;
    list->count = 0;
    return list;
}

BMX_ADDR(0x0048c2b0)
void zlistDestroy(ZList *list)
{
    zmemFree((void *)list->items);
    zmemFree(list);
}

/* Calls visit on each item while it returns non-zero, then empties the list (items are not freed). */
BMX_ADDR(0x0048c2d0)
void zlistClear(ZList *list, ZListVisitFn visit, void *arg)
{
    if (visit != NULL) {
        for (int i = 0; i < list->count; i++) {
            if (visit(list->items[i], arg) == 0) {
                break;
            }
        }
    }
    zmemFill((void *)list->items, 0, (uint32_t)list->capacity << 2);
    list->cursor = -1;
    list->count = 0;
}

/* Rewinds the cursor to the first item and returns it (NULL when empty). */
BMX_ADDR(0x0048c330)
void *zlistFirst(ZList *list)
{
    list->cursor = -1;
    if (list->count > 0) {
        list->cursor = 0;
        return list->items[0];
    }
    return NULL;
}

/* Advances the cursor; returns NULL and resets it at the end. */
BMX_ADDR(0x0048c360)
void *zlistNext(ZList *list)
{
    if (list->cursor > -1) {
        list->cursor++;
        if (list->cursor < list->count) {
            return list->items[list->cursor];
        }
    }
    list->cursor = -1;
    return NULL;
}

BMX_ADDR(0x0048c390)
void *zlistCurrent(const ZList *list)
{
    if (list->cursor > -1) {
        return list->items[list->cursor];
    }
    return NULL;
}

/* No lower bound check on index. */
BMX_ADDR(0x0048c3b0)
void *zlistGet(const ZList *list, int index)
{
    if (list->count <= index) {
        return NULL;
    }
    return list->items[index];
}

BMX_ADDR(0x0048c400)
int zlistCount(const ZList *list)
{
    return list->count;
}

/*
 * Adds item, doubling the array when full. With no cursor it appends; with a cursor the item takes the
 * current slot and the displaced item moves to the end.
 */
BMX_ADDR(0x0048c460)
void zlistInsert(ZList *list, void *item)
{
    if (item == NULL) {
        zdebugFail("Cannot insert NULL values into a heap list.", zlistFile, 0x1cf, 0);
    }
    if (list->count == list->capacity) {
        void **old = list->items;
        list->items = (void **)zmemAlloc((uint32_t)list->capacity << 3);
        zmemFill((void *)list->items, 0, (uint32_t)list->capacity << 3);
        zmemCopy((void *)list->items, (const void *)old, (uint32_t)list->capacity << 2);
        list->capacity <<= 1;
        zmemFree((void *)old);
    }
    if (list->cursor == -1) {
        list->items[list->count] = item;
        list->count++;
        return;
    }
    list->items[list->count] = list->items[list->cursor];
    list->count++;
    list->items[list->cursor] = item;
}
