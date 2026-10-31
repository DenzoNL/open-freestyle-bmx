/* Actors and their skeletal motion playback (C:\zaxis\Bmx\src\com\zge\zactor.c). */
#include "bmx/com/zge/zactor.h"

#include "compat/crt.h"
#include "zge/zasset.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zstring.h"

static const char zactorFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\zactor.c";

#define kActorRadiusPerson 220.0f
#define kActorRadiusProp   80.0f
#define kActorRadiusEffect 50.0f

/* Scratch results shared between the motion routines (one global each in the exe). */
#define gMotionDeltaA      ORIG_VAR(0x00601d68, ZMotionDelta, gMotionDeltaA)
#define gMotionDeltaB      ORIG_VAR(0x00601d80, ZMotionDelta, gMotionDeltaB)
#define gMotionDeltaC      ORIG_VAR(0x00601d98, ZMotionDelta, gMotionDeltaC)
#define gMotionDeltaD      ORIG_VAR(0x00601db0, ZMotionDelta, gMotionDeltaD)
#define gMotionDeltaE      ORIG_VAR(0x00601de0, ZMotionDelta, gMotionDeltaE)
#define gMotionLastDelta   ORIG_VAR(0x00601d48, ZMotionDelta, gMotionLastDelta)
#define gMotionPrevYawE    ORIG_VAR(0x00601e08, int16_t, gMotionPrevYawE)
#define gMotionSampleB     ORIG_VAR(0x00601e18, ZMotionDelta *, gMotionSampleB)
#define gActorRootPoseTemp ORIG_VAR(0x00601d30, ZActorRootPose, gActorRootPoseTemp)
#define gMotionEntryTemp   ORIG_VAR(0x00601d60, ZMotionEntry, gMotionEntryTemp)

/* Joint index remap applied to mirrored motions: joint j of a mirrored key is stored at map[j]. */
typedef uint16_t ZMirrorMap[19];
#define gMotionMirrorMap ORIG_VAR(0x0055f364, ZMirrorMap, gMotionMirrorMap)
/* Skeleton tables, rows of (skeleton * 19 + bone): parent chain (0xff terminated, 8 bytes per row) and bone
 * offsets. */
typedef int8_t ZSkeletonChain[1];
typedef ZVec3 ZSkeletonOffsets[1];
#define gSkeletonChain   ORIG_VAR(0x0055f390, ZSkeletonChain, gSkeletonChain)
#define gSkeletonOffsets ORIG_VAR(0x0055f4c0, ZSkeletonOffsets, gSkeletonOffsets)

/* x87 FCOM followed by TEST AH,0x40 is true for "equal" and for "unordered". */
static int zfpEqual(float a, float b)
{
    return !(a < b || a > b);
}

BMX_ADDR(0x00472630)
void zactorTableInit(void)
{
    gZactors = zmemStaticAlloc((uint32_t)gZactorMax * sizeof(ZActor));
    zmemFill(gZactors, 0, (uint32_t)gZactorMax * sizeof(ZActor));
    for (int i = 0; i < gZactorMax; i++) {
        gZactors[i].type = 0;
    }
    gMotionBankCount = 0;
}

BMX_ADDR(0x004726c0)
void zactorNop0(void) {}

/* Copies a prop actor over another actor slot. */
BMX_ADDR(0x004726e0)
void zactorCopy(ZActor *dst, const ZActor *src)
{
    if (src->type != kZactorTypeProp) {
        zdebugFail("Only coded for prop types", zactorFile, 0xce, 0);
    }
    zmemCopy(dst, src, sizeof(ZActor));
}

/* First free slot (type 0) of the actor table, or NULL. */
BMX_ADDR(0x00472730)
ZActor *zactorFindFree(void)
{
    ZActor *a = gZactors;
    ZActor *const end = gZactors + gZactorMax;
    for (; a < end; a++) {
        if (a->type == 0) {
            return a;
        }
    }
    gZdebugPrint("ZACTOR: Error - failed to allocate actor\n");
    return NULL;
}

/* Allocates a slot and initialises it. */
BMX_ADDR(0x00472780)
ZActor *zactorCreate(int type)
{
    ZActor *a = zactorFindFree();
    if (a == NULL) {
        return NULL;
    }
    return zactorInit(type, a);
}

/* Resets a slot to a fresh actor: unit scale, neutral colour, and a motion player for persons. */
BMX_ADDR(0x004727b0)
ZActor *zactorInit(int type, ZActor *a)
{
    zmemFill(a, 0, sizeof(ZActor));
    a->type = type;
    a->skeleton = 0;
    a->flags = kZactorVisible;
    a->scale.x = 1.0f;
    a->scale.y = 1.0f;
    a->scale.z = 1.0f;
    a->field4c = 0;
    a->color = 0x808080;
    a->field5c = 0;
    a->field64 = 0;
    a->field18e = 2;
    if (type == kZactorTypePerson) {
        a->player = zmemStaticAlloc(sizeof(ZMotionPlayer));
        zmemFill(a->player, 0, sizeof(ZMotionPlayer));
        a->player->ch[0].field176 = 0x72;
        a->player->ch[0].jointCount = 0x13;
        a->player->ch[0].extra = NULL;
        a->flags |= kZactorPerson;
        a->radius = kActorRadiusPerson;
    } else if (type == kZactorTypeProp) {
        a->flags = 0x51;
        a->radius = kActorRadiusProp;
    } else if (type == kZactorTypeEffect) {
        a->radius = kActorRadiusEffect;
    }
    return a;
}

/* Frees a slot. The motion player is not released (static memory). */
BMX_ADDR(0x004728c0)
void zactorDestroy(ZActor *a)
{
    zactorClear(a);
}

BMX_ADDR(0x004728e0)
void zactorClear(ZActor *a)
{
    a->flags = 0;
    a->type = 0;
}

BMX_ADDR(0x00472910)
void zactorSetPos(ZActor *a, const ZVec3 *pos)
{
    a->pos = *pos;
}

BMX_ADDR(0x00472940)
ZVec3 *zactorGetPos(ZActor *a)
{
    return &a->pos;
}

/* Sets heading and y rotation from a motion-space angle. */
BMX_ADDR(0x00472960)
void zactorSetHeading(ZActor *a, int angle)
{
    const ZAngle world = (ZAngle)zactorAngleToWorld(angle);
    a->heading = world;
    a->rot[1] = world;
}

BMX_ADDR(0x00472990)
ZAngle zactorGetHeading(const ZActor *a)
{
    return (ZAngle)zactorAngleFromWorld((uint16_t)a->heading);
}

/* Motion-space angle to world angle (the original reads a stale EAX for the upper half: zero here). */
BMX_ADDR(0x004729c0)
int zactorAngleToWorld(int angle)
{
    if (gActorAngleFlip != 0) {
        return gActorAngleBase - angle;
    }
    return gActorAngleBase + angle;
}

BMX_ADDR(0x00472a00)
int zactorAngleFromWorld(int angle)
{
    angle -= gActorAngleBase;
    if (gActorAngleFlip != 0) {
        angle = -angle;
    }
    return angle;
}

/* Persons and props note whether the scale is non-trivial; objects (type 8) just store it. */
BMX_ADDR(0x00472a30)
void zactorSetScale(ZActor *a, const ZVec3 *scale)
{
    const ZVec3 s = *scale;
    if (a->type == kZactorTypePerson || a->type == kZactorTypeProp) {
        if (zfpEqual(s.x, 1.0f) && zfpEqual(s.y, 1.0f) && zfpEqual(s.z, 1.0f)) {
            a->flags &= ~(uint32_t)kZactorScaled;
        } else {
            a->flags |= kZactorScaled;
        }
    } else if (a->type != kZactorTypeObject) {
        return;
    }
    a->scale = s;
}

BMX_ADDR(0x00472af0)
ZVec3 *zactorGetScale(ZActor *a)
{
    return &a->scale;
}

/* Component-wise (a + b) * 0.5. */
BMX_ADDR(0x00474d20)
void zvec3Average(const ZVec3 *a, const ZVec3 *b, ZVec3 *out)
{
    out->x = (float)(((double)a->x + b->x) * 0.5f);
    out->y = (float)(((double)a->y + b->y) * 0.5f);
    out->z = (float)(((double)a->z + b->z) * 0.5f);
}

/* Component-wise a * 0.5 + b * 1.5. */
BMX_ADDR(0x00474d70)
void zvec3Blend(const ZVec3 *a, const ZVec3 *b, ZVec3 *out)
{
    out->x = (float)((double)a->x * 0.5f + (double)b->x * 1.5f);
    out->y = (float)((double)a->y * 0.5f + (double)b->y * 1.5f);
    out->z = (float)((double)a->z * 0.5f + (double)b->z * 1.5f);
}

/* True when the vector is exactly zero (NaN counts as zero, like the x87 compare). */
BMX_ADDR(0x00474070)
int zvec3IsZero(const ZVec3 *v)
{
    return zfpEqual(v->x, 0.0f) && zfpEqual(v->y, 0.0f) && zfpEqual(v->z, 0.0f);
}

