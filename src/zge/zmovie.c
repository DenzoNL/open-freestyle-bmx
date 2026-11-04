/* AVI movie playback (original source file unknown, see zmovie.h). */
#include "zge/zmovie.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

/* ---- Win32 interfaces (import thunks and import table slots of the exe) ------------------------------ */

typedef uint32_t(BMX_STDCALL *WaveOutHandleFn)(uintptr_t hwo);
typedef uint32_t(BMX_STDCALL *WaveOutHeaderFn)(uintptr_t hwo, ZMovieWaveHeader *header, uint32_t size);
typedef uint32_t(BMX_STDCALL *WaveOutOpenFn)(uintptr_t *hwo, int32_t device, const void *format,
                                             uintptr_t callback, uintptr_t instance, uint32_t flags);
typedef uint32_t(BMX_STDCALL *SndPlaySoundFn)(const char *name, uint32_t flags);
typedef uint32_t(BMX_STDCALL *GetTickCountFn)(void);
typedef int32_t(BMX_STDCALL *GetClientRectFn)(void *window, ZMovieRect *rect);
typedef void *(BMX_STDCALL *GetDcFn)(void *window);
typedef int32_t(BMX_STDCALL *ReleaseDcFn)(void *window, void *hdc);

#define gWaveOutWrite     ORIG_VAR(0x00699808, WaveOutHeaderFn, gWaveOutWrite)
#define gWaveOutOpen      ORIG_VAR(0x0069980c, WaveOutOpenFn, gWaveOutOpen)
#define gSndPlaySound     ORIG_VAR(0x00699818, SndPlaySoundFn, gSndPlaySound)
#define gWaveOutRestart   ORIG_VAR(0x0069981c, WaveOutHandleFn, gWaveOutRestart)
#define gWaveOutPause     ORIG_VAR(0x00699820, WaveOutHandleFn, gWaveOutPause)
#define gWaveOutReset     ORIG_VAR(0x00699828, WaveOutHandleFn, gWaveOutReset)
#define gWaveOutUnprepare ORIG_VAR(0x0069982c, WaveOutHeaderFn, gWaveOutUnprepare)
#define gWaveOutClose     ORIG_VAR(0x00699830, WaveOutHandleFn, gWaveOutClose)
#define gWaveOutPrepare   ORIG_VAR(0x0069983c, WaveOutHeaderFn, gWaveOutPrepare)
#define gGetTickCount     ORIG_VAR(0x006995fc, GetTickCountFn, gGetTickCount)
#define gGetClientRect    ORIG_VAR(0x00699798, GetClientRectFn, gGetClientRect)
#define gReleaseDc        ORIG_VAR(0x0069979c, ReleaseDcFn, gReleaseDc)
#define gGetDc            ORIG_VAR(0x006997a0, GetDcFn, gGetDc)

/* The video library (VFW) thunks. */
ORIG_FN_STDCALL(0x004af9d0, int32_t, aviFileInit, (void));
ORIG_FN_STDCALL(0x004af9d6, int32_t, aviFileExit, (void));
ORIG_FN_STDCALL(0x004af9dc, int32_t, aviStreamReadFormat,
                (void *stream, int32_t pos, void *format, int32_t *size));
ORIG_FN_STDCALL(0x004af9e2, int32_t, aviStreamInfo, (void *stream, void *info, int32_t size));
ORIG_FN_STDCALL(0x004af9e8, int32_t, aviStreamRead,
                (void *stream, int32_t start, int32_t samples, void *buffer, int32_t bufferBytes,
                 uint32_t *bytes, int32_t *samplesRead));
ORIG_FN_STDCALL(0x004af9ee, int32_t, aviStreamStart, (void *stream));
ORIG_FN_STDCALL(0x004af9f4, int32_t, aviStreamLength, (void *stream));
ORIG_FN_STDCALL(0x004af9fa, int32_t, aviStreamTimeToSample, (void *stream, int32_t time));
ORIG_FN_STDCALL(0x004afa00, void *, drawDibOpen, (void));
ORIG_FN_STDCALL(0x004afa06, void *, aviStreamGetFrameOpen, (void *stream, void *format));
ORIG_FN_STDCALL(0x004afa0c, int32_t, aviStreamSampleToTime, (void *stream, int32_t sample));
ORIG_FN_STDCALL(0x004afa12, uint32_t, aviFileRelease, (void *file));
ORIG_FN_STDCALL(0x004afa18, int32_t, aviFileGetStream,
                (void *file, void **stream, uint32_t type, int32_t index));
