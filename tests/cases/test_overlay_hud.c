/* Overlay HUD elements (table entries 0..16): init, draw, show, set and get callbacks. */
#include "bmx/com/action/overlay.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_overlay.h"

/* ---- fade, letterbox ---------------------------------------------------------------------------- */

DTO_SETUP(setup_fill_bar, DTS_FILLBAR)

static void setup_fade_set(DtCtx *c)
{
    dtOvlBegin(c, 0);
    uint8_t *fade = dt_alloc(8);
    for (int i = 0; i < 4; i++) {
        fade[i] = (uint8_t)dt_u32();
    }
    const uint16_t ticks = (uint16_t)(dt_u32() % 5 == 0 ? 0 : dt_range(1, 90));
    fade[4] = (uint8_t)ticks;
    fade[5] = (uint8_t)(ticks >> 8);
    DT_ARG_PTR(c, 0, fade);
}

/* ---- score and timer ---------------------------------------------------------------------------- */

DTO_SETUP_CREATE(setup_score_init, DTS_TEXTWIDTH | DTS_ADDTEXT | DTS_SHOW | DTS_REMOVE)
DTO_SETUP(setup_score_draw, DTS_GETMODE | DTS_GETSUB | DTS_REMOVE | DTS_TEXTWIDTH | DTS_ADDTEXT)
DTO_SETUP_CREATE(setup_score_show, DTS_GETMODE | DTS_GETSUB | DTS_SHOW)

static void setup_score_set(DtCtx *c)
{
    dtOvlBegin(c, DTS_GETMODE | DTS_GETSUB | DTS_SHOW);
    uint32_t *value = dt_alloc(8);
    value[0] = dt_u32() % 4 == 0 ? dt_u32() : (uint32_t)dt_range(0, 3000);
    value[1] = dt_u32();
    DT_ARG_PTR(c, 0, value);
}

DTO_SETUP_CREATE(setup_timer_init, DTS_ADDTEXT | DTS_SHOW | DTS_REMOVE)
/* The text is only rebuilt when it changed, so sometimes make the shown text the current one. */
static void setup_timer_draw(DtCtx *c)
{
    dtOvlBegin(c, DTS_GETMODE | DTS_GETSUB | DTS_REMOVE | DTS_ADDTEXT);
    if (dt_u32() % 3 == 0) {
        uint32_t left = gOvlTimerTicks + 0x1d;
        uint32_t minutes = 0;
        uint32_t seconds = 0;
        if (0x708 < left) {
            minutes = (gOvlTimerTicks + 0x1c) / 0x708;
            left -= minutes * 0x708;
        }
        if (0x1e < left) {
            seconds = (left - 1) / 0x1e;
        }
        gOvlTimerText[0] = (char)('0' + minutes % 10);
        gOvlTimerText[1] = left < 0xf ? ' ' : ':';
        gOvlTimerText[2] = (char)('0' + seconds / 10 % 10);
        gOvlTimerText[3] = (char)('0' + seconds % 10);
        gOvlTimerText[4] = '\0';
    }
}
DTO_SETUP_CREATE(setup_timer_show, DTS_GETMODE | DTS_GETSUB | DTS_SHOW)

static void setup_timer_set(DtCtx *c)
{
    dtOvlBegin(c, 0);
    uint32_t *ticks = dt_alloc(4);
    *ticks = dt_u32();
    DT_ARG_PTR(c, 0, ticks);
}

DTO_SETUP(setup_name_draw, DTS_DRAWTEXT)

/* ---- height and balance meters ------------------------------------------------------------------ */

DTO_SETUP_CREATE(setup_height_init, DTS_ADDRECT | DTS_SHOW | DTS_REMOVE)
/* The scale text is formatted into an 8 byte buffer, so keep the scroll position a small positive height. */
static void setup_height_draw(DtCtx *c)
{
    static const float special[] = {0.0f, 1.0f, 4.0f, 5.0f, 15.0f, 99.5f, 100.0f, 379.0f, 2.5f};
    dtOvlBegin(c, DTS_DRAWRECT | DTS_DRAWTEXT | DTS_RECORD | DTS_GETMODE | DTS_GETSUB);
    gOvlHeightScroll = dt_u32() % 4 == 0 ? special[dt_u32() % 9] : dt_float(0.0f, 3000.0f);
    if (dt_u32() % 8 == 0) {
        dtOvlNullRecord(gOvlHeightKind, 0);
    }
}
DTO_SETUP_CREATE(setup_height_show, DTS_SHOW)

