/* Animation file chunks (C:\NgEngine\Zge\zanim.c). */
#include "zge/zanim.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/ziff.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kZanimFile[] = "C:\\NgEngine\\Zge\\zanim.c";

enum { kZanimNameSize = 0x24, kZanimKeySize = 0x14 };

#define gZanimInitialized ORIG_VAR(0x006746c0, int, gZanimInitialized)

/* The animation being read (gZiffAnim): { name, names list, rotation channels, translation channels }. */
typedef struct ZanimRecord {
    uint32_t name;
    ZanimNames *names;
    ZanimChannel *rotations;
    ZanimChannel *translations;
} ZanimRecord;

/* Bone hash of the bone at `index` in the newest name table (an unchecked index, like the original). */
static uint32_t boneHash(const ZanimRecord *anim, uint32_t index)
{
    return *(const uint32_t *)((const uint8_t *)anim->names->names + 0x20 + index * kZanimNameSize);
}

BMX_ADDR(0x00498fb0)
int zanimReadAnim(void *iff)
{
    (void)iff;
    return 1;
}

/* Reads the animation's name and its table of bone names (0x24 bytes each), newest first. */
BMX_ADDR(0x00498fc0)
int zanimReadDefinitions(void *iff)
{
    ZanimNames *names = zmemAlloc(sizeof(ZanimNames));

    if (names == NULL) {
        zdebugFail("Can't allocate namelist", kZanimFile, 0x224, 0);
    }
    zmemFill(names, 0, sizeof(ZanimNames));
    names->next = ((ZanimRecord *)gZiffAnim)->names;
    ((ZanimRecord *)gZiffAnim)->names = names;
    ziffReadWordA(iff, gZiffAnim);
    names->count = (ziffChunkSize(iff) - 4U) / kZanimNameSize;
    if (names->count * kZanimNameSize + 4 != ziffChunkSize(iff)) {
        zdebugFail("DEFI size mismatch", kZanimFile, 0x22d, 0);
    }
    names->names = zmemAlloc(names->count * kZanimNameSize);
    if (names->names == NULL) {
        zdebugFail("Can't allocate nadname", kZanimFile, 0x230, 0);
    }
    ziffRead(iff, names->names, ziffChunkSize(iff) - 4);
    return 1;
}

/* A rotation channel: the bone index, then 5 words per key. */
BMX_ADDR(0x004990b0)
int zanimReadRotations(void *iff)
{
    ZanimChannel *channel = zmemAlloc(sizeof(ZanimChannel));
    uint32_t index;

    if (channel == NULL) {
        zdebugFail("Can't allocate rot channel", kZanimFile, 0x247, 0);
    }
    zmemFill(channel, 0, sizeof(ZanimChannel));
    channel->next = ((ZanimRecord *)gZiffAnim)->rotations;
    ((ZanimRecord *)gZiffAnim)->rotations = channel;
    ziffReadWordA(iff, &index);
    channel->boneHash = boneHash(gZiffAnim, index);
    channel->count = (ziffChunkSize(iff) - 4U) / kZanimKeySize;
    channel->keys = zmemAlloc(channel->count * kZanimKeySize);
    if (channel->keys == NULL) {
        zdebugFail("Can't Allocate NadRots", kZanimFile, 0x255, 0);
    }
    for (index = 0; index < channel->count; index++) {
        uint8_t *const key = (uint8_t *)channel->keys + index * kZanimKeySize;

        for (uint32_t word = 0; word < 5; word++) {
            ziffReadWordB(iff, key + word * 4);
        }
    }
    return 1;
}

/* A translation channel: the bone index, then 4 words per key (but room for 5 is allocated). */
BMX_ADDR(0x00499200)
int zanimReadTranslations(void *iff)
{
    ZanimChannel *channel = zmemAlloc(sizeof(ZanimChannel));
    uint32_t index;

    if (channel == NULL) {
        zdebugFail("Can't allocate Trans channel", kZanimFile, 0x272, 0);
    }
    zmemFill(channel, 0, sizeof(ZanimChannel));
    channel->next = ((ZanimRecord *)gZiffAnim)->translations;
    ((ZanimRecord *)gZiffAnim)->translations = channel;
    ziffReadWordA(iff, &index);
    channel->boneHash = boneHash(gZiffAnim, index);
    channel->count = (ziffChunkSize(iff) - 4U) >> 4;
    channel->keys = zmemAlloc(channel->count * kZanimKeySize);
    if (channel->keys == NULL) {
        zdebugFail("Can't Allocate NadTrans", kZanimFile, 0x280, 0);
    }
    for (index = 0; index < channel->count; index++) {
        uint8_t *const key = (uint8_t *)channel->keys + index * kZanimKeySize;

        for (uint32_t word = 0; word < 4; word++) {
            ziffReadWordB(iff, key + word * 4);
        }
    }
    return 1;
}

BMX_ADDR(0x00499330)
int zanimInitialize(void)
{
    ZiffCallbackNode *anim;

    if (gZanimInitialized != 0) {
        zdebugFail("Already initialized", kZanimFile, 0x29d, 0);
    }
    gZanimInitialized = 1;
    anim = ziffSetupCallback(0x4d494e41, BMX_FNPTR(int(BMX_CDECL *)(void *), 0x00498fb0, zanimReadAnim), 0);
    ziffSetupCallback(0x49464544, BMX_FNPTR(int(BMX_CDECL *)(void *), 0x00498fc0, zanimReadDefinitions),
                      (int)(uintptr_t)anim);
    ziffSetupCallback(0x544f5241, BMX_FNPTR(int(BMX_CDECL *)(void *), 0x004990b0, zanimReadRotations),
                      (int)(uintptr_t)anim);
    ziffSetupCallback(0x41525441, BMX_FNPTR(int(BMX_CDECL *)(void *), 0x00499200, zanimReadTranslations),
                      (int)(uintptr_t)anim);
    return 1;
}
