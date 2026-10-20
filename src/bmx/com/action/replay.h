/* Replays (C:\zaxis\Bmx\src\com\action\replay.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_REPLAY_H
#define BMX_COM_ACTION_REPLAY_H

#include <stdint.h>

#include "compat/orig.h"

/* Gets the replay buffer of a recording: its start and size. */
ORIG_FN(0x00430d70, void, replayGetBuffer, (uint32_t source, uint8_t **data, int *size));

/* Parameters of replayCreate: bit 0 / 1 / 2 of flags select what is recorded, size in bytes (0: 0x2000). */
typedef struct ReplayParams {
    uint8_t flags;
    uint8_t index;
    uint8_t pad02[2];
    int32_t size;
} ReplayParams;

/* Allocates a replay recorder and returns its handle (0xff when out of slots or memory). */
ORIG_FN(0x00430180, uint8_t, replayCreate, (const ReplayParams *params));
/* Frees a recorder (asserts the handle is valid; only the low byte is read). */
ORIG_FN(0x00430380, void, replayDestroy, (uint32_t handle));

#endif
