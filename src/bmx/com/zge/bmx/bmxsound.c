/* Game sound front end (C:\zaxis\Bmx\src\com\zge\bmx\bmxsound.c). */
#include "bmx/com/zge/bmx/bmxsound.h"

#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/com/zge/zsndseq.h"
#include "bmx/com/zge/zsndsrc.h"
#include "bmx/pc/zge/sound/winapi.h"
#include "bmx/pc/zge/sound/winchan.h"
#include "bmx/pc/zge/sound/winload.h"
#include "bmx/pc/zge/sound/winstrm.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zmath.h"
#include "zge/zstring.h"

static const char bmxsoundFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\bmx\\bmxsound.c";

BMX_ADDR(0x0044dc30)
void bmxMenuShutdown(void)
{
    appLog("APP: Shutdown MENU\n");
    bmxSoundPlayEvent(1000, 0, 0, 0, 0);
    zrollerShutdown();
    guiModeShutdown();
    guiPadShutdown();
}

BMX_ADDR(0x0044dc70)
int bmxMenuUpdate(void)
{
    GuiPad *pad;

    guiPadPoll();
    pad = guiPadGet(0);
    /* Auto-repeat of the d-pad: after a press the direction bits repeat every few updates. */
    if ((pad->pressed & 0xfffffcffu) == 0) {
        if (pad->flags2 == 0) {
            pad->pressed = pad->held & 0xf;
            pad->flags2 = (pad->held & 0x300) != 0 ? 0 : 3;
        } else {
            pad->flags2--;
        }
    } else {
        pad->flags2 = 0x14;
    }
    if (gBmxMenuExit != 0) {
        if (gBmxMenuExitResult == 0) {
            appShowError();
            return 0;
        }
        return 1;
    }
    return guiUpdate() != 0;
}

BMX_ADDR(0x0044dd10)
void bmxMenuRequestExit(int result)
{
    gBmxMenuExit = 1;
    gBmxMenuExitResult = result;
}

BMX_ADDR(0x0044dd40)
int bmxSoundSetTickRate(int ticks)
{
    return zsndSetTickRate(ticks);
}

BMX_ADDR(0x0044dd60)
uint32_t bmxSoundProbeDevice(void *window)
{
    return winchanProbeDevice(window);
}

BMX_ADDR(0x0044dd80)
int bmxSoundNop(void)
{
    return zsndNop();
}

BMX_ADDR(0x0044dda0)
void bmxSoundOpen(void)
{
    WinchanInitDesc desc;
    void *platform;
    WinTimeCaps caps;
    uint32_t period;

    if (gBmxSoundState != kBmxSoundClosed) {
        zdebugFail("sound already open", bmxsoundFile, 0xfc, 0);
    }
    memset(&desc, 0, sizeof desc);
    gZsystemGetObject(0x15, &platform, 0);
    desc.window = *(void **)platform;
    desc.format = 0;
    desc.rate = kBmxSoundMixerRate;
    if (winchanInit(&desc) != 0) {
        zdebugFail("failure initializing channel manager", bmxsoundFile, 0x111, 0);
    }
    gZsndTicksPerSec = kBmxSoundTicksPerSecond;
    if (zsndSeqReset() != 0) {
        zdebugFail("failure initializing sequenced audio manager", bmxsoundFile, 0x114, 0);
    }
    if (zsndSrcInit(kBmxSoundSourceRange, kBmxSoundSourceLifetimeMs) != 0) {
        zdebugFail("failure initializing 3-d sound source manager", bmxsoundFile, 0x116, 0);
    }
    if (winloadLoad(zfileBuildFileName("bin\\audio\\", "SFXCOM.ZSD"), 1, &gBmxSfxFile) != 0) {
        zdebugFail("failure loading common SFX asset file", bmxsoundFile, 0x11a, 0);
    }
    if (winloadLoad(zfileBuildFileName("bin\\audio\\", "MUSIC.ZSD"), 0, &gBmxMusicFile) != 0) {
        zdebugFail("failure loading music asset file", bmxsoundFile, 0x11d, 0);
    }
    if (winloadLoad(zfileBuildFileName("bin\\audio\\", "SPEECH.ZSD"), 0, &gBmxSpeechFile) != 0) {
        zdebugFail("failure loading speech asset file", bmxsoundFile, 0x120, 0);
    }
    gBmxSoundState = kBmxSoundOpen;
    if (gWinTimeGetDevCaps(&caps, sizeof caps) != 0) {
        zdebugFail("error getting timer device capabilities", bmxsoundFile, 0x130, 0);
        return;
    }
    /* Finest resolution the timer supports, but at least 1 ms. */
    period = caps.periodMin > 1 ? caps.periodMin : 1;
    if (period >= caps.periodMax) {
        period = caps.periodMax;
    }
    gBmxTimerPeriod = period;
    gWinTimeBeginPeriod(period);
    gBmxTimerId = gWinTimeSetEvent(kBmxSoundTimerMillis, period,
                                   BMX_FNPTR(WinTimeCallback, 0x00450020, zoverPollTimer), 0, 1);
    if (gBmxTimerId == 0) {
        zdebugFail("error creating timer event", bmxsoundFile, 300, 0);
    }
    gWinTimeEndPeriod(gBmxTimerPeriod);
}

