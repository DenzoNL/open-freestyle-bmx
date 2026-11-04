/*
 * Tests for zobj.c: the object accessors and setters (with type hooks that log what they see), the hierarchy
 * (detach, children, refresh, the per frame update) and the start-up of the object system.
 */
#include "dt.h"
#include "dt_s5.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zmem.h"
#include "zge/zobj.h"

#define gZobjPool     ORIG_VAR(0x006722d4, ZMemFreeList *, gZobjPool)
#define gZobjLinkPool ORIG_VAR(0x006722d8, ZMemFreeList *, gZobjLinkPool)

/* ---- type hooks ------------------------------------------------------------------------------------ */

static int BMX_CDECL opDestroy(ZObj *o)
{
    s5Log(0x2001);
    s5LogPtr(o);
    return 1;
}

static int BMX_CDECL opUpdate(ZObj *o)
{
    s5Log(0x2002);
    s5LogPtr(o);
    return 1;
}

static int BMX_CDECL opMoved(ZObj *o, const ZVec3 *p)
{
    s5Log(0x2003);
    s5LogPtr(o);
    s5LogWords(p, 3);
    return 1;
}

static int BMX_CDECL opRotated(ZObj *o, const ZQuat *q)
{
    s5Log(0x2004);
    s5LogPtr(o);
    s5LogWords(q, 4);
    return 1;
}

static int BMX_CDECL opVisible(ZObj *o, int v)
{
    s5Log(0x2005);
    s5LogPtr(o);
    s5Log((uint32_t)v);
    return 1;
}

static const ZObjOps gOpsAll = {opDestroy, opUpdate, opMoved, opRotated, NULL, opVisible};
static const ZObjOps gOpsNone = {NULL, NULL, NULL, NULL, NULL, NULL};
static const ZObjOps gOpsMoved = {NULL, NULL, opMoved, NULL, NULL, NULL};
static const ZObjOps gOpsRest = {opDestroy, opUpdate, NULL, opRotated, NULL, opVisible};

static const ZObjOps *pickOps(void)
{
    switch (dt_u32() % 4) {
    case 0:
        return &gOpsAll;
    case 1:
        return &gOpsNone;
    case 2:
        return &gOpsMoved;
    default:
        return &gOpsRest;
    }
}

static void fillObject(ZObj *o)
{
    dt_fill(o, sizeof *o);
    o->flags = dt_u32();
    s5RandVec(&o->pos);
    s5RandQuat(&o->rot);
    o->links = NULL;
    o->ops = pickOps();
    o->model = (void *)(uintptr_t)(0x20000000u + (dt_u32() & 0xffffffu));
    o->events = NULL;
}

static ZObj *newObject(void)
{
    ZObj *o = dt_alloc(sizeof *o);
    fillObject(o);
    return o;
}

static ZObjLinks *newLinks(ZObj *parent, uint32_t part, ZObj *next, ZObj *firstChild)
{
    ZObjLinks *l = dt_alloc(sizeof *l);
    l->parent = parent;
    l->partId = (void *)(uintptr_t)part;
    l->next = next;
    l->firstChild = firstChild;
    return l;
}

/* The pool the objects are allocated from. */
static void makePool(void)
{
    gZobjPool = S5_POOL_CREATE(0x3c, 8, NULL);
    gZobjLinkPool = S5_POOL_CREATE(0x10, 4, NULL);
}

static ZObj *poolObject(void)
{
    ZObj *o = S5_POOL_ALLOC(gZobjPool);
    fillObject(o);
    return o;
}

/* ---- accessors and setters ------------------------------------------------------------------------- */

static void setup_accessor(DtCtx *c)
{
    s5Begin(c, 0);
    DT_ARG_PTR(c, 0, newObject());
}

static void setup_visible(DtCtx *c)
{
    s5Begin(c, 0);
    DT_ARG_PTR(c, 0, newObject());
    DT_ARG_INT(c, 1, dt_u32() % 3 == 0 ? 0 : (int32_t)dt_u32());
}

static void setup_position(DtCtx *c)
{
    ZVec3 *p = dt_alloc(sizeof *p);

    s5Begin(c, 0);
    s5RandVec(p);
    DT_ARG_PTR(c, 0, newObject());
    DT_ARG_PTR(c, 1, p);
}

static void setup_rotation(DtCtx *c)
{
    ZQuat *q = dt_alloc(sizeof *q);

    s5Begin(c, 0);
    s5RandQuat(q);
    DT_ARG_PTR(c, 0, newObject());
    DT_ARG_PTR(c, 1, q);
}

