/* Windows sound channels (C:\zaxis\Bmx\src\Pc\zge\sound\winChan.c). */
#include "bmx/pc/zge/sound/winchan.h"

#include "bmx/pc/zge/sound/winapi.h"
#include "compat/crt.h"
#include "compat/x87.h"

/*
 * Floating point follows the original x87 code: `double` is a value held in an x87 register, `float` is a
 * value the original stored to memory.
 */

static const char winchanFile[] = "C:\\zaxis\\Bmx\\src\\Pc\\zge\\sound\\winChan.c";

/* Exe tables: volume curve, envelope times (ms), semitone and cent pitch factors (12 bit fixed point). */
typedef float GainTable[128];
/* The envelope table has 128 entries; voice times up to 255 read on into the following data. */
typedef uint16_t EnvelopeTable[256];
typedef uint16_t SemitoneTable[73];
typedef uint16_t CentTable[100];
#define gGainCurve   ORIG_VAR(0x0055e97c, GainTable, gGainCurve)
#define gEnvelopeMs  ORIG_VAR(0x0055eb7c, EnvelopeTable, gEnvelopeMs)
#define gSemitones   ORIG_VAR(0x0055e820, SemitoneTable, gSemitones)
#define gCents       ORIG_VAR(0x0055e8b4, CentTable, gCents)
#define kEnvelopeInv ((float)-127.0)

#define kInvRange127 0x1.020408p-7f

static void requireInitialized(int line)
{
    if (gWinchanInitialized == 0) {
        _assert("fzsndcInitialized", winchanFile, (unsigned)line);
    }
}

/* ---- console port stubs ---- */

/* TODO(file): these 19 stubs (0x0046d6e0 .. 0x0046d920) probably belong to a card API source file. */
BMX_ADDR(0x0046d6e0)
int winchanCardStub6e0(void)
{
    return 0;
}

BMX_ADDR(0x0046d700)
void winchanCardStub700(void) {}

BMX_ADDR(0x0046d720)
void winchanStart(uint32_t slot) {}

BMX_ADDR(0x0046d740)
int winchanCardStub740(void)
{
    return 0;
}

BMX_ADDR(0x0046d760)
int winchanCardStub760(void)
{
    return 0;
}

BMX_ADDR(0x0046d780)
int winchanCardStub780(void)
{
    return 0;
}

BMX_ADDR(0x0046d7a0)
int winchanSetCallback(WinchanCallback callback)
{
    return 0;
}

BMX_ADDR(0x0046d7c0)
int winchanCardStub7c0(void)
{
    return 0;
}

BMX_ADDR(0x0046d7e0)
int winchanCardStub7e0(void)
{
    return 0;
}

BMX_ADDR(0x0046d800)
int winchanCardStub800(void)
{
    return 0;
}

BMX_ADDR(0x0046d820)
int winchanCardStub820(void)
{
    return 0;
}

BMX_ADDR(0x0046d840)
int winchanCardStub840(void)
{
    return 0;
}

BMX_ADDR(0x0046d860)
int winchanCardStub860(void)
{
    return 0;
}

BMX_ADDR(0x0046d880)
int winchanCardStub880(void)
{
    return 0;
}

BMX_ADDR(0x0046d8a0)
int winchanQuery(void)
{
    return 0;
}

BMX_ADDR(0x0046d8c0)
void winchanBegin(void) {}

BMX_ADDR(0x0046d8e0)
void winchanEnd(void) {}

BMX_ADDR(0x0046d900)
void winchanCardStub900(void) {}

BMX_ADDR(0x0046d920)
int winchanCardStub920(void)
{
    return 0;
}

/* ---- handles and channels ---- */

BMX_ADDR(0x0046f470)
WinchanChannel *winchanChannelOf(uint8_t handle)
{
    if (handle > kWinchanNumHandles - 1) {
        _assert("hChan < kzsndNumChannelHandles", winchanFile, 0x4c1);
    }
    if (gWinchanHandles[handle].allocated == 0) {
        _assert("fzsndcHandle[hChan].allocated", winchanFile, 0x4c2);
    }
    if (gWinchanHandles[handle].channelNum != kWinchanNoChannel) {
        if (gWinchanHandles[handle].channelNum > kWinchanNumChannels - 1) {
            _assert("fzsndcHandle[hChan].channelNum < kzsndNumChannels", winchanFile, 0x4c4);
        }
        return &gWinchanChannels[gWinchanHandles[handle].channelNum];
    }
    return 0;
}

BMX_ADDR(0x0046f530)
void winchanChannelRelease(WinchanChannel *channel)
{
    channel->state = kWinchanFree;
    gWinchanHandles[channel->handle].channelNum = kWinchanNoChannel;
    if ((channel->flags & kWinchanFlagFreeHandle) != 0) {
        gWinchanHandles[channel->handle].allocated = 0;
    }
    gWinchanActive--;
}

BMX_ADDR(0x0046f580)
double winchanEnvelopeStep(int32_t amount, int32_t millis)
{
    if (millis != 0) {
        return ((double)amount * (double)1000.0f) /
               (((double)gWinchanRate * (double)millis) * (double)127.0f);
    }
    return (double)amount * (double)kInvRange127;
}

/* Frames of an envelope phase that lasts `millis` (the product wraps at 32 bits like the original). */
static uint32_t envelopeFrames(uint16_t millis)
{
    return (uint32_t)((uint32_t)millis * gWinchanRate) / 1000u;
}

