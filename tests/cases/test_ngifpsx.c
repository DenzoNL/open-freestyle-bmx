/*
 * Tests for the display list primitive writers of NgIFpsx.c (clip, sprite, rectangle, line) and the
 * callbacks of its function table. The writers fill a primitive buffer pre-filled with random bytes, so
 * bytes the original leaves alone must stay alone, and link it into an ordering table.
 */
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "dt.h"
#include "dt_ngif.h"

enum { kPrimBytes = 0x100, kOtWords = 8 };

/* Primitive buffer, ordering table and the colour state with edge values mixed in. */
static void setup_display_list(DtCtx *c)
{
    uint32_t *prim = dt_alloc(kPrimBytes);
    uint32_t *ot = dt_alloc(kOtWords * 4);

    ngBegin(c, 0);
    dt_fill(prim, kPrimBytes);
    dt_fill(ot, kOtWords * 4);
    gNgifPrim = prim;
    gNgifOrderTable = ot + kOtWords - 2;
    gNgifColor = (dt_u32() & 3) ? dt_u32() : 0;
    gNgifBrightness = (dt_u32() & 3) ? dt_u32() : (dt_u32() & 1 ? 0 : 0xff);
    gNgifScreenW = (dt_u32() & 3) ? 640.0f : dt_float_edgy(0.0f, 2000.0f);
    gNgifScreenH = (dt_u32() & 3) ? 480.0f : dt_float_edgy(0.0f, 2000.0f);
}

/* A coordinate with junk in the upper bits (the original only reads 16) and edge values. */
static uint32_t coord(void)
{
    const uint32_t v = dt_u32();

    return (v & 3) == 0 ? (v & 0xffffu) : ((v >> 4) & 0x3ff) | ((v & 4) ? (v & 0xffff0000u) : 0);
}

static void setup_clip(DtCtx *c)
{
    uint32_t x0, y0, x1, y1;

    setup_display_list(c);
    x0 = coord();
    y0 = coord();
    x1 = coord();
    y1 = coord();
    if (dt_u32() % 3 == 0) {
        /* the full-screen rectangle (any junk in the upper halves) */
        const int flip = dt_u32() & 1;

        x1 = flip ? x0 - 0x27f : x0 + 0x27f;
        y1 = (dt_u32() & 1) ? y0 - 0x1df : y0 + 0x1df;
        if (dt_u32() & 1) {
            x1 ^= 0x10000u * (dt_u32() & 0xf);
        }
    }
    if (dt_u32() % 8 == 0) {
        gNgifPrim = NULL;
    }
    DT_ARG_INT(c, 0, x0);
    DT_ARG_INT(c, 1, y0);
    DT_ARG_INT(c, 2, x1);
    DT_ARG_INT(c, 3, y1);
}

static void setup_sprite(DtCtx *c)
{
    const uint32_t frames = (uint32_t)dt_range(1, 12);
    uint8_t *set = dt_alloc((frames + 2) * 0x10);

    setup_display_list(c);
    dt_fill(set, (frames + 2) * 0x10);
    DT_ARG_PTR(c, 0, set);
    DT_ARG_INT(c, 1, (dt_u32() & 0xffff0000u) | (uint32_t)dt_range(0, (int)frames - 1));
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, (dt_u32() & 1) ? (dt_u32() & 0xffffff00u) | (uint32_t)(dt_u32() & 1) : dt_u32());
    DT_ARG_INT(c, 4, dt_u32());
    DT_ARG_INT(c, 5, dt_u32());
    DT_ARG_INT(c, 6, dt_u32());
    DT_ARG_INT(c, 7, coord());
    DT_ARG_INT(c, 8, coord());
    DT_ARG_INT(c, 9, coord());
    DT_ARG_INT(c, 10, coord());
    DT_ARG_INT(c, 11, dt_u32());
    DT_ARG_INT(c, 12, dt_u32());
}

static void setup_rect(DtCtx *c)
{
    setup_display_list(c);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, (dt_u32() & 1) ? 1u + (dt_u32() & 0xffffff00u) : dt_u32());
    DT_ARG_INT(c, 2, coord());
    DT_ARG_INT(c, 3, coord());
    DT_ARG_INT(c, 4, coord());
    DT_ARG_INT(c, 5, coord());
}

static void setup_nop(DtCtx *c)
{
    ngBegin(c, 0);
}

DT_CASES(ngifpsx_cases) = {
    {"ngifSetClip", 0x00464030, ngifSetClip, 4, DT_RET_VOID, setup_clip, 2000, 0},
    {"ngifNop", 0x00464200, ngifNop, 0, DT_RET_VOID, setup_nop, 2, 0},
    {"ngifDrawSprite", 0x00464220, ngifDrawSprite, 13, DT_RET_VOID, setup_sprite, 1000, 0},
    {"ngifFillRect", 0x00464420, ngifFillRect, 6, DT_RET_VOID, setup_rect, 1000, 0},
    {"ngifDrawLine", 0x004645d0, ngifDrawLine, 6, DT_RET_VOID, setup_rect, 1000, 0},
    {"ngifReturnOneA", 0x004646b0, ngifReturnOneA, 0, DT_RET_INT, setup_nop, 2, 0},
    {"ngifReturnOneB", 0x004646d0, ngifReturnOneB, 0, DT_RET_INT, setup_nop, 2, 0},
};
