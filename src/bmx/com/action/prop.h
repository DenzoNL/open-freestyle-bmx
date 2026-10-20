/* Level props (C:\zaxis\Bmx\src\com\action\prop.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_PROP_H
#define BMX_COM_ACTION_PROP_H

#include "compat/orig.h"

/* Notifies a prop that player `player` touched it. */
ORIG_FN(0x0042eec0, void, propTouch, (int player, void *prop));

/* Resets the prop system. */
ORIG_FN(0x0042ecf0, void, propsReset, (void));
/* The prop record with this id on the current level (0x84 bytes; NULL and a debug message if missing). */
ORIG_FN(0x0042ed10, uint8_t *, propFind, (uint32_t id));
/* Marks a prop as collected by a player / puts it back. */
ORIG_FN(0x0042ed80, void, propCollect, (uint32_t player, uint8_t *prop));
ORIG_FN(0x0042ee40, void, propRelease, (uint32_t player, uint8_t *prop));

#endif
