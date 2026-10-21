/* Action loop (C:\zaxis\Bmx\src\com\action\action.c). */
#include "bmx/com/action/action.h"

#include "bmx/com/action/camera.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/fx.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/prop.h"
#include "bmx/com/action/replay.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/ngif/missingzge.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/com/zge/zactor.h"
#include "zge/zasset.h"
#include "zge/zdebug.h"

#define ACTION_FILE "C:\\zaxis\\Bmx\\Src\\com\\action\\action.c"

/* Overlay elements the screens switch together; a set bit means "leave this element alone". */
enum {
    kSkipOnlyBar = 0x10000,
    kSkipPauseKeep = 0x1e607,
    kSkipStartKeep = 0x16201,
    kSkipRunKeep = 0x1e201,
};

/* Button masks of contrlerPressed. */
enum {
    kPadStart = 0x10,
    kPadSelect = 0x20,
    kPadBack = 0x40,
    kPadView = 0x80,
    kPadMenu = 0xf0,
    kPadHoldA = 0x4000,
    kPadHoldB = 0x8000,
    kPadAny = 0x14000,
};

/* Event data of overlay element 0x10 (6 bytes). */
typedef struct ActionFade {
    uint8_t kind;
    uint8_t color[3];
    uint16_t ticks;
} ActionFade;

/* Description of the demo replay buffer handed to replayCreate. */
typedef struct ActionReplayDesc {
    uint8_t flags;
    uint8_t pad01;
    uint16_t pad02;
    uint32_t size;
} ActionReplayDesc;

static void setElements(uint32_t skip, uint32_t show)
{
    for (uint32_t i = 0; i < 0x11; i++) {
        if ((skip & (1u << i)) == 0) {
            ovlSetElementVisible((int)i, show);
        }
    }
}

static void setFade(uint8_t kind)
{
    ActionFade fade;

    fade.kind = kind;
    fade.color[0] = 0xff;
    fade.color[1] = 0xff;
    fade.color[2] = 0xff;
    fade.ticks = 0x1e;
    ovlSetElementA(0x10, &fade);
}

static void setScreenCode(int index, uint8_t value)
{
    ovlSetElementA(index, &value);
}

/* Sets the camera view of the rider of a replay back to what it is (the original reads it first). */
static void refreshView(uint32_t player)
{
    const uint32_t rider = replayGetRider(player);
    const uint32_t view = comdataGetView(rider);
    const uint32_t rider2 = replayGetRider(player);

    comdataSetView(rider2, view);
}

/* Remembers the view of the rider and switches it to the first one. */
static void saveView(uint32_t player)
{
    const uint32_t rider = replayGetRider(player);
    const uint32_t view = comdataGetView(rider);
    const uint32_t rider2 = replayGetRider(player);

    gAction.savedView = (uint8_t)view;
    comdataSetView(rider2, 0);
}

BMX_ADDR(0x0040249b)
int actionShutdownRoller(void)
{
    return zrollerShutdown();
}

BMX_ADDR(0x00403c30)
int actionOpenMotions(void)
{
    if (zrollerInit() != 0) {
        return 1;
    }
    zrollerSetLocal();
    if (zmotionOpenLibrary("Assets\\MOTION.ZAL") != 0) {
        gZdebugPrint("ZARTVIEW: Failed to open motions %s\n", "Assets\\MOTION.ZAL");
        zdebugFail("Failed to open motions", ACTION_FILE, 0xbf, 0);
        return 1;
    }
    comdataInit();
    zsystemSetMenuMode();
    return 0;
}

BMX_ADDR(0x00403cc0)
void actionCloseMotions(void)
{
    zmotionCloseLibrary();
}

BMX_ADDR(0x00403ce0)
uint32_t actionPickDemo(void)
{
    const uint32_t counter = gAction.random;

    gCurrentLevel = gActionDemoTable[(counter % 3) * 2 + 1];
    return counter / 3;
}

