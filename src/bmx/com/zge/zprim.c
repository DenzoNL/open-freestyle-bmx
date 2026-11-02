/* 2D primitives and sprite assets (C:\zaxis\Bmx\src\com\zge\zprim.c). */
#include "bmx/com/zge/zprim.h"

#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "compat/crt.h"
#include "zge/zasset.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char zprimFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\zprim.c";

/* The state stack (zprimStatePush lets the depth reach 10, one past it: an original quirk) and the asset
 * table. */
enum { kZprimStateDepth = 10, kZprimAssetCount = 0x80 };

/*
 * Every state accessor opens the state stack the same way: if no caller has it locked, lock the DMM block
 * and remember to unlock it again on the way out.
 */
static ZPrimState *zprimOpen(int *took)
{
    *took = gZprimStates == NULL;
    if (*took) {
        gZprimStates = zmemdmmLock(gZprimHandle);
    }
    return gZprimStates + gZprimDepth;
}

static void zprimClose(int took)
{
    if (took) {
        gZprimStates = NULL;
        zmemdmmUnlock(gZprimHandle);
    }
}

/* ---- setup and teardown --------------------------------------------------------------------- */

/* Allocates the state stack and the asset table. Returns 0 on success, 1 on failure. */
BMX_ADDR(0x004779e0)
int zprimInit(void)
{
    if (ngifInit() != 0) {
        return 1;
    }
    gZprimHandle = zmemdmmAlloc(kZprimStateDepth * sizeof(ZPrimState));
    if (gZprimHandle != 0) {
        ZPrimState *states = zmemdmmLock(gZprimHandle);
        gZprimStates = states;
        gZprimDepth = 0;
        zmemFill(states, 0, kZprimStateDepth * sizeof(ZPrimState));
        zmemFill(states, 0, sizeof(ZPrimState));
        zprimStateReset();
        zprimStatePush();
        zmemdmmUnlock(gZprimHandle);
        /* The original clears library, pool, free, used, clip and groups in one fill
         * (0x00678400..0x00678428); natively each is its own definition (docs/PORTING.md X2a). */
        gZprimLibrary = 0;
        gZprimPool = NULL;
        gZprimFree = NULL;
        gZprimUsed = NULL;
        zmemFill(&gZprimClip, 0, (uint32_t)sizeof gZprimClip);
        zmemFill(&gZprimGroups, 0, (uint32_t)sizeof gZprimGroups);
        zprimSetClip(0, 0, 0x27f, 0x1df);
        ZPrimAsset *pool = zmemStaticAlloc(kZprimAssetCount * sizeof(ZPrimAsset));
        gZprimPool = pool;
        if (pool != NULL) {
            zmemFill(pool, 0, kZprimAssetCount * sizeof(ZPrimAsset));
            gZprimFree = pool;
            for (int i = 0; i < kZprimAssetCount; i++) {
                pool = gZprimPool;
                if (i == kZprimAssetCount - 1) {
                    pool[i].next = NULL;
                } else {
                    pool[i].next = pool + (i + 1);
                }
            }
            gZprimUsed = NULL;
            states->color = 0xffff;
            gZprimNextId = 0xc001;
            return 0;
        }
    }
    gZdebugPrint("ZPRIM: Failed to allocate buffer\n");
    return 1;
}

BMX_ADDR(0x00477b50)
void zprimShutdown(void)
{
    if (gZprimLibrary != 0) {
        zassetCloseLibrary(gZprimLibrary);
    }
    gZprimLibrary = 0;
    if (gZprimHandle != 0) {
        zmemdmmFree(gZprimHandle);
        gZprimHandle = 0;
    }
    ngifShutdown();
}

BMX_ADDR(0x00477ba0)
void zprimOpenLibrary(const char *name)
{
    if (gZprimLibrary != 0) {
        zassetCloseLibrary(gZprimLibrary);
    }
    gZprimLibrary = 0;
    gZprimLibrary = zassetOpenLibrary(name);
}

/* Starts a frame: clears to the current state's screen picture, or fills the clip rectangle with its colour.
 */
BMX_ADDR(0x00477be0)
void zprimClear(void)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    ngifBegin();
    switch (st->kind) {
    case 1: {
        const ZPrimSet *pic = zprimAssetData(st->asset);
        ngifSetPictureScreen(pic, pic->width, pic->height);
        zprimAssetRelease(st->asset);
        break;
    }
    case 2:
    case 3:
    case 4:
        break;
    default:
        ngifFillRect((int)st->color, 0, gZprimClip.x0, gZprimClip.y0, gZprimClip.x1, gZprimClip.y1);
    }
    zprimClose(took);
}

/* Frees every asset that is not marked "keep", then compacts the DMM pool. */
BMX_ADDR(0x00477d20)
void zprimFlush(void)
{
    ZPrimAsset *a = gZprimUsed;
    while (a != NULL) {
        ZPrimAsset *next = a->next;
        if ((a->flags & 1) == 0) {
            zprimAssetFree(a);
        }
        a = next;
    }
    gZprimNextId = 0xc101;
    zprimStateRestore();
    if (zmemdmmOptimize() != 0) {
        zdebugFail("Error Optimizing DMM memory", zprimFile, 0x2db, 0);
    }
}

BMX_ADDR(0x00477da0)
void zprimSetClip(int x0, int y0, int x1, int y1)
{
    gZprimClip.x0 = (int16_t)x0;
    gZprimClip.y0 = (int16_t)y0;
    gZprimClip.x1 = (int16_t)x1;
    gZprimClip.y1 = (int16_t)y1;
    ngifSetClip(x0, y0, x1, y1);
}

/* ---- state stack ---------------------------------------------------------------------------- */

/* state[0] = state[depth]. */
BMX_ADDR(0x00477df0)
void zprimStateSave(void)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    *gZprimStates = *st;
    zprimClose(took);
}

/* state[depth] = state[0]. */
BMX_ADDR(0x00477e80)
void zprimStateRestore(void)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    *st = *gZprimStates;
    zprimClose(took);
}

/* Pushes a copy of the current state (at most ten levels deep). */
BMX_ADDR(0x00477f10)
void zprimStatePush(void)
{
    int took;
    zprimOpen(&took);
    if (gZprimDepth < 10) {
        gZprimStates[gZprimDepth + 1] = gZprimStates[gZprimDepth];
        gZprimDepth++;
    }
    zprimClose(took);
}

BMX_ADDR(0x00477fb0)
void zprimStatePop(void)
{
    int took;
    zprimOpen(&took);
    if (gZprimDepth > 0) {
        gZprimStates[gZprimDepth - 1] = gZprimStates[gZprimDepth];
        gZprimDepth--;
    }
    zprimClose(took);
}

/*
 * zprimGet's jump table targets (caseD_2 0x4783f0 through caseD_4 0x47872b, 17 blocks) are parts of zprimGet
 * below, not functions of their own; they have no BMX_ADDR.
 */

/* Sets one property of the current state; unknown ids are ignored. */
BMX_ADDR(0x00478050)
void zprimSet(uint16_t prop, int value)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    switch (prop) {
    case kZprimScreen:
        st->kind = 1;
        st->asset = (ZPrimAsset *)(intptr_t)value;
        break;
    case kZprimPicture:
        st->kind = 2;
        st->asset = (ZPrimAsset *)(intptr_t)value;
        break;
    case kZprimSprite:
        st->kind = 3;
        st->asset = (ZPrimAsset *)(intptr_t)value;
        break;
    case kZprimColorOnly:
        st->kind = 4;
        break;
    case kZprimColor:
        st->color = (uint32_t)value;
        break;
    case kZprimFlagA2:
        st->flagsA = value == 0 ? (uint16_t)(st->flagsA & 0xfffd) : (uint16_t)(st->flagsA | 2);
        break;
    case kZprimFlagB2:
        st->flagsB = value == 0 ? (uint16_t)(st->flagsB & 0xfffd) : (uint16_t)(st->flagsB | 2);
        break;
    case kZprimFlagB1:
        st->flagsB = value == 0 ? (uint16_t)(st->flagsB & 0xfffe) : (uint16_t)(st->flagsB | 1);
        break;
    case kZprimFlagB4:
        st->flagsB = value == 0 ? (uint16_t)(st->flagsB & 0xfffb) : (uint16_t)(st->flagsB | 4);
        break;
    case kZprimField20:
        st->field20 = value;
        break;
    case kZprimField1c:
        st->field1c = value;
        break;
    case kZprimFlagB8:
        st->flagsB = value == 0 ? (uint16_t)(st->flagsB & 0xfff7) : (uint16_t)(st->flagsB | 8);
        break;
    case kZprimFlagB10:
        st->flagsB = value == 0 ? (uint16_t)(st->flagsB & 0xffef) : (uint16_t)(st->flagsB | 0x10);
        break;
    case kZprimField30:
        st->field30 = value;
        break;
    case kZprimField24:
        st->field24 = (uint16_t)value;
        break;
    case kZprimField34:
        st->field34 = value;
        break;
    case kZprimField38:
        st->field38 = value;
        break;
    case kZprimFlagC2:
        st->flagsC = value == 0 ? (uint16_t)(st->flagsC & 0xfffd) : (uint16_t)(st->flagsC | 2);
        break;
    case kZprimFlagC1:
        st->flagsC = value == 0 ? (uint16_t)(st->flagsC & 0xfffe) : (uint16_t)(st->flagsC | 1);
        break;
    case kZprimFlagC4:
        st->flagsC = value == 0 ? (uint16_t)(st->flagsC & 0xfffb) : (uint16_t)(st->flagsC | 4);
        break;
    case kZprimField4c:
        st->field4c = value;
        break;
    case kZprimField48:
        st->field48 = value;
        break;
    case kZprimField40:
        st->field40 = value;
        break;
    case kZprimSpacing:
        st->spacing = value;
        break;
    default:
        break;
    }
    zprimClose(took);
}

