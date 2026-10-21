/* Sound streaming (C:\zaxis\Bmx\src\Pc\zge\sound\winStrm.c). */
#include "bmx/pc/zge/sound/winstrm.h"

#include "compat/crt.h"

static const char winstrmFile[] = "C:\\zaxis\\Bmx\\src\\Pc\\zge\\sound\\winStrm.c";

/* Exe tables of the ADPCM decoder: step index adjustments per nibble, and the step sizes. */
typedef int32_t AdpcmIndexTable[16];
typedef uint16_t AdpcmStepTable[89];
#define gAdpcmIndex ORIG_VAR(0x0055f1ac, AdpcmIndexTable, gAdpcmIndex)
#define gAdpcmStep  ORIG_VAR(0x0055f1ec, AdpcmStepTable, gAdpcmStep)
/* DirectSound volume (1/100 dB) for 0..127. */
typedef int16_t VolumeTable[128];
#define gDsVolume ORIG_VAR(0x0055ec7c, VolumeTable, gDsVolume)

static void requireInitialized(int line)
{
    if (gWinstrmInitialized == 0) {
        _assert("fzsndStreamInitialized", winstrmFile, (unsigned)line);
    }
}

/* DirectSound calls must succeed. */
static void requireOk(DsResult hr, int line)
{
    if (hr != 0) {
        _assert("hResult == DS_OK", winstrmFile, (unsigned)line);
    }
}

BMX_ADDR(0x00470aa0)
int winstrmInit(void)
{
    uint32_t *p;
    int i;

    if (gWinstrmInitialized != 0) {
        _assert("!fzsndStreamInitialized", winstrmFile, 0x73);
    }
    p = (uint32_t *)&gWinstrmStreams;
    for (i = 0; i < 0x88; i++) {
        p[i] = 0;
    }
    p = (uint32_t *)&gWinstrmNodes;
    for (i = 0; i < 0x60; i++) {
        p[i] = 0;
    }
    gWinstrmStreamHint = 0;
    gWinstrmNodeHint = 0;
    gWinstrmPaused = 0;
    gWinInitCriticalSection(gWinstrmLock);
    gWinstrmInitialized = 1;
    return 0;
}

BMX_ADDR(0x00470b20)
int winstrmShutdown(void)
{
    int i;

    requireInitialized(0x8f);
    for (i = 0; i < kWinstrmMaxStreams; i++) {
        if ((gWinstrmStreams[i].flags & kWinstrmFlagUsed) != 0) {
            winstrmDestroy(&gWinstrmStreams[i]);
        }
    }
    gWinDeleteCriticalSection(gWinstrmLock);
    gWinstrmInitialized = 0;
    return 0;
}

BMX_ADDR(0x00470ba0)
int winstrmUpdateFades(void)
{
    int32_t next;
    int i;

    if (gWinstrmInitialized == 0) {
        return 0;
    }
    next = gWinstrmTick + 1;
    for (i = 0; i < kWinstrmMaxStreams; i++) {
        WinstrmStream *s = &gWinstrmStreams[i];
        int32_t ticks;

        if ((s->flags & kWinstrmFlagUsed) == 0 || s->state != kWinstrmPlaying ||
            (s->fadeFlags & kWinstrmFadeActive) == 0) {
            continue;
        }
        for (ticks = next - gWinstrmTick; ticks != 0; ticks--) {
            s->fadeLevel += s->fadeStep;
            if (s->fadeStep > 0) {
                if (s->fadeLevel >= s->fadeTarget) {
                    s->fadeLevel = s->fadeTarget;
                    s->fadeFlags &= ~(uint32_t)kWinstrmFadeActive;
                    break;
                }
            } else if (s->fadeLevel <= s->fadeTarget) {
                s->fadeLevel = s->fadeTarget;
                s->fadeFlags &= ~(uint32_t)kWinstrmFadeActive;
                break;
            }
        }
        requireOk(s->buffer->vtbl->setVolume(s->buffer, ((s->fadeLevel >> 16) - 0x7f) * 12), 0xd3);
        s->volume = (uint8_t)(s->fadeLevel >> 16);
        if ((s->fadeFlags & kWinstrmFadeActive) == 0 && (s->fadeFlags & kWinstrmFadeThenStop) != 0) {
            if ((s->flags & kWinstrmFlagAutoDestroy) != 0) {
                winstrmDestroy(s);
            } else {
                winstrmStop(s);
            }
            gWinstrmTick = next;
            return 0;
        }
    }
    gWinstrmTick = next;
    return 0;
}

