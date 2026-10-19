/* GUI toggle (multiple choice) widget (C:\zaxis\Bmx\src\com\menu\gui\guitoggl.c). */
#include "bmx/com/menu/gui/guitoggl.h"

#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/menu/gui/guilabel.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"
#include "zge/zdebug.h"

static const char guitogglFile[] = "C:\\zaxis\\Bmx\\src\\com\\menu\\gui\\guitoggl.c";

static GuiToggleData *guiToggleLock(const GuiElement *el, int line)
{
    GuiToggleData *data = zmemdmmLock(el->dataHandle);
    if (data == NULL) {
        zdebugFail("GUITOGGL: Failed to lock toggle data\n", guitogglFile, line, 0);
    }
    return data;
}

static char *guiToggleLockText(int handle, int line)
{
    char *text = zmemdmmLock(handle);
    if (text == NULL) {
        zdebugFail("GUITOGGL: Failed to lock toggle text\n", guitogglFile, line, 0);
    }
    return text;
}

/* ---- setters and getters (all do nothing without a data block) -------------------------------- */

BMX_ADDR(0x0043b560)
void guiToggleSetTextFlags(GuiElement *el, uint16_t flags)
{
    if (el->dataHandle != 0) {
        GuiToggleData *data = guiToggleLock(el, 0x20b);
        data->textFlags = flags;
        zmemdmmUnlock(el->dataHandle);
    }
}

BMX_ADDR(0x0043b5e0)
void guiToggleSetTextPos(GuiElement *el, int16_t x, int16_t y)
{
    if (el->dataHandle != 0) {
        GuiToggleData *data = guiToggleLock(el, 0x21c);
        data->textX = x;
        data->textY = y;
        zmemdmmUnlock(el->dataHandle);
    }
}

BMX_ADDR(0x0043b660)
void guiToggleSetTextStyle(GuiElement *el, uint32_t style)
{
    if (el->dataHandle != 0) {
        GuiToggleData *data = guiToggleLock(el, 0x22e);
        data->textStyle = style & 0xffff;
        zmemdmmUnlock(el->dataHandle);
    }
}

BMX_ADDR(0x0043b6e0)
void guiToggleSetFont(GuiElement *el, uint32_t fontId)
{
    void *font = zprimLoadAsset(fontId);
    if (font == NULL) {
        (*gGuiDebugPrint)("GUITOGGL: Error - failed to load font %04.4X\n", fontId & 0xffff);
        return;
    }
    if (el->dataHandle != 0) {
        GuiToggleData *data = guiToggleLock(el, 0x245);
        data->font = font;
        zmemdmmUnlock(el->dataHandle);
    }
}

BMX_ADDR(0x0043b780)
void guiToggleSetTextRgb(GuiElement *el, uint8_t r, uint8_t g, uint8_t b)
{
    if (el->dataHandle != 0) {
        GuiToggleData *data = guiToggleLock(el, 0x256);
        data->textRgb[0] = r;
        data->textRgb[1] = g;
        data->textRgb[2] = b;
        zmemdmmUnlock(el->dataHandle);
    }
}

BMX_ADDR(0x0043b800)
void guiToggleSetSpacing(GuiElement *el, int16_t spacing)
{
    if (el->dataHandle != 0) {
        GuiToggleData *data = guiToggleLock(el, 0x269);
        data->textSpacing = spacing;
        zmemdmmUnlock(el->dataHandle);
    }
}

BMX_ADDR(0x0043bd10)
uint16_t guiToggleGetIndex(const GuiElement *el)
{
    uint16_t index = 0;
    if (el->dataHandle != 0) {
        GuiToggleData *data = guiToggleLock(el, 0x313);
        index = data->index;
        zmemdmmUnlock(el->dataHandle);
    }
    return index;
}

BMX_ADDR(0x0043bd90)
void guiToggleSetCount(GuiElement *el, uint16_t count)
{
    if (el->dataHandle != 0) {
        GuiToggleData *data = guiToggleLock(el, 0x326);
        data->count = count;
        zmemdmmUnlock(el->dataHandle);
    }
}