/*
 * Reads one property of the current state. Ids 1 to 3 return the asset only when the state is of that kind;
 * unknown ids (and 4, 6) return 0.
 */
BMX_ADDR(0x00478370)
uint32_t zprimGet(uint16_t prop)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    uint32_t r = 0;
    switch (prop) {
    case kZprimScreen:
    case kZprimPicture:
    case kZprimSprite:
        if (st->kind == prop) {
            r = (uint32_t)(uintptr_t)st->asset;
        }
        break;
    case kZprimColor:
        r = st->color;
        break;
    case kZprimFlagB2:
        r = (st->flagsB & 2) != 0;
        break;
    case kZprimFlagB1:
        r = (st->flagsB & 1) != 0;
        break;
    case kZprimFlagB4:
        r = (st->flagsB & 4) != 0;
        break;
    case kZprimField20:
        r = (uint32_t)st->field20;
        break;
    case kZprimField1c:
        r = (uint32_t)st->field1c;
        break;
    case kZprimFlagB8:
        r = (st->flagsB & 8) != 0;
        break;
    case kZprimFlagB10:
        r = (st->flagsB & 0x10) != 0;
        break;
    case kZprimField30:
        r = (uint32_t)st->field30;
        break;
    case kZprimField24:
        r = st->field24;
        break;
    case kZprimField34:
        r = (uint32_t)st->field34;
        break;
    case kZprimField38:
        r = (uint32_t)st->field38;
        break;
    case kZprimFlagC2:
        r = (st->flagsC & 2) != 0;
        break;
    case kZprimFlagC1:
        r = (st->flagsC & 1) != 0;
        break;
    case kZprimFlagC4:
        r = (st->flagsC & 4) != 0;
        break;
    case kZprimField4c:
        r = (uint32_t)st->field4c;
        break;
    case kZprimField48:
        r = (uint32_t)st->field48;
        break;
    case kZprimField40:
        r = (uint32_t)st->field40;
        break;
    case kZprimSpacing:
        r = (uint32_t)st->spacing;
        break;
    default:
        break;
    }
    zprimClose(took);
    return r;
}

/* ---- state setters / getters ---------------------------------------------------------------- */

BMX_ADDR(0x004788c0)
void zprimSetScreen(ZPrimAsset *asset)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    if (asset == NULL) {
        zprimClose(took);
        return;
    }
    st->kind = 1;
    st->asset = asset;
    zprimClose(took);
}

/* Selects a picture (type 2) or sprite (type 3) as the drawing source. */
BMX_ADDR(0x00478980)
void zprimSetSource(ZPrimAsset *asset, int16_t type, uint16_t arg6, uint16_t arg8, uint16_t arg10)
{
    if (asset == NULL) {
        return;
    }
    int took;
    ZPrimState *st = zprimOpen(&took);
    st->kind = type == 3 ? 3 : 2;
    st->kindArg = arg6;
    st->asset = asset;
    st->arg8 = arg8;
    st->arg10 = arg10;
    zprimClose(took);
}

BMX_ADDR(0x00478a50)
void zprimSetFont(ZPrimAsset *font)
{
    if (font == NULL) {
        return;
    }
    int took;
    ZPrimState *st = zprimOpen(&took);
    st->font = font;
    zprimClose(took);
}

BMX_ADDR(0x00478ae0)
ZPrimAsset *zprimGetFont(void)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    ZPrimAsset *font = st->font;
    zprimClose(took);
    return font;
}

BMX_ADDR(0x00478b60)
void zprimSetPicture(ZPrimAsset *picture)
{
    if (picture == NULL) {
        return;
    }
    int took;
    ZPrimState *st = zprimOpen(&took);
    st->picture = picture;
    zprimClose(took);
}

BMX_ADDR(0x00478bf0)
void zprimSetScale(float x, float y)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    st->scaleX = x;
    st->scaleY = y;
    zprimClose(took);
}

BMX_ADDR(0x00478c80)
void zprimStateReset(void)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    st->scaleX = 1.0f;
    st->scaleY = 1.0f;
    zprimClose(took);
}

BMX_ADDR(0x00478d00)
ZPrimAsset *zprimGetPicture(void)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    ZPrimAsset *picture = st->picture;
    zprimClose(took);
    return picture;
}

/* Sets colour property `prop` from three bytes. */
BMX_ADDR(0x00478d80)
void zprimSetColor(uint32_t r, uint32_t g, uint32_t b, uint8_t prop)
{
    zprimSet(prop, (int)((((b & 0xff) << 8 | (g & 0xff)) << 8) | (r & 0xff)));
}

BMX_ADDR(0x00478dc0)
void zprimGetColor(void)
{
    zdebugFail("ZPRIM: zprimGetColor not implemented\n", zprimFile, 0x5f0, 0);
}

/* Mode for the drawing calls: 1 when flagsA bit 1 is set, else 2 for bit 2, else 3 for bit 0, else 0. */
static int zprimDrawMode(const ZPrimState *st)
{
    const uint16_t f = st->flagsA;
    if (f & 2) {
        return 1;
    }
    if (f & 4) {
        return 2;
    }
    if (f & 1) {
        return 3;
    }
    return 0;
}

/* Draws a line; the end points are ordered left to right first. */
BMX_ADDR(0x00478df0)
void zprimDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    if (x1 < x0) {
        const int16_t tx = x0;
        const int16_t ty = y0;
        x0 = x1;
        y0 = y1;
        x1 = tx;
        y1 = ty;
    }
    ngifDrawLine((int)st->color, zprimDrawMode(st), x0, y0, x1, y1);
    zprimClose(took);
}

/* Fills a rectangle; the corners are ordered first. */
BMX_ADDR(0x00478f00)
void zprimFillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    int16_t xmin = x0;
    int16_t xmax = x1;
    if (x1 < x0) {
        xmin = x1;
        xmax = x0;
    }
    int16_t ymin = y0;
    int16_t ymax = y1;
    if (y1 < y0) {
        ymin = y1;
        ymax = y0;
    }
    ngifFillRect((int)st->color, zprimDrawMode(st), xmin, ymin, xmax, ymax);
    zprimClose(took);
}

/* ---- asset table ---------------------------------------------------------------------------- */

/* Pops a node off the free list and pushes it on the used list; returns NULL when none is left. */
BMX_ADDR(0x0047b830)
ZPrimAsset *zprimAssetAlloc(void)
{
    ZPrimAsset *a = gZprimFree;
    if (a == NULL) {
        return NULL;
    }
    gZprimFree = a->next;
    a->id = 0;
    a->flags = 0;
    a->next = NULL;
    a->dmm = 0;
    a->offset = 0;
    a->next = gZprimUsed;
    gZprimUsed = a;
    return a;
}

/* Unlinks a node from the used list and puts it on the free list. */
BMX_ADDR(0x0047b880)
void zprimAssetUnlink(ZPrimAsset *a)
{
    if (a == gZprimUsed) {
        gZprimUsed = a->next;
        a->next = gZprimFree;
        gZprimFree = a;
        return;
    }
    ZPrimAsset *prev = gZprimUsed;
    if (prev != NULL) {
        ZPrimAsset *next;
        while ((next = prev->next) != a) {
            prev = next;
            if (next == NULL) {
                a->next = gZprimFree;
                gZprimFree = a;
                return;
            }
        }
        if (prev != NULL) {
            prev->next = a->next;
        }
    }
    a->next = gZprimFree;
    gZprimFree = a;
}

