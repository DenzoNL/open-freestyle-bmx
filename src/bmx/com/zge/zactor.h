/* Actors and their skeletal motion playback (C:\zaxis\Bmx\src\com\zge\zactor.c). */
#ifndef BMX_COM_ZGE_ZACTOR_H
#define BMX_COM_ZGE_ZACTOR_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

enum {
    kZactorTypePerson = 1, /* has a motion player */
    kZactorTypeProp = 3,
    kZactorTypeEffect = 4,
    kZactorTypeObject = 8, /* only the scale is stored */
};

/* ZActor.flags */
enum {
    kZactorVisible = 0x1,
    kZactorFlag2 = 0x2,
    kZactorPerson = 0x4,
    kZactorScaled = 0x8, /* scale differs from 1.0 */
    kZactorFlag20 = 0x20,
    kZactorFlag800 = 0x800,
    kZactorYawOffset = 0x2000, /* zactorAddRiderYaw has stashed the original yaw */
    kZactorMotionPlayed = 0x4000,
};

typedef struct ZActor {
    int32_t type;                 /* 0x00, 0 = free slot */
    uint32_t flags;               /* 0x04 */
    int32_t skeleton;             /* 0x08 index into the skeleton chain tables */
    ZVec3 pos;                    /* 0x0c */
    uint8_t pad18[4];             /* 0x18 */
    ZAngle heading;               /* 0x1c */
    ZAngle rot[3];                /* 0x1e angles about x, y, z */
    uint8_t pad24[4];             /* 0x24 */
    ZVec3 scale;                  /* 0x28 */
    uint8_t pad34[4];             /* 0x34 */
    float extent[3];              /* 0x38 */
    uint8_t pad44[4];             /* 0x44 */
    struct ZMotionPlayer *player; /* 0x48, persons only */
    int32_t field4c;              /* 0x4c */
    uint32_t color;               /* 0x50 */
    uint8_t pad54[8];             /* 0x54 */
    int32_t field5c;              /* 0x5c */
    uint8_t pad60[4];             /* 0x60 */
    uint16_t field64;             /* 0x64 */
    uint8_t pad66[0x170 - 0x66];
    uint16_t model;     /* 0x170 */
    uint16_t field172;  /* 0x172 */
    int32_t field174;   /* 0x174 */
    int32_t attachBone; /* 0x178 */
    ZVec3 attachOffset; /* 0x17c */
    uint8_t pad188[0x18e - 0x188];
    uint16_t field18e;          /* 0x18e */
    uint8_t pad190[4];          /* 0x190 */
    uint8_t lod[0x1d0 - 0x194]; /* uint16 model every 0x10 bytes, up to four levels of detail */
    float radius;               /* 0x1d0 */
    int32_t object;             /* 0x1d4 handle passed to the object module */
    int32_t shadow;             /* 0x1d8 shadow object, posed like `object` (see zactorSyncPose) */
    uint32_t partHash[15];      /* 0x1dc name hash of each part (riders use 15, bikes 10), see ngifzactor.c */
    uint8_t pad218[0x228 - 0x218];
    uint32_t partOrder[15]; /* 0x228 part indices in drawing order */
    uint8_t pad264[0x274 - 0x264];
    int32_t partCount; /* 0x274 */
} ZActor;

_Static_assert(sizeof(ZActor) == 0x278, "actor size");
_Static_assert(__builtin_offsetof(ZActor, pos) == 0xc, "actor layout");
_Static_assert(__builtin_offsetof(ZActor, rot) == 0x1e, "actor layout");
_Static_assert(__builtin_offsetof(ZActor, scale) == 0x28, "actor layout");
_Static_assert(__builtin_offsetof(ZActor, player) == 0x48, "actor layout");
_Static_assert(__builtin_offsetof(ZActor, lod) == 0x194, "actor layout");
_Static_assert(__builtin_offsetof(ZActor, radius) == 0x1d0, "actor layout");
_Static_assert(__builtin_offsetof(ZActor, object) == 0x1d4, "actor layout");
_Static_assert(__builtin_offsetof(ZActor, shadow) == 0x1d8, "actor layout");
_Static_assert(__builtin_offsetof(ZActor, partHash) == 0x1dc, "actor layout");
_Static_assert(__builtin_offsetof(ZActor, partOrder) == 0x228, "actor layout");
_Static_assert(__builtin_offsetof(ZActor, partCount) == 0x274, "actor layout");