BMX_ADDR(0x0043be10)
uint16_t guiToggleGetCount(const GuiElement *el)
{
    uint16_t count = 0;
    if (el->dataHandle != 0) {
        GuiToggleData *data = guiToggleLock(el, 0x338);
        count = data->count;
        zmemdmmUnlock(el->dataHandle);
    }
    return count;
}

/*
 * Stores a new "a|b|c" choices string (nothing happens if it is unchanged), re-selects the current
 * choice from it and tells the element its text changed (also when there is no data block).
 */
BMX_ADDR(0x0043b880)
void guiToggleSetChoices(GuiElement *el, const char *choices)
{
    const char *text = choices;
    if (el->dataHandle != 0) {
        GuiToggleData *data = guiToggleLock(el, 0x27d);
        text = guiResolveText(choices);
        if (data->choicesHandle != 0) {
            const char *old = zmemdmmLock(data->choicesHandle);
            const int same = strcmp(text, old) == 0;
            zmemdmmUnlock(data->choicesHandle);
            if (same) {
                zmemdmmUnlock(el->dataHandle);
                return;
            }
        }
        if (data->choicesHandle != 0) {
            zmemdmmFree(data->choicesHandle);
            data->choicesHandle = 0;
        }
        const uint32_t size = (uint32_t)strlen(text) + 1;
        data->choicesHandle = zmemdmmAlloc(size);
        if (data->choicesHandle == 0) {
            zdebugFail("GUITOGGL: Error - failed to malloc text string\n", guitogglFile, 0x29f, 0);
        }
        char *dst = guiToggleLockText(data->choicesHandle, 0x2a6);
        memcpy(dst, text, size);
        zmemdmmUnlock(data->choicesHandle);
        const uint16_t index = data->index;
        zmemdmmUnlock(el->dataHandle);
        guiToggleSetIndex(el, index);
    }
    guiSend(kGuiMsgTextChanged, el, (int)(intptr_t)text, 0);
}

/*
 * Selects choice `index`: copies the index-th '|' separated entry of the choices into the toggle's text
 * (an index past the last entry gives an empty text) and sends kGuiMsgTextChanged with it. If the text
 * is unchanged the message is not sent. Without a data block (or element) only the message goes out,
 * with an uninitialised text buffer.
 */
BMX_ADDR(0x0043ba90)
void guiToggleSetIndex(GuiElement *el, uint16_t index)
{
    char buf[200];
    if (el != NULL && el->dataHandle != 0) {
        GuiToggleData *data = guiToggleLock(el, 0x2c6);
        data->index = index;
        char *out = buf;
        if (data->choicesHandle != 0) {
            const char *p = zmemdmmLock(data->choicesHandle);
            int seen = 0;
            while (seen < (int)index) {
                const char c = *p;
                if (c == '|' || c == '\0') {
                    seen++;
                }
                if (c != '\0') {
                    p++;
                }
            }
            while (*p != '|' && *p != '\0') {
                *out++ = *p++;
            }
            zmemdmmUnlock(data->choicesHandle);
        }
        *out = '\0';
        if (data->textHandle != 0) {
            const char *old = zmemdmmLock(data->textHandle);
            const int same = strcmp(buf, old) == 0;
            zmemdmmUnlock(data->textHandle);
            if (same) {
                zmemdmmUnlock(el->dataHandle);
                return;
            }
        }
        if (data->textHandle != 0) {
            zmemdmmFree(data->textHandle);
            data->textHandle = 0;
        }
        const uint32_t size = (uint32_t)strlen(buf) + 1;
        data->textHandle = zmemdmmAlloc(size);
        if (data->textHandle == 0) {
            zdebugFail("GUITOGGL: Error - failed to malloc text string\n", guitogglFile, 0x2f9, 0);
        }
        char *dst = guiToggleLockText(data->textHandle, 0x2fe);
        memcpy(dst, buf, size);
        zmemdmmUnlock(data->textHandle);
        zmemdmmUnlock(el->dataHandle);
    }
    guiSend(kGuiMsgTextChanged, el, (int)(intptr_t)buf, 0);
}

/* ---- element handler -------------------------------------------------------------------------- */