/* Copies joints 1 to 9 (the secondary chains) of a's channel 6 (the mirror companion of sampled motion
 * 2) into the blended result of b. */
BMX_ADDR(0x00474960)
void zactorCopySecondaryJoints(const ZActor *a, const ZActor *b)
{
    zmemCopy(&b->player->ch[0].joints[1], &a->player->ch[6].joints[1], 9 * sizeof(ZAngle3));
}

/* One angle of zmotionAverageAngles. A difference of 0x8000 or more is a wrap: the sum is then unsigned. */
static ZAngle zmotionAverageAngle(ZAngle a, ZAngle b)
{
    const int32_t sa = a;
    const int32_t sb = b;
    const int32_t d = sa - sb;
    if ((d < 0 ? -d : d) < 0x8000) {
        return (ZAngle)((sb + sa) >> 1);
    }
    return (ZAngle)(((uint32_t)(uint16_t)b + (uint32_t)(uint16_t)a) >> 1);
}

/* Component-wise average of `count` joint triples. */
BMX_ADDR(0x004749a0)
void zmotionAverageAngles(const ZAngle3 *a, const ZAngle3 *b, ZAngle3 *out, int count)
{
    do {
        out->x = zmotionAverageAngle(a->x, b->x);
        out->y = zmotionAverageAngle(a->y, b->y);
        out->z = zmotionAverageAngle(a->z, b->z);
        a++;
        b++;
        out++;
    } while (--count != 0);
}

/* 1/4 a + 3/4 b in integers, with the same wrap rule as the average. */
static ZAngle zmotionBlendAngle(ZAngle a, ZAngle b)
{
    const int32_t sa = a;
    const int32_t sb = b;
    const int32_t d = sa - sb;
    if ((d < 0 ? -d : d) < 0x8000) {
        return (ZAngle)((sa + sb * 3) >> 2);
    }
    return (ZAngle)(((uint32_t)(uint16_t)b * 3 + (uint32_t)(uint16_t)a) >> 2);
}

/*
 * Blends `count` joint triples as 1/4 a + 3/4 b. The first angle is treated as degrees on a circle and
 * takes the short way round; the other two use integer arithmetic with the same wrap rule as the average.
 */
BMX_ADDR(0x00474b10)
void zmotionBlendAngles(const ZAngle3 *a, const ZAngle3 *b, ZAngle3 *out, int count)
{
    do {
        double fa = (double)a->x * 0x1.68p-8f; /* 360 / 0x10000 */
        double fb = (double)b->x * 0x1.68p-8f;
        double d = fa - fb;
        if (d < 0.0) {
            d = -d;
        }
        double r;
        /* The original compares against -180.0, so this branch is never taken. */
        if (d < -180.0f) {
            r = fa * 0.25f + fb * 0.75f;
        } else {
            if (fb < fa) {
                fb += 360.0f;
            } else {
                fa += 360.0f;
            }
            r = fa * 0.25f + fb * 0.75f;
            while (r > 360.0f) {
                r -= 360.0f;
            }
            while (r < -360.0f) {
                r += 360.0f;
            }
        }
        out->x = (ZAngle)(int)(r * 0x1.6c16c2p+7f); /* 0x10000 / 360 */

        out->y = zmotionBlendAngle(a->y, b->y);
        out->z = zmotionBlendAngle(a->z, b->z);
        a++;
        b++;
        out++;
    } while (--count != 0);
}

/* Root bone pose of the current motion, in the actor's world orientation. NULL unless the player is in
 * mode 2. */
BMX_ADDR(0x00474de0)
ZActorRootPose *zactorGetRootPose(const ZActor *a)
{
    const ZMotionChannel *ch = &a->player->ch[0];
    if (ch->boneCount != 2) {
        return NULL;
    }
    gActorRootPoseTemp.pos.x = ch->rootAcc.x;
    gActorRootPoseTemp.pos.y = ch->rootAcc.y;
    gActorRootPoseTemp.pos.z = ch->rootAcc.z;
    gActorRootPoseTemp.pad16 = 0;
    zvec3RotateY(&gActorRootPoseTemp.pos, &gActorRootPoseTemp.pos,
                 gActorAngleFlip == 0 ? (ZAngle)gActorAngleBase : (ZAngle)-gActorAngleBase);
    if (a->flags & kZactorScaled) {
        gActorRootPoseTemp.pos.x = (float)((double)gActorRootPoseTemp.pos.x * a->scale.x);
        gActorRootPoseTemp.pos.y = (float)((double)gActorRootPoseTemp.pos.y * a->scale.y);
        gActorRootPoseTemp.pos.z = (float)((double)gActorRootPoseTemp.pos.z * a->scale.z);
    }
    gActorRootPoseTemp.rot.x = ch->rootAngle.x;
    gActorRootPoseTemp.rot.y = ch->rootAngle.y;
    gActorRootPoseTemp.rot.z = ch->rootAngle.z;
    return &gActorRootPoseTemp;
}

/* Opens the motion library and loads motion bank 0. Returns 0 on success, 1 if the file is missing. */
BMX_ADDR(0x00474f10)
int zmotionOpenLibrary(const char *name)
{
    int i = 0;
    if (gMotionLibrary != 0) {
        zdebugFail("Motion library already open", zactorFile, 0x957, 0);
    }
    gMotionLibrary = zassetOpenLibrary(name);
    if (gMotionLibrary == 0) {
        return 1;
    }
    int found = zassetFind(gMotionLibrary, 0, 0);
    while (found == 1) {
        if (zassetSize(gMotionLibrary) > 0x40000) {
            zdebugFail("Motions are too large.", zactorFile, 0x962, 0);
        }
        i++;
        found = zassetFind(gMotionLibrary, i, 0);
    }
    gMotionBuffer = gMotionStaging;
    zmotionLoadBank(0);
    return 0;
}

BMX_ADDR(0x00475000)
void zmotionCloseLibrary(void)
{
    if (gMotionLibrary == 0) {
        zdebugFail("Motions not opened", zactorFile, 0x982, 0);
    }
    zassetCloseLibrary(gMotionLibrary);
    gMotionLibrary = 0;
}

/* Loads motion bank `id` into the staging buffer and registers it as bank 0. */
BMX_ADDR(0x00475050)
ZMotionBank *zmotionLoadBank(uint32_t id)
{
    id &= 0xffff;
    if (id == gMotionLoadedBank) {
        gZdebugPrint("Not reloading motion library\n");
        return &gMotionBanks[0];
    }
    if (zassetFind(gMotionLibrary, (int)id, 0) == 0) {
        zdebugFail("Could not find motion bank", zactorFile, 0x9a0, 0);
        return NULL;
    }
    const uint32_t size = zassetSize(gMotionLibrary);
    if (size > 0x40000) {
        zdebugFail("Too big", zactorFile, 0x9a6, 0);
    }
    zassetRead(gMotionLibrary, gMotionBuffer);
    gMotionLoadedBank = id;
    return zmotionBankAdd(gMotionBuffer, size, 1);
}

/* Loads another bank into its own static allocation. */
BMX_ADDR(0x00475140)
void zmotionLoadExtraBank(uint16_t id)
{
    if (gMotionLibrary == 0) {
        zdebugFail("Motion bank not opened", zactorFile, 0x9bf, 0);
    }
    if (zassetFind(gMotionLibrary, id, 0) == 0) {
        return;
    }
    const uint32_t size = zassetSize(gMotionLibrary);
    void *buf = zmemStaticAlloc(size);
    if (buf == NULL) {
        return;
    }
    zassetRead(gMotionLibrary, buf);
    zmotionBankAdd(buf, size, 0);
}

/*
 * Registers a loaded library. With `replaceFirst` (and a bank already present) bank 0 is overwritten,
 * otherwise a new bank is appended. Libraries start with an optional 0x10 byte header (magic 0x2a81511c)
 * or an 8 byte one (first byte 0xff).
 */
BMX_ADDR(0x00475200)
ZMotionBank *zmotionBankAdd(uint8_t *data, uint32_t size, int replaceFirst)
{
    ZMotionBank *banks = gMotionBanks;
    const int32_t count = gMotionBankCount;
    ZMotionBank *bank;
    if (count == 0) {
        gMotionFirstBank = data;
    }
    if (replaceFirst == 0 || count == 0) {
        bank = banks + count;
        gMotionBankCount = count + 1;
    } else {
        bank = banks;
    }
    bank->base = data;
    bank->size = size;
    if (*(uint32_t *)data == kZmotionBankMagic) {
        /* Banks with a 0x10 byte header: the table follows it, entries start behind the table's two
         * 16 bit words. */
        bank->base += kZmotionBankHeaderSize;
        bank->cursor = (const uint32_t *)(data + kZmotionBankHeaderSize + 4);
        bank->table = (const uint16_t *)(data + kZmotionBankHeaderSize);
        bank->remaining = *(const uint16_t *)(data + kZmotionBankHeaderSize + 2);
        return bank;
    }
    if (data[0] == 0xff) {
        data += 8;
    }
    bank->table = (const uint16_t *)data;
    bank->cursor = (const uint32_t *)(data + 4);
    bank->remaining = *(const uint16_t *)(data + 2);
    return bank;
}

