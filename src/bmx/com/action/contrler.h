/* Controller handling (C:\zaxis\Bmx\src\com\action\contrler.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_CONTRLER_H
#define BMX_COM_ACTION_CONTRLER_H

#include "compat/orig.h"

/* Force feedback pulse; empty in the PC build. */
ORIG_FN(0x004086d0, void, contrlerRumble, (int player, int a, int b));

/* True when the controller of `player` has one of `mask` newly pressed (mask < 8 are directions); asserts
 * player < 8. */
ORIG_FN(0x00408590, int, contrlerPressed, (int player, uint32_t mask, uint32_t repeat));
/* Controller state record (asserts index < 2); bits 0 and 1 of the byte at +0xc are the analog flags. */
ORIG_FN(0x004086f0, uint8_t *, contrlerGet, (uint32_t index));

/* Whether `action` (0..0x14) is in state `status` (0..2) for the controller state record `state`. */
ORIG_FN(0x004084b0, int, contrlerAction,
        (uint32_t config, const void *state, uint32_t action, uint32_t status));
/* The two stick axes of stick `index` (0 or 1) of a controller state record, in a static buffer. */
ORIG_FN(0x00408610, const float *, contrlerAxes, (const void *state, uint32_t index));

#endif
