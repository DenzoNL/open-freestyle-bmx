/*
 * Cameras (C:\zaxis\Bmx\src\com\action\camera.c): the per player camera state (0xb8 bytes each at
 * 0x56b448, kept next to the view code of comdata.c) and the selection of the active camera.
 */
#ifndef BMX_COM_ACTION_CAMERA_H
#define BMX_COM_ACTION_CAMERA_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

/* Camera flags (CameraState.flags). */
enum {
    kCameraFlagsHidden = 0x03, /* set while view mode 3 hides the actors */
    kCameraFlagsSplit = 0x04,  /* set with type 1 */
    kCameraFlagsActive = 0x10, /* moves to the player that becomes the selected one */
    kCameraFlagsFrozen = 0x20, /* moves along with it */
};

/* State of one player's camera. */
typedef struct CameraState {
    uint8_t type; /* 0x00 0xff = unset, 0 normal, 1, 3 and 4 */
    uint8_t pad01;
    uint8_t mode;   /* 0x02 view mode (3 = actors hidden) */
    uint8_t view;   /* 0x03 chosen view (set by comdataSetView) */
    uint32_t timer; /* 0x04 cleared whenever the view changes */
    uint8_t pad08[2];
    uint8_t changed; /* 0x0a set when the view changed */
    uint8_t flags;   /* 0x0b kCameraFlags* */
    uint8_t pad0c[0x14];
    ZVec3 target; /* 0x20 respawn face centre the view looks at (comdataSetView) */
    uint8_t pad2c[0x8c];
} CameraState;

_Static_assert(sizeof(CameraState) == 0xb8, "CameraState size");

typedef CameraState CameraStates[2];
#define gCameras      ORIG_VAR(0x0056b448, CameraStates, gCameras)
#define gCameraPlayer ORIG_VAR(0x0056b408, uint8_t, gCameraPlayer) /* player whose camera is selected */
#define gCameraReset  ORIG_VAR(0x0056b5b8, uint32_t, gCameraReset) /* cleared by every view reset */

/* Size of the bar the letterbox callback fills (width, height). */
typedef uint32_t CameraRect[2];
#define gCameraRect ORIG_VAR(0x0056b410, CameraRect, gCameraRect)

/* Where a camera looks: an object, its field of view, and an eye / target pair (also the sound listener). */
typedef struct CameraView {
    void *camera; /* 0x00 ZObjCamera */
    uint16_t fov; /* 0x04 */
    uint8_t pad06[2];
    ZVec3 eye; /* 0x08 */
    uint8_t pad14[4];
    ZVec3 target; /* 0x18 */
} CameraView;

_Static_assert(sizeof(CameraView) == 0x24, "CameraView size");

/* Non-zero while a replay or other playback runs (timers and goal markers stand still). */
uint32_t cameraPlaybackActive(void);
void cameraInit(void);
void cameraNop(void);
void cameraApply(const CameraView *view);
int cameraDrawLetterbox(void);
/* Makes `player` the active camera (asserts player < 2). */
void cameraSelectPlayer(uint32_t player);

#endif
