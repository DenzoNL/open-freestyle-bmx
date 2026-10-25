/* Overlay screens of the multiplayer modes (C:\zaxis\Bmx\Src\com\action\overlymp.c). */
#include "bmx/com/action/overlymp.h"

#include "bmx/com/action/action.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zstring.h"

static const char overlympFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\overlymp.c";

/* Pad buttons as contrlerPressed masks. */
enum {
    kPadUp = 1,
    kPadDown = 2,
    kPadLeft = 4,
    kPadRight = 8,
    kPadCancel = 0x10010,
    kPadConfirm = 0x20080,
    kPadStart = 0x4000,
};

enum {
    kColorHeader = 0xf0f0f0,
    kColorText = 0x13040e0,       /* captions */
    kColorUnselected = 0x23040e0, /* menu line */
    kColorValue = 0xd0d0d0,       /* a score */
    kColorBlink = 0x2c05050,      /* the player's own new record, every other frame */
    kColorTurn = 0xe0a040,        /* the player whose turn it is */
    kColorOther = 0xd0d0d0,
    kCenterX = 0x13f,
    kValueRight = 0x24a,
    kLabelX = 0x35,
    kLineStep = 0x1d,
    kSectionStep = 0x23,
    kFirstLine = 0x6c,
};

/* Fields of a player record (kOvlPlayerStride bytes at gPlayerRecs) that the score screens show. */
enum {
    kPlayerScoreA = 0x5c,
    kPlayerScoreB = 0x60,
    kPlayerRounds = 0x68,
};

static uint32_t playerWord(uint32_t player, uint32_t offset)
{
    return *(const uint32_t *)((const uint8_t *)&gPlayerRecs + player * kOvlPlayerStride + offset);
}

/* x that centres a text of width w (the callee leaves stale bits above AX; only 16 bits are used). */
static int centreX(int16_t w)
{
    return kCenterX - (int)((uint16_t)w >> 1);
}

static void drawCentred(const void *font, const char *text, int y, uint32_t color)
{
    const int16_t w = zoverTextWidth(text, font);
    zoverDrawText(font, text, centreX(w), y, color, 0);
}

static void sound(int id)
{
    bmxSoundPlayEvent(id, 0, 0, 0, 0);
}

BMX_ADDR(0x00412620)
void overlympDrawHeader(const char *title)
{
    drawCentred(&gOvlFontSmall, title, 0x3c, kColorHeader);
}

/* Draws the selection lines: the first one names the mode and the current sub mode. */
BMX_ADDR(0x004124c0)
void overlympDrawModes(void)
{
    char buf[32];

    overlympDrawHeader(zstringGet(0x3000));
    int y = 0xf0 - (((int)gOvlMpModeLines * 20 - 20) >> 1);
    for (uint32_t i = 0; i < (uint32_t)gOvlMpModeLines - 1; i++) {
        const char *text;

        if (i == 0) {
            const char *sub = zstringGet((uint16_t)(0x300d + gOvlSavedSubMode));
            zstrFormat(buf, "%s - %s", zstringGet(0x3002), sub);
            text = buf;
        } else {
            text = zstringGet((uint16_t)(i + 0x3002));
        }
        uint32_t color = kColorUnselected;
        if (i == gOvlScreen->selected) {
            color = ovlPulseColor();
        }
        drawCentred(&gOvlFontTiny, text, y, color);
        y += 0x14;
    }
    ovlDrawMenuItems(2, gOvlScreen->itemCount - 1);
}

