/* Front-end shared helpers: index wrap, the frame box, the row slide animation. */
#include "bmx/com/menu/frontend.h"

#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"
#include "compat/pktlink.h"

/* Wraps value into 0..count-1 by repeated adding/subtracting (count must be positive). */
BMX_ADDR(0x0043be90)
int menuWrap(int value, int count)
{
    while (value < 0) {
        value += count;
    }
    while (count <= value) {
        value -= count;
    }
    return value;
}

/* Creates the 64x32 sprite the frame box is drawn from and fills it with asset 0x40c4. */
BMX_ADDR(0x0043bec0)
void menuFrameInit(void)
{
    gMenuFrameSprite = zprimCreateSpriteSmall(0x40, 0x20, 4, 0x10);
    zprimCopyTexture(zprimLoadAsset(0x40c4), gMenuFrameSprite);
}

BMX_ADDR(0x0043bf00)
void menuFrameFree(void)
{
    if (gMenuFrameSprite != NULL) {
        zprimFreeSprite(gMenuFrameSprite);
    }
    gMenuFrameSprite = NULL;
}

/* Appends the polygon to the display list: it takes over the ordering table head's link. */
static void menuPolyLink(uint32_t *table, FramePoly *poly)
{
    poly->tag = (poly->tag & 0xf0000000u) | (table[0] & 0x0fffffffu);
    table[0] = (table[0] & 0xf0000000u) | bmxPacketLink(poly);
}

typedef struct FrameCorner {
    int16_t x, y, u, v;
} FrameCorner;

/* Fills one frame piece (code 0x3e, flat grey tint per vertex) and links it into the table. */
static void menuFramePiece(FramePoly *p, uint32_t *table, uint32_t tex, const uint8_t tint[4],
                           const FrameCorner c[4])
{
    p->tag = (p->tag & 0x0fffffffu) | 0xc0000000u;
    p->r0 = p->g0 = p->b0 = tint[0];
    p->code = 0x3e;
    p->r1 = p->g1 = p->b1 = tint[1];
    p->r2 = p->g2 = p->b2 = tint[2];
    p->r3 = p->g3 = p->b3 = tint[3];
    p->unkA = 0x20;
    p->x0 = c[0].x;
    p->y0 = c[0].y;
    p->u0 = c[0].u;
    p->v0 = c[0].v;
    p->x1 = c[1].x;
    p->y1 = c[1].y;
    p->u1 = c[1].u;
    p->v1 = c[1].v;
    p->x2 = c[2].x;
    p->y2 = c[2].y;
    p->u2 = c[2].u;
    p->v2 = c[2].v;
    p->x3 = c[3].x;
    p->y3 = c[3].y;
    p->u3 = c[3].u;
    p->v3 = c[3].v;
    p->texture = tex;
    p->flags = (p->flags & 0xffffff82u) | 0x7f82u;
    menuPolyLink(table, p);
}

/*
 * Draws a frame box around (x0,y0)-(x1,y1) from the 64x32 frame sprite: 5 pieces, or 8 with `full` (the
 * right column). Pieces are 32 wide; without `full` the top and bottom edge pieces fade their right
 * vertices to tint 1. Leaves the cursor after the pieces and moves the ordering table pointer down one.
 */
BMX_ADDR(0x0043bf30)
void menuDrawFrame(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color, int full)
{
    const ZPrimSet *sprite = zprimAssetData(gMenuFrameSprite);
    const uint32_t tex = (uint32_t)sprite->frames[0].texture; /* a picture: frame 0's texture handle */
    FramePoly *p = (FramePoly *)gPrimCursor;
    uint32_t *table = ngifGetOrderTable();

    const int16_t top = (int16_t)(y0 - 0x10);
    const int16_t bottom = (int16_t)(y0 + 0x10);
    const int16_t left2 = (int16_t)(x0 + 0x20);
    const int16_t right2 = (int16_t)(x1 - 0x20);
    const int16_t low = (int16_t)(y1 - 0x10);
    const int16_t low2 = (int16_t)(y1 - 0x30);
    const uint8_t flat[4] = {color, color, color, color};
    const uint8_t fade[4] = {color, 1, color, 1};

    const FrameCorner topLeft[4] = {
        {x0, top, 0, 0}, {left2, top, 0x20, 0}, {x0, bottom, 0, 0x20}, {left2, bottom, 0x20, 0x20}};
    menuFramePiece(p++, table, tex, flat, topLeft);
    const FrameCorner topEdge[4] = {{left2, top, 0x10, 0},
                                    {right2, top, 0x20, 0},
                                    {left2, bottom, 0x20, 0x20},
                                    {right2, bottom, 0x20, 0x20}};
    menuFramePiece(p++, table, tex, full ? flat : fade, topEdge);
    if (full) {
        const FrameCorner topRight[4] = {
            {right2, bottom, 0x20, 0x20}, {right2, top, 0, 0x20}, {x1, bottom, 0x20, 0}, {x1, top, 0, 0}};
        menuFramePiece(p++, table, tex, flat, topRight);
        const FrameCorner rightEdge[4] = {{x1, bottom, 0x10, 0},
                                          {x1, low2, 0x20, 0},
                                          {right2, bottom, 0x1f, 0x20},
                                          {right2, low2, 0x1f, 0x20}};
        menuFramePiece(p++, table, tex, flat, rightEdge);
        const FrameCorner bottomRight[4] = {
            {x1, low, 0, 0}, {right2, low, 0x20, 0}, {x1, low2, 0, 0x1f}, {right2, low2, 0x20, 0x1f}};
        menuFramePiece(p++, table, tex, flat, bottomRight);
    }
    const FrameCorner leftEdge[4] = {
        {x0, low2, 0x10, 0}, {x0, bottom, 0x20, 0}, {left2, low2, 0x20, 0x20}, {left2, bottom, 0x20, 0x20}};
    menuFramePiece(p++, table, tex, flat, leftEdge);
    const FrameCorner bottomLeft[4] = {
        {x0, low, 0, 0}, {x0, low2, 0x20, 0}, {left2, low, 0, 0x20}, {left2, low2, 0x20, 0x20}};
    menuFramePiece(p++, table, tex, flat, bottomLeft);
    const FrameCorner bottomEdge[4] = {
        {left2, low2, 0x20, 0x20}, {right2, low2, 0x10, 0x20}, {left2, low, 0x10, 0}, {right2, low, 0x20, 0}};
    menuFramePiece(p++, table, tex, full ? flat : fade, bottomEdge);

    ngifSetOrderTable(table - 1);
    gPrimCursor = (uint8_t *)p;
    zprimAssetRelease(gMenuFrameSprite);
}

