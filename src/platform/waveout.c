/*
 * WINMM waveOut stand-ins on SDL3 audio (docs/PLATFORM.md P14, slice S9): the movies' sound (zge/zmovie.c).
 * The game opens the wave mapper with CALLBACK_WINDOW, prepares 16 buffers of 32 KB, writes them, and refills
 * one each time MM_WOM_DONE (0x3bd) reaches zrwPlayMovie's message loop; Pause / Restart around the start,
 * Reset / Unprepare / Close at the end.
 *
 * One SDL audio stream per open device, in the file's own format (SDL converts and resamples to the device).
 * Its get-callback (SDL's audio thread) hands the queued buffers to SDL in order; a buffer whose last byte
 * has gone is marked WHDR_DONE and MM_WOM_DONE is posted to the window, as winmm does. MM_WOM_OPEN /
 * MM_WOM_CLOSE are posted too, into the game's message queue (window.c, slice S10).
 *
 * BMX_MUTE=1: the bytes are consumed at the same pace but SDL gets silence. Each close logs the bytes
 * written, played and dropped by a reset, with FNV-1a 64 hashes (to compare with what AVIStreamRead returned,
 * avi.c).
 */
#include <SDL3/SDL.h>
#include <string.h>

#include "platform/standins.h"

enum {
    kWaveMapper = -1,
    kCallbackTypeMask = 0x70000,
    kCallbackNull = 0,
    kCallbackWindow = 0x10000,
    kWaveFormatQuery = 0x1,
    kWhdrDone = 0x1,
    kWhdrPrepared = 0x2,
    kWhdrInQueue = 0x10,
    kMmsyserrError = 1,
    kMmsyserrInvalHandle = 5,
    kMmsyserrNoMem = 7,
    kMmsyserrInvalFlag = 10,
    kMmsyserrInvalParam = 11,
    kWaverrBadFormat = 32,
    kWaverrStillPlaying = 33,
    kWaverrUnprepared = 34,
    kMmWomOpen = 0x3bb,
    kMmWomClose = 0x3bc,
    kMmWomDone = 0x3bd,
    kMaxQueue = 64,
    kHeaderSize = 0x20,
};

/* WAVEHDR */
typedef struct WaveHeader {
    uint8_t *data;
    uint32_t length;
    uint32_t recorded;
    uint32_t user;
    uint32_t flags;
    uint32_t loops;
    void *next;
    uint32_t reserved;
} WaveHeader;

_Static_assert(sizeof(WaveHeader) == kHeaderSize, "WAVEHDR");

/* WAVEFORMATEX up to cbSize */
typedef struct WaveFormat {
    uint16_t tag, channels;
    uint32_t samplesPerSec, avgBytesPerSec;
    uint16_t blockAlign, bitsPerSample;
} WaveFormat;

typedef struct WaveOut {
    uint32_t magic;
    SDL_AudioStream *stream;
    SDL_Mutex *mutex;
    uint32_t window;
    WaveHeader *queue[kMaxQueue]; /* written, not yet done; [0] is playing */
    int queued;
    uint32_t offset; /* bytes of queue[0] already handed to SDL */
    int paused;
    int mute;
    uint64_t written, played, dropped;
    uint64_t writtenHash, playedHash;
    uint32_t buffers;
    uint32_t openTime; /* bmxTimeGetTime at open (the close log line says how long the device was open) */
} WaveOut;

static const uint32_t kWaveMagic = 0x564f5657;

static uint64_t fnv(uint64_t hash, const uint8_t *p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        hash = (hash ^ p[i]) * 0x100000001b3ull;
    }
    return hash;
}

/* The window callback (CALLBACK_WINDOW): a posted message, as winmm's, into the game's queue (window.c). */
static void postMessage(const WaveOut *w, uint32_t message, uint32_t lParam)
{
    if (w->window != 0) {
        bmxWindowPost((void *)(uintptr_t)w->window, message, (uintptr_t)w, lParam);
    }
}

static WaveOut *asWave(uint32_t handle)
{
    WaveOut *const w = (WaveOut *)(uintptr_t)handle;

    return w != NULL && w->magic == kWaveMagic ? w : NULL;
}

/* Marks the first queued buffer done and tells the window (under the mutex). */
static void finishFirst(WaveOut *w)
{
    WaveHeader *const h = w->queue[0];

    w->queued--;
    for (int i = 0; i < w->queued; i++) {
        w->queue[i] = w->queue[i + 1];
    }
    w->offset = 0;
    h->flags = (h->flags & ~(uint32_t)kWhdrInQueue) | kWhdrDone;
    postMessage(w, kMmWomDone, (uint32_t)(uintptr_t)h);
}