BMX_ADDR(0x00403d10)
uint32_t actionInitialize(void)
{
    ZMotionBank *bank;
    uint32_t i;
    uint32_t count;

    contrlerCreateDevices();
    zpadGetDevice(0);
    zrollerLoadCodeModule();
    zrollerInitializeLoadingScreen();
    if (zrollerStartup() == 1) {
        return 1;
    }
    physRegisterDebugDraw();
    fxInit();
    zactorSetAngleBase(0xc000, 1);
    if (gPlayerCount == 0) {
        const uint32_t pick = gAction.random % 3 * 2;

        gProfiles[0].pad04[0] = 4;
        gProfiles[0].pad04[1] = 0;
        gProfiles[0].character = gActionDemoTable[pick];
        gCurrentLevel = gActionDemoTable[pick + 1];
        gAction.random = (uint8_t)(gAction.random + 1);
        zrollerSetMap();
        if (riderCreate(0, gProfiles[0].character, gProfiles[0].pad04[0], gProfiles[0].pad04[1]) == NULL) {
            gZdebugPrint("Failed to create demo rider\n");
            return 1;
        }
    } else {
        zrollerSetMap();
        for (i = 0; i < gPlayerCount; i++) {
            if (riderCreate(i, gProfiles[i].character, gProfiles[i].pad04[0], gProfiles[i].pad04[1]) ==
                NULL) {
                gZdebugPrint("Failed to create rider %ld\n", i);
                return 1;
            }
        }
    }
    ovlLoadFonts();
    if (zrollerLoadAssets() != 0) {
        gZdebugPrint("Failed to load assets\n");
        return 1;
    }
    ovlLoadStrings(gActionLanguage);
    zoverInit();
    ovlInit();
    ovlSetElementVisible(0x10, 1);
    cameraInit();
    bank = zmotionLoadBank(0);
    if (bank == NULL) {
        gZdebugPrint("ZARTVIEW: Failed to open motion bank %d\n", 0);
        return 1;
    }
    count = 0;
    for (;;) {
        const ZMotionEntry *entry = zmotionBankNext(bank);

        if (entry == NULL) {
            break;
        }
        gActionMotions[count] = (uint32_t)(uintptr_t)entry->motion;
        count++;
    }
    if (count >= 0xef) {
        zdebugFail("Code does not match motion asset file.", ACTION_FILE, 0x1f2, 0);
    }
    replayInit();
    bmxSoundPlayAt(1000, (const void *)(uintptr_t)2, gCurrentLevel, (gActionOptions >> 5) & 1, 0);
    propInit();
    gActionMode = 0;
    gAction.step = 0;
    gAction.field08 = 0;
    gDemoPlayer = 0xff;
    gCurrentPlayer = 0;
    gameInit();
    cameraSelectPlayer(0);
    comdataResetView(0, 4);
    if (gPlayerCount == 0) {
        actionSetMode(kActionPlayback);
    } else if (gPlayerCount == 1) {
        gameSetSubMode(gActionSubMode);
        gameStartMode(0);
    } else {
        ovlSetScreen(0x16);
    }
    zrollerShutdownLoadingScreen();
    return 0;
}

BMX_ADDR(0x004040c0)
uint32_t actionStopSounds(void)
{
    bmxSoundUpdateStreams();
    return 0;
}

/*
 * The frame is one big switch on the mode and then on the step; the original is a tangle of jumps between
 * the cases, so this follows its blocks (labels carry their addresses).
 */
