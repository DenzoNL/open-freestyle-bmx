/* Fonts (C:\NgEngine\Zge\ZgeMods\zFont.c). */
#include "zge/zgemods/zfont.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/ziff.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char zfontFile[] = "C:\\NgEngine\\Zge\\ZgeMods\\zFont.c";

typedef int(BMX_CDECL *ZiffCallback)(void *iff);

/* Reads a little endian 16 bit value from the (unaligned) chunk data. */
static uint16_t read16(const uint8_t *p)
{
    uint16_t v;

    zmemCopy(&v, p, 2);
    return v;
}

static uint32_t read32(const uint8_t *p)
{
    uint32_t v;

    zmemCopy(&v, p, 4);
    return v;
}

BMX_ADDR(0x004a2760)
int zfontChunkHandler(void *iff)
{
    uint32_t size = ziffChunkSize(iff);
    uint8_t *data = zmemAlloc(size);

    ziffRead(iff, data, size);
    zfontParse(data);
    zmemFree(data);
    return 1;
}

BMX_ADDR(0x004a27a0)
int zfontInitialize(void)
{
    if (gFont.initialized != 0) {
        zdebugFail("Already initialized", zfontFile, 151, 0);
    }
    zmemFill(&gFont, 0, sizeof gFont);
    ziffSetupCallback(0x544e465a, BMX_FNPTR(ZiffCallback, 0x004a2760, zfontChunkHandler), 0);
    zrwListInit(&gFont.fonts);
    gFont.initialized = 1;
    return 1;
}

/* The part of zfontParse after the version check (a jump target in the original, entered with the data
 * pointer in EDI). */
BMX_ADDR(0x004a29d4)
int zfontParseBody(const uint8_t *p)
{
    ZFont *font = zmemAlloc(sizeof(ZFont));
    uint32_t *array;
    uint32_t i;
    uint16_t k;

    font->field00 = 1;
    font->setCount = 0;
    font->sets = 0;
    zmemCopy(&font->setCount, p, 2);
    if (font->setCount == 0) {
        zdebugFail("Bad font glyph Number", zfontFile, 261, 0);
    }
    zrwListAdd(&gFont.fonts, (void *)&font->next);
    array = zmemAlloc((uint32_t)font->setCount * sizeof(ZFontSet) + 4);
    array[0] = font->setCount;
    font->sets = (ZFontSet *)(array + 1);
    for (i = 0; i < font->setCount; i++) {
        font->sets[i].glyphCount = 0;
        font->sets[i].rangeCount = 0;
        font->sets[i].glyphs = 0;
        font->sets[i].ranges = 0;
    }
    p += 2;
    for (k = 0; k < font->setCount; k++) {
        ZFontSet *set = &font->sets[k];

        set->glyphCount = read16(p);
        p += 2;
        if (set->glyphCount != 0) {
            uint16_t g, r;

            set->rangeCount = read16(p);
            p += 2;
            set->ranges = zmemAlloc((uint32_t)set->rangeCount * sizeof(ZFontRange));
            set->glyphs = zmemAlloc((uint32_t)set->glyphCount * sizeof(ZFontGlyph));
            for (g = 0; g < set->glyphCount; g++) {
                ZFontGlyph *glyph = &set->glyphs[g];

                glyph->field00 = read32(p);
                glyph->field04 = read32(p + 4);
                glyph->field10 = read32(p + 8);
                glyph->field14 = read32(p + 12);
                p += 16;
                glyph->field08 = 0;
            }
            for (r = 0; r < set->rangeCount; r++) {
                ZFontRange *range = &set->ranges[r];
                uint16_t n = read16(p);
                uint16_t m;

                p += 2;
                range->glyphs = (ZFontGlyph **)zmemAlloc((uint32_t)n << 2);
                range->count = n;
                range->field00 = 0;
                for (m = 0; m < n; m++) {
                    range->glyphs[m] = &set->glyphs[read16(p)];
                    p += 2;
                }
            }
        }
    }
    return 1;
}

BMX_ADDR(0x004a2990)
int zfontParse(const uint8_t *data)
{
    if (read16(data) != kZfontVersion) {
        zdebugFail("Bad font version", zfontFile, 254, 0);
        return 0;
    }
    return zfontParseBody(data + 2);
}