/* Bytes between the play cursor and the ring offset where the data ended (a full lap when equal). */
static uint32_t drainBytes(const WinstrmStream *s, uint32_t play, uint32_t endPos)
{
    return (endPos <= play ? s->bufferBytes : 0) - play + endPos;
}

/* The source ran dry while filling: mark the stream as draining towards `endPos`. */
static void startDraining(WinstrmStream *s, uint32_t play, uint32_t endPos)
{
    s->flags |= kWinstrmFlagDraining;
    s->endOffset = endPos;
    s->drain = drainBytes(s, play, endPos);
}

BMX_ADDR(0x00470d10)
int winstrmUpdate(void)
{
    int i;

    if (gWinstrmInitialized == 0) {
        return 0;
    }
    gWinEnterCriticalSection(gWinstrmLock);
    for (i = 0; i < kWinstrmMaxStreams; i++) {
        WinstrmStream *s = &gWinstrmStreams[i];
        uint32_t play, write, delta, filled, endPos;

        if ((s->flags & kWinstrmFlagUsed) == 0 || s->state != kWinstrmPlaying || gWinstrmPaused >= 1) {
            continue;
        }
        requireOk(s->buffer->vtbl->getCurrentPosition(s->buffer, &play, &write), 0xfb);
        delta = play >= s->lastPlay ? play - s->lastPlay : s->bufferBytes - s->lastPlay + play;
        if ((s->flags & kWinstrmFlagDraining) != 0) {
            if (s->drain <= delta) {
                if ((s->flags & kWinstrmFlagAutoDestroy) != 0) {
                    winstrmDestroy(s);
                } else {
                    winstrmStop(s);
                }
                continue;
            }
            s->drain -= delta;
        }
        s->played += delta;
        while (s->played >= s->threshold) {
            if (s->writeOffset + s->threshold < s->bufferBytes) {
                filled = winstrmFill(s, s->threshold, &endPos);
                if (filled < s->threshold && (s->flags & kWinstrmFlagDraining) == 0) {
                    startDraining(s, play, endPos);
                }
            } else {
                /* The refill wraps around the end of the ring buffer: fill in two pieces. */
                const uint32_t head = s->bufferBytes - s->writeOffset;

                filled = winstrmFill(s, head, &endPos);
                if (filled >= head) {
                    const uint32_t rest = s->threshold - head;

                    s->writeOffset = 0;
                    if (rest != 0) {
                        filled = winstrmFill(s, rest, &endPos);
                        if (filled < rest && (s->flags & kWinstrmFlagDraining) == 0) {
                            startDraining(s, play, endPos);
                        }
                    }
                } else {
                    if ((s->flags & kWinstrmFlagDraining) == 0) {
                        startDraining(s, play, endPos);
                    }
                    s->writeOffset = 0;
                    winstrmFill(s, s->threshold - head, &endPos);
                }
            }
            s->flags &= (uint16_t)~kWinstrmFlagPrimed;
            s->played -= s->threshold;
        }
        s->lastPlay = play;
    }
    gWinLeaveCriticalSection(gWinstrmLock);
    return 0;
}

