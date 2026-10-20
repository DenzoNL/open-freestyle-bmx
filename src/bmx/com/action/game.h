/* Game state (C:\zaxis\Bmx\src\com\action\game.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_GAME_H
#define BMX_COM_ACTION_GAME_H

#include <stdint.h>

#include "compat/orig.h"

/* (options & mask) != 0, where options is the byte at 0x56b700. */
ORIG_FN(0x0040a890, int, gameTestOption, (uint32_t mask));
/* Starts game mode `mode` (resets scores and records) and returns it. */
ORIG_FN(0x0040a8c0, uint32_t, gameStartMode, (int mode));
/* Current game mode (byte at 0x56b701): 0 free ride, 2 and 3 are the competition modes, 6..10 timed. */
ORIG_FN(0x0040a9f0, uint32_t, gameGetMode, (void));
ORIG_FN(0x0040aa10, uint32_t, gameSetSubMode, (uint32_t subMode));
/* Game sub-mode (byte at 0x56b702). */
ORIG_FN(0x0040aa30, uint32_t, gameGetSubMode, (void));
/* Switches the top-level game state machine (the original calls it game_c_BigGameFunction). */
ORIG_FN(0x0040aa50, uint32_t, gameBigGameFunction, (int state));
/* Run type (byte at 0x56b704). */
ORIG_FN(0x0040bd50, uint32_t, gameGetRunType, (void));
/* Number of runs of the current session (byte at 0x56b703). */
ORIG_FN(0x0040bdc0, uint32_t, gameGetRunCount, (void));
/*
 * Score record by key: (mode << 24 | player << 22 | run << 16 | slot) style; returns a pointer to
 * {u32 value; u8 rank; u8 flags; ...} or NULL when the key is invalid.
 */
ORIG_FN(0x0040be50, uint8_t *, gameGetRecord, (uint32_t key));
/* Pointer to the player name table at 0x56ba1c. */
ORIG_FN(0x0040c050, const char *, gameGetNameTable, (void));
/* Mode dependent byte (0x56b6fc, 0x56b6f8 or 0x56b6f9). */
ORIG_FN(0x0040c070, uint32_t, gameGetModeByte, (void));
/* Leaves the current run: back to the menu, or on to the replay screens. */
ORIG_FN(0x0040c0b0, void, gameLeaveRun, (void));
/* Resets the goals of the current level for a player. */
ORIG_FN(0x0040cd40, void, gameResetGoals, (int player));
/* Non-zero when the player has a pending unlock to show. */
ORIG_FN(0x0040cf70, int, gameHasPendingUnlock, (int player));

#endif