BMX_ADDR(0x0046e410)
uint8_t winchanVoiceAlloc(const ZsdVoice *voice, int priority)
{
    const WinloadSampleFile *sampleFile = 0;
    WinchanChannel *channel;
    uint32_t handle, i;
    int chosen, best;
    uint8_t bestPriority;

    if (voice != 0) {
        const ZsdSample *sample;

        if (voice->sample == kWinloadNoSample) {
            return kWinchanNoHandle;
        }
        sample = winloadGetSample(voice->file, voice->sample);
        if (sample->sampleFile == kWinloadNoSample) {
            return kWinchanNoHandle;
        }
        sampleFile = winloadGetSampleFile(voice->file, sample->sampleFile);
    }
    gWinchanLock++;
    handle = (uint32_t)gWinchanNextHandle;
    while (gWinchanHandles[handle].allocated != 0) {
        handle++;
        if (handle == kWinchanNumHandles) {
            handle = 0;
        }
        if (handle == (uint32_t)gWinchanNextHandle) {
            _assert("i != fzsndcNextChannelHandleNum", winchanFile, 0x2b2);
        }
    }
    gWinchanHandles[handle].allocated = 1;
    gWinchanHandles[handle].channelNum = kWinchanNoChannel;
    gWinchanNextHandle = (int32_t)handle + 1;
    if (gWinchanNextHandle == kWinchanNumHandles) {
        gWinchanNextHandle = 0;
    }

    /* Look for a free channel; remember the one with the lowest priority in case of a steal. */
    bestPriority = 0xff;
    best = gWinchanNextChannel;
    chosen = gWinchanNextChannel;
    channel = 0;
    while (gWinchanChannels[chosen].state != kWinchanFree) {
        if (gWinchanChannels[chosen].priority <= bestPriority) {
            best = chosen;
            bestPriority = gWinchanChannels[chosen].priority;
        }
        chosen++;
        if (chosen == kWinchanNumChannels) {
            chosen = 0;
        }
        if (chosen == gWinchanNextChannel) {
            goto steal;
        }
    }
    channel = &gWinchanChannels[chosen];
    gWinchanNextChannel = chosen + 1;
    if (gWinchanNextChannel == kWinchanNumChannels) {
        gWinchanNextChannel = 0;
    }
    goto setup;

steal:
    if ((uint8_t)priority < bestPriority) {
        gWinchanHandles[handle & 0xff].allocated = 0;
        gWinchanLock--;
        return kWinchanNoHandle;
    }
    chosen = best;
    channel = &gWinchanChannels[chosen];
    winchanChannelRelease(channel);
    gWinchanNextChannel = chosen + 1;
    if (gWinchanNextChannel == kWinchanNumChannels) {
        gWinchanNextChannel = 0;
    }

setup:
    channel->priority = (uint8_t)priority;
    channel->state = kWinchanStopped;
    channel->flags = 0;
    channel->handle = (uint8_t)handle;
    channel->voice = voice;
    if (voice != 0) {
        const ZsdSample *sample = winloadGetSample(voice->file, voice->sample);
        const uint32_t bytesPerFrame =
            (uint32_t)((sample->format & 4) != 0 ? 1 : 2) * (uint32_t)((sample->format & 2) != 0 ? 2 : 1);

        channel->sample = sample;
        channel->length = sampleFile->size / bytesPerFrame;
        channel->step = (uint32_t)((uint32_t)voice->pitch * sample->rate) / gWinchanRate;
        channel->position = 0;
        channel->fraction = 0;
        channel->volume = voice->volume;
        channel->pan = voice->pan;
        if (voice->volume == 0) {
            channel->gainRight = 0;
            channel->gainLeft = 0;
        } else if (voice->pan <= 0) {
            channel->gainLeft = gGainCurve[voice->volume];
            channel->gainRight = gGainCurve[((uint32_t)(voice->pan + 0x7f) * voice->volume >> 7) & 0xff];
        } else {
            channel->gainRight = gGainCurve[voice->volume];
            channel->gainLeft = gGainCurve[((uint32_t)(0x7f - voice->pan) * voice->volume >> 7) & 0xff];
        }
        channel->attackStep = (float)winchanEnvelopeStep(0x7f, gEnvelopeMs[voice->attack]);
        if (voice->decay == 0x7f || voice->sustain == 0x7f) {
            channel->decayStep = 0;
        } else {
            channel->decayStep = (float)winchanEnvelopeStep(0x7f - voice->sustain, gEnvelopeMs[voice->decay]);
        }
        channel->sustainLevel = gGainCurve[voice->sustain];
        if (voice->fade == 0x7f) {
            channel->fadeStep = 0;
        } else {
            channel->fadeStep =
                (float)winchanEnvelopeStep(-(int32_t)voice->sustain, gEnvelopeMs[voice->fade]);
        }
        gWinchanActive++;
    }
    i = handle & 0xff;
    gWinchanHandles[i].channelNum = (uint8_t)chosen;
    gWinchanLock--;
    return (uint8_t)handle;
}

BMX_ADDR(0x0046e890)
int winchanVoiceFree(uint8_t channel)
{
    winchanVoiceStop(channel, 2);
    return 0;
}

