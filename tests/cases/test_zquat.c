#include <stddef.h>

#include "dt.h"

#include "zge/zquat.h"

static ZQuat *rand_quat(float range)
{
    ZQuat *q = dt_alloc(sizeof *q);
    q->x = dt_float_edgy(-range, range);
    q->y = dt_float_edgy(-range, range);
    q->z = dt_float_edgy(-range, range);
    q->w = dt_float_edgy(-range, range);
    return q;
}

static ZQuat *rand_unit_quat(void)
{
    ZQuat *q;
    float len;
    do {
        q = rand_quat(1.0f);
        len = q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w;
    } while (len < 0.01f);
    return q;
}

static ZMatrix *rand_rotation(void)
{
    ZMatrix *m = dt_alloc(sizeof *m);
    zquatToMatrix(m, rand_unit_quat());
    return m;
}

static void setup_identity(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(ZQuat)));
}

static void setup_mul(DtCtx *c)
{
    ZQuat *a, *b;
    dt_seed(c);
    a = rand_quat(2.0f);
    b = rand_quat(2.0f);
    switch (c->seed % 4) {
    case 0: /* out aliases a */
        DT_ARG_PTR(c, 0, a);
        break;
    case 1: /* out aliases b */
        DT_ARG_PTR(c, 0, b);
        break;
    default:
        DT_ARG_PTR(c, 0, dt_alloc(sizeof(ZQuat)));
    }
    DT_ARG_PTR(c, 1, a);
    DT_ARG_PTR(c, 2, b);
}

static void setup_vec_quat(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(ZVec3)));
    DT_ARG_PTR(c, 1, c->seed % 5 ? rand_unit_quat() : rand_quat(1e-6f));
}

static void setup_scaled_axis(DtCtx *c)
{
    ZQuat *q;
    dt_seed(c);
    q = rand_unit_quat();
    zquatNormalize(q);
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(ZVec3)));
    DT_ARG_PTR(c, 1, q);
}

static void setup_from_euler(DtCtx *c)
{
    ZAngle3 *e;
    dt_seed(c);
    e = dt_alloc(sizeof *e);
    e->x = (ZAngle)dt_u32();
    e->y = (ZAngle)dt_u32();
    e->z = (ZAngle)dt_u32();
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(ZQuat)));
    DT_ARG_PTR(c, 1, e);
    DT_ARG_INT(c, 2, c->seed == 3 ? 1 : 0); /* one iteration hits the "Not coded" assert */
}

static void setup_to_euler(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(ZAngle3)));
    DT_ARG_PTR(c, 1, rand_unit_quat());
    DT_ARG_INT(c, 2, 0);
}

/* Stubbed zmatrixToEuler: records the matrix it was given and returns fixed angles. */
static ZMatrix stubMatrix;
static int stubOrder;
static void BMX_CDECL stub_matrix_to_euler(ZAngle3 *out, const ZMatrix *m, int order)
{
    stubMatrix = *m;
    stubOrder = order;
    out->x = 0x1234;
    out->y = -0x0567;
    out->z = 0x7ff0;
}

static void setup_to_euler_stubbed(DtCtx *c)
{
    setup_to_euler(c);
    dt_stub(0x00481fc0, NULL, (void *)stub_matrix_to_euler);
}

static void setup_to_matrix(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(ZMatrix)));
    DT_ARG_PTR(c, 1, c->seed == 2 ? rand_quat(1e-7f) : rand_quat(3.0f)); /* seed 2: assert path */
}

static void setup_from_matrix(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(ZQuat)));
    DT_ARG_PTR(c, 1, rand_rotation());
}

static void setup_normalize(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, c->seed == 1 ? rand_quat(1e-7f) : rand_quat(4.0f));
}

DT_CASES(zquat_cases) = {
    {"zquatIdentity", 0x00483020, zquatIdentity, 1, DT_RET_VOID, setup_identity, 1, 0},
    {"zquatMul", 0x00483070, zquatMul, 3, DT_RET_VOID | DT_FLOAT_MEM, setup_mul, 200, 1},
    {"zquatToAxisAngle", 0x00483170, zquatToAxisAngle, 2, DT_RET_INT16 | DT_FLOAT_MEM, setup_vec_quat, 200,
     1},
    {"zquatToScaledAxis", 0x00483210, zquatToScaledAxis, 2, DT_RET_VOID | DT_FLOAT_MEM, setup_scaled_axis,
     200, 1},
    {"zquatFromEuler", 0x004832b0, zquatFromEuler, 3, DT_RET_VOID | DT_FLOAT_MEM, setup_from_euler, 200, 1},
    {"zquatToEuler", 0x00483420, zquatToEuler, 3, DT_RET_VOID | DT_FLOAT_MEM, setup_to_euler, 200, 1},
    {"zquatToEuler_stubbed", 0x00483420, zquatToEuler, 3, DT_RET_VOID, setup_to_euler_stubbed, 50, 0},
    {"zquatToMatrix", 0x00483470, zquatToMatrix, 2, DT_RET_VOID | DT_FLOAT_MEM, setup_to_matrix, 200, 1},
    {"zquatFromMatrix", 0x00483610, zquatFromMatrix, 2, DT_RET_INT | DT_FLOAT_MEM, setup_from_matrix, 200, 1},
    {"zquatNormalize", 0x00483670, zquatNormalize, 1, DT_RET_FLOAT | DT_FLOAT_MEM, setup_normalize, 200, 1},
};
