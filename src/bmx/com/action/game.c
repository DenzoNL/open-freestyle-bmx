/* Game session logic (C:\zaxis\Bmx\Src\com\action\game.c). */
#include "bmx/com/action/game.h"

#include "bmx/com/action/camera.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/fx.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/prop.h"
#include "bmx/com/action/replay.h"
#include "bmx/com/action/rider.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/ngif/ngiffx.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzres.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zquat.h"
#include "zge/zstring.h"

/*
 * Floating point follows the original x87 code exactly: `double` is a value held in an x87
 * register (53-bit precision control), `float` is a value the original stored to memory.
 */

static const char gameFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\game.c";
static const char kGoalLevelError[] = "improper level for objective logic";

enum {
    kModeFreeRide = 0,
    kModeCompetition = 2,
    kModeCompetitionLast = 5,
    kModeNone = 0xb,
    kRunTypeSession = 4,
    kRunTypeReplay = 6,
    kPhaseEnd = 5,
    kSoundCountdown = 5000,
    kSoundRunEnd = 0x3e9,
    kSoundRunStart = 1000,
    kSoundRecord = 0x1391,
    kElementTimer = 2,
    kElementRecord = 0xc,
    kElementBail = 7,
    kElementHud = 8,
    kStatusActive = 2,
    kStatusDone = 4,
};

/* Level -> index into the level info table of ngifzres (0x551fb8). */
typedef uint8_t GameLevelInfoIndex[12];
#define gGameLevelInfoIndex ORIG_VAR(0x00551fb8, GameLevelInfoIndex, gGameLevelInfoIndex)

/* ---- small accessors ----------------------------------------------------------------------------- */

/* (options & mask) != 0, where options is the byte at 0x56b700. */
BMX_ADDR(0x0040a890)
int gameTestOption(uint32_t mask)
{
    return (gGameOptions & (uint8_t)mask) != 0;
}

BMX_ADDR(0x0040a850)
void gameSetOption(uint32_t mask, int on)
{
    if (on != 0) {
        gGameOptions = (uint8_t)(gGameOptions | (uint8_t)mask);
    } else {
        gGameOptions = (uint8_t)(gGameOptions & (uint8_t)~(uint8_t)mask);
    }
}

/* Starts game mode `mode`: clears the scores and records of the mode's tables and returns the mode. */
BMX_ADDR(0x0040a8c0)
uint32_t gameStartMode(int mode)
{
    gGameOptions = 0;
    gGameMode = (uint8_t)mode;
    gGameRunCount = 0;
    gGameRunType = 0xff;
    gGameStartPlayer = 0;
    gCurrentPlayer = (uint8_t)gameGetStartPlayer();
    for (int i = 0; i < 2; i++) {
        gPhysRiders[i].combo.total = 0;
        gPhysRiders[i].combo.landed = 0;
    }
    for (uint32_t slot = 0; slot < 10; slot++) {
        for (uint32_t run = 0; run < 3; run++) {
            const uint32_t sub = gameGetSubMode();
            uint8_t *rec = gameGetRecord((((sub | (uint32_t)(mode * 4)) << 6 | slot) << 16) | run | 0x1000);
            if (rec != NULL) {
                *(uint32_t *)rec = 0;
                rec[5] = 0;
            }
        }
    }
    for (uint32_t slot = 0; slot < 4; slot++) {
        for (uint32_t run = 0; run < 3; run++) {
            const uint32_t sub = gameGetSubMode();
            uint8_t *rec = gameGetRecord((((sub | (uint32_t)(mode * 4)) << 10 | slot) << 12) | run);
            if (rec != NULL) {
                rec[5] = 0;
            }
        }
    }
    gameSetOption(8, mode != 0);
    gameBigGameFunction(1);
    return gGameMode;
}

/* Current game mode: 0 free ride, 2 and 3 are the competition modes, 6..10 timed. */
BMX_ADDR(0x0040a9f0)
uint32_t gameGetMode(void)
{
    return gGameMode;
}

BMX_ADDR(0x0040aa10)
uint32_t gameSetSubMode(uint32_t subMode)
{
    gGameSubMode = (uint8_t)subMode;
    return subMode & 0xff;
}

BMX_ADDR(0x0040aa30)
uint32_t gameGetSubMode(void)
{
    return gGameSubMode;
}

BMX_ADDR(0x0040bd50)
uint32_t gameGetRunType(void)
{
    return gGameRunType;
}

/* Rounds of the current mode: free ride levels past the sixth have two, the rest come from a table. */
BMX_ADDR(0x0040bd70)
uint32_t gameGetRoundCount(void)
{
    if (gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel >= 6) {
        return 2;
    }
    return gGameRoundCounts[gameGetMode()];
}

BMX_ADDR(0x0040bdc0)
uint32_t gameGetRunCount(void)
{
    return gGameRunCount;
}

BMX_ADDR(0x0040bde0)
uint32_t gameGetStartPlayer(void)
{
    return gGameStartPlayer;
}

/* State of round `round` of the current competition; kinds 0 and 1 share one cell. */
BMX_ADDR(0x0040be00)
uint32_t gameGetRoundState(uint32_t round, uint32_t kind)
{
    if (kind < 2) {
        return gGameRoundStates[15];
    }
    if (gameGetRoundCount() <= round) {
        zdebugFail("round number out of range", gameFile, 0x59a, 0);
    }
    return gGameRoundStates[round];
}

/*
 * Score record by key (mode << 24 | player << 22 | slot << 16 | run | ... ): returns a pointer to
 * {u32 value; u8 rank; u8 flags; ...} or NULL for a key that names no table.
 */
BMX_ADDR(0x0040be50)
uint8_t *gameGetRecord(uint32_t key)
{
    const uint32_t slot = key >> 16 & 0x3f;
    const uint32_t player = key >> 22 & 3;
    const uint32_t group = key >> 12 & 0xf;
    uint8_t *base = gGameRecords;

    if (slot > 1) {
        return base + 0x70 + (slot + group * 0x18) * 8;
    }
    switch (key >> 24) {
    case 0:
        return base + ((key & 0xfff) + group * 0x18) * 8;
    case 1:
        return base + 0x18 + (player + group * 0x18) * 8;
    case 2:
        return slot == 1 ? base + 0x38 + group * 0xc0 : NULL;
    case 3:
        return slot == 1 ? base + 0x40 + group * 0xc0 : NULL;
    case 4:
        if (slot == 0) {
            return base + 0x50 + group * 0xc0;
        }
        return slot == 1 ? base + 0x48 + group * 0xc0 : NULL;
    case 5:
        return slot == 1 ? base + 0x58 + group * 0xc0 : NULL;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        return base + 0x70 + (gScoreRecordSlot[key >> 24] + group * 0x18) * 8;
    case 11:
        return base + 0x60 + (player + group * 0x18) * 8;
    default:
        return NULL;
    }
}

/* Sets the word the horse game spells (at most 8 characters) and returns the buffer. */
BMX_ADDR(0x0040bfd0)
char *gameSetHorseWord(const char *word)
{
    const size_t len = strlen(word);
    if (len > 8) {
        zdebugFail("horse word too long", gameFile, 0x5e4, 0);
    }
    memcpy(gGameHorseWord, word, len + 1);
    return gGameHorseWord;
}

BMX_ADDR(0x0040c050)
const char *gameGetNameTable(void)
{
    return gGameHorseWord;
}

