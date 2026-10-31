/*
 * DirectSound on SDL3 audio (docs/PLATFORM.md 3.2, P9, P10, S7): the stand-ins for KERNEL32's LoadLibraryA /
 * GetProcAddress / FreeLibrary hand winchan.c a "DSOUND.DLL" whose DirectSoundCreate returns our
 * IDirectSound; its buffers are mixed by our own mixer into one SDL audio stream. winchan.c / winstrm.c are
 * unchanged.
 *
 * The mixer runs in 10 ms quanta, like DirectSound's (on Vista and later a software mixer in dsound.dll), at
 * the audio device's rate, pulled by SDL's device callback. Per quantum, every playing buffer advances by
 * exactly quantum * frequency / rate frames (rounded up on the running total, so a 22050 Hz buffer steps 220
 * / 221 frames), the frames are fed through a private SDL_AudioStream per buffer (SDL's resampler, P10-A),
 * scaled by DirectSound's volume and pan and summed.
 *
 * The cursors follow what dsound.dll reports, measured on Windows 11 (tests/platform; docs/PLATFORM.md 4c):
 * after k mixes since Play the write cursor is ceil(k * quantum * frequency / rate) frames past the start,
 * plus the resampler's lookahead (33 frames when the buffer is resampled up, 0 at the device rate); the play
 * cursor is the write cursor minus 30 ms of frames (at most half the buffer), never before the start. Before
 * the first mix both are at the start; a stopped buffer reports its play cursor for both. The frames up to
 * the write cursor are what the mixer has read: the game's writes between the play and the write cursor are
 * not heard, as on Windows. A non-looping buffer stops (status 0, position 0) at the mix that reaches its
 * end.
 *
 * Focus: buffers without DSBCAPS_GLOBALFOCUS / STICKYFOCUS are silent while the game's window does not have
 * the focus (measured: dsound.dll mutes them), but their cursors run on.
 *
 * BMX_MUTE=1: the mixer outputs silence (everything else unchanged). BMX_SOUND_CHECK=1: every call is
 * repeated on the real dsound.dll and compared (soundcheck.c).
 *
 * The override seam (sound.h, the music override): a registered BmxSoundOverride may take over what a buffer
 * plays from its Play on; the mixer then feeds the override's stereo frames for the same frame range into a
 * second resampler instead of the ring's. Cursors, status and the game's calls are not affected.
 */
#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "platform/sound.h"
#include "platform/standins.h"

enum {
    kLockSlots = 16,
    kResamplerLookahead = 33, /* frames, upsampling (measured for 8000..44100 Hz into 48000 Hz) */
    kPrimaryBytes = 32768,    /* what dsound.dll reports for the primary buffer */
    kDsCapsSize = 0x60,
};

static const BmxGuid kIidUnknown = {0, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46}};
static const BmxGuid kIidDirectSound = {
    0x279afa83, 0x4981, 0x11ce, {0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60}};
static const BmxGuid kIidDirectSoundBuffer = {
    0x279afa85, 0x4981, 0x11ce, {0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60}};
static const BmxGuid kDefaultPlayback = {
    0xdef00000, 0x9c6d, 0x47ed, {0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03}};
static const BmxGuid kGuidNull;

/* ---- objects ---- */

typedef struct LockRegion {
    int used;
    uint32_t offset, bytes1, bytes2;
} LockRegion;

/* The bytes and the locks on them: duplicates share both (a lock on the original blocks the duplicate). */
typedef struct Data {
    int refs;
    uint8_t *bytes;
    LockRegion locks[kLockSlots];
} Data;

typedef struct Device Device;

typedef struct Buffer {
    BmxDsBuffer iface; /* first: the object the game holds */
    uint32_t refs;
    Device *device;
    int primary;
    uint32_t flags, align, sizeBytes, sizeFrames;
    BmxWaveFormat format;
    Data *data;
    uint32_t frequency;
    int32_t volume, pan;
    float gainLeft, gainRight;
    int playing, looping;
    uint32_t pos; /* frames: the position of a stopped buffer */
    /* The current play (unwrapped frames; see the header comment). */
    uint64_t base, num, mBase, end, fed;
    uint32_t lookahead;
    SDL_AudioStream *resampler;
    /* The override (sound.h): its token for this play (-1: none), its stereo resampler, and the track
     * position of frame `base` while playing / of `pos` while stopped. */
    int ovrToken;
    SDL_AudioStream *ovrResampler;
    uint64_t trackBase, trackStop;
    BmxSoundCheckBuffer *check;
    struct Buffer *next;
} Buffer;

struct Device {
    BmxDs iface;
    uint32_t refs;  /* COM references */
    int holders;    /* 1 while the COM object lives, + 1 per buffer */
    uint32_t level; /* cooperative level, 0 before SetCooperativeLevel */
    Buffer *primary;
    BmxWaveFormat primaryFormat;
    BmxDs *check;
};

/* ---- the mixer ---- */

static SDL_Mutex *gMutex;
static Buffer *gBuffers;
static SDL_AudioStream *gDevice;
static uint32_t gRate, gQuantum, gOfflineRate;
static int gStarted, gMute, gFocusForced = -1;
static float *gMix, *gTemp;
static uint64_t gQuanta, gUnderruns, gBufferCount;
static const BmxSoundOverride *gOverride;
static int16_t gOvrFrames[2 * 1024];

static uint64_t ceilDiv(uint64_t a, uint64_t b)
{
    return (a + b - 1) / b;
}

uint32_t bmxDsLookahead(uint32_t frequency, uint32_t rate)
{
    if (frequency == rate) {
        return 0;
    }
    if (frequency < rate) {
        return kResamplerLookahead;
    }
    /* Downsampling widens the filter: 96000 into 48000 Hz measured 63. */
    return (uint32_t)ceilDiv((uint64_t)32 * frequency, rate) - 1;
}

uint32_t bmxDsLead(uint32_t frequency, uint32_t rate, uint32_t sizeFrames)
{
    const uint32_t lead = (uint32_t)((uint64_t)frequency * 3 * (rate / 100) / rate);

    return lead < sizeFrames / 2 ? lead : sizeFrames / 2;
}