/* Header of a motion (keyframe animation), as stored in a motion bank. */
typedef struct ZMotionBone {
    uint8_t joints; /* joints driven by this entry */
    uint8_t flags;  /* bit 1: entry has no keyframes */
    uint16_t kind;  /* 0 = root bone (also carries the root motion), else a secondary chain */
} ZMotionBone;

typedef struct ZMotion {
    uint16_t frames;      /* 0x00 bits 0-14: key intervals, bit 15: a 0x64 byte block precedes the keys */
    uint8_t boneByte;     /* 0x02 high nibble: bone entry count */
    uint8_t format;       /* 0x03 bit 7: joint angles stored as 16 bit, else 8 bit; 0x70: marker angles */
    uint32_t step;        /* 0x04 time per key interval */
    int32_t markerOffset; /* 0x08 */
    ZMotionBone bones[];  /* 0x0c, boneCount entries, then the key stream */
} ZMotion;

/*
 * Marker block of a motion, at ZMotion.markerOffset. The count is read as a byte by the counting function
 * and as a 16 bit value by the readers. Entries are {weight, x, y, z[, angle]} as int16.
 */
typedef struct ZMotionMarkerBlock {
    uint8_t pad00[0x0c];
    union {
        uint8_t count;    /* 0x0c */
        uint16_t count16; /* 0x0c */
    };
    uint8_t pad0e[2];
    int16_t points[]; /* 0x10 */
} ZMotionMarkerBlock;

_Static_assert(__builtin_offsetof(ZMotionMarkerBlock, count) == 0x0c, "marker block layout");
_Static_assert(__builtin_offsetof(ZMotionMarkerBlock, points) == 0x10, "marker block layout");

/* Magic of a motion bank file that starts with a 0x10 byte header. */
enum { kZmotionBankMagic = 0x2a81511c, kZmotionBankHeaderSize = 0x10 };

/* Result of sampling a channel: root displacement for this step. */
typedef struct ZMotionDelta {
    ZVec3 delta;      /* 0x00 */
    uint32_t pad0c;   /* 0x0c */
    ZAngle yaw;       /* 0x10 */
    uint8_t pad12[2]; /* 0x12 */
    float rootSpeed;  /* 0x14 */
} ZMotionDelta;

/* Playback state of one motion: time, key stream cursor and the joints decoded so far. */
typedef struct ZMotionChannel {
    uint32_t time;            /* 0x00 */
    uint32_t step;            /* 0x04 */
    uint32_t length;          /* 0x08 */
    uint32_t nextKey;         /* 0x0c */
    const uint8_t *cursor;    /* 0x10 next key record */
    uint8_t pad14[8];         /* 0x14 */
    float rootSpeed;          /* 0x1c */
    const uint8_t *streamEnd; /* 0x20 */
    const ZMotionBone *bones; /* 0x24 */
    const uint8_t *extra;     /* 0x28 the 0x64 byte block, if the motion has one */
    ZVec3 rootDelta;          /* 0x2c */
    uint8_t pad38[4];         /* 0x38 */
    ZVec3 boneDelta;          /* 0x3c */
    uint8_t pad48[4];         /* 0x48 */
    ZAngle3 stage[19];        /* 0x4c angles decoded from the keys, added to joints per step */
    uint8_t pad_be[2];        /* 0xbe */
    const ZMotion *motion;    /* 0xc0 */
    uint32_t flags;           /* 0xc4 flags of the last sample call */
    uint8_t padc8[4];         /* 0xc8 */
    uint32_t format;          /* 0xcc 0x10: 16 bit angles */
    ZVec3 rootSaved;          /* 0xd0 */
    uint8_t paddc[4];         /* 0xdc */
    uint8_t boneCount;        /* 0xe0 */
    uint8_t jointCount;       /* 0xe1 */
    uint8_t pade2[2];         /* 0xe2 */
    ZVec3 rootAcc;            /* 0xe4 accumulated root offset of the secondary chains */
    uint8_t padf0[4];         /* 0xf0 */
    ZAngle3 rootAngle;        /* 0xf4 */
    ZAngle3 rootAngle2;       /* 0xfa */
    float height;             /* 0x100 */
    ZAngle3 joints[19];       /* 0x104 */
    uint16_t field176;        /* 0x176 */
    uint16_t savedYaw;        /* 0x178 */
    uint8_t counter;          /* 0x17a */
    uint8_t pad17b;           /* 0x17b */
} ZMotionChannel;

