/* Scene objects (C:\NgEngine\Zge\zobj.c). Not ported yet. */
#ifndef ZGE_ZOBJ_H
#define ZGE_ZOBJ_H

#include <stdint.h>

#include "compat/orig.h"

/* Shows or hides an object handle and notifies its owner. */
ORIG_FN(0x00487600, void, zobjSetVisible, (int32_t object, int visible));

#endif
