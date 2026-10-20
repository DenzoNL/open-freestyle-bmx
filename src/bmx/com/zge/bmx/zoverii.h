/* Screen overlays and limb targets (C:\zaxis\Bmx\src\com\zge\bmx\zoverii.c). Not ported yet; callees only. */
#ifndef BMX_COM_ZGE_BMX_ZOVERII_H
#define BMX_COM_ZGE_BMX_ZOVERII_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

/* Shows (hide == 0) or hides an overlay by handle; 0xffff is ignored. Only the low 16 bits are read. */
ORIG_FN(0x00458400, int, zoverShow, (uint32_t handle, int hide));
/* Removes an overlay; 0xffff is ignored. */
ORIG_FN(0x004581a0, void, zoverRemove, (uint32_t handle));
/* Adds a text overlay and returns its handle (0xffff on failure). */
ORIG_FN(0x004594c0, uint32_t, zoverAddText,
        (int layer, const void *font, const char *text, int x, int y, uint32_t color, int flags));
/* Width of a text in pixels. */
ORIG_FN(0x00459f30, int16_t, zoverTextWidth, (const char *text, const void *font));
/* Stores a limb target pair in the limb block for every bit of `mask` (1, 2, 4, 8). */
ORIG_FN(0x00457790, void, zoverSetLimbTargets, (void *limbs, const ZVec3 *a, const ZVec3 *b, uint32_t mask));
ORIG_FN(0x00455ec0, void, zoverResetLimbs, (void *limbs));
/* Empty in the shipped build. */
ORIG_FN(0x00455ef0, void, zoverLimbsBegin, (void *actor, void *limbs));
ORIG_FN(0x00456120, void, zoverSolveLimbs, (void *actor, void *limbs));
/* Scripted-motion helpers for the rider and bike objects. */
ORIG_FN(0x004547c0, void, zoverStepRiderMotion, (void *motionObj, void *actor));
ORIG_FN(0x00452a40, void, zoverStepBikeMotion, (void *motionObj, void *bike));
ORIG_FN(0x004527c0, void, zoverStartBikeMotion,
        (void *motionObj, void *bike, const ZMatrix *m, const ZVec3 *vel, const ZAngle3 *rot));
ORIG_FN(0x004534a0, void, zoverStartRiderMotion,
        (void *motionObj, void *actor, const ZMatrix *m, const ZVec3 *vel, const ZAngle3 *rot));

#endif