ORIG_FN_STDCALL(0x004afa24, int32_t, aviFileOpen,
                (void **file, const char *path, uint32_t mode, void *handler));
ORIG_FN_STDCALL(0x004afa2a, uint32_t, aviStreamRelease, (void *stream));
ORIG_FN_STDCALL(0x004afa30, int32_t, aviSaveOptionsFree, (int32_t count, void *options));
ORIG_FN_STDCALL(0x004afa36, int32_t, drawDibClose, (void *drawDib));
ORIG_FN_STDCALL(0x004afa3c, int32_t, aviStreamGetFrameClose, (void *getFrame));
ORIG_FN_STDCALL(0x004afa42, int32_t, drawDibDraw,
                (void *drawDib, void *hdc, int32_t x, int32_t y, int32_t width, int32_t height,
                 void *bitmapInfo, void *bits, int32_t srcX, int32_t srcY, int32_t srcWidth,
                 int32_t srcHeight, uint32_t flags));
ORIG_FN_STDCALL(0x004afa48, void *, aviStreamGetFrame, (void *getFrame, int32_t position));

/* DirectDraw object and its GDI surface (COM: the vtable is the first word). */
#define gDirectDraw        ORIG_VAR(0x0069730c, void **, gDirectDraw)
#define gDirectDrawSurface ORIG_VAR(0x006977cc, void **, gDirectDrawSurface)

typedef int32_t(BMX_STDCALL *ComCall1Fn)(void *self);
typedef int32_t(BMX_STDCALL *ComCall2Fn)(void *self, void *arg);

/* The AVISTREAMINFO fields the movie code reads. */
typedef struct AviStreamInfo {
    uint32_t type;
    uint8_t pad04[0x2c];
    uint32_t sampleSize; /* 0x30 */
    uint8_t pad34[0x8c - 0x34];
} AviStreamInfo;

_Static_assert(sizeof(AviStreamInfo) == 0x8c, "AVISTREAMINFO size");

typedef struct WaveFormat {
    uint16_t tag, channels;
    uint32_t samplesPerSec; /* 0x04 */
    uint32_t avgBytesPerSec;
} WaveFormat;

/* Prepared buffers go back one by one, last first. */
static void releaseBuffers(ZMovie *m)
{
    while (m->bufferCount != 0) {
        m->bufferCount--;
        gWaveOutUnprepare(m->waveOut, m->buffers[m->bufferCount], sizeof(ZMovieWaveHeader));
        zmemFree(m->buffers[m->bufferCount]);
    }
}

static void closeWaveOut(ZMovie *m)
{
    if (m->waveOut != 0) {
        releaseBuffers(m);
        gWaveOutClose(m->waveOut);
        m->waveOut = 0;
    }
}

/* Stops the audio device when audio is playing (and releases the wave format). */
static void stopAudio(ZMovie *m)
{
    if (m->audioOn != 0 && m->audioStream != -1 && m->waveOut != 0) {
        gWaveOutReset(m->waveOut);
        gZdebugPrint("Movie : Close Audio\n");
        if (m->waveOut != 0) {
            releaseBuffers(m);
            gWaveOutClose(m->waveOut);
            m->waveOut = 0;
        }
        m->queued = 0;
        m->head = 0;
        m->audioOn = 0;
        zmemFree(m->waveFormat);
    }
}

BMX_ADDR(0x0049fb30)
void zmovieInit(ZMovie *m)
{
    int i;

    gZdebugPrint("Movie : Init\n");
    aviFileInit();
    for (i = 0; i < kZmovieMaxStreams; i++) {
        m->infoPtr[i] = &m->info[i];
    }
    for (i = 0; i < kZmovieMaxStreams; i++) {
        m->getFrame[i] = 0;
        m->drawDib[i] = 0;
        m->stream[i] = 0;
        zmemFill(&m->info[i], 0, sizeof(ZMovieStream));
    }
    m->streamCount = 0;
    m->waveOut = 0;
    m->position = 0;
    m->audioOn = 0;
    m->audioEnabled = 1;
    m->playing = 0;
    m->queued = 0;
    m->audioStream = -1;
    m->videoStream = -1;
    m->useDirectDraw = 0;
}

BMX_ADDR(0x0049fbe0)
void zmovieShutdown(ZMovie *m)
{
    gZdebugPrint("Movie : Shutdown\n");
    if (m->playing == 1) {
        stopAudio(m);
        m->playing = 0;
    }
    zmovieClose(m);
    aviFileExit();
}