BMX_ADDR(0x004752c0)
void zmotionNop(void) {}

BMX_ADDR(0x004752e0)
ZMotionBank *zmotionBankGet(int index)
{
    ZMotionBank *banks = gMotionBanks;
    return banks + index;
}

/* Finds motion `id` in the loaded banks. */
BMX_ADDR(0x00475300)
const ZMotion *zmotionFind(int id)
{
    ZMotionBank *bank = gMotionBanks;
    for (int b = 0; b < gMotionBankCount; b++, bank++) {
        const uint16_t *hdr = bank->table;
        const int n = hdr[1];
        const uint8_t *entry = (const uint8_t *)(hdr + 2);
        for (int i = 0; i < n; i++) {
            if (*(const int32_t *)entry == id) {
                return (const ZMotion *)(bank->base + *(const int32_t *)(entry + 4));
            }
            entry += 4 + hdr[0] * 8;
        }
    }
    return NULL;
}

/* Iterates the entries of a bank; NULL at the end. The result lives in a global. */
BMX_ADDR(0x004753c0)
ZMotionEntry *zmotionBankNext(ZMotionBank *bank)
{
    gMotionEntryTemp.id = 0;
    gMotionEntryTemp.motion = NULL;
    if (bank->remaining == 0) {
        return NULL;
    }
    bank->remaining--;
    const uint32_t *cur = bank->cursor;
    gMotionEntryTemp.id = cur[0];
    gMotionEntryTemp.motion = (const ZMotion *)(bank->base + cur[1]);
    bank->cursor = cur + (uint32_t)bank->table[0] * 2 + 1;
    return &gMotionEntryTemp;
}

BMX_ADDR(0x00475430)
void zactorSetModel(ZActor *a, uint16_t model)
{
    a->model = model;
    *(uint16_t *)(a->lod) = model;
}

BMX_ADDR(0x00475460)
void zactorSetLodModel(ZActor *a, uint16_t lod, uint16_t model)
{
    if (lod > 3) {
        zdebugFail("Exeeded maximum LODs for actor", zactorFile, 0xa76, 0);
    }
    *(uint16_t *)(a->lod + (uint32_t)lod * 0x10) = model;
    if (lod == 0) {
        a->model = model;
    }
}

BMX_ADDR(0x004754d0)
uint16_t zactorGetModel(const ZActor *a)
{
    return a->model;
}

/* Number of markers stored with the motion (0 when it has none). */
BMX_ADDR(0x004754f0)
uint8_t zmotionMarkerCount(const ZMotion *m)
{
    if (m->markerOffset == 0) {
        return 0;
    }
    return ((const ZMotionMarkerBlock *)((const uint8_t *)m + m->markerOffset))->count;
}

static ZAngle zmotionMarkerRotation(void)
{
    return gActorAngleFlip == 0 ? (ZAngle)gActorAngleBase : (ZAngle)-gActorAngleBase;
}

/*
 * Reads every marker of a motion (positions in 1/8 units, weights in 1/1024) rotated into world space.
 * Returns the marker count (low byte).
 */
BMX_ADDR(0x00475520)
uint8_t zmotionGetMarkers(const ZMotion *m, ZMotionMarker *out)
{
    if (m->markerOffset == 0) {
        return 0;
    }
    const ZMotionMarkerBlock *block = (const ZMotionMarkerBlock *)((const uint8_t *)m + m->markerOffset);
    const int16_t *src = block->points;
    const int wide = (m->format & 0x70) != 0; /* entries carry an extra angle */
    int i = 0;
    if (block->count16 != 0) {
        do {
            out->pos.x = (float)((double)src[1] * 0x1p-3f);
            out->pos.y = (float)((double)src[2] * 0x1p-3f);
            out->pos.z = (float)((double)src[3] * 0x1p-3f);
            out->weight = (float)((double)src[0] * 0x1p-10f);
            out->angle = wide ? src[4] : 0;
            zvec3RotateY(&out->pos, &out->pos, zmotionMarkerRotation());
            src += wide ? 5 : 4;
            i++;
            out++;
        } while (i < block->count16);
    }
    return (uint8_t)block->count16;
}

/* Like zmotionGetMarkers for the first marker only. */
BMX_ADDR(0x00475730)
uint8_t zmotionGetFirstMarker(const ZMotion *m, ZMotionMarker *out)
{
    if (m->markerOffset == 0) {
        return 0;
    }
    const ZMotionMarkerBlock *block = (const ZMotionMarkerBlock *)((const uint8_t *)m + m->markerOffset);
    const int16_t *src = block->points;
    const int wide = (m->format & 0x70) != 0;
    out->pos.x = (float)((double)src[1] * 0x1p-3f);
    out->pos.y = (float)((double)src[2] * 0x1p-3f);
    out->pos.z = (float)((double)src[3] * 0x1p-3f);
    out->weight = (float)((double)src[0] * 0x1p-10f);
    out->angle = wide ? src[4] : 0;
    zvec3RotateY(&out->pos, &out->pos, zmotionMarkerRotation());
    return (uint8_t)block->count16;
}

BMX_ADDR(0x004758f0)
void zactorSetField172(ZActor *a, uint8_t v)
{
    a->field172 = v;
}

BMX_ADDR(0x00475920)
void zactorSetRadius(ZActor *a, float radius)
{
    a->radius = radius;
}

/* Stores the extents and sets the radius to the largest of the three. */
BMX_ADDR(0x00475940)
void zactorSetExtent(ZActor *a, const ZVec3 *extent)
{
    a->extent[0] = extent->x;
    a->extent[1] = extent->y;
    a->extent[2] = extent->z;
    const float m = (a->extent[1] >= a->extent[0]) ? a->extent[1] : a->extent[0];
    zactorSetRadius(a, (m > a->extent[2]) ? m : a->extent[2]);
}

/* Angle base and flip for motion-space conversions; only the low half of the base is written. */
BMX_ADDR(0x004759d0)
void zactorSetAngleBase(uint16_t base, int flip)
{
    *(uint16_t *)&gActorAngleBase = base;
    gActorAngleFlip = flip;
}

BMX_ADDR(0x00475a00)
int zactorOffsetAngle(int a, int b)
{
    if (gActorAngleFlip != 0) {
        return a - b;
    }
    return a + b;
}

BMX_ADDR(0x00475a30)
void zactorShow(ZActor *a)
{
    a->flags |= kZactorVisible;
    zobjSetVisible(a->object, 1);
}

BMX_ADDR(0x00475a60)
void zactorHide(ZActor *a)
{
    a->flags &= ~(uint32_t)kZactorVisible;
    zobjSetVisible(a->object, 0);
}

BMX_ADDR(0x00475a90)
void zactorSetFlag2(ZActor *a)
{
    a->flags |= kZactorFlag2;
}

BMX_ADDR(0x00475ab0)
void zactorClearFlag2(ZActor *a)
{
    a->flags &= ~(uint32_t)kZactorFlag2;
}

BMX_ADDR(0x00475ad0)
void zactorSetAttachment(ZActor *a, const ZVec3 *offset, int bone)
{
    a->attachBone = bone;
    a->attachOffset = *offset;
}

/* Tag 0 is the only one known; it sets flag 0x800. */
BMX_ADDR(0x00475b10)
void zactorSetTag(ZActor *a, int tag)
{
    if (tag == 0) {
        a->flags |= kZactorFlag800;
        return;
    }
    gZdebugPrint("ZACTOR: Unknown tag identifier \n");
}

BMX_ADDR(0x00475b50)
void zactorClearTag(ZActor *a, int tag)
{
    if (tag == 0) {
        a->flags &= ~(uint32_t)kZactorFlag800;
        return;
    }
    gZdebugPrint("ZACTOR: Unknown tag identifier \n");
}

BMX_ADDR(0x00475b90)
void zactorSetFlag20(ZActor *a)
{
    a->flags |= kZactorFlag20;
}

BMX_ADDR(0x00475bb0)
void zactorClearFlag20(ZActor *a)
{
    a->flags &= ~(uint32_t)kZactorFlag20;
}

BMX_ADDR(0x00475bd0)
void zactorNop1(void) {}

/* Motion playback rate: gMotionTimeScale = 1024 / rate. */
BMX_ADDR(0x00475bf0)
void zmotionSetRate(int16_t rate)
{
    gMotionTimeScale = (float)(1024.0f / (double)(int)rate);
}

BMX_ADDR(0x00475c20)
void zactorNop2(void) {}

BMX_ADDR(0x00475c40)
void zactorSetField174(ZActor *a, int v)
{
    a->field174 = v;
}

