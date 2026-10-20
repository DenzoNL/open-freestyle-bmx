#include <stddef.h>

#include "dt.h"

#include "compat/crt.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmath.h"
#include "zge/zobj.h"

static ZVec3 *rand_vec(float range)
{
    ZVec3 *v = dt_alloc(sizeof *v);
    v->x = dt_float_edgy(-range, range);
    v->y = dt_float_edgy(-range, range);
    v->z = dt_float_edgy(-range, range);
    return v;
}

/* Every word random, including the flag and padding words the helpers must leave alone. */
static ZMatrix *rand_matrix(float range)
{
    ZMatrix *m = dt_alloc(sizeof *m);
    float *f = (float *)m;
    int i;
    for (i = 0; i < 16; i++) {
        f[i] = dt_float_edgy(-range, range);
    }
    m->flags = dt_u32();
    m->pad1 = dt_u32();
    m->pad2 = dt_u32();
    m->pad3 = dt_u32();
    return m;
}

static uint32_t rand_angle(void)
{
    return dt_u32(); /* garbage in the upper half: only the low 16 bits may matter */
}

static void clear_tables(void)
{
    uint32_t i;
    uint8_t *p = (uint8_t *)&gZmathTables;
    /* Everything but the random seed: the port has to rebuild every table entry and pointer. */
    for (i = 0; i < offsetof(ZMathTables, randomSeed); i++) {
        p[i] = 0;
    }
}

static void setup_init_tables(DtCtx *c)
{
    dt_seed(c);
    clear_tables();
}

static void setup_angle(DtCtx *c)
{
    dt_seed(c);
    switch (c->seed) {
    case 0:
        DT_ARG_INT(c, 0, 0x4000); /* cos is (almost) zero */
        break;
    case 1:
        DT_ARG_INT(c, 0, 0xc000);
        break;
    case 2:
        DT_ARG_INT(c, 0, 0xffff0000u | 0x4005);
        break;
    default:
        DT_ARG_INT(c, 0, rand_angle());
    }
}

static void setup_unit_float(DtCtx *c)
{
    dt_seed(c);
    switch (c->seed) {
    case 0:
        DT_ARG_FLOAT(c, 0, 1.0f);
        break;
    case 1:
        DT_ARG_FLOAT(c, 0, -1.0f);
        break;
    case 2:
        DT_ARG_FLOAT(c, 0, 0.999999f);
        break;
    case 3:
        DT_ARG_FLOAT(c, 0, -3.0f);
        break;
    default:
        DT_ARG_FLOAT(c, 0, dt_float_edgy(-1.3f, 1.3f));
    }
}

static void setup_atan2(DtCtx *c)
{
    float y, x;
    dt_seed(c);
    y = dt_float_edgy(-100.0f, 100.0f);
    x = dt_float_edgy(-100.0f, 100.0f);
    switch (c->seed % 8) {
    case 0:
        x = y;
        break;
    case 1:
        x = -y;
        break;
    case 2:
        y = 0.0f;
        break;
    case 3:
        x = 0.0f;
        break;
    case 4:
        y = x * dt_float(0.0f, 1.0f); /* shallow slope */
        break;
    case 5:
        x = y * dt_float(0.0f, 1.0f); /* steep slope */
        break;
    default:
        break;
    }
    if (c->seed == 7) {
        x = 0.0f; /* both zero: 0 / 0 */
        y = 0.0f;
    }
    DT_ARG_FLOAT(c, 0, y);
    DT_ARG_FLOAT(c, 1, x);
}

static void setup_random(DtCtx *c)
{
    dt_seed(c);
    gZmathTables.randomSeed = dt_u32();
}

static void setup_vec_in(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, c->seed == 0 ? dt_alloc(sizeof(ZVec3)) : rand_vec(50.0f));
}

