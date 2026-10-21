/*
 * Action loop (C:\zaxis\Bmx\src\com\action\action.c): sets up a run (riders, assets, motion bank), runs the
 * per frame state machine of the in-game pause / replay / demo screens, and tears everything down again.
 */
#ifndef BMX_COM_ACTION_ACTION_H
#define BMX_COM_ACTION_ACTION_H

#include <stdint.h>

#include "bmx/com/action/camera.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/replay.h"
#include "bmx/com/action/rider.h"
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "compat/orig.h"

/* Mode of the action loop (byte at 0x551a30). */
enum {
    kActionRun = 0,      /* riding */
    kActionPaused = 1,   /* pause menu */
    kActionReplay = 2,   /* replay of the last run */
    kActionDemo = 3,     /* attract mode demo */
    kActionPlayback = 4, /* recorded demo replay */
};

/* State of the screen layer on top of the action loop (0x56b3e0, 0x1c bytes). */
typedef struct ActionState {
    uint8_t step;   /* 0x00 sub state of the current mode */
    uint8_t replay; /* 0x01 replay handle of the demo playback */
    uint8_t player; /* 0x02 player the screens are shown for */
    uint8_t pad03;
    uint32_t field04;  /* 0x04 cleared when a mode starts */
    uint8_t field08;   /* 0x08 */
    uint8_t savedView; /* 0x09 camera view restored when a screen closes */
    uint8_t pad0a[2];
    uint32_t timer;    /* 0x0c frames in the current step */
    uint32_t field10;  /* 0x10 */
    uint32_t hudShown; /* 0x14 hud elements are visible */
    uint8_t random;    /* 0x18 counter that picks the next demo */
    uint8_t pad19;
    uint16_t holdCount; /* 0x1a frames both shoulder buttons are held */
} ActionState;

_Static_assert(sizeof(ActionState) == 0x1c, "ActionState size");

typedef uint8_t ActionBytes6[6];
typedef uint8_t ActionBytes256[0x100];

#define gAction            ORIG_VAR(0x0056b3e0, ActionState, gAction)
#define gActionMode        ORIG_VAR(0x00551a30, uint8_t, gActionMode)
#define gActionDemoTable   ORIG_VAR(0x00551a34, ActionBytes6, gActionDemoTable)  /* {rider, level} per demo */
#define gActionMapTable    ORIG_VAR(0x00551fb8, ActionBytes256, gActionMapTable) /* map per level, 12 each */
#define gActionFrameReload ORIG_VAR(0x00551a3c, int32_t, gActionFrameReload)
#define gActionFrameCount  ORIG_VAR(0x00551a40, int32_t, gActionFrameCount)
#define gActionQuit        ORIG_VAR(0x00551fd4, uint32_t, gActionQuit)
#define gActionSubMode     ORIG_VAR(0x0056b616, uint8_t, gActionSubMode)
#define gActionLanguage    ORIG_VAR(0x00696218, uint8_t, gActionLanguage)
#define gActionOptions     ORIG_VAR(0x006955e2, uint8_t, gActionOptions)
#define gActionReplayLib   ORIG_VAR(0x0068ffa8, int32_t, gActionReplayLib)
typedef uint32_t ActionMotions[0xee];
#define gActionMotions ORIG_VAR(0x00696300, ActionMotions, gActionMotions)

/* Changes the action state: 0 resume, 1 pause, 2 restart, -1 leave the screen overlay running. */
uint32_t actionSetMode(uint32_t mode);
/* 0x40249b is a linker jump thunk to zrollerShutdown that the table lists as part of this file. */
int actionShutdownRoller(void);
/* Opens the motion library; 1 on failure. */
int actionOpenMotions(void);
void actionCloseMotions(void);
/* Picks the rider and level of the next demo (low byte of the counter mod 3); returns counter / 3. */
uint32_t actionPickDemo(void);
/* Creates the riders and loads the assets of a run; 1 on failure. */
uint32_t actionInitialize(void);
uint32_t actionStopSounds(void);
/* One frame of the pause / replay / demo screens. */
uint32_t actionUpdate(void);
void actionShutdown(void);

/*
 * Callees that are not ported yet. Names are guessed from what the functions do.
 */
ORIG_FN(0x00405780, void, actionStepPlayers, (void));
ORIG_FN(0x004078c0, void, comdataSetView, (uint32_t player, uint32_t view));
ORIG_FN(0x0042deb0, void, propInit, (void));
ORIG_FN(0x0042e2f0, void, propUpdate, (void));
ORIG_FN(0x0042fd50, void, replayUpdate, (void));
ORIG_FN(0x00430410, void, replaySetMode, (uint32_t handle, uint32_t mode));
ORIG_FN(0x0045a1a0, void, zsystemEndFrame, (void));

#endif
