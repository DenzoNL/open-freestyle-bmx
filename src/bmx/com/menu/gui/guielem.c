/*
 * GUI element messaging, setters/getters and the line/box element types.
 * TODO(file): the original source file of these functions is unknown (they sit between guimain.c's
 * code and guisfile.c); they are part of the GUIMAIN module.
 */
#include "bmx/com/menu/gui/guimain.h"

#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"

/* Function pointer at 0x561024 that releases the sprite sheet built by guiLoadBackdrop. */
typedef void(BMX_CDECL *GuiFreeSpriteSheetFn)(void *sprites);
#define gGuiFreeSpriteSheet ORIG_VAR(0x00561024, GuiFreeSpriteSheetFn, gGuiFreeSpriteSheet)

/* ---- messaging -------------------------------------------------------------------------------- */

/* Sends a message to one element: its own handler if set, else the type handler. */
BMX_ADDR(0x00439460)
int guiSend(int msg, GuiElement *el, int arg3, int arg4)
{
    if (el == NULL) {
        (*gGuiDebugPrint)("GUIMAIN: Error - sending message %d to NULL element\n", msg);
        return 0;
    }
    if ((el->flags & kGuiFlagHidden) == 0) {
        GuiProcFn proc = el->proc;
        if (proc == NULL) {
            proc = el->defaultProc;
        }
        if (proc != NULL) {
            return proc(msg, el, arg3, arg4);
        }
        (*gGuiDebugPrint)("POST MESSAGE: No type handler\n");
    }
    return 0;
}

BMX_ADDR(0x004394e0)
int guiSendById(int msg, uint32_t id, int arg3, int arg4)
{
    GuiElement *el = guiFindById(id);
    if (el != NULL) {
        return guiSend(msg, el, arg3, arg4);
    }
    return 0;
}

/* Offers the message to the screen hook first, then to every element until one handles it. */
BMX_ADDR(0x00439520)
int guiBroadcast(int msg, GuiElement *el, int arg3, int arg4)
{
    int handled;
    if (gGuiScreen.hook == NULL) {
        handled = guiDefaultHookProc(msg, el, arg3, arg4);
    } else {
        handled = gGuiScreen.hook(msg, el, arg3, arg4);
    }
    if (handled != 0) {
        return 1;
    }
    for (int i = 0; i < gGuiElementCount; i++) {
        if (guiSend(msg, gGuiSorted[i], arg3, arg4) != 0) {
            return 1;
        }
    }
    return 0;
}

/* Offers a message to every visible, focused element (broadcast then direct); true if one handled it. */
BMX_ADDR(0x004395e0)
int guiPostToVisible(int msg, int arg2, int arg3)
{
    for (int i = 0; i < gGuiElementCount; i++) {
        GuiElement *el = gGuiSorted[i];
        if ((el->flags & kGuiFlagHidden) == 0 && (el->flags & kGuiFlagFocused) != 0) {
            if (guiBroadcast(msg, el, arg2, arg3) != 0) {
                return 1;
            }
            if (guiSend(msg, el, arg2, arg3) != 0) {
                return 1;
            }
        }
    }
    guiBroadcast(msg, NULL, arg2, arg3);
    return 0;
}

/* Hook used when a screen installs none: turns the draw-all/tick-all broadcasts into per-element messages. */
BMX_ADDR(0x004396b0)
int BMX_CDECL guiDefaultHookProc(int msg, GuiElement *el, int arg3, int arg4)
{
    if (msg == kGuiMsgDrawAll) {
        for (int i = 0; i < gGuiElementCount; i++) {
            if (gGuiSorted[i]->type != 0) {
                guiSend(kGuiMsgDraw, gGuiSorted[i], arg3, arg4);
            }
        }
        return 1;
    }
    if (msg != kGuiMsgTickAll) {
        return 0;
    }
    for (int i = 0; i < gGuiElementCount; i++) {
        if (gGuiSorted[i]->type != 0) {
            guiSend(kGuiMsgTick, gGuiSorted[i], arg3, arg4);
        }
    }
    return 1;
}

BMX_ADDR(0x00439780)
void guiFocusById(uint32_t id)
{
    GuiElement *el = guiFindById(id);
    if (el != NULL) {
        guiSend(kGuiMsgFocus, el, 0, 0);
    }
}

/* ---- element setters and getters -------------------------------------------------------------- */

BMX_ADDR(0x00438760)
void guiSetFlags(GuiElement *el, uint16_t flags)
{
    el->flags = flags;
}

