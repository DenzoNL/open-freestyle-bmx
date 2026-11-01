/* Level props (C:\zaxis\Bmx\Src\com\action\prop.c). */
#include "bmx/com/action/prop.h"

#include "bmx/com/action/contrler.h"
#include "bmx/com/action/fx.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/rider.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "compat/crt.h"
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

static const char kPropFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\prop.c";

/*
 * Sets up the props of the current level: each definition is matched to a collision object of the level
 * (props, types 0..3 and above 5) or to a face node (types 4 and 5), the record gets its position and
 * orientation, and the looping sounds start. The tagged nodes get their visibility state, the collectables
 * of level 0 that start out collected are switched, and the result is saved as the defaults.
 */
BMX_ADDR(0x0042deb0)
void propInit(void)
{
    uint32_t *slot;
    uint32_t nodeCount = 0;

    for (uint32_t i = 0; i < kPropMax; i++) {
        PropRecord *const rec = &gPropRecords[i];
        const PropDef *const def = &gPropDefs[0][0] + (uint32_t)gCurrentLevel * 16 + i;

        rec->state = 0;
        rec->object = NULL;
        if (def->id == 0) {
            continue;
        }
        if (def->type < kPropTypeFixed || def->type > kPropTypeToggle) {
            PhysPropObject **const hits = (PhysPropObject **)zbspFindPropsNear(def->id, def->id);

            if (hits == NULL) {
                gZdebugPrint("bmxpropInit: error finding prop with tag %d\n", def->id);
                continue;
            }
            {
                PhysPropObject *const object = hits[0];

                rec->index = (uint8_t)i;
                rec->object = object;
                rec->pos.x = (float)(int)object->pos[0];
                rec->pos.y = (float)(int)object->pos[1];
                rec->pos.z = (float)(int)object->pos[2];
                zvec3Zero(&rec->vel);
                zmatrixIdentity(&rec->orient);
                zmatrixRotateEuler(&rec->orient, &rec->orient, &object->rot);
                rec->angles.x = 0;
                rec->angles.y = 0;
                rec->angles.z = 0;
                rec->flags = 0;
                rec->timer = 0;
                rec->state = kPropStateRecord;
                rec->mode = 0;
                rec->paramA = def->paramA;
                rec->paramB = def->paramB;
                object->flags |= 0x400;
                object->prop = (const PhysPropInfo *)(const void *)rec;
                rec->sound = 0xffffffffu;
                if (def->soundStop != 0xff) {
                    rec->sound = (uint32_t)bmxSoundPlayAt(kSoundProp, &rec->pos, 0, def->soundStop, 0x7f);
                }
                for (PhysPropObject **extra = hits + 1; *extra != NULL; extra++) {
                    gZdebugPrint("bmxpropInit: error: extra prop with tag %d (X: %d, Z: %d)\n", def->id,
                                 (int)(*extra)->pos[0], (int)(*extra)->pos[2]);
                }
            }
        } else {
            const PhysPropNode *const node = propFindNode(def->id);

            if (node == NULL) {
                gZdebugPrint("bmxpropInit: error finding face with tag %d\n", def->id);
                continue;
            }
            rec->index = (uint8_t)i;
            rec->object = node->object;
            rec->pos.x = node->frame->pos.x;
            rec->pos.y = node->frame->pos.y;
            rec->pos.z = node->frame->pos.z;
            rec->state = kPropStateRecord;
            rec->sound = 0xffffffffu;
            if (def->soundStop != 0xff) {
                rec->sound = (uint32_t)bmxSoundPlayAt(kSoundProp, &rec->pos, 0, def->soundStop, 0x7f);
            }
        }
    }
    zmemFill(&gPropState.pad840, 0, 0x300);
    /* The slot is the node field of a state record; the original also clears the word 8 bytes before it. */
    slot = (uint32_t *)(uintptr_t)&gPropNodeStates[0].node;
    for (PhysPropNode *node = *zobjPropList(); node != NULL; node = node->next) {
        if (node->tag != NULL && node->tag->range != 0) {
            if (nodeCount >= 64) {
                zdebugFail("state info overflow", kPropFile, 0x1eb, 0);
            }
            slot[-2] = 0;
            slot[0] = (uint32_t)(uintptr_t)node;
            slot += 3;
            nodeCount++;
        }
    }
    if (gCurrentLevel == 0) {
        for (uint32_t i = 0; i < kPropMax; i++) {
            PropRecord *const rec = &gPropRecords[i];

            if (rec->state != 0) {
                const uint32_t id = (&gPropDefs[0][0] + (uint32_t)gCurrentLevel * 16 + i)->id;

                if (id >= 0x14 && id <= 0x19) {
                    propSwitch(rec, 0, 0);
                    rec->state = kPropStateCollected;
                }
            }
        }
    }
    zmemCopy(&gPropDefaults, &gPropState, kPropStateSize);
    zrollerAddDrawCallback(BMX_FNPTR(ZrollerDrawFn, 0x00402527, propReturnZero));
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

/* Kinds of the type 0 props that a rider bumps into (float off when hit). */
static int propIsBumper(const PropDef *def)
{
    return def->kind >= 8 && def->kind <= 9;
}

/*
 * A moving type 0 prop against the riders: a rider whose position (plus step) lies inside the prop's
 * box (in the prop's frame, the extents of its collision shape, up to 120 units above) is thrown off by a
 * bumper prop (state 2, launch velocity, score, sounds once per cooldown); outside it, a bumper rumbles.
 * A rider grinding / riding on the prop is carried along by its velocity.
 */
static void propPushRiders(PropRecord *rec, const PropDef *def)
{
    /* the original passes the 16 bit index with whatever the upper half of EAX holds */
    const float *const shape = (const float *)(const void *)ngifzCollideGetObject(rec->object->index) + 1;
    const float extX = rec->object->scale[0] * shape[0];
    const float extY = rec->object->scale[1] * shape[1];
    const float extZ = rec->object->scale[2] * shape[2];

    for (uint32_t p = 0; p < (gPhysPlayerCount < 1 ? 1u : gPhysPlayerCount); p++) {
        PhysRider *const r = &gPhysRiders[p];
        ZVec3 d;

        if ((r->flags & 1) == 0) {
            continue;
        }
        d.x = (float)(((double)r->pos.x + r->step.x) - rec->pos.x);
        d.y = (float)(((double)r->pos.y + r->step.y) - rec->pos.y);
        d.z = (float)(((double)r->pos.z + r->step.z) - rec->pos.z);
        {
            const uint32_t ix = (uint32_t)bmxFtol(d.x);
            const uint32_t iy = (uint32_t)bmxFtol(d.y);
            const uint32_t iz = (uint32_t)bmxFtol(d.z);

            if ((int32_t)(iz * iz + iy * iy + ix * ix) >= 0x15f900) {
                continue;
            }
        }
        {
            ZVec3 local;

            zmatrixTransformPoint(&local, &rec->orient, &d);
            d = local;
        }
        if (__builtin_fabsf(d.x) > extX || __builtin_fabsf(d.z) > extZ || d.y > 120.0f ||
            -((double)extY + 120.0f) > d.y) {
            if (propIsBumper(def)) {
                contrlerRumble(r->player, 1, 0x40);
            }
        } else if (propIsBumper(def)) {
            const float speed = (float)zvec3Length(&rec->vel);
            ZRig *const rig = r->actor2;
            ZVec3 v;

            physSetState(r, 2);
            v.x = (float)((double)rec->vel.x * 0x1.0ccccc0p+0f);
            v.y = (float)((double)rec->vel.y * 0x1.0ccccc0p+0f - 15.0f);
            v.z = (float)((double)rec->vel.z * 0x1.0ccccc0p+0f);
            r->bailReason = 3;
            zvec3RotateY(&v, &v, 0);
            rig->points[0].vel = v;
            rig->points[1].vel = v;
            rig->points[2].vel = v;
            rig->points[13].vel = v;
            riderAddScore(r, 0, speed);
            riderAddScore(r, 13, speed);
            if (r->soundCooldown == 0) {
                bmxSoundPlayAt(0xbbc, &r->pos, 7, 0x7f, 0);
                bmxSoundPlayAt(0xbbd, &r->pos, 1, 3, 0);
                bmxSoundPlayAt(0xbbe, &r->pos, 1, 3, 0);
                contrlerRumble(r->player, 1, 0xff);
                fxBurst();
                r->soundCooldown = 0x3c;
            }
        }
        if ((r->surface == 0xc && r->contact == rec->object) ||
            (r->surface == 9 && ((const PhysEdge *)r->contact)->prop == rec->object)) {
            const uint8_t state = physGetState(r);

            if (state == 4 || state == 6) {
                zvec3Add(&r->pos, &r->pos, &rec->vel);
                zvec3Add(&r->center, &r->center, &rec->vel);
            }
        }
    }
}

/*
 * Per frame update of the 16 props: counts down the timers of the props switching (states 2 / 4 start
 * the switch when they run out, 3 / 5 move until they run out, then come to rest), moves them, pushes the
 * riders (type 0) and keeps the looping sounds at the props. Note the definition is taken by the record's
 * slot, not by its index.
 */
BMX_ADDR(0x0042e2f0)
void propUpdate(void)
{
    for (uint32_t i = 0; i < kPropMax; i++) {
        PropRecord *const rec = &gPropRecords[i];
        const PropDef *const def = &gPropDefs[0][0] + (uint32_t)gCurrentLevel * 16 + i;
        uint32_t t;
        int moving;

        if (rec->object == NULL) {
            continue;
        }
        switch (def->type) {
        case kPropTypeCollect:
            switch (rec->state) {
            case 1:
                propNop();
                break;
            case 2:
            case 4:
                if (--rec->timer == 0) {
                    propSwitch(rec, rec->player, 0);
                    rec->state++;
                }
                break;
            case 3:
            case 5:
                /* a timer of -1 moves for ever; otherwise it counts down and the prop stops after 0 */
                t = rec->timer;
                moving = t == 0xffffffffu;
                if (!moving) {
                    rec->timer = t - 1;
                    moving = t != 0;
                }
                if (moving) {
                    zvec3Add(&rec->pos, &rec->pos, &rec->vel);
                    zbspPropSetPosition(rec->object, &rec->pos);
                    if (def->kind >= 11 && def->kind <= 12) {
                        const int16_t yaw = rec->angles.y;

                        zvec3Scale(&rec->vel, &rec->vel, 0x1.e147aep-1f);
                        rec->angles.y = (ZAngle)(yaw - bmxFtol((double)yaw * 0x1.68p-8f * 0x1.eb851ep-5f *
                                                               0x1.6c16c2p+7f));
                    }
                    propApplyTurn(rec);
                    propPushRiders(rec, def);
                    propUpdateMove(rec);
                    if (def->soundStop != 0xff) {
                        bmxSoundPlayAt(kSoundProp, &rec->pos, 2, (int)rec->sound, 0x7f);
                    }
                } else {
                    rec->state = kPropStateRecord;
                    if ((rec->flags & 1) != 0 && rec->mode != 0) {
                        zbspObjectSignal(rec->object, 4);
                        zbspObjectSignal(rec->object, 2);
                        rec->flags |= 2;
                    }
                    if (def->kind >= 13 && def->kind <= 20) {
                        const uint32_t *linked = &rec->paramA;

                        for (int k = 0; k < 2; k++) {
                            if (linked[k] != 0) {
                                uint8_t *const other = propFind(linked[k]);

                                if (other != NULL) {
                                    propCollect(rec->player, other);
                                }
                            }
                        }
                    }
                }
                break;
            default:
                break;
            }
            break;
        case 1:
        case 2:
        case kPropTypeBoard:
            switch (rec->state) {
            case 2:
            case 4:
                if (--rec->timer == 0) {
                    propSwitch(rec, rec->player, 0);
                    rec->state++;
                    gameCheckPropGoal(rec->player, rec);
                }
                break;
            case 3:
            case 5:
                if (--rec->timer == 0) {
                    if (def->type == 2 && rec->state == 3) {
                        /* a type 2 prop waits paramA seconds before it switches back */
                        rec->state = 4;
                        t = rec->paramA * 30;
                        rec->timer = t < 1 ? 1 : t;
                    } else {
                        rec->state = kPropStateRecord;
                    }
                } else {
                    if (def->kind == 1) {
                        zvec3Add(&rec->pos, &rec->pos, &rec->vel);
                        zbspPropSetPosition(rec->object, &rec->pos);
                    }
                    propApplyTurn(rec);
                    if (def->soundStop != 0xff) {
                        bmxSoundPlayAt(kSoundProp, &rec->pos, 2, (int)rec->sound, 0x7f);
                    }
                }
                break;
            default:
                break;
            }
            break;
        case kPropTypeFixed:
        case kPropTypeToggle:
            if (def->soundStop != 0xff) {
                bmxSoundPlayAt(kSoundProp, &rec->pos, 2, (int)rec->sound, 0x7f);
            }
            break;
        default:
            break;
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

        fxShards(&pos, &gPhysRiders[player].vel);
        bmxSoundPlayAt(kSoundBreak, &pos, 3, 3, 0x7f);
    }
    gameCheckGrindGoal(player, (const PhysEdge *)(const void *)face);
    gPropNodeStates[i].flags |= 1;
}

/*
 * Starts a prop moving to its other position (release == 0: collected by `player`) or puts it there at
 * once (release != 0). The prop's mode toggles between its two positions; how it moves depends on the
 * definition's type and kind: drop / rise along y, turn by a quarter, fly off along the rider's velocity
 * (kinds 11, 12), slide along the orientation's first row (13..16). Afterwards the world object is
 * signalled solid (2) or not (3), and a release snaps the object in place and releases the linked props.
 */
BMX_ADDR(0x0042f0d0)
void propSwitch(PropRecord *prop, uint32_t player, int release)
{
    const PropDef *const def = propDef(prop);
    const PhysRider *const rider =
        (const PhysRider *)((const uint8_t *)&gPhysRiders[0] + player * (uint32_t)sizeof(PhysRider));
    int solid = 0;
    uint32_t mode;

    if (def->type != 0) {
        if (def->type <= kPropTypeBoard) {
            switch (def->kind) {
            case 1:
                mode = prop->mode;
                if (release == 0) {
                    prop->timer = 0x3c;
                    prop->vel.y = mode != 0 ? -0x1.d55556p+0f : 0x1.d55556p+0f;
                } else {
                    prop->pos.y = (float)((double)(mode != 0 ? -110.0f : 110.0f) + prop->pos.y);
                }
                prop->mode = mode ^ 1;
                break;
            case 2:
                mode = prop->mode;
                if (release == 0) {
                    prop->timer = 0x3c;
                    prop->angles.x = (ZAngle)((mode != 0 ? -0x4000 : 0x4000) / 60);
                } else {
                    prop->angles.x = (ZAngle)(mode != 0 ? -0x4000 : 0x4000);
                }
                prop->mode = mode ^ 1;
                break;
            case 3:
                if (release == 0) {
                    prop->angles.y = 0x5b;
                    prop->timer = 0x3c;
                } else {
                    prop->angles.y = 0x1555;
                }
                prop->mode++;
                break;
            default:
                break;
            }
            if (def->type == kPropTypeBoard) {
                solid = 1;
            }
        }
    } else {
        const uint8_t kind = def->kind;

        switch (kind) {
        case 1:
            zvec3Zero(&prop->vel);
            prop->flags |= 1;
            prop->mode ^= 1;
            prop->timer = 1;
            break;
        case 2:
            mode = prop->mode;
            if (release == 0) {
                prop->timer = 0xf0;
                prop->vel.y = mode != 0 ? 0x1.aaaaaap+0f : -0x1.aaaaaap+0f;
            } else {
                prop->pos.y = (float)((double)(mode != 0 ? 400.0f : -400.0f) + prop->pos.y);
            }
            prop->mode = mode ^ 1;
            prop->flags |= 1;
            break;
        case 3:
        case 4:
        case 5:
            if (release == 0) {
                prop->timer = 0x1e;
                prop->angles.x = (ZAngle)((kind != 5 ? 0x4000 : -0x4000) / 30);
            } else {
                prop->angles.x = (ZAngle)(kind != 5 ? 0x4000 : -0x4000);
            }
            prop->mode ^= 1;
            if (def->kind == 3) {
                prop->flags |= 1;
            }
            break;
        case 6:
        case 7:
            /* the sign test compares with 4, which these kinds never are */
            if (release == 0) {
                prop->timer = 0xf0;
                prop->angles.z = (ZAngle)((kind != 4 ? -0x4000 : 0x4000) / 240);
            } else {
                prop->angles.z = (ZAngle)(kind != 4 ? -0x4000 : 0x4000);
            }
            prop->mode ^= 1;
            break;
        case 8:
        case 9:
            prop->vel.z = -40.0f;
            prop->timer = 0xffffffffu;
            break;
        case 10:
            mode = prop->mode;
            if (release == 0) {
                prop->timer = 0x78;
                prop->vel.y = mode != 0 ? 0x1.aaaaaap+0f : -0x1.aaaaaap+0f;
            } else {
                prop->pos.y = (float)((double)(mode != 0 ? 200.0f : -200.0f) + prop->pos.y);
            }
            prop->mode = mode ^ 1;
            break;
        case 11:
        case 12:
            if (release == 0) {
                /* flies off with the rider's velocity minus its part along the prop's up axis, 25% faster */
                const double d = zvec3Dot(&rider->vel, &prop->orient.up);

                prop->vel.x = (float)((double)rider->vel.x - d * prop->orient.up.x);
                prop->vel.y = (float)((double)rider->vel.y - d * prop->orient.up.y);
                prop->vel.z = (float)((double)rider->vel.z - d * prop->orient.up.z);
                zvec3Scale(&prop->vel, &prop->vel, 1.25f);
                if (def->kind == 11) {
                    prop->angles.y = 0x5b0;
                    prop->timer = 0x3c;
                } else {
                    prop->angles.x = (ZAngle)0xf99a;
                    prop->angles.y = 0x444;
                    prop->angles.z = 0x16c;
                    prop->timer = 0xf;
                }
            }
            prop->flags |= 1;
            prop->mode ^= 1;
            solid = 1;
            break;
        case 13:
        case 14: {
            double d = kind == 13 ? -1200.0f : -400.0f;

            if (prop->mode != 0) {
                d = -d;
            }
            if (release == 0) {
                d = d * 0x1.b4e81cp-8f;
                prop->timer = 0x96;
                prop->vel.x = (float)(d * prop->orient.right.x);
                prop->vel.y = (float)(d * prop->orient.right.y);
                prop->vel.z = (float)(d * prop->orient.right.z);
            } else {
                prop->pos.x = (float)(d * prop->orient.right.x + prop->pos.x);
                prop->pos.y = (float)(d * prop->orient.right.y + prop->pos.y);
                prop->pos.z = (float)(d * prop->orient.right.z + prop->pos.z);
            }
            prop->mode ^= 1;
            break;
        }
        case 15:
        case 16: {
            double d = kind == 16 ? -1100.0f : -3200.0f;

            prop->timer = kind != 16 ? 0x168 : 0x5a;
            mode = prop->mode;
            if (mode != 0) {
                d = -d;
            }
            if (release == 0) {
                d = d / (double)(int16_t)prop->timer;
                prop->vel.x = (float)(d * prop->orient.right.x);
                prop->vel.y = (float)(d * prop->orient.right.y);
                prop->vel.z = (float)(d * prop->orient.right.z);
                prop->mode = mode + 1;
            } else {
                prop->pos.x = (float)(d * prop->orient.right.x + prop->pos.x);
                prop->pos.y = (float)(d * prop->orient.right.y + prop->pos.y);
                prop->pos.z = (float)(d * prop->orient.right.z + prop->pos.z);
                if (def->kind == 0x10) {
                    /* the slide's turn and move phases (see propUpdateMove) applied at once */
                    prop->angles.y = 0x4000;
                    prop->mode = mode + 1;
                    propOrient(prop);
                    prop->angles.y = 0;
                    prop->mode++;
                    prop->pos.x = (float)((double)prop->orient.right.x * -2400.0f + prop->pos.x);
                    prop->pos.y = (float)((double)prop->orient.right.y * -2400.0f + prop->pos.y);
                    prop->pos.z = (float)((double)prop->orient.right.z * -2400.0f + prop->pos.z);
                }
                prop->mode++;
            }
            break;
        }
        case 17:
        case 18:
        case 19:
        case 20:
            prop->timer = 0x3c;
            prop->mode ^= 1;
            break;
        default:
            break;
        }
    }
    zbspObjectSignal(prop->object, 5);
    zbspObjectSignal(prop->object, solid != 0 ? 2 : 3);
    prop->flags &= (uint8_t)~2;
    if (release == 0) {
        if (def->sound != 0xff) {
            bmxSoundPlayAt(kSoundProp, &prop->pos, 3, def->sound, 0x7f);
        }
        return;
    }
    zbspPropSetPosition(prop->object, &prop->pos);
    propOrient(prop);
    if ((prop->flags & 1) != 0 && prop->mode != 0) {
        zbspObjectSignal(prop->object, 4);
        zbspObjectSignal(prop->object, 2);
        prop->flags |= 2;
    }
    prop->timer = 0;
    if (def->kind >= 13 && def->kind <= 20) {
        const uint32_t *linked = &prop->paramA;

        for (int i = 0; i < 2; i++) {
            if (linked[i] != 0) {
                uint8_t *const other = propFind(linked[i]);

                if (other != NULL) {
                    propRelease(prop->player, other);
                }
            }
        }
    }
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
