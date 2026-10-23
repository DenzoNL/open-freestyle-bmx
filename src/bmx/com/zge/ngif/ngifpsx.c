/* Display list builder (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFpsx.c): primitive writers. */
#include "bmx/com/zge/ngif/ngifpsx.h"

#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"
#include "zge/com/zglrw/zrwdbg.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"

/* Primitives are filled through byte offsets; the layout is the PlayStation GPU packet format. */
static void put8(uint8_t *p, unsigned off, uint32_t v)
{
    p[off] = (uint8_t)v;
}

static void put16(uint8_t *p, unsigned off, uint32_t v)
{
    *(uint16_t *)(p + off) = (uint16_t)v;
}

static void put32(uint8_t *p, unsigned off, uint32_t v)
{
    *(uint32_t *)(p + off) = v;
}

static uint32_t get32(const uint8_t *p, unsigned off)
{
    return *(const uint32_t *)(p + off);
}

/* Replaces the low 28 bits (the link) of `word` with those of `link`. */
static uint32_t withLink(uint32_t word, uint32_t link)
{
    return ((link ^ word) & 0x0fffffffu) ^ word;
}

/*
 * Links the primitive at `p` (header word `tag`) into the ordering table entry and moves both cursors
 * on; `words` is the primitive size.
 */
static void linkPrim(uint8_t *p, uint32_t tag, unsigned words)
{
    uint32_t *prim = (uint32_t *)p;
    *prim = withLink(tag, *gNgifOrderTable);
    *gNgifOrderTable = withLink(*gNgifOrderTable, (uint32_t)(uintptr_t)prim);
    gNgifOrderTable = gNgifOrderTable - 1;
    gNgifPrim = prim + words;
}

/* Colour bytes of a flat primitive: the modulation colour scaled by the brightness, else `color`. */
static void putColor(uint8_t *p, uint32_t color)
{
    if (gNgifColor == 0) {
        put8(p, 4, color);
        put8(p, 5, color >> 8);
        put8(p, 6, color >> 16);
    } else {
        put8(p, 4, ((gNgifColor & 0xff) * gNgifBrightness) >> 8);
        put8(p, 5, (((gNgifColor >> 8) & 0xff) * gNgifBrightness) >> 8);
        put8(p, 6, (((gNgifColor >> 16) & 0xff) * gNgifBrightness) >> 8);
    }
}

/*
 * Sets the draw area. A rectangle of 640x480 (mod 16 bit) is replaced by an empty one at the origin;
 * anything else is shifted down 16 lines. The rectangle is scaled by the screen size / 512 and / 480.
 */
BMX_ADDR(0x00464030)
void ngifSetClip(int x0, int y0, int x1, int y1)
{
    uint32_t *prim = gNgifPrim;
    int16_t rect[4];
    int dx, dy;

    dx = (int)(x1 & 0xffff) - (int)(x0 & 0xffff);
    dy = (int)(y1 & 0xffff) - (int)(y0 & 0xffff);
    if ((dx < 0 ? -dx : dx) == 0x27f && (dy < 0 ? -dy : dy) == 0x1df) {
        x0 = x1 = y0 = y1 = 0;
    } else {
        y0 += 0x10;
        y1 += 0x10;
    }
    if (prim == NULL) {
        return;
    }
    {
        const double scaleX = (double)gNgifScreenW * 0.001953125;
        const float scaleY = (float)((double)gNgifScreenH * 0.0020833333333333333);
        const int ux0 = x0 & 0xffff, uy0 = y0 & 0xffff;

        rect[0] = (int16_t)bmxFtol((double)ux0 * scaleX);
        rect[1] = (int16_t)bmxFtol((double)uy0 * (double)scaleY);
        rect[2] = (int16_t)bmxFtol((double)((x1 & 0xffff) - ux0) * scaleX);
        rect[3] = (int16_t)bmxFtol((double)((y1 & 0xffff) - uy0) * (double)scaleY);
    }
    ngifPrimSetDrawArea(prim, rect);
    linkPrim((uint8_t *)prim, *prim, 6);
}

BMX_ADDR(0x00464200)
void ngifNop(void) {}