typedef struct GuiToggleVariant {
    int dataLine;   /* assert line for locking the data block while drawing */
    int textLine;   /* assert line for locking the text */
    int styleFirst; /* the two variants set text style/spacing in opposite order */
} GuiToggleVariant;

/*
 * Draws the toggle: the label part first, then the current choice's text in the toggle's own font and
 * colour, positioned inside the element. As in the label, the alignment uses the width of the whole
 * text. The scale set for the label is not reset afterwards.
 */
static void guiToggleDraw(GuiElement *el, int arg3, int arg4, const GuiToggleVariant *variant)
{
    guiLabelProc(kGuiMsgDraw, el, (el->flags & kGuiFlagFocused) != 0 ? 1 : arg3, arg4);
    int x = (int16_t)(el->slideX + el->x);
    int y = (int16_t)(el->slideY + el->y);
    int16_t w = el->w;
    int16_t h = el->h;
    if (arg3 != 0 && el->spriteFocused != NULL) {
        w = zprimSpriteFrameWidth(el->spriteFocused, el->frame);
        h = zprimSpriteFrameHeight(el->spriteFocused, el->frame);
    } else if (el->sprite != NULL) {
        w = zprimSpriteFrameWidth(el->sprite, el->frame);
        h = zprimSpriteFrameHeight(el->sprite, el->frame);
    }
    const uint16_t flags = el->flags;
    if ((flags & kGuiFlagScaled) != 0) {
        if ((flags & kGuiFlagWide) != 0) {
            w = (int16_t)(w * 160 / 100);
        } else {
            w = (int16_t)(int)((double)w * el->scaleX);
            h = (int16_t)(int)((double)h * el->scaleY);
        }
        zprimSetScale(el->scaleX, el->scaleY);
    } else if ((flags & kGuiFlagWide) != 0) {
        w = (int16_t)(w * 160 / 100);
        zprimSetScale(1.0f, 1.0f);
    }
    if ((el->flags & kGuiFlagCenterX) != 0) {
        x -= w / 2;
    }
    if ((el->flags & kGuiFlagCenterY) != 0) {
        y -= h / 2;
    }
    if (el->dataHandle == 0) {
        return;
    }
    GuiToggleData *data = guiToggleLock(el, variant->dataLine);
    if ((data->textFlags & kGuiTextDisabled) == 0 && data->textHandle != 0) {
        char *const text = guiToggleLockText(data->textHandle, variant->textLine);
        char *line = text;
        int lineY = y;
        while (strlen(line) != 0) {
            char *newline = strchr(line, '\n');
            if (newline != NULL) {
                *newline = '\0';
            }
            zprimSet(0x13, (data->textFlags & kGuiTextBit4) != 0);
            if ((data->textFlags & kGuiTextColored) != 0) {
                zprimSet(0x14, 1);
                zprimSetColor(data->textRgb[0], data->textRgb[1], data->textRgb[2], 0x16);
            } else {
                zprimSet(0x14, 0);
            }
            if (variant->styleFirst) {
                zprimSet(0x17, (int)data->textStyle);
                zprimSet(0x18, data->textSpacing);
            } else {
                zprimSet(0x18, data->textSpacing);
                zprimSet(0x17, (int)data->textStyle);
            }
            int tx = data->textX;
            int ty = data->textY;
            if ((data->textFlags & kGuiTextCenterX) != 0) {
                tx += (w - (int)zprimTextWidth(data->font, (const uint8_t *)text)) / 2;
            }
            if ((data->textFlags & kGuiTextCenterY) != 0) {
                ty += (h - (int)zprimFontFlags(data->font)) / 2;
            }
            if ((data->textFlags & kGuiTextRight) != 0) {
                tx += w - (int)zprimTextWidth(data->font, (const uint8_t *)text);
            }
            zprimSetFont(data->font);
            zprimDrawText((const uint8_t *)line, tx + x, ty + lineY);
            zprimSet(0x18, 0);
            zprimSet(0x17, 0);
            lineY = lineY + (int)zprimFontFlags(data->font) + 1;
            if (newline != NULL) {
                *newline = '\n';
                line = newline + 1;
            } else {
                line = strchr(line, '\0');
            }
        }
        zmemdmmUnlock(data->textHandle);
    }
    zmemdmmUnlock(el->dataHandle);
}

