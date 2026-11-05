/*
 * Motion rigs and overlays (C:\zaxis\Bmx\src\com\zge\bmx\zoverii.c).
 *
 * Two unrelated halves share the file. The first is the position based "rig" that the rider and the bike
 * are thrown around with after a bail: a handful of points joined by distance sticks and orientation
 * hinges, relaxed once per frame, plus the limb target block of the rider's IK. The second is the screen
 * overlay layer: a double buffered pool of 0xa0 byte display packets (rectangles, gradients, sprites, one
 * per glyph of a text) addressed by 16 bit handles, which overlay.c draws its HUD and menus through.
 */
#ifndef BMX_COM_ZGE_BMX_ZOVERII_H
#define BMX_COM_ZGE_BMX_ZOVERII_H

#include <stdint.h>

#include "bmx/com/zge/zactor.h"
#include "compat/orig.h"
#include "zge/zmath.h"

/* ---- rig -------------------------------------------------------------------------------------- */

/* ZRigPoint.mode */
enum {
    kZoverPointPinned = 0, /* kinematic: placed from outside, never integrated */
    kZoverPointFree = 1,
    kZoverPointFree2 = 2, /* accepted by zoverRigAddPoint; integrates like kZoverPointFree */
};

enum {
    kZoverRigMaxPoints = 15,
    kZoverRigMaxConstraints = 22,
    kZoverRigMaxHinges = 7,
};

/* ZRigPoint.flags, set by zoverRigCollidePoint */
enum {
    kZoverPointTouching = 0x1, /* touched level geometry in the last relaxation (its velocity is kept) */
    kZoverPointStopped = 0x2,  /* came to rest on it (zoverRigResolvePoint leaves it alone) */
};

/* What zoverRigAddPoint reads. */
typedef struct ZRigPointDef {
    uint16_t mode;     /* 0x00 */
    uint8_t pad02[2];  /* 0x02 */
    ZVec3 pos;         /* 0x04 */
    uint8_t pad10[4];  /* 0x10 */
    float mass;        /* 0x14 */
    float friction;    /* 0x18 */
    float restitution; /* 0x1c */
} ZRigPointDef;

_Static_assert(sizeof(ZRigPointDef) == 0x20, "rig point definition size");

/* One simulated point. The basis is the orientation of the body part the point stands for. */
typedef struct ZRigPoint {
    uint16_t mode;     /* 0x00 */
    uint16_t flags;    /* 0x02 */
    ZMatrix basis;     /* 0x04 */
    float mass;        /* 0x44 */
    float weight;      /* 0x48 accumulated stiffness of the constraints relaxed this pass */
    float friction;    /* 0x4c */
    float restitution; /* 0x50 */
    ZVec3 pos;         /* 0x54 */
    uint8_t pad60[4];  /* 0x60 */
    ZVec3 prev;        /* 0x64 position before this frame's move */
    uint8_t pad70[4];  /* 0x70 */
    ZVec3 accum;       /* 0x74 mass weighted position sum built up while relaxing */
    uint8_t pad80[4];  /* 0x80 */
    ZVec3 vel;         /* 0x84 */
    uint8_t pad90[0xb4 - 0x90];
} ZRigPoint;

/* ZRigConstraint.type */
enum {
    kZoverStick = 0,       /* keep two points at their rest distance */
    kZoverStickLinked = 1, /* a stick whose stiffness grows with the alignment of two other points */
    kZoverCorner = 2,      /* remembered angle between two sticks; relaxing it is a no-op */
};

typedef struct ZRigConstraint {
    int32_t type;    /* 0x00 */
    float stiffness; /* 0x04 */
    ZRigPoint *a;    /* 0x08 */
    ZRigPoint *b;    /* 0x0c */
    union {
        struct {
            float length;    /* 0x10 rest distance */
            ZRigPoint *c;    /* 0x14 */
            ZRigPoint *d;    /* 0x18 */
            float alignGain; /* 0x1c */
        } stick;             /* types 0 and 1 */
        struct {
            uint16_t angle;   /* 0x10 acos of the angle between the two segments */
            uint8_t index[3]; /* 0x12 the three points that span the reference frame */
        } corner;             /* type 2 */
    } u;
} ZRigConstraint;

