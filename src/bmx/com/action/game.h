/*
 * Game session logic (C:\zaxis\Bmx\src\com\action\game.c): game modes, the run timer and phases, the
 * goal bookkeeping of the career levels, high score records and the world markers that show goals.
 *
 * A session has a mode (0 free ride/career, 2 and 3 are the competition modes, 6..10 timed modes), a
 * sub mode, a run type (the phase machine of gameTick advances gGamePhase) and per-level goal rows.
 */
#ifndef BMX_COM_ACTION_GAME_H
#define BMX_COM_ACTION_GAME_H

#include <stddef.h>
#include <stdint.h>

#include "bmx/com/action/physics_types.h"
#include "compat/orig.h"
#include "zge/zmath.h"

/* Session state (.data / .bss). */
#define gGameOptions     ORIG_VAR(0x0056b700, uint8_t, gGameOptions) /* bit 3: competition run */
#define gGameMode        ORIG_VAR(0x0056b701, uint8_t, gGameMode)
#define gGameSubMode     ORIG_VAR(0x0056b702, uint8_t, gGameSubMode)
#define gGameRunCount    ORIG_VAR(0x0056b703, uint8_t, gGameRunCount)
#define gGameRunType     ORIG_VAR(0x0056b704, uint8_t, gGameRunType)
#define gGamePhase       ORIG_VAR(0x0056b705, uint8_t, gGamePhase)
#define gGamePhaseTicks  ORIG_VAR(0x0056b706, uint8_t, gGamePhaseTicks)
#define gGameStartPlayer ORIG_VAR(0x0056b707, uint8_t, gGameStartPlayer)
#define gGameTimer       ORIG_VAR(0x0056b708, uint32_t, gGameTimer) /* frames left in the run */
#define gGameFrozen      ORIG_VAR(0x0056bc30, uint32_t, gGameFrozen)
#define gGameFrame       ORIG_VAR(0x0056bc2c, uint32_t, gGameFrame)

/* Replay recorders: one per player, plus one for all of them (and the first player's, repeated). */
typedef uint8_t GameReplayHandles[4];
#define gGameReplayAll    ORIG_VAR(0x0056b6f8, uint8_t, gGameReplayAll)
#define gGameReplayShared ORIG_VAR(0x0056b6f9, uint8_t, gGameReplayShared)
#define gGameReplays      ORIG_VAR(0x0056b6fc, GameReplayHandles, gGameReplays)

/* Where the player starts in the run type 4 (a free ride started from a record). */
typedef float GameStartPos[3];
#define gGameStartPos ORIG_VAR(0x0056b6e8, GameStartPos, gGameStartPos)

/* World extent / 4 of the level (x, z). */
typedef uint16_t GameExtent[2];
#define gGameExtent ORIG_VAR(0x0056bc28, GameExtent, gGameExtent)

/* Score records: 8 bytes each, {u32 value; u8 flags at +5}, in tables selected by gameGetRecord. */
typedef uint8_t GameRecordBytes[0x1000];
#define gGameRecords ORIG_VAR(0x0056b70c, GameRecordBytes, gGameRecords)
typedef uint8_t GameRecordSlot[16];

/* Rounds of each mode, the state of each round of the current competition, the word being spelled. */
typedef uint8_t GameRoundCounts[12];
typedef uint8_t GameRoundStates[16];
typedef char GameHorseWord[12];
#define gGameRoundCounts ORIG_VAR(0x00554c8c, GameRoundCounts, gGameRoundCounts)
#define gGameRoundStates ORIG_VAR(0x0056ba0c, GameRoundStates, gGameRoundStates)
#define gGameHorseWord   ORIG_VAR(0x0056ba1c, GameHorseWord, gGameHorseWord)
#define gGameDefaultWord ORIG_VAR(0x00554d00, uint32_t, gGameDefaultWord)

/* World markers that show the goals: two lists of marker objects ended by NULL. */
typedef void *GameMarkerList[64];
#define gGameMarkersA ORIG_VAR(0x0056ba28, GameMarkerList, gGameMarkersA)
#define gGameMarkersB ORIG_VAR(0x0056bb28, GameMarkerList, gGameMarkersB)
typedef uint32_t GameMarkerColors[3];
#define gGameMarkerColors ORIG_VAR(0x00554cb4, GameMarkerColors, gGameMarkerColors)

/* One goal of a goal row (type 0..0x15), its parameters and the props it unlocks. */
typedef struct GameGoal {
    uint16_t type;
    uint16_t min; /* first parameter (min/max/arg3: meaning depends on the type) */
    uint16_t max;
    uint16_t arg3;
    int16_t area[4]; /* x0, z0, x1, z1: the goal is active while the rider is inside */
    uint8_t prop[4]; /* 0x10 props (ids into the prop table) that appear when the goal is met */
} GameGoal;

/* A row: up to four goals. */
typedef struct GameGoalRow {
    uint8_t count;
    uint8_t prop[3];
    GameGoal goal[4];
} GameGoalRow;

typedef GameGoalRow GameGoalSet[4];
typedef GameGoalSet GameGoalTable[18]; /* [level * 3 + set] */
#define gGameGoals ORIG_VAR(0x00555118, GameGoalTable, gGameGoals)

/* Per player view of one level's goals (0x27 bytes, comdataGetGoalInfo). */
typedef struct GameGoalInfo {
    uint8_t pad00;
    uint8_t set;        /* goal set being played: 0..2 */
    uint8_t done;       /* rows finished in the current set */
    uint8_t row[12][3]; /* [set * 4 + row]: flags (0x80 locked, 0x40 reward pending), status of goals 0/1 and
                           2/3 (nibbles) */
} GameGoalInfo;

