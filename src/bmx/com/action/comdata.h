/*
 * Common game data (C:\zaxis\Bmx\src\com\action\comdata.c): the view mode of each player's camera, the
 * level record tables and the reset of the profile / rider data. The view code of the camera state
 * (camera.h) lives here.
 */
#ifndef BMX_COM_ACTION_COMDATA_H
#define BMX_COM_ACTION_COMDATA_H

#include <stdint.h>

#include "compat/orig.h"

/* A level record (0x14 bytes); 24 of them per level, starting at 0x551fd8. */
typedef uint32_t ComdataRecord[5];
typedef ComdataRecord ComdataRecordTable[0x1200];
#define gComdataRecords ORIG_VAR(0x00551fd8, ComdataRecordTable, gComdataRecords)

/* Sets the view mode of a player's camera (hides / shows the actors for mode 3). */
void comdataSetViewMode(int player, uint32_t mode);
/*
 * Chooses the view of a player's camera (1..10): places the camera target at a respawn face near the rider
 * (ahead of the velocity or, standing still, of the facing direction) and sets the view flags.
 */
void comdataSetView(uint32_t player, uint32_t view);
/* Switches a camera to another view type and clears its state. */
void comdataResetView(int player, int mode);
/* The player whose camera is selected (low byte of the camera state). */
uint32_t comdataGetViewPlayer(void);
uint32_t comdataGetViewMode(int player);
uint32_t comdataGetViewType(int player);
/* The chosen view of a player's camera. */
uint32_t comdataGetView(uint32_t player);
/* Sets the profiles, riders and options to their defaults. */
void comdataInit(void);
/*
 * Level record of key (type << 24 | player << 22 | slot << 16 | ...) in the per level table at
 * 0x551fd8 (NULL for unknown types). The index inside a type is not range checked.
 */
uint32_t *comdataGetLevelRecord(uint32_t key);
/* Goal info of a level for a player: 0x27 bytes per level (asserts level < 6). */
uint8_t *comdataGetGoalInfo(int player, uint32_t level);

#endif
