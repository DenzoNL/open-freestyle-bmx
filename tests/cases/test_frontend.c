/* Tests for frontend.c: index wrap, frame box, row slide animation. */
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_frontend.h"

enum { kRowA = 0xa0140004, kRowB = 0xa0140005, kRowC = 0xa0140006, kRowD = 0xa0140007 };

static void setup_wrap(DtCtx *c)
{
    dt_seed(c);
    const int n = c->seed % 5 == 0 ? dt_range(1, 400) : dt_range(1, 9);
    DT_ARG_INT(c, 0, c->seed % 7 == 0 ? dt_range(-2000, 2000) : dt_range(-30, 30));
    DT_ARG_INT(c, 1, n);
}

static void setup_frame_init(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    dtFrontendStub(DT_FS_FRAME);
    gMenuFrameSprite = (ZPrimAsset *)(uintptr_t)dt_u32();
}

static void setup_frame_free(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    gMenuFrameSprite = c->seed % 3 == 0 ? NULL : (ZPrimAsset *)dt_alloc(0x20);
    dtFrontendStub(DT_FS_FRAME);
}

static void setup_draw_frame(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    uint8_t data[0x50];
    for (uint32_t i = 0; i < sizeof data; i++) {
        data[i] = (uint8_t)dt_u32();
    }
    gMenuFrameSprite = dtGuiMakeAsset(0x40c5, data, sizeof data);
    uint8_t *prims = dt_alloc(10 * sizeof(FramePoly));
    for (uint32_t i = 0; i < 10 * sizeof(FramePoly); i++) {
        prims[i] = (uint8_t)dt_u32();
    }
    gPrimCursor = prims;
    uint32_t *table = dt_alloc(8 * 4);
    for (int i = 0; i < 8; i++) {
        table[i] = dt_u32();
    }
    gNgifOrderTable = table + dt_range(1, 6);
    DT_ARG_INT(c, 0, dt_range(-100, 700));
    DT_ARG_INT(c, 1, dt_range(-100, 500));
    DT_ARG_INT(c, 2, dt_range(-100, 700));
    DT_ARG_INT(c, 3, dt_range(-100, 500));
    DT_ARG_INT(c, 4, c->seed % 4 == 0 ? dt_u32() : (uint32_t)dt_range(0, 255));
    DT_ARG_INT(c, 5, c->seed % 3 == 0 ? 0 : (int)(dt_u32() | 1));
}

static void setup_slide(DtCtx *c)
{
    dt_seed(c);
    dtFrontendReset();
    static const uint32_t ids[4] = {kRowA, kRowB, kRowC, kRowD};
    dtFrontendElements(ids, 4);
    int *slide[4] = {&gMenuSlide0, &gMenuSlide1, &gMenuSlide2, &gMenuSlide3};
    const int mode = dt_range(0, 3);
    for (int i = 0; i < 4; i++) {
        switch (mode) {
        case 0:
            *slide[i] = dt_range(-0x60, 0x40);
            break;
        case 1: /* a staggered slide: the leading rows are done, the later ones waiting */
            *slide[i] = i < 2 ? dt_range(-0x10, 0x30) : -0x48;
            break;
        case 2:
            *slide[i] = dt_range(-0x48, -0x40);
            break;
        default:
            *slide[i] = dt_range(0x20, 0x30);
        }
    }
    for (int i = 0; i < 4; i++) {
        DT_ARG_INT(c, i, ids[i]);
    }
}

static void setup_slide_reset(DtCtx *c)
{
    dt_seed(c);
    gMenuSlide0 = (int)dt_u32();
    gMenuSlide1 = (int)dt_u32();
    gMenuSlide2 = (int)dt_u32();
    gMenuSlide3 = (int)dt_u32();
}

DT_CASES(frontend_cases) = {
    {"menuWrap", 0x0043be90, menuWrap, 2, DT_RET_INT, setup_wrap, 400, 0},
    {"menuFrameInit", 0x0043bec0, menuFrameInit, 0, DT_RET_VOID, setup_frame_init, 20, 0},
    {"menuFrameFree", 0x0043bf00, menuFrameFree, 0, DT_RET_VOID, setup_frame_free, 20, 0},
    {"menuDrawFrame", 0x0043bf30, menuDrawFrame, 6, DT_RET_VOID, setup_draw_frame, 400, 0},
    {"menuSlideOut", 0x0043c8d0, menuSlideOut, 4, DT_RET_INT, setup_slide, 400, 0},
    {"menuSlideReset", 0x0043cab0, menuSlideReset, 0, DT_RET_VOID, setup_slide_reset, 20, 0},
    {"menuSlideIn", 0x0043cae0, menuSlideIn, 4, DT_RET_INT, setup_slide, 400, 0},
};
