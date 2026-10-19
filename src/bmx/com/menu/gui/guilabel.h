/* GUI label element and its text markup (type 1/5 handler and the "!S/!T/!B" markup parser). */
#ifndef BMX_COM_MENU_GUI_GUILABEL_H
#define BMX_COM_MENU_GUI_GUILABEL_H

#include <stdint.h>

#include "bmx/com/menu/gui/gui.h"

/* Label/sprite element: tick is a no-op, draw renders the sprite (with animation) and the text. */
int BMX_CDECL guiLabelProc(int msg, GuiElement *el, int arg3, int arg4);

/* Parses an optionally signed integer after spaces/commas; returns the position after it (and trailing
 * spaces/commas). */
const char *guiParseInt(const char *s, int *out);
/*
 * Runs the inline commands of a text line that starts with '!': "!S a b id" draws sprite id at
 * (x+a, y+b), "!B$text$" sets the text of the element registered with guiSetMarkupTarget, "!T a b $text$"
 * is parsed but draws nothing. Commands can be chained.
 */
void guiParseMarkup(const char *s, int x, int y);
void guiSetMarkupTarget(uint32_t id);

#endif
