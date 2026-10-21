/* Replays (C:\zaxis\Bmx\Src\com\action\replay.c). */
#include "bmx/com/action/replay.h"

#include "bmx/com/action/comdata.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
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

    *data = (uint8_t *)r->pad0c;
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