/* The replay recorder of the current mode (original returns only AL). */
BMX_ADDR(0x0040c070)
uint32_t gameGetModeByte(void)
{
    if (gameGetMode() == 0) {
        return gGameReplays[0];
    }
    return gameGetRunType() == kRunTypeReplay ? gGameReplayAll : gGameReplayShared;
}

/* Leaves the current run: back to the HUD, or on to the run end for the replay screens. */
BMX_ADDR(0x0040c0b0)
void gameLeaveRun(void)
{
    const uint32_t type = gameGetRunType();
    if (type != kRunTypeSession) {
        if (type < 4 || type > kRunTypeReplay) {
            ovlSetScreen(0);
            return;
        }
        cameraSelectPlayer(gCurrentPlayer);
        comdataResetView(gCurrentPlayer, 0);
    }
    const uint8_t state = gPhysRiders[gCurrentPlayer].combo.runState;
    if (state != 0) {
        if (state < 3) {
            gameEndRun(gCurrentPlayer, 0);
        } else if (state == 3) {
            gameEndRun(gCurrentPlayer, 1);
        }
    }
    gameBigGameFunction(kPhaseEnd);
    bmxSoundPlayEvent(kSoundRunEnd, 4, 0, 0, 0);
    bmxSoundPlayEvent(kSoundRunStart, 2, 0, 0, 0);
}

/* Empty (debug output stripped from the shipped build). */
BMX_ADDR(0x0040cf50)
void gameNop(void) {}

/* ---- session start and shutdown ----------------------------------------------------------------- */

/* (x + 3) / 4 as the compiler emits it for a signed 32 bit value. */
static int32_t divRound4(int32_t x)
{
    const int32_t v = (int32_t)((uint32_t)x + 3u);
    return (int32_t)((uint32_t)v + (uint32_t)((v >> 31) & 3)) >> 2;
}

/* Creates the replay recorders of the players and loads the stored high scores into the records. */
BMX_ADDR(0x004099c0)
void gameInit(void)
{
    ReplayParams params = {0};

    if (gPlayerCount == 0) {
        gGameMode = 0;
        gGameRunType = 4;
    } else {
        for (uint32_t i = 0; i < gPlayerCount; i++) {
            params.flags = 1;
            params.size = 0x1000 / (int32_t)gPlayerCount;
            if (gPlayerCount > 1) {
                params.flags = 7;
            }
            params.index = (uint8_t)i;
            gGameReplays[i] = replayCreate(&params);
            if (gGameReplays[i] == 0xff) {
                zdebugFail("Failed to create replay (probably out of RAM).", gameFile, 0xde, 0);
            }
            gGameRiders[gProfiles[i].character].flags &= 0xfffffe3fu;
        }
        if (gPlayerCount == 1) {
            gGameReplayAll = gGameReplays[0];
        } else {
            params.flags = 7;
            params.size = 0x1000 / (int32_t)gPlayerCount;
            params.index = 0;
            gGameReplayAll = replayCreate(&params);
            if (gGameReplayAll == 0xff) {
                zdebugFail("Failed to create replay (probably out of RAM).", gameFile, 0xea, 0);
            }
        }
        gGameReplayShared = gGameReplays[0];
        *(uint32_t *)gGameHorseWord = gGameDefaultWord;
        gGameMode = 0xff;
    }
    gGameSubMode = 0;

    for (uint32_t a = 0; a < 12; a++) {
        for (uint32_t b = 0; b < 4; b++) {
            for (uint32_t c = 0; c < 10; c++) {
                for (uint32_t d = 0; d < 3; d++) {
                    const uint32_t key = (((a * 4 | b) << 6 | c) << 16) | d;
                    uint8_t *rec = gameGetRecord(key);
                    if (rec != NULL) {
                        const uint32_t *stored = comdataGetLevelRecord(key);
                        if (stored == NULL) {
                            zdebugFail("no stored high score entry found", gameFile, 0xfe, 0);
                        }
                        const uint32_t value = *stored;
                        rec[5] = 0;
                        *(uint32_t *)rec = value;
                    }
                }
            }
        }
    }
    const float *bounds = zrollerGetWorldBounds();
    gGameExtent[0] = (uint16_t)divRound4(bmxFtol((double)bounds[4] - (double)bounds[0]));
    gGameExtent[1] = (uint16_t)divRound4(bmxFtol((double)bounds[6] - (double)bounds[2]));
    zrollerAddDrawCallback(BMX_FNPTR(ZrollerDrawFn, 0x0040a400, gameDrawGoalMarkers));
}

/* Frees the replay recorders. */
BMX_ADDR(0x00409cd0)
void gameShutdown(void)
{
    if (gPlayerCount != 0) {
        if (gPlayerCount != 1) {
            replayDestroy(gGameReplayAll);
        }
        for (uint32_t i = gPlayerCount; (int32_t)(i - 1) >= 0; i--) {
            replayDestroy(gGameReplays[i - 1]);
        }
    }
}

/* ---- run phases ---------------------------------------------------------------------------------- */

/* Plays the countdown tick at every whole second of the last five seconds of the run timer. */
static void countdownSound(uint32_t timer)
{
    if (timer <= 0x96 && timer % 30 == 0) {
        bmxSoundPlayEvent(kSoundCountdown, (int)(timer / 30), 0, 0, 0);
    }
}

static void showTimer(void)
{
    uint32_t value = gGameTimer;
    ovlSetElementA(kElementTimer, &value);
}

/* Phase 5 ends the run; otherwise nothing more happens this frame. */
static void finishPhase(void)
{
    if (gGamePhase == kPhaseEnd) {
        gameBigGameFunction(kPhaseEnd);
    }
}

static void endRunPhase(void)
{
    gGamePhase = kPhaseEnd;
    gameBigGameFunction(kPhaseEnd);
}

/*
 * Per frame session logic of the current player: feeds the HUD, counts the run timer down and walks
 * the run phases (0 counting down, 1 last moment, 2 and 3 waiting for the rider to finish, 4 delay,
 * 5 over). Competition modes (mode != 0) and the free ride modes use different phase rules.
 */
