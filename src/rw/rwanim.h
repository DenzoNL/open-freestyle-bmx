/*
 * RenderWare 3.1 animation plugin (rw/plugins.c, plugin id 0x108): sequences of keyframes per frame, the
 * named sequences of a clump that animate its frames, their playback and their stream chunks.
 *
 * Every frame keeps translation sequences (list A) and rotation sequences (list B); a clump sequence collects
 * a node per frame that has a frame sequence of its name. Playing a sequence moves the instances of its nodes
 * through their keys and poses the frames (rotation by slerp of quaternions, translation linearly).
 */
#ifndef RW_RWANIM_H
#define RW_RWANIM_H

#include "rw/rwmath.h"
#include "rw/rwplugins.h"
#include "rw/rwstream.h"

/*
 * A sequence of one frame (0x1c bytes, reference counted by the frames that have it): keyframes (type 1:
 * translations of 12 bytes, type 2: rotation quaternions of 16 bytes) and the keys that interpolate between
 * two keyframes over a time (RpAnimKey, 0xc bytes).
 */
typedef struct RpAnimFrameSequence {
    int32_t refCount;  /* 0x00 */
    char *name;        /* 0x04 */
    int32_t type;      /* 0x08 */
    int32_t numFrames; /* 0x0c */
    void *frames;      /* 0x10 */
    int32_t numKeys;   /* 0x14 */
    uint8_t *keys;     /* 0x18 */
} RpAnimFrameSequence;

typedef struct RpAnimKey {
    int32_t start;  /* keyframe index */
    int32_t end;    /* keyframe index */
    float duration; /* 0x08 */
} RpAnimKey;

_Static_assert(sizeof(RpAnimFrameSequence) == 0x1c, "RpAnimFrameSequence layout");
_Static_assert(sizeof(RpAnimKey) == 0xc, "RpAnimKey layout");

typedef struct RpAnimFrameLink {
    RpAnimFrameSequence *sequence;
    struct RpAnimFrameLink *next;
} RpAnimFrameLink;

/* What the plugin keeps per frame (the pointer at RwFrame + rpAnimFrameOffset): its translation sequences
 * (list A) and rotation sequences (list B) with their counts; the links come from the plugin's free list. */
typedef struct RpAnimFrameData {
    int32_t numA;           /* 0x00 */
    RpAnimFrameLink *listA; /* 0x04 */
    int32_t numB;           /* 0x08 */
    RpAnimFrameLink *listB; /* 0x0c */
} RpAnimFrameData;

/* A frame's sequence in play: the sequence, its current key and the time into it (0xc bytes, malloc'd). */
typedef struct RpAnimInstance {
    RpAnimFrameSequence *sequence;
    uint8_t *keys; /* RpAnimKey */
    float time;
} RpAnimInstance;

/* The rotation interpolation between two quaternions (x, y, z, w): both scaled by 1 / sin(angle) unless they
 * are close enough to blend linearly (0x28 bytes, filled by 0x51ce10). */
typedef struct RpAnimSlerpCache {
    float q0[4];    /* 0x00 */
    float q1[4];    /* 0x10 */
    float angle;    /* 0x20 */
    int32_t linear; /* 0x24 */
} RpAnimSlerpCache;

_Static_assert(sizeof(RpAnimSlerpCache) == 0x28, "RpAnimSlerpCache layout");

/* One animated frame of a clump sequence (0x3c bytes, 0x4fc4f0). 0x5029e0 frees the instances (from +4). */
typedef struct RpAnimNode {
    RwFrame *frame;          /* 0x00 */
    int32_t state;           /* 0x04 bit 0: updated, bit 1: the rotation key changed (the cache is stale) */
    RpAnimSlerpCache cache;  /* 0x08 */
    RpAnimInstance *a;       /* 0x30 from the frame's list A */
    RpAnimInstance *b;       /* 0x34 from list B */
    struct RpAnimNode *next; /* 0x38 */
} RpAnimNode;

/* A named sequence of a clump (0x24 bytes): the nodes of the frames that have it and its play callback. */
typedef struct RpAnimClumpSequence {
    char *name;                       /* 0x00 */
    float duration;                   /* 0x04 the longest of the frames' key time sums */
    float time;                       /* 0x08 */
    int32_t direction;                /* 0x0c 1: forward */
    int32_t priority;                 /* 0x10 frames take it over from sequences of lower priority */
    RpAnimNode *nodes;                /* 0x14 */
    void *callBack;                   /* 0x18 */
    void *data;                       /* 0x1c */
    struct RpAnimClumpSequence *next; /* 0x20 */
} RpAnimClumpSequence;

/* The plugin's data in a clump (at rpAnimClumpOffset): the frames, and two sequence lists behind a head. */
typedef struct RpAnimClumpData {
    int32_t numSequences;           /* 0x00 */
    RwFrameList frameList;          /* 0x04 */
    RpAnimClumpSequence *sequences; /* 0x0c head element, the new ones go after it */
    RpAnimClumpSequence *others;    /* 0x10 */
} RpAnimClumpData;

_Static_assert(sizeof(RpAnimNode) == 0x3c, "RpAnimNode layout");
_Static_assert(sizeof(RpAnimClumpSequence) == 0x24, "RpAnimClumpSequence layout");

/* The walk's data for the children of the clump's frame (0x4fc540 appends their nodes). */
typedef struct RpAnimNodeSearch {
    RpAnimNode *nodes;
    const char *name;
} RpAnimNodeSearch;

