/*
 * The DirectSound surface the game uses (winchan.c, winstrm.c; see bmx/pc/zge/sound/winds.h), in our own
 * types (src/platform has no windows.h): the IDirectSound and IDirectSoundBuffer vtable layouts in full,
 * WAVEFORMATEX, DSBUFFERDESC, DSBCAPS and the HRESULTs. The stand-in (dsound.c) implements these;
 * BMX_SOUND_CHECK (soundcheck.c) and the tests call the real dsound.dll through the same declarations.
 */
#ifndef BMX_PLATFORM_DSFACE_H
#define BMX_PLATFORM_DSFACE_H

#include <stdint.h>

#include "platform/diface.h"

#define kDsOk                    ((BmxHresult)0)
#define kDsErrInvalidParam       ((BmxHresult)0x80070057)
#define kDsErrPrioLevelNeeded    ((BmxHresult)0x88780046)
#define kDsErrControlUnavail     ((BmxHresult)0x8878001e)
#define kDsErrInvalidCall        ((BmxHresult)0x88780032)
#define kDsErrBadFormat          ((BmxHresult)0x88780064)
#define kDsErrNoAggregation      ((BmxHresult)0x80040110)
#define kDsErrAlreadyInitialized ((BmxHresult)0x88780082)
#define kDsErrNoDriver           ((BmxHresult)0x88780078)
#define kDsErrNoInterface        ((BmxHresult)0x80004002)
#define kDsErrUnsupported        ((BmxHresult)0x80004001)
#define kDsErrOutOfMemory        ((BmxHresult)0x8007000e)

enum {
    /* DSSCL_* */
    kDsCoopNormal = 1,
    kDsCoopPriority = 2,
    kDsCoopExclusive = 3,
    kDsCoopWritePrimary = 4,
    /* DSBCAPS_* */
    kDsbPrimary = 0x1,
    kDsbStatic = 0x2,
    kDsbLocHardware = 0x4,
    kDsbLocSoftware = 0x8,
    kDsbCtrlFrequency = 0x20,
    kDsbCtrlPan = 0x40,
    kDsbCtrlVolume = 0x80,
    kDsbStickyFocus = 0x4000,
    kDsbGlobalFocus = 0x8000,
    /* DSBPLAY_* */
    kDsbPlayLooping = 0x1,
    /* DSBSTATUS_* */
    kDsbStatusPlaying = 0x1,
    kDsbStatusLooping = 0x4,
    /* DSBLOCK_* */
    kDsbLockFromWriteCursor = 0x1,
    kDsbLockEntireBuffer = 0x2,
    /* limits */
    kDsbSizeMin = 4,
    kDsbSizeMax = 0x0fffffff,
    kDsbFrequencyMin = 100,
    kDsbFrequencyMax = 200000,
    kDsbVolumeMin = -10000,
    kDsbPanLeft = -10000,
    kDsbPanRight = 10000,
};

/* WAVEFORMATEX (18 bytes; the game passes a 16-byte PCMWAVEFORMAT to CreateSoundBuffer). */
typedef struct __attribute__((packed)) BmxWaveFormat {
    uint16_t formatTag;
    uint16_t channels;
    uint32_t samplesPerSec;
    uint32_t avgBytesPerSec;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint16_t extraSize;
} BmxWaveFormat;

_Static_assert(sizeof(BmxWaveFormat) == 18, "WAVEFORMATEX");

/* DSBUFFERDESC (0x24; DirectX 3's DSBUFFERDESC1 is 0x14, without the 3D algorithm GUID). */
typedef struct BmxDsBufferDesc {
    uint32_t size, flags, bufferBytes, reserved;
    const BmxWaveFormat *format;
    BmxGuid algorithm3d;
} BmxDsBufferDesc;

_Static_assert(sizeof(void *) != 4 || sizeof(BmxDsBufferDesc) == 0x24, "DSBUFFERDESC");

/* DSBCAPS. */
typedef struct BmxDsbCaps {
    uint32_t size, flags, bufferBytes, unlockTransferRate, playCpuOverhead;
} BmxDsbCaps;