BMX_ADDR(0x0049fcf0)
int zmovieOpenAudio(ZMovie *m, uintptr_t callback)
{
    AviStreamInfo info;
    void *stream;
    int32_t formatSize;

    if (m->audioStream < 0 || m->audioEnabled == 0) {
        return 0;
    }
    if (m->waveOut != 0) {
        return 1;
    }
    stream = m->stream[m->audioStream];
    aviStreamInfo(stream, &info, 0x8c);
    m->sampleSize = (int32_t)info.sampleSize;
    if (m->sampleSize <= 0 || m->sampleSize > 0x8000) {
        return 0;
    }
    aviStreamReadFormat(stream, 0, 0, &formatSize);
    m->waveFormat = zmemAlloc((uint32_t)formatSize);
    if (m->waveFormat == 0) {
        return 0;
    }
    aviStreamReadFormat(stream, 0, m->waveFormat, &formatSize);
    m->samplesPerSecLow = (uint16_t)((const WaveFormat *)m->waveFormat)->samplesPerSec;
    m->avgBytesPerSecLow = (uint16_t)((const WaveFormat *)m->waveFormat)->avgBytesPerSec;
    if (gWaveOutOpen(&m->waveOut, -1, m->waveFormat, callback, 0, 0x10000) != 0) {
        gSndPlaySound(0, 0);
        if (gWaveOutOpen(&m->waveOut, -1, m->waveFormat, callback, 0, 0x10000) != 0) {
            return 0;
        }
    }
    m->bufferCount = 0;
    do {
        ZMovieWaveHeader *buffer = zmemAlloc(kZmovieBufferBytes + sizeof(ZMovieWaveHeader));

        m->buffers[m->bufferCount] = buffer;
        if (buffer == 0) {
            break;
        }
        buffer->flags = 1;
        buffer->data = (uint8_t *)(buffer + 1); /* the data follows the header */
        buffer->length = kZmovieBufferBytes;
        if (gWaveOutPrepare(m->waveOut, buffer, sizeof(ZMovieWaveHeader)) != 0) {
            zmemFree(buffer);
            break;
        }
        m->bufferCount++;
    } while (m->bufferCount < kZmovieMaxBuffers);
    if (m->bufferCount < 2) {
        gZdebugPrint("Movie : Close Audio\n");
        closeWaveOut(m);
        m->queued = 0;
        m->head = 0;
        m->audioOn = 0;
        return 0;
    }
    m->audioOn = 1;
    m->queued = 0;
    m->head = 0;
    return 1;
}

BMX_ADDR(0x004a0040)
int zmovieFillAudio(ZMovie *m)
{
    gZdebugPrint("Movie : Fill Audio\n");
    if (m->audioOn != 0) {
        if (m->queued < m->bufferCount) {
            do {
                ZMovieWaveHeader *buffer;
                int32_t samples, readSamples;
                int32_t perBuffer;

                samples = m->audioEnd;
                if (m->audioPos >= samples) {
                    if (m->loop == 0) {
                        break;
                    }
                    m->audioPos = m->audioStart;
                }
                perBuffer = kZmovieBufferBytes / m->sampleSize;
                samples -= m->audioPos;
                if (samples > perBuffer) {
                    samples = perBuffer;
                }
                buffer = m->buffers[m->head];
                aviStreamRead(m->stream[m->audioStream], m->audioPos, samples, buffer->data,
                              kZmovieBufferBytes, &buffer->length, &readSamples);
                if (readSamples != samples) {
                    return 0;
                }
                m->audioPos += readSamples;
                if (gWaveOutWrite(m->waveOut, m->buffers[m->head], sizeof(ZMovieWaveHeader)) != 0) {
                    return 0;
                }
                m->queued++;
                m->head++;
                if (m->head >= m->bufferCount) {
                    m->head = 0;
                }
            } while (m->queued < m->bufferCount);
        }
        if (m->queued == 0 && m->audioPos >= m->audioEnd) {
            stopAudio(m);
        }
    }
    return 1;
}

BMX_ADDR(0x004a0270)
int zmovieStartAudio(ZMovie *m, uintptr_t callback)
{
    int32_t sample, end;

    if (m->audioStream == -1) {
        return 0;
    }
    sample = aviStreamTimeToSample(m->stream[m->audioStream], m->position);
    end = aviStreamLength(m->stream[m->audioStream]);
    end += aviStreamStart(m->stream[m->audioStream]);
    zmovieOpenAudio(m, callback);
    if (sample >= end) {
        return 0;
    }
    if (m->playing == 0) {
        gWaveOutPause(m->waveOut);
        m->audioStart = sample;
        m->audioPos = sample;
    }
    m->audioEnd = end;
    zmovieFillAudio(m);
    gWaveOutRestart(m->waveOut);
    return 1;
}