_Static_assert(sizeof(ZMotionChannel) == 0x17c, "channel size");
_Static_assert(__builtin_offsetof(ZMotionChannel, motion) == 0xc0, "channel layout");
_Static_assert(__builtin_offsetof(ZMotionChannel, rootAcc) == 0xe4, "channel layout");
_Static_assert(__builtin_offsetof(ZMotionChannel, joints) == 0x104, "channel layout");

enum { kZmotionChannels = 9, kZmotionMaxJoints = 19 };

/*
 * Motion player of a person actor. Channel 0 is the blended result; 1 to 4 are sampled motions and
 * 5 to 8 their mirror companions.
 */
typedef struct ZMotionPlayer {
    ZMotionChannel ch[kZmotionChannels];
    uint8_t pad_d5c[0xd5d - 0xd5c];
    uint8_t blend; /* 0xd5d: counts down from 3 after a motion change */
    uint8_t pad_d5e[0xd60 - 0xd5e];
} ZMotionPlayer;

_Static_assert(sizeof(ZMotionPlayer) == 0xd60, "player size");
/* zactorCopySecondaryJoints copies from +0x9f2 to +0x10a. */
_Static_assert(__builtin_offsetof(ZMotionPlayer, ch[6].joints[1]) == 0x9f2, "player layout");
_Static_assert(__builtin_offsetof(ZMotionPlayer, ch[0].joints[1]) == 0x10a, "player layout");

/* Entry of a motion bank (a loaded motion library). */
typedef struct ZMotionBank {
    uint8_t *base;          /* 0x00 library data (advanced past the 0x10 byte header when present) */
    uint32_t size;          /* 0x04 */
    const uint16_t *table;  /* 0x08 {entry stride in 8 byte units, count} followed by entries */
    const uint32_t *cursor; /* 0x0c next entry for zmotionBankNext */
    uint32_t remaining;     /* 0x10 entries left for zmotionBankNext */
} ZMotionBank;

/* Result of zmotionBankNext. */
typedef struct ZMotionEntry {
    uint32_t id;
    const ZMotion *motion;
} ZMotionEntry;

/* One marker point of a motion, after conversion to world units. */
typedef struct ZMotionMarker {
    ZVec3 pos;    /* 0x00 */
    float weight; /* 0x0c */
    ZAngle angle; /* 0x10 */
    uint8_t pad12[2];
} ZMotionMarker;

_Static_assert(sizeof(ZMotionMarker) == 0x14, "marker size");

/* Root bone pose for the actor's current motion. */
typedef struct ZActorRootPose {
    ZVec3 pos;    /* 0x00 */
    float pad0c;  /* 0x0c */
    ZAngle3 rot;  /* 0x10 */
    ZAngle pad16; /* 0x16 */
} ZActorRootPose;

typedef ZMotionBank ZMotionBankTable[2];

