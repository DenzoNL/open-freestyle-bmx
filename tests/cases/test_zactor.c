#include "bmx/com/zge/zactor.h"
#include "dt.h"
#include "dt_zactor.h"
#include "dt_zmem.h"
#include "dt_zprim.h"
#include "zge/zmem.h"

static int rand_type(void)
{
    static const int types[] = {
        kZactorTypePerson, kZactorTypeProp, kZactorTypeEffect, kZactorTypeObject, 0, 2, 7};
    return types[dt_u32() % (sizeof types / sizeof types[0])];
}

/* ---- slots and lifetime ------------------------------------------------------------------------ */

static void setup_none(DtCtx *c)
{
    dt_seed(c);
}

static void setup_table_init(DtCtx *c)
{
    dt_seed(c);
    dt_static_region(0x4000);
    gZactorMax = dt_range(0, 6);
    gMotionBankCount = dt_range(0, 5);
    gZactors = NULL;
}

static ZActor *rand_table(int n, int freeSlots)
{
    ZActor *t = dt_alloc((uint32_t)n * sizeof(ZActor));
    for (int i = 0; i < n; i++) {
        t[i].type = freeSlots && (dt_u32() & 1) ? 0 : rand_type() | 1;
    }
    return t;
}

static void setup_find_free(DtCtx *c)
{
    dt_seed(c);
    const int n = dt_range(1, 6);
    gZactorMax = n;
    gZactors = rand_table(n, c->seed % 4 != 3);
    if (c->seed % 4 == 3) {
        for (int i = 0; i < n; i++) {
            gZactors[i].type = rand_type() | 1; /* full table: prints the allocation failure */
        }
    }
}

static void setup_create(DtCtx *c)
{
    setup_find_free(c);
    dt_static_region(0x4000);
    DT_ARG_INT(c, 0, rand_type());
}

static void setup_init(DtCtx *c)
{
    dt_seed(c);
    dt_static_region(0x4000);
    ZActor *a = dt_alloc(sizeof(ZActor));
    dt_fill(a, sizeof *a);
    DT_ARG_INT(c, 0, rand_type());
    DT_ARG_PTR(c, 1, a);
}

static void setup_copy(DtCtx *c)
{
    dt_seed(c);
    ZActor *src = dt_rand_actor(c->seed % 5 == 4 ? kZactorTypePerson : kZactorTypeProp);
    ZActor *dst = dt_rand_actor(rand_type());
    DT_ARG_PTR(c, 0, dst);
    DT_ARG_PTR(c, 1, src);
}

static void setup_actor(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
}

static void setup_person_angles(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    DT_ARG_PTR(c, 0, dt_rand_actor(kZactorTypePerson));
}

static void setup_actor_vec(DtCtx *c)
{
    dt_seed(c);
    ZVec3 *v = dt_alloc(sizeof *v);
    dt_rf3(v);
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_PTR(c, 1, v);
}

static void setup_actor_int(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
}

static void setup_int(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    DT_ARG_INT(c, 0, (c->seed & 1) ? (int32_t)dt_u32() : dt_range(-0x10000, 0x20000));
}

static void setup_scale(DtCtx *c)
{
    dt_seed(c);
    ZVec3 *v = dt_alloc(sizeof *v);
    if (c->seed % 3 == 0) {
        v->x = v->y = v->z = 1.0f;
    } else if (c->seed % 3 == 1) {
        v->x = 1.0f;
        v->y = (c->seed & 4) ? 1.0f : dt_rf();
        v->z = 1.0f;
    } else {
        dt_rf3(v);
    }
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_PTR(c, 1, v);
}

DT_CASES(zactor_basic_cases) = {
    {"zactorTableInit", 0x00472630, zactorTableInit, 0, DT_RET_VOID, setup_table_init, 40, 0},
    {"zactorNop0", 0x004726c0, zactorNop0, 0, DT_RET_VOID, setup_none, 1, 0},
    {"zactorCopy", 0x004726e0, zactorCopy, 2, DT_RET_VOID, setup_copy, 60, 0},
    {"zactorFindFree", 0x00472730, zactorFindFree, 0, DT_RET_INT, setup_find_free, 60, 0},
    {"zactorCreate", 0x00472780, zactorCreate, 1, DT_RET_INT, setup_create, 120, 0},
    {"zactorInit", 0x004727b0, zactorInit, 2, DT_RET_INT, setup_init, 120, 0},
    {"zactorDestroy", 0x004728c0, zactorDestroy, 1, DT_RET_VOID, setup_actor, 20, 0},
    {"zactorClear", 0x004728e0, zactorClear, 1, DT_RET_VOID, setup_actor, 20, 0},
    {"zactorSetPos", 0x00472910, zactorSetPos, 2, DT_RET_VOID, setup_actor_vec, 20, 0},
    {"zactorGetPos", 0x00472940, zactorGetPos, 1, DT_RET_INT, setup_actor, 10, 0},
    {"zactorSetHeading", 0x00472960, zactorSetHeading, 2, DT_RET_VOID, setup_actor_int, 200, 0},
    {"zactorGetHeading", 0x00472990, zactorGetHeading, 1, DT_RET_INT16, setup_person_angles, 200, 0},
    {"zactorAngleToWorld", 0x004729c0, zactorAngleToWorld, 1, DT_RET_INT, setup_int, 200, 0},
    {"zactorAngleFromWorld", 0x00472a00, zactorAngleFromWorld, 1, DT_RET_INT, setup_int, 200, 0},
    {"zactorSetScale", 0x00472a30, zactorSetScale, 2, DT_RET_VOID, setup_scale, 300, 0},
    {"zactorGetScale", 0x00472af0, zactorGetScale, 1, DT_RET_INT, setup_actor, 10, 0},
};

/* ---- vectors and joint blends ------------------------------------------------------------------ */

