/* Scene objects (C:\NgEngine\Zge\zobj.c). Not ported yet. */
#ifndef ZGE_ZOBJ_H
#define ZGE_ZOBJ_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

struct ZbspTreeNode;
struct PhysPropNode;

/* Allocates a zeroed-by-zobjInit object slot (0x3c bytes used) from the object free list; NULL on failure. */
ORIG_FN(0x00487370, void *, zobjAlloc, (void));
/* Clears an object, resets its rotation to the identity and sets its type (0-7). */
ORIG_FN(0x004873a0, void *, zobjInit, (void *object, uint32_t type));
/* Calls the destroy hook of the object's type, then frees it. */
ORIG_FN(0x00487400, int, zobjFree, (void *object));

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
