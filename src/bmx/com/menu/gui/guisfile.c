/* GUI screen files (C:\zaxis\Bmx\src\com\menu\gui\guisfile.c): builds a screen from its asset. */
#include "bmx/com/menu/gui/guisfile.h"

#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/menu/gui/guitoggl.h"
#include "bmx/com/zge/zprim.h"

/* Scale factors are stored in 1/256 units (float at 0x54d21c). */
static const double kGuiScaleUnit = 1.0 / 256.0;

/* Applies the sprite part of an element record. */
static void guiApplySpritePart(GuiElement *el, const GuiScreenFileElement *rec)
{
    guiSetAnimFlags(el, rec->animFlags);
    if (rec->sprite != 0) {
        guiSetSprite(el, rec->sprite);
    }
    if (rec->spriteFocused != 0) {
        guiSetSpriteFocused(el, rec->spriteFocused);
    }
    guiSetSpriteOffset(el, rec->spriteX, rec->spriteY);
    guiSetFrame(el, rec->frame);
    if ((rec->animFlags & 0x100) != 0) {
        guiSetAnimCount(el, rec->animCount);
        guiSetAnimLists(el, rec->animCount, rec->animFrames, rec->animHold);
    }
    if ((rec->animFlags & 8) != 0) {
        guiSetAlpha(el, rec->alpha);
    }
}

/* Applies the text part of an element record. */
static void guiApplyTextPart(GuiElement *el, const GuiScreenFileElement *rec, const uint8_t *strings)
{
    guiSetTextFlags(el, rec->textFlags);
    guiSetTextSpacing(el, (int16_t)rec->textSpacing);
    if (rec->font != 0) {
        guiSetFont(el, rec->font);
    }
    guiSetTextPos(el, rec->textX, rec->textY);
    if (rec->textOffset != 0) {
        guiSetText(el, (const char *)(strings + rec->textOffset));
    }
    guiSetTextRgb(el, rec->textRgb[0], rec->textRgb[1], rec->textRgb[2]);
}

/* Applies the toggle part of an element record (type 3 only). */
static void guiApplyTogglePart(GuiElement *el, const GuiScreenFileElement *rec, const uint8_t *strings)
{
    guiToggleSetTextFlags(el, rec->toggleFlags);
    guiToggleSetSpacing(el, (int16_t)rec->toggleSpacing);
    if (rec->toggleFont != 0) {
        guiToggleSetFont(el, rec->toggleFont);
    }
    guiToggleSetTextPos(el, rec->toggleX, rec->toggleY);
    if (rec->toggleChoicesOffset != 0) {
        guiToggleSetChoices(el, (const char *)(strings + rec->toggleChoicesOffset));
    }
    guiToggleSetTextRgb(el, rec->toggleRgb[0], rec->toggleRgb[1], rec->toggleRgb[2]);
}

/*
 * Loads asset `id` (a screen definition), creates every element in it and focuses the one flagged
 * 0x10. Returns 1 if the asset is missing, malformed or an element cannot be allocated.
 */
BMX_ADDR(0x00439a00)
int guiBuildScreen(uint32_t id)
{
    const int asset = (int)(intptr_t)zassetFindSprite(id);
    if (asset == 0) {
        (*gGuiDebugPrint)("GUISFILE: Error - failed to load screen %x\n", id & 0xffff);
        return 1;
    }
    const uint8_t *base = zassetLock(asset);
    const GuiScreenFile *file = (const GuiScreenFile *)base;
    if (file->magic != gGuiScreenMagic) {
        (*gGuiDebugPrint)("GUISFILE: Error - bad screen %04.4X\n", id & 0xffff);
        zassetRelease(asset);
        return 1;
    }
    if (file->backdrop != 0) {
        /* The original passes 0x0055 in the upper half (a leftover register); only 16 bits are used. */
        guiSetBackdrop(0x550000u | file->backdrop);
    }
    guiSetFocusFont(file->focusFont);
    guiSetFocusRgb(file->focusRgb[0], file->focusRgb[1], file->focusRgb[2]);
    guiSetNormalFont(file->normalFont);
    guiSetNormalRgb(file->normalRgb[0], file->normalRgb[1], file->normalRgb[2]);

    GuiElement *focused = NULL;
    if (file->elementCount != 0) {
        const uint8_t *strings = base + file->stringsOffset;
        const GuiScreenFileElement *rec = (const GuiScreenFileElement *)(base + file->elementsOffset);
        for (int i = 0; i < (int)file->elementCount; i++, rec++) {
            if (rec->magic != gGuiElementMagic) {
                (*gGuiDebugPrint)("GUISFILE: Error - bad screen element def %04.4X\n", id & 0xffff);
                zassetRelease(asset);
                return 1;
            }
            GuiElement *el = guiCreateElement((int16_t)rec->type, (int16_t)rec->order, rec->id);
            if (el == NULL) {
                (*gGuiDebugPrint)("GUISFILE: Error - failed to allocate element\n");
                zassetRelease(asset);
                return 1;
            }
            if ((rec->flags & 0x10) != 0) {
                focused = el;
            }
            guiSetFlags(el, rec->flags);
            for (uint32_t n = 0; n < 4; n++) {
                guiSetNav(el, n, rec->nav[n]);
            }
            guiSetSlideStart(el, rec->slideStartX, rec->slideStartY);
            guiSetPos(el, rec->x, rec->y);
            if ((rec->flags & 0x20) != 0) {
                guiSetScale(el, (float)((double)rec->scaleX256 * kGuiScaleUnit),
                            (float)((double)rec->scaleY256 * kGuiScaleUnit));
            }
            guiSetSize(el, rec->w, rec->h);
            if ((rec->animFlags & 2) == 0) {
                guiApplySpritePart(el, rec);
            } else {
                guiSetAnimFlags(el, 2);
            }
            if ((rec->textFlags & 2) == 0) {
                guiApplyTextPart(el, rec, strings);
            } else {
                guiSetTextFlags(el, 2);
            }
            if (rec->type == 3 && (rec->toggleFlags & 2) == 0) {
                guiApplyTogglePart(el, rec, strings);
            } else {
                guiToggleSetTextFlags(el, 2);
            }
        }
        if (focused != NULL) {
            guiSend(kGuiMsgFocus, focused, 0, 0);
        }
    }
    zassetRelease(asset);
    return 0;
}
