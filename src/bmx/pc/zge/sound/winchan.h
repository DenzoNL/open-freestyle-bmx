/*
 * Windows sound channels (C:\zaxis\Bmx\src\Pc\zge\sound\winChan.c): a software mixer with 24 voice
 * channels ("zsndc") that renders into a ring buffer, which a DirectSound looping buffer plays (see
 * winds.h). Callers address a playing voice by a byte handle (64 of them, 0xff is none); a handle maps to
 * a mixer channel while the voice sounds. Handles and small values are passed as full words with
 * undefined upper bits.
 *
 * Not ported here (separate pass): winchanInit (0x0046d940) and the mixer itself, winchanMix (0x0046f5f0).
 *
 * The first block of functions are empty stubs: the PS1-style memory card API of the console port, which
 * the packet files under winchan.c (probably a separate source file).
 */
#ifndef BMX_PC_ZGE_SOUND_WINCHAN_H
#define BMX_PC_ZGE_SOUND_WINCHAN_H

#include <stdint.h>

#include "bmx/pc/zge/sound/winds.h"
#include "bmx/pc/zge/sound/winload.h"
#include "compat/orig.h"

enum {
    kWinchanNumChannels = 24,
    kWinchanNumHandles = 64,
    kWinchanNoHandle = 0xff,
    kWinchanNoChannel = 0xff,
};

/* WinchanChannel.state */
enum {
    kWinchanFree = 0,
    kWinchanStopped = 1,
    kWinchanPlaying = 2,
    kWinchanFinishing = 3,
};

/* WinchanChannel.flags */
enum {
    kWinchanFlagFreeHandle = 0x1, /* release the handle when the channel ends */
    kWinchanFlagNativeRate = 0x2, /* the voice plays at the mixer rate: no resampling */
};

/* WinchanChannel.phase (the amplitude envelope) */
enum {
    kWinchanPhaseAttack = 0,
    kWinchanPhaseDecay = 1,
    kWinchanPhaseSustain = 2,
    kWinchanPhaseRelease = 3,
};

/* One mixer channel (0x48 bytes). Gains are looked up in the exe's 128 entry volume curve. */
typedef struct WinchanChannel {
    uint8_t state;    /* 0x00 */
    uint8_t flags;    /* 0x01 */
    uint8_t priority; /* 0x02 */
    uint8_t handle;   /* 0x03 */
    uint8_t pad04[4];
    const ZsdVoice *voice;   /* 0x08 */
    const ZsdSample *sample; /* 0x0c */
    uint32_t length;         /* 0x10 sample frames */
    uint32_t step;           /* 0x14 resampling step per output frame, 20.12 fixed point */
    uint32_t position;       /* 0x18 */
    uint32_t fraction;       /* 0x1c */
    uint8_t volume;          /* 0x20 voice volume scaled by the caller's volume */
    int8_t pan;              /* 0x21 */
    uint8_t phase;           /* 0x22 */
    uint8_t pad23;
    float gainLeft;       /* 0x24 */
    float gainRight;      /* 0x28 */
    uint32_t phaseFrames; /* 0x2c frames left in the current envelope phase, 0: until stopped */
    float level;          /* 0x30 envelope level */
    float levelStep;      /* 0x34 change of the level per frame in the current phase */
    float attackStep;     /* 0x38 */
    float decayStep;      /* 0x3c */
    float sustainLevel;   /* 0x40 */
    float fadeStep;       /* 0x44 */
} WinchanChannel;

typedef struct WinchanHandle {
    uint8_t allocated;  /* 0x00 */
    uint8_t channelNum; /* 0x01 mixer channel index, 0xff none */
} WinchanHandle;

_Static_assert(sizeof(WinchanChannel) == 0x48, "channel layout");

/* Parameters for winchanSetEffect. */
typedef struct WinchanEffect {
    int32_t kind;    /* 0 or 1 */
    int32_t level;   /* low byte, signed */
    uint32_t millis; /* length of the effect buffer */
    union {
        uint32_t raw; /* kind 0 */
        float scale;  /* kind 1 */
    } param;
    float tail; /* 0x10 */
} WinchanEffect;

typedef WinchanChannel WinchanChannelTable[kWinchanNumChannels];
/* One entry more than there are handles: winchanFreeAllChannels scans one past the table. */
typedef WinchanHandle WinchanHandleTable[kWinchanNumHandles + 1];