BMX_ADDR(0x00464220)
void ngifDrawSprite(const void *set, int frame, const void *pixels, int mode, int a, int b, int blend, int x,
                    int y, int w, int h, int bpp, int d)
{
    uint8_t *p = (uint8_t *)gNgifPrim;
    const uint32_t tag = (get32(p, 0) & 0x0fffffffu) | 0x90000000u;
    const uint32_t flags = (get32(p, 0x34) & 0xffffff82u) | 0x7f82u;
    const uint32_t y68 = (uint32_t)y + 0x68;
    const uint32_t frameWord =
        *(const uint32_t *)((const uint8_t *)set + (((uint32_t)frame & 0xffff) + 2) * 0x10);
    uint32_t bright;

    put32(p, 0, tag);
    put8(p, 7, 0x2c);
    put32(p, 0x34, flags);
    if (gNgifColor == 0) {
        put8(p, 4, 0xff);
        put8(p, 5, 0xff);
        put8(p, 6, 0xff);
    } else {
        putColor(p, 0);
    }
    put16(p, 0xc, 0);
    put16(p, 0xe, 0);
    put16(p, 0x8, (uint32_t)x);
    put16(p, 0x1c, (uint32_t)x);
    put16(p, 0xa, y68);
    put16(p, 0x14, y68);
    put16(p, 0x22, (uint32_t)h);
    put16(p, 0x2c, (uint32_t)h);
    put16(p, 0x1e, y68 + (uint32_t)h);
    put16(p, 0x28, y68 + (uint32_t)h);
    put16(p, 0x12, (uint32_t)(x + w));
    put16(p, 0x16, (uint32_t)w);
    put16(p, 0x18, 0);
    put16(p, 0x20, 0);
    put16(p, 0x26, (uint32_t)(x + w));
    put16(p, 0x2a, (uint32_t)w);
    put32(p, 0x30, frameWord);
    bright = gNgifBrightness == 0 ? 0xff : gNgifBrightness;
    put32(p, 0x34, ((bright & 0xff) << 7) | (flags & 0xffff807fu) | 4);
    if ((mode & 0xff) == 1) {
        put8(p, 7, 0x2e);
        put16(p, 0x1a, (uint32_t)blend << 5);
    } else {
        put8(p, 7, 0x2c);
        put16(p, 0x1a, 0);
    }
    linkPrim(p, tag, 0x27);
}

BMX_ADDR(0x00464420)
void ngifFillRect(int color, int mode, int x0, int y0, int x1, int y1)
{
    uint8_t *p = (uint8_t *)gNgifPrim;
    const uint32_t tag = (get32(p, 0) & 0x0fffffffu) | 0x50000000u;
    const uint32_t flags = (get32(p, 0x1c) & 0xffffff82u) | 0x7f82u;
    const uint32_t ya = (uint32_t)y0 + 0x68, yb = (uint32_t)y1 + 0x68;
    uint32_t bright;

    put32(p, 0, tag);
    put8(p, 7, 0x28);
    put32(p, 0x1c, flags);
    putColor(p, (uint32_t)color);
    put16(p, 0xa, ya);
    put16(p, 0x8, (uint32_t)x0);
    put16(p, 0xc, (uint32_t)x1);
    put16(p, 0xe, ya);
    put16(p, 0x10, (uint32_t)x0);
    put16(p, 0x12, yb);
    put16(p, 0x14, (uint32_t)x1);
    put16(p, 0x16, yb);
    put32(p, 0x18, 0);
    bright = gNgifBrightness == 0 ? 0xff : gNgifBrightness;
    put32(p, 0x1c, ((bright & 0xff) << 7) | (flags & 0xffff807fu) | 4);
    put8(p, 7, (mode & 0xff) == 1 ? 0x2a : 0x28);
    linkPrim(p, tag, 0x21);
}

BMX_ADDR(0x004645d0)
void ngifDrawLine(int color, int unused, int x0, int y0, int x1, int y1)
{
    uint8_t *p = (uint8_t *)gNgifPrim;
    const uint32_t tag = (get32(p, 0) & 0x0fffffffu) | 0x30000000u;

    put8(p, 4, (uint32_t)color);
    put8(p, 6, (uint32_t)color >> 16);
    put16(p, 8, (uint32_t)x0);
    put16(p, 0xa, (uint32_t)y0 + 0x68);
    put16(p, 0xc, (uint32_t)x1);
    put16(p, 0xe, (uint32_t)y1 + 0x68);
    put32(p, 0, tag);
    put8(p, 7, 0x40);
    put8(p, 5, (uint32_t)color >> 8);
    put32(p, 0x14, (get32(p, 0x14) & 0xffffffdbu) | 2);
    linkPrim(p, tag, 0x1f);
}

