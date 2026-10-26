/* Particle emitters and look-at (C:\NgEngine\Zge\ZgeMods\zobjPEmit.c). */
#include "zge/zgemods/zobjpemit.h"

#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kPEmitFile[] = "C:\\NgEngine\\Zge\\ZgeMods\\zobjPEmit.c";

/* The modifier list of an effect sits at +0x24 of its table entry: head, tail, count. */
static ZrwList *modifierList(ZfxEffect *effect)
{
    return (ZrwList *)&effect->modifiers;
}

BMX_ADDR(0x00495380)
void zobjPEmitEffectDestroy(ZfxEffect *effect)
{
    ((void(BMX_CDECL *)(ZfxEffect *))(uintptr_t)effect->pad0c[0])(effect);
}

BMX_ADDR(0x00495390)
ZfxEmitter *zobjPEmitCreate(uint32_t effect, uint16_t capacity)
{
    ZfxEmitter *emitter = zmemAlloc(0x30);

    if (emitter == NULL) {
        return NULL;
    }
    emitter->particles = zlistCreate(capacity);
    emitter->pool = zmemFreeListCreate(0x60, capacity, NULL);
    emitter->effect = effect;
    emitter->lastUpdate = gTimeGetTime();
    return emitter;
}

BMX_ADDR(0x004953e0)
void zobjPEmitDestroy(ZfxEffect *effect, ZfxEmitter *emitter)
{
    (void)effect;
    if (emitter->particles != NULL) {
        zlistDestroy(emitter->particles);
    }
    zmemFreeListDestroy(emitter->pool);
    zmemFree(emitter);
}

BMX_ADDR(0x00495410)
void zobjPEmitAddModifier(ZfxEffect *effect, ZfxModifier *modifier, void *arg)
{
    ZfxModifierNode *node = zmemAlloc(0x10);

    node->modifier = modifier;
    node->arg = arg;
    zrwListAdd(modifierList(effect), node);
}

BMX_ADDR(0x00495440)
void zobjPEmitClearModifiers(ZfxEffect *effect)
{
    ZfxModifierNode *node = effect->modifiers;

    while (node != NULL) {
        zrwListRemove(modifierList(effect), node);
        zmemFree(node);
        node = effect->modifiers;
    }
}

BMX_ADDR(0x00495470)
void zobjPEmitAddModifierByName(ZfxEffect *effect, const char *name, void *arg)
{
    ZfxModifier *modifier = zstrNameTable1Find(name);

    if (modifier != NULL) {
        ZfxModifierNode *node = zmemAlloc(0x10);

        node->modifier = modifier;
        node->arg = arg;
        zrwListAdd(modifierList(effect), node);
    }
}

/* Milliseconds to seconds. */
#define kMillisecond 0x1.0624dep-10f

/*
 * Advances all particles by the time since the last update, rebuilds the emitter's bounds, retires the
 * particles past their life and runs the modifiers. Quirk: a moving particle gets its velocity added once
 * more on top of the time scaled step. The elapsed time is not rounded to float (the x87 loads it as an
 * integer).
 */
