/* Common game data (C:\zaxis\Bmx\Src\com\action\comdata.c). */
#include "bmx/com/action/comdata.h"

#include "bmx/com/action/action.h"
#include "bmx/com/action/camera.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/menu/frontend.h"
#include "bmx/com/zge/ngif/missingzge.h"
#include "bmx/com/zge/zactor.h"
#include "zge/zdebug.h"

static const char comdataFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\comdata.c";

/* Option word at 0x6955e2 (the byte gActionOptions is its low half). */
#define gComdataOptions ORIG_VAR(0x006955e2, uint16_t, gComdataOptions)

/*
 * Sets the view mode of a player's camera. Entering mode 3 hides the rider and bike actors (and the
 * horizon), leaving it shows them again; only for camera type 0.
 */
BMX_ADDR(0x00407620)
void comdataSetViewMode(int player, uint32_t mode)
{
    CameraState *const cam = &gCameras[player];
    PhysRider *const rider = &gPhysRiders[player];

    if (cam->mode == mode) {
        return;
    }
    cam->changed = 0;
    if (cam->mode == 3 && cam->type == 0) {
        cam->changed = 1;
        cam->flags &= (uint8_t)~kCameraFlagsHidden;
        if ((rider->flags & 1) != 0) {
            zactorShow(rider->actor);
            zactorShow(rider->bikeObj);
        }
        zglEnableHorizon();
    }
    if (mode == 3 && cam->type == 0) {
        cam->changed = 1;
        cam->flags |= (uint8_t)mode;
        zactorHide(rider->actor);
        zactorHide(rider->bikeObj);
        zglDisableHorizon();
    }
    cam->mode = (uint8_t)mode;
    if (cam->type == 0) {
        cam->timer = 0;
    }
}

BMX_ADDR(0x00407730)
uint32_t comdataGetViewMode(int player)
{
    return gCameras[player].mode;
}

/* Resets the camera of a player to view type `mode` (0, 1, 3 or 4) and updates the actors. */
BMX_ADDR(0x00407760)
void comdataResetView(int player, int mode)
{
    CameraState *const cam = &gCameras[player];
    const uint8_t flags = cam->flags;

    cam->timer = 0;
    cam->changed = 0;
    cam->flags = flags & 0xfc;
    gCameraReset = 0;
    switch (cam->type) {
    case 0:
        if (cam->mode == 3) {
            cam->changed = 1;
            zglEnableHorizon();
        }
        break;
    case 1:
    case 3:
    case 4:
        cam->changed = 1;
        break;
    default:
        break;
    }
    switch ((uint32_t)mode) {
    case 0:
        if (cam->mode == 3) {
            cam->changed = 1;
            zglDisableHorizon();
        }
        break;
    case 1:
        cam->changed = 1;
        comdataSetView((uint32_t)player, 6);
        cam->flags = (cam->flags & 0xf7) | 4;
        cam->type = (uint8_t)mode;
        return;
    case 3:
    case 4:
        cam->changed = 1;
        cam->flags |= 3;
        break;
    default:
        break;
    }
    cam->type = (uint8_t)mode;
}

BMX_ADDR(0x00407890)
uint32_t comdataGetViewType(int player)
{
    return gCameras[player].type;
}

BMX_ADDR(0x00407600)
uint32_t comdataGetViewPlayer(void)
{
    return gCameraPlayer;
}

BMX_ADDR(0x00407b70)
uint32_t comdataGetView(uint32_t player)
{
    return gCameras[player].view;
}

/* Offsets inside a rider's unlock data (GameRiderData) that the reset touches. */
enum {
    kRiderLevelsA = 4,
    kRiderGoalInfo = 7, /* six GameGoalInfo of 0x27 bytes */
    kRiderLevelDone = 0xeb,
    kRiderTrickBits = 0xf7,
    kGoalInfoSize = 0x27,
};

/*
 * Resets riders, profiles and options to their defaults. Quirks kept: of each level's goal info only the
 * first three bytes are cleared, of the 12 levelDone bytes only the last six, and the bike of a profile is
 * left alone.
 */
