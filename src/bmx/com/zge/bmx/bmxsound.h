/*
 * Game sound front end (C:\zaxis\Bmx\src\com\zge\bmx\bmxsound.c): opens and closes the sound system (mixer,
 * sequencer, 3-D sources, the .zsd files, the 16 ms timer), runs the per-frame update with the background
 * music stream, and plays sounds through one event dispatcher (bmxSoundPlayAt). The menu application hooks
 * (shutdown / update / exit flag) are filed here too by the packet.
 */
#ifndef BMX_COM_ZGE_BMX_BMXSOUND_H
#define BMX_COM_ZGE_BMX_BMXSOUND_H

#include <stdint.h>

#include "bmx/com/zge/ngif/missingzge.h"
#include "bmx/pc/zge/sound/winload.h"
#include "compat/orig.h"

/* gBmxSoundState */
enum {
    kBmxSoundClosed = -1,
    kBmxSoundOpen = 0,
    kBmxSoundInGame = 2, /* background music plays during the update */
};

enum {
    kBmxSoundNoTrack = 0xff,
    kBmxSoundTrackCooldown = 0x3c, /* updates before the next music track change */
    kBmxSoundTimerMillis = 16,
    kBmxSoundSourceRange = 0x9c4,    /* audible range of 3-D sources */
    kBmxSoundSourceLifetimeMs = 100, /* lifetime of a source between position updates */
    kBmxSoundTicksPerSecond = 30,
    kBmxSoundMixerRate = 0x5622,
    kBmxSoundMusicFormat = 0x40, /* stream format flags of the music (see winstrm.h): 44100 Hz ADPCM */
};

typedef uint8_t BmxLevelTrackTable[20];

/* gBmxSoundState: -1 closed, 0 open, 1 menu, 2 in game, 3 in game paused. */
#define gBmxSoundState ORIG_VAR(0x0055b530, int32_t, gBmxSoundState)
#define gBmxMusicMode  ORIG_VAR(0x0055b534, int32_t, gBmxMusicMode)
/* The loaded sound files (winload.h handles). */
#define gBmxSfxFile     ORIG_VAR(0x0055b53c, uint16_t, gBmxSfxFile)
#define gBmxMusicFile   ORIG_VAR(0x0055b548, uint16_t, gBmxMusicFile)
#define gBmxSpeechFile  ORIG_VAR(0x0055b54a, uint16_t, gBmxSpeechFile)
#define gBmxMusicVolume ORIG_VAR(0x0055b539, uint8_t, gBmxMusicVolume)
#define gBmxSfxVolume   ORIG_VAR(0x0055b538, uint8_t, gBmxSfxVolume)
/* The volumes as event 1002 indexes them: 0 effects, 1 music (higher indices write on into the file handles).
 */
typedef uint8_t BmxVolumeTable[4];
#define gBmxVolumes ORIG_VAR(0x0055b538, BmxVolumeTable, gBmxVolumes)
/* Sound files loaded for the menu and for a level (0xffff when not loaded). */
#define gBmxMenuSfxFile  ORIG_VAR(0x0055b53e, uint16_t, gBmxMenuSfxFile)
#define gBmxGameSfxFile  ORIG_VAR(0x0055b540, uint16_t, gBmxGameSfxFile)
#define gBmxGunSfxFile   ORIG_VAR(0x0055b542, uint16_t, gBmxGunSfxFile)
#define gBmxRiderSfxFile ORIG_VAR(0x0055b544, uint16_t, gBmxRiderSfxFile)
#define gBmxLevelSfxFile ORIG_VAR(0x0055b546, uint16_t, gBmxLevelSfxFile)
#define gBmxNextTrack    ORIG_VAR(0x0055b54c, uint8_t, gBmxNextTrack)
#define gBmxTrackLoop    ORIG_VAR(0x0055b54d, uint8_t, gBmxTrackLoop)
#define gBmxLevelTracks  ORIG_VAR(0x0055b550, BmxLevelTrackTable, gBmxLevelTracks)
#define gBmxCurrentTrack ORIG_VAR(0x00574668, uint8_t, gBmxCurrentTrack)
#define gBmxLevelIndex   ORIG_VAR(0x0057469e, uint8_t, gBmxLevelIndex)
#define gBmxRiderIndex   ORIG_VAR(0x0057469f, uint8_t, gBmxRiderIndex)
/*
 * Two looping 3-D sources per rider (the rolling sound and a second layer), indexed by the player number of
 * event 4000 (only 0 and 1 exist; the table is typed long enough for the 4 bit index the event takes).
 */
