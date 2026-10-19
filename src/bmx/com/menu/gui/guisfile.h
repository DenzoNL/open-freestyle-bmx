/* GUI screen files (C:\zaxis\Bmx\src\com\menu\gui\guisfile.c): builds a screen from its asset. */
#ifndef BMX_COM_MENU_GUI_GUISFILE_H
#define BMX_COM_MENU_GUI_GUISFILE_H

#include <stdint.h>

#include "compat/orig.h"

/* On-disk screen header; elements and strings are addressed by byte offsets from its start. */
typedef struct GuiScreenFile {
    uint32_t magic;
    uint32_t unknown4;
    uint32_t unknown8;
    uint16_t backdrop; /* 0x0c: sprite id, 0 = none */
    uint16_t pad0e;
    uint32_t elementCount;   /* 0x10 */
    uint32_t elementsOffset; /* 0x14 */
    uint32_t unknown18;
    uint32_t stringsOffset; /* 0x1c */
    uint16_t focusFont;     /* 0x20 */
    uint8_t focusRgb[3];    /* 0x22 */
    uint8_t pad25;
    uint16_t normalFont;  /* 0x26 */
    uint8_t normalRgb[3]; /* 0x28 */
    uint8_t pad2b;
} GuiScreenFile;

/* On-disk element record (0xd4 bytes). Texts are offsets into the string area, 0 = none. */
typedef struct GuiScreenFileElement {
    uint32_t magic;
    uint32_t id;
    uint8_t nav[4];                   /* 0x08 */
    uint16_t flags;                   /* 0x0c */
    uint16_t type;                    /* 0x0e */
    uint16_t alpha;                   /* 0x10 */
    int16_t x, y;                     /* 0x12 */
    int16_t w, h;                     /* 0x16 */
    int16_t scaleX256, scaleY256;     /* 0x1a: scale in 1/256 units, used when flag 0x20 is set */
    int16_t slideStartX, slideStartY; /* 0x1e */
    uint16_t order;                   /* 0x22 */
    uint16_t sprite;                  /* 0x24 */
    uint16_t frame;                   /* 0x26 */
    uint16_t animCount;               /* 0x28 */
    uint16_t animFrames[30];          /* 0x2a */
    uint16_t animHold[30];            /* 0x66 */
    uint16_t spriteFocused;           /* 0xa2 */
    uint16_t animFlags;       /* 0xa4: bit 1 = no sprite part, bit 3 = alpha is set, bit 8 = has animation */
    int16_t spriteX, spriteY; /* 0xa6 */
    uint16_t font;            /* 0xaa */
    uint16_t textFlags;       /* 0xac: bit 1 = no text part */
    int16_t textX, textY;     /* 0xae */
    uint32_t textOffset;      /* 0xb4 */
    uint8_t textRgb[3];       /* 0xb8 */
    uint8_t padBB;
    uint16_t textSpacing; /* 0xbc */
    uint16_t padBE;
    uint16_t toggleFont;          /* 0xc0 */
    uint16_t toggleFlags;         /* 0xc2: bit 1 = no toggle part */
    int16_t toggleX, toggleY;     /* 0xc4 */
    uint32_t toggleChoicesOffset; /* 0xc8 */
    uint8_t toggleRgb[3];         /* 0xcc */
    uint8_t padCF;
    uint16_t toggleSpacing; /* 0xd0 */
    uint16_t padD2;
} GuiScreenFileElement;

_Static_assert(sizeof(GuiScreenFile) == 0x2c, "GuiScreenFile layout");
_Static_assert(sizeof(GuiScreenFileElement) == 0xd4, "GuiScreenFileElement layout");

/* Magic numbers (.data words at 0x55a210 and 0x55a1dc) the screen and element records must start with. */
#define gGuiScreenMagic  ORIG_VAR(0x0055a210, uint32_t, gGuiScreenMagic)
#define gGuiElementMagic ORIG_VAR(0x0055a1dc, uint32_t, gGuiElementMagic)

/* Creates all elements of screen `id`; returns non-zero on failure (after printing why). */
int guiBuildScreen(uint32_t id);

#endif
