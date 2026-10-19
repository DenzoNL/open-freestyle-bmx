/* Tests for guitoggl.c: toggle data setters/getters and the two toggle element handlers. */
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/menu/gui/guitoggl.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_gui.h"

/* The asset archive handle of the sprite layer; an empty directory makes lookups fail quietly. */
#define gZprimArchive ORIG_VAR(0x00678400, void *, gZprimArchive)

enum { kFontId = 0x180, kSpriteId = 0x181 };

/* A toggle element with a data block; `shape` picks which parts of the block exist. */
static GuiElement *make_toggle(DtCtx *c)
{
    GuiElement *el = dtGuiRandomElement(0);
    el->type = kGuiTypeToggle;
    el->flags &= (uint16_t)~kGuiFlagHidden;
    if (c->seed % 11 != 10) {
        dtGuiMakeToggleData(el, c->seed % 5 != 0, c->seed % 3 != 0);
    }
    return el;
}

static void setup_nop(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
}

static void setup_t(DtCtx *c)
{
    setup_nop(c);
    GuiElement *el = make_toggle(c);
    if (c->seed % 23 == 22) {
        el->dataHandle = dt_range(0x1000, 0x7fffff); /* not a handle: locking fails and asserts */
    }
    DT_ARG_PTR(c, 0, el);
}

