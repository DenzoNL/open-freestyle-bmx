/* Action loop (C:\zaxis\Bmx\src\com\action\action.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_ACTION_H
#define BMX_COM_ACTION_ACTION_H

#include <stdint.h>

#include "compat/orig.h"

/* Changes the action state: 0 resume, 1 pause, 2 restart, -1 leave the screen overlay running. */
ORIG_FN(0x00404f40, uint32_t, actionSetMode, (uint32_t mode));

#endif
