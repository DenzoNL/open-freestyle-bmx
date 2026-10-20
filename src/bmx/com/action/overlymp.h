/* Overlay score screens and the save file (C:\zaxis\Bmx\src\com\action\overlymp.c). Not ported yet; callees
 * only. */
#ifndef BMX_COM_ACTION_OVERLYMP_H
#define BMX_COM_ACTION_OVERLYMP_H

#include <stdint.h>

#include "bmx/com/action/memcard.h"
#include "compat/orig.h"

/* Writes the save file; returns non-zero on success. */
/* Draws the header of a scoreboard page. */
ORIG_FN(0x00412620, void, overlympDrawHeader, (const char *title));
/* Draws the body of a scoreboard page starting at y. */
ORIG_FN(0x004134a0, void, overlympDrawBody, (int y));

#endif