BMX_ADDR(0x004a0400)
void zmovieScanStreams(ZMovie *m)
{
    int32_t i;

    m->startTime = 0x7fffffff;
    m->endTime = 0;
    for (i = 0; i < m->streamCount; i++) {
        AviStreamInfo info;
        ZMovieStream *s = &m->info[i];
        void *stream = m->stream[i];
        uint32_t oldStart, oldEnd, t;

        aviStreamInfo(stream, &info, 0x8c);
        m->infoPtr[i] = s;
        zmemFill(s, 0, sizeof(ZMovieStream));
        s->type = info.type;
        if (info.type == kZmovieStreamVideo) {
            s->flags = 0xe;
            s->field04 = 0;
            s->field0c = -1;
            s->field08 = -1;
            s->field10 = 0;
            s->field28 = 1;
        } else if (info.type == kZmovieStreamAudio) {
            int32_t formatSize;

            s->flags |= 8;
            s->field28 = 1;
            aviStreamReadFormat(stream, aviStreamStart(stream), 0, &formatSize);
            s->formatSize = (uint32_t)formatSize;
            if (formatSize != 0) {
                s->format = zmemAlloc((uint32_t)formatSize);
            }
            if (s->format != 0) {
                aviStreamReadFormat(stream, aviStreamStart(stream), s->format, &formatSize);
            }
        }
        oldStart = m->startTime;
        t = (uint32_t)aviStreamSampleToTime(stream, aviStreamStart(stream));
        if (oldStart >= t) {
            t = (uint32_t)aviStreamSampleToTime(stream, aviStreamStart(stream));
        } else {
            t = oldStart;
        }
        m->startTime = t;
        oldEnd = m->endTime;
        t = (uint32_t)aviStreamSampleToTime(stream, aviStreamLength(stream) + aviStreamStart(stream));
        if (oldEnd > t) {
            t = oldEnd;
        } else {
            t = (uint32_t)aviStreamSampleToTime(stream, aviStreamLength(stream) + aviStreamStart(stream));
        }
        m->endTime = t;
        if (info.type == kZmovieStreamVideo) {
            m->getFrame[i] = aviStreamGetFrameOpen(stream, 0);
            if (m->getFrame[i] != 0) {
                m->drawDib[i] = drawDibOpen();
                if (m->videoStream < 0) {
                    m->videoStream = i;
                }
            }
        } else if (info.type == kZmovieStreamAudio) {
            m->getFrame[i] = 0;
            m->drawDib[i] = 0;
            if (m->audioStream < 0) {
                m->audioStream = i;
            }
        }
    }
    m->length = (int32_t)(m->endTime - m->startTime);
    if (m->length == 0) {
        m->length = 1;
    }
}

BMX_ADDR(0x004a06f0)
int zmovieOpen(ZMovie *m, const char *path)
{
    void *file;
    int32_t count = 0;

    zmovieClose(m);
    m->audioStream = -1;
    m->videoStream = -1;
    if (aviFileOpen(&file, path, 0, 0) != 0) {
        return 0;
    }
    for (;;) {
        void *stream;

        if (aviFileGetStream(file, &stream, 0, count) != 0) {
            break;
        }
        m->stream[count] = stream; /* not bounded by kZmovieMaxStreams, as in the original */
        count++;
    }
    aviFileRelease(file);
    if (count == 0) {
        return 0;
    }
    m->streamCount = count;
    zmovieScanStreams(m);
    return 1;
}

BMX_ADDR(0x004a0780)
void zmovieClose(ZMovie *m)
{
    int32_t i;

    gZdebugPrint("Movie : Close\n");
    if (m->playing == 1) {
        stopAudio(m);
        m->playing = 0;
    }
    for (i = 0; i < m->streamCount; i++) {
        if (m->getFrame[i] != 0) {
            aviStreamGetFrameClose(m->getFrame[i]);
            m->getFrame[i] = 0;
        }
        if (m->drawDib[i] != 0) {
            drawDibClose(m->drawDib[i]);
            m->drawDib[i] = 0;
        }
        if (m->infoPtr[i]->format != 0) {
            zmemFree(m->infoPtr[i]->format);
        }
    }
    /* The original hands the pointer table to AVISaveOptionsFree. */
    aviSaveOptionsFree(m->streamCount, (void *)m->infoPtr);
    for (i = 0; i < m->streamCount; i++) {
        aviStreamRelease(m->stream[i]);
    }
    gZdebugPrint("Movie : Close Audio\n");
    closeWaveOut(m);
    m->queued = 0;
    m->head = 0;
    m->audioOn = 0;
    m->audioStream = -1;
    m->videoStream = -1;
    m->streamCount = 0;
}