/* Address of the asset's data: locks the DMM block, so every call needs a zprimAssetRelease. */
BMX_ADDR(0x0047b900)
void *zprimAssetData(const ZPrimAsset *a)
{
    uint8_t *base = zmemdmmLock(a->dmm);
    return base + a->offset;
}

BMX_ADDR(0x0047b930)
void zprimAssetRelease(const ZPrimAsset *a)
{
    zmemdmmUnlock(a->dmm);
}

/* Finds a loaded asset by id. */
BMX_ADDR(0x0047bac0)
ZPrimAsset *zprimAssetFind(int16_t id)
{
    for (ZPrimAsset *a = gZprimUsed; a != NULL; a = a->next) {
        if ((int16_t)a->id == id) {
            return a;
        }
    }
    return NULL;
}

/*
 * Looks for `id` in the table of each loaded group. On a hit the group's `offset` is set to the entry's
 * offset and the group is returned.
 */
BMX_ADDR(0x0047baf0)
ZPrimAsset *zprimGroupFind(uint32_t id)
{
    ZPrimAsset **groups = gZprimGroups;
    for (ZPrimAsset **g = groups; g < groups + 4; g++) {
        ZPrimAsset *group = *g;
        if (group == NULL) {
            continue;
        }
        const uint32_t *table = zmemdmmLock(group->dmm);
        const uint32_t *entry = table + 1;
        for (uint32_t i = 0; i < table[0]; i++, entry += 2) {
            if (entry[0] == (id & 0xffff)) {
                zmemdmmUnlock(group->dmm);
                group->offset = (int32_t)entry[1];
                return group;
            }
        }
        zmemdmmUnlock(group->dmm);
    }
    return NULL;
}

BMX_ADDR(0x0047b3d0)
int zprimGroupLoaded(uint32_t slot)
{
    ZPrimGroupTable *groups = &gZprimGroups;
    return (*groups)[slot & 0xffff] != NULL;
}

/* Number of entries in a loaded group. */
BMX_ADDR(0x0047ba10)
uint16_t zprimGroupCount(uint32_t slot)
{
    ZPrimAsset *group = gZprimGroups[slot & 0xffff];
    const uint16_t *table = zmemdmmLock(group->dmm);
    zmemdmmUnlock(group->dmm);
    return table[0];
}

/* Points a group's offset at entry `index` and returns the group. */
BMX_ADDR(0x0047ba50)
ZPrimAsset *zprimGroupSelect(uint32_t slot, uint32_t index)
{
    ZPrimAsset *group = gZprimGroups[slot & 0xffff];
    const uint32_t *table = zmemdmmLock(group->dmm);
    zmemdmmUnlock(group->dmm);
    group->offset = (int32_t)table[(index & 0xffff) * 2 + 2];
    return group;
}

/* ---- textures ------------------------------------------------------------------------------- */

/* Rounds a texture dimension up to the next power of two (only 3 to 255 are touched; 3 becomes 4). */
static uint32_t zprimRoundDimension(uint32_t v)
{
    if (v >= 0x81 && v <= 0xff) {
        return 0x100;
    }
    if (v >= 0x41 && v <= 0x7f) {
        return 0x80;
    }
    if (v >= 0x21 && v <= 0x3f) {
        return 0x40;
    }
    if (v >= 0x11 && v <= 0x1f) {
        return 0x20;
    }
    if (v >= 9 && v <= 0xf) {
        return 0x10;
    }
    if (v >= 5 && v <= 7) {
        return 8;
    }
    if (v > 2 && v < 4) {
        return 4;
    }
    return v;
}

/* Bitmap file header and BITMAPINFOHEADER, back to back (0x36 bytes, unaligned fields). */
typedef struct __attribute__((packed)) ZBmpHeader {
    uint16_t magic;
    uint32_t fileSize;
    uint32_t reserved;
    uint32_t dataOffset;
    uint32_t infoSize;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t imageSize;
    uint32_t xPixelsPerMeter;
    uint32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
} ZBmpHeader;

_Static_assert(sizeof(ZBmpHeader) == 0x36, "BMP header size");

/*
 * Builds a bottom-up BMP in static memory from `pixels` (w x h, bpp 1, 4 or 8) and a 15 bit palette
 * ({flags, count, colours}), hands it to the renderer and returns the texture handle. The texture is
 * padded to a square power of two. Magenta (31, 0, 31) becomes full 255, 0, 255.
 */
BMX_ADDR(0x0047c030)
int zprimCreateTexture(uint32_t w, uint32_t h, uint32_t bpp, const uint8_t *pixels, const uint16_t *palette)
{
    const uint32_t srcW = w;
    const uint32_t nameId = (uint32_t)gZprimTextureCounter & 0xffff;
    char name[32];
    *(uint16_t *)&gZprimTextureCounter = (uint16_t)((uint16_t)gZprimTextureCounter + 1);
    zstrFormat(name, "Sprite%d", nameId);

    uint32_t texW = zprimRoundDimension(w);
    uint32_t texH = zprimRoundDimension(h);
    if (texW != texH) {
        if (texH < texW) {
            texH = texW;
        } else {
            texW = texH;
        }
    }
    const uint32_t rowBytes = (uint32_t)((uint64_t)srcW / (8 / (uint64_t)bpp));
    const uint16_t *entries = palette + 2;
    const uint32_t pitch = ((uint32_t)((uint64_t)texW / (8 / (uint64_t)bpp)) + 3) & ~3u;
    uint32_t *mark = zmemStaticMark();
    const uint32_t imageBytes = pitch * texH;
    const uint32_t colors = 1u << (bpp & 0x1f);
    const uint32_t size = imageBytes + sizeof(ZBmpHeader) + colors * 4;
    uint8_t *bmp = zmemStaticAlloc(size);
    zmemFill(bmp, 0, size);
    ZBmpHeader *header = (ZBmpHeader *)bmp;
    header->magic = 0x4d42;
    header->fileSize = size;
    header->reserved = 0;
    header->dataOffset = colors * 4 + 0x36;
    header->infoSize = 0x28;
    header->width = texW;
    header->height = texH;
    header->planes = 1;
    header->bitCount = (uint16_t)bpp;
    header->compression = 0;
    header->imageSize = imageBytes;
    header->xPixelsPerMeter = 0;
    header->yPixelsPerMeter = 0;
    header->colorsUsed = 0;
    header->colorsImportant = 0;
    for (uint32_t i = 0; i < colors; i++) {
        /* A palette entry is blue, green, red and a fourth byte, which gets the index. The 15 bit colours
         * keep red in the low bits. */
        uint8_t *q = bmp + sizeof(ZBmpHeader) + i * 4;
        if (i < palette[1]) {
            const uint16_t v = *entries++;
            q[2] = (uint8_t)(v << 3);
            q[1] = (uint8_t)((v >> 5) << 3);
            q[0] = (uint8_t)((v >> 10) << 3);
            q[3] = (uint8_t)i;
            if (q[2] == 0xf8 && q[1] == 0 && q[0] == 0xf8) {
                q[0] = 0xff;
                q[2] = 0xff;
            }
        } else {
            q[2] = 0;
            q[1] = 0;
            q[0] = 0;
            q[3] = 0;
        }
    }
    uint8_t *dst = bmp + imageBytes + colors * 4 + sizeof(ZBmpHeader);
    for (uint32_t row = h; row != 0; row--) {
        dst -= pitch;
        zmemCopy(dst, pixels, rowBytes);
        pixels += rowBytes;
    }
    if (bpp == 4) {
        for (uint32_t i = 0; i < imageBytes; i++) {
            /* The pixels start at 0x76: the header and 16 palette entries. */
            const uint8_t b = bmp[0x76 + i];
            bmp[0x76 + i] = (uint8_t)(b * 16 + (b >> 4));
        }
    }
    const int texture = gRwTextureLoad(bmp, size, name, 0, 0);
    if (texture == 0) {
        zdebugFail("Invalid material handle", zprimFile, 0x16a5, 0);
    }
    zmemStaticRelease(mark);
    return texture;
}

/* ---- pictures ------------------------------------------------------------------------------- */

