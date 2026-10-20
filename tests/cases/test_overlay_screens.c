/* Overlay menu screens (screens 1..18 and 25): enter, update, draw and refresh callbacks. */
#include "bmx/com/action/overlay.h"
#include "dt.h"
#include "dt_overlay.h"

#define TEXT (DTS_STRING | DTS_TEXTWIDTH | DTS_ADDTEXT)

DTO_SETUP_CREATE(setup_screen1_enter, TEXT | DTS_GETMODE | DTS_GETSUB)
DTO_SETUP(setup_screen1_update, DTG_SCREEN | DTS_PRESSED | DTS_STARTMODE | DTS_BIGGAME)
DTO_SETUP_CREATE(setup_screen2_enter, TEXT | DTS_REMOVE)
DTO_SETUP(setup_screen2_update, DTG_SCREEN | DTS_PRESSED | DTS_SETVIEW | DTS_CONTROLLER | TEXT)
DTO_SETUP(setup_screen2_refresh, TEXT | DTS_REMOVE)
DTO_SETUP_CREATE(setup_screen3_enter, TEXT | DTS_REMOVE | DTS_SOUND | DTS_ADDRECT)
static void setup_screen3_update(DtCtx *c)
{
    static const uint8_t volumes[] = {0, 1, 0x10, 0x11, 0x12, 0x13, 5, 9};
    dtOvlBegin(c, DTG_SCREEN | DTS_PRESSED | DTS_ADDRECT | TEXT);
    gSoundVolumes[0] = volumes[dt_u32() % sizeof volumes];
    gSoundVolumes[1] = volumes[dt_u32() % sizeof volumes];
    gSoundVolumes[2] = (uint8_t)(dt_u32() % 2 ? dt_range(0, 11) : 9 * (dt_u32() % 2));
    if (dt_u32() % 3 == 0) { /* left or right on one of the options */
        dtOvlSetPad(gCurrentPlayer, dt_u32() % 2 ? 4 : 8);
        gOvlSharedPad = 0;
    }
}
DTO_SETUP(setup_screen3_refresh, TEXT | DTS_REMOVE | DTS_ADDRECT)
DTO_SETUP(setup_screen3_draw, DTS_DRAWRECT)
DTO_SETUP_CREATE(setup_screen4_enter, TEXT | DTS_REMOVE)
DTO_SETUP(setup_screen4_update, DTG_SCREEN | DTS_PRESSED | TEXT)
DTO_SETUP(setup_screen4_draw, DTS_STRING | DTS_TEXTWIDTH | DTS_DRAWTEXT)
DTO_SETUP(setup_screen4_refresh, TEXT | DTS_REMOVE)
static void setup_screen12_enter(DtCtx *c)
{
    dtOvlBegin(c, TEXT | DTS_GETMODE);
    DT_ARG_INT(c, 0, dt_u32() % 3 != 0);
    gCurrentLevel = (uint8_t)dt_range(0, 12);
}
DTO_SETUP(setup_screen12_update, DTG_SCREEN | DTS_PRESSED | DTS_BIGGAME)
DTO_SETUP_CREATE(setup_screen13_enter, TEXT | DTS_GOALINFO)
DTO_SETUP(setup_screen13_update, DTG_SCREEN | DTS_PRESSED | DTS_BIGGAME | DTS_SAVEWRITE)
DTO_SETUP(setup_screen13_draw, TEXT | DTS_GOALINFO | DTS_SETCOLOR | DTS_DRAWRECT | DTS_DRAWTEXT)
DTO_SETUP_CREATE(setup_screen16_enter, TEXT | DTS_REMOVE | DTS_GOALINFO)
static void setup_screen16_update(DtCtx *c)
{
    dtOvlBegin(c, DTG_SCREEN | DTS_PRESSED | DTS_SAVEWRITE | DTS_GOALINFO | TEXT);
    gOvlPage = (uint8_t)dt_range(0, 3);
    gOvlFrame = (uint32_t)dt_range(0x20, 0x80);
    gProfiles[gCurrentPlayer & 1].character = (uint8_t)(10 + dt_range(0, 2) % 3);
    *(uint32_t *)(gPlayerRecs[gCurrentPlayer & 1] + kOvlPlayerUnlockMask) = dt_u32();
    dtOvlSetPad(gCurrentPlayer, 0x20080);
    gOvlSharedPad = 0;
}
static void setup_screen16_draw(DtCtx *c)
{
    dtOvlBegin(c, DTS_SETCOLOR | DTS_STRING | DTS_TEXTWIDTH | DTS_DRAWTEXT);
    if (dt_u32() % 6 == 0) {
        gOvlGoalRows[0][0] = kOvlNoHandle; /* no goal rows */
    }
}
DTO_SETUP_CREATE(setup_screen17_enter, 0)
static void setup_screen17_update(DtCtx *c)
{
    static const char letters[] = {' ', 'A', 'Z', 'a', 'z', 'm', 'B', 'y'};
    dtOvlBegin(c, DTG_SCREEN | DTS_PRESSED | DTS_RECORD | DTS_LEVELREC | DTS_ROLLERREC);
    gOvlNameBuf[gOvlNameCursor % 11] = letters[dt_u32() % sizeof letters];
    gOvlNameCursor %= 11;
    if (dt_u32() % 2 == 0) {
        dtOvlSetPad(gCurrentPlayer, 1u << dt_range(0, 1));
    }
}
DTO_SETUP(setup_screen17_draw, DTS_STRING | DTS_TEXTWIDTH | DTS_DRAWTEXT)
DTO_SETUP_CREATE(setup_screen18_enter,
                 TEXT | DTS_REMOVE | DTS_LEVELREC | DTS_RIDERREC | DTS_GETMODE | DTS_GETSUB)
