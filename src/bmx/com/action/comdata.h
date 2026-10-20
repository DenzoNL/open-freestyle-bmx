/* Per player data (C:\zaxis\Bmx\src\com\action\comdata.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_COMDATA_H
#define BMX_COM_ACTION_COMDATA_H

#include <stdint.h>

#include "compat/orig.h"

/* Sets the view mode of a player's camera state (0x56b448 + 0xb8 * player). */
ORIG_FN(0x00407620, void, comdataSetViewMode, (int player, uint32_t mode));
ORIG_FN(0x00407760, void, comdataResetView, (int player, int mode));
/* The player whose camera is selected (low byte of the camera state). */
ORIG_FN(0x00407600, uint32_t, comdataGetViewPlayer, (void));
/* Level record of key (type << 24 | ...) in the per level table at 0x551fd8 / 0x5520f0. */
ORIG_FN(0x00407d20, uint32_t *, comdataGetLevelRecord, (uint32_t key));
/* Goal info of a level for a player: 0x27 bytes per level, 4 goals of 3 bytes per run set (asserts level <
 * 6). */
ORIG_FN(0x00407fc0, uint8_t *, comdataGetGoalInfo, (int player, uint32_t level));

#endif