BMX_ADDR(0x00475c60)
int zactorGetField174(const ZActor *a)
{
    return a->field174;
}

/* Position of the actor with the motion's height offset (scaled when the actor is). */
BMX_ADDR(0x00475c80)
void zactorGetBasePos(const ZActor *a, ZVec3 *out)
{
    double h = a->player->ch[0].height;
    if (a->flags & kZactorScaled) {
        h = h * a->scale.y;
    }
    out->x = a->pos.x;
    out->y = (float)(h + a->pos.y);
    out->z = a->pos.z;
}

/*
 * World position of the head: (0, -160, 0) rotated through the two top joints and the actor's y rotation,
 * plus the motion height, the actor scale and position. The scale product stays in an x87 register.
 */
BMX_ADDR(0x00475cc0)
void zactorGetHeadPos(const ZActor *a, ZVec3 *out)
{
    const ZMotionChannel *ch = &a->player->ch[0];
    ZVec3 v = {0.0f, -160.0f, 0.0f};
    zvec3RotateZ(&v, &v, ch->joints[0].z);
    zvec3RotateY(&v, &v, ch->joints[0].y);
    zvec3RotateX(&v, &v, ch->joints[0].x);
    zvec3RotateZ(&v, &v, ch->joints[1].z);
    zvec3RotateY(&v, &v, ch->joints[1].y);
    zvec3RotateX(&v, &v, ch->joints[1].x);
    zvec3RotateY(&v, &v, (ZAngle)-a->rot[1]);
    double y = (double)v.y + ch->height;
    double x = v.x;
    float z = v.z;
    if (a->flags & kZactorScaled) {
        x = x * a->scale.x;
        y = y * a->scale.y;
        z = (float)((double)v.z * a->scale.z);
    }
    out->x = (float)(x + a->pos.x);
    out->y = (float)(y + a->pos.y);
    out->z = (float)((double)z + a->pos.z);
}

/*
 * Position of `bone` in the actor's skeleton: walks the parent chain of the bone (0xff terminated),
 * rotating by each joint and adding its offset. flags: 1 start from `in` instead of the origin, 2 replace
 * the result by `in` before scaling, 4 skip the actor's own rotation and position.
 */
BMX_ADDR(0x00475e30)
void zactorGetBonePos(const ZActor *a, int bone, const ZVec3 *in, ZVec3 *out, uint8_t flags)
{
    const ZMotionChannel *ch = &a->player->ch[0];
    const int8_t *chain = (const int8_t *)&gSkeletonChain + (bone + a->skeleton * 19) * 8;
    ZVec3 v;
    if (flags & 1) {
        v = *in;
    } else {
        v.x = v.y = v.z = 0.0f;
    }
    while (*chain != -1) {
        const int j = (int)*chain;
        zvec3RotateX(&v, &v, ch->joints[j].x);
        zvec3RotateY(&v, &v, (ZAngle)-ch->joints[j].y);
        zvec3RotateZ(&v, &v, ch->joints[j].z);
        const ZVec3 *off = (const ZVec3 *)((const uint8_t *)&gSkeletonOffsets + (j + a->skeleton * 19) * 16);
        chain++;
        v.x = (float)((double)v.x + off->x);
        v.y = (float)((double)v.y + off->y);
        v.z = (float)((double)v.z + off->z);
    }
    v.y = (float)((double)v.y + ch->height);
    if (flags & 2) {
        v = *in;
    }
    if (a->flags & kZactorScaled) {
        v.x = (float)((double)v.x * a->scale.x);
        v.y = (float)((double)v.y * a->scale.y);
        v.z = (float)((double)v.z * a->scale.z);
    }
    if ((flags & 4) == 0) {
        zvec3RotateX(&v, &v, a->rot[0]);
        zvec3RotateY(&v, &v, (ZAngle)-a->rot[1]);
        zvec3RotateZ(&v, &v, a->rot[2]);
        out->x = (float)((double)v.x + a->pos.x);
        out->y = (float)((double)v.y + a->pos.y);
        out->z = (float)((double)v.z + a->pos.z);
        return;
    }
    *out = v;
}

/* Adds `delta` to the joint-2 yaw, remembering the original the first time. */
BMX_ADDR(0x004760b0)
void zactorAddRiderYaw(ZActor *a, int16_t delta)
{
    if (a->type != kZactorTypePerson) {
        zdebugFail("Not a person type", zactorFile, 0xdf3, 0);
    }
    ZMotionChannel *ch = &a->player->ch[0];
    if (a->flags & kZactorYawOffset) {
        ch->joints[2].y = (ZAngle)(ch->savedYaw + (uint16_t)delta);
        return;
    }
    ch->savedYaw = (uint16_t)ch->joints[2].y;
    ch->joints[2].y = (ZAngle)((uint16_t)ch->joints[2].y + (uint16_t)delta);
    a->flags |= kZactorYawOffset;
}

BMX_ADDR(0x00476140)
void zactorSetJointAngles(ZActor *a, uint32_t joint, const ZAngle3 *angles)
{
    a->player->ch[0].joints[joint & 0xffff] = *angles;
}

BMX_ADDR(0x00476190)
void zactorGetJointAngles(const ZActor *a, uint32_t joint, ZAngle3 *angles)
{
    *angles = a->player->ch[0].joints[joint & 0xffff];
}

BMX_ADDR(0x004761e0)
void zactorSetSkeleton(ZActor *a, int skeleton)
{
    a->skeleton = skeleton;
}

BMX_ADDR(0x00476200)
int zactorGetSkeleton(const ZActor *a)
{
    return a->skeleton;
}

BMX_ADDR(0x00476220)
void zactorSetHeight(ZActor *a, float height)
{
    a->player->ch[0].height = height;
}

/* Number of root-bone key steps played so far (persons only). */
BMX_ADDR(0x00476250)
uint8_t zactorGetStepCount(const ZActor *a)
{
    if (a->type != kZactorTypePerson) {
        return 0;
    }
    return a->player->ch[0].counter;
}

BMX_ADDR(0x00476280)
float zactorGetHeight(const ZActor *a)
{
    return a->player->ch[0].height;
}

/* Actor rotation in motion space (y converted back through the angle base). */
BMX_ADDR(0x004762a0)
void zactorGetRotation(const ZActor *a, ZAngle3 *out)
{
    uint16_t y = (uint16_t)((uint16_t)a->rot[1] - (uint16_t)gActorAngleBase);
    if (gActorAngleFlip != 0) {
        y = (uint16_t)-y;
    }
    out->x = a->rot[0];
    out->y = (ZAngle)y;
    out->z = a->rot[2];
}

/* m = rotation by joint 0 of the motion, then by the actor rotation. */
BMX_ADDR(0x004762f0)
void zactorGetMatrix(const ZActor *a, ZMatrix *m)
{
    zmatrixIdentity(m);
    zmatrixRotateEuler(m, m, &a->player->ch[0].joints[0]);
    zmatrixRotateEuler(m, m, (const ZAngle3 *)&a->rot[0]);
}

/* Sets the actor rotation from motion-space angles. Writes the converted y back into `angles`. */
BMX_ADDR(0x00476340)
void zactorSetRotation(ZActor *a, ZAngle3 *angles)
{
    if (gActorAngleFlip != 0) {
        angles->y = (ZAngle)-angles->y;
    }
    angles->y = (ZAngle)((uint16_t)angles->y + (uint16_t)gActorAngleBase);
    a->rot[0] = angles->x;
    a->rot[1] = angles->y;
    a->rot[2] = angles->z;
    a->heading = angles->y;
}

/* Orients the actor along the rotation part of `m`. */
BMX_ADDR(0x004763b0)
void zactorFaceMatrix(ZActor *a, const ZMatrix *m)
{
    ZMatrix mat;
    ZAngle3 zero = {0, 0, 0};
    zmatrixCopyRotation(&mat, m);
    zactorOrient(a, &zero, &mat.up, &mat.right, &mat);
}

/*
 * Builds an orthonormal basis from the right and (negated) up vectors, applies `angles` and the angle base,
 * stores it in `out` and derives the actor's rotation from it. The up row of `out` ends up un-negated.
 */