#define gWinchanChannels    ORIG_VAR(0x005fb770, WinchanChannelTable, gWinchanChannels)
#define gWinchanHandles     ORIG_VAR(0x005fbe38, WinchanHandleTable, gWinchanHandles)
#define gWinchanActive      ORIG_VAR(0x005fb768, int32_t, gWinchanActive)
#define gWinchanInitialized ORIG_VAR(0x005fff0c, int32_t, gWinchanInitialized)
#define gWinchanLock        ORIG_VAR(0x005fff10, int32_t, gWinchanLock)
#define gWinchanNextChannel ORIG_VAR(0x005fff14, int32_t, gWinchanNextChannel)
#define gWinchanNextHandle  ORIG_VAR(0x005fff18, int32_t, gWinchanNextHandle)
#define gWinchanPumpCount   ORIG_VAR(0x005fff24, int32_t, gWinchanPumpCount)
/* Mixer output rate in Hz. */
#define gWinchanRate ORIG_VAR(0x005ffee4, uint32_t, gWinchanRate)
/* The software ring buffer the mixer renders into, and what the pump tracks of it. */
#define gWinchanRing        ORIG_VAR(0x005ffed8, uint8_t *, gWinchanRing)
#define gWinchanRingBytes   ORIG_VAR(0x005fff08, uint32_t, gWinchanRingBytes)
#define gWinchanChunkBytes  ORIG_VAR(0x005ffedc, uint32_t, gWinchanChunkBytes)
#define gWinchanCopyOffset  ORIG_VAR(0x005fdebc, uint32_t, gWinchanCopyOffset)
#define gWinchanWriteOffset ORIG_VAR(0x005fdec0, uint32_t, gWinchanWriteOffset)
#define gWinchanMixOffset   ORIG_VAR(0x005ffef4, uint32_t, gWinchanMixOffset)
#define gWinchanLastPlay    ORIG_VAR(0x005ffef8, uint32_t, gWinchanLastPlay)
/* Effect state set by winchanSetEffect (the mixer reads it). */
#define gWinchanEffectMaxFrames  ORIG_VAR(0x005fb76c, uint32_t, gWinchanEffectMaxFrames)
#define gWinchanEffectAFrames    ORIG_VAR(0x005ffeec, uint32_t, gWinchanEffectAFrames)
#define gWinchanEffectAGain      ORIG_VAR(0x005fbe34, float, gWinchanEffectAGain)
#define gWinchanEffectAParam     ORIG_VAR(0x005ffef0, uint32_t, gWinchanEffectAParam)
#define gWinchanEffectBFrames    ORIG_VAR(0x005ffee0, uint32_t, gWinchanEffectBFrames)
#define gWinchanEffectBGain      ORIG_VAR(0x005ffee8, float, gWinchanEffectBGain)
#define gWinchanEffectBTailStart ORIG_VAR(0x005ffefc, int32_t, gWinchanEffectBTailStart)
#define gWinchanEffectBTail      ORIG_VAR(0x005fff04, int32_t, gWinchanEffectBTail)
/* DirectSound objects and the DLL they come from. */
#define gWinchanBuffer       ORIG_VAR(0x005fb760, DsBuffer *, gWinchanBuffer)
#define gWinchanPrimary      ORIG_VAR(0x005fff20, DsBuffer *, gWinchanPrimary)
#define gWinchanDirectSound  ORIG_VAR(0x005fff28, DsDevice *, gWinchanDirectSound)
#define gWinchanDsoundModule ORIG_VAR(0x005ffed4, void *, gWinchanDsoundModule)
#define gWinchanDsoundCreate ORIG_VAR(0x005fff1c, void *, gWinchanDsoundCreate)

/* ---- console port stubs ------------------------------------------------------------------------------ */

typedef void(BMX_CDECL *WinchanCallback)(int a, int b);

