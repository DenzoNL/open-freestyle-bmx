/*
 * Tests for the triangle list reordering of rw/clump.c (rpMeshSeparateTriangles) and its zeroing helper
 * (EAX, ECX register arguments, through a wrapper). The lists never contain a triangle of three 0xffff
 * indices (the original would not end); allocations of the engine can fail.
 */
#include "dt_rw.h"
#include "rw/rwclump.h"

/* ---- rwZeroBytes (EAX = buffer, ECX = size, EDX unused) --------------------------------------------- */

typedef void *(__attribute__((regparm(3))) * ZeroFn)(void *, int32_t, uint32_t);

static void *zero_orig(void *buffer, uint32_t size)
{
    return ((ZeroFn)(uintptr_t)0x004c5e10)(buffer, 0, size);
}

static void *zero_impl(void *buffer, uint32_t size)
{
    return rwZeroBytes(buffer, 0, size);
}

static void setup_zero(DtCtx *c)
{
    dt_seed(c);
    const uint32_t size = (uint32_t)dt_range(0, 70);
    uint8_t *buffer = dt_alloc(80);
    dt_fill(buffer, 80);
    DT_ARG_PTR(c, 0, buffer + dt_range(0, 4));
    DT_ARG_INT(c, 1, size);
}

/* ---- the reordering ----------------------------------------------------------------------------------- */

/* 0..9 triangles over a few vertices (so that triangles share) or over a wide range of indices. */
static void setup_reorder(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    gRwtMallocFailAt = dt_u32() % 6 == 0 ? dt_range(1, 2) : 0;
    const uint32_t triangles = (uint32_t)dt_range(0, 9);
    const uint32_t extra =
        dt_u32() % 5 == 0 ? (uint32_t)dt_range(1, 2) : 0; /* a count that is not a multiple of 3 */
    const uint32_t count = triangles * 3 + extra;
    const int wide = dt_u32() % 4 == 0;
    uint16_t *indices = dt_alloc(count * 2 + 16);
    for (uint32_t i = 0; i < count; i++) {
        indices[i] = (uint16_t)(wide ? dt_range(0, 0x7ffe) : dt_range(0, dt_u32() % 2 ? 4 : 40));
    }
    RwIndexList *list = dt_alloc(sizeof *list);
    list->indices = indices;
    list->count = count;
    DT_ARG_PTR(c, 0, list);
    rwtEnd();
}

DT_CASES(rwindexorder_cases) = {
    {"rwZeroBytes", (uint32_t)(uintptr_t)zero_orig, zero_impl, 2, DT_RET_INT, setup_zero, 200, 0},
    {"rpMeshSeparateTriangles", 0x004c5bd0, rpMeshSeparateTriangles, 1, DT_RET_INT, setup_reorder, 1500, 0},
};