/* No bounds check: the index is any byte, so it can reach past nav[]. */
BMX_ADDR(0x00438780)
void guiSetNav(GuiElement *el, uint32_t index, uint8_t value)
{
    el->nav[index & 0xff] = value;
}

BMX_ADDR(0x004387b0)
void guiSetFlagById(uint32_t id, uint16_t mask, int16_t on)
{
    GuiElement *el = guiFindById(id);
    if (el != NULL) {
        if (on == 1) {
            el->flags |= mask;
            return;
        }
        el->flags &= (uint16_t)~mask;
    }
}

BMX_ADDR(0x004387f0)
void guiSetPos(GuiElement *el, int16_t x, int16_t y)
{
    el->x = x;
    el->y = y;
}

BMX_ADDR(0x00438820)
void guiGetPos(const GuiElement *el, int16_t *x, int16_t *y)
{
    if (x != NULL) {
        *x = el->x;
    }
    if (y != NULL) {
        *y = el->y;
    }
}

BMX_ADDR(0x00438860)
void guiSetOrder(GuiElement *el, uint16_t order)
{
    el->order = order;
}

BMX_ADDR(0x00438880)
void guiGetOrder(const GuiElement *el, uint16_t *order)
{
    if (order != NULL) {
        *order = el->order;
    }
}

/*
 * Sets the scale and the scaled flag. The flag is cleared only when both factors compare equal to 1.0
 * (a NaN counts as equal: the x87 compare reports it as unordered, which the original treats as equal).
 */
BMX_ADDR(0x004388b0)
void guiSetScale(GuiElement *el, float sx, float sy)
{
    el->scaleX = sx;
    el->scaleY = sy;
    if (!(sx < 1.0f || sx > 1.0f) && !(sy < 1.0f || sy > 1.0f)) {
        el->flags &= (uint16_t)~kGuiFlagScaled;
        return;
    }
    el->flags |= kGuiFlagScaled;
}

BMX_ADDR(0x00438900)
void guiGetScale(const GuiElement *el, float *sx, float *sy)
{
    if (sx != NULL) {
        *sx = el->scaleX;
    }
    if (sy != NULL) {
        *sy = el->scaleY;
    }
}

BMX_ADDR(0x00438930)
void guiSetSize(GuiElement *el, int16_t w, int16_t h)
{
    el->w = w;
    el->h = h;
}

BMX_ADDR(0x00438960)
void guiSetSlideStart(GuiElement *el, int16_t x, int16_t y)
{
    el->slideStartX = x;
    el->slideStartY = y;
}

BMX_ADDR(0x00438990)
void guiSetAnimFlags(GuiElement *el, uint16_t flags)
{
    el->animFlags = flags;
}

/* Loads `count` animation steps from {frame, hold} byte pairs and restarts at step 0. No bounds check. */
BMX_ADDR(0x004389b0)
void guiSetAnimPairs(GuiElement *el, uint16_t count, const uint8_t *pairs)
{
    for (uint32_t i = 0; i < count; i++) {
        el->animFrames[i] = pairs[2 * i];
        el->animHold[i] = pairs[2 * i + 1];
    }
    el->animCount = count;
    el->animIndex = 0;
}

BMX_ADDR(0x00438a30)
void guiSetSpriteOffset(GuiElement *el, int16_t x, int16_t y)
{
    el->spriteX = x;
    el->spriteY = y;
}

BMX_ADDR(0x00438a60)
void guiSetSprite(GuiElement *el, uint32_t spriteId)
{
    void *sprite = zprimLoadAsset(spriteId);
    if (sprite == NULL) {
        (*gGuiDebugPrint)("GUIMAIN: Error - failed to load sprite %04.4X\n", spriteId & 0xffff);
        return;
    }
    el->sprite = sprite;
}

BMX_ADDR(0x00438ab0)
void guiSetSpritePtr(GuiElement *el, void *sprite)
{
    el->sprite = sprite;
}

BMX_ADDR(0x00438ad0)
void guiSetSpriteFocused(GuiElement *el, uint32_t spriteId)
{
    void *sprite = zprimLoadAsset(spriteId);
    if (sprite == NULL) {
        (*gGuiDebugPrint)("GUIMAIN: Error - failed to load selected sprite %04.4X\n", spriteId & 0xffff);
        return;
    }
    el->spriteFocused = sprite;
}

BMX_ADDR(0x00438b20)
void guiSetFrame(GuiElement *el, uint16_t frame)
{
    el->frame = frame;
}

BMX_ADDR(0x00438b40)
void guiSetAnimIndex(GuiElement *el, int16_t index)
{
    el->animIndex = index;
}