BMX_ADDR(0x0044e090)
void bmxSoundClose(void)
{
    if (gBmxSoundState == kBmxSoundClosed) {
        zdebugFail("sound already closed", bmxsoundFile, 0x13e, 0);
    }
    gWinTimeKillEvent(gBmxTimerId);
    gWinTimeEndPeriod(gBmxTimerPeriod);
    winloadUnload(kWinloadAllFiles);
    zsndSrcShutdown();
    zsndSeqStopAll();
    winchanShutdown();
    gBmxSoundState = kBmxSoundClosed;
}

BMX_ADDR(0x0044e120)
void bmxSoundUpdate(void)
{
    uint32_t state;

    if (gBmxTrackCooldown != 0) {
        gBmxTrackCooldown--;
    }
    if (gBmxSoundState == kBmxSoundClosed) {
        return;
    }
    if (gBmxSoundState == kBmxSoundInGame && gBmxMusicMode >= 0 && gBmxMusicMode <= 2) {
        winstrmGetState(gBmxMusicStream, &state);
        if ((state == kWinstrmStopped || gBmxNextTrack != kBmxSoundNoTrack) && gBmxTrackCooldown == 0) {
            uint8_t track = gBmxNextTrack;

            if (track == kBmxSoundNoTrack) {
                /* Pick the next track: the level's own first, then (when looping) the following ones. */
                track = gBmxCurrentTrack;
                if (track == kBmxSoundNoTrack) {
                    track = gBmxLevelTracks[gBmxLevelIndex];
                } else if (gBmxTrackLoop != 0) {
                    track++;
                    gBmxCurrentTrack = track;
                    if ((int8_t)track >= (int32_t)winloadGetSampleFileCount(gBmxMusicFile)) {
                        track = 0;
                    }
                }
                gBmxCurrentTrack = track;
                gBmxNextTrack = track;
            }
            winstrmReplaceSource(gBmxMusicStream,
                                 winloadGetSampleFile(gBmxMusicFile, (uint32_t)(int32_t)(int8_t)track),
                                 kBmxSoundMusicFormat);
            winstrmSetVolumePan(gBmxMusicStream, (uint8_t)((uint32_t)gBmxMusicVolume * 15 >> 4), 0);
            winstrmStart(gBmxMusicStream);
            gBmxCurrentTrack = gBmxNextTrack;
            gBmxNextTrack = kBmxSoundNoTrack;
            gBmxTrackCooldown = kBmxSoundTrackCooldown;
        }
    }
    zsndSeqUpdate();
    zsndSrcUpdate();
    winstrmUpdateFades();
}

BMX_ADDR(0x0044e2e0)
void bmxSoundUpdateStreams(void)
{
    if (gBmxSoundState != kBmxSoundClosed) {
        winstrmUpdate();
    }
}

BMX_ADDR(0x0044ffb0)
int bmxSoundMeasure(const struct ZVec3 *position, uint8_t volume, uint8_t *outVolume, int8_t *outPan)
{
    int result;

    if (position != 0) {
        result = zsndSrcMeasure((const ZVec3 *)position, outVolume, outPan);
        *outVolume = (uint8_t)((uint32_t)*outVolume * volume >> 7);
        return result;
    }
    *outVolume = volume;
    *outPan = 0;
    return 1;
}

/* ---- the sound event dispatcher ---- */

/*
 * Effect parameters for winchanSetEffect. The dispatcher always passes kind 2, which the PC mixer does not
 * know (winchanSetEffect returns 0x10d): every effect change here is a no-op (original quirk).
 */