/* A float meter value: the current one (no change), zero (hide), or something else. */
static float meterValue(float current, float range)
{
    switch (dt_u32() % 9) {
    case 0:
        return current;
    case 1:
        return 0.0f;
    case 2:
        return -0.0f;
    case 3:
        return __builtin_nanf("");
    default:
        return dt_float(-range, range);
    }
}

static void setup_height_set(DtCtx *c)
{
    dtOvlBegin(c, DTS_SHOW | DTS_SETRECT);
    float *value = dt_alloc(12);
    value[0] = meterValue(gOvlHeightValue, 700.0f);
    value[1] = dt_float(-5.0f, 400.0f);
    uint32_t packed = dt_u32();
    memcpy(&value[2], &packed, 4);
    DT_ARG_PTR(c, 0, value);
}

DTO_SETUP_CREATE(setup_balance_init, DTS_ADDGRADIENT | DTS_ADDRECT | DTS_SHOW | DTS_REMOVE)
DTO_SETUP_CREATE(setup_balance_show, DTS_SHOW)

static void setup_balance_set(DtCtx *c)
{
    dtOvlBegin(c, DTS_SHOW | DTS_SETRECT);
    float *value = dt_alloc(4);
    value[0] = meterValue(gOvlBalanceValue, 3.0f);
    DT_ARG_PTR(c, 0, value);
}

/* ---- caption, markers --------------------------------------------------------------------------- */

DTO_SETUP_CREATE(setup_caption_init, DTS_ADDTEXT | DTS_REMOVE)
DTO_SETUP(setup_caption_draw, DTS_STRING | DTS_TEXTWIDTH | DTS_ADDTEXT | DTS_REMOVE | DTS_SHOW)
DTO_SETUP_CREATE(setup_caption_show, DTS_SHOW)

static void setup_byte_arg(DtCtx *c)
{
    dtOvlBegin(c, 0);
    uint8_t *value = dt_alloc(4);
    value[0] = (uint8_t)(dt_u32() % 4 == 0 ? 0xff : dt_u32());
    DT_ARG_PTR(c, 0, value);
}

DTO_SETUP_CREATE(setup_no_stubs_create, 0)
DTO_SETUP(setup_none, 0)
DTO_SETUP(setup_indicator_draw, DTS_TEXTWIDTH | DTS_DRAWTEXT)

static void setup_dword_arg(DtCtx *c)
{
    dtOvlBegin(c, 0);
    uint32_t *value = dt_alloc(8);
    value[0] = dt_u32();
    DT_ARG_PTR(c, 0, value);
}

/* ---- timed messages ----------------------------------------------------------------------------- */

DTO_SETUP_CREATE(setup_remove_create, DTS_REMOVE)
DTO_SETUP(setup_msg_draw, DTS_REMOVE | DTS_SETCOLOR)
DTO_SETUP_CREATE(setup_show_only, DTS_SHOW)

/* A message {id (0xff = clear), arg, timer} and for the status message a number. */
static void setup_msg_set_masked(DtCtx *c, uint64_t mask, int words)
{
    dtOvlBegin(c, mask);
    uint32_t *value = dt_alloc(8);
    value[0] = (dt_u32() % 4 == 0 ? 0xffu : (uint32_t)dt_range(0, 0x18)) | (dt_u32() & 0xffff00u);
    value[1] = words > 1 ? dt_u32() % 3 == 0 ? dt_u32() : (uint32_t)dt_range(0, 400) : 0;
    DT_ARG_PTR(c, 0, value);
}

static void setup_hint_set(DtCtx *c)
{
    setup_msg_set_masked(c, DTS_REMOVE | DTS_STRING | DTS_TEXTWIDTH | DTS_ADDTEXT, 1);
}

static void setup_goal_msg_set(DtCtx *c)
{
    setup_msg_set_masked(c, DTS_REMOVE | DTS_STRING | DTS_TEXTWIDTH | DTS_ADDTEXT, 1);
}

static void setup_status_set(DtCtx *c)
{
    setup_msg_set_masked(c, DTS_REMOVE | DTS_STRING | DTS_TEXTWIDTH | DTS_ADDTEXT, 2);
}

