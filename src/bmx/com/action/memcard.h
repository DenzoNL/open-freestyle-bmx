/* Memory card layer (C:\zaxis\Bmx\src\com\action\memcard.c and the PS1-style card API stubs). Not ported yet.
 */
#ifndef BMX_COM_ACTION_MEMCARD_H
#define BMX_COM_ACTION_MEMCARD_H

#include <stdint.h>

#include "compat/orig.h"

/*
 * Save file operations on the selected card slot. The status codes are the PS1 card codes of the front end:
 * 0 ok, 1/2 read errors, 4 no file, 5 unformatted, 6 ok (file exists), -1 read failure.
 */
ORIG_FN(0x00412130, int, saveFileClose, (void));
ORIG_FN(0x00412190, int, saveFileRead, (void));
ORIG_FN(0x00412340, int, saveFileWrite, (void));
/* Asked for an unformatted card (status 5); returns 0. */
ORIG_FN(0x00412170, int, memcardFormat, (void));

/* Card API stubs left over from the console port (filed under winchan.c by the packet; empty or constant). */
ORIG_FN(0x0046d720, void, memcardSelectSlot, (int slot));
ORIG_FN(0x0046d8c0, void, memcardBegin, (void));
ORIG_FN(0x0046d8e0, void, memcardEnd, (void));
/* Returns 0. */
ORIG_FN(0x0046d8a0, int, memcardQuery, (void));

#endif
