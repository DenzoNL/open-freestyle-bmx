/* Scene objects (C:\NgEngine\Zge\zobj.c). */
#include "zge/zobj.h"

#include "compat/crt.h"
#include "zge/com/zglrw/zrwdbg.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zgemods/zobjlight.h"
#include "zge/zgemods/zobjmodel.h"
#include "zge/zgemods/zobjpemit.h"
#include "zge/zmem.h"
#include "zge/zquat.h"

static const char kZobjFile[] = "C:\\NgEngine\\Zge\\zobj.c";

#define gZobjPool       ORIG_VAR(0x006722d4, ZMemFreeList *, gZobjPool)
#define gZobjLinkPool   ORIG_VAR(0x006722d8, ZMemFreeList *, gZobjLinkPool)
#define gZobjDebugNode  ORIG_VAR(0x006722dc, ZrwUpdateNode *, gZobjDebugNode)
#define gZobjUpdateNode ORIG_VAR(0x006722e0, ZrwUpdateNode *, gZobjUpdateNode)

BMX_ADDR(0x00486ec0)
struct ZbspTreeNode **zobjTreeObjectList(void)
{
    return &ORIG_VAR(0x006722b8, struct ZbspTreeNode *, gZobjTreeObjects);
}

BMX_ADDR(0x00486ed0)
struct PhysPropNode **zobjPropList(void)
{
    return &ORIG_VAR(0x006722c4, struct PhysPropNode *, gZobjProps);
}

/*
 * World transform of an attached object: its parent's matrix (or, without a part id, the parent's own
 * position and rotation) applied to its local position and rotation. The results go to the type's moved and
 * rotated hooks, then the model's pending events run and the type's update hook is called.
 */
static void zobjApplyParent(ZObj *object)
{
    ZMatrix parent;
    ZMatrix local;
    ZMatrix combined;
    ZVec3 position;
    ZQuat rotation;
    const ZObjLinks *links = object->links;

    if (links->partId == 0) {
        zquatToMatrix(&parent, &links->parent->rot);
        parent.pos = links->parent->pos;
    } else {
        zrwmatNodeMatrix(links->parent, links->partId, &parent, 1);
    }
    zmatrixTransformPoint(&position, &parent, &object->pos);
    zmatrixZeroPos(&local);
    zquatToMatrix(&local, &object->rot);
    zmatrixMul(&combined, &parent, &local);
    zquatFromMatrix(&rotation, &combined);
    if (object->ops->moved != NULL) {
        object->ops->moved(object, &position);
    }
    if (object->ops->rotated != NULL) {
        object->ops->rotated(object, &rotation);
    }
    zobjDispatchEvents(object);
    if (object->ops->update != NULL) {
        object->ops->update(object);
    }
}

BMX_ADDR(0x004879c0)
int zobjForEachChild(ZObj *object, ZObjChildFn fn, int arg)
{
    if (object->links != NULL) {
        for (ZObj *child = object->links->firstChild; child != NULL; child = child->links->next) {
            fn(child, arg);
            if (child->links == NULL) {
                return 1;
            }
        }
    }
    return 1;
}

BMX_ADDR(0x00486ee0)
int zobjRefresh(ZObj *object, int arg)
{
    if ((object->flags & kZobjActive) != 0) {
        zobjApplyParent(object);
        if (object->links != NULL) {
            for (ZObj *child = object->links->firstChild; child != NULL; child = child->links->next) {
                zobjRefresh(child, arg);
                if (child->links == NULL) {
                    return 1;
                }
            }
        }
    }
    return 1;
}

