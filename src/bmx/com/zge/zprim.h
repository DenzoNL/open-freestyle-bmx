/* 2D primitives and sprite assets (C:\zaxis\Bmx\src\com\zge\zprim.c). */
#ifndef BMX_COM_ZGE_ZPRIM_H
#define BMX_COM_ZGE_ZPRIM_H

#include <stdint.h>

#include "compat/orig.h"

/* Property ids of zprimSet / zprimGet. */
enum {
    kZprimScreen = 1,    /* kind 1: screen asset */
    kZprimPicture = 2,   /* kind 2: picture asset */
    kZprimSprite = 3,    /* kind 3: sprite asset */
    kZprimColorOnly = 4, /* kind 4: no asset */
    kZprimColor = 5,
    kZprimFlagA2 = 6,
    kZprimFlagB2 = 7,
    kZprimFlagB1 = 8,
    kZprimFlagB4 = 9,
    kZprimField20 = 10,
    kZprimField1c = 11,
    kZprimFlagB8 = 12,
    kZprimFlagB10 = 13,
    kZprimField30 = 14,
    kZprimField24 = 15,
    kZprimField34 = 16,
    kZprimField38 = 17,
    kZprimFlagC2 = 18,
    kZprimFlagC1 = 19,
    kZprimFlagC4 = 20,
    kZprimField4c = 21,
    kZprimField48 = 22,
    kZprimField40 = 23,
    kZprimSpacing = 24,
};

/* The current drawing state; a stack of them lives in one locked DMM block (see gZprimStates). */
typedef struct ZPrimState {
    struct ZPrimAsset *asset; /* 0x00 asset (screen / picture / sprite), by kind */
    uint16_t kind;            /* 0x04 1 screen, 2 picture, 3 sprite, 4 none */
    uint16_t kindArg;         /* 0x06 */
    uint16_t arg8;            /* 0x08 */
    uint16_t arg10;           /* 0x0a */
    uint32_t color;           /* 0x0c */
    uint16_t flagsA;          /* 0x10 */
    uint8_t pad12[2];
    struct ZPrimAsset *picture; /* 0x14 */
    uint16_t flagsB;            /* 0x18 */
    uint8_t pad1a[2];
    int32_t field1c;  /* 0x1c */
    int32_t field20;  /* 0x20 */
    uint16_t field24; /* 0x24 */
    uint8_t pad26[2];
    float scaleX;            /* 0x28 */
    float scaleY;            /* 0x2c */
    int32_t field30;         /* 0x30 */
    int32_t field34;         /* 0x34 */
    int32_t field38;         /* 0x38 */
    struct ZPrimAsset *font; /* 0x3c */
    int32_t field40;         /* 0x40 */
    uint16_t flagsC;         /* 0x44 */
    uint8_t pad46[2];
    int32_t field48; /* 0x48 */
    int32_t field4c; /* 0x4c */
    int32_t spacing; /* 0x50 */
} ZPrimState;

_Static_assert(sizeof(ZPrimState) == 0x54, "state size");
_Static_assert(__builtin_offsetof(ZPrimState, picture) == 0x14, "state layout");
_Static_assert(__builtin_offsetof(ZPrimState, scaleX) == 0x28, "state layout");
_Static_assert(__builtin_offsetof(ZPrimState, font) == 0x3c, "state layout");
_Static_assert(__builtin_offsetof(ZPrimState, flagsC) == 0x44, "state layout");
_Static_assert(__builtin_offsetof(ZPrimState, spacing) == 0x50, "state layout");

/*
 * A loaded asset (picture, sprite set, font, screen). The data lives in a DMM block; `offset` is where this
 * asset starts inside it.
 */
typedef struct ZPrimAsset {
    uint16_t id;             /* 0x00; top three bits give the type */
    uint16_t flags;          /* 0x02 bit 0: keep when flushing, bit 1: lives inside a group */
    struct ZPrimAsset *next; /* 0x04 */
    int32_t dmm;             /* 0x08 DMM handle of the data */
    int32_t offset;          /* 0x0c */
} ZPrimAsset;

_Static_assert(sizeof(ZPrimAsset) == 0x10, "asset size");

