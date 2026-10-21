/* Positional sound sources (C:\zaxis\Bmx\src\com\zge\zsndsrc.c). */
#include "bmx/com/zge/zsndsrc.h"

#include "bmx/com/zge/zsndseq.h"
#include "bmx/pc/zge/sound/winchan.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"

/*
 * Floating point follows the original x87 code: `double` is a value held in an x87 register, `float` is a
 * value the original stored to memory.
 */

static const char zsndsrcFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\zsndsrc.c";

/* The constants the exe keeps in .rdata: 0.0f at 0x54d05c, 127.0f at 0x54d1e4. */
#define kZero    0.0f
#define k127     127.0f
#define kPitchHi 700 /* doppler pitch offset limit in cents */
#define kPitchLo (-700)

/* TODO(file): the stream wrappers below probably belong to a separate source file (they come first). */
BMX_ADDR(0x0047de20)
void zsndStreamInit(uint32_t bufferBytes)
{
    if (winstrmInit() == 0) {
        gZsndStreamBytes = bufferBytes;
    }
}

BMX_ADDR(0x0047de50)
int zsndStreamShutdown(void)
{
    if (gZsndStream != 0) {
        winstrmDestroy(gZsndStream);
        gZsndStream = 0;
    }
    winstrmShutdown();
    return 0;
}

BMX_ADDR(0x0047de90)
int zsndStreamNop(void)
{
    return 0;
}

BMX_ADDR(0x0047deb0)
int zsndStreamPlay(const WinloadSampleFile *file, uint8_t mode, int repeat)
{
    uint32_t format = kWinstrmFmtRate44k;
    int rc;

    if (gZsndStream != 0) {
        winstrmDestroy(gZsndStream);
    }
    if ((mode & 2) != 0) {
        format = 0;
    } else if ((mode & 1) != 0) {
        format = kWinstrmFmtRate11k;
    }
    if ((mode & 4) == 0) {
        format |= kWinstrmFmtStereo;
    }
    rc = winstrmCreate(&gZsndStream, file, 0, gZsndStreamBytes, format);
    if (rc == 0) {
        winstrmSetRepeat(gZsndStream, repeat);
        rc = 0;
    }
    return rc;
}

BMX_ADDR(0x0047df40)
int zsndStreamGetState(uint32_t *state)
{
    if (gZsndStream != 0) {
        return winstrmGetState(gZsndStream, state);
    }
    return 0;
}

BMX_ADDR(0x0047df70)
int zsndStreamStart(void)
{
    if (gZsndStream != 0) {
        return winstrmStart(gZsndStream);
    }
    return 0;
}

BMX_ADDR(0x0047dfa0)
int zsndStreamStop(void)
{
    if (gZsndStream != 0) {
        return winstrmStop(gZsndStream);
    }
    return 0;
}

BMX_ADDR(0x0047dfd0)
int zsndStreamStop2(void)
{
    if (gZsndStream != 0) {
        return winstrmStop(gZsndStream);
    }
    return 0;
}

BMX_ADDR(0x0047e000)
int zsndStreamSetVolumePan(uint8_t volume, int8_t pan)
{
    if (gZsndStream != 0) {
        return winstrmSetVolumePan(gZsndStream, volume, pan);
    }
    return 0;
}

BMX_ADDR(0x0047e030)
int zsndStreamFade(int millis, uint32_t target, uint32_t flags)
{
    if (gZsndStream != 0) {
        return winstrmFade(gZsndStream, millis, target, flags);
    }
    return 0;
}

BMX_ADDR(0x0047e070)
void zsndStreamPauseAll(int pause)
{
    winstrmPauseAll(pause);
}

BMX_ADDR(0x0047e090)
void zsndStreamSetFlag(uint32_t *flag)
{
    winstrmSetFlag(flag);
}

/* ---- sources ---- */

/* The original asserts that the module is initialised and the handle is not 0xffff (lines per caller). */
static void requireHandle(uint16_t source, int initLine)
{
    if (gZsndSrcInitialized == 0) {
        zdebugFail("not initialized", zsndsrcFile, initLine, 0);
    }
    if (source == kZsndSrcNone) {
        zdebugFail("invalid handle", zsndsrcFile, initLine + 1, 0);
    }
}

BMX_ADDR(0x0047e0b0)
int zsndSrcInit(uint32_t range, int lifetimeMs)
{
    int i;

    if (gZsndSrcInitialized != 0) {
        zdebugFail("already initialized", zsndsrcFile, 0x52, 0);
    }
    for (i = 0; i < kZsndSrcMax; i++) {
        gZsndSources[i].flags = 0;
    }
    gZsndSrcRange = range;
    gZsndSrcLifetime = (uint32_t)((uint32_t)(gZsndTicksPerSec * (uint32_t)lifetimeMs)) / 1000u;
    gZsndSrcRange2 = range * range;
    gZsndSrcNoDoppler = 0;
    gZsndSrcInitialized = 1;
    gZsndSrcDopplerScale = (float)((double)kZero / (double)range);
    return 0;
}