BMX_ADDR(0x0046e8c0)
int winchanVoicePlay(const ZsdVoice *voice, int16_t cents, uint8_t volume, int8_t pan, int priority)
{
    uint8_t handle;

    requireInitialized(0x332);
    if (voice != 0) {
        gWinchanLock++;
        handle = winchanVoiceAlloc(voice, priority);
        if (handle != kWinchanNoHandle) {
            winchanVoiceSetVolume(handle, volume);
            winchanVoiceSetPan(handle, pan);
            winchanVoiceSetPitch(handle, cents);
            winchanVoiceStart(handle);
            gWinchanChannels[gWinchanHandles[handle].channelNum].flags |= kWinchanFlagFreeHandle;
        }
        gWinchanLock--;
    }
    return 0;
}

BMX_ADDR(0x0046e9a0)
int winchanVoiceStart(uint8_t channel)
{
    WinchanChannel *ch;

    requireInitialized(0x351);
    gWinchanLock++;
    ch = winchanChannelOf(channel);
    if (ch != 0) {
        const ZsdVoice *voice = ch->voice;

        if (winloadGetSample(voice->file, voice->sample) == 0) {
            _assert("sample", winchanFile, 0x357);
        }
        ch->position = 0;
        ch->fraction = 0;
        if (voice->attack > 0) {
            ch->level = 0;
            ch->levelStep = ch->attackStep;
            ch->phaseFrames = envelopeFrames(gEnvelopeMs[voice->attack]);
            ch->phase = kWinchanPhaseAttack;
        } else if (voice->decay > 0) {
            ch->level = 1.0f;
            ch->levelStep = ch->decayStep;
            ch->phaseFrames = envelopeFrames(gEnvelopeMs[voice->decay]);
            ch->phase = kWinchanPhaseDecay;
        } else {
            ch->level = ch->sustainLevel;
            ch->levelStep = ch->fadeStep;
            if (voice->fade < 0x7f) {
                uint32_t frames = envelopeFrames(gEnvelopeMs[voice->fade]);

                ch->phaseFrames = frames < 1 ? 1 : frames;
            } else {
                ch->phaseFrames = 0;
            }
            ch->phase = kWinchanPhaseSustain;
        }
        ch->state = kWinchanPlaying;
    }
    gWinchanLock--;
    return 0;
}

BMX_ADDR(0x0046eb60)
int winchanVoiceStop(uint8_t channel, uint8_t flags)
{
    WinchanChannel *ch;

    requireInitialized(0x392);
    gWinchanLock++;
    ch = winchanChannelOf(channel);
    if (ch == 0) {
        if ((flags & 2) != 0) {
            gWinchanHandles[channel].allocated = 0;
        }
    } else {
        const ZsdVoice *voice = ch->voice;

        if ((flags & 1) == 0 && voice->release != 0) {
            ch->levelStep = (float)winchanEnvelopeStep(bmxFtol((double)ch->level * (double)kEnvelopeInv),
                                                       gEnvelopeMs[voice->release]);
            ch->phaseFrames = envelopeFrames(gEnvelopeMs[voice->release]);
            ch->phase = kWinchanPhaseRelease;
        } else {
            ch->state = kWinchanStopped;
        }
        if ((flags & 2) != 0) {
            if (ch->state == kWinchanStopped) {
                winchanChannelRelease(ch);
                gWinchanHandles[ch->handle].allocated = 0;
                gWinchanLock--;
                return 0;
            }
            ch->flags |= kWinchanFlagFreeHandle;
            gWinchanLock--;
            return 0;
        }
    }
    gWinchanLock--;
    return 0;
}

BMX_ADDR(0x0046ecd0)
int winchanVoiceStatus(uint8_t channel)
{
    const WinchanChannel *ch = winchanChannelOf(channel);

    if (ch != 0) {
        if (ch->state == kWinchanStopped) {
            return 1;
        }
        if (ch->state == kWinchanPlaying) {
            return 2;
        }
        if (ch->state == kWinchanFinishing) {
            return 3;
        }
    }
    return 0;
}

/* Left and right gains for a pan position: the far side is attenuated, the near one keeps the volume. */
static void setPanGains(WinchanChannel *ch, int8_t pan)
{
    if (pan >= 0) {
        ch->gainRight = gGainCurve[ch->volume];
        ch->gainLeft = gGainCurve[((uint32_t)(0x7f - pan) * ch->volume >> 7) & 0xff];
    } else {
        ch->gainLeft = gGainCurve[ch->volume];
        ch->gainRight = gGainCurve[((uint32_t)(pan + 0x7f) * ch->volume >> 7) & 0xff];
    }
}

BMX_ADDR(0x0046ed20)
int winchanVoiceSetVolume(uint8_t channel, uint8_t volume)
{
    WinchanChannel *ch;

    requireInitialized(0x3e1);
    gWinchanLock++;
    ch = winchanChannelOf(channel);
    if (ch != 0) {
        ch->volume = (uint8_t)((uint32_t)ch->voice->volume * volume >> 7);
        if (ch->volume != 0) {
            setPanGains(ch, ch->pan);
        } else {
            ch->gainRight = 0;
            ch->gainLeft = 0;
        }
    }
    gWinchanLock--;
    return 0;
}

BMX_ADDR(0x0046ee80)
int winchanVoiceSetPan(uint8_t channel, int8_t pan)
{
    WinchanChannel *ch;

    requireInitialized(0x407);
    gWinchanLock++;
    ch = winchanChannelOf(channel);
    if (ch != 0) {
        int32_t sum = (int32_t)ch->voice->pan + pan;

        if (sum < -0x7f) {
            sum = -0x7f;
        } else if (sum >= 0x7f) {
            sum = 0x7f;
        }
        ch->pan = (int8_t)sum;
        setPanGains(ch, (int8_t)sum);
    }
    gWinchanLock--;
    return 0;
}