typedef struct BmxDsBuffer BmxDsBuffer;
typedef struct BmxDsBufferVtbl {
    BmxHresult(BMX_WINAPI *queryInterface)(BmxDsBuffer *self, const BmxGuid *iid, void **out);
    uint32_t(BMX_WINAPI *addRef)(BmxDsBuffer *self);
    uint32_t(BMX_WINAPI *release)(BmxDsBuffer *self);
    BmxHresult(BMX_WINAPI *getCaps)(BmxDsBuffer *self, BmxDsbCaps *caps);
    BmxHresult(BMX_WINAPI *getCurrentPosition)(BmxDsBuffer *self, uint32_t *play, uint32_t *write);
    BmxHresult(BMX_WINAPI *getFormat)(BmxDsBuffer *self, BmxWaveFormat *format, uint32_t size,
                                      uint32_t *written);
    BmxHresult(BMX_WINAPI *getVolume)(BmxDsBuffer *self, int32_t *volume);
    BmxHresult(BMX_WINAPI *getPan)(BmxDsBuffer *self, int32_t *pan);
    BmxHresult(BMX_WINAPI *getFrequency)(BmxDsBuffer *self, uint32_t *frequency);
    BmxHresult(BMX_WINAPI *getStatus)(BmxDsBuffer *self, uint32_t *status);
    BmxHresult(BMX_WINAPI *initialize)(BmxDsBuffer *self, void *device, const BmxDsBufferDesc *desc);
    BmxHresult(BMX_WINAPI *lock)(BmxDsBuffer *self, uint32_t offset, uint32_t bytes, void **ptr1,
                                 uint32_t *bytes1, void **ptr2, uint32_t *bytes2, uint32_t flags);
    BmxHresult(BMX_WINAPI *play)(BmxDsBuffer *self, uint32_t reserved, uint32_t priority, uint32_t flags);
    BmxHresult(BMX_WINAPI *setCurrentPosition)(BmxDsBuffer *self, uint32_t position);
    BmxHresult(BMX_WINAPI *setFormat)(BmxDsBuffer *self, const BmxWaveFormat *format);
    BmxHresult(BMX_WINAPI *setVolume)(BmxDsBuffer *self, int32_t volume);
    BmxHresult(BMX_WINAPI *setPan)(BmxDsBuffer *self, int32_t pan);
    BmxHresult(BMX_WINAPI *setFrequency)(BmxDsBuffer *self, uint32_t frequency);
    BmxHresult(BMX_WINAPI *stop)(BmxDsBuffer *self);
    BmxHresult(BMX_WINAPI *unlock)(BmxDsBuffer *self, void *ptr1, uint32_t bytes1, void *ptr2,
                                   uint32_t bytes2);
    BmxHresult(BMX_WINAPI *restore)(BmxDsBuffer *self);
} BmxDsBufferVtbl;

_Static_assert(__builtin_offsetof(BmxDsBufferVtbl, lock) == 0x2c * (sizeof(void *) / 4), "vtbl");
_Static_assert(__builtin_offsetof(BmxDsBufferVtbl, restore) == 0x50 * (sizeof(void *) / 4), "vtbl");

struct BmxDsBuffer {
    const BmxDsBufferVtbl *vtbl;
};

typedef struct BmxDs BmxDs;
typedef struct BmxDsVtbl {
    BmxHresult(BMX_WINAPI *queryInterface)(BmxDs *self, const BmxGuid *iid, void **out);
    uint32_t(BMX_WINAPI *addRef)(BmxDs *self);
    uint32_t(BMX_WINAPI *release)(BmxDs *self);
    BmxHresult(BMX_WINAPI *createSoundBuffer)(BmxDs *self, const BmxDsBufferDesc *desc, BmxDsBuffer **out,
                                              void *outer);
    BmxHresult(BMX_WINAPI *getCaps)(BmxDs *self, void *caps);
    BmxHresult(BMX_WINAPI *duplicateSoundBuffer)(BmxDs *self, BmxDsBuffer *original, BmxDsBuffer **out);
    BmxHresult(BMX_WINAPI *setCooperativeLevel)(BmxDs *self, void *window, uint32_t level);
    BmxHresult(BMX_WINAPI *compact)(BmxDs *self);
    BmxHresult(BMX_WINAPI *getSpeakerConfig)(BmxDs *self, uint32_t *config);
    BmxHresult(BMX_WINAPI *setSpeakerConfig)(BmxDs *self, uint32_t config);
    BmxHresult(BMX_WINAPI *initialize)(BmxDs *self, const BmxGuid *device);
} BmxDsVtbl;

_Static_assert(__builtin_offsetof(BmxDsVtbl, setCooperativeLevel) == 0x18 * (sizeof(void *) / 4), "vtbl");

struct BmxDs {
    const BmxDsVtbl *vtbl;
};

typedef BmxHresult(BMX_WINAPI *BmxDirectSoundCreateFn)(const BmxGuid *device, BmxDs **out, void *outer);

#endif