/* Loads picture `id` (from the library or a loaded group) and builds its texture. */
BMX_ADDR(0x00479010)
ZPrimAsset *zprimLoadPicture(uint32_t id, int keep)
{
    if ((id & 0xe000) != 0) {
        return NULL;
    }
    ZPrimAsset *found = zprimAssetFind((int16_t)id);
    int32_t offset = 0;
    if (found != NULL) {
        return found;
    }
    ZPrimAsset *group = zprimGroupFind(id);
    int inGroup;
    int32_t handle;
    if (group == NULL) {
        inGroup = 0;
        if (zassetFind(gZprimLibrary, (int)(id & 0xffff), 0) != 1) {
            gZdebugPrint("ZPRIM: Failed to find picture %x\n", id & 0xffff);
            return NULL;
        }
        handle = zmemdmmAlloc(zassetSize(gZprimLibrary));
        if (handle == 0) {
            gZdebugPrint("ZPRIM: Failed to allocate picture\n");
            return NULL;
        }
        zassetRead(gZprimLibrary, zmemdmmLock(handle));
        zmemdmmUnlock(handle);
    } else {
        inGroup = 1;
        if (group->flags & 1) {
            keep = 1;
        }
        handle = group->dmm;
        offset = group->offset;
    }
    ZPrimAsset *node = zprimAssetAlloc();
    if (node == NULL) {
        gZdebugPrint("ZPRIM: Failed to allocate picture\n");
        return NULL;
    }
    node->flags = 0;
    if (keep != 0) {
        node->flags = 1;
    }
    if (inGroup) {
        node->flags |= 2;
    }
    node->id = (uint16_t)id;
    node->dmm = handle;
    node->offset = offset;
    ZPrimSet *data = zprimAssetData(node);
    data->frames[0].texture =
        zprimCreateTexture(data->width, data->height, data->bpp, (const uint8_t *)data + data->pixelOffset,
                           (const uint16_t *)((const uint8_t *)data + data->paletteOffset));
    zprimAssetRelease(node);
    return node;
}

BMX_ADDR(0x004791c0)
void zprimFreePicture(ZPrimAsset *a)
{
    zprimAssetFree(a);
}

BMX_ADDR(0x004791e0)
uint16_t zprimPictureWidth(const ZPrimAsset *a)
{
    const ZPrimSet *data = zprimAssetData(a);
    const uint16_t w = data->width;
    zprimAssetRelease(a);
    return w;
}

BMX_ADDR(0x00479210)
uint16_t zprimPictureHeight(const ZPrimAsset *a)
{
    const ZPrimSet *data = zprimAssetData(a);
    const uint16_t h = data->height;
    zprimAssetRelease(a);
    return h;
}

BMX_ADDR(0x00479240)
void zprimNop1(void) {}

BMX_ADDR(0x00479260)
void zprimNop2(const ZPrimAsset *sprite, const ZPrimAsset *backdrop, int a, int b) {}

/* Replaces the texture of picture `dst` with a copy of the one of `src`. */
BMX_ADDR(0x00479280)
void zprimCopyTexture(const ZPrimAsset *src, const ZPrimAsset *dst)
{
    ZPrimSet *s = zprimAssetData(src);
    ZPrimSet *d = zprimAssetData(dst);
    if (d->frames[0].texture != 0) {
        gRwTextureFree(d->frames[0].texture);
        d->frames[0].texture = 0;
    }
    d->frames[0].texture = gRwTextureClone(s->frames[0].texture);
    zprimAssetRelease(src);
    zprimAssetRelease(dst);
}

/*
 * Rebuilds the texture of `dst` from the pixels of `src` with a greyscale palette: every colour except
 * magenta becomes (r + g + b) * brightness / 768 in all three channels.
 */
BMX_ADDR(0x00479300)
void zprimReplaceVramGrey(const ZPrimAsset *src, const ZPrimAsset *dst, int unused, uint32_t brightness)
{
    uint8_t buf[0xff0];
    ZPrimSet *s = zprimAssetData(src);
    ZPrimSet *d = zprimAssetData(dst);
    const uint8_t *pal = (const uint8_t *)s + s->paletteOffset;
    const uint32_t count = *(const uint16_t *)(pal + 2);
    if (count * 2 + 4 >= 0x1000) {
        zdebugFail("Buffer overflow in zprimReplaceVRAMGrey()", zprimFile, 0x7c4, 0);
    }
    zmemCopy(buf, pal, count * 2 + 4);
    uint16_t *entries = (uint16_t *)(buf + 4);
    for (uint32_t i = 0; (uint16_t)i < *(const uint16_t *)(pal + 2); i++) {
        const uint32_t v = entries[i];
        const uint32_t lo = v & 0x1f;
        const uint32_t mid = (v >> 5) & 0x1f;
        const uint32_t hi = (v >> 10) & 0x1f;
        if (!(lo == 0x1f && mid == 0 && hi == lo)) {
            const uint32_t g = (((hi + mid + lo) * (brightness & 0xff)) / 768) & 0x1f;
            entries[i] = (uint16_t)(((g << 5 | g) << 5) | g);
        }
    }
    if (d->frames[0].texture != 0) {
        gRwTextureFree(d->frames[0].texture);
        d->frames[0].texture = 0;
    }
    /* Reads the pixels through frame 0 (the picture loader uses the header's offset, the same value). */
    d->frames[0].texture = zprimCreateTexture(
        s->width, s->height, s->bpp, (const uint8_t *)s + s->frames[0].pixelOffset, (const uint16_t *)buf);
    zprimAssetRelease(src);
    zprimAssetRelease(dst);
}

/* Rebuilds the texture of `dst` from the pixels of `src` using a 16 colour slice (index * 16) of its palette.
 */
BMX_ADDR(0x00479500)
void zprimReplaceVramPalette(const ZPrimAsset *src, const ZPrimAsset *dst, int unused, uint32_t index)
{
    uint8_t buf[0xff0];
    ZPrimSet *s = zprimAssetData(src);
    ZPrimSet *d = zprimAssetData(dst);
    const uint8_t *pal = (const uint8_t *)s + s->paletteOffset;
    const uint32_t count = *(const uint16_t *)(pal + 2);
    if (count * 2 + 4 >= 0x1000) {
        zdebugFail("Buffer overflow in zprimReplaceVRAMGrey()", zprimFile, 0x7f8, 0);
    }
    zmemCopy(buf, pal, count * 2 + 4);
    uint16_t *entries = (uint16_t *)(buf + 4);
    const uint16_t *slice = (const uint16_t *)(buf + 4 + ((index & 0xff) << 5));
    for (int i = 0; i < 16; i++) {
        entries[i] = slice[i];
    }
    if (d->frames[0].texture != 0) {
        gRwTextureFree(d->frames[0].texture);
        d->frames[0].texture = 0;
    }
    /* Reads the pixels through frame 0 (the picture loader uses the header's offset, the same value). */
    d->frames[0].texture = zprimCreateTexture(
        s->width, s->height, s->bpp, (const uint8_t *)s + s->frames[0].pixelOffset, (const uint16_t *)buf);
    zprimAssetRelease(src);
    zprimAssetRelease(dst);
}

/* ---- sprites -------------------------------------------------------------------------------- */

/*
 * Loads sprite set `id` (type 0x4000, 0x6000 compressed or 0xc000 virtual) and builds one texture per
 * frame when the set carries a palette.
 */
BMX_ADDR(0x00479660)
ZPrimAsset *zprimLoadSprite(uint32_t id, int keep)
{
    const uint16_t type = (uint16_t)id & 0xe000;
    if (type != 0x4000 && type != 0x6000 && type != 0xc000) {
        return NULL;
    }
    ZPrimAsset *found = zprimAssetFind((int16_t)id);
    int32_t offset = 0;
    if (found != NULL) {
        return found;
    }
    ZPrimAsset *group = zprimGroupFind(id);
    int inGroup;
    int32_t handle;
    if (group == NULL) {
        inGroup = 0;
        if (zassetFind(gZprimLibrary, (int)(id & 0xffff), 0) != 1) {
            gZdebugPrint("ZPRIM: Failed to find sprite %x\n", id & 0xffff);
            return NULL;
        }
        handle = zmemdmmAlloc(zassetSize(gZprimLibrary));
        if (handle == 0) {
            gZdebugPrint("ZPRIM: Failed to allocate sprite\n");
            return NULL;
        }
        zassetRead(gZprimLibrary, zmemdmmLock(handle));
        zmemdmmUnlock(handle);
    } else {
        inGroup = 1;
        if (group->flags & 1) {
            keep = 1;
        }
        handle = group->dmm;
        offset = group->offset;
    }
    ZPrimAsset *node = zprimAssetAlloc();
    if (node == NULL) {
        gZdebugPrint("ZPRIM: Failed to allocate sprite\n");
        return NULL;
    }
    node->flags = 0;
    if (keep != 0) {
        node->flags = 1;
    }
    if (inGroup) {
        node->flags |= 2;
    }
    node->id = (uint16_t)id;
    node->dmm = handle;
    node->offset = offset;
    ZPrimSet *set = zprimAssetData(node);
    if (set->paletteOffset != 0) {
        const uint32_t frames = set->frameCount;
        if (!((set->flags & 1) != 0 && frames == 0)) {
            /* the original wraps this in a loop that always runs once */
            for (uint32_t i = 0; i < frames; i++) {
                ZPrimFrame *frame = &set->frames[i];
                frame->texture = zprimCreateTexture(
                    frame->width, frame->height, set->bpp, (const uint8_t *)set + frame->pixelOffset,
                    (const uint16_t *)((const uint8_t *)set + set->paletteOffset));
            }
        }
    }
    zprimAssetRelease(node);
    return node;
}

