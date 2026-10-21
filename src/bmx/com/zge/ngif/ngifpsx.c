/* Display list builder (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFpsx.c): primitive writers. */
#include "bmx/com/zge/ngif/ngifpsx.h"

#include "compat/crt.h"

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