static void setup_angles(DtCtx *c)
{
    ZAngle3 *a = dt_alloc(sizeof *a);

    s5Begin(c, 0);
    a->x = (ZAngle)dt_u32();
    a->y = (ZAngle)dt_u32();
    a->z = (ZAngle)dt_u32();
    DT_ARG_PTR(c, 0, newObject());
    DT_ARG_PTR(c, 1, a);
}

static void setup_lists(DtCtx *c)
{
    s5Begin(c, 0);
    ORIG_VAR(0x006722b8, uint32_t, gTreeHead) = dt_u32();
    ORIG_VAR(0x006722c4, uint32_t, gPropHead) = dt_u32();
}

/* ---- allocation ----------------------------------------------------------------------------------- */

static void *BMX_CDECL stubPoolAlloc(void *pool)
{
    s5Log(0x486050);
    s5LogPtr(pool);
    return dt_u32() % 3 == 0 ? NULL : (void *)(uintptr_t)(0x30000000u + (dt_u32() & 0xffff0u));
}

static void setup_alloc(DtCtx *c)
{
    s5Begin(c, 0);
    gZobjPool = (ZMemFreeList *)(uintptr_t)(0x28000000u + (dt_u32() & 0xffff0u));
    dt_stub(0x00486050, NULL, (void *)stubPoolAlloc);
}

static void setup_init(DtCtx *c)
{
    ZObj *o = dt_alloc(sizeof *o);
    int initialized = dt_u32() % 4 != 0;

    s5Begin(c, 1);
    gZobjInitialized = initialized;
    dt_fill(o, sizeof *o);
    DT_ARG_PTR(c, 0, o);
    DT_ARG_INT(c, 1, dt_u32() % 8);
}

static void setup_free(DtCtx *c)
{
    ZObj *o;
    ZObj *parent;

    s5Begin(c, 1);
    makePool();
    o = poolObject();
    parent = poolObject();
    switch (dt_u32() % 3) {
    case 0:
        break;
    case 1:
        o->links = newLinks(NULL, 0, NULL, NULL);
        break;
    default:
        parent->links = newLinks(NULL, 0, NULL, o);
        o->links = newLinks(parent, dt_u32() % 3, NULL, NULL);
        o->flags |= kZobjAttached;
        break;
    }
    DT_ARG_PTR(c, 0, o);
}

/* ---- hierarchy ------------------------------------------------------------------------------------ */

static void setup_detach(DtCtx *c)
{
    ZObj *parent = newObject();
    ZObj *a = newObject();
    ZObj *b = newObject();
    ZObj *obj = newObject();
    ZObj *d = newObject();
    ZObj *kid = newObject();
    ZObj *kid2 = newObject();

    s5Begin(c, 1);
    obj->flags |= kZobjAttached;
    a->flags |= kZobjAttached;
    switch (dt_u32() % 7) {
    case 0: /* no links */
        break;
    case 1: /* first child of its parent */
        parent->links = newLinks(NULL, 0, NULL, obj);
        obj->links = newLinks(parent, dt_u32() % 3, d, NULL);
        d->links = newLinks(parent, 0, NULL, NULL);
        break;
    case 2: /* later child: the previous sibling is the one that gets reset */
        parent->links = newLinks(NULL, 0, NULL, a);
        a->links = newLinks(parent, 1, b, NULL);
        b->links = newLinks(parent, 2, obj, NULL);
        obj->links = newLinks(parent, 3, d, NULL);
        d->links = newLinks(parent, 4, NULL, NULL);
        break;
    case 3: /* not in the parent's list */
        parent->links = newLinks(NULL, 0, NULL, a);
        a->links = newLinks(parent, 1, b, NULL);
        b->links = newLinks(parent, 2, NULL, NULL);
        obj->links = newLinks(parent, 3, NULL, kid);
        kid->links = newLinks(obj, 1, kid2, NULL);
        kid2->links = newLinks(obj, 2, NULL, NULL);
        break;
    case 4: /* root with children */
        obj->links = newLinks(NULL, 0, NULL, kid);
        kid->links = newLinks(obj, 5, kid2, NULL);
        kid2->links = newLinks(obj, 6, NULL, NULL);
        break;
    case 5: /* root without children */
        obj->links = newLinks(NULL, 0, NULL, NULL);
        break;
    default: /* only child */
        parent->links = newLinks(NULL, 0, NULL, obj);
        obj->links = newLinks(parent, 7, NULL, NULL);
        break;
    }
    DT_ARG_PTR(c, 0, obj);
}

