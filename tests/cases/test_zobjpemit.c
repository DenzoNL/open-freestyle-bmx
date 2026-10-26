/*
 * Tests for zobjPEmit.c: the handlers of the effect tables, the emitter updater (real lists and free lists,
 * modifiers that log what they see), the emitter object methods and the geometry helpers. The look-at and
 * axis matrices run on the real maths code.
 */
#include "dt.h"
#include "dt_ngif.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zgemods/zobjpemit.h"

#define NOW 1001u /* the first timeGetTime of an iteration */

typedef ZList *(*ListCreateFn)(int);
typedef void (*ListInsertFn)(ZList *, void *);
typedef ZMemFreeList *(*PoolCreateFn)(uint32_t, uint32_t, ZMemDestroyFn);
typedef void *(*PoolAllocFn)(ZMemFreeList *);

static float rf(void)
{
    return dt_float_edgy(-100.0f, 100.0f);
}

/* ---- geometry -------------------------------------------------------------------------------------- */

static void random_vec(ZVec3 *v, float range)
{
    v->x = dt_float_edgy(-range, range);
    v->y = dt_float_edgy(-range, range);
    v->z = dt_float_edgy(-range, range);
}

static void setup_lookat(DtCtx *c)
{
    ZMatrix *out = dt_alloc(sizeof *out);
    ZVec3 *eye = dt_alloc(sizeof *eye);
    ZVec3 *target = dt_alloc(sizeof *target);

    ngBegin(c, 0);
    dt_fill(out, sizeof *out);
    random_vec(eye, 200.0f);
    random_vec(target, 200.0f);
    if (dt_u32() % 8 == 0) { /* straight up / down */
        target->x = eye->x;
        target->z = eye->z;
    }
    DT_ARG_PTR(c, 0, out);
    DT_ARG_PTR(c, 1, eye);
    DT_ARG_PTR(c, 2, target);
}

static void setup_plane(DtCtx *c)
{
    float *plane = dt_alloc(5 * sizeof *plane);
    float *point = dt_alloc(4 * sizeof *point);
    float *dir = dt_alloc(4 * sizeof *dir);

    ngBegin(c, 0);
    dt_fill(plane, 5 * sizeof *plane);
    for (int i = 0; i < 4; i++) {
        point[i] = rf();
        dir[i] = rf();
    }
    DT_ARG_PTR(c, 0, plane);
    DT_ARG_PTR(c, 1, point);
    DT_ARG_PTR(c, 2, dir);
}

static void setup_clip(DtCtx *c)
{
    float *a = dt_alloc(4 * sizeof *a);
    float *b = dt_alloc(4 * sizeof *b);
    float *plane = dt_alloc(5 * sizeof *plane);
    ZVec3 normal;

    ngBegin(c, 0);
    for (int i = 0; i < 4; i++) {
        a[i] = rf();
        b[i] = rf();
    }
    random_vec(&normal, 1.0f);
    plane[0] = normal.x;
    plane[1] = normal.y;
    plane[2] = normal.z;
    plane[3] = 0.0f;
    plane[4] = rf();
    switch (dt_u32() % 5) {
    case 0: /* both ends on the plane's side, or on the plane */
        plane[4] = -(plane[0] * a[0] + plane[1] * a[1] + plane[2] * a[2]);
        break;
    case 1: /* the segment crosses it */
        plane[4] = -(float)(((double)(plane[0] * (a[0] + b[0]) + plane[1] * (a[1] + b[1]) +
                                      plane[2] * (a[2] + b[2]))) *
                            0.5);
        break;
    default:
        break;
    }
    /*
     * The original copies a stale stack word into a[3] / b[3] when it moves an end point; in the harness that
     * word is the address of b. Start with it there, so only the real results are compared (the fourth word
     * itself is not).
     */
    __builtin_memcpy(&a[3], &(uint32_t){(uint32_t)(uintptr_t)b}, 4);
    __builtin_memcpy(&b[3], &(uint32_t){(uint32_t)(uintptr_t)b}, 4);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
    DT_ARG_PTR(c, 2, plane);
}

