/* Level props (C:\zaxis\Bmx\Src\com\action\prop.c). */
#include "bmx/com/action/prop.h"

#include "bmx/com/action/fx.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zstring.h"

/*
 * Floating point follows the original x87 code exactly: `double` is a value held in an x87
 * register (53-bit precision control), `float` is a value the original stored to memory.
 */

enum {
    kPropMax = 16,
    kPropStateSize = 0xb40,
    kPropStateRecord = 1,
    kPropStateCollected = 3,
    kPropTypeCollect = 0, /* PropDef.type values with their own handling */
    kPropTypeBoard = 3,
    kPropTypeFixed = 4,
    kPropTypeToggle = 5,
    kSoundProp = 0xfa3,
    kSoundBreak = 0xfa2,
};

/* Constants of the movement (.rdata). */
#define kPropFloatStep  12000.0f
#define kPropFloatLimit (-8200.0f)
#define kPropSlideScale 0x1.1c71c8p+3f
#define kPropFlatLimit  0x1.0624dep-10f

static const PropDef *propDef(const PropRecord *prop)
{
    return &gPropDefs[0][0] + (uint32_t)gCurrentLevel * 16 + prop->index;
}

/* The prop node whose tag has `id` (compared as a 16 bit range value). */
BMX_ADDR(0x0042de70)
PhysPropNode *propFindNode(uint32_t id)
{
    PhysPropNode *node = *zobjPropList();

    while (node != NULL && (node->tag == NULL || node->tag->range != id)) {
        node = node->next;
    }
    return node;
}

BMX_ADDR(0x0042e270)
void propStopSounds(void)
{
    for (uint32_t i = 0; i < kPropMax; i++) {
        PropRecord *const rec = &gPropRecords[i];

        if (rec->state != 0 && (&gPropDefs[0][0] + (uint32_t)gCurrentLevel * 16 + i)->soundStop != 0xff) {
            bmxSoundPlayAt(kSoundProp, &rec->pos, 1, (int)rec->sound, 0);
        }
    }
}

BMX_ADDR(0x0042eb50)
uint32_t propReturnZero(void)
{
    return 0;
}

/*
 * Loads the saved state block, then re-applies it to the world: moved props are put in place again and
 * the tagged nodes are shown or hidden according to their flags.
 */
BMX_ADDR(0x0042eb70)
void propLoadState(const void *state)
{
    zmemCopy(&gPropState, state, kPropStateSize);
    for (uint32_t i = 0; i < kPropMax; i++) {
        PropRecord *const rec = &gPropRecords[i];
        const PropDef *const def = &gPropDefs[0][0] + (uint32_t)gCurrentLevel * 16 + i;

        if (rec->state == 0 || def->type == kPropTypeFixed) {
            continue;
        }
        if (def->type == kPropTypeToggle) {
            propSetShown(rec, (int)rec->mode);
        } else {
            if ((rec->flags & 2) == 0) {
                zbspObjectSignal(rec->object, 5);
                zbspObjectSignal(rec->object, 3);
            } else {
                zbspObjectSignal(rec->object, 4);
                zbspObjectSignal(rec->object, 2);
            }
            zbspPropSetPosition(rec->object, &rec->pos);
            propOrient(rec);
        }
    }
    for (uint32_t i = 0; i < 64; i++) {
        PropNodeState *const ns = &gPropNodeStates[i];
        const PhysPropNode *const node = ns->node;
        int visible;

        if (node == NULL) {
            return;
        }
        if ((ns->flags & 1) == 0) {
            visible = node->tag != NULL && (node->tag->range & 1) == 0;
        } else {
            visible = !(node->tag != NULL && (node->tag->range & 1) == 0);
        }
        zobjSetVisible((int32_t)(uintptr_t)node->frame, visible);
    }
}

BMX_ADDR(0x0042ecf0)
void propsReset(void)
{
    propLoadState(&gPropDefaults);
}