BMX_ADDR(0x00476420)
void zactorOrient(ZActor *a, const ZAngle3 *angles, const ZVec3 *up, const ZVec3 *rightIn, ZMatrix *out)
{
    ZVec3 negUp = {-up->x, -up->y, -up->z};
    ZVec3 right = *rightIn;
    ZVec3 zero = {0.0f, 0.0f, 0.0f};
    ZVec3 fwd;
    int base = 0;

    const double d = zvec3Dot(&negUp, &right);
    if ((d < 0 ? -d : d) >= 0x1.47ae14p-7f) { /* 0.01; NaN takes the other branch */
        zvec3Cross3(&fwd, &zero, &right, &negUp);
        zvec3Cross3(&right, &zero, &negUp, &fwd);
        zvec3NormalizeScaled(&right, &right);
        zvec3NormalizeScaled(&fwd, &fwd);
    } else {
        zvec3Cross3(&fwd, &zero, &right, &negUp);
        const double d2 = zvec3Dot(&right, &right) - 1.0f;
        if (0x1.47ae14p-7f < (d2 < 0 ? -d2 : d2)) {
            zvec3NormalizeScaled(&right, &right);
        }
        zvec3NormalizeScaled(&fwd, &fwd);
    }

    ZMatrix rotY;
    ZMatrix rotE;
    zmatrixIdentity(&rotY);
    if ((int16_t)gActorAngleBase != 0) {
        base = gActorAngleFlip == 0 ? -gActorAngleBase : gActorAngleBase;
        zmatrixRotateY(&rotY, &rotY, (ZAngle)base);
    }
    zmatrixIdentity(&rotE);
    zmatrixRotateEuler(&rotE, &rotE, angles);

    zmatrixIdentity(out);
    out->right = right;
    out->up = negUp;
    out->at = fwd;
    zmatrixMul(out, out, &rotE);
    if ((int16_t)gActorAngleBase != 0) {
        zmatrixMul(out, out, &rotY);
    }
    zactorAnglesFromMatrix(a, out);
    if ((int16_t)gActorAngleBase != 0) {
        zmatrixIdentity(&rotY);
        zmatrixRotateY(&rotY, &rotY, (ZAngle)-base);
        zmatrixMul(out, out, &rotY);
    }
    out->up.x = -out->up.x;
    out->up.y = -out->up.y;
    out->up.z = -out->up.z;
    zvec3NormalizeScaled(&out->up, &out->up);
}

/* Derives the actor's rotation angles and heading from the basis in `m`. */
BMX_ADDR(0x00476770)
void zactorAnglesFromMatrix(ZActor *a, const ZMatrix *m)
{
    const float *f = (const float *)m;
    const double sq = __builtin_sqrt((double)f[0] * f[0] + (double)f[1] * f[1]);
    const float sqf = (float)sq;
    ZAngle rx, ry, rz;
    if (!(sq > 0x1.0624dep-10f)) { /* 0.001; NaN counts as small */
        rx = zmathAtan2(-f[9], f[5]);
        ry = zmathAtan2(-f[2], sqf);
        rz = 0;
    } else {
        rx = zmathAtan2(f[6], f[10]);
        ry = zmathAtan2(-f[2], sqf);
        rz = zmathAtan2(f[1], f[0]);
    }
    a->rot[0] = rx;
    a->rot[1] = ry;
    a->rot[2] = rz;
    a->heading = zmathAtan2(-f[2], f[0]);
}

BMX_ADDR(0x00476850)
void zactorBreak(void)
{
    __builtin_debugtrap();
}

/* Copies the root angles of b's motion result into joint 0 of a. */
BMX_ADDR(0x00476880)
void zactorCopyRootAngles(ZActor *a, const ZActor *b)
{
    a->player->ch[0].joints[0].x = b->player->ch[0].rootAngle.x;
    a->player->ch[0].joints[0].y = b->player->ch[0].rootAngle.y;
    a->player->ch[0].joints[0].z = b->player->ch[0].rootAngle.z;
}

BMX_ADDR(0x004768f0)
void zactorCopyRotation(ZActor *a, const ZActor *b)
{
    a->rot[0] = b->rot[0];
    a->rot[1] = b->rot[1];
    a->rot[2] = b->rot[2];
    a->heading = b->heading;
}

/* out = a * t + b * (1 - t) for `count` joint triples, truncated to 16 bit. */
BMX_ADDR(0x00476930)
void zmotionLerpAngles(const ZAngle3 *a, const ZAngle3 *b, ZAngle3 *out, float t, uint8_t count)
{
    const double s = 1.0f - (double)t;
    do {
        out->x = (ZAngle)(int)((double)a->x * t + (double)b->x * s);
        out->y = (ZAngle)(int)((double)a->y * t + (double)b->y * s);
        out->z = (ZAngle)(int)((double)a->z * t + (double)b->z * s);
        a++;
        b++;
        out++;
    } while (--count != 0);
}

/* Sets scale[i] = 1.0 and re-truncates each joint triple (a no-op multiplication by the scale). */
BMX_ADDR(0x00476a10)
void zmotionResetScale(ZAngle3 *angles, const void *unused, float *scale, uint8_t count)
{
    uint8_t i = 0;
    do {
        scale[i] = 1.0f;
        angles->x = (ZAngle)(int)(double)angles->x;
        angles->y = (ZAngle)(int)((double)angles->y * scale[i]);
        angles->z = (ZAngle)(int)((double)angles->z * scale[i]);
        i++;
        angles++;
    } while (--count != 0);
}

/*
 * Motion key streams. A record starts with a 32 bit header: bit 0 a float follows the 6 byte header,
 * bit 1 a root delta (three int16) follows, bits 2.. say which joints of the bone entry (bits 28-31) have
 * data, and a 16 bit time stamp sits at +4. Records are padded to 4 bytes. `size` tracks the byte count
 * modulo 4 the way the original does.
 */
static uint32_t zmotionKeyHeader(const uint8_t *p)
{
    return *(const uint32_t *)p;
}

static uint32_t zmotionKeyTime(const uint8_t *p)
{
    return *(const uint16_t *)(p + 4);
}

/* Decodes one joint's angles at `p`. `absolute` is the initial pose encoding (8 bit values are unsigned and
 * scaled by 256). */
static const uint8_t *zmotionReadAngles(const uint8_t *p, ZAngle3 *dst, int wide, int mirror, int absolute,
                                        uint32_t *size)
{
    if (wide) {
        const int16_t *w = (const int16_t *)p;
        dst->x = w[0];
        dst->y = mirror ? (ZAngle)-w[1] : w[1];
        dst->z = mirror ? (ZAngle)-w[2] : w[2];
        *size += 2;
        return p + 6;
    }
    const int8_t *b = (const int8_t *)p;
    const int shift = absolute ? 8 : 7;
    if (absolute) {
        dst->x = (ZAngle)((uint32_t)p[0] << 8);
        dst->y = mirror ? (ZAngle)(-b[1] * 256) : (ZAngle)((uint32_t)p[1] << 8);
        dst->z = mirror ? (ZAngle)(-b[2] * 256) : (ZAngle)((uint32_t)p[2] << 8);
    } else {
        dst->x = (ZAngle)(b[0] * (1 << shift));
        dst->y = (ZAngle)((mirror ? -b[1] : b[1]) * (1 << shift));
        dst->z = (ZAngle)((mirror ? -b[2] : b[2]) * (1 << shift));
    }
    *size += 3;
    return p + 3;
}

/* Skips the padding that keeps records 4 byte aligned. */
static const uint8_t *zmotionKeyAlign(const uint8_t *p, uint32_t size)
{
    size &= 3;
    if (size != 0) {
        p = p - size + 4;
    }
    return p;
}

/*
 * Samples the motion of a channel: advances its clock by the playback rate times `speed` and applies every
 * key up to the new time. `flags` bit 1 restarts the motion, bit 3 mirrors it, bit 7 (with restart) keeps
 * the previous motion as the blend source. Returns `out` (the root step of the actor), or NULL when the
 * stream has ended.
 */