BMX_ADDR(0x0046efb0)
int winchanVoiceSetPitch(uint8_t channel, int16_t cents)
{
    WinchanChannel *ch;
    const ZsdSample *sample;
    uint32_t rate;

    requireInitialized(0x428);
    gWinchanLock++;
    ch = winchanChannelOf(channel);
    if (ch != 0) {
        sample = winloadGetSample(ch->voice->file, ch->voice->sample);
        if (sample == 0) {
            _assert("sample", winchanFile, 0x42d);
        }
        rate = (uint32_t)((uint32_t)ch->voice->pitch * sample->rate) >> 12;
        if (cents != 0) {
            int32_t octave = cents / 100;
            int32_t rest = cents % 100;

            if (rest < 0) {
                const uint32_t borrow = (99u - (uint32_t)rest) / 100u;

                octave -= (int32_t)borrow;
                rest += (int32_t)(borrow * 100u);
            }
            if (octave < -0x24) {
                octave = -0x24;
            } else if (octave > 0x23) {
                octave = 0x24;
            }
            rate = (uint32_t)((int32_t)((uint32_t)gSemitones[octave + 0x24] * rate) >> 12);
            rate = (uint32_t)((int32_t)(rate * (uint32_t)gCents[rest]) >> 12);
        }
        if (rate == gWinchanRate) {
            ch->step = 0x1000;
            ch->flags |= kWinchanFlagNativeRate;
            gWinchanLock--;
            return 0;
        }
        ch->step = (rate << 12) / gWinchanRate;
        ch->flags &= (uint8_t)~kWinchanFlagNativeRate;
    }
    gWinchanLock--;
    return 0;
}

BMX_ADDR(0x0046f170)
const ZsdVoice *winchanVoiceGetVoice(uint8_t channel)
{
    const WinchanChannel *ch;

    requireInitialized(0x455);
    ch = winchanChannelOf(channel);
    if (ch != 0) {
        return ch->voice;
    }
    return 0;
}

BMX_ADDR(0x0046f1c0)
void winchanStopVoice(const ZsdVoice *voice)
{
    int i;

    requireInitialized(0x467);
    gWinchanLock++;
    for (i = 0; i < kWinchanNumChannels; i++) {
        if (gWinchanChannels[i].state != kWinchanFree && gWinchanChannels[i].voice == voice) {
            winchanVoiceStop(gWinchanChannels[i].handle, 3);
        }
    }
    gWinchanLock--;
}

BMX_ADDR(0x0046e3c0)
void winchanFreeAllChannels(void)
{
    int i;

    for (i = 0; i < kWinchanNumHandles + 1; i++) {
        if (gWinchanHandles[i].allocated != 0) {
            winchanVoiceFree((uint8_t)i);
        }
    }
    gWinchanNextChannel = 0;
}

BMX_ADDR(0x0046f250)
uint32_t winchanSetEffect(const WinchanEffect *effect)
{
    if (effect->kind == 0) {
        const uint32_t frames = (uint32_t)((uint32_t)effect->millis * gWinchanRate) / 1000u;

        gWinchanEffectAFrames = gWinchanEffectMaxFrames < frames ? gWinchanEffectMaxFrames : frames;
        gWinchanEffectAGain = (float)((double)(int32_t)(int8_t)effect->level * (double)kInvRange127);
        gWinchanEffectAParam = effect->param.raw;
        return 0;
    }
    if (effect->kind != 1) {
        return 0x10d;
    }
    {
        uint32_t frames = (uint32_t)((uint32_t)effect->millis * gWinchanRate) / 1000u;

        if (frames > 0x400) {
            frames = 0x400;
        }
        gWinchanEffectBFrames = frames;
        gWinchanEffectBGain = (float)((double)(int32_t)(int8_t)effect->level * (double)kInvRange127);
        gWinchanEffectBTailStart = bmxFtol((double)(uint64_t)(0x400 - frames) * (double)effect->param.scale);
        gWinchanEffectBTail = bmxFtol((double)effect->tail * 4096.0);
    }
    return 0;
}

BMX_ADDR(0x0046f380)
uint32_t winchanProbeDevice(void *window)
{
    typedef int32_t(BMX_STDCALL * DirectSoundCreateFn)(void *guid, DsDevice **device, void *outer);
    uint32_t rc;

    gWinchanDsoundModule = gWinLoadLibrary("DSOUND.DLL");
    if (gWinchanDsoundModule == 0) {
        return 0x101;
    }
    gWinchanDsoundCreate = gWinGetProcAddress(gWinchanDsoundModule, "DirectSoundCreate");
    if (gWinchanDsoundCreate == 0) {
        rc = 0x102;
    } else {
        if (((DirectSoundCreateFn)gWinchanDsoundCreate)(0, &gWinchanDirectSound, 0) == 0) {
            if (window == 0) {
                rc = 0;
            } else {
                rc = gWinchanDirectSound->vtbl->setCooperativeLevel(gWinchanDirectSound, window, 1) != 0
                         ? 0x105
                         : 0;
            }
            gWinchanDirectSound->vtbl->release(gWinchanDirectSound);
            gWinchanDirectSound = 0;
        } else {
            rc = 0x107;
        }
        gWinchanDsoundCreate = 0;
    }
    gWinFreeLibrary(gWinchanDsoundModule);
    return rc;
}