static void setup_vec3_zero(DtCtx *c)
{
    dt_seed(c);
    ZVec3 *v = dt_alloc(sizeof *v);
    const float zero = (c->seed & 8) ? -0.0f : 0.0f;
    v->x = (c->seed & 1) ? dt_rf() : zero;
    v->y = (c->seed & 2) ? dt_rf() : zero;
    v->z = (c->seed & 4) ? dt_rf() : zero;
    DT_ARG_PTR(c, 0, v);
}

static void setup_vec3_binary(DtCtx *c)
{
    dt_seed(c);
    ZVec3 *a = dt_alloc(sizeof *a);
    ZVec3 *b = dt_alloc(sizeof *b);
    dt_rf3(a);
    dt_rf3(b);
    ZVec3 *out = dt_alloc(sizeof *out);
    switch (c->seed % 4) {
    case 0:
        out = a;
        break;
    case 1:
        out = b;
        break;
    default:
        break;
    }
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
    DT_ARG_PTR(c, 2, out);
}

static ZAngle rand_angle(void)
{
    switch (dt_u32() % 6) {
    case 0:
        return (ZAngle)0x7fff;
    case 1:
        return (ZAngle)-0x8000;
    case 2:
        return (ZAngle)dt_range(-300, 300);
    default:
        return (ZAngle)dt_u32();
    }
}

static ZAngle3 *rand_angles(int n)
{
    ZAngle3 *a = dt_alloc((uint32_t)n * sizeof *a);
    for (int i = 0; i < n; i++) {
        a[i].x = rand_angle();
        a[i].y = rand_angle();
        a[i].z = rand_angle();
    }
    return a;
}

static void setup_angles_binary(DtCtx *c)
{
    dt_seed(c);
    const int n = dt_range(1, 9);
    ZAngle3 *a = rand_angles(n);
    ZAngle3 *b = rand_angles(n);
    if (c->seed % 3 == 0) { /* values that differ by about half a turn exercise the wrap branch */
        for (int i = 0; i < n; i++) {
            b[i].x = (ZAngle)(a[i].x + 0x8000 + dt_range(-3, 3));
            b[i].y = (ZAngle)(a[i].y + 0x8000 + dt_range(-3, 3));
            b[i].z = (ZAngle)(a[i].z + 0x7fff + dt_range(-3, 3));
        }
    }
    ZAngle3 *out = dt_alloc((uint32_t)n * sizeof *out);
    if (c->seed % 5 == 1) {
        out = b;
    } else if (c->seed % 5 == 2) {
        out = a;
    }
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
    DT_ARG_PTR(c, 2, out);
    DT_ARG_INT(c, 3, n);
}

static void setup_angles_lerp(DtCtx *c)
{
    dt_seed(c);
    const int n = dt_range(1, 9);
    DT_ARG_PTR(c, 0, rand_angles(n));
    DT_ARG_PTR(c, 1, rand_angles(n));
    DT_ARG_PTR(c, 2, dt_alloc((uint32_t)n * sizeof(ZAngle3)));
    DT_ARG_FLOAT(c, 3, dt_float_edgy(-1.5f, 2.5f));
    DT_ARG_INT(c, 4, n | ((int)dt_u32() & ~0xff)); /* only the low byte is the count */
}

static void setup_reset_scale(DtCtx *c)
{
    dt_seed(c);
    const int n = dt_range(1, 9);
    float *scale = dt_alloc(256 * sizeof(float));
    for (int i = 0; i < 256; i++) {
        scale[i] = dt_rf();
    }
    DT_ARG_PTR(c, 0, rand_angles(n));
    DT_ARG_PTR(c, 1, rand_angles(n));
    DT_ARG_PTR(c, 2, scale);
    DT_ARG_INT(c, 3, n | ((int)dt_u32() & ~0xff));
}

static void setup_select_joints(DtCtx *c)
{
    dt_seed(c);
    const int n = dt_range(0, 9);
    uint8_t *mask = dt_alloc(16);
    for (int i = 0; i < 16; i++) {
        mask[i] = (uint8_t)(dt_u32() & 1 ? dt_u32() : 0);
    }
    DT_ARG_PTR(c, 0, rand_angles(9));
    DT_ARG_PTR(c, 1, rand_angles(9));
    DT_ARG_PTR(c, 2, dt_alloc(9 * sizeof(ZAngle3)));
    DT_ARG_PTR(c, 3, mask);
    DT_ARG_INT(c, 4, n | ((int)dt_u32() & ~0xff));
}

static void setup_two_actors(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_rand_actor(kZactorTypePerson));
    DT_ARG_PTR(c, 1, dt_rand_actor(kZactorTypePerson));
}

DT_CASES(zactor_vec_cases) = {
    {"zvec3IsZero", 0x00474070, zvec3IsZero, 1, DT_RET_INT, setup_vec3_zero, 100, 0},
    {"zvec3Average", 0x00474d20, zvec3Average, 3, DT_RET_VOID, setup_vec3_binary, 300, 0},
    {"zvec3Blend", 0x00474d70, zvec3Blend, 3, DT_RET_VOID, setup_vec3_binary, 300, 0},
    {"zmotionAverageAngles", 0x004749a0, zmotionAverageAngles, 4, DT_RET_VOID, setup_angles_binary, 400, 0},
    {"zmotionBlendAngles", 0x00474b10, zmotionBlendAngles, 4, DT_RET_VOID, setup_angles_binary, 600, 0},
    {"zactorCopySecondaryJoints", 0x00474960, zactorCopySecondaryJoints, 2, DT_RET_VOID, setup_two_actors, 20,
     0},
    {"zmotionLerpAngles", 0x00476930, zmotionLerpAngles, 5, DT_RET_VOID, setup_angles_lerp, 400, 0},
    {"zmotionResetScale", 0x00476a10, zmotionResetScale, 4, DT_RET_VOID, setup_reset_scale, 200, 0},
    {"zmotionSelectJoints", 0x004772a0, zmotionSelectJoints, 5, DT_RET_VOID, setup_select_joints, 200, 0},
};