BMX_ADDR(0x004646b0)
int ngifReturnOneA(void)
{
    return 1;
}

BMX_ADDR(0x004646d0)
int ngifReturnOneB(void)
{
    return 1;
}

/* ---- engine hooks (function pointers in the exe's data) ----------------------------------------- */

typedef int(BMX_CDECL *NgifUpdateFn)(void);
typedef int(BMX_CDECL *NgifRegisterFn)(int type, NgifUpdateFn callback, void *arg, int append);
typedef void(BMX_CDECL *NgifUnregisterFn)(int node);
typedef void(BMX_CDECL *NgifQueryFn)(int what, void *out, int arg);
typedef void *(BMX_CDECL *NgifCameraFn)(void);
typedef void(BMX_CDECL *NgifSetViewportFn)(void *camera, int flags, const float *rect);
typedef void(BMX_CDECL *NgifSetOffsetFn)(void *camera, float x, float y);
#define gNgifRegister      ORIG_VAR(0x00561470, NgifRegisterFn, gNgifRegister)
#define gNgifUnregister    ORIG_VAR(0x00561474, NgifUnregisterFn, gNgifUnregister)
#define gNgifQuery         ORIG_VAR(0x00561488, NgifQueryFn, gNgifQuery)
#define gNgifGetCamera     ORIG_VAR(0x00561db0, NgifCameraFn, gNgifGetCamera)
#define gNgifSetViewport   ORIG_VAR(0x00561db8, NgifSetViewportFn, gNgifSetViewport)
#define gNgifSetViewOffset ORIG_VAR(0x00561dc0, NgifSetOffsetFn, gNgifSetViewOffset)
#define gNgifDrawQuads     ORIG_VAR(0x00561738, ZrwDrawFn, gNgifDrawQuads)

static const char kNgifFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFpsx.c";

typedef struct NgifPoly4 {
    uint32_t head;
    ZrwDbgVertex *verts[4];
} NgifPoly4;

static uint16_t rd16(const void *p, unsigned off)
{
    uint16_t v;
    memcpy(&v, (const uint8_t *)p + off, sizeof v);
    return v;
}

static uint32_t rd32(const void *p, unsigned off)
{
    uint32_t v;
    memcpy(&v, (const uint8_t *)p + off, sizeof v);
    return v;
}

/* ---- particle camera script ----------------------------------------------------------------------- */

/*
 * Camera animation of the particle system: each step moves the view offset by a pair of signed bytes
 * (in 0.005 units); 0x80 ends the script and resets the offset.
 */
BMX_ADDR(0x0045fcf0)
int ngifpsxUpdate(void)
{
    void *camera = gNgifGetCamera();

    if (gNgifScript != NULL) {
        const int8_t first = *gNgifScript;

        if (first == -0x80) {
            gNgifUnregister(gNgifScriptNode);
            gNgifSetViewOffset(camera, 0.0f, 0.0f);
            gNgifScript = NULL;
            gNgifScriptNode = 0;
            return 1;
        }
        const int8_t second = gNgifScript[1];

        gNgifScript += 2;
        gNgifSetViewOffset(camera, (float)((double)first * 0.005f), (float)((double)second * 0.005f));
    }
    return 1;
}

BMX_ADDR(0x0045fdc0)
void ngifpsxSetup(const void *params)
{
    gNgifScript = params;
    if (gNgifScriptNode == 0) {
        gNgifScriptNode = gNgifRegister(1, BMX_FNPTR(NgifUpdateFn, 0x00401a4b, ngifpsxUpdate), NULL, 0);
    }
}

/* Draw-area primitive: 5 words, command 0xf8 (the command byte store is overwritten by the tag store). */
BMX_ADDR(0x0045fe00)
void ngifPrimSetDrawArea(uint32_t *prim, const int16_t *rect)
{
    const uint32_t word = *prim;

    ((uint8_t *)prim)[7] = 0xf8;
    *prim = (word & 0x0fffffffu) | 0x50000000u;
    prim[2] = (uint32_t)(int32_t)rect[0];
    prim[3] = (uint32_t)(int32_t)rect[1];
    prim[4] = (uint32_t)(int32_t)rect[2];
    prim[5] = (uint32_t)(int32_t)rect[3];
}