BMX_ADDR(0x0046df20)
void winchanShutdown(void)
{
    int i;

    requireInitialized(0x1d1);
    for (i = 0; i < kWinchanNumHandles; i++) {
        if (gWinchanHandles[i].allocated != 0) {
            winchanVoiceFree((uint8_t)i);
        }
    }
    gWinchanBuffer->vtbl->stop(gWinchanBuffer);
    gWinchanBuffer->vtbl->release(gWinchanBuffer);
    gWinchanPrimary->vtbl->stop(gWinchanPrimary);
    gWinchanPrimary->vtbl->release(gWinchanPrimary);
    gWinchanDirectSound->vtbl->release(gWinchanDirectSound);
    gWinchanDirectSound = 0;
    gWinFreeLibrary(gWinchanDsoundModule);
    free(gWinchanRing);
    gWinchanInitialized = 0;
}

/* Copies `bytes` of the ring starting at the copy offset to `dst` (wrapping), advancing the offset. */
static void copyFromRing(uint8_t *dst, uint32_t bytes)
{
    const uint32_t total = gWinchanRingBytes;

    if (gWinchanCopyOffset + bytes < total) {
        memcpy(dst, gWinchanRing + gWinchanCopyOffset, bytes);
        gWinchanCopyOffset += bytes;
    } else {
        const uint32_t head = total - gWinchanCopyOffset;

        memcpy(dst, gWinchanRing + gWinchanCopyOffset, head);
        memcpy(dst + head, gWinchanRing, gWinchanCopyOffset - total + bytes);
        gWinchanCopyOffset += bytes - total;
    }
}

BMX_ADDR(0x0046e010)
uint32_t winchanPump(void)
{
    uint32_t status, play, write, last, total, chunk, elapsed, w, mixOffset;
    uint32_t result = 0;

    if (gWinchanLock != 0) {
        return 0;
    }
    gWinchanPumpCount++;
    if (gWinchanBuffer->vtbl->getStatus(gWinchanBuffer, &status) != 0) {
        return 0x10c;
    }
    if ((status & kDsStatusPlaying) == 0) {
        return result;
    }
    gWinchanBuffer->vtbl->getCurrentPosition(gWinchanBuffer, &play, &write);
    last = gWinchanLastPlay;
    total = gWinchanRingBytes;
    chunk = gWinchanChunkBytes;
    if (play < last) {
        elapsed = total - last + play;
        if (elapsed > chunk) {
            gWinchanWriteOffset += chunk;
            if (gWinchanWriteOffset >= total) {
                gWinchanWriteOffset -= total;
            }
            gWinchanCopyOffset += chunk;
            if (gWinchanCopyOffset >= total) {
                gWinchanCopyOffset -= total;
            }
        }
    } else {
        elapsed = play - last;
    }
    w = gWinchanWriteOffset;
    gWinchanLastPlay = play;
    /* Refill the ring buffer when the write offset is inside the next chunk behind the play cursor, or
     * far behind it. */
    if ((w > play && w < play + chunk) || (w < play && w + chunk < play)) {
        void *ptr1, *ptr2;
        uint32_t bytes1, bytes2;
        DsResult hr = gWinchanBuffer->vtbl->lock(gWinchanBuffer, w, chunk, &ptr1, &bytes1, &ptr2, &bytes2, 0);

        if (hr == kDsErrBufferLost) {
            gWinchanBuffer->vtbl->restore(gWinchanBuffer);
            hr = gWinchanBuffer->vtbl->lock(gWinchanBuffer, gWinchanWriteOffset, gWinchanChunkBytes, &ptr1,
                                            &bytes1, &ptr2, &bytes2, 0);
        }
        if (hr == 0) {
            copyFromRing(ptr1, bytes1);
            gWinchanBuffer->vtbl->unlock(gWinchanBuffer, ptr1, bytes1, ptr2, bytes2);
            gWinchanWriteOffset += gWinchanChunkBytes;
            if (gWinchanWriteOffset >= gWinchanRingBytes) {
                gWinchanWriteOffset -= gWinchanRingBytes;
            }
        } else {
            total = gWinchanRingBytes;
            result = 0x109;
        }
    }
    /* Mix the elapsed time into the ring, behind what was mixed before. */
    mixOffset = gWinchanMixOffset;
    if (mixOffset + elapsed < total) {
        winchanMix(gWinchanRing + mixOffset, elapsed);
        gWinchanMixOffset += elapsed;
    } else {
        winchanMix(gWinchanRing + mixOffset, total - mixOffset);
        winchanMix(gWinchanRing, gWinchanMixOffset - gWinchanRingBytes + elapsed);
        gWinchanMixOffset += elapsed - gWinchanRingBytes;
    }
    return result;
}

/* Frees the device on an error after DirectSoundCreate. */
static int releaseDevice(int rc)
{
    gWinchanDirectSound->vtbl->release(gWinchanDirectSound);
    gWinchanDirectSound = 0;
    return rc;
}

/* Releases the primary buffer and the device on an error after the primary buffer exists. */
static int releasePrimary(int rc)
{
    gWinchanPrimary->vtbl->release(gWinchanPrimary);
    return releaseDevice(rc);
}

