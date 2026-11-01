/*
 * Sound streaming (C:\zaxis\Bmx\src\Pc\zge\sound\winStrm.c): plays long sample files (music, speech,
 * ADPCM compressed) through one looping DirectSound buffer per stream (see winds.h for the buffer
 * protocol). A stream plays a chain of sources: a source is a sample file region opened for reading, with
 * a repeat count; the stream reads from the first one and moves to the next when it has been played that
 * often. Up to 8 streams, 32 open sources and 32 chain nodes exist at once.
 *
 * All calls take the module's critical section; winstrmUpdate and winstrmUpdateFades are meant to be run
 * from the multimedia timer (bmxsound.c). Result codes: 0 ok, 0x201 no such file, 0x202 no free stream,
 * 0x204 buffer too small, 0x205 no free chain node, 0x206 format mismatch with the playing source, 0x207
 * bad repeat count.
 */
#ifndef BMX_PC_ZGE_SOUND_WINSTRM_H
#define BMX_PC_ZGE_SOUND_WINSTRM_H

#include <stdint.h>

#include "bmx/pc/zge/sound/winapi.h"
#include "bmx/pc/zge/sound/winds.h"
#include "bmx/pc/zge/sound/winload.h"
#include "compat/orig.h"

enum {
    kWinstrmMaxStreams = 8,
    kWinstrmMaxSources = 32,
    kWinstrmMaxNodes = 32,
    kWinstrmDefaultBytes = 0x10000, /* ring buffer size when the caller passes 0 */
};

/* WinstrmStream.flags */
enum {
    kWinstrmFlagAutoDestroy = 0x0002, /* destroy instead of stop when the data has played out */
    kWinstrmFlagDraining = 0x2000,    /* the last source ran dry; the stream ends when the cursor passes */
    kWinstrmFlagPrimed = 0x4000,      /* the ring buffer already holds data for the start */
    kWinstrmFlagUsed = 0x8000,
};

/* WinstrmStream.state */
enum {
    kWinstrmStopped = 1,
    kWinstrmPlaying = 2,
    kWinstrmPaused = 3,
};

/* WinstrmStream.fadeFlags */
enum {
    kWinstrmFadeActive = 0x1,
    kWinstrmFadeThenStop = 0x2, /* stop (or destroy) the stream when the fade is done */
};

/*
 * Format flags of winstrmCreate / winstrmOpenSource: the sample rate is 44100 Hz with 0x40, 16000 with
 * 0x10, 11025 with 8, otherwise 22050.
 */
enum {
    kWinstrmFmtRate11k = 0x008,
    kWinstrmFmtRate16k = 0x010,
    kWinstrmFmtRate44k = 0x040,
    kWinstrmFmtStereo = 0x100, /* only for raw PCM: ADPCM sources are mono */
    kWinstrmFmt8Bit = 0x400,
    kWinstrmFmtPcm = 0x800, /* raw PCM; otherwise 4 bit ADPCM that is expanded to 16 bit */
};

/* WinstrmSource.flags */
enum {
    kWinstrmSourceAdpcm = 0x1,
};

/* An open sample file region (0x24 bytes). A source with size 0 is a free slot. */
typedef struct WinstrmSource {
    WinHandle file;       /* 0x00 */
    uint32_t size;        /* 0x04 bytes of data */
    uint16_t rate;        /* 0x08 */
    uint8_t bits;         /* 0x0a per sample: 8 or 16 */
    uint8_t channels;     /* 0x0b */
    uint32_t startOffset; /* 0x0c position of the data in the file */
    uint32_t position;    /* 0x10 bytes consumed so far */
    uint32_t flags;       /* 0x14 */
    int32_t predictor;    /* 0x18 ADPCM decoder state */
    uint32_t step;        /* 0x1c */
    int32_t index;        /* 0x20 */
} WinstrmSource;

/* Entry of a stream's source chain: play `source` `repeat` times, then continue with `next`. */
typedef struct WinstrmNode {
    uint32_t repeat;          /* 0x00 plays left, 0 once the last one is under way */
    WinstrmSource *source;    /* 0x04 */
    struct WinstrmNode *next; /* 0x08 */
} WinstrmNode;

