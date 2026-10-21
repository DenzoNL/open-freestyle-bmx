/* Display list builder (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFpsx.c). Partly ported. */
#ifndef BMX_COM_ZGE_NGIF_NGIFPSX_H
#define BMX_COM_ZGE_NGIF_NGIFPSX_H

#include <stdint.h>

#include "compat/orig.h"

/*
 * Display list state: primitives are written at gNgifPrim (advanced past each one) and linked into the
 * ordering table entry gNgifOrderTable points at (which then moves down one word). Words keep the
 * PlayStation GPU tag layout: low 28 bits = link, top 4 bits = size.
 */
#define gNgifPrim       ORIG_VAR(0x005d2510, uint32_t *, gNgifPrim)
#define gNgifOrderTable ORIG_VAR(0x0068c3d0, uint32_t *, gNgifOrderTable)
/* Colour modulation (0 = none) and brightness (0 = full) applied to flat primitives. */
#define gNgifColor      ORIG_VAR(0x0068fc00, uint32_t, gNgifColor)
#define gNgifBrightness ORIG_VAR(0x0055c68c, uint32_t, gNgifBrightness)
/* Screen size the clip rectangle is scaled from (640 x 480). */
#define gNgifScreenW ORIG_VAR(0x0055c690, float, gNgifScreenW)
#define gNgifScreenH ORIG_VAR(0x0055c694, float, gNgifScreenH)

/* Writes a draw-area primitive (rect = x, y, w, h as int16) at prim; not ported yet. */
ORIG_FN(0x0045fe00, void, ngifPrimSetDrawArea, (uint32_t *prim, const int16_t *rect));

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
void ngifSetClip(int x0, int y0, int x1, int y1);
/*
 * Draws a sprite frame as a textured quad. `set` is a sprite set whose frame table (16 byte entries
 * from index 2) holds the texture word of each frame; mode 1 selects the semi-transparent variant, which
 * takes `blend` as its blend word. The original never reads pixels, a, b, bpp and d (callers still push
 * all thirteen arguments).
 */
void ngifDrawSprite(const void *set, int frame, const void *pixels, int mode, int a, int b, int blend, int x,
                    int y, int w, int h, int bpp, int d);
/* Flat coloured rectangle (x0,y0)-(x1,y1); mode 1 selects the semi-transparent variant. */
void ngifFillRect(int color, int mode, int x0, int y0, int x1, int y1);
/* Flat coloured line; the second argument is not read. */
void ngifDrawLine(int color, int unused, int x0, int y0, int x1, int y1);

/* Empty callback of the display list function table. */
void ngifNop(void);
/* Callbacks of the function table that report success (1). */
int ngifReturnOneA(void);
int ngifReturnOneB(void);

/* Sets the parameter block of the particle system's emitter and creates its helper object on first use. */
ORIG_FN(0x0045fdc0, void, ngifpsxSetup, (const void *params));

/* Draws one glyph of an overlay text from the scratch record at 0x55b960 (x is passed zero extended). */
ORIG_FN(0x00462940, void, ngifDrawTextGlyph, (uint32_t x, int y, const void *record));

/* Letterbox fill used by the overlay: colour, x, y, width and a pointer to an ordering table entry. */
ORIG_FN(0x004623a0, void, ngifFillScreenBar,
        (uint32_t color, uint32_t x, int y, uint32_t width, const void *ot));

#endif
