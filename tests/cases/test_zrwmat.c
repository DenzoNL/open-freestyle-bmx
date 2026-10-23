/*
 * Tests for zrwmat.c: the model wrappers (their targets are stubs that log their arguments) and the material
 * system (pool, texture slots, MATI / MADF chunk readers). RenderWare and the texture interface are stubs.
 */
#include "dt.h"
#include "dt_s5.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/zobj.h"

#define gModelFlag2 ORIG_VAR(0x0067230c, int, gModelFlag2)

/* ---- model table targets ----------------------------------------------------------------------------- */
S5_STUB0(sObjAlloc, 0x487370)
S5_STUB2(sObjInit, 0x4873a0)
S5_STUB1(sSkeletonBuild, 0x4993f0)
S5_STUB1(sPartCount, 0x488310)
S5_STUB3(sSetFlags, 0x4888c0)
S5_STUB2(sOwner, 0x488ba0)
S5_STUB2(sLoadModel, 0x49db90)
S5_STUB2(sLoadChunk, 0x49da90)
S5_STUB1(sInstance, 0x49ce90)
S5_STUB1(sDestroy, 0x49dcd0)
S5_STUB2(sNodeHashes, 0x488320)
S5_STUB2(sFindNode, 0x488350)
S5_STUB5(sNodeToWorld, 0x4883a0)
S5_STUB4(sNodeVector, 0x488480)
S5_STUB4(sNodeMatrix, 0x4884b0)
S5_STUB2(sSkeleton, 0x488250)
S5_STUB1(sUpdate, 0x488650)

static void wrapBegin(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x487370, sObjAlloc);
    S5_ON(0x4873a0, sObjInit);
    S5_ON(0x4993f0, sSkeletonBuild);
    S5_ON(0x488310, sPartCount);
    S5_ON(0x4888c0, sSetFlags);
    S5_ON(0x488ba0, sOwner);
    S5_ON(0x49db90, sLoadModel);
    S5_ON(0x49da90, sLoadChunk);
    S5_ON(0x49ce90, sInstance);
    S5_ON(0x49dcd0, sDestroy);
    S5_ON(0x488320, sNodeHashes);
    S5_ON(0x488350, sFindNode);
    S5_ON(0x4883a0, sNodeToWorld);
    S5_ON(0x488480, sNodeVector);
    S5_ON(0x4884b0, sNodeMatrix);
    S5_ON(0x488250, sSkeleton);
    S5_ON(0x488650, sUpdate);
}

static ZObj *newObject(void)
{
    ZObj *o = dt_alloc(sizeof *o);

    dt_fill(o, sizeof *o);
    o->model = (void *)(uintptr_t)(dt_u32() | 1);
    return o;
}

static void setup_create_object(DtCtx *c)
{
    ZObj *o = newObject();

    wrapBegin(c);
    s5SetRet(0x487370, dt_u32() % 6 == 0 ? 0 : (uint32_t)(uintptr_t)o);
    s5SetRet(0x488310, (uint32_t)dt_range(0, 3));
    gModelFlag2 = (int)(dt_u32() % 2);
    DT_ARG_INT(c, 0, dt_u32() % 5 == 0 ? 0 : (dt_u32() | 1));
}

