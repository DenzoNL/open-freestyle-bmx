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

/*
 * The ordering table of the 3D debug primitives and the flush callbacks (0x400 entries chained downward:
 * entry 0x3ff is the start, entry 0 ends the chain). The 3D primitives (0x80 bytes each) are written at
 * gNgifPrim3D.
 */
typedef uint32_t NgifWords[1];
#define gNgifOtBase       ORIG_VAR(0x0055c698, uint32_t *, gNgifOtBase)
#define gNgifPrim3D       ORIG_VAR(0x0068ebe0, uint32_t *, gNgifPrim3D)
#define gNgifPrim3DBuffer ORIG_VAR(0x00678560, NgifWords, gNgifPrim3DBuffer)
#define gNgifPrimBuffer   ORIG_VAR(0x005ab4fc, NgifWords, gNgifPrimBuffer)
#define gNgifPrimStart    ORIG_VAR(0x005d250c, uint32_t *, gNgifPrimStart)
/* Frame flags: set when the ordering table holds something to draw (two tables of callbacks). */
#define gNgifFlushPending  ORIG_VAR(0x005d24fc, int, gNgifFlushPending)
#define gNgifFlushPending2 ORIG_VAR(0x005d2504, int, gNgifFlushPending2)
/* Update node handles of the two flush callbacks. */
#define gNgifFlushNode  ORIG_VAR(0x005d2500, void *, gNgifFlushNode)
#define gNgifFlushNode2 ORIG_VAR(0x005d2508, void *, gNgifFlushNode2)
/* Particle effect script (pairs of bytes, 0x80 ends it) and the update node that plays it. */
#define gNgifScript     ORIG_VAR(0x005ab4f0, const int8_t *, gNgifScript)
#define gNgifScriptNode ORIG_VAR(0x0068fcfc, void *, gNgifScriptNode)

/* Free list of the glyph texture tables (0x20 handles per font). */
#define gNgifGlyphPages ORIG_VAR(0x0068ff94, struct ZMemFreeList *, gNgifGlyphPages)

/* Writes a draw-area primitive (rect = x, y, w, h as int16) at prim. */
void ngifPrimSetDrawArea(uint32_t *prim, const int16_t *rect);

/* Vertex of the 3D debug primitives: position and an RGB colour. */
typedef struct NgifVert3 {
    float x, y, z;
    uint32_t pad;
    uint8_t rgb[3];
    uint8_t pad13;
} NgifVert3;

/* A texture of the 3D list: window in the page (u0, v0, u1, v1), size, and the material that holds it. */
typedef struct NgifTexture {
    uint16_t pageWord;      /* 0x00 */
    uint16_t clutWord;      /* 0x02 */
    uint8_t u0, v0, u1, v1; /* 0x04 */
    uint8_t pad08[4];
    uint8_t width, height; /* 0x0c */
    uint8_t pad0e[2];
    uint32_t material; /* 0x10 the material's handle (zrwmatHandle); 0 = unused */
} NgifTexture;

_Static_assert(sizeof(NgifTexture) == 0x14, "NgifTexture size");
#define gNgifTextures ORIG_VAR(0x0068c3e0, NgifTexture, gNgifTextures)

/* Draws a polygon (3 or 4 vertices, optionally textured) into the 3D list. */
void ngifDrawPoly3D(int flags, int count, const NgifVert3 *verts, int texture);
/* Writes a 3D line (colour words at +0x10 of each end) into the 3D list; only the low 2 bits of flags are
 * read. */
void ngifDrawLine3D(int flags, const NgifVert3 *a, const NgifVert3 *b);
int ngifInit(void);
void ngifShutdown(void);
void ngifBegin(void);
/* Starts a frame that shows picture data (texture handle at +0x20); the size arguments are not read. */
void ngifSetPictureScreen(const void *picture, int w, int h);
void ngifSetBlend(int a, int b);
/*
 * Draws one glyph of a font (palette `d` selects the texture, created on first use). a, b and c are not
 * read.
 */
void ngifDrawGlyph(const void *font, const void *glyph, int mode, int a, int b, int x, int y, int c, int d);
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

/* Sets the script of the particle system's camera animation and starts its update node on first use. */
void ngifpsxSetup(const void *params);
/* Update callback of that node: plays one step of the script; returns 1. */
int ngifpsxUpdate(void);

/* ---- ordering table ---------------------------------------------------------------------------- */

/* Links entries 0x3ff .. 1 of the table `ot` (n entries) to their predecessors and ends entry 0. */
void ngifInitOrderTable(uint32_t *ot, int n);
/* Draws every primitive of an ordering table, starting at its end entry. */
void ngifDrawList(uint32_t *entry);
/* Draws one primitive (dispatch on its command byte); unsupported kinds print "Prim not coded". */
void ngifDrawPrimitive(const uint8_t *prim);
void ngifNop2(void);
void ngifResetOrderTable(void);
/* Flush callbacks (update nodes of type 2): query the screen size, draw the pending list; return 1. */
int ngifFlushFrame(void);
int ngifFlushFrame2(void);
void ngifQueryScreen(void);
void ngifRegisterFlush(void);
void ngifUnregisterFlush(void);
void ngifRequestFlush2(void);
/* The ordering table entry the next primitive is linked into, and setter. */
uint32_t *ngifGetOrderTable(void);
void ngifSetOrderTable(uint32_t *table);

/* ---- primitives -------------------------------------------------------------------------------- */

/* Primitive kinds drawn through the renderer's draw hooks (flag word of the primitive, blend on / off). */
void ngifDrawPolyF3(const uint8_t *prim, int blend);
void ngifDrawPolyG4(const uint8_t *prim, int blend);
void ngifDrawLineF2(const uint8_t *prim, int blend);
void ngifDrawLineG2(const uint8_t *prim, int blend);
/* Draw-area primitive (code 0xf8): moves the current camera's viewport. */
void ngifDrawAreaPrim(const uint8_t *prim);

/* Quads: flat (0x28), textured flat (0x2c, nothing without a material) and textured Gouraud (0x3c). */
void ngifDrawPolyF4(const uint8_t *prim, int blend);
void ngifDrawPolyFT4(const uint8_t *prim, int blend);
void ngifDrawPolyGT4(const uint8_t *prim, int blend);

/* Draws one glyph of an overlay text from the scratch record at 0x55b960 (x is passed zero extended). */
void ngifDrawTextGlyph(uint32_t x, int y, const void *record);

/*
 * Screen space rectangle in the 3D list (letterbox bars, fades, overlay rectangles): colour (or texture
 * number), x, y, flag word and the size / window information the flags ask for.
 */
void ngifFillScreenBar(uint32_t color, uint32_t x, int y, uint32_t flags, const void *info);

#endif