BMX_ADDR(0x00472b10)
ZMotionDelta *zactorMotionSample(ZActor *a, float speed, uint32_t flags, ZMotionDelta *out, uint32_t channel)
{
    ZMotionPlayer *player = a->player;
    const uint32_t idx = channel & 0xff;
    ZMotionChannel *ch = &player->ch[idx + 1];
    ZMotionChannel *mir = &player->ch[idx + 5];
    const ZMotion *m = ch->motion;
    const int mirror = (flags & 8) != 0;

    if (a->flags & kZactorYawOffset) {
        ch->joints[2].y = (ZAngle)ch->savedYaw;
        a->flags &= ~(uint32_t)kZactorYawOffset;
    }
    out->delta.x = 0.0f;
    out->delta.y = 0.0f;
    out->delta.z = 0.0f;
    out->yaw = 0;
    out->rootSpeed = 0.0f;
    ch->flags = flags;

    if (flags & 2) {
        out->delta.x = ch->rootSaved.x;
        out->delta.z = ch->rootSaved.z;
        ch->rootAcc.x = 0.0f;
        ch->rootAcc.y = 0.0f;
        ch->rootAcc.z = 0.0f;
        ch->step = m->step;
        ch->length = (uint32_t)(m->frames & 0x7fff) * m->step;
        ch->format &= 0x21;
        if (m->format & 0x80) {
            ch->format |= 0x10;
        }
        ch->time = 0;
        ch->nextKey = 0;
        ch->counter = 0;
        ch->boneCount = m->boneByte >> 4;
        const ZMotionBone *bones = m->bones;
        ch->bones = bones;
        ch->streamEnd = (const uint8_t *)bones + ch->boneCount * 4 + m->markerOffset;
        ch->jointCount = bones[0].joints;
        ch->cursor = (const uint8_t *)(bones + ch->boneCount);
        if (m->frames & 0x8000) {
            ch->extra = ch->cursor;
            ch->cursor += 0x64;
        }
        const int wide = (ch->format & 0x10) != 0;
        for (uint32_t k = 0; k < ch->boneCount; k++) {
            const ZMotionBone *bone = &bones[k];
            const uint8_t *p = ch->cursor;
            uint32_t size = 2;
            if (bone->kind == 0) {
                __builtin_memcpy(&ch->rootSpeed, p + 8, 4);
                __builtin_memcpy(&out->rootSpeed, p + 8, 4);
                const double x = (double)*(const int16_t *)(p + 0xc) * 0x1p-3f;
                out->delta.x = (float)(mirror ? -x : x);
                out->delta.z = (float)((double)*(const int16_t *)(p + 0x10) * 0x1p-3f);
                ch->height = (float)((double)*(const int16_t *)(p + 0xe) * 0x1p-3f);
                p += 0x12;
                for (uint32_t j = 0; j < bone->joints; j++) {
                    ZAngle3 *dst = mirror ? &ch->joints[gMotionMirrorMap[j]] : &ch->joints[j];
                    p = zmotionReadAngles(p, dst, wide, mirror, 1, &size);
                }
                ch->cursor = zmotionKeyAlign(p, size);
            } else if ((bone->flags & 2) == 0) {
                const uint32_t hdr = zmotionKeyHeader(p);
                size = 0;
                const uint8_t *q = p + 0xc;
                if ((hdr & 1) == 0) {
                    q = p + 6;
                    size = 2;
                }
                if (hdr & 2) {
                    const double x = (double)((const int16_t *)q)[0] * 0x1p-3f;
                    ch->rootAcc.x = (float)(mirror ? -x : x);
                    ch->rootAcc.y = (float)((double)((const int16_t *)q)[1] * 0x1p-3f);
                    ch->rootAcc.z = (float)((double)((const int16_t *)q)[2] * 0x1p-3f);
                    q += 6;
                    size += 2;
                }
                if (hdr & 4) {
                    for (uint32_t j = 0; j < bone->joints; j++) {
                        q = zmotionReadAngles(q, &mir->joints[j], wide, mirror, 1, &size);
                    }
                    ch->rootAngle = mir->joints[0];
                } else {
                    ch->rootAngle.x = 0;
                    ch->rootAngle.y = 0;
                    ch->rootAngle.z = 0;
                    ch->rootAngle2.x = 0;
                    ch->rootAngle2.y = 0;
                    ch->rootAngle2.z = 0;
                }
                ch->cursor = zmotionKeyAlign(q, size);
            }
        }
        ch->nextKey += ch->step;
    } else {
        ch->time += (uint32_t)(int)((double)gMotionTimeScale * speed);
        const uint8_t *p = ch->cursor;
        if (p == NULL || *(const uint32_t *)p == 0) {
            return NULL;
        }
        const ZMotionBone *bones = ch->bones;
        const int wide = (ch->format & 0x10) != 0;
        if (ch->nextKey <= ch->time) {
            do {
                if (*(const uint32_t *)p != 0) {
                    const uint32_t t0 = zmotionKeyTime(p);
                    if (t0 <= ch->time) {
                        do {
                            const uint32_t hdr = zmotionKeyHeader(p);
                            const uint32_t bone = hdr >> 28;
                            uint32_t size = 2;
                            const uint8_t *q;
                            if (bones[bone].kind == 0) {
                                q = p + 6;
                                if (hdr & 1) {
                                    __builtin_memcpy(&ch->rootSpeed, p + 8, 4);
                                    __builtin_memcpy(&out->rootSpeed, p + 8, 4);
                                    size = 0;
                                    q = p + 0xc;
                                }
                                if (hdr & 2) {
                                    const double x = (double)((const int16_t *)q)[0] * 0x1p-8f;
                                    ch->rootDelta.x = (float)(mirror ? -x : x);
                                    ch->rootDelta.z = (float)((double)((const int16_t *)q)[2] * 0x1p-8f);
                                    ch->rootDelta.y = (float)((double)((const int16_t *)q)[1] * 0x1p-8f);
                                    q += 6;
                                    size += 2;
                                }
                                uint32_t mask = 2;
                                for (uint32_t j = 0; j < bones[bone].joints; j++) {
                                    mask <<= 1;
                                    if (hdr & mask) {
                                        ZAngle3 *dst =
                                            mirror ? &ch->stage[gMotionMirrorMap[j]] : &ch->stage[j];
                                        q = zmotionReadAngles(q, dst, wide, mirror, 0, &size);
                                    }
                                }
                            } else {
                                q = p + 6;
                                size = 2;
                                if (hdr & 1) {
                                    q = p + 0xc;
                                    size = 0;
                                }
                                if (hdr & 2) {
                                    const double x = (double)((const int16_t *)q)[0] * 0x1p-8f;
                                    ch->boneDelta.x = (float)(mirror ? -x : x);
                                    ch->boneDelta.y = (float)((double)((const int16_t *)q)[1] * 0x1p-8f);
                                    ch->boneDelta.z = (float)((double)((const int16_t *)q)[2] * 0x1p-8f);
                                    q += 6;
                                    size += 2;
                                }
                                uint32_t mask = 2;
                                for (uint32_t j = 0; j < bones[bone].joints; j++) {
                                    mask <<= 1;
                                    if (hdr & mask) {
                                        q = zmotionReadAngles(q, &mir->stage[j], wide, mirror, 0, &size);
                                    }
                                }
                            }
                            ch->cursor = zmotionKeyAlign(q, size);
                            p = ch->cursor;
                        } while (zmotionKeyTime(p) == t0);
                    }
                    for (uint32_t k = 0; k < ch->boneCount; k++) {
                        const ZMotionBone *bone = &bones[k];
                        if (bone->kind == 0) {
                            ch->counter++;
                            ch->height = (float)((double)ch->rootDelta.y + ch->height);
                            out->delta.x = ch->rootDelta.x;
                            out->delta.z = ch->rootDelta.z;
                            for (uint32_t j = 0; j < bone->joints; j++) {
                                ch->joints[j].x = (ZAngle)(ch->joints[j].x + ch->stage[j].x);
                                ch->joints[j].y = (ZAngle)(ch->joints[j].y + ch->stage[j].y);
                                ch->joints[j].z = (ZAngle)(ch->joints[j].z + ch->stage[j].z);
                            }
                        } else if ((bone->flags & 2) == 0) {
                            ch->rootAcc.x = (float)((double)ch->boneDelta.x + ch->rootAcc.x);
                            ch->rootAcc.y = (float)((double)ch->boneDelta.y + ch->rootAcc.y);
                            ch->rootAcc.z = (float)((double)ch->boneDelta.z + ch->rootAcc.z);
                            for (uint32_t j = 0; j < bone->joints; j++) {
                                mir->joints[j].x = (ZAngle)(mir->joints[j].x + mir->stage[j].x);
                                mir->joints[j].y = (ZAngle)(mir->joints[j].y + mir->stage[j].y);
                                mir->joints[j].z = (ZAngle)(mir->joints[j].z + mir->stage[j].z);
                            }
                            ch->rootAngle = mir->joints[0];
                        }
                    }
                }
                ch->nextKey += ch->step;
            } while (ch->nextKey <= ch->time);
        }
    }

    if (ch->nextKey >= ch->length) {
        ch->rootSaved = ch->rootDelta;
    }
    out->yaw = ch->joints[0].y;
    out->rootSpeed = ch->rootSpeed;
    if (gActorAngleFlip == 0) {
        out->yaw = (ZAngle)(out->yaw - (uint16_t)gActorAngleBase);
        zvec3RotateY(&out->delta, &out->delta, (ZAngle)gActorAngleBase);
    } else {
        out->yaw = (ZAngle)((uint16_t)gActorAngleBase - (uint16_t)out->yaw);
        zvec3RotateY(&out->delta, &out->delta, (ZAngle)-gActorAngleBase);
    }
    if (a->flags & kZactorScaled) {
        out->delta.x = (float)((double)a->scale.x * out->delta.x);
        out->delta.y = (float)((double)a->scale.y * out->delta.y);
        out->delta.z = (float)((double)a->scale.z * out->delta.z);
    }
    return out;
}

/* Skips a record of a secondary bone entry. */
static const uint8_t *zmotionSkipKey(const uint8_t *p)
{
    const uint32_t hdr = zmotionKeyHeader(p);
    uint32_t size = 0;
    if (hdr & 1) {
        p += 0xc;
    } else {
        p += 6;
        size = 2;
    }
    if (hdr & 2) {
        p += 6;
        size += 2;
    }
    return zmotionKeyAlign(p, size);
}

/*
 * Runs the key stream of `m` for `steps` time steps and adds the root-bone joint deltas to `joints`,
 * without keeping any playback state. The initial pose is written to `joints` first. `flags` bit 3 mirrors.
 * The original's staging buffer is uninitialised stack and is zeroed here. It matters for mirrored motions:
 * keys are stored through the joint map but accumulated by plain index, so stale entries get added.
 */