/* ---- row slide animation: four rows share one slide state ------------------------------------- */

static void menuSlideApply(uint32_t id0, uint32_t id1, uint32_t id2, uint32_t id3)
{
    guiSetSpriteOffset(guiFindById(id0), (int16_t)gMenuSlide0, 0);
    guiSetSpriteOffset(guiFindById(id1), (int16_t)gMenuSlide1, 0);
    guiSetSpriteOffset(guiFindById(id2), (int16_t)gMenuSlide2, 0);
    guiSetSpriteOffset(guiFindById(id3), (int16_t)gMenuSlide3, 0);
}

/* Moves the four rows out to the left one after the other; returns 1 once the last one is gone. */
BMX_ADDR(0x0043c8d0)
int menuSlideOut(uint32_t id0, uint32_t id1, uint32_t id2, uint32_t id3)
{
    gMenuSlide0 += (0x2f - gMenuSlide0) / -3;
    if (gMenuSlide0 < 0) {
        gMenuSlide1 += (0x2f - gMenuSlide1) / -3;
    }
    if (gMenuSlide1 < 0) {
        gMenuSlide2 += (0x2f - gMenuSlide2) / -3;
    }
    if (gMenuSlide2 < 0) {
        gMenuSlide3 += (0x2f - gMenuSlide3) / -3;
    }
    if (gMenuSlide0 < -0x48) {
        gMenuSlide0 = -0x48;
    }
    if (gMenuSlide1 < -0x48) {
        gMenuSlide1 = -0x48;
    }
    if (gMenuSlide2 < -0x48) {
        gMenuSlide2 = -0x48;
    }
    if (gMenuSlide3 < -0x48) {
        gMenuSlide3 = -0x48;
    }
    menuSlideApply(id0, id1, id2, id3);
    return gMenuSlide3 <= -0x48;
}

/* Parks all four rows far to the left, ready for menuSlideIn. */
BMX_ADDR(0x0043cab0)
void menuSlideReset(void)
{
    gMenuSlide0 = -0x48;
    gMenuSlide1 = -0x48;
    gMenuSlide2 = -0x48;
    gMenuSlide3 = -0x48;
}

/* Moves the four rows in from the left one after the other (and brightens them); 1 once the last arrived. */
BMX_ADDR(0x0043cae0)
int menuSlideIn(uint32_t id0, uint32_t id1, uint32_t id2, uint32_t id3)
{
    gMenuSlide0 += (0x2f - gMenuSlide0) / 7;
    if (gMenuSlide0 > 0) {
        gMenuSlide1 += (0x2f - gMenuSlide1) / 7;
    }
    if (gMenuSlide1 > 0) {
        gMenuSlide2 += (0x2f - gMenuSlide2) / 7;
    }
    if (gMenuSlide2 > 0) {
        gMenuSlide3 += (0x2f - gMenuSlide3) / 7;
    }
    if (gMenuSlide0 > 0x28) {
        gMenuSlide0 = 0x28;
    }
    if (gMenuSlide1 > 0x28) {
        gMenuSlide1 = 0x28;
    }
    if (gMenuSlide2 > 0x28) {
        gMenuSlide2 = 0x28;
    }
    if (gMenuSlide3 > 0x28) {
        gMenuSlide3 = 0x28;
    }
    menuSlideApply(id0, id1, id2, id3);
    guiSetTint(guiFindById(id0), 0xff, 0xff, 0xff, 0x50);
    guiSetTint(guiFindById(id1), 0xff, 0xff, 0xff, 0x50);
    guiSetTint(guiFindById(id2), 0xff, 0xff, 0xff, 0x50);
    guiSetTint(guiFindById(id3), 0xff, 0xff, 0xff, 0x50);
    return gMenuSlide3 > 0x27;
}
