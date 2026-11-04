/* Rider state (C:\zaxis\Bmx\Src\com\action\rider.c). */
#include "bmx/com/action/rider.h"

#include "bmx/com/action/camera.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/fx.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/physics_air.h"
#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/com/zge/zactor.h"
#include "compat/crt.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmath.h"

/*
 * Floating point follows the original x87 code exactly: `double` is a value held in an x87
 * register (53-bit precision control), `float` is a value the original stored to memory.
 */

static const char riderFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\rider.c";

/* Tunables and tables (.data / .rdata). */
typedef float RiderFloats16[16];
typedef float RiderStatTable[16 * 7];
typedef uint8_t RiderBytes256[256];
typedef uint16_t RiderWords256[256];
#define gRiderHopSpeed       ORIG_VAR(0x0055886c, float, gRiderHopSpeed)
#define gRiderHopBoost       ORIG_VAR(0x00558870, float, gRiderHopBoost)
#define gRiderHopMargin      ORIG_VAR(0x00558874, float, gRiderHopMargin)
#define gRiderJumpPower      ORIG_VAR(0x00558878, float, gRiderJumpPower)
#define gRiderJumpBase       ORIG_VAR(0x0055887c, float, gRiderJumpBase)
#define gRiderLandGrip       ORIG_VAR(0x00558888, float, gRiderLandGrip)
#define gRiderSteerBase      ORIG_VAR(0x0055888c, float, gRiderSteerBase)
#define gRiderSteerSpeedGain ORIG_VAR(0x00558890, float, gRiderSteerSpeedGain)
#define gRiderSteerFalloff   ORIG_VAR(0x00558894, float, gRiderSteerFalloff)
#define gRiderTrickSteer     ORIG_VAR(0x005588b0, float, gRiderTrickSteer)
#define gRiderJumpFrames     ORIG_VAR(0x005588b4, uint32_t, gRiderJumpFrames)
#define gRiderScoreWeight    ORIG_VAR(0x00558914, RiderFloats16, gRiderScoreWeight)
#define gRiderRiderStats     ORIG_VAR(0x00553664, RiderStatTable, gRiderRiderStats)
#define gRiderBikeStats      ORIG_VAR(0x005537b4, RiderStatTable, gRiderBikeStats)
#define gRiderStickDir       ORIG_VAR(0x00558904, RiderBytes256, gRiderStickDir)
#define gRiderTrickA         ORIG_VAR(0x00558f90, RiderBytes256, gRiderTrickA)
#define gRiderTrickB         ORIG_VAR(0x00558f9c, RiderBytes256, gRiderTrickB)
#define gRiderTrickKind      ORIG_VAR(0x00558fa8, RiderBytes256, gRiderTrickKind)
#define gRiderTrickCombo     ORIG_VAR(0x00558b40, RiderBytes256, gRiderTrickCombo)
#define gRiderPoseFlags      ORIG_VAR(0x00558fd0, RiderBytes256, gRiderPoseFlags)
#define gRiderPoseAnim       ORIG_VAR(0x00559030, RiderWords256, gRiderPoseAnim)

/* Air turning (degrees per frame): the rate a held turn starts at and its cap (scaled by stat[2]), the nudge
 * of the stick alone, and how fast the turn dies out without input. */
#define gRiderAirTurnNudge ORIG_VAR(0x00558898, float, gRiderAirTurnNudge)
#define gRiderAirTurnStart ORIG_VAR(0x0055889c, float, gRiderAirTurnStart)
#define gRiderAirTurnMax   ORIG_VAR(0x005588a0, float, gRiderAirTurnMax)
#define gRiderAirYawDamp   ORIG_VAR(0x00557808, float, gRiderAirYawDamp)
/* Bike spin of a spinning air trick (orientBRot.z, degrees per frame and cap, scaled by stat[2]). */
#define gRiderBikeSpinStep ORIG_VAR(0x005588a4, float, gRiderBikeSpinStep)
#define gRiderBikeSpinMax  ORIG_VAR(0x005588a8, float, gRiderBikeSpinMax)

/* A rider's signature air tricks: up to `count` stick directions that select anim and ik pose. */
typedef struct RiderSignatureTrick {
    uint8_t count;
    uint8_t dirs[3];
    uint8_t anim;
    uint8_t pose;
} RiderSignatureTrick;
typedef RiderSignatureTrick RiderSignatureTable[12][4];
typedef uint8_t RiderBytes4k[0x1000];
#define gRiderSignatureTricks ORIG_VAR(0x00553840, RiderSignatureTable, gRiderSignatureTricks)
/* Air trick by stick directions: [prev * 8 + last] for two directions, [last] for one (0xff: none). */
#define gRiderAirTrickPick ORIG_VAR(0x00558af7, RiderBytes256, gRiderAirTrickPick)
/* Modifier ik pose of an air trick, [anim * 0x26 + combo pose] (0xff: none). */
#define gRiderAirTrickMod ORIG_VAR(0x00558b8c, RiderBytes4k, gRiderAirTrickMod)
/* Per trick: first motion of its animation, and flags (bits 0..2 spin class, bit 3 held). The pose tables
 * gRiderPoseAnim / gRiderPoseFlags are the same tables from entry 0x1c on. */
#define gRiderTrickAnim  ORIG_VAR(0x00558ff8, RiderWords256, gRiderTrickAnim)
#define gRiderTrickFlags ORIG_VAR(0x00558fb4, RiderBytes256, gRiderTrickFlags)

enum {
    kAngleLimit = 0x238e, /* 50 degrees */
    kYawLimitStop = 0x444,
    kInputJump = 7,
    kInputTrick = 6,
    kInputHop = 1,
    kStatusPressed = 1,
    kStatusReleased = 2,
};

#define kDegreesToAngle 0x1.6c16c2p+7f /* 0x10000 / 360 */

/* ---- helpers ------------------------------------------------------------------------------------- */

static double absd(double v)
{
    return __builtin_fabs(v);
}

static float bitsToFloat(uint32_t bits)
{
    float f;
    memcpy(&f, &bits, sizeof f);
    return f;
}

static float readFloat(const uint8_t *base, uint32_t offset)
{
    float f;
    memcpy(&f, base + offset, sizeof f);
    return f;
}

static const void *inputState(const PhysRider *r)
{
    return &r->inputBits;
}

static int pressed(const PhysRider *r, uint32_t action, uint32_t status)
{
    return contrlerAction(r->padIndex, inputState(r), action, status);
}

/* The rider's fwd/up/side/omega rows, which mirror the bike basis (negated together on a turn around). */
static void flipBasis(PhysRider *r)
{
    zvec3Negate(&r->fwd, &r->fwd);
    zvec3Negate(&r->side, &r->side);
    zvec3Negate(&r->basis.right, &r->basis.right);
    zvec3Negate(&r->basis.at, &r->basis.at);
    r->rot.x = (ZAngle)-r->rot.x;
    r->rot.z = (ZAngle)-r->rot.z;
}

static uint32_t stateTimerCapped(const PhysRider *r, int level)
{
    const uint32_t t = (uint16_t)r->stateTimer[level];
    return t < gRiderJumpFrames ? t : gRiderJumpFrames;
}

/* The upward kick of a jump that grows with the frames spent charging it (level `level`). */
static double jumpKick(const PhysRider *r, uint32_t frames)
{
    double v = (double)gRiderJumpPower * (double)r->stat[0];
    v = v - (double)gRiderJumpBase;
    v = v * (double)(int32_t)frames;
    v = v / (double)(int32_t)gRiderJumpFrames;
    return v + (double)gRiderJumpBase;
}

/* Launches the rider upwards with the charged kick and enters the air mode. */
static void launchRider(PhysRider *r, int level)
{
    ZVec3 v;
    zvec3Zero(&v);
    v.y = (float)-jumpKick(r, stateTimerCapped(r, level));
    physAddVelocity(r, &v);
    physSetState(r, kPhysStateAir);
    riderSetStateLevel(r, 0, kRiderAir);
    r->fieldB9C = 0xf;
}

/* Shared by the roll, manual and mode 5 steps: turns the rider's yaw (rot.y) with the stick. */
static void steerYaw(PhysRider *r)
{
    const float *axes = contrlerAxes(inputState(r), 0);
    const float steer = axes[0];

    if (!(steer < 0.0f || steer > 0.0f)) {
        const int16_t yaw = r->rot.y;
        double t = (double)(int32_t)yaw * (double)0x1.68p-8f;
        t = t * (double)gPhysRollYawDamp;
        t = t * (double)kDegreesToAngle;
        r->rot.y = (ZAngle)(yaw - bmxFtol(t));
        if (absd((double)r->rot.y) < 91.0) {
            r->rot.y = 0;
        }
        return;
    }
    int16_t target;
    int16_t limit;
    if (r->speed > 0.0f) {
        double q = (double)gRiderSteerFalloff * (double)r->speed + 1.0f;
        q = (double)r->stat[1] / q;
        if ((double)0.05f > q) {
            q = (double)0.05f;
        }
        double t = (double)gRiderSteerBase * q;
        t = t * (double)steer;
        t = t * (double)r->stat[1];
        t = t * (double)kDegreesToAngle;
        const int32_t kick = bmxFtol(t);
        double l = absd((double)steer) * (double)gRiderSteerSpeedGain;
        l = l * q;
        l = l * (double)kDegreesToAngle;
        target = (int16_t)((int16_t)kick + r->rot.y);
        limit = (int16_t)bmxFtol(l);
    } else {
        double t = (double)steer * 3.0f;
        t = t * (double)r->stat[1];
        t = t * (double)kDegreesToAngle;
        target = (int16_t)((int16_t)bmxFtol(t) + r->rot.y);
        limit = kYawLimitStop;
    }
    const int lim = limit;
    const int smaller = target < limit ? target : lim;
    int value;
    if (-lim > smaller) {
        value = -lim;
    } else {
        value = target < limit ? target : lim;
    }
    r->rot.y = (ZAngle)value;
}

/* ---- callbacks and set up ------------------------------------------------------------------------ */

/* Empty (debug output stripped from the shipped build). */
BMX_ADDR(0x00430e30)
void riderNop0(void) {}

BMX_ADDR(0x00430e50)
void riderNop1(void) {}

/* Reads each player's controller into the rider record: button bits, the sticks and their byte scaled values.
 */
