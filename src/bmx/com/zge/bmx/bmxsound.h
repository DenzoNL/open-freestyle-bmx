/* Game sound layer (C:\zaxis\Bmx\src\com\zge\bmx\bmxsound.c). Not ported yet; callees only. */
#ifndef BMX_COM_ZGE_BMX_BMXSOUND_H
#define BMX_COM_ZGE_BMX_BMXSOUND_H

#include "compat/orig.h"

/* Plays sound `id` at a world position at the given volume (0..127). */
ORIG_FN(0x0044e300, int, bmxSoundPlayAt, (int id, const void *position, int a, int volume, int b));

/* The same entry with the second argument as a plain number (a channel or an index, not a position). */
ORIG_FN(0x0044e300, int, bmxSoundPlayEvent, (int id, int a, int b, int c, int d));

#endif