/* ZRigHinge.type */
enum {
    kZoverHingeSwing = 0, /* push a and b along the other's axis */
    kZoverHingeTwist = 7, /* push b along an axis of refA */
    kZoverHingeBone = 9,  /* pull b to a fixed distance below a along b's up axis */
};

/* What zoverRigAddHinge* reads: the axes are 1 right, 2 up, 3 at; 4, 5, 6 the same, negated. */
typedef struct ZRigHingeDef {
    int32_t axisA;   /* 0x00 axis of refA */
    int32_t axisB;   /* 0x04 axis of refB */
    ZRigPoint *a;    /* 0x08 */
    ZRigPoint *b;    /* 0x0c */
    ZRigPoint *c;    /* 0x10 */
    ZRigPoint *refA; /* 0x14 */
    ZRigPoint *refB; /* 0x18 */
    int32_t sign;    /* 0x1c 1: act only when the projection is not positive, 2: when it is not negative */
    float strength;  /* 0x20 */
} ZRigHingeDef;

typedef struct ZRigHinge {
    int32_t type;    /* 0x00 */
    ZRigPoint *a;    /* 0x04 */
    ZRigPoint *b;    /* 0x08 */
    ZRigPoint *c;    /* 0x0c */
    ZRigPoint *refA; /* 0x10 */
    ZRigPoint *refB; /* 0x14 */
    int32_t sign;    /* 0x18 */
    int32_t axisA;   /* 0x1c */
    int32_t axisB;   /* 0x20 */
    float strength;  /* 0x24 (for bones: the distance) */
} ZRigHinge;

/* ZRigHit.kind */
enum {
    kZoverHitLevel = 1,  /* level geometry: matRef is set */
    kZoverHitObject = 2, /* a prop's face: object and tag are set */
};

/* What zoverRigCollidePoint reports about a point's contacts with the level. */
typedef struct ZRigHit {
    uint32_t kind;          /* 0x00 kind of the last face that counted */
    uint8_t pad04[4];       /* 0x04 */
    ZVec3 point;            /* 0x08 where the point was put */
    uint8_t pad14[8];       /* 0x14 */
    ZVec3 normal;           /* 0x1c average plane of the faces */
    float d;                /* 0x28 */
    uint8_t pad2c[0xc];     /* 0x2c */
    const uint32_t *matRef; /* 0x38 zbspGetMatRef of a level face */
    uint16_t tag;           /* 0x3c first word of the object */
    uint8_t pad3e[2];       /* 0x3e */
    const void *object;     /* 0x40 */
    uint8_t pad44[0xd8 - 0x44];
} ZRigHit;