static void setup_screen18_update(DtCtx *c)
{
    static const uint8_t ids[] = {0x12, 0x13, 0x13, 0x13, 0x14, 0x15, 0x16};
    dtOvlBegin(c, DTG_SCREEN | DTS_PRESSED | DTS_BIGGAME | DTS_PENDING | DTS_SAVEWRITE);
    gOvlScreenId = ids[dt_u32() % sizeof ids];
    if (dt_u32() % 2 == 0) {
        dtOvlSetMode(0, 0);
    }
}
DTO_SETUP_CREATE(setup_screen7_enter, TEXT | DTS_RUNTYPE)
static void setup_screen7_update(DtCtx *c)
{
    dtOvlBegin(c, DTG_SCREEN | DTS_PRESSED | DTS_RUNTYPE | DTS_RESETGOALS | DTS_RIDERRESET | DTS_CAMERA |
                      DTS_RESETVIEW | DTS_LEAVERUN | DTS_MENUMODE);
    if (dt_u32() % 3 == 0) {
        gOvlReturnScreen = 0x16;
    }
}
DTO_SETUP(setup_screen8_update, DTG_SCREEN | DTS_PRESSED | DTS_SAVEWRITE)
DTO_SETUP(setup_screen9_draw,
          DTS_HEADER | DTS_RUNCOUNT | DTS_STRING | DTS_TEXTWIDTH | DTS_DRAWTEXT | DTS_DRAWRECT | DTS_SOUND)
DTO_SETUP(setup_screen9_update, DTG_SCREEN | DTS_PRESSED | DTS_BIGGAME)
DTO_SETUP_CREATE(setup_screen10_enter, TEXT | DTS_GETMODE | DTS_GETSUB | DTS_TESTOPT)
static void setup_screen10_draw(DtCtx *c)
{
    dtOvlBegin(c, DTS_GETMODE | DTS_GETSUB | DTS_TESTOPT | DTS_HEADER | DTS_BODY | DTS_RECORD | DTS_STRING |
                      DTS_TEXTWIDTH | DTS_DRAWTEXT | DTS_DRAWRECT | DTS_SOUND | DTS_RUNCOUNT | DTS_NAMETABLE);
    if (dt_u32() % 6 == 0) { /* either of the two record lookups fails */
        dtOvlNullRecord(gScoreRecordSlot[dt_range(0, 11)], dt_u32() % 2 + 1);
    }
}
DTO_SETUP(setup_screen10_update, DTG_SCREEN | DTS_PRESSED | DTS_REPLAY | DTS_MODEBYTE | DTS_FILEOPEN |
                                     DTS_FILEWRITE | DTS_STARTMODE | DTS_SETSUB | DTS_BIGGAME |
                                     DTS_RIDERRESET | DTS_CAMERA | DTS_RESETVIEW | DTS_SAVEWRITE)
DTO_SETUP_CREATE(setup_screen11_enter, TEXT | DTS_REMOVE | DTS_SOUND | DTS_WINBEGIN | DTS_WINEND)
static void setup_screen11_update(DtCtx *c)
{
    static const uint32_t frames[] = {0, 1, 2, 3, 0x96, 0x97, 0x98, 0x20};
    dtOvlBegin(c, DTG_SCREEN | DTS_PRESSED | DTS_SETCOLOR | DTS_SAVECLOSE | DTS_SAVEQUERY | DTS_SAVEWRITE |
                      DTS_WINQUERY | DTS_WINSTART | DTS_WINCALLBACK | DTS_RESETGOALS | TEXT);
    if (dt_u32() % 2 == 0) {
        gOvlPage = (uint8_t)dt_range(9, 11);
        gOvlFrame = frames[dt_u32() % 8];
        dtOvlNoInput();
    }
}
DTO_SETUP(setup_screen11_draw, DTS_SETCOLOR | DTS_DRAWRECT)
DTO_SETUP(setup_screen11_refresh, TEXT | DTS_REMOVE)
DTO_SETUP(setup_run_times,
          DTS_RUNCOUNT | DTS_STRING | DTS_TEXTWIDTH | DTS_DRAWTEXT | DTS_DRAWRECT | DTS_SOUND)