#define gBmxSoundEffect ORIG_VAR(0x00574680, WinchanEffect, gBmxSoundEffect)

static void setEffect(uint8_t level, uint8_t time)
{
    uint8_t *bytes = (uint8_t *)&gBmxSoundEffect.level;

    gBmxSoundEffect.kind = 2;
    bytes[0] = level;
    bytes[1] = time;
    winchanSetEffect(&gBmxSoundEffect);
}

/* Plays voice `index` of a sound file. */
static void playVoice(uint16_t file, uint32_t index, uint32_t cents, uint32_t volume, uint32_t pan,
                      int priority)
{
    winchanVoicePlay(winloadGetVoice(file, index), (int16_t)cents, (uint8_t)volume, (int8_t)pan, priority);
}

/* The effects volume scaled by 5/32. */
static uint8_t quietVolume(void)
{
    return (uint8_t)((uint32_t)gBmxSfxVolume * 20 >> 7);
}

/* Ambient track volume on level 4: track 0 is quiet on side 1, the others on side 0. */
static uint8_t sideVolume(uint32_t track, uint32_t side)
{
    return ((uint32_t)(track == 0) ^ side) == 0 ? quietVolume() : gBmxSfxVolume;
}

/* Ambient track volume: full, except on level 4 (see gBmxAmbientSide). */
static uint8_t ambientVolume(uint32_t track)
{
    return gBmxLevelIndex == 4 ? sideVolume(track, gBmxAmbientSide) : gBmxSfxVolume;
}

/* Loads a sound file of bin\audio (sample data resident); fails with `why` at `line`. */
static void loadSfx(const char *name, uint16_t *file, const char *why, int line)
{
    if (winloadLoad(zfileBuildFileName("bin\\audio\\", name), 1, file) != 0) {
        zdebugFail(why, bmxsoundFile, line, 0);
    }
}

/* Leaving a level (event 1000 to state 0 or 1): frees every level sound and the music stream. */
static void levelSoundsRelease(void)
{
    uint32_t i;

    for (i = 0; i < winloadGetTrackDefCount(gBmxLevelSfxFile); i++) {
        zsndSeqDestroy(gBmxAmbientTracks[i]);
        gBmxAmbientTracks[i] = 0xff;
    }
    winstrmStop(gBmxMusicStream);
    for (i = 0; i < 2; i++) {
        zsndSrcDestroy(gBmxRiderLoopA[i]);
        gBmxRiderLoopA[i] = 0xffff;
        zsndSrcDestroy(gBmxRiderLoopB[i]);
        gBmxRiderLoopB[i] = 0xffff;
    }
    if (gBmxGunChannel != 0xff) {
        winchanVoiceFree(gBmxGunChannel);
        gBmxGunChannel = 0xff;
    }
    winstrmDestroy(gBmxMusicStream);
    gBmxMusicStream = 0;
    winloadUnload(gBmxRiderSfxFile);
    gBmxRiderSfxFile = 0xffff;
    winloadUnload(gBmxLevelSfxFile);
    gBmxLevelSfxFile = 0xffff;
    if (gBmxGunSfxFile != 0xffff) {
        winloadUnload(gBmxGunSfxFile);
        gBmxGunSfxFile = 0xffff;
    }
    winloadUnload(gBmxGameSfxFile);
    gBmxGameSfxFile = 0xffff;
    winstrmShutdown();
    setEffect(0, 0);
}