BMX_ADDR(0x00409d20)
void gameTick(void)
{
    PhysRider *r = &gPhysRiders[gCurrentPlayer];

    ngifzresLoadLevelInfo(gGameLevelInfoIndex[gCurrentLevel]);
    gameUpdateRun(r->player);
    if (gameGetMode() == 0) {
        gameAwardGoals(r->player);
    }
    gameEvaluateRun(r->player);
    uint8_t hud = r->hudByte;
    ovlSetElementA(kElementHud, &hud);
    uint8_t bail = riderGetStateLevel(r, 0) == kRiderBail ? r->bailReason : 0xff;
    ovlSetElementA(kElementBail, &bail);

    const uint32_t mode = gameGetMode();
    if (mode != 0) {
        if (mode == kModeNone || gameGetRunType() != kRunTypeSession) {
            return;
        }
        if (gGamePhase < 2) {
            if (cameraPlaybackActive() == 0 && gGameTimer != 0) {
                gGameTimer--;
                countdownSound(gGameTimer);
            }
            showTimer();
        }
        const uint8_t state = r->combo.runState;
        switch (gGamePhase) {
        case 0:
            if (gGameTimer == 0 || state == 1 || state == 2) {
                gGamePhase = 1;
                return;
            }
            break;
        case 1:
            if (gGameTimer == 0) {
                if (state == 1 || state == 2) {
                    gGamePhase = 2;
                    return;
                }
                if (state != 0) {
                    gameEndRun(gCurrentPlayer, 1);
                }
                endRunPhase();
                return;
            }
            const uint32_t current = gameGetMode();
            if (current >= kModeCompetition && current <= kModeCompetitionLast && state != 1 && state != 2) {
                gGamePhase = 2;
                return;
            }
            break;
        case 2:
            if (state == 0 || state > 3) {
                gGamePhase = 4;
                gGamePhaseTicks = 0;
                return;
            }
            if (state == 3) {
                gGamePhase = 3;
                return;
            }
            break;
        case 3:
            if (state == 0) {
                endRunPhase();
                return;
            }
            break;
        case 4:
            gGamePhaseTicks++;
            if (gGamePhaseTicks == 0x3c) {
                endRunPhase();
                return;
            }
            break;
        default:
            break;
        }
        finishPhase();
        return;
    }

    if (gameGetRunType() != kRunTypeSession) {
        return;
    }
    if (gameGetSubMode() != 2) {
        if (gGamePhase < 2 && cameraPlaybackActive() == 0 && gGameTimer != 0 && gGameFrozen == 0) {
            gGameTimer--;
            countdownSound(gGameTimer);
        }
        showTimer();
    }
    const uint8_t phase = gGamePhase;
    uint8_t state = r->combo.runState;
    switch (phase) {
    case 0:
        if (gGameTimer == 0 || state == 1 || state == 2) {
            gGamePhase = 1;
            return;
        }
        break;
    case 1:
        if (gGameTimer != 0) {
            break;
        }
        if (state == 1 || state == 2 || physGetState(r) == kPhysStateAir) {
            gGamePhase = 2;
            return;
        }
        if (r->combo.runState != 0) {
            gameEndRun(gCurrentPlayer, 1);
        }
        gGamePhase = 3;
        gameBigGameFunction(kPhaseEnd);
        bmxSoundPlayEvent(kSoundRunEnd, 4, 0, 0, 0);
        return;
    case 2:
        if (state != 0) {
            break;
        }
        gGamePhase = 3;
        gameBigGameFunction(kPhaseEnd);
        bmxSoundPlayEvent(kSoundRunEnd, 4, 0, 0, 0);
        return;
    default:
        break;
    }
    if (phase == 3) {
        gameBigGameFunction(kPhaseEnd);
        bmxSoundPlayEvent(kSoundRunEnd, 4, 0, 0, 0);
    }
}

/* ---- goal bookkeeping ---------------------------------------------------------------------------- */

static GameGoalInfo *goalInfo(int player, uint32_t level)
{
    return (GameGoalInfo *)(void *)comdataGetGoalInfo(player, level);
}

/* The byte of the goal rows that holds the status nibble of `goal` (it may reach into the next row). */
static uint8_t *goalStatusByte(GameGoalInfo *info, int row, uint32_t goal)
{
    return (uint8_t *)info + 4 + (goal >> 1) + (row + info->set * 4) * 3;
}

/* Status 0..5 of a goal: 2 active, 3 pending, 4 done, 5 collected. */
BMX_ADDR(0x004102c0)
uint32_t gameGoalStatus(int player, int row, uint32_t goal)
{
    if (gCurrentLevel > 5) {
        zdebugFail(kGoalLevelError, gameFile, 0xe9e, 0);
    }
    GameGoalInfo *info = goalInfo(player, gCurrentLevel);
    return (uint32_t)(*goalStatusByte(info, row, goal) >> ((goal & 1) << 2)) & 7;
}

/* Sets the status of a goal; status 5 also collects the props the goal unlocks. */
BMX_ADDR(0x00410350)
void gameSetGoalStatus(int player, int row, uint32_t goal, int status)
{
    if (gCurrentLevel > 5) {
        zdebugFail(kGoalLevelError, gameFile, 0xeb6, 0);
    }
    GameGoalInfo *info = goalInfo(player, gCurrentLevel);
    uint8_t *byte = goalStatusByte(info, row, goal);
    const int shift = (int)((goal & 1) << 2);
    *byte = (uint8_t)((~(0xf << shift) & *byte) | ((int8_t)status << shift));
    if (status == 5) {
        const uint8_t *props =
            (const uint8_t *)&gGameGoals[gCurrentLevel * 3 + info->set][row] + 0x14 + goal * 0x14;
        for (uint32_t k = 0; k < 4; k++) {
            if (props[k] != 0xff) {
                uint8_t *prop = propFind(props[k]);
                if (prop != NULL) {
                    propCollect((uint32_t)player, prop);
                }
            }
        }
    }
}

/* Bit 3 of a goal's status nibble ANDed with `mask` (the byte of the mask is used). */
BMX_ADDR(0x00410480)
uint32_t gameGoalFlag(int player, int row, uint32_t goal, uint32_t mask)
{
    if (gCurrentLevel > 5) {
        zdebugFail(kGoalLevelError, gameFile, 0xeda, 0);
    }
    GameGoalInfo *info = goalInfo(player, gCurrentLevel);
    return (uint32_t)(*goalStatusByte(info, row, goal) >> (((goal & 1) * 4) + 3)) & 1 & (uint8_t)mask;
}

/* Sets or clears the flag (shifted into bit 3 of the goal's nibble) in the goal's status byte. */
BMX_ADDR(0x00410510)
void gameSetGoalFlag(int player, int row, uint32_t goal, uint32_t flag, int on)
{
    if (gCurrentLevel > 5) {
        zdebugFail(kGoalLevelError, gameFile, 0xef0, 0);
    }
    GameGoalInfo *info = goalInfo(player, gCurrentLevel);
    uint8_t *byte = goalStatusByte(info, row, goal);
    const int bit = (int8_t)flag << (((goal & 1) * 4) + 3);
    if (on != 0) {
        *byte |= (uint8_t)bit;
    } else {
        *byte &= (uint8_t)~bit;
    }
}

/* Resets the goals of the current level for a player: all goals open again, or their props released. */
BMX_ADDR(0x0040cd40)
void gameResetGoals(int player)
{
    GameGoalInfo *info = goalInfo(player, gCurrentLevel);

    info->set = 0;
    do {
        info->done = 0;
        const GameGoalRow *row = gGameGoals[gCurrentLevel * 3 + info->set];
        for (uint32_t r = 0; r < 4; r++, row++) {
            uint8_t *flags = &info->row[info->set * 4 + r][0];
            if ((*flags & 0x80) == 0) {
                for (uint32_t g = 0; g < row->count; g++) {
                    gameSetGoalStatus(player, (int)r, g, 0);
                    gameSetGoalFlag(player, (int)r, g, 1, 1);
                }
                *flags = 0;
            } else {
                info->done++;
                for (uint32_t g = 0; g < row->count; g++) {
                    for (uint32_t k = 0; k < 4; k++) {
                        const uint8_t id = row->goal[g].prop[k];
                        uint8_t *prop = id != 0xff ? propFind(id) : NULL;
                        if (prop != NULL) {
                            propRelease((uint32_t)player, prop);
                        }
                    }
                }
                for (uint32_t k = 0; k < 3; k++) {
                    const uint8_t id = row->prop[k];
                    uint8_t *prop = id != 0xff ? propFind(id) : NULL;
                    if (prop != NULL) {
                        propRelease((uint32_t)player, prop);
                    }
                }
            }
            *flags &= 0xbf;
        }
    } while (info->done > 3 && ++info->set < 3);
    if (info->set > 1) {
        info->set = 2;
    }
}