/* Last item: back to the pad sharing screen; item 0: left / right change the sub mode; confirm starts. */
BMX_ADDR(0x00412670)
int overlympModesUpdate(void)
{
    if (ovlPadConfirm((uint32_t)gOvlScreen->itemCount - 1) != 0 &&
        (uint32_t)gOvlScreen->selected - gOvlScreen->itemCount + 1 == 0) {
        gOvlReturnScreen = 0x16;
        ovlSetScreen(7);
        gOvlSharedPad = 0;
        sound(0x7d2);
        return 1;
    }
    for (uint32_t p = 0; p < gPlayerCount; p++) {
        if (contrlerPressed((int)p, kPadConfirm, 0) != 0 || contrlerPressed((int)p, kPadStart, 0) != 0) {
            gameSetSubMode((uint32_t)(int32_t)gOvlSavedSubMode);
            gameStartMode((int)gOvlScreen->selected + 1);
            sound(0x7d2);
            return 1;
        }
        if (contrlerPressed((int)p, kPadLeft, 0) != 0) {
            if ((uint32_t)gOvlScreen->selected + 1 == 1) {
                gOvlSavedSubMode--;
                if (gOvlSavedSubMode < 0) {
                    gOvlSavedSubMode = 3;
                }
                sound(0x7d1);
                return 1;
            }
        } else if (contrlerPressed((int)p, kPadRight, 0) != 0 && (uint32_t)gOvlScreen->selected + 1 == 1) {
            gOvlSavedSubMode++;
            if (gOvlSavedSubMode == 4) {
                gOvlSavedSubMode = 0;
            }
            sound(0x7d1);
            return 1;
        }
    }
    return 0;
}

/* The caption and the eight letters of the word, the one under the cursor pulsing. */
BMX_ADDR(0x00412850)
void overlympDrawWordEntry(void)
{
    char buf[32];
    const char *const title = zstringGet(0x304d);

    drawCentred(&gOvlFontSmall, title, 0x6c, kColorText);
    {
        const uint32_t len = (uint32_t)strlen(gOvlMpWord);
        int x = kCenterX - (int)((len * 53 - 53) >> 1);

        for (uint32_t i = 0; i < 8; i++) {
            uint32_t color;

            zstrFormat(buf, "%c", (int)gOvlMpWord[i]);
            color = i == gOvlMpWordCursor ? ovlPulseColor() : kColorText;
            zoverDrawText(&gOvlFontSmall, buf, x - ((uint16_t)zoverTextWidth(buf, &gOvlFontSmall) >> 1), 0xe2,
                          color, 0);
            x += 0x35;
        }
    }
}

/*
 * Letters of the word: up / down cycle the letter under the cursor through A..Z and a..z (wrapping between
 * the cases), left deletes it and steps back, right copies it forward; confirm starts the game with the
 * word, cancel goes back one screen.
 */
BMX_ADDR(0x00412970)
int overlympWordEntryUpdate(void)
{
    for (uint32_t p = 0; p < gPlayerCount; p++) {
        if (contrlerPressed((int)p, kPadConfirm, 0) != 0) {
            if (strlen(gOvlMpWord) != 0) {
                ovlSetScreen(0);
                gameSetHorseWord(gOvlMpWord);
                gameBigGameFunction(3);
                sound(0x7d2);
                return 1;
            }
            sound(0x7d4);
            return 1;
        }
        if (contrlerPressed((int)p, kPadCancel, 0) != 0) {
            ovlSetScreen(0xc);
            sound(0x7d3);
            return 1;
        }
        if (contrlerPressed((int)p, kPadLeft, 0) != 0) {
            if (gOvlMpWordCursor != 0) {
                const uint32_t at = gOvlMpWordCursor;
                gOvlMpWord[at] = 0;
                gOvlMpWordCursor = (uint8_t)(at - 1);
                sound(0x7d5);
            } else {
                sound(0x7d4);
            }
            gOvlPulse = 0;
            return 1;
        }
        if (contrlerPressed((int)p, kPadRight, 0) != 0) {
            if (gOvlMpWordCursor < 7) {
                const uint32_t at = gOvlMpWordCursor;
                gOvlMpWord[at + 1] = gOvlMpWord[at];
                gOvlMpWordCursor = (uint8_t)(at + 1);
                sound(0x7d2);
            } else {
                sound(0x7d4);
            }
            gOvlPulse = 0;
            return 1;
        }
        if (contrlerPressed((int)p, kPadUp, 0) != 0) {
            char *const c = &gOvlMpWord[gOvlMpWordCursor];
            if (*c == 'Z') {
                *c = 'a';
            } else if (*c == 'z') {
                *c = 'A';
            } else {
                *c = (char)(*c + 1);
            }
            sound(0x7d1);
            gOvlPulse = 0;
            return 1;
        }
        if (contrlerPressed((int)p, kPadDown, 0) != 0) {
            char *const c = &gOvlMpWord[gOvlMpWordCursor];
            if (*c == 'A') {
                *c = 'z';
            } else if (*c == 'a') {
                *c = 'Z';
            } else {
                *c = (char)(*c - 1);
            }
            sound(0x7d1);
            gOvlPulse = 0;
            return 1;
        }
    }
    return 0;
}

