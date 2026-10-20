/*
 * AVI movie playback (original source file unknown; data/functions.csv files it under zfont.c by
 * address). A movie is a set of AVI streams: video frames are drawn with DrawDib, the audio stream is
 * fed to a waveOut device through a ring of 32 KB buffers. Time is milliseconds from GetTickCount.
 */
#ifndef ZGE_ZMOVIE_H
#define ZGE_ZMOVIE_H

#include <stdint.h>

#include "compat/orig.h"

enum {
    kZmovieMaxStreams = 8,
    kZmovieMaxBuffers = 16,
    kZmovieBufferBytes = 0x8000,
    kZmovieStreamVideo = 0x73646976, /* 'vids' */
    kZmovieStreamAudio = 0x73647561, /* 'auds' */
};

/* Per stream bookkeeping (0x2c bytes). */
typedef struct ZMovieStream {
    uint32_t type;       /* 0x00 fccType */
    int32_t field04;     /* 0x04 */
    int32_t field08;     /* 0x08 */
    int32_t field0c;     /* 0x0c */
    int32_t field10;     /* 0x10 */
    uint32_t flags;      /* 0x14 */
    void *format;        /* 0x18 audio stream format, from zmemAlloc */
    uint32_t formatSize; /* 0x1c */
    uint8_t pad20[8];
    int32_t field28; /* 0x28 */
} ZMovieStream;

/* The RECT the movie is drawn into. */
typedef struct ZMovieRect {
    int32_t left, top, right, bottom;
} ZMovieRect;

/* The part of WAVEHDR this code touches (0x20 bytes, followed by the 0x8000 byte data). */
typedef struct ZMovieWaveHeader {
    uint8_t *data;     /* 0x00 lpData */
    uint32_t length;   /* 0x04 dwBufferLength */
    uint32_t recorded; /* 0x08 */
    uint32_t user;     /* 0x0c */
    uint32_t flags;    /* 0x10 dwFlags */
    uint32_t loops;    /* 0x14 */
    void *next;        /* 0x18 */
    uint32_t reserved; /* 0x1c */
} ZMovieWaveHeader;

typedef struct ZMovie {
    int32_t videoStream;                      /* 0x000 index, -1 none */
    int32_t audioStream;                      /* 0x004 index, -1 none */
    int32_t streamCount;                      /* 0x008 */
    void *stream[kZmovieMaxStreams];          /* 0x00c PAVISTREAM */
    uint32_t startTime;                       /* 0x02c earliest stream start (ms) */
    uint32_t endTime;                         /* 0x030 latest stream end (ms) */
    int32_t length;                           /* 0x034 endTime - startTime, at least 1 */
    int32_t position;                         /* 0x038 current time (ms) */
    ZMovieStream info[kZmovieMaxStreams];     /* 0x03c */
    ZMovieStream *infoPtr[kZmovieMaxStreams]; /* 0x19c */
    void *getFrame[kZmovieMaxStreams];        /* 0x1bc PGETFRAME of video streams */
    void *drawDib[kZmovieMaxStreams];         /* 0x1dc HDRAWDIB of video streams */
    int32_t playing;                          /* 0x1fc */
    uint32_t startTick;                       /* 0x200 GetTickCount at position 0 */
    void *window;                             /* 0x204 */
    ZMovieRect rect;                          /* 0x208 */
    int32_t loop;                             /* 0x218 */
    int32_t useDirectDraw;                    /* 0x21c release the DirectDraw GDI surface before drawing */
    void *waveFormat;                         /* 0x220 WAVEFORMATEX, from zmemAlloc */
    int32_t sampleSize;                       /* 0x224 bytes per sample */
    uint16_t samplesPerSecLow;                /* 0x228 */
    uint16_t avgBytesPerSecLow;               /* 0x22a */
    uint32_t waveOut;                         /* 0x22c HWAVEOUT, 0 when closed */
    uint16_t bufferCount;                     /* 0x230 prepared buffers */
    uint16_t queued;                          /* 0x232 buffers handed to the device */
    uint16_t head;                            /* 0x234 next buffer to fill */
    uint8_t pad236[2];
    ZMovieWaveHeader *buffers[kZmovieMaxBuffers]; /* 0x238 */
    int32_t audioStart;                           /* 0x278 first audio sample */
    int32_t audioPos;                             /* 0x27c next sample to read */
    int32_t audioEnd;                             /* 0x280 end of the audio */
    int32_t audioOn;                              /* 0x284 audio device open and feeding */
    int32_t audioEnabled;                         /* 0x288 audio may be played at all */
} ZMovie;

_Static_assert(sizeof(ZMovieStream) == 0x2c, "stream info layout");
_Static_assert(__builtin_offsetof(ZMovie, info) == 0x3c, "movie layout");
_Static_assert(__builtin_offsetof(ZMovie, drawDib) == 0x1dc, "movie layout");
_Static_assert(__builtin_offsetof(ZMovie, playing) == 0x1fc, "movie layout");
_Static_assert(__builtin_offsetof(ZMovie, buffers) == 0x238, "movie layout");
_Static_assert(sizeof(ZMovie) == 0x28c, "movie layout");

/* Clears the movie and starts the AVI library. */
void zmovieInit(ZMovie *movie);
void zmovieShutdown(ZMovie *movie);
/* Opens the waveOut device for the audio stream and prepares the buffers; 1 when audio is ready. */
int zmovieOpenAudio(ZMovie *movie, uint32_t callback);
/* Reads audio into free buffers and queues them. Returns 0 on a read or device error. */
int zmovieFillAudio(ZMovie *movie);
/* Starts the audio at the current position (opening the device first). */
int zmovieStartAudio(ZMovie *movie, uint32_t callback);
/* Collects the time span and the video/audio streams of an opened file. */
void zmovieScanStreams(ZMovie *movie);
int zmovieOpen(ZMovie *movie, const char *path);
void zmovieClose(ZMovie *movie);
/* Starts playback into window; rect is the target (NULL: the window's client area). */
void zmoviePlay(ZMovie *movie, void *window, int useDirectDraw, int loop, const ZMovieRect *rect);
/* Refills the audio after a buffer finished. */
void zmovieRefillAudio(ZMovie *movie);
/* Advances the clock and draws the current frame; hdc 0 means draw on the window's DC. */
void zmovieUpdate(ZMovie *movie, void *hdc, const ZMovieRect *rect);
void zmovieStop(ZMovie *movie);
/* Draws the frame at `position` (ms; -1 keeps the current one) into the rectangle. */
void zmovieDraw(ZMovie *movie, void *hdc, int32_t left, int32_t top, int32_t right, int32_t bottom,
                int32_t position);
int zmovieIsPlaying(const ZMovie *movie);

#endif
