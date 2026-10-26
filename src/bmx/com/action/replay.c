/* Replays (C:\zaxis\Bmx\Src\com\action\replay.c). */
#include "bmx/com/action/replay.h"

#include "bmx/com/action/action.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/prop.h"
#include "bmx/com/zge/zactor.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char replayFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\replay.c";
static const char kBadHandle[] = "invalid replay handle";
static const char kDestroyed[] = "invalid handle passed or replay has been destroyed";

enum {
    kReplayMax = 5,
    kReplayDefaultSize = 0x2000,
    kReplayHeader = 0x40,
    kReplayNoPartA = 0x5c10, /* header + part A (two buffers, the first one 0x2de8 long) */
    kReplayPartB = 0x1680,   /* copy of the 0xb40 byte table and its working space */
    kReplayPartC = 0x4e,     /* goal info copy (0x27 bytes) and more */
};

/* Sticks are stored as signed bytes: float * 127 when recording, byte / 127 (as 0x3c010204) when playing. */
static const float kReplayStickScale = 127.0f;
static const float kReplayStickUnit = 0x1.020408p-7f;

/* Level goal table copied into recorders that keep part B. */
#define gReplayTable ORIG_VAR(0x00694460, uint8_t, gReplayTable)

BMX_ADDR(0x0042fce0)
void replayInit(void)
{
    for (int i = 0; i < kReplayMax; i++) {
        gReplays[i] = NULL;
    }
}

BMX_ADDR(0x0042fd10)
void replayDestroyAll(void)
{
    for (int i = 0; i < kReplayMax; i++) {
        if (gReplays[i] != NULL) {
            replayDestroy((uint32_t)i);
        }
    }
}

/* Address of the last byte a record (3 bytes) can still start at: the end of the frame buffer minus 3. */
static uintptr_t bufferLimit(const Replay *r)
{
    return (uintptr_t)r + r->headerSize + r->size - 3;
}

/* A recorder that ran out of buffer reports it and stops (mode 0). */
static void bufferFull(uint32_t handle, const Replay *r)
{
    gZdebugPrint("bmxreplayUpdate: reached end of replay buffer (replay %d, %d seconds)\n", handle,
                 r->field10 / 30);
    replaySetMode(handle, 0);
}

/* Appends a 3 byte record: header byte (kind in bits 5..7, frame counter below), then the changed value. */
static void putRecord(uint32_t handle, Replay *r, uint8_t head, uint8_t lo, uint8_t hi)
{
    if ((uintptr_t)r->cursor < bufferLimit(r)) {
        *r->cursor = head;
        r->cursor++;
        *r->cursor = lo;
        r->cursor[1] = hi;
        r->counter = 0;
        r->cursor += 2;
    } else {
        bufferFull(handle, r);
    }
}

static void recordFrame(uint32_t handle, Replay *r)
{
    const ContrlerPad *pad;

    r->counter++;
    r->field08++;
    r->field10++;
    /* The counters are advanced before the lookup: a bad controller index asserts (and aborts) inside it. */
    pad = (const ContrlerPad *)contrlerGet(((const OvlProfile *)gProfiles)[r->rider].controller);
    if ((pad->flags & 1) != 0) {
        return;
    }
    if (pad->state.buttons != r->buttons || r->counter == 0x1f) {
        putRecord(handle, r, r->counter, (uint8_t)(r->buttons ^ pad->state.buttons),
                  (uint8_t)((r->buttons >> 8) ^ (pad->state.buttons >> 8)));
        r->buttons = pad->state.buttons;
    }
    for (uint32_t i = 0; i < 2; i++) {
        const uint8_t hi = (uint8_t)bmxFtol((double)pad->state.axis[2 * i] * (double)kReplayStickScale);
        const uint8_t lo = (uint8_t)bmxFtol((double)pad->state.axis[2 * i + 1] * (double)kReplayStickScale);
        const uint16_t word = (uint16_t)(hi << 8 | lo);

        if (word != r->sticks[i]) {
            putRecord(handle, r, (uint8_t)(((i + 1) << 5) | r->counter), lo, hi);
            r->sticks[i] = word;
        }
    }
}

/* The recorded sticks (signed bytes) as the floats the rider reads. */
static void stickFloats(float *out, const Replay *r)
{
    const int8_t *const b = (const int8_t *)r->sticks;

    for (int i = 0; i < 2; i++) {
        out[2 * i] = (float)((double)b[2 * i + 1] * (double)kReplayStickUnit);
        out[2 * i + 1] = (float)((double)b[2 * i] * (double)kReplayStickUnit);
    }
}