/* Rounds the player has taken part in (compared with the round count of the game). */
static uint32_t scoreOf(uint32_t index)
{
    return playerWord(index, kPlayerRounds);
}

/*
 * One line under a score screen's title: who is playing (or "p1-p2" results in the 2 player competition),
 * or the competition and the round (the caption of the "best" line).
 */
BMX_ADDR(0x004134a0)
void overlympDrawBody(int y)
{
    char buf[32];

    if (gameTestOption(8) == 0) {
        const uint32_t round = gameGetRoundState(gameGetRunCount(), 1);
        uint32_t id;

        if (gameTestOption(1) == 0) {
            id = 0x1017;
        } else {
            id = (scoreOf(round) != gameGetRoundCount()) + 0x1015;
        }
        {
            const char *const a = zstringGet((uint16_t)id);
            const char *const b = zstringGet((uint16_t)(round + 0x100c));
            const char *const c = zstringGet(0x1000);
            zstrFormat(buf, "%s %s %s ", c, b, a);
        }
    } else {
        zstrFormat(buf, "%s ", zstringGet(0x1018));
    }
    if (gPlayerCount == 2 && gameTestOption(8) == 0) {
        const uint32_t round = gameGetRoundState(gameGetRunCount(), 1);
        zstrFormat(buf + strlen(buf), "%ld-%ld", (long)scoreOf(round), (long)scoreOf(round == 0));
    } else {
        for (uint32_t p = 0; p < gPlayerCount; p++) {
            zstrFormat(buf + strlen(buf), "%s%ld", p == 0 ? (const char *)gOvlEmpty : "-", (long)scoreOf(p));
        }
    }
    drawCentred(&gOvlFontSmall, buf, y, kColorText);
}

/* The menu after a run: item 0 plays on, item 1 leaves for a rematch when allowed, item 2 goes back. */
BMX_ADDR(0x004136f0)
int overlympResultsUpdate(void)
{
    int rematch = 0;

    if (gameGetMode() == 3 && gameTestOption(4) == 0) {
        rematch = 1;
    }
    if (ovlPadConfirm(0) == 0) {
        return 0;
    }
    switch (gOvlScreen->selected) {
    case 0:
        ovlSetScreen(0);
        gameBigGameFunction(4);
        break;
    case 1:
        if (rematch) {
            ovlSetScreen(0);
            actionSetMode(2);
            gDemoPlayer = gCurrentPlayer;
            break;
        }
        /* fall through */
    case 2:
        gOvlReturnScreen = 0x18;
        ovlSetScreen(7);
        break;
    default:
        break;
    }
    sound(0x7d2);
    return 1;
}

/* A caption at the left and a score right aligned; blink != 0 colours the score as a new record. */
static void drawScoreLine(int y, uint16_t label, uint32_t value, uint32_t color)
{
    char buf[32];

    zstrFormat(buf, "%s:", zstringGet(label));
    zoverDrawText(&gOvlFontSmall, buf, kLabelX, y, kColorText, 0);
    zstrFormat(buf, "%ld", (long)value);
    zoverDrawText(&gOvlFontSmall, buf, kValueRight - zoverTextWidth(buf, &gOvlFontSmall), y, color, 0);
}

