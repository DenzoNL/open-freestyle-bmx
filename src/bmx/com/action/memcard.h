/*
 * The save file (C:\zaxis\Bmx\src\com\action\memcard.c): the console memory card API is replaced by one
 * file, .\save\SAVE.DAT, that holds the options, the rider data, the level records and the profiles.
 */
#ifndef BMX_COM_ACTION_MEMCARD_H
#define BMX_COM_ACTION_MEMCARD_H

#include <stdint.h>

#include "bmx/pc/zge/sound/winchan.h"
#include "compat/orig.h"

/*
 * Status codes of the front end (PS1 card codes): 0 ok, 5 cannot open (no file), 6 file exists, 8 invalid
 * data (old version), 9 read / write failure, -1 out of memory.
 */
enum {
    kSaveOk = 0,
    kSaveNoFile = 5,
    kSaveExists = 6,
    kSaveInvalid = 8,
    kSaveIoError = 9,
};

/* The file: magic and size, then copies of the three blocks of game state. */
enum {
    kSaveMagic = 0x2a81511c,
    kSaveSize = 0x235c,
};

/* Opens the file for reading and closes it again: 6 if it exists, else 5. */
int saveFileClose(void);
/* Always 0 (a card in this build is always formatted). */
int memcardFormat(void);
/* Loads the save file into the game state. */
int saveFileRead(void);
/* Writes the game state to the save file. */
int saveFileWrite(void);

/* Card API stubs left over from the console port: ported as empty functions in winchan.c. */
#define memcardSelectSlot winchanStart
#define memcardBegin      winchanBegin
#define memcardEnd        winchanEnd
/* Returns 0. */
#define memcardQuery winchanQuery

#endif
