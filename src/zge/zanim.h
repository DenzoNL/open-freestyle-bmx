/*
 * Animation file chunks (C:\NgEngine\Zge\zanim.c): the ZIFF readers of an animation library. ziffReadAnim
 * (ziff.c) allocates the animation record (gZiffAnim); the handlers below fill it with its bone names (DEFI)
 * and the rotation (ROTA) and translation (ATRA) channels.
 */
#ifndef ZGE_ZANIM_H
#define ZGE_ZANIM_H

#include <stdint.h>

#include "compat/orig.h"

/* Bone name table of an animation (record at animation + 4). */
typedef struct ZanimNames {
    uint32_t count;
    void *names; /* count entries of 0x24 bytes */
    struct ZanimNames *next;
} ZanimNames;

/* A rotation or translation channel (records at animation + 8 and + 0xc). */
typedef struct ZanimChannel {
    uint32_t count;
    uint32_t boneHash; /* name hash of the bone it drives */
    void *keys;        /* count entries of 0x14 bytes, read as 4 or 5 words each */
    struct ZanimChannel *next;
} ZanimChannel;

/*
 * The animation record ziffReadAnim allocates (gZiffAnim): its name, the bone name tables, the rotation and
 * translation channels, newest first, and a fifth list nothing fills (ziffFreeAnim frees it like a channel
 * list).
 */
typedef struct ZanimRecord {
    uint32_t name;
    ZanimNames *names;
    ZanimChannel *rotations;
    ZanimChannel *translations;
    ZanimChannel *channels10;
} ZanimRecord;

BMX_LAYOUT32_ASSERT(sizeof(ZanimRecord) == 0x14, "animation record layout");

/* Chunk handler of the animation container (ANIM): nothing to read. */
int zanimReadAnim(void *iff);
/* DEFI: the bone names. */
int zanimReadDefinitions(void *iff);
/* ROTA: a rotation channel. */
int zanimReadRotations(void *iff);
/* ATRA: a translation channel. */
int zanimReadTranslations(void *iff);
/* Registers the four chunk handlers; fatal when called twice. */
int zanimInitialize(void);

#endif
