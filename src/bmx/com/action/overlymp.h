/*
 * Overlay screens of the multiplayer modes (C:\zaxis\Bmx\src\com\action\overlymp.c): the mode selection,
 * the word entry of the HORSE style games and the score screens after a run.
 */
#ifndef BMX_COM_ACTION_OVERLYMP_H
#define BMX_COM_ACTION_OVERLYMP_H

#include <stdint.h>

#include "bmx/com/action/memcard.h"
#include "compat/orig.h"

/* The word being entered: 8 letters plus the terminator, then the cursor (0..7). */
typedef char OvlMpWord[9];
#define gOvlMpWord       ORIG_VAR(0x00557254, OvlMpWord, gOvlMpWord)
#define gOvlMpWordCursor ORIG_VAR(0x0055725d, uint8_t, gOvlMpWordCursor)
/* Number of text lines of the mode selection screen (the captions plus one). */
#define gOvlMpModeLines ORIG_VAR(0x00557184, uint8_t, gOvlMpModeLines)

/* Draws a screen title in the small font, centred at the top. */
void overlympDrawHeader(const char *title);
/* Mode selection: the captions with the current sub mode, then the menu items. */
void overlympDrawModes(void);
/* Input of the mode selection; returns 1 when a press was handled. */
int overlympModesUpdate(void);
/* Word entry: title and the eight letters (the one under the cursor pulses). */
void overlympDrawWordEntry(void);
int overlympWordEntryUpdate(void);
/* The line of the score screens that names the competition and the current round, centred at y. */
void overlympDrawBody(int y);
/* Menu of the screen after a run (play on / continue / menu). */
int overlympResultsUpdate(void);
/* Score screen after a run (one layout per game mode). */
void overlympDrawResults(void);
/* Run statistics screen of the HORSE, competition and timed modes. */
void overlympDrawRunStats(void);

#endif
