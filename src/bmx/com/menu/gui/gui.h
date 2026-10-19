/*
 * Front-end GUI core types shared by guimain.c, guisfile.c, guilabel.c, guiinput.c and guitoggl.c
 * (C:\zaxis\Bmx\src\com\menu\gui\). A screen is a set of up to 100 elements; each element has a type
 * handler that receives messages (draw, focus, navigation, ...).
 */
#ifndef BMX_COM_MENU_GUI_GUI_H
#define BMX_COM_MENU_GUI_GUI_H

#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"

enum { kGuiMaxElements = 100 };

/* Message ids sent to element handlers (and broadcast through the screen hook). */
enum GuiMsg {
    kGuiMsgCreate = 0xa,
    kGuiMsgTick = 0xb,
    kGuiMsgDraw = 0xc,
    kGuiMsgDestroy = 0xd,
    kGuiMsgFocus = 0xe,
    kGuiMsgBlur = 0xf,
    kGuiMsgTextChanged = 0x10,
    kGuiMsgScreenLoaded = 0x14,
    kGuiMsgDrawAll = 0x15, /* broadcast: every live element gets kGuiMsgDraw */
    kGuiMsgTickAll = 0x16, /* broadcast: every live element gets kGuiMsgTick */
    kGuiMsgScreenClosing = 0x17,
    kGuiMsgSlideDone = 0x18,
    kGuiMsgNavUp = 0x1e,
    kGuiMsgNavDown = 0x1f,
    kGuiMsgNavLeft = 0x20,
    kGuiMsgNavRight = 0x21,
    kGuiMsgButton22 = 0x22, /* 0x22..0x2a: pad button presses */
    kGuiMsgButton23 = 0x23,
    kGuiMsgButton24 = 0x24,
    kGuiMsgButton25 = 0x25,
    kGuiMsgButton26 = 0x26,
    kGuiMsgButton27 = 0x27,
    kGuiMsgButton28 = 0x28,
    kGuiMsgButton29 = 0x29,
    kGuiMsgButton2a = 0x2a,
    kGuiMsgNavUpAlt = 0x2b,
    kGuiMsgNavDownAlt = 0x2c,
    kGuiMsgNavLeftAlt = 0x2d,
    kGuiMsgNavRightAlt = 0x2e,
    kGuiMsgIdle = 0x32,
    kGuiMsgButton3e = 0x3e,
    kGuiMsgButton3f = 0x3f,
};

/* Element types (GuiElement.type); 0 marks a free slot. */
enum GuiType {
    kGuiTypeFree = 0,
    kGuiTypeLabel = 1,
    kGuiTypeButton = 2,
    kGuiTypeToggle = 3,
    kGuiTypeLabel5 = 5, /* label whose animation frame starts at 0 */
    kGuiTypeNone = 6,
    kGuiTypeRect = 7,
    kGuiTypeBox = 8,
};

/* GuiElement.flags */
enum {
    kGuiFlagHidden = 0x1,
    kGuiFlagFocused = 0x2,
    kGuiFlagScaled = 0x20,     /* scaleX/scaleY apply */
    kGuiFlagWide = 0x40,       /* stretch the width to 160% */
    kGuiFlagCenterX = 0x80,    /* x is the centre of the sprite */
    kGuiFlagCenterY = 0x100,   /* y is the centre of the sprite */
    kGuiFlagHighlight = 0x200, /* draw the highlight tint while focused */
    kGuiFlagTinted = 0x400,    /* tint colour and alpha are set */
};

/* GuiElement.animFlags */
enum {
    kGuiAnimDisabled = 0x3, /* any of these bits: no sprite animation, sprite is not drawn */
    kGuiAnimBit4 = 0x4,     /* render state 8 */
    kGuiAnimBit8 = 0x8,     /* render states 7/0xf: extra blend with alpha */
    kGuiAnimPlaying = 0x100,
    kGuiAnimReverse = 0x200,
    kGuiAnimLoop = 0x400,
    kGuiAnimPingPong = 0x800,
};