/* out, a, b with out aliasing a or b now and then. */
static void setup_vec_binary(DtCtx *c)
{
    ZVec3 *a, *b;
    dt_seed(c);
    a = rand_vec(100.0f);
    b = rand_vec(100.0f);
    switch (c->seed % 4) {
    case 0:
        DT_ARG_PTR(c, 0, a);
        break;
    case 1:
        DT_ARG_PTR(c, 0, b);
        break;
    default:
        DT_ARG_PTR(c, 0, dt_alloc(sizeof(ZVec3)));
    }
    DT_ARG_PTR(c, 1, a);
    DT_ARG_PTR(c, 2, b);
}

static void setup_vec_unary(DtCtx *c)
{
    ZVec3 *a;
    dt_seed(c);
    a = rand_vec(100.0f);
    DT_ARG_PTR(c, 0, c->seed % 3 == 0 ? a : dt_alloc(sizeof(ZVec3)));
    DT_ARG_PTR(c, 1, a);
}

static void setup_vec_normalize(DtCtx *c)
{
    setup_vec_unary(c);
    if (c->seed == 5) {
        ZVec3 *z = (ZVec3 *)(uintptr_t)c->args[1];
        z->x = 0.0f; /* zero length */
        z->y = 0.0f;
        z->z = 0.0f;
    }
}

static void setup_vec_scalar(DtCtx *c)
{
    setup_vec_unary(c);
    DT_ARG_FLOAT(c, 2, dt_float_edgy(-10.0f, 10.0f));
}

static void setup_vec_scale_xyz(DtCtx *c)
{
    setup_vec_unary(c);
    DT_ARG_FLOAT(c, 2, dt_float_edgy(-10.0f, 10.0f));
    DT_ARG_FLOAT(c, 3, dt_float_edgy(-10.0f, 10.0f));
    DT_ARG_FLOAT(c, 4, dt_float_edgy(-10.0f, 10.0f));
}

static void setup_vec_rotate(DtCtx *c)
{
    setup_vec_unary(c);
    DT_ARG_INT(c, 2, rand_angle());
}

/* Quiet NaNs with random payload and sign in the vector: the FPU keeps the one with the larger payload. */
static void setup_vec_rotate_nan(DtCtx *c)
{
    uint32_t *w;

    setup_vec_rotate(c);
    w = (uint32_t *)(uintptr_t)c->args[1];
    for (int i = 0; i < 3; i++) {
        if (dt_u32() % 4 != 0) {
            w[i] = 0x7fc00000u | (dt_u32() & 0x3fffffu) | (dt_u32() & 0x80000000u);
        }
    }
}

static void setup_vec_rotate_euler(DtCtx *c)
{
    ZVec3 *v;
    ZAngle3 *a;
    dt_seed(c);
    v = rand_vec(100.0f);
    a = dt_alloc(sizeof *a);
    a->x = (ZAngle)dt_u32();
    a->y = (ZAngle)dt_u32();
    a->z = (ZAngle)dt_u32();
    /* The original reads x from `out`, so a separate `out` starts with its own data. */
    DT_ARG_PTR(c, 0, c->seed % 4 == 3 ? rand_vec(100.0f) : v);
    DT_ARG_PTR(c, 1, v);
    DT_ARG_PTR(c, 2, a);
}

static void setup_vec_dot(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, rand_vec(100.0f));
    DT_ARG_PTR(c, 1, rand_vec(100.0f));
}

static void setup_vec_cross3(DtCtx *c)
{
    ZVec3 *a, *b, *cc;
    dt_seed(c);
    a = rand_vec(100.0f);
    b = rand_vec(100.0f);
    cc = rand_vec(100.0f);
    switch (c->seed % 5) {
    case 0:
        DT_ARG_PTR(c, 0, a);
        break;
    case 1:
        DT_ARG_PTR(c, 0, b);
        break;
    case 2:
        DT_ARG_PTR(c, 0, cc);
        break;
    default:
        DT_ARG_PTR(c, 0, dt_alloc(sizeof(ZVec3)));
    }
    DT_ARG_PTR(c, 1, a);
    DT_ARG_PTR(c, 2, b);
    DT_ARG_PTR(c, 3, cc);
}

