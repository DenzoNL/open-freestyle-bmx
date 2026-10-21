/*
 * Game sound front end (C:\zaxis\Bmx\src\com\zge\bmx\bmxsound.c): opens and closes the sound system (mixer,
 * sequencer, 3-D sources, the .zsd files, the 16 ms timer), runs the per-frame update with the background
 * music stream, and plays sounds. bmxSoundPlayAt (0x0044e300, the 5.7 KB sound dispatcher) is in a separate
 * pass. The menu application hooks (shutdown / update / exit flag) are filed here too by the packet.
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

#define gBmxSoundState ORIG_VAR(0x0055b530, int32_t, gBmxSoundState)
#define gBmxMusicMode  ORIG_VAR(0x0055b534, int32_t, gBmxMusicMode)
/* The loaded sound files (winload.h handles). */
#define gBmxSfxFile      ORIG_VAR(0x0055b53c, uint16_t, gBmxSfxFile)
#define gBmxMusicFile    ORIG_VAR(0x0055b548, uint16_t, gBmxMusicFile)
#define gBmxSpeechFile   ORIG_VAR(0x0055b54a, uint16_t, gBmxSpeechFile)
#define gBmxMusicVolume  ORIG_VAR(0x0055b539, uint8_t, gBmxMusicVolume)
#define gBmxNextTrack    ORIG_VAR(0x0055b54c, uint8_t, gBmxNextTrack)
#define gBmxTrackLoop    ORIG_VAR(0x0055b54d, uint8_t, gBmxTrackLoop)
#define gBmxLevelTracks  ORIG_VAR(0x0055b550, BmxLevelTrackTable, gBmxLevelTracks)
#define gBmxCurrentTrack ORIG_VAR(0x00574668, uint8_t, gBmxCurrentTrack)
#define gBmxLevelIndex   ORIG_VAR(0x0057469e, uint8_t, gBmxLevelIndex)
struct WinstrmStream;
struct ZVec3;

#define gBmxMusicStream    ORIG_VAR(0x005746a4, struct WinstrmStream *, gBmxMusicStream)
#define gBmxTrackCooldown  ORIG_VAR(0x005746a8, uint32_t, gBmxTrackCooldown)
#define gBmxTimerId        ORIG_VAR(0x00574674, uint32_t, gBmxTimerId)
#define gBmxTimerPeriod    ORIG_VAR(0x00574694, uint32_t, gBmxTimerPeriod)
#define gBmxMenuExit       ORIG_VAR(0x00574654, int32_t, gBmxMenuExit)
#define gBmxMenuExitResult ORIG_VAR(0x00574658, int32_t, gBmxMenuExitResult)

/* Plays sound `id` at a world position at the given volume (0..127). */
ORIG_FN(0x0044e300, int, bmxSoundPlayAt, (int id, const void *position, int a, int volume, int b));

/* The same entry with the second argument as a plain number (a channel or an index, not a position). */
ORIG_FN(0x0044e300, int, bmxSoundPlayEvent, (int id, int a, int b, int c, int d));

/* Not ported (other owners). */
/* Sets the timer ticks per second the sequencer and sources count in (0x0055ffc0). Returns the value. */
ORIG_FN(0x0047eba0, int, zsndSetTickRate, (int ticks));
/* Returns 0. */
ORIG_FN(0x0047ebc0, int, zsndNop, (void));
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
