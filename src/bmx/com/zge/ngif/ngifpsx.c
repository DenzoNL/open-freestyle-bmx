/* Display list builder (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFpsx.c): primitive writers. */
#include "bmx/com/zge/ngif/ngifpsx.h"

#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"
#include "zge/com/zglrw/zrwdbg.h"
#include "zge/com/zglrw/zrwmat.h"
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
typedef void *(BMX_CDECL *NgifGetTextureFn)(uint32_t material, int index);
typedef int(BMX_CDECL *NgifTextureSizeFn)(void *texture);
#define gNgifGetTexture    ORIG_VAR(0x00561034, NgifGetTextureFn, gNgifGetTexture)
#define gNgifTextureWidth  ORIG_VAR(0x00562404, NgifTextureSizeFn, gNgifTextureWidth)
#define gNgifTextureHeight ORIG_VAR(0x00562408, NgifTextureSizeFn, gNgifTextureHeight)

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

/*
 * Texture coordinates of a corner: with flag 1 two float words at `floatOff`, else the 16 bit texel
 * position at `texelOff` biased by 0.01 (u) and by `vBias` (v), over the texture size. The float words go
 * through an x87 FLD / FSTP pair, which keeps their bits in the emulated original (a signalling NaN is not
 * quietened there), so they are copied.
 */
static void setTexel(ZrwDbgVertex *v, const uint8_t *prim, uint32_t flags, unsigned texelOff,
                     unsigned floatOff, float width, double height, double vBias)
{
    if ((flags & 1) != 0) {
        v->u = rd32(prim, floatOff);
        v->v = rd32(prim, floatOff + 4);
    } else {
        const float u = (float)(((double)(int16_t)rd16(prim, texelOff) + 0.01) / width);
        const float tv = (float)(((double)(int16_t)rd16(prim, texelOff + 2) + vBias) / height);

        memcpy(&v->u, &u, 4);
        memcpy(&v->v, &tv, 4);
    }
}

/* Size of texture 0 of a material, as the 16 bit values the original keeps. */
static void textureSize(uint32_t material, float *width, double *height)
{
    *width = (float)(int16_t)gNgifTextureWidth(gNgifGetTexture(material, 0));
    *height = (double)(int16_t)gNgifTextureHeight(gNgifGetTexture(material, 0));
}

/* Base renderer flags of the quads: 0x2000000 unless the float flag 0x20 is set, 4 unless flag 8 is set. */
static uint32_t quadFlags(uint32_t pf, uint32_t base)
{
    return (pf & 8) == 0 ? base | 4 : base;
}

/*
 * Textured Gouraud quad (code 0x3c): a colour per corner and the material word at +0x38. Alpha is 0xff
 * without blending, else by the blend mode in bits 5..6 of byte 0xa, or the primitive's own (flag 4, bits
 * 7..14). The v coordinate of the last two corners is biased by -0.01 instead of +0.01.
 */
BMX_ADDR(0x0045ffd0)
void ngifDrawPolyGT4(const uint8_t *prim, int blend)
{
    const uint32_t material = rd32(prim, 0x38);
    const uint32_t pf = rd32(prim, 0x3c);
    const uint32_t base = quadFlags(pf, (pf & 0x20) != 0 ? 0u : 0x2000000u);
    uint8_t alpha = 0xff;
    uint32_t flags;
    float width;
    double height;
    ZrwDbgVertex v0, v1, v2, v3;
    NgifPoly4 poly = {material, {&v0, &v1, &v2, &v3}};

    textureSize(material, &width, &height);
    if (blend == 0) {
        flags = base | 0x120a;
    } else {
        flags = blendFlags(base, blend, (prim[0xa] >> 5) & 3, &alpha);
        if ((pf & 4) != 0) {
            alpha = (uint8_t)(pf >> 7);
        }
    }
    fillVertex(&v0, prim, pf, 0x18, 0x74, prim + 0x14, alpha);
    setTexel(&v0, prim, pf, 0x1c, 0x4c, width, height, 0.01);
    fillVertex(&v1, prim, pf, 0xc, 0x64, prim + 4, alpha);
    setTexel(&v1, prim, pf, 0x10, 0x44, width, height, 0.01);
    fillVertex(&v2, prim, pf, 0x24, 0x84, prim + 0x20, alpha);
    setTexel(&v2, prim, pf, 0x28, 0x54, width, height, -0.01);
    fillVertex(&v3, prim, pf, 0x30, 0x94, prim + 0x2c, alpha);
    setTexel(&v3, prim, pf, 0x34, 0x5c, width, height, -0.01);
    gNgifDrawQuads(flags, &poly, 1, NULL, 0);
}