/*
 * Draw-area primitive: moves the viewport of the current camera (words 4 and 5 zero: back to the full
 * one). The y position is lifted by the screen scale times 32.
 */
BMX_ADDR(0x0045fe50)
void ngifDrawAreaPrim(const uint8_t *prim)
{
    const uint32_t *const w = (const uint32_t *)prim;
    void *camera;
    float scale;
    float rect[4];

    if (w[4] == 0 && w[5] == 0) {
        camera = gNgifGetCamera();
        gNgifSetViewport(camera, 0, NULL);
        return;
    }
    scale = 1.0f;
    gNgifQuery(0xb, &scale, 0);
    scale = (float)((double)scale * 0.0020833334f);
    rect[0] = (float)w[2];
    rect[1] = (float)((double)w[3] - (double)scale * 32.0f);
    rect[2] = (float)w[4];
    rect[3] = (float)w[5];
    camera = gNgifGetCamera();
    gNgifSetViewport(camera, 0, rect);
}

/* Prepares an ordering table of n entries: every entry links to the one below, entry 0 ends the chain. */
BMX_ADDR(0x0045ff60)
void ngifInitOrderTable(uint32_t *ot, int n)
{
    int16_t last = (int16_t)(n - 1);

    if (last > 0) {
        for (int32_t i = last; i != 0; i--) {
            ot[i] = (uint32_t)(uintptr_t)(ot + i - 1) & 0x0fffffffu;
        }
        last = 0;
    }
    ot[last] |= 0x0fffffffu;
}

/* ---- polygons and lines through the renderer ------------------------------------------------------ */

/*
 * Renderer vertex of a primitive corner: the 16 bit screen position scaled to the screen (x by 1/512 with
 * the half width flag, else 1/640; y by 1/480), or with the float flag (0x20 of the primitive's flag word)
 * three float words at `floatOff`. The vertex's pad word is never written by the original.
 */
static void fillVertex(ZrwDbgVertex *v, const uint8_t *prim, uint32_t flags, unsigned shortOff,
                       unsigned floatOff, const uint8_t *rgb, uint8_t alpha)
{
    memset(v, 0, sizeof *v);
    if ((flags & 0x20) != 0) {
        memcpy(&v->x, prim + floatOff, 4);
        memcpy(&v->y, prim + floatOff + 4, 4);
        memcpy(&v->z, prim + floatOff + 8, 4);
    } else {
        const double kx = (double)gNgifScreenW * ((flags & 2) != 0 ? 0.001953125 : 0.0015625);
        const double ky = (double)gNgifScreenH * 0.0020833333333333333;

        v->x = (float)((double)(int16_t)rd16(prim, shortOff) * kx);
        v->y = (float)((double)(int16_t)rd16(prim, shortOff + 2) * ky);
    }
    v->color[0] = rgb[0];
    v->color[1] = rgb[1];
    v->color[2] = rgb[2];
    v->color[3] = alpha;
}

/* Renderer flags of a primitive: `base` plus the blend mode bits (and the alpha byte they imply). */
static uint32_t blendFlags(uint32_t base, int blend, uint8_t mode, uint8_t *alpha)
{
    if (blend == 0) {
        return base | 0x1200;
    }
    switch (mode) {
    case 0:
        *alpha = 0x80;
        return base | 2 | 0x6502;
    case 2:
        *alpha = 0x80;
        return base | 2 | 0x4102;
    default:
        *alpha = 0xff;
        return base | 2 | 0x2202;
    }
}

/*
 * Flat triangle (code 0x20). Without blending the alpha byte of the vertices is the low byte of the
 * primitive's address (the original leaves its argument slot in place of the value).
 */
BMX_ADDR(0x00461700)
void ngifDrawPolyF3(const uint8_t *prim, int blend)
{
    const uint32_t pf = rd32(prim, 0x18);
    uint8_t alpha = (uint8_t)(uintptr_t)prim;
    const uint32_t flags = blendFlags((pf & 0x20) != 0 ? 0u : 0x2000002u, blend, prim[0x1c], &alpha);
    ZrwDbgVertex v0, v1, v2;
    ZrwDbgPoly3 poly = {0, {&v0, &v1, &v2}};

    fillVertex(&v0, prim, pf, 8, 0x40, prim + 4, alpha);
    fillVertex(&v1, prim, pf, 0xc, 0x50, prim + 4, alpha);
    fillVertex(&v2, prim, pf, 0x10, 0x60, prim + 4, alpha);
    gZrwDrawTriangle(flags, &poly, 1, NULL, 0);
}