/* Colour of a record's score: the blink colour while it is the current player's fresh record. */
static uint32_t recordColor(const uint8_t *rec, int ownPlayerOnly)
{
    if ((rec[5] & 1) != 0 && (!ownPlayerOnly || rec[4] == gCurrentPlayer) && (gOvlFrame & 1) != 0) {
        return kColorBlink;
    }
    return kColorValue;
}

/* The record table entry of the running mode (`extra` selects the table), asserting that it exists. */
static const uint8_t *modeRecord(uint32_t extra, int line)
{
    const uint32_t mode = gameGetMode();
    const uint8_t *rec = gameGetRecord(((mode << 2 | gameGetSubMode()) << 0x16) | extra);

    if (rec == NULL) {
        zdebugFail((const char *)gOvlEmpty, overlympFile, line, 0);
    }
    return rec;
}

static void drawPlayerTitle(int y)
{
    char buf[32];
    const char *const player = zstringGet((uint16_t)(0x100c + gCurrentPlayer));

    zstrFormat(buf, "%s %s", zstringGet(0x1000), player);
    drawCentred(&gOvlFontSmall, buf, y, kColorText);
}

/* Score screen after a run; one layout per game mode (1: free score, 2: competition, 3: names, 4: HORSE). */
BMX_ADDR(0x004137d0)
void overlympDrawResults(void)
{
    const uint32_t cur = gCurrentPlayer;
    int y;
    const uint8_t *rec;

    switch (gameGetMode()) {
    case 1:
        overlympDrawHeader(zstringGet(0x3050));
        y = kFirstLine;
        drawPlayerTitle(y);
        y += kSectionStep;
        rec = modeRecord(0, 0x270);
        drawScoreLine(y, 0x1001, playerWord(cur, kPlayerScoreA), recordColor(rec, 1));
        y += kLineStep;
        drawScoreLine(y, 0x100b, *(const uint32_t *)rec, recordColor(rec, 1));
        break;
    case 2:
        rec = modeRecord(0x10000, 0x29e);
        if (gameTestOption(2) == 0) {
            overlympDrawHeader(zstringGet(0x3050));
            y = kFirstLine;
            drawPlayerTitle(y);
            y += kSectionStep;
            drawScoreLine(y, 0x1001, playerWord(cur, kPlayerScoreA), recordColor(rec, 1));
            y += kLineStep;
            drawScoreLine(y, 0x100b, *(const uint32_t *)rec, recordColor(rec, 1));
            y += kSectionStep;
            overlympDrawBody(y);
        } else if (gameTestOption(0x10) == 0) {
            overlympDrawHeader(zstringGet(0x3052));
            y = kFirstLine;
            for (uint32_t p = 0; p < gPlayerCount; p++) {
                char buf[32];
                uint32_t color;

                zstrFormat(buf, "%s %s:", zstringGet(0x1000), zstringGet((uint16_t)(0x100c + p)));
                zoverDrawText(&gOvlFontSmall, buf, kLabelX, y, kColorText, 0);
                zstrFormat(buf, "%ld", (long)playerWord(p, kPlayerScoreA));
                color = p != gameGetRoundState(gameGetRunCount(), 2) ? kColorOther : kColorTurn;
                if ((rec[5] & 1) != 0 && rec[4] == p && (gOvlFrame & 1) != 0) {
                    color = kColorBlink;
                }
                zoverDrawText(&gOvlFontSmall, buf, kValueRight - zoverTextWidth(buf, &gOvlFontSmall), y,
                              color, 0);
                y += kLineStep;
            }
            {
                char buf[32];
                uint32_t color = kColorValue;

                zstrFormat(buf, "%s:", zstringGet(0x100b));
                zoverDrawText(&gOvlFontSmall, buf, kLabelX, y, kColorText, 0);
                zstrFormat(buf, "%ld", (long)*(const uint32_t *)rec);
                if ((rec[5] & 1) != 0 && (gOvlFrame & 1) != 0) {
                    color = kColorBlink;
                }
                zoverDrawText(&gOvlFontSmall, buf, kValueRight - zoverTextWidth(buf, &gOvlFontSmall), y,
                              color, 0);
            }
            y += kSectionStep;
            overlympDrawBody(y);
        } else {
            overlympDrawHeader(zstringGet(0x1019));
            y = kFirstLine;
            drawCentred(&gOvlFontSmall, zstringGet(0x3051), y, kColorText);
            y += kSectionStep;
            overlympDrawBody(y);
        }
        break;
    case 3:
        overlympDrawHeader(zstringGet(0x3050));
        y = kFirstLine;
        if (gameTestOption(2) != 0) {
            ovlDrawPlayerNames(&y);
        }
        break;
    case 4:
        overlympDrawHeader(zstringGet(0x3050));
        y = kFirstLine;
        drawPlayerTitle(y);
        y += kSectionStep;
        rec = modeRecord(0x10000, 0x33c);
        drawScoreLine(y, 0x1001, playerWord(cur, kPlayerScoreA), recordColor(rec, 1));
        y += kLineStep;
        drawScoreLine(y, 0x100b, *(const uint32_t *)rec, recordColor(rec, 1));
        y += kLineStep;
        rec = modeRecord(0, 0x367);
        drawScoreLine(y, 0x1003, playerWord(cur, kPlayerScoreB), recordColor(rec, 1));
        y += kLineStep;
        drawScoreLine(y, 0x100b, *(const uint32_t *)rec, recordColor(rec, 1));
        break;
    default:
        break;
    }
    ovlDrawMenuItems(1, 0);
}

