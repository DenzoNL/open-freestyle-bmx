/*
 * Particle emitters and look-at (C:\NgEngine\Zge\ZgeMods\zobjpemit.c): the handlers of the effect tables
 * (an effect keeps a list of modifiers, its emitters a list of particles; see ngiffx.h for the types), the
 * methods of the emitter object, and a few geometry helpers the packet filed here.
 */
#ifndef ZGE_ZGEMODS_ZOBJPEMIT_H
#define ZGE_ZGEMODS_ZOBJPEMIT_H

#include <stdint.h>

#include "bmx/com/zge/ngif/ngiffx.h"
#include "compat/orig.h"
#include "zge/zlist.h"
#include "zge/zmath.h"

/* Builds the view matrix of a camera at `eye` looking at `target`; returns out. Callers pass a fourth, unused
 * 0. */
ZMatrix *zobjLookAtMatrix(ZMatrix *out, const ZVec3 *eye, const ZVec3 *target);

/* Builds the clipping plane through `point` for the face direction `dir` (a plane is 5 floats); returns it.
 */
float *zobjPlaneFromFace(float *plane, const float *point, const float *dir);
/*
 * Clips the segment a-b against a plane in place; returns 0 when nothing is left. Quirk: the original also
 * copies an uninitialised fourth word (stack garbage) into a[3] / b[3] when it moves an end point; the port
 * leaves them alone.
 */
int zobjClipSegmentToPlane(float *a, float *b, const float *plane);
/* Rotation matrix that turns the direction `from` into `to` (the identity when they almost agree). */
void zobjAxisMatrix(ZMatrix *out, const ZVec3 *from, const ZVec3 *to);

/* Effect handlers (slots of ZfxEffect): destroy, add a modifier, add one by name, clear the modifiers. */
void zobjPEmitEffectDestroy(ZfxEffect *effect);
void zobjPEmitAddModifier(ZfxEffect *effect, ZfxModifier *modifier, void *arg);
void zobjPEmitAddModifierByName(ZfxEffect *effect, const char *name, void *arg);
void zobjPEmitClearModifiers(ZfxEffect *effect);
/* Creates and destroys the emitter of an effect (`capacity` particles); NULL when out of memory. */
ZfxEmitter *zobjPEmitCreate(uint32_t effect, uint16_t capacity);
void zobjPEmitDestroy(ZfxEffect *effect, ZfxEmitter *emitter);
/* Advances the particles and runs the modifiers; returns the number of particles seen. */
int zobjPEmitUpdate(ZfxEffect *effect, ZfxEmitter *emitter);

/* Item class of an emitter object: methods at +0x14 (destroy) and +0x1c (update). */
typedef struct ZobjPEmitClass {
    uint8_t pad00[0x14];
    void(BMX_CDECL *destroyItem)(struct ZobjPEmitClass *self, void *item); /* 0x14 */
    uint32_t pad18;
    int(BMX_CDECL *updateItem)(struct ZobjPEmitClass *self, void *item); /* 0x1c */
} ZobjPEmitClass;

typedef struct ZobjPEmitItem {
    uint8_t pad00[0x2c];
    ZobjPEmitClass *cls; /* 0x2c */
} ZobjPEmitItem;

/* State of an emitter object (at +0x30 of the object). */
typedef struct ZobjPEmitState {
    ZList *items;       /* 0x00 */
    void *updateNode;   /* 0x04 registered update function */
    float position[3];  /* 0x08 */
    uint32_t pad14;     /* 0x14 */
    float rect[4];      /* 0x18 */
    float direction[3]; /* 0x28 */
    uint32_t pad34;     /* 0x34 */
    uint32_t value;     /* 0x38 */
} ZobjPEmitState;

typedef struct ZobjPEmitObject {
    uint8_t pad00[0x30];
    ZobjPEmitState *state; /* 0x30 */
} ZobjPEmitObject;

/* Object methods: update every item, destroy (also frees the state), and the setters (0 without a state). */
int zobjPEmitObjectUpdate(ZobjPEmitObject *object);
int zobjPEmitObjectDestroy(ZobjPEmitObject *object);
int zobjPEmitSetPosition(ZobjPEmitObject *object, const float *position);
int zobjPEmitSetRect(ZobjPEmitObject *object, const float *rect);
int zobjPEmitSetDirection(ZobjPEmitObject *object, const float *direction);
int zobjPEmitSetValue(ZobjPEmitObject *object, uint32_t value);
int zobjPEmitInitialize(void);

#define gZobjPEmitInitialized ORIG_VAR(0x006741b4, int, gZobjPEmitInitialized)

#endif