/* Entering a level (event 1000 to state 2 from the menu): level, rider and gun sounds, music, loops. */
static void levelSoundsLoad(uint32_t level, uint32_t rider, int gun)
{
    char name[32];
    ZVec3 far = {0.0f, 20000.0f, 0.0f};
    uint32_t i;

    gBmxMusicMode = 0;
    gBmxLevelIndex = (uint8_t)(level < 12 ? level : 0);
    gBmxRiderIndex = (uint8_t)(rider < 2 ? rider : 0);
    if (gBmxLevelIndex == 4) {
        setEffect(2, 0x46);
        gBmxAmbientSide = 0;
    } else {
        setEffect(1, 0x11);
        gBmxAmbientSide = 1;
    }
    winstrmInit();
    loadSfx("SFXGAME.ZSD", &gBmxGameSfxFile, "failure loading game SFX asset file", 0x2c8);
    if (gun == 0) {
        zstrFormat(name, "SFXLEV%02d.ZSD", (uint32_t)gBmxLevelIndex);
    } else {
        loadSfx("SFXGAME2.ZSD", &gBmxGunSfxFile, "failure loading gun SFX asset file", 0x2d1);
        zstrFormat(name, "SFXLEV%02d.ZSD", 12);
    }
    gBmxGunChannel = 0xff;
    loadSfx(name, &gBmxLevelSfxFile, "failure loading level SFX asset file", 0x2d7);
    zstrFormat(name, "SFXCHR%02d.ZSD", (uint32_t)gBmxRiderIndex);
    loadSfx(name, &gBmxRiderSfxFile, "failure loading character SFX asset file", 0x2de);
    if (winstrmCreate(&gBmxMusicStream, winloadGetSampleFile(gBmxMusicFile, 0), 0, 0x40000, 0x44) != 0) {
        zdebugFail("failure creating music stream (probably out of SPU RAM)", bmxsoundFile, 0x2f1, 0);
    }
    gBmxCurrentTrack = kBmxSoundNoTrack;
    gBmxNextTrack = kBmxSoundNoTrack;
    winstrmSetVolumePan(gBmxMusicStream, (uint8_t)((uint32_t)gBmxMusicVolume * 120 >> 7), 0);
    for (i = 0; i < 2; i++) {
        gBmxRiderLoopA[i] = zsndSrcCreate(winloadGetVoice(gBmxGameSfxFile, 0), &far);
        zsndSrcSetVolume(gBmxRiderLoopA[i], gBmxSfxVolume);
        gBmxRiderLoopB[i] = zsndSrcCreate(winloadGetVoice(gBmxGameSfxFile, 9), &far);
        zsndSrcSetVolume(gBmxRiderLoopB[i], gBmxSfxVolume);
    }
    for (i = 0; i < winloadGetTrackDefCount(gBmxLevelSfxFile); i++) {
        const uint8_t track =
            zsndSeqCreate((const ZsndSeqHeader *)winloadGetTrackDef(gBmxLevelSfxFile, i), 0);

        gBmxAmbientTracks[i] = track;
        zsndSeqSetVolume(track, ambientVolume(i), 0);
        zsndSeqStart(gBmxAmbientTracks[i]);
    }
}

/* Event 1000: switches the sound state, tearing down what the old state had and loading the new one's. */
static int eventState(int32_t state, uint32_t level, uint32_t rider, int gun)
{
    const int32_t old = gBmxSoundState;
    uint32_t i;

    if (old == state) {
        return 0;
    }
    if (old == 1) {
        winchanVoiceFree(gBmxMenuChannels[0]);
        winchanVoiceFree(gBmxMenuChannels[1]);
        winloadUnload(gBmxMenuSfxFile);
        gBmxMenuSfxFile = 0xffff;
    } else if (old > 1 && old <= 3) {
        if ((uint32_t)state <= 1) {
            levelSoundsRelease();
        } else if (state == 3) {
            playVoice(gBmxGameSfxFile, 0x6e, 0, gBmxSfxVolume, 0, 0x21);
            for (i = 0; i < winloadGetTrackDefCount(gBmxLevelSfxFile); i++) {
                zsndSeqPause(gBmxAmbientTracks[i]);
            }
            if (gBmxMusicMode >= 0 && gBmxMusicMode <= 2 && gBmxCurrentTrack != kBmxSoundNoTrack) {
                winstrmPause(gBmxMusicStream, 1);
            }
        }
    }
    if (state == 1) {
        char name[32];

        zstrFormat(name, "SFXMEN%02d.ZSD", (uint32_t)gBmxMenuSfxSet);
        loadSfx(name, &gBmxMenuSfxFile, "failure loading menu SFX asset file", 0x281);
        gBmxMenuChannels[0] = winchanVoiceAlloc(winloadGetVoice(gBmxMenuSfxFile, 0), 0x22);
        gBmxMenuChannels[1] = winchanVoiceAlloc(winloadGetVoice(gBmxMenuSfxFile, 1), 0x22);
        winchanVoiceSetVolume(gBmxMenuChannels[0], gBmxMusicVolume);
        winchanVoiceSetVolume(gBmxMenuChannels[1], gBmxMusicVolume);
        winchanVoiceStart(gBmxMenuChannels[0]);
        winchanVoiceStart(gBmxMenuChannels[1]);
        gBmxMenuSfxSet = (uint8_t)((gBmxMenuSfxSet - 1) & 1);
    } else if (state == 2 && gBmxSoundState >= 0) {
        if (gBmxSoundState <= 1) {
            levelSoundsLoad(level, rider, gun);
        } else if (gBmxSoundState == 3) {
            /* Resume. */
            if (gBmxMusicMode >= 0 && gBmxMusicMode <= 2) {
                if (gBmxCurrentTrack != kBmxSoundNoTrack) {
                    winstrmStart(gBmxMusicStream);
                }
            } else if (gBmxMusicMode == 4) {
                winstrmStop(gBmxMusicStream);
            }
            for (i = 0; i < winloadGetTrackDefCount(gBmxLevelSfxFile); i++) {
                zsndSeqStart(gBmxAmbientTracks[i]);
            }
        }
    }
    gBmxSoundState = state;
    return 0;
}