BMX_ADDR(0x004739c0)
void zactorMotionSkim(ZActor *a, ZAngle3 *joints, const ZMotion *m, int steps, uint8_t flags)
{
    ZAngle3 stage[kZmotionMaxJoints] = {{0, 0, 0}};
    const int mirror = (flags & 8) != 0;

    if (a->flags & kZactorYawOffset) {
        a->flags &= ~(uint32_t)kZactorYawOffset;
    }
    const uint32_t step = m->step;
    const uint32_t boneCount = m->boneByte >> 4;
    const ZMotionBone *bones = m->bones;
    const uint8_t *p = (const uint8_t *)(bones + boneCount);
    if (m->frames & 0x8000) {
        p += 0x64;
    }
    /* The format flag is read from an uninitialised local and masked with 0x21: 8 bit angles always. */
    for (uint32_t k = 0; k < boneCount; k++) {
        const ZMotionBone *bone = &bones[k];
        if (bone->kind == 0) {
            const uint8_t *q = p + 0x12;
            uint32_t size = 2;
            for (uint32_t j = 0; j < bone->joints; j++) {
                ZAngle3 *dst = mirror ? &joints[gMotionMirrorMap[j]] : &joints[j];
                q = zmotionReadAngles(q, dst, 0, mirror, 1, &size);
            }
            p = zmotionKeyAlign(q, size);
        } else if ((bone->flags & 2) == 0) {
            p = zmotionSkipKey(p);
        }
    }

    uint32_t time = 0;
    uint32_t nextKey = step;
    uint32_t keyClock = step;
    while (steps != 0) {
        time += step;
        steps--;
        if (p == NULL || *(const uint32_t *)p == 0) {
            return;
        }
        while (nextKey <= time) {
            if (*(const uint32_t *)p != 0) {
                const uint32_t t0 = zmotionKeyTime(p);
                if (t0 <= time) {
                    do {
                        const uint32_t hdr = zmotionKeyHeader(p);
                        const uint32_t bone = hdr >> 28;
                        if (bones[bone].kind == 0) {
                            uint32_t size = 2;
                            const uint8_t *q = p + 6;
                            if (hdr & 1) {
                                q = p + 0xc;
                                size = 0;
                            }
                            if (hdr & 2) {
                                q += 6;
                                size += 2;
                            }
                            uint32_t mask = 2;
                            for (uint32_t j = 0; j < bones[bone].joints; j++) {
                                mask <<= 1;
                                if (hdr & mask) {
                                    ZAngle3 *dst = mirror ? &stage[gMotionMirrorMap[j]] : &stage[j];
                                    q = zmotionReadAngles(q, dst, 0, mirror, 0, &size);
                                }
                            }
                            p = zmotionKeyAlign(q, size);
                        } else {
                            p = zmotionSkipKey(p);
                        }
                    } while (zmotionKeyTime(p) == t0);
                }
                for (uint32_t k = 0; k < boneCount; k++) {
                    const ZMotionBone *bone = &bones[k];
                    if (bone->kind == 0) {
                        for (uint32_t j = 0; j < bone->joints; j++) {
                            joints[j].x = (ZAngle)(joints[j].x + stage[j].x);
                            joints[j].y = (ZAngle)(joints[j].y + stage[j].y);
                            joints[j].z = (ZAngle)(joints[j].z + stage[j].z);
                        }
                    }
                }
            }
            keyClock += step;
            nextKey = keyClock;
        }
    }
}

/*
 * Plays `motion` on channel 1 and blends it with the previous motion (channel 2) according to the blend
 * counter, writing the result to channel 0. flags: 2 restart (start a blend from the current pose), 0x80
 * restart without blending, 8 mirror. Returns the sampled root step, or NULL when the stream ended.
 */
BMX_ADDR(0x004740c0)
ZMotionDelta *zactorPlayMotion(ZActor *a, const ZMotion *motion, float speed, uint32_t flags)
{
    ZMotionPlayer *pl = a->player;
    ZMotionChannel *c0 = &pl->ch[0];
    ZMotionChannel *c1 = &pl->ch[1];
    ZMotionChannel *c2 = &pl->ch[2];
    a->flags |= kZactorMotionPlayed;
    c1->motion = motion;
    ZMotionDelta *r = zactorMotionSample(a, speed, flags, &gMotionDeltaE, 0);
    if (r == NULL) {
        return NULL;
    }
    if (flags & 2) {
        if (flags & 0x80) {
            c2->motion = NULL;
            pl->blend = 0;
        } else if (c2->motion == NULL) {
            pl->blend = 0;
        } else {
            const uint32_t words = (uint32_t)c1->jointCount * 3;
            for (uint32_t i = 0; i < words; i++) {
                ((uint16_t *)c2->joints)[i] = ((const uint16_t *)c0->joints)[i];
            }
            c2->rootAcc = c0->rootAcc;
            c2->rootAngle = c0->rootAngle;
            c2->height = c0->height;
            gMotionPrevYawE = gMotionDeltaE.yaw;
            pl->blend = 3;
        }
        c0->jointCount = c1->jointCount;
        c0->boneCount = c1->boneCount;
    }
    ZAngle3 *secondaryCur = &pl->ch[5].joints[1];
    ZAngle3 *secondaryPrev = &pl->ch[6].joints[1];
    switch (pl->blend) {
    case 0:
        gMotionLastDelta.yaw = gMotionDeltaE.yaw;
        zmemCopy(c0->joints, c1->joints, (uint32_t)c0->jointCount * 6);
        c0->rootAcc = c1->rootAcc;
        c0->rootAngle = c1->rootAngle;
        c0->height = c1->height;
        zmemCopy(secondaryPrev, secondaryCur, 0x36);
        break;
    case 1:
        gMotionLastDelta.yaw = (ZAngle)((3 * (int)gMotionDeltaE.yaw + (int)gMotionPrevYawE) / 4);
        c0->height = (float)(((double)c1->height * 3.0f + c2->height) * 0.25f);
        zmotionBlendAngles(c2->joints, c1->joints, c0->joints, c0->jointCount);
        if (!zvec3IsZero(&c2->rootAcc)) {
            zmotionBlendAngles(secondaryPrev, secondaryCur, secondaryPrev, 9);
            zmotionBlendAngles(&c2->rootAngle, &c1->rootAngle, &c0->rootAngle, 1);
            zvec3Blend(&c2->rootAcc, &c1->rootAcc, &c0->rootAcc);
        } else {
            c0->rootAcc = c1->rootAcc;
            c0->rootAngle = c1->rootAngle;
        }
        break;
    case 2:
        gMotionLastDelta.yaw = (ZAngle)(((int)gMotionPrevYawE + (int)gMotionDeltaE.yaw) / 2);
        c0->height = (float)(((double)c2->height + c1->height) * 0.5f);
        zmotionAverageAngles(c1->joints, c2->joints, c0->joints, c0->jointCount);
        if (!zvec3IsZero(&c2->rootAcc)) {
            zmotionAverageAngles(secondaryCur, secondaryPrev, secondaryPrev, 9);
            zmotionAverageAngles(&c1->rootAngle, &c2->rootAngle, &c0->rootAngle, 1);
            zvec3Average(&c1->rootAcc, &c2->rootAcc, &c0->rootAcc);
        } else {
            c0->rootAngle = c1->rootAngle;
            c0->rootAcc = c1->rootAcc;
        }
        break;
    case 3:
        gMotionLastDelta.yaw = (ZAngle)((3 * (int)gMotionPrevYawE + (int)gMotionDeltaE.yaw) / 4);
        c0->height = (float)(((double)c2->height * 3.0f + c1->height) * 0.25f);
        zmotionBlendAngles(c1->joints, c2->joints, c0->joints, c0->jointCount);
        if (!zvec3IsZero(&c2->rootAcc)) {
            zmotionBlendAngles(secondaryCur, secondaryPrev, secondaryPrev, 9);
            zmotionBlendAngles(&c1->rootAngle, &c2->rootAngle, &c0->rootAngle, 1);
            zvec3Blend(&c1->rootAcc, &c2->rootAcc, &c0->rootAcc);
        } else {
            c0->rootAcc = c1->rootAcc;
            c0->rootAngle = c1->rootAngle;
        }
        break;
    case 4:
        gMotionLastDelta.yaw = gMotionPrevYawE;
        zmemCopy(c0->joints, c2->joints, (uint32_t)c0->jointCount * 6);
        if (!zvec3IsZero(&c2->rootAcc)) {
            c0->rootAcc = c2->rootAcc;
            c0->rootAngle = c2->rootAngle;
        } else {
            c0->rootAcc = c1->rootAcc;
            c0->rootAngle = c1->rootAngle;
        }
        c0->height = c2->height;
        break;
    default:
        break;
    }
    gMotionLastDelta.rootSpeed = gMotionDeltaE.rootSpeed;
    gMotionLastDelta.delta = gMotionDeltaE.delta;
    pl = a->player;
    if (pl->blend != 0) {
        pl->blend--;
    }
    c0->counter = c1->counter;
    c0->extra = c1->extra;
    if ((flags & 0x80) == 0) {
        c2->motion = motion;
    }
    c0->motion = motion;
    c0->rootAcc = c1->rootAcc;
    return r;
}

