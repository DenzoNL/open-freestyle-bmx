/*
 * RenderWare 3.1 lights (rw/light.c): RpLight objects and their setters. A light lives in the world's list
 * of lights and carries its colour, radius and cone; every setter bumps the object's stamp.
 */
#ifndef RW_RWLIGHT_H
#define RW_RWLIGHT_H

#include "rw/rwcore.h"

enum { rpLIGHTLIGHTATOMICS = 1, rpLIGHTLIGHTWORLD = 2 };

/* An RpLight (a lamp of the world); `stamp` is incremented by every setter. */
typedef struct RpLight {
    RwObjectHasFrame object; /* 0x00 type 3, subType = light type, parent = frame, sync at 0x10 */
    void *worldExtCallback;  /* 0x14 the lighting routine of its type (rpWorldExtLightSelectCallback) */
    float radius;            /* 0x18 */
    float color[4];          /* 0x1c red, green, blue, alpha */
    float coneAngle;         /* 0x2c tan of the angle */
    RwLLLink sectors;        /* 0x30 list of the world sectors it lights */
    RwLLLink inWorld;        /* 0x38 */
    uint16_t lightFrame;     /* 0x40 */
    uint8_t stamp;           /* 0x42 */
} RpLight;

_Static_assert(offsetof(RpLight, radius) == 0x18, "RpLight layout");
_Static_assert(offsetof(RpLight, coneAngle) == 0x2c, "RpLight layout");
_Static_assert(offsetof(RpLight, lightFrame) == 0x40, "RpLight layout");
_Static_assert(offsetof(RpLight, stamp) == 0x42, "RpLight layout");

/* The light plugin's data in the engine instance: the free list of lights. */
#define rpLightOffset ORIG_VAR(0x006767a0, int32_t, rpLightOffset)

/* What the for-all-lights walk hands each light: the visitor's context, callback and data (rpLightVisitOnce).
 */
typedef void(BMX_CDECL *RpLightVisitFn)(void *context, RpLight *light, void *block, void *tail, void *extra);

typedef struct RpLightVisit {
    void *context;           /* 0x00 */
    RpLightVisitFn callback; /* 0x04 */
    void *extra;             /* 0x08 */
    uint8_t block[0x40];     /* 0x0c passed by address */
    void *tail;              /* 0x4c */
} RpLightVisit;

_Static_assert(sizeof(RpLightVisit) == 0x50, "RpLightVisit layout");

void rpWorldExtLightSelectCallback(RpLight *light);
RpLight *rpLightVisitOnce(RpLight *light, RpLightVisit *visit);
RpLight *rpLightVisitEnabled(RpLight *light, RpLightVisit *visit);
int rpLightFreeListDestroyCB(void *light, void *data);
void *rpLightSyncCallback(void *light);
void *_rpLightOpen(void *engine, int32_t offset);
void *_rpLightClose(void *engine);
void rpWorldExtLightInit(RpLight *light);
RpLight *RpLightCreate(uint32_t type);
int RpLightDestroy(RpLight *light);
RpLight *RpLightSetRadius(RpLight *light, float radius);
const float *RpLightGetColor(const RpLight *light);
RpLight *RpLightSetColor(RpLight *light, const float *color);
RpLight *RpLightSetConeAngle(RpLight *light, float angle);
RpLight *RpLightSetFrame(RpLight *light, RwFrame *frame);
RwFrame *RpLightGetFrame(const RpLight *light);
uint32_t RpLightGetType(const RpLight *light);
RpLight *RpLightSetFlags(RpLight *light, uint32_t flags);
uint32_t RpLightGetFlags(const RpLight *light);

#endif