/* ---- motion banks and markers ----------------------------------------------------------------- */

static void setup_bank_add(DtCtx *c)
{
    dt_seed(c);
    uint8_t *data = dt_alloc(0x80);
    dt_fill(data, 0x80);
    switch (c->seed % 3) {
    case 0: /* library with the magic header */
        *(uint32_t *)data = 0x2a81511c;
        break;
    case 1: /* 8 byte header */
        data[0] = 0xff;
        break;
    default:
        data[0] = (uint8_t)dt_range(0, 0xfe);
    }
    gMotionBankCount = dt_range(0, 3);
    gMotionFirstBank = NULL;
    DT_ARG_PTR(c, 0, data);
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
    DT_ARG_INT(c, 2, dt_range(0, 1));
}

static void setup_bank_get(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_INT(c, 0, dt_range(0, 3));
}

/* Fills bank `n` of gMotionBanks with a table of random entries; returns the ids stored. */
static void make_bank(int n, uint32_t *ids, int *count)
{
    ZMotionBank *banks = gMotionBanks;
    const int stride = dt_range(1, 3);
    const int entries = dt_range(0, 4);
    uint8_t *base = dt_alloc(0x400);
    dt_fill(base, 0x400);
    uint16_t *hdr = dt_alloc((uint32_t)(4 + entries * (4 + stride * 8)));
    hdr[0] = (uint16_t)stride;
    hdr[1] = (uint16_t)entries;
    uint8_t *e = (uint8_t *)(hdr + 2);
    for (int i = 0; i < entries; i++) {
        const uint32_t id = (uint32_t)dt_range(1, 40);
        *(uint32_t *)e = id;
        *(uint32_t *)(e + 4) = (uint32_t)dt_range(0, 0x300);
        ids[*count] = id;
        (*count)++;
        e += 4 + stride * 8;
    }
    banks[n].base = base;
    banks[n].size = 0x400;
    banks[n].table = hdr;
    banks[n].cursor = (const uint32_t *)(hdr + 2);
    banks[n].remaining = (uint32_t)entries;
}

static void setup_find(DtCtx *c)
{
    dt_seed(c);
    uint32_t ids[16];
    int count = 0;
    gMotionBankCount = dt_range(0, 2);
    for (int b = 0; b < gMotionBankCount; b++) {
        make_bank(b, ids, &count);
    }
    DT_ARG_INT(c, 0, count && (c->seed & 3) ? (int)ids[dt_u32() % (uint32_t)count] : dt_range(0, 50));
}

static void setup_bank_next(DtCtx *c)
{
    dt_seed(c);
    uint32_t ids[16];
    int count = 0;
    make_bank(0, ids, &count);
    ZMotionBank *banks = gMotionBanks;
    ZMotionBank *b = dt_alloc(sizeof *b);
    *b = banks[0];
    if (c->seed % 5 == 4) {
        b->remaining = 0;
    }
    DT_ARG_PTR(c, 0, b);
}

static void setup_load_bank(DtCtx *c)
{
    dt_seed(c);
    const uint32_t id = (uint32_t)dt_range(0, 0xffff);
    gMotionLoadedBank = id; /* same id: "Not reloading motion library" */
    gMotionLibrary = 1;
    DT_ARG_INT(c, 0, (int32_t)(id | (dt_u32() & 0xffff0000u)));
}

/* Library closed: the load / close routines assert. */
static void setup_motion_lib_closed(DtCtx *c)
{
    dt_seed(c);
    gMotionLibrary = 0;
    DT_ARG_INT(c, 0, dt_range(0, 0x100));
}

static void setup_open_missing(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    gMotionLibrary =
        c->seed % 2; /* odd: already open, asserts; even: tries (and fails) to open a missing file */
    DT_ARG_PTR(c, 0, "missing-motions.lib");
}

typedef struct MarkerFixture {
    ZMotion *m;
} MarkerFixture;

static ZMotion *make_marker_motion(void)
{
    const int count = dt_range(0, 5);
    const int wide = dt_u32() & 1;
    uint8_t *buf = dt_alloc(0x40 + (uint32_t)count * 10 + 0x40);
    dt_fill(buf, 0x40);
    ZMotion *m = (ZMotion *)buf;
    m->format = (uint8_t)((wide ? (uint32_t)(dt_range(1, 7) << 4) : 0) | (dt_u32() & 0x8f));
    m->markerOffset = (dt_u32() % 5 == 0) ? 0 : 0x20;
    uint8_t *block = buf + 0x20;
    *(uint16_t *)(block + 0xc) = (uint16_t)count;
    int16_t *e = (int16_t *)(block + 0x10);
    for (int i = 0; i < count * (wide ? 5 : 4); i++) {
        e[i] = (int16_t)(dt_u32() & 1 ? dt_range(-2000, 2000) : (int)dt_u32());
    }
    return m;
}

static void setup_markers(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    DT_ARG_PTR(c, 0, make_marker_motion());
    DT_ARG_PTR(c, 1, dt_alloc(6 * sizeof(ZMotionMarker)));
}

static void setup_marker_count(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, make_marker_motion());
}