BMX_ADDR(0x00430e70)
void riderReadPads(void)
{
    for (uint32_t i = 0; i < gPlayerCount; i++) {
        PhysRider *r = &gPhysRiders[i];
        const uint8_t *pad = contrlerGet(gProfiles[i].controller);

        memcpy(&r->inputBits, pad + 0x10, 4);
        memcpy(&r->inputBits2, pad + 0x14, 4);
        for (uint32_t j = 0; j < 2; j++) {
            const int32_t a = bmxFtol((double)readFloat(pad, 0x28 + j * 8) * 127.0f);
            const int32_t b = bmxFtol((double)readFloat(pad, 0x2c + j * 8) * 127.0f);
            r->axis[j * 2] = (float)((double)(int8_t)a * (double)0x1.020408p-7f);
            r->axis[j * 2 + 1] = (float)((double)(int8_t)b * (double)0x1.020408p-7f);
            const int32_t c = bmxFtol((double)readFloat(pad, 0x1c + j * 8) * 127.0f);
            const int32_t d = bmxFtol((double)readFloat(pad, 0x18 + j * 8) * 127.0f);
            r->stick[j * 2] = (float)((double)(int8_t)d * (double)0x1.020408p-7f);
            r->stick[j * 2 + 1] = (float)((double)(int8_t)c * (double)0x1.020408p-7f);
        }
    }
}

/*
 * Adds the stick directions that were just entered to the direction buffer (dirList, at most four, the
 * oldest dropped). `held` is the pad's direction bits plus the deflected main stick; a direction counts as
 * new when it shows in exactly one of the stick and the second axis pair, or in inputBits2.
 */
static void bufferDirections(PhysRider *r)
{
    uint8_t *bytes = (uint8_t *)r;
    uint32_t held = r->inputBits & 0xf;
    uint32_t fresh = r->inputBits2 & 0xf;
    const double dead = 0.5f;

    if (absd((double)r->stick[0]) > dead) {
        held |= r->stick[0] < 0.0f ? 4 : 8;
        if (!(absd((double)r->axis[0]) > dead)) {
            fresh |= r->stick[0] < 0.0f ? 4 : 8;
        }
    } else if (absd((double)r->axis[0]) > dead) {
        fresh |= r->axis[0] < 0.0f ? 4 : 8;
    }
    if (absd((double)r->stick[1]) > dead) {
        held |= r->stick[1] < 0.0f ? 1 : 2;
        if (!(absd((double)r->axis[1]) > dead)) {
            fresh |= r->stick[1] < 0.0f ? 1 : 2;
        }
    } else if (absd((double)r->axis[1]) > dead) {
        fresh |= r->axis[1] < 0.0f ? 1 : 2;
    }
    if (fresh != 0) {
        if (fresh == held) {
            if (r->dirCount == 4) {
                for (int i = 0; i < 3; i++) {
                    r->dirList[i] = r->dirList[i + 1];
                }
                r->dirCount--;
            }
            bytes[6 + r->dirCount] = gRiderStickDir[held];
            r->dirCount++;
        } else if ((fresh & held) != 0) {
            /* a direction changed while held: the last entry is replaced */
            if (r->dirCount != 0) {
                bytes[5 + r->dirCount] = gRiderStickDir[held];
            } else {
                bytes[6] = gRiderStickDir[held];
                r->dirCount++;
            }
        }
    }
    if (r->dirCount > 4) {
        zdebugFail("direction buffer out of range (post)\n", riderFile, 0x14d, 0);
    }
}

/* Whether the rider actor's first motion channel still has keys to play. */
static int motionPlaying(const PhysRider *r)
{
    const ZMotionPlayer *player = ((const ZActor *)r->actor)->player;
    const uint8_t *cursor = player->ch[1].cursor;
    uint32_t key = 0;
    if (cursor != NULL) {
        memcpy(&key, cursor, sizeof key);
    }
    return cursor != NULL && key != 0;
}

/* Adds a motion step's root displacement to the rider's motion offset; y follows the actor's height. */
static void addMotionStep(PhysRider *r, const ZMotionDelta *d)
{
    r->fieldBFC.y = (float)-(double)zactorGetHeight(r->actor);
    r->fieldBFC.x = (float)((double)d->delta.x + (double)r->fieldBFC.x);
    r->fieldBA4 &= 0xfffffffdu;
    r->fieldBFC.z = (float)((double)d->delta.z + (double)r->fieldBFC.z);
}

static const ZMotion *riderMotion(uint32_t index)
{
    return (const ZMotion *)gPhysMotions[index];
}

/*
 * Per frame update of every rider: direction buffer, respawn when out of the world bounds, the mode step
 * functions until one finishes, then the animation (one motion, or two blended while bit 0x200 of fieldBA4
 * is set). Flag 0x200 of flags is set while no motion is playing.
 */
BMX_ADDR(0x00431000)
void riderUpdateAll(void)
{
    for (uint32_t i = 0; i < (gPlayerCount == 0 ? 1u : gPlayerCount); i++) {
        PhysRider *r = &gPhysRiders[i];
        if ((r->flags & 1) == 0) {
            continue;
        }
        if ((r->flags & 0x10) != 0) {
            bufferDirections(r);
        }
        const float *bounds = zrollerGetWorldBounds();
        const double margin = 100.0f;
        if (!((double)bounds[5] + margin >= (double)r->pos.y) ||
            (double)bounds[0] - margin > (double)r->pos.x ||
            !((double)bounds[4] + margin >= (double)r->pos.x) ||
            (double)bounds[2] - margin > (double)r->pos.z ||
            !((double)bounds[6] + margin >= (double)r->pos.z)) {
            physRespawn(r);
            riderSetStateLevel(r, 0, kRiderRoll);
        }
        r->flags &= 0xfffdff7fu;
        r->fieldC60 = 0.0f;
        r->fieldC64 = 0.0f;
        const RiderStepFn step = gRiderModeStep[r->stateLevel[0]];
        if (step == NULL) {
            zdebugFail("Invalid main update proc. (NULL)", riderFile, 0x168, 0);
        }
        int done = step(r);
        while (done == 0) {
            done = gRiderModeStep[r->stateLevel[0]](r);
        }
        const uint32_t anim = r->fieldBA4 | 0x80;
        r->stateTimer[0]++;
        r->fieldBA4 = anim;
        if ((anim & 0x100) == 0) {
            if ((anim & 2) != 0) {
                zvec3Zero(&r->fieldBFC);
            }
            if ((r->fieldBA4 & 0x200) == 0) {
                ZMotionDelta *d =
                    zactorPlayMotion(r->actor, riderMotion((uint16_t)r->fieldB9C), r->animSpeed, r->fieldBA4);
                r->fieldBA0 = d;
                if (d != NULL) {
                    addMotionStep(r, d);
                    zactorCopySecondaryJoints(r->actor, r->bikeObj);
                }
                if (d != NULL && motionPlaying(r)) {
                    r->flags &= 0xfffffdffu;
                } else {
                    r->flags |= 0x200;
                }
            } else {
                const uint16_t blendFlags[2] = {(uint16_t)r->fieldBA4, r->motionFlags};
                ZMotionDelta **d =
                    zactorBlendMotions(r->actor, riderMotion((uint16_t)r->fieldB9C),
                                       riderMotion(r->motionIndex), blendFlags, r->motionWeight);
                if (d != NULL && d[0] != NULL && d[1] != NULL) {
                    r->fieldBA0 = d[0];
                    r->motionSample = d[1];
                    addMotionStep(r, d[1]);
                    r->motionFlags &= 0xfffdu;
                    zactorCopySecondaryJoints(r->actor, r->bikeObj);
                    r->flags &= 0xfffffdffu;
                } else {
                    r->fieldBA0 = NULL;
                    r->motionSample = NULL;
                    r->flags |= 0x200;
                }
                r->fieldBA4 &= 0xfffffdffu;
            }
            r->fieldBA4 &= 0xffffff7fu;
            zactorSyncPose(r->actor);
            zrwmatUpdate(((const ZActor *)r->actor)->object);
        }
        if (r->soundCooldown != 0) {
            r->soundCooldown--;
        }
    }
}

/*
 * Creates the rider and bike actors of a player: look numbers, tuning values (the rider's and the bike's
 * 7 stats multiplied), wheel positions and the IK limbs. Returns NULL if an actor could not be created.
 */