typedef uint16_t BmxRiderLoopTable[16];
#define gBmxRiderLoopA ORIG_VAR(0x00574670, BmxRiderLoopTable, gBmxRiderLoopA)
#define gBmxRiderLoopB ORIG_VAR(0x0057466c, BmxRiderLoopTable, gBmxRiderLoopB)
/* Sequencer tracks of the level's ambient sound file (one per track definition). */
typedef uint8_t BmxAmbientTrackTable[8];
#define gBmxAmbientTracks ORIG_VAR(0x00574678, BmxAmbientTrackTable, gBmxAmbientTracks)
/* Level 4 swaps which ambient track plays quietly when the camera crosses x = 0. */
#define gBmxAmbientSide ORIG_VAR(0x005746a0, uint8_t, gBmxAmbientSide)
/* Mixer channel of the looping gun sound, 0xff none. */
#define gBmxGunChannel ORIG_VAR(0x00574698, uint8_t, gBmxGunChannel)
/* The two looping menu sounds. */
typedef uint8_t BmxMenuChannelTable[2];
#define gBmxMenuChannels ORIG_VAR(0x0057469c, BmxMenuChannelTable, gBmxMenuChannels)
/* Which of the two menu sound sets loads next (alternates). */
#define gBmxMenuSfxSet ORIG_VAR(0x005746ac, uint8_t, gBmxMenuSfxSet)
struct WinstrmStream;
struct ZVec3;

#define gBmxMusicStream    ORIG_VAR(0x005746a4, struct WinstrmStream *, gBmxMusicStream)
#define gBmxTrackCooldown  ORIG_VAR(0x005746a8, uint32_t, gBmxTrackCooldown)
#define gBmxTimerId        ORIG_VAR(0x00574674, uint32_t, gBmxTimerId)
#define gBmxTimerPeriod    ORIG_VAR(0x00574694, uint32_t, gBmxTimerPeriod)
#define gBmxMenuExit       ORIG_VAR(0x00574654, int32_t, gBmxMenuExit)
#define gBmxMenuExitResult ORIG_VAR(0x00574658, int32_t, gBmxMenuExitResult)

/*
 * The sound event dispatcher. `id` selects the event; `arg1` is a world position (ZVec3) for the positional
 * sounds and a plain number otherwise. Events: 1000 change state (menu / level / pause), 1001 music mode,
 * 1002 set a volume, 1003 effect, 1004 current track, 1005 next track, 2000.. sounds of the common file,
 * 3000.. rider and object sounds, 4000.. rider loops, ambient side and props, 5000.. game sounds. Returns 0
 * when handled, 1 for an unknown event, or the event's value (a track, a source handle).
 */
int bmxSoundPlayAt(int id, const void *arg1, int arg2, int arg3, int arg4);

/* The same entry with the second argument as a plain number (a channel or an index, not a position). */
static inline int bmxSoundPlayEvent(int id, int arg1, int arg2, int arg3, int arg4)
{
    return bmxSoundPlayAt(id, (const void *)(intptr_t)arg1, arg2, arg3, arg4);
}

/* Not ported (other owners). */
/* Platform object lookup in the exe (function pointer at 0x00561488): kind 0x15 is the window holder. */
typedef int(BMX_CDECL *ZsystemGetObjectFn)(int kind, void **object, int flags);
#define gZsystemGetObject ORIG_VAR(0x00561488, ZsystemGetObjectFn, gZsystemGetObject)

/* Menu application hooks: logs, stops the sounds and shuts the GUI down. */
void bmxMenuShutdown(void);
/* Repeats the d-pad, then returns 0 when the menu should quit (and reports the error when it asked to fail).
 */
int bmxMenuUpdate(void);
/* Requests the menu to exit with `result` (0: failure). */
void bmxMenuRequestExit(int result);
/* Forwards to zsndSetTickRate. */
int bmxSoundSetTickRate(int ticks);
/* Forwards to winchanProbeDevice: checks that DirectSound works with the window. */
uint32_t bmxSoundProbeDevice(void *window);
/* Returns 0. */
int bmxSoundNop(void);

/* Opens the sound system: mixer, sequencer, sources, sound files and the 16 ms timer. */
void bmxSoundOpen(void);
void bmxSoundClose(void);
/* Per-frame update: music track changes, the sequencer, the 3-D sources, the stream fades. */
void bmxSoundUpdate(void);
/* Keeps the streams fed (does nothing when the sound is closed). */
void bmxSoundUpdateStreams(void);
/*
 * Volume and pan of a sound at a world position: `volume` scaled by the distance volume, or just `volume`
 * with centre pan when there is no position. Returns 1 when out of range or without a position.
 */
int bmxSoundMeasure(const struct ZVec3 *position, uint8_t volume, uint8_t *outVolume, int8_t *outPan);

#endif