DT_CASES(zactor_bank_cases) = {
    {"zmotionBankAdd", 0x00475200, zmotionBankAdd, 3, DT_RET_INT, setup_bank_add, 120, 0},
    {"zmotionBankGet", 0x004752e0, zmotionBankGet, 1, DT_RET_INT, setup_bank_get, 8, 0},
    {"zmotionNop", 0x004752c0, zmotionNop, 0, DT_RET_VOID, setup_none, 1, 0},
    {"zmotionFind", 0x00475300, zmotionFind, 1, DT_RET_INT, setup_find, 200, 0},
    {"zmotionBankNext", 0x004753c0, zmotionBankNext, 1, DT_RET_INT, setup_bank_next, 100, 0},
    {"zmotionLoadBank", 0x00475050, zmotionLoadBank, 1, DT_RET_INT, setup_load_bank, 40, 0},
    {"zmotionLoadExtraBank", 0x00475140, zmotionLoadExtraBank, 1, DT_RET_VOID, setup_motion_lib_closed, 2, 0},
    {"zmotionCloseLibrary", 0x00475000, zmotionCloseLibrary, 0, DT_RET_VOID, setup_motion_lib_closed, 2, 0},
    {"zmotionOpenLibrary", 0x00474f10, zmotionOpenLibrary, 1, DT_RET_INT, setup_open_missing, 2, 0},
    {"zmotionMarkerCount", 0x004754f0, zmotionMarkerCount, 1, DT_RET_INT8, setup_marker_count, 60, 0},
    {"zmotionGetMarkers", 0x00475520, zmotionGetMarkers, 2, DT_RET_INT8, setup_markers, 300, 0},
    {"zmotionGetFirstMarker", 0x00475730, zmotionGetFirstMarker, 2, DT_RET_INT8, setup_markers, 200, 0},
};

/* ---- accessors --------------------------------------------------------------------------------- */

static uint32_t vis_log[4];

static void BMX_CDECL vis_cb(int32_t object, int visible)
{
    vis_log[0]++;
    vis_log[1] += (uint32_t)object;
    vis_log[2] += (uint32_t)visible;
}

static void setup_visible(DtCtx *c)
{
    dt_seed(c);
    ZActor *a = dt_rand_actor(rand_type());
    uint32_t *obj = dt_alloc(0x40);
    uint32_t *vtab = dt_alloc(0x20);
    obj[0] = dt_u32();
    obj[0x28 / 4] = (uint32_t)(uintptr_t)vtab;
    vtab[0x14 / 4] = (c->seed & 1) ? (uint32_t)(uintptr_t)vis_cb : 0;
    a->object = (int32_t)(uintptr_t)obj;
    DT_ARG_PTR(c, 0, a);
}

static void setup_actor_u16(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
}

static void setup_lod(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_INT(c, 1,
               c->seed % 6 == 5 ? (int32_t)(dt_range(4, 9) | (dt_u32() & 0xffff0000u))
                                : (int32_t)(dt_range(0, 3) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 2, (int32_t)dt_u32());
}

static void setup_actor_float(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_FLOAT(c, 1, dt_rf());
}

static void setup_extent(DtCtx *c)
{
    dt_seed(c);
    ZVec3 *v = dt_alloc(sizeof *v);
    dt_rf3(v);
    switch (c->seed % 8) {
    case 0:
        v->x = v->y;
        break;
    case 1:
        v->z = v->y;
        break;
    case 2:
        v->z = v->x;
        break;
    case 3:
        v->x = v->y = v->z;
        break;
    case 4:
        *(uint32_t *)&v->x = 0x7fc00000; /* NaN compares as unordered */
        break;
    case 5:
        *(uint32_t *)&v->y = 0x7fc00000;
        break;
    case 6:
        *(uint32_t *)&v->z = 0x7fc00000;
        break;
    default:
        break;
    }
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_PTR(c, 1, v);
}

static void setup_angle_base(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    DT_ARG_INT(c, 0, (int32_t)dt_u32());
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
}

static void setup_two_ints(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    DT_ARG_INT(c, 0, (int32_t)dt_u32());
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
}

static void setup_attachment(DtCtx *c)
{
    dt_seed(c);
    ZVec3 *v = dt_alloc(sizeof *v);
    dt_rf3(v);
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_PTR(c, 1, v);
    DT_ARG_INT(c, 2, (int32_t)dt_u32());
}

static void setup_tag(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_INT(c, 1, c->seed % 3 == 0 ? dt_range(1, 5) : 0);
}

static void setup_rate(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_INT(c, 0,
               (int32_t)((uint32_t)(int16_t)(c->seed % 9 == 0 ? 0 : dt_range(-3000, 3000)) |
                         (dt_u32() & 0xffff0000u)));
}

static void setup_actor_vec_out(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    ZActor *a = dt_rand_actor(kZactorTypePerson);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, dt_alloc(sizeof(ZVec3)));
}

static void setup_bone_pos(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    ZActor *a = dt_rand_actor(kZactorTypePerson);
    ZVec3 *in = dt_alloc(sizeof *in);
    dt_rf3(in);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_INT(c, 1, dt_range(0, 14));
    DT_ARG_PTR(c, 2, in);
    DT_ARG_PTR(c, 3, c->seed % 7 == 0 ? (void *)in : dt_alloc(sizeof(ZVec3)));
    DT_ARG_INT(c, 4, (int32_t)(((uint32_t)dt_range(0, 7)) | (dt_u32() & ~0xffu)));
}

static void setup_rider_yaw(DtCtx *c)
{
    dt_seed(c);
    ZActor *a = dt_rand_actor(c->seed % 9 == 8 ? kZactorTypeProp : kZactorTypePerson);
    if (c->seed & 1) {
        a->flags |= kZactorYawOffset;
    } else {
        a->flags &= ~(uint32_t)kZactorYawOffset;
    }
    DT_ARG_PTR(c, 0, a);
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
}

static void setup_joint_angles(DtCtx *c)
{
    dt_seed(c);
    ZAngle3 *v = rand_angles(1);
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_INT(c, 1, (int32_t)((uint32_t)dt_range(0, 18) | (dt_u32() & 0xffff0000u)));
    DT_ARG_PTR(c, 2, v);
}

static void setup_actor_int_plain(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
}