BMX_ADDR(0x00438b60)
void guiSetAnimCount(GuiElement *el, uint16_t count)
{
    el->animCount = count;
}

BMX_ADDR(0x00438b80)
void guiSetPalette(GuiElement *el, uint32_t palette)
{
    el->palette = palette & 0xffff;
}

BMX_ADDR(0x00438bb0)
uint32_t guiGetPalette(const GuiElement *el)
{
    return el->palette;
}

BMX_ADDR(0x00438bd0)
void guiSetAlpha(GuiElement *el, uint16_t alpha)
{
    el->alpha = alpha;
}

/* Loads animation steps from separate frame and hold lists (the low 16 bits of count) and restarts. */
BMX_ADDR(0x00438bf0)
void guiSetAnimLists(GuiElement *el, uint32_t count, const uint16_t *frames, const uint16_t *holds)
{
    count &= 0xffff;
    for (uint32_t i = 0; i < count; i++) {
        el->animFrames[i] = frames[i];
        el->animHold[i] = holds[i];
    }
    el->animIndex = 0;
}

BMX_ADDR(0x00438c70)
void guiSetTextFlags(GuiElement *el, uint16_t flags)
{
    el->textFlags = flags;
}

BMX_ADDR(0x00438ca0)
void guiSetTextRgb(GuiElement *el, uint8_t r, uint8_t g, uint8_t b)
{
    el->textRgb[0] = r;
    el->textRgb[1] = g;
    el->textRgb[2] = b;
}

BMX_ADDR(0x00438ce0)
void guiSetTextPos(GuiElement *el, int16_t x, int16_t y)
{
    el->textX = x;
    el->textY = y;
}

BMX_ADDR(0x00438d10)
void guiSetTextStyle(GuiElement *el, uint32_t style)
{
    el->textStyle = style & 0xffff;
}

BMX_ADDR(0x00438d40)
uint32_t guiGetTextStyle(const GuiElement *el)
{
    return el->textStyle;
}

BMX_ADDR(0x00438d60)
void guiSetFont(GuiElement *el, uint32_t fontId)
{
    void *font = zprimLoadAsset(fontId);
    if (font == NULL) {
        (*gGuiDebugPrint)("GUIMAIN: Error - failed to load font %04.4X\n", fontId & 0xffff);
        return;
    }
    el->font = font;
}

/*
 * Replaces the element's text. Does nothing if it already holds the same string; the empty string
 * only blanks the buffer. Otherwise the buffer is reallocated when too small and the element gets a
 * kGuiMsgTextChanged message.
 */
BMX_ADDR(0x00438db0)
void guiSetText(GuiElement *el, const char *str)
{
    if (el == NULL || str == NULL) {
        return;
    }
    const char *text = guiResolveText(str);
    if (text == NULL) {
        return;
    }
    if (el->textHandle != 0) {
        const char *old = zmemdmmLock(el->textHandle);
        const int same = strcmp(text, old) == 0;
        zmemdmmUnlock(el->textHandle);
        if (same) {
            return;
        }
    }
    const uint32_t size = (uint32_t)strlen(text) + 1;
    if (size == 1 && el->textHandle != 0) {
        char *blank = zmemdmmLock(el->textHandle);
        *blank = (char)gGuiEmptyChar;
        zmemdmmUnlock(el->textHandle);
        el->textCapacity = 1;
        return;
    }
    char *dst;
    if (el->textHandle != 0) {
        dst = zmemdmmLock(el->textHandle);
        if (size > el->textCapacity) {
            zmemdmmUnlock(el->textHandle);
            zmemdmmFree(el->textHandle);
            el->textHandle = zmemdmmAlloc(size);
            el->textCapacity = size;
            dst = zmemdmmLock(el->textHandle);
        }
    } else {
        el->textHandle = zmemdmmAlloc(size);
        el->textCapacity = size;
        dst = zmemdmmLock(el->textHandle);
    }
    memcpy(dst, text, size);
    zmemdmmUnlock(el->textHandle);
    guiSend(kGuiMsgTextChanged, el, (int)(intptr_t)text, 0);
}

BMX_ADDR(0x00439060)
void guiSetTextSpacing(GuiElement *el, int16_t spacing)
{
    el->textSpacing = spacing;
}

BMX_ADDR(0x00439090)
int16_t guiGetTextSpacing(const GuiElement *el)
{
    return el->textSpacing;
}

BMX_ADDR(0x004390b0)
void guiSetTextFixedPitch(GuiElement *el, int on)
{
    if (on != 0) {
        el->textFlags |= kGuiTextFixedPitch;
        return;
    }
    el->textFlags &= (uint16_t)~kGuiTextFixedPitch;
}