/* Like drawCentred, around the column cx. */
static void drawCentredAt(const void *font, const char *text, int cx, int y, uint32_t color)
{
    const int16_t w = zoverTextWidth(text, font);
    zoverDrawText(font, text, cx - (int)((uint16_t)w >> 1), y, color, 0);
}

/* A caption at the left of a stats line. */
static void drawCaption(int y, uint16_t label)
{
    char buf[256];

    zstrFormat(buf, "%s:", zstringGet(label));
    zoverDrawText(&gOvlFontSmall, buf, kLabelX, y, kColorText, 0);
}

/* A value right aligned in the small font; the timed modes (6..10) show a length in feet and inches. */
static void drawStatsValue(int y, uint32_t value, int isLength)
{
    char buf[256];

    if (isLength) {
        ovlFormatFeetInches(buf, (float)value);
    } else {
        zstrFormat(buf, "%ld", (long)value);
    }
    zoverDrawText(&gOvlFontSmall, buf, kValueRight - zoverTextWidth(buf, &gOvlFontSmall), y, kColorValue, 0);
}

/* The tricks the next rider of the HORSE game must repeat: name in the tiny font on the left, the turn and
 * barspin captions in the button font on the right, "to" between the lines. */
static void drawHorseTricks(int y)
{
    char buf[256];

    for (uint32_t i = 0; i < gGameHorseCount; i++) {
        const RiderTrick *t = &gGameHorseTricks[i];

        ovlFormatTrickName(buf, &t->flags, 0);
        drawCentredAt(&gOvlFontTiny, buf, 0xba, gOvlFontButton.height / 2 + y - 8, 0x2808080);
        buf[0] = '\0';
        if (t->trick != 0xff) {
            zstrFormat(buf + strlen(buf), "%s", zstringGet((uint16_t)(0x10bb + t->trick)));
        }
        if (t->style != 0xff) {
            if (t->trick != 0xff) {
                zstrFormat(buf + strlen(buf), " ");
            }
            zstrFormat(buf + strlen(buf), "%s", zstringGet((uint16_t)(0x10e9 + t->style)));
        }
        drawCentredAt(&gOvlFontButton, buf, 0x1c4, y, 0x808080);
        if (i < (uint32_t)gGameHorseCount - 1) {
            zstrFormat(buf, "to");
            drawCentredAt(&gOvlFontTiny, buf, 0xba, gOvlFontButton.height + y - 8, 0x2a07010);
        }
        y += gOvlFontButton.height;
    }
}