/*
 * Flags of the flat quads (F4, FT4): without the 0x40 flag the base is 0x2000001 whatever the float flag
 * says; without blending only 8 is added (no 0x1200 as in the other kinds). The blend mode is the whole
 * byte at `modeOff` (values above 3 select the default 0x2202).
 */
static uint32_t flatQuadFlags(const uint8_t *prim, uint32_t pf, int blend, unsigned modeOff, uint8_t *alpha)
{
    uint32_t base = (pf & 0x20) != 0 ? 0u : 0x2000000u;
    uint32_t flags;

    if ((pf & 0x40) == 0) {
        base = 0x2000001u;
    }
    base = quadFlags(pf, base);
    *alpha = 0xff;
    if (blend == 0) {
        return base | 8;
    }
    flags = blendFlags(base, blend, prim[modeOff], alpha);
    if ((pf & 4) != 0) {
        *alpha = (uint8_t)(pf >> 7);
    }
    return flags;
}

/* Textured flat quad (code 0x2c): material word at +0x30; nothing is drawn without one. */
BMX_ADDR(0x00460b40)
void ngifDrawPolyFT4(const uint8_t *prim, int blend)
{
    const uint32_t material = rd32(prim, 0x30);
    uint32_t pf;
    uint32_t flags;
    uint8_t alpha;
    float width;
    double height;
    ZrwDbgVertex v0, v1, v2, v3;
    NgifPoly4 poly = {material, {&v0, &v1, &v2, &v3}};

    if (material == 0) {
        return;
    }
    textureSize(material, &width, &height);
    pf = rd32(prim, 0x34);
    flags = flatQuadFlags(prim, pf, blend, 0x38, &alpha);
    fillVertex(&v0, prim, pf, 0x12, 0x6c, prim + 4, alpha);
    setTexel(&v0, prim, pf, 0x16, 0x44, width, height, 0.01);
    fillVertex(&v1, prim, pf, 8, 0x5c, prim + 4, alpha);
    setTexel(&v1, prim, pf, 0xc, 0x3c, width, height, 0.01);
    fillVertex(&v2, prim, pf, 0x1c, 0x7c, prim + 4, alpha);
    setTexel(&v2, prim, pf, 0x20, 0x4c, width, height, -0.01);
    fillVertex(&v3, prim, pf, 0x26, 0x8c, prim + 4, alpha);
    setTexel(&v3, prim, pf, 0x2a, 0x54, width, height, -0.01);
    gNgifDrawQuads(flags, &poly, 1, NULL, 0);
}

