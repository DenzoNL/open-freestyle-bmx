/* Actor part lookup (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzactor.c). */
#include "bmx/com/zge/ngif/ngifzactor.h"

#include "bmx/com/action/comdata.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/zge/zactor.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zstring.h"

static const char kActorFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFzactor.c";

static const char *const kRiderPartNames[kZactorRiderParts] = {
    "Hips",       "Chest",      "Head",        "LeftUpArm",   "LeftLowArm",
    "LeftHand",   "RightUpArm", "RightLowArm", "RightHand",   "LeftUpLeg",
    "LeftLowLeg", "LeftFoot",   "RightUpLeg",  "RightLowLeg", "RightFoot",
};

static const char *const kBikePartNames[kZactorBikeParts] = {
    "Hipsdeck",  "Handlebars", "Sprocket", "RightPedal", "LeftPedal",
    "BackWheel", "FrontWheel", "LeftGrip", "RightGrip",  "SeatGrip",
};

static const uint32_t kRiderPartOrder[kZactorRiderParts] = {0, 1,   2,   6,   7, 8,   3,  4,
                                                            5, 0xc, 0xd, 0xe, 9, 0xa, 0xb};
static const uint32_t kBikePartOrder[kZactorBikeParts] = {0, 1, 2, 4, 3, 5, 6, 7, 8, 9};

static void *actorObject(const void *actor)
{
    return (void *)(uintptr_t)((const ZActor *)actor)->object;
}

/*
 * Row of the part cache for a player. The player index is not range checked (it is the current view
 * player byte); the tables hold two rows.
 */
static uint32_t *riderRow(int player)
{
    return (uint32_t *)((uint8_t *)&gZactorPose.riderNodes +
                        (size_t)player * sizeof gZactorPose.riderNodes[0]);
}

static uint32_t *bikeRow(int player)
{
    return (uint32_t *)((uint8_t *)&gZactorPose.bikeNodes + (size_t)player * sizeof gZactorPose.bikeNodes[0]);
}

static uint32_t *riderReady(int player)
{
    return (uint32_t *)((uint8_t *)&gZactorPose.riderReady + (size_t)player * 4);
}

static uint32_t *bikeReady(int player)
{
    return (uint32_t *)((uint8_t *)&gZactorPose.bikeReady + (size_t)player * 4);
}

/* The (lazily built) node row of the actor's kind for a player. */
static const uint32_t *partRow(void *actor, int player)
{
    if (((ZActor *)actor)->skeleton == 0) {
        if (*riderReady(player) == 0) {
            zactorInitRiderParts(actor, player);
        }
        return riderRow(player);
    }
    if (*bikeReady(player) == 0) {
        zactorInitBikeParts(actor, player);
    }
    return bikeRow(player);
}

/* Rider arms and legs (parts 3..8) are modelled turned by a quarter; the offset is rotated to match. */
static void rotateLocal(const ZActor *actor, int part, const ZVec3 *in, ZVec3 *v)
{
    const uint32_t p = (uint32_t)part & 0xffff;

    v->x = in->x;
    v->y = in->y;
    if (actor->skeleton == 0) {
        if (p >= 3 && p <= 5) {
            v->x = in->y;
            v->y = (float)-(
                       double)in->x; /* through the x87 like the original: a signalling NaN comes out quiet */
        } else if (p >= 6 && p <= 8) {
            v->y = in->x;
            v->x = (float)-(double)in->y;
        }
    }
    v->z = in->z;
}

static void printUnknownPart(int line, const char *what)
{
    gZdebugPrint("%s %d %s\n", kActorFile, line, what);
}

/* Shared body of the two position getters. */
static void partPosition(void *actor, int part, const ZVec3 *local, ZVec3 *out, int line, int player)
{
    ZVec3 v;
    uint32_t node;

    rotateLocal(actor, part, local, &v);
    node = partRow(actor, player)[(uint32_t)part & 0xffff];
    if (node == 0) {
        printUnknownPart(line, "testzactorGetAbsolutePartPosition: Unknown part");
        out->x = 0;
        out->y = 0;
        out->z = 0;
        return;
    }
    zrwmatNodeToWorld(actorObject(actor), node, &v.x, &out->x, 1);
}

BMX_ADDR(0x00464720)
uint32_t zactorViewPlayer(void)
{
    return comdataGetViewPlayer();
}