/*
 * Gouraud quad (code 0x38) with a colour per corner. Alpha is 0xff without blending, else by the blend
 * mode, or the primitive's own (flag 4, bits 7..14).
 */
BMX_ADDR(0x00460640)
void ngifDrawPolyG4(const uint8_t *prim, int blend)
{
    const uint32_t pf = rd32(prim, 0x28);
    uint32_t base = (pf & 0x20) != 0 ? 0u : 0x2000000u;
    uint8_t alpha = 0xff;
    uint32_t flags;
    ZrwDbgVertex v0, v1, v2, v3;
    NgifPoly4 poly = {0, {&v0, &v1, &v2, &v3}};

    if ((pf & 8) == 0) {
        base |= 4;
    }
    if (blend == 0) {
        flags = base | 0x120a;
    } else {
        flags = blendFlags(base, blend, prim[0x2c], &alpha);
        if ((pf & 4) != 0) {
            alpha = (uint8_t)(pf >> 7);
        }
    }
    fillVertex(&v0, prim, pf, 0x10, 0x60, prim + 0xc, alpha);
    fillVertex(&v1, prim, pf, 8, 0x50, prim + 4, alpha);
    fillVertex(&v2, prim, pf, 0x18, 0x70, prim + 0x14, alpha);
    fillVertex(&v3, prim, pf, 0x20, 0x80, prim + 0x1c, alpha);
    gNgifDrawQuads(flags, &poly, 1, NULL, 0);
}

/* Flat line (code 0x40), alpha 0x80. */
BMX_ADDR(0x00461af0)
void ngifDrawLineF2(const uint8_t *prim, int blend)
{
    const uint32_t pf = rd32(prim, 0x14);
    uint8_t ignored = 0;
    const uint32_t flags = blendFlags((pf & 0x20) != 0 ? 0u : 0x2000002u, blend, prim[0x18], &ignored);
    ZrwDbgVertex v0, v1;
    ZrwDbgPoly2 poly = {0, {&v0, &v1}};

    fillVertex(&v0, prim, pf, 0xc, 0x4c, prim + 4, 0x80);
    fillVertex(&v1, prim, pf, 8, 0x3c, prim + 4, 0x80);
    gZrwDrawPolys(flags, &poly, 1, NULL, 0);
}

/* Gouraud line (code 0x50): a colour per end, alpha 0x80. */
BMX_ADDR(0x00461da0)
void ngifDrawLineG2(const uint8_t *prim, int blend)
{
    const uint32_t pf = rd32(prim, 0x18);
    uint8_t ignored = 0;
    const uint32_t flags = blendFlags((pf & 0x20) != 0 ? 0u : 0x2000002u, blend, prim[0x1c], &ignored);
    ZrwDbgVertex v0, v1;
    ZrwDbgPoly2 poly = {0, {&v0, &v1}};

    fillVertex(&v0, prim, pf, 0x10, 0x50, prim + 4, 0x80);
    fillVertex(&v1, prim, pf, 8, 0x40, prim + 0xc, 0x80);
    gZrwDrawPolys(flags, &poly, 1, NULL, 0);
}

/* ---- primitive dispatch ---------------------------------------------------------------------------- */

