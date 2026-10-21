/*
 * Differential tests of overlymp.c: the multiplayer overlay screens. Text drawing, the string table, pad
 * presses (scripted per button and player), the other overlay screens and the game calls are stubbed and
 * logged; the game state functions run natively on random state.
 */
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/overlymp.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_gameplay.h"

enum { kGroups = DTGP_UI | DTGP_SOUND | DTGP_DEBUG };

static void randomWord(void)
{
    const uint32_t len = (uint32_t)dt_range(0, 8);

    dt_fill(gOvlMpWord, sizeof gOvlMpWord);
    for (uint32_t i = 0; i < 9; i++) {
        if (i < len) {
            const int32_t pick = dt_range(0, 3);
            gOvlMpWord[i] = pick == 0   ? 'Z'
                            : pick == 1 ? 'z'
                            : pick == 2 ? 'A'
                                        : (char)dt_range('a' - 2, 'z' + 2);
        } else {
            gOvlMpWord[i] = 0;
        }
    }
    gOvlMpWord[0] = (dt_u32() & 7) == 0 ? 'a' : gOvlMpWord[0];
    gOvlMpWordCursor = (uint8_t)((dt_u32() & 7) == 0 ? dt_range(7, 8) : dt_range(0, 7));
    if ((dt_u32() & 15) == 0) {
        gOvlMpWord[gOvlMpWordCursor] = (char)dt_u32();
    }
}

static void randomScreenState(void)
{
    OvlScreen *screen = dt_alloc(sizeof *screen);

    /* The record table is followed by overlay state in memory: fill it first. */
    dt_fill(&gGameRecords, sizeof gGameRecords);
    dt_fill(&gPlayerRecs, sizeof gPlayerRecs);
    /* Scores stay short: the screens format them into 32 byte buffers (long ones smash the stack). */
    for (uint32_t i = 0; i < sizeof gGameRecords / 8; i++) {
        uint8_t *rec = gGameRecords + i * 8;
        *(uint32_t *)rec = (uint32_t)dt_range(0, 99999);
        rec[4] = (uint8_t)dt_range(0, 1);
    }
    for (uint32_t p = 0; p < 2; p++) {
        uint8_t *rec = (uint8_t *)&gPlayerRecs + p * kOvlPlayerStride;
        *(uint32_t *)(rec + 0x5c) = (uint32_t)dt_range(0, 99999);
        *(uint32_t *)(rec + 0x60) = (uint32_t)dt_range(0, 99999);
        *(uint32_t *)(rec + 0x68) = (uint32_t)dt_range(0, 5);
    }
    screen->itemCount = (uint8_t)dt_range(1, 6);
    screen->selected = (uint8_t)((dt_u32() & 7) == 0 ? dt_range(0, 9) : dt_range(0, screen->itemCount - 1));
    gOvlScreen = screen;
    gPlayerCount = (uint8_t)dt_range(0, 2);
    gCurrentPlayer = (uint8_t)dt_range(0, 1);
    gCurrentLevel = (uint8_t)dt_range(0, 11);
    gOvlFrame = dt_u32();
    gOvlPulse = dt_u32();
    gOvlSharedPad = dt_u32();
    gOvlReturnScreen = (uint8_t)dt_u32();
    gDemoPlayer = (uint8_t)dt_u32();
    gOvlMpModeLines = (uint8_t)dt_range(1, 8);
    gOvlSavedSubMode = (int8_t)dt_range(-1, 4);
    for (uint32_t i = 0; i < sizeof gGameRoundStates; i++) {
        gGameRoundStates[i] = (uint8_t)dt_range(0, 3);
    }
    gGameMode = (uint8_t)((dt_u32() & 7) == 0 ? dt_range(0, 6) : dt_range(1, 4));
    gGameSubMode = (uint8_t)dt_range(0, 3);
    gGameOptions = (uint8_t)dt_u32();
    gGameRunCount = (uint8_t)dt_range(0, 5);
    randomWord();
    {
        const uint32_t a = dt_u32();
        const uint32_t b = dt_u32();
        dtgp.extra[3] = (dt_u32() & 3) == 0 ? 0 : a & b;
    }
    dtgp.extra[4] = dt_u32();
}

static void setupHeader(DtCtx *c)
{
    char *title = dt_alloc(16);

    dtgpBegin(c, kGroups);
    for (int i = 0; i < dt_range(0, 12); i++) {
        title[i] = (char)dt_range('a', 'z');
    }
    DT_ARG_PTR(c, 0, title);
}

static void setupScreen(DtCtx *c)
{
    dtgpBegin(c, kGroups);
    randomScreenState();
}

static void setupBody(DtCtx *c)
{
    dtgpBegin(c, kGroups);
    randomScreenState();
    DT_ARG_INT(c, 0, (dt_u32() & 7) == 0 ? (int32_t)dt_u32() : dt_range(0, 480));
}

DT_CASES(overlymp_cases) = {
    {"overlympDrawHeader", 0x00412620, overlympDrawHeader, 1, DT_RET_VOID, setupHeader, 100, 0},
    {"overlympDrawModes", 0x004124c0, overlympDrawModes, 0, DT_RET_VOID, setupScreen, 300, 0},
    {"overlympModesUpdate", 0x00412670, overlympModesUpdate, 0, DT_RET_INT, setupScreen, 1500, 0},
    {"overlympDrawWordEntry", 0x00412850, overlympDrawWordEntry, 0, DT_RET_VOID, setupScreen, 300, 0},
    {"overlympWordEntryUpdate", 0x00412970, overlympWordEntryUpdate, 0, DT_RET_INT, setupScreen, 1500, 0},
    {"overlympDrawBody", 0x004134a0, overlympDrawBody, 1, DT_RET_VOID, setupBody, 600, 0},
    {"overlympResultsUpdate", 0x004136f0, overlympResultsUpdate, 0, DT_RET_INT, setupScreen, 1000, 0},
    {"overlympDrawResults", 0x004137d0, overlympDrawResults, 0, DT_RET_VOID, setupScreen, 2000, 0},
};
