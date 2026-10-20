/* Level props (C:\zaxis\Bmx\src\com\action\prop.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_PROP_H
#define BMX_COM_ACTION_PROP_H

#include "compat/orig.h"

/* Notifies a prop that player `player` touched it. */
ORIG_FN(0x0042eec0, void, propTouch, (int player, void *prop));

#endif