/*
 * The gain of an attenuation in 1/100 dB as dsound.dll applies it (measured through the session's peak meter,
 * docs/PLATFORM.md 4c): the attenuation is taken in 0.1 dB steps (1..14 -> 0.1 dB, 15..24 -> 0.1 dB, 25..34
 * -> 0.2 dB, ..., 995..1004 -> 9.9 dB, 1005 -> 10 dB) and the gain truncated to 1/65536, which makes
 * everything below about -96 dB silent.
 */
static float attenuationGain(int32_t hundredths)
{
    int32_t steps;

    if (hundredths <= 0) {
        return 1.0f;
    }
    steps = (hundredths + 5) / 10 - 1;
    steps = steps < 1 ? 1 : steps;
    return (float)(floor(65536.0 * pow(10.0, -(double)steps / 200.0)) / 65536.0);
}

void bmxDsGains(int32_t volume, int32_t pan, float *left, float *right)
{
    /* The volume and the pan's attenuation of the far side add before the steps. */
    *left = attenuationGain(-volume + (pan > 0 ? pan : 0));
    *right = attenuationGain(-volume + (pan < 0 ? -pan : 0));
}

static int focused(void)
{
    if (gFocusForced >= 0) {
        return gFocusForced;
    }
    /* SDL's keyboard focus is our game window (SDL wraps its HWND, driver/gpu.c); a pointer read. */
    return SDL_WasInit(SDL_INIT_VIDEO) && SDL_GetKeyboardFocus() != NULL;
}

/* The write cursor of a playing buffer, unwrapped. */
static uint64_t writeAbs(const Buffer *b)
{
    if (b->num == 0) {
        return b->base;
    }
    return b->base + (ceilDiv(b->num, gRate) - b->mBase) + b->lookahead;
}

static void cursorFrames(const Buffer *b, uint32_t *play, uint32_t *write)
{
    if (!b->playing) {
        *play = *write = b->pos;
    } else {
        const uint64_t w = writeAbs(b);
        const uint64_t lead = bmxDsLead(b->frequency, gRate, b->sizeFrames);
        const uint64_t p = w - b->base > lead ? w - lead : b->base;

        *play = (uint32_t)(p % b->sizeFrames);
        *write = (uint32_t)(w % b->sizeFrames);
    }
}

/* Stops a buffer at `pos` frames (mixer lock held). */
static void stopAt(Buffer *b, uint32_t pos)
{
    b->playing = 0;
    b->pos = pos;
    if (b->resampler != NULL) {
        SDL_ClearAudioStream(b->resampler);
    }
    if (b->ovrResampler != NULL) {
        SDL_ClearAudioStream(b->ovrResampler);
    }
}

/* Puts the override's frames for the buffer's frames up to `target` (unwrapped) into its resampler. */
static void feedOverride(Buffer *b, uint64_t target)
{
    const uint32_t chunk = (uint32_t)(sizeof gOvrFrames / sizeof gOvrFrames[0] / 2);

    while (b->fed < target) {
        const uint32_t n = target - b->fed < chunk ? (uint32_t)(target - b->fed) : chunk;

        gOverride->read(b->ovrToken, b->trackBase + (b->fed - b->base), n, gOvrFrames);
        SDL_PutAudioStreamData(b->ovrResampler, gOvrFrames, (int)(n * 4));
        b->fed += n;
    }
}

/* Puts the buffer's frames up to `target` (unwrapped) into its resampler. */
static void feed(Buffer *b, uint64_t target)
{
    if (b->ovrToken >= 0) {
        feedOverride(b, target);
        return;
    }
    while (b->fed < target) {
        const uint32_t at = (uint32_t)(b->fed % b->sizeFrames);
        const uint64_t left = target - b->fed;
        const uint32_t n = left < b->sizeFrames - at ? (uint32_t)left : b->sizeFrames - at;

        SDL_PutAudioStreamData(b->resampler, b->data->bytes + (size_t)at * b->align, (int)(n * b->align));
        b->fed += n;
    }
}

/* One quantum of one playing buffer, added to out (mixer lock held). */
static void mixBuffer(Buffer *b, float *out, int focus)
{
    const int channels = b->ovrToken >= 0 ? 2 : b->format.channels;
    SDL_AudioStream *const resampler = b->ovrToken >= 0 ? b->ovrResampler : b->resampler;
    const int want = (int)(gQuantum * channels * sizeof(float));
    uint64_t target;
    int ending = 0, got;

    b->num += (uint64_t)gQuantum * b->frequency;
    target = writeAbs(b);
    if (target >= b->end) {
        target = b->end;
        ending = 1;
    }
    feed(b, target);
    if (ending) {
        SDL_FlushAudioStream(resampler);
    }
    got = SDL_GetAudioStreamData(resampler, gTemp, want);
    if (got < 0) {
        got = 0;
    }
    if (got < want && !ending) {
        gUnderruns++;
    }
    if (focus || (b->flags & (kDsbGlobalFocus | kDsbStickyFocus)) != 0) {
        const uint32_t frames = (uint32_t)got / (uint32_t)(channels * (int)sizeof(float));

        for (uint32_t i = 0; i < frames; i++) {
            const float l = gTemp[i * channels], r = gTemp[i * channels + channels - 1];

            out[2 * i] += l * b->gainLeft;
            out[2 * i + 1] += r * b->gainRight;
        }
    }
    if (ending) {
        b->trackStop = 0;
        stopAt(b, 0);
    }
}

static void mixQuantum(float *out)
{
    int focus;

    memset(out, 0, sizeof(float) * 2 * gQuantum);
    SDL_LockMutex(gMutex);
    focus = focused();
    for (Buffer *b = gBuffers; b != NULL; b = b->next) {
        if (b->playing) {
            mixBuffer(b, out, focus);
        }
    }
    gQuanta++;
    SDL_UnlockMutex(gMutex);
    for (uint32_t i = 0; i < 2 * gQuantum; i++) {
        out[i] = gMute ? 0.0f : (out[i] > 1.0f ? 1.0f : (out[i] < -1.0f ? -1.0f : out[i]));
    }
}

static void SDLCALL deviceCallback(void *user, SDL_AudioStream *stream, int additional, int total)
{
    (void)user;
    (void)total;
    while (additional > 0) {
        mixQuantum(gMix);
        SDL_PutAudioStreamData(stream, gMix, (int)(sizeof(float) * 2 * gQuantum));
        additional -= (int)(sizeof(float) * 2 * gQuantum);
    }
}