/* The anim plugin data of a frame (at rpAnimFrameOffset): the sequence lists, a word, and the user data. */
typedef struct RpAnimFramePlugin {
    struct RpAnimFrameData *data; /* 0x00 */
    int32_t priority;             /* 0x04 of the sequence that moved it last */
    uint32_t userData;            /* 0x08 -1 until set */
} RpAnimFramePlugin;

/* The interpolator state of a node as the update routines get it (RpAnimNode + 4). */
typedef struct RpAnimInterpolator {
    int32_t state;          /* 0x00 */
    RpAnimSlerpCache cache; /* 0x04 */
    RpAnimInstance *a;      /* 0x2c translation */
    RpAnimInstance *b;      /* 0x30 rotation */
} RpAnimInterpolator;

_Static_assert(offsetof(RpAnimNode, a) - offsetof(RpAnimNode, state) == offsetof(RpAnimInterpolator, a),
               "RpAnimInterpolator layout");
_Static_assert(offsetof(RpAnimNode, b) - offsetof(RpAnimNode, state) == offsetof(RpAnimInterpolator, b),
               "RpAnimInterpolator layout");

/* The plugin's engine data (the link free list) and open count. */
#define rpAnimEngineOffset ORIG_VAR(0x00678280, int32_t, rpAnimEngineOffset)
#define rpAnimModuleCount  ORIG_VAR(0x00678284, int32_t, rpAnimModuleCount)

/* The data the bone frame search callback of RpSkinClumpUpdate works with. */
typedef struct RpSkinChildSearch {
    uint32_t reserved;
    RwMatrix *parentInverse;
    uint32_t zero;
} RpSkinChildSearch;

/* Frame sequences. */
RpAnimFrameSequence *rpAnimSequenceDeinit(RpAnimFrameSequence *sequence);
RpAnimFrameSequence *rpAnimSequenceInit(RpAnimFrameSequence *sequence, const char *name, int32_t type,
                                        int32_t numFrames, int32_t numKeys);
__attribute__((regparm(3))) RpAnimFrameSequence *rpAnimSequenceCreate(const char *name, int32_t type,
                                                                      int32_t numFrames, uint32_t unused0,
                                                                      uint32_t unused1, uint32_t unused2,
                                                                      int32_t numKeys);
__attribute__((regparm(1))) int rpAnimSequenceRelease(RpAnimFrameSequence *sequence);

/* The frame plugin: data, constructor / destructor / copy and the stream chunk. */
struct RpAnimFrameData *rpAnimFrameDataCreate(void);
__attribute__((regparm(1))) int rpAnimFrameDataDestroy(RpAnimFrameData *data);
void *rpAnimFrameDeinit(void *frame);
__attribute__((regparm(2))) void *rpAnimFrameAddSequence(void *frame, RpAnimFrameSequence *sequence);
void *rpAnimFrameCopy(void *destination, const void *source);
void *rpAnimFrameChunkRead(void *stream, int32_t length, void *frame);
void *rpAnimFrameChunkWrite(void *stream, int32_t length, const void *frame);
__attribute__((regparm(1))) RpAnimFrameSequence *rpAnimSequenceRead(RwStream *stream);
__attribute__((regparm(3))) RpAnimFrameSequence *rpAnimSequenceWrite(RpAnimFrameSequence *sequence,
                                                                     RwStream *stream, int32_t *size);
__attribute__((regparm(3))) void *rpAnimWriteRotations(const void *frames, int32_t count, RwStream *stream,
                                                       uint32_t unused0, uint32_t unused1, uint32_t unused2,
                                                       int32_t *size);
__attribute__((regparm(3))) void *rpAnimWriteTranslations(const void *frames, int32_t count, RwStream *stream,
                                                          uint32_t unused0, uint32_t unused1,
                                                          uint32_t unused2, int32_t *size);

/* The clump plugin. */
RpAnimNode *rpAnimNodeCreate(void);
void *rpAnimClumpFindNodes(void *frame, void *search);
RpAnimClumpData *rpAnimClumpDataInit(RpAnimClumpData *data);
RpAnimClumpData *rpAnimClumpDataDeinit(RpAnimClumpData *data);
void *rpAnimClumpInit(void *clump);
void *rpAnimClumpDeinit(void *clump);

/*
 * Playback. The x87 entries take register arguments and a value in ST0 (asm entries, see rw/plugins.c):
 * rpAnimInstanceReset (EAX = instance, EDX = direction, ST0 = time), rpAnimRotationUpdate (EAX = instance,
 * EDX = interpolator, ECX = frame, 4th stack slot = direction, ST0 = dt), rpAnimInterpolatorUpdate (EAX =
 * interpolator, EDX = frame, ECX = direction, ST0 = dt), rpAnimClumpStartSequence (EAX = clump, EDX = name,
 * ECX = direction, 4th stack slot = priority, ST0 = time).
 */
RpAnimInstance *rpAnimInstanceReset(void);
RpAnimInstance *rpAnimRotationUpdate(uint32_t unused0, uint32_t unused1, uint32_t unused2, int32_t forward);
RpAnimInterpolator *rpAnimInterpolatorUpdate(void);
RpClump *rpAnimClumpStartSequence(uint32_t unused0, uint32_t unused1, uint32_t unused2, int32_t priority);
void rpAnimSlerpCacheSetup(const float *q0, const float *q1, RpAnimSlerpCache *cache);
RpClump *rpAnimClumpDefaultCallBack(RpClump *clump, RpAnimClumpSequence *sequence, float time, void *data);

/* The bone plugin's matrix walk and geometry chunk reader. */
void *rpBoneUpdateMatrices(void *frame, void *data);
void *rpBoneGeometryRead(void *stream, int32_t length, void *geometry);

#endif