/* Non-zero when the player has an unlock waiting to be shown. */
BMX_ADDR(0x0040cf70)
int gameHasPendingUnlock(int player)
{
    if (gameGetMode() == 0 && gameGetSubMode() == 0) {
        if (gCurrentLevel < 6) {
            const GameGoalInfo *info = goalInfo(player, gCurrentLevel);
            if (info->done == 4) {
                for (uint32_t i = 0; i < 4; i++) {
                    if ((info->row[info->set * 4 + i][0] & 0x40) != 0) {
                        return 1;
                    }
                }
            }
        } else if (gPhysRiders[player].goalMask != 0) {
            return 1;
        }
    }
    return 0;
}

/* Goal type 0xc: a prop of the level was touched. Returns 1 if a goal was completed. */
BMX_ADDR(0x0040ca60)
int gameCheckPropGoal(int player, void *prop)
{
    int result = 0;

    if (gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel < 6) {
        const GameGoalInfo *info = goalInfo(player, gCurrentLevel);
        const GameGoalRow *row = gGameGoals[gCurrentLevel * 3 + info->set];
        for (uint32_t r = 0; r < 4; r++, row++) {
            if ((info->row[info->set * 4 + r][0] & 0x80) != 0) {
                continue;
            }
            for (uint32_t g = 0; g < row->count; g++) {
                const GameGoal *goal = &row->goal[g];
                if (gameGoalStatus(player, (int)r, g) < 4 && goal->type == 0xc) {
                    const uint8_t *propInfo = *(const uint8_t *const *)((const uint8_t *)prop + 8);
                    const uint16_t kind = (uint16_t)(*(const uint16_t *)(propInfo + 2) & 0x1f);
                    if (goal->min <= kind && kind <= goal->max) {
                        gameSetGoalStatus(player, (int)r, g, kStatusDone);
                        propCollect((uint32_t)player, prop);
                        fxBurst();
                        result = 1;
                    }
                }
            }
        }
        return result;
    }
    return 0;
}

/* Goal type 0x15: a grind edge was used. Returns 1 if a goal was completed. */
BMX_ADDR(0x0040cbe0)
int gameCheckGrindGoal(int player, const PhysEdge *edge)
{
    int result = 0;

    if (gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel < 6) {
        const GameGoalInfo *info = goalInfo(player, gCurrentLevel);
        const GameGoalRow *row = gGameGoals[gCurrentLevel * 3 + info->set];
        for (uint32_t r = 0; r < 4; r++, row++) {
            if ((info->row[info->set * 4 + r][0] & 0x80) != 0) {
                continue;
            }
            for (uint32_t g = 0; g < row->count; g++) {
                const GameGoal *goal = &row->goal[g];
                if (gameGoalStatus(player, (int)r, g) < 4 && goal->type == 0x15) {
                    const uint16_t id = edge->tag & 0xfff;
                    if (goal->min <= id && id <= goal->max) {
                        gameSetGoalStatus(player, (int)r, g, kStatusDone);
                        result = 1;
                    }
                }
            }
        }
        return result;
    }
    return 0;
}

/* Marks goals that wait for the end of the run (status 3) as met or failed. */
BMX_ADDR(0x0040fc30)
void gameEvaluateGoals(int player, int finished)
{
    if (gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel < 6) {
        const PhysRider *r = &gPhysRiders[player];
        const GameGoalInfo *info = goalInfo(player, gCurrentLevel);
        const GameGoalRow *row = gGameGoals[gCurrentLevel * 3 + info->set];
        for (uint32_t i = 0; i < 4; i++, row++) {
            for (uint32_t g = 0; g < row->count; g++) {
                const GameGoal *goal = &row->goal[g];
                if (gameGoalStatus(player, (int)i, g) != 3) {
                    continue;
                }
                int status;
                if (finished == 0) {
                    status = 0;
                } else if (goal->type == 0) {
                    uint32_t k = 0;
                    const uint32_t count = r->combo.count;
                    for (; k < count; k++) {
                        const RiderTrick *t = &r->combo.trick[k];
                        if ((goal->min == 0xff || t->trick == goal->min) && goal->max <= t->tier) {
                            break;
                        }
                    }
                    status = k < count ? kStatusDone : 0;
                } else if (goal->type < 10 || goal->type > 11) {
                    status = kStatusDone;
                } else {
                    status = kStatusDone;
                    if (r->snapB.surface == 0xc) {
                        const uint16_t tag =
                            (uint16_t)(*(const uint16_t *)((const uint8_t *)r->snapB.contact + 2) & 0x1f);
                        if (goal->min <= tag && tag <= goal->max) {
                            status = 0;
                        }
                    }
                }
                gameSetGoalStatus(player, (int)i, g, status);
            }
        }
    }
}

/*
 * Offers a score to the records of the current mode (four per kind: the run, the best of the session,
 * ...) and to the goals of type 0x14 of the level. A new record shows the status message once.
 */
BMX_ADDR(0x0040e030)
void gameSubmitScore(int player, uint32_t kind, uint32_t value, uint32_t sound, uint32_t flags)
{
    if (cameraPlaybackActive() == 0) {
        for (uint32_t slot = 0; slot < 4; slot++) {
            if (slot == 0 && ((gameGetMode() == 0 && gameGetSubMode() == 2) ||
                              zrollerGetRecord((uint32_t)player)[7] >= 4)) {
                continue;
            }
            const uint32_t mode = gameGetMode();
            const uint32_t sub = gameGetSubMode();
            uint8_t *rec = gameGetRecord((((mode << 2 | sub) << 6 | kind) << 4 | slot) << 12);
            if (rec == NULL || *(uint32_t *)rec >= value) {
                continue;
            }
            if (slot == 0 && ((flags & 1) == 0 || (rec[5] & 2) == 0)) {
                OvlStatusMsg msg;
                ovlSetElementB(kElementRecord, &msg);
                if (msg.msg.timer == 0) {
                    msg.msg.id = (uint8_t)sound;
                    msg.value = value;
                    ovlSetElementA(kElementRecord, &msg);
                    rec[5] |= 2;
                    bmxSoundPlayEvent(kSoundRecord, (int)sound, (int)value, 0, 0);
                }
            }
            *(uint32_t *)rec = value;
            rec[4] = (uint8_t)player;
            rec[5] |= 1;
        }
    }
    if (gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel < 6) {
        const GameGoalInfo *info = goalInfo(player, gCurrentLevel);
        const GameGoalRow *row = gGameGoals[gCurrentLevel * 3 + info->set];
        for (uint32_t r = 0; r < 4; r++, row++) {
            if ((info->row[info->set * 4 + r][0] & 0x80) != 0) {
                continue;
            }
            for (uint32_t g = 0; g < row->count; g++) {
                const GameGoal *goal = &row->goal[g];
                if (goal->type == 0x14 && kind == goal->min && gameGoalStatus(player, (int)r, g) < 4) {
                    const int met =
                        kind < 4 ? goal->max < value : (int64_t)(int16_t)goal->max < (int64_t)value;
                    if (met) {
                        gameSetGoalStatus(player, (int)r, g, kStatusDone);
                    }
                }
            }
        }
    }
}

/* Whether the rider is mid trick: spinning hard, flagged busy or with an animation or IK pose set. */
BMX_ADDR(0x0040e2f0)
int gameRiderBusy(const PhysRider *r)
{
    if ((r->flags & 0x1000) == 0) {
        const uint32_t sign = (uint32_t)(r->spin >> 31);
        const uint32_t v = (((uint32_t)r->spin ^ sign) - sign) + 0x4000;
        if ((v & 0xffff8000u) > 0x37fffu || (v >> 15) != 0) {
            return 1;
        }
    }
    if ((r->flags & 0x80) != 0) {
        return 1;
    }
    if (r->anim != 0xff) {
        return 1;
    }
    return r->ikPose != 0xff;
}