BMX_ADDR(0x0047e180)
void zsndSrcShutdown(void)
{
    int i;

    if (gZsndSrcInitialized == 0) {
        zdebugFail("not initialized", zsndsrcFile, 0x6b, 0);
    }
    for (i = 0; i < kZsndSrcMax; i++) {
        if ((gZsndSources[i].flags & kZsndSrcUsed) != 0) {
            zsndSrcDestroy((uint16_t)i);
        }
    }
    gZsndSrcNext = 0;
    gZsndSrcInitialized = 0;
}

BMX_ADDR(0x0047e200)
int zsndSrcUpdate(void)
{
    int result = 0;
    int i;

    if (gZsndSrcInitialized == 0) {
        return 0;
    }
    for (i = 0; i < kZsndSrcMax; i++) {
        ZsndSource *s = &gZsndSources[i];
        ZVec3 local;
        int32_t x, y, z;
        uint32_t dist2;

        if ((s->flags & kZsndSrcUsed) == 0 || s->lifetime == 0) {
            continue;
        }
        s->lifetime--;
        if (s->lifetime == 0) {
            if (s->channel != kZsndSrcNoChannel) {
                winchanVoiceFree(s->channel);
                s->channel = kZsndSrcNoChannel;
            }
            continue;
        }
        if (s->channel != kZsndSrcNoChannel && winchanVoiceGetVoice(s->channel) != s->voice) {
            winchanVoiceFree(s->channel);
            s->channel = kZsndSrcNoChannel;
        }
        zsystemToListener(&s->position, &local);
        x = bmxFtol((double)local.x);
        y = bmxFtol((double)local.y);
        z = bmxFtol((double)local.z);
        dist2 = (uint32_t)z * (uint32_t)z + (uint32_t)y * (uint32_t)y + (uint32_t)x * (uint32_t)x;
        if (dist2 < gZsndSrcRange2) {
            uint8_t volume;
            int32_t pan;

            if (s->channel == kZsndSrcNoChannel) {
                s->channel = winchanVoiceAlloc(s->voice, kZsndSrcPriority);
                if (s->channel == kZsndSrcNoChannel) {
                    result = 2;
                    continue;
                }
                winchanVoiceStart(s->channel);
            }
            volume =
                (uint8_t)(0x7f - (uint8_t)((((dist2 >> 7) << 7) - (dist2 >> 7)) / (gZsndSrcRange2 >> 7)));
            s->volume = volume;
            winchanVoiceSetVolume(s->channel, (uint8_t)((uint32_t)s->baseVolume * volume >> 7));
            if (x == 0) {
                s->pan = 0;
            } else {
                pan = (int32_t)(((uint32_t)x << 7) - (uint32_t)x) / (int32_t)gZsndSrcRange;
                s->pan = (int8_t)pan;
            }
            winchanVoiceSetPan(s->channel, s->pan);
            if (gZsndSrcNoDoppler == 0) {
                const int16_t doppler =
                    (int16_t)(0 - (uint32_t)bmxFtol((double)(int32_t)(dist2 - s->distance2) *
                                                    (double)gZsndSrcDopplerScale));

                s->doppler = doppler;
                if (doppler < kPitchLo) {
                    s->doppler = kPitchLo;
                } else if (doppler >= kPitchHi) {
                    s->doppler = kPitchHi;
                }
            } else {
                s->doppler = 0;
            }
            winchanVoiceSetPitch(s->channel, (int16_t)(s->pitch + s->doppler));
            s->distance2 = dist2;
        } else {
            s->volume = 0;
            if (s->channel != kZsndSrcNoChannel) {
                winchanVoiceFree(s->channel);
                s->channel = kZsndSrcNoChannel;
            }
            s->distance2 = dist2;
        }
    }
    gZsndSrcNoDoppler = 0;
    return result;
}

BMX_ADDR(0x0047e4b0)
void zsndSrcClearDoppler(void)
{
    int i;

    gZsndSrcNoDoppler = 1;
    for (i = 0; i < kZsndSrcMax; i++) {
        const ZsndSource *s = &gZsndSources[i];

        if ((s->flags & kZsndSrcUsed) != 0 && s->lifetime != 0 && s->doppler != 0) {
            zsndSrcSetPitch((uint16_t)i, s->pitch);
        }
    }
}

