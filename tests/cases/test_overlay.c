/*
 * Overlay core: setup, per frame update and draw, screen switching and the shared drawing helpers.
 * Callees outside overlay.c are stubbed by dt_overlay.c; see there.
 */
#include "bmx/com/action/overlay.h"
#include "dt.h"
#include "dt_overlay.h"

/* ---- setup, shutdown and per frame -------------------------------------------------------------- */

DTO_SETUP(setup_player_names_stubs, DTS_DRAWTEXT | DTS_STRING | DTS_NAMETABLE)
static void setup_player_names(DtCtx *c)
{
    setup_player_names_stubs(c);
    int *y = dt_alloc(4);
    *y = dt_range(-40, 400);
    DT_ARG_PTR(c, 0, y);
}

DTO_SETUP(setup_screen25, DTG_SCREEN | DTS_PRESSED | DTS_BIGGAME)
DTO_SETUP(setup_init, DTS_ADDRECT | DTS_SHOW | DTS_DRAWCB)
static void setup_shutdown(DtCtx *c)
{
    dtOvlBegin(c, DTG_SCREEN | DTS_STRSETS);
    dtOvlMaybeNoScreen();
}

DTO_SETUP(setup_load_fonts, DTS_REGISTER | DTS_RESERVE | DTS_ASSET | DTS_ALLOC | DTS_PARSEFONT)

static void setup_load_strings(DtCtx *c)
{
    dtOvlBegin(c, DTS_ASSET | DTS_ALLOC | DTS_STRSETS);
    DT_ARG_INT(c, 0, dt_u32() % 4 == 0 ? dt_u32() : (uint32_t)dt_range(0, 5));
}

static void setup_draw(DtCtx *c)
{
    dtOvlBegin(c, DTS_SETCOLOR | DTS_FLUSH | DTS_DRAWTEXT | DTS_TEXTWIDTH | DTS_DRAWRECT);
    dtOvlMaybeNoScreen();
}

static void setup_update(DtCtx *c)
{
    dtOvlBegin(c, DTS_PRESSED | DTS_SOUND | DTS_SETCOLOR);
    dtOvlMaybeNoScreen();
}

static void setup_element_visible(DtCtx *c)
{
    dtOvlBegin(c, 0);
    DT_ARG_INT(c, 0, dt_range(0, 16));
    DT_ARG_INT(c, 1, dt_u32() % 4 == 0 ? dt_u32() : dt_u32() % 2);
}

static void setup_element_set(DtCtx *c)
{
    dtOvlBegin(c, 0);
    DT_ARG_INT(c, 0, dt_range(0, 16));
    DT_ARG_PTR(c, 1, dtOvlRandomBytes(8));
}

DTO_SETUP(setup_none, 0)

static void setup_set_screen(DtCtx *c)
{
    static const uint8_t special[] = {1, 7, 10, 0x16, 0xc, 0x17, 0x11, 0x19, 0x18, 0, 0x1a, 1, 10};
    dtOvlBegin(c, DTG_SCREEN);
    dtOvlMaybeNoScreen();
    uint32_t id = dt_u32() % 8 == 0 ? 0x1a : (uint32_t)dt_range(0, 25);
    if (dt_u32() % 2 == 0) {
        id = special[dt_u32() % sizeof special];
    }
    if (dt_u32() % 3 == 0) { /* coming from the transient screen 7 */
        gOvlScreenId = 7;
        gOvlReturnScreen = (uint8_t)(dt_u32() % 2 ? id : dt_range(0, 25));
    }
    if (dt_u32() % 3 == 0) {
        dtOvlSetMode(0, 0);
        gCurrentLevel = (uint8_t)dt_range(0, 7);
    }
    DT_ARG_INT(c, 0, id);
}