BMX_ADDR(0x00470fb0)
int winstrmCreate(WinstrmStream **out, const WinloadSampleFile *file, uint32_t reserved, uint32_t bufferBytes,
                  uint32_t flags)
{
    WinstrmSource *source;
    WinstrmStream *s = 0;
    DsWaveFormat format;
    DsBufferDesc desc;
    uint32_t filled, endPos;
    int slot;

    requireInitialized(0x152);
    source = winstrmOpenSource(file, flags);
    if (source == 0) {
        *out = 0;
        return 0x201;
    }
    gWinEnterCriticalSection(gWinstrmLock);
    slot = gWinstrmStreamHint;
    while ((gWinstrmStreams[slot].flags & kWinstrmFlagUsed) != 0) {
        slot++;
        if (slot == kWinstrmMaxStreams) {
            slot = 0;
        }
        if (slot == gWinstrmStreamHint) {
            goto slotDone;
        }
    }
    s = &gWinstrmStreams[slot];
    s->flags = kWinstrmFlagUsed;
    s->state = kWinstrmStopped;
    gWinstrmStreamHint = slot + 1;
    if (gWinstrmStreamHint == kWinstrmMaxStreams) {
        gWinstrmStreamHint = 0;
    }
slotDone:
    gWinLeaveCriticalSection(gWinstrmLock);
    if (s == 0) {
        winstrmCloseSource(source);
        *out = 0;
        return 0x202;
    }
    s->node.source = source;
    s->node.repeat = 1;
    s->node.next = 0;
    s->fadeFlags = 0;
    s->volume = 0x7f;
    s->pan = 0;
    s->bufferBytes = bufferBytes != 0 ? bufferBytes & ~(uint32_t)3 : kWinstrmDefaultBytes;
    if (s->bufferBytes == 0) {
        winstrmCloseSource(source);
        *out = 0;
        return 0x204;
    }
    winstrmSetThreshold(s, 50);
    if ((flags & 2) != 0) {
        s->flags |= kWinstrmFlagAutoDestroy;
    } else {
        s->flags &= (uint16_t)~kWinstrmFlagAutoDestroy;
    }
    format.formatTag = 1;
    format.channels = source->channels;
    format.samplesPerSec = source->rate;
    format.blockAlign = (uint16_t)((int32_t)(source->bits * source->channels) / 8);
    format.avgBytesPerSec = (uint32_t)format.blockAlign * source->rate;
    format.bitsPerSample = source->bits;
    memset(&desc, 0, sizeof desc);
    desc.size = sizeof desc;
    desc.flags |= kDsCapsControlPan | kDsCapsControlVolume | kDsCapsControlFrequency;
    desc.bufferBytes = s->bufferBytes;
    desc.format = &format;
    requireOk(gWinstrmDirectSound->vtbl->createSoundBuffer(gWinstrmDirectSound, &desc, &s->buffer, 0), 0x184);
    s->writeOffset = 0;
    filled = winstrmFill(s, s->bufferBytes, &endPos);
    if (filled < s->bufferBytes) {
        s->flags |= kWinstrmFlagDraining;
        s->endOffset = endPos;
        s->drain = endPos;
    } else {
        s->flags &= (uint16_t)~kWinstrmFlagDraining;
    }
    s->flags |= kWinstrmFlagPrimed;
    s->played = 0;
    s->state = kWinstrmStopped;
    *out = s;
    return 0;
}

BMX_ADDR(0x004712f0)
int winstrmDestroy(WinstrmStream *stream)
{
    WinstrmNode *node;
    int i;

    requireInitialized(0x1b1);
    gWinEnterCriticalSection(gWinstrmLock);
    requireOk((int32_t)stream->buffer->vtbl->release(stream->buffer), 0x1b6);
    node = &stream->node;
    while (node != 0) {
        WinstrmNode *next = node->next;

        winstrmCloseSource(node->source);
        winstrmNodeFree(node);
        node = next;
    }
    for (i = 0; i < kWinstrmMaxStreams; i++) {
        if (stream == &gWinstrmStreams[i]) {
            gWinstrmStreamHint = i;
            gWinstrmStreams[i].flags = 0;
            break;
        }
    }
    gWinLeaveCriticalSection(gWinstrmLock);
    return 0;
}

BMX_ADDR(0x00471400)
int winstrmSetThreshold(WinstrmStream *stream, int percent)
{
    requireInitialized(0x1d3);
    stream->threshold = (uint32_t)((uint32_t)((int32_t)stream->bufferBytes * percent)) / 100 & ~(uint32_t)3;
    return 0;
}

/* Source of the same format as the one playing? */
static int sameFormat(const WinstrmSource *a, const WinstrmSource *b)
{
    return a->channels == b->channels && a->rate == b->rate && a->bits == b->bits;
}

/*
 * After the chain changed behind the end of the data: refill from where the data ended, and note whether
 * the new data runs dry again.
 */
static void refillAfterEnd(WinstrmStream *s)
{
    uint32_t endPos, filled;

    s->writeOffset = s->endOffset;
    filled = winstrmFill(s, s->bufferBytes - s->endOffset, &endPos);
    if (filled < s->bufferBytes - s->endOffset) {
        s->flags |= kWinstrmFlagDraining;
        s->endOffset = endPos;
        s->drain = endPos;
    } else {
        s->flags &= (uint16_t)~kWinstrmFlagDraining;
    }
}