/* SDL's audio thread: hand over queued bytes until SDL has what it asked for. */
static void SDLCALL feed(void *user, SDL_AudioStream *stream, int additional, int total)
{
    WaveOut *const w = user;
    uint8_t silence[4096];

    (void)total;
    SDL_LockMutex(w->mutex);
    while (additional > 0 && w->queued > 0 && !w->paused) {
        WaveHeader *const h = w->queue[0];
        const uint32_t left = h->length - w->offset;
        const uint32_t n = SDL_min(left, (uint32_t)additional);

        if (n > 0) {
            const uint8_t *const src = h->data + w->offset;

            w->playedHash = fnv(w->playedHash, src, n);
            w->played += n;
            if (w->mute) {
                memset(silence, 0, sizeof silence);
                for (uint32_t done = 0; done < n;) {
                    const uint32_t part = SDL_min(n - done, (uint32_t)sizeof silence);

                    SDL_PutAudioStreamData(stream, silence, (int)part);
                    done += part;
                }
            } else {
                SDL_PutAudioStreamData(stream, src, (int)n);
            }
            w->offset += n;
            additional -= (int)n;
        }
        if (w->offset >= h->length) {
            finishFirst(w);
        }
    }
    SDL_UnlockMutex(w->mutex);
}

static int formatSupported(const WaveFormat *f, SDL_AudioSpec *spec)
{
    if (f->tag != 1 || f->channels < 1 || f->channels > 2 || f->samplesPerSec < 1000 ||
        f->samplesPerSec > 200000 || (f->bitsPerSample != 8 && f->bitsPerSample != 16) ||
        f->blockAlign != f->channels * f->bitsPerSample / 8) {
        return 0;
    }
    spec->format = f->bitsPerSample == 8 ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE;
    spec->channels = f->channels;
    spec->freq = (int)f->samplesPerSec;
    return 1;
}

uint32_t BMX_WINAPI bmxStandWaveOutOpen(uint32_t *out, int32_t device, const void *format, uint32_t callback,
                                        uint32_t instance, uint32_t flags)
{
    const WaveFormat *const f = format;
    const char *mute = SDL_getenv("BMX_MUTE");
    SDL_AudioSpec spec;
    WaveOut *w;

    (void)instance;
    if (f == NULL) {
        return kMmsyserrInvalParam;
    }
    if (!formatSupported(f, &spec)) {
        bmxLog("waveout: format %u, %u channels, %u Hz, %u bits not supported\n", f->tag, f->channels,
               f->samplesPerSec, f->bitsPerSample);
        return kWaverrBadFormat;
    }
    if ((flags & kWaveFormatQuery) != 0) {
        return 0;
    }
    if (out == NULL) {
        return kMmsyserrInvalParam;
    }
    if ((flags & kCallbackTypeMask) != kCallbackWindow && (flags & kCallbackTypeMask) != kCallbackNull) {
        bmxLog("waveout: callback type 0x%x not supported (only windows)\n", flags & kCallbackTypeMask);
        return kMmsyserrInvalFlag;
    }
    if (device != kWaveMapper && device != 0) {
        return 2; /* MMSYSERR_BADDEVICEID */
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        bmxLog("waveout: SDL audio: %s\n", SDL_GetError());
        return kMmsyserrError;
    }
    w = SDL_calloc(1, sizeof *w);
    if (w == NULL) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return kMmsyserrNoMem;
    }
    w->magic = kWaveMagic;
    w->mutex = SDL_CreateMutex();
    w->window = (flags & kCallbackTypeMask) == kCallbackWindow ? callback : 0;
    w->mute = mute != NULL && *mute != 0 && *mute != '0';
    w->writtenHash = w->playedHash = 0xcbf29ce484222325ull;
    w->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, feed, w);
    if (w->stream == NULL) {
        bmxLog("waveout: no audio device (%s)\n", SDL_GetError());
        SDL_DestroyMutex(w->mutex);
        SDL_free(w);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return kMmsyserrError;
    }
    bmxLog("waveout: open %u Hz, %u channels, %u bits on SDL \"%s\"%s\n", f->samplesPerSec, f->channels,
           f->bitsPerSample, SDL_GetCurrentAudioDriver(), w->mute ? "; BMX_MUTE: output silent" : "");
    w->openTime = bmxTimeGetTime();
    *out = (uint32_t)(uintptr_t)w;
    SDL_ResumeAudioStreamDevice(w->stream); /* a wave device plays as soon as it has buffers */
    postMessage(w, kMmWomOpen, 0);
    return 0;
}

uint32_t BMX_WINAPI bmxStandWaveOutPrepareHeader(uint32_t handle, void *header, uint32_t size)
{
    WaveHeader *const h = header;

    if (asWave(handle) == NULL) {
        return kMmsyserrInvalHandle;
    }
    if (h == NULL || size < kHeaderSize) {
        return kMmsyserrInvalParam;
    }
    h->flags |= kWhdrPrepared;
    return 0;
}