/* Selects the previous choice, wrapping to the last one. */
static void guiTogglePrev(GuiElement *el)
{
    const uint16_t index = guiToggleGetIndex(el);
    if (index == 0) {
        guiToggleSetIndex(el, (uint16_t)(guiToggleGetCount(el) - 1));
        return;
    }
    guiToggleSetIndex(el, (uint16_t)(index - 1));
}

/* Selects the next choice, wrapping to the first one. */
static void guiToggleNext(GuiElement *el)
{
    const uint16_t index = guiToggleGetIndex(el);
    const int last = (int)guiToggleGetCount(el) - 1;
    if ((int)index == last) {
        guiToggleSetIndex(el, 0);
        return;
    }
    guiToggleSetIndex(el, (uint16_t)(index + 1));
}

/*
 * Toggle handler where left/right change the choice and up/down move the focus (like a button). The
 * original is one function: the dispatch at 0x43a840 with the cases split out at 0x43a887 (draw),
 * 0x43ac43 (focus), 0x43ac66 (blur), 0x43ac89 (focus movement), 0x43aca9 (previous), 0x43acee (next)
 * and the jump table at 0x43ad3c.
 */
BMX_ADDR(0x0043a840)
int BMX_CDECL guiToggleProc(int msg, GuiElement *el, int arg3, int arg4)
{
    static const GuiToggleVariant variant = {0x85, 0x8a, 1};
    switch (msg) {
    case kGuiMsgTick:
        guiLabelProc(msg, el, arg3, arg4);
        return 1;
    case kGuiMsgDraw:
        guiToggleDraw(el, arg3, arg4, &variant);
        return 1;
    case kGuiMsgFocus:
    case kGuiMsgBlur:
        guiButtonProc(msg, el, arg3, arg4);
        return 1;
    case kGuiMsgNavUp:
    case kGuiMsgNavDown:
    case kGuiMsgNavUpAlt:
    case kGuiMsgNavDownAlt:
        guiButtonProc(msg, el, arg3, arg4);
        return 0;
    case kGuiMsgNavLeft:
    case kGuiMsgNavLeftAlt:
        guiTogglePrev(el);
        return 0;
    case kGuiMsgNavRight:
    case kGuiMsgNavRightAlt:
        guiToggleNext(el);
        return 0;
    default:
        return 0;
    }
}

/*
 * The same widget with the directions swapped: up/down change the choice, left/right move the focus.
 * Not referenced by guiCreateElement. Cases at 0x43af17 (draw), 0x43b2d3, 0x43b2f6 (focus, blur),
 * 0x43b319 (focus movement), 0x43b339 (previous), 0x43b37e (next); jump table at 0x43b3cc, padding at
 * 0x43b4e8.
 */
BMX_ADDR(0x0043aed0)
int BMX_CDECL guiToggleProcHoriz(int msg, GuiElement *el, int arg3, int arg4)
{
    static const GuiToggleVariant variant = {0x170, 0x175, 0};
    switch (msg) {
    case kGuiMsgTick:
        guiLabelProc(msg, el, arg3, arg4);
        return 1;
    case kGuiMsgDraw:
        guiToggleDraw(el, arg3, arg4, &variant);
        return 1;
    case kGuiMsgFocus:
    case kGuiMsgBlur:
        guiButtonProc(msg, el, arg3, arg4);
        return 1;
    case kGuiMsgNavUp:
    case kGuiMsgNavUpAlt:
        guiTogglePrev(el);
        return 0;
    case kGuiMsgNavDown:
    case kGuiMsgNavDownAlt:
        guiToggleNext(el);
        return 0;
    case kGuiMsgNavLeft:
    case kGuiMsgNavRight:
    case kGuiMsgNavLeftAlt:
    case kGuiMsgNavRightAlt:
        guiButtonProc(msg, el, arg3, arg4);
        return 0;
    default:
        return 0;
    }
}