BMX_ADDR(0x00471450)
int winstrmReplaceSource(WinstrmStream *stream, const WinloadSampleFile *file, uint32_t flags)
{
    WinstrmSource *source;
    uint32_t filled, endPos;

    requireInitialized(0x1e9);
    source = winstrmOpenSource(file, flags);
    if (source == 0) {
        return 0x201;
    }
    if (!sameFormat(source, stream->node.source)) {
        winstrmCloseSource(source);
        return 0x206;
    }
    gWinEnterCriticalSection(gWinstrmLock);
    winstrmCloseSource(stream->node.source);
    requireOk(stream->buffer->vtbl->stop(stream->buffer), 0x1f1);
    stream->node.source = source;
    stream->node.repeat = 1;
    stream->writeOffset = 0;
    filled = winstrmFill(stream, stream->bufferBytes, &endPos);
    if (filled < stream->bufferBytes) {
        stream->flags |= kWinstrmFlagDraining;
        stream->endOffset = endPos;
        stream->drain = endPos;
    } else {
        stream->flags &= (uint16_t)~kWinstrmFlagDraining;
    }
    stream->flags |= kWinstrmFlagPrimed;
    stream->lastPlay = 0;
    stream->played = 0;
    if (stream->state == kWinstrmPlaying) {
        requireOk(stream->buffer->vtbl->play(stream->buffer, 0, 0, kDsPlayLooping), 0x202);
    }
    gWinLeaveCriticalSection(gWinstrmLock);
    return 0;
}

BMX_ADDR(0x00471610)
int winstrmSetRepeat(WinstrmStream *stream, int count)
{
    if (count == 0) {
        return 0x207;
    }
    stream->node.repeat = (uint32_t)count;
    if ((stream->flags & kWinstrmFlagDraining) != 0) {
        stream->node.repeat = (uint32_t)count - 1;
        if (stream->node.repeat != 0) {
            gWinEnterCriticalSection(gWinstrmLock);
            refillAfterEnd(stream);
            stream->played = 0;
            gWinLeaveCriticalSection(gWinstrmLock);
        }
    }
    return 0;
}

BMX_ADDR(0x004716b0)
int winstrmQueueSource(WinstrmStream *stream, const WinloadSampleFile *file, uint32_t flags)
{
    WinstrmSource *source;
    WinstrmNode *node, *last;

    requireInitialized(0x242);
    source = winstrmOpenSource(file, flags);
    if (source == 0) {
        return 0x201;
    }
    if (!sameFormat(source, stream->node.source)) {
        return 0x206;
    }
    node = winstrmNodeAlloc();
    if (node == 0) {
        return 0x205;
    }
    gWinEnterCriticalSection(gWinstrmLock);
    last = &stream->node;
    if ((flags & 1) != 0) {
        node->next = last->next;
    } else {
        while (last->next != 0) {
            last = last->next;
        }
        node->next = 0;
    }
    last->next = node;
    node->source = source;
    node->repeat = 1;
    if ((stream->flags & kWinstrmFlagDraining) != 0) {
        refillAfterEnd(stream);
        stream->played = 0;
    }
    gWinLeaveCriticalSection(gWinstrmLock);
    return 0;
}

BMX_ADDR(0x00471840)
int winstrmGetState(const WinstrmStream *stream, uint32_t *state)
{
    requireInitialized(0x27a);
    *state = stream->state;
    return 0;
}

BMX_ADDR(0x00471890)
int winstrmStart(WinstrmStream *stream)
{
    uint32_t filled, endPos;

    requireInitialized(0x28e);
    switch (stream->state) {
    case kWinstrmPaused:
        stream->pauseCount = 0;
        stream->state = kWinstrmPlaying;
        requireOk(stream->buffer->vtbl->play(stream->buffer, 0, 0, kDsPlayLooping), 0x296);
        return 0;
    case kWinstrmPlaying:
        requireOk(stream->buffer->vtbl->stop(stream->buffer), 0x29a);
        stream->state = kWinstrmStopped;
        /* fall through */
    case kWinstrmStopped:
        break;
    default:
        return 0;
    }
    requireOk(stream->buffer->vtbl->setCurrentPosition(stream->buffer, 0), 0x29e);
    if ((stream->flags & kWinstrmFlagPrimed) == 0) {
        winstrmRewind(stream->node.source);
        stream->writeOffset = 0;
        filled = winstrmFill(stream, stream->bufferBytes, &endPos);
        if (filled < stream->bufferBytes) {
            stream->flags |= kWinstrmFlagDraining;
            stream->flags |= kWinstrmFlagPrimed;
            stream->endOffset = endPos;
            stream->drain = endPos;
        } else {
            stream->flags &= (uint16_t)~kWinstrmFlagDraining;
            stream->flags |= kWinstrmFlagPrimed;
        }
    } else if ((stream->flags & kWinstrmFlagDraining) != 0) {
        stream->drain = stream->endOffset;
    }
    stream->lastPlay = 0;
    stream->played = 0;
    stream->state = kWinstrmPlaying;
    requireOk(stream->buffer->vtbl->play(stream->buffer, 0, 0, kDsPlayLooping), 0x2b5);
    return 0;
}