/* Creates an empty virtual sprite set with `count` frames of w x h and a 256 entry palette. */
BMX_ADDR(0x00479880)
ZPrimAsset *zprimCreateSprite(uint16_t w, uint16_t h, uint16_t count)
{
    const uint32_t pixels = (uint32_t)w * h;
    const uint32_t n = count;
    uint32_t pixelBase = n * sizeof(ZPrimFrame) + sizeof(ZPrimSet);
    const uint32_t size = (pixels + 0x10) * n + 0x318;
    const uint32_t paletteOffset = n * pixels + pixelBase;
    const int32_t handle = zmemdmmAlloc(size);
    if (handle == 0) {
        gZdebugPrint("ZPRIM: Failed to allocate buffer in CreateSprite\n");
        return NULL;
    }
    ZPrimAsset *node = zprimAssetAlloc();
    if (node == NULL) {
        gZdebugPrint("ZPRIM: Failed to allocate asset handle\n");
        return NULL;
    }
    node->flags = 4;
    node->id = gZprimNextId;
    gZprimNextId = (uint16_t)(gZprimNextId + 1);
    node->dmm = handle;
    node->offset = 0;
    ZPrimSet *data = zmemdmmLock(handle);
    zmemFill(data, 0, size);
    data->frameCount = count;
    data->flags = 0;
    data->paletteOffset = (int32_t)paletteOffset;
    data->width = w;
    data->height = h;
    data->pixelOffset = (int32_t)pixelBase;
    ZPrimFrame *frame = data->frames;
    for (uint32_t i = n; i != 0; i--) {
        frame->width = w;
        frame->height = h;
        frame->pixelOffset = (int32_t)pixelBase;
        frame->originX = 0;
        frame->originY = 0;
        pixelBase += pixels;
        frame++;
    }
    uint8_t *palette = (uint8_t *)data + paletteOffset;
    *(uint16_t *)palette = 0;
    *(uint16_t *)(palette + 2) = 0x100;
    uint16_t *colors = (uint16_t *)(palette + 4);
    for (uint32_t i = 0; (int)i < 0x100; i++) {
        const uint32_t hiBits = i & 0xfff8;
        *colors++ =
            (uint16_t)((uint16_t)((((hiBits | 0xff00) << 5) | hiBits) << 2) | (uint16_t)((int)i >> 3));
    }
    zmemdmmUnlock(handle);
    return node;
}

/* Locks and unlocks the data block of a sprite (a no-op apart from the counters). */
BMX_ADDR(0x00479a80)
void zprimSpriteTouch(const ZPrimAsset *a)
{
    zmemdmmLock(a->dmm);
    zmemdmmUnlock(a->dmm);
}

/* Resets a sprite set to a single empty w x h frame of `bpp` bits with `colors` transparent palette entries.
 */
BMX_ADDR(0x00479ab0)
void zprimSpriteReset(const ZPrimAsset *a, uint16_t w, uint16_t h, uint16_t bpp, uint16_t colors)
{
    ZPrimSet *set = zmemdmmLock(a->dmm);
    /* The old palette position, read before the header is rewritten. */
    uint16_t *palette = (uint16_t *)((uint8_t *)set + set->paletteOffset);
    set->frameCount = 1;
    set->flags = 0;
    set->width = w;
    set->height = h;
    set->pixelOffset = 0;
    set->paletteOffset = sizeof(ZPrimSet) + sizeof(ZPrimFrame); /* directly behind frame 0 */
    set->bpp = bpp;
    set->frames[0].width = w;
    set->frames[0].height = h;
    set->frames[0].pixelOffset = 0;
    set->frames[0].originX = 0;
    set->frames[0].originY = 0;
    if (set->frames[0].texture != 0) {
        gRwTextureFree(set->frames[0].texture);
        set->frames[0].texture = 0;
    }
    palette[0] = 0;
    palette[1] = colors;
    uint16_t *entries = palette + 2;
    for (uint32_t i = 0; i < colors; i++) {
        entries[i] = 0x8000;
    }
    zmemdmmUnlock(a->dmm);
}

/* Creates a fresh single frame virtual sprite of w x h with `bpp` bits and `colors` transparent palette
 * entries. */
BMX_ADDR(0x00479b90)
ZPrimAsset *zprimCreateSpriteSmall(uint16_t w, uint16_t h, uint16_t bpp, uint16_t colors)
{
    const int32_t handle = zmemdmmAlloc(0x328);
    if (handle == 0) {
        gZdebugPrint("ZPRIM: Failed to allocate buffer in CreateSprite\n");
        return NULL;
    }
    ZPrimAsset *node = zprimAssetAlloc();
    if (node == NULL) {
        gZdebugPrint("ZPRIM: Failed to allocate asset handle\n");
        return NULL;
    }
    node->flags = 4;
    node->id = gZprimNextId;
    gZprimNextId = (uint16_t)(gZprimNextId + 1);
    node->dmm = handle;
    node->offset = 0;
    ZPrimSet *set = zmemdmmLock(handle);
    zmemFill(set, 0, 0xca * 4);
    set->frameCount = 1;
    set->flags = 0;
    set->width = w;
    set->height = h;
    set->pixelOffset = 0;
    set->paletteOffset = sizeof(ZPrimSet) + sizeof(ZPrimFrame); /* directly behind frame 0 */
    set->bpp = bpp;
    set->frames[0].height = h;
    set->frames[0].width = w;
    set->frames[0].pixelOffset = 0;
    set->frames[0].originX = 0;
    set->frames[0].originY = 0;
    set->frames[0].texture = 0;
    uint16_t *palette = (uint16_t *)&set->frames[1]; /* {flags, count, colours} */
    palette[1] = colors;
    palette[0] = 0;
    if (colors != 0) {
        uint16_t *entries = palette + 2;
        for (uint32_t i = 0; i < colors; i++) {
            entries[i] = 0x8000;
        }
    }
    zmemdmmUnlock(handle);
    return node;
}

BMX_ADDR(0x00479cd0)
void zprimFreeSprite(ZPrimAsset *a)
{
    zprimAssetFree(a);
}

/* Draws frame `frame` of the state's picture asset at (x, y), through the display list. */
BMX_ADDR(0x00479cf0)
void zprimDrawSprite(uint32_t frame, int16_t x, int16_t y)
{
    if ((uint16_t)frame == 0xffff) {
        return;
    }
    int took;
    ZPrimState *st = zprimOpen(&took);
    ZPrimAsset *picture = st->picture;
    if (picture == NULL) {
        zprimClose(took);
        return;
    }
    ZPrimSet *data = zprimAssetData(picture);
    if ((uint16_t)frame >= data->frameCount) {
        zprimAssetRelease(st->picture);
        zprimClose(took);
        return;
    }
    const uint32_t f = frame & 0xffff;
    const ZPrimFrame *entry = &data->frames[f];
    const uint32_t entryH = entry->height;
    const uint32_t entryW = entry->width;
    if ((picture->id & 0xe000) != 0x6000) {
        x = (int16_t)(x - entry->originX);
        y = (int16_t)(y - entry->originY);
    }
    int32_t mode = 0;
    int32_t extra = 0;
    const uint16_t flags = st->flagsB;
    if (flags & 2) {
        mode = 1;
    } else if (flags & 4) {
        mode = 2;
        if ((flags & 1) == 0) {
            extra = st->field20;
        }
    } else if (flags & 1) {
        mode = 3;
    }
    const uint8_t *pixels = NULL;
    const int32_t paletteOffset = data->paletteOffset;
    if (paletteOffset != 0) {
        if (data->flags & 1) {
            pixels = (const uint8_t *)data + paletteOffset + f * 0x204;
        } else {
            pixels = (const uint8_t *)data + paletteOffset;
        }
    }
    if (st->field34 != 0) {
        ngifSetBlend(st->field38, st->field34);
    }
    ngifDrawSprite(data, (int)frame, pixels, mode, 0, extra, st->field24, x, y, (int)entryW, (int)entryH,
                   data->bpp, st->field30);
    if (st->field34 != 0) {
        ngifSetBlend(0, 0);
    }
    zprimAssetRelease(st->picture);
    zprimClose(took);
}