BMX_ADDR(0x00407ba0)
void comdataInit(void)
{
    for (uint32_t i = 0; i < 12; i++) {
        uint8_t *const rider = (uint8_t *)&gGameRiders[i];

        *(uint32_t *)rider = 0;
        rider[kRiderLevelsA] = 1;
        rider[kRiderLevelsA + 1] = 1;
        rider[kRiderLevelsA + 2] = 1;
        zmemFill(rider + kRiderTrickBits, 0, 11);
        for (uint32_t level = 0; level < 6; level++) {
            uint8_t *const info = rider + kRiderGoalInfo + level * kGoalInfoSize;
            info[0] = 0;
            info[1] = 0;
            info[2] = 0;
        }
        *(uint32_t *)(rider + kRiderLevelDone + 6) = 0;
        *(uint16_t *)(rider + kRiderLevelDone + 10) = 0;
    }
    gSoundVolumes[3] = 10;
    gGameUnlockBits = 0;
    gComdataOptions = 0;
    gSoundVolumes[0] = 0x12;
    gSoundVolumes[1] = 0x12;
    gSoundVolumes[2] = 1;
    gActionLanguage = 0;
    for (uint8_t p = 0; p < 2; p++) {
        OvlProfile *const profile = &gProfiles[p];
        PlayerSetup *const setup = &gPlayers[p];

        profile->flags = 3;
        profile->controller = p;
        profile->pad02 = 0;
        profile->character = 0;
        profile->pad04[0] = 0;
        profile->option = 0;
        zstrFormat(profile->name, "player");
        setup->bind[4] = 0x80;
        setup->bind[5] = 0x20;
        setup->bind[6] = 0x40;
        setup->bind[7] = 0x10;
        setup->bind[8] = 0x100;
        setup->bind[9] = 0x200;
        setup->bind[10] = 0x4000;
        setup->layout = 0;
    }
    gCurrentPlayer = 0;
    gPlayerCount = 1;
}

/*
 * Record of a level table by key: bits 24..31 table, 22..23 and 16..21 sub index (indices above 1 in
 * bits 16..21 go to the shared table at record 14), low 12 bits the index inside table 0. The level is
 * the current one; the records of all tables share one array.
 */
BMX_ADDR(0x00407d20)
uint32_t *comdataGetLevelRecord(uint32_t key)
{
    const uint32_t sub = key >> 16 & 0x3f;
    const uint32_t set = key >> 22 & 3;
    uint32_t *const base = gComdataRecords[0] + (uint32_t)gCurrentLevel * 24 * 5;
    uint32_t index;

    if (sub > 1) {
        return base + (14 + sub) * 5;
    }
    switch (key >> 24) {
    case 0:
        index = key & 0xfff;
        break;
    case 1:
        index = 3 + set;
        break;
    case 2:
        if (sub != 1) {
            return NULL;
        }
        index = 7;
        break;
    case 3:
        if (sub != 1) {
            return NULL;
        }
        index = 8;
        break;
    case 4:
        if (sub == 0) {
            index = 10;
        } else if (sub == 1) {
            index = 9;
        } else {
            return NULL;
        }
        break;
    case 5:
        if (sub != 1) {
            return NULL;
        }
        index = 11;
        break;
    case 6:
        index = 20;
        break;
    case 7:
        index = 19;
        break;
    case 8:
        index = 18;
        break;
    case 9:
        index = 21;
        break;
    case 10:
        index = 22;
        break;
    case 11:
        index = 12 + set;
        break;
    default:
        return NULL;
    }
    return base + index * 5;
}

BMX_ADDR(0x00407fc0)
uint8_t *comdataGetGoalInfo(int player, uint32_t level)
{
    if (level > 5) {
        zdebugFail("invalid level for goal info", comdataFile, 0x34a, 0);
    }
    return (uint8_t *)&gGameRiders[0] + (uint32_t)gProfiles[player].character * sizeof(GameRiderData) +
           kRiderGoalInfo + level * kGoalInfoSize;
}