BMX_ADDR(0x00471a20)
int winstrmStop(WinstrmStream *stream)
{
    requireInitialized(0x2c4);
    stream->state = kWinstrmStopped;
    stream->fadeFlags &= ~(uint32_t)kWinstrmFadeActive;
    requireOk(stream->buffer->vtbl->stop(stream->buffer), 0x2c9);
    return 0;
}

BMX_ADDR(0x00471aa0)
int winstrmPause(WinstrmStream *stream, int pause)
{
    requireInitialized(0x2d7);
    if (pause != 0) {
        stream->pauseCount++;
    } else {
        stream->pauseCount--;
    }
    if (stream->pauseCount != 0) {
        stream->state = kWinstrmPaused;
        requireOk(stream->buffer->vtbl->stop(stream->buffer), 0x2e1);
    } else {
        stream->state = kWinstrmPlaying;
        requireOk(stream->buffer->vtbl->play(stream->buffer, 0, 0, kDsPlayLooping), 0x2e5);
    }
    return 0;
}

BMX_ADDR(0x00471b60)
int winstrmSetVolumePan(WinstrmStream *stream, uint8_t volume, int8_t pan)
{
    requireInitialized(0x2f5);
    stream->fadeFlags = 0;
    requireOk(stream->buffer->vtbl->setVolume(stream->buffer, gDsVolume[volume]), 0x2fc);
    stream->volume = volume;
    requireOk(stream->buffer->vtbl->setPan(stream->buffer, pan * 0x4e), 0x300);
    stream->pan = pan;
    return 0;
}

BMX_ADDR(0x00471c40)
int winstrmFade(WinstrmStream *stream, int millis, uint32_t target, uint32_t flags)
{
    uint32_t ticks;
    int32_t level, goal;

    requireInitialized(0x311);
    ticks = (uint32_t)((uint32_t)(gWinstrmTicksPerSec * (uint32_t)millis)) / 1000u;
    if (ticks < 1) {
        ticks = 1;
    }
    goal = (int32_t)((target & 0xff) << 16);
    level = (int32_t)((uint32_t)stream->volume << 16);
    stream->fadeTarget = goal;
    stream->fadeLevel = level;
    stream->fadeStep = (goal - level) / (int32_t)ticks;
    stream->fadeFlags = (flags & 2) | 1;
    return 0;
}

BMX_ADDR(0x00471ce0)
int winstrmPauseAll(int pause)
{
    if (pause != 0) {
        gWinstrmPaused++;
        return 0;
    }
    gWinstrmPaused--;
    return 0;
}

BMX_ADDR(0x00471d10)
int winstrmSetFlag(uint32_t *flag)
{
    *flag = 1;
    return 0;
}

BMX_ADDR(0x00471d30)
void winstrmPlayOnce(const WinloadSampleFile *file, uint32_t bufferBytes, uint8_t volume, int8_t pan)
{
    WinstrmStream *stream;

    requireInitialized(0x345);
    if (winstrmCreate(&stream, file, 0, bufferBytes, 2) == 0) {
        if (winstrmSetVolumePan(stream, volume, pan) == 0) {
            winstrmStart(stream);
        }
    }
}