BMX_ADDR(0x00487020)
int zobjUpdateAll(void *arg)
{
    for (ZMemFreeNode *node = zmemFreeListFirst(gZobjPool); node != NULL; node = zmemFreeNodeNext(node)) {
        ZObj *object = zmemFreeNodePayload(node);

        if ((object->flags & kZobjActive) == 0 || (object->flags & kZobjAttached) != 0) {
            continue;
        }
        zobjDispatchEvents(object);
        if (object->ops->update != NULL) {
            object->ops->update(object);
        }
        if (object->links == NULL) {
            continue;
        }
        for (ZObj *child = object->links->firstChild; child != NULL;) {
            if ((child->flags & kZobjActive) != 0) {
                zobjApplyParent(child);
                zobjForEachChild(child, zobjRefresh, (int)(intptr_t)arg);
            }
            if (child->links == NULL) {
                break;
            }
            child = child->links->next;
        }
    }
    return 1;
}

BMX_ADDR(0x004871e0)
int zobjDebugPrint(void *arg)
{
    (void)arg;
    ZDebugPrintBlya(0, -1, "ZObj: Used:%d", zmemFreeListUsedCount(gZobjPool));
    return 1;
}

BMX_ADDR(0x00487210)
int zobjInitialize(int count)
{
    if (gZobjInitialized != 0) {
        zdebugFail("Already initialized", kZobjFile, 0xae, 0);
    }
    if (count == 0) {
        zdebugFail("Can't initialize with 0", kZobjFile, 0xaf, 0);
    }
    gZobjInitialized = 1;
    gZobjPool = zmemFreeListCreate(sizeof(ZObj), 0x100, BMX_FNPTR(ZMemDestroyFn, 0x00487400, zobjFree));
    gZobjLinkPool = zmemFreeListCreate(0x10, 4, NULL);
    gZobjUpdateNode =
        zrwRegisterUpdateFunction(8, BMX_FNPTR(ZrwUpdateFn, 0x00487020, zobjUpdateAll), NULL, 0);
    if (gZobjUpdateNode == NULL) {
        return 0;
    }
    gZobjDebugNode = zrwDbgAddCallback(0, BMX_FNPTR(ZrwUpdateFn, 0x004871e0, zobjDebugPrint), NULL);
    if (zobjModelInitialize() == 0) {
        return 0;
    }
    zobjLightInitialize();
    if (zobjCameraInitialize() == 0) {
        return 0;
    }
    zobjPEmitInitialize();
    return 1;
}

BMX_ADDR(0x00487370)
void *zobjAlloc(void)
{
    void *object = zmemFreeListAlloc(gZobjPool);

    if (object == NULL) {
        gZdebugPrint("ZOBJ: Error - failed to allocate obj\n");
    }
    return object;
}

BMX_ADDR(0x004873a0)
void *zobjInit(void *object, uint32_t type)
{
    ZObj *obj = object;

    if (gZobjInitialized == 0) {
        zdebugFail("Not initialized", kZobjFile, 0x119, 0);
    }
    memset(obj, 0, sizeof *obj);
    zquatIdentity(&obj->rot);
    obj->flags = (((obj->flags ^ type) & 7) ^ obj->flags) | kZobjActive;
    return obj;
}

BMX_ADDR(0x00487400)
int zobjFree(void *object)
{
    ZObj *obj = object;

    if (obj->ops->destroy != NULL) {
        obj->ops->destroy(obj);
    }
    if (obj->links != NULL) {
        zobjDetach(obj);
    }
    zmemFreeListFree(gZobjPool, obj);
    return 1;
}

BMX_ADDR(0x00487440)
void *zobjGetModel(const void *object)
{
    return ((const ZObj *)object)->model;
}

BMX_ADDR(0x00487470)
void zobjSetPosition(void *object, const ZVec3 *pos)
{
    ZObj *obj = object;

    obj->pos = *pos;
    if (obj->ops->moved != NULL) {
        obj->ops->moved(obj, pos);
    }
}

BMX_ADDR(0x004874a0)
const ZVec3 *zobjGetPosition(const void *object)
{
    return &((const ZObj *)object)->pos;
}