static void setup_flash_set(DtCtx *c)
{
    setup_msg_set_masked(c, DTS_REMOVE | DTS_STRING | DTS_ADDTEXT, 1);
}

static void setup_get(DtCtx *c)
{
    dtOvlBegin(c, 0);
    DT_ARG_PTR(c, 0, dt_alloc(8));
}

DTO_SETUP(setup_flash_draw, DTS_SETCOLOR)
DTO_SETUP_CREATE(setup_icon_init, DTS_ADDSPRITE | DTS_REMOVE)

DT_CASES(overlay_hud_cases) = {
    {"ovlLetterboxDraw", 0x0041bc90, ovlLetterboxDraw, 0, DT_RET_VOID, setup_fill_bar, 50, 0},
    {"ovlFadeDraw", 0x0041bce0, ovlFadeDraw, 0, DT_RET_VOID, setup_fill_bar, 500, 0},
    {"ovlFadeSet", 0x0041bdc0, ovlFadeSet, 1, DT_RET_VOID, setup_fade_set, 500, 0},
    {"ovlScoreInit", 0x0041bf00, ovlScoreInit, 1, DT_RET_VOID, setup_score_init, 200, 0},
    {"ovlScoreDraw", 0x0041bfc0, ovlScoreDraw, 0, DT_RET_VOID, setup_score_draw, 800, 0},
    {"ovlScoreShow", 0x0041c190, ovlScoreShow, 1, DT_RET_VOID, setup_score_show, 300, 0},
    {"ovlScoreSet", 0x0041c210, ovlScoreSet, 1, DT_RET_VOID, setup_score_set, 400, 0},
    {"ovlTimerInit", 0x0041c290, ovlTimerInit, 1, DT_RET_VOID, setup_timer_init, 200, 0},
    {"ovlTimerDraw", 0x0041c310, ovlTimerDraw, 0, DT_RET_VOID, setup_timer_draw, 800, 0},
    {"ovlTimerShow", 0x0041c4a0, ovlTimerShow, 1, DT_RET_VOID, setup_timer_show, 300, 0},
    {"ovlTimerSet", 0x0041c4e0, ovlTimerSet, 1, DT_RET_VOID, setup_timer_set, 100, 0},
    {"ovlNameDraw", 0x0041c500, ovlNameDraw, 0, DT_RET_VOID, setup_name_draw, 300, 0},
    {"ovlHeightInit", 0x0041cae0, ovlHeightInit, 1, DT_RET_VOID, setup_height_init, 200, 0},
    {"ovlHeightDraw", 0x0041cc60, ovlHeightDraw, 0, DT_RET_VOID, setup_height_draw, 1500, 0},
    {"ovlHeightShow", 0x0041cfa0, ovlHeightShow, 1, DT_RET_VOID, setup_height_show, 300, 0},
    {"ovlHeightSet", 0x0041d010, ovlHeightSet, 1, DT_RET_VOID, setup_height_set, 1500, 0},
    {"ovlBalanceInit", 0x0041d1c0, ovlBalanceInit, 1, DT_RET_VOID, setup_balance_init, 200, 0},
    {"ovlBalanceDraw", 0x0041d380, ovlBalanceDraw, 0, DT_RET_VOID, setup_none, 10, 0},
    {"ovlBalanceShow", 0x0041d3a0, ovlBalanceShow, 1, DT_RET_VOID, setup_balance_show, 300, 0},
    {"ovlBalanceSet", 0x0041d410, ovlBalanceSet, 1, DT_RET_VOID, setup_balance_set, 1000, 0},
    {"ovlCaptionInit", 0x0041d530, ovlCaptionInit, 1, DT_RET_VOID, setup_caption_init, 200, 0},
    {"ovlCaptionDraw", 0x0041d5c0, ovlCaptionDraw, 0, DT_RET_VOID, setup_caption_draw, 400, 0},
    {"ovlCaptionShow", 0x0041d680, ovlCaptionShow, 1, DT_RET_VOID, setup_caption_show, 300, 0},
    {"ovlCaptionSet", 0x0041d6d0, ovlCaptionSet, 1, DT_RET_VOID, setup_byte_arg, 100, 0},
    {"ovlElement8Init", 0x0041d6f0, ovlElement8Init, 1, DT_RET_VOID, setup_no_stubs_create, 20, 0},
    {"ovlElement8Draw", 0x0041d710, ovlElement8Draw, 0, DT_RET_VOID, setup_none, 20, 0},
    {"ovlElement8Show", 0x0041d730, ovlElement8Show, 1, DT_RET_VOID, setup_no_stubs_create, 20, 0},
    {"ovlElement8Set", 0x0041d750, ovlElement8Set, 1, DT_RET_VOID, setup_byte_arg, 100, 0},
    {"ovlIndicatorInit", 0x0041d770, ovlIndicatorInit, 1, DT_RET_VOID, setup_no_stubs_create, 50, 0},
    {"ovlIndicatorDraw", 0x0041d790, ovlIndicatorDraw, 0, DT_RET_VOID, setup_indicator_draw, 400, 0},
    {"ovlIndicatorShow", 0x0041d810, ovlIndicatorShow, 1, DT_RET_VOID, setup_no_stubs_create, 20, 0},
    {"ovlIndicatorSet", 0x0041d830, ovlIndicatorSet, 1, DT_RET_VOID, setup_dword_arg, 100, 0},
    {"ovlHintInit", 0x0041d860, ovlHintInit, 1, DT_RET_VOID, setup_remove_create, 200, 0},
    {"ovlHintDraw", 0x0041d8e0, ovlHintDraw, 0, DT_RET_VOID, setup_msg_draw, 1000, 0},
    {"ovlHintShow", 0x0041da90, ovlHintShow, 1, DT_RET_VOID, setup_show_only, 200, 0},
    {"ovlHintSet", 0x0041dad0, ovlHintSet, 1, DT_RET_VOID, setup_hint_set, 1500, 0},
    {"ovlHintGet", 0x0041ddc0, ovlHintGet, 1, DT_RET_VOID, setup_get, 50, 0},
    {"ovlGoalMsgInit", 0x0041dde0, ovlGoalMsgInit, 1, DT_RET_VOID, setup_remove_create, 200, 0},
    {"ovlGoalMsgDraw", 0x0041de60, ovlGoalMsgDraw, 0, DT_RET_VOID, setup_msg_draw, 1000, 0},
    {"ovlGoalMsgShow", 0x0041dfc0, ovlGoalMsgShow, 1, DT_RET_VOID, setup_show_only, 200, 0},
    {"ovlGoalMsgSet", 0x0041e000, ovlGoalMsgSet, 1, DT_RET_VOID, setup_goal_msg_set, 500, 0},
    {"ovlGoalMsgGet", 0x0041e130, ovlGoalMsgGet, 1, DT_RET_VOID, setup_get, 50, 0},
    {"ovlStatusMsgInit", 0x0041e150, ovlStatusMsgInit, 1, DT_RET_VOID, setup_remove_create, 200, 0},
    {"ovlStatusMsgDraw", 0x0041e190, ovlStatusMsgDraw, 0, DT_RET_VOID, setup_msg_draw, 1000, 0},
    {"ovlStatusMsgShow", 0x0041e280, ovlStatusMsgShow, 1, DT_RET_VOID, setup_show_only, 200, 0},
    {"ovlStatusMsgSet", 0x0041e2b0, ovlStatusMsgSet, 1, DT_RET_VOID, setup_status_set, 800, 0},
    {"ovlStatusMsgGet", 0x0041e470, ovlStatusMsgGet, 1, DT_RET_VOID, setup_get, 50, 0},
    {"ovlFlashInit", 0x0041e4a0, ovlFlashInit, 1, DT_RET_VOID, setup_remove_create, 200, 0},
    {"ovlFlashDraw", 0x0041e510, ovlFlashDraw, 0, DT_RET_VOID, setup_flash_draw, 1000, 0},
    {"ovlFlashShow", 0x0041e620, ovlFlashShow, 1, DT_RET_VOID, setup_show_only, 200, 0},
    {"ovlFlashSet", 0x0041e660, ovlFlashSet, 1, DT_RET_VOID, setup_flash_set, 500, 0},
    {"ovlIconInit", 0x0041e750, ovlIconInit, 1, DT_RET_VOID, setup_icon_init, 200, 0},
    {"ovlIconShow", 0x0041e7c0, ovlIconShow, 1, DT_RET_VOID, setup_show_only, 100, 0},
};