static void setup_t1(DtCtx *c)
{
    setup_t(c);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_t2(DtCtx *c)
{
    setup_t1(c);
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_t3(DtCtx *c)
{
    setup_t2(c);
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_set_font(DtCtx *c)
{
    setup_t(c);
    dtGuiMakeFont(kFontId, (uint16_t)dt_range(6, 18));
    if (c->seed % 6 == 5) { /* id that is not loaded and not in the (empty) archive: reports and returns */
        gZprimArchive = dt_alloc(0x130);
        DT_ARG_INT(c, 1, 0xfed | (dt_u32() & 0xffff0000u));
    } else {
        DT_ARG_INT(c, 1, kFontId | (dt_u32() & 0xffff0000u));
    }
}

/* Index setter with a choices list: indices inside, at the end of and past the list. */
static void setup_set_index(DtCtx *c)
{
    setup_t(c);
    GuiElement *el = (GuiElement *)(uintptr_t)c->args[0];
    if (c->seed % 13 == 12) {
        el = NULL;
    }
    DT_ARG_PTR(c, 0, el);
    DT_ARG_INT(c, 1,
               c->seed % 5 == 0 ? (dt_u32() & 0xffff0000u) | (uint32_t)dt_range(0, 6)
                                : (uint32_t)dt_range(0, 6));
}

static void setup_set_choices(DtCtx *c)
{
    setup_t(c);
    GuiElement *el = (GuiElement *)(uintptr_t)c->args[0];
    char *text = dt_alloc(40);
    const int n = dt_range(0, 4);
    char *at = text;
    for (int i = 0; i < n; i++) {
        const int len = dt_range(0, 5);
        for (int k = 0; k < len; k++) {
            *at++ = (char)dt_range('a', 'z');
        }
        if (i + 1 < n) {
            *at++ = '|';
        }
    }
    if (el->dataHandle != 0 && c->seed % 7 == 0 && c->seed % 23 != 22) {
        /* the same list again: no change, no message */
        GuiToggleData *data = zmemdmmLock(el->dataHandle);
        if (data->choicesHandle != 0) {
            const char *current = zmemdmmLock(data->choicesHandle);
            memcpy(text, current, strlen(current) + 1);
            zmemdmmUnlock(data->choicesHandle);
        }
        zmemdmmUnlock(el->dataHandle);
    }
    DT_ARG_PTR(c, 1, text);
}

/* ---- the element handlers --------------------------------------------------------------------- */

static void setup_proc(DtCtx *c)
{
    static const int msgs[] = {kGuiMsgTick,       kGuiMsgDraw,        kGuiMsgDraw,     kGuiMsgDraw,
                               kGuiMsgFocus,      kGuiMsgBlur,        kGuiMsgNavUp,    kGuiMsgNavDown,
                               kGuiMsgNavLeft,    kGuiMsgNavRight,    kGuiMsgNavUpAlt, kGuiMsgNavDownAlt,
                               kGuiMsgNavLeftAlt, kGuiMsgNavRightAlt, kGuiMsgCreate,   kGuiMsgNavLeft,
                               kGuiMsgNavRight,   kGuiMsgDraw};
    setup_nop(c);
    const int msg = msgs[dt_u32() % (sizeof msgs / sizeof msgs[0])];
    dtGuiRandomScreen(dt_range(0, 3)); /* other elements receive the blur messages */
    GuiElement *el = dtGuiRandomElement(10);
    el->type = kGuiTypeToggle;
    el->flags &= (uint16_t)~kGuiFlagHidden;
    el->id = 0x500u + (dt_u32() & 0xff);
    for (int i = 0; i < 4; i++) {
        el->nav[i] = dt_u32() % 2 ? 0 : (uint8_t)(dt_u32() & 3);
    }
    gGuiSorted[gGuiElementCount++] = el;
    dtGuiMakeToggleData(el, c->seed % 6 != 0, c->seed % 4 != 0);
    if (c->seed % 31 == 30) {
        el->dataHandle = 0;
    }
    const int frames = dt_range(1, 3);
    el->sprite = c->seed % 4 == 0 ? NULL : dtGuiMakeSprite(kSpriteId, frames);
    el->spriteFocused = c->seed % 5 == 0 ? dtGuiMakeSprite(kSpriteId + 1, frames) : NULL;
    el->frame = (uint16_t)dt_range(0, frames);
    el->font = dtGuiMakeFont(kFontId, (uint16_t)dt_range(6, 18));
    el->animFlags = (uint16_t)dt_u32() & (uint16_t)~kGuiAnimPlaying;
    el->textFlags = (uint16_t)(dt_u32() % 3 == 0 ? dt_u32() : dt_u32() | kGuiTextDisabled);
    if (el->dataHandle != 0) {
        GuiToggleData *data = zmemdmmLock(el->dataHandle);
        data->font = dtGuiMakeFont(kFontId + 2, (uint16_t)dt_range(6, 18));
        if (dt_u32() % 4 != 0) {
            data->textFlags &= (uint16_t)~kGuiTextDisabled;
        }
        if (data->textHandle != 0 && dt_u32() % 3 == 0) {
            char *text = zmemdmmLock(data->textHandle);
            if (text[0] != '\0') {
                text[dt_u32() % 2] = '\n'; /* a second line */
            }
            zmemdmmUnlock(data->textHandle);
        }
        if (c->seed % 41 == 40 && data->textHandle != 0 &&
            msg == kGuiMsgDraw) { /* only drawing checks the lock */
            data->textFlags &= (uint16_t)~kGuiTextDisabled;
            data->textHandle = dt_range(0x1000, 0x7fffff); /* text lock fails and asserts */
        }
        zmemdmmUnlock(el->dataHandle);
    }
    if (c->seed % 37 == 36) {
        el->dataHandle = dt_range(0x1000, 0x7fffff); /* data lock fails and asserts */
    }
    gGuiPulse = dt_range(0x10, 0xf0);
    DT_ARG_INT(c, 0, msg);
    DT_ARG_PTR(c, 1, el);
    DT_ARG_INT(c, 2, dt_u32() % 3 != 0);
    DT_ARG_INT(c, 3, dt_u32());
}

DT_CASES(guitoggl_cases) = {
    {"guiToggleSetTextFlags", 0x0043b560, guiToggleSetTextFlags, 2, DT_RET_VOID, setup_t1, 200, 0},
    {"guiToggleSetTextPos", 0x0043b5e0, guiToggleSetTextPos, 3, DT_RET_VOID, setup_t2, 200, 0},
    {"guiToggleSetTextStyle", 0x0043b660, guiToggleSetTextStyle, 2, DT_RET_VOID, setup_t1, 200, 0},
    {"guiToggleSetFont", 0x0043b6e0, guiToggleSetFont, 2, DT_RET_VOID, setup_set_font, 200, 0},
    {"guiToggleSetTextRgb", 0x0043b780, guiToggleSetTextRgb, 4, DT_RET_VOID, setup_t3, 200, 0},
    {"guiToggleSetSpacing", 0x0043b800, guiToggleSetSpacing, 2, DT_RET_VOID, setup_t1, 200, 0},
    {"guiToggleSetChoices", 0x0043b880, guiToggleSetChoices, 2, DT_RET_VOID, setup_set_choices, 400, 0},
    {"guiToggleSetIndex", 0x0043ba90, guiToggleSetIndex, 2, DT_RET_VOID, setup_set_index, 400, 0},
    {"guiToggleGetIndex", 0x0043bd10, guiToggleGetIndex, 1, DT_RET_INT16, setup_t, 200, 0},
    {"guiToggleSetCount", 0x0043bd90, guiToggleSetCount, 2, DT_RET_VOID, setup_t1, 200, 0},
    {"guiToggleGetCount", 0x0043be10, guiToggleGetCount, 1, DT_RET_INT16, setup_t, 200, 0},
    {"guiToggleProc", 0x0043a840, guiToggleProc, 4, DT_RET_INT, setup_proc, 600, 0},
    {"guiToggleProcHoriz", 0x0043aed0, guiToggleProcHoriz, 4, DT_RET_INT, setup_proc, 600, 0},
};