static void setup_axis(DtCtx *c)
{
    ZMatrix *out = dt_alloc(sizeof *out);
    ZVec3 *from = dt_alloc(sizeof *from);
    ZVec3 *to = dt_alloc(sizeof *to);

    ngBegin(c, 0);
    dt_fill(out, sizeof *out);
    random_vec(from, 1.0f);
    random_vec(to, 1.0f);
    switch (dt_u32() % 4) {
    case 0: /* almost the same direction: the identity */
        *to = *from;
        to->x += dt_float(-0.0001f, 0.0001f);
        break;
    case 1: /* opposite directions */
        to->x = -from->x;
        to->y = -from->y;
        to->z = -from->z;
        break;
    default:
        break;
    }
    DT_ARG_PTR(c, 0, out);
    DT_ARG_PTR(c, 1, from);
    DT_ARG_PTR(c, 2, to);
}

/* ---- effect handlers ------------------------------------------------------------------------------- */

static void BMX_CDECL effectHook(ZfxEffect *effect)
{
    ngLog(0xe0e0);
    ngLogPtr(effect);
}

static void BMX_CDECL stubModifier(ZfxModifier *self, ZfxEmitter *emitter, void *arg)
{
    ngLog(0x1000 + (uint32_t)(uintptr_t)arg);
    ngLogPtr(self);
    ngLogPtr(emitter);
    ngLogFloat(emitter->min.x);
    ngLogFloat(emitter->max.z);
}

static void setup_effect_destroy(DtCtx *c)
{
    ZfxEffect *effect = dt_alloc(sizeof *effect);

    ngBegin(c, 0);
    effect->pad0c[0] = (uint32_t)(uintptr_t)effectHook;
    DT_ARG_PTR(c, 0, effect);
}

/* Modifier nodes of an effect: head pointer, tail and count as the list code keeps them. */
static void fill_modifiers(ZfxEffect *effect, int count)
{
    uint32_t *words = (uint32_t *)effect;
    ZfxModifierNode *prev = NULL;

    for (int i = 0; i < count; i++) {
        ZfxModifierNode *node = dt_alloc(sizeof *node);

        node->modifier = dt_alloc(sizeof(ZfxModifier));
        node->modifier->apply = stubModifier;
        node->arg = (void *)(uintptr_t)(i + 1);
        if (prev == NULL) {
            effect->modifiers = node;
        } else {
            prev->next = node;
            prev->next = node;
        }
        node->pad4 = (uint32_t)(uintptr_t)prev;
        prev = node;
    }
    words[10] = (uint32_t)(uintptr_t)prev; /* tail */
    words[11] = (uint32_t)count;
}

static void setup_add_modifier(DtCtx *c)
{
    ZfxEffect *effect = dt_alloc(sizeof *effect);

    ngBegin(c, 0);
    fill_modifiers(effect, dt_range(0, 3));
    DT_ARG_PTR(c, 0, effect);
    DT_ARG_PTR(c, 1, dt_alloc(8));
    DT_ARG_INT(c, 2, dt_u32());
}

static void *BMX_CDECL stubFindName(const char *name)
{
    ngLog(0x482e90);
    ngLog(ngHash(name));
    return (dt_u32() % 3 == 0) ? NULL : (void *)(uintptr_t)(dt_u32() | 4u);
}

