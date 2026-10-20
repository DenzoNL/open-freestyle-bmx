/* Controller handling (C:\zaxis\Bmx\src\com\action\contrler.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_CONTRLER_H
#define BMX_COM_ACTION_CONTRLER_H

#include "compat/orig.h"

/* Force feedback pulse; empty in the PC build. */
ORIG_FN(0x004086d0, void, contrlerRumble, (int player, int a, int b));

#endif
