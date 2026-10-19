/* Tests for guilabel.c: the label element handler and the text markup parser. */
#include "bmx/com/menu/gui/guilabel.h"
#include "bmx/com/menu/gui/guimain.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_gui.h"

static void setup_target(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    DT_ARG_INT(c, 0, dt_u32());
}

/* Strings of digits, signs, separators and a few stray characters. */
static void setup_parse_int(DtCtx *c)
{
    static const char alphabet[] = "0123456789-, 09 --,,x$!";
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    const int len = dt_range(0, 14);
    char *s = dt_alloc((uint32_t)len + 1);
    for (int i = 0; i < len; i++) {
        s[i] = alphabet[dt_u32() % (sizeof alphabet - 1)];
    }
    if (c->seed % 11 == 10) {
        s[0] = (char)dt_range(0x80, 0xff);
    }
    DT_ARG_PTR(c, 0, s);
    DT_ARG_PTR(c, 1, c->seed % 5 == 0 ? NULL : dt_alloc(4));
}

/* A random chain of markup commands (some malformed); every part starts with '!'. */
static char *random_markup(void)
{
    static const char *const parts[] = {
        "!",      "!S 1,2,3",        "!s 10 -4 2", "!B$ab$", "!b$ $",     "!T 3 4 $x~y$", "!t5,6$q$", "!X",
        "!S 1 2", "!B$unterminated", "!B",         "!T 7",   "!S -3,5,1", "!B$q!",        "!!"};
    char *s = dt_alloc(160);
    char *at = s;
    const int n = dt_range(1, 5);
    for (int i = 0; i < n; i++) {
        const char *p = parts[dt_u32() % (sizeof parts / sizeof parts[0])];
        while (*p != '\0') {
            *at++ = *p++;
        }
        if (dt_u32() % 4 == 0) {
            *at++ = ' ';
        }
    }
    *at = '\0';
    return s;
}

static void setup_markup(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    dtGuiRandomScreen(dt_range(1, 4));
    dtGuiMakeSprite(0x120, 4);
    if (c->seed % 3 != 0) {
        void *sprite = dtGuiMakeSprite(0x121, 4);
        DT_GUI_ORIG(0x00478b60, void (*)(void *))(sprite);
    }
    gGuiMarkupTarget = c->seed % 4 == 0 ? 0 : (c->seed % 5 == 0 ? dt_u32() : gGuiSorted[0]->id);
    for (int i = 0; i < gGuiElementCount; i++) {
        if (dt_u32() & 1) {
            gGuiSorted[i]->textHandle = dtGuiTextHandle(dtGuiRandomText(6, 0));
            gGuiSorted[i]->textCapacity = 7;
        }
    }
    DT_ARG_PTR(c, 0, c->seed % 17 == 16 ? dtGuiRandomText(5, 0) : random_markup());
    DT_ARG_INT(c, 1, dt_range(-100, 700));
    DT_ARG_INT(c, 2, dt_range(-100, 500));
}

/* A fully populated label: sprite, optional focused sprite, animation, text with markup lines. */
static void setup_label(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    GuiElement *el = dtGuiRandomElement(0);
    const int frames = dt_range(1, 4);
    el->sprite = c->seed % 4 == 0 ? NULL : dtGuiMakeSprite(0x130, frames);
    el->spriteFocused = c->seed % 3 == 0 ? dtGuiMakeSprite(0x131, frames) : NULL;
    el->font = dtGuiMakeFont(0x140, (uint16_t)dt_range(6, 18));
    el->flags &= (uint16_t)~(kGuiFlagHidden);
    el->frame = (uint16_t)dt_range(0, frames + 1); /* the last value is past the end: nothing drawn */
    el->animFlags = (uint16_t)dt_u32();
    if (dt_u32() % 3 != 0) { /* sprite part runs (animation flags 1 and 2 clear) */
        el->animFlags &= (uint16_t)~kGuiAnimDisabled;
    }
    if (dt_u32() % 2) {
        el->animFlags |= kGuiAnimPlaying;
    }
    el->animCount = (uint16_t)dt_range(0, 5);
    switch (dt_u32() % 3) { /* start at either end of the animation to reach the wrap-around cases */
    case 0:
        el->animIndex = 0;
        break;
    case 1:
        el->animIndex = (int16_t)(el->animCount > 0 ? el->animCount - 1 : 0);
        break;
    default:
        el->animIndex = (int16_t)dt_range(0, el->animCount > 0 ? el->animCount - 1 : 0);
    }
    el->animTicks = (uint16_t)dt_range(0, 2);
    for (int i = 0; i < 30; i++) {
        el->animFrames[i] = (uint16_t)dt_range(0, frames + 1);
        el->animHold[i] = (uint16_t)dt_range(0, 2);
    }
    if (dt_u32() % 4 != 0) {
        el->textFlags &= (uint16_t)~kGuiTextDisabled;
        char *text = dtGuiRandomText(30, 1);
        if (dt_u32() % 4 == 0) {
            char *markup = random_markup();
            text = markup;
        }
        el->textHandle = dtGuiTextHandle(text);
        el->textCapacity = (uint32_t)strlen(text) + 1;
    }
    gGuiPulse = dt_range(0x10, 0xf0);
    DT_ARG_INT(c, 0, c->seed % 9 == 0 ? dt_range(0, 0x40) : (c->seed % 5 == 0 ? kGuiMsgTick : kGuiMsgDraw));
    DT_ARG_PTR(c, 1, el);
    DT_ARG_INT(c, 2, dt_u32() % 3 != 0);
    DT_ARG_INT(c, 3, dt_u32());
}

DT_CASES(guilabel_cases) = {
    {"guiSetMarkupTarget", 0x00436aa0, guiSetMarkupTarget, 1, DT_RET_VOID, setup_target, 200, 0},
    {"guiParseInt", 0x004367c0, guiParseInt, 2, DT_RET_INT, setup_parse_int, 400, 0},
    {"guiParseMarkup", 0x00436840, guiParseMarkup, 3, DT_RET_VOID, setup_markup, 500, 0},
    {"guiLabelProc", 0x00439f80, guiLabelProc, 4, DT_RET_INT, setup_label, 500, 0},
};