static int BMX_CDECL childCallback(ZObj *child, int arg)
{
    s5Log(0x2010);
    s5LogPtr(child);
    s5Log((uint32_t)arg);
    return 1;
}

static void setup_each_child(DtCtx *c)
{
    ZObj *parent = newObject();
    ZObj *prev = NULL;
    int count = dt_range(0, 4);
    ZObj *first = NULL;

    s5Begin(c, 0);
    for (int i = 0; i < count; i++) {
        ZObj *k = newObject();
        k->links = newLinks(parent, 0, NULL, NULL);
        if (prev != NULL) {
            prev->links->next = k;
        } else {
            first = k;
        }
        prev = k;
    }
    if (dt_u32() % 5 != 0) {
        parent->links = newLinks(NULL, 0, NULL, first);
    }
    DT_ARG_PTR(c, 0, parent);
    DT_ARG_PTR(c, 1, (void *)childCallback);
    DT_ARG_INT(c, 2, dt_u32());
}

/* Parent matrix of a part: the renderer's node matrix, random. */
static void BMX_CDECL stubNodeMatrix(void *obj, uint32_t node, void *out, int flag)
{
    float *f = out;

    s5Log(0x4891b0);
    s5LogPtr(obj);
    s5Log(node);
    s5Log((uint32_t)flag);
    for (int i = 0; i < 16; i++) {
        f[i] = dt_float(-2.0f, 2.0f);
    }
}

static void BMX_CDECL stubDispatch(ZObj *o)
{
    s5Log(0x49c570);
    s5LogPtr(o);
}

static void stubsRefresh(void)
{
    dt_stub(0x004891b0, NULL, (void *)stubNodeMatrix);
    dt_stub(0x0049c570, NULL, (void *)stubDispatch);
}

/* A chain of objects: each active one has a few children, some of those have children too. */
static void makeTree(ZObj *root, int depth)
{
    ZObj *prev = NULL;
    const int count = dt_range(0, 3);

    for (int i = 0; i < count; i++) {
        ZObj *k = newObject();
        k->flags = (k->flags & ~(uint32_t)kZobjActive) | (dt_u32() % 4 != 0 ? (uint32_t)kZobjActive : 0);
        k->links = newLinks(root, dt_u32() % 3 == 0 ? 0x1000 + dt_u32() % 4 : 0, NULL, NULL);
        if (prev != NULL) {
            prev->links->next = k;
        } else {
            root->links->firstChild = k;
        }
        prev = k;
        if (depth > 0 && dt_u32() % 2 == 0) {
            makeTree(k, depth - 1);
        }
    }
}

