/* Pointer lists with an iteration cursor (C:\NgEngine\Zge\zList.c). */
#ifndef ZGE_ZLIST_H
#define ZGE_ZLIST_H

#include "compat/orig.h"

/* Unordered list of non-NULL pointers; `cursor` is the position of the current item, -1 when none. */
typedef struct ZList {
    void **items;
    int count;
    int capacity;
    int cursor;
} ZList;

/* Called by zlistClear for each item until it returns 0. */
typedef int(BMX_CDECL *ZListVisitFn)(void *item, void *arg);

ZList *zlistCreate(int capacity);
void zlistDestroy(ZList *list);
void zlistClear(ZList *list, ZListVisitFn visit, void *arg);
void *zlistFirst(ZList *list);
void *zlistNext(ZList *list);
void *zlistCurrent(const ZList *list);
void *zlistGet(const ZList *list, int index);
int zlistCount(const ZList *list);
void zlistInsert(ZList *list, void *item);

/* Belongs to zList.c too (the packet filed it under zrwprim.c); removes and returns the current item. */
ORIG_FN(0x0048c570, void *, zlistRemoveCurrent, (ZList * list));

#endif