static void setup_vec_lerp(DtCtx *c)
{
    setup_vec_binary(c);
    DT_ARG_FLOAT(c, 3, dt_float_edgy(-2.0f, 2.0f));
}

static void setup_mat_out(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, rand_matrix(1.0f));
}

/* out, m, v */
static void setup_transform(DtCtx *c)
{
    ZVec3 *v;
    dt_seed(c);
    v = rand_vec(100.0f);
    DT_ARG_PTR(c, 0, c->seed % 3 == 0 ? v : dt_alloc(sizeof(ZVec3)));
    DT_ARG_PTR(c, 1, rand_matrix(10.0f));
    DT_ARG_PTR(c, 2, v);
}

/* out, in, <extra>: out aliases in now and then. */
static ZMatrix *setup_mat_pair(DtCtx *c)
{
    ZMatrix *in;
    dt_seed(c);
    in = rand_matrix(10.0f);
    DT_ARG_PTR(c, 0, c->seed % 3 == 0 ? in : rand_matrix(10.0f));
    DT_ARG_PTR(c, 1, in);
    return in;
}

static void setup_mat_pair_only(DtCtx *c)
{
    setup_mat_pair(c);
}

static void setup_mat_translate(DtCtx *c)
{
    setup_mat_pair(c);
    DT_ARG_PTR(c, 2, rand_vec(100.0f));
}

static void setup_mat_rotate(DtCtx *c)
{
    setup_mat_pair(c);
    DT_ARG_INT(c, 2, rand_angle());
}

static void setup_mat_euler(DtCtx *c)
{
    ZAngle3 *a;
    setup_mat_pair(c);
    a = dt_alloc(sizeof *a);
    a->x = (ZAngle)dt_u32();
    a->y = (ZAngle)dt_u32();
    a->z = (ZAngle)dt_u32();
    DT_ARG_PTR(c, 2, a);
}

static void setup_mat_to_euler(DtCtx *c)
{
    ZMatrix *m;
    dt_seed(c);
    m = rand_matrix(1.0f);
    switch (c->seed % 4) {
    case 0:
        m->right.x = 0.0f; /* gimbal lock branch */
        m->right.y = 0.0f;
        break;
    case 1:
        m->right.x = 1e-4f;
        m->right.y = -1e-4f;
        break;
    default:
        break;
    }
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(ZAngle3)));
    DT_ARG_PTR(c, 1, m);
    DT_ARG_INT(c, 2, 0);
}

static void setup_mat_mul(DtCtx *c)
{
    ZMatrix *a, *b;
    dt_seed(c);
    a = rand_matrix(10.0f);
    b = rand_matrix(10.0f);
    switch (c->seed % 4) {
    case 0:
        DT_ARG_PTR(c, 0, a);
        break;
    case 1:
        DT_ARG_PTR(c, 0, b);
        break;
    default:
        DT_ARG_PTR(c, 0, rand_matrix(10.0f));
    }
    DT_ARG_PTR(c, 1, a);
    DT_ARG_PTR(c, 2, b);
}