BMX_ADDR(0x00471dc0)
uint32_t winstrmFill(WinstrmStream *stream, uint32_t bytes, uint32_t *endPos)
{
    void *ptr1, *ptr2;
    uint32_t bytes1, bytes2;
    uint32_t produced = 0;
    uint8_t *dst;
    WinstrmNode *node;
    DsBuffer *buffer;
    DsResult hr;

    requireInitialized(0x364);
    if (bytes == 0) {
        return 0;
    }
    gWinEnterCriticalSection(gWinstrmLock);
    buffer = stream->buffer;
    hr = buffer->vtbl->lock(buffer, stream->writeOffset, bytes, &ptr1, &bytes1, &ptr2, &bytes2, 0);
    if (hr != 0) {
        buffer->vtbl->restore(buffer);
        hr = buffer->vtbl->lock(buffer, stream->writeOffset, bytes, &ptr1, &bytes1, &ptr2, &bytes2, 0);
        requireOk(hr, 0x36d);
    }
    if (bytes2 != 0) {
        _assert("dwLen2 == 0", winstrmFile, 0x36f);
    }
    node = &stream->node;
    dst = ptr1;
    {
        uint32_t left = bytes;

        for (;;) {
            const uint32_t got = winstrmRead(node->source, dst, left);

            produced += got;
            dst += got;
            if (got >= left) {
                break;
            }
            left -= got;
            if (node->repeat == 0) {
                WinstrmNode *next = node->next;

                if (next == 0) {
                    break;
                }
                node->source = next->source;
                node->repeat = next->repeat;
                node->next = next->next;
                winstrmNodeFree(next);
            } else {
                node->repeat--;
                if (node->repeat != 0) {
                    winstrmRewind(node->source);
                }
            }
        }
    }
    if (produced < bytes) {
        /* Pad with silence: 0x80 for 8 bit data, 0 for 16 bit. */
        const uint8_t silence = node->source->bits != 16 ? 0x80 : 0;
        uint32_t i;

        for (i = 0; i < bytes - produced; i++) {
            dst[i] = silence;
        }
        *endPos = stream->writeOffset + produced;
    }
    requireOk(buffer->vtbl->unlock(buffer, ptr1, bytes1, ptr2, bytes2), 0x395);
    stream->writeOffset += bytes;
    if (stream->writeOffset >= stream->bufferBytes) {
        stream->writeOffset -= stream->bufferBytes;
    }
    gWinLeaveCriticalSection(gWinstrmLock);
    return produced;
}

BMX_ADDR(0x00472030)
WinstrmSource *winstrmOpenSource(const WinloadSampleFile *file, uint32_t flags)
{
    WinstrmSource *source;
    WinHandle handle;
    int slot = gWinstrmSourceHint;

    while (gWinstrmSources[slot].size != 0) {
        slot++;
        if (slot == kWinstrmMaxSources) {
            slot = 0;
        }
        if (slot == gWinstrmSourceHint) {
            return 0;
        }
    }
    source = &gWinstrmSources[slot];
    gWinstrmSourceHint = slot + 1;
    if (gWinstrmSourceHint == kWinstrmMaxSources) {
        gWinstrmSourceHint = 0;
    }
    source->flags = 0;
    handle =
        gWinCreateFile(file->name, kWinGenericRead, kWinFileShareRead, 0, kWinOpenExisting,
                       ((file->offset != 0 ? kWinFileFlagSequentialScan : 0) + kWinFileFlagSequentialScan) |
                           kWinFileAttributeNormal,
                       0);
    source->file = handle;
    if (handle != kWinInvalidHandle) {
        if (gWinSetFilePointer(handle, (int32_t)file->offset, 0, kWinFileBegin) == file->offset) {
            source->startOffset = file->offset;
            source->size = file->size;
            source->channels = (uint8_t)((flags & kWinstrmFmtStereo) != 0) + 1;
            if ((flags & kWinstrmFmtRate44k) != 0) {
                source->rate = 44100;
            } else if ((flags & kWinstrmFmtRate16k) != 0) {
                source->rate = 16000;
            } else {
                source->rate = (flags & kWinstrmFmtRate11k) != 0 ? 11025 : 22050;
            }
            source->bits = (flags & kWinstrmFmt8Bit) != 0 ? 8 : 16;
            if ((flags & kWinstrmFmtPcm) == 0) {
                source->channels = 1;
                source->flags |= kWinstrmSourceAdpcm;
                source->step = 7;
                source->predictor = 0;
                source->index = 0;
            }
            source->position = 0;
            return source;
        }
        gWinCloseHandle(handle);
    }
    /* Original quirk: the cleanup frees the slot before the one that was taken. */
    slot = slot == 0 ? kWinstrmMaxSources - 1 : slot - 1;
    gWinstrmSourceHint = slot;
    gWinstrmSources[slot].size = 0;
    return 0;
}