_Static_assert(sizeof(GameGoal) == 0x14 && offsetof(GameGoal, area) == 8, "GameGoal layout");
_Static_assert(sizeof(GameGoalRow) == 0x54, "GameGoalRow size");
_Static_assert(sizeof(GameGoalSet) == 0x150, "GameGoalSet size");
_Static_assert(sizeof(GameGoalInfo) == 0x27, "GameGoalInfo size");

/* Per rider unlock data (0x104 bytes each, at 0x6955e8; the overlay screens read it too). */
typedef struct GameRiderData {
    uint32_t flags; /* 1 .. 0x100 unlock bits (0x40 / 0x80 / 0x100: progress, 2: all levels done) */
    uint8_t levelsA;
    uint8_t levelsB;
    uint8_t levelsC;
    uint8_t pad07[0xe4];
    uint8_t levelDone[12]; /* 0xeb: sets completed per level */
    uint8_t pad0f7[0xd];
} GameRiderData;

_Static_assert(sizeof(GameRiderData) == 0x104, "GameRiderData size");
typedef GameRiderData GameRiderDataTable[12];
#define gGameRiders     ORIG_VAR(0x006955e8, GameRiderDataTable, gGameRiders)
#define gGameUnlockBits ORIG_VAR(0x006955e0, uint16_t, gGameUnlockBits)

/* Competition judges: ten of them, each with a score per run and the average (0x695200), their ranking
 * (0x69521e), the par score of the competition levels (index: level, 6..11) and each judge's bias. */
typedef uint8_t GameJudgeScores[10][3];
typedef uint8_t GameJudgeRank[10];
typedef uint32_t GameJudgePar[12];
typedef int8_t GameJudgeBias[10];
#define gGameJudgeScores ORIG_VAR(0x00695200, GameJudgeScores, gGameJudgeScores)
#define gGameJudgeRank   ORIG_VAR(0x0069521e, GameJudgeRank, gGameJudgeRank)
#define gGameJudgePar    ORIG_VAR(0x00554c80, GameJudgePar, gGameJudgePar)
#define gGameJudgeBias   ORIG_VAR(0x00554cc0, GameJudgeBias, gGameJudgeBias)

/* The horse game: the tricks the next rider must repeat, and the rider in the lead. */
typedef RiderTrick GameHorseTricks[3];
#define gGameHorseTricks ORIG_VAR(0x00695260, GameHorseTricks, gGameHorseTricks)
#define gGameHorseCount  ORIG_VAR(0x00695290, uint8_t, gGameHorseCount)
#define gGameLeader      ORIG_VAR(0x0056ba1b, uint8_t, gGameLeader)

/* Length of a timed free ride, in seconds. */
#define gGameRideSeconds ORIG_VAR(0x00554cb0, uint32_t, gGameRideSeconds)

/* Session functions. */
int gameTestOption(uint32_t mask);
void gameSetOption(uint32_t mask, int on);
uint32_t gameStartMode(int mode);
uint32_t gameGetMode(void);
uint32_t gameSetSubMode(uint32_t subMode);
uint32_t gameGetSubMode(void);
uint32_t gameGetRunType(void);
uint32_t gameGetRunCount(void);
uint32_t gameGetStartPlayer(void);
uint32_t gameGetRoundCount(void);
uint32_t gameGetRoundState(uint32_t round, uint32_t kind);
uint8_t *gameGetRecord(uint32_t key);
char *gameSetHorseWord(const char *word);
const char *gameGetNameTable(void);
uint32_t gameGetModeByte(void);
void gameLeaveRun(void);
void gameInit(void);
void gameShutdown(void);
void gameTick(void);
void gameNop(void);

/* Goals. */
void gameResetGoals(int player);
int gameHasPendingUnlock(int player);
int gameCheckPropGoal(int player, void *prop);
int gameCheckGrindGoal(int player, const PhysEdge *edge);
void gameSubmitScore(int player, uint32_t kind, uint32_t value, uint32_t sound, uint32_t flags);
int gameRiderBusy(const PhysRider *r);
void gameEvaluateGoals(int player, int finished);
void gameScoreCombo(RiderCombo *combo);
uint32_t gameGoalStatus(int player, int row, uint32_t goal);
void gameSetGoalStatus(int player, int row, uint32_t goal, int status);
uint32_t gameGoalFlag(int player, int row, uint32_t goal, uint32_t mask);
void gameSetGoalFlag(int player, int row, uint32_t goal, uint32_t flag, int on);
void gameUpdateUnlocks(int player);
void gameCompleteGoal(int player, const uint8_t *reward);
void gameResetRider(int player, int full);

/* World markers and effects. */
int gameDrawGoalMarkers(void);
void gameDrawPlaneMarker(const void *marker, uint32_t color);
void gameDrawBoxMarker(const void *marker, uint32_t color);
void gameDrawPropMarker(const void *marker, uint32_t color);
void gameDrawEdgeMarker(const void *marker, uint32_t color);

/* Run phases, combos, judges and the goals of the current run. */
uint32_t gameBigGameFunction(int type);
void gameScoreJump(int player, int landed);
void gameUpdateCombo(int player);
void gameEndCombo(int player, int landed);
void gameUpdateHints(int player);
void gameUpdateGoals(int player);
void gameJudgeRun(int player);

#endif
