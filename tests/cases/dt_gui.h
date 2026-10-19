/* Shared fixtures for the GUI tests (guimain, guielem, guilabel, guisfile, guitoggl, guiinput). */
#ifndef DT_GUI_H
#define DT_GUI_H

#include <stdint.h>

#include "bmx/com/menu/gui/gui.h"
#include "dt.h"

/* Default size of the test handle heap. */
#define DT_GUI_HEAP 0x8000

/* Calls original (unported) code at a fixed address. */
#define DT_GUI_ORIG(addr, type) ((type)(uintptr_t)(addr))

/*
 * Replacement for the engine's debug print: logs how often it was called and a hash of the format text
 * and its integer arguments (one per '%'). Both sides are compared through this DLL state.
 */
void BMX_CDECL dtGuiDebugPrint(const char *fmt, ...);

/* Calls recorded by dtGuiProc / dtGuiHook (message, element, args) folded into a hash. */
int BMX_CDECL dtGuiProc(int msg, GuiElement *el, int arg3, int arg4);
int BMX_CDECL dtGuiProcOther(int msg, GuiElement *el, int arg3, int arg4);
int BMX_CDECL dtGuiHook(int msg, GuiElement *el, int arg3, int arg4);

/*
 * Clears the element table and the GUI globals, installs the debug stub and sets up an empty handle
 * heap (the allocator used for texts and toggle data) with `heapSize` bytes.
 */
void dtGuiReset(uint32_t heapSize);

/* A random free-standing element in table slot `slot` (not registered in the sorted list). */
GuiElement *dtGuiRandomElement(int slot);
/* Registers slot `slot` in the sorted list (appended). */
void dtGuiRegister(int slot);
/* `count` random elements registered in slots 0..count-1, sorted by order like the original keeps them. */
void dtGuiRandomScreen(int count);

/*
 * Random controller state (button bits biased towards d-pad subsets, repeat state around the 20 frame
 * threshold), a random device behind it, and a random analog flag.
 */
void dtGuiRandomPad(void);

/* A NUL-terminated random text and a handle holding a copy of it (allocated with the original code). */
char *dtGuiRandomText(int maxLen, int allowNewline);
int dtGuiTextHandle(const char *text);

/*
 * Gives a toggle element its data block: random text settings, a choices string "a|b|c" (when
 * withChoices) and the current choice's text (when withText), both as handles.
 */
void dtGuiMakeToggleData(GuiElement *el, int withChoices, int withText);

/* Fake asset record (what zassetFindSprite returns) backed by a handle; `data` is copied into it. */
void *dtGuiMakeAsset(uint16_t id, const void *data, uint32_t size);
/* Sprite sheet asset with `frames` frames of random size; fills sizes[] with width/height pairs. */
void *dtGuiMakeSprite(uint16_t id, int frames);
/* Font asset covering the printable characters with random advances; `height` is its line height. */
void *dtGuiMakeFont(uint16_t id, uint16_t height);

#endif
