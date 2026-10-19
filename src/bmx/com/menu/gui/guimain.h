/* GUI core (C:\zaxis\Bmx\src\com\menu\gui\guimain.c): element list, messaging, screen update. */
#ifndef BMX_COM_MENU_GUI_GUIMAIN_H
#define BMX_COM_MENU_GUI_GUIMAIN_H

#include <stdint.h>

#include "bmx/com/menu/gui/gui.h"

/* Lifecycle. */
int guiInit(void);
void guiExit(void);
int guiModeInit(void);
void guiModeShutdown(void);
const char *guiSetAssetFile(const char *name);
void guiDestroyAll(void);
void guiRequestScreen(uint16_t screenId, GuiHookFn hook);
void guiLoadScreen(void);
int guiUpdate(void);
void guiPulseUpdate(void);
double guiAbsF(float x);
void guiSetHook(GuiHookFn hook);

/* Element list. */
GuiScreenState *guiGetScreenState(void);
GuiElement *guiFindById(uint32_t id);
GuiElement *guiGetByIndex(uint32_t index);
int BMX_CDECL guiCompareOrder(const void *a, const void *b);
void guiSortElements(void);
GuiElement *guiCreateElement(int16_t type, int16_t order, uint32_t id);
void guiDestroyElement(GuiElement *element);

/* Messaging. */
int guiSend(int msg, GuiElement *element, int arg3, int arg4);
int guiSendById(int msg, uint32_t id, int arg3, int arg4);
int guiBroadcast(int msg, GuiElement *element, int arg3, int arg4);
int guiPostToVisible(int msg, int arg2, int arg3);
int BMX_CDECL guiDefaultHookProc(int msg, GuiElement *element, int arg3, int arg4);
void guiFocusById(uint32_t id);

/* Element setters and getters. */
void guiSetFlags(GuiElement *el, uint16_t flags);
void guiSetNav(GuiElement *el, uint32_t index, uint8_t value);
void guiSetFlagById(uint32_t id, uint16_t mask, int16_t on);
void guiSetPos(GuiElement *el, int16_t x, int16_t y);
void guiGetPos(const GuiElement *el, int16_t *x, int16_t *y);
void guiSetOrder(GuiElement *el, uint16_t order);
void guiGetOrder(const GuiElement *el, uint16_t *order);
void guiSetScale(GuiElement *el, float sx, float sy);
void guiGetScale(const GuiElement *el, float *sx, float *sy);
void guiSetSize(GuiElement *el, int16_t w, int16_t h);
void guiSetSlideStart(GuiElement *el, int16_t x, int16_t y);
void guiSetAnimFlags(GuiElement *el, uint16_t flags);
void guiSetAnimPairs(GuiElement *el, uint16_t count, const uint8_t *pairs);
void guiSetSpriteOffset(GuiElement *el, int16_t x, int16_t y);
void guiSetSprite(GuiElement *el, uint32_t spriteId);
void guiSetSpritePtr(GuiElement *el, void *sprite);
void guiSetSpriteFocused(GuiElement *el, uint32_t spriteId);
void guiSetFrame(GuiElement *el, uint16_t frame);
void guiSetAnimIndex(GuiElement *el, int16_t index);
void guiSetAnimCount(GuiElement *el, uint16_t count);
void guiSetPalette(GuiElement *el, uint32_t palette);
uint32_t guiGetPalette(const GuiElement *el);
void guiSetAlpha(GuiElement *el, uint16_t alpha);
void guiSetAnimLists(GuiElement *el, uint32_t count, const uint16_t *frames, const uint16_t *holds);
void guiSetTextFlags(GuiElement *el, uint16_t flags);
void guiSetTextRgb(GuiElement *el, uint8_t r, uint8_t g, uint8_t b);
void guiSetTextPos(GuiElement *el, int16_t x, int16_t y);
void guiSetTextStyle(GuiElement *el, uint32_t style);
uint32_t guiGetTextStyle(const GuiElement *el);
void guiSetFont(GuiElement *el, uint32_t fontId);
void guiSetText(GuiElement *el, const char *str);
void guiSetTextSpacing(GuiElement *el, int16_t spacing);
int16_t guiGetTextSpacing(const GuiElement *el);
void guiSetTextFixedPitch(GuiElement *el, int on);
uint16_t guiGetTextWidth(const GuiElement *el);
void guiShow(GuiElement *el);
void guiHide(GuiElement *el);
int guiIsVisible(const GuiElement *el);
void guiSetHighlight(GuiElement *el, int on);
void guiSetTint(GuiElement *el, uint8_t r, uint8_t g, uint8_t b, uint32_t alpha);
const char *guiResolveText(const char *text);

/* Rectangle/line (type 7) and box (type 8) elements: two corner points. */
int BMX_CDECL guiRectProc(int msg, GuiElement *el, int arg3, int arg4);
int BMX_CDECL guiBoxProc(int msg, GuiElement *el, int arg3, int arg4);
void guiLineSetStart(GuiElement *el, int16_t x, int16_t y);
void guiLineSetEnd(GuiElement *el, int16_t x, int16_t y);
void guiLineGetX(const GuiElement *el, int16_t *x0, int16_t *x1);
void guiLineGetY(const GuiElement *el, int16_t *y0, int16_t *y1);
void guiBoxSetStart(GuiElement *el, int16_t x, int16_t y);
void guiBoxSetEnd(GuiElement *el, int16_t x, int16_t y);

/* Screen backdrop and focus fonts/colours. */
void guiDrawBackdropSprites(uint32_t spriteId, uint32_t backdropId, int arg3, int arg4);
void guiLoadBackdrop(uint16_t id, int arg);
void guiFreeBackdropSprites(void *sprites);
void guiSetBackdrop(uint32_t spriteId);
void guiSetFocusFont(uint16_t font);
void guiSetNormalFont(uint16_t font);
void guiSetFocusRgb(uint8_t r, uint8_t g, uint8_t b);
void guiSetNormalRgb(uint8_t r, uint8_t g, uint8_t b);
uint16_t guiGetFocusFont(void);
uint16_t guiGetNormalFont(void);
void guiGetFocusRgb(uint8_t *r, uint8_t *g, uint8_t *b);
void guiGetNormalRgb(uint8_t *r, uint8_t *g, uint8_t *b);

#endif
