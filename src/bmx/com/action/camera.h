/* Cameras (C:\zaxis\Bmx\src\com\action\camera.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_CAMERA_H
#define BMX_COM_ACTION_CAMERA_H

#include <stdint.h>

#include "compat/orig.h"

/* Makes `player` the active camera (asserts player < 2). */
ORIG_FN(0x00407530, void, cameraSelectPlayer, (uint32_t player));

#endif
