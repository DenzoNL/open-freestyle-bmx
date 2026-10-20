/* Rider state (C:\zaxis\Bmx\src\com\action\rider.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_RIDER_H
#define BMX_COM_ACTION_RIDER_H

#include <stdint.h>

#include "compat/orig.h"

/* Contact kind of wheel `wheel` (asserts wheel < 4). */
ORIG_FN(0x00431dd0, int, riderGetWheelContact, (const void *rider, int wheel));
/* Id of the current trick (two lookups). */
ORIG_FN(0x00435ea0, uint32_t, riderGetTrickA, (const void *rider));
ORIG_FN(0x00435ef0, uint32_t, riderGetTrickB, (const void *rider));

#endif
