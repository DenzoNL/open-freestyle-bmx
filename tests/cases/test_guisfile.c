/* Tests for guisfile.c: building a screen from a screen asset. */
#include "bmx/com/menu/gui/guisfile.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_gui.h"

enum { kScreenId = 0x1aa, kStringArea = 512 };

/* Asset ids the generated records may refer to (all exist, so no file access is attempted). */
static const uint16_t kSpriteIds[] = {0x150, 0x151, 0x152, 0x153};
static const uint16_t kFontIds[] = {0x160, 0x161};
static const uint16_t kBackdropId = 0x170;

static uint16_t pick(const uint16_t *ids, uint32_t n)
{
    return ids[dt_u32() % n];
}

/* Offset of a random string in the string area, or 0 for "none". */
static uint32_t string_offset(void)
{
    return dt_u32() % 3 == 0 ? 0 : 16 + 32 * (dt_u32() % 8);
}

static void fill_element(GuiScreenFileElement *rec, int index)
{
    static const uint16_t types[] = {1, 2, 3, 3, 5, 6, 7, 8, 4, 9};
    memset(rec, 0, sizeof *rec);
    rec->magic = gGuiElementMagic;
    rec->id = 0x100u * (uint32_t)(index + 1) + (dt_u32() & 0xff);
    for (int i = 0; i < 4; i++) {
        rec->nav[i] = dt_u32() % 3 == 0 ? 0 : (uint8_t)dt_u32();
    }
    rec->flags = (uint16_t)dt_u32();
    if (dt_u32() % 3 != 0) {
        rec->flags &= (uint16_t)~0x10; /* focus request: only some elements */
    }
    rec->type = types[dt_u32() % 10];
    rec->alpha = (uint16_t)dt_u32();
    rec->x = (int16_t)dt_range(-300, 700);
    rec->y = (int16_t)dt_range(-300, 500);
    rec->w = (int16_t)dt_range(0, 300);
    rec->h = (int16_t)dt_range(0, 200);
    rec->scaleX256 = (int16_t)dt_range(-600, 600);
    rec->scaleY256 = (int16_t)dt_range(-600, 600);
    rec->slideStartX = (int16_t)dt_range(-200, 200);
    rec->slideStartY = (int16_t)dt_range(-200, 200);
    rec->order = (uint16_t)dt_u32();
    rec->sprite = dt_u32() % 3 == 0 ? 0 : pick(kSpriteIds, 4);
    rec->frame = (uint16_t)dt_range(0, 3);
    rec->animCount = (uint16_t)dt_range(0, 30);
    for (int i = 0; i < 30; i++) {
        rec->animFrames[i] = (uint16_t)dt_range(0, 3);
        rec->animHold[i] = (uint16_t)dt_range(0, 9);
    }
    rec->spriteFocused = dt_u32() % 3 == 0 ? 0 : pick(kSpriteIds, 4);
    rec->animFlags = (uint16_t)dt_u32();
    rec->spriteX = (int16_t)dt_range(-20, 60);
    rec->spriteY = (int16_t)dt_range(-20, 60);
    rec->font = dt_u32() % 3 == 0 ? 0 : pick(kFontIds, 2);
    rec->textFlags = (uint16_t)dt_u32();
    rec->textX = (int16_t)dt_range(-10, 40);
    rec->textY = (int16_t)dt_range(-10, 40);
    rec->textOffset = string_offset();
    for (int i = 0; i < 3; i++) {
        rec->textRgb[i] = (uint8_t)dt_u32();
    }
    rec->textSpacing = (uint16_t)dt_range(0, 9);
    rec->toggleFont = dt_u32() % 3 == 0 ? 0 : pick(kFontIds, 2);
    rec->toggleFlags = (uint16_t)dt_u32();
    rec->toggleX = (int16_t)dt_range(-10, 40);
    rec->toggleY = (int16_t)dt_range(-10, 40);
    rec->toggleChoicesOffset = string_offset();
    for (int i = 0; i < 3; i++) {
        rec->toggleRgb[i] = (uint8_t)dt_u32();
    }
    rec->toggleSpacing = (uint16_t)dt_range(0, 9);
}

