/*
 * Tests for NgIFzactor.c: the cached part lookup of rider and bike actors. The renderer entry points the
 * code calls (node lookup, node transforms, pose sync) are stubbed; they log their arguments and answer
 * from the seeded generator.
 */
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/zactor.h"
#include "dt.h"
#include "dt_ngif.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/zstring.h"

#define gViewPlayer ORIG_VAR(0x0056b408, uint32_t, gViewPlayer)

static uint32_t BMX_CDECL stubFindNode(void *object, uint32_t hash)
{
    ngLogPtr(object);
    ngLog(hash);
    return (dt_u32() % 5 == 0) ? 0 : dt_u32();
}

static void BMX_CDECL stubSyncPose(void *actor)
{
    ngLog(0x4654b0);
    ngLogPtr(actor);
}

static void BMX_CDECL stubUpdate(void *object)
{
    ngLog(0x4892a0);
    ngLogPtr(object);
}

static void BMX_CDECL stubNodeToWorld(void *object, uint32_t node, const float *local, float *out, int flag)
{
    ngLog(0x489150);
    ngLogPtr(object);
    ngLog(node);
    ngLogVec((const ZVec3 *)local);
    ngLog((uint32_t)flag);
    for (int i = 0; i < 3; i++) {
        out[i] = dt_float_edgy(-100.0f, 100.0f);
    }
}

static void BMX_CDECL stubNodeVector(void *object, uint32_t node, float *out, int flag)
{
    ngLog(0x489180);
    ngLogPtr(object);
    ngLog(node);
    ngLog((uint32_t)flag);
    for (int i = 0; i < 3; i++) {
        out[i] = dt_float_edgy(-100.0f, 100.0f);
    }
}

/* The tables with a mix of ready and not ready rows and some empty nodes. */
static void randomise_tables(void)
{
    dt_fill(&gZactorPose, sizeof gZactorPose);
    for (int i = 0; i < 2; i++) {
        gZactorPose.bikeReady[i] = (dt_u32() & 1) ? 0 : dt_u32();
        gZactorPose.riderReady[i] = (dt_u32() & 1) ? 0 : dt_u32();
    }
    for (uint32_t *p = &gZactorPose.bikeNodes[0][0]; p < &gZactorPose.bikeNodes[0][0] + 20; p++) {
        if (dt_u32() % 4 == 0) {
            *p = 0;
        }
    }
    for (uint32_t *p = &gZactorPose.riderNodes[0][0]; p < &gZactorPose.riderNodes[0][0] + 30; p++) {
        if (dt_u32() % 4 == 0) {
            *p = 0;
        }
    }
}

static ZActor *make_actor(void)
{
    ZActor *a = dt_alloc(sizeof *a);

    dt_fill(a, sizeof *a);
    a->skeleton = (dt_u32() & 1) ? 0 : (int32_t)(dt_u32() | 1);
    a->object = (int32_t)dt_u32();
    return a;
}

static void install_stubs(void)
{
    dt_stub(0x00489110, NULL, (void *)stubFindNode);
    dt_stub(0x004654b0, NULL, (void *)stubSyncPose);
    dt_stub(0x004892a0, NULL, (void *)stubUpdate);
    dt_stub(0x00489150, NULL, (void *)stubNodeToWorld);
    dt_stub(0x00489180, NULL, (void *)stubNodeVector);
}

/* A player index in the rows the tables have, a part usually in a row but sometimes beyond it. */
static void begin(DtCtx *c)
{
    ngBegin(c, 0);
    gViewPlayer = (dt_u32() & ~0xffu) | (uint32_t)dt_range(0, 1);
    randomise_tables();
    install_stubs();
}

static uint32_t random_part(void)
{
    const uint32_t part = (uint32_t)dt_range(0, 24);

    return (dt_u32() & 1) ? part | (dt_u32() & 0xffff0000u) : part;
}

static void setup_view_player(DtCtx *c)
{
    ngBegin(c, 0);
    gViewPlayer = dt_u32();
}

static void setup_init_parts(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_actor());
    DT_ARG_INT(c, 1, dt_range(0, 1));
}

static void setup_position(DtCtx *c)
{
    ZVec3 *in = dt_alloc(sizeof *in);
    ZVec3 *out = dt_alloc(sizeof *out);

    begin(c);
    ngRf3(in);
    dt_fill(out, sizeof *out);
    DT_ARG_PTR(c, 0, make_actor());
    DT_ARG_INT(c, 1, random_part());
    DT_ARG_PTR(c, 2, in);
    DT_ARG_PTR(c, 3, out);
    if (dt_u32() % 6 == 0) {
        DT_ARG_PTR(c, 3, in); /* aliased in and out */
    }
}

static void setup_length(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_actor());
    DT_ARG_INT(c, 1, random_part());
}

static void setup_node(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_actor());
    DT_ARG_INT(c, 1, random_part());
    DT_ARG_INT(c, 2, dt_range(0, 1));
}

static void setup_vector(DtCtx *c)
{
    ZVec3 *out = dt_alloc(sizeof *out);

    begin(c);
    dt_fill(out, sizeof *out);
    DT_ARG_PTR(c, 0, make_actor());
    DT_ARG_INT(c, 1, dt_range(0, 24));
    DT_ARG_PTR(c, 2, out);
    DT_ARG_INT(c, 3, dt_range(0, 1));
}