static void playFrame(uint32_t handle, Replay *r)
{
    PhysRider *const rider = &gPhysRiders[r->rider];
    uint32_t buttons2 = 0;

    if (r->field10 == 0) {
        return;
    }
    stickFloats(rider->axis, r);
    r->counter--;
    while (r->counter == 0 && (uintptr_t)r->cursor < bufferLimit(r)) {
        const uint8_t op = *r->cursor;
        const uint8_t *const data = ++r->cursor;
        const uint32_t kind = op >> 5;

        if (kind == 0) {
            buttons2 = (uint32_t)(data[1] << 8) + data[0];
            r->cursor += 2;
        } else if (kind < 3) {
            r->sticks[kind - 1] = (uint16_t)((data[1] << 8) + data[0]);
            r->cursor += 2;
        }
        r->counter = *r->cursor & 0x1f;
    }
    rider->inputBits2 = buttons2;
    r->buttons ^= buttons2;
    rider->inputBits = r->buttons;
    stickFloats(rider->stick, r);
    r->field08++;
    if (r->field08 == r->field10) {
        replaySetMode(handle, 4);
    }
}

BMX_ADDR(0x0042fd50)
void replayUpdate(void)
{
    for (uint32_t handle = 0; handle < kReplayMax; handle++) {
        Replay *const r = gReplays[handle];

        if (r == NULL) {
            continue;
        }
        if (r->state == 1) {
            recordFrame(handle, r);
        } else if (r->state == 2) {
            playFrame(handle, r);
        }
    }
}

/*
 * Reserves a recorder in the static region: header, optional parts (those whose flag bit is clear) and
 * the frame buffer. Returns the slot, or 0xff when all five are taken. A failed allocation asserts and
 * then tries the next slot.
 */
BMX_ADDR(0x00430180)
uint8_t replayCreate(const ReplayParams *params)
{
    uint8_t handle = 0;

    do {
        if (gReplays[handle] == NULL) {
            uint32_t *const mark = zmemStaticMark();
            int total = (params->flags & 1) != 0 ? kReplayHeader : kReplayNoPartA;
            uint32_t size;
            Replay *r;

            if ((params->flags & 2) == 0) {
                total += kReplayPartB;
            }
            if ((params->flags & 4) == 0) {
                total += kReplayPartC;
            }
            size = params->size == 0 ? kReplayDefaultSize : ((uint32_t)params->size + 3) & 0xfffffffc;
            r = zmemStaticAlloc((uint32_t)total + size);
            if (r != NULL) {
                uint32_t at;

                r->mark = mark;
                r->flags = params->flags;
                r->rider = params->index;
                r->state = 0;
                r->field10 = 0;
                r->size = size;
                r->headerSize = kReplayHeader;
                r->partA = 0;
                r->partAEnd = 0;
                r->partB = 0;
                r->partBEnd = 0;
                at = size + kReplayHeader;
                if ((params->flags & 1) == 0) {
                    r->partA = at;
                    r->partAEnd = at + 0x2de8;
                    at += 0x5bd0;
                }
                if ((params->flags & 2) == 0) {
                    r->partB = at;
                    r->partBEnd = at + 0xb40;
                    at += 0x1680;
                    zmemCopy((uint8_t *)r + r->partB, &gReplayTable, 0xb40);
                }
                if ((params->flags & 4) == 0) {
                    r->partC = at;
                    at += 0x27;
                    r->partCEnd = at;
                    if (gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel < 6) {
                        zmemCopy((uint8_t *)r + r->partC, comdataGetGoalInfo(r->rider, gCurrentLevel), 0x27);
                    }
                }
                gReplays[handle] = r;
                return handle;
            }
            zdebugFail("bmxreplayCreate: failed to allocate buffer\n", replayFile, 0x14e, 0);
        }
        handle++;
    } while (handle < kReplayMax);
    return 0xff;
}

/* The recorder of a handle; asserts that the handle is below 5 and in use. */
static Replay *replayLookup(uint32_t handle, int lineRange, int lineUsed)
{
    const uint8_t index = (uint8_t)handle;

    if (index >= kReplayMax) {
        zdebugFail(kBadHandle, replayFile, lineRange, 0);
    }
    Replay *const r = gReplays[index];
    if (r == NULL) {
        zdebugFail(kDestroyed, replayFile, lineUsed, 0);
    }
    return r;
}