/* A screen asset with `count` element records (all flagged valid unless asked otherwise). */
static void make_screen(uint32_t seed, int count, int badScreenMagic, int badElementIndex)
{
    const uint32_t size =
        sizeof(GuiScreenFile) + (uint32_t)count * sizeof(GuiScreenFileElement) + kStringArea;
    uint8_t *blob = dt_alloc(size);
    GuiScreenFile *file = (GuiScreenFile *)blob;
    file->magic = badScreenMagic ? 0x12345678u : gGuiScreenMagic;
    file->backdrop = seed % 3 == 0 ? 0 : kBackdropId;
    file->elementCount = (uint32_t)count;
    file->elementsOffset = sizeof *file;
    file->stringsOffset = sizeof *file + (uint32_t)count * (uint32_t)sizeof(GuiScreenFileElement);
    file->focusFont = dt_u32() % 3 == 0 ? 0 : pick(kFontIds, 2);
    file->normalFont = dt_u32() % 3 == 0 ? 0 : pick(kFontIds, 2);
    for (int i = 0; i < 3; i++) {
        file->focusRgb[i] = (uint8_t)dt_u32();
        file->normalRgb[i] = (uint8_t)dt_u32();
    }
    GuiScreenFileElement *recs = (GuiScreenFileElement *)(blob + file->elementsOffset);
    for (int i = 0; i < count; i++) {
        fill_element(&recs[i], i);
    }
    if (badElementIndex >= 0 && badElementIndex < count) {
        recs[badElementIndex].magic ^= 0x5a5a5a5au;
    }
    char *strings = (char *)(blob + file->stringsOffset);
    for (int s = 0; s < 8; s++) {
        char *dst = strings + 16 + 32 * s;
        const int toggleList = s % 2;
        const int len = dt_range(1, 20);
        for (int i = 0; i < len; i++) {
            dst[i] = toggleList && dt_u32() % 4 == 0 ? '|' : (char)dt_range('a', 'z');
        }
    }
    dtGuiMakeAsset(kScreenId, blob, size);
}

static void setup_build(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(0x20000);
    for (int i = 0; i < 4; i++) {
        dtGuiMakeSprite(kSpriteIds[i], 4);
    }
    for (int i = 0; i < 2; i++) {
        dtGuiMakeFont(kFontIds[i], (uint16_t)dt_range(6, 18));
    }
    dtGuiMakeSprite(kBackdropId, 2);
    gGuiPulse = dt_range(0x10, 0xf0);
    gGuiScreen.hook = dtGuiHook;
    const int count = dt_range(0, 5);
    make_screen(c->seed, count, c->seed % 17 == 16, c->seed % 11 == 10 ? dt_range(0, 4) : -1);
    if (c->seed % 13 == 12) { /* the element table is full */
        for (int i = 0; i < kGuiMaxElements; i++) {
            dtGuiRandomElement(i);
        }
    }
    DT_ARG_INT(c, 0, kScreenId | (dt_u32() & 0xffff0000u));
}

static void setup_build_missing(DtCtx *c)
{
    setup_build(c);
    /* id 0xe0aa selects no asset class: the lookup aborts */
    DT_ARG_INT(c, 0, 0xe0aau);
}

/* The asset archive handle of the sprite layer; an empty directory makes every lookup fail quietly. */
#define gZprimArchive ORIG_VAR(0x00678400, void *, gZprimArchive)

static void setup_build_not_found(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    gZprimArchive = dt_alloc(0x130);
    DT_ARG_INT(c, 0, 0xfed | (dt_u32() & 0xffff0000u));
}

DT_CASES(guisfile_cases) = {
    {"guiBuildScreenNotFound", 0x00439a00, guiBuildScreen, 1, DT_RET_INT, setup_build_not_found, 10, 0},
    {"guiBuildScreen", 0x00439a00, guiBuildScreen, 1, DT_RET_INT, setup_build, 400, 0},
    {"guiBuildScreenBadId", 0x00439a00, guiBuildScreen, 1, DT_RET_INT, setup_build_missing, 10, 0},
};