/* One stream (0x44 bytes). The first chain node is embedded; the rest come from the node pool. */
typedef struct WinstrmStream {
    uint16_t flags;       /* 0x00 */
    uint8_t state;        /* 0x02 */
    int8_t pauseCount;    /* 0x03 */
    uint32_t bufferBytes; /* 0x04 size of the DirectSound ring buffer */
    uint32_t threshold;   /* 0x08 bytes the play cursor advances between refills */
    uint32_t writeOffset; /* 0x0c next ring buffer byte to fill */
    uint32_t lastPlay;    /* 0x10 play cursor at the last update */
    uint32_t endOffset;   /* 0x14 where the data stopped when the source ran dry */
    uint32_t played;      /* 0x18 bytes played since the last refill */
    WinstrmNode node;     /* 0x1c */
    uint32_t fadeFlags;   /* 0x28 */
    int32_t fadeLevel;    /* 0x2c volume, 16.16 */
    int32_t fadeTarget;   /* 0x30 */
    int32_t fadeStep;     /* 0x34 per tick */
    DsBuffer *buffer;     /* 0x38 */
    uint8_t volume;       /* 0x3c 0..127 */
    int8_t pan;           /* 0x3d */
    uint8_t pad3e[2];
    uint32_t drain; /* 0x40 bytes left until the end of the data reaches the play cursor */
} WinstrmStream;

BMX_LAYOUT32_ASSERT(sizeof(WinstrmSource) == 0x24, "source layout");
BMX_LAYOUT32_ASSERT(sizeof(WinstrmNode) == 0xc, "node layout");
BMX_LAYOUT32_ASSERT(sizeof(WinstrmStream) == 0x44, "stream layout");

typedef WinstrmStream WinstrmStreamTable[kWinstrmMaxStreams];
typedef WinstrmSource WinstrmSourceTable[kWinstrmMaxSources];
typedef WinstrmNode WinstrmNodeTable[kWinstrmMaxNodes];
/* The CRITICAL_SECTION (0x18 bytes). */
typedef uint8_t WinstrmLockStorage[0x18];

#define gWinstrmStreams     ORIG_VAR(0x006017d0, WinstrmStreamTable, gWinstrmStreams)
#define gWinstrmSources     ORIG_VAR(0x00601350, WinstrmSourceTable, gWinstrmSources)
#define gWinstrmNodes       ORIG_VAR(0x006019f8, WinstrmNodeTable, gWinstrmNodes)
#define gWinstrmLock        ORIG_VAR(0x00601330, WinstrmLockStorage, gWinstrmLock)
#define gWinstrmSourceHint  ORIG_VAR(0x00601328, int32_t, gWinstrmSourceHint)
#define gWinstrmStreamHint  ORIG_VAR(0x00601348, int32_t, gWinstrmStreamHint)
#define gWinstrmNodeHint    ORIG_VAR(0x006019f0, int32_t, gWinstrmNodeHint)
#define gWinstrmInitialized ORIG_VAR(0x00601b78, int32_t, gWinstrmInitialized)
#define gWinstrmPaused      ORIG_VAR(0x00601b7c, int32_t, gWinstrmPaused)
#define gWinstrmTick        ORIG_VAR(0x00601b80, int32_t, gWinstrmTick)
/* DirectSound device the stream buffers are created from (winchan.h owns it). */
#define gWinstrmDirectSound ORIG_VAR(0x005fff28, DsDevice *, gWinstrmDirectSound)
/* Timer ticks per second, set by bmxsound.c. */
#define gWinstrmTicksPerSec ORIG_VAR(0x0055ffc0, uint32_t, gWinstrmTicksPerSec)

/* Clears the stream, source and node pools. Returns 0. */
int winstrmInit(void);
/* Destroys every stream. Returns 0. */
int winstrmShutdown(void);
/* Timer tick: advances the volume fades of the playing streams. Returns 0. */
int winstrmUpdateFades(void);
/* Timer tick: refills the ring buffers of the playing streams as their cursors advance. Returns 0. */
int winstrmUpdate(void);
/*
 * Creates a stream on `file` (a region of a sound file, see winloadGetSampleFile) and fills its ring buffer.
 * `bufferBytes` 0 selects the default size; `flags` carries the format flags and kWinstrmFlagAutoDestroy.
 * The stream starts stopped. Stores it in *out (0 on failure). Returns a result code. `reserved` is unused.
 */