static void setup_add_by_name(DtCtx *c)
{
    ZfxEffect *effect = dt_alloc(sizeof *effect);
    static const char *const kNames[] = {"gravity", "floor", "", "x"};

    ngBegin(c, 0);
    fill_modifiers(effect, dt_range(0, 3));
    dt_stub(0x00482e90, NULL, (void *)stubFindName);
    DT_ARG_PTR(c, 0, effect);
    DT_ARG_PTR(c, 1, kNames[dt_u32() % 4]);
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_clear(DtCtx *c)
{
    ZfxEffect *effect = dt_alloc(sizeof *effect);

    ngBegin(c, 0);
    dt_zmem_env();
    for (int i = dt_range(0, 3); i > 0; i--) {
        ZfxModifierNode *node = dt_zmemalloc(sizeof *node);

        node->modifier = NULL;
        node->arg = NULL;
        DT_ORIG(0x0048c150, void (*)(void *, void *))((void *)&effect->modifiers, node);
    }
    DT_ARG_PTR(c, 0, effect);
}

/* ---- emitters -------------------------------------------------------------------------------------- */

static void new_particle(ZfxEmitter *e)
{
    ZfxParticle *p = DT_ORIG(0x00486050, PoolAllocFn)(e->pool);
    const uint32_t birth = (uint32_t)dt_range(800, 1001);
    static const uint32_t kRemaining[] = {0, 1, 5, 100, 1000, 30000, 0xffffffffu};

    dt_fill(p, sizeof *p);
    p->kind = dt_range(0, 3);
    p->pos = (ZVec3){rf(), rf(), rf()};
    p->prev = (ZVec3){rf(), rf(), rf()};
    p->vel = (ZVec3){rf(), rf(), rf()};
    p->axis = (ZVec3){rf(), rf(), rf()};
    p->spin = (ZAngle3){(ZAngle)dt_u32(), (ZAngle)dt_u32(), (ZAngle)dt_u32()};
    p->birth = birth;
    p->life = kRemaining[dt_u32() % 7] + NOW - birth;
    DT_ORIG(0x0048c460, ListInsertFn)(e->particles, p);
}

static ZfxEmitter *new_emitter(int particles)
{
    ZfxEmitter *e = dt_alloc(sizeof *e);

    dt_fill(e, sizeof *e);
    e->particles = DT_ORIG(0x0048c260, ListCreateFn)(0x20);
    e->pool = DT_ORIG(0x00485e80, PoolCreateFn)(sizeof(ZfxParticle), (uint32_t)dt_range(1, 6), NULL);
    for (int i = 0; i < particles; i++) {
        new_particle(e);
    }
    e->lastUpdate = NOW - (uint32_t)(dt_u32() % 3 == 0 ? dt_range(0, 100000) : dt_range(0, 40));
    return e;
}

static void setup_create(DtCtx *c)
{
    ngBegin(c, 0);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, (dt_u32() & 0xffff0000u) | (uint32_t)dt_range(0, 50));
}

static void setup_destroy_emitter(DtCtx *c)
{
    ZfxEmitter *e;

    ngBegin(c, 0);
    e = new_emitter(dt_range(0, 3));
    if (dt_u32() % 4 == 0) {
        DT_ORIG(0x0048c2b0, void (*)(ZList *))(e->particles);
        e->particles = NULL;
    }
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_PTR(c, 1, e);
}

static void setup_update(DtCtx *c)
{
    ZfxEffect *effect = dt_alloc(sizeof *effect);
    ZfxEmitter *e;

    ngBegin(c, 0);
    e = new_emitter(dt_range(0, 5));
    fill_modifiers(effect, dt_range(0, 3));
    if (dt_u32() % 8 == 0) { /* no particle list */
        e->particles = NULL;
    }
    DT_ARG_PTR(c, 0, effect);
    DT_ARG_PTR(c, 1, e);
}

/* ---- emitter objects ------------------------------------------------------------------------------- */

static int BMX_CDECL itemUpdate(ZobjPEmitClass *self, void *item)
{
    ngLog(0x1c0001);
    ngLogPtr(self);
    ngLogPtr(item);
    return 1;
}

static void BMX_CDECL itemDestroy(ZobjPEmitClass *self, void *item)
{
    ngLog(0x140001);
    ngLogPtr(self);
    ngLogPtr(item);
}

static int BMX_CDECL stubUnregister(void *node)
{
    ngLog(0x48bad0);
    ngLogPtr(node);
    return 1;
}

static ZobjPEmitObject *new_object(int items, int withState)
{
    ZobjPEmitObject *o = dt_alloc(0x40);
    ZobjPEmitClass *cls = dt_alloc(sizeof *cls);

    dt_fill(o, 0x40);
    cls->updateItem = itemUpdate;
    cls->destroyItem = itemDestroy;
    o->state = NULL;
    if (withState) {
        ZobjPEmitState *s = dt_zmemalloc(sizeof *s);

        dt_fill(s, sizeof *s);
        s->items = DT_ORIG(0x0048c260, ListCreateFn)(0x10);
        for (int i = 0; i < items; i++) {
            ZobjPEmitItem *item = dt_alloc(sizeof *item);

            dt_fill(item, sizeof *item);
            item->cls = cls;
            DT_ORIG(0x0048c460, ListInsertFn)(s->items, item);
        }
        s->updateNode = (void *)(uintptr_t)(dt_u32() | 4u);
        o->state = s;
    }
    return o;
}