/* Width of frame `index` of a sprite set (the state's picture when `asset` is NULL). */
BMX_ADDR(0x00479fa0)
uint16_t zprimSpriteFrameWidth(ZPrimAsset *asset, uint16_t index)
{
    int took = gZprimStates == NULL;
    if (took) {
        gZprimStates = zmemdmmLock(gZprimHandle);
    }
    uint16_t w = 0;
    if (asset == NULL) {
        asset = gZprimStates[gZprimDepth].picture;
    }
    if (asset != NULL) {
        const ZPrimSet *data = zprimAssetData(asset);
        if (index < data->frameCount) {
            w = data->frames[index].width;
        }
        zprimAssetRelease(asset);
    }
    zprimClose(took);
    return w;
}

BMX_ADDR(0x0047a070)
uint16_t zprimSpriteFrameHeight(ZPrimAsset *asset, uint16_t index)
{
    int took = gZprimStates == NULL;
    if (took) {
        gZprimStates = zmemdmmLock(gZprimHandle);
    }
    uint16_t h = 0;
    if (asset == NULL) {
        asset = gZprimStates[gZprimDepth].picture;
    }
    if (asset != NULL) {
        const ZPrimSet *data = zprimAssetData(asset);
        if (index < data->frameCount) {
            h = data->frames[index].height;
        }
        zprimAssetRelease(asset);
    }
    zprimClose(took);
    return h;
}

BMX_ADDR(0x0047a140)
uint16_t zprimSpriteFrameCount(ZPrimAsset *asset)
{
    int took = gZprimStates == NULL;
    if (took) {
        gZprimStates = zmemdmmLock(gZprimHandle);
    }
    uint16_t n = 0;
    if (asset == NULL) {
        asset = gZprimStates[gZprimDepth].picture;
    }
    if (asset != NULL) {
        const ZPrimSet *data = zprimAssetData(asset);
        n = data->frameCount;
        zprimAssetRelease(asset);
    }
    zprimClose(took);
    return n;
}

/* Run length decodes frame `srcFrame` of a compressed sprite into frame `dstFrame` of a virtual one. */
BMX_ADDR(0x0047a1e0)
void zprimDecompress(const ZPrimAsset *src, uint16_t srcFrame, const ZPrimAsset *dst, uint16_t dstFrame)
{
    ZPrimSet *s = zprimAssetData(src);
    ZPrimSet *d = zprimAssetData(dst);
    if ((src->id & 0xe000) != 0x6000 || (dst->id & 0xe000) != 0xc000) {
        gZdebugPrint("ZPRIM: Error - can only decompress from compressed sprite to virtual sprite (%d->%d)\n",
                     src->id, dst->id);
        zprimAssetRelease(src);
        zprimAssetRelease(dst);
        return;
    }
    if (srcFrame < s->frameCount && dstFrame < d->frameCount) {
        zprimRleDecode((const uint8_t *)s + s->frames[srcFrame].pixelOffset,
                       (uint8_t *)d + d->frames[dstFrame].pixelOffset);
        zprimAssetRelease(src);
        zprimAssetRelease(dst);
        return;
    }
    gZdebugPrint("ZPRIM: Error - frame out of bounds during decompress\n");
    zprimAssetRelease(src);
    zprimAssetRelease(dst);
}

/* Address of the pixel data of frame `frame` inside a sprite set, or 0. */
BMX_ADDR(0x0047a300)
int zprimSpriteFrameData(const uint16_t *set, uint16_t frame)
{
    const ZPrimSet *s = (const ZPrimSet *)set;
    if (s->frameCount <= frame) {
        return 0;
    }
    return s->frames[frame].pixelOffset + (int)(intptr_t)set;
}

/*
 * Run length decoder. A block starts with its byte size (16 bit); then each token is: 0x80 value + u16
 * count (fill), 0 + u16 count (zeros), 1..0x7f literal bytes, or 0x81..0xff + value (fill of tag - 0x80).
 * Returns the number of bytes written.
 */
BMX_ADDR(0x0047a330)
int zprimRleDecode(const uint8_t *src, uint8_t *dst)
{
    const uint16_t *in = (const uint16_t *)src + 1;
    const uint16_t size = *(const uint16_t *)src;
    uint8_t *out = dst;
    while ((const uint8_t *)in < src + size) {
        const uint8_t tag = (uint8_t)*in;
        if (tag == 0x80) {
            const uint16_t n = in[1];
            zmemFill(out, ((const uint8_t *)in)[1], n);
            out += n;
            in += 2;
        } else if (tag == 0) {
            const uint16_t n = *(const uint16_t *)((const uint8_t *)in + 1);
            zmemFill(out, 0, n);
            out += n;
            in = (const uint16_t *)((const uint8_t *)in + 3);
        } else if ((tag & 0x80) == 0) {
            zmemCopy(out, (const uint8_t *)in + 1, tag);
            out += tag;
            in = (const uint16_t *)((const uint8_t *)in + tag + 1);
        } else {
            zmemFill(out, ((const uint8_t *)in)[1], tag - 0x80);
            out += tag - 0x80;
            in += 1;
        }
    }
    return (int)(out - dst);
}

/* ---- fonts and text ------------------------------------------------------------------------- */

/* Loads font `id`. The page table (`pages`) is filled in lazily when glyphs are drawn. */
BMX_ADDR(0x0047a420)
ZPrimAsset *zprimLoadFont(uint32_t id, int keep)
{
    if ((id & 0xe000) != 0x8000) {
        return NULL;
    }
    ZPrimAsset *node = zprimAssetFind((int16_t)id);
    int32_t offset = 0;
    if (node != NULL) {
        return node;
    }
    ZPrimAsset *group = zprimGroupFind(id);
    int inGroup;
    int32_t handle;
    if (group == NULL) {
        inGroup = 0;
        if (zassetFind(gZprimLibrary, (int)(id & 0xffff), 0) != 1) {
            gZdebugPrint("ZPRIM: Failed to find font %x\n", id & 0xffff);
            return NULL;
        }
        handle = zmemdmmAlloc(zassetSize(gZprimLibrary));
        if (handle == 0) {
            gZdebugPrint("ZPRIM: Failed to allocate font\n");
            return NULL;
        }
        zassetRead(gZprimLibrary, zmemdmmLock(handle));
        zmemdmmUnlock(handle);
    } else {
        inGroup = 1;
        if (group->flags & 1) {
            keep = 1;
        }
        handle = group->dmm;
        offset = group->offset;
    }
    node = zprimAssetAlloc();
    if (node == NULL) {
        zdebugFail("Possible memory leak of fntMemHdl here.", zprimFile, 0xd1b, 0);
        gZdebugPrint("ZPRIM: Failed to allocate font\n");
        return NULL;
    }
    node->flags = 0;
    if (keep != 0) {
        node->flags = 1;
    }
    if (inGroup) {
        node->flags |= 2;
    }
    node->id = (uint16_t)id;
    node->dmm = handle;
    node->offset = offset;
    ZPrimFont *data = zprimAssetData(node);
    data->pages = 0;
    zprimAssetRelease(node);
    return node;
}

BMX_ADDR(0x0047a5e0)
void zprimFreeFont(ZPrimAsset *a)
{
    zprimAssetFree(a);
}

BMX_ADDR(0x0047a600)
void zprimNop3(void) {}

BMX_ADDR(0x0047aa60)
void zprimNop4(void) {}

BMX_ADDR(0x0047c010)
void zprimNop5(void) {}

/* Finds the glyph record for `code`, retrying with the upper case letter. NULL when the char has no glyph. */
static const ZPrimGlyph *zprimFindGlyph(const ZPrimFont *font, uint32_t code, uint32_t first, int smallFont)
{
    for (;;) {
        const ZPrimGlyph *glyph = NULL;
        if (!smallFont) {
            const ZPrimGlyph *g = font->glyphs;
            int32_t counter = font->glyphCount;
            if (counter > 0) {
                do {
                    if (g->code == code) {
                        counter = -999; /* found: leaves the loop and marks the hit */
                    } else {
                        g++;
                    }
                    counter--;
                } while (counter > 0);
            }
            if (counter < -500) {
                glyph = g;
            }
        } else if (!((int32_t)code < (int32_t)first || (int32_t)(font->glyphCount + first) < (int32_t)code)) {
            /* The upper bound is glyphCount + first inclusive, one past the table. */
            glyph = font->glyphs + (code - first);
        }
        if (glyph != NULL) {
            return glyph;
        }
        if ((int32_t)code < 0x61 || (int32_t)code > 0x7a) {
            return NULL;
        }
        code -= 0x20;
    }
}

/* Decodes the character at *str (a lead byte 1 introduces a two byte code) and advances *str to its last
 * byte. */
static uint32_t zprimTextCode(const uint8_t **str)
{
    const uint8_t *s = *str;
    uint32_t code;
    if (*s == 1) {
        const uint32_t hi = s[1];
        s += 2;
        code = (hi - 2) * 0xfe - 2 + *s;
    } else {
        code = *s;
    }
    *str = s;
    return code;
}