/* Applies one reward of a completed goal to the rider data of the player's character. */
BMX_ADDR(0x00411090)
void gameCompleteGoal(int player, const uint8_t *reward)
{
    const uint32_t character = gProfiles[player].character;
    GameRiderData *rd = &gGameRiders[character];

    for (;;) {
        switch (reward[0]) {
        case 1:
            rd->levelsC = (uint8_t)(rd->levelsC > reward[1] + 1 ? rd->levelsC : reward[1] + 1);
            rd->flags |= 0x40;
            return;
        case 2:
            rd->levelsB = (uint8_t)(rd->levelsB > reward[1] + 1 ? rd->levelsB : reward[1] + 1);
            rd->flags |= 0x80;
            return;
        case 3:
            if (reward[1] > 0xb) {
                zdebugFail("goal completion error - char", gameFile, 0x1040, 0);
            }
            if (gSoundVolumes[3] <= reward[1] + 1) {
                gSoundVolumes[3] = (uint8_t)(reward[1] + 1);
            }
            return;
        case 4:
            if (reward[1] > 0xb) {
                zdebugFail("goal completion error - level", gameFile, 0x1044, 0);
            }
            rd->levelsA = (uint8_t)(rd->levelsA > reward[1] + 1 ? rd->levelsA : reward[1] + 1);
            rd->flags |= 0x100;
            if (reward[1] == 1) {
                gProfiles[player].flags &= 0xfd;
            }
            return;
        case 8:
            rd->flags |= 1;
            return;
        case 9:
            reward = gGoalEntriesD[reward[1] + character * 2];
            if (reward[0] - 1u > 9) {
                return;
            }
            continue;
        case 10:
            gGameUnlockBits |= (uint16_t)(1 << (reward[1] & 0x1f));
            return;
        default:
            return;
        }
    }
}

/* Gives out the rewards earned by finishing goal sets (career levels) or levels (the later levels). */
BMX_ADDR(0x00410c60)
void gameUpdateUnlocks(int player)
{
    if (gameGetMode() != 0 || gameGetSubMode() != 0) {
        return;
    }
    const uint32_t character = gProfiles[player].character;
    GameRiderData *rd = &gGameRiders[character];
    PhysRider *r = &gPhysRiders[player];
    uint32_t from;
    uint32_t to;

    if (gCurrentLevel < 6) {
        const GameGoalInfo *info = goalInfo(player, gCurrentLevel);
        to = info->set;
        from = to;
        if (info->done == 4) {
            uint32_t i = 0;
            for (; i < 4; i++) {
                if ((info->row[to * 4 + i][0] & 0x40) != 0) {
                    break;
                }
            }
            if (i == 4) {
                from = to + 1;
            }
            to = to + 1;
        }
    } else {
        to = 0;
        for (uint32_t i = 0; i < 3; i++) {
            if (gCareerUnlocked[i] == character || (character > 9 && gCareerUnlocked[i] == 5)) {
                to = 3 - i;
            }
        }
        from = rd->levelDone[gCurrentLevel];
        if (to <= from) {
            return;
        }
        rd->levelDone[gCurrentLevel] = (uint8_t)to;
    }
    for (; from < to; from++) {
        for (uint32_t m = 0; m < 2; m++) {
            r->goalMask |= 1u << ((from * 2 + m) & 0x1f);
            gameCompleteGoal(player, gGoalEntriesA[m + (from + gCurrentLevel * 3) * 2]);
        }
        for (uint32_t pass = 0; pass < 2; pass++) {
            uint32_t level = pass == 0 ? (gCurrentLevel / 3) * 3 : 0;
            const uint32_t last = pass == 0 ? level + 2 : 0xb;
            int complete = 1;
            for (; level <= last; level++) {
                int unfinished;
                if (level < 6) {
                    const GameGoalInfo *levelInfo = goalInfo(player, level);
                    unfinished = levelInfo->set <= from && (levelInfo->set != from || levelInfo->done != 4);
                } else {
                    unfinished = rd->levelDone[level] <= from;
                }
                if (unfinished) {
                    complete = 0;
                    break;
                }
            }
            if (!complete) {
                break;
            }
            if (pass == 0) {
                const uint32_t index = from + (gCurrentLevel / 3) * 3;
                if (gGoalEntriesB[index][0] != 0) {
                    r->goalMask |= 1u << ((from + 6) & 0x1f);
                    gameCompleteGoal(player, gGoalEntriesB[index]);
                }
                if (from == 2) {
                    rd->flags |= 4u << (gCurrentLevel / 3 & 0x1f);
                }
            } else if (from == 2) {
                r->goalMask |= 0x200;
                rd->flags |= 2;
                gameCompleteGoal(player, gGoalEntriesB[12]);
                const uint32_t current = gProfiles[player].character;
                const uint8_t *entry;
                if (current < 10) {
                    uint32_t k = 0;
                    for (; k < 10; k++) {
                        if ((gGameRiders[k].flags & 2) == 0) {
                            break;
                        }
                    }
                    if (k < 10) {
                        continue;
                    }
                    entry = gGoalEntriesB[13];
                    r->goalMask |= 0x400;
                } else {
                    if (current != 0xb) {
                        continue;
                    }
                    entry = gGoalEntriesB[14];
                    r->goalMask |= 0x800;
                }
                gameCompleteGoal(player, entry);
            }
        }
    }
}

/*
 * Resets a rider to the start of the level (or to the recorded start of a free ride run): the bike
 * frame is rebuilt from the level start orientation, then the riders are stepped once.
 */
BMX_ADDR(0x0040c190)
void gameResetRider(int player, int full)
{
    PhysRider *r = &gPhysRiders[player];
    ZVec3 pos;
    ZAngle3 rot;

    riderReset(player, full);
    propsReset();
    const uint32_t mode = gameGetMode();

    if (mode == 4) {
        const float y = (float)((double)gGameStartPos[1] - 50.0f);
        pos.x = gGameStartPos[0];
        pos.y = y;
        pos.z = gGameStartPos[2];
        rot.x = 0;
        rot.y = 0;
        rot.z = 0;
        r->center = pos;
    } else {
        if (mode == 0 && full != 0) {
            if (gCurrentLevel < 6) {
                gameResetGoals(player);
            }
            r->goalMask = 0;
        }
        zrollerGetStart(0, &pos, &rot);
    }
    r->pos = pos;
    ZMatrix *frame = (ZMatrix *)(void *)&r->fwd;
    zmatrixIdentity(frame);
    zvec3Negate(&r->up, &r->up);
    zmatrixRotateEuler(frame, frame, &rot);
    if (full != 0) {
        riderUpdateInput();
    }
    physUpdateRiders();
}

/* ---- trick scoring ------------------------------------------------------------------------------- */

/* Score tables (.data): per trick and per style two base values and a maximum each. */
typedef struct GameScoreEntry {
    uint16_t base;
    uint16_t limit;
} GameScoreEntry;
typedef GameScoreEntry GameScoreTable[64];
typedef float GameFloats[16];
typedef uint16_t GameWords[16];
#define gScoreTrick     ORIG_VAR(0x00559118, GameScoreTable, gScoreTrick)
#define gScoreStyle     ORIG_VAR(0x005591d4, GameScoreTable, gScoreStyle)
#define gScoreStyleMul  ORIG_VAR(0x00559284, GameFloats, gScoreStyleMul)
#define gScoreTierAdd   ORIG_VAR(0x0055926c, GameWords, gScoreTierAdd)
#define gScoreTierMul   ORIG_VAR(0x0055931c, GameFloats, gScoreTierMul)
#define gScoreLevelMul  ORIG_VAR(0x00559340, GameFloats, gScoreLevelMul)
#define gScoreMultTable ORIG_VAR(0x0055934c, GameFloats, gScoreMultTable)
#define gScoreSpinMul   ORIG_VAR(0x00559370, GameFloats, gScoreSpinMul)