DT_CASES(zobjcamera_cases) = {
    {"zmathInitTables", 0x00480d20, zmathInitTables, 0, DT_RET_INT, setup_init_tables, 3, 0},
    {"zmathInitialize", 0x00481f50, zmathInitialize, 0, DT_RET_INT, setup_init_tables, 3, 0},
    {"zmathSin", 0x00480e40, zmathSin, 1, DT_RET_DOUBLE, setup_angle, 300, 0},
    {"zmathCos", 0x00480eb0, zmathCos, 1, DT_RET_DOUBLE, setup_angle, 300, 0},
    {"zmathAsin", 0x00480e60, zmathAsin, 1, DT_RET_INT16, setup_unit_float, 300, 0},
    {"zmathAcos", 0x00480ed0, zmathAcos, 1, DT_RET_INT16, setup_unit_float, 300, 0},
    {"zmathTan", 0x00480f30, zmathTan, 1, DT_RET_DOUBLE, setup_angle, 300, 0},
    {"zmathAtan2", 0x00480ff0, zmathAtan2, 2, DT_RET_INT16, setup_atan2, 600, 0},
    {"zmathRandom", 0x00481f80, zmathRandom, 0, DT_RET_INT, setup_random, 50, 0},
    {"zvec3Zero", 0x00481120, zvec3Zero, 1, DT_RET_VOID, setup_vec_in, 4, 0},
    {"zvec3Add", 0x00481130, zvec3Add, 3, DT_RET_VOID, setup_vec_binary, 300, 0},
    {"zvec3Sub", 0x00481160, zvec3Sub, 3, DT_RET_VOID, setup_vec_binary, 300, 0},
    {"zvec3NormalizeScaled", 0x004811c0, zvec3NormalizeScaled, 2, DT_RET_DOUBLE, setup_vec_normalize, 300, 0},
    {"zvec3Scale", 0x00481210, zvec3Scale, 3, DT_RET_VOID, setup_vec_scalar, 300, 0},
    {"zvec3ScaleXYZ", 0x00481240, zvec3ScaleXYZ, 5, DT_RET_VOID, setup_vec_scale_xyz, 300, 0},
    {"zvec3LengthSq", 0x004812c0, zvec3LengthSq, 1, DT_RET_DOUBLE, setup_vec_in, 300, 0},
    {"zvec3Negate", 0x004812f0, zvec3Negate, 2, DT_RET_VOID, setup_vec_unary, 300, 0},
    {"zvec3Length", 0x00481310, zvec3Length, 1, DT_RET_DOUBLE, setup_vec_in, 300, 0},
    {"zvec3RotateX", 0x00481340, zvec3RotateX, 3, DT_RET_VOID, setup_vec_rotate, 300, 0},
    {"zvec3RotateY", 0x00481390, zvec3RotateY, 3, DT_RET_VOID, setup_vec_rotate, 300, 0},
    {"zvec3RotateZ", 0x004813e0, zvec3RotateZ, 3, DT_RET_VOID, setup_vec_rotate, 300, 0},
    {"zvec3RotateX_nan", 0x00481340, zvec3RotateX, 3, DT_RET_VOID, setup_vec_rotate_nan, 300, 0},
    {"zvec3RotateY_nan", 0x00481390, zvec3RotateY, 3, DT_RET_VOID, setup_vec_rotate_nan, 300, 0},
    {"zvec3RotateZ_nan", 0x004813e0, zvec3RotateZ, 3, DT_RET_VOID, setup_vec_rotate_nan, 300, 0},
    {"zvec3RotateEuler", 0x00481430, zvec3RotateEuler, 3, DT_RET_VOID, setup_vec_rotate_euler, 500, 0},
    {"zvec3Dot", 0x00481540, zvec3Dot, 2, DT_RET_DOUBLE, setup_vec_dot, 300, 0},
    {"zvec3Cross", 0x00481560, zvec3Cross, 3, DT_RET_VOID, setup_vec_binary, 300, 0},
    {"zvec3Cross3", 0x004815b0, zvec3Cross3, 4, DT_RET_VOID, setup_vec_cross3, 400, 0},
    {"zmatrixTransformPoint", 0x00481640, zmatrixTransformPoint, 3, DT_RET_VOID, setup_transform, 300, 0},
    {"zmatrixTransformVector", 0x00481780, zmatrixTransformVector, 3, DT_RET_VOID, setup_transform, 300, 0},
    {"zvec3Lerp", 0x004817e0, zvec3Lerp, 4, DT_RET_VOID, setup_vec_lerp, 300, 0},
    {"zvec3AddScalar", 0x00481820, zvec3AddScalar, 3, DT_RET_VOID, setup_vec_scalar, 300, 0},
    {"zvec3SubScalar", 0x00481850, zvec3SubScalar, 3, DT_RET_VOID, setup_vec_scalar, 300, 0},
    {"zmatrixIdentity", 0x00481880, zmatrixIdentity, 1, DT_RET_VOID, setup_mat_out, 4, 0},
    {"zmatrixZeroPos", 0x004818b0, zmatrixZeroPos, 1, DT_RET_VOID, setup_mat_out, 4, 0},
    {"zmatrixTranslate", 0x004818f0, zmatrixTranslate, 3, DT_RET_INT, setup_mat_translate, 300, 0},
    {"zmatrixRotateX", 0x00481930, zmatrixRotateX, 3, DT_RET_INT, setup_mat_rotate, 300, 0},
    {"zmatrixRotateY", 0x00481a00, zmatrixRotateY, 3, DT_RET_INT, setup_mat_rotate, 300, 0},
    {"zmatrixRotateZ", 0x00481ad0, zmatrixRotateZ, 3, DT_RET_INT, setup_mat_rotate, 300, 0},
    {"zmatrixRotateEuler", 0x00481ba0, zmatrixRotateEuler, 3, DT_RET_INT, setup_mat_euler, 300, 0},
    {"zmatrixScaleDiagonal", 0x00481be0, zmatrixScaleDiagonal, 3, DT_RET_INT, setup_mat_translate, 300, 0},
    {"zmatrixCopyRotation", 0x00481c60, zmatrixCopyRotation, 2, DT_RET_INT, setup_mat_pair_only, 100, 0},
    {"zmatrixCopyBasis", 0x00481cb0, zmatrixCopyBasis, 2, DT_RET_VOID, setup_mat_pair_only, 100, 0},
    {"zmatrixMul", 0x00481cf0, zmatrixMul, 3, DT_RET_VOID, setup_mat_mul, 400, 0},
    {"zmatrixTranspose", 0x00481e90, zmatrixTranspose, 2, DT_RET_VOID, setup_mat_pair_only, 200, 0},
    {"zmatrixInvertRigid", 0x00481f00, zmatrixInvertRigid, 2, DT_RET_INT, setup_mat_pair_only, 300, 0},
    {"zmatrixToEuler", 0x00481fc0, zmatrixToEuler, 3, DT_RET_VOID, setup_mat_to_euler, 400, 0},
};

