/*
 * Sound sequencer (C:\zaxis\Bmx\src\com\zge\zsndseq.c): plays event streams (notes, controllers, loops)
 * on the mixer's voice channels. A track is a pooled slot addressed by a byte handle; running notes
 * hang on a per-track list ordered by the time they end.
 */
#ifndef BMX_COM_ZGE_ZSNDSEQ_H
#define BMX_COM_ZGE_ZSNDSEQ_H

#include <stdint.h>

#include "bmx/pc/zge/sound/winload.h"
#include "compat/orig.h"

enum {
    kzsndSeqMaxTracks = 32,
    kzsndSeqMaxNotes = 64,
    kzsndSeqNoTrack = 0xff,
};

/* ZsndSeqTrack.flags */
enum {
    kzsndSeqModeMask = 0x3,         /* 0, 1 or 2+: how stopping/pausing treats running notes */
    kzsndSeqFlagOneShot = 0x4,      /* the track destroys itself when it ends */
    kzsndSeqFlagLoop = 0x800,       /* restart at the end marker */
    kzsndSeqFlagGroupMute = 0x1000, /* silenced while the global mute is on */
    kzsndSeqFlagMute = 0x2000,
    kzsndSeqFlagPending = 0x4000, /* a state change waits for the next update */
    kzsndSeqFlagUsed = 0x8000,
};

/* ZsndSeqTrack.state */
enum {
    kzsndSeqStateStopped = 0,
    kzsndSeqStatePlaying = 1,
    kzsndSeqStatePaused = 2,
};

/* zsndSeqGetStatus results. */
enum {
    kzsndSeqStatusReady = 1,
    kzsndSeqStatusRunning = 2,
    kzsndSeqStatusPaused = 3,
    kzsndSeqStatusStopped = 4,
};

/* ZsndNote.flags */
enum {
    kzsndNoteEventFlagActive = 0x1,
    kzsndNoteEventFlagSilent = 0x2, /* has no mixer channel (track muted) */
};

/* Sequence description: a track definition of the sound file (winload.h). */
typedef ZsdTrackDef ZsndSeqHeader;

struct ZsndNote;
struct ZsndSeqTrack;

/* A sounding (or muted) note. Notes of a track are chained in the order they end. */
typedef struct ZsndNote {
    uint8_t flags;     /* 0x00 */
    uint8_t velocity;  /* 0x01 */
    int16_t cents;     /* 0x02 pitch offset */
    const void *voice; /* 0x04 */
    uint8_t channel;   /* 0x08 mixer channel handle */
    uint8_t pad09[3];
    uint32_t remaining;    /* 0x0c ticks until the note ends, relative to the previous note on the list */
    struct ZsndNote *next; /* 0x10 */
} ZsndNote;

typedef void(BMX_CDECL *ZsndSeqEndFn)(uint32_t arg, uint32_t type, uint32_t value);

typedef struct ZsndSeqTrack {
    uint16_t flags;       /* 0x00 */
    uint8_t state;        /* 0x02 */
    uint8_t lastVelocity; /* 0x03 */
    uint8_t volume;       /* 0x04 */
    uint8_t expression;   /* 0x05 */
    uint8_t pan;          /* 0x06 */
    uint8_t pad07;
    const ZsndSeqHeader *header; /* 0x08 */
    uint32_t registers[4];       /* 0x0c read by an arm of the event decoder that cannot be reached */
    const void *voice;           /* 0x1c */
    const uint8_t *keymap;       /* 0x20 */
    const uint8_t *start;        /* 0x24 */
    const uint8_t *cursor;       /* 0x28 */
    uint32_t wait;               /* 0x2c ticks (16.12) until the next event */
    uint32_t step;               /* 0x30 time added per update (16.12) */
    uint32_t noteScale;          /* 0x34 event delta to note length factor */
    ZsndNote *notes;             /* 0x38 */
    ZsndSeqEndFn onEvent;        /* 0x3c called for end markers */
    uint32_t onEventArg;         /* 0x40 */
    uint16_t bank;               /* 0x44 */
    uint8_t pad46[2];
} ZsndSeqTrack;

BMX_LAYOUT32_ASSERT(sizeof(ZsndNote) == 0x14, "note layout");
BMX_LAYOUT32_ASSERT(sizeof(ZsndSeqTrack) == 0x48, "track layout");

struct ZsndSeqEvent;
typedef int(BMX_CDECL *ZsndSeqHandler)(struct ZsndSeqEvent *event);

/* A decoded event: the handler to run and its two operand bytes (or one 32 bit operand). */
typedef struct ZsndSeqEvent {
    ZsndSeqTrack *track; /* 0x00 */
    uint32_t delta;      /* 0x04 time to the following event */
    union {
        struct {
            uint8_t a, b;
        } byte;
        uint32_t value;
    } data;                 /* 0x08 */
    ZsndSeqHandler handler; /* 0x0c */
} ZsndSeqEvent;

