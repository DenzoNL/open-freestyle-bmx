/*
 * Positional sound sources (C:\zaxis\Bmx\src\com\zge\zsndsrc.c): a source is a voice at a world position.
 * Every update the source is transformed into the listener's space; inside the audible range it gets a
 * mixer channel (allocated on demand) with volume from the distance, pan from the sideways offset and a
 * (disabled) doppler pitch shift, and outside it releases the channel. Sources expire after a lifetime of
 * timer ticks unless their position is refreshed. 24 sources exist at once; handles are 16 bit (0xffff
 * is none).
 *
 * The stream wrappers at the top of the file (zsndStream*) belong to the same sound front end but only
 * forward to winstrm.c, with one stream and its buffer size kept in globals.
 */
#ifndef BMX_COM_ZGE_ZSNDSRC_H
#define BMX_COM_ZGE_ZSNDSRC_H

#include <stdint.h>

#include "bmx/pc/zge/sound/winload.h"
#include "bmx/pc/zge/sound/winstrm.h"
#include "compat/orig.h"
#include "zge/zmath.h"

enum {
    kZsndSrcMax = 24,
    kZsndSrcNone = 0xffff,
    kZsndSrcNoChannel = 0xff,
    kZsndSrcPriority = 0xfe, /* mixer priority of source voices */
};

/* ZsndSource.flags */
enum {
    kZsndSrcUsed = 0x8000,
};

typedef struct ZsndSource {
    uint16_t flags;  /* 0x00 */
    uint8_t channel; /* 0x02 mixer handle while audible, 0xff otherwise */
    uint8_t pad03;
    const ZsdVoice *voice; /* 0x04 */
    uint32_t lifetime;     /* 0x08 ticks left, 0: never updated */
    ZVec3 position;        /* 0x0c world position */
    uint8_t pad18[4];
    uint32_t distance2; /* 0x1c squared distance at the last update */
    int16_t doppler;    /* 0x20 pitch offset from the change of distance */
    int16_t pitch;      /* 0x22 base pitch offset in cents */
    uint8_t volume;     /* 0x24 distance volume of the last update */
    uint8_t baseVolume; /* 0x25 */
    int8_t pan;         /* 0x26 */
    uint8_t pad27;
} ZsndSource;

_Static_assert(sizeof(ZsndSource) == 0x28, "source layout");

typedef ZsndSource ZsndSourceTable[kZsndSrcMax];

#define gZsndSources      ORIG_VAR(0x0064fcd0, ZsndSourceTable, gZsndSources)
#define gZsndSrcNoDoppler ORIG_VAR(0x0064fcc8, int32_t, gZsndSrcNoDoppler)
/* Audible range in world units, its square, the doppler scale and the lifetime in ticks. */
#define gZsndSrcRange        ORIG_VAR(0x0064fccc, uint32_t, gZsndSrcRange)
#define gZsndSrcRange2       ORIG_VAR(0x00650094, uint32_t, gZsndSrcRange2)
#define gZsndSrcDopplerScale ORIG_VAR(0x00650090, float, gZsndSrcDopplerScale)
#define gZsndSrcLifetime     ORIG_VAR(0x006500a0, uint32_t, gZsndSrcLifetime)
#define gZsndSrcInitialized  ORIG_VAR(0x00650098, int32_t, gZsndSrcInitialized)
#define gZsndSrcNext         ORIG_VAR(0x0065009c, uint32_t, gZsndSrcNext)
#define gZsndStream          ORIG_VAR(0x0064fcb8, WinstrmStream *, gZsndStream)
#define gZsndStreamBytes     ORIG_VAR(0x0064fcbc, uint32_t, gZsndStreamBytes)

/* Transforms a world position into the listener's space (camera of the current frame). */
void zsystemToListener(const ZVec3 *world, ZVec3 *local);

/*
 * Clears the sources and sets the audible `range` (world units) and the source `lifetimeMs`.
 * Returns 0 (asserts when already initialised).
 */
int zsndSrcInit(uint32_t range, int lifetimeMs);
/* Destroys every source. */
void zsndSrcShutdown(void);
/*
 * Timer tick: counts the lifetimes down and (re)allocates, starts, releases and adjusts the mixer channels
 * of the sources. Returns 2 when a source could not get a channel, otherwise 0.
 */
int zsndSrcUpdate(void);
/* Makes the next update leave doppler pitch offsets out, and re-applies the base pitch of sources that had
 * one. */
void zsndSrcClearDoppler(void);
/* Creates a source for `voice` at `position`; returns its handle or 0xffff. */
uint16_t zsndSrcCreate(const ZsdVoice *voice, const ZVec3 *position);
void zsndSrcDestroy(uint16_t source);
int zsndSrcSetVoice(uint16_t source, const ZsdVoice *voice);
/* Moves the source and renews its lifetime. */
int zsndSrcSetPosition(uint16_t source, const ZVec3 *position);
/* Base pitch offset in cents. */
int zsndSrcSetPitch(uint16_t source, int16_t cents);
int zsndSrcSetVolume(uint16_t source, uint8_t volume);
/* The mixer handle of the source (0xff when it is not audible). */
uint8_t zsndSrcGetChannel(uint16_t source);
/*
 * Volume (0..127, falling off with the squared distance) and pan (+-127) of a world position for the
 * listener, without a source. Returns 1 when it is out of range (volume and pan 0), otherwise 0.
 */
int zsndSrcMeasure(const ZVec3 *position, uint8_t *volume, int8_t *pan);

/* ---- stream wrappers ---- */

/* Initialises the stream player and remembers the ring buffer size for zsndStreamPlay. */
void zsndStreamInit(uint32_t bufferBytes);
int zsndStreamShutdown(void);
int zsndStreamNop(void);
/* Starts a stream on `file`: mode bit 1 selects 22050 Hz, bit 0 11025 Hz (else 44100), bit 2 mono; repeats
 * it. */
int zsndStreamPlay(const WinloadSampleFile *file, uint8_t mode, int repeat);
int zsndStreamGetState(uint32_t *state);
int zsndStreamStart(void);
int zsndStreamStop(void);
/* Same as zsndStreamStop. */
int zsndStreamStop2(void);
int zsndStreamSetVolumePan(uint8_t volume, int8_t pan);
int zsndStreamFade(int millis, uint32_t target, uint32_t flags);
void zsndStreamPauseAll(int pause);
void zsndStreamSetFlag(uint32_t *flag);

#endif
