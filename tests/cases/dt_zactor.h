/* Shared fixtures for the actor / motion tests (test_zactor.c). */
#ifndef DT_ZACTOR_H
#define DT_ZACTOR_H

#include <stdint.h>

#include "bmx/com/zge/zactor.h"
#include "dt.h"
#include "zge/zmem.h"

static inline void dt_fill(void *p, uint32_t n)
{
    uint8_t *b = p;
    for (uint32_t i = 0; i < n; i++) {
        b[i] = (uint8_t)dt_u32();
    }
}

/* A float that keeps blends and sums well behaved (no NaN, no denormals). */
static inline float dt_rf(void)
{
    return dt_float_edgy(-1000.0f, 1000.0f);
}

static inline void dt_rf3(ZVec3 *v)
{
    v->x = dt_rf();
    v->y = dt_rf();
    v->z = dt_rf();
}

/* Gives zmemStaticAlloc a fresh region. */
static inline void dt_static_region(uint32_t size)
{
    uint8_t *region = dt_alloc(size);
    gZmem.initialized = 1;
    gZmem.staticBase = (uint32_t *)region;
    gZmem.staticTop = (uint32_t *)region;
    gZmem.staticEnd = (uintptr_t)region + size;
    gZmem.lastStaticAlloc = NULL;
}

/* Random angle base / flip, sometimes with the upper half set (the exe reads both widths). */
static inline void dt_angle_globals(void)
{
    gActorAngleBase = (dt_u32() & 3) ? (int32_t)(dt_u32() & 0xffff) : (int32_t)dt_u32();
    gActorAngleFlip = (dt_u32() & 1) ? (int32_t)dt_u32() : 0;
}

static inline ZMotionPlayer *dt_rand_player(void)
{
    ZMotionPlayer *p = dt_alloc(sizeof *p);
    dt_fill(p, sizeof *p);
    for (int i = 0; i < kZmotionChannels; i++) {
        ZMotionChannel *c = &p->ch[i];
        c->jointCount = (uint8_t)dt_range(1, 19); /* 0 would make the blend loops run 2^32 times */
        c->boneCount = (uint8_t)dt_range(0, 4);
        c->counter = (uint8_t)dt_u32();
        c->motion = NULL;
        c->cursor = NULL;
        c->bones = NULL;
        c->extra = NULL;
        c->streamEnd = NULL;
        dt_rf3(&c->rootAcc);
        dt_rf3(&c->rootDelta);
        dt_rf3(&c->rootSaved);
        dt_rf3(&c->boneDelta);
        c->height = dt_rf();
        c->rootSpeed = dt_rf();
    }
    p->blend = (uint8_t)dt_range(0, 5);
    return p;
}

/* Random actor of `type` with a random motion player; flags are fully random. */
static inline ZActor *dt_rand_actor(int type)
{
    ZActor *a = dt_alloc(sizeof *a);
    dt_fill(a, sizeof *a);
    a->type = type;
    a->flags = dt_u32();
    a->skeleton = 0;
    dt_rf3(&a->pos);
    dt_rf3(&a->scale);
    dt_rf3((ZVec3 *)a->extent);
    a->radius = dt_rf();
    a->player = dt_rand_player();
    return a;
}

/* ---- synthetic motions -------------------------------------------------------------------------- */

typedef struct MotionGen {
    int wide;      /* 16 bit joint angles */
    int extra;     /* 0x64 byte block before the keys */
    int maxJoints; /* joints of the root bone entry */
    int safeSkim;  /* every root bone keyed completely at t = 0 (no stale staging data in the skim) */
    uint32_t step;
    int keys; /* number of key records */
} MotionGen;

static inline uint8_t *dt_align4(const uint8_t *start, uint8_t *p)
{
    while (((uintptr_t)(p - start)) & 3) {
        *p++ = (uint8_t)dt_u32();
    }
    return p;
}

static inline uint8_t *dt_put16(uint8_t *p, int v)
{
    *(int16_t *)p = (int16_t)v;
    return p + 2;
}

static inline int dt_rand_delta(void)
{
    return (dt_u32() & 3) ? dt_range(-3000, 3000) : (int)(int16_t)dt_u32();
}

/* Joint data: three 16 bit or 8 bit values per joint with the bit set in `mask` (bit j of mask, all if mask <
 * 0). */
static inline uint8_t *dt_put_joints(uint8_t *p, int wide, int joints, int64_t mask)
{
    for (int j = 0; j < joints; j++) {
        if (mask >= 0 && !((mask >> j) & 1)) {
            continue;
        }
        for (int k = 0; k < 3; k++) {
            if (wide) {
                p = dt_put16(p, dt_rand_delta());
            } else {
                *p++ = (uint8_t)dt_u32();
            }
        }
    }
    return p;
}

