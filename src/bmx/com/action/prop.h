/*
 * Level props (C:\zaxis\Bmx\src\com\action\prop.c): the interactive objects of a level that goals refer
 * to (collectables, breakable boards, ramps that move). The state of the 16 props of the current level
 * lives in one 0xb40 byte block that replays save and restore.
 */
#ifndef BMX_COM_ACTION_PROP_H
#define BMX_COM_ACTION_PROP_H

#include <stdint.h>

#include "bmx/com/action/physics_hit.h"
#include "bmx/com/action/physics_types.h"
#include "compat/orig.h"
#include "zge/zmath.h"

/* Definition of a prop of a level (10 bytes, 16 per level at 0x557d28). */
typedef struct PropDef {
    uint16_t id;       /* 0x00 id the goals and the level's prop tags use */
    uint8_t type;      /* 0x02 0 collectable, 3 / 4 / 5 other behaviours (see propCollect) */
    uint8_t kind;      /* 0x03 8, 9: float up, 11, 12: collect on touch, 0x10: slides in */
    uint16_t paramA;   /* 0x04 copied to PropRecord.paramA */
    uint16_t paramB;   /* 0x06 */
    uint8_t sound;     /* 0x08 sound played when it is collected (0xff: none) */
    uint8_t soundStop; /* 0x09 sound that keeps playing for it (0xff: none) */
} PropDef;

_Static_assert(sizeof(PropDef) == 10, "PropDef size");
typedef PropDef PropDefs[12][16];
#define gPropDefs ORIG_VAR(0x00557d28, PropDefs, gPropDefs)

/* State of a prop of the current level (0x84 bytes, 16 at 0x694460). */
typedef struct PropRecord {
    uint8_t state;          /* 0x00 0 unused, 1 in place, 3 collected */
    uint8_t flags;          /* 0x01 bit 1: set when the prop's object has the signalled flag */
    uint8_t index;          /* 0x02 index into the level's PropDefs */
    uint8_t player;         /* 0x03 who collected it */
    uint32_t timer;         /* 0x04 */
    PhysPropObject *object; /* 0x08 */
    ZVec3 pos;              /* 0x0c */
    uint8_t pad18[4];
    ZVec3 vel; /* 0x1c */
    uint8_t pad28[4];
    ZMatrix orient; /* 0x2c */
    ZAngle3 angles; /* 0x6c turn applied to the orientation by propOrient */
    uint8_t pad72[2];
    uint32_t sound;  /* 0x74 handle of the looping sound */
    uint32_t mode;   /* 0x78 sub state of the movement / whether it is shown (kind 5) */
    uint32_t paramA; /* 0x7c from PropDef.paramA */
    uint32_t paramB; /* 0x80 */
} PropRecord;

_Static_assert(sizeof(PropRecord) == 0x84, "PropRecord size");

/* Visibility of a prop's node in the world: bit 0 set once the player hit its face. */
typedef struct PropNodeState {
    uint32_t flags;
    PhysPropNode *node;
    uint32_t pad08;
} PropNodeState;

/* The saved block: the records, a word, and the node states (the last 4 bytes of the last one are not saved).
 */
typedef struct PropState {
    PropRecord record[16];
    uint32_t pad840;
    uint8_t nodes[0xb40 - 0x844];
} PropState;

_Static_assert(sizeof(PropState) == 0xb40, "PropState size");
typedef PropRecord PropRecords[16];
typedef PropNodeState PropNodeStates[64];
#define gPropState      ORIG_VAR(0x00694460, PropState, gPropState)
#define gPropRecords    ORIG_VAR(0x00694460, PropRecords, gPropRecords)
#define gPropNodeStates ORIG_VAR(0x00694ca4, PropNodeStates, gPropNodeStates)
#define gPropDefaults   ORIG_VAR(0x0056c4f8, PropState, gPropDefaults)

/* Starts a prop moving (collect: release == 0) or putting it back (release != 0). */
void propSwitch(PropRecord *prop, uint32_t player, int release);
/* Per frame update of the props of the level: timers, movement, rider pushes, looping sounds. */
void propUpdate(void);

/* The prop node whose tag has this id (NULL if none). */
PhysPropNode *propFindNode(uint32_t id);
/* Stops the sounds of the props that keep one playing. */
void propStopSounds(void);
uint32_t propReturnZero(void);
/* Copies a saved state in and re-applies it to the world. */
void propLoadState(const void *state);
/* Resets the prop system to the level's defaults. */
void propsReset(void);
/* The prop record with this id on the current level (NULL and a debug message if missing). */
uint8_t *propFind(uint32_t id);
/* Marks a prop as collected by a player / puts it back. */
void propCollect(uint32_t player, uint8_t *prop);
void propRelease(uint32_t player, uint8_t *prop);
/* Notifies a prop that player `player` touched it; returns 0 when the touch is absorbed. */
int propTouch(int player, void *prop);
/* A player hit a face of a prop: breaks the tagged board node. */
void propHitFace(int player, PhysFace *face);
/* Re-applies the turn of a prop when it has one. */
void propApplyTurn(PropRecord *prop);
/* Applies the angles of a prop to its orientation and to the world object. */
void propOrient(PropRecord *prop);
void propNop(void);
/* Per frame movement of the floating and sliding props. */
void propUpdateMove(PropRecord *prop);
/* Shows or hides the nodes that belong to a prop (and its +1 twin) and stores its mode. */
void propSetShown(PropRecord *prop, int shown);
/* Sets up the 16 props of the current level from the level's collision data and saves them as the defaults.
 */
void propInit(void);

#endif