BMX_ADDR(0x004954b0)
int zobjPEmitUpdate(ZfxEffect *effect, ZfxEmitter *emitter)
{
    const uint32_t now = gTimeGetTime();
    const uint32_t elapsed = now - emitter->lastUpdate;
    int count = 0;

    if (emitter != NULL && emitter->particles != NULL) {
        const double dt = (double)elapsed;
        ZfxParticle *p;

        emitter->min.x = emitter->min.y = emitter->min.z = 99999.0f;
        emitter->max.x = emitter->max.y = emitter->max.z = -99999.0f;
        p = zlistFirst(emitter->particles);
        while (p != NULL) {
            count++;
            if ((p->kind & 1) != 0) {
                p->prev = p->pos;
                p->pos.x = (float)((dt * p->vel.x) * kMillisecond + p->pos.x);
                p->pos.y = (float)((dt * p->vel.y) * kMillisecond + p->pos.y);
                p->pos.z = (float)((dt * p->vel.z) * kMillisecond + p->pos.z);
                zvec3Add(&p->pos, &p->pos, &p->vel);
                if (p->pos.x > emitter->max.x) {
                    emitter->max.x = p->pos.x;
                }
                if (p->pos.y > emitter->max.y) {
                    emitter->max.y = p->pos.y;
                }
                if (p->pos.z > emitter->max.z) {
                    emitter->max.z = p->pos.z;
                }
                if (!(p->pos.x >= emitter->min.x)) {
                    emitter->min.x = p->pos.x;
                }
                if (!(p->pos.y >= emitter->min.y)) {
                    emitter->min.y = p->pos.y;
                }
                if (!(p->pos.z >= emitter->min.z)) {
                    emitter->min.z = p->pos.z;
                }
            }
            if ((p->kind & 2) != 0) {
                zvec3RotateEuler(&p->axis, &p->axis, &p->spin);
            }
            if (now - p->birth < p->life) {
                p = zlistNext(emitter->particles);
            } else {
                zlistRemoveCurrent(emitter->particles);
                zmemFreeListFree(emitter->pool, p);
                p = zlistCurrent(emitter->particles);
            }
        }
    }
    for (ZfxModifierNode *node = effect->modifiers; node != NULL; node = node->next) {
        node->modifier->apply(node->modifier, emitter, node->arg);
    }
    emitter->lastUpdate = now;
    return count;
}

BMX_ADDR(0x004956f0)
float *zobjPlaneFromFace(float *plane, const float *point, const float *dir)
{
    plane[0] = dir[0];
    plane[1] = dir[1];
    plane[2] = dir[2];
    plane[4] = (float)(zvec3Dot((const ZVec3 *)dir, (const ZVec3 *)point) * -1.0f);
    return plane;
}

/* Where the end points of a segment lie against a plane: 0 in front, 1 behind (or on it), 2 within 1e-5 of
 * it. */
static int pointSide(float distance)
{
    if (!(__builtin_fabsf(distance) >= 0x1.4f8b58p-17f)) {
        return 2;
    }
    return (distance > 0.0f) ? 0 : 1;
}

BMX_ADDR(0x00495730)
int zobjClipSegmentToPlane(float *a, float *b, const float *plane)
{
    const ZVec3 *normal = (const ZVec3 *)plane;
    const float d0 = (float)(zvec3Dot(normal, (const ZVec3 *)a) + plane[4]);
    const float d1 = (float)(zvec3Dot(normal, (const ZVec3 *)b) + plane[4]);
    const int side0 = pointSide(d0);
    const int side1 = pointSide(d1);

    if (side0 == side1) {
        return side0 != 1;
    }
    if (side0 != 0 && side1 != 0) {
        return 0;
    }
    if (side0 != 2 && side1 != 2) {
        ZVec3 cut;
        const float t = (float)((double)d0 / ((double)d0 - (double)d1));

        zvec3Lerp(&cut, (const ZVec3 *)a, (const ZVec3 *)b, t);
        if (side0 == 0) {
            b[0] = cut.x;
            b[1] = cut.y;
            b[2] = cut.z;
            return 1;
        }
        a[0] = cut.x;
        a[1] = cut.y;
        a[2] = cut.z;
    }
    return 1;
}

BMX_ADDR(0x00495860)
ZMatrix *zobjLookAtMatrix(ZMatrix *out, const ZVec3 *eye, const ZVec3 *target)
{
    ZVec3 dir;
    ZAngle pitch;
    ZAngle yaw;

    zvec3Sub(&dir, target, eye);
    zvec3NormalizeScaled(&dir, &dir);
    pitch = zmathAcos(dir.y);
    yaw = zmathAtan2(dir.x, dir.z);
    zmatrixIdentity(out);
    zmatrixRotateY(out, out, (ZAngle)-yaw);
    zmatrixRotateX(out, out, (ZAngle)(0x4000 - pitch));
    zmatrixTransformPoint(&out->pos, out, eye);
    zvec3Negate(&out->pos, &out->pos);
    return out;
}

