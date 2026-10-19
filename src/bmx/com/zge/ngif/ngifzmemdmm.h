/* Handle based memory manager (C:\zaxis\Bmx\src\com\zge\ngif\ngifzmemdmm.c). Not ported yet. */
#ifndef BMX_COM_ZGE_NGIF_NGIFZMEMDMM_H
#define BMX_COM_ZGE_NGIF_NGIFZMEMDMM_H

#include <stdint.h>

#include "compat/orig.h"

/*
 * Handles are small integers. Locking a handle yields its payload and bumps a lock count, so every
 * zmemdmmLock must be paired with a zmemdmmUnlock.
 */
ORIG_FN(0x0046aca0, int, zmemdmmInit, (int a, uint32_t size, uint32_t b));
ORIG_FN(0x0046ade0, void, zmemdmmShutdown, (void));
ORIG_FN(0x0046b030, int, zmemdmmAlloc, (uint32_t size));
ORIG_FN(0x0046b140, void, zmemdmmFree, (int handle));
ORIG_FN(0x0046b470, void *, zmemdmmLock, (int handle));
ORIG_FN(0x0046b4f0, void, zmemdmmUnlock, (int handle));
ORIG_FN(0x0046b570, void, zmemdmmCompact, (void));

#endif
