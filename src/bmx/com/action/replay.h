/* Replays (C:\zaxis\Bmx\src\com\action\replay.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_REPLAY_H
#define BMX_COM_ACTION_REPLAY_H

#include <stdint.h>

#include "compat/orig.h"

/* Gets the replay buffer of a recording: its start and size. */
ORIG_FN(0x00430d70, void, replayGetBuffer, (uint32_t source, uint8_t **data, int *size));

#endif
