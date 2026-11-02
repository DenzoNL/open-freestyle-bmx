/*
 * The DirectSound and winmm surface of the sound system, as typed COM vtables.
 *
 * Everything in src/bmx/pc/zge/sound that touches DirectSound does it through these two interfaces, so
 * the layer can be re-implemented on SDL3 audio without changing the game logic. What the game uses:
 *
 * IDirectSound (gWinchanDirectSound, created by DirectSoundCreate from DSOUND.DLL; the stream player
 * creates its buffers from the same device)
 *   SetCooperativeLevel  DSSCL_NORMAL (1) on the game window; only the start-up probe uses it.
 *   CreateSoundBuffer    stream player, one secondary buffer per stream (WinstrmStream.buffer): PCM, 16 or
 *                        8 bit, mono or stereo, at the sample file's rate (11025, 16000, 22050 or 44100
 *                        Hz), flags CTRLPAN | CTRLVOLUME | CTRLFREQUENCY (0xe0), `bufferBytes` long
 *                        (default 0x10000). The mixer's own output buffer is created in winchanInit
 *                        (not in this pass) in the same way.
 *   Release              shutdown.
 *
 * IDirectSoundBuffer
 *   GetStatus            mixer: is the output buffer playing (DSBSTATUS_PLAYING, bit 0)?
 *   GetCurrentPosition   mixer and streams: the play cursor, in bytes into the looping buffer.
 *   Lock / Unlock        mixer: copy the next chunk of software mixed audio from the ring buffer into the
 *                        device buffer; DSERR_BUFFERLOST (0x88780096) is answered with Restore and a
 *                        second Lock. Streams: write decoded sample data behind the write offset; the
 *                        code never uses the second region (it asserts it is empty).
 *   Play                 always looping (DSBPLAY_LOOPING, 1): the buffers are rings.
 *   Stop                 stop or pause (a paused stream is a stopped buffer that keeps its position).
 *   SetCurrentPosition   restart a stream from byte 0.
 *   SetVolume            1/100 dB. Streams: a 128 entry table (-10000 .. 0) indexed by the 0..127 game
 *                        volume, and (volume - 127) * 12 while a fade runs.
 *   SetPan               1/100 dB, stream pan (-127..127) * 78 (0x4e).
 *   Release              when a stream is destroyed or the mixer shuts down.
 *
 * winmm (winapi.h): timeSetEvent (16 ms periodic callback that pumps the mixer, polls input and drives the
 * sequencer, see bmxsound.c) with timeBeginPeriod / timeEndPeriod / timeGetDevCaps / timeKillEvent around it.
 *
 * Streaming protocol (winstrm.c): every stream owns a looping DirectSound buffer of `bufferBytes` and a
 * write offset into it. winstrmUpdate (called from the timer) reads the play cursor, adds the bytes the
 * cursor advanced to `played`, and whenever `played` reaches the refill threshold (half the buffer by
 * default) it locks the region at the write offset and fills one threshold's worth from the current source
 * (4 bit ADPCM is expanded to 16 bit on the way), splitting the refill where the buffer wraps. When a
 * source ends, the next one of the chain is started (or the same one again while its repeat count
 * lasts); when nothing is left, the rest is padded with silence and the stream is retired once the play
 * cursor reaches the end of the data.
 */
#ifndef BMX_PC_ZGE_SOUND_WINDS_H
#define BMX_PC_ZGE_SOUND_WINDS_H

#include <stdint.h>

#include "compat/orig.h"

typedef int32_t DsResult;

enum {
    kDsOk = 0,
    kDsErrBufferLost = (int32_t)0x88780096,
    kDsStatusPlaying = 1,
    kDsPlayLooping = 1,
    kDsCapsControlFrequency = 0x20,
    kDsCapsControlPan = 0x40,
    kDsCapsControlVolume = 0x80,
};

struct DsBuffer;