BMX_LAYOUT32_ASSERT(sizeof(ZRigHit) == 0xd8, "rig hit size");
_Static_assert(__builtin_offsetof(ZRigHit, normal) == 0x1c, "rig hit layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZRigHit, object) == 0x40, "rig hit layout");

struct ZRig;
/* Called when a point hits level geometry during zoverRigRelax, with the impact speed along the normal. */
typedef void(BMX_CDECL *ZRigHitFn)(struct ZRig *rig, uint32_t index, float impact, ZRigHit *hit);

typedef struct ZRig {
    uint32_t pointCount;                                 /* 0x000 */
    ZRigPoint points[kZoverRigMaxPoints];                /* 0x004 */
    uint32_t constraintCount;                            /* 0xa90 */
    ZRigConstraint constraints[kZoverRigMaxConstraints]; /* 0xa94 */
    uint32_t hingeCount;                                 /* 0xd54 */
    ZRigHinge hinges[kZoverRigMaxHinges];                /* 0xd58 */
    ZVec3 center;                                        /* 0xe70 centre of mass of the points */
    uint8_t padE7c[4];
    ZRigHitFn onHit;    /* 0xe80 */
    uint32_t hingesOff; /* 0xe84 non-zero: skip the hinges */
} ZRig;

_Static_assert(sizeof(ZRigPoint) == 0xb4, "rig point size");
BMX_LAYOUT32_ASSERT(sizeof(ZRigConstraint) == 0x20, "rig constraint size");
BMX_LAYOUT32_ASSERT(sizeof(ZRigHinge) == 0x28, "rig hinge size");
BMX_LAYOUT32_ASSERT(sizeof(ZRigHingeDef) == 0x24, "rig hinge definition size");
_Static_assert(__builtin_offsetof(ZRigPoint, mass) == 0x44, "rig point layout");
_Static_assert(__builtin_offsetof(ZRigPoint, pos) == 0x54, "rig point layout");
_Static_assert(__builtin_offsetof(ZRigPoint, prev) == 0x64, "rig point layout");
_Static_assert(__builtin_offsetof(ZRigPoint, accum) == 0x74, "rig point layout");
_Static_assert(__builtin_offsetof(ZRigPoint, vel) == 0x84, "rig point layout");
_Static_assert(__builtin_offsetof(ZRig, points) == 4, "rig layout");
_Static_assert(__builtin_offsetof(ZRig, constraintCount) == 0xa90, "rig layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZRig, hingeCount) == 0xd54, "rig layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZRig, center) == 0xe70, "rig layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZRig, onHit) == 0xe80, "rig layout");
BMX_LAYOUT32_ASSERT(sizeof(ZRig) == 0xe88, "rig size");

/* The rider rig and the bike rig; the zoverRig* state functions work on the one selected by zoverRigSelect.
 */
typedef ZRig *ZoverRigTable[2];
typedef int32_t ZoverRigFlags[2];
typedef int16_t ZoverRigAngles[2];
typedef int32_t ZoverIndices6[6];
typedef int32_t ZoverIndices8[8];
typedef int32_t ZoverIndices3[3];
#define gZoverRigs         ORIG_VAR(0x00574bc4, ZoverRigTable, gZoverRigs)
#define gZoverRigIndex     ORIG_VAR(0x00574ca8, int32_t, gZoverRigIndex)
#define gZoverRigNext      ORIG_VAR(0x00574ca4, int32_t, gZoverRigNext) /* slot the next rider rig goes in */
#define gZoverHandsGrabbed ORIG_VAR(0x0057491c, ZoverRigFlags, gZoverHandsGrabbed)
#define gZoverHandsHolding ORIG_VAR(0x00574924, ZoverRigFlags, gZoverHandsHolding)
#define gZoverHandsEased   ORIG_VAR(0x005748d4, ZoverRigFlags, gZoverHandsEased)
#define gZoverLimp         ORIG_VAR(0x00574930, ZoverRigFlags, gZoverLimp)
#define gZoverStiff        ORIG_VAR(0x0057473c, ZoverRigFlags, gZoverStiff)
#define gZoverRelaxed      ORIG_VAR(0x00574734, ZoverRigFlags, gZoverRelaxed)
#define gZoverSticky       ORIG_VAR(0x00574ad4, ZoverRigFlags, gZoverSticky)
#define gZoverPitchOn      ORIG_VAR(0x00574b24, ZoverRigFlags, gZoverPitchOn)
#define gZoverYawOn        ORIG_VAR(0x00574b1c, ZoverRigFlags, gZoverYawOn)
#define gZoverPitch        ORIG_VAR(0x00574cac, ZoverRigAngles, gZoverPitch)
#define gZoverYaw          ORIG_VAR(0x00574cb0, ZoverRigAngles, gZoverYaw)
/* Hinge and constraint numbers of the rider rig that the state functions adjust. */
#define gZoverHingeIdx ORIG_VAR(0x00574cb4, ZoverIndices6, gZoverHingeIdx)
#define gZoverStickIdx ORIG_VAR(0x00574ccc, ZoverIndices8, gZoverStickIdx)
#define gZoverBackIdx  ORIG_VAR(0x00574cec, ZoverIndices3, gZoverBackIdx)

/* Polls the input devices (called from the timer callback). */
ORIG_FN(0x0046e010, void, zoverPollInput, (void));

/*
 * A stdcall callback. The label keeps the symbol undecorated: the generated hook table declares every
 * ported function as a plain `void f(void)` and so refers to `_name`, not `_name@20`.
 */
#if defined(BMX_HOOK)
uint32_t BMX_STDCALL zoverPollTimer(uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                                    uint32_t e) __asm__(BMX_SYM("zoverPollTimer"));
#else
uint32_t BMX_STDCALL zoverPollTimer(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
#endif
ZRig *zoverRigCreate(void);
int zoverRigAddPoint(ZRig *rig, const ZRigPointDef *def);
int zoverRigAddStick(ZRig *rig, int a, int b, float stiffness);
void zoverRigAddStickLinked(ZRig *rig, int a, int b, int c, int d, float stiffness, float alignGain);
void zoverRigAddCorner(ZRig *rig, int a, int b, int i0, int i1, int i2, float stiffness);
int zoverRigAddHingeSwing(ZRig *rig, const ZRigHingeDef *def);
int zoverRigAddHingeTwist(ZRig *rig, const ZRigHingeDef *def);
void zoverRigAddBone(ZRig *rig, const ZRigHingeDef *def);
void zoverRigSolveHingeSwing(ZRigHinge *h);
void zoverRigSolveHingeTwist(ZRigHinge *h);
void zoverRigSolveBone(ZRigHinge *h);
void zoverRigSolveHinges(ZRig *rig);
void zoverRigRelax(ZRig *rig);
void zoverRigSetHitCallback(ZRig *rig, ZRigHitFn fn);
void zoverRigBuildFrame(ZRig *rig, int a, int b, int c, ZMatrix *out);
int zoverRigCollidePoint(ZRigPoint *p, float *impact, ZRigHit *hit);
void zoverRigResolvePoint(ZRigPoint *p);
void zoverRigProject(const ZVec3 *a, const ZVec3 *b, float depth, float *outMid, float *outDepth,
                     const ZVec3 *n);
void zoverRigSolveStick(ZRigConstraint *c);
void zoverRigSolveStickLinked(ZRigConstraint *c);
void zoverRigSolveCorner(ZRigConstraint *c);
ZRig *zoverRigCreateTriangle(void);

/* The rider and bike motion objects are rigs; callers hold them as plain pointers. */
void zoverStartBikeMotion(void *rig, void *bike, const ZMatrix *m, const ZVec3 *vel, const ZAngle3 *rot);
void zoverStepBikeMotion(void *rig, void *bike);
/* Only rig, rider and vel are read; the matrix and angle parameters exist in the original signature. */
void zoverStartRiderMotion(void *rig, void *rider, const ZMatrix *m, const ZVec3 *vel, const ZAngle3 *rot);
void zoverZeroAngles(ZAngle3 *angles);
void zoverRigLoadPose(ZRig *rig, ZActor *rider);
ZRig *zoverRigCreateRider(ZActor *rider, uint32_t flags);
void zoverRigSelect(ZRig *rig);
void zoverRigEase(ZRig *rig, ZActor *rider);
void zoverRigGrabEdges(ZRig *rig);
int zoverRigHandsHolding(void);
void zoverRigReleaseHands(ZRig *rig);
void zoverBlendVec(ZVec3 *a, ZVec3 *b);
void zoverRigAimHands(ZRig *rig);
void zoverRigGoStiff(ZRig *rig, ZActor *rider);
void zoverRigEndStiff(ZRig *rig);
void zoverRigBlendToPose(ZRig *rig);
void zoverRigGoLimp(ZRig *rig, ZActor *rider);
void zoverRigEndLimp(ZRig *rig);
void zoverRigClearHinges(ZRig *rig);
void zoverRigRequestSticky(ZRig *rig);
void zoverRigClearSticky(ZRig *rig);
void zoverRigFreezeFlagged(ZRig *rig);
void zoverRigRequestRelaxed(ZRig *rig);
void zoverRigClearRelaxed(ZRig *rig);
void zoverRigApplyRotation(ZRig *rig, ZActor *rider, const ZMatrix *rot);
void zoverRigApplyPitch(ZRig *rig, ZActor *rider);
void zoverRigRequestPitch(ZRig *rig, ZActor *rider, int16_t angle);
void zoverRigClearPitch(ZRig *rig);
void zoverRigApplyYaw(ZRig *rig, ZActor *rider);
void zoverRigRequestYaw(ZRig *rig, ZActor *rider, int16_t angle);
void zoverRigClearYaw(ZRig *rig);

/* Frame and vector helpers of the rider rig step (zoverStepRiderMotion). */
void zoverVecSub(const ZVec3 *a, const ZVec3 *b, ZVec3 *out);
void zoverCrossNeg(const ZVec3 *a, const ZVec3 *b, ZVec3 *out);
void zoverTransposeRotation(const ZMatrix *in, ZMatrix *out);
void zoverFrameFromPoints(const ZRig *rig, int i, int j, int k, int l, ZMatrix *out);
void zoverFrameFollow(const ZRig *rig, int ia, int ib, const ZMatrix *ref, ZMatrix *out);
void zoverFrameFollowFlipped(const ZRig *rig, int ia, int ib, const ZMatrix *ref, ZMatrix *out);
void zoverFrameSnap(const ZRig *rig, int ia, int ib, const ZMatrix *ref, ZMatrix *out);
void zoverFrameFromUp(const ZRig *rig, ZVec3 *d, const ZMatrix *ref, ZMatrix *out);
void zoverFrameReflect(const ZRig *rig, int ia, int ib, const ZMatrix *ref, ZMatrix *out, int flip);
void zoverMul3(ZMatrix *out, const ZMatrix *a, const ZMatrix *b);
void zoverMatrixToAngles(const ZMatrix *m, ZAngle3 *out);
void zoverActorAnglesFromFrame(ZActor *actor, const ZMatrix *frame, ZAngle3 *out);
void zoverStepRiderMotion(void *rigp, void *actorp);

/* ---- limbs ------------------------------------------------------------------------------------ */

/* One IK target: where a limb's end should go and the bend direction. */
typedef struct ZLimbTarget {
    ZVec3 a;
    uint8_t pad0c[4];
    ZVec3 b;
    uint8_t pad1c[4];
} ZLimbTarget;

/* Limb target block of the rider IK (PhysRider.ik). */
typedef struct ZLimbs {
    uint8_t pad000[0x110];
    uint32_t field110; /* 0x110 */
    uint8_t pad114[0x11c - 0x114];
    float length[8];       /* 0x11c bone lengths: parts 4, 5, 0xb, 0xa, 7, 8, 0xe, 0xd of the rider actor */
    float reachSum[4];     /* 0x13c */
    float blend;           /* 0x14c */
    float epsilon;         /* 0x150 */
    uint32_t mask;         /* 0x154 limbs that have a target */
    ZLimbTarget target[4]; /* 0x158 */
} ZLimbs;

_Static_assert(__builtin_offsetof(ZLimbs, length) == 0x11c, "limbs layout");
_Static_assert(__builtin_offsetof(ZLimbs, blend) == 0x14c, "limbs layout");
_Static_assert(__builtin_offsetof(ZLimbs, target) == 0x158, "limbs layout");
_Static_assert(sizeof(ZLimbs) == 0x1d8, "limbs size");

void zoverSetLimbBlend(ZLimbs *limbs, float blend, float epsilon);
void zoverLimbsSnap(ZLimbs *limbs);
void zoverLimbsInit(ZActor *rider, ZLimbs *limbs);
void zoverCombine3(const float *w, const ZVec3 *a, const ZVec3 *b, const ZVec3 *c, ZVec3 *out);
/* Stores a limb target pair in the limb block for every bit of `mask` (1, 2, 4, 8). */
void zoverSetLimbTargets(void *limbs, const ZVec3 *a, const ZVec3 *b, uint32_t mask);
void zoverResetLimbs(void *limbs);
/* Empty in the shipped build. */
void zoverLimbsBegin(void *actor, void *limbs);
void zoverSolveLimbs(void *actorp, void *limbsp);

/* ---- overlays --------------------------------------------------------------------------------- */

/*
 * A display packet: a link word (low 28 bits: address of the next packet in the ordering table, top nibble:
 * length of the packet's command part, 0 = not drawn), then a GPU style command (colour in bytes 4 to 6,
 * command in byte 7) with its vertices, and at 0x9c the overlay's own bookkeeping.
 */
typedef union ZOverItem {
    uint8_t b[0xa0];
    uint16_t h[0x50];
    uint32_t w[0x28];
} ZOverItem;

_Static_assert(sizeof(ZOverItem) == 0xa0, "overlay item size");

/* ZOverItem bytes. */
enum {
    kZoverItemKind = 0x9c,  /* kZoverKind*, 0 = free */
    kZoverItemFlags = 0x9d, /* bits 0 to 2 alpha mode, bit 7 on the dirty list */
    kZoverItemNext = 0x4f,  /* h[]: handle of the next glyph of a text, 0xffff = last */
};

enum {
    kZoverKindFree = 0,
    kZoverKindSprite = 2,
    kZoverKindRect = 3,
    kZoverKindGradient = 4,
    kZoverKindGlyph = 5,
};

/* One glyph of a font: its place in the font image, and its width. */
typedef struct ZOverGlyph {
    uint16_t x;     /* 0x00 */
    uint16_t y;     /* 0x02 */
    uint16_t width; /* 0x04 */
    uint16_t pad06; /* 0x06 */
} ZOverGlyph;

/* A font record (filled by zoverParseFont); `image` points at a u16 index into the sprite table. */
typedef struct ZOverFont {
    const uint16_t *image;    /* 0x00 */
    uint8_t pad04;            /* 0x04 */
    uint8_t height;           /* 0x05 */
    int8_t firstChar;         /* 0x06 */
    uint8_t charCount;        /* 0x07 */
    const ZOverGlyph *glyphs; /* 0x08 */
} ZOverFont;

/* Entry of the sprite table at 0x68c3e0 (also the image of every font). */
typedef struct ZOverSprite {
    uint16_t tpage;   /* 0x00 */
    uint16_t clut;    /* 0x02 */
    uint8_t u0, v0;   /* 0x04 */
    uint8_t u1, v1;   /* 0x06 */
    uint8_t pad08[4]; /* 0x08 */
    uint8_t width;    /* 0x0c */
    uint8_t height;   /* 0x0d */
    uint8_t pad0e[2]; /* 0x0e */
    uint32_t mode;    /* 0x10 */
} ZOverSprite;

_Static_assert(sizeof(ZOverSprite) == 0x14, "overlay sprite size");

/* Scratch record the text drawer hands to the glyph renderer (0x55b960). */
typedef struct ZOverGlyphRecord {
    uint8_t pad00[6];
    uint16_t image;  /* 0x06 */
    uint16_t x;      /* 0x08 glyph position in the image */
    uint16_t y;      /* 0x0a */
    uint16_t width;  /* 0x0c */
    uint16_t height; /* 0x0e */
    uint16_t color;  /* 0x10 */
    uint16_t flags;  /* 0x12 */
} ZOverGlyphRecord;

/* Item pools, two frames each: kind 0 (rectangles), 1 (glyphs), 2 and 3 (sprites). The last item of a pool is
 * a terminator. */
typedef ZOverItem ZOverPool0[2][16];
typedef ZOverItem ZOverPool1[2][128];
typedef ZOverItem ZOverPool2[2][28];
typedef ZOverItem ZOverPool3[2][400];
typedef int16_t ZOverHandleList[0x23c];
typedef struct ZOverTerminator {
    uint32_t link;
    uint32_t command;
} ZOverTerminator;
typedef ZOverTerminator ZOverTerminators[2];
typedef const ZOverFont *ZOverFontTable[8];
typedef struct ZOverRectSize {
    uint32_t w, h;
} ZOverRectSize;

#define gZoverPool0       ORIG_VAR(0x005a0a38, ZOverPool0, gZoverPool0)
#define gZoverPool1       ORIG_VAR(0x00596a38, ZOverPool1, gZoverPool1)
#define gZoverPool2       ORIG_VAR(0x00594270, ZOverPool2, gZoverPool2)
#define gZoverPool3       ORIG_VAR(0x00574e70, ZOverPool3, gZoverPool3)
#define gZoverTerminators ORIG_VAR(0x00596a20, ZOverTerminators, gZoverTerminators)
/* Handles waiting to be freed after the next flush, and items that changed this frame. */
#define gZoverFreeList   ORIG_VAR(0x005965a4, ZOverHandleList, gZoverFreeList)
#define gZoverDirtyList  ORIG_VAR(0x005a1e38, ZOverHandleList, gZoverDirtyList)
#define gZoverBusy       ORIG_VAR(0x005a22b0, uint32_t, gZoverBusy)
#define gZoverDirtyCount ORIG_VAR(0x005a22b4, uint32_t, gZoverDirtyCount)
#define gZoverFreeCount  ORIG_VAR(0x005a22b8, uint32_t, gZoverFreeCount)
/* The pool frame being written (0 or 1) and the layer set the flush chains (0 pools 0 and 1, 1 pools 2 and
 * 3). */
#define gZoverFrame ORIG_VAR(0x005a22bc, uint8_t, gZoverFrame)
#define gZoverLayer ORIG_VAR(0x005a22bd, uint8_t, gZoverLayer)
/* The ordering table head the flush chains its packets into (a pointer to the head word). */
#define gZoverOt        ORIG_VAR(0x0055c698, uint32_t *, gZoverOt)
#define gZoverFontCount ORIG_VAR(0x00596a30, uint8_t, gZoverFontCount)
#define gZoverFonts     ORIG_VAR(0x00574e50, ZOverFontTable, gZoverFonts)
#define gZoverRectSize  ORIG_VAR(0x00596570, ZOverRectSize, gZoverRectSize)
#define gZoverGlyph     ORIG_VAR(0x0055b960, ZOverGlyphRecord, gZoverGlyph)
#define gZoverSprites   ORIG_VAR(0x0068c3e0, ZOverSprite, gZoverSprites)

int zoverInit(void);
/* Presents the overlays of the frame. */
int zoverFlush(void);
int zoverSetLayerFlag(uint32_t flag);
int zoverGetPool(uint32_t hi, uint32_t lo, uint32_t frame, ZOverItem **base, uint32_t *count, uint32_t *kind);
/* Allocates a free item of the pool (layer hi, kind lo); returns its handle or 0xffff. */
uint32_t zoverAllocItem(uint32_t hi, uint32_t lo);
/* Removes an overlay (a whole text if it is the first glyph); 0xffff is ignored. */
void zoverRemove(uint32_t handle);
/*
 * Shows (show != 0) or hides (show == 0) an overlay by handle; 0xffff is ignored. Only the low 16 bits of
 * the handle are read.
 */
int zoverShow(uint32_t handle, int show);
/* Adds a filled rectangle overlay and returns its handle. */
uint32_t zoverAddRect(int layer, int x, int y, int w, int h, uint32_t color);
/* Moves and resizes an overlay rectangle. */
int zoverSetRect(uint32_t handle, int x, int y, int w, int h);
int zoverSetRectColor(uint32_t handle, uint32_t color);
/* Draws a filled rectangle for this frame only. */
void zoverDrawRect(int x, int y, int w, int h, uint32_t color);
/* Adds a rectangle overlay with a vertical gradient. */
uint32_t zoverAddGradient(int layer, int x, int y, int w, int h, uint32_t top, uint32_t bottom);
uint32_t zoverAddSprite(int layer, uint32_t id, int x, int y, uint32_t color);
/* Adds a text overlay, one packet per glyph, and returns its handle (0xffff on failure). */
uint32_t zoverAddText(int layer, const void *font, const char *text, int x, int y, uint32_t color, int clut);
/* Changes colour and clut of a text overlay (font by pointer; only the low 16 bits of handle are read). */
int zoverSetColor(uint32_t handle, const void *font, uint32_t color, int clut);
int zoverMoveText(uint32_t handle, int x, int y);
/* Draws a text for this frame only. */
void zoverDrawText(const void *font, const char *text, int x, int y, uint32_t color, int flags);
/* Width of a text in pixels. */
int16_t zoverTextWidth(const char *text, const void *font);
/* Parses a font resource into a font record; returns 0 on success. */
int zoverParseFont(const void *data, void *font);

/* Rider and bike motion objects (the animation layer under the rider actors). */
ORIG_FN(0x00452e80, void *, zoverCreateRiderMotion, (void *actor, uint32_t flags));
ORIG_FN(0x004526a0, void *, zoverCreateBikeMotion, (void));
/* Registers the callback the motion object calls when a body part hits something. */
/* What zoverSetHitCallback installs: the motion object's rig hit callback. */
typedef ZRigHitFn ZoverHitFn;
ORIG_FN(0x00451280, void, zoverSetHitCallback, (void *motion, ZoverHitFn fn));
ORIG_FN(0x00454400, void, zoverMotionSetupA, (void *motion, void *actor));
ORIG_FN(0x00454430, void, zoverMotionSetupB, (void *motion, void *actor));
ORIG_FN(0x00455f10, void, zoverReadLimbs, (void *actor, void *limbs));
ORIG_FN(0x00454140, void, zoverRiderPoseA, (void *motion, void *actor));
ORIG_FN(0x00453b70, void, zoverRiderPoseB, (void *motion, void *actor));
ORIG_FN(0x004546a0, void, zoverRiderPoseC, (void *motion, void *actor));
ORIG_FN(0x00454790, void, zoverRiderPoseD, (void *motion, void *actor));
ORIG_FN(0x00453890, void, zoverRiderPoseE, (void *motion, void *actor));

#endif