BMX_ADDR(0x004040e0)
uint32_t actionUpdate(void)
{
    uint32_t *const padFlags = *(uint32_t **)contrlerGet(0);
    const uint32_t buttons = contrlerPoll();
    uint32_t player = 0;
    uint32_t i;
    uint32_t value;

    if (contrlerPressed(gCurrentPlayer, kPadHoldA, 2) != 0 &&
        contrlerPressed(gCurrentPlayer, kPadHoldB, 2) != 0) {
        gAction.holdCount = (uint16_t)(gAction.holdCount + 1);
        if (gAction.holdCount > 0x3c) {
            gActionQuit = 1;
            gAction.holdCount = 0;
            zsystemSetMenuMode();
            return 0;
        }
    } else {
        gAction.holdCount = 0;
    }
    gActionFrameCount--;
    if (gActionFrameCount != 0) {
        goto L_474c;
    }
    gActionFrameCount = gActionFrameReload;
    if ((buttons & 1) != 0 && cameraPlaybackActive() == 0) {
        bmxSoundPlayAt(1000, (const void *)(uintptr_t)3, 0, 0, 0);
        ovlSetScreen(1);
    }
    value = cameraPlaybackActive();
    if (value > 4) {
        goto L_4725;
    }
    switch (value) {
    case 0:
        if (contrlerPressed(gCurrentPlayer, kPadAny, 0) != 0) {
            bmxSoundPlayAt(1000, (const void *)(uintptr_t)3, 0, 0, 0);
            ovlSetScreen(1);
        }
        if (cameraPlaybackActive() != 0) {
            goto L_4720;
        }
        replayUpdate();
        riderReadPads();
        goto L_4211;
    case 1:
        for (i = 0; i < 2; i++) {
            uint8_t *const pad = contrlerGet(i);
            uint32_t *const padWords = (uint32_t *)(void *)pad;

            if ((padWords[5] & 0xfffffcffu) != 0) {
                *(uint16_t *)(void *)(pad + 0xe) = 0x14;
                gAction.timer = 0;
            } else {
                uint16_t hold = *(uint16_t *)(void *)(pad + 0xe);

                if (hold == 0) {
                    const uint32_t keys = padWords[4];

                    padWords[5] = keys & 0xf;
                    hold = (keys & 0x300) != 0 ? 0 : 3;
                } else {
                    hold = (uint16_t)(hold - 1);
                }
                *(uint16_t *)(void *)(pad + 0xe) = hold;
            }
        }
        ovlUpdate();
        if (cameraPlaybackActive() != 1) {
            return 0;
        }
        value = ovlGetScreenId();
        if (value < 9 || (value > 0xb && value != 0x19)) {
            goto L_4720;
        }
        replayUpdate();
        player = (uint8_t)gameGetModeByte();
        value = replayGetState(player);
        if (value == 2) {
            goto L_4211;
        }
        if (value != 4) {
            goto L_4720;
        }
        replaySetMode(player, 2);
        comdataResetView(replayGetRider(player), 1);
        comdataSetView(replayGetRider(player), 8);
        goto L_4720;
    case 2:
    case 3:
        if (cameraPlaybackActive() == 3) {
            player = gAction.player;
        } else {
            player = (uint8_t)gameGetModeByte();
        }
        if (gAction.step > 7) {
            goto L_4725;
        }
        switch (gAction.step) {
        case 0:
            goto L_434b;
        case 2:
            goto L_478a;
        case 3:
        case 5:
            goto L_4820;
        default:
            goto L_437a;
        }
    default: /* 4 */
        player = gAction.replay;
        switch (gAction.step) {
        case 0:
            goto L_49ea;
        case 1:
            goto L_4a2d;
        case 2:
        case 3:
            goto L_4ae6;
        default:
            goto L_4725;
        }
    }

L_4211:
    riderUpdateInput();
    physUpdateRiders();
    propUpdate();
    gameTick();
    goto L_4720;

L_434b:
    gAction.timer++;
    if (gAction.timer == 1) {
        goto L_49fe;
    }
L_435f:
    if (gAction.timer != 0x1e) {
        goto L_4725;
    }
    gAction.step = 1;
    gAction.timer = 0;
    goto L_4725;

L_437a:
    i = 0;
    if (gPlayerCount == 0) {
        goto L_4548;
    }
L_438d:
    if (gDemoPlayer != 0xff && gDemoPlayer != (uint8_t)i) {
        goto L_4475;
    }
    if (contrlerPressed((int)i, kPadAny, 0) != 0) {
        gAction.step = 2;
        gAction.timer = 0;
        setScreenCode(9, 4);
        goto L_4548;
    }
    if (contrlerPressed((int)i, kPadSelect, 0) != 0) {
        gAction.step = 3;
        gAction.timer = 0;
        if (gAction.hudShown != 0) {
            setElements(kSkipStartKeep, 0);
        }
        setScreenCode(9, 3);
        goto L_4548;
    }
    if (contrlerPressed((int)i, kPadBack, 0) != 0) {
        const uint32_t view = comdataGetView(replayGetRider(player));

        gAction.savedView = (uint8_t)view;
        comdataSetView(replayGetRider(player), 0);
        gAction.step = 5;
        gAction.timer = 0;
        setScreenCode(9, 3);
        goto L_4548;
    }
    if (contrlerPressed((int)i, kPadView, 0) != 0) {
        uint32_t view = comdataGetView(replayGetRider(player));

        if (view > 7) {
            view = 0;
        } else {
            view = comdataGetView(replayGetRider(player)) + 1;
        }
        comdataSetView(replayGetRider(player), view);
    }
    if (contrlerPressed((int)i, kPadHoldB, 0) != 0) {
        gAction.hudShown ^= 1;
        setElements(kSkipPauseKeep, gAction.hudShown);
    }
L_4475:
    i++;
    if (i < gPlayerCount) {
        goto L_438d;
    }
L_4548:
    switch (gAction.step) {
    case 1:
        goto L_4563;
    case 4:
    case 6:
        goto L_475b;
    case 7:
        goto L_4602;
    default:
        goto L_4725;
    }

L_4563:
    i = 0;
    if (gPlayerCount == 0) {
        goto L_4602;
    }
    do {
        if ((gDemoPlayer == 0xff || gDemoPlayer == (uint8_t)i) &&
            contrlerPressed((int)i, kPadStart, 0) != 0) {
            replaySetMode(player, 4);
            replaySetMode(player, 2);
            if (cameraPlaybackActive() == 2) {
                comdataResetView(replayGetRider(player), 1);
            } else {
                refreshView(player);
            }
            actionStepPlayers();
            gAction.step = 4;
            gAction.timer = 0;
            break;
        }
        i++;
    } while (i < gPlayerCount);
L_4602:
    if (gAction.step == 1) {
        goto L_4699;
    }
    if (gAction.step != 7) {
        goto L_4725;
    }
    gAction.timer++;
    if (gAction.timer >= 0xf0) {
        gAction.step = 2;
        gAction.timer = 0;
        goto L_4725;
    }
    {
        const uint32_t rider = replayGetRider(player);

        if (gPlayerRecs[rider][0x4d] != 0) {
            const uint32_t rider2 = replayGetRider(player);

            gPlayerRecs[rider2][0x4d] = (uint8_t)(gPlayerRecs[rider2][0x4d] - 1);
        }
    }
    physUpdateRiders();
    propUpdate();
    goto L_4720;

L_4699:
    replayUpdate();
    value = replayGetState(player);
    if (value == 2) {
        riderUpdateInput();
        physUpdateRiders();
        propUpdate();
        gameTick();
        goto L_4717;
    }
    if (value != 4) {
        goto L_4717;
    }
    if (cameraPlaybackActive() != 3) {
        gAction.step = 6;
        gAction.timer = 0;
        goto L_4725;
    }
    replaySetMode(player, 2);
    refreshView(player);
    gAction.step = 4;
    gAction.timer = 0;
    goto L_4725;

L_4717:
    if (gAction.step != 1) {
        goto L_4725;
    }
    goto L_4720;

L_475b:
    gAction.timer++;
    if (gAction.timer != 0x1e) {
        goto L_4720;
    }
    value = gAction.step;
    gAction.step = 1;
    if (value != 4) {
        goto L_477b;
    }
    goto L_4782;

L_477b:
    gAction.step = 2;
L_4782:
    gAction.timer = 0;
    goto L_4720;

L_478a:
    gAction.timer++;
    if (gAction.timer == 1) {
        goto L_47c4;
    }
    if (gAction.timer != 0x1e) {
        goto L_4725;
    }
    if (cameraPlaybackActive() == 2) {
        ovlSetScreen(0x1a);
    } else {
        actionSetMode(0);
    }
    goto L_4720;

L_47c4:
    setFade(0);
    if (gameGetMode() != 0) {
        goto L_4725;
    }
    if (cameraPlaybackActive() == 3) {
        goto L_4725;
    }
    bmxSoundPlayAt(0x3e9, (const void *)(uintptr_t)4, 0, 0, 0);
    goto L_4725;

L_4820:
    i = 0;
    if (gPlayerCount == 0) {
        goto L_4988;
    }
L_4834:
    if (gDemoPlayer != 0xff && gDemoPlayer != (uint8_t)i) {
        goto L_4884;
    }
    if (contrlerPressed((int)i, kPadAny, 0) != 0) {
        if (gAction.step == 5) {
            comdataSetView(replayGetRider(player), gAction.savedView);
        }
        gAction.step = 2;
        gAction.timer = 0;
        setScreenCode(9, 4);
        goto L_4988;
    }
    if (gAction.step == 3 && contrlerPressed((int)i, kPadBack, 0) != 0) {
        saveView(player);
        gAction.step = 5;
        gAction.timer = 0;
        goto L_4988;
    }
    if (contrlerPressed((int)i, kPadMenu, 0) != 0) {
        if (gAction.step == 5) {
            comdataSetView(replayGetRider(player), gAction.savedView);
        }
        gAction.step = 1;
        gAction.timer = 0;
        if (gAction.hudShown != 0) {
            setElements(kSkipPauseKeep, 1);
        }
        setScreenCode(9, 2);
        goto L_4988;
    }
L_4884:
    i++;
    if (i < gPlayerCount) {
        goto L_4834;
    }
L_4988:
    if (gAction.step == 3) {
        gAction.timer++;
        if (gAction.timer == 1) {
            bmxSoundPlayAt(6000, NULL, 0, 0, 0);
        }
        goto L_4725;
    }
    if (gAction.step == 5) {
        goto L_4720;
    }
    goto L_4725;

L_49ea:
    gAction.timer++;
    if (gAction.timer != 1) {
        goto L_435f;
    }
L_49fe:
    setFade(1);
    goto L_4720;

L_4a2d:
    for (i = 0; i < 2; i++) {
        const uint8_t *const pad = contrlerGet(i);
        const uint32_t *const padWords = (const uint32_t *)(const void *)pad;

        if ((pad[0xc] & 1) == 0 && (padWords[4] & padWords[5]) != 0) {
            goto L_477b;
        }
    }
    if (gAction.step != 1) {
        goto L_4720;
    }
    replayUpdate();
    value = replayGetState(player);
    if (value == 2) {
        riderUpdateInput();
        physUpdateRiders();
        propUpdate();
        gAction.timer++;
        if (gAction.timer == 0x1e || gAction.timer == 0xd7a) {
            const uint8_t sound = (uint8_t)bmxSoundPlayAt(0x3ec, NULL, 0, 0, 0);

            setScreenCode(0xd, sound);
            goto L_4720;
        }
        if (gAction.timer != 0xf3c) {
            goto L_4720;
        }
    } else if (value != 4) {
        goto L_4720;
    }
    gAction.step = 3;
    gAction.timer = 0;
    goto L_4720;

L_4ae6:
    gAction.timer++;
    if (gAction.timer != 1) {
        if (gAction.timer != 0x1e) {
            goto L_4725;
        }
        zsystemSetMenuMode();
        return 0;
    }
    setFade(0);
    bmxSoundPlayAt(0x3e9, (const void *)(uintptr_t)4, 0, 0, 0);
    goto L_4725;

L_4720:
    actionStepPlayers();
L_4725:
    zrollerRunDrawCallbacks();
    if (gActionFrameReload == -1) {
        padFlags[2] |= 0x800;
        padFlags[3] |= 0x800;
    }
L_474c:
    zsystemEndFrame();
    return 0;
}