static void setup_clear(DtCtx *c)
{
    ngBegin(c, 0);
    randomise_tables();
}

static void setup_setup_actor(DtCtx *c)
{
    ngBegin(c, 0);
    DT_ARG_PTR(c, 0, make_actor());
}

/* ---- zactorSyncPose -------------------------------------------------------------------------------- */

static void *BMX_CDECL stubFindSkeletonNode(void *skeleton, uint32_t hash)
{
    ngLogPtr(skeleton);
    ngLog(hash);
    return (dt_u32() % 6 == 0) ? NULL : (void *)(uintptr_t)(dt_u32() | 1);
}

static void BMX_CDECL stubSetNodeRotation(void *node, const float *quat)
{
    ngLog(0x49a570);
    ngLogPtr(node);
    for (int i = 0; i < 4; i++) {
        ngLogFloat(quat[i]);
    }
}

static void BMX_CDECL stubSetPosition(void *object, const ZVec3 *pos)
{
    ngLog(0x487470);
    ngLogPtr(object);
    ngLogVec(pos);
}

static void BMX_CDECL stubSetAngles(void *object, const ZAngle3 *angles)
{
    ngLog(0x487520);
    ngLogPtr(object);
    ngLog((uint16_t)angles->x);
    ngLog((uint16_t)angles->y);
    ngLog((uint16_t)angles->z);
}

static int32_t make_model_object(void)
{
    uint8_t *object = dt_alloc(0x40);

    dt_fill(object, 0x40);
    return (int32_t)(uintptr_t)object;
}

static void setup_sync_pose(DtCtx *c)
{
    static const char *const kArms[6] = {"LeftUpArm",  "LeftLowArm",  "LeftHand",
                                         "RightUpArm", "RightLowArm", "RightHand"};
    ZActor *a;
    uint8_t *player = dt_alloc(0x200);

    ngBegin(c, 0);
    a = make_actor();
    dt_fill(player, 0x200);
    *(float *)(player + 0x100) = dt_float_edgy(-5.0f, 5.0f);
    a->player = (struct ZMotionPlayer *)player;
    a->object = make_model_object();
    a->object2 = make_model_object();
    a->partCount = dt_range(0, 15);
    for (int i = 0; i < 15; i++) {
        a->partHash[i] = (dt_u32() % 3 == 0) ? dt_u32() : (uint32_t)zstrHash(kArms[dt_range(0, 5)]);
    }
    switch (dt_u32() % 4) {
    case 0:
        break; /* looked up by the call */
    case 1:
        for (int i = 0; i < 6; i++) {
            gZactorArmHashes[i] = (uint32_t)zstrHash(kArms[i]);
        }
        break;
    default:
        for (int i = 0; i < 6; i++) {
            gZactorArmHashes[i] = (i == 0 && dt_u32() & 1) ? 0 : (uint32_t)zstrHash(kArms[i]);
        }
        break;
    }
    if (dt_u32() % 4 == 0) {
        gZactorArmHashes[0] = 0;
    }
    for (int i = 0; i < 3; i++) {
        (&a->pos.x)[i] = dt_float_edgy(-100.0f, 100.0f);
    }
    dt_stub(0x0049a540, NULL, (void *)stubFindSkeletonNode);
    dt_stub(0x0049a570, NULL, (void *)stubSetNodeRotation);
    dt_stub(0x00487470, NULL, (void *)stubSetPosition);
    dt_stub(0x00487520, NULL, (void *)stubSetAngles);
    DT_ARG_PTR(c, 0, a);
}

DT_CASES(ngifzactor_cases) = {
    {"zactorSyncPose", 0x004654b0, zactorSyncPose, 1, DT_RET_VOID, setup_sync_pose, 1000, 0},
    {"zactorViewPlayer", 0x00464720, zactorViewPlayer, 0, DT_RET_INT, setup_view_player, 20, 0},
    {"zactorInitRiderParts", 0x00464740, zactorInitRiderParts, 2, DT_RET_VOID, setup_init_parts, 200, 0},
    {"zactorInitBikeParts", 0x004649a0, zactorInitBikeParts, 2, DT_RET_VOID, setup_init_parts, 200, 0},
    {"zactorGetPartPosition", 0x00464b50, zactorGetPartPosition, 4, DT_RET_VOID, setup_position, 2000, 0},
    {"zactorGetPartPositionNoSync", 0x00464d10, zactorGetPartPositionNoSync, 4, DT_RET_VOID, setup_position,
     2000, 0},
    {"zactorGetPartLength", 0x00464ea0, zactorGetPartLength, 2, DT_RET_DOUBLE, setup_length, 1000, 0},
    {"zactorGetPartNode", 0x00464fb0, zactorGetPartNode, 3, DT_RET_INT, setup_node, 1000, 0},
    {"zactorGetPartVector", 0x00465040, zactorGetPartVector, 4, DT_RET_VOID, setup_vector, 1000, 0},
    {"zactorClearPoseTables", 0x004646f0, zactorClearPoseTables, 0, DT_RET_VOID, setup_clear, 20, 0},
    {"zactorSetupRider", 0x00465130, zactorSetupRider, 1, DT_RET_VOID, setup_setup_actor, 100, 0},
    {"zactorSetupBike", 0x00465340, zactorSetupBike, 1, DT_RET_VOID, setup_setup_actor, 100, 0},
};
