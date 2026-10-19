/* GUI toggle (multiple choice) widget (C:\zaxis\Bmx\src\com\menu\gui\guitoggl.c). */
#ifndef BMX_COM_MENU_GUI_GUITOGGL_H
#define BMX_COM_MENU_GUI_GUITOGGL_H

#include <stdint.h>

#include "bmx/com/menu/gui/gui.h"

/*
 * Data block of a toggle element (locked through GuiElement.dataHandle). The choices are one string
 * "first|second|third"; `index` selects the one shown, and its text is copied into `textHandle`.
 */
typedef struct GuiToggleData {
    uint32_t unknown0;
    uint16_t index;       /* 0x04: current choice */
    uint16_t count;       /* 0x06: number of choices (set by the owner, not derived) */
    int textHandle;       /* 0x08: handle of the current choice's text */
    uint16_t textFlags;   /* 0x0c: same bits as GuiElement.textFlags */
    int16_t textSpacing;  /* 0x0e */
    int16_t textX, textY; /* 0x10 */
    void *font;           /* 0x14 */
    uint32_t textStyle;   /* 0x18 */
    int choicesHandle;    /* 0x1c: handle of the "a|b|c" list */
    uint8_t textRgb[3];   /* 0x20 */
    uint8_t pad23;
} GuiToggleData;

_Static_assert(sizeof(GuiToggleData) == 0x24, "GuiToggleData layout");

/* Type handlers: vertical (up/down move focus, left/right change) and horizontal. */
int BMX_CDECL guiToggleProc(int msg, GuiElement *el, int arg3, int arg4);
int BMX_CDECL guiToggleProcHoriz(int msg, GuiElement *el, int arg3, int arg4);

void guiToggleSetTextFlags(GuiElement *el, uint16_t flags);
void guiToggleSetTextPos(GuiElement *el, int16_t x, int16_t y);
void guiToggleSetTextStyle(GuiElement *el, uint32_t style);
void guiToggleSetFont(GuiElement *el, uint32_t fontId);
void guiToggleSetTextRgb(GuiElement *el, uint8_t r, uint8_t g, uint8_t b);
void guiToggleSetSpacing(GuiElement *el, int16_t spacing);
void guiToggleSetChoices(GuiElement *el, const char *choices);
void guiToggleSetIndex(GuiElement *el, uint16_t index);
uint16_t guiToggleGetIndex(const GuiElement *el);
void guiToggleSetCount(GuiElement *el, uint16_t count);
uint16_t guiToggleGetCount(const GuiElement *el);

#endif
