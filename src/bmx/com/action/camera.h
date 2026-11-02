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
    kCameraFlagsNoKeepAway = 0x01, /* the eye may come closer to the rider than the minimum distance */
    kCameraFlagsNoFaces = 0x02,    /* the eye is not pushed off nearby faces */
    kCameraFlagsHidden = 0x03,     /* set while view mode 3 hides the actors */
    kCameraFlagsSplit = 0x04,      /* set with type 1: the view was chosen (comdataSetView not repeated) */
    kCameraFlagsViewDone = 0x08,   /* the type 1 view is over: the next update picks the following one */
    kCameraFlagsActive = 0x10,     /* moves to the player that becomes the selected one; cross fade (16) */
    kCameraFlagsFrozen = 0x20,     /* moves along with it; cross fade (8) */
};

/*
 * State of one player's camera. The eye follows `target` through a smoothed copy and a damped spring;
 * the look-at point follows `lookGoal`; the field of view eases towards `fovGoal`.
 */
typedef struct CameraState {
    uint8_t type;     /* 0x00 0xff = unset, 0 chase, 1 scripted views, 3 orbit, 4 random orbit (2: frozen) */
    uint8_t phase;    /* 0x01 stage of a scripted view (type 1) */
    uint8_t mode;     /* 0x02 view mode of the chase camera (3 = in the rider's head, actors hidden) */
    uint8_t view;     /* 0x03 chosen view (set by comdataSetView) */
    uint32_t timer;   /* 0x04 cleared whenever the view changes, counts the frames of the selected camera */
    ZAngle orbit;     /* 0x08 yaw of the orbiting views */
    uint8_t changed;  /* 0x0a set when the view changed: the next update jumps straight to the goals */
    uint8_t flags;    /* 0x0b kCameraFlags* */
    uint16_t blocked; /* 0x0c frames the line from the eye to the rider has been blocked */
    uint8_t pad0e[2];
    ZVec3 eye; /* 0x10 */
    uint8_t pad1c[4];
    ZVec3 target; /* 0x20 where the eye wants to be (comdataSetView: respawn face centre) */
    uint8_t pad2c[4];
    ZVec3 targetSmooth; /* 0x30 target eased by the smoothing parameter */
    uint8_t pad3c[4];
    ZVec3 lookAt; /* 0x40 */
    uint8_t pad4c[4];
    ZVec3 lookGoal; /* 0x50 */
    uint8_t pad5c[4];
    ZVec3 eyeVel; /* 0x60 last move of the eye (the spring's velocity) */
    uint8_t pad6c[4];
    float fov;      /* 0x70 field of view in degrees */
    float fovGoal;  /* 0x74 */
    ZMatrix orient; /* 0x78 view orientation (rows: forward, up, side) */
} CameraState;

_Static_assert(sizeof(CameraState) == 0xb8, "CameraState size");

typedef CameraState CameraStates[2];
#define gCameras      ORIG_VAR(0x0056b448, CameraStates, gCameras)
#define gCameraPlayer ORIG_VAR(0x0056b408, uint8_t, gCameraPlayer) /* player whose camera is selected */
#define gCameraReset  ORIG_VAR(0x0056b5b8, uint32_t, gCameraReset) /* cleared by every view reset */

/*
 * Camera tuning (.data at 0x551c54): eye offsets of the type 1 views 2, 4 and 6 in the rider's frame, the six
 * parameters of each camera (spring gain, damping, target smoothing, look smoothing, fov easing, fov) per
 * type 1 view, for the chase camera in the air, per other type and per chase mode, then the chase
 * distances.
 */
typedef struct CameraOffset {
    ZVec3 v;
    float pad;
} CameraOffset;

typedef struct CameraTuning {
    CameraOffset viewOffset[11]; /* 0x551c54 */
    float view[11][6];           /* 0x551d04 */
    float air[6];                /* 0x551e0c */
    float type[3][6];            /* 0x551e24 types 2, 3 and 4 */
    float mode[4][6];            /* 0x551e6c */
    float lookAhead;             /* 0x551ecc velocity scale of the look-at point */
    float keepAway;              /* 0x551ed0 minimum horizontal distance of the eye */
    float faceRadius;            /* 0x551ed4 faces the eye keeps clear of */
    float distance;              /* 0x551ed8 chase distance */
    float height;                /* 0x551edc chase height */
    float heightLow;             /* 0x551ee0 */
} CameraTuning;

_Static_assert(sizeof(CameraTuning) == 0x551ee4 - 0x551c54, "CameraTuning size");
#define gCameraTuning ORIG_VAR(0x00551c54, CameraTuning, gCameraTuning)

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

BMX_LAYOUT32_ASSERT(sizeof(CameraView) == 0x24, "CameraView size");

/* Non-zero while a replay or other playback runs (timers and goal markers stand still). */
uint32_t cameraPlaybackActive(void);
void cameraInit(void);
void cameraNop(void);
void cameraApply(const CameraView *view);
int cameraDrawLetterbox(void);
/* Makes `player` the active camera (asserts player < 2). */
void cameraSelectPlayer(uint32_t player);
/* Per frame update of the selected player's camera: goals by camera type and view, eye spring, face and
 * rider clearance, line of sight, field of view, orientation; then applies the view. */
void cameraUpdate(void);

#endif