/*
 * Data of a picture or a sprite set (the block zprimAssetData returns). A picture is a set with one frame,
 * which is why its texture handle sits at 0x20 and the group loader copies 0x24 header bytes. Offsets are
 * relative to the start of the block. Pixel rows are `width` bytes wide (bpp 8) in sprite frames.
 */
typedef struct ZPrimFrame {
    uint16_t width;      /* 0x00 */
    uint16_t height;     /* 0x02 */
    int32_t pixelOffset; /* 0x04 */
    uint16_t originX;    /* 0x08 subtracted from the draw position (not for compressed sets) */
    uint16_t originY;    /* 0x0a */
    int32_t texture;     /* 0x0c renderer texture handle, 0 = none */
} ZPrimFrame;

typedef struct ZPrimSet {
    uint16_t frameCount;   /* 0x00 (1 for a picture) */
    uint16_t flags;        /* 0x02 bit 0: one 0x204 byte palette per frame */
    uint16_t width;        /* 0x04 */
    uint16_t height;       /* 0x06 */
    int32_t pixelOffset;   /* 0x08 pixels of frame 0 */
    int32_t paletteOffset; /* 0x0c {flags, count, colours}, 0 = none */
    uint16_t bpp;          /* 0x10 */
    uint8_t pad12[2];
    ZPrimFrame frames[]; /* 0x14 */
} ZPrimSet;

_Static_assert(sizeof(ZPrimFrame) == 0x10, "frame size");
_Static_assert(__builtin_offsetof(ZPrimFrame, pixelOffset) == 0x04, "frame layout");
_Static_assert(__builtin_offsetof(ZPrimFrame, originX) == 0x08, "frame layout");
_Static_assert(__builtin_offsetof(ZPrimFrame, texture) == 0x0c, "frame layout");
_Static_assert(sizeof(ZPrimSet) == 0x14, "set header size");
_Static_assert(__builtin_offsetof(ZPrimSet, pixelOffset) == 0x08, "set layout");
_Static_assert(__builtin_offsetof(ZPrimSet, paletteOffset) == 0x0c, "set layout");
_Static_assert(__builtin_offsetof(ZPrimSet, bpp) == 0x10, "set layout");
_Static_assert(__builtin_offsetof(ZPrimSet, frames) == 0x14, "set layout");
/* A picture's only texture handle is frame 0's. */
_Static_assert(sizeof(ZPrimSet) + __builtin_offsetof(ZPrimFrame, texture) == 0x20, "picture texture at 0x20");

/* One glyph record of a font (10 bytes). */
typedef struct ZPrimGlyph {
    uint8_t x, y;          /* 0x00 top left of the glyph in the pixel sheet */
    uint8_t width, height; /* 0x02 */
    int8_t offsetX;        /* 0x04 */
    int8_t offsetY;        /* 0x05 */
    int8_t trailingX;      /* 0x06 space after the glyph (adds to its advance and box) */
    int8_t trailingY;      /* 0x07 space below the glyph (adds to its box height) */
    uint16_t code;         /* 0x08 character code */
} ZPrimGlyph;

/*
 * Data of a font. Fonts with fewer than 0x100 glyphs are "small": they cover consecutive codes from the
 * code of glyph 0, and glyphs are found by index. Larger fonts are searched by code.
 */
typedef struct ZPrimFont {
    uint16_t glyphCount; /* 0x00 */
    uint16_t flags;      /* 0x02 returned by zprimFontFlags */
    int32_t pixelOffset; /* 0x04 */
    uint16_t pitch;      /* 0x08 bytes per row of the pixel sheet */
    uint8_t pad0a[2];
    int32_t *pages; /* 0x0c 32 texture handles, created when glyphs are drawn; NULL before */
    uint8_t pad10[4];
    uint16_t field14; /* 0x14 low half of the `c` argument zprimDrawText hands to ngifDrawGlyph */
    uint8_t pad16[2];
    ZPrimGlyph glyphs[]; /* 0x18 */
} ZPrimFont;