static void setup_object_update(DtCtx *c)
{
    ngBegin(c, 0);
    dt_zmem_env();
    DT_ARG_PTR(c, 0, new_object(dt_range(0, 4), 1));
}

static void setup_object_destroy(DtCtx *c)
{
    setup_object_update(c);
    dt_stub(0x0048bad0, NULL, (void *)stubUnregister);
}

static void setup_setter(DtCtx *c)
{
    float *values = dt_alloc(4 * sizeof *values);

    ngBegin(c, 0);
    dt_zmem_env();
    for (int i = 0; i < 4; i++) {
        values[i] = rf();
    }
    DT_ARG_PTR(c, 0, new_object(0, dt_u32() % 4 != 0));
    DT_ARG_PTR(c, 1, values);
}

static void setup_set_value(DtCtx *c)
{
    ngBegin(c, 0);
    dt_zmem_env();
    DT_ARG_PTR(c, 0, new_object(0, dt_u32() % 4 != 0));
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_initialize(DtCtx *c)
{
    ngBegin(c, 1);
    gZobjPEmitInitialized = (int)(dt_u32() & 1);
}

DT_CASES(zobjpemit_cases) = {
    {"zobjPEmitEffectDestroy", 0x00495380, zobjPEmitEffectDestroy, 1, DT_RET_VOID, setup_effect_destroy, 20,
     0},
    {"zobjPEmitCreate", 0x00495390, zobjPEmitCreate, 2, DT_RET_INT, setup_create, 100, 0},
    {"zobjPEmitDestroy", 0x004953e0, zobjPEmitDestroy, 2, DT_RET_VOID, setup_destroy_emitter, 100, 0},
    {"zobjPEmitAddModifier", 0x00495410, zobjPEmitAddModifier, 3, DT_RET_VOID, setup_add_modifier, 100, 0},
    {"zobjPEmitClearModifiers", 0x00495440, zobjPEmitClearModifiers, 1, DT_RET_VOID, setup_clear, 100, 0},
    {"zobjPEmitAddModifierByName", 0x00495470, zobjPEmitAddModifierByName, 3, DT_RET_VOID, setup_add_by_name,
     100, 0},
    {"zobjPEmitUpdate", 0x004954b0, zobjPEmitUpdate, 2, DT_RET_INT, setup_update, 1000, 0},
    {"zobjPlaneFromFace", 0x004956f0, zobjPlaneFromFace, 3, DT_RET_INT, setup_plane, 500, 0},
    {"zobjClipSegmentToPlane", 0x00495730, zobjClipSegmentToPlane, 3, DT_RET_INT, setup_clip, 2000, 0},
    {"zobjLookAtMatrix", 0x00495860, zobjLookAtMatrix, 3, DT_RET_INT, setup_lookat, 1000, 0},
    {"zobjAxisMatrix", 0x00495900, zobjAxisMatrix, 3, DT_RET_VOID, setup_axis, 1000, 0},
    {"zobjPEmitObjectUpdate", 0x00495a10, zobjPEmitObjectUpdate, 1, DT_RET_INT, setup_object_update, 100, 0},
    {"zobjPEmitObjectDestroy", 0x00495a60, zobjPEmitObjectDestroy, 1, DT_RET_INT, setup_object_destroy, 100,
     0},
    {"zobjPEmitSetPosition", 0x00495ad0, zobjPEmitSetPosition, 2, DT_RET_INT, setup_setter, 100, 0},
    {"zobjPEmitSetRect", 0x00495b00, zobjPEmitSetRect, 2, DT_RET_INT, setup_setter, 100, 0},
    {"zobjPEmitSetDirection", 0x00495b30, zobjPEmitSetDirection, 2, DT_RET_INT, setup_setter, 100, 0},
    {"zobjPEmitSetValue", 0x00495b60, zobjPEmitSetValue, 2, DT_RET_INT, setup_set_value, 100, 0},
    {"zobjPEmitInitialize", 0x00495b80, zobjPEmitInitialize, 0, DT_RET_INT, setup_initialize, 20, 0},
};