/*
 * Run statistics screen of the HORSE / competition / timed modes: the player's title, the score of the
 * run, the best of the mode and (HORSE) the tricks to repeat.
 */
BMX_ADDR(0x00412bf0)
void overlympDrawRunStats(void)
{
    const uint32_t cur = gCurrentPlayer;
    char buf[256];
    int y = 0x8f;
    uint32_t mode;

    switch (gameGetMode()) {
    case 2:
    case 4: {
        const long run = (long)gameGetRunCount() + 1;
        zstrFormat(buf, "%s %ld", zstringGet(0x1004), run);
        overlympDrawHeader(buf);
    } break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10: {
        const uint32_t rounds = gameGetRoundCount();
        uint16_t id = 0x1014;
        const char *a;

        if (gameGetRunCount() < ((rounds - 1) & 0xffff)) {
            id = (uint16_t)(gameGetRunCount() + 0x1010);
        }
        a = zstringGet(0x1005);
        zstrFormat(buf, "%s %s", zstringGet(id), a);
        overlympDrawHeader(buf);
        break;
    }
    default:
        overlympDrawHeader(zstringGet(0x304e));
        break;
    }
    drawPlayerTitle(kFirstLine);

    mode = gameGetMode();
    if (mode == 3) {
        if (gameTestOption(4) == 0) {
            char text[256];

            zstrFormat(text, "%s:", zstringGet(0x304f));
            drawCentred(&gOvlFontSmall, text, y, kColorText);
            drawHorseTricks(0xac);
        }
    } else {
        if (mode == 4) {
            drawCaption(y, 0x1001);
            drawStatsValue(y, playerWord(cur, kPlayerScoreB), 0);
            y = 0xac;
        }
        if (gameTestOption(4) == 0 || (gameGetMode() != 2 && gameGetRunCount() != 0)) {
            uint32_t key;
            const uint32_t *rec;

            if (gameGetMode() == 2) {
                const uint32_t m = gameGetMode();
                key = ((m << 2 | gameGetSubMode()) << 0x16) | 0x12000;
            } else {
                const uint32_t m = gameGetMode();
                const uint32_t sub = gameGetSubMode();
                key = (((m << 2 | sub) << 6 | gScoreRecordSlot[gameGetMode()]) << 0x10) | 0x1000;
            }
            rec = (const uint32_t *)gameGetRecord(key);
            if (rec == NULL) {
                zdebugFail((const char *)gOvlEmpty, overlympFile, 0x195, 0);
            }
            drawCaption(y, 0x100a);
            mode = gameGetMode();
            drawStatsValue(y, *rec, mode >= 6 && mode <= 10);
            y += kLineStep;
        }
        {
            const uint32_t m = gameGetMode();
            const uint32_t sub = gameGetSubMode();
            const uint32_t key = ((m << 2 | sub) << 6 | gScoreRecordSlot[gameGetMode()]) << 0x10;
            const uint32_t *rec = (const uint32_t *)gameGetRecord(key);

            if (rec == NULL) {
                zdebugFail((const char *)gOvlEmpty, overlympFile, 0x1b8, 0);
            }
            drawCaption(y, 0x100b);
            mode = gameGetMode();
            drawStatsValue(y, *rec, mode >= 6 && mode <= 10);
        }
        if (gameGetMode() == 2) {
            overlympDrawBody(y + kSectionStep);
        }
    }
    if (gameGetMode() == 3 && gameTestOption(4) == 0) {
        ovlDrawMenuItems(1, 0);
    } else {
        ovlDrawMenuItems(0, 0);
    }
}
