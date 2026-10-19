/* Tests for guielem.c: element messaging, setters and getters, line/box elements, backdrop state. */
#include "bmx/com/menu/gui/guimain.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_gui.h"

/* ---- argument builders ------------------------------------------------------------------------ */

static void setup_el(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    DT_ARG_PTR(c, 0, dtGuiRandomElement(0));
}

static void setup_el1(DtCtx *c)
{
    setup_el(c);
    DT_ARG_INT(c, 1, dt_u32());
}

/* One flag argument: zero or non-zero. */
static void setup_el_flag(DtCtx *c)
{
    setup_el(c);
    DT_ARG_INT(c, 1, dt_u32() % 2 ? (dt_u32() | 1) : 0);
}

static void setup_el2(DtCtx *c)
{
    setup_el1(c);
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_el3(DtCtx *c)
{
    setup_el2(c);
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_el4(DtCtx *c)
{
    setup_el3(c);
    DT_ARG_INT(c, 4, dt_u32() % 4 == 0 ? 0 : dt_u32());
}

/* Output pointers that are sometimes NULL. */
static void *maybe_out(uint32_t size)
{
    return dt_u32() % 4 == 0 ? NULL : dt_alloc(size);
}

static void setup_el_out2(DtCtx *c)
{
    setup_el(c);
    DT_ARG_PTR(c, 1, maybe_out(4));
    DT_ARG_PTR(c, 2, maybe_out(4));
}

static void setup_el_out2_nn(DtCtx *c)
{
    setup_el(c);
    DT_ARG_PTR(c, 1, dt_alloc(4));
    DT_ARG_PTR(c, 2, dt_alloc(4));
}

static void setup_el_out1(DtCtx *c)
{
    setup_el(c);
    DT_ARG_PTR(c, 1, maybe_out(4));
}

static void setup_out3(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    for (uint32_t i = 0; i < 6; i++) {
        ((uint32_t *)&gGuiScreen)[i] = dt_u32();
    }
    DT_ARG_PTR(c, 0, maybe_out(4));
    DT_ARG_PTR(c, 1, maybe_out(4));
    DT_ARG_PTR(c, 2, maybe_out(4));
}

static void setup_screen_state(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    for (uint32_t i = 0; i < 6; i++) {
        ((uint32_t *)&gGuiScreen)[i] = dt_u32();
    }
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

/* ---- messaging -------------------------------------------------------------------------------- */

static void setup_send(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    GuiElement *el = dtGuiRandomElement(0);
    if (c->seed % 5 == 4) {
        el->proc = NULL;
        el->defaultProc = NULL; /* "No type handler" */
    }
    DT_ARG_INT(c, 0, dt_range(0, 0x40));
    DT_ARG_PTR(c, 1, c->seed % 9 == 8 ? NULL : el);
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_send_by_id(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    dtGuiRandomScreen(dt_range(0, 8));
    DT_ARG_INT(c, 0, dt_range(0, 0x40));
    DT_ARG_INT(c, 1,
               gGuiElementCount > 0 && c->seed % 4 != 3
                   ? gGuiSorted[dt_u32() % (uint32_t)gGuiElementCount]->id
                   : dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_broadcast(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    dtGuiRandomScreen(dt_range(0, 8));
    gGuiScreen.hook = c->seed % 3 == 0 ? NULL : dtGuiHook;
    DT_ARG_INT(c, 0, c->seed % 4 == 0 ? 0x15 + dt_range(0, 1) : dt_range(0, 0x40));
    DT_ARG_PTR(c, 1, c->seed % 5 == 4 ? NULL : (gGuiElementCount > 0 ? gGuiSorted[0] : NULL));
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_post(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    dtGuiRandomScreen(dt_range(0, 8));
    for (int i = 0; i < gGuiElementCount; i++) {
        if (dt_u32() % 2) {
            gGuiSorted[i]->flags |= kGuiFlagFocused;
        }
    }
    gGuiScreen.hook = c->seed % 3 == 0 ? NULL : dtGuiHook;
    DT_ARG_INT(c, 0, c->seed % 4 == 0 ? 0x15 + dt_range(0, 1) : dt_range(0, 0x40));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_default_hook(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    dtGuiRandomScreen(dt_range(0, 8));
    for (int i = 0; i < gGuiElementCount; i++) {
        if (dt_u32() % 4 == 0) {
            gGuiSorted[i]->type = 0;
        }
    }
    DT_ARG_INT(c, 0, dt_range(0x14, 0x17));
    DT_ARG_PTR(c, 1, NULL);
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_focus_by_id(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    dtGuiRandomScreen(dt_range(0, 8));
    DT_ARG_INT(c, 0,
               gGuiElementCount > 0 && c->seed % 4 != 3
                   ? gGuiSorted[dt_u32() % (uint32_t)gGuiElementCount]->id
                   : dt_u32());
}

static void setup_flag_by_id(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    dtGuiRandomScreen(dt_range(0, 8));
    DT_ARG_INT(c, 0,
               gGuiElementCount > 0 && c->seed % 4 != 3
                   ? gGuiSorted[dt_u32() % (uint32_t)gGuiElementCount]->id
                   : dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, c->seed % 2 ? 1 : dt_u32());
}

/* ---- setters with structure ------------------------------------------------------------------- */

static void setup_scale(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    DT_ARG_PTR(c, 0, dtGuiRandomElement(0));
    switch (c->seed % 6) {
    case 0:
        DT_ARG_FLOAT(c, 1, 1.0f);
        DT_ARG_FLOAT(c, 2, 1.0f);
        break;
    case 1:
        DT_ARG_FLOAT(c, 1, 1.0f);
        DT_ARG_FLOAT(c, 2, dt_float_edgy(0.5f, 2.0f));
        break;
    case 2:
        DT_ARG_FLOAT(c, 1, dt_float_edgy(0.5f, 2.0f));
        DT_ARG_FLOAT(c, 2, 1.0f);
        break;
    case 3: /* NaN compares as equal to 1.0 in the original's x87 test */
        DT_ARG_INT(c, 1, 0x7fc00000u);
        DT_ARG_INT(c, 2, c->seed % 12 == 3 ? 0x3f800000u : 0x7fc00000u);
        break;
    default:
        DT_ARG_FLOAT(c, 1, dt_float_edgy(-2.0f, 4.0f));
        DT_ARG_FLOAT(c, 2, dt_float_edgy(-2.0f, 4.0f));
    }
}

static void setup_anim_pairs(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    GuiElement *el = dtGuiRandomElement(0);
    uint8_t *pairs = dt_alloc(80);
    for (int i = 0; i < 80; i++) {
        pairs[i] = (uint8_t)dt_u32();
    }
    DT_ARG_PTR(c, 0, el);
    DT_ARG_INT(c, 1, dt_u32() % 5 == 0 ? (dt_u32() & 0xffff0000u) : (uint32_t)dt_range(0, 30));
    DT_ARG_PTR(c, 2, pairs);
}

static void setup_anim_lists(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    GuiElement *el = dtGuiRandomElement(0);
    uint16_t *frames = dt_alloc(80);
    uint16_t *holds = dt_alloc(80);
    for (int i = 0; i < 40; i++) {
        frames[i] = (uint16_t)dt_u32();
        holds[i] = (uint16_t)dt_u32();
    }
    DT_ARG_PTR(c, 0, el);
    /* upper half is ignored; counts above 30 stay within the 40 entries given */
    DT_ARG_INT(c, 1, (dt_u32() & 0xffff0000u) | (uint32_t)dt_range(0, 30));
    DT_ARG_PTR(c, 2, frames);
    DT_ARG_PTR(c, 3, holds);
}

static void setup_nav(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    DT_ARG_PTR(c, 0, dtGuiRandomElement(0));
    DT_ARG_INT(c, 1, dt_u32() % 3 == 0 ? dt_u32() : (uint32_t)dt_range(0, 3));
    DT_ARG_INT(c, 2, dt_u32());
}

/* ---- text ------------------------------------------------------------------------------------- */

static void setup_set_text(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    GuiElement *el = dtGuiRandomElement(0);
    char *text = dtGuiRandomText(24, 0);
    el->textHandle = 0;
    el->textCapacity = 0;
    char *current = dtGuiRandomText(24, 0);
    switch (c->seed % 8) {
    case 0: /* no text yet */
        break;
    case 1: /* same text again: nothing happens */
        text = current;
        el->textHandle = dtGuiTextHandle(current);
        el->textCapacity = (uint32_t)strlen(current) + 1;
        break;
    case 2: /* shrinks into the existing buffer */
        el->textHandle = dtGuiTextHandle(current);
        el->textCapacity = (uint32_t)strlen(current) + 1 + 30;
        break;
    case 3: /* the existing buffer is too small */
        el->textHandle = dtGuiTextHandle(current);
        el->textCapacity = (uint32_t)strlen(current) + 1;
        break;
    case 4: /* empty string over an existing text */
        text[0] = '\0';
        el->textHandle = dtGuiTextHandle(current);
        el->textCapacity = (uint32_t)strlen(current) + 1;
        break;
    case 6:
    case 7: /* different text of exactly the buffer's size: it still fits */
        el->textHandle = dtGuiTextHandle(current);
        el->textCapacity = (uint32_t)strlen(current) + 1;
        text = dt_alloc((uint32_t)strlen(current) + 1);
        for (size_t i = 0; current[i] != '\0'; i++) {
            text[i] = (char)(current[i] == 'z' ? 'a' : current[i] + 1);
        }
        break;
    default: /* empty string, no text yet */
        text[0] = '\0';
    }
    DT_ARG_PTR(c, 0, c->seed % 17 == 16 ? NULL : el);
    DT_ARG_PTR(c, 1, c->seed % 19 == 18 ? NULL : text);
}

static void setup_text_width(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    GuiElement *el = dtGuiRandomElement(0);
    el->textHandle = dtGuiTextHandle(dtGuiRandomText(12, 0));
    el->font = dtGuiMakeFont((uint16_t)dt_range(0x100, 0x1ff), (uint16_t)dt_range(6, 20));
    DT_ARG_PTR(c, 0, el);
}

/* Makes lookups of unknown asset ids fail quietly: the sprite layer's archive directory is empty. */
#define gZprimArchive ORIG_VAR(0x00678400, void *, gZprimArchive)

static uint32_t missing_id(void)
{
    gZprimArchive = dt_alloc(0x130);
    return 0xfed | (dt_u32() & 0xffff0000u);
}

static void setup_font_by_id(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    GuiElement *el = dtGuiRandomElement(0);
    const uint16_t id = (uint16_t)dt_range(0x100, 0x1ff);
    dtGuiMakeFont(id, (uint16_t)dt_range(6, 20));
    DT_ARG_PTR(c, 0, el);
    DT_ARG_INT(c, 1,
               c->seed % 5 == 4 ? missing_id()
                                : (uint32_t)id | (c->seed % 4 == 0 ? 0 : (dt_u32() & 0xffff0000u)));
}

static void setup_sprite_by_id(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    GuiElement *el = dtGuiRandomElement(0);
    const uint16_t id = (uint16_t)dt_range(0x100, 0x1ff);
    dtGuiMakeSprite(id, dt_range(1, 4));
    DT_ARG_PTR(c, 0, el);
    DT_ARG_INT(c, 1,
               c->seed % 5 == 4 ? missing_id()
                                : (uint32_t)id | (c->seed % 4 == 0 ? 0 : (dt_u32() & 0xffff0000u)));
}

/* ---- backdrop --------------------------------------------------------------------------------- */

/* Stands in for the engine routine that releases a sprite sheet: logs its calls in DLL state. */
typedef void(BMX_CDECL *FreeSheetFn)(void *sprites);
#define gFreeSpriteSheet ORIG_VAR(0x00561024, FreeSheetFn, gFreeSpriteSheet)
static uint32_t freeSheetLog[2];

static void BMX_CDECL free_sheet_stub(void *sprites)
{
    freeSheetLog[0]++;
    freeSheetLog[1] += (uint32_t)(uintptr_t)sprites;
}

static void setup_free_sheet(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    gFreeSpriteSheet = free_sheet_stub;
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_backdrop_by_id(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    const uint16_t id = (uint16_t)dt_range(0x100, 0x1ff);
    dtGuiMakeSprite(id, 2);
    DT_ARG_INT(c, 0, id);
}

static void setup_set_backdrop(DtCtx *c)
{
    setup_backdrop_by_id(c);
    if (c->seed % 4 == 3) {
        DT_ARG_INT(c, 0, missing_id());
    }
}

static void setup_draw_backdrop(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    const uint16_t a = (uint16_t)dt_range(0x100, 0x17f);
    const uint16_t b = (uint16_t)dt_range(0x180, 0x1ff);
    dtGuiMakeSprite(a, 2);
    dtGuiMakeSprite(b, 2);
    DT_ARG_INT(c, 0, c->seed % 5 == 3 ? missing_id() : a);
    DT_ARG_INT(c, 1, c->seed % 5 == 2 ? missing_id() : b);
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

/* ---- draw handlers ---------------------------------------------------------------------------- */

static void setup_line_proc(DtCtx *c)
{
    dt_seed(c);
    dtGuiReset(DT_GUI_HEAP);
    DT_ARG_INT(c, 0, c->seed % 4 == 0 ? dt_range(0, 0x40) : kGuiMsgDraw);
    DT_ARG_PTR(c, 1, dtGuiRandomElement(0));
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

DT_CASES(guielem_cases) = {
    {"guiSend", 0x00439460, guiSend, 4, DT_RET_INT, setup_send, 300, 0},
    {"guiSendById", 0x004394e0, guiSendById, 4, DT_RET_INT, setup_send_by_id, 300, 0},
    {"guiBroadcast", 0x00439520, guiBroadcast, 4, DT_RET_INT, setup_broadcast, 400, 0},
    {"guiPostToVisible", 0x004395e0, guiPostToVisible, 3, DT_RET_INT, setup_post, 400, 0},
    {"guiDefaultHookProc", 0x004396b0, guiDefaultHookProc, 4, DT_RET_INT, setup_default_hook, 300, 0},
    {"guiFocusById", 0x00439780, guiFocusById, 1, DT_RET_VOID, setup_focus_by_id, 300, 0},
    {"guiSetFlags", 0x00438760, guiSetFlags, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiSetNav", 0x00438780, guiSetNav, 3, DT_RET_VOID, setup_nav, 200, 0},
    {"guiSetFlagById", 0x004387b0, guiSetFlagById, 3, DT_RET_VOID, setup_flag_by_id, 300, 0},
    {"guiSetPos", 0x004387f0, guiSetPos, 3, DT_RET_VOID, setup_el2, 200, 0},
    {"guiGetPos", 0x00438820, guiGetPos, 3, DT_RET_VOID, setup_el_out2, 200, 0},
    {"guiSetOrder", 0x00438860, guiSetOrder, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiGetOrder", 0x00438880, guiGetOrder, 2, DT_RET_VOID, setup_el_out1, 200, 0},
    {"guiSetScale", 0x004388b0, guiSetScale, 3, DT_RET_VOID, setup_scale, 300, 0},
    {"guiGetScale", 0x00438900, guiGetScale, 3, DT_RET_VOID, setup_el_out2, 200, 0},
    {"guiSetSize", 0x00438930, guiSetSize, 3, DT_RET_VOID, setup_el2, 200, 0},
    {"guiSetSlideStart", 0x00438960, guiSetSlideStart, 3, DT_RET_VOID, setup_el2, 200, 0},
    {"guiSetAnimFlags", 0x00438990, guiSetAnimFlags, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiSetAnimPairs", 0x004389b0, guiSetAnimPairs, 3, DT_RET_VOID, setup_anim_pairs, 200, 0},
    {"guiSetSpriteOffset", 0x00438a30, guiSetSpriteOffset, 3, DT_RET_VOID, setup_el2, 200, 0},
    {"guiSetSprite", 0x00438a60, guiSetSprite, 2, DT_RET_VOID, setup_sprite_by_id, 200, 0},
    {"guiSetSpritePtr", 0x00438ab0, guiSetSpritePtr, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiSetSpriteFocused", 0x00438ad0, guiSetSpriteFocused, 2, DT_RET_VOID, setup_sprite_by_id, 200, 0},
    {"guiSetFrame", 0x00438b20, guiSetFrame, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiSetAnimIndex", 0x00438b40, guiSetAnimIndex, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiSetAnimCount", 0x00438b60, guiSetAnimCount, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiSetPalette", 0x00438b80, guiSetPalette, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiGetPalette", 0x00438bb0, guiGetPalette, 1, DT_RET_INT, setup_el, 200, 0},
    {"guiSetAlpha", 0x00438bd0, guiSetAlpha, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiSetAnimLists", 0x00438bf0, guiSetAnimLists, 4, DT_RET_VOID, setup_anim_lists, 200, 0},
    {"guiSetTextFlags", 0x00438c70, guiSetTextFlags, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiSetTextRgb", 0x00438ca0, guiSetTextRgb, 4, DT_RET_VOID, setup_el3, 200, 0},
    {"guiSetTextPos", 0x00438ce0, guiSetTextPos, 3, DT_RET_VOID, setup_el2, 200, 0},
    {"guiSetTextStyle", 0x00438d10, guiSetTextStyle, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiGetTextStyle", 0x00438d40, guiGetTextStyle, 1, DT_RET_INT, setup_el, 200, 0},
    {"guiSetFont", 0x00438d60, guiSetFont, 2, DT_RET_VOID, setup_font_by_id, 200, 0},
    {"guiSetText", 0x00438db0, guiSetText, 2, DT_RET_VOID, setup_set_text, 400, 0},
    {"guiSetTextSpacing", 0x00439060, guiSetTextSpacing, 2, DT_RET_VOID, setup_el1, 200, 0},
    {"guiGetTextSpacing", 0x00439090, guiGetTextSpacing, 1, DT_RET_INT16, setup_el, 200, 0},
    {"guiSetTextFixedPitch", 0x004390b0, guiSetTextFixedPitch, 2, DT_RET_VOID, setup_el_flag, 200, 0},
    {"guiGetTextWidth", 0x004390e0, guiGetTextWidth, 1, DT_RET_INT16, setup_text_width, 200, 0},
    {"guiShow", 0x004397c0, guiShow, 1, DT_RET_VOID, setup_el, 200, 0},
    {"guiHide", 0x004397e0, guiHide, 1, DT_RET_VOID, setup_el, 200, 0},
    {"guiIsVisible", 0x00439800, guiIsVisible, 1, DT_RET_INT, setup_el, 200, 0},
    {"guiSetHighlight", 0x00439830, guiSetHighlight, 2, DT_RET_VOID, setup_el_flag, 200, 0},
    {"guiSetTint", 0x00439860, guiSetTint, 5, DT_RET_VOID, setup_el4, 200, 0},
    {"guiResolveText", 0x004398b0, guiResolveText, 1, DT_RET_INT, setup_el1, 200, 0},
    {"guiDrawBackdropSprites", 0x00439130, guiDrawBackdropSprites, 4, DT_RET_VOID, setup_draw_backdrop, 200,
     0},
    {"guiLoadBackdrop", 0x004391c0, guiLoadBackdrop, 2, DT_RET_VOID, setup_backdrop_by_id, 200, 0},
    {"guiFreeBackdropSprites", 0x00439280, guiFreeBackdropSprites, 1, DT_RET_VOID, setup_free_sheet, 200, 0},
    {"guiSetBackdrop", 0x004392a0, guiSetBackdrop, 1, DT_RET_VOID, setup_set_backdrop, 200, 0},
    {"guiSetFocusFont", 0x004392e0, guiSetFocusFont, 1, DT_RET_VOID, setup_screen_state, 200, 0},
    {"guiSetNormalFont", 0x00439300, guiSetNormalFont, 1, DT_RET_VOID, setup_screen_state, 200, 0},
    {"guiSetFocusRgb", 0x00439320, guiSetFocusRgb, 3, DT_RET_VOID, setup_screen_state, 200, 0},
    {"guiSetNormalRgb", 0x00439350, guiSetNormalRgb, 3, DT_RET_VOID, setup_screen_state, 200, 0},
    {"guiGetFocusFont", 0x00439380, guiGetFocusFont, 0, DT_RET_INT16, setup_screen_state, 200, 0},
    {"guiGetNormalFont", 0x004393a0, guiGetNormalFont, 0, DT_RET_INT16, setup_screen_state, 200, 0},
    {"guiGetFocusRgb", 0x004393c0, guiGetFocusRgb, 3, DT_RET_VOID, setup_out3, 200, 0},
    {"guiGetNormalRgb", 0x00439410, guiGetNormalRgb, 3, DT_RET_VOID, setup_out3, 200, 0},
    {"guiLineSetStart", 0x004374e0, guiLineSetStart, 3, DT_RET_VOID, setup_el2, 200, 0},
    {"guiLineSetEnd", 0x00437510, guiLineSetEnd, 3, DT_RET_VOID, setup_el2, 200, 0},
    {"guiLineGetX", 0x00437540, guiLineGetX, 3, DT_RET_VOID, setup_el_out2_nn, 200, 0},
    {"guiLineGetY", 0x00437570, guiLineGetY, 3, DT_RET_VOID, setup_el_out2_nn, 200, 0},
    {"guiBoxSetStart", 0x004399a0, guiBoxSetStart, 3, DT_RET_VOID, setup_el2, 200, 0},
    {"guiBoxSetEnd", 0x004399d0, guiBoxSetEnd, 3, DT_RET_VOID, setup_el2, 200, 0},
    {"guiRectProc", 0x00437460, guiRectProc, 4, DT_RET_INT, setup_line_proc, 200, 0},
    {"guiBoxProc", 0x004398d0, guiBoxProc, 4, DT_RET_INT, setup_line_proc, 200, 0},
};