_Static_assert(sizeof(ZPrimGlyph) == 10, "glyph size");
_Static_assert(__builtin_offsetof(ZPrimGlyph, code) == 0x08, "glyph layout");
_Static_assert(sizeof(ZPrimFont) == 0x18, "font header size");
_Static_assert(__builtin_offsetof(ZPrimFont, pixelOffset) == 0x04, "font layout");
_Static_assert(__builtin_offsetof(ZPrimFont, pitch) == 0x08, "font layout");
_Static_assert(__builtin_offsetof(ZPrimFont, pages) == 0x0c, "font layout");
_Static_assert(__builtin_offsetof(ZPrimFont, field14) == 0x14, "font layout");
_Static_assert(__builtin_offsetof(ZPrimFont, glyphs) == 0x18, "font layout");

typedef struct ZPrimRect {
    int16_t x0, y0, x1, y1;
} ZPrimRect;

typedef ZPrimAsset *ZPrimGroupTable[4];

#define gZprimHandle  ORIG_VAR(0x006783fc, int32_t, gZprimHandle)
#define gZprimLibrary ORIG_VAR(0x00678400, int32_t, gZprimLibrary)
#define gZprimPool    ORIG_VAR(0x00678404, ZPrimAsset *, gZprimPool)
#define gZprimFree    ORIG_VAR(0x00678408, ZPrimAsset *, gZprimFree)
#define gZprimUsed    ORIG_VAR(0x0067840c, ZPrimAsset *, gZprimUsed)
#define gZprimClip    ORIG_VAR(0x00678410, ZPrimRect, gZprimClip)
#define gZprimGroups  ORIG_VAR(0x00678418, ZPrimGroupTable, gZprimGroups)
#define gZprimNextId  ORIG_VAR(0x00678428, uint16_t, gZprimNextId)
#define gZprimDepth   ORIG_VAR(0x0067842c, int32_t, gZprimDepth)
/* Locked state array while some zprim call has it open, else NULL. */
#define gZprimStates ORIG_VAR(0x00678430, ZPrimState *, gZprimStates)

/* Renderer texture entry points the exe stores in data (0x561024, 0x56103c, 0x561048). */
typedef void(BMX_CDECL *ZRwTextureFreeFn)(int32_t texture);
typedef int32_t(BMX_CDECL *ZRwTextureCloneFn)(int32_t texture);
typedef int32_t(BMX_CDECL *ZRwTextureLoadFn)(const void *bmp, uint32_t size, const char *name, int a, int b);
#define gRwTextureFree  ORIG_VAR(0x00561024, ZRwTextureFreeFn, gRwTextureFree)
#define gRwTextureClone ORIG_VAR(0x0056103c, ZRwTextureCloneFn, gRwTextureClone)
#define gRwTextureLoad  ORIG_VAR(0x00561048, ZRwTextureLoadFn, gRwTextureLoad)
/* Counter behind the "Sprite%d" texture names; only its low half is used. */
#define gZprimTextureCounter ORIG_VAR(0x0064eb34, uint32_t, gZprimTextureCounter)
/* Free list that hands out the 32 entry texture tables of fonts. */
struct ZMemFreeList;
#define gZprimFontPageList ORIG_VAR(0x0068ff94, struct ZMemFreeList *, gZprimFontPageList)