/* Event 1001: switches the music mode; leaving a playing mode in game fades the music out first. */
static int eventMusicMode(int32_t mode)
{
    if (gBmxMusicMode == mode) {
        return 0;
    }
    if (gBmxSoundState == kBmxSoundInGame && gBmxMusicMode >= 0 && gBmxMusicMode <= 2) {
        if (mode != 3) {
            winstrmFade(gBmxMusicStream, 0x2ee, 0, 2);
            gBmxMusicMode = mode;
            gBmxTrackCooldown = 0x1e;
            return 0;
        }
        winstrmStop(gBmxMusicStream);
    }
    gBmxMusicMode = mode;
    return 0;
}

/*
 * Event 1002: sets volume `which` (0 effects, 1 music) and applies it to what is playing. Returns 2 for an
 * index past 2 (after writing it).
 */
static int eventVolume(uint32_t which, uint8_t volume)
{
    const int32_t state = gBmxSoundState;
    uint32_t i;

    gBmxVolumes[which] = volume;
    if (which == 0) {
        if (state >= 2 && state <= 3) {
            for (i = 0; i < winloadGetTrackDefCount(gBmxLevelSfxFile); i++) {
                zsndSeqSetVolume(gBmxAmbientTracks[i], ambientVolume(i), 0);
            }
        }
        return 0;
    }
    if (which == 1) {
        if (state == 1) {
            winchanVoiceSetVolume(gBmxMenuChannels[0], gBmxMusicVolume);
            winchanVoiceSetVolume(gBmxMenuChannels[1], gBmxMusicVolume);
        } else if (state >= 2 && state <= 3) {
            winstrmSetVolumePan(gBmxMusicStream, (uint8_t)((uint32_t)gBmxMusicVolume * 120 >> 7), 0);
        }
        return 0;
    }
    return which == 2 ? 0 : 2;
}

/* Event 1005: queues music track `track` (clamped to the last one). */
static int eventNextTrack(uint32_t track)
{
    if (track >= winloadGetSampleFileCount(gBmxMusicFile) - 1) {
        track = winloadGetSampleFileCount(gBmxMusicFile) - 1;
    }
    if ((int32_t)track == (int32_t)(int8_t)gBmxCurrentTrack || gBmxMusicMode < 0 || gBmxMusicMode > 1) {
        return 0;
    }
    gBmxNextTrack = (uint8_t)track;
    return 0;
}

/* Rider loop volume and pitch from a speed: silent at 0, a ramp below 0x20, full above. */
static void loopLevel(uint32_t speed, uint8_t *volume, uint32_t *cents)
{
    *cents = 0;
    if (speed == 0) {
        *volume = 0;
        return;
    }
    *volume = speed < 0x20 ? (uint8_t)(speed * 4 * gBmxSfxVolume >> 7) : gBmxSfxVolume;
    *cents = speed * 8 - 0x1a0;
}

/*
 * Event 4000: updates the two loops of rider `bits` >> 28 at `position`. bits: byte 0 the voice of loop A,
 * byte 1 how loop A changes voice (1: that voice, or 0xb when bits 24..27 are set; 2: 12 further on; else
 * unchanged), byte 2 the voice of loop B (+9). The speeds give volume and pitch of each loop.
 */