BMX_ADDR(0x00430380)
void replayDestroy(uint32_t handle)
{
    const uint8_t index = (uint8_t)handle;
    const Replay *const r = replayLookup(handle, 0x15f, 0x161);

    zmemStaticRelease(r->mark);
    gReplays[index] = NULL;
}

/*
 * Snapshot of a rider in part A (0x2de8 bytes): the rider record, its actor (and the block at +0x48 of
 * it), its rig, the bike actor's position and height, and the pad's buttons.
 */
enum {
    kSnapActor = 0xf70,
    kSnapActorBlock = 0x11e8,
    kSnapRig = 0x1f48,
    kSnapBikePos = 0x2dd0,
    kSnapBikeHeight = 0x2de0,
    kSnapButtons = 0x2de4,
};

static void *actorBlock(const PhysRider *r)
{
    return *(void **)((uint8_t *)r->actor + 0x48);
}

static void snapshotSave(uint8_t *snap, PhysRider *r, const ContrlerPad *pad)
{
    zmemCopy(snap, r, sizeof *r);
    zmemCopy(snap + kSnapActor, r->actor, 0x278);
    zmemCopy(snap + kSnapActorBlock, actorBlock(r), 0xd60);
    zmemCopy(snap + kSnapRig, r->actor2, 0xe88);
    ((uint32_t *)(snap + kSnapBikePos))[0] = ((const uint32_t *)zactorGetPos(r->bikeObj))[0];
    ((uint32_t *)(snap + kSnapBikePos))[1] = ((const uint32_t *)zactorGetPos(r->bikeObj))[1];
    ((uint32_t *)(snap + kSnapBikePos))[2] = ((const uint32_t *)zactorGetPos(r->bikeObj))[2];
    *(float *)(snap + kSnapBikeHeight) = zactorGetHeight(r->bikeObj);
    *(uint32_t *)(snap + kSnapButtons) = pad->state.buttons;
}

/* Puts a snapshot back (the rider record first: the actor pointers are the restored ones). */
static void snapshotRestore(PhysRider *r, const uint8_t *snap, int rig)
{
    zmemCopy(r, snap, sizeof *r);
    zmemCopy(r->actor, snap + kSnapActor, 0x278);
    zmemCopy(actorBlock(r), snap + kSnapActorBlock, 0xd60);
    if (rig != 0) {
        zmemCopy(r->actor2, snap + kSnapRig, 0xe88);
    }
    zactorFaceMatrix(r->bikeObj, &r->basis);
    zactorSetPos(r->bikeObj, (const ZVec3 *)(snap + kSnapBikePos));
    zactorSetHeight(r->bikeObj, *(const float *)(snap + kSnapBikeHeight));
}

/* Whether the goal info is part of the replay: part C kept and the game mode has one (mode 0, sub 0, level <
 * 6). */
static int keepsGoalInfo(const Replay *r)
{
    return (r->flags & 4) == 0 && gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel < 6;
}

/*
 * Mode switch of a recorder. Leaving a recording closes it with a final counter record; leaving a playback
 * for mode 0 / 1 restores the live rider (without its rig), props and goal info saved when playback began.
 */