BMX_ADDR(0x004a09d0)
void zmoviePlay(ZMovie *m, void *window, int useDirectDraw, int loop, const ZMovieRect *rect)
{
    if (m->playing == 0) {
        m->useDirectDraw = useDirectDraw;
        m->loop = loop;
        m->startTick = gGetTickCount() - (uint32_t)m->position;
        m->window = window;
        zmovieStartAudio(m, (uintptr_t)window);
        if (rect != 0) {
            m->rect = *rect;
            m->playing = 1;
            return;
        }
        gGetClientRect(window, &m->rect);
        m->playing = 1;
    }
}

BMX_ADDR(0x004a0a70)
void zmovieRefillAudio(ZMovie *m)
{
    m->queued--;
    zmovieFillAudio(m);
}

BMX_ADDR(0x004a0a90)
void zmovieUpdate(ZMovie *m, void *hdc, const ZMovieRect *rect)
{
    int32_t position;
    int release = 0;
    uint32_t elapsed;

    if (rect != 0) {
        m->rect = *rect;
    }
    if (hdc == 0) {
        if (m->useDirectDraw != 0 && gDirectDraw != 0) {
            ((ComCall2Fn)(uintptr_t)(*(uint32_t **)gDirectDraw)[0x90 / 4])((void *)gDirectDraw,
                                                                           (void *)&gDirectDrawSurface);
            ((ComCall1Fn)(uintptr_t)(*(uint32_t **)gDirectDrawSurface)[0x28 / 4])((void *)gDirectDrawSurface);
        }
        hdc = gGetDc(m->window);
        release = 1;
    }
    if (m->playing == 1) {
        elapsed = gGetTickCount() - m->startTick;
        if (elapsed > (uint32_t)m->length) {
            gZdebugPrint("Movie : End Of Movie\n");
            if (m->loop == 1) {
                position = 0;
                m->startTick = gGetTickCount();
                m->position = 0;
                goto draw;
            }
            elapsed = (uint32_t)(m->length - 1);
            if (m->playing == 1) {
                stopAudio(m);
                m->playing = 0;
            }
        }
        m->position = (int32_t)elapsed;
        position = (int32_t)elapsed;
    } else {
        position = m->position;
    }
draw:
    zmovieDraw(m, hdc, m->rect.left, m->rect.top, m->rect.right, m->rect.bottom, position);
    if (release) {
        gReleaseDc(m->window, hdc);
    }
}

BMX_ADDR(0x004a0cb0)
void zmovieStop(ZMovie *m)
{
    if (m->playing == 1) {
        stopAudio(m);
        m->playing = 0;
    }
}

BMX_ADDR(0x004a0db0)
void zmovieDraw(ZMovie *m, void *hdc, int32_t left, int32_t top, int32_t right, int32_t bottom,
                int32_t position)
{
    int32_t i;

    if (position == -1) {
        position = m->position;
    } else {
        m->position = position;
    }
    for (i = 0; i < m->streamCount; i++) {
        AviStreamInfo info;
        void *stream = m->stream[i];

        aviStreamInfo(stream, &info, 0x8c);
        if (info.type == kZmovieStreamVideo && m->getFrame[i] != 0) {
            int32_t endTime, sample;
            void *frame;

            endTime = aviStreamSampleToTime(stream, aviStreamLength(stream) + aviStreamStart(stream));
            if (position <= endTime) {
                sample = aviStreamTimeToSample(stream, position);
            } else {
                const int32_t total = aviStreamLength(stream) + aviStreamStart(stream);

                sample = aviStreamTimeToSample(stream, position - endTime) + total;
            }
            if (m->getFrame[i] != 0 && sample >= aviStreamStart(stream)) {
                frame = aviStreamGetFrame(m->getFrame[i], sample);
                if (frame != 0) {
                    drawDibDraw(m->drawDib[i], hdc, left, top, right - left, bottom - top, frame, 0, 0, 0, -1,
                                -1, i != m->videoStream ? 0x200u : 0u);
                }
            }
        }
    }
}

BMX_ADDR(0x004a1050)
int zmovieIsPlaying(const ZMovie *m)
{
    return m->playing;
}