/*
 * Draws `str` at (x, y) with the state's font: an optional background box per glyph (field4c colour) and the
 * glyph itself through the display list. With `proportional` each glyph advances by its own width, else only
 * by the spacing. Characters 0 to 2 end the string.
 */
static void zprimDrawTextImpl(const uint8_t *str, int x, int y, int proportional)
{
    int took;
    ZPrimState *st = zprimOpen(&took);
    ZPrimAsset *fontNode = st->font;
    if (fontNode == NULL) {
        zprimClose(took);
        return;
    }
    const ZPrimFont *font = zprimAssetData(fontNode);
    const int smallFont = font->glyphCount < 0x100;
    uint32_t first = 0;
    if (smallFont) {
        first = font->glyphs[0].code;
    }
    if (proportional && (st->flagsC & 2) != 0 && st->field34 != 0) {
        ngifSetBlend(st->field38, st->field34);
    }
    uint8_t c = *str;
    while (c > 2) {
        if ((int16_t)x >= gZprimClip.x1) {
            break;
        }
        uint32_t code = zprimTextCode(&str);
        const ZPrimGlyph *g = zprimFindGlyph(font, code, first, smallFont);
        if (g != NULL) {
            if ((st->flagsC & 1) == 0) {
                const int16_t w = g->width;
                const int16_t h = g->height;
                int16_t boxW = (int16_t)(g->trailingX + w + g->offsetX);
                int16_t boxH = (int16_t)(g->trailingY + h + g->offsetY);
                if (boxW > w && boxH > h) {
                    int xc = (int16_t)x;
                    if ((int16_t)x < gZprimClip.x0) {
                        boxW = (int16_t)(boxW + (int16_t)x - gZprimClip.x0);
                        xc = gZprimClip.x0;
                    }
                    if (boxW + xc > gZprimClip.x1) {
                        boxW = (int16_t)(gZprimClip.x1 - xc);
                    }
                    int yc = (int16_t)y;
                    if ((int16_t)y < gZprimClip.y0) {
                        boxH = (int16_t)(boxH + (int16_t)((int16_t)y - gZprimClip.y0));
                        yc = gZprimClip.y0;
                    }
                    if (boxH + yc > gZprimClip.y1) {
                        boxH = (int16_t)(gZprimClip.y1 - yc);
                    }
                    if (boxW > 0 && boxH > 0) {
                        ngifFillRect(st->field4c, 0, xc, yc, boxW + xc, boxH + yc);
                    }
                }
            }
            int mode = 0;
            int a = 0;
            int b = 0;
            const uint16_t f = st->flagsC;
            if (f & 2) {
                mode = 1;
            } else if (f & 4) {
                mode = 2;
                if (f & 1) {
                    a = st->field48;
                } else {
                    b = st->field4c;
                    a = st->field48;
                }
            } else if (f & 1) {
                mode = 3;
            }
            ngifDrawGlyph(font, g, mode, a, b, (int16_t)x + g->offsetX, (int16_t)y + g->offsetY,
                          (int)(((uint32_t)st->field40 & 0xffff0000u) | font->field14), st->field40);
            if (proportional) {
                x = (int16_t)(x + (int16_t)(g->trailingX + g->width + (uint16_t)st->spacing) + g->offsetX);
            } else {
                x = (int16_t)(x + (uint16_t)st->spacing);
            }
        }
        str++;
        c = *str;
    }
    if (proportional && (st->flagsC & 2) != 0 && st->field34 != 0) {
        ngifSetBlend(0, 0);
    }
    zprimAssetRelease(fontNode);
    zprimClose(took);
}

BMX_ADDR(0x0047a620)
void zprimDrawTextMono(const uint8_t *str, int x, int16_t y)
{
    zprimDrawTextImpl(str, x, y, 0);
}

BMX_ADDR(0x0047aa80)
void zprimDrawText(const uint8_t *str, int x, int16_t y)
{
    zprimDrawTextImpl(str, x, y, 1);
}

/* The state's font (or `font` when given): the flags word of its data. */
BMX_ADDR(0x0047af10)
uint16_t zprimFontFlags(ZPrimAsset *font)
{
    int took;
    zprimOpen(&took);
    if (font == NULL) {
        font = gZprimStates[gZprimDepth].font;
    }
    const ZPrimFont *data = zprimAssetData(font);
    if (data != NULL) {
        const uint16_t flags = data->flags;
        zprimAssetRelease(font);
        zprimClose(took);
        return flags;
    }
    return 0;
}

/* Width in pixels of `str` in the given font (the state's font when `font` is NULL), including spacing. */
BMX_ADDR(0x0047afd0)
int16_t zprimTextWidth(ZPrimAsset *font, const uint8_t *str)
{
    int16_t width = 0;
    int took;
    ZPrimState *st = zprimOpen(&took);
    if (font == NULL) {
        font = st->font;
    }
    const ZPrimFont *fdata = zprimAssetData(font);
    int smallFont = fdata->glyphCount < 0x100;
    uint32_t first = 0;
    if (smallFont) {
        first = fdata->glyphs[0].code;
    }
    uint8_t c = *str;
    while (c >= 3) {
        uint32_t code = zprimTextCode(&str);
        const ZPrimGlyph *g = zprimFindGlyph(fdata, code, first, smallFont);
        if (g != NULL) {
            width = (int16_t)(width + g->trailingX + g->offsetX + g->width + (int16_t)st->spacing);
        }
        str++;
        c = *str;
    }
    zprimAssetRelease(font);
    zprimClose(took);
    return width;
}

/*
 * Renders `str` with a font into frame `frame` of a virtual sprite at (x, y): glyph pixels 1 take `color`,
 * other non-zero pixels are copied.
 */
BMX_ADDR(0x0047bd20)
void zprimDrawTextToSprite(ZPrimAsset *font, const uint8_t *str, uint32_t x, uint32_t y, ZPrimAsset *sprite,
                           uint16_t frame, uint8_t color)
{
    if ((font->id & 0xe000) == 0x8000 && (sprite->id & 0xe000) == 0xc000) {
        ZPrimSet *sdata = zprimAssetData(sprite);
        if (sdata->frameCount <= frame) {
            gZdebugPrint("ZPRIM: Error - frame out of bounds during drawtexttosprite\n");
            zprimAssetRelease(sprite);
            return;
        }
        const uint32_t pitch = sdata->frames[frame].width;
        uint8_t *dstBase =
            (uint8_t *)sdata + (x & 0xffff) + (y & 0xffff) * pitch + sdata->frames[frame].pixelOffset;
        int took;
        ZPrimState *st = zprimOpen(&took);
        ZPrimFont *fdata = zprimAssetData(font);
        uint8_t ch = *str;
        while (ch != 0) {
            /* Indexed by character as in a small font, without a range check. */
            const ZPrimGlyph *g = &fdata->glyphs[ch - fdata->glyphs[0].code];
            const uint8_t *src =
                (const uint8_t *)fdata + g->y * (uint32_t)fdata->pitch + g->x + fdata->pixelOffset;
            if (g->height != 0) {
                uint32_t row = 0;
                do {
                    if (g->width != 0) {
                        int col = 0;
                        do {
                            const int8_t px = (int8_t)src[col];
                            if (px != 0) {
                                dstBase[g->offsetX + col + (g->offsetY + (int)row) * (int)pitch] =
                                    px == 1 ? color : (uint8_t)px;
                            }
                            col++;
                        } while (col < g->width);
                    }
                    src += fdata->pitch;
                    row++;
                } while ((int)row < g->height);
            }
            dstBase += g->trailingX + g->offsetX + g->width + st->spacing;
            str++;
            ch = *str;
        }
        zprimAssetRelease(sprite);
        zprimAssetRelease(font);
        zprimClose(took);
        return;
    }
    gZdebugPrint("ZPRIM: Error - can only drawtexttosprite from font to virtual sprite (%d->%d)\n", font->id,
                 sprite->id);
}

/* ---- groups, screens and generic asset handling --------------------------------------------- */