BMX_ADDR(0x00472200)
void winstrmCloseSource(WinstrmSource *source)
{
    int i;

    gWinCloseHandle(source->file);
    for (i = 0; i < kWinstrmMaxSources; i++) {
        if (source == &gWinstrmSources[i]) {
            gWinstrmSourceHint = i;
            gWinstrmSources[i].size = 0;
            return;
        }
    }
}

BMX_ADDR(0x00472250)
uint32_t winstrmRead(WinstrmSource *source, uint8_t *dst, uint32_t bytes)
{
    uint8_t buffer[0x4000];
    uint32_t read = 0;
    uint32_t produced = 0;
    uint32_t budget, chunk;

    if ((source->flags & kWinstrmSourceAdpcm) == 0) {
        chunk = source->size - source->position;
        if (bytes < chunk) {
            chunk = bytes;
        }
        if (chunk != 0) {
            gWinReadFile(source->file, dst, chunk, &read, 0);
            source->position += read;
        }
        return read;
    }
    /* Every ADPCM byte expands to 4 output bytes. */
    budget = bytes >> 2;
    for (;;) {
        chunk = source->size - source->position;
        if ((budget < chunk ? budget : chunk) > 0x4000) {
            chunk = 0x4000;
        } else {
            if (budget < chunk) {
                chunk = budget;
            }
            if (chunk == 0) {
                break;
            }
        }
        gWinReadFile(source->file, buffer, chunk, &read, 0);
        source->position += chunk;
        winstrmDecodeAdpcm((int16_t *)dst, buffer, read * 2, &source->predictor, &source->step,
                           &source->index);
        budget -= chunk;
        dst += read * 4;
        produced += read * 4;
    }
    return produced;
}

BMX_ADDR(0x004723b0)
int winstrmRewind(WinstrmSource *source)
{
    gWinSetFilePointer(source->file, (int32_t)source->startOffset, 0, kWinFileBegin);
    source->position = 0;
    if ((source->flags & kWinstrmSourceAdpcm) != 0) {
        source->predictor = 0;
        source->step = 7;
        source->index = 0;
    }
    return 0;
}

BMX_ADDR(0x00472400)
WinstrmNode *winstrmNodeAlloc(void)
{
    int slot = gWinstrmNodeHint;

    /* A node is free while its repeat count is 0. */
    while (gWinstrmNodes[slot].repeat != 0) {
        slot++;
        if (slot == kWinstrmMaxNodes) {
            slot = 0;
        }
        if (slot == gWinstrmNodeHint) {
            return 0;
        }
    }
    gWinstrmNodeHint = slot + 1 != kWinstrmMaxNodes ? slot + 1 : 0;
    return &gWinstrmNodes[slot];
}

BMX_ADDR(0x00472480)
void winstrmNodeFree(WinstrmNode *node)
{
    int i;

    for (i = 0; i < kWinstrmMaxNodes; i++) {
        if (node == &gWinstrmNodes[i]) {
            gWinstrmNodeHint = i;
            gWinstrmNodes[i].repeat = 0;
            return;
        }
    }
}

BMX_ADDR(0x004724c0)
void winstrmDecodeAdpcm(int16_t *dst, const uint8_t *src, uint32_t count, int32_t *predictor, uint32_t *step,
                        int32_t *index)
{
    int32_t pred = *predictor;
    int32_t stepSize = (int32_t)*step;
    int32_t idx = *index;
    uint8_t byte = 0;
    uint32_t i;

    for (i = 0; i != count; i++) {
        uint32_t nibble;
        int32_t diff = 0;

        if ((i & 1) == 0) {
            byte = src[i >> 1];
            nibble = byte & 0xf;
        } else {
            nibble = byte >> 4;
        }
        if ((nibble & 4) != 0) {
            diff += stepSize;
        }
        if ((nibble & 2) != 0) {
            diff += stepSize >> 1;
        }
        if ((nibble & 1) != 0) {
            diff += stepSize >> 2;
        }
        diff += stepSize >> 3;
        if ((nibble & 8) != 0) {
            diff = -diff;
        }
        pred += diff;
        if (pred > 0x7fff) {
            pred = 0x7fff;
        } else if (pred < -0x8000) {
            pred = -0x8000;
        }
        dst[i] = (int16_t)pred;
        idx += gAdpcmIndex[nibble];
        if (idx > 0x58) {
            idx = 0x58;
        } else if (idx < 0) {
            idx = 0;
        }
        stepSize = gAdpcmStep[idx];
    }
    *predictor = pred;
    *step = (uint32_t)stepSize;
    *index = idx;
}
