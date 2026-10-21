/*
 * Replays (C:\zaxis\Bmx\src\com\action\replay.c): up to five recorders, each a block of the static memory
 * region with a header, a ring of input / state frames and optional parts that are copied in (the
 * level's goal info, a table of 0xb40 bytes).
 */
#ifndef BMX_COM_ACTION_REPLAY_H
#define BMX_COM_ACTION_REPLAY_H

#include <stdint.h>

#include "compat/orig.h"

/* Header of a recorder (0x40 bytes before the frame buffer). */
typedef struct Replay {
    uint32_t *mark; /* 0x00 static region mark taken before the block was allocated */
    uint8_t state;  /* 0x04 */
    uint8_t pad05;
    uint8_t rider; /* 0x06 index of the rider (player) recorded */
    uint8_t flags; /* 0x07 bit 0 / 1 / 2 set: the part is left out */
    uint32_t field08;
    uint8_t pad0c[4];
    uint32_t field10;
    uint32_t partA;      /* 0x14 offsets of the optional parts (0: absent) */
    uint32_t partAEnd;   /* 0x18 */
    uint32_t partB;      /* 0x1c */
    uint32_t partBEnd;   /* 0x20 */
    uint32_t partC;      /* 0x24 goal info copy (0x27 bytes) */
    uint32_t partCEnd;   /* 0x28 */
    uint32_t headerSize; /* 0x2c always 0x40 */
    uint32_t size;       /* 0x30 size of the frame buffer */
    uint8_t pad34[0xc];
} Replay;

_Static_assert(sizeof(Replay) == 0x40, "Replay size");

typedef Replay *ReplayTable[5];
#define gReplays ORIG_VAR(0x0056d278, ReplayTable, gReplays)

/* Parameters of replayCreate: bit 0 / 1 / 2 of flags leave out parts, size in bytes (0: 0x2000). */
typedef struct ReplayParams {
    uint8_t flags;
    uint8_t index;
    uint8_t pad02[2];
    int32_t size;
} ReplayParams;

/* Clears the table of recorders. */
void replayInit(void);
/* Destroys every recorder that exists. */
void replayDestroyAll(void);
/* Allocates a replay recorder and returns its handle (0xff when out of slots or memory). */
uint8_t replayCreate(const ReplayParams *params);
/* Frees a recorder (asserts the handle is valid; only the low byte is read). */
void replayDestroy(uint32_t handle);
uint32_t replayGetState(uint32_t handle);
uint32_t replaySetRider(uint32_t handle, uint32_t rider);
uint32_t replayGetRider(uint32_t handle);
uint32_t replayGetField08(uint32_t handle);
/* Gets the replay buffer of a recording: its start and size. */
void replayGetBuffer(uint32_t source, uint8_t **data, int *size);

#endif