BMX_ADDR(0x004316c0)
PhysRider *riderCreate(int player, int look, int bikeLook, uint32_t bikeIndex)
{
    PhysRider *r = &gPhysRiders[player];

    r->player = (uint8_t)player;
    r->padIndex = gProfiles[player].pad02;
    r->look = (uint16_t)look;
    r->bikeLook = (uint8_t)bikeLook;
    r->bikeLookId = bikeIndex < 4 ? (int32_t)(bikeIndex + (uint32_t)look * 4) : 0x30;
    r->actor = zrollerCreatePlayerActor((const uint16_t *)r);
    if (r->actor == NULL) {
        gZdebugPrint("Failed to create rider actor for rider %ld (lookNum %ld)\n", player, bikeLook);
        return NULL;
    }
    r->bikeObj = zrollerCreateBike(&r->bikeLookId);
    if (r->bikeObj == NULL) {
        gZdebugPrint("Failed to create bike actor for rider %ld (bikeLookNum %ld)\n", player,
                     (int32_t)bikeIndex);
        return NULL;
    }
    ((uint8_t *)zrollerGetRecord((uint32_t)player))[7] = (uint8_t)bikeIndex;
    r->actor2 = zoverCreateRiderMotion(r->actor, (gPhysSyncFlags & 4) != 0 && gPlayerCount != 0 ? 1 : 0);
    zoverSetHitCallback(r->actor2, BMX_FNPTR(ZoverHitFn, 0x00436020, riderCollide));
    if ((gPhysSyncFlags & 0x80) != 0 && gPlayerCount != 0) {
        zoverMotionSetupA(r->actor2, r->actor);
    } else {
        zoverMotionSetupB(r->actor2, r->actor);
    }
    r->bikeObj2 = zoverCreateBikeMotion();
    zoverSetHitCallback(r->bikeObj2, BMX_FNPTR(ZoverHitFn, 0x004364b0, guiPlayVelocitySound));
    memset((uint8_t *)r + offsetof(PhysRider, motionScale), 0, 0x3c);
    for (uint32_t k = 0; k < 7; k++) {
        const double product =
            (double)gRiderBikeStats[bikeIndex * 7 + k] * (double)gRiderRiderStats[(uint32_t)look * 7 + k];
        r->stat[k] = (float)product;
        if (bikeIndex < 4) {
            const double limit = k == 6 ? 2.0f : 1.0f;
            r->stat[k] = limit < product ? (float)limit : (float)product;
        }
    }
    r->fieldC68 = bitsToFloat(0x42480000);
    r->fieldC6C = bitsToFloat(0x40a00000);
    r->fieldC70 = bitsToFloat(0x3e4ccccd);
    r->wheel[0].anchor.x = bitsToFloat(0x425d0312);
    r->wheel[0].anchor.y = bitsToFloat(0xc0b3126f);
    r->wheel[0].anchor.z = 0.0f;
    r->wheel[0].radius = bitsToFloat(0x41c528f6);
    r->wheel[1].anchor.x = bitsToFloat(0xc212147b);
    r->wheel[1].anchor.y = bitsToFloat(0xc0b3126f);
    r->wheel[1].anchor.z = 0.0f;
    r->wheel[1].radius = bitsToFloat(0x41c528f6);
    if ((gPhysSyncFlags & 0x10) != 0 && gPlayerCount != 0) {
        r->travelMin = bitsToFloat(0xc1700000);
        r->travelMax = bitsToFloat(0x41c80000);
        r->fieldC7C = bitsToFloat(0x3e99999a);
        r->fieldC80 = bitsToFloat(0x3dcccccd);
        r->fieldC84 = bitsToFloat(0x3dcccccd);
    } else {
        r->travelMin = 0.0f;
        r->travelMax = 0.0f;
        r->fieldC7C = 0.0f;
        r->fieldC80 = 0.0f;
        r->fieldC84 = 0.0f;
    }
    zoverLimbsBegin(r->actor, r->ik);
    const uint32_t viewPlayer = comdataGetViewPlayer();
    cameraSelectPlayer((uint32_t)player);
    zoverReadLimbs(r->actor, r->ik);
    cameraSelectPlayer(viewPlayer);
    const float one = 1.0f;
    const float zero = 0.0f;
    memcpy(r->ik + 0x100, &one, 4);
    memcpy(r->ik + 0x104, &zero, 4);
    memcpy(r->ik + 0x108, &zero, 4);
    return r;
}

/* Resets a rider to the level start: orientation, motion poses, state levels and visibility. */
BMX_ADDR(0x00431aa0)
void riderReset(int player, int full)
{
    PhysRider *r = &gPhysRiders[player];
    ZAngle3 rot;

    zrollerGetStart(0, &r->pos, &rot);
    ZMatrix *frame = (ZMatrix *)(void *)&r->fwd;
    zmatrixIdentity(frame);
    zmatrixRotateEuler(frame, frame, &rot);
    zoverRiderPoseA(r->actor2, r->actor);
    zoverRiderPoseB(r->actor2, r->actor);
    zoverRiderPoseC(r->actor2, r->actor);
    zoverRiderPoseD(r->actor2, r->actor);
    zoverRiderPoseE(r->actor2, r->actor);
    r->fieldBA4 = 0x80;
    r->inputBits = 0;
    r->inputBits2 = 0;
    for (int i = 0; i < 4; i++) {
        r->stick[i] = 0.0f;
    }
    for (int i = 0; i < 4; i++) {
        r->axis[i] = 0.0f;
    }
    memset(r->dirList, 0, sizeof r->dirList);
    r->flags |= 0x200;
    r->dirCount = 0;
    riderSetStateLevel(r, 0, 0);
    r->state = 0xff;
    physRespawn(r);
    r->soundCooldown = 0;
    r->animSpeed = 1.0f;
    if (full != 0) {
        r->flags |= 1;
        zactorShow(r->actor);
        zactorShow(r->bikeObj);
    } else {
        r->flags &= 0xfffffffeu;
        zactorHide(r->actor);
        zactorHide(r->bikeObj);
    }
    zoverLimbsBegin(r->actor, r->ik);
}

BMX_ADDR(0x00431c80)
void riderNop2(void) {}

/*
 * Sets state level `level` to `value` and clears the deeper levels. Entering mode `value` on level 0 sets
 * up the matching animation state; mode 5 returns without touching anything more.
 */
BMX_ADDR(0x00431ca0)
uint32_t riderSetStateLevel(PhysRider *r, int level, uint32_t value)
{
    r->stateLevel[level] = (uint8_t)value;
    r->stateTimer[level] = 0;
    for (uint32_t i = (uint32_t)level + 1; i < 4; i++) {
        r->stateLevel[i] = 0;
        r->stateTimer[i] = 0;
    }
    const uint32_t anim = r->fieldBA4;
    r->animSpeed = 1.0f;
    r->fieldBA4 = anim & 0xfffffeffu;
    if (level == 0) {
        switch (value) {
        case 1:
            riderLatchDirection(r, 1);
            r->flags &= 0xffcfffffu;
            break;
        case 2:
        case 3:
            break;
        case 5:
            return value;
        default:
            r->fieldBA4 = anim & 0xfffffef7u;
            riderLatchDirection(r, 0);
            r->anim = 0xff;
            r->ikPose = 0xff;
            r->spin = 0;
            r->ikFrame = 0;
            r->flags &= 0xf7fff7ffu;
            r->motionIndex = 4;
            break;
        }
    } else if (level != 1) {
        return r->stateLevel[level];
    }
    r->field54 = 0;
    r->flags &= 0xfffffbfbu;
    return r->stateLevel[level];
}

BMX_ADDR(0x00431dd0)
int riderGetStateLevel(const PhysRider *r, unsigned level)
{
    if (level > 3) {
        zdebugFail("stateLevel out of range.", riderFile, 0x2c5, 0);
    }
    return r->stateLevel[level];
}

/* Adds points to the rider's score: kind selects the weight, `amount` is the impact or trick size. */
BMX_ADDR(0x00431e10)
void riderAddScore(PhysRider *r, int kind, float amount)
{
    double scale = 1.0f;

    if ((r->flags & 0x1000000) == 0) {
        if (kind == 13 && r->up.y > 0.0f) {
            scale = (double)r->up.y * 5.0f + 1.0f;
        }
    } else {
        scale = 0.5f;
    }
    scale = scale * (double)gRiderScoreWeight[kind];
    scale = scale * (double)amount;
    scale = scale * (double)0x1.555556p-4f;
    r->field58 = (int32_t)((uint32_t)r->field58 + (uint32_t)bmxFtol(scale));
}

/* ---- mode steps ---------------------------------------------------------------------------------- */

/* Runs a mode's sub state handler until a handler finishes or the mode changes. */
#define RUN_SUBSTATES(table, mode)                                                                           \
    do {                                                                                                     \
        int result = (table)[r->stateLevel[1]](r);                                                           \
        while (result == 0 && riderGetStateLevel(r, 0) == (mode)) {                                          \
            result = (table)[r->stateLevel[1]](r);                                                           \
        }                                                                                                    \
        return result;                                                                                       \
    } while (0)

/* Rolling on the ground: follows the physics state into the matching mode, else steers and runs a sub state.
 */
BMX_ADDR(0x00431ea0)
int riderStepRoll(PhysRider *r)
{
    switch (physGetState(r)) {
    case 1:
        if (riderGetStateLevel(r, 1) == 4 && riderGetStateLevel(r, 2) == 2) {
            r->fieldBA4 |= 0x80;
            flipBasis(r);
        }
        riderSetStateLevel(r, 0, kRiderAir);
        r->fieldB9C = 0x17;
        return 0;
    case 2:
        riderSetStateLevel(r, 0, kRiderBail);
        return 0;
    case 3:
        riderSetStateLevel(r, 0, kRiderGrind);
        return 0;
    case 4:
        riderSetStateLevel(r, 0, kRiderGrind);
        riderSetStateLevel(r, 2, 2);
        r->anim = (uint8_t)riderGetTrickA(r);
        return 0;
    case 6:
        riderSetStateLevel(r, 0, kRiderLip);
        riderSetStateLevel(r, 2, 2);
        r->anim = (uint8_t)riderGetTrickB(r);
        r->flags |= 0x100;
        return 0;
    default:
        break;
    }
    if ((r->flags & 0x40000) != 0 && riderGetStateLevel(r, 1) != 4) {
        riderSetStateLevel(r, 1, 4);
    }
    steerYaw(r);
    if (pressed(r, kInputJump, kStatusPressed) != 0) {
        r->flags |= 0x20000;
    }
    RUN_SUBSTATES(gRiderRollStep, kRiderRoll);
}

/* Landing from the air: a held manual (up up, or down down plus the stick) carries over, else mode 5. */
static int landFromAir(PhysRider *r)
{
    gameScoreJump(r->player, 1);
    if ((r->flags & 0x400000) != 0 && r->dirCount > 1) {
        const uint8_t *bytes = (const uint8_t *)r;
        uint32_t manual = 0;
        if (bytes[5 + r->dirCount] == 4 && bytes[4 + r->dirCount] == 4 &&
            (double)contrlerAxes(inputState(r), 0)[1] > (double)0.5f) {
            manual = 0x100000;
        } else if (bytes[5 + r->dirCount] == 3 && bytes[4 + r->dirCount] == 3 &&
                   !((double)contrlerAxes(inputState(r), 0)[1] >= (double)-0.5f)) {
            manual = 0x200000;
        }
        if (manual != 0) {
            r->flags |= manual;
            riderSetStateLevel(r, 0, kRiderManual);
            r->flags |= 0x80;
            return 0;
        }
    }
    riderSetStateLevel(r, 0, kRiderMode5);
    return 0;
}

/* Turning in the air with a turn button held: starts at gRiderAirTurnStart and speeds up to the cap. */
static void airTurnHeld(PhysRider *r)
{
    const int16_t cap =
        (int16_t)bmxFtol((double)gRiderAirTurnMax * (double)r->stat[2] * (double)kDegreesToAngle);
    const int16_t yaw = r->rot.y;
    if (yaw != 0 && (pressed(r, 0xa, kStatusPressed) ^ (yaw < 0)) == 0) {
        double t = (double)(int32_t)yaw * (double)0x1.68p-8f;
        t = t * 1.25f;
        t = t * (double)kDegreesToAngle;
        const int16_t faster = (int16_t)bmxFtol(t);
        const int smaller = faster < cap ? faster : cap;
        r->rot.y = (ZAngle)(-(int)cap > smaller ? -(int)cap : smaller);
        return;
    }
    double start = (double)gRiderAirTurnStart;
    if (pressed(r, 0xa, kStatusPressed) != 0) {
        start = -start;
    }
    r->rot.y = (ZAngle)bmxFtol(start * (double)kDegreesToAngle);
}