/* Finishes a two-motion blend: restarts the previous-motion bookkeeping and mixes channel 1 with channel 2
 * into channel 0. */
static void zactorBlendResult(ZActor *a, const ZMotion *motionA, const uint16_t *flags, ZMotionChannel *l1,
                              ZMotionChannel *l2, uint16_t tempYawSrc, int16_t *tempYawDst)
{
    ZMotionPlayer *pl = a->player;
    ZMotionChannel *c0 = &pl->ch[0];
    if (flags[0] & 2) {
        if (flags[0] & 0x80) {
            l2->motion = NULL;
            pl->blend = 0;
        } else if (l2->motion == NULL) {
            pl->blend = 0;
        } else {
            const uint32_t words = (uint32_t)l1->jointCount * 3;
            for (uint32_t i = 0; i < words; i++) {
                ((uint16_t *)l2->joints)[i] = ((const uint16_t *)c0->joints)[i];
            }
            l2->rootAcc = c0->rootAcc;
            l2->height = c0->height;
            *tempYawDst = (int16_t)tempYawSrc;
            pl->blend = 3;
        }
        c0->jointCount = l1->jointCount;
        c0->boneCount = l1->boneCount;
    }
    switch (pl->blend) {
    case 0:
        zmemCopy(c0->joints, l1->joints, (uint32_t)c0->jointCount * 6);
        c0->rootAcc = l1->rootAcc;
        c0->height = l1->height;
        break;
    case 1:
        c0->height = (float)(((double)l1->height * 3.0f + l2->height) * 0.25f);
        zmotionBlendAngles(l2->joints, l1->joints, c0->joints, c0->jointCount);
        if (!zvec3IsZero(&l2->rootAcc)) {
            zvec3Blend(&l2->rootAcc, &l1->rootAcc, &c0->rootAcc);
        } else {
            c0->rootAcc = l1->rootAcc;
        }
        break;
    case 2:
        c0->height = (float)(((double)l2->height + l1->height) * 0.5f);
        zmotionAverageAngles(l1->joints, l2->joints, c0->joints, c0->jointCount);
        if (!zvec3IsZero(&l2->rootAcc)) {
            zvec3Average(&l1->rootAcc, &l2->rootAcc, &c0->rootAcc);
        } else {
            c0->rootAcc = l1->rootAcc;
        }
        break;
    case 3:
        c0->height = (float)(((double)l2->height * 3.0f + l1->height) * 0.25f);
        zmotionBlendAngles(l1->joints, l2->joints, c0->joints, c0->jointCount);
        if (!zvec3IsZero(&l2->rootAcc)) {
            zvec3Blend(&l1->rootAcc, &l2->rootAcc, &c0->rootAcc);
        } else {
            c0->rootAcc = l1->rootAcc;
        }
        break;
    case 4:
        zmemCopy(c0->joints, l2->joints, (uint32_t)c0->jointCount * 6);
        if (!zvec3IsZero(&l2->rootAcc)) {
            c0->rootAcc = l2->rootAcc;
        } else {
            c0->rootAcc = l1->rootAcc;
        }
        c0->height = l2->height;
        break;
    default:
        break;
    }
    pl = a->player;
    if (pl->blend != 0) {
        pl->blend--;
    }
    c0->counter = l1->counter;
    c0->extra = l1->extra;
    if ((flags[0] & 0x80) == 0) {
        l2->motion = motionA;
    }
    c0->motion = motionA;
    c0->rootAcc = l1->rootAcc;
    c0->rootAngle = l1->rootAngle;
}

/*
 * Samples motion A and B (channels 3 and 4) and mixes them as A * t + B * (1 - t) into channel 1, then
 * blends that with the previous result as zactorPlayMotion does. flags[0]/flags[1] are the sample flags
 * of A and B. Returns the address of the global that holds A's root step.
 */
BMX_ADDR(0x00476ae0)
ZMotionDelta **zactorBlendMotions(ZActor *a, const ZMotion *motionA, const ZMotion *motionB,
                                  const uint16_t *flags, float t)
{
    ZMotionPlayer *pl = a->player;
    ZMotionChannel *c0 = &pl->ch[0];
    a->flags |= kZactorMotionPlayed;
    ZMotionChannel *l1 = &pl->ch[1];
    ZMotionChannel *l2 = &pl->ch[2];
    ZMotionChannel *l3 = &pl->ch[3];
    ZMotionChannel *l4 = &pl->ch[4];
    l3->motion = motionA;
    l4->motion = motionB;
    gMotionSampleB = zactorMotionSample(a, 1.0f, flags[1], &gMotionDeltaB, 3);
    gMotionSampleA = zactorMotionSample(a, 1.0f, flags[0], &gMotionDeltaA, 2);
    zmotionLerpAngles(l3->joints, l4->joints, l1->joints, t, c0->jointCount);
    const double s = 1.0f - (double)t;
    l1->height = (float)((double)t * l3->height + s * l4->height);
    l1->rootAcc.x = (float)((double)t * l3->rootAcc.x + s * l4->rootAcc.x);
    l1->rootAngle.x = (ZAngle)(int)((double)l4->rootAngle.x * s + (double)l3->rootAngle.x * t);
    l1->rootAcc.y = (float)((double)t * l3->rootAcc.y + s * l4->rootAcc.y);
    l1->rootAngle.y = (ZAngle)(int)((double)l4->rootAngle.y * s + (double)l3->rootAngle.y * t);
    l1->rootAcc.z = (float)((double)t * l3->rootAcc.z + s * l4->rootAcc.z);
    l1->rootAngle.z = (ZAngle)(int)((double)l4->rootAngle.z * s + (double)l3->rootAngle.z * t);
    zactorBlendResult(a, motionA, flags, l1, l2, (uint16_t)gMotionDeltaA.yaw, &gMotionDeltaB.yaw);
    return &gMotionSampleA;
}

/*
 * Samples motion `motion` on channel 3 and normalises channel 1's joints against the scale array (which
 * is reset to 1.0). Returns the root step.
 */
BMX_ADDR(0x00477210)
ZMotionDelta *zactorSampleScaled(ZActor *a, const ZMotion *motion, uint16_t flags, float *scale)
{
    ZMotionPlayer *pl = a->player;
    ZMotionDelta tmp;
    pl->ch[3].motion = motion;
    gMotionSampleA = zactorMotionSample(a, 1.0f, flags, &tmp, 2);
    zmotionResetScale(pl->ch[1].joints, pl->ch[3].joints, scale, pl->ch[0].jointCount);
    return gMotionSampleA;
}

/* out[i] = mask[i] ? b[i] : a[i] for `count` joint triples. */
BMX_ADDR(0x004772a0)
void zmotionSelectJoints(const ZAngle3 *a, const ZAngle3 *b, ZAngle3 *out, const uint8_t *mask, uint8_t count)
{
    for (int i = 0; i < count; i++) {
        *out = mask[i] != 0 ? *b : *a;
        a++;
        b++;
        out++;
    }
}

/*
 * Like zactorBlendMotions, but the two motions are combined joint by joint with a mask instead of a
 * weight: channel 1 takes B's joints where mask is set, A's elsewhere (root data from A).
 */
BMX_ADDR(0x00477330)
ZMotionDelta **zactorBlendMotionsMasked(ZActor *a, const ZMotion *motionA, const ZMotion *motionB,
                                        const uint16_t *flags, const uint8_t *mask)
{
    ZMotionPlayer *pl = a->player;
    ZMotionChannel *c0 = &pl->ch[0];
    a->flags |= kZactorMotionPlayed;
    ZMotionChannel *l1 = &pl->ch[1];
    ZMotionChannel *l2 = &pl->ch[2];
    ZMotionChannel *l3 = &pl->ch[3];
    ZMotionChannel *l4 = &pl->ch[4];
    l3->motion = motionA;
    l4->motion = motionB;
    gMotionSampleB = zactorMotionSample(a, 1.0f, flags[1], &gMotionDeltaD, 3);
    gMotionSampleA = zactorMotionSample(a, 1.0f, flags[0], &gMotionDeltaC, 2);
    zmotionSelectJoints(l3->joints, l4->joints, l1->joints, mask, c0->jointCount);
    l1->height = l3->height;
    l1->rootAcc = l3->rootAcc;
    l1->rootAngle = l3->rootAngle;
    zactorBlendResult(a, motionA, flags, l1, l2, (uint16_t)gMotionDeltaC.yaw, &gMotionDeltaD.yaw);
    return &gMotionSampleA;
}