BMX_ADDR(0x00404ed0)
void actionShutdown(void)
{
    appLog("APP: Shutdown ACTION\n");
    propStopSounds();
    ovlShutdown();
    gameShutdown();
    bmxSoundPlayAt(1000, NULL, 0, 0, 0);
    replayDestroyAll();
    contrlerFreeDevices();
    fxShutdown();
    physNop();
    (void)actionShutdownRoller();
}

BMX_ADDR(0x00404f40)
uint32_t actionSetMode(uint32_t mode)
{
    uint32_t player = 0;
    uint32_t i;
    uint32_t screen;

    gDemoPlayer = 0xff;
    gAction.timer = 0;
    gAction.field10 = 0;
    switch (gActionMode) {
    case kActionRun:
        if (mode != 1) {
            setElements(kSkipOnlyBar, 0);
            gAction.hudShown = 0;
        }
        break;
    case kActionPaused:
        screen = ovlGetScreenId();
        if (screen >= 9 && (screen <= 0xa || screen == 0x19)) {
            player = (uint8_t)gameGetModeByte();
            replaySetMode(player, 0);
            gameResetRider(replayGetRider(player), 0);
            comdataResetView(gCurrentPlayer, 4);
            cameraSelectPlayer(gCurrentPlayer);
        }
        break;
    case kActionReplay:
        player = (uint8_t)gameGetModeByte();
        replaySetMode(player, 0);
        gameResetRider(replayGetRider(player), 0);
        if (gameGetMode() == 3) {
            gameResetRider(gCurrentPlayer, 1);
        }
        /* fall through */
    case kActionPlayback:
        if (gActionMode == kActionPlayback) {
            gameResetRider(replayGetRider(gAction.replay), 0);
            replayDestroy(gAction.replay);
        }
        setScreenCode(9, 0);
        ovlSetElementVisible(9, 0);
        ovlSetElementVisible(0xe, 0);
        ovlSetElementVisible(0xf, 0);
        setScreenCode(0xd, 0xff);
        ovlSetElementVisible(0xd, 0);
        ovlSetElementVisible(0, 0);
        comdataResetView(gCurrentPlayer, 4);
        cameraSelectPlayer(gCurrentPlayer);
        if (gameGetMode() == 0) {
            bmxSoundPlayAt(0x3e9, (const void *)(uintptr_t)4, 0, 0, 0);
        }
        break;
    case kActionDemo:
        replaySetMode(gAction.player, 0);
        setScreenCode(9, 0);
        ovlSetElementVisible(9, 0);
        ovlSetElementVisible(0xe, 0);
        ovlSetElementVisible(0xf, 0);
        comdataResetView(gCurrentPlayer, 0);
        gAction.field04 = 0;
        break;
    default:
        break;
    }
    switch (mode) {
    case 0:
        setElements(kSkipRunKeep, 1);
        gAction.hudShown = 1;
        contrlerSetMode(0);
        gAction.field04 = 0;
        break;
    case 1:
        screen = ovlGetScreenId();
        if (screen >= 9 && (screen <= 0xa || screen == 0x19)) {
            for (i = 0; i < gPlayerCount; i++) {
                gameResetRider(i, 0);
            }
            player = (uint8_t)gameGetModeByte();
            cameraSelectPlayer(replayGetRider(player));
            replaySetMode(player, 2);
            comdataResetView(replayGetRider(player), 1);
            comdataSetView(replayGetRider(player), 8);
        }
        contrlerSetMode(1);
        break;
    case 2:
    case 4:
        for (i = 0; i < gPlayerCount; i++) {
            gameResetRider(i, 0);
        }
        if (mode == 4) {
            ActionReplayDesc desc;
            uint8_t *data;
            int size;

            gCurrentPlayer = 0;
            desc.flags = 7;
            desc.pad01 = 0;
            desc.pad02 = 0;
            desc.size = 0x2800;
            gAction.replay = replayCreate((const ReplayParams *)&desc);
            if (gAction.replay == 0xff) {
                zdebugFail("Failed to create replay (probably out of RAM).", ACTION_FILE, 0x6bc, 0);
            }
            replayGetBuffer(gAction.replay, &data, &size);
            if (zassetFind(gActionReplayLib, (int)(gProfiles[0].character + gCurrentLevel * 2u), 0) == 0) {
                zdebugFail("replay data not found in asset library", ACTION_FILE, 0x6c2, 0);
            } else {
                if (zassetSize(gActionReplayLib) > (uint32_t)size) {
                    zdebugFail("replay data too big for demo replay buffer", ACTION_FILE, 0x6bf, 0);
                }
                zassetRead(gActionReplayLib, data);
            }
            player = gAction.replay;
        } else {
            player = (uint8_t)gameGetModeByte();
        }
        cameraSelectPlayer(replayGetRider(player));
        replaySetMode(player, 2);
        setScreenCode(9, 2);
        setScreenCode(10, 0xff);
        comdataResetView(replayGetRider(player), 1);
        if (mode == 4) {
            setElements(kSkipOnlyBar, 0);
            gAction.hudShown = 0;
            ovlSetElementVisible(0xd, 1);
            ovlSetElementVisible(0, 1);
            ovlSetElementVisible(0xe, 1);
            ovlSetElementVisible(0xf, 1);
        } else {
            ovlSetElementVisible(9, 1);
            ovlSetElementVisible(2, 0);
            ovlSetElementVisible(1, 0);
            ovlSetElementVisible(10, 0);
            ovlSetElementVisible(0xe, 0);
            ovlSetElementVisible(0xf, 0);
        }
        if (gameGetMode() == 0) {
            bmxSoundPlayAt(0x3e9, (const void *)(uintptr_t)2, 0, 0, 0);
        }
        break;
    case 3:
        player = gAction.player;
        replaySetMode(player, 2);
        setScreenCode(9, 2);
        ovlSetElementVisible(9, 1);
        ovlSetElementVisible(0xe, 0);
        ovlSetElementVisible(0xf, 0);
        comdataResetView(replayGetRider(player), 1);
        comdataSetView(replayGetRider(player), 1);
        break;
    default:
        break;
    }
    gAction.step = 0;
    gActionMode = (uint8_t)mode;
    return mode & 0xff;
}