static void setup_refresh(DtCtx *c)
{
    ZObj *root = newObject();

    s5Begin(c, 0);
    stubsRefresh();
    root->flags |= dt_u32() % 5 != 0 ? kZobjActive : 0;
    root->links = newLinks(newObject(), dt_u32() % 3 == 0 ? 0x2001 : 0, NULL, NULL);
    root->links->parent->links = newLinks(NULL, 0, NULL, root);
    makeTree(root, 2);
    DT_ARG_PTR(c, 0, root);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_update_all(DtCtx *c)
{
    int roots = dt_range(0, 3);

    s5Begin(c, 0);
    stubsRefresh();
    makePool();
    for (int i = 0; i < roots; i++) {
        ZObj *o = poolObject();

        o->flags &= ~(uint32_t)kZobjAttached;
        o->flags |= dt_u32() % 5 != 0 ? kZobjActive : 0;
        if (dt_u32() % 6 == 0) {
            o->flags |= kZobjAttached;
        }
        if (dt_u32() % 3 != 0) {
            o->links = newLinks(NULL, 0, NULL, NULL);
            makeTree(o, 1);
        }
    }
    DT_ARG_INT(c, 0, dt_u32());
}

static int BMX_CDECL stubBlya(int16_t x, int16_t y, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    s5Log(0x47f940);
    s5Log((uint32_t)(uint16_t)x);
    s5Log((uint32_t)(uint16_t)y);
    s5Log(s5Hash(format));
    s5Log((uint32_t)va_arg(ap, int));
    va_end(ap);
    return 0;
}

static void setup_debug_print(DtCtx *c)
{
    s5Begin(c, 0);
    dt_stub(0x0047f940, NULL, (void *)stubBlya);
    makePool();
    for (int i = dt_range(0, 5); i > 0; i--) {
        poolObject();
    }
}

/* ---- initialisation ------------------------------------------------------------------------------- */

static int gModelOk, gCameraOk;

static ZrwUpdateNode *BMX_CDECL stubRegister(int type, ZrwUpdateFn fn, void *arg, int append)
{
    s5Log(0x48ba30);
    s5Log((uint32_t)type);
    s5Log((uint32_t)(uintptr_t)fn);
    s5LogPtr(arg);
    s5Log((uint32_t)append);
    return dt_u32() % 4 == 0 ? NULL : (ZrwUpdateNode *)(uintptr_t)(0x31000000u + (dt_u32() & 0xfff0u));
}

static ZrwUpdateNode *BMX_CDECL stubDbgAdd(int unused, ZrwUpdateFn fn, void *arg)
{
    s5Log(0x4801f0);
    s5Log((uint32_t)unused);
    s5Log((uint32_t)(uintptr_t)fn);
    s5LogPtr(arg);
    return (ZrwUpdateNode *)(uintptr_t)(0x32000000u + (dt_u32() & 0xfff0u));
}

static int BMX_CDECL stubModelInit(void)
{
    s5Log(0x488f40);
    return gModelOk;
}

static void BMX_CDECL stubLightInit(void)
{
    s5Log(0x48ffc0);
}

static int BMX_CDECL stubCameraInit(void)
{
    s5Log(0x482130);
    return gCameraOk;
}

static void BMX_CDECL stubPEmitInit(void)
{
    s5Log(0x495b80);
}

static void setup_initialize(DtCtx *c)
{
    s5Begin(c, 1);
    gZobjInitialized = dt_u32() % 5 == 0;
    gModelOk = dt_u32() % 4 != 0;
    gCameraOk = dt_u32() % 4 != 0;
    dt_stub(0x0048ba30, NULL, (void *)stubRegister);
    dt_stub(0x004801f0, NULL, (void *)stubDbgAdd);
    dt_stub(0x00488f40, NULL, (void *)stubModelInit);
    dt_stub(0x0048ffc0, NULL, (void *)stubLightInit);
    dt_stub(0x00482130, NULL, (void *)stubCameraInit);
    dt_stub(0x00495b80, NULL, (void *)stubPEmitInit);
    DT_ARG_INT(c, 0, dt_u32() % 6 == 0 ? 0 : dt_range(1, 100));
}

DT_CASES(zobj_cases) = {
    {"zobjTreeObjectList", 0x00486ec0, zobjTreeObjectList, 0, DT_RET_INT, setup_lists, 2, 0},
    {"zobjPropList", 0x00486ed0, zobjPropList, 0, DT_RET_INT, setup_lists, 2, 0},
    {"zobjRefresh", 0x00486ee0, zobjRefresh, 2, DT_RET_INT, setup_refresh, 300, 0},
    {"zobjUpdateAll", 0x00487020, zobjUpdateAll, 1, DT_RET_INT, setup_update_all, 300, 0},
    {"zobjDebugPrint", 0x004871e0, zobjDebugPrint, 0, DT_RET_INT, setup_debug_print, 20, 0},
    {"zobjInitialize", 0x00487210, zobjInitialize, 1, DT_RET_INT, setup_initialize, 200, 0},
    {"zobjAlloc", 0x00487370, zobjAlloc, 0, DT_RET_INT, setup_alloc, 30, 0},
    {"zobjInit", 0x004873a0, zobjInit, 2, DT_RET_INT, setup_init, 100, 0},
    {"zobjFree", 0x00487400, zobjFree, 1, DT_RET_INT, setup_free, 200, 0},
    {"zobjGetModel", 0x00487440, zobjGetModel, 1, DT_RET_INT, setup_accessor, 10, 0},
    {"zobjSetPosition", 0x00487470, zobjSetPosition, 2, DT_RET_VOID, setup_position, 200, 0},
    {"zobjGetPosition", 0x004874a0, zobjGetPosition, 1, DT_RET_INT, setup_accessor, 10, 0},
    {"zobjSetRotation", 0x004874d0, zobjSetRotation, 2, DT_RET_VOID, setup_rotation, 200, 0},
    {"zobjSetAngles", 0x00487520, zobjSetAngles, 2, DT_RET_VOID, setup_angles, 200, 0},
    {"zobjSetVisible", 0x00487600, zobjSetVisible, 2, DT_RET_VOID, setup_visible, 200, 0},
    {"zobjDetach", 0x00487830, zobjDetach, 1, DT_RET_VOID, setup_detach, 300, 0},
    {"zobjForEachChild", 0x004879c0, zobjForEachChild, 3, DT_RET_INT, setup_each_child, 100, 0},
};
