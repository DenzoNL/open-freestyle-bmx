/*
 * Fonts (C:\NgEngine\Zge\ZgeMods\zFont.c): loads the "ZFNT" chunks of IFF files into glyph tables.
 * A font is split into sets of glyphs; ranges map runs of characters onto glyphs of their set.
 */
#ifndef ZGE_ZGEMODS_ZFONT_H
#define ZGE_ZGEMODS_ZFONT_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/com/zglrw/zrwzgl.h"

enum { kZfontVersion = 0x101 };

/* 0x1c bytes read from the chunk (offsets 0, 4, 0x10 and 0x14); 8 is cleared, the rest is never set. */
typedef struct ZFontGlyph {
    uint32_t field00;
    uint32_t field04;
    uint32_t field08;
    uint8_t pad0c[4];
    uint32_t field10;
    uint32_t field14;
    uint8_t pad18[4];
} ZFontGlyph;

typedef struct ZFontRange {
    uint32_t field00;
    ZFontGlyph **glyphs; /* 0x04 */
    uint32_t count;      /* 0x08 */
} ZFontRange;

typedef struct ZFontSet {
    uint16_t glyphCount; /* 0x00 */
    uint16_t rangeCount; /* 0x02 */
    ZFontGlyph *glyphs;  /* 0x04 */
    ZFontRange *ranges;  /* 0x08 */
} ZFontSet;

/* A loaded font (0x14 bytes): the list node (next, prev) is at +4. */
typedef struct ZFont {
    uint32_t field00;  /* 0x00 set to 1 */
    void *next, *prev; /* 0x04 */
    uint16_t setCount; /* 0x0c */
    uint8_t pad0e[2];
    ZFontSet *sets; /* 0x10 points 4 bytes into an array that starts with the set count */
} ZFont;

_Static_assert(sizeof(ZFont) == 0x14, "font layout");
_Static_assert(sizeof(ZFontGlyph) == 0x1c, "glyph layout");

typedef struct ZFontState {
    int32_t initialized; /* 0x6763d0 */
    ZrwList fonts;       /* 0x6763d4 */
} ZFontState;

#define gFont ORIG_VAR(0x006763d0, ZFontState, gFont)

/* Registers the "ZFNT" chunk handler; asserts when called twice. Returns 1. */
int zfontInitialize(void);
/* Chunk handler: reads the chunk and loads the font from it. Returns 1. */
int zfontChunkHandler(void *iff);
/* Loads a font from its chunk data (version 0x101); asserts on other versions. */
int zfontParse(const uint8_t *data);

#endif