/* Looks up the 15 rider parts by name hash in the actor's model. */
BMX_ADDR(0x00464740)
void zactorInitRiderParts(void *actor, int player)
{
    uint32_t *row = riderRow(player);

    *riderReady(player) = 1;
    for (int i = 0; i < kZactorRiderParts; i++) {
        row[i] = zrwmatFindNode(actorObject(actor), (uint32_t)zstrHash(kRiderPartNames[i]));
    }
}

/* Looks up the 10 bike parts by name hash in the actor's model. */
BMX_ADDR(0x004649a0)
void zactorInitBikeParts(void *actor, int player)
{
    uint32_t *row = bikeRow(player);

    *bikeReady(player) = 1;
    for (int i = 0; i < kZactorBikeParts; i++) {
        row[i] = zrwmatFindNode(actorObject(actor), (uint32_t)zstrHash(kBikePartNames[i]));
    }
}

BMX_ADDR(0x00464b50)
void zactorGetPartPosition(void *actor, int part, const ZVec3 *local, ZVec3 *out)
{
    const int player = (int)zactorViewPlayer();

    zactorSyncPose(actor);
    zrwmatUpdate(actorObject(actor));
    partPosition(actor, part, local, out, 0xb5, player);
}

BMX_ADDR(0x00464d10)
void zactorGetPartPositionNoSync(void *actor, int part, const ZVec3 *local, ZVec3 *out)
{
    partPosition(actor, part, local, out, 0xf4, (int)zactorViewPlayer());
}

BMX_ADDR(0x00464ea0)
double zactorGetPartLength(void *actor, int part)
{
    const int player = (int)zactorViewPlayer();
    const uint32_t node = partRow(actor, player)[(uint32_t)part & 0xffff];
    ZVec3 v;

    if (node == 0) {
        printUnknownPart(0x11b, "testzactorGetPartLength: Unknown part");
        v.x = 0;
        v.y = 0;
        v.z = 0;
    } else {
        zrwmatNodeVector(actorObject(actor), node, &v.x, 1);
    }
    return zvec3Length(&v);
}

BMX_ADDR(0x00464fb0)
uint32_t zactorGetPartNode(void *actor, int part, int player)
{
    return partRow(actor, player)[(uint32_t)part & 0xffff];
}

BMX_ADDR(0x00465040)
void zactorGetPartVector(void *actor, int part, ZVec3 *out, int player)
{
    const uint32_t node = partRow(actor, player)[part];

    if (node != 0) {
        zrwmatNodeVector(actorObject(actor), node, &out->x, 1);
        return;
    }
    printUnknownPart(0x153, "testzactorGetPartLength: Unknown part");
    out->x = 0;
    out->y = 0;
    out->z = 0;
}

BMX_ADDR(0x004646f0)
void zactorClearPoseTables(void)
{
    for (int i = 0; i < 2; i++) {
        gZactorPose.bikeReady[i] = 0;
        gZactorPose.riderReady[i] = 0;
    }
}

BMX_ADDR(0x00465130)
void zactorSetupRider(void *actor)
{
    ZActor *a = actor;

    for (int i = 0; i < kZactorRiderParts; i++) {
        a->partHash[i] = (uint32_t)zstrHash(kRiderPartNames[i]);
    }
    for (int i = 0; i < kZactorRiderParts; i++) {
        a->partOrder[i] = kRiderPartOrder[i];
    }
    a->partCount = kZactorRiderParts;
}

BMX_ADDR(0x00465340)
void zactorSetupBike(void *actor)
{
    ZActor *a = actor;

    for (int i = 0; i < kZactorBikeParts; i++) {
        a->partHash[i] = (uint32_t)zstrHash(kBikePartNames[i]);
    }
    for (int i = 0; i < kZactorBikeParts; i++) {
        a->partOrder[i] = kBikePartOrder[i];
    }
    a->partCount = kZactorBikeParts;
}

/* Index of the rider the player controls (byte at 0x56b408); read directly, not through comdata. */
#define gCurrentPlayer ORIG_VAR(0x0056b408, uint8_t, gCurrentPlayer)

/* Rider records at 0x691f40 are 0xf70 bytes; the index is not range checked. */
static PhysRider *currentRider(void)
{
    return (PhysRider *)((uintptr_t)&gPhysRiders + (size_t)gCurrentPlayer * sizeof(PhysRider));
}

BMX_ADDR(0x004659e0)
void zactorSyncRiderActors(void)
{
    zactorSyncPose(currentRider()->actor);
    zactorSyncPose(currentRider()->bikeObj);
}