BMX_ADDR(0x00495900)
void zobjAxisMatrix(ZMatrix *out, const ZVec3 *from, const ZVec3 *to)
{
    const double dot = zvec3Dot(from, to);
    const float c = (float)dot;
    ZVec3 axis;
    float k;
    double t;
    double a;
    double b;
    double d;
    float fb;
    float *m = (float *)out;

    if (dot > 0.999f) {
        zmatrixIdentity(out);
        return;
    }
    zvec3Cross(&axis, from, to);
    k = (float)(1.0 - (double)c);
    t = (double)k / zvec3Dot(&axis, &axis);
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[0] = (float)((double)axis.x * axis.x * t + c);
    a = (double)axis.y * axis.x * t;
    m[4] = (float)(a - axis.z);
    b = (double)axis.z * axis.x * t;
    fb = (float)b;
    m[8] = (float)(b + axis.y);
    m[1] = (float)(a + axis.z);
    m[5] = (float)((double)axis.y * axis.y * t + c);
    d = (double)axis.z * axis.y * t;
    m[9] = (float)(d - axis.x);
    m[2] = (float)((double)fb - axis.y);
    m[6] = (float)(d + axis.x);
    m[10] = (float)((double)axis.z * axis.z * t + c);
}

BMX_ADDR(0x00495a10)
int zobjPEmitObjectUpdate(ZobjPEmitObject *object)
{
    ZobjPEmitItem *item = zlistFirst(object->state->items);

    while (item != NULL) {
        item->cls->updateItem(item->cls, item);
        item = zlistNext(object->state->items);
    }
    return 1;
}

BMX_ADDR(0x00495a60)
int zobjPEmitObjectDestroy(ZobjPEmitObject *object)
{
    ZobjPEmitItem *item = zlistFirst(object->state->items);

    while (item != NULL) {
        item->cls->destroyItem(item->cls, item);
        item = zlistNext(object->state->items);
    }
    zlistDestroy(object->state->items);
    zrwUnregisterUpdateFunction(object->state->updateNode);
    if (object->state != NULL) {
        zmemFree(object->state);
    }
    return 1;
}

BMX_ADDR(0x00495ad0)
int zobjPEmitSetPosition(ZobjPEmitObject *object, const float *position)
{
    ZobjPEmitState *state = object->state;

    if (state == NULL) {
        return 0;
    }
    __builtin_memcpy(state->position, position, sizeof state->position);
    return 1;
}

BMX_ADDR(0x00495b00)
int zobjPEmitSetRect(ZobjPEmitObject *object, const float *rect)
{
    ZobjPEmitState *state = object->state;

    if (state == NULL) {
        return 0;
    }
    __builtin_memcpy(state->rect, rect, sizeof state->rect);
    return 1;
}

BMX_ADDR(0x00495b30)
int zobjPEmitSetDirection(ZobjPEmitObject *object, const float *direction)
{
    ZobjPEmitState *state = object->state;

    if (state == NULL) {
        return 0;
    }
    __builtin_memcpy(state->direction, direction, sizeof state->direction);
    return 1;
}

BMX_ADDR(0x00495b60)
int zobjPEmitSetValue(ZobjPEmitObject *object, uint32_t value)
{
    if (object->state == NULL) {
        return 0;
    }
    object->state->value = value;
    return 1;
}

/* Returns 1: the original ends with MOV EAX,1 (Ghidra shows void) and its caller tests EAX. */
BMX_ADDR(0x00495b80)
int zobjPEmitInitialize(void)
{
    if (gZobjPEmitInitialized != 0) {
        zdebugFail("Already initialized", kPEmitFile, 0xd3, 0);
    }
    gZobjPEmitInitialized = 1;
    return 1;
}