BMX_ADDR(0x004390e0)
uint16_t guiGetTextWidth(const GuiElement *el)
{
    const char *text = zmemdmmLock(el->textHandle);
    const uint16_t width = zprimTextWidth(el->font, (const uint8_t *)text);
    zmemdmmUnlock(el->textHandle);
    return width;
}

BMX_ADDR(0x004397e0)
void guiHide(GuiElement *el)
{
    el->flags |= kGuiFlagHidden;
}

BMX_ADDR(0x004397c0)
void guiShow(GuiElement *el)
{
    el->flags &= (uint16_t)~kGuiFlagHidden;
}

BMX_ADDR(0x00439800)
int guiIsVisible(const GuiElement *el)
{
    return (~el->flags & kGuiFlagHidden) & 0xff;
}

BMX_ADDR(0x00439830)
void guiSetHighlight(GuiElement *el, int on)
{
    if (on != 0) {
        el->flags |= kGuiFlagHighlight;
        return;
    }
    el->flags &= (uint16_t)~kGuiFlagHighlight;
}

/* Sets (or with alpha 0 clears) the tint: colour is 0x00BBGGRR, alpha is kept as given. */
BMX_ADDR(0x00439860)
void guiSetTint(GuiElement *el, uint8_t r, uint8_t g, uint8_t b, uint32_t alpha)
{
    if (alpha == 0) {
        el->flags &= (uint16_t)~kGuiFlagTinted;
        return;
    }
    el->tintAlpha = alpha;
    el->flags |= kGuiFlagTinted;
    el->tintRgb = (((uint32_t)b << 8 | g) << 8) | r;
}

/* Text ids were once resolved through a string table; the original returns its argument unchanged. */
BMX_ADDR(0x004398b0)
const char *guiResolveText(const char *text)
{
    return text;
}

/* ---- backdrop, fonts and colours -------------------------------------------------------------- */

BMX_ADDR(0x00439130)
void guiDrawBackdropSprites(uint32_t spriteId, uint32_t backdropId, int arg3, int arg4)
{
    void *sprite = zprimLoadAsset(spriteId);
    if (sprite == NULL) {
        (*gGuiDebugPrint)("GUIMAIN: Error - failed to load sprite %04.4X\n", spriteId & 0xffff);
        return;
    }
    void *backdrop = zprimLoadAsset(backdropId);
    if (backdrop == NULL) {
        (*gGuiDebugPrint)("GUIMAIN: Error - failed to load backdrop %04.4X\n", backdropId & 0xffff);
        return;
    }
    zprimNop2(sprite, backdrop, arg3, arg4);
}

/* Loads screen asset `id` as the backdrop and builds its sprite sheet, replacing the previous one. */
BMX_ADDR(0x004391c0)
void guiLoadBackdrop(uint16_t id, int arg)
{
    ZPrimAsset *const handle = zprimLoadGroupPicture((uint16_t)((id & 0xfff) | 0xe000), arg);
    if (gGuiScreen.backdrop != NULL) {
        zprimAssetFree(gGuiScreen.backdrop);
    }
    gGuiScreen.backdrop = handle;
    /* A backdrop is a one frame picture; frame 0's texture handle (+0x20) holds the sprite sheet. */
    ZPrimSet *pic = zprimAssetData(handle);
    uint8_t *asset = (uint8_t *)pic;
    if (pic->frames[0].texture != 0) {
        guiFreeBackdropSprites((void *)(uintptr_t)pic->frames[0].texture);
    }
    pic->frames[0].texture = zprimCreateTexture(pic->width, pic->height, pic->bpp, asset + pic->pixelOffset,
                                                (const uint16_t *)(asset + pic->paletteOffset));
    zprimAssetRelease(handle);
    zmemdmmOptimize();
}

BMX_ADDR(0x00439280)
void guiFreeBackdropSprites(void *sprites)
{
    (*gGuiFreeSpriteSheet)(sprites);
}

BMX_ADDR(0x004392a0)
void guiSetBackdrop(uint32_t spriteId)
{
    void *backdrop = zprimLoadAsset(spriteId);
    if (backdrop == NULL) {
        (*gGuiDebugPrint)("GUIMAIN: Error - failed to load backdrop %04.4X\n", spriteId & 0xffff);
        return;
    }
    gGuiScreen.backdrop = backdrop;
}

BMX_ADDR(0x004392e0)
void guiSetFocusFont(uint16_t font)
{
    gGuiScreen.focusFont = font;
}

BMX_ADDR(0x00439300)
void guiSetNormalFont(uint16_t font)
{
    gGuiScreen.normalFont = font;
}