/* ---- camera objects ------------------------------------------------------------------------------ */

/* The renderer and object-table callees are stubbed: they log what they are given. */
static uint32_t gLog[512];
static uint32_t gLogN;
static uint8_t gObjSlot[0x3c];
static int gInitResult, gCreateResult, gAllocResult;

static void logU(uint32_t v)
{
    if (gLogN < sizeof gLog / sizeof gLog[0]) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static void logWords(const void *p, uint32_t n)
{
    const uint32_t *w = p;
    uint32_t i;
    for (i = 0; i < n; i++) {
        logU(w[i]);
    }
}

static int BMX_CDECL stub_cam_init(void)
{
    logU(0x1000);
    return gInitResult;
}

static void *BMX_CDECL stub_cam_create(int flags, void *owner)
{
    logU(0x1001);
    logU((uint32_t)flags);
    logU(dt_canon((uint32_t)(uintptr_t)owner));
    return gCreateResult ? (void *)(uintptr_t)0x12345678 : NULL;
}

static int BMX_CDECL stub_cam_destroy(void *camera)
{
    logU(0x1002);
    logU(dt_canon((uint32_t)(uintptr_t)camera));
    return 1;
}

static void BMX_CDECL stub_cam_set_fov(void *camera, int fov)
{
    logU(0x1003);
    logU(dt_canon((uint32_t)(uintptr_t)camera));
    logU((uint32_t)fov);
}

static void BMX_CDECL stub_cam_set_view(void *camera, const ZMatrix *view, const ZVec3 *eye)
{
    logU(0x1004);
    logU(dt_canon((uint32_t)(uintptr_t)camera));
    logWords(view, 16);
    logWords(eye, 3);
}

static void *BMX_CDECL stub_obj_alloc(void)
{
    logU(0x1005);
    return gAllocResult ? gObjSlot : NULL;
}

static void *BMX_CDECL stub_obj_init(void *object, uint32_t type)
{
    logU(0x1006);
    logU((uint32_t)(uintptr_t)object);
    logU(type);
    return object;
}

static int BMX_CDECL stub_obj_free(void *object)
{
    logU(0x1007);
    logU((uint32_t)(uintptr_t)object);
    return 1;
}

static void BMX_CDECL stub_obj_set_position(void *object, const ZVec3 *pos)
{
    logU(0x1008);
    logU((uint32_t)(uintptr_t)object);
    logWords(pos, 3);
}

static void BMX_CDECL stub_obj_set_rotation(void *object, const ZQuat *rot)
{
    logU(0x1009);
    logU((uint32_t)(uintptr_t)object);
    logWords(rot, 4);
}

/* Look-at matrix builder: writes a random rotation and logs its arguments. */
static ZMatrix *BMX_CDECL stub_look_at(ZMatrix *out, const ZVec3 *eye, const ZVec3 *target)
{
    ZMatrix *m;
    float *f = (float *)out;
    int i;
    logU(0x100a);
    logWords(eye, 3);
    logWords(target, 3);
    for (i = 0; i < 12; i++) {
        f[i] = dt_float(-1.0f, 1.0f);
    }
    m = out;
    m->flags = 0;
    m->pad1 = 0;
    m->pad2 = 0;
    m->pad3 = 0;
    return out;
}

static void stubs_on(void)
{
    gLogN = 0;
    dt_stub(0x00492340, NULL, (void *)stub_cam_init);
    dt_stub(0x004925f0, NULL, (void *)stub_cam_create);
    dt_stub(0x00492970, NULL, (void *)stub_cam_destroy);
    dt_stub(0x00492a60, NULL, (void *)stub_cam_set_fov);
    dt_stub(0x00492ac0, NULL, (void *)stub_cam_set_view);
    dt_stub(0x00487370, NULL, (void *)stub_obj_alloc);
    dt_stub(0x004873a0, NULL, (void *)stub_obj_init);
    dt_stub(0x00487400, NULL, (void *)stub_obj_free);
    dt_stub(0x00487470, NULL, (void *)stub_obj_set_position);
    dt_stub(0x004874d0, NULL, (void *)stub_obj_set_rotation);
    dt_stub(0x00495860, NULL, (void *)stub_look_at);
    gInitResult = 1;
    gCreateResult = 1;
    gAllocResult = 1;
}

/* Cache list as the original builds it. */
static void make_cache_list(void)
{
    dt_zmem_env();
    gZobjCameraCaches = DT_ORIG(0x00485e80, ZMemFreeList * (*)(uint32_t, uint32_t, void *))(0x104, 4, 0);
}

static ZObjCamera *new_camera(void)
{
    ZObjCamera *cam = dt_alloc(sizeof *cam);
    ZCameraCache *cache = DT_ORIG(0x00486050, void *(*)(ZMemFreeList *))(gZobjCameraCaches);
    ZQuat *q = &cam->rot;
    float len;
    int i;
    float *f = (float *)cache;

    cam->flags = dt_u32();
    cam->pos.x = dt_float_edgy(-100.0f, 100.0f);
    cam->pos.y = dt_float_edgy(-100.0f, 100.0f);
    cam->pos.z = dt_float_edgy(-100.0f, 100.0f);
    do {
        q->x = dt_float(-1.0f, 1.0f);
        q->y = dt_float(-1.0f, 1.0f);
        q->z = dt_float(-1.0f, 1.0f);
        q->w = dt_float(-1.0f, 1.0f);
        len = q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w;
    } while (len < 0.1f);
    cam->vtable = &gZobjCameraVtable;
    cam->camera = (void *)(uintptr_t)(0x20000000u + dt_u32() % 0x1000u);
    cam->cache = cache;
    for (i = 0; i < 0x40; i++) {
        f[i] = dt_float_edgy(-10.0f, 10.0f);
    }
    cache->valid = dt_u32();
    return cam;
}

static void setup_cam_init(DtCtx *c)
{
    dt_seed(c);
    stubs_on();
    dt_zmem_env();
    gZobjCameraInitialized = c->seed % 5 == 4; /* seed 4: the "Already initialized" assert */
    gZobjCameraCaches = NULL;
    gInitResult = c->seed % 3 != 2;
}

static void setup_cam_obj(DtCtx *c)
{
    dt_seed(c);
    stubs_on();
    make_cache_list();
    DT_ARG_PTR(c, 0, new_camera());
}

static void setup_cam_create(DtCtx *c)
{
    dt_seed(c);
    stubs_on();
    make_cache_list();
    gCreateResult = c->seed % 4 != 3;
    gAllocResult = c->seed % 5 != 4;
    memset(gObjSlot, 0, sizeof gObjSlot);
    gZobjCameraTarget.x = dt_float(-50.0f, 50.0f);
    gZobjCameraTarget.y = dt_float(-50.0f, 50.0f);
    gZobjCameraTarget.z = dt_float(-50.0f, 50.0f);
}

static void setup_cam_fov(DtCtx *c)
{
    setup_cam_obj(c);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_cam_look_at(DtCtx *c)
{
    setup_cam_obj(c);
    DT_ARG_PTR(c, 1, rand_vec(100.0f));
    DT_ARG_PTR(c, 2, rand_vec(100.0f));
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_cam_matrix(DtCtx *c)
{
    setup_cam_obj(c);
    switch (c->seed % 8) {
    case 0:
    case 1:
    case 2:
    case 3:
        DT_ARG_INT(c, 1, c->seed % 8);
        break;
    case 4:
        DT_ARG_INT(c, 1, 4); /* unknown kind: assert */
        break;
    case 5:
        DT_ARG_INT(c, 1, 33); /* tests bit 1 of the valid mask, then indexes far outside the cache */
        break;
    default:
        DT_ARG_INT(c, 1, dt_range(0, 3));
    }
    /* Half of the iterations start with the dependent matrix already valid or nothing valid. */
    if (c->seed % 3 == 0) {
        ((ZObjCamera *)(uintptr_t)c->args[0])->cache->valid = 0;
    } else if (c->seed % 3 == 1) {
        ((ZObjCamera *)(uintptr_t)c->args[0])->cache->valid &= 0xf;
    }
}

DT_CASES(zobjcamera_object_cases) = {
    {"zobjCameraDestroy", 0x00482090, zobjCameraDestroy, 1, DT_RET_INT, setup_cam_obj, 20, 0},
    {"zobjCameraDraw", 0x004820c0, zobjCameraDraw, 1, DT_RET_INT, setup_cam_obj, 200, 0},
    {"zobjCameraMoved", 0x004820f0, zobjCameraMoved, 1, DT_RET_INT, setup_cam_obj, 20, 0},
    {"zobjCameraRotated", 0x00482110, zobjCameraRotated, 1, DT_RET_INT, setup_cam_obj, 20, 0},
    {"zobjCameraInitialize", 0x00482130, zobjCameraInitialize, 0, DT_RET_VOID, setup_cam_init, 20, 0},
    {"zobjCameraCreate", 0x004821c0, zobjCameraCreate, 0, DT_RET_INT, setup_cam_create, 200, 0},
    {"zobjCameraSetFov", 0x004822a0, zobjCameraSetFov, 2, DT_RET_VOID, setup_cam_fov, 20, 0},
    {"zobjCameraLookAt", 0x004822c0, zobjCameraLookAt, 4, DT_RET_VOID, setup_cam_look_at, 200, 0},
    {"zobjCameraGetMatrix", 0x00482310, zobjCameraGetMatrix, 2, DT_RET_INT, setup_cam_matrix, 400, 0},
};