static void setup_screen16_refresh(DtCtx *c)
{
    dtOvlBegin(c, DTS_REMOVE | DTS_GOALINFO | TEXT | DTS_SOUND);
    DT_ARG_INT(c, 0, dt_u32() % 8 == 0 ? dt_u32() : (uint32_t)dt_range(0, 5));
}

static void setup_screen18_draw(DtCtx *c)
{
    dtOvlBegin(c, DTS_RECORD | DTS_SETCOLOR | DTS_GETMODE | DTS_GETSUB | DTS_STRING | DTS_TEXTWIDTH |
                      DTS_DRAWTEXT);
    dtOvlNoNullRecords();
}

static void setup_run_times_arg(DtCtx *c)
{
    setup_run_times(c);
    int *y = dt_alloc(4);
    *y = dt_range(-40, 300);
    DT_ARG_PTR(c, 0, y);
}

static void setup_save_step(DtCtx *c)
{
    dtOvlBegin(c, 0);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_trick_available(DtCtx *c)
{
    dtOvlBegin(c, 0);
    DT_ARG_INT(c, 0, dt_range(0, 0x60));
}

static void setup_goal_line(DtCtx *c)
{
    dtOvlBegin(c, DTS_STRING | DTS_TEXTWIDTH | DTS_ADDTEXT);
    uint8_t *goal = dt_alloc(4);
    goal[0] = (uint8_t)dt_range(0, 11);
    goal[1] = (uint8_t)dt_range(0, 9);
    int *handle = dt_alloc(4);
    *handle = dt_range(0, 18);
    int *y = dt_alloc(4);
    *y = dt_range(-40, 400);
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_PTR(c, 1, goal);
    DT_ARG_PTR(c, 2, handle);
    DT_ARG_PTR(c, 3, y);
    DT_ARG_INT(c, 4, dt_u32() % 2);
}

DT_CASES(overlay_screen_cases) = {
    {"ovlScreen1Enter", 0x004157e0, ovlScreen1Enter, 1, DT_RET_VOID, setup_screen1_enter, 300, 0},
    {"ovlScreen1Update", 0x00415a00, ovlScreen1Update, 0, DT_RET_INT, setup_screen1_update, 600, 0},
    {"ovlScreen2Enter", 0x00415b70, ovlScreen2Enter, 1, DT_RET_VOID, setup_screen2_enter, 300, 0},
    {"ovlScreen2Update", 0x00415cf0, ovlScreen2Update, 0, DT_RET_INT, setup_screen2_update, 800, 0},
    {"ovlScreen2Refresh", 0x00416060, ovlScreen2Refresh, 0, DT_RET_VOID, setup_screen2_refresh, 300, 0},
    {"ovlScreen3Enter", 0x004161f0, ovlScreen3Enter, 1, DT_RET_VOID, setup_screen3_enter, 300, 0},
    {"ovlScreen3Update", 0x004163d0, ovlScreen3Update, 0, DT_RET_INT, setup_screen3_update, 1000, 0},
    {"ovlScreen3Refresh", 0x00416810, ovlScreen3Refresh, 0, DT_RET_VOID, setup_screen3_refresh, 300, 0},
    {"ovlScreen3Draw", 0x00416a10, ovlScreen3Draw, 0, DT_RET_VOID, setup_screen3_draw, 300, 0},
    {"ovlScreen4Enter", 0x00416ae0, ovlScreen4Enter, 1, DT_RET_VOID, setup_screen4_enter, 300, 0},
    {"ovlScreen4Update", 0x00416c70, ovlScreen4Update, 0, DT_RET_INT, setup_screen4_update, 1000, 0},
    {"ovlScreen4Draw", 0x00416ec0, ovlScreen4Draw, 0, DT_RET_VOID, setup_screen4_draw, 200, 0},
    {"ovlScreen4Refresh", 0x00416ee0, ovlScreen4Refresh, 0, DT_RET_VOID, setup_screen4_refresh, 500, 0},
    {"ovlTrickAvailable", 0x00417190, ovlTrickAvailable, 1, DT_RET_INT, setup_trick_available, 200, 0},
    {"ovlScreen12Enter", 0x004171e0, ovlScreen12Enter, 1, DT_RET_VOID, setup_screen12_enter, 300, 0},
    {"ovlScreen12Update", 0x00417360, ovlScreen12Update, 0, DT_RET_INT, setup_screen12_update, 600, 0},
    {"ovlScreen12Draw", 0x004174a0, ovlScreen12Draw, 0, DT_RET_VOID, setup_screen4_draw, 200, 0},
    {"ovlScreen13Enter", 0x004174c0, ovlScreen13Enter, 1, DT_RET_VOID, setup_screen13_enter, 500, 0},
    {"ovlScreen13Update", 0x00417820, ovlScreen13Update, 0, DT_RET_INT, setup_screen13_update, 500, 0},
    {"ovlScreen13Draw", 0x00417930, ovlScreen13Draw, 0, DT_RET_VOID, setup_screen13_draw, 600, 0},
    {"ovlScreen16Enter", 0x00417b80, ovlScreen16Enter, 1, DT_RET_VOID, setup_screen16_enter, 300, 0},
    {"ovlScreen16Refresh", 0x00417c00, ovlScreen16Refresh, 1, DT_RET_VOID, setup_screen16_refresh, 800, 0},
    {"ovlAddGoalLine", 0x004180f0, ovlAddGoalLine, 5, DT_RET_VOID, setup_goal_line, 800, 0},
    {"ovlScreen16Update", 0x004184b0, ovlScreen16Update, 0, DT_RET_INT, setup_screen16_update, 600, 0},
    {"ovlScreen16Draw", 0x00418620, ovlScreen16Draw, 0, DT_RET_VOID, setup_screen16_draw, 800, 0},
    {"ovlScreen17Enter", 0x00418800, ovlScreen17Enter, 1, DT_RET_VOID, setup_screen17_enter, 200, 0},
    {"ovlScreen17Update", 0x00418870, ovlScreen17Update, 0, DT_RET_INT, setup_screen17_update, 1000, 0},
    {"ovlScreen17Draw", 0x00418be0, ovlScreen17Draw, 0, DT_RET_VOID, setup_screen17_draw, 300, 0},
    {"ovlScreen18Enter", 0x00418d70, ovlScreen18Enter, 1, DT_RET_VOID, setup_screen18_enter, 400, 0},
    {"ovlScreen18Update", 0x00419080, ovlScreen18Update, 0, DT_RET_INT, setup_screen18_update, 800, 0},
    {"ovlScreen18Draw", 0x004192b0, ovlScreen18Draw, 0, DT_RET_VOID, setup_screen18_draw, 400, 0},
    {"ovlScreen7Enter", 0x00419380, ovlScreen7Enter, 1, DT_RET_VOID, setup_screen7_enter, 300, 0},
    {"ovlScreen7Update", 0x004194c0, ovlScreen7Update, 0, DT_RET_INT, setup_screen7_update, 800, 0},
    {"ovlScreen8Update", 0x00419ca0, ovlScreen8Update, 0, DT_RET_INT, setup_screen8_update, 500, 0},
    {"ovlScreen9Draw", 0x00419d90, ovlScreen9Draw, 0, DT_RET_VOID, setup_screen9_draw, 400, 0},
    {"ovlDrawRunTimes", 0x00419eb0, ovlDrawRunTimes, 1, DT_RET_VOID, setup_run_times_arg, 800, 0},
    {"ovlScreen9Update", 0x0041a360, ovlScreen9Update, 0, DT_RET_INT, setup_screen9_update, 500, 0},
    {"ovlScreen10Enter", 0x0041a480, ovlScreen10Enter, 1, DT_RET_VOID, setup_screen10_enter, 600, 0},
    {"ovlScreen10Draw", 0x0041a830, ovlScreen10Draw, 0, DT_RET_VOID, setup_screen10_draw, 1000, 0},
    {"ovlScreen10Update", 0x0041ad00, ovlScreen10Update, 0, DT_RET_INT, setup_screen10_update, 1000, 0},
    {"ovlScreen11Enter", 0x0041b0a0, ovlScreen11Enter, 1, DT_RET_VOID, setup_screen11_enter, 300, 0},
    {"ovlScreen11Update", 0x0041b170, ovlScreen11Update, 0, DT_RET_INT, setup_screen11_update, 2000, 0},
    {"ovlScreen11Draw", 0x0041b880, ovlScreen11Draw, 0, DT_RET_VOID, setup_screen11_draw, 400, 0},
    {"ovlScreen11Refresh", 0x0041ba10, ovlScreen11Refresh, 0, DT_RET_VOID, setup_screen11_refresh, 400, 0},
    {"ovlSetSaveStep", 0x0041bc70, ovlSetSaveStep, 2, DT_RET_VOID, setup_save_step, 50, 0},
};