static int openDevice(void)
{
    SDL_AudioSpec spec;
    int frames = 0;
    char text[16];

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        return 0;
    }
    if (!SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &frames) || spec.freq < 8000) {
        spec.freq = 48000;
    }
    gRate = (uint32_t)spec.freq;
    gQuantum = gRate / 100;
    /* Ask for 10 ms device periods, so the callback (and the cursors) step every 10 ms like dsound.dll's. */
    SDL_snprintf(text, sizeof text, "%u", (unsigned)gQuantum);
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, text);
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    gMix = calloc(2 * (size_t)gQuantum, sizeof(float));
    gTemp = calloc(2 * (size_t)gQuantum, sizeof(float));
    gDevice = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, deviceCallback, NULL);
    if (gDevice == NULL) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return 0;
    }
    if (SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(gDevice), &spec, &frames)) {
        bmxLog(
            "sound: SDL audio \"%s\" on \"%s\": mixing at %u Hz in 10 ms quanta of %u frames (device: %d Hz, "
            "%d channels, %d-frame periods)%s\n",
            SDL_GetCurrentAudioDriver(), SDL_GetAudioDeviceName(SDL_GetAudioStreamDevice(gDevice)),
            (unsigned)gRate, (unsigned)gQuantum, spec.freq, spec.channels, frames,
            gMute ? "; BMX_MUTE: output silent" : "");
    }
    SDL_ResumeAudioStreamDevice(gDevice);
    return 1;
}

/* At the first DirectSoundCreate: the audio device (or the offline mixer of the tests). */
static void mixerStart(void)
{
    const char *mute = SDL_getenv("BMX_MUTE");

    if (gStarted) {
        return;
    }
    gStarted = 1;
    gMutex = SDL_CreateMutex();
    gMute = mute != NULL && *mute != 0 && *mute != '0';
    if (gOfflineRate != 0) {
        gRate = gOfflineRate;
        gQuantum = gRate / 100;
        gMix = calloc(2 * (size_t)gQuantum, sizeof(float));
        gTemp = calloc(2 * (size_t)gQuantum, sizeof(float));
        return;
    }
    if (!openDevice()) {
        bmxLog("sound: no audio device (%s); using SDL's dummy driver (silent, cursors run)\n",
               SDL_GetError());
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        free(gMix);
        free(gTemp);
        if (!openDevice()) {
            bmxLog("sound: the dummy driver failed too (%s): cursors stand still\n", SDL_GetError());
            gRate = 48000;
            gQuantum = gRate / 100;
            gMix = calloc(2 * (size_t)gQuantum, sizeof(float));
            gTemp = calloc(2 * (size_t)gQuantum, sizeof(float));
        }
    }
}

void bmxSoundUseOffline(uint32_t rate)
{
    gOfflineRate = rate;
}

uint32_t bmxSoundRate(void)
{
    return gRate;
}

uint32_t bmxSoundQuantum(void)
{
    return gQuantum;
}

void bmxSoundMixOffline(float *out)
{
    mixQuantum(out != NULL ? out : gMix);
}

void bmxSoundForceFocus(int focus)
{
    gFocusForced = focus;
}

void bmxSoundSetOverride(const BmxSoundOverride *override)
{
    if (gMutex != NULL) {
        SDL_LockMutex(gMutex);
    }
    gOverride = override;
    if (gMutex != NULL) {
        SDL_UnlockMutex(gMutex);
    }
}

void bmxSoundShutdown(void)
{
    if (gDevice != NULL) {
        SDL_DestroyAudioStream(gDevice); /* waits for the callback */
        gDevice = NULL;
    }
    if (gOverride != NULL && gOverride->shutdown != NULL) {
        gOverride->shutdown();
    }
    if (gStarted) {
        bmxLog("sound: %llu quanta mixed (%.1f s), %llu buffers created, %llu resampler underruns\n",
               (unsigned long long)gQuanta, (double)gQuanta / 100.0, (unsigned long long)gBufferCount,
               (unsigned long long)gUnderruns);
    }
    bmxSoundCheckSummary();
}

void bmxSoundReset(void)
{
    bmxSoundShutdown();
    while (gBuffers != NULL) {
        Buffer *const b = gBuffers;

        gBuffers = b->next;
        SDL_DestroyAudioStream(b->resampler);
        SDL_DestroyAudioStream(b->ovrResampler);
    }
    if (gMutex != NULL) {
        SDL_DestroyMutex(gMutex);
    }
    free(gMix);
    free(gTemp);
    gMix = gTemp = NULL;
    gMutex = NULL;
    gRate = gQuantum = 0;
    gStarted = 0;
    gFocusForced = -1;
    gQuanta = gUnderruns = gBufferCount = 0;
    bmxSoundCheckReset();
}

/* ---- IDirectSoundBuffer ---- */

static int sameGuid(const BmxGuid *a, const BmxGuid *b)
{
    return memcmp(a, b, sizeof *a) == 0;
}

static void deviceUnhold(Device *d)
{
    if (--d->holders == 0) {
        free(d);
    }
}

static BmxHresult BMX_WINAPI bufQueryInterface(BmxDsBuffer *self, const BmxGuid *iid, void **out)
{
    if (out == NULL) {
        return kDsErrInvalidParam;
    }
    if (iid != NULL && (sameGuid(iid, &kIidUnknown) || sameGuid(iid, &kIidDirectSoundBuffer))) {
        self->vtbl->addRef(self);
        *out = self;
        return kDsOk;
    }
    *out = NULL;
    return kDsErrNoInterface;
}

static uint32_t BMX_WINAPI bufAddRef(BmxDsBuffer *self)
{
    Buffer *const b = (Buffer *)self;

    SDL_LockMutex(gMutex);
    b->refs++;
    SDL_UnlockMutex(gMutex);
    return b->refs;
}