enum {
    kTrickGrindFirst = 0x1b,
    kTrickGrindLast = 0x23,
    kTrickManualA = 0x2c,
    kTrickManualB = 0x2d,
    kElementHeight = 5,
    kHeightGrind = 6,
    kHeightManual = 8,
};

/* A byte read as signed (movsx). */
static int signedByte(uint8_t v)
{
    return v >= 0x80 ? (int)v - 256 : (int)v;
}

/* x87 round trip of an unsigned 32 bit value through FILD qword, a multiply and _ftol. */
static uint32_t scaleInt(uint32_t v, double factor)
{
    return (uint32_t)bmxFtol((double)v * factor);
}

/*
 * Scores the tricks of a combo: each trick gets its points (tables by trick, style, spin, landing
 * level and tier, +20/+30% bonuses) and the total is multiplied by a factor that grows with the number
 * of distinct tricks. The grind and manual times go to the height meter.
 */
BMX_ADDR(0x0040fe40)
void gameScoreCombo(RiderCombo *combo)
{
    float multiplier = 0.0f;
    float grind = 0.0f;
    float manual = 0.0f;
    uint32_t total = 0;

    if (combo->count != 0) {
        uint32_t i = 0;
        do {
            RiderTrick *t = &combo->trick[i];
            const uint32_t mode = gameGetMode();
            if (mode != 3 && (mode < 5 || mode > 10)) {
                const int id = signedByte(t->trick);
                const int style = signedByte(t->style);
                uint32_t v = (uint32_t)gScoreTrick[id].base + gScoreStyle[style].base;
                if (t->trick != 0xff) {
                    v = scaleInt(v, (double)gScoreStyleMul[style]);
                }
                const uint32_t spin = t->spin > 7 ? 7 : t->spin;
                const uint32_t tier = t->tier;
                const uint8_t landed = ((const uint8_t *)combo)[0x9d + ((style + 1) >> 2) + id * 10];
                const uint32_t level = (uint32_t)(landed >> (((t->style + 1) & 3) << 1)) & 3;
                v = scaleInt(v, (double)gScoreSpinMul[spin]);
                v = scaleInt(v, (double)gScoreLevelMul[level]);
                v = scaleInt(v, (double)gScoreTierMul[tier]);
                v += gScoreTierAdd[tier];
                if ((t->flags & 1) != 0) {
                    v = scaleInt(v, (double)1.2f);
                }
                if ((t->flags & 2) != 0) {
                    v = scaleInt(v, (double)1.2f);
                }
                if ((t->flags & 4) != 0) {
                    v = scaleInt(v, (double)1.3f);
                }
                const uint16_t limitA = gScoreTrick[id].limit;
                const uint16_t limitB = gScoreStyle[style].limit;
                const uint32_t limit = limitA > limitB ? limitA : limitB;
                v += (uint32_t)(uint16_t)t->count * limit;
                t->points = (uint16_t)v;
                total += v;

                const RiderTrick *prev = i > 0 ? &combo->trick[i - 1] : NULL;
                if (t->trick == 0xff) {
                    if (t->style != 0xff && (prev == NULL || prev->style != t->style)) {
                        multiplier = (float)((double)multiplier + 1.0f);
                    }
                } else if (prev == NULL || prev->trick != t->trick || prev->style != t->style) {
                    /* ids up to 0x1a and the manuals (0x2b..0x2d) count fully, grinds and lips a quarter */
                    if (t->trick < kTrickGrindFirst || (t->trick >= 0x2b && t->trick < 0x2e)) {
                        multiplier = (float)((double)multiplier + 1.0f);
                    } else if (t->trick < 0x2b) {
                        multiplier = (float)((double)multiplier + 0.25f);
                    }
                }
            }
            if (t->trick >= kTrickGrindFirst && t->trick < kTrickGrindLast + 1) {
                grind = (float)((double)grind + t->duration);
            } else if (t->trick == kTrickManualA || t->trick == kTrickManualB) {
                manual = (float)((double)manual + t->duration);
            }
            i++;
        } while (i < combo->count);
    }
    if (combo->count != 0) {
        const uint8_t last = combo->trick[combo->count - 1].trick;
        struct {
            float value;
            uint32_t pad04;
            uint16_t pad08;
            uint16_t kind;
        } height = {0};
        if (last >= kTrickGrindFirst && last < kTrickGrindLast + 1) {
            height.value = grind;
            height.kind = kHeightGrind;
            ovlSetElementA(kElementHeight, &height);
        } else if (last == kTrickManualA || last == kTrickManualB) {
            height.value = manual;
            height.kind = kHeightManual;
            ovlSetElementA(kElementHeight, &height);
        }
    }
    if (1.0f > multiplier) {
        multiplier = 1.0f;
    }
    const int whole = bmxFtol((double)multiplier);
    const int idx = whole > 8 ? 8 : whole;
    double factor = (double)gScoreMultTable[idx];
    const double frac = (double)multiplier - (double)whole;
    if (!(frac == 0.0 || frac != frac)) {
        const int whole2 = bmxFtol((double)multiplier + 1.0f);
        const int idx2 = whole2 > 8 ? 8 : whole2;
        factor = factor + ((double)gScoreMultTable[idx2] - factor) * frac;
    }
    combo->points = bmxFtol((double)total * factor);
}

/* ---- world markers of the goals ------------------------------------------------------------------ */

/* Vertex colours of a marker: the pulse brightness (frame & 0x1f) scales the colour; kinds are RGB bytes. */
static uint32_t markerBrightness(void)
{
    const uint32_t t = 0x1f - (gGameFrame & 0x1f);
    return (t & 0x1fffffff) < 2 ? 0x10 : (uint8_t)(t << 3);
}

static void markerRgb(NgifVert3 *v, uint32_t color, uint32_t brightness)
{
    v->rgb[0] = (uint8_t)((color >> 16 & 0xff) * brightness >> 8);
    v->rgb[1] = (uint8_t)((color >> 8 & 0xff) * brightness >> 8);
    v->rgb[2] = (uint8_t)((color & 0xff) * brightness >> 8);
}

static int16_t rdS16(const void *base, uint32_t offset)
{
    int16_t v;
    memcpy(&v, (const uint8_t *)base + offset, sizeof v);
    return v;
}

static float rdFloat(const void *base, uint32_t offset)
{
    float v;
    memcpy(&v, (const uint8_t *)base + offset, sizeof v);
    return v;
}