/* Turning in the air with the stick alone; without any input the turn dies out. */
static void airTurnFree(PhysRider *r)
{
    const double steer = contrlerAxes(inputState(r), 0)[0];
    const int16_t yaw = r->rot.y;
    if (!(steer < 0.0f || steer > 0.0f)) {
        double t = (double)(int32_t)yaw * (double)0x1.68p-8f;
        t = t * (double)gRiderAirYawDamp;
        t = t * (double)kDegreesToAngle;
        r->rot.y = (ZAngle)(yaw - bmxFtol(t));
        if (absd((double)r->rot.y) < 91.0f) {
            r->rot.y = 0;
        }
        return;
    }
    const int32_t cap = bmxFtol((double)gRiderAirTurnMax * (double)r->stat[2] * (double)kDegreesToAngle);
    double v;
    if (yaw != 0 && ((steer < 0.0f ? 1 : 0) ^ (yaw < 0)) == 0) {
        double t = (double)(int32_t)yaw * (double)0x1.68p-8f;
        t = t * 1.25f;
        const float limit = (float)((double)(int16_t)cap * (double)0x1.68p-8f);
        v = t < (double)limit ? t : (double)limit;
        if ((double)-limit > v) {
            v = (double)-limit;
        }
    } else {
        v = (double)gRiderAirTurnNudge;
        if (steer < 0.0f) {
            v = -v;
        }
    }
    r->rot.y = (ZAngle)bmxFtol(v * (double)kDegreesToAngle);
}

/* Restarts a running air sub state at 0, running its handler once. Returns whether it did. */
static int airSubRestart(PhysRider *r)
{
    if (riderGetStateLevel(r, 1) == 0) {
        return 0;
    }
    riderSetStateLevel(r, 1, 0);
    gRiderAirStep[r->stateLevel[1]](r);
    r->flags |= 0x80;
    return 1;
}

/*
 * Flying: follows the physics state (landing, bail, grind), else bail buttons, turning, the manual pre
 * trim of yawTrim and the trick buttons (3 and 4 start the air sub states 1 and 2, 7 grabs a grind or lip,
 * 8 a lip trick), then runs the air sub state.
 */
BMX_ADDR(0x004322b0)
int riderStepAir(PhysRider *r)
{
    switch (physGetState(r)) {
    case 0:
        return landFromAir(r);
    case 2:
        gameScoreJump(r->player, 0);
        riderSetStateLevel(r, 0, kRiderBail);
        return 0;
    case 3:
        riderSetStateLevel(r, 0, kRiderGrind);
        return 0;
    default:
        break;
    }
    if (pressed(r, 0x10, 0) != 0 || pressed(r, 0x11, 0) != 0) {
        physSetState(r, 2);
        r->bailReason = 0xb;
        riderSetStateLevel(r, 0, kRiderBail);
        return 0;
    }
    const int turnB = pressed(r, 0xb, kStatusPressed);
    const int turnA = pressed(r, 0xa, kStatusPressed);
    if ((turnB ^ turnA) != 0) {
        airTurnHeld(r);
    } else {
        airTurnFree(r);
    }
    r->spin = (int32_t)((uint32_t)r->spin + (uint32_t)(int32_t)r->rot.y);
    if ((r->flags & 0x1000) != 0 && r->timer != 0) {
        if (absd((double)r->orientARot.y) < absd((double)r->rot.y)) {
            r->flags &= 0xffffefffu;
            r->timer = 0;
        } else {
            r->spin = (int32_t)((uint32_t)r->spin + (uint32_t)(int32_t)r->orientARot.y);
        }
    }
    r->rot.z = (ZAngle)(r->rot.z >> 1);
    if (absd((double)r->rot.z) < 91.0f) {
        r->rot.z = 0;
    }
    if (r->dirCount > 1) {
        const float stick = contrlerAxes(inputState(r), 0)[1];
        if (absd((double)stick) > (double)0.5f) {
            const uint8_t *bytes = (const uint8_t *)r;
            const uint8_t last = bytes[5 + r->dirCount];
            if (last == 4 && bytes[4 + r->dirCount] == 4 && stick > 0.5f) {
                const int trim = r->yawTrim + 0x2d8;
                r->yawTrim = (int16_t)(trim > 0x18e3 ? 0x18e3 : trim);
            } else if (last == 3 && bytes[4 + r->dirCount] == 3 && stick < -0.5f) {
                const int trim = r->yawTrim - 0x1c7;
                r->yawTrim = (int16_t)(trim < -0x18e3 ? -0x18e3 : trim);
            }
        }
    }
    int buttons = riderGetStateLevel(r, 1) == 0;
    if (!buttons) {
        buttons = (riderGetStateLevel(r, 1) == 1 && (uint32_t)riderGetStateLevel(r, 2) >= 3) ||
                  (riderGetStateLevel(r, 1) == 2 && (uint32_t)riderGetStateLevel(r, 2) >= 3);
    }
    if (buttons) {
        if (pressed(r, 3, 0) != 0 || ((r->flags & 0x20) != 0 && r->dirCount != 0)) {
            if (r->dirCount != 0) {
                if (airSubRestart(r)) {
                    r->rot.z = 0;
                }
                riderSetStateLevel(r, 1, 1);
                return 0;
            }
            r->stateTimer[3] = 0;
            r->flags = (r->flags & 0xffffffbfu) | 0x20;
        } else if ((r->flags & 0x20) != 0) {
            const uint32_t f = r->flags;
            r->stateTimer[3]++;
            if ((uint16_t)r->stateTimer[3] > 7) {
                r->flags = f & 0xffffffdfu;
            }
        }
        if (pressed(r, 4, 0) != 0 || ((r->flags & 0x40) != 0 && r->dirCount != 0)) {
            if (r->dirCount != 0) {
                airSubRestart(r);
                riderSetStateLevel(r, 1, 2);
                r->rot.z = 0;
                return 0;
            }
            r->stateTimer[3] = 0;
            r->flags = (r->flags & 0xffffffdfu) | 0x40;
        } else if ((r->flags & 0x40) != 0) {
            const uint32_t f = r->flags;
            r->stateTimer[3]++;
            if ((uint16_t)r->stateTimer[3] > 7) {
                r->flags = f & 0xffffffbfu;
            }
        }
        if (pressed(r, kInputJump, kStatusPressed) != 0 && riderGetStateLevel(r, 2) != 0) {
            const uint8_t state = physSetState(r, 3);
            if (state == 3) {
                airSubRestart(r);
                riderSetStateLevel(r, 0, kRiderGrind);
                return 0;
            }
            if (state == 5) {
                airSubRestart(r);
                riderSetStateLevel(r, 0, kRiderLip);
                r->anim = (uint8_t)riderGetTrickB(r);
                return 0;
            }
        }
        if (pressed(r, 8, 0) != 0 && riderGetTrickKind(r) != 0xff) {
            if (physSetState(r, 5) == 5) {
                airSubRestart(r);
                riderSetStateLevel(r, 0, kRiderLip);
                r->anim = (uint8_t)riderGetTrickKind(r);
                return 0;
            }
            riderLatchDirection(r, 1);
        }
    }
    RUN_SUBSTATES(gRiderAirStep, kRiderAir);
}

BMX_ADDR(0x00432d50)
int riderStepGrind(PhysRider *r)
{
    switch (physGetState(r)) {
    case 0:
        riderSetStateLevel(r, 0, kRiderRoll);
        return 0;
    case 1:
        riderSetStateLevel(r, 0, kRiderAir);
        r->fieldB9C = (int16_t)((r->anim + 0x24) * 3);
        r->flags |= 0x8000000;
        return 0;
    case 2:
        riderSetStateLevel(r, 0, kRiderBail);
        return 0;
    default:
        RUN_SUBSTATES(gRiderGrindStep, kRiderGrind);
    }
}

BMX_ADDR(0x00432e40)
int riderStepLip(PhysRider *r)
{
    switch (physGetState(r)) {
    case 0:
        riderSetStateLevel(r, 0, kRiderRoll);
        return 0;
    case 1:
        riderSetStateLevel(r, 0, kRiderAir);
        r->fieldB9C = (int16_t)((r->anim + 0x24) * 3);
        r->flags |= 0x8000000;
        return 0;
    case 2:
        riderSetStateLevel(r, 0, kRiderBail);
        return 0;
    default:
        RUN_SUBSTATES(gRiderLipStep, kRiderLip);
    }
}

BMX_ADDR(0x00432f30)
int riderStepManual(PhysRider *r)
{
    switch (physGetState(r)) {
    case 1:
        riderSetStateLevel(r, 0, kRiderAir);
        r->fieldB9C = 0x15;
        r->flags |= 0x8000000;
        return 0;
    case 2:
        riderSetStateLevel(r, 0, kRiderBail);
        return 0;
    default:
        break;
    }
    if ((r->flags & 0x400000) == 0) {
        riderSetStateLevel(r, 0, kRiderRoll);
        return 0;
    }
    steerYaw(r);
    RUN_SUBSTATES(gRiderManualStep, kRiderManual);
}

BMX_ADDR(0x004331d0)
int riderStepMode5(PhysRider *r)
{
    switch (physGetState(r)) {
    case 1:
        riderSetStateLevel(r, 0, kRiderAir);
        r->fieldB9C = 0x17;
        return 0;
    case 2:
        riderSetStateLevel(r, 0, kRiderBail);
        return 0;
    default:
        break;
    }
    steerYaw(r);
    const int level = riderGetStateLevel(r, 1);
    if (level == 0) {
        r->fieldB9C = (r->flags & 0x10000) == 0 ? 5 : 0x16;
        riderSetStateLevel(r, 1, 1);
        r->fieldBA4 |= 2;
    } else if (level == 1) {
        r->stateTimer[1]++;
        if ((uint16_t)r->stateTimer[1] > 6) {
            riderSetStateLevel(r, 0, kRiderRoll);
            return 0;
        }
    }
    return 1;
}