/* WAVEFORMATEX */
typedef struct WinchanWaveFormatEx {
    DsWaveFormat pcm;
    uint16_t extraBytes;
} WinchanWaveFormatEx;

typedef DsResult(BMX_STDCALL *DsSetFormatFn)(DsBuffer *self, const WinchanWaveFormatEx *format);

/* winchanInit after the device exists: cooperative levels, primary format, the looping output buffer. */
static int createBuffers(const WinchanInitDesc *desc)
{
    WinchanWaveFormatEx format;
    DsBufferDesc bufferDesc;
    void *ptr1, *ptr2;
    uint32_t bytes1, bytes2;
    uint32_t rate;

    if (desc->window == 0) {
        _assert("init->hWnd", winchanFile, 0x16a);
    }
    if (gWinchanDirectSound->vtbl->setCooperativeLevel(gWinchanDirectSound, desc->window, 1) != 0) {
        return releaseDevice(0x105);
    }
    memset(&bufferDesc, 0, sizeof bufferDesc);
    bufferDesc.size = sizeof bufferDesc;
    bufferDesc.flags = 1; /* DSBCAPS_PRIMARYBUFFER */
    if (gWinchanDirectSound->vtbl->createSoundBuffer(gWinchanDirectSound, &bufferDesc, &gWinchanPrimary, 0) !=
        0) {
        return releaseDevice(0x106);
    }
    if (gWinchanDirectSound->vtbl->setCooperativeLevel(gWinchanDirectSound, desc->window, 2) != 0) {
        return releasePrimary(0x105);
    }
    rate = desc->rate;
    if (rate == 0) {
        rate = 22050;
    }
    format.pcm.formatTag = 1;
    format.pcm.samplesPerSec = rate;
    format.extraBytes = 0;
    format.pcm.bitsPerSample = (desc->format & 1) != 0 ? 8 : 16;
    format.pcm.channels = (desc->format & 2) != 0 ? 1 : 2;
    format.pcm.blockAlign = (uint16_t)((format.pcm.bitsPerSample >> 3) * format.pcm.channels);
    format.pcm.avgBytesPerSec = format.pcm.blockAlign * rate;
    if (((DsSetFormatFn)gWinchanPrimary->vtbl->setFormat)(gWinchanPrimary, &format) != 0) {
        return releasePrimary(0x104);
    }
    gWinchanRate = rate;
    gWinchanDirectSound->vtbl->setCooperativeLevel(gWinchanDirectSound, desc->window, 1);
    if (gWinchanPrimary->vtbl->play(gWinchanPrimary, 0, 0, kDsPlayLooping) != 0) {
        return releasePrimary(0x103);
    }
    /* The output buffer is 16 bit stereo at the primary rate. */
    format.pcm.avgBytesPerSec = rate * 4;
    memset(&bufferDesc, 0, sizeof bufferDesc);
    bufferDesc.flags = kDsCapsControlFrequency | kDsCapsControlPan | kDsCapsControlVolume;
    bufferDesc.format = &format.pcm;
    format.pcm.bitsPerSample = 16;
    format.pcm.channels = 2;
    format.pcm.blockAlign = 4;
    bufferDesc.size = sizeof bufferDesc;
    bufferDesc.bufferBytes = gWinchanRingBytes;
    if (gWinchanDirectSound->vtbl->createSoundBuffer(gWinchanDirectSound, &bufferDesc, &gWinchanBuffer, 0) !=
        0) {
        return releasePrimary(0x10a);
    }
    if (gWinchanBuffer->vtbl->lock(gWinchanBuffer, 0, gWinchanRingBytes, &ptr1, &bytes1, &ptr2, &bytes2, 0) !=
        0) {
        return releasePrimary(0x109);
    }
    /* Clears a whole ring length from the first region, whatever size the lock returned. */
    memset(ptr1, 0, gWinchanRingBytes);
    gWinchanBuffer->vtbl->unlock(gWinchanBuffer, ptr1, gWinchanRingBytes, 0, 0);
    gWinchanWriteOffset = 0;
    gWinchanLastPlay = 0;
    if (gWinchanBuffer->vtbl->play(gWinchanBuffer, 0, 0, kDsPlayLooping) != 0) {
        /* The output buffer is not released. */
        return releasePrimary(0x10b);
    }
    gWinchanInitialized = 1;
    return 0;
}