static int eventRiderLoops(const ZVec3 *position, uint32_t bits, uint32_t speedA, uint32_t speedB)
{
    const ZVec3 pos = *position;
    const uint32_t rider = bits >> 28;
    uint32_t voice = bits & 0xff, cents;
    uint8_t volume;

    loopLevel(speedA, &volume, &cents);
    if ((bits >> 8 & 0xff) == 1) {
        if ((bits >> 24 & 0xf) != 0) {
            voice = 0xb;
        }
        zsndSrcSetVoice(gBmxRiderLoopA[rider], winloadGetVoice(gBmxGameSfxFile, voice));
    } else if ((bits >> 8 & 0xff) == 2) {
        zsndSrcSetVoice(gBmxRiderLoopA[rider], winloadGetVoice(gBmxGameSfxFile, voice + 0xc));
    }
    zsndSrcSetVolume(gBmxRiderLoopA[rider], volume);
    zsndSrcSetPitch(gBmxRiderLoopA[rider], (int16_t)cents);
    loopLevel(speedB, &volume, &cents);
    zsndSrcSetVoice(gBmxRiderLoopB[rider], winloadGetVoice(gBmxGameSfxFile, (bits >> 16 & 0xff) + 9));
    zsndSrcSetVolume(gBmxRiderLoopB[rider], volume);
    zsndSrcSetPitch(gBmxRiderLoopB[rider], (int16_t)cents);
    zsndSrcSetPosition(gBmxRiderLoopA[rider], &pos);
    zsndSrcSetPosition(gBmxRiderLoopB[rider], &pos);
    return 0;
}

/* Event 4001 (camera position): on level 4 swaps the quiet ambient track at x = 0 and fades by height. */
static int eventAmbientSide(const ZVec3 *position)
{
    ZVec3 pos;
    uint32_t i;

    if (position == 0) {
        zdebugFail("NULL pointer passed as positional SFX argument", bmxsoundFile, 0x4c7, 0);
    }
    pos = *position; // NOLINT(clang-analyzer-core.NullDereference): the assert does not return
    if (gBmxLevelIndex != 4) {
        return 0;
    }
    if (!(pos.x > 0.0f)) {
        if (gBmxAmbientSide == 0) {
            setEffect(2, 1);
            for (i = 0; i < winloadGetTrackDefCount(gBmxLevelSfxFile); i++) {
                zsndSeqSetVolume(gBmxAmbientTracks[i], sideVolume(i, 1), 0);
            }
            gBmxAmbientSide = 1;
        }
        return 0;
    }
    if (gBmxAmbientSide != 0) {
        setEffect(2, 0x46);
        for (i = 0; i < winloadGetTrackDefCount(gBmxLevelSfxFile); i++) {
            zsndSeqSetVolume(gBmxAmbientTracks[i], sideVolume(i, 0), 0);
        }
        gBmxAmbientSide = 0;
    }
    if (winloadGetTrackDefCount(gBmxLevelSfxFile) != 0) {
        /* The first track fades with the distance from y = -2000, down to 50. */
        const int32_t drop =
            bmxFtol(__builtin_fabs((double)pos.y - (double)-2000.0f) * (double)0x1.5acb7p-5f);
        const int32_t level = (int32_t)(0x7fu - (uint32_t)drop);
        const uint8_t volume = level < 0x32 ? 0x32 : (uint8_t)level;

        zsndSeqSetVolume(gBmxAmbientTracks[0], (uint8_t)((uint32_t)gBmxSfxVolume * volume >> 7), 0);
    }
    return 0;
}

/*
 * Events 4002 / 4003: a prop sound of the level file (4003 uses the second set). action 0 creates a 3-D
 * source (returns its handle), 1 destroys source `value`, 2 moves it and sets its volume, 3 plays sound
 * `value` once.
 */
static int eventProp(int id, const ZVec3 *position, uint32_t action, uint32_t value, uint32_t volume)
{
    const uint32_t set = id != 0xfa2 ? 8 : 0;
    ZVec3 pos;

    if (position == 0) {
        zdebugFail("NULL pointer passed as positional SFX argument", bmxsoundFile, 0x46c, 0);
    }
    pos = *position; // NOLINT(clang-analyzer-core.NullDereference): the assert does not return
    switch (action) {
    case 0: {
        uint16_t source;

        if (value > 3) {
            zdebugFail("prop/level sound type out of range", bmxsoundFile, 0x472, 0);
        }
        source = zsndSrcCreate(winloadGetVoice(gBmxLevelSfxFile, set + value + 4), &pos);
        if (source != 0xffff) {
            zsndSrcSetVolume(source, (uint8_t)((uint32_t)gBmxSfxVolume * volume >> 7));
        }
        return source;
    }
    case 1:
        if ((uint16_t)value != 0xffff) {
            zsndSrcDestroy((uint16_t)value);
        }
        return 0;
    case 2:
        if ((uint16_t)value != 0xffff) {
            zsndSrcSetVolume((uint16_t)value, (uint8_t)((uint32_t)gBmxSfxVolume * volume >> 7));
            zsndSrcSetPosition((uint16_t)value, &pos);
        }
        return 0;
    case 3: {
        uint8_t measured;
        int8_t pan;

        bmxSoundMeasure(position, (uint8_t)((uint32_t)gBmxSfxVolume * volume >> 7), &measured, &pan);
        if (value > 3) {
            zdebugFail("prop/level sound type out of range", bmxsoundFile, 0x485, 0);
        }
        playVoice(gBmxLevelSfxFile, set + value, 0, measured, (uint32_t)pan, 0x20);
        return 0;
    }
    default:
        return 0;
    }
}

