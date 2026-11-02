/*
 * The shadow state the roller hands to the shadow update (zrwshadow.c): kept apart from zrwshadow.h so that
 * the roller (ngifzroller.h) can embed it without the renderer's model header.
 */
#ifndef ZGE_COM_ZGLRW_ZRWSHADOWSTATE_H
#define ZGE_COM_ZGLRW_ZRWSHADOWSTATE_H

#include <stddef.h>
#include <stdint.h>

#include "compat/layout.h"
#include "zge/zmath.h"

struct ZrwModel;

/* The shadow state of the roller (ZrollerShadow in ngifzroller.h, the update callback's argument): the model
 * whose object casts the shadow and the offset of the ground probes' eye from that object. */
typedef struct ZrwShadow {
    int32_t kind; /* 0x00 */
    void **slots; /* 0x04 the shadow models: [0] the rider's, [1] the bike's (ngifzroller.c) */
    const struct ZrwModel *model; /* 0x08 its user value is the scene object whose position is sampled */
    ZVec3 offset;                 /* 0x0c */
} ZrwShadow;

BMX_LAYOUT32_ASSERT(offsetof(ZrwShadow, model) == 0x08, "shadow state");
BMX_LAYOUT32_ASSERT(offsetof(ZrwShadow, offset) == 0x0c, "shadow state");

#endif
