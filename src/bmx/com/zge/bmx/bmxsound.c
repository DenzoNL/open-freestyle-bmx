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
    desc.reserved = 0;
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