static uint32_t BMX_WINAPI bufRelease(BmxDsBuffer *self)
{
    Buffer *const b = (Buffer *)self;
    uint32_t refs;

    SDL_LockMutex(gMutex);
    refs = --b->refs;
    if (refs == 0) {
        for (Buffer **p = &gBuffers; *p != NULL; p = &(*p)->next) {
            if (*p == b) {
                *p = b->next;
                break;
            }
        }
        if (b->primary) {
            b->device->primary = NULL;
        }
    }
    SDL_UnlockMutex(gMutex);
    bmxSoundCheckSimple(b->check, "Release", 0, (BmxHresult)refs);
    if (refs == 0) {
        bmxSoundCheckRelease(b->check);
        SDL_DestroyAudioStream(b->resampler);
        SDL_DestroyAudioStream(b->ovrResampler);
        if (b->data != NULL && --b->data->refs == 0) {
            free(b->data->bytes);
            free(b->data);
        }
        deviceUnhold(b->device);
        free(b);
    }
    return refs;
}

static BmxHresult BMX_WINAPI bufGetCaps(BmxDsBuffer *self, BmxDsbCaps *caps)
{
    const Buffer *const b = (const Buffer *)self;

    if (caps == NULL || caps->size != sizeof *caps) {
        return kDsErrInvalidParam;
    }
    caps->flags = b->flags;
    caps->bufferBytes = b->sizeBytes;
    caps->unlockTransferRate = 0;
    caps->playCpuOverhead = 0;
    return kDsOk;
}

static BmxHresult BMX_WINAPI bufGetCurrentPosition(BmxDsBuffer *self, uint32_t *play, uint32_t *write)
{
    Buffer *const b = (Buffer *)self;
    uint32_t p, w;

    if (play == NULL && write == NULL) {
        return kDsErrInvalidParam;
    }
    if (b->primary) {
        if (b->device->level < kDsCoopWritePrimary) {
            bmxSoundCheckPosition(b->check, kDsErrPrioLevelNeeded, 0, 0);
            return kDsErrPrioLevelNeeded;
        }
        p = w = 0;
    } else {
        SDL_LockMutex(gMutex);
        cursorFrames(b, &p, &w);
        SDL_UnlockMutex(gMutex);
        p *= b->align;
        w *= b->align;
    }
    if (play != NULL) {
        *play = p;
    }
    if (write != NULL) {
        *write = w;
    }
    bmxSoundCheckPosition(b->check, kDsOk, p, w);
    return kDsOk;
}

static BmxHresult BMX_WINAPI bufGetFormat(BmxDsBuffer *self, BmxWaveFormat *format, uint32_t size,
                                          uint32_t *written)
{
    const Buffer *const b = (const Buffer *)self;
    const BmxWaveFormat *const f = b->primary ? &b->device->primaryFormat : &b->format;

    if (format == NULL) {
        if (written == NULL) {
            return kDsErrInvalidParam;
        }
        *written = sizeof *f;
        return kDsOk;
    }
    if (size < sizeof *f) {
        return kDsErrInvalidParam;
    }
    memcpy(format, f, sizeof *f);
    if (written != NULL) {
        *written = sizeof *f;
    }
    return kDsOk;
}

static BmxHresult BMX_WINAPI bufGetVolume(BmxDsBuffer *self, int32_t *volume)
{
    const Buffer *const b = (const Buffer *)self;

    if (volume == NULL) {
        return kDsErrInvalidParam;
    }
    if ((b->flags & kDsbCtrlVolume) == 0) {
        return kDsErrControlUnavail;
    }
    *volume = b->volume;
    return kDsOk;
}

static BmxHresult BMX_WINAPI bufGetPan(BmxDsBuffer *self, int32_t *pan)
{
    const Buffer *const b = (const Buffer *)self;

    if (pan == NULL) {
        return kDsErrInvalidParam;
    }
    if ((b->flags & kDsbCtrlPan) == 0) {
        return kDsErrControlUnavail;
    }
    *pan = b->pan;
    return kDsOk;
}

static BmxHresult BMX_WINAPI bufGetFrequency(BmxDsBuffer *self, uint32_t *frequency)
{
    const Buffer *const b = (const Buffer *)self;

    if (frequency == NULL) {
        return kDsErrInvalidParam;
    }
    if ((b->flags & kDsbCtrlFrequency) == 0) {
        return kDsErrControlUnavail;
    }
    *frequency = b->frequency;
    return kDsOk;
}

static BmxHresult BMX_WINAPI bufGetStatus(BmxDsBuffer *self, uint32_t *status)
{
    Buffer *const b = (Buffer *)self;
    uint32_t s;

    if (status == NULL) {
        return kDsErrInvalidParam;
    }
    SDL_LockMutex(gMutex);
    s = b->playing ? (kDsbStatusPlaying | (b->looping ? kDsbStatusLooping : 0)) : 0;
    SDL_UnlockMutex(gMutex);
    *status = s;
    bmxSoundCheckStatus(b->check, kDsOk, s);
    return kDsOk;
}

static BmxHresult BMX_WINAPI bufInitialize(BmxDsBuffer *self, void *device, const BmxDsBufferDesc *desc)
{
    (void)self;
    return device == NULL || desc == NULL ? kDsErrInvalidParam : kDsErrAlreadyInitialized;
}

static int overlaps(uint32_t a, uint32_t an, uint32_t b, uint32_t bn)
{
    return an != 0 && bn != 0 && a < b + bn && b < a + an;
}

static BmxHresult lockRegion(Buffer *b, uint32_t offset, uint32_t bytes, void **ptr1, uint32_t *bytes1,
                             void **ptr2, uint32_t *bytes2, uint32_t flags)
{
    uint32_t n1, n2;
    int slot = -1;

    if (b->primary) {
        return b->device->level < kDsCoopWritePrimary ? kDsErrPrioLevelNeeded : kDsErrUnsupported;
    }
    if ((flags & kDsbLockFromWriteCursor) != 0) {
        uint32_t p, w;

        cursorFrames(b, &p, &w);
        offset = w * b->align;
    }
    if ((flags & kDsbLockEntireBuffer) != 0) {
        bytes = b->sizeBytes;
    }
    if (bytes == 0 || bytes > b->sizeBytes || offset >= b->sizeBytes) {
        return kDsErrInvalidParam;
    }
    n1 = bytes < b->sizeBytes - offset ? bytes : b->sizeBytes - offset;
    n2 = bytes - n1; /* locked even when the caller passed no second pointer */
    for (int i = 0; i < kLockSlots; i++) {
        const LockRegion *const l = &b->data->locks[i];

        if (!l->used) {
            slot = slot < 0 ? i : slot;
        } else if (overlaps(offset, n1, l->offset, l->bytes1) || overlaps(offset, n1, 0, l->bytes2) ||
                   overlaps(0, n2, l->offset, l->bytes1) || overlaps(0, n2, 0, l->bytes2)) {
            return kDsErrInvalidParam;
        }
    }
    if (slot < 0) {
        return kDsErrInvalidParam;
    }
    b->data->locks[slot] = (LockRegion){1, offset, n1, n2};
    *ptr1 = b->data->bytes + offset;
    *bytes1 = n1;
    if (ptr2 != NULL) {
        *ptr2 = n2 != 0 ? b->data->bytes : NULL;
        *bytes2 = n2;
    }
    return kDsOk;
}