/* GuiElement.textFlags */
enum {
    kGuiTextDisabled = 0x3,
    kGuiTextBit4 = 0x4, /* render state 0x13 */
    kGuiTextBit8 = 0x8, /* render state 0x12: tint */
    kGuiTextColored = 0x10,
    kGuiTextCenterX = 0x20,
    kGuiTextCenterY = 0x40,
    kGuiTextRight = 0x80,
    kGuiTextFixedPitch = 0x2000, /* width is length * spacing; draws with the alternative routine */
};

typedef struct GuiElement GuiElement;
/* Element message handler: (message, element, arg3, arg4); returns non-zero when handled. */
typedef int(BMX_CDECL *GuiProcFn)(int msg, GuiElement *element, int arg3, int arg4);

struct GuiElement {
    int16_t type;           /* 0x00: GuiType, 0 = slot unused */
    uint16_t flags;         /* 0x02 */
    uint32_t id;            /* 0x04: element id (low byte is the index within a screen block) */
    uint8_t nav[4];         /* 0x08: low byte of the id to focus for up/down/left/right (0 = none) */
    int16_t x, y;           /* 0x0c */
    int16_t w, h;           /* 0x10 */
    int16_t slideX, slideY; /* 0x14: current screen-transition offset */
    int16_t slideStartX, slideStartY;  /* 0x18: offset the element slides in from / out to */
    uint16_t slideDelayX, slideDelayY; /* 0x1c: frames to wait before sliding in */
    float scaleX, scaleY;              /* 0x20 */
    uint16_t order;                    /* 0x28: sort key of the element list */
    uint16_t pad2a;
    uint32_t tintRgb;         /* 0x2c: 0x00BBGGRR */
    uint32_t tintAlpha;       /* 0x30 */
    void *sprite;             /* 0x34 */
    void *spriteFocused;      /* 0x38 */
    int16_t spriteX, spriteY; /* 0x3c: sprite offset inside the element */
    uint16_t animFlags;       /* 0x40 */
    uint16_t frame;           /* 0x42: sprite frame being drawn */
    int16_t animIndex;        /* 0x44: current step of the animation */
    uint16_t animTicks;       /* 0x46: frames spent on the current step */
    uint32_t palette;         /* 0x48: palette index (render state 0xe), must be below 16 to draw */
    uint16_t alpha;           /* 0x4c: render state 0xf */
    uint16_t animCount;       /* 0x4e: steps */
    uint16_t animFrames[30];  /* 0x50: frame of each step */
    uint16_t animHold[30];    /* 0x8c: ticks each step is held */
    uint16_t textFlags;       /* 0xc8 */
    int16_t textSpacing;      /* 0xca */
    int16_t textX, textY;     /* 0xcc */
    void *font;               /* 0xd0 */
    uint32_t textStyle;       /* 0xd4 */
    int textHandle;           /* 0xd8: handle of the NUL-terminated text, 0 = none */
    uint32_t textCapacity;    /* 0xdc: bytes allocated for the text */
    uint8_t textRgb[3];       /* 0xe0 */
    uint8_t pade3;
    GuiProcFn proc;        /* 0xe4: per-element override, tried first */
    GuiProcFn defaultProc; /* 0xe8: type handler */
    int dataHandle;        /* 0xec: type specific data (toggle choices) */
};

_Static_assert(sizeof(GuiElement) == 0xf0, "GuiElement layout");
_Static_assert(offsetof(GuiElement, animFrames) == 0x50, "GuiElement layout");
_Static_assert(offsetof(GuiElement, textFlags) == 0xc8, "GuiElement layout");
_Static_assert(offsetof(GuiElement, dataHandle) == 0xec, "GuiElement layout");

/* Screen hook: gets every broadcast first and may consume it. */
typedef int(BMX_CDECL *GuiHookFn)(int msg, GuiElement *element, int arg3, int arg4);

