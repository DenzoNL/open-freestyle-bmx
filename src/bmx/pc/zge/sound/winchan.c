/* Windows sound channels (C:\zaxis\Bmx\src\Pc\zge\sound\winChan.c). */
#include "bmx/pc/zge/sound/winchan.h"

#include "bmx/pc/zge/sound/winapi.h"
#include "compat/crt.h"

/*
 * Floating point follows the original x87 code: `double` is a value held in an x87 register, `float` is a
 * value the original stored to memory.
 */

static const char winchanFile[] = "C:\\zaxis\\Bmx\\src\\Pc\\zge\\sound\\winChan.c";

/* Exe tables: volume curve, envelope times (ms), semitone and cent pitch factors (12 bit fixed point). */
typedef float GainTable[128];
typedef uint16_t EnvelopeTable[128];
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