BMX_ADDR(0x00462050)
void ngifDrawPrimitive(const uint8_t *prim)
{
    const uint8_t code = prim[7];
    const int blend = code & 2;
    const char *what;
    int line;

    switch (code & 0xfd) {
    case 0x20:
        ngifDrawPolyF3(prim, blend);
        return;
    case 0x28:
        ngifDrawPolyF4(prim, blend);
        return;
    case 0x2c:
        ngifDrawPolyFT4(prim, blend);
        return;
    case 0x38:
        ngifDrawPolyG4(prim, blend);
        return;
    case 0x3c:
        ngifDrawPolyGT4(prim, blend);
        return;
    case 0x40:
        ngifDrawLineF2(prim, blend);
        return;
    case 0x50:
        ngifDrawLineG2(prim, blend);
        return;
    case 0xf8:
        ngifDrawAreaPrim(prim);
        return;
    case 0x24:
        what = "DrawPrim:  Prim not coded FT3";
        line = 0x505;
        break;
    case 0x30:
        what = "DrawPrim:  Prim not coded G3";
        line = 0x50b;
        break;
    case 0x34:
        what = "DrawPrim:  Prim not coded GT3";
        line = 0x513;
        break;
    case 0x74:
        what = "DrawPrim:  Prim not coded SPRT8";
        line = 0x530;
        break;
    case 0x7c:
        what = "DrawPrim:  Prim not coded SPRT16";
        line = 0x538;
        break;
    case 0x64:
        what = "DrawPrim:  Prim not coded SPRT";
        line = 0x540;
        break;
    case 0x48:
        what = "DrawPrim:  Prim not coded LINEF3";
        line = 0x55f;
        break;
    case 0x58:
        what = "DrawPrim:  Prim not coded LINEG3";
        line = 0x567;
        break;
    case 0x4c:
        what = "DrawPrim:  Prim not coded LINEF4";
        line = 0x56f;
        break;
    case 0x5c:
        what = "DrawPrim:  Prim not coded LINEG4";
        line = 0x577;
        break;
    default:
        return;
    }
    gZdebugPrint("%s %d %s\n", kNgifFile, line, what);
}

/* Draws the primitives of an ordering table chain, starting after `entry`, until the end marker. */
BMX_ADDR(0x00462350)
void ngifDrawList(uint32_t *entry)
{
    uint32_t word = *entry;

    while ((word & 0x0fffffffu) != 0x0fffffffu) {
        entry = (uint32_t *)(uintptr_t)(*entry & 0x0fffffffu);
        if ((*entry & 0xf0000000u) != 0) {
            ngifDrawPrimitive((const uint8_t *)entry);
        }
        word = *entry;
    }
}

/* ---- 3D line and frame control -------------------------------------------------------------------- */

BMX_ADDR(0x00463710)
void ngifDrawLine3D(int flags, const NgifVert3 *a, const NgifVert3 *b)
{
    uint8_t *const p = (uint8_t *)gNgifPrim3D;
    uint32_t *const ot = gNgifOtBase;
    uint32_t tag;

    put32(p, 4, rd32(a, 0x10));
    put32(p, 0xc, rd32(b, 0x10));
    tag = (get32(p, 0) & 0x0fffffffu) | 0x40000000u;
    put32(p, 0, tag);
    put8(p, 7, 0x50);
    put8(p, 0x1c, (uint32_t)((flags & 3) - 1));
    put32(p, 0x18, get32(p, 0x18) | 0x30);
    put32(p, 0x40, rd32(a, 0));
    put32(p, 0x44, rd32(a, 4));
    put32(p, 0x48, rd32(a, 8));
    put32(p, 0x50, rd32(b, 0));
    put32(p, 0x54, rd32(b, 4));
    put32(p, 0x58, rd32(b, 8));
    if ((flags & 3) != 0) {
        put8(p, 7, 0x52);
    }
    put32(p, 0, withLink(tag, *ot));
    *ot = withLink(*ot, (uint32_t)(uintptr_t)p);
    gNgifPrim3D += 0x20;
}

BMX_ADDR(0x00463800)
void ngifNop2(void) {}

BMX_ADDR(0x00463820)
void ngifResetOrderTable(void)
{
    ngifInitOrderTable(gNgifOtBase, 0x400);
    gNgifFlushPending = 1;
    gNgifPrim3D = gNgifPrim3DBuffer;
}

BMX_ADDR(0x00463860)
int ngifFlushFrame(void)
{
    ngifQueryScreen();
    if (gNgifFlushPending != 0) {
        ngifDrawList(gNgifOtBase + 0x3ff);
        gNgifFlushPending = 0;
    }
    return 1;
}

/* Reads the screen size (queries 10 and 11) into the floats the primitives are scaled by. */
BMX_ADDR(0x004638b0)
void ngifQueryScreen(void)
{
    gNgifQuery(10, &gNgifScreenW, 0);
    gNgifQuery(0xb, &gNgifScreenH, 0);
}