/* Lets go of whatever the rider rig's hands hold. */
static void bailReleaseHands(PhysRider *r)
{
    zoverRigReleaseHands(r->actor2);
    r->flags &= 0xfbffffffu;
}

/*
 * Bailing: sub state 1 tumbles (the stick pitches and yaws the rag doll while falling, button 0x12 goes limp,
 * 0x13 grabs), 2 hangs on, 3 waits to respawn, 4 and 5 sink in water for a second and respawn.
 */
BMX_ADDR(0x00433480)
int riderStepBail(PhysRider *r)
{
    switch ((uint32_t)riderGetStateLevel(r, 1)) {
    case 0:
        bailReleaseHands(r);
        riderSetStateLevel(r, 1, 1);
        return 1;
    case 1: {
        zoverRigEndLimp(r->actor2);
        zoverRigEndStiff(r->actor2);
        zoverRigClearPitch(r->actor2);
        zoverRigClearYaw(r->actor2);
        r->flags &= 0xfeffffffu;
        if (r->surface == 0xd) {
            riderSetStateLevel(r, 1, 4);
            return 0;
        }
        PhysGroundHit ground;
        if (physProbeGround(&r->pos, &ground) != 0 &&
            absd((double)ground.pos.y - (double)r->pos.y) > (double)100.0f) {
            if (pressed(r, 0x12, kStatusPressed) != 0 && (r->flags & 0x4000000) == 0) {
                zoverRigGoLimp(r->actor2, r->actor);
                r->flags |= 0x1000000;
            }
            const double pitch = contrlerAxes(inputState(r), 0)[1];
            if (absd(pitch) > (double)0.5f) {
                zoverRigRequestPitch(r->actor2, r->actor,
                                     (int16_t)bmxFtol(pitch * (double)-10.0f * (double)kDegreesToAngle));
            }
            const double yaw = contrlerAxes(inputState(r), 0)[0];
            if (absd(yaw) > (double)0.5f) {
                zoverRigRequestYaw(r->actor2, r->actor,
                                   (int16_t)bmxFtol(yaw * (double)10.0f * (double)kDegreesToAngle));
            }
        }
        if ((r->flags & 0x1000000) == 0) {
            if (pressed(r, 0x13, kStatusPressed) != 0) {
                zoverRigEase(r->actor2, r->actor);
                r->flags |= 0x4000000;
                if (zoverRigHandsHolding() != 0) {
                    riderSetStateLevel(r, 1, 2);
                    zoverRigClearPitch(r->actor2);
                    zoverRigClearYaw(r->actor2);
                    return 0;
                }
            } else {
                bailReleaseHands(r);
            }
        }
        if ((uint16_t)r->stateTimer[0] > 0x96) {
            riderSetStateLevel(r, 1, 3);
            return 0;
        }
        break;
    }
    case 2:
        if (pressed(r, 0x13, kStatusPressed) == 0 || zoverRigHandsHolding() == 0) {
            bailReleaseHands(r);
            riderSetStateLevel(r, 1, 1);
            r->stateTimer[0] = 0;
            return 0;
        }
        if ((uint16_t)r->stateTimer[0] > 300) {
            riderSetStateLevel(r, 1, 3);
            return 0;
        }
        break;
    case 3:
        if ((r->flags & 6) == 0) {
            zoverRigClearPitch(r->actor2);
            zoverRigClearYaw(r->actor2);
            r->flags &= 0xfeffffffu;
            r->fieldBA4 |= 0x80;
            physRespawn(r);
            riderSetStateLevel(r, 0, kRiderRoll);
            return 0;
        }
        return 1;
    case 4:
        fxWaterSplash(r->actor, 0, &gPhysZero.x);
        bmxSoundPlayAt(0xfa2, &r->pos, 3, 0, 0);
        zactorHide(r->actor);
        zactorHide(r->bikeObj);
        riderSetStateLevel(r, 1, 5);
        /* fall through */
    case 5:
        r->stateTimer[1]++;
        if (r->stateTimer[1] == 0x3c) {
            zactorShow(r->actor);
            zactorShow(r->bikeObj);
            physRespawn(r);
            riderSetStateLevel(r, 0, kRiderRoll);
            return 0;
        }
        return 1;
    default:
        return 1;
    }
    /* sub states 1 and 2: give up when rolling slowly for long enough */
    if (!(r->speed >= 3.0f)) {
        r->stateTimer[1]++;
        if ((uint16_t)r->stateTimer[1] > 0xf) {
            riderSetStateLevel(r, 1, 3);
            return 0;
        }
    }
    return 1;
}

/* ---- rolling sub states -------------------------------------------------------------------------- */

static int steepEnough(const PhysRider *r)
{
    return zmathAcos(-r->up.y) < kAngleLimit;
}

static int fwdLevelEnough(const PhysRider *r)
{
    return zmathAsin((float)absd((double)r->fwd.y)) < kAngleLimit;
}

/* Whether a hop input starts a hop: pressed, or (at speed) fast enough to leave the ground, or at a
 * standstill. */
static int hopRequested(const PhysRider *r)
{
    if (pressed(r, kInputHop, kStatusPressed) != 0) {
        return 1;
    }
    if (r->speed < 0.0f || r->speed > 0.0f) {
        if (r->padIndex < 2 && (r->flags & 0x400000) == 0 && (r->flags & 8) != 0 &&
            (double)gRiderHopSpeed * (double)r->stat[3] - (double)gRiderHopMargin > (double)r->speed) {
            return 1;
        }
    }
    if (!(r->speed < 0.0f || r->speed > 0.0f)) {
        return pressed(r, 0, 0) != 0;
    }
    return 0;
}

/* Riding: grind, trick, hop and jump inputs, then the crouch / landing sub states 0..2 of level 2. */
BMX_ADDR(0x004339d0)
int riderRollSub0(PhysRider *r)
{
    if (r->speed > 0.0f) {
        if (r->surface != 0 && r->surface < 4 && pressed(r, kInputJump, kStatusPressed) != 0 &&
            physSetState(r, kPhysStateGrind) == kPhysStateGrind) {
            return 0;
        }
        if (pressed(r, kInputTrick, kStatusPressed) != 0) {
            riderSetStateLevel(r, 1, 3);
            return 0;
        }
        if (contrlerAxes(inputState(r), 0)[1] > 0.95f && steepEnough(r)) {
            riderSetStateLevel(r, 1, 2);
            return 0;
        }
    }
    if (fwdLevelEnough(r) && hopRequested(r)) {
        riderSetStateLevel(r, 1, 1);
        r->flags |= 8;
        return 0;
    }
    const int level2 = riderGetStateLevel(r, 2);
    if (level2 == 1) {
        if (fwdLevelEnough(r)) {
            const uint32_t flags = r->flags;
            if ((flags & 0x200) != 0) {
                r->fieldBA4 |= 2;
            }
            r->fieldC64 = 1.0f;
            if (r->rot.y == 0) {
                r->stateTimer[2]++;
                if ((uint16_t)r->stateTimer[2] > 0x384) {
                    r->flags = flags | 4;
                }
                return 1;
            }
            r->stateTimer[2] = 0;
            r->flags = flags & 0xfffffffbu;
            return 1;
        }
    } else if (level2 != 0 && !(level2 == 2 && (r->flags & 0x200) != 0)) {
        return 1;
    } else if (!(r->speed < 0.0f || r->speed > 0.0f) && fwdLevelEnough(r)) {
        r->fieldB9C = 2;
        riderSetStateLevel(r, 2, 1);
        r->fieldBA4 |= 2;
        return 1;
    }
    r->fieldB9C = 5;
    riderSetStateLevel(r, 2, 2);
    r->fieldBA4 |= 2;
    return 1;
}

/* Hop: charges while the crouch lasts and decides between hop, grind-in and a plain landing. */
BMX_ADDR(0x00433d10)
int riderRollSub1(PhysRider *r)
{
    if (!fwdLevelEnough(r)) {
        riderSetStateLevel(r, 1, 0);
        return 0;
    }
    if (pressed(r, kInputTrick, kStatusPressed) != 0) {
        const int16_t timer = r->stateTimer[1];
        riderSetStateLevel(r, 1, 3);
        if (r->padIndex == 1) {
            r->stateTimer[1] = timer;
        }
        return 0;
    }
    if (contrlerAxes(inputState(r), 0)[1] > 0.95f && steepEnough(r)) {
        riderSetStateLevel(r, 1, 2);
        return 0;
    }
    const int level2 = riderGetStateLevel(r, 2);
    if (level2 == 0) {
        r->fieldB9C = 8;
        r->fieldBA4 |= 2;
        riderSetStateLevel(r, 2, 1);
        const double v = (double)r->speed * (double)0.05f;
        r->animSpeed = 1.0f > v ? 1.0f : (float)v;
    } else if (level2 == 1) {
        const double v = (double)r->speed * (double)0.05f;
        r->animSpeed = 1.0f > v ? 1.0f : (float)v;
        if ((r->flags & 0x200) != 0) {
            if (pressed(r, kInputHop, kStatusPressed) == 0 &&
                (r->padIndex >= 2 || (r->flags & 0x400000) != 0 || (r->flags & 8) == 0 ||
                 !((double)gRiderHopSpeed * (double)r->stat[3] - (double)gRiderHopMargin >
                   (double)r->speed))) {
                riderSetStateLevel(r, 1, 0);
                return 0;
            }
            r->fieldB9C = 8;
            r->fieldBA4 |= 2;
        }
    }
    if ((double)gRiderHopSpeed * (double)r->stat[3] > (double)r->speed) {
        r->fieldC60 = (float)((double)gRiderHopBoost * (double)r->stat[3]);
    }
    r->stateTimer[1]++;
    return 1;
}

/* Wheelie / manual entry (stick held forward): sub state 0 enters, 1 holds. */
BMX_ADDR(0x00433f90)
int riderRollSub2(PhysRider *r)
{
    if (contrlerAxes(inputState(r), 0)[1] > 0.95f && steepEnough(r)) {
        const int level2 = riderGetStateLevel(r, 2);
        if (level2 == 0) {
            r->fieldB9C = 10;
            r->fieldBA4 |= 2;
            r->impactTotal = 0.0f;
            riderSetStateLevel(r, 2, 1);
        } else if (level2 == 1) {
            if ((r->flags & 0x200) != 0) {
                r->fieldB9C = 0xb;
                r->fieldBA4 |= 2;
            }
            r->fieldC64 = gRiderLandGrip;
        }
        return 1;
    }
    riderSetStateLevel(r, 1, 0);
    return 0;
}

