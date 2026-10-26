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
    uint32_t *mark;      /* 0x00 static region mark taken before the block was allocated */
    uint8_t state;       /* 0x04 1 recording, 2 playing back */
    uint8_t counter;     /* 0x05 frames since the last change record (low 5 bits when read back) */
    uint8_t rider;       /* 0x06 index of the rider (player) recorded */
    uint8_t flags;       /* 0x07 bit 0 / 1 / 2 set: the part is left out */
    uint32_t field08;    /* 0x08 frames played back */
    uint32_t padIndex;   /* 0x0c the rider's controller index when recording started */
    uint32_t field10;    /* 0x10 frames recorded */
    uint32_t partA;      /* 0x14 offsets of the optional parts (0: absent); A: rider snapshot at the start */
    uint32_t partAEnd;   /* 0x18 rider snapshot taken when playback starts (restored afterwards) */
    uint32_t partB;      /* 0x1c prop state at the start */
    uint32_t partBEnd;   /* 0x20 prop state when playback starts */
    uint32_t partC;      /* 0x24 goal info copy (0x27 bytes) */
    uint32_t partCEnd;   /* 0x28 */
    uint32_t headerSize; /* 0x2c always 0x40 */
    uint32_t size;       /* 0x30 size of the frame buffer */
    uint8_t *cursor;     /* 0x34 next byte to write (recording) or read (playback) */
    uint32_t buttons;    /* 0x38 the pad's buttons as of the last change record */
    uint16_t sticks[2];  /* 0x3c the two sticks as bytes (x low, y high) as of the last change record */
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

/* Records the pad of every recording replay or feeds the recorded input to every playing one (per frame). */
void replayUpdate(void);
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
/*
 * Switches a recorder's mode (0 off, 1 recording, 2 playing back, 4 finished): starting a recording
 * snapshots the rider, props and goal info; starting playback saves the live state and puts the recorded
 * one in place; leaving playback restores the live state. Returns the new state (only AL is set).
 */
uint8_t replaySetMode(uint32_t handle, uint32_t mode);
/* Gets the replay buffer of a recording: its start and size. */
void replayGetBuffer(uint32_t source, uint8_t **data, int *size);

#endif