/* Builds a motion: header, bone table, initial pose records and a key stream, all consistent with each other.
 */
static inline ZMotion *dt_gen_motion(const MotionGen *g)
{
    uint8_t *buf = dt_alloc(0x4000);
    ZMotion *m = (ZMotion *)buf;
    const int nb = dt_range(1, 3);
    m->frames = (uint16_t)(dt_range(1, 12) | (g->extra ? 0x8000 : 0));
    m->boneByte = (uint8_t)((nb << 4) | dt_range(0, 15));
    m->format = (uint8_t)((g->wide ? 0x80 : 0) | (dt_u32() & 0x7f));
    m->step = g->step;
    m->markerOffset = dt_range(0, 0x100);
    int joints[4];
    for (int k = 0; k < nb; k++) {
        joints[k] = k == 0 ? g->maxJoints : dt_range(1, g->safeSkim ? g->maxJoints : 9);
        m->bones[k].joints = (uint8_t)joints[k];
        m->bones[k].flags = (uint8_t)((k > 0 && dt_u32() % 6 == 0) ? 2 : 0);
        m->bones[k].kind = (uint16_t)(k == 0 ? 0 : (dt_u32() % 3 == 0 ? 0 : dt_range(1, 3)));
    }
    uint8_t *p = (uint8_t *)&m->bones[nb];
    if (g->extra) {
        dt_fill(p, 0x64);
        p += 0x64;
    }
    /* initial pose */
    for (int k = 0; k < nb; k++) {
        uint8_t *rec = p;
        if (m->bones[k].kind == 0) {
            dt_fill(p, 0x12);
            *(float *)(p + 8) = dt_rf();
            p += 0x12;
            p = dt_put_joints(p, g->wide, joints[k], -1);
            p = dt_align4(rec, p);
        } else if ((m->bones[k].flags & 2) == 0) {
            const uint32_t hdr = (uint32_t)dt_range(0, g->safeSkim ? 3 : 7);
            *(uint32_t *)p = hdr;
            p += 6;
            if (hdr & 1) {
                *(float *)(rec + 8) = dt_rf();
                p = rec + 12;
            }
            if (hdr & 2) {
                for (int i = 0; i < 3; i++) {
                    p = dt_put16(p, dt_rand_delta());
                }
            }
            if (hdr & 4) {
                p = dt_put_joints(p, g->wide, joints[k], -1);
            }
            p = dt_align4(rec, p);
        }
    }
    /* key stream */
    uint32_t t = 0;
    int r = 0;
    int forced = 0;
    for (;;) {
        int bone;
        int64_t mask;
        uint32_t hdr;
        if (g->safeSkim && forced < nb) {
            bone = forced++;
            if (m->bones[bone].kind != 0) {
                continue;
            }
            mask = -1;
            t = 0;
        } else {
            if (r >= g->keys) {
                break;
            }
            r++;
            bone = dt_range(0, nb - 1);
            mask = (int64_t)(dt_u32() & ((1u << joints[bone]) - 1));
            if (g->safeSkim && m->bones[bone].kind == 0) {
                mask = ((int64_t)1 << joints[bone]) - 1;
            }
            t += (uint32_t)dt_range(0, 2 * (int)g->step);
        }
        if (mask < 0) {
            mask = ((int64_t)1 << joints[bone]) - 1;
        }
        if (g->safeSkim && m->bones[bone].kind != 0) {
            mask = 0; /* the skim does not step over joint data of secondary bones */
        }
        hdr = ((uint32_t)bone << 28) | ((uint32_t)mask << 2) | (dt_u32() & 3);
        if (hdr == 0) {
            hdr = 2; /* all zero would be the terminator */
        }
        uint8_t *rec = p;
        *(uint32_t *)p = hdr;
        *(uint16_t *)(p + 4) = (uint16_t)t;
        p += 6;
        if (hdr & 1) {
            *(uint16_t *)(rec + 6) = (uint16_t)dt_u32();
            *(float *)(rec + 8) = dt_rf();
            p = rec + 12;
        }
        if (hdr & 2) {
            for (int i = 0; i < 3; i++) {
                p = dt_put16(p, dt_rand_delta());
            }
        }
        p = dt_put_joints(p, g->wide, joints[bone], mask);
        p = dt_align4(rec, p);
    }
    *(uint32_t *)p = 0;
    *(uint16_t *)(p + 4) = 0xffff;
    return m;
}

#endif