static void setup_actor_angles_out(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_PTR(c, 1, dt_alloc(sizeof(ZAngle3) + 8));
}

static void setup_actor_matrix_out(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_rand_actor(kZactorTypePerson));
    DT_ARG_PTR(c, 1, dt_alloc(sizeof(ZMatrix)));
}

static void setup_set_rotation(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    DT_ARG_PTR(c, 0, dt_rand_actor(rand_type()));
    DT_ARG_PTR(c, 1, rand_angles(1));
}

DT_CASES(zactor_accessor_cases) = {
    {"zactorSetModel", 0x00475430, zactorSetModel, 2, DT_RET_VOID, setup_actor_u16, 20, 0},
    {"zactorSetLodModel", 0x00475460, zactorSetLodModel, 3, DT_RET_VOID, setup_lod, 60, 0},
    {"zactorGetModel", 0x004754d0, zactorGetModel, 1, DT_RET_INT16, setup_actor, 10, 0},
    {"zactorSetField172", 0x004758f0, zactorSetField172, 2, DT_RET_VOID, setup_actor_u16, 20, 0},
    {"zactorSetRadius", 0x00475920, zactorSetRadius, 2, DT_RET_VOID, setup_actor_float, 20, 0},
    {"zactorSetExtent", 0x00475940, zactorSetExtent, 2, DT_RET_VOID, setup_extent, 200, 0},
    {"zactorSetAngleBase", 0x004759d0, zactorSetAngleBase, 2, DT_RET_VOID, setup_angle_base, 50, 0},
    {"zactorOffsetAngle", 0x00475a00, zactorOffsetAngle, 2, DT_RET_INT, setup_two_ints, 100, 0},
    {"zactorShow", 0x00475a30, zactorShow, 1, DT_RET_VOID, setup_visible, 20, 0},
    {"zactorHide", 0x00475a60, zactorHide, 1, DT_RET_VOID, setup_visible, 20, 0},
    {"zactorSetFlag2", 0x00475a90, zactorSetFlag2, 1, DT_RET_VOID, setup_actor, 10, 0},
    {"zactorClearFlag2", 0x00475ab0, zactorClearFlag2, 1, DT_RET_VOID, setup_actor, 10, 0},
    {"zactorSetAttachment", 0x00475ad0, zactorSetAttachment, 3, DT_RET_VOID, setup_attachment, 20, 0},
    {"zactorSetTag", 0x00475b10, zactorSetTag, 2, DT_RET_VOID, setup_tag, 40, 0},
    {"zactorClearTag", 0x00475b50, zactorClearTag, 2, DT_RET_VOID, setup_tag, 40, 0},
    {"zactorSetFlag20", 0x00475b90, zactorSetFlag20, 1, DT_RET_VOID, setup_actor, 10, 0},
    {"zactorClearFlag20", 0x00475bb0, zactorClearFlag20, 1, DT_RET_VOID, setup_actor, 10, 0},
    {"zactorNop1", 0x00475bd0, zactorNop1, 0, DT_RET_VOID, setup_none, 1, 0},
    {"zmotionSetRate", 0x00475bf0, zmotionSetRate, 1, DT_RET_VOID, setup_rate, 60, 0},
    {"zactorNop2", 0x00475c20, zactorNop2, 0, DT_RET_VOID, setup_none, 1, 0},
    {"zactorSetField174", 0x00475c40, zactorSetField174, 2, DT_RET_VOID, setup_actor_int_plain, 10, 0},
    {"zactorGetField174", 0x00475c60, zactorGetField174, 1, DT_RET_INT, setup_actor, 10, 0},
    {"zactorGetBasePos", 0x00475c80, zactorGetBasePos, 2, DT_RET_VOID, setup_actor_vec_out, 200, 0},
    {"zactorGetHeadPos", 0x00475cc0, zactorGetHeadPos, 2, DT_RET_VOID, setup_actor_vec_out, 300, 0},
    {"zactorGetBonePos", 0x00475e30, zactorGetBonePos, 5, DT_RET_VOID, setup_bone_pos, 400, 0},
    {"zactorAddRiderYaw", 0x004760b0, zactorAddRiderYaw, 2, DT_RET_VOID, setup_rider_yaw, 80, 0},
    {"zactorSetJointAngles", 0x00476140, zactorSetJointAngles, 3, DT_RET_VOID, setup_joint_angles, 40, 0},
    {"zactorGetJointAngles", 0x00476190, zactorGetJointAngles, 3, DT_RET_VOID, setup_joint_angles, 40, 0},
    {"zactorSetSkeleton", 0x004761e0, zactorSetSkeleton, 2, DT_RET_VOID, setup_actor_int_plain, 10, 0},
    {"zactorGetSkeleton", 0x00476200, zactorGetSkeleton, 1, DT_RET_INT, setup_actor, 10, 0},
    {"zactorSetHeight", 0x00476220, zactorSetHeight, 2, DT_RET_VOID, setup_actor_float, 20, 0},
    {"zactorGetStepCount", 0x00476250, zactorGetStepCount, 1, DT_RET_INT8, setup_actor, 40, 0},
    {"zactorGetHeight", 0x00476280, zactorGetHeight, 1, DT_RET_FLOAT, setup_actor, 20, 0},
    {"zactorGetRotation", 0x004762a0, zactorGetRotation, 2, DT_RET_VOID, setup_actor_angles_out, 100, 0},
    {"zactorGetMatrix", 0x004762f0, zactorGetMatrix, 2, DT_RET_VOID, setup_actor_matrix_out, 100, 0},
    {"zactorSetRotation", 0x00476340, zactorSetRotation, 2, DT_RET_VOID, setup_set_rotation, 100, 0},
    {"zactorCopyRootAngles", 0x00476880, zactorCopyRootAngles, 2, DT_RET_VOID, setup_two_actors, 20, 0},
    {"zactorCopyRotation", 0x004768f0, zactorCopyRotation, 2, DT_RET_VOID, setup_two_actors, 20, 0},
};