/* A pulsing double sided triangle that grows over 30 frames (the marker of a point in the world). */
BMX_ADDR(0x00411270)
void gameDrawPlaneMarker(const void *marker, uint32_t color)
{
    const uint32_t brightness = markerBrightness();
    const int32_t steps = (int32_t)(400 - brightness) / 30;
    const float step = (float)steps;
    const float dir[3] = {rdFloat(marker, 0), rdFloat(marker, 4), rdFloat(marker, 8)};
    NgifVert3 v[3] = {0};

    for (uint32_t i = 0; i < 3; i++) {
        const uint32_t at = 0x30 + i * 16;
        v[i].x = (float)((double)step * (double)dir[0] + (double)rdFloat(marker, at));
        v[i].y = (float)((double)step * (double)dir[1] + (double)rdFloat(marker, at + 4));
        v[i].z = (float)((double)step * (double)dir[2] + (double)rdFloat(marker, at + 8));
        markerRgb(&v[i], color, brightness);
    }
    ngifDrawPoly3D(10, 3, v, 0);
    for (uint32_t i = 0; i < 3; i++) {
        const uint32_t at = 0x30 + i * 16;
        NgifVert3 *out = &v[2 - i];
        out->x = (float)((double)rdFloat(marker, at) - (double)step * (double)dir[0]);
        out->y = (float)((double)rdFloat(marker, at + 4) - (double)step * (double)dir[1]);
        out->z = (float)((double)rdFloat(marker, at + 8) - (double)step * (double)dir[2]);
        markerRgb(out, color, brightness);
    }
    ngifDrawPoly3D(10, 3, v, 0);
}

/* The outline of an oriented box (four rings of four corners). */
BMX_ADDR(0x00411440)
void gameDrawBoxMarker(const void *marker, uint32_t color)
{
    ZMatrix basis;
    ZVec3 axisA = {rdFloat(marker, 0x18), rdFloat(marker, 0x1c), rdFloat(marker, 0x20)};
    ZVec3 axisB = {rdFloat(marker, 0x24), rdFloat(marker, 0x28), rdFloat(marker, 0x2c)};
    NgifVert3 v[4] = {0};

    memset(&basis, 0, sizeof basis);
    basis.right = axisA;
    basis.up = axisB;
    zvec3Cross3(&basis.at, &basis.right, &gZmathOrigin, &basis.up);
    zvec3Zero(&basis.pos);
    const float size = (float)((double)(gGameFrame & 0x1f) * 1.5f);
    const uint32_t brightness = markerBrightness();

    for (uint32_t ring = 0; ring < 4; ring++) {
        for (uint32_t j = 0; j < 4; j++) {
            ZVec3 corner = {size, size, size};
            if ((ring & 1) == 0) {
                corner.x = (j & 2) != 0 ? size : -size;
                corner.y = ((j + 1) & 2) != 0 ? -size : size;
                corner.z = (ring & 2) != 0 ? size : -size;
            } else {
                corner.x = (j & 2) != 0 ? size : -size;
                corner.y = (ring & 2) != 0 ? size : -size;
                corner.z = ((j + 1) & 2) != 0 ? -size : size;
            }
            ZVec3 out;
            zmatrixTransformPoint(&out, &basis, &corner);
            const uint32_t at = (j & 2) == 0 ? 8 : 0xe;
            v[j].x = (float)((double)rdS16(marker, at) + (double)out.x);
            v[j].y = (float)((double)rdS16(marker, at + 2) + (double)out.y);
            v[j].z = (float)((double)rdS16(marker, at + 4) + (double)out.z);
            markerRgb(&v[j], color, brightness);
        }
        ngifDrawLine3D(10, &v[0], &v[1]);
        ngifDrawLine3D(10, &v[1], &v[2]);
        ngifDrawLine3D(10, &v[2], &v[3]);
        ngifDrawLine3D(10, &v[3], &v[0]);
    }
}

/*
 * Shared by the prop and edge markers: the box (six rings of four corners, two of them skipped for
 * object type 3) of a world object, oriented by the object's own rotation and its frame.
 * `edge` selects the polygon version and raises the corner height by the object's offset.
 */
static void drawObjectMarker(const void *marker, uint32_t color, int polygons)
{
    const uint8_t *object = ngifzCollideGetObject(*(const uint16_t *)marker);
    const float pulse = (float)((double)(gGameFrame & 0x1f) * 1.5f + 20.0f);
    const uint32_t brightness = markerBrightness();
    ZMatrix m;
    ZQuat q;
    ZAngle3 angles;
    NgifVert3 v[4] = {0};

    zmatrixIdentity(&m);
    angles.x = (ZAngle)rdS16(marker, 4);
    angles.y = (ZAngle)-rdS16(marker, 6);
    angles.z = (ZAngle)rdS16(marker, 8);
    const uint8_t *frame =
        *(const uint8_t *const *)(*(const uint8_t *const *)((const uint8_t *)marker + 0x1f0) + 0x1c);
    q.x = rdFloat(frame, 0x14);
    q.y = -rdFloat(frame, 0x18);
    q.z = rdFloat(frame, 0x1c);
    q.w = rdFloat(frame, 0x20);
    zquatToMatrix(&m, &q);
    zmatrixRotateEuler(&m, &m, &angles);
    zmatrixRotateEuler(&m, &m, (const ZAngle3 *)((const uint8_t *)marker + 4));

    for (uint32_t face = 0; face < 6; face++) {
        const uint32_t high = face & 4;
        for (uint32_t j = 0; j < 4; j++) {
            if (high != 0 && object[0] == 3) {
                break;
            }
            ZVec3 d;
            switch (object[0]) {
            case 1:
                d.x = rdFloat(object, 4);
                d.y = rdFloat(object, 8);
                d.z = rdFloat(object, 4);
                break;
            case 2:
            case 3:
            case 4:
                d.x = rdFloat(object, 4);
                d.y = rdFloat(object, 8);
                d.z = rdFloat(object, 0xc);
                break;
            default:
                zvec3Zero(&d);
                break;
            }
            ZVec3 corner;
            corner.y = pulse;
            if (high == 0) {
                corner.x = (float)((double)d.x + pulse);
                if ((j & 2) == 0) {
                    corner.x = -corner.x;
                }
                if ((face & 1) == 0) {
                    if (((j + 1) & 2) != 0) {
                        corner.y = (float)-((double)d.y + pulse);
                    }
                    corner.z = (float)((double)d.z + pulse);
                    if ((face & 2) == 0) {
                        corner.z = -corner.z;
                    }
                } else {
                    if ((face & 2) == 0) {
                        corner.y = (float)-((double)d.y + pulse);
                    }
                    corner.z = (float)((double)d.z + pulse);
                    if (((j + 1) & 2) != 0) {
                        corner.z = -corner.z;
                    }
                }
            } else {
                corner.x = (float)((double)d.x + pulse);
                if ((face & 1) == 0) {
                    corner.x = -corner.x;
                }
                if (((j + 1) & 2) != 0) {
                    corner.y = (float)-((double)d.y + pulse);
                }
                corner.z = (float)((double)d.z + pulse);
                if ((j & 2) == 0) {
                    corner.z = -corner.z;
                }
            }
            if (polygons != 0) {
                corner.y = (float)((double)corner.y + rdFloat(object, 0x10));
            }
            ZVec3 out;
            zmatrixTransformPoint(&out, &m, &corner);
            v[j].x = (float)((double)rdS16(marker, 0xa) + (double)out.x);
            if (polygons != 0) {
                v[j].y = (float)((double)rdS16(marker, 0xc) + (double)out.y);
            } else {
                v[j].y =
                    (float)(((double)rdS16(marker, 0xc) + (double)rdFloat(object, 0x10)) + (double)out.y);
            }
            v[j].z = (float)((double)rdS16(marker, 0xe) + (double)out.z);
            markerRgb(&v[j], color, brightness);
        }
        if (polygons != 0) {
            NgifVert3 reversed[4] = {v[3], v[2], v[1], v[0]};
            ngifDrawPoly3D(10, 4, v, 0);
            ngifDrawPoly3D(10, 4, reversed, 0);
        } else {
            ngifDrawLine3D(10, &v[0], &v[1]);
            ngifDrawLine3D(10, &v[1], &v[2]);
            ngifDrawLine3D(10, &v[2], &v[3]);
            ngifDrawLine3D(10, &v[3], &v[0]);
        }
    }
}