static void setup_one_arg(DtCtx *c)
{
    wrapBegin(c);
    s5SetRet(0x49db90, dt_u32());
    s5SetRet(0x49da90, dt_u32());
    s5SetRet(0x49ce90, dt_u32());
    s5SetRet(0x49dcd0, dt_u32());
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_object_1(DtCtx *c)
{
    wrapBegin(c);
    s5SetRet(0x488310, (uint32_t)dt_range(0, 5));
    DT_ARG_PTR(c, 0, newObject());
}

static void setup_object_2(DtCtx *c)
{
    wrapBegin(c);
    s5SetRet(0x488350, dt_u32());
    DT_ARG_PTR(c, 0, newObject());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_object_3(DtCtx *c)
{
    wrapBegin(c);
    DT_ARG_PTR(c, 0, newObject());
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_object_4(DtCtx *c)
{
    setup_object_3(c);
    DT_ARG_INT(c, 4, dt_u32());
}

/* ---- materials --------------------------------------------------------------------------------------- */
S5_STUB1(sTexDrop, 0x498810)
S5_STUB1(sTexRetain, 0x498af0)
S5_STUB1(sTexRaster, 0x498ad0)
S5_STUB2(sSetUserData, 0x498b50)
S5_STUB1(sSetLoadName, 0x498b40)
S5_STUB2(sRpSetTexture, 0x4b9380)
S5_STUB2(sRpSetColor, 0x4b9310)
S5_STUB1(sTexRw, 0x498ac0)
S5_STUB5(sTexLoad, 0x497b00)
S5_STUB1(sDestroyList, 0x485fb0)
S5_STUB3(sPoolCreate, 0x485e80)
S5_STUB3(sSetupCallback, 0x49aad0)
S5_STUB1(sIffBegin, 0x49b130)
S5_STUB1(sIffEnd, 0x49b250)

/* Hash of a texture handle: a few distinct values so that searches match. */
static uint32_t BMX_CDECL sTexHash(uint32_t texture)
{
    s5Log(0x4988b0);
    s5Log(dt_canon(texture));
    return (texture >> 4) & 3;
}

static uint32_t gHookCalls;

static void *BMX_CDECL materialHook(void *material, void *arg)
{
    s5Log(0xb00c);
    s5LogPtr(material);
    s5LogPtr(arg);
    gHookCalls++;
    return material;
}

static void matBegin(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x498810, sTexDrop);
    S5_ON(0x498af0, sTexRetain);
    S5_ON(0x498ad0, sTexRaster);
    S5_ON(0x498b50, sSetUserData);
    S5_ON(0x498b40, sSetLoadName);
    S5_ON(0x4b9380, sRpSetTexture);
    S5_ON(0x4b9310, sRpSetColor);
    S5_ON(0x498ac0, sTexRw);
    S5_ON(0x497b00, sTexLoad);
    S5_ON(0x4988b0, sTexHash);
    gHookCalls = 0;
    gZrwMat.initialized = (int)(dt_u32() % 2);
    gZrwMat.pool = S5_POOL_CREATE(0x74, 4, NULL);
    gZrwMat.hook = NULL;
    gZrwMat.madf = NULL;
    gZrwMat.last = NULL;
}

/* A pool record with random texture handles; `max` is the largest texture count. */
static ZrwMaterial *newMaterial(int max)
{
    ZrwMaterial *m = S5_POOL_ALLOC(gZrwMat.pool);

    dt_fill(m, sizeof *m);
    m->textureCount = (uint16_t)dt_range(0, max);
    m->refCount = (uint16_t)dt_range(1, 3);
    m->rwMaterial = dt_u32() % 3 == 0 ? NULL : (void *)(uintptr_t)(dt_u32() | 1);
    for (uint32_t i = 0; i < 6; i++) {
        *(uint32_t *)(m->slots + i * 12) = (dt_u32() % 6 == 0) ? 0 : (dt_u32() | 1);
    }
    return m;
}

static void setup_initialize(DtCtx *c)
{
    matBegin(c);
    S5_ON(0x485e80, sPoolCreate);
    S5_ON(0x49aad0, sSetupCallback);
    s5SetRet(0x485e80, 0x34000000u + (dt_u32() & 0xffff) * 16);
    memset(&gZrwMat, 0, sizeof gZrwMat);
    gZrwMat.initialized = dt_u32() % 4 == 0;
    gZrwMat.hook = (void *)(uintptr_t)dt_u32();
}

static void setup_shutdown(DtCtx *c)
{
    matBegin(c);
    S5_ON(0x485fb0, sDestroyList);
}

static void setup_none(DtCtx *c)
{
    matBegin(c);
}

static void setup_release(DtCtx *c)
{
    ZrwMaterial *m;

    matBegin(c);
    m = newMaterial(6);
    DT_ARG_PTR(c, 0, m);
}

static void setup_replace(DtCtx *c)
{
    ZrwMaterial *m;

    matBegin(c);
    m = newMaterial(6);
    DT_ARG_PTR(c, 0, m);
    DT_ARG_INT(c, 1, dt_u32() | 1);
    DT_ARG_INT(c, 2, (uint32_t)dt_range(0, 6));
}

static void setup_add(DtCtx *c)
{
    ZrwMaterial *m;

    matBegin(c);
    m = newMaterial(dt_u32() % 8 == 0 ? 6 : 5);
    DT_ARG_PTR(c, 0, m);
    DT_ARG_INT(c, 1, dt_u32() | 1);
}

static void setup_set_color(DtCtx *c)
{
    matBegin(c);
    DT_ARG_PTR(c, 0, newMaterial(6));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_hook(DtCtx *c)
{
    matBegin(c);
    gZrwMat.hook = (void *)(uintptr_t)dt_u32();
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_load_sprite(DtCtx *c)
{
    matBegin(c);
    s5SetRet(0x497b00, dt_u32() % 5 == 0 ? 0 : (dt_u32() | 1));
    gZrwMat.last = (ZrwMaterial *)(uintptr_t)dt_u32();
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32() & 0xffff);
    DT_ARG_PTR(c, 2, dt_rand_str(12, 0));
    DT_ARG_INT(c, 3, (uint32_t)dt_range(0, 4));
}

static void setup_match(DtCtx *c)
{
    uint32_t *state = dt_alloc(12);

    matBegin(c);
    state[0] = (uint32_t)dt_range(0, 3);
    state[1] = (uint32_t)dt_range(0, 2);
    state[2] = dt_u32();
    DT_ARG_PTR(c, 0, newMaterial(dt_u32() % 8 == 0 ? 0 : 6));
    DT_ARG_PTR(c, 1, state);
}

static void setup_find(DtCtx *c)
{
    matBegin(c);
    for (int i = dt_range(0, 3); i > 0; i--) {
        (void)newMaterial(5);
    }
    DT_ARG_INT(c, 0, (uint32_t)dt_range(0, 3));
    DT_ARG_INT(c, 1, (uint32_t)dt_range(0, 2));
}

static void setup_get(DtCtx *c)
{
    matBegin(c);
    DT_ARG_PTR(c, 0, newMaterial(6));
    DT_ARG_INT(c, 1, (uint32_t)dt_range(0, 7));
}

static void setup_get_in_range(DtCtx *c)
{
    ZrwMaterial *m;

    matBegin(c);
    m = newMaterial(6);
    DT_ARG_PTR(c, 0, m);
    DT_ARG_INT(c, 1, dt_u32() % 12 == 0 ? 7u : (uint32_t)dt_range(0, 5));
}

static void setup_clone(DtCtx *c)
{
    matBegin(c);
    DT_ARG_PTR(c, 0, newMaterial(6));
}

static void setup_set_texture(DtCtx *c)
{
    matBegin(c);
    DT_ARG_PTR(c, 0, newMaterial(6));
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_madf_flag(DtCtx *c)
{
    ZrwMadf *madf = dt_alloc(sizeof *madf);

    matBegin(c);
    dt_fill(madf, sizeof *madf);
    gZrwMat.madf = dt_u32() % 10 == 0 ? NULL : madf;
}

static void setup_loaded(DtCtx *c)
{
    matBegin(c);
    gZrwMat.last = NULL;
    DT_ARG_INT(c, 0, dt_u32() | 1);
}

/* MATI / MADF chunk readers: scripted chunk functions. */
static uint32_t gChunkSize;
static uint8_t gChunkBytes[0xa0];
static ZrwMaterial *gProcessLast;
static int gProcessHasMadf;

static uint32_t BMX_CDECL sChunkSize(uint32_t iff)
{
    s5Log(0x49abc0);
    s5Log(dt_canon(iff));
    return gChunkSize;
}

static uint32_t BMX_CDECL sChunkRead(uint32_t iff, uint32_t dst)
{
    s5Log(0x49abe0);
    s5Log(dt_canon(iff));
    memcpy((void *)(uintptr_t)dst, gChunkBytes, gChunkSize);
    return gChunkSize;
}

static uint32_t BMX_CDECL sProcess(uint32_t iff, uint32_t arg)
{
    s5Log(0x49a8e0);
    s5Log(dt_canon(iff));
    s5Log(arg);
    gZrwMat.madf = gProcessHasMadf ? &gZrwMadfRec : NULL;
    gZrwMat.last = gProcessLast;
    return 1;
}

static void setup_read_mati(DtCtx *c)
{
    matBegin(c);
    S5_ON(0x49b130, sIffBegin);
    S5_ON(0x49b250, sIffEnd);
    S5_ON(0x49a8e0, sProcess);
    s5SetRet(0x49b130, dt_u32() % 5 != 0);
    dt_fill(&gZrwMadfRec, sizeof gZrwMadfRec);
    gProcessHasMadf = dt_u32() % 4 != 0;
    gProcessLast = dt_u32() % 4 == 0 ? NULL : newMaterial(2);
    gZrwMat.last = newMaterial(2);
    gZrwMat.hook = dt_u32() % 2 ? (void *)materialHook : NULL;
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_read_madf(DtCtx *c)
{
    matBegin(c);
    S5_ON(0x49abc0, sChunkSize);
    S5_ON(0x49abe0, sChunkRead);
    dt_fill(gChunkBytes, sizeof gChunkBytes);
    dt_fill(&gZrwMadfRec, sizeof gZrwMadfRec);
    gZrwMadfRec.name[0x47] = 0;
    gChunkSize = (uint32_t)dt_range(0x48, 0x8f);
    if (dt_u32() % 12 == 0) {
        gChunkSize = (uint32_t)dt_range(0x90, 0x100);
    }
    {
        uint32_t magic = dt_u32() % 6 == 0 ? dt_u32() : 0x48;

        memcpy(gChunkBytes, &magic, 4);
    }
    /* keep the name terminated wherever the chunk ends */
    gChunkBytes[0x47] = 0;
    DT_ARG_INT(c, 0, dt_u32());
}

DT_CASES(zrwmat_cases) = {
    {"zrwmatCreateObject", 0x00488fb0, zrwmatCreateObject, 1, DT_RET_INT, setup_create_object, 400, 0},
    {"zrwmatLoadModel", 0x00489070, zrwmatLoadModel, 1, DT_RET_INT, setup_one_arg, 50, 0},
    {"zrwmatLoadChunk", 0x00489090, zrwmatLoadChunk, 1, DT_RET_INT, setup_one_arg, 50, 0},
    {"zrwmatReleaseModel", 0x004890b0, zrwmatReleaseModel, 1, DT_RET_VOID, setup_one_arg, 50, 0},
    {"zrwmatModelFromObject", 0x004890c0, zrwmatModelFromObject, 1, DT_RET_INT, setup_one_arg, 50, 0},
    {"zrwmatNodeCount", 0x004890d0, zrwmatNodeCount, 1, DT_RET_INT, setup_object_1, 100, 0},
    {"zrwmatGetNodeHashes", 0x004890f0, zrwmatGetNodeHashes, 2, DT_RET_VOID, setup_object_2, 100, 0},
    {"zrwmatFindNode", 0x00489110, zrwmatFindNode, 2, DT_RET_INT, setup_object_2, 100, 0},
    {"zrwmatNodeToWorld", 0x00489150, zrwmatNodeToWorld, 5, DT_RET_VOID, setup_object_4, 100, 0},
    {"zrwmatNodeVector", 0x00489180, zrwmatNodeVector, 4, DT_RET_VOID, setup_object_3, 100, 0},
    {"zrwmatNodeMatrix", 0x004891b0, zrwmatNodeMatrix, 4, DT_RET_VOID, setup_object_3, 100, 0},
    {"zrwmatSetSkeleton", 0x00489210, zrwmatSetSkeleton, 2, DT_RET_VOID, setup_object_2, 100, 0},
    {"zrwmatUpdate", 0x004892a0, zrwmatUpdate, 1, DT_RET_VOID, setup_object_1, 100, 0},
    {"zrwmatReadMATI", 0x004892b0, zrwmatReadMATI, 2, DT_RET_INT, setup_read_mati, 300, 0},
    {"zrwmatReadMADF", 0x004893b0, zrwmatReadMADF, 1, DT_RET_INT, setup_read_madf, 400, 0},
    {"zrwmatInitialize", 0x00489440, zrwmatInitialize, 0, DT_RET_INT, setup_initialize, 100, 0},
    {"zrwmatShutdown", 0x004894d0, zrwmatShutdown, 0, DT_RET_INT, setup_shutdown, 50, 0},
    {"zrwmatCreateRecord", 0x00489500, zrwmatCreateRecord, 0, DT_RET_INT, setup_none, 100, 0},
    {"zrwmatReleaseTexture", 0x00489530, zrwmatReleaseTexture, 1, DT_RET_VOID, setup_release, 300, 0},
    {"zrwmatReplaceTexture", 0x00489590, zrwmatReplaceTexture, 3, DT_RET_VOID, setup_replace, 400, 0},
    {"zrwmatAddTexture", 0x00489610, zrwmatAddTexture, 2, DT_RET_VOID, setup_add, 400, 0},
    {"zrwmatSetColor", 0x00489680, zrwmatSetColor, 3, DT_RET_VOID, setup_set_color, 200, 0},
    {"zrwmatSetMaterialHook", 0x004896c0, zrwmatSetMaterialHook, 1, DT_RET_INT, setup_hook, 50, 0},
    {"zrwmatLoadSpriteTexture", 0x004896d0, zrwmatLoadSpriteTexture, 4, DT_RET_INT, setup_load_sprite, 400,
     0},
    {"zrwmatMatchHash", 0x004899a0, zrwmatMatchHash, 2, DT_RET_INT, setup_match, 400, 0},
    {"zrwmatFindByTextureHash", 0x00489a00, zrwmatFindByTextureHash, 2, DT_RET_INT, setup_find, 400, 0},
    {"zrwmatGetTexture", 0x00489a40, zrwmatGetTexture, 2, DT_RET_INT, setup_get, 300, 0},
    {"zrwmatClone", 0x00489a80, zrwmatClone, 1, DT_RET_INT, setup_clone, 300, 0},
    {"zrwmatRetainMaterial", 0x00489af0, zrwmatRetainMaterial, 1, DT_RET_INT, setup_clone, 300, 0},
    {"zrwmatGetColor", 0x00489b30, zrwmatGetColor, 1, DT_RET_INT, setup_clone, 20, 0},
    {"zrwmatGetRaster", 0x00489b40, zrwmatGetRaster, 2, DT_RET_INT, setup_get_in_range, 300, 0},
    {"zrwmatSetTexture", 0x00489b80, zrwmatSetTexture, 2, DT_RET_VOID, setup_set_texture, 50, 0},
    {"zrwmatTextureLoadBegin", 0x00489ba0, zrwmatTextureLoadBegin, 0, DT_RET_INT, setup_madf_flag, 100, 0},
    {"zrwmatTextureLoaded", 0x00489be0, zrwmatTextureLoaded, 1, DT_RET_VOID, setup_loaded, 300, 0},
};