static BmxHresult BMX_WINAPI bufLock(BmxDsBuffer *self, uint32_t offset, uint32_t bytes, void **ptr1,
                                     uint32_t *bytes1, void **ptr2, uint32_t *bytes2, uint32_t flags)
{
    Buffer *const b = (Buffer *)self;
    BmxHresult hr;

    if (ptr1 == NULL || bytes1 == NULL || (ptr2 != NULL && bytes2 == NULL)) {
        return kDsErrInvalidParam;
    }
    *ptr1 = NULL;
    *bytes1 = 0;
    if (ptr2 != NULL) {
        *ptr2 = NULL;
        *bytes2 = 0;
    }
    SDL_LockMutex(gMutex);
    hr = lockRegion(b, offset, bytes, ptr1, bytes1, ptr2, bytes2, flags);
    SDL_UnlockMutex(gMutex);
    if (b->check != NULL) {
        const uint32_t off1 = hr == kDsOk ? (uint32_t)((uint8_t *)*ptr1 - b->data->bytes) : 0;

        bmxSoundCheckLock(b->check, offset, bytes, flags, hr, off1, *bytes1, ptr2 != NULL && *ptr2 != NULL,
                          ptr2 != NULL ? *bytes2 : 0);
    }
    return hr;
}

/* Play from stopped: the override's token for this play (mixer lock held). */
static void attachOverride(Buffer *b)
{
    b->trackBase = b->trackStop;
    b->ovrToken = gOverride != NULL ? gOverride->attach(&b->iface, b->trackBase) : -1;
    if (b->ovrToken >= 0 && b->ovrResampler == NULL) {
        const SDL_AudioSpec src = {SDL_AUDIO_S16LE, 2, (int)b->frequency};
        const SDL_AudioSpec dst = {SDL_AUDIO_F32, 2, (int)gRate};

        b->ovrResampler = SDL_CreateAudioStream(&src, &dst);
        if (b->ovrResampler == NULL) {
            b->ovrToken = -1;
        }
    }
    if (b->ovrResampler != NULL) {
        SDL_ClearAudioStream(b->ovrResampler);
    }
}

static BmxHresult BMX_WINAPI bufPlay(BmxDsBuffer *self, uint32_t reserved, uint32_t priority, uint32_t flags)
{
    Buffer *const b = (Buffer *)self;
    BmxHresult hr = kDsOk;

    SDL_LockMutex(gMutex);
    if (reserved != 0 || priority != 0 || (flags & ~(uint32_t)kDsbPlayLooping) != 0) {
        hr = kDsErrInvalidParam;
    } else if (b->primary) {
        if ((flags & kDsbPlayLooping) == 0) {
            hr = kDsErrInvalidParam;
        } else {
            b->playing = b->looping = 1;
        }
    } else if (b->playing) {
        /* Only the looping flag changes; a non-looping play ends at the end of the current lap. */
        b->looping = (flags & kDsbPlayLooping) != 0;
        b->end = b->looping ? UINT64_MAX : (writeAbs(b) / b->sizeFrames + 1) * b->sizeFrames;
    } else {
        b->playing = 1;
        b->looping = (flags & kDsbPlayLooping) != 0;
        b->base = b->fed = b->pos;
        b->num = b->mBase = 0;
        b->lookahead = bmxDsLookahead(b->frequency, gRate);
        b->end = b->looping ? UINT64_MAX : b->sizeFrames;
        SDL_ClearAudioStream(b->resampler);
        attachOverride(b);
    }
    SDL_UnlockMutex(gMutex);
    bmxSoundCheckSimple(b->check, "Play", flags, hr);
    return hr;
}

static BmxHresult BMX_WINAPI bufSetCurrentPosition(BmxDsBuffer *self, uint32_t position)
{
    Buffer *const b = (Buffer *)self;
    BmxHresult hr = kDsOk;

    SDL_LockMutex(gMutex);
    if (b->primary) {
        hr = kDsErrInvalidCall;
    } else if (position >= b->sizeBytes) {
        hr = kDsErrInvalidParam;
    } else if (!b->playing) {
        b->pos = position / b->align;
        b->trackStop = b->pos;
    } else {
        /* While playing, the resampler keeps what it has (its lookahead is not counted again). */
        b->base = b->fed = position / b->align;
        b->trackBase = b->base;
        b->mBase = ceilDiv(b->num, gRate);
        b->lookahead = 0;
        if (!b->looping) {
            b->end = b->sizeFrames;
        }
    }
    SDL_UnlockMutex(gMutex);
    bmxSoundCheckSimple(b->check, "SetCurrentPosition", position, hr);
    return hr;
}

static int validPcm(const BmxWaveFormat *f)
{
    return f->blockAlign == f->channels * f->bitsPerSample / 8 &&
           f->avgBytesPerSec == f->samplesPerSec * f->blockAlign && f->blockAlign != 0;
}

static BmxHresult BMX_WINAPI bufSetFormat(BmxDsBuffer *self, const BmxWaveFormat *format)
{
    Buffer *const b = (Buffer *)self;
    BmxHresult hr = kDsOk;

    if (!b->primary) {
        hr = kDsErrInvalidCall;
    } else if (b->device->level < kDsCoopPriority) {
        hr = kDsErrPrioLevelNeeded;
    } else if (format == NULL || !validPcm(format)) {
        hr = kDsErrInvalidParam;
    } else {
        /* Kept for GetFormat; dsound.dll mixes at the device's format whatever is set (so does ours). */
        memcpy(&b->device->primaryFormat, format, 16);
        b->device->primaryFormat.extraSize = 0;
    }
    bmxSoundCheckSetFormat(b->check, format, hr);
    return hr;
}