/* Jump: releasing the trick button launches the rider with a kick that grows with the charge. */
BMX_ADDR(0x004340a0)
int riderRollSub3(PhysRider *r)
{
    if (pressed(r, kInputTrick, kStatusReleased) != 0) {
        ZVec3 v;
        zvec3Zero(&v);
        v.y = (float)-jumpKick(r, stateTimerCapped(r, 1));
        if (r->surface == 1 || r->surface == 3) {
            if (contrlerAxes(inputState(r), 0)[1] < -0.5f &&
                absd((double)contrlerAxes(inputState(r), 0)[0]) < 0.25f) {
                r->flags |= 0x4000;
            }
        }
        physAddVelocity(r, &v);
        physSetState(r, kPhysStateAir);
        riderSetStateLevel(r, 0, kRiderAir);
        r->fieldB9C = 0xf;
        return 0;
    }
    const int level2 = riderGetStateLevel(r, 2);
    switch (level2) {
    case 0:
        r->fieldB9C = 0xd;
        r->fieldBA4 |= 2;
        riderSetStateLevel(r, 2, 1);
        break;
    case 1:
        if ((double)gRiderHopSpeed * (double)r->stat[3] > (double)r->speed && fwdLevelEnough(r)) {
            r->fieldC60 = (float)(((double)gRiderHopBoost * (double)0.5f) * (double)r->stat[3]);
        }
        /* fall through */
    case 2:
        if ((r->flags & 0x200) != 0) {
            r->fieldB9C = 0xe;
            r->fieldBA4 |= 2;
            riderSetStateLevel(r, 2, 2);
        }
        break;
    default:
        break;
    }
    r->stateTimer[1]++;
    return 1;
}

/* Fakie / turn around: sub state 0 enters, 1 waits for the input, 2 flips the bike, 3 launches. */
BMX_ADDR(0x004342f0)
int riderRollSub4(PhysRider *r)
{
    if ((r->flags & 0x40000) == 0 && riderGetStateLevel(r, 2) != 2) {
        riderSetStateLevel(r, 1, 0);
        return 0;
    }
    switch (riderGetStateLevel(r, 2)) {
    case 0:
        r->fieldB9C = 0x11;
        r->fieldBA4 |= 2;
        riderSetStateLevel(r, 2, 1);
        r->stateTimer[2]++;
        return 1;
    case 1:
        if (pressed(r, 9, 0) == 0) {
            if (pressed(r, kInputTrick, kStatusPressed) != 0) {
                riderSetStateLevel(r, 2, 3);
                return 0;
            }
        } else {
            r->fieldB9C = 0x12;
            r->fieldBA4 |= 2;
            riderSetStateLevel(r, 2, 2);
        }
        if ((r->flags & 0x200) != 0) {
            r->fieldBA4 |= 2;
        }
        if (contrlerAxes(inputState(r), 0)[1] > 0.95f && steepEnough(r)) {
            r->stateTimer[2]++;
            r->fieldC64 = gRiderLandGrip;
            return 1;
        }
        r->stateTimer[2]++;
        r->fieldC64 = bitsToFloat(0x3c23d70a);
        return 1;
    case 2:
        if ((r->flags & 0x200) == 0) {
            r->stateTimer[2]++;
            r->rot.y = 0;
            return 1;
        }
        r->flags &= 0xfffbffffu;
        r->fieldBA4 |= 0x80;
        flipBasis(r);
        riderSetStateLevel(r, 1, 0);
        return 0;
    case 3:
        if (pressed(r, kInputTrick, kStatusReleased) != 0) {
            launchRider(r, 2);
            return 0;
        }
        if ((r->flags & 0x200) != 0) {
            r->fieldBA4 |= 2;
        }
        r->stateTimer[2]++;
        return 1;
    default:
        r->stateTimer[2]++;
        return 1;
    }
}

BMX_ADDR(0x004346a0)
int riderRollSub5(PhysRider *r)
{
    return 1;
}

/* Dismount / stop animation. */
BMX_ADDR(0x004346c0)
int riderRollSub6(PhysRider *r)
{
    const int level2 = riderGetStateLevel(r, 2);
    if (level2 == 0) {
        r->spin = 0;
        r->ikFrame = 0;
        r->fieldBA4 |= 2;
        r->anim = 0xff;
        r->ikPose = 0xff;
        r->flags &= 0xfffff7ffu;
        riderSetStateLevel(r, 2, 1);
    } else if (level2 == 1 && (r->flags & 0x200) != 0) {
        r->fieldB9C = 0x18;
        r->fieldBA4 |= 2;
        return 1;
    }
    return 1;
}

/* The ik pose for the stick directions entered so far (the last two decide, else the last one). */
static uint8_t comboPose(const PhysRider *r)
{
    const uint32_t count = r->dirCount;
    if (count == 0) {
        return gRiderTrickCombo[0];
    }
    if (count >= 2) {
        const uint32_t row = ((const uint8_t *)r)[5 + count];
        const uint32_t prev = ((const uint8_t *)r)[4 + count];
        const uint8_t pose = gRiderTrickCombo[3 + row + prev * 8];
        if (pose != 0xff) {
            return pose;
        }
    }
    return gRiderTrickCombo[((const uint8_t *)r)[5 + count]];
}

/* The air trick for the stick directions entered: a signature trick of the rider, else the direction tables.
 */
static void pickAirTrick(PhysRider *r)
{
    const uint8_t *bytes = (const uint8_t *)r;
    const uint32_t count = r->dirCount;
    r->anim = 0xff;
    if (count == 0) {
        return;
    }
    const RiderSignatureTrick *sig = gRiderSignatureTricks[gProfiles[r->player].character];
    for (int k = 0; k < 4; k++) {
        const uint32_t n = sig[k].count;
        if (n == 0 || count < n) {
            continue;
        }
        const uint8_t *dirs = &sig[k].count + 1;
        uint32_t i = 0;
        while (i < n && bytes[6 + count - n + i] == dirs[i]) {
            i++;
        }
        if (i == n) {
            r->anim = sig[k].anim;
            r->ikPose = sig[k].pose;
            break;
        }
    }
    const uint32_t last = bytes[5 + count];
    if (count > 1) {
        if (r->anim != 0xff) {
            return;
        }
        r->anim = gRiderAirTrickPick[bytes[4 + count] * 8 + last];
    }
    if (r->anim == 0xff) {
        r->anim = gRiderAirTrickPick[last];
    }
}

/* Raises the weight of the modifier pose (ikBlend) towards 1 while one is active. */
static void ikBlendIn(PhysRider *r)
{
    if ((r->flags & 0x400) != 0) {
        const double v = (double)r->ikBlend + (double)0x1.99999ap-3f;
        r->ikBlend = !(1.0f >= v) ? 1.0f : (float)v;
    }
}

/* Counts ik pose frames of a held trick without a modifier (ikPose 0). */
static void countHoldFrame(PhysRider *r)
{
    if (r->ikPose == 0 && (r->ikBlend < 0.0f || r->ikBlend > 0.0f) && (r->stateTimer[1] & 0xf) == 0) {
        r->ikFrame++;
    }
}

/*
 * Air sub state 1 (the table slot is shared with the rolling sub states, see gRiderAirStep): the air trick.
 * Level 2: 0 picks the trick from the stick directions, 1 and 2 play it (button 4 adds a modifier pose; a
 * spinning trick turns the bike by orientBRot.z up to its class's limit), 3 and 4 end it.
 */