/* ---- orientation ------------------------------------------------------------------------------- */

static void rand_unit3(ZVec3 *v)
{
    float len;
    do {
        v->x = dt_float(-1.0f, 1.0f);
        v->y = dt_float(-1.0f, 1.0f);
        v->z = dt_float(-1.0f, 1.0f);
        len = v->x * v->x + v->y * v->y + v->z * v->z;
    } while (len < 0.05f || len > 1.0f);
}

static ZMatrix *rand_matrix(int seed)
{
    ZMatrix *m = dt_alloc(sizeof *m);
    rand_unit3(&m->right);
    rand_unit3(&m->up);
    rand_unit3(&m->at);
    dt_rf3(&m->pos);
    m->flags = dt_u32();
    m->pad1 = dt_u32();
    m->pad2 = dt_u32();
    m->pad3 = dt_u32();
    switch (seed % 6) {
    case 0: /* up perpendicular to right: the |dot| < 0.01 branch */
        m->right.x = 1.0f;
        m->right.y = 0.0f;
        m->right.z = 0.0f;
        m->up.x = 0.0f;
        break;
    case 1: /* nearly perpendicular */
        m->right.x = 1.0f;
        m->right.y = 0.0f;
        m->right.z = 0.0f;
        m->up.x = dt_float(-0.012f, 0.012f);
        break;
    case 2: /* tiny x / y column: the near-vertical branch of the angle extraction */
        m->right.x = dt_float(-1e-4f, 1e-4f);
        m->right.y = dt_float(-1e-4f, 1e-4f);
        break;
    case 3: /* right.x exactly 0.001 */
        m->right.x = 0x1.0624dep-10f;
        m->right.y = 0.0f;
        break;
    default:
        break;
    }
    return m;
}

static void setup_orient(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    if (c->seed % 4 == 3) {
        gActorAngleBase &= ~0xffff; /* (short)base == 0: no base rotation */
    }
    ZActor *a = dt_rand_actor(kZactorTypePerson);
    ZMatrix *m = rand_matrix((int)c->seed);
    ZAngle3 *angles = rand_angles(1);
    if (c->seed % 3 == 0) {
        angles->x = angles->y = angles->z = 0;
    }
    ZMatrix *out = (c->seed % 2) ? m : dt_alloc(sizeof(ZMatrix));
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, angles);
    DT_ARG_PTR(c, 2, &m->up);
    DT_ARG_PTR(c, 3, &m->right);
    DT_ARG_PTR(c, 4, out);
}

static void setup_face_matrix(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    if (c->seed % 4 == 3) {
        gActorAngleBase &= ~0xffff;
    }
    DT_ARG_PTR(c, 0, dt_rand_actor(kZactorTypePerson));
    DT_ARG_PTR(c, 1, rand_matrix((int)c->seed));
}

static void setup_angles_from_matrix(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_rand_actor(kZactorTypePerson));
    DT_ARG_PTR(c, 1, rand_matrix((int)c->seed));
}

DT_CASES(zactor_orient_cases) = {
    {"zactorOrient", 0x00476420, zactorOrient, 5, DT_RET_VOID, setup_orient, 300, 0},
    {"zactorFaceMatrix", 0x004763b0, zactorFaceMatrix, 2, DT_RET_VOID, setup_face_matrix, 200, 0},
    {"zactorAnglesFromMatrix", 0x00476770, zactorAnglesFromMatrix, 2, DT_RET_VOID, setup_angles_from_matrix,
     300, 0},
};

/* ---- motion playback --------------------------------------------------------------------------- */

typedef ZMotionDelta *(*OrigSampleFn)(ZActor *, float, uint32_t, ZMotionDelta *, uint32_t);
typedef ZMotionDelta *(*OrigPlayFn)(ZActor *, const ZMotion *, float, uint32_t);
#define ORIG_SAMPLE DT_ORIG(0x00472b10, OrigSampleFn)
#define ORIG_PLAY   DT_ORIG(0x004740c0, OrigPlayFn)

/* skim: 1 for the skim function, 2 when it will also run mirrored (see zactorMotionSkim). */
static ZMotion *rand_motion(int skim)
{
    MotionGen g = {.wide = skim ? 0 : (int)(dt_u32() & 1), /* the skim only reads 8 bit angles */
                   .extra = (int)(dt_u32() & 1),
                   .maxJoints = dt_range(1, skim == 2 ? 3 : (skim ? 15 : 19)),
                   .safeSkim = skim,
                   .step = (uint32_t)dt_range(8, 64),
                   .keys = dt_range(0, 10)};
    return dt_gen_motion(&g);
}

static void motion_env(void)
{
    dt_angle_globals();
    gMotionTimeScale = dt_float(0.5f, 40.0f);
}

static float rand_speed(void)
{
    return (dt_u32() % 5 == 0) ? 0.0f : dt_float(0.0f, 3.0f);
}

/* Person with motion `m` loaded into sampling channel `idx`, restarted by the original code. */
static ZActor *started_actor(ZMotion *m, uint32_t idx, uint32_t flags)
{
    ZActor *a = dt_rand_actor(kZactorTypePerson);
    a->player->ch[idx + 1].motion = m;
    ORIG_SAMPLE(a, 1.0f, flags | 2, dt_alloc(sizeof(ZMotionDelta)), idx);
    return a;
}

static void setup_sample_restart(DtCtx *c)
{
    dt_seed(c);
    motion_env();
    const uint32_t idx = (uint32_t)dt_range(0, 3);
    ZActor *a = dt_rand_actor(kZactorTypePerson);
    a->player->ch[idx + 1].motion = rand_motion(0);
    ZMotionDelta *out = dt_alloc(sizeof *out);
    dt_fill(out, sizeof *out);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_FLOAT(c, 1, rand_speed());
    DT_ARG_INT(c, 2, (int32_t)(dt_u32() | 2));
    DT_ARG_PTR(c, 3, out);
    DT_ARG_INT(c, 4, (int32_t)(idx | (dt_u32() & ~0xffu)));
}

