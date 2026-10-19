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
ORIG_FN(0x0048c1f0, void, zrwListRemove, (ZrwList * list, void *node));

#endif