BMX_LAYOUT32_ASSERT(sizeof(ZsndSeqEvent) == 0x10, "event layout");

typedef ZsndNote ZsndNotePool[kzsndSeqMaxNotes];
typedef ZsndSeqTrack ZsndTrackPool[kzsndSeqMaxTracks];
typedef ZsndSeqTrack *ZsndTrackTable[kzsndSeqMaxTracks];

#define gZsndNotes       ORIG_VAR(0x0064eb38, ZsndNotePool, gZsndNotes)
#define gZsndNextNote    ORIG_VAR(0x0064f038, int32_t, gZsndNextNote)
#define gZsndMuteAll     ORIG_VAR(0x0064f03c, int32_t, gZsndMuteAll)
#define gZsndNextTrack   ORIG_VAR(0x0064f040, int32_t, gZsndNextTrack)
#define gZsndTrackPool   ORIG_VAR(0x0064f048, ZsndTrackPool, gZsndTrackPool)
#define gZsndTracks      ORIG_VAR(0x0064f948, ZsndTrackTable, gZsndTracks)
#define gZsndSeqLock     ORIG_VAR(0x0064f9c8, int32_t, gZsndSeqLock)
#define gZsndTicksPerSec ORIG_VAR(0x0055ffc0, uint32_t, gZsndTicksPerSec)

/* Clears the track and note pools. Returns 0. */
int zsndSeqReset(void);
/* Destroys every live track. Returns 0. */
int zsndSeqStopAll(void);
/* Advances every track by one update and runs the events that fall due. Does nothing while locked. */
int zsndSeqUpdate(void);
/* Takes a free track slot for the sequence; returns its handle or kzsndSeqNoTrack. */
uint8_t zsndSeqCreate(const ZsndSeqHeader *header, uint16_t flags);
int zsndSeqDestroy(uint8_t hTrack);
/* Starts (or resumes a paused track); a stopped track restarts at the beginning. */
int zsndSeqStart(uint8_t hTrack);
int zsndSeqStop(uint8_t hTrack);
int zsndSeqPause(uint8_t hTrack);
/* Silences or restores the notes of a track. */
int zsndSeqMute(uint8_t hTrack, int mute);
/* Marks the track as muted by the global mute; sets or clears that global switch. */
int zsndSeqSetGroupMute(uint8_t hTrack, int enable);
int zsndSeqGetStatus(uint8_t hTrack);
/* Sets the low two flag bits and applies a pending state change. */
int zsndSeqSetMode(uint8_t hTrack, uint8_t mode);
/* Scales the playback speed (1.0 is normal). */
int zsndSeqSetTempo(uint8_t hTrack, float speed);
int zsndSeqSetVolume(uint8_t hTrack, uint8_t volume, uint8_t pan);
int zsndSeqSetLoop(uint8_t hTrack, int loop);
/* Plays a sequence once with the given volume and pan. Returns 0, or 0xff when no track was free. */
uint32_t zsndSeqPlayOnce(const ZsndSeqHeader *header, uint8_t volume, uint8_t pan);
int zsndSeqSetEndCallback(uint8_t hTrack, ZsndSeqEndFn callback, uint32_t arg);
/* Nonzero while the track is playing, or has a pending state change. */
uint32_t zsndSeqIsRunning(const ZsndSeqTrack *track);
/* Time to the next event of the track, from the delta field at the cursor. */
uint32_t zsndSeqPeekDelta(const ZsndSeqTrack *track);
/* Decodes the event at the cursor and advances past it; returns 0 for an unknown event type. */
int zsndSeqDecodeEvent(ZsndSeqTrack *track, ZsndSeqEvent *event);
/* Event handlers (stored into ZsndSeqEvent.handler). */
int zsndSeqOnNote(ZsndSeqEvent *event);
int zsndSeqOnController(ZsndSeqEvent *event);
int zsndSeqOnEnd(ZsndSeqEvent *event);
int zsndSeqOnJump(ZsndSeqEvent *event);
/* Inserts a note into the track's list, keeping the order and converting its time to a relative one. */
void zsndSeqInsertNote(ZsndSeqTrack *track, ZsndNote *note);
void zsndSeqReleaseNote(ZsndNote *note);
/* Looks the key up in a key map; returns 1 and the voice and pitch offset when it is mapped. */
int zsndSeqLookupKey(const uint8_t *keymap, uint8_t key, const void **voice, int16_t *cents);
/* Little endian 32 bit read. */
int32_t zsndSeqReadU32(const uint8_t *p);

#endif
