/* Cameras (C:\zaxis\Bmx\src\com\action\camera.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_CAMERA_H
#define BMX_COM_ACTION_CAMERA_H

#include <stdint.h>

#include "compat/orig.h"

/* Makes `player` the active camera (asserts player < 2). */
ORIG_FN(0x00407530, void, cameraSelectPlayer, (uint32_t player));

/* Non-zero while a replay or other playback runs (timers and goal markers stand still). */
ORIG_FN(0x004056a0, uint32_t, cameraPlaybackActive, (void));

#endif