BMX_ADDR(0x00434760)
int riderRollSub7(PhysRider *r)
{
    if (pressed(r, 3, kStatusReleased) != 0) {
        r->flags &= 0xfffffeffu;
    }
    if (r->ikPose != 0xff && pressed(r, 4, kStatusReleased) != 0) {
        r->flags &= 0xfffffeffu;
    }
    if ((r->flags & 0x800) != 0) {
        /* the bike spins back from the last trick */
        ZAngle *angles[3] = {&r->orientBRot.x, &r->orientBRot.y, &r->orientBRot.z};
        for (int i = 0; i < 3; i++) {
            const int16_t a = *angles[i];
            double t = (double)(int32_t)a * (double)0x1.68p-8f;
            t = t * (double)0x1.333334p-3f;
            t = t * (double)kDegreesToAngle;
            *angles[i] = (ZAngle)(a - bmxFtol(t));
        }
    }
    switch ((uint32_t)riderGetStateLevel(r, 2)) {
    case 0:
        pickAirTrick(r);
        if (r->anim == 0xff) {
            riderLatchDirection(r, 1);
            r->fieldB9C = 0x18;
            riderSetStateLevel(r, 1, 0);
            break;
        }
        r->fieldB9C = (int16_t)gRiderTrickAnim[r->anim];
        r->fieldBA4 = (r->fieldBA4 & 0xfffffff7u) | 2;
        zmatrixCopyRotation(&r->orientB, (const ZMatrix *)(const void *)&r->fwd);
        r->orientBRot.x = 0;
        r->orientBRot.y = 0;
        r->orientBRot.z = 0;
        if ((gRiderTrickFlags[r->anim] & 7) != 0) {
            r->flags |= 0x800;
            if ((r->flags & 0x1000) != 0) {
                r->flags &= 0xffffefffu;
                r->timer = 0;
            }
        }
        if (pressed(r, 3, kStatusPressed) != 0) {
            r->flags |= 0x100;
        } else {
            r->flags &= 0xfffffeffu;
        }
        riderLatchDirection(r, 1);
        riderSetStateLevel(r, 2, 1);
        r->animSpeed = r->stat[6];
        if (r->ikPose != 0xff) {
            zoverLimbsBegin(r->actor, r->ik);
            r->ikBlend = 0.0f;
            r->flags |= 0x400;
        }
        break;
    case 1:
    case 2: {
        if (pressed(r, 4, 0) != 0 || ((r->flags & 0x40) != 0 && r->dirCount != 0)) {
            if (r->dirCount == 0) {
                r->flags |= 0x40;
            } else {
                const uint8_t pose = gRiderAirTrickMod[r->anim * 0x26 + comboPose(r)];
                if (pose != 0xff) {
                    r->ikPose = pose;
                    riderLatchDirection(r, 1);
                    zoverLimbsBegin(r->actor, r->ik);
                    r->flags |= 0x400;
                    r->ikBlend = 0.0f;
                    if (pressed(r, 4, kStatusPressed) != 0) {
                        r->flags |= 0x100;
                    } else {
                        r->flags &= 0xfffffeffu;
                    }
                }
            }
        }
        const uint32_t spinClass = gRiderTrickFlags[r->anim] & 7;
        if (spinClass == 0) {
            const int level2 = riderGetStateLevel(r, 2);
            if (level2 == 1) {
                if ((r->flags & 0x200) != 0) {
                    if ((r->flags & 0x100) == 0 && (gRiderTrickFlags[r->anim] & 8) == 0) {
                        riderSetStateLevel(r, 2, 3);
                        return 0;
                    }
                    r->fieldBA4 |= 2;
                    r->fieldB9C = (int16_t)(gRiderTrickAnim[r->anim] + 1);
                    riderSetStateLevel(r, 2, 2);
                    r->animSpeed = r->stat[6];
                }
            } else if (level2 == 2) {
                countHoldFrame(r);
                if ((r->flags & 0x200) != 0) {
                    if ((gRiderTrickFlags[r->anim] & 8) != 0) {
                        r->flags &= 0xfffffeffu;
                    }
                    if ((r->flags & 0x100) == 0) {
                        riderSetStateLevel(r, 2, 3);
                        return 0;
                    }
                    r->fieldBA4 |= 2;
                }
            }
            ikBlendIn(r);
            break;
        }
        uint32_t maxSpin;
        int16_t target;
        double limit;
        if (spinClass == 4 || spinClass == 5) {
            /* the reverse spins: three quarter speed the other way */
            maxSpin = spinClass == 4 ? 0xeaaau : 0x1eaaau;
            double t = (double)gRiderBikeSpinStep * (double)0.75f;
            t = t * (double)r->stat[2];
            t = -t * (double)kDegreesToAngle;
            target = (int16_t)((int16_t)bmxFtol(t) + r->orientBRot.z);
            limit = (double)gRiderBikeSpinMax * (double)0.75f;
        } else {
            maxSpin = spinClass == 2 ? 0x1d555u : spinClass == 3 ? 0x2d555u : 0xd555u;
            double t = (double)gRiderBikeSpinStep * (double)r->stat[2];
            t = t * (double)kDegreesToAngle;
            target = (int16_t)((int16_t)bmxFtol(t) + r->orientBRot.z);
            limit = (double)gRiderBikeSpinMax;
        }
        limit = limit * (double)r->stat[2];
        const int16_t cap = (int16_t)bmxFtol(limit * (double)kDegreesToAngle);
        const int32_t spin = r->field54;
        const uint32_t magnitude = spin < 0 ? 0u - (uint32_t)spin : (uint32_t)spin;
        if (magnitude >= maxSpin) {
            riderSetStateLevel(r, 2, 3);
            return 0;
        }
        const int smaller = target < cap ? target : cap;
        const int16_t turn = (int16_t)(-(int)cap > smaller ? -(int)cap : smaller);
        r->orientBRot.z = turn;
        r->field54 = (int32_t)((uint32_t)(int32_t)turn + (uint32_t)spin);
        const int level2 = riderGetStateLevel(r, 2);
        if (level2 == 1) {
            if ((r->flags & 0x200) != 0) {
                r->fieldBA4 |= 2;
                r->fieldB9C = (int16_t)(gRiderTrickAnim[r->anim] + 1);
                riderSetStateLevel(r, 2, 2);
            }
        } else if (level2 == 2) {
            countHoldFrame(r);
        }
        ikBlendIn(r);
        break;
    }
    case 3:
        r->fieldB9C = (int16_t)(gRiderTrickAnim[r->anim] + 2);
        riderSetStateLevel(r, 2, 4);
        r->fieldBA4 |= 2;
        ikBlendIn(r);
        break;
    case 4:
        if ((r->flags & 0x200) != 0) {
            riderSetStateLevel(r, 1, 0);
            r->fieldB9C = 0x17;
            return 0;
        }
        if ((r->flags & 0x400) != 0) {
            const double v = (double)r->ikBlend - (double)0x1.333334p-2f;
            r->ikBlend = 0.0f > v ? 0.0f : (float)v;
        }
        break;
    default:
        break;
    }
    if (r->ikBlend < 0.0f || r->ikBlend > 0.0f) {
        r->stateTimer[1]++;
    }
    return 1;
}

/* Ground trick: picks the pose from the entered stick directions and plays it through. */
BMX_ADDR(0x004350f0)
int riderRollSub8(PhysRider *r)
{
    if (pressed(r, 4, kStatusReleased) != 0) {
        r->flags &= 0xfffffeffu;
    }
    switch (riderGetStateLevel(r, 2)) {
    case 0: {
        const uint8_t pose = comboPose(r);
        r->ikPose = pose;
        r->fieldB9C = (int16_t)gRiderPoseAnim[pose];
        r->fieldBA4 = (r->fieldBA4 & 0xfffffff7u) | 2;
        r->ikFrame = 0;
        if (pressed(r, 4, kStatusPressed) != 0) {
            r->flags |= 0x100;
        } else {
            r->flags &= 0xfffffeffu;
        }
        riderLatchDirection(r, 1);
        riderSetStateLevel(r, 2, 1);
        r->animSpeed = r->stat[6];
        return 1;
    }
    case 1:
        if ((r->flags & 0x200) == 0) {
            return 1;
        }
        r->ikFrame++;
        if ((r->flags & 0x100) == 0) {
            r->fieldB9C = (int16_t)(gRiderPoseAnim[r->ikPose] + 2);
            r->fieldBA4 |= 2;
            riderSetStateLevel(r, 2, 3);
            return 1;
        }
        r->fieldBA4 |= 2;
        r->fieldB9C = (int16_t)(gRiderPoseAnim[r->ikPose] + 1);
        riderSetStateLevel(r, 2, 2);
        r->animSpeed = r->stat[6];
        return 1;
    case 2:
        if ((r->flags & 0x200) == 0) {
            return 1;
        }
        if ((gRiderPoseFlags[r->ikPose] & 8) != 0) {
            r->ikFrame++;
        }
        if ((r->flags & 0x100) != 0) {
            r->fieldBA4 |= 2;
            return 1;
        }
        r->fieldB9C = (int16_t)(gRiderPoseAnim[r->ikPose] + 2);
        r->fieldBA4 |= 2;
        riderSetStateLevel(r, 2, 3);
        return 1;
    case 3:
        if ((r->flags & 0x200) != 0) {
            riderSetStateLevel(r, 1, 0);
            r->fieldB9C = 0x17;
            return 0;
        }
        return 1;
    default:
        return 1;
    }
}

/* Plays the grind or lip trick animation `anim` (a table index) and keeps the motion weights in sync. */
static void startTrickAnimation(PhysRider *r, int lipOrGrind)
{
    (void)lipOrGrind;
    const int16_t anim = (int16_t)((uint16_t)r->anim * 3 + 0x6b);
    r->fieldB9C = anim;
    r->motionIndex = (uint16_t)anim;
    r->motionWeight = 1.0f;
    r->fieldBA4 |= 2;
    if ((r->flags & 0x80000) != 0) {
        r->fieldBA4 |= 8;
    }
    r->motionFlags = (uint16_t)r->fieldBA4;
    r->motionFlagsHi = (uint16_t)(r->fieldBA4 >> 16);
    r->animSpeed = 1.0f;
}

/* Steers the bike's pitch (rot.x) during a grind or lip trick from the stick and the travel direction. */
static void pitchBike(PhysRider *r, double stick)
{
    const int16_t turn = (int16_t)bmxFtol((double)gRiderTrickSteer * stick * (double)kDegreesToAngle);
    const double along = zvec3Dot(&r->vel, &r->fwd);
    if (along < 0.0) {
        r->rot.x = (ZAngle)(r->rot.x + turn);
    } else {
        r->rot.x = (ZAngle)(r->rot.x - turn);
    }
    r->motionWeight = 1.0f;
    if (r->fieldBA0 == NULL || r->motionSample == NULL) {
        r->fieldBA4 |= 0x82;
        r->motionFlags |= 0x82;
    }
}

/* Plays the entry animation of a grind or lip: speed set so the move lasts `timer` frames. */
static void startEntryAnimation(PhysRider *r, uint8_t anim)
{
    r->anim = anim;
    const uint32_t f = r->fieldBA4;
    r->fieldBA4 = f | 0x82;
    r->fieldB9C = (int16_t)((uint16_t)anim * 3 + 0x6a);
    if ((r->flags & 0x80000) != 0) {
        r->fieldBA4 = (f | 0x82) | 8;
    }
    r->animSpeed = (float)((double)8.0f / (double)(int32_t)r->timer);
}

/* Grinding: sub state 0 starts the animation, 1 waits for the physics grind, 2 plays, 3 and 4 hold. */
BMX_ADDR(0x004353a0)
int riderGrindSub0(PhysRider *r)
{
    switch (riderGetStateLevel(r, 2)) {
    case 0:
        startEntryAnimation(r, (uint8_t)riderGetTrickA(r));
        riderLatchDirection(r, 0);
        riderSetStateLevel(r, 2, 1);
        r->stateTimer[2]++;
        return 1;
    case 1:
        if (physGetState(r) == kPhysStateGrind) {
            riderSetStateLevel(r, 2, 2);
            return 0;
        }
        break;
    case 2:
        startTrickAnimation(r, 0);
        gameScoreJump(r->player, 1);
        riderSetStateLevel(r, 2, 3);
        r->stateTimer[2]++;
        return 1;
    case 3:
    case 4:
        if (riderGetStateLevel(r, 2) == 3) {
            if (pressed(r, kInputTrick, kStatusPressed) != 0) {
                riderSetStateLevel(r, 2, 4);
                return 0;
            }
        } else if (riderGetStateLevel(r, 2) == 4 && pressed(r, kInputTrick, kStatusReleased) != 0) {
            const double k = jumpKick(r, stateTimerCapped(r, 2));
            ZVec3 v;
            v.x = (float)(k * (double)r->up.x);
            v.y = (float)(k * (double)r->up.y);
            v.z = (float)(k * (double)r->up.z);
            physAddVelocity(r, &v);
            physSetState(r, kPhysStateAir);
            riderSetStateLevel(r, 0, kRiderAir);
            r->fieldB9C = (int16_t)((r->anim + 0x24) * 3);
            r->flags |= 0x8000000;
            return 0;
        }
        pitchBike(r, (double)contrlerAxes(inputState(r), 0)[0]);
        break;
    default:
        break;
    }
    r->stateTimer[2]++;
    return 1;
}

