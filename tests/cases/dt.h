/*
 * Differential test cases, executed by tests/oracle/oracle.py.
 *
 * A case names an original function (by address) and our port of it, plus a setup() that builds the
 * arguments in DtCtx from a per-iteration seed. The harness runs both from identical memory and
 * compares return value + every writable byte. Setup code may allocate (malloc goes to the exe CRT,
 * which the harness intercepts) and may poke ORIG_VAR globals.
 *
 * The baseline is a real process state: the exe's own CRT startup has run (heap, stdio, locale) up to
 * WinMain. File APIs see the original install read-only (paths relative to the install root, case-
 * insensitive); writes stay in memory and are compared between both sides like memory.
 */
#ifndef DT_H
#define DT_H

#include <stdint.h>

enum {
    DT_RET_VOID = 0,
    DT_RET_INT = 1,
    DT_RET_FLOAT = 2,
    DT_RET_DOUBLE = 3,
    DT_RET_INT64 = 4,
    DT_RET_INT16 = 5,
    DT_RET_INT8 = 6,
    DT_FLOAT_MEM = 0x10,    /* tolerate `ulps` difference in float-typed memory */
    DT_NEEDS_ASSETS = 0x40, /* reads original game files; skipped when no install is available */
};

typedef struct DtCtx {
    uint32_t seed;
    uint32_t args[8];
} DtCtx;

typedef struct DtCase {
    const char *name;
    uint32_t orig;
    void *impl;
    uint32_t argc;
    uint32_t flags;
    void (*setup)(DtCtx *ctx);
    uint32_t iters;
    uint32_t ulps;
} DtCase;

/*
 * Each test file registers its cases with DT_CASES; the linker gathers all of them between the
 * .dtcase$a and .dtcase$z markers in dt.c (MSVC grouped-section ordering).
 */
#pragma section(".dtcase$m", read)
#define DT_CASES(name) __declspec(allocate(".dtcase$m")) __attribute__((used)) static const DtCase name[]

#define DT_ARG_PTR(ctx, i, p) ((ctx)->args[i] = (uint32_t)(uintptr_t)(p))
#define DT_ARG_INT(ctx, i, v) ((ctx)->args[i] = (uint32_t)(v))
#define DT_ARG_FLOAT(ctx, i, v)                                                                              \
    do {                                                                                                     \
        float dt_f_ = (v);                                                                                   \
        uint32_t dt_u_;                                                                                      \
        __builtin_memcpy(&dt_u_, &dt_f_, 4);                                                                 \
        (ctx)->args[i] = dt_u_;                                                                              \
    } while (0)

/* Deterministic per-iteration randomness. The first iterations yield edge values. */
void dt_seed(const DtCtx *ctx);
uint32_t dt_u32(void);
int32_t dt_range(int32_t lo, int32_t hi);
float dt_float(float lo, float hi);
/* Random float drawn from edge values (0, -0, +-1, tiny, huge) on early iterations. */
float dt_float_edgy(float lo, float hi);
void *dt_alloc(uint32_t size);
/*
 * Canonicalises a value for stub logs: a pointer into the emulated stack becomes DT_STACK_PTR, because
 * the original and our port lay out their frames differently (log the pointed-to data instead if it
 * matters). Never log uninitialised bytes (struct padding, unset locals) either.
 */
#define DT_STACK_PTR 0x57ac0000u
uint32_t dt_canon(uint32_t v);

/* Fills n bytes with deterministic random data. Served by the harness (fast), not emulated. */
void dt_fill(void *p, uint32_t n);

/*
 * Replaces a callee for the current iteration, on both sides: the harness patches `jmp stub` over the
 * original entry `orig` and over our port `ours` (NULL if not ported yet), then restores both after the
 * run. Use it for callees that need a loaded level, a renderer or other state the emulator lacks. The
 * stub must have the callee's signature. Anything it records into DLL globals is compared like any other
 * memory, so log arguments by value and return deterministic results (e.g. from dt_u32()).
 */
void dt_stub(uint32_t orig, void *ours, void *stub);

#endif