static BmxHresult BMX_WINAPI bufSetVolume(BmxDsBuffer *self, int32_t volume)
{
    Buffer *const b = (Buffer *)self;
    BmxHresult hr = kDsOk;

    if ((b->flags & kDsbCtrlVolume) == 0) {
        hr = kDsErrControlUnavail;
    } else if (volume > 0 || volume < kDsbVolumeMin) {
        hr = kDsErrInvalidParam;
    } else {
        SDL_LockMutex(gMutex);
        b->volume = volume;
        bmxDsGains(b->volume, b->pan, &b->gainLeft, &b->gainRight);
        SDL_UnlockMutex(gMutex);
    }
    bmxSoundCheckSimple(b->check, "SetVolume", (uint32_t)volume, hr);
    return hr;
}

static BmxHresult BMX_WINAPI bufSetPan(BmxDsBuffer *self, int32_t pan)
{
    Buffer *const b = (Buffer *)self;
    BmxHresult hr = kDsOk;

    if ((b->flags & kDsbCtrlPan) == 0) {
        hr = kDsErrControlUnavail;
    } else if (pan > kDsbPanRight || pan < kDsbPanLeft) {
        hr = kDsErrInvalidParam;
    } else {
        SDL_LockMutex(gMutex);
        b->pan = pan;
        bmxDsGains(b->volume, b->pan, &b->gainLeft, &b->gainRight);
        SDL_UnlockMutex(gMutex);
    }
    bmxSoundCheckSimple(b->check, "SetPan", (uint32_t)pan, hr);
    return hr;
}

static BmxHresult BMX_WINAPI bufSetFrequency(BmxDsBuffer *self, uint32_t frequency)
{
    Buffer *const b = (Buffer *)self;
    BmxHresult hr = kDsOk;

    if ((b->flags & kDsbCtrlFrequency) == 0) {
        hr = kDsErrControlUnavail;
    } else if (frequency != 0 && (frequency < kDsbFrequencyMin || frequency > kDsbFrequencyMax)) {
        hr = kDsErrInvalidParam;
    } else {
        const SDL_AudioSpec src = {b->format.bitsPerSample == 8 ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE,
                                   b->format.channels,
                                   (int)(frequency != 0 ? frequency : b->format.samplesPerSec)};

        const SDL_AudioSpec stereo = {SDL_AUDIO_S16LE, 2, src.freq};

        SDL_LockMutex(gMutex);
        b->frequency = (uint32_t)src.freq;
        SDL_SetAudioStreamFormat(b->resampler, &src, NULL);
        if (b->ovrResampler != NULL) {
            SDL_SetAudioStreamFormat(b->ovrResampler, &stereo, NULL);
        }
        SDL_UnlockMutex(gMutex);
    }
    bmxSoundCheckSimple(b->check, "SetFrequency", frequency, hr);
    return hr;
}

static BmxHresult BMX_WINAPI bufStop(BmxDsBuffer *self)
{
    Buffer *const b = (Buffer *)self;

    SDL_LockMutex(gMutex);
    if (b->primary) {
        b->playing = 0;
    } else if (b->playing) {
        const uint64_t w = writeAbs(b);
        const uint64_t lead = bmxDsLead(b->frequency, gRate, b->sizeFrames);
        uint32_t p, unused;

        /* The track position of the play cursor (cursorFrames, unwrapped). */
        b->trackStop = b->trackBase + (w - b->base > lead ? w - lead - b->base : 0);
        cursorFrames(b, &p, &unused);
        stopAt(b, p);
    }
    SDL_UnlockMutex(gMutex);
    bmxSoundCheckSimple(b->check, "Stop", 0, kDsOk);
    return kDsOk;
}

static BmxHresult BMX_WINAPI bufUnlock(BmxDsBuffer *self, void *ptr1, uint32_t bytes1, void *ptr2,
                                       uint32_t bytes2)
{
    Buffer *const b = (Buffer *)self;
    BmxHresult hr = kDsErrInvalidParam;
    uint32_t off1 = UINT32_MAX;

    SDL_LockMutex(gMutex);
    if (ptr1 == NULL) {
        hr = bytes1 == 0 && (ptr2 == NULL || bytes2 == 0) ? kDsOk : kDsErrInvalidParam;
    } else if (!b->primary) {
        /* dsound.dll matches the lock by its first pointer only; the sizes are not checked. A second pointer
         * must be the lock's second part (else the call fails, but the lock is gone all the same); without
         * one, the second part stays locked. */
        off1 = (uint32_t)((uint8_t *)ptr1 - b->data->bytes);
        for (int i = 0; i < kLockSlots; i++) {
            LockRegion *const l = &b->data->locks[i];

            if (l->used && l->offset == off1) {
                if (ptr2 == NULL) {
                    *l = (LockRegion){l->bytes2 != 0, 0, l->bytes2, 0};
                    hr = kDsOk;
                } else {
                    l->used = 0;
                    hr = l->bytes2 != 0 && ptr2 == b->data->bytes ? kDsOk : kDsErrInvalidParam;
                }
                break;
            }
        }
    }
    SDL_UnlockMutex(gMutex);
    bmxSoundCheckUnlock(b->check, off1, bytes1, ptr2 != NULL, bytes2, hr);
    return hr;
}

static BmxHresult BMX_WINAPI bufRestore(BmxDsBuffer *self)
{
    Buffer *const b = (Buffer *)self;

    bmxSoundCheckSimple(b->check, "Restore", 0, kDsOk);
    return kDsOk; /* never lost */
}

static const BmxDsBufferVtbl kBufferVtbl = {
    .queryInterface = bufQueryInterface,
    .addRef = bufAddRef,
    .release = bufRelease,
    .getCaps = bufGetCaps,
    .getCurrentPosition = bufGetCurrentPosition,
    .getFormat = bufGetFormat,
    .getVolume = bufGetVolume,
    .getPan = bufGetPan,
    .getFrequency = bufGetFrequency,
    .getStatus = bufGetStatus,
    .initialize = bufInitialize,
    .lock = bufLock,
    .play = bufPlay,
    .setCurrentPosition = bufSetCurrentPosition,
    .setFormat = bufSetFormat,
    .setVolume = bufSetVolume,
    .setPan = bufSetPan,
    .setFrequency = bufSetFrequency,
    .stop = bufStop,
    .unlock = bufUnlock,
    .restore = bufRestore,
};