BMX_ADDR(0x0046d940)
int winchanInit(WinchanInitDesc *desc)
{
    typedef int32_t(BMX_STDCALL * DirectSoundCreateFn)(void *guid, DsDevice **device, void *outer);
    int i, rc;

    for (i = 0; i < kWinchanNumHandles + 1; i++) {
        gWinchanHandles[i].allocated = 0;
        gWinchanHandles[i].channelNum = kWinchanNoChannel;
    }
    for (i = 0; i < kWinchanNumChannels; i++) {
        gWinchanChannels[i].state = kWinchanFree;
    }
    gWinchanActive = 0;
    if (desc->chunkMillis == 0) {
        desc->chunkMillis = 50;
    }
    gWinchanChunkBytes = desc->chunkMillis * desc->rate / 1000u * 4;
    gWinchanRingBytes = gWinchanChunkBytes * 2;
    gWinchanRing = malloc(gWinchanRingBytes);
    if (gWinchanRing == 0) {
        return 0x108;
    }
    memset(gWinchanRing, 0, gWinchanRingBytes);
    gWinchanMixOffset = 0;
    gWinchanCopyOffset = gWinchanRingBytes - gWinchanChunkBytes;
    gWinchanEffectMaxFrames = (uint32_t)bmxFtol((double)desc->rate * (double)desc->echoSeconds);
    /* A failure here keeps the ring buffer. */
    gWinchanEchoBuffer = malloc(gWinchanEffectMaxFrames * sizeof(WinchanFrame));
    if (gWinchanEchoBuffer == 0) {
        return 0x108;
    }
    gWinchanEffectAFrames = 0;
    gWinchanEffectAGain = 0.0f;
    gWinchanEffectAParam = 0;
    gWinchanEchoPos = 0;
    /* Raised cosine, 0 .. 4096 .. 0 over the table. */
    for (i = 0; i < 4096; i++) {
        gWinchanLfoTable[i] =
            (uint16_t)bmxFtol((0.5 - bmxX87CosScaled(i, 0x1.921fb54411744p-10) * 0.5) * 4096.0);
    }
    gWinchanEffectBFrames = 0;
    gWinchanEffectBGain = 0.0f;
    gWinchanEffectBTailStart = 0;
    gWinchanEffectBTail = 0;
    gWinchanFlangerPhase = 0;
    gWinchanFlangerPos = 0;
    gWinchanFlangerDelay = 0;
    gWinchanDsoundModule = gWinLoadLibrary("DSOUND.DLL");
    if (gWinchanDsoundModule == 0) {
        rc = 0x101;
    } else {
        gWinchanDsoundCreate = gWinGetProcAddress(gWinchanDsoundModule, "DirectSoundCreate");
        if (gWinchanDsoundCreate == 0) {
            rc = 0x102;
        } else if (((DirectSoundCreateFn)gWinchanDsoundCreate)(0, &gWinchanDirectSound, 0) != 0) {
            rc = 0x107;
        } else {
            rc = createBuffers(desc);
            if (rc == 0) {
                return 0;
            }
        }
    }
    /* Errors free the ring buffer (not the echo buffer) and leave the DLL loaded. */
    free(gWinchanRing);
    return rc;
}

/* x87 FISTP in the round-to-nearest mode: NaN and values outside int32 store 0x80000000. */
static int32_t fistp32(double v)
{
    if (!(v >= -2147483648.5 && v < 2147483647.5)) {
        return INT32_MIN;
    }
    return (int32_t)((v + 0x1.8p52) - 0x1.8p52);
}

/* An 8 bit (unsigned) sample as the 16 bit value the mixer multiplies by. */
static int32_t sample8(uint8_t v)
{
    return (int16_t)(uint16_t)(((uint32_t)v << 8) ^ 0x8000u);
}

/* Ends a channel the mixer ran out of: stopped, and released when it was flagged to free its handle. */
static void mixStop(WinchanChannel *ch)
{
    ch->state = kWinchanStopped;
    ch->phaseFrames = 0;
    if ((ch->flags & kWinchanFlagFreeHandle) != 0) {
        winchanChannelRelease(ch);
    }
}

/*
 * Advances a channel's envelope by one frame. Returns 0 when the last phase ended (the channel is
 * stopped). The level of a new phase is loaded and then overwritten by the running sum, so a phase
 * change never jumps the level (original quirk).
 */
static int mixEnvelope(WinchanChannel *ch)
{
    const double level = (double)ch->level + (double)ch->levelStep;

    ch->phaseFrames--;
    if (ch->phaseFrames == 0) {
        if ((int8_t)ch->phase >= kWinchanPhaseSustain) {
            mixStop(ch);
            return 0;
        }
        if (ch->phase == kWinchanPhaseAttack) {
            ch->phase++;
            if (ch->voice->decay != 0) {
                ch->levelStep = ch->decayStep;
                ch->phaseFrames = envelopeFrames(gEnvelopeMs[ch->voice->decay]);
                ch->level = (float)level;
                return 1;
            }
        }
        ch->phase++;
        ch->levelStep = ch->fadeStep;
        ch->phaseFrames = ch->voice->fade == 0x7f ? 0 : envelopeFrames(gEnvelopeMs[ch->voice->fade]);
    }
    ch->level = (float)level;
    return 1;
}

/* Advances a channel's sample position by its step, looping or ending at the sample's bounds. */
static void mixAdvance(WinchanChannel *ch)
{
    const ZsdSample *sample = ch->sample;
    const int32_t step = (int32_t)ch->step;

    if (step >= 0) {
        const uint32_t sum = ch->fraction + (uint32_t)step;

        ch->fraction = sum & 0xfff;
        ch->position += sum >> 12;
        if ((int32_t)ch->position >= (int32_t)ch->length) {
            if ((sample->format & 1) == 0) {
                mixStop(ch);
            } else if ((sample->format & 8) != 0) {
                ch->step = (uint32_t)-step;
                ch->position = ch->length - 1;
                ch->fraction = 0;
            } else {
                ch->position -= ch->length - (uint32_t)sample->loopStart;
            }
        }
    } else {
        const int32_t sum = (int32_t)ch->fraction + step;

        ch->fraction = (uint32_t)sum & 0xfff;
        if (sum >= 0) {
            ch->position += (uint32_t)sum >> 12;
        } else {
            ch->position += (uint32_t)(sum >> 12);
        }
        if ((int32_t)ch->position < sample->loopStart) {
            ch->step = (uint32_t)-step;
            ch->position = (uint32_t)sample->loopStart;
            ch->fraction = 0;
        }
    }
}

