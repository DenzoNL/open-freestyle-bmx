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
/* Trick class of the current move (table at 0x558fa8); 0x2a/0x2b are lip tricks. */
ORIG_FN(0x00435f40, uint32_t, riderGetTrickKind, (const void *rider));

/* Resets a player's rider and bike to the start state. */
ORIG_FN(0x00431aa0, void, riderReset, (int player, int a));

#endif