#define gZactorMax       ORIG_VAR(0x0055f360, int32_t, gZactorMax)
#define gZactors         ORIG_VAR(0x00641e1c, ZActor *, gZactors)
#define gMotionBankCount ORIG_VAR(0x00641e20, int32_t, gMotionBankCount)
/* Added to (or, when gActorAngleFlip is set, subtracted from) angles that cross the motion boundary. */
#define gActorAngleBase ORIG_VAR(0x00641e24, int32_t, gActorAngleBase)
#define gActorAngleFlip ORIG_VAR(0x00641e28, int32_t, gActorAngleFlip)
#define gMotionLibrary  ORIG_VAR(0x00641e2c, int32_t, gMotionLibrary)
#define gMotionBuffer   ORIG_VAR(0x00641e30, void *, gMotionBuffer)
#define gMotionBanks    ORIG_VAR(0x00601dc8, ZMotionBankTable, gMotionBanks)
/* Data of the first bank loaded, and the id of the bank loaded by zmotionLoadBank. */
#define gMotionFirstBank  ORIG_VAR(0x00601e10, void *, gMotionFirstBank)
#define gMotionLoadedBank ORIG_VAR(0x0055f38c, uint32_t, gMotionLoadedBank)
/* Time scale for motion playback: step = ftol(gMotionTimeScale * speed). */
#define gMotionTimeScale ORIG_VAR(0x0055f35c, float, gMotionTimeScale)
/* Result of the last sample call of the two-motion blends. */
#define gMotionSampleA ORIG_VAR(0x00601e14, ZMotionDelta *, gMotionSampleA)
/* Staging buffer a motion bank is read into (0x40000 bytes). */
typedef uint8_t ZMotionStaging[0x40000];
#define gMotionStaging ORIG_VAR(0x00601e1c, ZMotionStaging, gMotionStaging)