/* Backdrop, selection fonts/colours and the hook, one block of six dwords. */
typedef struct GuiScreenState {
    int backdrop; /* 0x00: sprite handle, 0 = none */
    uint32_t unknown4;
    uint16_t focusFont;  /* 0x08: font id for focused text, 0 = leave alone */
    uint8_t focusRgb[3]; /* 0x0a */
    uint8_t padD;
    uint16_t normalFont;  /* 0x0e */
    uint8_t normalRgb[3]; /* 0x10 */
    uint8_t pad13;
    GuiHookFn hook; /* 0x14 */
} GuiScreenState;

_Static_assert(sizeof(GuiScreenState) == 0x18, "GuiScreenState layout");

typedef GuiElement GuiElementArray[kGuiMaxElements];
typedef GuiElement *GuiElementPtrArray[kGuiMaxElements];
#define gGuiElements ORIG_VAR(0x0056d2c0, GuiElementArray, gGuiElements)
/* Pointers to the live elements, sorted by `order` (descending), gGuiElementCount entries. */
#define gGuiSorted       ORIG_VAR(0x005730a4, GuiElementPtrArray, gGuiSorted)
#define gGuiElementCount ORIG_VAR(0x0057323c, int, gGuiElementCount)
#define gGuiScreen       ORIG_VAR(0x00573088, GuiScreenState, gGuiScreen)
/* Hook installed when a screen is loaded (read by guiLoadScreen / guiInit). */
#define gGuiDefaultHook   ORIG_VAR(0x005730a0, GuiHookFn, gGuiDefaultHook)
#define gGuiScreenId      ORIG_VAR(0x00573080, uint16_t, gGuiScreenId)
#define gGuiInitialized   ORIG_VAR(0x00573234, int, gGuiInitialized)
#define gGuiModeReady     ORIG_VAR(0x00573238, int, gGuiModeReady)
#define gGuiScreenLoaded  ORIG_VAR(0x00573240, int, gGuiScreenLoaded)
#define gGuiReloadPending ORIG_VAR(0x00573244, int, gGuiReloadPending)
#define gGuiReloadDelay   ORIG_VAR(0x00573248, int, gGuiReloadDelay)
#define gGuiSkipUpdate    ORIG_VAR(0x0057324c, int, gGuiSkipUpdate)
#define gGuiAssetsLoaded  ORIG_VAR(0x00573250, int, gGuiAssetsLoaded)
/* Pulsing brightness of the focus highlight (0x10..0xf0) and its step. */
#define gGuiPulse     ORIG_VAR(0x00573254, int, gGuiPulse)
#define gGuiPulseStep ORIG_VAR(0x00559d4c, int, gGuiPulseStep)
#define gGuiPulseHold ORIG_VAR(0x00573258, int, gGuiPulseHold)
/* Screen transition: 0 idle, 1 sliding out, 2 sliding in. */
#define gGuiSlideState ORIG_VAR(0x0057325c, int, gGuiSlideState)
#define gGuiSlideMax   ORIG_VAR(0x00573260, int, gGuiSlideMax)
#define gGuiAssetFile  ORIG_VAR(0x00559d48, const char *, gGuiAssetFile)
/* Set while the front end runs in its idle (attract) mode: input is replaced by kGuiMsgIdle. */
#define gGuiIdleMode ORIG_VAR(0x005ab474, int, gGuiIdleMode)
/* Free-slot filler byte used when a text is set to the empty string. */
#define gGuiEmptyChar ORIG_VAR(0x0056bd44, uint8_t, gGuiEmptyChar)
/* Element id whose text the next "!B$text$" markup sets (0 = none). */
#define gGuiMarkupTarget ORIG_VAR(0x0056d290, uint32_t, gGuiMarkupTarget)
/* Engine debug print: printf style, goes through a function pointer. */
typedef void(BMX_CDECL *GuiDebugPrintFn)(const char *fmt, ...);
#define gGuiDebugPrint ORIG_VAR(0x00560074, GuiDebugPrintFn, gGuiDebugPrint)

#endif
