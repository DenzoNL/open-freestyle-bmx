/* Scene objects (C:\NgEngine\Zge\zobj.c). Not ported yet. */
#ifndef ZGE_ZOBJ_H
#define ZGE_ZOBJ_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

struct ZbspTreeNode;
struct PhysPropNode;

/* Shows or hides an object handle and notifies its owner. */
ORIG_FN(0x00487600, void, zobjSetVisible, (int32_t object, int visible));

/* Model handle of a scene object (+0x2c). */
ORIG_FN(0x00487440, void *, zobjGetModel, (const void *object));

/* Renderer hooks (function pointers in the exe): set / clear flag bits of a model, set its colour. */
typedef void(BMX_CDECL *ZobjFlagsFn)(void *model, int set, int clear);
typedef void(BMX_CDECL *ZobjColorFn)(void *model, const uint8_t *rgba);
#define gZobjSetFlags ORIG_VAR(0x00560ed8, ZobjFlagsFn, gZobjSetFlags)
#define gZobjSetColor ORIG_VAR(0x00560ef4, ZobjColorFn, gZobjSetColor)

/* Heads of the two static object lists (globals 0x6722b8: tree objects, 0x6722c4: props). */
ORIG_FN(0x00486ec0, struct ZbspTreeNode **, zobjTreeObjectList, (void));
ORIG_FN(0x00486ed0, struct PhysPropNode **, zobjPropList, (void));
/* Moves / rotates an object and notifies its owner. */
ORIG_FN(0x00487470, void, zobjSetPosition, (void *object, const ZVec3 *pos));
ORIG_FN(0x004874d0, void, zobjSetRotation, (void *object, const ZQuat *rot));

#endif
