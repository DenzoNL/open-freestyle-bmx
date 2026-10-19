/* 2D primitive/sprite/font layer (C:\zaxis\Bmx\src\com\zge\zprim.c). Not ported yet; callees only. */
#ifndef BMX_COM_ZGE_ZPRIM_H
#define BMX_COM_ZGE_ZPRIM_H

#include <stdint.h>

#include "compat/orig.h"

/* zprimSetState(state, value) takes a numeric render-state id; the GUI code passes literals. */
ORIG_FN(0x004779e0, int, zprimInit, (void));
ORIG_FN(0x00477b50, void, zprimShutdown, (void));
ORIG_FN(0x00477ba0, void, zprimSetAssetFile, (const char *name));
ORIG_FN(0x00477be0, void, zprimBeginFrame, (void));
ORIG_FN(0x00477d00, void, zprimEndFrame, (void));
ORIG_FN(0x00477d20, void, zprimClear, (void));
ORIG_FN(0x00478050, int, zprimSetState, (int state, int value));
ORIG_FN(0x00478370, int, zprimGetState, (int state));
ORIG_FN(0x00478a50, void, zprimSetFont, (void *font));
ORIG_FN(0x00478b60, void, zprimSetSprite, (void *sprite));
ORIG_FN(0x00478bf0, void, zprimSetScale, (float sx, float sy));
ORIG_FN(0x00478c80, void, zprimResetScale, (void));
ORIG_FN(0x00478d80, void, zprimSetColor, (uint8_t r, uint8_t g, uint8_t b, uint8_t state));
ORIG_FN(0x00478df0, void, zprimLine, (int16_t x0, int16_t y0, int16_t x1, int16_t y1));
ORIG_FN(0x00479260, void, zprimDrawBackdrop, (void *sprite, void *backdrop, int a, int b));
ORIG_FN(0x00479cf0, void, zprimDrawSprite, (int frame, int x, int y));
ORIG_FN(0x00479fa0, int16_t, zspriteWidth, (void *sprite, uint16_t frame));
ORIG_FN(0x0047a070, int16_t, zspriteHeight, (void *sprite, uint16_t frame));
ORIG_FN(0x0047a620, void, zfontDrawFixed, (const char *text, int x, int y));
ORIG_FN(0x0047aa80, void, zfontDraw, (const char *text, int x, int y));
ORIG_FN(0x0047af10, uint16_t, zfontHeight, (void *font));
ORIG_FN(0x0047afd0, uint16_t, zfontTextWidth, (void *font, const char *text));
ORIG_FN(0x0047b220, int, zassetLoadScreen, (uint16_t id, int arg));
ORIG_FN(0x0047b400, int, zassetLoadGroup, (uint32_t slot, uint32_t id));
ORIG_FN(0x0047b5c0, void, zassetUnloadGroup, (int slot));
ORIG_FN(0x0047b6b0, void, zassetUnloadScreen, (int handle));
ORIG_FN(0x0047b900, uint8_t *, zassetLock, (int handle));
ORIG_FN(0x0047b930, void, zassetRelease, (int handle));
ORIG_FN(0x0047b950, void *, zassetFindSprite, (uint32_t id));
ORIG_FN(0x0047c030, int, zprimBuildSprites,
        (uint16_t a, uint16_t b, uint16_t c, const void *offsets, const void *data));

#endif