int zprimInit(void);
void zprimShutdown(void);
void zprimOpenLibrary(const char *name);
void zprimClear(void);
void zprimFlush(void);
void zprimSetClip(int x0, int y0, int x1, int y1);
void zprimStateSave(void);
void zprimStateRestore(void);
void zprimStatePush(void);
void zprimStatePop(void);
void zprimSet(uint16_t prop, int value);
uint32_t zprimGet(uint16_t prop);
void zprimSetScreen(ZPrimAsset *asset);
void zprimSetSource(ZPrimAsset *asset, int16_t type, uint16_t arg6, uint16_t arg8, uint16_t arg10);
void zprimSetFont(ZPrimAsset *font);
ZPrimAsset *zprimGetFont(void);
void zprimSetPicture(ZPrimAsset *picture);
void zprimSetScale(float x, float y);
void zprimStateReset(void);
ZPrimAsset *zprimGetPicture(void);
void zprimSetColor(uint32_t r, uint32_t g, uint32_t b, uint8_t prop);
void zprimGetColor(void);
void zprimDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void zprimFillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
ZPrimAsset *zprimAssetAlloc(void);
void zprimAssetUnlink(ZPrimAsset *a);
void *zprimAssetData(const ZPrimAsset *a);
void zprimAssetRelease(const ZPrimAsset *a);
ZPrimAsset *zprimAssetFind(int16_t id);
ZPrimAsset *zprimGroupFind(uint32_t id);
int zprimGroupLoaded(uint32_t slot);
uint16_t zprimGroupCount(uint32_t slot);
ZPrimAsset *zprimGroupSelect(uint32_t slot, uint32_t index);
int zprimCreateTexture(uint32_t w, uint32_t h, uint32_t bpp, const uint8_t *pixels, const uint16_t *palette);
ZPrimAsset *zprimLoadPicture(uint32_t id, int keep);
void zprimFreePicture(ZPrimAsset *a);
uint16_t zprimPictureWidth(const ZPrimAsset *a);
uint16_t zprimPictureHeight(const ZPrimAsset *a);
void zprimNop1(void);
/* Backdrop draw hook; empty on PC (callers still pass sprite, backdrop and two ints). */
void zprimNop2(const ZPrimAsset *sprite, const ZPrimAsset *backdrop, int a, int b);
void zprimCopyTexture(const ZPrimAsset *src, const ZPrimAsset *dst);
void zprimReplaceVramGrey(const ZPrimAsset *src, const ZPrimAsset *dst, int unused, uint32_t brightness);
void zprimReplaceVramPalette(const ZPrimAsset *src, const ZPrimAsset *dst, int unused, uint32_t index);
ZPrimAsset *zprimLoadSprite(uint32_t id, int keep);
ZPrimAsset *zprimCreateSprite(uint16_t w, uint16_t h, uint16_t count);
void zprimSpriteTouch(const ZPrimAsset *a);
void zprimSpriteReset(const ZPrimAsset *a, uint16_t w, uint16_t h, uint16_t bpp, uint16_t colors);
ZPrimAsset *zprimCreateSpriteSmall(uint16_t w, uint16_t h, uint16_t bpp, uint16_t colors);
void zprimFreeSprite(ZPrimAsset *a);
void zprimDrawSprite(uint32_t frame, int16_t x, int16_t y);
uint16_t zprimSpriteFrameWidth(ZPrimAsset *asset, uint16_t index);
uint16_t zprimSpriteFrameHeight(ZPrimAsset *asset, uint16_t index);
uint16_t zprimSpriteFrameCount(ZPrimAsset *asset);
void zprimDecompress(const ZPrimAsset *src, uint16_t srcFrame, const ZPrimAsset *dst, uint16_t dstFrame);
int zprimSpriteFrameData(const uint16_t *set, uint16_t frame);
int zprimRleDecode(const uint8_t *src, uint8_t *dst);
ZPrimAsset *zprimLoadFont(uint32_t id, int keep);
void zprimFreeFont(ZPrimAsset *a);
void zprimNop3(void);
void zprimNop4(void);
void zprimNop5(void);
void zprimDrawTextMono(const uint8_t *str, int x, int16_t y);
void zprimDrawText(const uint8_t *str, int x, int16_t y);
uint16_t zprimFontFlags(ZPrimAsset *font);
int16_t zprimTextWidth(ZPrimAsset *font, const uint8_t *str);
void zprimDrawTextToSprite(ZPrimAsset *font, const uint8_t *str, uint32_t x, uint32_t y, ZPrimAsset *sprite,
                           uint16_t frame, uint8_t color);
ZPrimAsset *zprimLoadGroupPicture(uint16_t group, uint16_t entry);
int zprimLoadGroup(uint16_t slot, uint16_t group);
void zprimFreeGroup(uint16_t slot);
void zprimAssetFree(ZPrimAsset *a);
ZPrimAsset *zprimLoadAsset(uint32_t id);
ZPrimAsset *zprimLoadScreen(uint32_t id, int keep);
void zprimFreeScreen(ZPrimAsset *a);

/* Marks the frame as finished (the original jumps through the ILT to the setter of the flag at 0x5d2504). */
void zprimEndFrame(void);

#endif