uint32_t BMX_WINAPI bmxStandWaveOutUnprepareHeader(uint32_t handle, void *header, uint32_t size)
{
    WaveHeader *const h = header;

    if (asWave(handle) == NULL) {
        return kMmsyserrInvalHandle;
    }
    if (h == NULL || size < kHeaderSize) {
        return kMmsyserrInvalParam;
    }
    if ((h->flags & kWhdrInQueue) != 0) {
        return kWaverrStillPlaying;
    }
    h->flags &= ~(uint32_t)kWhdrPrepared;
    return 0;
}

uint32_t BMX_WINAPI bmxStandWaveOutWrite(uint32_t handle, void *header, uint32_t size)
{
    WaveOut *const w = asWave(handle);
    WaveHeader *const h = header;
    uint32_t result = 0;

    if (w == NULL) {
        return kMmsyserrInvalHandle;
    }
    if (h == NULL || size < kHeaderSize) {
        return kMmsyserrInvalParam;
    }
    if ((h->flags & kWhdrPrepared) == 0) {
        return kWaverrUnprepared;
    }
    SDL_LockMutex(w->mutex);
    if ((h->flags & kWhdrInQueue) != 0) {
        result = kWaverrStillPlaying;
    } else if (w->queued == kMaxQueue) {
        result = kMmsyserrNoMem;
    } else {
        h->flags = (h->flags & ~(uint32_t)kWhdrDone) | kWhdrInQueue;
        w->queue[w->queued++] = h;
        w->written += h->length;
        w->writtenHash = fnv(w->writtenHash, h->data, h->length);
        w->buffers++;
    }
    SDL_UnlockMutex(w->mutex);
    return result;
}

uint32_t BMX_WINAPI bmxStandWaveOutPause(uint32_t handle)
{
    WaveOut *const w = asWave(handle);

    if (w == NULL) {
        return kMmsyserrInvalHandle;
    }
    SDL_LockMutex(w->mutex);
    w->paused = 1;
    SDL_UnlockMutex(w->mutex);
    SDL_PauseAudioStreamDevice(w->stream);
    return 0;
}

uint32_t BMX_WINAPI bmxStandWaveOutRestart(uint32_t handle)
{
    WaveOut *const w = asWave(handle);

    if (w == NULL) {
        return kMmsyserrInvalHandle;
    }
    SDL_LockMutex(w->mutex);
    w->paused = 0;
    SDL_UnlockMutex(w->mutex);
    SDL_ResumeAudioStreamDevice(w->stream);
    return 0;
}

/* Stops at once: the queued buffers come back done (MM_WOM_DONE each), what SDL still holds is dropped. */
uint32_t BMX_WINAPI bmxStandWaveOutReset(uint32_t handle)
{
    WaveOut *const w = asWave(handle);

    if (w == NULL) {
        return kMmsyserrInvalHandle;
    }
    SDL_LockMutex(w->mutex);
    while (w->queued > 0) {
        w->dropped += w->queue[0]->length - w->offset;
        finishFirst(w);
    }
    SDL_UnlockMutex(w->mutex);
    /* Outside our mutex: SDL calls feed() with the stream locked, so the locks are always taken in that
     * order. */
    SDL_ClearAudioStream(w->stream);
    return 0;
}

uint32_t BMX_WINAPI bmxStandWaveOutClose(uint32_t handle)
{
    WaveOut *const w = asWave(handle);
    int queued;

    if (w == NULL) {
        return kMmsyserrInvalHandle;
    }
    SDL_LockMutex(w->mutex);
    queued = w->queued;
    SDL_UnlockMutex(w->mutex);
    if (queued != 0) {
        return kWaverrStillPlaying;
    }
    SDL_DestroyAudioStream(w->stream); /* waits for a running callback */
    bmxLog("waveout: close after %.1f s: %u buffers, %llu bytes written (FNV-1a 64 %016llx), %llu played "
           "(%016llx), "
           "%llu dropped by reset\n",
           (double)(bmxTimeGetTime() - w->openTime) / 1000.0, (unsigned)w->buffers,
           (unsigned long long)w->written, (unsigned long long)w->writtenHash, (unsigned long long)w->played,
           (unsigned long long)w->playedHash, (unsigned long long)w->dropped);
    postMessage(w, kMmWomClose, 0);
    w->magic = 0;
    SDL_DestroyMutex(w->mutex);
    SDL_free(w);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return 0;
}

/* sndPlaySoundA(NULL, 0) stops a playing sound (zmovieOpenAudio's retry); there is none. */
int32_t BMX_WINAPI bmxStandSndPlaySoundA(const char *sound, uint32_t flags)
{
    if (sound != NULL) {
        bmxLog("waveout: sndPlaySoundA(\"%s\", 0x%x) not supported\n", sound, flags);
        return 0;
    }
    return 1;
}