static void setup_sample_advance(DtCtx *c)
{
    dt_seed(c);
    motion_env();
    const uint32_t idx = (uint32_t)dt_range(0, 3);
    ZActor *a = started_actor(rand_motion(0), idx, (uint32_t)(dt_u32() & 8));
    for (int i = dt_range(0, 3); i > 0; i--) { /* some history */
        ORIG_SAMPLE(a, rand_speed(), (uint32_t)(dt_u32() & 8), dt_alloc(sizeof(ZMotionDelta)), idx);
    }
    ZMotionDelta *out = dt_alloc(sizeof *out);
    dt_fill(out, sizeof *out);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_FLOAT(c, 1, rand_speed() * (c->seed % 4 == 0 ? 8.0f : 1.0f));
    DT_ARG_INT(c, 2, (int32_t)(dt_u32() & ~2u));
    DT_ARG_PTR(c, 3, out);
    DT_ARG_INT(c, 4, (int32_t)(idx | (dt_u32() & ~0xffu)));
}

static void setup_skim(DtCtx *c)
{
    dt_seed(c);
    motion_env();
    ZActor *a = dt_rand_actor(kZactorTypePerson);
    ZAngle3 *joints = rand_angles(19);
    const uint32_t flags = (dt_u32() & 8) | (dt_u32() & ~0xffu);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, joints);
    DT_ARG_PTR(c, 2, rand_motion((flags & 8) ? 2 : 1));
    DT_ARG_INT(c, 3, dt_range(0, 14));
    DT_ARG_INT(c, 4, (int32_t)flags);
}

static void setup_play(DtCtx *c)
{
    dt_seed(c);
    motion_env();
    ZActor *a = dt_rand_actor(kZactorTypePerson);
    ZMotion *m0 = rand_motion(0);
    ZMotion *m1 = rand_motion(0);
    uint32_t flags = (uint32_t)((dt_u32() & 8) | (dt_u32() & 0x80));
    if (c->seed % 4 != 0) {
        /* previous motion in channel 1, so a restart blends it */
        ORIG_PLAY(a, m0, 1.0f, (dt_u32() & 8) | 2);
        for (int i = dt_range(0, 3); i > 0; i--) {
            ORIG_PLAY(a, m0, rand_speed(), dt_u32() & 8);
        }
    }
    if (c->seed % 3 != 0) {
        flags |= 2;
        a->player->blend = (uint8_t)dt_range(0, 5);
    } else {
        a->player->blend =
            (uint8_t)dt_range(0, 5); /* continue playing: blend counts down through the cases */
    }
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, m1);
    DT_ARG_FLOAT(c, 2, rand_speed());
    DT_ARG_INT(c, 3, (int32_t)(flags | (dt_u32() & 0x7f00u)));
}

DT_CASES(zactor_motion_cases) = {
    {"zactorMotionSample/restart", 0x00472b10, zactorMotionSample, 5, DT_RET_INT, setup_sample_restart, 400,
     0},
    {"zactorMotionSample/advance", 0x00472b10, zactorMotionSample, 5, DT_RET_INT, setup_sample_advance, 1000,
     0},
    {"zactorMotionSkim", 0x004739c0, zactorMotionSkim, 5, DT_RET_VOID, setup_skim, 600, 0},
    {"zactorPlayMotion", 0x004740c0, zactorPlayMotion, 4, DT_RET_INT, setup_play, 800, 0},
};

typedef ZMotionDelta **(*OrigBlendFn)(ZActor *, const ZMotion *, const ZMotion *, const uint16_t *, float);
typedef ZMotionDelta **(*OrigBlendMaskedFn)(ZActor *, const ZMotion *, const ZMotion *, const uint16_t *,
                                            const uint8_t *);
#define ORIG_BLEND        DT_ORIG(0x00476ae0, OrigBlendFn)
#define ORIG_BLEND_MASKED DT_ORIG(0x00477330, OrigBlendMaskedFn)

static uint16_t *rand_flag_pair(int restart)
{
    uint16_t *f = dt_alloc(2 * sizeof *f);
    for (int i = 0; i < 2; i++) {
        f[i] = (uint16_t)((dt_u32() & 0x88) | (dt_u32() & 0x7f00));
        if (restart) {
            f[i] |= 2;
        }
    }
    f[0] = (uint16_t)(f[0] | (dt_u32() % 5 == 0 ? 0x80 : 0));
    return f;
}

static float rand_weight(void)
{
    return dt_float_edgy(-0.5f, 1.5f);
}

static uint8_t *rand_mask(void)
{
    uint8_t *mask = dt_alloc(24);
    for (int i = 0; i < 24; i++) {
        mask[i] = (uint8_t)(dt_u32() & 1 ? dt_u32() : 0);
    }
    return mask;
}

static void setup_blend(DtCtx *c, int masked)
{
    dt_seed(c);
    motion_env();
    ZActor *a = dt_rand_actor(kZactorTypePerson);
    ZMotion *a0 = rand_motion(0);
    ZMotion *b0 = rand_motion(0);
    ZMotion *a1 = rand_motion(0);
    ZMotion *b1 = rand_motion(0);
    uint8_t *mask = rand_mask();
    uint16_t *f1 = rand_flag_pair(1);
    uint16_t *f2 = rand_flag_pair(0);
    if (c->seed % 4 !=
        0) { /* first blend: restart both motions; the second call then continues or blends again */
        if (masked) {
            ORIG_BLEND_MASKED(a, a0, b0, f1, mask);
        } else {
            ORIG_BLEND(a, a0, b0, f1, rand_weight());
        }
    }
    a->player->blend = (uint8_t)dt_range(0, 5);
    if (c->seed % 3 == 0) {
        a1 = (ZMotion *)a0;
        b1 = (ZMotion *)b0;
    } else if (c->seed % 3 == 1) {
        f2 = rand_flag_pair(1);
    }
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, a1);
    DT_ARG_PTR(c, 2, b1);
    DT_ARG_PTR(c, 3, f2);
    if (masked) {
        DT_ARG_PTR(c, 4, mask);
    } else {
        DT_ARG_FLOAT(c, 4, rand_weight());
    }
}