/* Flat quad (code 0x28): one colour, no texture (texture word and UVs zero). */
BMX_ADDR(0x004611d0)
void ngifDrawPolyF4(const uint8_t *prim, int blend)
{
    const uint32_t pf = rd32(prim, 0x1c);
    uint8_t alpha;
    const uint32_t flags = flatQuadFlags(prim, pf, blend, 0x20, &alpha);
    ZrwDbgVertex v0, v1, v2, v3;
    NgifPoly4 poly = {0, {&v0, &v1, &v2, &v3}};

    fillVertex(&v0, prim, pf, 0xc, 0x54, prim + 4, alpha);
    fillVertex(&v1, prim, pf, 8, 0x44, prim + 4, alpha);
    fillVertex(&v2, prim, pf, 0x10, 0x64, prim + 4, alpha);
    fillVertex(&v3, prim, pf, 0x14, 0x74, prim + 4, alpha);
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

/* ---- 3D list primitives -------------------------------------------------------------------------- */

/* zrwtxr.h clashes with zrwdbg.h; only the size queries are needed. */
struct ZrwTexture;
int zrwTextureGetWidth(const struct ZrwTexture *texture);
int zrwTextureGetHeight(const struct ZrwTexture *texture);

/* Links the primitive at p (header word `tag`) into entry `index` of the ordering table. */
static void linkPrim3D(uint8_t *p, uint32_t tag, uint32_t index)
{
    uint32_t *const ot = gNgifOtBase;

    put32(p, 0, withLink(tag, ot[index]));
    ot[index] = withLink(ot[index], (uint32_t)(uintptr_t)p);
}

/*
 * The semi-transparency follow-up of a 3D primitive: an 8 byte mode packet at p (blend `mode` 1..4,
 * anything else clamps) linked into table entry `index`.
 */
static void putBlendPacket(uint8_t *p, uint32_t blend, uint32_t index)
{
    const uint32_t tag = (get32(p, 0) & 0x0fffffffu) | 0x10000000u;
    int32_t mode = (int32_t)blend - 1;

    put32(p, 0, tag);
    mode = mode < 0 ? 0 : (mode >= 3 ? 3 : mode);
    put32(p, 4, (((uint32_t)mode & 3) | 0xff080030u) << 5);
    linkPrim3D(p, tag, index);
}

/*
 * Draws a screen space rectangle into the 3D list: a flat colour quad, or (flags & 4) a textured one
 * (`color` is then the texture number). The size comes from `info` (flags & 0x200: its first two words,
 * & 0x80: a corner at info[4], info[5]), else from the texture; & 0x100 scales it by info[2], info[3];
 * & 8 and & 0x10 centre it on x and y, & 0x20 and & 0x40 anchor it at the right and the bottom. & 1 and
 * & 2 flip the texture, & 0x400 and & 0x4000 take its window from info, & 0x800 and & 0x1000 take a clut
 * offset and an ordering table entry from info[10] and info[11]. The alpha byte of a flat colour (or
 * & 0x2000) makes it semi-transparent. Original quirk: a flag set without size information asserts.
 */
BMX_ADDR(0x004623a0)
void ngifFillScreenBar(uint32_t color, uint32_t x, int y, uint32_t flags, const void *infop)
{
    uint8_t *p = (uint8_t *)gNgifPrim3D;
    const uint32_t *info = infop;
    const NgifTexture *tex = NULL;
    const int textured = (flags & 4) != 0;
    uint32_t w = 0;
    uint32_t h;
    int32_t xx;
    int32_t yy;
    int32_t xl;
    int32_t x2;
    int32_t y2;
    uint16_t clutShift = 0;
    uint32_t otIndex = 0;

    if (textured) {
        tex = &(&gNgifTextures)[color];
        if (tex->material == 0) {
            return;
        }
    }
    if ((flags & 0x800) != 0) {
        clutShift = (uint16_t)info[10];
    }
    if ((flags & 0x1000) != 0) {
        otIndex = info[11];
    }
    if ((flags & 0x200) != 0) {
        w = info[0];
        h = info[1];
    } else if ((flags & 0x80) != 0) {
        w = info[4] - x;
        h = info[5] - (uint32_t)y;
    } else if (!textured) {
        h = 0;
        zdebugFail("Cannot get size of object", kNgifFile, 0x63f, 0);
    } else if ((flags & 0x400) != 0) {
        w = info[8];
        h = info[9];
    } else {
        w = tex->width;
        h = tex->height;
    }
    if ((flags & 0x100) != 0) {
        w *= info[2];
        h *= info[3];
    }
    xx = (int32_t)x;
    if ((flags & 8) != 0) {
        xx -= (int32_t)w >> 1;
    }
    yy = y;
    if ((flags & 0x10) != 0) {
        yy -= (int32_t)h >> 1;
    }
    xl = xx;
    if ((flags & 0x80) != 0) {
        x2 = (int32_t)info[4];
        y2 = (int32_t)info[5];
    } else {
        x2 = (int32_t)((uint32_t)xx + w);
        y2 = (int32_t)((uint32_t)yy + h);
    }
    if ((flags & 0x20) != 0) {
        xl = (int32_t)((uint32_t)xx - w);
        x2 = (int32_t)((uint32_t)x2 - w);
    }
    if ((flags & 0x40) != 0) {
        yy = (int32_t)((uint32_t)yy - h);
        y2 = (int32_t)((uint32_t)y2 - h);
    }
    if (p == NULL) {
        return;
    }

    if (textured) {
        const uint32_t tag = (get32(p, 0) & 0x0fffffffu) | 0x90000000u;

        if ((flags & 1) != 0) {
            const int32_t t = xl;
            xl = x2;
            x2 = t;
        }
        if ((flags & 2) != 0) {
            const int32_t t = yy;
            yy = y2;
            y2 = t;
        }
        put32(p, 0x34, (get32(p, 0x34) & 0xffffff80u) | 0x7f80u);
        put32(p, 0, tag);
        put8(p, 7, 0x2c);
        put16(p, 0x10, (uint32_t)(uint16_t)(tex->clutWord + clutShift));
        put16(p, 0x1a, tex->pageWord);
        put32(p, 0x30, tex->material);
        put32(p, 4, 0x2c808080u);
        put32(p, 0x1c, get32(p, 0x1c) & 0xffffffcfu);
        if ((flags & 0x2000) != 0) {
            put8(p, 0x20, 1);
            put8(p, 7, p[7] | 2);
        }
        put16(p, 0xa, (uint32_t)yy);
        put16(p, 8, (uint32_t)xl);
        put16(p, 0x1c, (uint32_t)xl);
        put16(p, 0x12, (uint32_t)x2);
        put16(p, 0x14, (uint32_t)yy);
        put16(p, 0x1e, (uint32_t)y2);
        put16(p, 0x26, (uint32_t)x2);
        put16(p, 0x28, (uint32_t)y2);
        if ((flags & 0x400) != 0) {
            put16(p, 0xc, (uint32_t)(tex->u0 + (uint16_t)info[6]));
            put16(p, 0xe, (uint32_t)(tex->v0 + (uint16_t)info[7]));
            put16(p, 0x2a, (uint32_t)((uint16_t)info[8] + rd16(p, 0xc)));
            put16(p, 0x2c, (uint32_t)((uint16_t)info[9] + rd16(p, 0xe)));
        } else if ((flags & 0x4000) != 0) {
            const uint8_t *b = (const uint8_t *)info;

            put16(p, 0xc, (uint32_t)(b[0x30] + tex->u0));
            put16(p, 0xe, (uint32_t)(b[0x31] + tex->v0));
            put16(p, 0x2a, (uint32_t)(b[0x32] + tex->u0));
            put16(p, 0x2c, (uint32_t)(b[0x33] + tex->v0));
        } else {
            put16(p, 0xc, tex->u0);
            put16(p, 0xe, tex->v0);
            put16(p, 0x2a, tex->u1);
            put16(p, 0x2c, tex->v1);
        }
        put16(p, 0x16, rd16(p, 0x2a));
        put16(p, 0x18, rd16(p, 0xe));
        put16(p, 0x20, rd16(p, 0xc));
        put16(p, 0x22, rd16(p, 0x2c));
        linkPrim3D(p, tag, otIndex);
        gNgifPrim3D = (uint32_t *)(p + 0x9c);
        return;
    }

    {
        const uint32_t tag = (get32(p, 0) & 0x0fffffffu) | 0x50000000u;
        const uint32_t alpha = color >> 24;

        put32(p, 4, (color & 0xffffffu) + 0x28000000u);
        put32(p, 0, tag);
        put32(p, 0x1c, (get32(p, 0x1c) & 0xffffff80u) | 0x7f80u);
        if (alpha != 0 || (flags & 0x2000) != 0) {
            put8(p, 7, p[7] | 2);
            put8(p, 0x20, alpha - 1);
        }
        put16(p, 8, (uint32_t)xl);
        put16(p, 0xa, (uint32_t)yy);
        put16(p, 0xc, (uint32_t)x2);
        put16(p, 0xe, (uint32_t)yy);
        put16(p, 0x10, (uint32_t)xl);
        put16(p, 0x12, (uint32_t)y2);
        put16(p, 0x14, (uint32_t)x2);
        put16(p, 0x16, (uint32_t)y2);
        put32(p, 0x18, 0);
        linkPrim3D(p, tag, otIndex);
        p += 0x84;
        if (alpha != 0 || (flags & 0x2000) != 0) {
            putBlendPacket(p, alpha, otIndex);
            p += 8;
        }
        gNgifPrim3D = (uint32_t *)p;
    }
}

/*
 * Draws the glyph records of an overlay text into the 3D list (x is passed zero extended): a record list
 * that ends with kind 0xff. Kinds 0 and 0x11 are flat triangles and quads (colour code in word 1, corners
 * from +4), 0x30..0x32 textured quads of a texture (+6) from the texture table: 0x30 takes the window
 * from the texture, 0x32 also scales it by the floats at +8 and +10 (they stay in force for the records
 * after it), 0x31 takes the corner positions and window from the record. Other kinds assert.
 */
BMX_ADDR(0x00462940)
void ngifDrawTextGlyph(uint32_t x, int y, const void *record)
{
    uint8_t *p = (uint8_t *)gNgifPrim3D;
    const uint8_t *rec = record;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    const uint16_t x16 = (uint16_t)x;
    const uint16_t y16 = (uint16_t)y;

    if (p == (uint8_t *)(uintptr_t)0xffffffffu || rec == NULL) {
        return;
    }
    while (rec != NULL) {
        const uint32_t kind = rec[0];
        const uint16_t cx = (uint16_t)rd16(rec, 2);

        if (kind > 0x32) {
            if (kind == 0xff) {
                return;
            }
            zdebugFail("Can not get size of object", kNgifFile, 0x7bd, 0);
            continue;
        }
        if (kind == 0 || kind == 0x11) {
            const int quad = kind == 0x11;
            uint32_t tag;

            if (quad) {
                tag = (get32(p, 0) & 0x0fffffffu) | 0x50000000u;
                put32(p, 0x1c, (get32(p, 0x1c) & 0xffffff80u) | 0x7f80u);
            } else {
                tag = (get32(p, 0) & 0x0fffffffu) | 0x30000000u;
            }
            put8(p, 4, (uint32_t)(cx >> 7) & 0xe0);
            put32(p, 0, tag);
            put8(p, 7, quad ? 0x28 : 0x40);
            put8(p, 5, (uint32_t)(cx >> 2) & 0xe0);
            put8(p, 6, (uint32_t)(cx & 0xfc) << 3);
            if ((rec[1] & 1) != 0) {
                put8(p, 7, quad ? 0x2a : 0x42);
            }
            put16(p, 8, rd16(rec, 4) + x16);
            put16(p, 0xa, rd16(rec, 6) + y16);
            put16(p, 0xc, rd16(rec, 8) + x16);
            put16(p, 0xe, rd16(rec, 0xa) + y16);
            if (quad) {
                put16(p, 0x10, rd16(rec, 0x10) + x16);
                put16(p, 0x12, rd16(rec, 0x12) + y16);
                put16(p, 0x14, rd16(rec, 0xc) + x16);
                put16(p, 0x16, rd16(rec, 0xe) + y16);
            }
            linkPrim3D(p, tag, 0);
            p += quad ? 0x84 : 0x7c;
            rec += quad ? 0x14 : 0xc;
            gNgifPrim3D = (uint32_t *)p;
            continue;
        }
        if (kind < 0x30) {
            zdebugFail("Can not get size of object", kNgifFile, 0x7bd, 0);
            continue;
        }

        {
            const NgifTexture *tex = &(&gNgifTextures)[rd16(rec, 6)];
            const int16_t dx = (int16_t)cx;
            const int16_t dy = (int16_t)rd16(rec, 4);
            const uint16_t x0 = (uint16_t)(x + cx);
            const uint16_t y0 = (uint16_t)(y + (int16_t)rd16(rec, 4));
            uint16_t x1;
            uint16_t y1;
            uint16_t u0;
            uint16_t v0;
            uint16_t u1;
            uint16_t v1;
            uint16_t wordF;
            uint16_t wordG;
            uint32_t flagBits;
            const uint32_t tag = 0x90000000u;

            if (tex->material == 0) {
                return;
            }
            if (kind == 0x32) {
                scaleX = (float)(int32_t)rd16(rec, 8);
                scaleY = (float)(int32_t)rd16(rec, 0xa);
            }
            memset(p, 0, 0x9c);
            put32(p, 0, tag);
            put8(p, 7, 0x2c);
            put32(p, 0x34, 0x7f80u);
            put16(p, 0x10, 0);
            if (kind == 0x31) {
                x1 = (uint16_t)bmxFtol((double)rd16(rec, 0xc) * (double)scaleX +
                                       (double)(int32_t)((uint32_t)(int32_t)dx + x));
                y1 = (uint16_t)bmxFtol((double)rd16(rec, 0xe) * (double)scaleY +
                                       (double)(int32_t)((uint32_t)(int32_t)dy + (uint32_t)y));
                u0 = (uint16_t)(tex->u0 + rd16(rec, 8));
                v0 = (uint16_t)(tex->v0 + rd16(rec, 0xa));
                u1 = (uint16_t)(rd16(rec, 0xc) + u0);
                v1 = (uint16_t)(rd16(rec, 0xe) + v0);
                wordF = (uint16_t)rd16(rec, 0x10);
                wordG = (uint16_t)rd16(rec, 0x12);
            } else {
                x1 = (uint16_t)bmxFtol((double)tex->width * (double)scaleX +
                                       (double)(int32_t)((uint32_t)(int32_t)dx + x));
                y1 = (uint16_t)bmxFtol((double)tex->height * (double)scaleY +
                                       (double)(int32_t)((uint32_t)(int32_t)dy + (uint32_t)y));
                u0 = tex->u0;
                v0 = tex->v0;
                u1 = tex->u1;
                v1 = tex->v1;
                wordF = (uint16_t)rd16(rec, kind == 0x32 ? 0xc : 8);
                wordG = (uint16_t)rd16(rec, kind == 0x32 ? 0xe : 0xa);
            }
            put16(p, 0x10, wordG);
            put8(p, 4, (uint32_t)(wordF >> 4) & 0xf0);
            put8(p, 5, (uint32_t)wordF & 0xf0);
            put8(p, 6, (uint32_t)wordF << 4);
            flagBits = (wordF >> 12) & 7u;
            if (flagBits == 0) {
                put8(p, 7, 0x2c);
                put16(p, 0x1a, tex->pageWord);
            } else {
                const uint32_t mode = flagBits - 1 > 3 ? 3 : flagBits - 1;

                put8(p, 7, 0x2e);
                put16(p, 0x1a, ((mode & 3) << 5) | (tex->pageWord & 0xff9fu));
                put8(p, 0x38, flagBits - 1);
            }
            put16(p, 0xc, u0);
            put32(p, 0x30, tex->material);
            put16(p, 8, x0);
            put16(p, 0xa, y0);
            put16(p, 0x12, x1);
            put16(p, 0x14, y0);
            put16(p, 0x1c, x0);
            put16(p, 0x1e, y1);
            put16(p, 0x20, u0);
            put16(p, 0x26, x1);
            put16(p, 0xe, v0);
            put16(p, 0x16, u1);
            put16(p, 0x18, v0);
            put16(p, 0x22, v1);
            put16(p, 0x28, y1);
            put16(p, 0x2a, u1);
            put16(p, 0x2c, v1);
            linkPrim3D(p, tag, 0);
            p += 0x9c;
            rec += kind == 0x30 ? 0xc : (kind == 0x32 ? 0x10 : 0x14);
            gNgifPrim3D = (uint32_t *)p;
        }
    }
}

/*
 * Draws a polygon into the 3D list from vertices of 0x14 bytes (position, then rgb at +0x10): `count` 4
 * is a quad, 0..3 a triangle (corners in the order the GPU packet wants them); the low two bits of flags
 * select the blend mode (0 = opaque), and bit 3 must be set (screen space polygons are not coded). A
 * texture number selects a textured quad, whose window is taken from the texture table and the size of the
 * texture of its material. Counts above 4 and an empty list draw nothing.
 */
BMX_ADDR(0x00463130)
void ngifDrawPoly3D(int flags, int count, const NgifVert3 *verts, int texture)
{
    uint8_t *p = (uint8_t *)gNgifPrim3D;
    const uint8_t *v = (const uint8_t *)verts;
    const uint32_t blend = (uint32_t)flags & 3;
    uint32_t tag;
    uint32_t color;

    if ((uint8_t)count > 4 || p == NULL) {
        return;
    }
    if (((uint32_t)flags & 8) == 0) {
        zdebugFail("Uncoded for polygons in screen space", kNgifFile, 0x838, 0);
    }
    color = ((((uint32_t)v[0x12]) << 8) + v[0x11]) << 8;
    color += v[0x10];
    if ((uint16_t)texture != 0) {
        const NgifTexture *tex;
        const struct ZrwTexture *image;

        if ((uint8_t)count != 4) {
            zdebugFail("Not coded for textured tris", kNgifFile, 0x853, 0);
        }
        tex = &(&gNgifTextures)[(uint16_t)texture];
        if (tex->material == 0) {
            return;
        }
        tag = (get32(p, 0) & 0x0fffffffu) | 0x90000000u;
        put32(p, 0, tag);
        put8(p, 7, 0x2c);
        put32(p, 0x34, (get32(p, 0x34) & 0xffffff80u) | 0x7f80u);
        put16(p, 0x10, tex->clutWord);
        put32(p, 4, color + 0x2c000000u);
        if (blend != 0) {
            const uint32_t mode = blend - 1 >= 3 ? 3 : blend - 1;

            put16(p, 0x1a, ((mode & 3) << 5) | (tex->pageWord & 0xff9fu));
            put8(p, 7, p[7] | 2);
            put8(p, 0x38, blend - 1);
        } else {
            put16(p, 0x1a, tex->pageWord);
        }
        put32(p, 0x30, tex->material);
        put8(p, 0x38, blend - 1);
        put32(p, 0x34, get32(p, 0x34) | 0x30);
        put32(p, 0x5c, get32(v, 0));
        put32(p, 0x60, get32(v, 4));
        put32(p, 0x64, get32(v, 8));
        put32(p, 0x6c, get32(v, 0x14));
        put32(p, 0x70, get32(v, 0x18));
        put32(p, 0x74, get32(v, 0x1c));
        put32(p, 0x8c, get32(v, 0x28));
        put32(p, 0x90, get32(v, 0x2c));
        put32(p, 0x94, get32(v, 0x30));
        put32(p, 0x7c, get32(v, 0x3c));
        put32(p, 0x80, get32(v, 0x40));
        put32(p, 0x84, get32(v, 0x44));
        put16(p, 0xc, tex->u0);
        put16(p, 0xe, tex->v0);
        image = zrwmatGetTexture((ZrwMaterial *)(uintptr_t)tex->material, 0);
        put16(p, 0x2a,
              (uint32_t)bmxFtol((double)(int16_t)zrwTextureGetWidth(image) + (double)rd16(p, 0xc) -
                                (double)1.0f));
        image = zrwmatGetTexture((ZrwMaterial *)(uintptr_t)tex->material, 0);
        put16(p, 0x2c,
              (uint32_t)bmxFtol((double)(int16_t)zrwTextureGetHeight(image) + (double)rd16(p, 0xe) -
                                (double)1.0f));
        put16(p, 0x16, rd16(p, 0x2a));
        put16(p, 0x18, rd16(p, 0xe));
        put16(p, 0x20, rd16(p, 0xc));
        put16(p, 0x22, rd16(p, 0x2c));
        linkPrim3D(p, tag, 0);
        gNgifPrim3D = (uint32_t *)(p + 0x9c);
        return;
    }
    if ((uint8_t)count == 4) {
        const uint32_t flagWord = (get32(p, 0x1c) & 0xffffff80u) | 0x7f80u;

        tag = (get32(p, 0) & 0x0fffffffu) | 0x50000000u;
        put32(p, 0, tag);
        put8(p, 7, 0x28);
        put32(p, 0x1c, flagWord);
        put32(p, 4, color + 0x28000000u);
        if (blend != 0) {
            put8(p, 7, p[7] | 2);
            put8(p, 0x20, blend - 1);
        }
        put8(p, 0x20, blend - 1);
        put32(p, 0x1c, flagWord | 0x30);
        put32(p, 0x44, get32(v, 0));
        put32(p, 0x48, get32(v, 4));
        put32(p, 0x4c, get32(v, 8));
        put32(p, 0x54, get32(v, 0x14));
        put32(p, 0x58, get32(v, 0x18));
        put32(p, 0x5c, get32(v, 0x1c));
        put32(p, 0x74, get32(v, 0x28));
        put32(p, 0x78, get32(v, 0x2c));
        put32(p, 0x7c, get32(v, 0x30));
        put32(p, 0x64, get32(v, 0x3c));
        put32(p, 0x68, get32(v, 0x40));
        put32(p, 0x6c, get32(v, 0x44));
        linkPrim3D(p, tag, 0);
        p += 0x84;
    } else {
        const uint32_t flagWord = (get32(p, 0x18) & 0xffffff80u) | 0x7f80u;

        tag = (get32(p, 0) & 0x0fffffffu) | 0x40000000u;
        put32(p, 0, tag);
        put32(p, 0x18, flagWord);
        put8(p, 7, 0x20);
        put32(p, 4, color + 0x20000000u);
        if (blend != 0) {
            put8(p, 7, p[7] | 2);
            put8(p, 0x1c, blend - 1);
        }
        put8(p, 0x1c, blend - 1);
        put32(p, 0x18, get32(p, 0x18) | 0x30);
        put32(p, 0x40, get32(v, 0));
        put32(p, 0x44, get32(v, 4));
        put32(p, 0x48, get32(v, 8));
        put32(p, 0x50, get32(v, 0x14));
        put32(p, 0x54, get32(v, 0x18));
        put32(p, 0x58, get32(v, 0x1c));
        put32(p, 0x60, get32(v, 0x28));
        put32(p, 0x64, get32(v, 0x2c));
        put32(p, 0x68, get32(v, 0x30));
        linkPrim3D(p, tag, 0);
        p += 0x80;
    }
    if (blend != 0) {
        putBlendPacket(p, blend, 0);
        p += 8;
    }
    gNgifPrim3D = (uint32_t *)p;
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
    ZPrimFont *const fontRec = (ZPrimFont *)font;
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
    if (fontRec->pages == 0) {
        cache = zmemFreeListAlloc(gNgifGlyphPages);
        fontRec->pages = bmxSlot32Put(cache);
        for (uint32_t i = 0; i < 32; i++) {
            cache[i] = 0;
        }
    }
    cache = bmxSlot32Get(fontRec->pages);
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
