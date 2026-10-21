/*
 * Controller handling (C:\zaxis\Bmx\src\com\action\contrler.c): two pads, each polled from an engine
 * device, with button edges, analog sticks and the mapping of game actions to buttons / stick directions.
 */
#ifndef BMX_COM_ACTION_CONTRLER_H
#define BMX_COM_ACTION_CONTRLER_H

#include <stdint.h>

#include "compat/orig.h"

/* Device of the engine's controller layer (0x480370): status bit 0 = connected. */
typedef struct ContrlerDevice {
    uint32_t unknown0;
    uint8_t status; /* 0x04 */
    uint8_t pad05[3];
    uint32_t held; /* 0x08 buttons down */
    uint8_t pad0c[8];
    uint32_t axis[4]; /* 0x14 sticks (floats; only copied here) */
} ContrlerDevice;

/*
 * What one poll yields. Buttons are bit masks (d-pad 1/2/4/8 = up/down/left/right as the code that
 * derives them from the sticks). axis[0..1] is the left stick, axis[2..3] the right one; prevAxis holds
 * the values of the previous poll.
 */
typedef struct ContrlerState {
    uint32_t buttons;  /* 0x00 */
    uint32_t pressed;  /* 0x04 buttons that went down (or up) this poll */
    float axis[4];     /* 0x08 */
    float prevAxis[4]; /* 0x18 */
    uint8_t pad28[4];
} ContrlerState;

typedef struct ContrlerPad {
    ContrlerDevice *device; /* 0x00 */
    uint32_t stuck;  /* 0x04 buttons held since before a (re)connect or mode switch: ignored until released */
    uint32_t saved;  /* 0x08 buttons saved while the pad runs in another mode (contrlerSetMode) */
    uint16_t flags;  /* 0x0c bit 0: device disconnected, bit 1: reconnected by the last poll */
    uint16_t flags2; /* 0x0e */
    ContrlerState state; /* 0x10 */
} ContrlerPad;

_Static_assert(sizeof(ContrlerPad) == 0x3c, "ContrlerPad size");

typedef ContrlerPad ContrlerPads[2];
#define gContrlerPads ORIG_VAR(0x0056b628, ContrlerPads, gContrlerPads)
#define gContrlerMode ORIG_VAR(0x0056b620, uint8_t, gContrlerMode)
typedef float ContrlerAxes[2];
#define gContrlerAxes ORIG_VAR(0x0056b6a0, ContrlerAxes, gContrlerAxes)

/* How a binding is tested (mode argument of contrlerTest); other values never match. */
enum {
    kContrlerNewPress = 0,    /* a mask button went down this poll */
    kContrlerHeldChanged = 1, /* all mask buttons held, some changed this poll */
    kContrlerAllHeld = 2,
    kContrlerAnyHeld = 3,
    kContrlerUpChanged = 4,  /* all mask buttons up, some changed this poll */
    kContrlerNewRelease = 5, /* a mask button went up this poll */
    kContrlerAlways = 6,
};

/* One game action's binding for a status (mask, test mode); [config][action][status]. */
typedef struct ContrlerBinding {
    uint32_t mask;
    uint32_t mode;
} ContrlerBinding;

typedef ContrlerBinding ContrlerBindings[3][0x15][3];
#define gContrlerBindings ORIG_VAR(0x00553ee0, ContrlerBindings, gContrlerBindings)

/* Creates the engine devices of both pads (asserts when one cannot be created). */
void contrlerCreateDevices(void);
void contrlerFreeDevices(void);
/* Polls both pads; result bit 0: a pad got disconnected, bit 1: a pad got reconnected. */
uint32_t contrlerPoll(void);
void contrlerClear(void);
/* Switches the pads between the menu (0) and the game mode; the held buttons carry over. */
void contrlerSetMode(uint32_t mode);
/* Whether `action` (0..0x14) is in state `status` (0..2) for the controller state record `state`. */
int contrlerAction(uint32_t config, const void *state, uint32_t action, uint32_t status);
/* True when the controller of `player` has one of `mask` newly pressed (mask < 8 are directions); asserts
 * status < 8. */
int contrlerPressed(int player, uint32_t mask, uint32_t status);
/* The two stick axes of stick `index` (0 or 1) of a controller state record, in a static buffer. */
const float *contrlerAxes(const void *state, uint32_t index);
/* Force feedback pulse; empty in the PC build. */
void contrlerRumble(int player, int a, int b);
/* Controller record (asserts index < 2); bits 0 and 1 of the byte at +0xc are the pad flags. */
uint8_t *contrlerGet(uint32_t index);
/* Tests a state against a mask in one of the modes above. */
int contrlerTest(const ContrlerState *state, uint32_t mask, uint32_t mode);

#endif