/* Lip stall: like the grind, but the stick pitches the bike along the lip and release leaves it. */
BMX_ADDR(0x00435790)
int riderLipSub0(PhysRider *r)
{
    if (pressed(r, 8, kStatusReleased) != 0 || pressed(r, kInputTrick, kStatusReleased) != 0) {
        r->flags &= 0xfffffeffu;
    }
    switch (riderGetStateLevel(r, 2)) {
    case 0:
        startEntryAnimation(r, r->anim);
        riderLatchDirection(r, 0);
        riderSetStateLevel(r, 2, 1);
        r->flags |= 0x100;
        return 1;
    case 1:
        if (physGetState(r) == kPhysStateStall) {
            riderSetStateLevel(r, 2, 2);
            return 0;
        }
        break;
    case 2:
        startTrickAnimation(r, 1);
        gameScoreJump(r->player, 1);
        riderSetStateLevel(r, 2, 3);
        return 1;
    case 3:
    case 4:
        if (riderGetStateLevel(r, 2) == 3) {
            r->stateTimer[2]++;
            if (r->stateTimer[2] == 5) {
                riderSetStateLevel(r, 2, 4);
            }
        } else if (riderGetStateLevel(r, 2) == 4 && (r->flags & 0x100) == 0) {
            physSetState(r, kPhysStateAir);
            riderSetStateLevel(r, 0, kRiderAir);
            r->fieldB9C = (int16_t)((r->anim + 0x24) * 3);
            r->flags |= 0x8000000;
            return 0;
        }
        if (r->anim >= 0x27 && r->anim <= 0x29) {
            pitchBike(r, -(double)contrlerAxes(inputState(r), 0)[1]);
        } else {
            pitchBike(r, (double)contrlerAxes(inputState(r), 0)[0]);
        }
        break;
    default:
        break;
    }
    return 1;
}

/* Manual: the stick leans the bike (rot.z), a landing wheel ends it, jump releases into the air. */
BMX_ADDR(0x00435af0)
int riderManualSub0(PhysRider *r)
{
    if ((uint32_t)riderGetStateLevel(r, 2) < 2) {
        r->rot.z = (ZAngle)bmxFtol((double)contrlerAxes(inputState(r), 0)[1] * (double)2.5f *
                                   (double)kDegreesToAngle);
    }
    if ((uint32_t)riderGetStateLevel(r, 2) < 3) {
        for (uint32_t i = 0; i < 2; i++) {
            if (r->wheel[i].mode != 2) {
                if (r->wheel[i].speed < 1.0f) {
                    riderSetStateLevel(r, 2, 3);
                }
                break;
            }
        }
    }
    switch (riderGetStateLevel(r, 2)) {
    case 0:
        r->fieldB9C = (int16_t)((r->fieldB9C == 0xf) + 0x13);
        r->fieldBA4 |= 2;
        riderSetStateLevel(r, 2, 1);
        break;
    case 1:
        if (pressed(r, kInputTrick, kStatusPressed) != 0) {
            riderSetStateLevel(r, 2, 2);
        }
        /* fall through */
    case 2:
        if (pressed(r, kInputTrick, kStatusReleased) != 0) {
            launchRider(r, 2);
            return 0;
        }
        if ((r->flags & 0x200) != 0) {
            r->fieldB9C = 0x14;
            r->fieldBA4 |= 2;
        }
        r->anim = (uint8_t)((r->wheel[0].mode != 2) + 0x2c);
        break;
    case 3:
        r->fieldB9C = 0x15;
        r->fieldBA4 |= 2;
        r->flags &= 0xffcfffffu;
        riderSetStateLevel(r, 2, 4);
        break;
    default:
        break;
    }
    r->stateTimer[1]++;
    r->stateTimer[2]++;
    r->fieldC64 = bitsToFloat(0x3b03126f);
    return 1;
}

/* ---- stick directions and trick lookups ---------------------------------------------------------- */

/* The stick bits (1 up, 2 down, 4 left, 8 right) of the pad snapshot plus half-way stick deflections. */
static uint32_t stickBits(const PhysRider *r)
{
    uint32_t bits = r->inputBits;
    if (absd((double)r->stick[0]) > (double)0.5f) {
        bits |= r->stick[0] < 0.0f ? 4 : 8;
    }
    if (absd((double)r->stick[1]) > (double)0.5f) {
        bits |= r->stick[1] < 0.0f ? 1 : 2;
    }
    return bits;
}

/*
 * Starts (latch != 0) or ends the collection of stick directions for a trick: dirList holds the
 * directions entered so far. The two bits of 0x60 in flags are cleared in both cases.
 */
BMX_ADDR(0x00435db0)
void riderLatchDirection(PhysRider *r, int latch)
{
    memset(r->dirList, 0, sizeof r->dirList);
    r->dirCount = 0;
    if (latch != 0) {
        if ((r->flags & 0x10) == 0) {
            const uint32_t bits = stickBits(r) & 0xf;
            if (bits != 0) {
                r->dirList[0] = gRiderStickDir[bits];
                r->dirCount++;
            }
        }
        r->flags = (r->flags | 0x10) & 0xffffff9fu;
        return;
    }
    r->flags = (r->flags & 0xffffffefu) & 0xffffff9fu;
}

/* The trick direction code of the stick as it is now (a row of the direction tables). */
BMX_ADDR(0x00435f90)
uint32_t riderStickDirection(const PhysRider *r)
{
    return gRiderStickDir[stickBits(r) & 0xf];
}

BMX_ADDR(0x00435ea0)
uint32_t riderGetTrickA(const PhysRider *r)
{
    if (r->dirCount != 0) {
        return gRiderTrickA[((const uint8_t *)r)[5 + r->dirCount]];
    }
    return gRiderTrickA[riderStickDirection(r)];
}

BMX_ADDR(0x00435ef0)
uint32_t riderGetTrickB(const PhysRider *r)
{
    if (r->dirCount != 0) {
        return gRiderTrickB[((const uint8_t *)r)[5 + r->dirCount]];
    }
    return gRiderTrickB[riderStickDirection(r)];
}

BMX_ADDR(0x00435f40)
uint32_t riderGetTrickKind(const PhysRider *r)
{
    if (r->dirCount != 0) {
        return gRiderTrickKind[((const uint8_t *)r)[5 + r->dirCount]];
    }
    return gRiderTrickKind[riderStickDirection(r)];
}

/* ---- collisions ---------------------------------------------------------------------------------- */

/*
 * Hit callback of the rider's motion object: finds the rider, plays the impact sounds and rumble,
 * throws sparks on hard hits and adds the impact to the score.
 */
BMX_ADDR(0x00436020)
void riderCollide(struct ZRig *rig, uint32_t index, float impact, struct ZRigHit *hit)
{
    if (!(impact > 0.0f)) {
        return;
    }
    PhysRider *rider = NULL;
    const uint32_t count = gPlayerCount < 1 ? 1 : gPlayerCount;
    for (uint32_t i = 0; i < count; i++) {
        if (gPhysRiders[i].actor2 == (void *)rig) {
            rider = &gPhysRiders[i];
            break;
        }
    }
    if (rider == NULL) {
        zdebugFail("Couldn't associate rider with dynamic collision structure", riderFile, 0xa85, 0);
    }
    uint32_t kind = 0;
    int solid = 0;
    if ((hit->kind & 1) != 0) {
        if (hit->matRef != NULL) {
            kind = *hit->matRef & 0xf;
        }
    } else if ((hit->kind & 2) != 0) {
        const uint16_t id = hit->tag;
        const uint8_t *info = (const uint8_t *)zbspGetShape(id);
        if (ngifzCollideGetObject(id) != NULL) {
            kind = info[2];
        }
        const PropRecord *prop = physObjectProp(hit->object);
        if (prop != NULL) {
            const ZVec3 local = hit->normal;
            ZVec3 *slot = &rig->points[index].vel;
            (void)zvec3Dot(slot, &local);
            (void)zvec3Dot(&prop->vel, &local);
            zvec3Add(slot, slot, &prop->vel);
        }
    }
    switch (kind) {
    case 5:
    case 9:
    case 10:
        solid = 1;
        break;
    case 6:
        if ((*hit->matRef >> 0x13 & 3) == 2) {
            if (index == 0) {
                rider->surface = 0xe;
            }
        } else if (index == 0) {
            rider->surface = 0xd;
        }
        break;
    default:
        break;
    }
    if (impact > 5.0f) {
        uint32_t level;
        if (impact < 10.0f) {
            level = 0;
        } else if (impact < 25.0f) {
            level = 1;
        } else {
            level = impact < 50.0f ? 2 : 3;
        }
        const void *position = &hit->point;
        int source = -1;
        uint32_t minimum = 0;
        switch (index) {
        case 1:
        case 2:
            source = 1;
            minimum = 1;
            break;
        case 5:
        case 6:
            source = 0;
            minimum = 2;
            break;
        case 0xb:
        case 0xc:
            source = 2;
            minimum = 2;
            break;
        case 0xd:
            source = 3;
            minimum = 1;
            break;
        default:
            break;
        }
        if (source >= 0) {
            bmxSoundPlayAt(0xbbd, position, source, (int)level, 0);
            if (rider->soundCooldown == 0 && level >= minimum) {
                bmxSoundPlayAt(0xbbe, position, source, (int)level, 0);
                rider->soundCooldown = 0x2d;
            }
        }
        int rumble = bmxFtol((double)impact * (double)8.0f);
        if (rumble >= 0xff) {
            rumble = 0xff;
        }
        contrlerRumble(rider->player, 1, rumble);
        if (impact > 20.0f) {
            (void)zactorGetPos(rider->actor);
            ZVec3 zero;
            zvec3Zero(&zero);
            if (solid != 0) {
                fxImpactSparks((const ZVec3 *)position, &zero);
            }
            if (impact > 30.0f) {
                fxBurst();
            }
        }
    }
    riderAddScore(rider, index, impact);
}
