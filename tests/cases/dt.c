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

static uint32_t dt_state;
static uint32_t dt_iter;

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

void *dt_alloc(uint32_t size)
{
    void *p = malloc(size);
    memset(p, 0, size);
    return p;
}