int winstrmCreate(WinstrmStream **out, const WinloadSampleFile *file, uint32_t reserved, uint32_t bufferBytes,
                  uint32_t flags);
/* Releases the buffer and every source of the stream, and frees its slot. Returns 0. */
int winstrmDestroy(WinstrmStream *stream);
/* Sets the refill threshold to `percent` of the ring buffer. Returns 0. */
int winstrmSetThreshold(WinstrmStream *stream, int percent);
/* Swaps the playing source for another of the same format and restarts the data. Returns a result code. */
int winstrmReplaceSource(WinstrmStream *stream, const WinloadSampleFile *file, uint32_t flags);
/* Sets how often the current source plays (> 0). Returns 0 or 0x207. */
int winstrmSetRepeat(WinstrmStream *stream, int count);
/* Appends (flags bit 0 clear) or inserts after the current source (bit 0 set) another source. */
int winstrmQueueSource(WinstrmStream *stream, const WinloadSampleFile *file, uint32_t flags);
/* Stores the state of the stream in *state. Returns 0. */
int winstrmGetState(const WinstrmStream *stream, uint32_t *state);
/* Starts playing from the beginning (or resumes a paused stream). Returns 0. */
int winstrmStart(WinstrmStream *stream);
/* Stops playing and cancels a fade. Returns 0. */
int winstrmStop(WinstrmStream *stream);
/* Pause (nonzero) or resume (0) with a nesting count. Returns 0. */
int winstrmPause(WinstrmStream *stream, int pause);
/* Sets volume 0..127 and pan (+-127, 1/100 dB steps of 0x4e) at once, cancelling a fade. Returns 0. */
int winstrmSetVolumePan(WinstrmStream *stream, uint8_t volume, int8_t pan);
/* Fades to `target` volume over `millis` ms; flags bit 1: stop when done. Returns 0. */
int winstrmFade(WinstrmStream *stream, int millis, uint32_t target, uint32_t flags);
/* Global pause counter: while it is positive no stream is updated. Returns 0. */
int winstrmPauseAll(int pause);
/* Sets *flag to 1. Returns 0. */
int winstrmSetFlag(uint32_t *flag);
/* Creates a one-shot stream (auto-destroying), sets volume and pan and starts it. */
void winstrmPlayOnce(const WinloadSampleFile *file, uint32_t bufferBytes, uint8_t volume, int8_t pan);
/*
 * Reads up to `bytes` of decoded sample data from the stream's source chain into the locked ring buffer at
 * the write offset, moving through the chain, and pads with silence when everything ran dry (then
 * *endPos is the ring offset where the data ended). Advances the write offset by `bytes`. Returns the
 * number of bytes of real data.
 */
uint32_t winstrmFill(WinstrmStream *stream, uint32_t bytes, uint32_t *endPos);
/* Opens the source for a file region. Returns the source or 0 (no file, or no free source slot). */
WinstrmSource *winstrmOpenSource(const WinloadSampleFile *file, uint32_t flags);
void winstrmCloseSource(WinstrmSource *source);
/* Reads and decodes up to `bytes` of output (4 bytes of 16 bit PCM per ADPCM byte). Returns the bytes
 * produced. */
uint32_t winstrmRead(WinstrmSource *source, uint8_t *dst, uint32_t bytes);
/* Rewinds the source (and resets the ADPCM decoder). Returns 0. */
int winstrmRewind(WinstrmSource *source);
WinstrmNode *winstrmNodeAlloc(void);
void winstrmNodeFree(WinstrmNode *node);
/* Expands `count` 4 bit ADPCM samples; the decoder state is updated through the pointers. */
void winstrmDecodeAdpcm(int16_t *dst, const uint8_t *src, uint32_t count, int32_t *predictor, uint32_t *step,
                        int32_t *index);

#endif