BMX_ADDR(0x004638f0)
void ngifRegisterFlush(void)
{
    if (gNgifFlushNode == 0) {
        gNgifFlushNode = gNgifRegister(2, BMX_FNPTR(NgifUpdateFn, 0x00402207, ngifFlushFrame), NULL, 1);
    }
}

BMX_ADDR(0x00463930)
void ngifUnregisterFlush(void)
{
    if (gNgifFlushNode != 0) {
        gNgifUnregister(gNgifFlushNode);
    }
    gNgifFlushNode = 0;
}

BMX_ADDR(0x00463960)
uint32_t *ngifGetOrderTable(void)
{
    return gNgifOrderTable;
}

BMX_ADDR(0x00463980)
void ngifSetOrderTable(uint32_t *table)
{
    gNgifOrderTable = table;
}

BMX_ADDR(0x004639a0)
int ngifFlushFrame2(void)
{
    ngifQueryScreen();
    if (gNgifFlushPending2 != 0) {
        ngifDrawList(gNgifOtBase + 0x3ff);
        gNgifFlushPending2 = 0;
    }
    return 1;
}

BMX_ADDR(0x004639f0)
int ngifInit(void)
{
    if (gNgifPrimStart == NULL) {
        gNgifPrimStart = gNgifPrimBuffer;
    }
    gNgifPrim = NULL;
    if (gNgifFlushNode2 == 0) {
        gNgifFlushNode2 = gNgifRegister(2, BMX_FNPTR(NgifUpdateFn, 0x004020f4, ngifFlushFrame2), NULL, 1);
    }
    return 0;
}

BMX_ADDR(0x00463a60)
void ngifShutdown(void)
{
    if (gNgifPrimStart != NULL) {
        gNgifPrimStart = NULL;
    }
    gNgifPrim = NULL;
    if (gNgifFlushNode2 != 0) {
        gNgifUnregister(gNgifFlushNode2);
    }
    gNgifFlushNode2 = 0;
}

/* Starts a frame: the primitive cursor goes to the start of the buffer, the table is cleared. */
BMX_ADDR(0x00463ab0)
void ngifBegin(void)
{
    gNgifPrim = gNgifPrimStart;
    gNgifOrderTable = gNgifOtBase + 0x3ff;
    ngifInitOrderTable(gNgifOtBase, 0x400);
    gNgifPrim3D = gNgifPrim3DBuffer;
}

BMX_ADDR(0x00463b00)
void ngifRequestFlush2(void)
{
    gNgifFlushPending2 = 1;
}

/* Full screen textured quad of a picture (texture handle at +0x20) over 512 x 480. */
BMX_ADDR(0x00463b20)
void ngifSetPictureScreen(const void *picture, int w, int h)
{
    uint8_t *const p = (uint8_t *)gNgifPrim;
    const uint32_t tag = (get32(p, 0) & 0x0fffffffu) | 0x90000000u;
    const uint32_t flags = (get32(p, 0x34) & 0xffffff82u) | 0x7f82u;

    put8(p, 4, 0xff);
    put8(p, 5, 0xff);
    put8(p, 6, 0xff);
    put16(p, 0xa, 0xffe0);
    put16(p, 0x12, 0x200);
    put16(p, 0x14, 0xffe0);
    put16(p, 0x16, 0x200);
    put16(p, 0x22, 0x200);
    put16(p, 0x26, 0x200);
    put16(p, 0x2a, 0x200);
    put16(p, 0x2c, 0x200);
    put32(p, 0, tag);
    put8(p, 7, 0x2c);
    put32(p, 0x34, flags);
    put16(p, 8, 0);
    put16(p, 0xc, 0);
    put16(p, 0xe, 0);
    put16(p, 0x18, 0);
    put16(p, 0x1c, 0);
    put16(p, 0x1e, 0x1e0);
    put16(p, 0x20, 0);
    put16(p, 0x28, 0x1e0);
    put32(p, 0x30, rd32(picture, 0x20));
    put32(p, 0x34, flags & 0xfffffff7u);
    linkPrim(p, tag, 0x27);
}

BMX_ADDR(0x00463c40)
void ngifSetBlend(int a, int b)
{
    gNgifColor = (uint32_t)a;
    gNgifBrightness = (uint32_t)b;
}

/*
 * Textured quad of one glyph. The texture of a palette is built on first use from the font's pixel data and
 * the palette's colours (the font keeps a table of 32 handles, allocated with the first glyph).
 */
