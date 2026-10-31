/*
 * Internals of the DirectSound stand-in (dsound.c): the mixer's test hooks (tests/platform drives it without
 * an audio device) and the BMX_SOUND_CHECK hooks (soundcheck.c).
 */
#ifndef BMX_PLATFORM_SOUND_H
#define BMX_PLATFORM_SOUND_H

#include <stdint.h>

#include "platform/dsface.h"

/* ---- the mixer ---- */

/*
 * Tests: run the mixer without an audio device at `rate` Hz (0: open the SDL device as the game does, at the
 * device's own rate). Call before the first DirectSoundCreate; bmxSoundReset forgets every object.
 */
void bmxSoundUseOffline(uint32_t rate);
void bmxSoundReset(void);
/* The mixer's rate and the frames of one 10 ms mix (after the first DirectSoundCreate). */
uint32_t bmxSoundRate(void);
uint32_t bmxSoundQuantum(void);
/* Offline: mixes one 10 ms quantum into out (quantum stereo float frames; NULL: discard). */
void bmxSoundMixOffline(float *out);
/* 1 when the game's window has the focus (buffers without DSBCAPS_GLOBALFOCUS are silent otherwise); tests
 * may force it (-1: ask SDL again). */
void bmxSoundForceFocus(int focused);
/* At exit: closes the audio device (bmxPlatformShutdown). */
void bmxSoundShutdown(void);

/* The resampler's lookahead DirectSound counts in the write cursor (frames of the buffer), and the write lead
 * (play cursor = write cursor - lead), both measured on Windows 10/11 (docs/PLATFORM.md 4c). */
uint32_t bmxDsLookahead(uint32_t frequency, uint32_t rate);
uint32_t bmxDsLead(uint32_t frequency, uint32_t rate, uint32_t sizeFrames);

/* DirectSound's gain for a volume and pan (1/100 dB; DSBVOLUME_MIN is silence): left and right. */
void bmxDsGains(int32_t volume, int32_t pan, float *left, float *right);

/* ---- BMX_SOUND_CHECK (soundcheck.c): the same calls on the real DirectSound, compared ---- */

typedef struct BmxSoundCheckBuffer BmxSoundCheckBuffer;

/* 1 when BMX_SOUND_CHECK is set. */
int bmxSoundCheckEnabled(void);
/* Creates the real IDirectSound (once per our device object; NULL when unavailable). */
BmxDs *bmxSoundCheckCreate(void);
void bmxSoundCheckReleaseDevice(BmxDs *real);
void bmxSoundCheckCooperative(BmxDs *real, void *window, uint32_t level, BmxHresult ours);
/* The real buffer created with the same description; ours: our result and buffer. */
BmxSoundCheckBuffer *bmxSoundCheckCreateBuffer(BmxDs *real, const BmxDsBufferDesc *desc, BmxHresult ours,
                                               BmxDsBuffer *buffer);
BmxSoundCheckBuffer *bmxSoundCheckDuplicate(BmxDs *real, BmxSoundCheckBuffer *original, BmxHresult ours,
                                            BmxDsBuffer *buffer);
void bmxSoundCheckRelease(BmxSoundCheckBuffer *c);
/* A call with only an HRESULT to compare (name: the method), and the state after it. */
void bmxSoundCheckSimple(BmxSoundCheckBuffer *c, const char *name, uint32_t arg, BmxHresult ours);
void bmxSoundCheckStatus(BmxSoundCheckBuffer *c, BmxHresult ours, uint32_t status);
void bmxSoundCheckPosition(BmxSoundCheckBuffer *c, BmxHresult ours, uint32_t play, uint32_t write);
void bmxSoundCheckLock(BmxSoundCheckBuffer *c, uint32_t offset, uint32_t bytes, uint32_t flags,
                       BmxHresult ours, uint32_t off1, uint32_t bytes1, int hasPtr2, uint32_t bytes2);
void bmxSoundCheckUnlock(BmxSoundCheckBuffer *c, uint32_t off1, uint32_t bytes1, int hasPtr2, uint32_t bytes2,
                         BmxHresult ours);
void bmxSoundCheckSetFormat(BmxSoundCheckBuffer *c, const BmxWaveFormat *format, BmxHresult ours);
/* Logs the totals (bmxPlatformShutdown); the number of differences so far; forget everything (tests). */
void bmxSoundCheckSummary(void);
uint64_t bmxSoundCheckDifferences(void);
void bmxSoundCheckReset(void);

#endif
