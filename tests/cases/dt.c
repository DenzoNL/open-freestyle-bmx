#include "dt.h"

#include "compat/crt.h"

/* Exported to the harness: the argument block, the case table bounds and the init list. */
__declspec(dllexport) DtCtx dt_ctx;

#pragma section(".dtcase$a", read)
#pragma section(".dtcase$z", read)
__declspec(allocate(".dtcase$a")) __attribute__((used)) static const DtCase dt_begin_marker = {0};
__declspec(allocate(".dtcase$z")) __attribute__((used)) static const DtCase dt_end_marker = {0};
__declspec(dllexport) const DtCase *const dt_cases_begin = &dt_begin_marker + 1;
__declspec(dllexport) const DtCase *const dt_cases_end = &dt_end_marker;

/* Original initialisers the harness runs once before taking its baseline snapshot. */
__declspec(dllexport) const uint32_t dt_init[] = {
    0x00480d20, /* zmathInitTables: sin/cos/acos/atan lookup tables */
    0,
};

typedef struct DtStub {
    uint32_t orig;
    void *ours;
    void *stub;
} DtStub;

#define DT_MAX_STUBS 128
__declspec(dllexport) DtStub dt_stubs[DT_MAX_STUBS];
__declspec(dllexport) uint32_t dt_stub_count;

void dt_stub(uint32_t orig, void *ours, void *stub)
{
    if (dt_stub_count < DT_MAX_STUBS) {
        dt_stubs[dt_stub_count].orig = orig;
        dt_stubs[dt_stub_count].ours = ours;
        dt_stubs[dt_stub_count].stub = stub;
    }
    dt_stub_count++; /* the harness rejects overflow */
}

static uint32_t dt_state;
static uint32_t dt_iter;

uint32_t dt_canon(uint32_t v)
{
    /* emulated stack: tests/oracle/oracle.py STACK_BASE / STACK_SIZE */
    return (v >= 0x00100000u && v < 0x00200000u) ? DT_STACK_PTR : v;
}

void dt_seed(const DtCtx *ctx)
{
    dt_iter = ctx->seed;
    dt_state = ctx->seed * 2654435761u + 0x9e3779b9u;
}

uint32_t dt_u32(void)
{
    /* xorshift32 */
    uint32_t x = dt_state ? dt_state : 1;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    dt_state = x;
    return x;
}

int32_t dt_range(int32_t lo, int32_t hi)
{
    return lo + (int32_t)(dt_u32() % (uint32_t)(hi - lo + 1));
}

float dt_float(float lo, float hi)
{
    return lo + (hi - lo) * ((float)(dt_u32() >> 8) / 16777216.0f);
}

float dt_float_edgy(float lo, float hi)
{
    static const float edges[] = {0.0f, -0.0f, 1.0f, -1.0f, 1e-6f, -1e-6f, 0.5f};
    if (dt_iter < sizeof edges / sizeof edges[0] && (dt_u32() & 1)) {
        return edges[dt_iter];
    }
    return dt_float(lo, hi);
}

/*
 * Routines the harness may run natively instead of emulating (identical semantics, much faster):
 * memset, memcpy, memmove of compat/crt.c. Order is fixed; keep in sync with tests/oracle/oracle.py.
 */
__declspec(dllexport) void *const dt_accel[] = {(void *)memset, (void *)memcpy, (void *)memmove, 0};

/* Harness hypercall (tests/oracle/oracle.py HYPER_FILL): random bytes keyed to the iteration. */
#define DT_HYPER_FILL 0x7ff0ffe0u

void dt_fill(void *p, uint32_t n)
{
    ((void(__cdecl *)(void *, uint32_t))DT_HYPER_FILL)(p, n);
}

void *dt_alloc(uint32_t size)
{
    void *p = malloc(size);
    memset(p, 0, size);
    return p;
}
