/* Menu mode switch (C:\zaxis\Bmx\src\com\zge\NgIF\Missing.c). Not ported yet; callees only. */
#ifndef BMX_COM_ZGE_NGIF_MISSINGZGE_H
#define BMX_COM_ZGE_NGIF_MISSINGZGE_H

#include <stdint.h>

#include "compat/orig.h"

/* Raises the menu-mode flag (zsystemSetMenuMode). */
ORIG_FN(0x0045a290, void, zsystemSetMenuMode, (void));

/* Debug-print stubs of engine calls the PC build lacks (they print "<file> <line> <name>"). */
ORIG_FN(0x0045a1f0, void, zglEnableHorizon, (void));
ORIG_FN(0x0045a220, void, zglDisableHorizon, (void));
ORIG_FN(0x0045a2d0, void, zfxDestroyAll, (int mode));
ORIG_FN(0x0045a300, int, zfxRegisterFuncTable, (const void *table, int count));

#endif