static void setup_tiny_text(DtCtx *c)
{
    dtOvlBegin(c, DTS_DRAWTEXT);
    DT_ARG_PTR(c, 0, dtOvlRandomText(10));
    DT_ARG_INT(c, 1, dt_range(-50, 700));
    DT_ARG_INT(c, 2, dt_range(-50, 500));
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_reset_state(DtCtx *c)
{
    dtOvlBegin(c, 0);
    dtOvlMaybeNoScreen();
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_backdrop(DtCtx *c)
{
    dtOvlBegin(c, DTS_SHOW);
    DT_ARG_INT(c, 0, dt_u32() % 3 == 0 ? dt_u32() : (uint32_t)dt_range(0, 2));
}

static void setup_menu_items(DtCtx *c)
{
    dtOvlBegin(c, DTS_STRING | DTS_TEXTWIDTH | DTS_DRAWTEXT);
    DT_ARG_INT(c, 0, dt_u32() % 16);
    DT_ARG_INT(c, 1, dt_range(0, 3));
}

static void setup_pad_confirm(DtCtx *c)
{
    dtOvlBegin(c, DTS_PRESSED);
    DT_ARG_INT(c, 0, dt_range(0, 3));
}

static void setup_feet_inches(DtCtx *c)
{
    dtOvlBegin(c, 0);
    DT_ARG_PTR(c, 0, dt_alloc(64));
    static const float edge[] = {0.0f, -1.0f, 1.0f, 0.5f, 100000.0f, -100000.0f, 3.0e9f, 11.99f, 12.01f};
    DT_ARG_FLOAT(c, 1, dt_u32() % 4 == 0 ? edge[dt_u32() % 9] : dt_float(-5.0f, 400.0f));
}

static void setup_trick_name(DtCtx *c)
{
    static const uint8_t turns[] = {0,    1,    2,    3,    4,    5,    10, 0x1a, 0x1b, 0x23,
                                    0x2a, 0x2b, 0x2c, 0x2d, 0xff, 0xff, 1,  3,    4,    10};
    static const uint8_t bars[] = {0, 1, 2, 5, 6, 7, 0xff, 0xff, 9, 0x25};
    dtOvlBegin(c, 0);
    uint8_t *trick = dt_alloc(8);
    trick[0] = (uint8_t)dt_range(0, 7);
    trick[1] = turns[dt_u32() % sizeof turns];
    trick[2] = bars[dt_u32() % sizeof bars];
    trick[3] = (uint8_t)dt_range(0, 7);
    trick[4] = (uint8_t)dt_range(0, 8);
    switch (dt_u32() % 8) { /* make the special combinations that have their own names likely */
    case 0:
        trick[1] = 1;
        trick[2] = 1;
        break;
    case 1:
        trick[1] = 3;
        trick[4] = 1;
        break;
    case 2:
        trick[1] = 3;
        trick[2] = 0;
        trick[4] = (uint8_t)dt_range(0, 8);
        break;
    case 3:
        trick[1] = 4;
        trick[2] = 6;
        break;
    case 4:
        trick[1] = 10;
        trick[4] = 2;
        break;
    case 5:
        trick[1] = 0xff;
        trick[2] = 0;
        trick[4] = (uint8_t)dt_range(1, 8);
        break;
    default:
        break;
    }
    DT_ARG_PTR(c, 0, dt_alloc(256));
    DT_ARG_PTR(c, 1, trick);
    DT_ARG_INT(c, 2, dt_u32() % 2);
}

DT_CASES(overlay_core_cases) = {
    {"ovlDrawPlayerNames", 0x00414580, ovlDrawPlayerNames, 1, DT_RET_VOID, setup_player_names, 300, 0},
    {"ovlScreen25Update", 0x004146f0, ovlScreen25Update, 0, DT_RET_INT, setup_screen25, 500, 0},
    {"ovlInit", 0x004147e0, ovlInit, 0, DT_RET_VOID, setup_init, 200, 0},
    {"ovlShutdown", 0x004148b0, ovlShutdown, 0, DT_RET_VOID, setup_shutdown, 300, 0},
    {"ovlLoadFonts", 0x00414970, ovlLoadFonts, 0, DT_RET_VOID, setup_load_fonts, 300, 0},
    {"ovlLoadStrings", 0x00414dd0, ovlLoadStrings, 1, DT_RET_VOID, setup_load_strings, 300, 0},
    {"ovlDraw", 0x00414f60, ovlDraw, 0, DT_RET_INT, setup_draw, 500, 0},
    {"ovlUpdate", 0x00415140, ovlUpdate, 0, DT_RET_INT, setup_update, 600, 0},
    {"ovlSetElementVisible", 0x00415300, ovlSetElementVisible, 2, DT_RET_VOID, setup_element_visible, 300, 0},
    {"ovlSetElementA", 0x00415370, ovlSetElementA, 2, DT_RET_VOID, setup_element_set, 200, 0},
    {"ovlSetElementB", 0x004153a0, ovlSetElementB, 2, DT_RET_VOID, setup_element_set, 200, 0},
    {"ovlGetScreenId", 0x004153d0, ovlGetScreenId, 0, DT_RET_INT, setup_none, 100, 0},
    {"ovlSetScreen", 0x00415400, ovlSetScreen, 1, DT_RET_VOID, setup_set_screen, 600, 0},
    {"ovlDrawTinyText", 0x004156f0, ovlDrawTinyText, 4, DT_RET_VOID, setup_tiny_text, 100, 0},
    {"ovlResetScreenState", 0x00415730, ovlResetScreenState, 1, DT_RET_VOID, setup_reset_state, 200, 0},
    {"ovlSetBackdrop", 0x00415770, ovlSetBackdrop, 1, DT_RET_VOID, setup_backdrop, 300, 0},
    {"ovlDrawMenuItems", 0x0041e7f0, ovlDrawMenuItems, 2, DT_RET_VOID, setup_menu_items, 400, 0},
    {"ovlPadConfirm", 0x0041e920, ovlPadConfirm, 1, DT_RET_INT, setup_pad_confirm, 300, 0},
    {"ovlPulseColor", 0x0041e9c0, ovlPulseColor, 0, DT_RET_INT, setup_none, 300, 0},
    {"ovlFormatFeetInches", 0x0041ea40, ovlFormatFeetInches, 2, DT_RET_VOID, setup_feet_inches, 400, 0},
    {"ovlFormatTrickName", 0x0041eab0, ovlFormatTrickName, 3, DT_RET_VOID, setup_trick_name, 1500, 0},
};