/* The record of the prop with this id, or NULL (with a debug message). */
BMX_ADDR(0x0042ed10)
uint8_t *propFind(uint32_t id)
{
    const PropDef *def = &gPropDefs[0][0] + (uint32_t)gCurrentLevel * 16;

    for (uint32_t i = 0; i < kPropMax; i++, def++) {
        if (def->id == id && gPropRecords[i].state != 0) {
            return (uint8_t *)&gPropRecords[i];
        }
    }
    gZdebugPrint("bmxpropGetPropByID: couldn't find prop %d\n", id);
    return NULL;
}

BMX_ADDR(0x0042ed80)
void propCollect(uint32_t player, uint8_t *prop)
{
    PropRecord *const rec = (PropRecord *)prop;
    const PropDef *const def = propDef(rec);

    if (rec->state != kPropStateRecord) {
        return;
    }
    switch (def->type) {
    case kPropTypeBoard:
        if (rec->mode != 0) {
            return;
        }
        break;
    case kPropTypeFixed:
        return;
    case kPropTypeToggle:
        if (rec->mode != 0) {
            return;
        }
        propSetShown(rec, 1);
        if (def->sound == 0xff) {
            return;
        }
        bmxSoundPlayAt(kSoundProp, &rec->pos, 3, def->sound, 0x7f);
        return;
    default:
        break;
    }
    propSwitch(rec, player, 0);
    rec->player = (uint8_t)player;
    rec->state = kPropStateCollected;
}

BMX_ADDR(0x0042ee40)
void propRelease(uint32_t player, uint8_t *prop)
{
    PropRecord *const rec = (PropRecord *)prop;

    if (rec->state == 0) {
        return;
    }
    switch (propDef(rec)->type) {
    case kPropTypeFixed:
        break;
    case kPropTypeToggle:
        propSetShown(rec, 1);
        break;
    default:
        propSwitch(rec, player, 1);
        break;
    }
    rec->player = (uint8_t)player;
    rec->state = kPropStateRecord;
}

/* Returns 0 when a collectable (kind 11 or 12) absorbed the touch, else 1. */
BMX_ADDR(0x0042eec0)
int propTouch(int player, void *prop)
{
    const PropDef *const def = propDef(prop);

    if (gameCheckPropGoal(player, prop) == 0) {
        if (def->type != 0) {
            return 1;
        }
        if (def->kind >= 11 && def->kind <= 12) {
            propCollect((uint32_t)player, prop);
        }
    }
    if (def->type == 0 && def->kind >= 11 && def->kind <= 12) {
        return 0;
    }
    return 1;
}

/*
 * A player hit a face that carries a prop tag: breaks the node with that tag (shows its twin with the
 * tag + 1 instead), spawns shards and plays the break sound, then lets the goals see the hit.
 */
BMX_ADDR(0x0042ef60)
void propHitFace(int player, PhysFace *face)
{
    PhysPropNode *const node = propFindNode(face->tag);
    PhysPropNode *const twin = propFindNode((uint32_t)face->tag + 1);
    uint32_t i;

    if (node == NULL) {
        return;
    }
    for (i = 0; i < 64; i++) {
        if (gPropNodeStates[i].node != NULL && gPropNodeStates[i].node == node) {
            break;
        }
    }
    if (i == 64 || (gPropNodeStates[i].flags & 1) != 0) {
        return;
    }
    zobjSetVisible((int32_t)(uintptr_t)node->frame, 0);
    if (twin != NULL) {
        zobjSetVisible((int32_t)(uintptr_t)twin->frame, 1);
        gZobjSetFlags(twin->frame->model, 8, 0);
    }
    {
        const ZVec3 *const at = zobjGetPosition(node->frame);
        const ZVec3 pos = {at->x, at->y, at->z};

        fxShards(&pos, (const uint8_t *)&gPhysRiders[player] + 0x448);
        bmxSoundPlayAt(kSoundBreak, &pos, 3, 3, 0x7f);
    }
    gameCheckGrindGoal(player, (const PhysEdge *)(const void *)face);
    gPropNodeStates[i].flags |= 1;
}

