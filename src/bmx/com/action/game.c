/* Game session logic (C:\zaxis\Bmx\Src\com\action\game.c). */
#include "bmx/com/action/game.h"

#include "bmx/com/action/action.h"
#include "bmx/com/action/camera.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/fx.h"
#include "bmx/com/action/memcard.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/physics_air.h"
#include "bmx/com/action/prop.h"
#include "bmx/com/action/replay.h"
#include "bmx/com/action/rider.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/ngif/ngiffx.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
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
            gameEndCombo(gCurrentPlayer, 0);
        } else if (state == 3) {
            gameEndCombo(gCurrentPlayer, 1);
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
    gameUpdateCombo(r->player);
    if (gameGetMode() == 0) {
        gameUpdateHints(r->player);
    }
    gameUpdateGoals(r->player);
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
                    gameEndCombo(gCurrentPlayer, 1);
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
            gameEndCombo(gCurrentPlayer, 1);
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
        riderUpdateAll();
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

/* A pulsing double sided triangle that grows over 30 frames (the marker of a point in the world). The marker
 * is a face record (PhysFace: the normal, then the corners p0..p2, 0x30 / 0x40 / 0x50 on 32-bit). */
BMX_ADDR(0x00411270)
void gameDrawPlaneMarker(const void *marker, uint32_t color)
{
    const PhysFace *const face = marker;
    const ZVec3 *const corner[3] = {&face->p0, &face->p1, &face->p2};
    const uint32_t brightness = markerBrightness();
    const int32_t steps = (int32_t)(400 - brightness) / 30;
    const float step = (float)steps;
    const float dir[3] = {face->normal.x, face->normal.y, face->normal.z};
    NgifVert3 v[3] = {0};

    for (uint32_t i = 0; i < 3; i++) {
        v[i].x = (float)((double)step * (double)dir[0] + (double)corner[i]->x);
        v[i].y = (float)((double)step * (double)dir[1] + (double)corner[i]->y);
        v[i].z = (float)((double)step * (double)dir[2] + (double)corner[i]->z);
        markerRgb(&v[i], color, brightness);
    }
    ngifDrawPoly3D(10, 3, v, 0);
    for (uint32_t i = 0; i < 3; i++) {
        NgifVert3 *out = &v[2 - i];
        out->x = (float)((double)corner[i]->x - (double)step * (double)dir[0]);
        out->y = (float)((double)corner[i]->y - (double)step * (double)dir[1]);
        out->z = (float)((double)corner[i]->z - (double)step * (double)dir[2]);
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
    const PhysPropFrame *frame = ((const PhysPropObject *)marker)->node->frame;
    q.x = frame->rot.x;
    q.y = -frame->rot.y;
    q.z = frame->rot.z;
    q.w = frame->rot.w;
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
                        const PhysFace *marker = gGameMarkersA[i];
                        if (marker->object == NULL) {
                            gameDrawPlaneMarker(marker, color);
                        } else {
                            gameDrawEdgeMarker(marker->object, color);
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

/* ---- combos, jumps, hints, goals and judges of the running session ----------------------------- */

enum {
    kSoundComboStart = 0x1389,
    kSoundComboEnd = 0x138b,
    kSoundComboScore = 0x138c,
    kSoundGoalStep = 0x138d,
    kSoundGoalRow = 0x138e,
    kSoundGoalSet = 0x138f,
    kSoundTransfer = 0x1392,
    kElementScore = 1,
    kElementValue = 5,
    kElementLanding = 6,
    kElementHint = 10,
    kElementGoal = 11,
    kElementMessage = 12,
};

/* Overlay message of elements 10 and 11: an id and a timer word (non-zero while one is showing). */
typedef struct GameSelMsg {
    uint8_t id;
    uint8_t pad01;
    uint16_t timer;
} GameSelMsg;

/* Overlay message of element 12: type byte, timer word and a value. */
typedef struct GameMessage {
    uint8_t type;
    uint8_t pad01;
    uint16_t timer;
    uint32_t value;
} GameMessage;

/* Element 1: the run score and a flag. */
typedef struct GameScoreMsg {
    uint32_t value;
    uint8_t flag;
    uint8_t pad05[3];
} GameScoreMsg;

/* Element 5: a number and the kind of number at +10. */
typedef struct GameValueMsg {
    float value;
    uint8_t pad04[6];
    uint16_t kind;
} GameValueMsg;

/*
 * Trick usage grid of the run: 28 rows (trick + 1; row 0 is "no trick") of 40 two-bit counters
 * (modifier + 1). It starts in the last byte of RiderCombo, so it is addressed from the combo.
 */
static uint8_t *trickGrid(RiderCombo *c)
{
    return (uint8_t *)c + 0x93;
}

/* trick[count - 1], the trick being entered (with no trick it aliases the combo header, as in the original).
 */
static RiderTrick *currentTrick(RiderCombo *c)
{
    return (RiderTrick *)(void *)((uint8_t *)c + c->count * sizeof(RiderTrick));
}

static const RiderTrick *lastTrick(const RiderCombo *c)
{
    return (const RiderTrick *)(const void *)((const uint8_t *)c + c->count * sizeof(RiderTrick));
}

/* Tag word of the rider's current contact (face or edge record: both keep it at the same place). */
static uint16_t contactTag(const void *contact)
{
    return ((const PhysContactData *)contact)->face.tag;
}

/* Tag group of a prop collision object (low 5 bits of the word at +2). */
static uint16_t propGroup(const void *object)
{
    return (uint16_t)(*(const uint16_t *)(const void *)((const uint8_t *)object + 2) & 0x1f);
}

/* x87 compare outcomes that differ from C for unordered operands (FCOMP + FNSTSW + TEST AH). */
static int x87Below(double a, double b) /* TEST AH,1: C0, below or unordered */
{
    return !(a >= b);
}

static int x87Equal(double a, double b) /* TEST AH,0x40: C3, equal or unordered */
{
    return !(a < b) && !(a > b);
}

/* |a - b| <= 5 for the 16 bit ids of goals and contacts. */
static int nearId(uint32_t a, uint32_t b)
{
    const int32_t d = (int32_t)(a - b);
    return (d < 0 ? -d : d) <= 5;
}

static uint32_t modeKey(void)
{
    return gameGetMode() << 2 | gameGetSubMode();
}

/* Score record of the current mode's slot: key (mode, sub mode, slot) << 16 | low. */
static uint8_t *modeRecord(uint32_t low)
{
    const uint32_t key = modeKey();
    return gameGetRecord((key << 6 | gScoreRecordSlot[gameGetMode()]) << 16 | low);
}

/* The current player's rider, or every rider in mode 11. */
static int activeRider(uint32_t i)
{
    return i == gCurrentPlayer || gameGetMode() == kModeNone;
}

/*
 * Judges' marks for the run just finished in a competition level (free ride sub mode 0, levels 6..11):
 * the player's five judges score trick variety, areas used and the trick counts of the run; in the
 * table of ten judges the rider's own judge repeats the average and the others draw random marks around
 * 0xa5. Debug output goes through gZdebugPrint.
 */
BMX_ADDR(0x004105d0)
void gameJudgeRun(int player)
{
    PhysRider *r = &gPhysRiders[player];
    RiderCombo *c = &r->combo;
    const uint8_t *grid = trickGrid(c);
    uint32_t order[10];

    if (gameGetMode() != 0 || gameGetSubMode() != 0 || gCurrentLevel < 6) {
        return;
    }
    if (gameGetRunCount() >= 2) {
        zdebugFail("", gameFile, 0xf23, 0);
    }
    RiderJudgeRun *run = &r->judgeRun[gameGetRunCount()];
    run->score = c->score;
    gZdebugPrint("judge score calculation:\n");
    const uint32_t ratio = (c->score << 7) / gGameJudgePar[gCurrentLevel];
    uint32_t sum = ratio > 0x80 ? 0x80 : ratio;

    uint32_t tricks = 0;
    for (uint32_t row = 10; row < 0x118; row += 10) {
        for (uint32_t col = 0; col < 0x27; col++) {
            if ((grid[row + (col >> 2)] >> ((col & 3) << 1) & 3) != 0) {
                tricks++;
                break;
            }
        }
    }
    uint32_t v = tricks << 5;
    sum += (v & ~7u) > 0x100 ? 0x20 : v >> 3;
    gZdebugPrint("total unique tricks performed: %d\n", tricks);

    uint32_t modifiers = 0;
    for (uint32_t col = 1; col <= 0x26; col++) {
        for (uint32_t row = 0; row < 0x118; row += 10) {
            if ((grid[row + (col >> 2)] >> ((col & 3) << 1) & 3) != 0) {
                modifiers++;
                break;
            }
        }
    }
    v = (modifiers << 5) / 10;
    int32_t score = (int32_t)(sum + (v > 0x20 ? 0x20 : v));
    gZdebugPrint("total unique modifiers performed: %d\n", modifiers);

    uint32_t areas = 0;
    for (int i = 0; i < 16; i++) {
        if (r->area[i].tricks != 0 || r->area[i].modifiers != 0) {
            areas++;
        }
    }
    v = areas << 4;
    score += (int32_t)((v & ~7u) > 0x80 ? 0x10 : v >> 3);
    gZdebugPrint("total areas used: %d\n", areas);

    const RiderAreaStats *t = &r->statTotal;
    int32_t part = t->tricks * 8 / 20;
    score += part > 8 ? 8 : part;
    gZdebugPrint("number of tricks: %d\n", t->tricks);
    part = t->modifiers * 8 / 24;
    score += part > 8 ? 8 : part;
    gZdebugPrint("number of modifiers: %d\n", t->modifiers);
    part = t->modified;
    score += part > 8 ? 8 : part;
    gZdebugPrint("number of modified tricks: %d\n", t->modified);
    part = t->grinds * 8 / 5;
    score += part > 8 ? 8 : part;
    gZdebugPrint("number of grinds: %d\n", t->grinds);
    part = t->stalls * 2;
    score += part > 4 ? 4 : part;
    gZdebugPrint("number of stalls: %d\n", t->stalls);
    part = t->manuals * 2;
    score += part > 4 ? 4 : part;
    gZdebugPrint("number of manuals: %d\n", t->manuals);
    part = t->falls * 4 / 3;
    score -= part > 4 ? 4 : part;
    gZdebugPrint("number of falls: %d\n", t->falls);
    gZdebugPrint("non-random judgeScore is %d\n", score);

    /* The player's five judges; the range test and the mark come from two different random draws. */
    uint32_t total = 0;
    for (uint32_t k = 0; k < 5; k++) {
        if (c->score == 0) {
            run->judge[k] = 0;
        } else {
            uint32_t mark = (((zmathRandom() & 7) + (uint32_t)score) >> 1) + 0x80;
            if (mark > 0xff) {
                mark = 0xff;
            } else {
                mark = (((zmathRandom() & 7) + (uint32_t)score) >> 1) + 0x80;
            }
            run->judge[k] = (uint8_t)mark;
            gZdebugPrint("  judge %d awards %d\n", k, mark & 0xff);
        }
        total += run->judge[k];
    }
    run->average = (uint8_t)(total / 5);

    /* The judge table: the rider's own judge (judge 5 for the extra riders) repeats the average. */
    const uint8_t character = gProfiles[player].character;
    uint8_t *scores = &gGameJudgeScores[0][0];
    for (uint32_t j = 0; j < 10; j++) {
        const uint32_t at = j * 3;
        uint8_t mark;
        if (j == character || (character >= 10 && at == 0xf)) {
            mark = run->average;
        } else {
            const uint32_t rnd = zmathRandom() & 0xff;
            const int32_t m = 0xa5 - bmxFtol((double)rnd * (double)-0x1.555556p-2f) + gGameJudgeBias[j];
            mark = (uint8_t)(m > 0xff ? 0xff : m);
        }
        scores[gameGetRunCount() + at] = mark;
        uint32_t s = 0;
        for (uint32_t i = 0; i < gameGetRunCount() + 1; i++) {
            s += scores[at + i];
        }
        const uint32_t avg = s / (gameGetRunCount() + 1);
        scores[at + 2] = (uint8_t)avg;
        order[j] = avg & 0xff;
    }
    for (uint32_t k = 0; k < 10; k++) {
        uint32_t best = 0;
        int32_t high = -1;
        for (uint32_t i = 0; i < 10; i++) {
            if ((int32_t)order[i] > high) {
                best = i;
                high = (int32_t)order[i];
            }
        }
        gGameJudgeRank[k] = (uint8_t)best;
        order[best] = 0xffffffffu;
    }
}

/*
 * What a contact snapshot touched, for the jump goals: kind 10 face or edge (id: tag), 9 lip edge, 12 prop
 * (id: tag group). Returns 0 for the surfaces that end gameScoreJump at once (10, 11, 13, 14).
 */
static int jumpContact(const PhysContactSnap *snap, uint16_t *kind, uint16_t *id)
{
    const uint8_t *contact = snap->contact;

    if (contact == NULL) {
        return 1;
    }
    switch (snap->surface) {
    case 0:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        *kind = 10;
        *id = (uint16_t)(contactTag(contact) & 0xfff);
        break;
    case 1:
    case 2:
    case 3:
        *kind = 10;
        *id = contactTag(contact);
        break;
    case 9: {
        const uint8_t *prop = ((const PhysEdge *)(const void *)contact)->prop;
        if (prop != NULL) {
            *kind = 0xc;
            *id = prop[2] & 0x1f;
        } else {
            *kind = 9;
            *id = contactTag(contact);
        }
        break;
    }
    case 12:
        *kind = 0xc;
        *id = contact[2] & 0x1f;
        break;
    case 10:
    case 11:
    case 13:
    case 14:
        return 0;
    default:
        break;
    }
    return 1;
}

/* Jump goals (types 0xd..0x13) between the take-off and the landing contact. */
static void jumpGoals(int player, const RiderCombo *c, const uint16_t *kind, const uint16_t *id)
{
    GameGoalInfo *info = goalInfo(player, gCurrentLevel);
    const GameGoalRow *row = gGameGoals[gCurrentLevel * 3 + info->set];

    for (uint32_t i = 0; i < 4; i++, row++) {
        if ((info->row[info->set * 4 + i][0] & 0x80) != 0) {
            continue;
        }
        for (uint32_t g = 0; g < row->count; g++) {
            const GameGoal *goal = &row->goal[g];
            int met = 0;
            if (gameGoalStatus(player, (int)i, g) >= 3) {
                continue;
            }
            switch (goal->type) {
            case 0xd:
            case 0xe:
            case 0xf:
                if (kind[0] != 10 || kind[1] != 10 ||
                    !((nearId(id[0], goal->min) && nearId(id[1], goal->max)) ||
                      (nearId(id[0], goal->max) && nearId(id[1], goal->min)))) {
                    break;
                }
                if (goal->type == 0xd) {
                    met = 1;
                    break;
                }
                for (uint32_t k = 0; k < c->count; k++) {
                    const RiderTrick *t = &c->trick[k];
                    if ((goal->type == 0xe && t->trick == goal->arg3) ||
                        (goal->type == 0xf && t->tier >= goal->arg3)) {
                        met = 1;
                        break;
                    }
                }
                break;
            case 0x10:
            case 0x11: {
                const uint16_t want = goal->type == 0x10 ? 9 : 0xc;
                met = kind[0] == want && kind[1] == want &&
                      ((id[0] == goal->min && id[1] == goal->max) ||
                       (id[1] == goal->min && id[0] == goal->max));
                break;
            }
            case 0x12:
                met = kind[0] == 10 && kind[1] == 9 && nearId(id[0], goal->min) && id[1] >= goal->max &&
                      id[2] <= goal->arg3;
                break;
            case 0x13:
                met = kind[0] == 10 && kind[1] == 0xc && nearId(id[0], goal->min) && id[1] >= goal->max &&
                      id[1] <= goal->arg3;
                break;
            default:
                break;
            }
            if (met) {
                gameSetGoalStatus(player, (int)i, g, kStatusDone);
            }
        }
    }
}

/*
 * A jump ended (landed != 0) or was abandoned: scores the distance between the take-off and landing
 * contacts, completes the jump goals of the level and shows the transfer message for long jumps between
 * two grind edges; then, unless a combo is running, settles the goals waiting for the end of the run.
 * Contacts of surfaces 10, 11, 13 and 14 end the function at once. The original reads uninitialised stack
 * words for a missing contact or an unknown surface, and for the third id of goal type 0x12; the port uses 0.
 */
BMX_ADDR(0x0040c300)
void gameScoreJump(int player, int landed)
{
    PhysRider *r = &gPhysRiders[player];
    uint16_t kind[2] = {0, 0};
    uint16_t id[3] = {0, 0, 0};

    if (landed != 0) {
        if (!jumpContact(&r->snapA, &kind[0], &id[0]) || !jumpContact(&r->snapB, &kind[1], &id[1])) {
            return;
        }
        const int32_t dx = bmxFtol((double)r->snapBPos.x - r->snapAPos.x);
        const int32_t dy = bmxFtol((double)r->snapBPos.y - r->snapAPos.y);
        const int32_t dz = bmxFtol((double)r->snapBPos.z - r->snapAPos.z);
        const uint32_t sq =
            (uint32_t)dz * (uint32_t)dz + (uint32_t)dy * (uint32_t)dy + (uint32_t)dx * (uint32_t)dx;
        const int32_t dist = bmxFtol(__builtin_sqrt((double)(int32_t)sq));
        gameSubmitScore(player, 7, (uint32_t)(int16_t)dist, 0x15, 0);
        if (id[0] != 0 && id[1] != 0 && id[0] != id[1]) {
            if (gameGetMode() == 0 && gameGetSubMode() == 0 && gCurrentLevel < 6) {
                jumpGoals(player, &r->combo, kind, id);
            }
            if (cameraPlaybackActive() == 0 && r->snapA.surface == 1 && r->snapB.surface == 1 &&
                !nearId(id[0], id[1]) && (double)(uint32_t)dist >= 400.0) {
                GameMessage msg;
                ovlSetElementB(kElementMessage, &msg);
                if (msg.timer == 0) {
                    msg.type = 0xb;
                    msg.value = id[1];
                    ovlSetElementA(kElementMessage, &msg);
                    bmxSoundPlayEvent(kSoundTransfer, 0, 0, 0, 0);
                }
            }
        }
    }
    if (r->combo.count == 0) {
        gameEvaluateGoals(player, landed);
    }
}

/*
 * Ends the combo of the rider: a bail (landed == 0) counts a fall; a landing scores every trick, marks
 * them in the trick grid, the run statistics of the area the rider is in and the rider's seen-trick bits,
 * then adds the combo to the run score (in the horse game it decides whether the tricks to repeat were
 * matched instead). Nothing but the reset happens during camera playback.
 */
BMX_ADDR(0x0040dae0)
void gameEndCombo(int player, int landed)
{
    PhysRider *r = &gPhysRiders[player];
    RiderCombo *c = &r->combo;
    uint8_t *seen = (uint8_t *)&gGameRiders[gProfiles[player].character] + 0xf7;

    if (cameraPlaybackActive() == 0) {
        const float *b = zrollerGetWorldBounds();
        const int32_t zq = bmxFtol((double)r->pos.z - b[2]) / (int32_t)gGameExtent[1];
        const int32_t xq = bmxFtol((double)r->pos.x - b[0]) / (int32_t)gGameExtent[0];
        uint32_t cell = (uint32_t)zq + (uint32_t)xq * 4;
        if (cell >= 15) {
            cell = 15;
        }
        RiderAreaStats *area = &r->area[cell];
        if (landed == 0) {
            bmxSoundPlayEvent(kSoundComboScore, 0, 0, 0, 0);
            area->falls++;
            r->statTotal.falls++;
        } else {
            float grinds = 0.0f;
            float manuals = 0.0f;
            for (uint32_t i = 0; i < c->count; i++) {
                const RiderTrick *t = &c->trick[i];
                gameSubmitScore(player, 3, t->points, 0xe, 0);
                const int32_t col = (int8_t)t->style + 1;
                uint8_t *cell8 = (uint8_t *)c + 0x9d + (int8_t)t->trick * 10 + (col >> 2);
                const int shift = (col & 3) << 1;
                if ((*cell8 >> shift & 3) < 3) {
                    *cell8 = (uint8_t)(*cell8 + (1 << shift));
                }
                if (t->trick < 0x1b) {
                    area->tricks++;
                    r->statTotal.tricks++;
                } else if (t->trick < 0x24) {
                    grinds = (float)((double)grinds + t->duration);
                    area->grinds++;
                    r->statTotal.grinds++;
                } else if (t->trick < 0x2a) {
                    area->stalls++;
                    r->statTotal.stalls++;
                } else if (t->trick < 0x2c) {
                    gameSubmitScore(player, 5, (uint32_t)bmxFtol(r->lipHeight), 0x13, 0);
                    area->stalls++;
                    r->statTotal.stalls++;
                } else if (t->trick < 0x2e) {
                    manuals = (float)((double)manuals + t->duration);
                    area->manuals++;
                    r->statTotal.manuals++;
                }
                if (t->trick != 0xff) {
                    seen[t->trick >> 3] |= (uint8_t)(1 << (t->trick & 7));
                }
                if (t->style != 0xff) {
                    seen[(t->style + 0x2e) >> 3] |= (uint8_t)(1 << ((t->style - 2) & 7));
                    area->modifiers++;
                    r->statTotal.modifiers++;
                    if (t->trick != 0xff) {
                        area->modified++;
                        r->statTotal.modified++;
                    }
                }
            }
            gameSubmitScore(player, 6, (uint32_t)bmxFtol(grinds), 0x10, 0);
            gameSubmitScore(player, 8, (uint32_t)bmxFtol(manuals), 0x11, 0);
            const uint32_t mode = gameGetMode();
            if (mode == 3) {
                if (gameTestOption(4)) {
                    const uint32_t n = c->count <= 3 ? c->count : 3;
                    zmemCopy(gGameHorseTricks, c->trick, n << 4);
                    gGameHorseCount = (uint8_t)n;
                    c->score = 1;
                } else {
                    const uint32_t n = gGameHorseCount;
                    uint32_t i = 0;
                    for (; i < n; i++) {
                        const RiderTrick *h = &gGameHorseTricks[i];
                        const RiderTrick *t = &c->trick[i];
                        if ((h->trick != 0xff && t->trick != h->trick) ||
                            (h->style != 0xff && t->style != h->style) || t->tier < h->tier) {
                            break;
                        }
                    }
                    if (i == n) {
                        c->score = 1;
                    }
                }
            } else if (mode <= 4 || mode > 10) {
                gameSubmitScore(player, 2, (uint32_t)c->points, 0xf, 0);
                c->score += (uint32_t)c->points;
                gameSubmitScore(player, 1, c->score, 0xd, 1);
            }
            GameMessage msg;
            ovlSetElementB(kElementMessage, &msg);
            if (msg.timer == 0 && c->count > 3) {
                msg.type = 0xc;
                msg.value = c->count;
                ovlSetElementA(kElementMessage, &msg);
                bmxSoundPlayEvent(kSoundTransfer, 0, 0, 0, 0);
            }
            bmxSoundPlayEvent(kSoundComboScore, 1, c->points, 0, 0);
        }
    }
    uint32_t zero = 0;
    ovlSetElementA(kElementValue, &zero);
    c->runState = 0;
    c->count = 0;
}

/* The combo end sound: louder the more points over 4000 the combo made. */
static void comboEndSound(const RiderCombo *c)
{
    if (c->count > 2 || (uint32_t)c->points > 4000) {
        const int32_t n =
            bmxFtol((double)(int16_t)(uint16_t)((uint32_t)c->points - 4000u) * (double)0x1.0624dep-11f);
        bmxSoundPlayEvent(kSoundComboEnd, n > 5 ? 5 : n, 1, 0, 0);
    }
}

/* Back on the ground: the combo waits 30 frames for the next trick before it ends. */
static void comboLanded(int player, RiderCombo *c)
{
    c->runState = 3;
    c->timer = 0x1e;
    gameEvaluateGoals(player, 1);
    comboEndSound(c);
}

static void comboBailed(int player)
{
    gameEndCombo(gCurrentPlayer, 0);
    gameEvaluateGoals(player, 0);
    bmxSoundPlayEvent(kSoundComboEnd, 2, 0, 0, 0);
}

/* Absolute value of a 32 bit angle as the CDQ/XOR/SUB idiom computes it (INT_MIN stays negative). */
static uint32_t absAngle(int32_t a)
{
    return a < 0 ? 0u - (uint32_t)a : (uint32_t)a;
}

/* Run state 2: follows the trick being entered (its id, spins, hold time, landing direction). */
static void comboTrickStep(int player, PhysRider *r, RiderCombo *c)
{
    const int mode = riderGetStateLevel(r, 0);

    if (mode == kRiderRoll) {
        RiderTrick *t = currentTrick(c);
        if (t->trick < 0x1b) {
            const uint32_t k = gRiderAnimFlags[t->trick] & 7u;
            if (k != 0) {
                switch (t->barspin) {
                case 0:
                    t->trick = 0xff;
                    break;
                case 1:
                    if (k >= 2) {
                        if (k <= 3) {
                            t->trick = 3;
                        } else if (k == 5) {
                            t->trick = 10;
                        }
                    }
                    break;
                case 2:
                    t->trick = 8;
                    break;
                default:
                    break;
                }
            }
        }
        comboLanded(player, c);
    } else if (mode == kRiderAir) {
        if (r->anim == 0xff && r->ikPose == 0xff && r->spin == 0) {
            c->runState = 1;
        }
    } else if (mode == kRiderBail) {
        comboBailed(player);
    }
    if (c->runState != 2 || riderGetStateLevel(r, 0) == kRiderMode5) {
        return;
    }

    RiderTrick *t = currentTrick(c);
    t->trick = r->anim;
    t->style = r->ikPose;
    t->spin = r->ikFrame;
    if ((r->flags & 0x1000) != 0) {
        t->tier = 0;
    } else {
        const uint32_t a = absAngle(r->spin) + 0x4000;
        t->tier = (uint8_t)((a & 0xffff8000u) < 0x40000 ? a >> 15 : 8);
    }
    const uint32_t b = absAngle(r->field54) + 0x8000;
    t->barspin = (uint8_t)((b & 0xffff0000u) < 0x30000 ? b >> 16 : 3);

    int held;
    const int l0 = riderGetStateLevel(r, 0);
    if (l0 == 1) {
        const int l1 = riderGetStateLevel(r, 1);
        if (l1 == 1) {
            held = (gRiderAnimFlags[r->anim] & 7) == 0 && riderGetStateLevel(r, 2) == 2 &&
                   (gRiderAnimFlags[r->anim] & 8) == 0;
        } else {
            held = l1 == 2 && riderGetStateLevel(r, 2) == 2;
        }
    } else if (l0 == 0 || l0 > 3) {
        held = 1;
    } else {
        held = riderGetStateLevel(r, 2) == 3 || riderGetStateLevel(r, 2) == 4;
    }
    if (held) {
        t->count++;
    }
    if (physGetState(r) == 4 || riderGetStateLevel(r, 0) == 4) {
        t->duration = (float)((double)r->speed + t->duration);
    }

    if (t->trick != 0xff && t->trick >= 0x1b) {
        if (r->anim < 0x24 && x87Below(zvec3Dot(&r->vel, &r->fwd), 0.0)) {
            t->flags |= 2;
        }
    } else if (t->tier != 0) {
        if (r->surface == 1 && (r->flags & 0x2000) != 0) {
            const ZVec3 down = {0.0f, -1.0f, 0.0f};
            ZVec3 axis;
            zvec3Cross3(&axis, &down, &gZmathOrigin, &r->up);
            const int back = x87Below(zvec3Dot(&r->vel, &axis), 0.0);
            if ((r->spin > 0) != back) {
                t->flags |= 2;
            }
        }
    } else if (t->trick == 0xff && t->style == 0xff) {
        c->count--;
        c->runState = c->count != 0;
    }
    if ((r->flags & 0x10000000) != 0 && t->tier != 0) {
        t->flags |= 4;
        r->flags &= ~0x10000000u;
    }
    if (t->trick >= 0x2a && t->trick < 0x2c && r->groundGap > r->lipHeight) {
        r->lipHeight = r->groundGap;
    }
}

/*
 * Per frame combo bookkeeping of a rider: the run state machine (0 riding, 1 combo open, 2 trick being
 * entered, 3 landed and waiting), trick ages, the HUD score, height and impact readouts and the score
 * record of the timed modes. Mode 5 only tracks whether the rider is in the air.
 */
BMX_ADDR(0x0040d010)
void gameUpdateCombo(int player)
{
    PhysRider *r = &gPhysRiders[player];
    RiderCombo *c = &r->combo;

    if (gameGetMode() == 5) {
        if (c->runState == 0) {
            if (physGetState(r) == 2) {
                c->runState = 1;
                c->timer = 0;
            }
        } else if (c->runState == 1) {
            if (physGetState(r) == 2) {
                c->score = (uint32_t)r->field58 * 100;
            } else {
                c->runState = 0;
            }
        }
    } else {
        if (c->runState == 2 && (r->flags & 0x80) != 0) {
            c->runState = 1;
        }
        switch (c->runState) {
        case 0:
            if (!gameRiderBusy(r)) {
                break;
            }
            c->runState = 1;
            c->timer = 0;
            r->lipHeight = 0.0f;
            /* fall through */
        case 1:
            if (gameRiderBusy(r) && c->count < 7) {
                RiderTrick *t = &c->trick[c->count];
                c->count++;
                t->count = 0;
                t->ticks = 0;
                t->flags = 0;
                t->duration = 0.0f;
                c->runState = 2;
                bmxSoundPlayEvent(kSoundComboStart, 2, c->count - 1, 0, 0);
                comboTrickStep(player, r, c);
            } else {
                const int mode = riderGetStateLevel(r, 0);
                if (mode == kRiderRoll) {
                    comboLanded(player, c);
                } else if (mode == kRiderBail) {
                    comboBailed(player);
                }
            }
            break;
        case 2:
            comboTrickStep(player, r, c);
            break;
        case 3:
            if (gameRiderBusy(r) || --c->timer == 0) {
                gameEndCombo(gCurrentPlayer, 1);
            }
            break;
        default:
            break;
        }
    }

    gameScoreCombo(c);
    /* Age the tricks; while a trick is being entered the older ones only age up to a limit. */
    for (uint32_t i = 0; i < c->count; i++) {
        RiderTrick *t = &c->trick[i];
        const uint32_t n = c->count;
        if (c->runState != 2 || (n > 1 && i < n - 1 && t->ticks < (n - 1 - i) * 16) || (n > 2 && i < n - 2)) {
            t->ticks++;
        }
    }
    const GameScoreMsg shown = {c->score, 0, {0}};
    ovlSetElementA(kElementScore, &shown);

    GameValueMsg value = {0};
    if (riderGetStateLevel(r, 0) == kRiderAir && (r->flags & 0x8000000) == 0 &&
        x87Below(r->pos.y, r->snapAPos.y)) {
        if (r->vel.y > 0.0f) {
            gameSubmitScore(player, 4, (uint32_t)bmxFtol((double)r->snapAPos.y - r->pos.y), 0x14, 0);
        } else {
            value.kind = 4;
            value.value = (float)((double)r->snapAPos.y - r->pos.y);
            ovlSetElementA(kElementValue, &value);
        }
    }
    if (riderGetStateLevel(r, 0) == kRiderRoll) {
        if (riderGetStateLevel(r, 1) == 2) {
            value.value = r->impactTotal;
            value.kind = 9;
        } else {
            if (!x87Equal(r->impactTotal, 0.0)) {
                gameSubmitScore(player, 9, (uint32_t)bmxFtol(r->impactTotal), 0x12, 0);
                r->impactTotal = 0.0f;
            }
            value.value = 0.0f;
        }
        ovlSetElementA(kElementValue, &value);
    }
    const uint32_t mode = gameGetMode();
    if (mode >= 6 && mode <= 10) {
        const uint8_t *rec = modeRecord(0x3000);
        if (rec == NULL) {
            zdebugFail("", gameFile, 0x9a6, 0);
        }
        c->score = *(const uint32_t *)(const void *)rec;
    }
    const float landing = riderGetStateLevel(r, 0) == kRiderManual ? r->landQuality : 0.0f;
    ovlSetElementA(kElementLanding, &landing);
}

/* Shows hint `bit` (element 10) and clears the bits in `clear` from the mask. */
static void showHint(PhysRider *r, GameSelMsg *msg, uint32_t clear)
{
    ovlSetElementA(kElementHint, msg);
    r->hintMask &= ~clear;
}

/*
 * Tutorial hints of a player (profile flag 2): when no hint is showing, the first pending hint of the air
 * hints (bits 0..11) is shown if the rider's state fits, then the ground hints (bits 12..21) are tried in
 * order until one is showing. The profile flag goes off once every hint was shown.
 */
BMX_ADDR(0x0040e380)
void gameUpdateHints(int player)
{
    OvlProfile *profile = &gProfiles[player];
    PhysRider *r = &gPhysRiders[player];
    RiderCombo *c = &r->combo;
    GameSelMsg msg;

    if ((profile->flags & 2) == 0) {
        return;
    }
    ovlSetElementB(kElementHint, &msg);
    if (msg.timer == 0) {
        uint32_t bit = 0;
        while (bit < 12 && (r->hintMask & 1u << bit) == 0) {
            bit++;
        }
        if (bit < 12) {
            msg.id = (uint8_t)bit;
            int show = 0;
            switch (bit) {
            case 0:
            case 2:
            case 6:
                show = riderGetStateLevel(r, 0) == 1 && riderGetStateLevel(r, 1) == 0 && c->count == 0 &&
                       x87Below(r->vel.y, 0.0);
                break;
            case 1:
                show = riderGetStateLevel(r, 0) == 1 && riderGetStateLevel(r, 1) == 2;
                break;
            case 3:
            case 4:
                show = riderGetStateLevel(r, 0) == 1 && riderGetStateLevel(r, 1) == 1;
                break;
            case 5:
                show = riderGetStateLevel(r, 0) == 1 && riderGetStateLevel(r, 1) == 0 && c->count != 0 &&
                       lastTrick(c)->trick < 0x1b;
                break;
            case 10:
                show = riderGetStateLevel(r, 0) == 1 && x87Below(r->vel.y, 0.0);
                break;
            case 11:
                show = riderGetStateLevel(r, 0) == 0 && (r->flags & 0x400000) != 0;
                break;
            default: /* 7..9 */
                show =
                    riderGetStateLevel(r, 0) == 1 && riderGetStateLevel(r, 1) == 0 && x87Below(r->vel.y, 0.0);
                break;
            }
            if (show) {
                showHint(r, &msg, 1u << bit);
            }
            if (r->hintMask == 0) {
                profile->flags &= 0xfd;
            }
        }
    }
    ovlSetElementB(kElementHint, &msg);
    if (msg.timer != 0) {
        return;
    }
    for (uint32_t bit = 12; bit < 0x16; bit++) {
        const uint32_t m = 1u << bit;
        if ((r->hintMask & m) == 0) {
            continue;
        }
        msg.id = (uint8_t)bit;
        const int l0 = riderGetStateLevel(r, 0);
        switch (bit) {
        case 12:
            if (l0 == 0 && riderGetStateLevel(r, 1) == 3) {
                showHint(r, &msg, m);
            }
            break;
        case 13:
            if (l0 == 0 && riderGetStateLevel(r, 1) == 0 && x87Equal(r->speed, 0.0)) {
                showHint(r, &msg, 0);
            }
            break;
        case 14:
            if (l0 == 0 && riderGetStateLevel(r, 1) == 0 && !x87Equal(r->speed, 0.0)) {
                showHint(r, &msg, m);
            }
            break;
        case 15:
            if (l0 == 0 && riderGetStateLevel(r, 1) == 1) {
                showHint(r, &msg, m);
            }
            break;
        case 16:
            if (l0 == 0 && r->surface == 1 && x87Below(r->vel.y, 0.0)) {
                showHint(r, &msg, m | 0x1000);
            }
            break;
        case 17:
            if (l0 == 0 && riderGetStateLevel(r, 1) == 4) {
                showHint(r, &msg, 0);
            }
            break;
        case 18:
            if (l0 == kRiderBail && (r->bailReason == 7 || r->bailReason == 8)) {
                showHint(r, &msg, m);
            }
            break;
        case 19:
            if (l0 == 0 && r->surface == 1 && x87Below(r->vel.y, 0.0)) {
                showHint(r, &msg, m);
            }
            break;
        case 20:
            if (l0 == kRiderGrind) {
                showHint(r, &msg, m);
            }
            break;
        default: /* 21 */
            if (l0 == 0 && riderGetStateLevel(r, 1) == 3 && r->surface == 1 && x87Below(r->vel.y, 0.0)) {
                showHint(r, &msg, m);
            }
            break;
        }
        ovlSetElementB(kElementHint, &msg);
        if (msg.timer != 0) {
            if (r->hintMask == 0) {
                profile->flags &= 0xfd;
            }
            return;
        }
    }
}

/* Combo condition of goal types 0..3: the current trick is `min` (0xff: any) at tier `max` or more. */
static int goalTrick(const RiderCombo *c, const GameGoal *goal)
{
    if (c->count == 0) {
        return 0;
    }
    const RiderTrick *t = lastTrick(c);
    return (goal->min == 0xff || t->trick == goal->min) && t->tier >= goal->max;
}

/* The trick of goal types 8 and 9 must have been landed backwards (flag 2) when arg3 asks for it. */
static int goalBackwards(const RiderCombo *c, const GameGoal *goal)
{
    return goal->arg3 == 0 || (c->count != 0 && (lastTrick(c)->flags & 2) != 0);
}

/* Prop group of a take-off contact (surface 12 prop, or the prop of a surface 9 edge). */
static int snapPropIn(const PhysContactSnap *snap, uint16_t lo, uint16_t hi)
{
    const void *object;
    if (snap->surface == 0xc) {
        object = snap->contact;
    } else if (snap->surface == 9) {
        object = ((const PhysEdge *)snap->contact)->prop;
        if (object == NULL) {
            return 0;
        }
    } else {
        return 0;
    }
    const uint16_t group = propGroup(object);
    return group >= lo && group <= hi;
}

/* Checks the in-run condition of an active goal (types 0..11) and moves it to status 3 or 4. */
static void goalCheck(int player, const PhysRider *r, int row, uint32_t g, const GameGoal *goal)
{
    const RiderCombo *c = &r->combo;
    int status = 0;

    switch (goal->type) {
    case 0:
        status = goalTrick(c, goal) ? 3 : 0;
        break;
    case 1:
        if (goalTrick(c, goal)) {
            switch (r->surface) {
            case 1:
            case 2:
            case 3:
                status = nearId(contactTag(r->contact), goal->arg3) ? 3 : 0;
                break;
            case 0:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 10:
            case 11:
            case 13:
            case 14:
                status = nearId(contactTag(r->contact) & 0xfffu, goal->arg3) ? 3 : 0;
                break;
            default:
                break;
            }
        }
        break;
    case 2:
        if (goalTrick(c, goal)) {
            const void *object = NULL;
            if (r->snapA.surface == 0xc) {
                object = r->snapA.contact;
            } else if (r->snapA.surface == 9) {
                object = ((const PhysEdge *)r->snapA.contact)->prop;
            }
            if (object != NULL && propGroup(object) == goal->arg3) {
                status = 3;
            }
        }
        break;
    case 3:
        if (goalTrick(c, goal) && r->surface == 9 && contactTag(r->contact) >= goal->arg3) {
            status = 3;
        }
        break;
    case 4: {
        uint32_t n = 0;
        for (uint32_t k = 0; k < c->count; k++) {
            if (c->trick[k].trick < 0x1b) {
                n++;
            }
        }
        if (n >= goal->min) {
            status = 3;
        }
        break;
    }
    case 5:
        if ((uint32_t)riderGetStateLevel(r, 0) == goal->min) {
            status = 4;
        }
        break;
    case 6:
        if ((uint32_t)riderGetStateLevel(r, 0) == goal->min) {
            const uint8_t s = r->surface;
            int on = 0;
            if (s == 0) {
                const uint16_t tag = (uint16_t)(contactTag(r->contact) & 0xfff);
                on = tag >= goal->max && tag <= goal->arg3;
            } else if (s >= 1 && s <= 3) {
                const uint16_t tag = contactTag(r->contact);
                on = tag >= goal->max && tag <= goal->arg3;
            }
            if (on) {
                status = goal->min == 0 || goal->min == 4 ? 4 : 3;
            }
        }
        break;
    case 7:
        if ((uint32_t)riderGetStateLevel(r, 0) == goal->min && r->surface == 0xc) {
            const uint16_t group = propGroup(r->contact);
            if (group >= goal->max && group <= goal->arg3) {
                status = 3;
            }
        }
        break;
    case 8:
        if ((riderGetStateLevel(r, 0) == 2 || riderGetStateLevel(r, 0) == 3) && r->surface == 9) {
            const uint16_t tag = contactTag(r->contact);
            if (tag >= goal->min && tag <= goal->max && goalBackwards(c, goal)) {
                status = 4;
            }
        }
        break;
    case 9:
        if (((riderGetStateLevel(r, 0) == 2 && physGetState(r) == 4) ||
             (riderGetStateLevel(r, 0) == 3 && physGetState(r) == 6)) &&
            r->surface == 9) {
            const void *prop = ((const PhysEdge *)r->contact)->prop;
            if (prop != NULL) {
                const uint16_t group = propGroup(prop);
                if (group >= goal->min && group <= goal->max && goalBackwards(c, goal)) {
                    status = 4;
                }
            }
        }
        break;
    case 10:
    case 11:
        if (riderGetStateLevel(r, 0) == kRiderAir && r->surface == 0xc) {
            const uint16_t group = propGroup(r->contact);
            if (group >= goal->min && group <= goal->max && !snapPropIn(&r->snapA, goal->min, goal->max)) {
                if (goal->type != 0xb) {
                    status = 3;
                } else {
                    for (uint32_t k = 0; k < c->count; k++) {
                        if (c->trick[k].trick == goal->arg3) {
                            status = 3;
                            break;
                        }
                    }
                }
            }
        }
        break;
    default:
        break;
    }
    if (status != 0) {
        gameSetGoalStatus(player, row, g, status);
    }
}

/* Fills the goal marker lists with what the goal is about (faces, edges or props by tag range). */
static void goalMarkers(const GameGoal *goal)
{
    void **a = NULL;
    void **b = NULL;

    switch (goal->type) {
    case 1:
        a = zbspFindTaggedFaces(goal->arg3, goal->arg3);
        break;
    case 2:
        a = zbspFindPropsNear(goal->arg3, goal->arg3);
        break;
    case 3:
        a = zbspFindTaggedEdges(goal->max, goal->arg3);
        break;
    case 6:
        a = zbspFindTaggedFaces(goal->max, goal->arg3);
        break;
    case 7:
        a = zbspFindPropsNear(goal->max, goal->arg3);
        break;
    case 8:
        a = zbspFindTaggedEdges(goal->min, goal->max);
        break;
    case 9:
    case 10:
    case 11:
    case 12:
        a = zbspFindPropsNear(goal->min, goal->max);
        break;
    case 13:
    case 14:
    case 15:
        a = zbspFindTaggedFaces(goal->min, goal->min);
        if (a != NULL) {
            zmemCopy((void *)gGameMarkersA, (const void *)a, sizeof gGameMarkersA);
        }
        b = zbspFindTaggedFaces(goal->max, goal->max);
        a = NULL;
        break;
    case 16:
        a = zbspFindTaggedEdges(goal->min, goal->min);
        if (a != NULL) {
            zmemCopy((void *)gGameMarkersA, (const void *)a, sizeof gGameMarkersA);
        }
        b = zbspFindTaggedEdges(goal->max, goal->max);
        a = NULL;
        break;
    case 17:
        a = zbspFindPropsNear(goal->min, goal->min);
        if (a != NULL) {
            zmemCopy((void *)gGameMarkersA, (const void *)a, sizeof gGameMarkersA);
        }
        b = zbspFindPropsNear(goal->max, goal->max);
        a = NULL;
        break;
    case 18:
        a = zbspFindTaggedFaces(goal->min, goal->min);
        if (a != NULL) {
            zmemCopy((void *)gGameMarkersA, (const void *)a, sizeof gGameMarkersA);
        }
        b = zbspFindTaggedEdges(goal->max, goal->arg3);
        a = NULL;
        break;
    case 19:
        a = zbspFindTaggedFaces(goal->min, goal->min);
        if (a != NULL) {
            zmemCopy((void *)gGameMarkersA, (const void *)a, sizeof gGameMarkersA);
        }
        b = zbspFindPropsNear(goal->max, goal->arg3);
        a = NULL;
        break;
    case 21:
        a = zbspFindTaggedFaces(goal->min, goal->max);
        break;
    default:
        break;
    }
    if (a != NULL) {
        zmemCopy((void *)gGameMarkersA, (const void *)a, sizeof gGameMarkersA);
    }
    if (b != NULL) {
        zmemCopy((void *)gGameMarkersB, (const void *)b, sizeof gGameMarkersB);
    }
}

/* A goal reached status 4: collect it, and finish its row (row props, messages) when it was the last. */
static void goalCollect(int player, GameGoalInfo *info, const GameGoalRow *row, int i, uint32_t g)
{
    gameSetGoalStatus(player, i, g, 5);
    uint8_t *flags = &info->row[info->set * 4 + i][0];
    (*flags)++;
    if ((*flags & 0x3f) == row->count) {
        info->done++;
        if (cameraPlaybackActive() == 0) {
            GameMessage m = {0};
            m.type = (uint8_t)(info->done + 2);
            if (info->done == 4) {
                m.type = (uint8_t)(m.type + info->set);
                bmxSoundPlayEvent(kSoundGoalSet, info->set, 0, 0, 0);
            } else {
                bmxSoundPlayEvent(kSoundGoalRow, info->done, 0, 0, 0);
            }
            m.value = 0;
            ovlSetElementA(kElementMessage, &m);
        }
        for (uint32_t k = 0; k < 3; k++) {
            if (row->prop[k] != 0xff) {
                uint8_t *prop = propFind(row->prop[k]);
                if (prop != NULL) {
                    propCollect((uint32_t)player, prop);
                }
            }
        }
        info->row[info->set * 4 + i][0] |= 0xc0;
    } else if (cameraPlaybackActive() == 0) {
        GameMessage m = {0};
        m.type = (uint8_t)(row->count - (*flags & 0x3f) - 1);
        ovlSetElementA(kElementMessage, &m);
        bmxSoundPlayEvent(kSoundGoalStep, *flags & 0x3f, 0, 0, 0);
    }
}

/*
 * Per frame goal logic of the career levels: goals switch on and off as the rider enters and leaves their
 * area, in-run conditions complete them, the first open goal is selected for the HUD (with its world
 * markers), and completed goals are collected, finishing their row when all of it is done.
 */
BMX_ADDR(0x0040ec40)
void gameUpdateGoals(int player)
{
    PhysRider *r = &gPhysRiders[player];
    GameSelMsg sel;

    if (gameGetMode() != 0 || gameGetSubMode() != 0 || gCurrentLevel >= 6) {
        return;
    }
    ovlSetElementB(kElementGoal, &sel);
    GameGoalInfo *info = goalInfo(player, gCurrentLevel);
    const GameGoalRow *row = gGameGoals[gCurrentLevel * 3 + info->set];
    for (int i = 0; i < 4; i++, row++) {
        if ((info->row[info->set * 4 + i][0] & 0x80) != 0) {
            continue;
        }
        for (uint32_t g = 0; g < row->count; g++) {
            const GameGoal *goal = &row->goal[g];
            int inside = 0;
            const int32_t x = bmxFtol(r->pos.x);
            if (x >= goal->area[0] && x < goal->area[2]) {
                const int32_t z = bmxFtol(r->pos.z);
                inside = z >= goal->area[1] && z < goal->area[3];
            }
            if (inside) {
                if (gameGoalStatus(player, i, g) == 0) {
                    gameSetGoalStatus(player, i, g, 1);
                }
            } else {
                const uint32_t st = gameGoalStatus(player, i, g);
                if (st == 1) {
                    gameSetGoalStatus(player, i, g, 0);
                } else if (st == 2) {
                    sel.id = 0xff;
                    sel.timer = 0;
                    ovlSetElementA(kElementGoal, &sel);
                    gameSetGoalStatus(player, i, g, 0);
                }
            }
            if (gameGoalStatus(player, i, g) != 0 && gameGoalStatus(player, i, g) < 3) {
                goalCheck(player, r, i, g, goal);
            }

            const uint32_t st = gameGoalStatus(player, i, g);
            if (st == 0 || st == 3) {
                continue;
            }
            if (st == 4) {
                goalCollect(player, info, row, i, g);
                continue;
            }
            if (st > 4 || (goal->type == 0x14 && goal->min == 1)) {
                continue;
            }
            if (sel.timer != 0) {
                if (sel.id != (uint32_t)(info->set * 4 + i)) {
                    gameSetGoalStatus(player, i, g, 1);
                }
                continue;
            }
            sel.timer = 0xff;
            sel.id = (uint8_t)(info->set * 4 + i);
            ovlSetElementA(kElementGoal, &sel);
            gameSetGoalStatus(player, i, g, 2);
            gGameMarkersA[0] = NULL;
            gGameMarkersB[0] = NULL;
            goalMarkers(goal);
            gGameMarkersA[63] = NULL;
            gGameMarkersB[63] = NULL;
            if (gameGoalFlag(player, i, g, 1) != 0) {
                gameSetGoalFlag(player, i, g, 1, 0);
            }
            gGameFrame = 0;
        }
    }
}

/* Picks a random start point on a respawn face for a free ride started from a record (mode 4). */
static void pickStartPos(void)
{
    const float *b = zrollerGetWorldBounds();
    void *face;

    gGameStartPos[1] = 0.0f;
    do {
        int32_t w = bmxFtol((double)b[4] - b[0]);
        int16_t off = (int16_t)(((zmathRandom() & 0xff) * (uint32_t)w) >> 8);
        gGameStartPos[0] = (float)((double)off + b[0]);
        w = bmxFtol((double)b[6] - b[2]);
        off = (int16_t)(((zmathRandom() & 0xff) * (uint32_t)w) >> 8);
        gGameStartPos[2] = (float)((double)off + b[2]);
        face = physFindRespawnFace((const ZVec3 *)(const void *)gGameStartPos, 3);
    } while (face == NULL);
    zbspFaceCentroid(face, (ZVec3 *)(void *)gGameStartPos);
}

/* Run type 3: clears the run records and resets the riders for a new run. */
static uint32_t startRun(void)
{
    ovlSetScreen(0);
    for (uint32_t slot = 0; slot < 10; slot++) {
        uint8_t *rec = gameGetRecord((modeKey() << 6 | slot) << 16 | 0x3000);
        if (rec != NULL) {
            *(uint32_t *)(void *)rec = 0;
            rec[5] = 0;
        }
        rec = gameGetRecord((modeKey() << 6 | slot) << 16);
        if (rec != NULL) {
            rec[5] = 0;
        }
    }
    if (gCurrentPlayer == gameGetStartPlayer()) {
        gameSetOption(4, 1);
        for (uint32_t slot = 0; slot < 10; slot++) {
            uint8_t *rec = gameGetRecord((modeKey() << 6 | slot) << 16 | 0x2000);
            if (rec != NULL) {
                *(uint32_t *)(void *)rec = 0;
                rec[5] = 0;
            }
        }
        for (uint32_t k = 0; k < 4; k++) {
            for (uint32_t slot = 0; slot < 10; slot++) {
                uint8_t *rec = gameGetRecord(((modeKey() << 6 | slot) << 4 | k) << 12);
                if (rec != NULL) {
                    rec[5] = 0;
                }
            }
        }
        if (gameGetMode() == 4) {
            pickStartPos();
        }
        gameSetOption(2, 0);
        gameSetOption(0x10, 0);
        gGameHorseCount = 0;
    } else {
        gameSetOption(4, 0);
    }
    for (uint32_t i = 0; i < gPlayerCount; i++) {
        if (!activeRider(i)) {
            riderReset((int)i, 0);
            continue;
        }
        PhysRider *r = &gPhysRiders[i];
        const OvlProfile *profile = &gProfiles[i];
        gameResetRider((int)i, 1);
        gameNop();
        r->padIndex = profile->pad02;
        r->combo.score = 0;
        r->combo.runState = 0;
        r->combo.count = 0;
        memset((uint8_t *)r + offsetof(PhysRider, combo) + 0x93, 0,
               0x1d6); /* trick grid and the rest of the run */
        const GameScoreMsg shown = {0, 1, {0}};
        ovlSetElementA(kElementScore, &shown);
        memset((uint8_t *)r + offsetof(PhysRider, area), 0, 0xee); /* area statistics and their totals */
        r->hudByte = 0xff;
        const uint8_t hud = 0xff;
        ovlSetElementA(kElementHud, &hud);
        if (gameGetMode() == 0 && (profile->flags & 2) != 0) {
            r->hintMask = 0x3fffff;
        }
    }
    cameraSelectPlayer(gCurrentPlayer);
    comdataResetView(gCurrentPlayer, 3);
    const GameMessage none = {0xff, 0, 0, 0};
    ovlSetElementA(kElementHint, &none);
    ovlSetElementA(kElementGoal, &none);
    const uint32_t bail = 0xff;
    ovlSetElementA(kElementBail, &bail);
    ovlSetElementA(kElementMessage, &none);
    gGameTimer = 300;
    const uint32_t mode = gameGetMode();
    if (mode == 0) {
        if (gameGetSubMode() == 0 && gCurrentLevel < 6) {
            ovlSetScreen(0xd);
            return gGameRunType;
        }
    } else if (mode != kModeNone) {
        ovlSetScreen(0x18);
        bmxSoundPlayEvent(kSoundRunEnd, 0, 0, 0, 0);
        return gGameRunType;
    }
    return gameBigGameFunction(4);
}

/* Run type 4: the run starts; sets the run timer of the mode. */
static void beginRun(void)
{
    for (uint32_t i = 0; i < gPlayerCount; i++) {
        if (!activeRider(i)) {
            continue;
        }
        const uint8_t replay = gGameReplays[i];
        gPhysRiders[i].combo.runState = 0;
        gPhysRiders[i].combo.count = 0;
        replaySetRider(replay, i);
        replaySetMode(gGameReplays[i], 1);
        if (modeRecord(0) == NULL) {
            zdebugFail("", gameFile, 0x415, 0);
        }
        cameraSelectPlayer(i);
        comdataResetView((int)i, 0);
    }
    switch (gameGetMode()) {
    case 0:
        gGameTimer = gGameRideSeconds * 30;
        bmxSoundPlayEvent(kSoundRunEnd, 1, 0, 0, 0);
        return;
    case 1:
    case kModeNone:
        switch (gameGetSubMode()) {
        case 0:
            break;
        case 1:
            gGameTimer = 0x708;
            return;
        case 3:
            gGameTimer = 9000;
            return;
        default:
            gGameTimer = gGameRideSeconds * 30;
            return;
        }
        break;
    case 4:
        gGameTimer = 0x1c2;
        return;
    default:
        break;
    }
    gGameTimer = 900;
}

/* Keeps the better of a record and a value (keys of the run records): the record notes the rider. */
static void beatRecord(uint32_t key, uint32_t value, uint32_t rider)
{
    uint8_t *rec = gameGetRecord(key);
    if (rec != NULL && value > *(uint32_t *)(void *)rec) {
        *(uint32_t *)(void *)rec = value;
        rec[4] = (uint8_t)rider;
        rec[5] |= 1;
    }
}

static uint32_t endScreen(uint32_t screen)
{
    ovlSetScreen(screen);
    cameraSelectPlayer(0);
    comdataResetView(0, 4);
    return gGameRunType;
}

/* Round end of a multi player mode: horse letters, leader, draws and the last round. */
static void endRound(void)
{
    if (gameGetMode() == 3) {
        uint8_t low = (uint8_t)strlen(gGameHorseWord);
        uint32_t out = 0;
        gameSetOption(0x10, 1);
        for (uint32_t i = 0; i < gPlayerCount; i++) {
            RiderCombo *c = &gPhysRiders[i].combo;
            if (c->score == 0) {
                c->landed++;
                if ((uint32_t)c->landed >= strlen(gGameHorseWord)) {
                    out++;
                }
                gameSetOption(0x10, 0);
            } else if ((uint32_t)c->landed < low) {
                gGameLeader = (uint8_t)i;
                low = (uint8_t)c->landed;
            }
        }
        gameSetOption(1, out == (uint32_t)gPlayerCount - 1);
        return;
    }
    const uint8_t *rec = gameGetRecord(modeKey() << 22 | 0x12000);
    if (rec != NULL) {
        gameSetOption(0x10, 0);
        for (uint32_t i = 0; i < gPlayerCount; i++) {
            if (i != rec[4] && gPhysRiders[i].combo.score == *(const uint32_t *)(const void *)rec) {
                gameSetOption(0x10, 1);
            }
        }
        if (!gameTestOption(0x10)) {
            gPhysRiders[rec[4]].combo.landed++;
            gGameRoundStates[gameGetRunCount()] = rec[4];
        }
    }
    if (gameGetMode() == 2) {
        if (gameTestOption(0x10)) {
            return;
        }
        uint8_t high = 0;
        gGameLeader = 0;
        gameSetOption(8, 0);
        for (uint32_t i = 0; i < gPlayerCount; i++) {
            const uint32_t wins = (uint32_t)gPhysRiders[i].combo.landed;
            if (wins > high) {
                gGameLeader = (uint8_t)i;
                high = (uint8_t)wins;
                gameSetOption(8, 0);
            } else if (wins == high) {
                gameSetOption(8, 1);
            }
        }
    }
    if (gameGetRoundCount() != 0 && gameGetRunCount() == gameGetRoundCount() - 1) {
        gameSetOption(1, 1);
    }
}

/* Run type 5: the run is over; records, judges, rounds and the next screen. */
static uint32_t endRun(void)
{
    for (uint32_t i = 0; i < gPlayerCount; i++) {
        if (activeRider(i)) {
            replaySetMode(gGameReplays[i], 0);
        }
    }
    gGameReplayShared = gGameReplays[gCurrentPlayer];
    if (gameGetMode() == 3) {
        const uint32_t cur = gCurrentPlayer;
        if (gameTestOption(4)) {
            if (gPhysRiders[cur].combo.score == 0) {
                gGameStartPlayer = (uint8_t)((gGameStartPlayer + 1) % gPlayerCount);
                gameSetOption(0x10, 1);
                gameSetOption(2, 1);
                return gameBigGameFunction(3);
            }
            gGameRoundStates[gameGetRunCount()] = gCurrentPlayer;
        }
    }
    if (gameGetMode() != 0 || gameGetSubMode() != 2) {
        for (uint32_t i = 0; i < gPlayerCount; i++) {
            if (activeRider(i)) {
                for (uint32_t k = 0; k < 3; k++) {
                    beatRecord((modeKey() << 10 | k | 0x10) << 12, gPhysRiders[i].combo.score, i);
                }
                gameJudgeRun((int)i);
            }
        }
    }
    const uint32_t mode = gameGetMode();
    if (mode < 2 || mode > 3) {
        RiderCombo *c = &gPhysRiders[gCurrentPlayer].combo;
        const uint32_t m = gameGetMode();
        if (m == 0) {
            c->total = (int32_t)c->score;
        } else if (m > 4 && m <= 10) {
            if (c->score > (uint32_t)c->total) {
                c->total = (int32_t)c->score;
            }
        } else {
            for (uint32_t i = 0; i < gPlayerCount; i++) {
                if (activeRider(i)) {
                    gPhysRiders[i].combo.total += (int32_t)gPhysRiders[i].combo.score;
                }
            }
        }
        if (gameGetMode() != 0 || gameGetSubMode() != 2) {
            for (uint32_t i = 0; i < gPlayerCount; i++) {
                if (activeRider(i)) {
                    for (uint32_t k = 0; k < 3; k++) {
                        beatRecord((modeKey() << 10 | k) << 12, (uint32_t)gPhysRiders[i].combo.total, i);
                    }
                }
            }
        }
    }
    if ((gCurrentPlayer + 1u) % gPlayerCount == (gameGetStartPlayer() & 0xff)) {
        gameSetOption(2, 1);
        endRound();
        if (gameGetMode() != 0) {
            const uint8_t *rec = modeRecord(0x1000);
            if (rec == NULL) {
                zdebugFail("", gameFile, 0x4ee, 0);
            }
            if ((rec[5] & 1) != 0) {
                const uint8_t all = gGameReplayAll;
                gGameReplayAll = gGameReplays[rec[4]];
                gGameReplays[rec[4]] = all;
            }
        }
    }
    for (uint32_t i = 0; i < gPlayerCount; i++) {
        riderReset((int)i, 0);
    }
    if (!gameTestOption(1)) {
        for (uint32_t slot = 0; slot < 10; slot++) {
            const uint8_t *rec = gameGetRecord((modeKey() << 6 | slot) << 16);
            if (rec != NULL && (rec[5] & 1) != 0) {
                return endScreen(0x11);
            }
        }
    }
    switch (gameGetMode()) {
    case 0:
        if (gameTestOption(1)) {
            return gameBigGameFunction(6);
        }
        return endScreen(8);
    case 3:
        if (gameTestOption(0x10) || !gameTestOption(2)) {
            return gameBigGameFunction(3);
        }
        break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        return gameBigGameFunction(gameTestOption(1) ? 6 : 3);
    default:
        break;
    }
    cameraSelectPlayer(gCurrentPlayer);
    comdataResetView(gCurrentPlayer, 4);
    ovlSetScreen(0x19);
    return gGameRunType;
}

/* Run type 6: the session is over; ties, unlocks, the record screen and the save. */
static uint32_t endSession(void)
{
    if (gameGetMode() != 2) {
        const uint8_t *rec = modeRecord(0x1000);
        gameSetOption(8, 0);
        for (uint32_t i = 0; i < gPlayerCount; i++) {
            if (i != rec[4] && (uint32_t)gPhysRiders[i].combo.total == *(const uint32_t *)(const void *)rec) {
                gameSetOption(8, 1);
            }
        }
    }
    gameUpdateUnlocks(gCurrentPlayer);
    for (uint32_t slot = 0; slot < 10; slot++) {
        const uint8_t *rec = gameGetRecord((modeKey() << 6 | slot) << 16);
        if (rec != NULL && (rec[5] & 1) != 0) {
            return endScreen(0x11);
        }
    }
    if (gameGetMode() == 0 && gameGetSubMode() == 0) {
        if (gCurrentLevel >= 6) {
            return endScreen(8);
        }
        if (gameHasPendingUnlock(gCurrentPlayer)) {
            return endScreen(0x10);
        }
    }
    saveFileWrite();
    ovlSetScreen(10);
    gOvlSaving = 1;
    return gGameRunType;
}

/*
 * The session's run phase driver (community name "BigGameFunction"): switches the run type to `type`
 * (1 menu, 3 new run, 4 run starts, 5 run over, 6 session over) and does what the new phase needs.
 * Starting a new run after a finished one first passes the turn to the next player. Returns the run
 * type, which the nested calls may have moved on.
 */
BMX_ADDR(0x0040aa50)
uint32_t gameBigGameFunction(int type)
{
    if (gGameRunType == kPhaseEnd && gameGetMode() != kModeNone && type == 3) {
        gCurrentPlayer = (uint8_t)((uint32_t)(uint8_t)(gCurrentPlayer + 1) % gPlayerCount);
        if (gameTestOption(2)) {
            if (gameGetMode() == 3 && !gameTestOption(0x10)) {
                const uint8_t round = (uint8_t)gameGetRoundState(gameGetRunCount(), 2);
                gGameStartPlayer = (uint8_t)((uint32_t)(uint8_t)(round + 1) % gPlayerCount);
            }
            gCurrentPlayer = (uint8_t)gameGetStartPlayer();
            const uint32_t mode = gameGetMode();
            if ((mode >= 5 && mode <= 10) || !gameTestOption(0x10)) {
                gGameRunCount++;
            }
        }
    }
    gGameRunType = (uint8_t)type;
    gGamePhase = 0;
    gGameTimer = 0;
    switch (type) {
    case 1:
        if (gameGetMode() != 0) {
            ovlSetScreen(0xc);
            return gGameRunType;
        }
        comdataResetView(gCurrentPlayer, 4);
        ovlSetScreen(0xc);
        bmxSoundPlayEvent(kSoundRunEnd, 4, 0, 0, 0);
        break;
    case 3:
        return startRun();
    case 4:
        beginRun();
        break;
    case 5:
        return endRun();
    case 6:
        return endSession();
    default:
        break;
    }
    return gGameRunType;
}