/* Extracts picture `entry` of group file `group` into an asset of its own (a copy of the picture data). */
BMX_ADDR(0x0047b220)
ZPrimAsset *zprimLoadGroupPicture(uint16_t group, uint16_t entry)
{
    ZPrimAsset *node = NULL;
    /* Zero-extended like the original (AND EAX,0xffff), see zprimLoadGroup. */
    if (zassetFind(gZprimLibrary, (int)group, 0) != 1) {
        gZdebugPrint("ZPRIM: Failed to find group\n");
        return NULL;
    }
    const int32_t handle = zmemdmmAlloc(zassetSize(gZprimLibrary));
    if (handle == 0) {
        gZdebugPrint("ZPRIM: Failed to allocate group\n");
        return NULL;
    }
    uint32_t *table = zmemdmmLock(handle);
    zassetRead(gZprimLibrary, table);
    uint32_t *e = table + 1;
    for (uint32_t i = 0; i < table[0]; i++, e += 2) {
        if (e[0] == entry && (e[0] & 0xe000) == 0) {
            const ZPrimSet *picture = (const ZPrimSet *)((uint8_t *)table + e[1]);
            if (picture->frameCount == 1) {
                uint32_t size = (uint32_t)picture->height * picture->width;
                if (picture->paletteOffset != 0) {
                    size += 0x204; /* 256 colours plus the palette header */
                }
                size += sizeof(ZPrimSet) + sizeof(ZPrimFrame);
                const int32_t copy = zmemdmmAlloc(size);
                if (copy != 0) {
                    node = zprimAssetAlloc();
                    if (node != NULL) {
                        ZPrimSet *dst = zmemdmmLock(copy);
                        zmemCopy(dst, picture, size);
                        dst->frames[0].texture = 0;
                        zmemdmmUnlock(copy);
                        node->flags = 1;
                        node->id = entry;
                        node->dmm = copy;
                        node->offset = 0;
                    }
                }
            }
            break;
        }
    }
    zmemdmmUnlock(handle);
    zmemdmmFree(handle);
    return node;
}

/* Loads group file `group` into slot `slot` (0 to 3) and preloads every asset it lists. Returns 0 on success.
 */
BMX_ADDR(0x0047b400)
int zprimLoadGroup(uint16_t slot, uint16_t group)
{
    uint32_t *table = NULL;
    if (slot > 3) {
        gZdebugPrint("ZPRIM: Error - bad slot for group\n");
        return 1;
    }
    if (gZprimGroups[slot] != NULL) {
        gZdebugPrint("ZPRIM: Error - loading group into used slot\n");
        return 1;
    }
    int32_t handle = 0;
    for (;;) {
        /* Zero-extended (AND EDI,0xffff): group 0xe000 must stay 57344, not -8192 (endless retry in-game). */
        while (zassetFind(gZprimLibrary, (int)group, 0) != 1) {
            printf("ZPRIM: Failed to find group\n");
        }
        if (handle == 0) {
            handle = zmemdmmAlloc(zassetSize(gZprimLibrary));
            if (handle == 0) {
                printf("ZPRIM: Failed to allocate group\n");
                continue;
            }
            table = zmemdmmLock(handle);
        }
        if (zassetRead(gZprimLibrary, table) == 1) {
            ZPrimAsset *node = zprimAssetAlloc();
            if (node == NULL) {
                gZdebugPrint("ZPRIM: Failed to allocate group\n");
                return 1;
            }
            node->flags = 1;
            node->id = group;
            node->dmm = handle;
            node->offset = 0;
            gZprimGroups[slot] = node;
            zmemdmmUnlock(handle);
            const uint32_t *entry = table + 1;
            for (uint32_t i = 0; i < table[0]; i++, entry += 2) {
                zprimLoadAsset((uint32_t)(int)(int16_t)entry[0]);
            }
            return 0;
        }
        printf("ZPRIM: Load Failed\n");
    }
}

/* Frees a group slot: the assets that live inside it, then the group data itself. */
BMX_ADDR(0x0047b5c0)
void zprimFreeGroup(uint16_t slot)
{
    if (slot >= 4) {
        return;
    }
    ZPrimAsset *group = gZprimGroups[slot];
    if (group == NULL) {
        return;
    }
    uint32_t *table = zmemdmmLock(group->dmm);
    for (uint32_t i = 0; i < table[0]; i++) {
        ZPrimAsset *a = zprimAssetFind((int16_t)table[i * 2 + 1]);
        if (a != NULL && (a->flags & 2) != 0) {
            zmemdmmUnlock(group->dmm);
            zprimAssetFree(a);
            table = zmemdmmLock(group->dmm);
        }
    }
    zmemdmmUnlock(group->dmm);
    zmemdmmFree(group->dmm);
    group->dmm = 0;
    zprimAssetUnlink(group);
    gZprimGroups[slot] = NULL;
}

/*
 * Releases an asset: its textures (picture, every sprite frame, font pages), the DMM block unless it lives
 * inside a group, and the table node.
 */
BMX_ADDR(0x0047b6b0)
void zprimAssetFree(ZPrimAsset *a)
{
    const uint16_t type = a->id & 0xe000;
    int locked = 1;
    if (type == 0) {
        ZPrimSet *data = zprimAssetData(a);
        if (data->frames[0].texture != 0) {
            gRwTextureFree(data->frames[0].texture);
        }
    } else if (type == 0x4000 || type == 0x6000 || type == 0xc000) {
        ZPrimSet *data = zprimAssetData(a);
        for (int32_t i = (int32_t)data->frameCount - 1; i >= 0; i--) {
            int32_t *tex = &data->frames[i].texture;
            if (*tex != 0) {
                gRwTextureFree(*tex);
            }
            *tex = 0;
        }
    } else if (type == 0x8000) {
        ZPrimFont *data = zprimAssetData(a);
        int32_t *pages = bmxSlot32Get(data->pages);
        if (pages != NULL) {
            for (int i = 0; i < 32; i++) {
                int32_t tex = pages[i];
                if (tex != 0) {
                    gRwTextureFree(tex);
                    pages[i] = 0;
                }
            }
            zmemFreeListFree(gZprimFontPageList, pages);
            bmxSlot32Drop(data->pages);
            data->pages = 0;
        }
    } else {
        locked = 0;
    }
    if (locked) {
        zprimAssetRelease(a);
    }
    if ((a->flags & 2) == 0) {
        zmemdmmFree(a->dmm);
        a->dmm = 0;
    }
    zprimAssetUnlink(a);
}

/* Loads any asset by id, dispatching on the type bits (top three bits of the id). */
BMX_ADDR(0x0047b950)
ZPrimAsset *zprimLoadAsset(uint32_t id)
{
    ZPrimAsset *found = zprimAssetFind((int16_t)id);
    if (found != NULL) {
        return found;
    }
    const uint16_t type = (uint16_t)id & 0xe000;
    if (type == 0) {
        return zprimLoadPicture(id, 0);
    }
    if (type == 0x4000 || type == 0x6000 || type == 0xc000) {
        return zprimLoadSprite(id, 0);
    }
    if (type == 0x8000) {
        return zprimLoadFont(id, 0);
    }
    if (type == 0xa000) {
        return zprimLoadScreen(id, 0);
    }
    zdebugFail("ZPRIM: Failed to load asset", zprimFile, 0x14b2, 0);
    return NULL;
}

/* Loads screen `id` (type 0xa000), which has no data of its own to prepare. */
BMX_ADDR(0x0047bb90)
ZPrimAsset *zprimLoadScreen(uint32_t id, int keep)
{
    if ((id & 0xe000) != 0xa000) {
        return NULL;
    }
    ZPrimAsset *node = zprimAssetFind((int16_t)id);
    if (node != NULL) {
        return node;
    }
    int32_t handle;
    int32_t offset;
    int inGroup;
    ZPrimAsset *group = zprimGroupFind(id);
    if (group == NULL) {
        offset = 0;
        inGroup = 0;
        if (zassetFind(gZprimLibrary, (int)(id & 0xffff), 0) != 1) {
            gZdebugPrint("ZPRIM: Failed to find screen\n");
            return NULL;
        }
        handle = zmemdmmAlloc(zassetSize(gZprimLibrary));
        if (handle == 0) {
            gZdebugPrint("ZPRIM: Failed to allocate picture\n");
            return NULL;
        }
        zassetRead(gZprimLibrary, zmemdmmLock(handle));
        zmemdmmUnlock(handle);
    } else {
        handle = group->dmm;
        offset = group->offset;
        inGroup = 1;
    }
    node = zprimAssetAlloc();
    if (node == NULL) {
        gZdebugPrint("ZPRIM: Failed to allocate screen\n");
        return NULL;
    }
    node->flags = 0;
    if (keep != 0) {
        node->flags = 1;
    }
    if (inGroup) {
        node->flags |= 2;
    }
    node->id = (uint16_t)id;
    node->dmm = handle;
    node->offset = offset;
    return node;
}

BMX_ADDR(0x0047bd00)
void zprimFreeScreen(ZPrimAsset *a)
{
    zprimAssetFree(a);
}

/* Set by zprimEndFrame (read by the display list code of the frame). */
#define gZprimFrameEnded ORIG_VAR(0x005d2504, uint32_t, gZprimFrameEnded)

BMX_ADDR(0x00477d00)
void zprimEndFrame(void)
{
    gZprimFrameEnded = 1;
}