BMX_ADDR(0x00463c70)
void ngifDrawGlyph(const void *fontData, const void *glyphData, int mode, int a, int b, int x, int y, int c,
                   int d)
{
    uint8_t *const font = (uint8_t *)(uintptr_t)fontData;
    const uint8_t *const glyph = glyphData;
    uint8_t *const p = (uint8_t *)gNgifPrim;
    uint8_t clut[0x1004];
    uint32_t *cache;
    uint32_t tag, flags, bright;
    uint32_t y68;
    const uint32_t index = (uint32_t)d & 0xffff;

    if (glyph[2] == 0 && glyph[3] == 0) {
        return;
    }
    tag = (get32(p, 0) & 0x0fffffffu) | 0x90000000u;
    flags = (get32(p, 0x34) & 0xffffff82u) | 0x7f82u;
    put32(p, 0, tag);
    put8(p, 7, 0x2c);
    put32(p, 0x34, flags);
    y68 = (uint32_t)y + 0x68;
    if (gNgifColor != 0) {
        put8(p, 4, gNgifColor);
        put8(p, 5, gNgifColor >> 8);
        put8(p, 6, gNgifColor >> 16);
    } else {
        put8(p, 4, 0xff);
        put8(p, 5, 0xff);
        put8(p, 6, 0xff);
    }
    put16(p, 8, (uint32_t)x);
    put16(p, 0xa, y68);
    put16(p, 0xc, glyph[0]);
    put16(p, 0xe, glyph[1]);
    put16(p, 0x14, y68);
    put16(p, 0x12, (uint32_t)glyph[2] + (uint32_t)x);
    put16(p, 0x16, (uint32_t)glyph[2] + glyph[0]);
    put16(p, 0x18, glyph[1]);
    put16(p, 0x1c, (uint32_t)x);
    put16(p, 0x1e, glyph[3] + y68);
    put16(p, 0x20, glyph[0]);
    put16(p, 0x22, (uint32_t)glyph[1] + glyph[3]);
    put16(p, 0x26, (uint32_t)glyph[2] + (uint32_t)x);
    put16(p, 0x28, glyph[3] + y68);
    put16(p, 0x2a, (uint32_t)glyph[2] + glyph[0]);
    put16(p, 0x2c, (uint32_t)glyph[1] + glyph[3]);
    if (rd32(font, 0xc) == 0) {
        cache = zmemFreeListAlloc(gNgifGlyphPages);
        memcpy(font + 0xc, (const void *)&cache, sizeof cache);
        for (uint32_t i = 0; i < 0x80; i += 4) {
            *(uint32_t *)((uint8_t *)cache + i) = 0;
        }
    }
    cache = (uint32_t *)(uintptr_t)rd32(font, 0xc);
    if (cache[index] == 0) {
        const uint8_t *const palettes = font + rd32(font, 0x10);
        const uint32_t bytes = (uint32_t)rd16(palettes, 2) * 2 + 4;

        if (bytes >= 0x1000) {
            zdebugFail("Buffer overflow in psxprimDrawFontChar()", kNgifFile, 0xaea, 0);
        }
        if ((uint16_t)d >= 0x10) {
            zdebugFail("Invalid palette index.", kNgifFile, 0xaef, 0);
        }
        memcpy(clut, palettes, bytes);
        for (uint32_t i = 0; i < 0x20; i += 2) {
            clut[4 + i] = clut[4 + index * 0x20 + i];
            clut[5 + i] = clut[5 + index * 0x20 + i];
        }
        cache[index] = (uint32_t)zprimCreateTexture(rd16(font, 8), rd16(font, 0xa), rd16(font, 0x14),
                                                    font + rd32(font, 4), (const uint16_t *)clut);
    }
    put32(p, 0x30, cache[index]);
    flags = get32(p, 0x34) | 4;
    put32(p, 0x34, flags);
    bright = gNgifBrightness != 0 ? gNgifBrightness : 0xff;
    put32(p, 0x34, (((bright & 0xff) << 7) | (flags & 0xffff807fu)) | 8);
    if ((mode & 0xff) == 1) {
        put8(p, 7, p[7] | 2);
        put8(p, 0x38, 0);
    } else {
        put8(p, 7, p[7] & 0xfd);
    }
    put16(p, 0x1a, 0);
    linkPrim(p, get32(p, 0), 0x27);
}