/* ---- IDirectSound ---- */

static Buffer *newBuffer(Device *d, uint32_t flags)
{
    Buffer *const b = calloc(1, sizeof *b);

    if (b == NULL) {
        return NULL;
    }
    b->iface.vtbl = &kBufferVtbl;
    b->refs = 1;
    b->device = d;
    b->flags = flags;
    b->ovrToken = -1;
    d->holders++;
    return b;
}

/* A secondary buffer of this format and size: the resampler, and in the mixer's list. */
static BmxHresult setUpSecondary(Buffer *b, const BmxWaveFormat *f, Data *data)
{
    const SDL_AudioSpec src = {f->bitsPerSample == 8 ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE, f->channels,
                               (int)f->samplesPerSec};
    const SDL_AudioSpec dst = {SDL_AUDIO_F32, f->channels, (int)gRate};

    memcpy(&b->format, f, 16);
    b->format.extraSize = 0;
    b->align = f->blockAlign;
    b->sizeFrames = b->sizeBytes / b->align;
    b->frequency = f->samplesPerSec;
    bmxDsGains(0, 0, &b->gainLeft, &b->gainRight);
    b->resampler = SDL_CreateAudioStream(&src, &dst);
    if (b->resampler == NULL) {
        return kDsErrOutOfMemory;
    }
    if (data == NULL) {
        data = calloc(1, sizeof *data);
        if (data == NULL) {
            return kDsErrOutOfMemory;
        }
        data->bytes = calloc(b->sizeBytes, 1);
        if (data->bytes == NULL) {
            free(data);
            return kDsErrOutOfMemory;
        }
    }
    data->refs++;
    b->data = data;
    SDL_LockMutex(gMutex);
    b->next = gBuffers;
    gBuffers = b;
    gBufferCount++;
    SDL_UnlockMutex(gMutex);
    return kDsOk;
}

static void freeUnlisted(Buffer *b)
{
    SDL_DestroyAudioStream(b->resampler);
    b->device->holders--; /* called from a method of the device, which holds it */
    free(b);
}

static BmxHresult createBuffer(Device *d, const BmxDsBufferDesc *desc, BmxDsBuffer **out)
{
    const BmxWaveFormat *f = desc->format;
    Buffer *b;
    BmxHresult hr;

    if (desc->size != sizeof *desc && desc->size != 0x14) {
        return kDsErrInvalidParam;
    }
    if ((desc->flags & kDsbPrimary) != 0) {
        if (desc->bufferBytes != 0 || f != NULL) {
            return kDsErrInvalidParam;
        }
        if (d->primary != NULL) {
            d->primary->iface.vtbl->addRef(&d->primary->iface);
            *out = &d->primary->iface;
            return kDsOk;
        }
        b = newBuffer(d, (desc->flags & ~(uint32_t)kDsbLocHardware) | kDsbLocSoftware);
        if (b == NULL) {
            return kDsErrOutOfMemory;
        }
        b->primary = 1;
        b->sizeBytes = kPrimaryBytes;
        b->align = 1;
        d->primary = b;
        *out = &b->iface;
        return kDsOk;
    }
    if (f == NULL || desc->bufferBytes < kDsbSizeMin || desc->bufferBytes > kDsbSizeMax) {
        return kDsErrInvalidParam;
    }
    if (f->formatTag != 1 || f->channels < 1 || f->channels > 2 || !validPcm(f) ||
        f->samplesPerSec < kDsbFrequencyMin || f->samplesPerSec > kDsbFrequencyMax) {
        return kDsErrInvalidParam;
    }
    if (f->bitsPerSample != 8 && f->bitsPerSample != 16) {
        return kDsErrBadFormat; /* dsound.dll also takes 24 / 32 bit; the game uses 8 and 16 */
    }
    b = newBuffer(d, (desc->flags & ~(uint32_t)kDsbLocHardware) | kDsbLocSoftware);
    if (b == NULL) {
        return kDsErrOutOfMemory;
    }
    /* The size is rounded up to whole frames. */
    b->sizeBytes = (uint32_t)ceilDiv(desc->bufferBytes, f->blockAlign) * f->blockAlign;
    hr = setUpSecondary(b, f, NULL);
    if (hr != kDsOk) {
        freeUnlisted(b);
        return hr;
    }
    *out = &b->iface;
    return kDsOk;
}

static BmxHresult BMX_WINAPI devQueryInterface(BmxDs *self, const BmxGuid *iid, void **out)
{
    if (out == NULL) {
        return kDsErrInvalidParam;
    }
    if (iid != NULL && (sameGuid(iid, &kIidUnknown) || sameGuid(iid, &kIidDirectSound))) {
        self->vtbl->addRef(self);
        *out = self;
        return kDsOk;
    }
    *out = NULL;
    return kDsErrNoInterface;
}

static uint32_t BMX_WINAPI devAddRef(BmxDs *self)
{
    return ++((Device *)self)->refs;
}

static uint32_t BMX_WINAPI devRelease(BmxDs *self)
{
    Device *const d = (Device *)self;
    const uint32_t refs = --d->refs;

    if (refs == 0) {
        bmxSoundCheckReleaseDevice(d->check);
        d->check = NULL;
        deviceUnhold(d);
    }
    return refs;
}

static BmxHresult BMX_WINAPI devCreateSoundBuffer(BmxDs *self, const BmxDsBufferDesc *desc, BmxDsBuffer **out,
                                                  void *outer)
{
    Device *const d = (Device *)self;
    BmxHresult hr;

    BmxDsBuffer *made = NULL;

    if (out == NULL) {
        return kDsErrInvalidParam;
    }
    if (outer != NULL) {
        hr = kDsErrNoAggregation;
    } else if (desc == NULL) {
        hr = kDsErrInvalidParam;
    } else {
        hr = createBuffer(d, desc, &made);
    }
    if (d->check != NULL && desc != NULL) {
        BmxSoundCheckBuffer *const c = bmxSoundCheckCreateBuffer(d->check, desc, hr, made);

        if (made != NULL && ((Buffer *)made)->check == NULL) {
            ((Buffer *)made)->check = c;
        }
    }
    if (hr == kDsOk) {
        *out = made; /* dsound.dll leaves it alone on a failure */
    }
    return hr;
}

