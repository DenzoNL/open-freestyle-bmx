/* Display list builder (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFpsx.c and NgIFzactor.c). Not ported yet. */
#ifndef BMX_COM_ZGE_NGIF_NGIFPSX_H
#define BMX_COM_ZGE_NGIF_NGIFPSX_H

#include <stdint.h>

#include "compat/orig.h"

/* Vertex of the 3D debug primitives: position and an RGB colour. */
typedef struct NgifVert3 {
    float x, y, z;
    uint32_t pad;
    uint8_t rgb[3];
    uint8_t pad13;
} NgifVert3;

ORIG_FN(0x00463130, void, ngifDrawPoly3D, (int flags, int count, const NgifVert3 *verts, int texture));
ORIG_FN(0x00463710, void, ngifDrawLine3D, (int flags, const NgifVert3 *a, const NgifVert3 *b));
ORIG_FN(0x004639f0, int, ngifInit, (void));
ORIG_FN(0x00463a60, void, ngifShutdown, (void));
ORIG_FN(0x00463ab0, void, ngifBegin, (void));
/* Starts a frame that shows picture data (texture handle at +0x20) at size w x h. */
ORIG_FN(0x00463b20, void, ngifSetPictureScreen, (const void *picture, int w, int h));
ORIG_FN(0x00463c40, void, ngifSetBlend, (int a, int b));
/* Draws one glyph of a font. */
ORIG_FN(0x00463c70, void, ngifDrawGlyph,
        (const void *font, const void *glyph, int mode, int a, int b, int x, int y, int c, int d));
ORIG_FN(0x00464030, void, ngifSetClip, (int x0, int y0, int x1, int y1));
/* Draws a sprite frame. */
ORIG_FN(0x00464220, void, ngifDrawSprite,
        (const void *set, int frame, const void *pixels, int mode, int a, int palette, int c, int x, int y,
         int w, int h, int bpp, int d));
ORIG_FN(0x00464420, void, ngifFillRect, (int color, int a, int x0, int y0, int x1, int y1));
ORIG_FN(0x004645d0, void, ngifDrawLine, (int color, int unused, int x0, int y0, int x1, int y1));

#endif