BMX_REDIRECT_SLOT(winchanMix, (uint8_t *, uint32_t)); /* [audio] smooth_resample (platform/smoothmix.c) */

/*
 * Hand written x87 in the original. Per frame: the sum of every playing channel (gain * envelope level *
 * sample), its envelope and position stepped; then the flanger and the echo; then rounded, clamped to
 * -0x10000 .. 0xffff and halved into the 16 bit output.
 */
BMX_ADDR(0x0046f5f0)
void winchanMix(uint8_t *ring, uint32_t bytes)
{
    BMX_REDIRECT_VOID(winchanMix, (ring, bytes));
    int16_t *out = (int16_t *)ring;

    while (bytes != 0) {
        double left = 0.0, right = 0.0;
        int32_t pos, mixLeft, mixRight;
        int i;

        for (i = 0; i < kWinchanNumChannels; i++) {
            WinchanChannel *ch = &gWinchanChannels[i];
            const ZsdSample *sample;
            double l, r;

            if (ch->state != kWinchanPlaying) {
                continue;
            }
            sample = ch->sample;
            l = (double)ch->gainLeft * (double)ch->level;
            r = (double)ch->gainRight * (double)ch->level;
            if ((sample->format & 4) != 0) {
                const uint8_t *data = sample->data;

                if ((sample->format & 2) != 0) {
                    l *= (double)sample8(data[ch->position * 2]);
                    r *= (double)sample8(data[ch->position * 2 + 1]);
                } else {
                    const int32_t v = sample8(data[ch->position]);

                    l *= (double)v;
                    r *= (double)v;
                }
            } else {
                const int16_t *data = (const int16_t *)sample->data;

                if ((sample->format & 2) != 0) {
                    l *= (double)data[ch->position * 2];
                    r *= (double)data[ch->position * 2 + 1];
                } else {
                    l *= (double)data[ch->position];
                    r *= (double)data[ch->position];
                }
            }
            left += l;
            right += r;
            if (ch->phaseFrames != 0 && !mixEnvelope(ch)) {
                continue;
            }
            mixAdvance(ch);
        }

        /* Flanger: the dry frame goes into the delay line, the frame `delay` back is mixed in. */
        pos = gWinchanFlangerPos;
        gWinchanFlangerLine[pos][0] = (float)left;
        gWinchanFlangerLine[pos][1] = (float)right;
        if (gWinchanEffectBTailStart != 0) {
            const int32_t back = gWinchanFlangerDelay >> 12;
            const double gain = (double)gWinchanEffectBGain;
            const double dry = 1.0 - gain;

            if (pos < back) {
                pos += 0x400;
            }
            pos -= back;
            left = left * dry + (double)gWinchanFlangerLine[pos][0] * gain;
            right = right * dry + (double)gWinchanFlangerLine[pos][1] * gain;
        }
        gWinchanFlangerPos = gWinchanFlangerPos + 1 < 0x400 ? gWinchanFlangerPos + 1 : 0;
        gWinchanFlangerDelay = (int32_t)((uint32_t)gWinchanLfoTable[gWinchanFlangerPhase >> 12] *
                                             (uint32_t)gWinchanEffectBTailStart +
                                         (gWinchanEffectBFrames << 12));
        gWinchanFlangerPhase = (int32_t)((uint32_t)gWinchanFlangerPhase + (uint32_t)gWinchanEffectBTail);
        if (gWinchanFlangerPhase >= 0x1000000) {
            gWinchanFlangerPhase -= 0x1000000;
        }

        /* Echo: the frame AFrames back is added; the ring keeps this frame plus that one times feedback. */
        pos = gWinchanEchoPos;
        if (gWinchanEffectAFrames != 0) {
            WinchanFrame *echo = gWinchanEchoBuffer;
            int32_t back = pos;
            double echoLeft, echoRight, feedback;

            if (back < (int32_t)gWinchanEffectAFrames) {
                back += (int32_t)gWinchanEffectMaxFrames;
            }
            back -= (int32_t)gWinchanEffectAFrames;
            echoRight = (double)echo[back][1];
            echoLeft = (double)echo[back][0];
            feedback = (double)gWinchanEchoFeedback;
            echo[pos][0] = (float)(feedback * echoLeft + left);
            echo[pos][1] = (float)(feedback * echoRight + right);
            left += echoLeft * (double)gWinchanEffectAGain;
            right += echoRight * (double)gWinchanEffectAGain;
        } else {
            gWinchanEchoBuffer[pos][0] = (float)left;
            gWinchanEchoBuffer[pos][1] = (float)right;
        }
        pos++;
        if (pos >= (int32_t)gWinchanEffectMaxFrames) {
            pos = 0;
        }
        gWinchanEchoPos = pos;

        gWinchanMixLeft = fistp32(left);
        gWinchanMixRight = fistp32(right);
        mixLeft = gWinchanMixLeft > 0xffff ? 0xffff : gWinchanMixLeft < -0x10000 ? -0x10000 : gWinchanMixLeft;
        mixRight = gWinchanMixRight > 0xffff     ? 0xffff
                   : gWinchanMixRight < -0x10000 ? -0x10000
                                                 : gWinchanMixRight;
        out[0] = (int16_t)(mixLeft >> 1);
        out[1] = (int16_t)(mixRight >> 1);
        out += 2;
        bytes -= 4;
    }
}