/* Event 3007: the looping gun sound, allocated on first use. */
static int eventGun(const ZVec3 *position, uint32_t voice)
{
    uint8_t volume;
    int8_t pan;

    bmxSoundMeasure(position, gBmxSfxVolume, &volume, &pan);
    if (gBmxGunChannel == 0xff) {
        gBmxGunChannel = winchanVoiceAlloc(winloadGetVoice(gBmxGunSfxFile, voice), 0x21);
        if (gBmxGunChannel == 0xff) {
            return 0;
        }
    }
    winchanVoiceSetVolume(gBmxGunChannel, volume);
    winchanVoiceSetPan(gBmxGunChannel, pan);
    winchanVoiceStart(gBmxGunChannel);
    return 0;
}

/* Events 3001 .. 3009: rider and object sounds at a position, at the distance volume and pan. */
static int eventPositional(int id, const ZVec3 *position, uint32_t a, uint32_t b, uint32_t random)
{
    uint16_t file = gBmxGameSfxFile;
    uint32_t index, cents = 0, volume;
    uint8_t measured;
    int8_t pan;

    if (id == 0xbbf) {
        return eventGun(position, a);
    }
    bmxSoundMeasure(position, gBmxSfxVolume, &measured, &pan);
    volume = measured;
    switch (id) {
    case 0xbb9:
        index = (a & 0xff) + 0x15;
        volume = (uint32_t)measured * b >> 7;
        break;
    case 0xbba:
        switch (a >> 8 & 0xff) {
        case 2:
            index = (a & 0xff) + 0x31;
            break;
        case 3:
            index = 0x3a;
            break;
        case 4:
            index = 0x3c;
            break;
        case 5:
            index = 0x3b;
            break;
        default:
            index = (a & 0xff) + 0x28;
            break;
        }
        volume = (uint32_t)measured * b >> 7;
        break;
    case 0xbbb:
        index = (a & 0xff) + 0x1f;
        volume = (uint32_t)measured * b >> 7;
        break;
    case 0xbbc:
        index = (a >> 8 & 0xff) * 3 + ((b & 0xffffffe0u) <= 0x40 ? b >> 5 : 2) + 0x3d;
        volume = (uint32_t)measured * b >> 7;
        break;
    case 0xbbd:
        index = a + 0x58;
        volume = (uint8_t)((b * 32 + 0x1f) * measured >> 7);
        break;
    case 0xbbe:
        file = gBmxRiderSfxFile;
        index = random & 3;
        cents = b << 6;
        volume = (uint8_t)((b * 16 + 0x4f) * measured >> 7);
        break;
    case 0xbc0:
        file = gBmxGunSfxFile;
        index = a + 4;
        break;
    default: /* 0xbc1 */
        file = gBmxGunSfxFile;
        index = a + 0xa;
        break;
    }
    playVoice(file, index, cents, volume, (uint32_t)pan, 0x20);
    return 0;
}

