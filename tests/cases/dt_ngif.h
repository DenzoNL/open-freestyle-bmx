/* Shared helpers for the NgIF tests (test_ngifzbsp.c, test_ngifzcollide.c, test_ngifzroller.c). */
#ifndef DT_NGIF_H
#define DT_NGIF_H

#include <stdarg.h>
#include <stdint.h>

#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_zmem.h"
#include "zge/zdebug.h"
#include "zge/zobj.h"

/* ---- call log ------------------------------------------------------------------------------------ */

/*
 * Stubs fold what they see into a log kept in file-static memory, which the harness compares between the
 * original and the port. Words are never taken from uninitialised bytes; pointers go through dt_canon().
 */
enum { kNgLogWords = 4096 };
static uint32_t gNgLog[kNgLogWords];
static uint32_t gNgLogN;

static inline void ngLog(uint32_t v)
{
    if (gNgLogN < kNgLogWords) {
        gNgLog[gNgLogN] = v;
    }
    gNgLogN++;
}

static inline void ngLogPtr(const void *p)
{
    ngLog(dt_canon((uint32_t)(uintptr_t)p));
}

static inline void ngLogFloat(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    ngLog(u);
}

static inline void ngLogVec(const ZVec3 *v)
{
    ngLogFloat(v->x);
    ngLogFloat(v->y);
    ngLogFloat(v->z);
}

static inline uint32_t ngHash(const char *s)
{
    uint32_t h = 5381;
    while (*s) {
        h = h * 33 + (uint8_t)*s++;
    }
    return h;
}

/* Replacement for the engine's printf-style debug print: logs the format and each %s / %d argument. */
static void BMX_CDECL ngPrint(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    ngLog(0x480240);
    ngLog(ngHash(fmt));
    for (const char *p = fmt; *p; p++) {
        if (p[0] == '%' && p[1] == 's') {
            ngLog(ngHash(va_arg(ap, const char *)));
        } else if (p[0] == '%' && p[1] == 'd') {
            ngLog((uint32_t)va_arg(ap, int));
        }
    }
    va_end(ap);
}

/* Replacement for zdebugFail: records the message and returns, so the code after the assert runs. */
static void BMX_CDECL ngFail(const char *why, const char *file, int line, int flags)
{
    ngLog(0x47ed10);
    ngLog(ngHash(why));
    ngLog(ngHash(file));
    ngLog((uint32_t)line);
    ngLog((uint32_t)flags);
}

/* Fresh log, debug print and (optionally) assert handler of the test. */
static inline void ngBegin(const DtCtx *c, int stubFail)
{
    dt_seed(c);
    dt_zmem_env();
    gNgLogN = 0;
    gZdebugPrint = ngPrint;
    if (stubFail) {
        dt_stub(0x0047ed10, NULL, (void *)ngFail);
    }
}

/* Float with edge values and a modest range, never NaN or huge. */
static inline float ngRf(void)
{
    return dt_float_edgy(-1000.0f, 1000.0f);
}

static inline void ngRf3(ZVec3 *v)
{
    v->x = ngRf();
    v->y = ngRf();
    v->z = ngRf();
}

/*
 * Tree object list of `count` nodes (installed as the list at 0x6722b8): a mix of objects without a
 * definition, with a definition and no shape yet (built on first use) and with a ready shape of kind 0..4.
 */
static inline ZbspTreeNode *ngMakeTree(int count)
{
    ZbspTreeNode *head = NULL;

    for (int i = count; i > 0; i--) {
        ZbspTreeNode *n = dt_alloc(sizeof *n);
        const uint32_t mode = dt_u32() % 4;

        n->next = head;
        n->object.size[0] = dt_float_edgy(0.0f, 20.0f);
        n->object.size[1] = dt_float_edgy(0.0f, 20.0f);
        n->object.size[2] = dt_float_edgy(0.0f, 20.0f);
        n->object.lift = dt_float_edgy(-5.0f, 5.0f);
        n->object.base = dt_float_edgy(-5.0f, 5.0f);
        if (mode == 1 || mode == 2) {
            ZbspObjectDef *def = dt_alloc(sizeof *def);

            def->flags = (uint8_t)dt_u32();
            def->kind = (uint8_t)dt_range(0, 5);
            n->object.def = def;
        }
        if (mode >= 2) {
            ZbspShape *shape = dt_alloc(sizeof *shape);

            shape->kind = (uint8_t)dt_range(0, 4);
            n->object.shape = shape;
        }
        head = n;
    }
    *zobjTreeObjectList() = head;
    return head;
}

/* Setups of the debug-print stub cases: no arguments, one, two, and (pointer, optional int out). */
static inline void ngSetupTrace(DtCtx *c)
{
    ngBegin(c, 0);
}

static inline void ngSetupTrace1(DtCtx *c)
{
    ngBegin(c, 0);
    DT_ARG_INT(c, 0, dt_u32());
}

static inline void ngSetupTrace2(DtCtx *c)
{
    ngBegin(c, 0);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
}

static inline void ngSetupSceneState(DtCtx *c)
{
    int *out = dt_alloc(4);

    ngBegin(c, 0);
    *out = (int)dt_u32();
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_PTR(c, 1, (c->seed & 1) ? out : NULL);
}

#endif