static BmxHresult BMX_WINAPI devGetCaps(BmxDs *self, void *caps)
{
    uint32_t *const c = caps;

    (void)self;
    if (c == NULL || c[0] != kDsCapsSize) {
        return kDsErrInvalidParam;
    }
    memset(c + 1, 0, kDsCapsSize - 4);
    c[1] = 0xf1f;  /* PRIMARY/SECONDARY MONO/STEREO/8BIT/16BIT, CONTINUOUSRATE */
    c[2] = 100;    /* dwMinSecondarySampleRate */
    c[3] = 200000; /* dwMaxSecondarySampleRate */
    return kDsOk;
}

static BmxHresult BMX_WINAPI devDuplicateSoundBuffer(BmxDs *self, BmxDsBuffer *original, BmxDsBuffer **out)
{
    Device *const d = (Device *)self;
    const Buffer *const o = (const Buffer *)original;
    Buffer *b;
    BmxHresult hr;

    if (out == NULL || original == NULL) {
        return kDsErrInvalidParam;
    }
    *out = NULL;
    if (o->primary) {
        return kDsErrInvalidCall;
    }
    b = newBuffer(d, o->flags);
    if (b == NULL) {
        return kDsErrOutOfMemory;
    }
    b->sizeBytes = o->sizeBytes;
    hr = setUpSecondary(b, &o->format, o->data);
    if (hr != kDsOk) {
        freeUnlisted(b);
        return hr;
    }
    b->volume = o->volume;
    b->pan = o->pan;
    b->gainLeft = o->gainLeft;
    b->gainRight = o->gainRight;
    if (o->frequency != o->format.samplesPerSec) {
        bufSetFrequency(&b->iface, o->frequency);
    }
    if (d->check != NULL) {
        b->check = bmxSoundCheckDuplicate(d->check, o->check, kDsOk, &b->iface);
    }
    *out = &b->iface;
    return kDsOk;
}

static BmxHresult BMX_WINAPI devSetCooperativeLevel(BmxDs *self, void *window, uint32_t level)
{
    Device *const d = (Device *)self;
    BmxHresult hr = kDsOk;

    if (window == NULL || level < kDsCoopNormal || level > kDsCoopWritePrimary) {
        hr = kDsErrInvalidParam;
    } else {
        d->level = level;
    }
    bmxSoundCheckCooperative(d->check, window, level, hr);
    return hr;
}

static BmxHresult BMX_WINAPI devCompact(BmxDs *self)
{
    (void)self;
    return kDsOk;
}

static BmxHresult BMX_WINAPI devGetSpeakerConfig(BmxDs *self, uint32_t *config)
{
    (void)self;
    if (config == NULL) {
        return kDsErrInvalidParam;
    }
    *config = 4; /* DSSPEAKER_STEREO */
    return kDsOk;
}

static BmxHresult BMX_WINAPI devSetSpeakerConfig(BmxDs *self, uint32_t config)
{
    (void)self;
    (void)config;
    return kDsOk;
}

static BmxHresult BMX_WINAPI devInitialize(BmxDs *self, const BmxGuid *device)
{
    (void)self;
    (void)device;
    return kDsErrAlreadyInitialized;
}

static const BmxDsVtbl kDeviceVtbl = {
    .queryInterface = devQueryInterface,
    .addRef = devAddRef,
    .release = devRelease,
    .createSoundBuffer = devCreateSoundBuffer,
    .getCaps = devGetCaps,
    .duplicateSoundBuffer = devDuplicateSoundBuffer,
    .setCooperativeLevel = devSetCooperativeLevel,
    .compact = devCompact,
    .getSpeakerConfig = devGetSpeakerConfig,
    .setSpeakerConfig = devSetSpeakerConfig,
    .initialize = devInitialize,
};

int32_t BMX_WINAPI bmxStandDirectSoundCreate(const void *guid, void **out, void *outer)
{
    Device *d;

    if (out == NULL) {
        return kDsErrInvalidParam;
    }
    *out = NULL;
    if (outer != NULL) {
        return kDsErrNoAggregation;
    }
    if (guid != NULL && !sameGuid(guid, &kGuidNull) && !sameGuid(guid, &kDefaultPlayback)) {
        return kDsErrNoDriver;
    }
    mixerStart();
    d = calloc(1, sizeof *d);
    if (d == NULL) {
        return kDsErrOutOfMemory;
    }
    d->iface.vtbl = &kDeviceVtbl;
    d->refs = 1;
    d->holders = 1;
    /* dsound.dll's primary buffer starts at 22050 Hz, 8 bit, stereo. */
    d->primaryFormat = (BmxWaveFormat){1, 2, 22050, 44100, 2, 8, 0};
    if (bmxSoundCheckEnabled()) {
        d->check = bmxSoundCheckCreate();
    }
    *out = &d->iface;
    return kDsOk;
}

/* ---- KERNEL32: the DLL the sound code loads ---- */

static const char gDsoundModule[] = "DSOUND.DLL (stand-in)"; /* its address is the module handle */

void *BMX_WINAPI bmxStandLoadLibraryA(const char *name)
{
    if (name != NULL && (SDL_strcasecmp(name, "DSOUND.DLL") == 0 || SDL_strcasecmp(name, "DSOUND") == 0)) {
        return (void *)gDsoundModule;
    }
    bmxLog("platform: LoadLibraryA(\"%s\") refused (only DSOUND.DLL is provided)\n",
           name != NULL ? name : "");
    return NULL;
}

void *BMX_WINAPI bmxStandGetProcAddress(void *module, const char *name)
{
    if (module == gDsoundModule && (uintptr_t)name > 0xffff && strcmp(name, "DirectSoundCreate") == 0) {
        return (void *)bmxStandDirectSoundCreate;
    }
    bmxLog("platform: GetProcAddress(%p, \"%s\") refused\n", module, (uintptr_t)name > 0xffff ? name : "#");
    return NULL;
}

int32_t BMX_WINAPI bmxStandFreeLibrary(void *module)
{
    return module == gDsoundModule;
}
