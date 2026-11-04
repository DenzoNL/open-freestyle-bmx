/* Scene objects (C:\NgEngine\Zge\zobj.c): a pool of 0x3c byte objects with a type, a position, a rotation and
 * an optional parent / child hierarchy. The type's behaviour is a table of hooks (ZObjOps). */
#ifndef ZGE_ZOBJ_H
#define ZGE_ZOBJ_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zlist.h"
#include "zge/zmath.h"

struct ZPropDef; /* the tree object list: prop definitions (ZbspTreeNode) */
struct PhysPropNode;
struct ZObj;

/* Type hooks of an object (tables in the exe's data); every hook is optional except where noted. */
typedef struct ZObjOps {
    int(BMX_CDECL *destroy)(struct ZObj *object);                        /* 0x00 */
    int(BMX_CDECL *update)(struct ZObj *object);                         /* 0x04 */
    int(BMX_CDECL *moved)(struct ZObj *object, const ZVec3 *position);   /* 0x08 */
    int(BMX_CDECL *rotated)(struct ZObj *object, const ZQuat *rotation); /* 0x0c */
    void *pad10;                                                         /* 0x10 */
    int(BMX_CDECL *setVisible)(struct ZObj *object, int visible);        /* 0x14 */
} ZObjOps;

/* Hierarchy record of an object (0x10 bytes, from the pool at 0x6722d8). */
typedef struct ZObjLinks {
    struct ZObj *parent; /* 0x00 */
    void *partId;      /* 0x04 skeleton node of the parent's model the object is attached to, NULL for none */
    struct ZObj *next; /* 0x08 next sibling */
    struct ZObj *firstChild; /* 0x0c */
} ZObjLinks;

/* Event queue of an object: pending events, and handlers that fire when an event with their id is dispatched.
 */
typedef struct ZObjEvent {
    uint32_t id; /* 0x00 */
} ZObjEvent;

typedef struct ZObjHandler {
    uint32_t kind;                                      /* 0x00 only kind 1 fires */
    uint32_t eventId;                                   /* 0x04 */
    void(BMX_CDECL *fn)(const struct ZObjEvent *event); /* 0x08 */
} ZObjHandler;

typedef struct ZObjEvents {
    ZList *pending;  /* 0x00 list of ZObjEvent */
    ZList *handlers; /* 0x04 list of ZObjHandler, may be NULL */
} ZObjEvents;

/* A scene object. The low three bits of `flags` are the type (0..7), 8 means active, 0x20 means attached. */
typedef struct ZObj {
    uint32_t flags;     /* 0x00 */
    ZVec3 pos;          /* 0x04 */
    uint8_t pad10[4];   /* 0x10 */
    ZQuat rot;          /* 0x14 */
    ZObjLinks *links;   /* 0x24 */
    const ZObjOps *ops; /* 0x28 */
    void *model;        /* 0x2c renderer handle of the type (model, light, camera) */
    void *data;         /* 0x30 by type: the skeleton (models), the effect host (effects), the transform */
    uint8_t pad34[4];   /* 0x34 */
    ZObjEvents *events; /* 0x38 */
} ZObj;

BMX_LAYOUT32_ASSERT(sizeof(ZObj) == 0x3c, "object layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZObj, links) == 0x24, "object layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZObj, ops) == 0x28, "object layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZObj, model) == 0x2c, "object layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZObj, events) == 0x38, "object layout");

enum {
    kZobjActive = 0x08,
    kZobjAttached = 0x20,
};

/* Callback for zobjForEachChild. */
typedef int(BMX_CDECL *ZObjChildFn)(struct ZObj *child, void *arg);

#define gZobjInitialized ORIG_VAR(0x006722d0, int, gZobjInitialized)

/* Allocates a zeroed-by-zobjInit object slot (0x3c bytes used) from the object free list; NULL on failure. */
void *zobjAlloc(void);
/* Clears an object, resets its rotation to the identity and sets its type (0-7). */
void *zobjInit(void *object, uint32_t type);
/* Calls the destroy hook of the object's type, detaches it from its hierarchy, then frees it. */
int zobjFree(void *object);

/* Shows or hides an object and notifies its owner. */
void zobjSetVisible(void *object, int visible);

/* Model handle of a scene object (+0x2c). */
void *zobjGetModel(const void *object);

/* Renderer hooks (function pointers in the exe): set / clear flag bits of a model, set its colour. */
typedef void(BMX_CDECL *ZobjFlagsFn)(void *model, int set, int clear);
typedef void(BMX_CDECL *ZobjColorFn)(void *model, const uint8_t *rgba);
#define gZobjSetFlags ORIG_VAR(0x00560ed8, ZobjFlagsFn, gZobjSetFlags)
#define gZobjSetColor ORIG_VAR(0x00560ef4, ZobjColorFn, gZobjSetColor)

/* Heads of the two static object lists (globals 0x6722b8: tree objects, 0x6722c4: props). */
struct ZPropDef **zobjTreeObjectList(void);
struct PhysPropNode **zobjPropList(void);
/* Position of a scene object (a vector inside its frame). */
const ZVec3 *zobjGetPosition(const void *object);
/* Moves / rotates an object and notifies its owner. */
void zobjSetPosition(void *object, const ZVec3 *pos);
void zobjSetRotation(void *object, const ZQuat *rot);
/* Sets the rotation of an object from Euler angles. */
void zobjSetAngles(void *object, const ZAngle3 *angles);

/* Detaches an object from its parent, or orphans its children when it has no parent. */
void zobjDetach(ZObj *object);
/*
 * Fires the handlers for every pending event of the object and drains the queue. By address (0x49c570) it
 * sits just before zrwModelCreate.c, in a source file without asserts; it is kept here with its callers.
 */
void zobjDispatchEvents(ZObj *object);
/* Calls fn(child, arg) for every child of the object. Always returns 1. */
int zobjForEachChild(ZObj *object, ZObjChildFn fn, void *arg);
/* Refreshes the world transform of an object and (recursively) its children. */
int zobjRefresh(ZObj *object, void *arg);
/* Update callback of the object system (registered at type 8): refreshes every active root object. */
int zobjUpdateAll(void *arg);
/* Debug callback: prints the number of objects in use. */
int zobjDebugPrint(void *arg);
/* Starts the object system; `count` must not be 0. Returns 0 when one of the type initialisers fails. */
int zobjInitialize(int count);

#endif