BMX_ADDR(0x00439320)
void guiSetFocusRgb(uint8_t r, uint8_t g, uint8_t b)
{
    gGuiScreen.focusRgb[0] = r;
    gGuiScreen.focusRgb[1] = g;
    gGuiScreen.focusRgb[2] = b;
}

BMX_ADDR(0x00439350)
void guiSetNormalRgb(uint8_t r, uint8_t g, uint8_t b)
{
    gGuiScreen.normalRgb[0] = r;
    gGuiScreen.normalRgb[1] = g;
    gGuiScreen.normalRgb[2] = b;
}

BMX_ADDR(0x00439380)
uint16_t guiGetFocusFont(void)
{
    return gGuiScreen.focusFont;
}

BMX_ADDR(0x004393a0)
uint16_t guiGetNormalFont(void)
{
    return gGuiScreen.normalFont;
}

BMX_ADDR(0x004393c0)
void guiGetFocusRgb(uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (r != NULL) {
        *r = gGuiScreen.focusRgb[0];
    }
    if (g != NULL) {
        *g = gGuiScreen.focusRgb[1];
    }
    if (b != NULL) {
        *b = gGuiScreen.focusRgb[2];
    }
}

BMX_ADDR(0x00439410)
void guiGetNormalRgb(uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (r != NULL) {
        *r = gGuiScreen.normalRgb[0];
    }
    if (g != NULL) {
        *g = gGuiScreen.normalRgb[1];
    }
    if (b != NULL) {
        *b = gGuiScreen.normalRgb[2];
    }
}

/* ---- line (type 7) and box (type 8) elements -------------------------------------------------- */

BMX_ADDR(0x004374e0)
void guiLineSetStart(GuiElement *el, int16_t x, int16_t y)
{
    el->x = x;
    el->y = y;
}

BMX_ADDR(0x00437510)
void guiLineSetEnd(GuiElement *el, int16_t x, int16_t y)
{
    el->spriteX = x;
    el->spriteY = y;
}

BMX_ADDR(0x00437540)
void guiLineGetX(const GuiElement *el, int16_t *x0, int16_t *x1)
{
    *x0 = el->x;
    *x1 = el->spriteX;
}

BMX_ADDR(0x00437570)
void guiLineGetY(const GuiElement *el, int16_t *y0, int16_t *y1)
{
    *y0 = el->y;
    *y1 = el->spriteY;
}

BMX_ADDR(0x004399a0)
void guiBoxSetStart(GuiElement *el, int16_t x, int16_t y)
{
    el->x = x;
    el->y = y;
}

BMX_ADDR(0x004399d0)
void guiBoxSetEnd(GuiElement *el, int16_t x, int16_t y)
{
    el->spriteX = x;
    el->spriteY = y;
}

/* Line element: draws a line from (x, y) to (spriteX, spriteY) in the text colour. */
BMX_ADDR(0x00437460)
int BMX_CDECL guiRectProc(int msg, GuiElement *el, int arg3, int arg4)
{
    if (msg != kGuiMsgDraw) {
        return 0;
    }
    const int16_t x0 = el->x;
    const int16_t x1 = el->spriteX;
    const int16_t y1 = el->spriteY;
    const int16_t y0 = el->y;
    zprimSetColor(el->textRgb[0], el->textRgb[1], el->textRgb[2], 5);
    zprimSet(6, 1);
    zprimDrawLine(x0, y0, x1, y1);
    return 1;
}

/* Box element: outline between the two corners, drawn twice (the second one inset by a pixel). */
BMX_ADDR(0x004398d0)
int BMX_CDECL guiBoxProc(int msg, GuiElement *el, int arg3, int arg4)
{
    if (msg != kGuiMsgDraw) {
        return 0;
    }
    int16_t x0 = el->x;
    int16_t x1 = el->spriteX;
    int16_t y1 = el->spriteY;
    int16_t y0 = el->y;
    zprimSetColor(el->textRgb[0], el->textRgb[1], el->textRgb[2], 5);
    zprimDrawLine(x0, y0, x1, y0);
    zprimDrawLine(x1, y0, x1, y1);
    zprimDrawLine(x1, y1, x0, y1);
    zprimDrawLine(x0, y1, x0, y0);
    y0++;
    x1--;
    x0++;
    zprimDrawLine(x0, y0, x1, y0);
    y1--;
    zprimDrawLine(x1, y0, x1, y1);
    zprimDrawLine(x1, y1, x0, y1);
    zprimDrawLine(x0, y1, x0, y0);
    return 1;
}
