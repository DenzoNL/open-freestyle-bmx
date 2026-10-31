/*
 * Smooth resampling for the game's software mixer (docs/PLATFORM.md P9 note, docs/NATIVE.md): an opt-in,
 * native-only variant of winchanMix (src/bmx/pc/zge/sound/winchan.c) that reads a channel's sample with the
 * 12 bit fraction of its position instead of ignoring it. The original holds each sample for several output
 * frames (zero-order hold), which images the spectrum at multiples of the sample rate; interpolating removes
 * most of that grit.
 *
 *   [audio]
 *   smooth_resample = 0   -> the original mixer
 *                     1   -> linear interpolation
 *                     2   -> cubic (4 point Hermite / Catmull-Rom)
 *
 * The seam (PLAN.md): the faithful winchanMix stays untouched. When the option is on the native loader
 * writes a jump over the entry of the ported winchanMix to bmxSmoothMix (bmxSmoothJumps, a jump table like
 * the SDL3 driver's and the PNG reader's, but over our own function: winchanPump calls it directly, as
 * the hook build's in-game hooks patch ported entries); the hook DLL and the oracle tests never see it.
 * Everything but the sample read is a copy of winchanMix: gains, envelopes, flanger, echo, the x87 FISTP
 * rounding, the clamp and the halving. The copy is kept honest by tests/platform, which compares it with the
 * real winchanMix at fraction 0.
 *
 * Neighbours of a position follow the position logic (mixAdvance): a one shot sample clamps at both ends, a
 * loop wraps by (length - loopStart), a ping-pong loop mirrors at length - 1 (and at loopStart while the
 * channel runs backwards).
 *
 * Compiled with BMX_HOOK=1 (the game's globals at their original addresses, like src/driver).
 */
#include "platform/smoothmix.h"

#include <stdint.h>

#include "bmx/pc/zge/sound/winchan.h"
#include "platform/platform.h"

/* Exe tables (as in winchan.c). */
typedef uint16_t EnvelopeTable[256];
#define gEnvelopeMs ORIG_VAR(0x0055eb7c, EnvelopeTable, gEnvelopeMs)

static int gMode;

static uint32_t envelopeFrames(uint16_t millis)
{
    return (uint32_t)((uint32_t)millis * gWinchanRate) / 1000u;
}

/* x87 FISTP in the round-to-nearest mode: NaN and values outside int32 store 0x80000000. */
static int32_t fistp32(double v)
{
    if (!(v >= -2147483648.5 && v < 2147483647.5)) {
        return INT32_MIN;
    }
    return (int32_t)((v + 0x1.8p52) - 0x1.8p52);
}

static int32_t sample8(uint8_t v)
{
    return (int16_t)(uint16_t)(((uint32_t)v << 8) ^ 0x8000u);
}

static void mixStop(WinchanChannel *ch)
{
    ch->state = kWinchanStopped;
    ch->phaseFrames = 0;
    if ((ch->flags & kWinchanFlagFreeHandle) != 0) {
        winchanChannelRelease(ch);
    }
}

/* As winchan.c: one frame of the envelope; 0 when the last phase ended. */
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

/* As winchan.c: advances the position, looping or ending at the sample's bounds. */
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

/* The frame index a neighbour `i` of the channel's position stands for (the position logic's wrap). */
int32_t bmxSmoothNeighbour(const WinchanChannel *ch, int32_t i)
{
    const int32_t length = (int32_t)ch->length;
    const int32_t loopStart = ch->sample->loopStart;
    const uint8_t format = ch->sample->format;

    if (length <= 0) {
        return 0;
    }
    if ((format & 1) != 0 && (format & 8) == 0) {
        /* Loop: past the end continues at the loop start. */
        while (i >= length && length > loopStart) {
            i -= length - loopStart;
        }
    } else if ((format & 1) != 0) {
        /* Ping-pong: mirrored at the last frame, and at the loop start when running backwards. */
        if (i >= length) {
            i = 2 * (length - 1) - i;
        }
        if ((int32_t)ch->step < 0 && i < loopStart) {
            i = 2 * loopStart - i;
        }
    }
    return i < 0 ? 0 : i >= length ? length - 1 : i;
}

/* One channel (0 left, 1 right; mono ignores it) of the frame at index i, as the mixer multiplies it. */
static double frameValue(const ZsdSample *sample, uint32_t i, int side)
{
    if ((sample->format & 4) != 0) {
        const uint8_t *data = sample->data;

        return (double)sample8((sample->format & 2) != 0 ? data[i * 2 + (uint32_t)side] : data[i]);
    }
    {
        const int16_t *data = (const int16_t *)sample->data;

        return (double)((sample->format & 2) != 0 ? data[i * 2 + (uint32_t)side] : data[i]);
    }
}

/* The sample of a channel at position + fraction / 4096. */
static double smoothValue(const WinchanChannel *ch, int side, int mode)
{
    const ZsdSample *sample = ch->sample;
    const int32_t pos = (int32_t)ch->position;
    const double t = (double)ch->fraction * (1.0 / 4096.0);
    const double p1 = frameValue(sample, ch->position, side);
    double p0, p2, p3;

    if (ch->fraction == 0 || mode < 1) {
        return p1;
    }
    p2 = frameValue(sample, (uint32_t)bmxSmoothNeighbour(ch, pos + 1), side);
    if (mode == 1) {
        return p1 + (p2 - p1) * t;
    }
    p0 = frameValue(sample, (uint32_t)bmxSmoothNeighbour(ch, pos - 1), side);
    p3 = frameValue(sample, (uint32_t)bmxSmoothNeighbour(ch, pos + 2), side);
    return p1 +
           0.5 * t *
               ((p2 - p0) + t * ((2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) + t * (3.0 * (p1 - p2) + p3 - p0)));
}

void bmxSmoothMixMode(uint8_t *ring, uint32_t bytes, int mode)
{
    int16_t *out = (int16_t *)ring;

    while (bytes != 0) {
        double left = 0.0, right = 0.0;
        int32_t pos, mixLeft, mixRight;
        int i;

        for (i = 0; i < kWinchanNumChannels; i++) {
            WinchanChannel *ch = &gWinchanChannels[i];
            double l, r;

            if (ch->state != kWinchanPlaying) {
                continue;
            }
            l = (double)ch->gainLeft * (double)ch->level;
            r = (double)ch->gainRight * (double)ch->level;
            if ((ch->sample->format & 2) != 0) {
                l *= smoothValue(ch, 0, mode);
                r *= smoothValue(ch, 1, mode);
            } else {
                const double v = smoothValue(ch, 0, mode);

                l *= v;
                r *= v;
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

/* The function the loader's jump lands on (the original's signature and calling convention). */
void bmxSmoothMix(uint8_t *ring, uint32_t bytes)
{
    bmxSmoothMixMode(ring, bytes, gMode);
}

static BmxHook gJumps[1];

const BmxHook *bmxSmoothJumps(uint32_t *count)
{
    /* Cubic by default (the user's call, PLAN.md Decisions); 0 gives the original mixer. */
    const int mode = bmxConfigInt("audio.smooth_resample", 2);

    gMode = mode < 0 ? 0 : mode > 2 ? 2 : mode;
    if (mode > 2) {
        bmxLog("audio: smooth_resample = %d is out of range, using 2 (cubic)\n", mode);
    }
    gJumps[0].orig = (uint32_t)(uintptr_t)winchanMix;
    gJumps[0].impl = (void *)bmxSmoothMix;
    *count = gMode != 0 ? 1u : 0u;
    return gJumps;
}