BMX_ADDR(0x004874d0)
void zobjSetRotation(void *object, const ZQuat *rot)
{
    ZObj *obj = object;

    obj->rot = *rot;
    if (obj->ops->rotated != NULL) {
        obj->ops->rotated(obj, rot);
    }
}

BMX_ADDR(0x00487520)
void zobjSetAngles(void *object, const ZAngle3 *angles)
{
    ZObj *obj = object;
    ZQuat rot;

    zquatFromEuler(&rot, angles, 0);
    obj->rot = rot;
    if (obj->ops->rotated != NULL) {
        obj->ops->rotated(obj, &rot);
    }
}

BMX_ADDR(0x00487600)
void zobjSetVisible(int32_t object, int visible)
{
    ZObj *obj = (ZObj *)(uintptr_t)(uint32_t)object;

    if (visible == 0) {
        obj->flags &= ~(uint32_t)kZobjActive;
    } else {
        obj->flags |= kZobjActive;
    }
    if (obj->ops->setVisible != NULL) {
        obj->ops->setVisible(obj, visible);
    }
}

/*
 * Detaches the object from its parent. Original quirk: when the object is not the parent's first child, the
 * code clears the attached flag and the links of its previous sibling instead of its own, and notifies that
 * sibling. An object without a parent orphans its children.
 */
BMX_ADDR(0x00487830)
void zobjDetach(ZObj *object)
{
    ZObjLinks *links = object->links;

    if (links == NULL) {
        return;
    }
    if (links->parent != NULL) {
        ZObjLinks *parentLinks = links->parent->links;
        ZObj *sibling = parentLinks->firstChild;

        if (sibling == object) {
            parentLinks->firstChild = links->next;
            object->flags &= ~(uint32_t)kZobjAttached;
            object->links->parent = NULL;
            object->links->partId = 0;
            object->links->next = NULL;
            if (object->ops->moved != NULL) {
                object->ops->moved(object, &object->pos);
            }
            if (object->ops->rotated != NULL) {
                object->ops->rotated(object, &object->rot);
            }
            return;
        }
        while (sibling != NULL) {
            if (sibling->links->next == object) {
                sibling->links->next = links->next;
                sibling->flags &= ~(uint32_t)kZobjAttached;
                sibling->links->parent = NULL;
                sibling->links->partId = 0;
                sibling->links->next = NULL;
                if (sibling->ops->moved != NULL) {
                    sibling->ops->moved(sibling, &object->pos);
                }
                if (sibling->ops->rotated != NULL) {
                    sibling->ops->rotated(sibling, &object->rot);
                }
                return;
            }
            sibling = sibling->links->next;
        }
        zdebugFail("Error in hierarchy", kZobjFile, 0x345, 0);
    }
    if (object->links != NULL) {
        ZObj *child = object->links->firstChild;

        if (child != NULL) {
            do {
                ZObjLinks *childLinks = child->links;
                ZObj *next = childLinks != NULL ? childLinks->next : NULL;

                if (childLinks == NULL) {
                    break; /* the original crashes here */
                }
                childLinks->parent = NULL;
                child->links->partId = 0;
                child->links->next = NULL;
                child = next;
            } while (child != NULL);
            object->links->firstChild = NULL;
        }
    }
}

/* Each event leaves the queue after its handlers ran. */
BMX_ADDR(0x0049c570)
void zobjDispatchEvents(ZObj *object)
{
    if (object->events == NULL || object->events->pending == NULL) {
        return;
    }
    ZObjEvent *event = zlistFirst(object->events->pending);

    while (event != NULL) {
        if (object->events->handlers != NULL) {
            ZObjHandler *handler = zlistFirst(object->events->handlers);

            while (handler != NULL) {
                if (handler->eventId == event->id && handler->kind == 1) {
                    handler->fn(event);
                }
                handler = zlistNext(object->events->handlers);
            }
        }
        zlistRemoveCurrent(object->events->pending);
        event = zlistCurrent(object->events->pending);
    }
}