static void setup_blend_weighted(DtCtx *c)
{
    setup_blend(c, 0);
}

static void setup_blend_masked(DtCtx *c)
{
    setup_blend(c, 1);
}

static void setup_sample_scaled(DtCtx *c)
{
    dt_seed(c);
    motion_env();
    ZMotion *m = rand_motion(0);
    ZActor *a = started_actor(m, 2, (uint32_t)(dt_u32() & 8));
    float *scale = dt_alloc(256 * sizeof(float));
    for (int i = 0; i < 256; i++) {
        scale[i] = dt_rf();
    }
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, m);
    DT_ARG_INT(c, 2, (int32_t)((dt_u32() & 0x88) | (c->seed & 1 ? 2 : 0) | (dt_u32() & 0xffff0000u)));
    DT_ARG_PTR(c, 3, scale);
}

/*
 * zactorSampleScaled samples into a local buffer and leaves its stack address in gMotionSampleA and in the
 * return value. Those cannot match between two compilations of the function, so both sides run through
 * wrappers that clear them; everything else is compared normally.
 */
typedef ZMotionDelta *(*OrigSampleScaledFn)(ZActor *, const ZMotion *, uint32_t, float *);

static ZMotionDelta *sample_scaled_orig(ZActor *a, const ZMotion *m, uint32_t flags, float *scale)
{
    DT_ORIG(0x00477210, OrigSampleScaledFn)(a, m, flags, scale);
    gMotionSampleA = NULL;
    return NULL;
}

static ZMotionDelta *sample_scaled_impl(ZActor *a, const ZMotion *m, uint32_t flags, float *scale)
{
    zactorSampleScaled(a, m, (uint16_t)flags, scale);
    gMotionSampleA = NULL;
    return NULL;
}

static void setup_root_pose(DtCtx *c)
{
    dt_seed(c);
    dt_angle_globals();
    ZActor *a = dt_rand_actor(kZactorTypePerson);
    a->player->ch[0].boneCount = c->seed % 4 ? 2 : (uint8_t)dt_range(0, 5);
    DT_ARG_PTR(c, 0, a);
}

DT_CASES(zactor_blend_cases) = {
    {"zactorBlendMotions", 0x00476ae0, zactorBlendMotions, 5, DT_RET_INT, setup_blend_weighted, 800, 0},
    {"zactorBlendMotionsMasked", 0x00477330, zactorBlendMotionsMasked, 5, DT_RET_INT, setup_blend_masked, 800,
     0},
    {"zactorSampleScaled", (uint32_t)(uintptr_t)sample_scaled_orig, sample_scaled_impl, 4, DT_RET_INT,
     setup_sample_scaled, 300, 0},
    {"zactorGetRootPose", 0x00474de0, zactorGetRootPose, 1, DT_RET_INT, setup_root_pose, 100, 0},
};

/* ---- motion bank loading through a fake library ------------------------------------------------ */

/* Library with motion banks 1 to 3; each blob starts with a magic header, an 0xff header or a plain table. */
static void fake_motion_library(DtCtx *c, DtFakeLib *lib)
{
    dt_static_region(0x10000);
    dt_zmem_env();
    gFileSystemReady = 1;
    dt_fake_lib_begin(lib, 8, 0x4000);
    for (uint32_t id = 1; id <= 3; id++) {
        uint8_t blob[0x80];
        dt_fill(blob, sizeof blob);
        switch (dt_u32() % 3) {
        case 0:
            *(uint32_t *)blob = 0x2a81511c;
            break;
        case 1:
            blob[0] = 0xff;
            break;
        default:
            blob[0] = 0;
        }
        dt_fake_lib_add(lib, id, blob, (uint32_t)dt_range(0x20, 0x80));
    }
    gMotionLibrary = (int32_t)(uintptr_t)lib->lib;
    gMotionBuffer = gMotionStaging;
    gMotionBankCount = dt_range(0, 1);
    gMotionLoadedBank = (uint32_t)dt_range(10, 20);
    (void)c;
}

static void setup_motion_load_bank(DtCtx *c)
{
    DtFakeLib lib;
    dt_seed(c);
    fake_motion_library(c, &lib);
    const uint32_t id = c->seed % 7 == 6 ? 9 : (uint32_t)dt_range(1, 3); /* 9: missing, asserts */
    DT_ARG_INT(c, 0, (int32_t)(id | (dt_u32() & 0xffff0000u)));
}

static void setup_motion_load_extra(DtCtx *c)
{
    DtFakeLib lib;
    dt_seed(c);
    fake_motion_library(c, &lib);
    const uint32_t id = c->seed % 7 == 6 ? 9 : (uint32_t)dt_range(1, 3);
    DT_ARG_INT(c, 0, (int32_t)(id | (dt_u32() & 0xffff0000u)));
}

DT_CASES(zactor_library_cases) = {
    {"zmotionLoadBank/library", 0x00475050, zmotionLoadBank, 1, DT_RET_INT, setup_motion_load_bank, 60, 0},
    {"zmotionLoadExtraBank/library", 0x00475140, zmotionLoadExtraBank, 1, DT_RET_VOID,
     setup_motion_load_extra, 60, 0},
};