BMX_ADDR(0x004117b0)
void gameDrawPropMarker(const void *marker, uint32_t color)
{
    drawObjectMarker(marker, color, 0);
}

BMX_ADDR(0x00411c50)
void gameDrawEdgeMarker(const void *marker, uint32_t color)
{
    drawObjectMarker(marker, color, 1);
}

/*
 * Draw callback of the roller: while a career level is played the markers of the active goals (status
 * 2) pulse in the world. Each goal type has its own kind of marker list.
 */
BMX_ADDR(0x0040a400)
int gameDrawGoalMarkers(void)
{
    if (cameraPlaybackActive() == 0 && gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel < 6) {
        const GameGoalInfo *info = goalInfo(gCurrentPlayer, gCurrentLevel);
        const GameGoalRow *row = gGameGoals[gCurrentLevel * 3 + info->set];
        const uint32_t color = gGameMarkerColors[info->set];

        for (uint32_t r = 0; r < 4; r++, row++) {
            if ((info->row[info->set * 4 + r][0] & 0x80) != 0) {
                continue;
            }
            for (uint32_t g = 0; g < row->count; g++) {
                if (gameGoalStatus(gCurrentPlayer, (int)r, g) != kStatusActive) {
                    continue;
                }
                switch (row->goal[g].type) {
                case 0xd:
                case 0xe:
                case 0xf:
                    for (uint32_t i = 0; gGameMarkersB[i] != NULL; i++) {
                        gameDrawPlaneMarker(gGameMarkersB[i], color);
                    }
                    /* fall through */
                case 1:
                case 6:
                case 0x15:
                    for (uint32_t i = 0; gGameMarkersA[i] != NULL; i++) {
                        const uint8_t *marker = gGameMarkersA[i];
                        if (*(const void *const *)(marker + 0x2c) == NULL) {
                            gameDrawPlaneMarker(marker, color);
                        } else {
                            gameDrawEdgeMarker(*(const void *const *)(marker + 0x2c), color);
                        }
                    }
                    break;
                case 0x10:
                    for (uint32_t i = 0; gGameMarkersB[i] != NULL; i++) {
                        gameDrawBoxMarker(gGameMarkersB[i], color);
                    }
                    /* fall through */
                case 3:
                case 8:
                    for (uint32_t i = 0; gGameMarkersA[i] != NULL; i++) {
                        gameDrawBoxMarker(gGameMarkersA[i], color);
                    }
                    break;
                case 0x11:
                    for (uint32_t i = 0; gGameMarkersB[i] != NULL; i++) {
                        gameDrawPropMarker(gGameMarkersB[i], color);
                    }
                    /* fall through */
                case 2:
                case 7:
                case 9:
                case 10:
                case 0xb:
                case 0xc:
                    for (uint32_t i = 0; gGameMarkersA[i] != NULL; i++) {
                        gameDrawPropMarker(gGameMarkersA[i], color);
                    }
                    break;
                case 0x12:
                    for (uint32_t i = 0; gGameMarkersA[i] != NULL; i++) {
                        gameDrawPlaneMarker(gGameMarkersA[i], color);
                    }
                    for (uint32_t i = 0; gGameMarkersB[i] != NULL; i++) {
                        gameDrawBoxMarker(gGameMarkersB[i], color);
                    }
                    break;
                case 0x13:
                    for (uint32_t i = 0; gGameMarkersA[i] != NULL; i++) {
                        gameDrawPlaneMarker(gGameMarkersA[i], color);
                    }
                    for (uint32_t i = 0; gGameMarkersB[i] != NULL; i++) {
                        gameDrawPropMarker(gGameMarkersB[i], color);
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }
    gGameFrame++;
    return 0;
}

/* ---- effect callbacks (TODO(fx.c)) ----------------------------------------------------------------- */

/* One block of parameters for the "ripple" / "splash" particle handlers. */
typedef struct FxRipple {
    ZVec3 pos;
    uint32_t unused0c;
    int32_t life;
    float spread;
    float speed;
    uint8_t color[4];
    uint32_t zero;
} FxRipple;

typedef struct FxSplash {
    ZVec3 pos;
    uint32_t zero0c;
    uint32_t zero10;
    float lift;
    uint32_t zero18;
    uint32_t zero1c;
    float scale;
    float growth;
    int32_t life;
    uint8_t color[4];
    uint32_t zero30;
} FxSplash;

_Static_assert(sizeof(FxRipple) == 0x24 && sizeof(FxSplash) == 0x34, "fx parameter blocks");

/* Water splash at the object's position plus an offset: a ripple and a splash handler call. */
BMX_ADDR(0x004096d0)
void fxWaterSplash(const uint8_t *obj, int unused, const float *offset)
{
    FxRipple ripple = {0};
    FxSplash splash = {0};
    const uint8_t *node = ngiffxGetNode(obj, 0);

    if (node == NULL) {
        return;
    }
    const float *target = *(const float *const *)(node + 0x30);
    const float objX = rdFloat(obj, 0xc);
    const float objY = rdFloat(obj, 0x10);
    const float objZ = rdFloat(obj, 0x14);
    ripple.pos.x = (float)(((double)objX + (double)offset[0]) - (double)target[2]);
    ripple.pos.y = (float)(((double)objY + (double)offset[1]) - (double)target[3]);
    ripple.pos.z = (float)(((double)objZ + (double)offset[2]) - (double)target[4]);
    ripple.color[0] = ripple.color[1] = ripple.color[2] = ripple.color[3] = 0xff;
    ripple.life = 1000;
    ripple.spread = 30.0f;
    ripple.speed = 250.0f;
    splash.pos = ripple.pos;
    splash.lift = -7.0f;
    splash.scale = 20.0f;
    splash.growth = 4.0f;
    splash.life = 1000;
    splash.color[0] = splash.color[1] = splash.color[2] = 0xff;
    splash.color[3] = 0x40;
    zstrNameTable2Call("ripple", (uint32_t)(uintptr_t)node, (uint32_t)(uintptr_t)&ripple);
    zstrNameTable2Call("splash", (uint32_t)(uintptr_t)node, (uint32_t)(uintptr_t)&splash);
}

/* Empty (the callers pass debug draw parameters; the shipped build does nothing). */
BMX_ADDR(0x00409860)
void fxNop(void) {}

BMX_ADDR(0x00409880)
void fxCallbackEmit(const uint8_t *obj, uint32_t type)
{
    fxEmit(0, 0xff, (const ZVec3 *)(const void *)(obj + 0x10), 0, type);
}

/* Event callback: on event 1 the original calls an empty debug function; always returns 0. */
BMX_ADDR(0x004098b0)
int fxCallbackLight(const uint8_t *obj, int event)
{
    return 0;
}

/* Creates the effects of a level's effect list (entries of 16 bytes after a 4 byte header). */
BMX_ADDR(0x004098f0)
int fxCreateFromList(const uint8_t *list, uint32_t count)
{
    const uint8_t *entry = list + 4;

    for (; count != 0; count--, entry += 16) {
        ZVec3 pos;
        pos.x = (float)rdS16(entry, 0);
        pos.y = (float)rdS16(entry, 2);
        pos.z = (float)rdS16(entry, 4);
        if (entry[6] == 7) {
            fxCreate(entry[7], &pos, entry + 8);
        } else {
            gZdebugPrint("fx: Don't know how to create:%d\n", entry[6]);
        }
    }
    return 0;
}
