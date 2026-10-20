/* Menu mode switch (C:\zaxis\Bmx\src\com\zge\NgIF\Missing.c). Not ported yet; callees only. */
#ifndef BMX_COM_ZGE_NGIF_MISSINGZGE_H
#define BMX_COM_ZGE_NGIF_MISSINGZGE_H

#include <stdint.h>

#include "compat/orig.h"

/* Raises the menu-mode flag (zsystemSetMenuMode). */
ORIG_FN(0x0045a290, void, zsystemSetMenuMode, (void));

#endif