/* Re-applies the orientation when any of the three turn angles is set. */
BMX_ADDR(0x0042f8f0)
void propApplyTurn(PropRecord *prop)
{
    if (prop->angles.x != 0 || prop->angles.y != 0 || prop->angles.z != 0) {
        propOrient(prop);
    }
}

/*
 * Turns the prop's orientation matrix by its angles and hands the resulting Euler angles to the world
 * object. When the matrix is (nearly) pitched straight up the roll is dropped and the yaw is taken from
 * the other axes.
 */
BMX_ADDR(0x0042f930)
void propOrient(PropRecord *prop)
{
    ZMatrix m;
    ZAngle3 angles;
    ZAngle3 turn;
    double h;
    float hf;

    zmatrixIdentity(&m);
    turn = prop->angles;
    zmatrixRotateEuler(&m, &m, &turn);
    zmatrixMul(&prop->orient, &prop->orient, &m);
    zmatrixCopyRotation(&m, &prop->orient);
    h = __builtin_sqrt((double)m.right.y * m.right.y + (double)m.right.x * m.right.x);
    hf = (float)h;
    if (h > kPropFlatLimit) {
        angles.x = zmathAtan2(m.up.z, m.at.z);
        angles.y = zmathAtan2(-m.right.z, hf);
        angles.z = zmathAtan2(m.right.y, m.right.x);
    } else {
        angles.x = zmathAtan2(-m.at.y, m.up.y);
        angles.y = zmathAtan2(-m.right.z, hf);
        angles.z = 0;
    }
    zbspPropSetRotation(prop->object, &angles);
}

BMX_ADDR(0x0042fab0)
void propNop(void) {}

/*
 * Movement of the props that float up (kinds 8 and 9: up to a limit) and that slide in (kind 0x10, in two
 * phases driven by prop->mode: turn, then move by the orientation's first row scaled up).
 */
BMX_ADDR(0x0042fad0)
void propUpdateMove(PropRecord *prop)
{
    const PropDef *const def = propDef(prop);
    uint8_t kind;

    if (def->type != 0) {
        return;
    }
    kind = def->kind;
    if (kind < 8) {
        return;
    }
    if (kind <= 9) {
        if (!((double)prop->pos.z >= kPropFloatLimit)) {
            prop->pos.z = (float)((double)prop->pos.z + kPropFloatStep);
        }
        return;
    }
    if (kind != 0x10 || prop->timer != 0) {
        return;
    }
    zvec3Zero(&prop->vel);
    prop->angles.x = 0;
    prop->angles.y = 0;
    prop->angles.z = 0;
    if (prop->mode == 1) {
        prop->timer = 0x5a;
        prop->angles.y = (ZAngle)0xff55;
        prop->mode = 2;
        return;
    }
    if (prop->mode == 2) {
        const float x = (float)((double)prop->orient.right.x * kPropSlideScale);

        prop->timer = 0x10e;
        prop->mode = 3;
        prop->vel.x = x;
        prop->vel.y = (float)((double)prop->orient.right.y * kPropSlideScale);
        prop->vel.z = (float)((double)prop->orient.right.z * kPropSlideScale);
    }
}

/*
 * Shows (shown != 0) or hides the node with the prop's tag id and the node of the id + 1: the first is
 * visible while the prop is not shown, the second while it is. Stores the mode.
 */
BMX_ADDR(0x0042fc00)
void propSetShown(PropRecord *prop, int shown)
{
    const PropDef *const def = propDef(prop);

    for (PhysPropNode *node = *zobjPropList(); node != NULL; node = node->next) {
        if (node->tag == NULL) {
            continue;
        }
        const uint16_t range = node->tag->range;
        if (range == def->id) {
            zobjSetVisible((int32_t)(uintptr_t)node->frame, shown == 0);
        } else if ((uint32_t)range == (uint32_t)def->id + 1) {
            zobjSetVisible((int32_t)(uintptr_t)node->frame, shown != 0);
        }
    }
    prop->mode = (uint32_t)shown;
}
