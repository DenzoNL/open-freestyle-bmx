/*
 * Shared fixtures of the camera / comdata / controller / effect / prop / replay / save tests: one log that
 * the stubs of the callees write into (compared like any other memory) and the stub groups a case asks
 * for. Every logged word is canonicalised or a plain value; stubs never log uninitialised bytes.
 */
#ifndef DT_GAMEPLAY_H
#define DT_GAMEPLAY_H

#include <stdint.h>

#include "dt.h"

/* Stub groups. */
enum {
    DTGP_DEBUG = 1u << 0,  /* zdebugFail, gZdebugPrint */
    DTGP_ACTOR = 1u << 1,  /* zactorShow / Hide, horizon on / off, comdataSetView */
    DTGP_SOUND = 1u << 2,  /* bmxSoundPlayAt */
    DTGP_CAMERA = 1u << 3, /* zobjCameraSetFov / LookAt, ngifFillScreenBar */
    DTGP_PAD = 1u << 4,    /* zpadCreateDevice / Destroy */
    DTGP_FX = 1u << 5,     /* effect engine: node lookup, create, name table call, particles */
    DTGP_PROP = 1u << 6,   /* prop callees: scene objects, goals, world object signals, shards */
    DTGP_ZMEM = 1u << 7,   /* static region: mark, alloc, release */
    DTGP_GAME = 1u << 8,   /* gameGetMode / SubMode, comdataGetGoalInfo */
    DTGP_FILE = 1u << 9,   /* zfile layer of the save file */
    DTGP_UI =
        1u << 10, /* overlay text drawing, string table, pad presses, overlay and game calls of the screens */
};

/* Scripted answers of the stubs. */
typedef struct DtgpScript {
    uint32_t padResults[2]; /* zpadCreateDevice(kind 2), (kind 0): returned device or 0 */
    void *node;             /* ngiffxGetNode result */
    uint32_t sparkWords;    /* how many words of the "spark" block the name table stub logs */
    void *position;         /* zobjGetPosition result */
    uint32_t goalResult;    /* gameCheckPropGoal result */
    uint32_t staticFail;    /* zmemStaticAlloc fails (returns NULL) */
    uint32_t mode;          /* gameGetMode */
    uint32_t subMode;       /* gameGetSubMode */
    uint8_t *goalInfo;      /* comdataGetGoalInfo result */
    uint8_t *region;        /* memory handed out by zmemStaticAlloc */
    uint32_t regionUsed;
    uint32_t extra[8]; /* UI group: [3] pad presses (bit player * 8 + button), [4] ovlPadConfirm result */
} DtgpScript;

extern DtgpScript dtgp;

/* Seeds from the case seed, clears the log, installs the stub groups. */
void dtgpBegin(const DtCtx *ctx, uint32_t groups);

/* Log access for stubs defined in the test files. */
void dtgpLog(uint32_t v);
void dtgpLogF(float f);
void dtgpLogP(const void *p);
void dtgpLogN(const void *p, uint32_t words);
uint32_t dtgpHash(const char *s);

#endif