int winchanCardStub6e0(void);
void winchanCardStub700(void);
void winchanStart(uint32_t slot);
int winchanCardStub740(void);
int winchanCardStub760(void);
int winchanCardStub780(void);
/* Registers the callback that reports the progress of the save thread. */
int winchanSetCallback(WinchanCallback callback);
int winchanCardStub7c0(void);
int winchanCardStub7e0(void);
int winchanCardStub800(void);
int winchanCardStub820(void);
int winchanCardStub840(void);
int winchanCardStub860(void);
int winchanCardStub880(void);
int winchanQuery(void);
void winchanBegin(void);
void winchanEnd(void);
void winchanCardStub900(void);
int winchanCardStub920(void);

/* ---- mixer ------------------------------------------------------------------------------------------- */

/* Frees every voice and releases the DirectSound objects and the ring buffer. */
void winchanShutdown(void);
/*
 * Timer tick: reads the play cursor of the output buffer, copies the next chunk of mixed audio from the
 * ring into it when the cursor closes in, and mixes new audio into the ring for the elapsed time. Returns
 * 0, 0x10c when the status query fails, 0x109 when locking the buffer fails.
 */
uint32_t winchanPump(void);
/* Frees every handle in use (and scans one past the table, see gWinchanHandles). */
void winchanFreeAllChannels(void);

/*
 * Reserves a handle and a mixer channel for the voice; steals the quietest channel when none is free and
 * the priority is at least as high. Returns the handle or 0xff.
 */
uint8_t winchanVoiceAlloc(const ZsdVoice *voice, int priority);
/* Stops the voice at once and frees its handle. Returns 0. */
int winchanVoiceFree(uint8_t channel);
/* Allocates, sets volume, pan and pitch, starts the voice, and frees the handle when it ends. Returns 0. */
int winchanVoicePlay(const ZsdVoice *voice, int16_t cents, uint8_t volume, int8_t pan, int priority);
/* Starts the envelope. Returns 0. */
int winchanVoiceStart(uint8_t channel);
/* flags: 1 stops immediately instead of releasing, 2 also frees the channel. */
int winchanVoiceStop(uint8_t channel, uint8_t flags);
/* 0 not playing, 1 stopped, 2 playing, 3 finishing. */
int winchanVoiceStatus(uint8_t channel);
int winchanVoiceSetVolume(uint8_t channel, uint8_t volume);
/* Pan offset added to the voice's own pan, clamped to +-127. */
int winchanVoiceSetPan(uint8_t channel, int8_t pan);
/* Pitch offset in cents. */
int winchanVoiceSetPitch(uint8_t channel, int16_t cents);
/* The voice a handle plays (0 for none). */
const ZsdVoice *winchanVoiceGetVoice(uint8_t channel);
/* Stops and frees every channel that plays the voice. */
void winchanStopVoice(const ZsdVoice *voice);
/* Sets the parameters of effect `kind` 0 or 1. Returns 0, or 0x10d for another kind. */
uint32_t winchanSetEffect(const WinchanEffect *effect);
/* Checks that DSOUND.DLL, DirectSoundCreate and (with a window) the cooperative level work. Returns 0 or an
 * error code. */
uint32_t winchanProbeDevice(void *window);
/* The channel behind a handle, or 0 when the handle has none. */
WinchanChannel *winchanChannelOf(uint8_t handle);
/* Returns a channel to the free pool (and frees its handle when it was flagged to). */
void winchanChannelRelease(WinchanChannel *channel);
/*
 * Envelope step for a change of `amount`/127 over `millis`: amount * 1000 / (rate * millis * 127) per frame
 * (amount / 127 / rate when millis is 0). Returned in st0.
 */
double winchanEnvelopeStep(int32_t amount, int32_t millis);

/* Parameters of winchanInit. */
typedef struct WinchanInitDesc {
    void *window;      /* 0x00 window handle (first field of the platform object 0x15) */
    uint32_t reserved; /* 0x04 */
    uint32_t rate;     /* 0x08 mixer output rate in Hz */
    uint8_t pad0c[8];
} WinchanInitDesc;

_Static_assert(sizeof(WinchanInitDesc) == 0x14, "init descriptor");

/* Creates the DirectSound objects, the ring buffer and the channel tables; returns 0 or an error code. */
ORIG_FN(0x0046d940, int, winchanInit, (const WinchanInitDesc *desc));
/* Renders `bytes` of audio of the playing channels into the ring buffer at `ring`. */
ORIG_FN(0x0046f5f0, void, winchanMix, (uint8_t *ring, uint32_t bytes));

#endif