typedef struct DsBufferVtbl {
    void *queryInterface;                                  /* 0x00 */
    void *addRef;                                          /* 0x04 */
    uint32_t(BMX_STDCALL *release)(struct DsBuffer *self); /* 0x08 */
    void *getCaps;                                         /* 0x0c */
    DsResult(BMX_STDCALL *getCurrentPosition)(struct DsBuffer *self, uint32_t *play,
                                              uint32_t *write);                /* 0x10 */
    void *getFormat;                                                           /* 0x14 */
    void *getVolume;                                                           /* 0x18 */
    void *getPan;                                                              /* 0x1c */
    void *getFrequency;                                                        /* 0x20 */
    DsResult(BMX_STDCALL *getStatus)(struct DsBuffer *self, uint32_t *status); /* 0x24 */
    void *initialize;                                                          /* 0x28 */
    DsResult(BMX_STDCALL *lock)(struct DsBuffer *self, uint32_t offset, uint32_t bytes, void **ptr1,
                                uint32_t *bytes1, void **ptr2, uint32_t *bytes2, uint32_t flags); /* 0x2c */
    DsResult(BMX_STDCALL *play)(struct DsBuffer *self, uint32_t reserved, uint32_t priority,
                                uint32_t flags);                                         /* 0x30 */
    DsResult(BMX_STDCALL *setCurrentPosition)(struct DsBuffer *self, uint32_t position); /* 0x34 */
    void *setFormat;                                                                     /* 0x38 */
    DsResult(BMX_STDCALL *setVolume)(struct DsBuffer *self, int32_t volume);             /* 0x3c */
    DsResult(BMX_STDCALL *setPan)(struct DsBuffer *self, int32_t pan);                   /* 0x40 */
    void *setFrequency;                                                                  /* 0x44 */
    DsResult(BMX_STDCALL *stop)(struct DsBuffer *self);                                  /* 0x48 */
    DsResult(BMX_STDCALL *unlock)(struct DsBuffer *self, void *ptr1, uint32_t bytes1, void *ptr2,
                                  uint32_t bytes2);        /* 0x4c */
    DsResult(BMX_STDCALL *restore)(struct DsBuffer *self); /* 0x50 */
} DsBufferVtbl;

typedef struct DsBuffer {
    const DsBufferVtbl *vtbl;
} DsBuffer;

/* PCMWAVEFORMAT (16 bytes) as the stream player builds it for CreateSoundBuffer. */
typedef struct DsWaveFormat {
    uint16_t formatTag;      /* 1: PCM */
    uint16_t channels;       /* 1 or 2 */
    uint32_t samplesPerSec;  /* sample rate */
    uint32_t avgBytesPerSec; /* blockAlign * rate */
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} DsWaveFormat;

_Static_assert(sizeof(DsWaveFormat) == 16, "PCMWAVEFORMAT");

/* DSBUFFERDESC (0x24 bytes). */
typedef struct DsBufferDesc {
    uint32_t size; /* 0x24 */
    uint32_t flags;
    uint32_t bufferBytes;
    uint32_t reserved;
    const DsWaveFormat *format;
    uint8_t pad14[0x10]; /* guid3DAlgorithm */
} DsBufferDesc;

struct DsDevice;

typedef struct DsDeviceVtbl {
    void *queryInterface;                                  /* 0x00 */
    void *addRef;                                          /* 0x04 */
    uint32_t(BMX_STDCALL *release)(struct DsDevice *self); /* 0x08 */
    DsResult(BMX_STDCALL *createSoundBuffer)(struct DsDevice *self, const DsBufferDesc *desc,
                                             DsBuffer **buffer, void *outer); /* 0x0c */
    void *getCaps;                                                            /* 0x10 */
    void *duplicateSoundBuffer;                                               /* 0x14 */
    DsResult(BMX_STDCALL *setCooperativeLevel)(struct DsDevice *self, void *window,
                                               uint32_t level); /* 0x18 */
} DsDeviceVtbl;

typedef struct DsDevice {
    const DsDeviceVtbl *vtbl;
} DsDevice;

BMX_LAYOUT32_ASSERT(sizeof(DsBufferDesc) == 0x24, "DSBUFFERDESC");

#endif