/* Calls zmathRandom once for every event while the sound is open (the rider sounds use it). */
BMX_ADDR(0x0044e300)
int bmxSoundPlayAt(int id, const void *arg1, int arg2, int arg3, int arg4)
{
    const uint32_t n = (uint32_t)(uintptr_t)arg1;
    const uint32_t a = (uint32_t)arg2;
    const uint32_t b = (uint32_t)arg3;
    const uint32_t event = (uint32_t)id;
    uint32_t random;

    if (gBmxSoundState == kBmxSoundClosed) {
        return 0;
    }
    random = zmathRandom();
    /* Muted ranges: game sounds in music mode 2, rider sounds while paused. */
    if (gBmxSoundState == kBmxSoundInGame) {
        if (gBmxMusicMode == 2 && event >= 5000 && event < 6000) {
            return 0;
        }
    } else if (gBmxSoundState == 3 && event >= 3001 && event < 4001) {
        return 0;
    }
    switch (event) {
    case 1000:
        return eventState((int32_t)n, a, b, arg4);
    case 1001:
        return eventMusicMode((int32_t)n);
    case 1002:
        return eventVolume(n, (uint8_t)a);
    case 1003:
        setEffect((uint8_t)n, (uint8_t)a);
        return 0;
    case 1004:
        return gBmxNextTrack != kBmxSoundNoTrack ? (int8_t)gBmxNextTrack : (int8_t)gBmxCurrentTrack;
    case 1005:
        return eventNextTrack(n);
    case 2000:
    case 2001:
    case 2002:
    case 2004:
    case 2005:
        playVoice(gBmxSfxFile, event - 2000, 0, gBmxSfxVolume, 0, 0x21);
        return 0;
    case 2003:
        playVoice(gBmxSfxFile, gBmxSoundState == 1 ? 5 : 3, 0, gBmxSfxVolume, 0, 0x21);
        return 0;
    case 2008:
    case 2009:
        return 0;
    case 3000:
        if (n != 0 || a >= winloadGetVoiceCount(gBmxSfxFile)) {
            return 0;
        }
        playVoice(gBmxSfxFile, a, 0, gBmxSfxVolume, 0, 0x20);
        return 0;
    case 0xbb9:
    case 0xbba:
    case 0xbbb:
    case 0xbbc:
    case 0xbbd:
    case 0xbbe:
    case 0xbbf:
    case 0xbc0:
    case 0xbc1:
        return eventPositional(id, arg1, a, b, random);
    case 4000:
        return eventRiderLoops(arg1, a, b, (uint32_t)arg4);
    case 4001:
        return eventAmbientSide(arg1);
    case 4002:
    case 4003:
        return eventProp(id, arg1, a, b, (uint32_t)arg4);
    case 5000:
        if (n == 0) {
            playVoice(gBmxGameSfxFile, 0x5d, 0, gBmxSfxVolume, 0, 0x21);
        } else if (n <= 10) {
            playVoice(gBmxGameSfxFile, 0x5c, 0, gBmxSfxVolume, 0, 0x21);
        }
        return 0;
    case 5001: {
        const uint32_t cents = n * 33;

        playVoice(gBmxGameSfxFile, 0x5e, cents < 500 ? cents : 500, gBmxSfxVolume, 0, 0x21);
        return 0;
    }
    case 5002:
        return 0;
    case 5003: {
        const uint8_t volume = (uint8_t)((uint8_t)n * 16 + 0x1f);
        const uint32_t index = a != 0 ? (n < 4 ? 0x10 : 0x11) : (n < 4 ? 0x12 : 0x13);

        playVoice(gBmxLevelSfxFile, index, n * 30 - (random & 0x1ff) + 0xc4,
                  (uint32_t)gBmxSfxVolume * volume >> 7, 0, 0x20);
        return 0;
    }
    case 5004:
        playVoice(gBmxGameSfxFile, n == 0 ? 0x60 : n == 1 ? 0x5f : 0x61, 0, gBmxSfxVolume, 0, 0x21);
        return 0;
    case 5005:
        playVoice(gBmxGameSfxFile, n + 0x61, 0, gBmxSfxVolume, 0, 0x21);
        return 0;
    case 5006:
        playVoice(gBmxGameSfxFile, n + 0x64, 0, gBmxSfxVolume, 0, 0x21);
        return 0;
    case 5007:
    case 5008:
    case 5009:
    case 5010:
        playVoice(gBmxGameSfxFile, event - 0x1327, 0, gBmxSfxVolume, 0, 0x21);
        return 0;
    case 5011:
        playVoice(gBmxGameSfxFile, 0x6c, 0, gBmxSfxVolume, 0, 0x20);
        playVoice(gBmxGameSfxFile, 0x6d, 0, gBmxSfxVolume, 0, 0x20);
        return 0;
    case 6000:
        playVoice(gBmxGameSfxFile, 0x6f, 0, gBmxSfxVolume, 0, 0x21);
        return 0;
    default:
        return 1;
    }
}