BMX_ADDR(0x00430410)
uint8_t replaySetMode(uint32_t handle, uint32_t mode)
{
    Replay *const rp = replayLookup(handle, 0x175, 0x177);
    PhysRider *const r = &gPhysRiders[rp->rider];
    const ContrlerPad *const pad =
        (const ContrlerPad *)contrlerGet(((const OvlProfile *)gProfiles)[rp->rider].controller);
    const uint32_t old = rp->state;

    if (mode == old) {
        return rp->state;
    }
    if (old == 1) {
        if ((uintptr_t)rp->cursor < bufferLimit(rp)) {
            *rp->cursor = (uint8_t)(rp->counter + 1);
            rp->cursor++;
            *rp->cursor = 0;
            rp->cursor[1] = 0;
            rp->counter = 0;
            rp->cursor += 2;
        }
    } else if ((old == 2 || old == 4) && mode <= 1) {
        if ((rp->flags & 1) == 0) {
            snapshotRestore(r, (uint8_t *)rp + rp->partAEnd, 0);
        }
        if ((rp->flags & 2) == 0) {
            propLoadState((uint8_t *)rp + rp->partBEnd);
        }
        if (keepsGoalInfo(rp)) {
            zmemCopy(comdataGetGoalInfo(rp->rider, gCurrentLevel), (uint8_t *)rp + rp->partCEnd, 0x27);
        }
    }
    if (mode == 1) {
        rp->padIndex = r->padIndex;
        if ((rp->flags & 2) == 0) {
            zmemCopy((uint8_t *)rp + rp->partB, &gPropState, 0xb40);
        }
        if (keepsGoalInfo(rp)) {
            zmemCopy((uint8_t *)rp + rp->partC, comdataGetGoalInfo(rp->rider, gCurrentLevel), 0x27);
        }
        if ((rp->flags & 1) == 0) {
            snapshotSave((uint8_t *)rp + rp->partA, r, pad);
            rp->buttons = pad->state.buttons;
        } else {
            rp->buttons = 0;
        }
        rp->sticks[0] = 0;
        rp->sticks[1] = 0;
        rp->field08 = 0;
        rp->cursor = (uint8_t *)rp + rp->headerSize;
        rp->field10 = 0;
        rp->counter = 0;
    } else if (mode == 2) {
        if (rp->field10 == 0) {
            /* nothing recorded: the recorder switches off */
            rp->state = 0;
            return 0;
        }
        r->padIndex = (uint8_t)rp->padIndex;
        if (rp->state <= 1) {
            if ((rp->flags & 2) == 0) {
                zmemCopy((uint8_t *)rp + rp->partBEnd, &gPropState, 0xb40);
            }
            if (keepsGoalInfo(rp)) {
                zmemCopy((uint8_t *)rp + rp->partCEnd, comdataGetGoalInfo(rp->rider, gCurrentLevel), 0x27);
            }
        }
        if ((rp->flags & 2) == 0) {
            propLoadState((uint8_t *)rp + rp->partB);
        }
        if (keepsGoalInfo(rp)) {
            zmemCopy(comdataGetGoalInfo(rp->rider, gCurrentLevel), (uint8_t *)rp + rp->partC, 0x27);
        }
        if ((rp->flags & 1) == 0) {
            const uint8_t *const start = (const uint8_t *)rp + rp->partA;

            if (rp->state <= 1) {
                snapshotSave((uint8_t *)rp + rp->partAEnd, r, pad);
            }
            snapshotRestore(r, start, 1);
            rp->buttons = *(const uint32_t *)(start + kSnapButtons);
        } else {
            gameResetRider(rp->rider, 1);
            rp->buttons = 0;
        }
        rp->sticks[0] = 0;
        rp->sticks[1] = 0;
        rp->field08 = 0;
        rp->cursor = (uint8_t *)rp + rp->headerSize;
        rp->state = (uint8_t)mode;
        rp->counter = *rp->cursor & 0x1f;
        return (uint8_t)mode;
    }
    rp->state = (uint8_t)mode;
    return rp->state;
}

BMX_ADDR(0x00430b70)
uint32_t replayGetState(uint32_t handle)
{
    return replayLookup(handle, 0x23e, 0x240)->state;
}

BMX_ADDR(0x00430bf0)
uint32_t replaySetRider(uint32_t handle, uint32_t rider)
{
    replayLookup(handle, 0x24e, 0x250)->rider = (uint8_t)rider;
    return (uint8_t)rider;
}

BMX_ADDR(0x00430c70)
uint32_t replayGetRider(uint32_t handle)
{
    return replayLookup(handle, 0x25f, 0x261)->rider;
}

BMX_ADDR(0x00430cf0)
uint32_t replayGetField08(uint32_t handle)
{
    return replayLookup(handle, 0x26f, 0x271)->field08;
}

/* Start and size of the serialisable part of a recorder (everything from offset 0xc). */
BMX_ADDR(0x00430d70)
void replayGetBuffer(uint32_t source, uint8_t **data, int *size)
{
    const Replay *const r = replayLookup(source, 0x282, 0x284);

    *data = (uint8_t *)&r->padIndex;
    *size = (int)r->size + 0x34;
    if ((r->flags & 1) == 0) {
        *size += 0x5bd0;
    }
    if ((r->flags & 2) == 0) {
        *size += 0x1680;
    }
    if ((r->flags & 4) == 0) {
        *size += 0x4e;
    }
}