void zactorTableInit(void);
void zactorNop0(void);
void zactorCopy(ZActor *dst, const ZActor *src);
ZActor *zactorFindFree(void);
ZActor *zactorCreate(int type);
ZActor *zactorInit(int type, ZActor *a);
void zactorDestroy(ZActor *a);
void zactorClear(ZActor *a);
void zactorSetPos(ZActor *a, const ZVec3 *pos);
ZVec3 *zactorGetPos(ZActor *a);
void zactorSetHeading(ZActor *a, int angle);
ZAngle zactorGetHeading(const ZActor *a);
int zactorAngleToWorld(int angle);
int zactorAngleFromWorld(int angle);
void zactorSetScale(ZActor *a, const ZVec3 *scale);
ZVec3 *zactorGetScale(ZActor *a);
ZMotionDelta *zactorMotionSample(ZActor *a, float speed, uint32_t flags, ZMotionDelta *out, uint32_t channel);
void zactorMotionSkim(ZActor *a, ZAngle3 *joints, const ZMotion *m, int steps, uint8_t flags);
int zvec3IsZero(const ZVec3 *v);
ZMotionDelta *zactorPlayMotion(ZActor *a, const ZMotion *motion, float speed, uint32_t flags);
void zactorCopySecondaryJoints(const ZActor *a, const ZActor *b);
void zmotionAverageAngles(const ZAngle3 *a, const ZAngle3 *b, ZAngle3 *out, int count);
void zmotionBlendAngles(const ZAngle3 *a, const ZAngle3 *b, ZAngle3 *out, int count);
void zvec3Average(const ZVec3 *a, const ZVec3 *b, ZVec3 *out);
void zvec3Blend(const ZVec3 *a, const ZVec3 *b, ZVec3 *out);
ZActorRootPose *zactorGetRootPose(const ZActor *a);
int zmotionOpenLibrary(const char *name);
void zmotionCloseLibrary(void);
ZMotionBank *zmotionLoadBank(uint32_t id);
void zmotionLoadExtraBank(uint16_t id);
ZMotionBank *zmotionBankAdd(uint8_t *data, uint32_t size, int replaceFirst);
void zmotionNop(void);
ZMotionBank *zmotionBankGet(int index);
const ZMotion *zmotionFind(int id);
ZMotionEntry *zmotionBankNext(ZMotionBank *bank);
void zactorSetModel(ZActor *a, uint16_t model);
void zactorSetLodModel(ZActor *a, uint16_t lod, uint16_t model);
uint16_t zactorGetModel(const ZActor *a);
uint8_t zmotionMarkerCount(const ZMotion *m);
uint8_t zmotionGetMarkers(const ZMotion *m, ZMotionMarker *out);
uint8_t zmotionGetFirstMarker(const ZMotion *m, ZMotionMarker *out);
void zactorSetField172(ZActor *a, uint8_t v);
void zactorSetRadius(ZActor *a, float radius);
void zactorSetExtent(ZActor *a, const ZVec3 *extent);
void zactorSetAngleBase(uint16_t base, int flip);
int zactorOffsetAngle(int a, int b);
void zactorShow(ZActor *a);
void zactorHide(ZActor *a);
void zactorSetFlag2(ZActor *a);
void zactorClearFlag2(ZActor *a);
void zactorSetAttachment(ZActor *a, const ZVec3 *offset, int bone);
void zactorSetTag(ZActor *a, int tag);
void zactorClearTag(ZActor *a, int tag);
void zactorSetFlag20(ZActor *a);
void zactorClearFlag20(ZActor *a);
void zactorNop1(void);
void zmotionSetRate(int16_t rate);
void zactorNop2(void);
void zactorSetField174(ZActor *a, int v);
int zactorGetField174(const ZActor *a);
void zactorGetBasePos(const ZActor *a, ZVec3 *out);
void zactorGetHeadPos(const ZActor *a, ZVec3 *out);
void zactorGetBonePos(const ZActor *a, int bone, const ZVec3 *in, ZVec3 *out, uint8_t flags);
void zactorAddRiderYaw(ZActor *a, int16_t delta);
void zactorSetJointAngles(ZActor *a, uint32_t joint, const ZAngle3 *angles);
void zactorGetJointAngles(const ZActor *a, uint32_t joint, ZAngle3 *angles);
void zactorSetSkeleton(ZActor *a, int skeleton);
int zactorGetSkeleton(const ZActor *a);
void zactorSetHeight(ZActor *a, float height);
uint8_t zactorGetStepCount(const ZActor *a);
float zactorGetHeight(const ZActor *a);
void zactorGetRotation(const ZActor *a, ZAngle3 *out);
void zactorGetMatrix(const ZActor *a, ZMatrix *m);
void zactorSetRotation(ZActor *a, ZAngle3 *angles);
void zactorFaceMatrix(ZActor *a, const ZMatrix *m);
void zactorOrient(ZActor *a, const ZAngle3 *angles, const ZVec3 *up, const ZVec3 *rightIn, ZMatrix *out);
void zactorAnglesFromMatrix(ZActor *a, const ZMatrix *m);
void zactorBreak(void);
void zactorCopyRootAngles(ZActor *a, const ZActor *b);
void zactorCopyRotation(ZActor *a, const ZActor *b);
void zmotionLerpAngles(const ZAngle3 *a, const ZAngle3 *b, ZAngle3 *out, float t, uint8_t count);
void zmotionResetScale(ZAngle3 *angles, const void *unused, float *scale, uint8_t count);
ZMotionDelta **zactorBlendMotions(ZActor *a, const ZMotion *motionA, const ZMotion *motionB,
                                  const uint16_t *flags, float t);
ZMotionDelta *zactorSampleScaled(ZActor *a, const ZMotion *motion, uint16_t flags, float *scale);
void zmotionSelectJoints(const ZAngle3 *a, const ZAngle3 *b, ZAngle3 *out, const uint8_t *mask,
                         uint8_t count);
ZMotionDelta **zactorBlendMotionsMasked(ZActor *a, const ZMotion *motionA, const ZMotion *motionB,
                                        const uint16_t *flags, const uint8_t *mask);

#endif
