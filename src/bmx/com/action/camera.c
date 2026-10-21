/* Cameras (C:\zaxis\Bmx\Src\com\action\camera.c). */
#include "bmx/com/action/camera.h"

#include "bmx/com/action/action.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"

static const char cameraFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\camera.c";

/* Mode of the action loop (riding, pause, replay, demo); non-zero while something plays back. */
BMX_ADDR(0x004056a0)
uint32_t cameraPlaybackActive(void)
{
    return gActionMode;
}

/* Resets the cameras of the active players, selects the first and registers the letterbox callback. */
BMX_ADDR(0x004056c0)
void cameraInit(void)
{
    for (uint32_t i = 0; (gPlayerCount < 1 ? 1u : gPlayerCount) > i; i++) {
        gCameras[i].mode = 0xff;
        comdataSetViewMode((int)i, gProfiles[i].option);
        gCameras[i].type = 0xff;
        comdataResetView((int)i, 0);
        gCameras[i].view = 0;
        gCameras[i].changed = 1;
        gCameras[i].flags = 0;
    }
    gCameraPlayer = 0;
    zrollerAddDrawCallback(BMX_FNPTR(ZrollerDrawFn, 0x004074c0, cameraDrawLetterbox));
}

BMX_ADDR(0x00405760)
void cameraNop(void) {}

/* Applies a view: field of view, eye and target; the eye is also where the sound is heard from. */
BMX_ADDR(0x00407470)
void cameraApply(const CameraView *view)
{
    zobjCameraSetFov(view->camera, view->fov);
    zobjCameraLookAt(view->camera, &view->eye, &view->target, 0);
    bmxSoundPlayAt(0xfa1, &view->eye, 0, 0, 0);
}

/* Draw callback: black bar over the split screen when two players play and option bit 1 is set. */
BMX_ADDR(0x004074c0)
int cameraDrawLetterbox(void)
{
    if ((gActionOptions & 2) != 0 && gPlayerCount != 0) {
        gCameraRect[0] = 0x280;
        gCameraRect[1] = 0x1e0;
        ngifFillScreenBar(0x70007000, 0, 0, 0x200, gCameraRect);
    }
    return 0;
}

/* Hands the selection (and the active / frozen flags) from the current camera to `player`. */
BMX_ADDR(0x00407530)
void cameraSelectPlayer(uint32_t player)
{
    if (player > 1) {
        zdebugFail("player number out of range", cameraFile, 0x49f, 0);
    }
    if (player != gCameraPlayer) {
        CameraState *const from = &gCameras[gCameraPlayer];
        CameraState *const to = &gCameras[player];

        if ((from->flags & kCameraFlagsActive) != 0) {
            to->flags |= kCameraFlagsActive;
            from->flags &= (uint8_t)~kCameraFlagsActive;
            gCameraPlayer = (uint8_t)player;
            return;
        }
        if ((from->flags & kCameraFlagsFrozen) != 0) {
            to->flags |= kCameraFlagsFrozen;
            from->flags &= (uint8_t)~kCameraFlagsFrozen;
        }
        gCameraPlayer = (uint8_t)player;
    }
}