BMX_ADDR(0x0047e520)
uint16_t zsndSrcCreate(const ZsdVoice *voice, const ZVec3 *position)
{
    ZsndSource *s;
    uint32_t slot;
    int32_t x, y, z;

    if (gZsndSrcInitialized == 0) {
        zdebugFail("not initialized", zsndsrcFile, 0xef, 0);
    }
    slot = gZsndSrcNext;
    /* Original quirk: when the slot at the hint is taken, the search gives up after the next one. */
    while ((gZsndSources[slot].flags & kZsndSrcUsed) != 0) {
        slot++;
        if (slot == kZsndSrcMax) {
            slot = 0;
        }
        if (slot != gZsndSrcNext) {
            return kZsndSrcNone;
        }
    }
    gZsndSrcNext = slot + 1;
    s = &gZsndSources[slot];
    s->flags = kZsndSrcUsed;
    if (gZsndSrcNext == kZsndSrcMax) {
        gZsndSrcNext = 0;
    }
    s->pitch = 0;
    s->channel = kZsndSrcNoChannel;
    s->voice = voice;
    s->baseVolume = 0x7f;
    x = bmxFtol((double)position->x);
    y = bmxFtol((double)position->y);
    z = bmxFtol((double)position->z);
    s->distance2 = (uint32_t)z * (uint32_t)z + (uint32_t)y * (uint32_t)y + (uint32_t)x * (uint32_t)x;
    zsndSrcSetPosition((uint16_t)slot, position);
    s->lifetime = 0;
    return (uint16_t)slot;
}

BMX_ADDR(0x0047e650)
void zsndSrcDestroy(uint16_t source)
{
    ZsndSource *s = &gZsndSources[source];

    requireHandle(source, 0x117);
    if ((s->flags & kZsndSrcUsed) == 0) {
        zdebugFail("source not allocated", zsndsrcFile, 0x11a, 0);
    }
    if (s->channel != kZsndSrcNoChannel) {
        winchanVoiceFree(s->channel);
        s->channel = kZsndSrcNoChannel;
    }
    s->flags = 0;
}

BMX_ADDR(0x0047e720)
int zsndSrcSetVoice(uint16_t source, const ZsdVoice *voice)
{
    ZsndSource *s = &gZsndSources[source];

    requireHandle(source, 0x12c);
    if ((s->flags & kZsndSrcUsed) == 0) {
        zdebugFail("source not allocated", zsndsrcFile, 0x12f, 0);
    }
    s->voice = voice;
    return 0;
}

BMX_ADDR(0x0047e7e0)
int zsndSrcSetPosition(uint16_t source, const ZVec3 *position)
{
    ZsndSource *s = &gZsndSources[source];

    requireHandle(source, 0x13f);
    s->position = *position;
    s->lifetime = gZsndSrcLifetime;
    return 0;
}

BMX_ADDR(0x0047e880)
int zsndSrcSetPitch(uint16_t source, int16_t cents)
{
    requireHandle(source, 0x159);
    gZsndSources[source].pitch = cents;
    return 0;
}

BMX_ADDR(0x0047e910)
int zsndSrcSetVolume(uint16_t source, uint8_t volume)
{
    requireHandle(source, 0x16d);
    gZsndSources[source].baseVolume = volume;
    return 0;
}

BMX_ADDR(0x0047e990)
uint8_t zsndSrcGetChannel(uint16_t source)
{
    requireHandle(source, 0x17e);
    return gZsndSources[source].channel;
}

BMX_ADDR(0x0047ea10)
int zsndSrcMeasure(const ZVec3 *position, uint8_t *volume, int8_t *pan)
{
    ZVec3 local;
    int32_t x, y, z;
    uint32_t dist2;

    if (gZsndSrcInitialized == 0) {
        zdebugFail("not initialized", zsndsrcFile, 400, 0);
    }
    zsystemToListener(position, &local);
    x = bmxFtol((double)local.x);
    y = bmxFtol((double)local.y);
    z = bmxFtol((double)local.z);
    dist2 = (uint32_t)z * (uint32_t)z + (uint32_t)y * (uint32_t)y + (uint32_t)x * (uint32_t)x;
    if (dist2 < gZsndSrcRange2) {
        *volume = (uint8_t)(0x7f - (uint8_t)((((dist2 << 7) - dist2)) / gZsndSrcRange2));
        /* The comparisons are x87 FCOMs: an unordered result counts as "equal". */
        if ((local.x > kZero || local.x < kZero) && (local.z > kZero || local.z < kZero)) {
            if (local.y > kZero || local.y < kZero) {
                zvec3RotateX(&local, &local, (ZAngle)(0 - (uint32_t)zmathAtan2(local.y, local.z)));
            }
            *pan = (int8_t)bmxFtol((double)local.x * (double)k127 / (double)(int32_t)gZsndSrcRange);
            return 0;
        }
        *pan = 0;
        return 0;
    }
    *volume = 0;
    *pan = 0;
    return 1;
}
