/* Intrusive doubly linked lists used by zrwzgl.c (C:\NgEngine\Zge\Com\ZglRw\zrwzgl.c). Not ported yet. */
#ifndef ZGE_COM_ZGLRW_ZRWZGL_H
#define ZGE_COM_ZGLRW_ZRWZGL_H

#include "compat/orig.h"

/* Nodes start with { next, prev } pointers. */
typedef struct ZrwList {
    void *head;
    void *tail;
    int count;
} ZrwList;

ORIG_FN(0x0048c110, void, zrwListInit, (ZrwList * list));
/* Appends a node. */
ORIG_FN(0x0048c150, void, zrwListAdd, (ZrwList * list, void *node));
ORIG_FN(0x0048c1f0, void, zrwListRemove, (ZrwList * list, void *node));

/* Registers the platform file devices into a ZfileDeviceTable (count, then device pointers); 0 on failure. */
ORIG_FN(0x0048a940, int, zrwFileSystemsInitialize, (void *table));

#endif
