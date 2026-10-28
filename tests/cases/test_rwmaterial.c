/*
 * Tests for rw/clump.c, part 3: materials, material lists, the texture and string chunks of a stream, the
 * plugin registry size/copy helpers, bounding boxes and the matrix checks. rwStreamReadString and
 * rwStreamWriteString take register arguments and are reached through RwTextureStreamRead / Write.
 */
#include "dt_rwclump.h"

/* Numbers the size stubs hand out, in order (set in setup, so both sides see the same). */
static int32_t gNumbers[16];
static int gNumberCalls;

static int32_t nextNumber(void)
{
    return gNumbers[gNumberCalls++ & 15];
}

static void scriptNumbers(void)
{
    for (int i = 0; i < 16; i++) {
        gNumbers[i] = dt_u32() % 4 == 0 ? dt_range(-2, 2) : dt_range(0, 200);
    }
    gNumberCalls = 0;
}

static int32_t BMX_CDECL stRegistrySize(const void *registry, const void *object)
{
    rwtLog(0x4e8d60);
    rwtLogPtr(registry);
    rwtLogPtr(object);
    return nextNumber();
}

static int32_t BMX_CDECL stTextureSize(const void *texture)
{
    rwtLog(0x4e9d20);
    rwtLogPtr(texture);
    return nextNumber();
}

static int32_t BMX_CDECL stMaterialSize(const void *material)
{
    rwtLog(0x4b94a0);
    rwtLogPtr(material);
    return nextNumber();
}

static void *BMX_CDECL stMaterialWrite(void *material, void *stream)
{
    rwtLog(0x4b94e0);
    rwtLogPtr(material);
    rwtLogPtr(stream);
    return clFails(kClStubOther) ? NULL : material;
}

static void *BMX_CDECL stTextureWrite(void *texture, void *stream)
{
    rwtLog(0x4e9d90);
    rwtLogPtr(texture);
    rwtLogPtr(stream);
    return clFails(kClStubOther) ? NULL : texture;
}

static RpMaterial *gMats[8];
static int gMatCalls;

static RpMaterial *BMX_CDECL stMaterialRead(void *stream)
{
    rwtLog(0x4b9630);
    rwtLogPtr(stream);
    return clFails(kClStubMesh) ? NULL : gMats[gMatCalls++ & 7];
}

static RwTexture *gTexture;

static RwTexture *BMX_CDECL stTextureStreamRead(void *stream)
{
    rwtLog(0x4e9f20);
    rwtLogPtr(stream);
    return clFails(kClStubMesh) ? NULL : gTexture;
}

static RpMaterial *BMX_CDECL stMaterialAddRef2(RpMaterial *material)
{
    rwtLog(0x4b91f0);
    rwtLogPtr(material);
    return material;
}

static RwTexture *BMX_CDECL stTextureReadByName(const char *name, const char *mask)
{
    rwtLog(0x4dfcb0);
    rwtLog(name != NULL);
    rwtLog(mask != NULL);
    return clFails(kClStubOther) ? NULL : gTexture;
}

static void *BMX_CDECL stSkipChunks(void *registry, void *stream)
{
    rwtLog(0x4e8ed0);
    rwtLogPtr(registry);
    rwtLogPtr(stream);
    return registry;
}

static void clRandomFails2(void)
{
    for (int i = 0; i < 8; i++) {
        gClFailAt[i] = dt_u32() % 6 == 0 ? dt_range(1, 3) : 0;
        gClFailCalls[i] = 0;
    }
}

/* ---- materials -------------------------------------------------------------------------------------- */

static void setup_material(DtCtx *c)
{
    clEnv(c);
    DT_ARG_PTR(c, 0, clMaterial());
    rwtEnd();
}

static void setup_material_destroy(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    dt_stub(0x004df760, NULL, (void *)stTextureDestroy);
    RpMaterial *m = clMaterial();
    m->refCount = (uint16_t)(dt_u32() % 3 == 0 ? dt_u32() : (uint32_t)dt_range(0, 2));
    DT_ARG_PTR(c, 0, m);
    rwtEnd();
}

static void setup_material_size(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    scriptNumbers();
    dt_stub(0x004e8d60, NULL, (void *)stRegistrySize);
    dt_stub(0x004e9d20, NULL, (void *)stTextureSize);
    DT_ARG_PTR(c, 0, clMaterial());
    rwtEnd();
}

static void setup_material_write(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    clWriteStubs();
    scriptNumbers();
    dt_stub(0x004e8d60, NULL, (void *)stRegistrySize);
    dt_stub(0x004e9d20, NULL, (void *)stTextureSize);
    dt_stub(0x004e9d90, NULL, (void *)stTextureWrite);
    gClMaskWord = 2;
    gClFailAt[kClStubOther] = dt_u32() % 6 == 0 ? 1 : 0;
    gClFailCalls[kClStubOther] = 0;
    DT_ARG_PTR(c, 0, clMaterial());
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

static void setup_material_read(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    clStreamScript();
    clRandomFails2();
    gTexture = dt_alloc(0x30);
    dt_stub(0x004e9f20, NULL, (void *)stTextureStreamRead);
    dt_stub(0x004df760, NULL, (void *)stTextureDestroy);
    for (int i = 0; i < 16; i++) {
        gClLengths[i] = dt_u32() % 2 ? 0x1c : 0x10;
    }
    for (int i = 0; i < 1024; i += 4) {
        if (dt_u32() % 3 == 0) {
            gClStream[i] = 0;
        }
    }
    gClAllocFailAt = dt_u32() % 8 == 0 ? 1 : 0;
    DT_ARG_PTR(c, 0, dt_alloc(8));
    rwtEnd();
}

/* ---- material lists --------------------------------------------------------------------------------- */

static void setup_list(DtCtx *c)
{
    clEnv(c);
    RpMaterialList *list = dt_alloc(sizeof *list);
    clMaterialList(list, dt_range(1, 4));
    DT_ARG_PTR(c, 0, list);
    DT_ARG_INT(c, 1, dt_range(0, list->numMaterials - 1));
    rwtEnd();
}

static void setup_list_init(DtCtx *c)
{
    clEnv(c);
    RpMaterialList *list = dt_alloc(sizeof *list);
    dt_fill(list, sizeof *list);
    DT_ARG_PTR(c, 0, list);
    rwtEnd();
}

static void setup_list_deinit(DtCtx *c)
{
    clEnv(c);
    dt_stub(0x004b9290, NULL, (void *)stMaterialDestroy);
    RpMaterialList *list = dt_alloc(sizeof *list);
    if (dt_u32() % 4 == 0) {
        _rpMaterialListInitialize(list);
    } else {
        clMaterialList(list, dt_range(0, 4));
    }
    DT_ARG_PTR(c, 0, list);
    rwtEnd();
}

static void setup_list_size(DtCtx *c)
{
    clEnv(c);
    scriptNumbers();
    dt_stub(0x004b94a0, NULL, (void *)stMaterialSize);
    RpMaterialList *list = dt_alloc(sizeof *list);
    clMaterialList(list, dt_range(0, 5));
    DT_ARG_PTR(c, 0, list);
    rwtEnd();
}

static void setup_list_write(DtCtx *c)
{
    clEnv(c);
    clWriteStubs();
    scriptNumbers();
    dt_stub(0x004b94a0, NULL, (void *)stMaterialSize);
    dt_stub(0x004b94e0, NULL, (void *)stMaterialWrite);
    gClFailAt[kClStubOther] = dt_u32() % 6 == 0 ? dt_range(1, 3) : 0;
    gClFailCalls[kClStubOther] = 0;
    RpMaterialList *list = dt_alloc(sizeof *list);
    clMaterialList(list, dt_range(0, 5));
    DT_ARG_PTR(c, 0, list);
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

static void setup_list_read(DtCtx *c)
{
    clEnv(c);
    clStreamScript();
    clRandomFails2();
    for (int i = 0; i < 16; i++) {
        gClLengths[i] = 4;
    }
    /* the count and the indices: small counts, indices of -1 or into the list so far */
    int32_t *words = (int32_t *)gClStream;
    words[0] = dt_range(0, 5);
    for (int i = 1; i < 12; i++) {
        words[i] = i == 1 || dt_u32() % 2 ? -1 : dt_range(0, 3);
    }
    gMatCalls = 0;
    for (int i = 0; i < 8; i++) {
        gMats[i] = clMaterial();
        gMats[i]->refCount = 1;
    }
    dt_stub(0x004b9630, NULL, (void *)stMaterialRead);
    dt_stub(0x004b91f0, NULL, (void *)stMaterialAddRef2);
    dt_stub(0x004b9290, NULL, (void *)stMaterialDestroy);
    dt_stub(0x004c5640, NULL, (void *)stMaterialDestroy);
    gClOldSize = 0x40;
    gClMallocFailAt =
        dt_u32() % 8 == 0 ? dt_range(3, 4) : 0; /* the grow of the array fails, not the index buffer */
    RpMaterialList *list = dt_alloc(sizeof *list);
    dt_fill(list, sizeof *list);
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_PTR(c, 1, list);
    rwtEnd();
}

/* ---- textures in streams ---------------------------------------------------------------------------- */

static void setup_texture_size(DtCtx *c)
{
    clEnv(c);
    scriptNumbers();
    dt_stub(0x004e8d60, NULL, (void *)stRegistrySize);
    DT_ARG_PTR(c, 0, mkTexture());
    rwtEnd();
}

static void setup_texture_write(DtCtx *c)
{
    clEnv(c);
    clWriteStubs();
    scriptNumbers();
    dt_stub(0x004e8d60, NULL, (void *)stRegistrySize);
    dt_stub(0x004e8db0, NULL, (void *)stWriteChunks);
    clRegistryStubs();
    RwTexture *t = mkTexture();
    t->filterMode = dt_range(0, 300);
    t->addressing = dt_range(0, 300);
    DT_ARG_PTR(c, 0, t);
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

static void setup_texture_read(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    clStreamScript();
    clRandomFails2();
    for (int i = 0; i < 16; i++) {
        gClLengths[i] = 4;
    }
    dt_stub(0x004e2e20, NULL, (void *)stReadChunkHeader);
    dt_stub(0x004e2c50, NULL, (void *)stStreamSkip);
    dt_stub(0x004dfcb0, NULL, (void *)stTextureReadByName);
    dt_stub(0x004e8ed0, NULL, (void *)stSkipChunks);
    gTexture = mkTexture();
    gTexture->refCount = dt_u32() % 2 ? 1 : 2;
    gClStream[0] = (uint8_t)dt_range(0, 8); /* filter mode: 3..6 turn the lookup flags on */
    gClStream[1] = (uint8_t)dt_range(0, 3);
    gClStream[2] = (uint8_t)(dt_u32() % 2);
    gClStream[3] = 0;
    texGlobals()->mipmapping = dt_range(0, 1);
    texGlobals()->findCallBack = (RwTextureCallBackFind)(uintptr_t)(dt_u32() % 2);
    gClMallocFailAt = dt_u32() % 8 == 0 ? dt_range(1, 4) : 0;
    DT_ARG_PTR(c, 0, dt_alloc(8));
    rwtEnd();
}

static void setup_texture_getter(DtCtx *c)
{
    clEnv(c);
    texGlobals()->mipmapping = (int32_t)dt_u32();
    DT_ARG_PTR(c, 0, mkTexture());
    rwtEnd();
}

/* ---- bounding boxes and matrices -------------------------------------------------------------------- */

static float clCoordinate(void)
{
    const uint32_t kind = dt_u32() % 10;
    if (kind == 0) {
        return __builtin_nanf("");
    }
    if (kind == 1) {
        return 0.0f;
    }
    return dt_float_edgy(-50.0f, 50.0f);
}

static void setup_bbox_init(DtCtx *c)
{
    clEnv(c);
    RwBBox *b = dt_alloc(sizeof *b);
    dt_fill(b, sizeof *b);
    ZVec3 *p = dt_alloc(sizeof *p);
    p->x = clCoordinate();
    p->y = clCoordinate();
    p->z = clCoordinate();
    DT_ARG_PTR(c, 0, b);
    DT_ARG_PTR(c, 1, p);
    rwtEnd();
}

static void setup_bbox_add(DtCtx *c)
{
    clEnv(c);
    RwBBox *b = dt_alloc(sizeof *b);
    float *f = (float *)b;
    for (int i = 0; i < 6; i++) {
        f[i] = clCoordinate();
    }
    ZVec3 *p = dt_alloc(sizeof *p);
    p->x = dt_u32() % 4 == 0 ? f[dt_u32() % 6] : clCoordinate();
    p->y = clCoordinate();
    p->z = clCoordinate();
    DT_ARG_PTR(c, 0, b);
    DT_ARG_PTR(c, 1, p);
    rwtEnd();
}

static void setup_matrix(DtCtx *c)
{
    clEnv(c);
    float *m = dt_alloc(sizeof(RwMatrix));
    for (int i = 0; i < 16; i++) {
        m[i] = dt_u32() % 5 == 0 ? (i % 5 == 0 ? 1.0f : 0.0f) : dt_float_edgy(-3.0f, 3.0f);
    }
    DT_ARG_PTR(c, 0, m);
    rwtEnd();
}

/* ---- registry helpers ------------------------------------------------------------------------------- */

static void *BMX_CDECL clCopyCb(void *dst, const void *src, int32_t offset, int32_t size)
{
    rwtLogPtr(dst);
    rwtLogPtr(src);
    rwtLog((uint32_t)offset);
    rwtLog((uint32_t)size);
    return dst;
}

static void setup_registry_copy(DtCtx *c)
{
    clEnv(c);
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 3), 0x100);
    for (RwPluginRegEntry *e = r->firstRegEntry; e != NULL; e = e->nextRegEntry) {
        e->copyCB = clCopyCb;
    }
    DT_ARG_PTR(c, 0, r);
    DT_ARG_PTR(c, 1, dt_alloc(8));
    DT_ARG_PTR(c, 2, dt_alloc(8));
    rwtEnd();
}

static void setup_registry_size(DtCtx *c)
{
    clEnv(c);
    DT_ARG_PTR(c, 0, rwtRegistry(dt_range(0, 4), 0x200));
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

DT_CASES(rwmaterial_cases) = {
    {"RpMaterialAddRef", 0x004b91f0, RpMaterialAddRef, 1, DT_RET_INT, setup_material, 60, 0},
    {"RpMaterialDestroy", 0x004b9290, RpMaterialDestroy, 1, DT_RET_INT, setup_material_destroy, 150, 0},
    {"RpMaterialStreamGetSize", 0x004b94a0, RpMaterialStreamGetSize, 1, DT_RET_INT, setup_material_size, 100,
     0},
    {"RpMaterialStreamWrite", 0x004b94e0, RpMaterialStreamWrite, 2, DT_RET_INT, setup_material_write, 300, 0},
    {"RpMaterialStreamRead", 0x004b9630, RpMaterialStreamRead, 1, DT_RET_INT, setup_material_read, 600, 0},
    {"_rpMaterialListGetMaterial", 0x004c4f90, _rpMaterialListGetMaterial, 2, DT_RET_INT, setup_list, 60, 0},
    {"_rpMaterialListInitialize", 0x004c56a0, _rpMaterialListInitialize, 1, DT_RET_INT, setup_list_init, 40,
     0},
    {"_rpMaterialListDeinitialize", 0x004c5640, _rpMaterialListDeinitialize, 1, DT_RET_INT, setup_list_deinit,
     100, 0},
    {"_rpMaterialListStreamGetSize", 0x004c50e0, _rpMaterialListStreamGetSize, 1, DT_RET_INT, setup_list_size,
     150, 0},
    {"_rpMaterialListStreamWrite", 0x004c5160, _rpMaterialListStreamWrite, 2, DT_RET_INT, setup_list_write,
     300, 0},
    {"_rpMaterialListStreamRead", 0x004c5330, _rpMaterialListStreamRead, 2, DT_RET_INT, setup_list_read, 600,
     0},
    {"RwTextureStreamGetSize", 0x004e9d20, RwTextureStreamGetSize, 1, DT_RET_INT, setup_texture_size, 100, 0},
    {"RwTextureStreamWrite", 0x004e9d90, RwTextureStreamWrite, 2, DT_RET_INT, setup_texture_write, 300, 0},
    {"RwTextureStreamRead", 0x004e9f20, RwTextureStreamRead, 1, DT_RET_INT, setup_texture_read, 800, 0},
    {"RwTextureGetMipmapping", 0x004df5a0, RwTextureGetMipmapping, 0, DT_RET_INT, setup_texture_getter, 20,
     0},
    {"RwTextureGetFilterMode", 0x004dff90, RwTextureGetFilterMode, 1, DT_RET_INT, setup_texture_getter, 30,
     0},
    {"RwTextureGetAddressing", 0x004dffc0, RwTextureGetAddressing, 1, DT_RET_INT, setup_texture_getter, 30,
     0},
    {"rwBBoxInitialize", 0x004e9900, rwBBoxInitialize, 2, DT_RET_VOID, setup_bbox_init, 80, 0},
    {"rwBBoxAddPoint", 0x004e9810, rwBBoxAddPoint, 2, DT_RET_INT, setup_bbox_add, 400, 0},
    {"rwMatrixDeterminant", 0x004d0470, rwMatrixDeterminant, 1, DT_RET_DOUBLE, setup_matrix, 400, 0},
    {"rwMatrixNormalError", 0x004d2190, rwMatrixNormalError, 1, DT_RET_DOUBLE, setup_matrix, 400, 0},
    {"rwMatrixOrthogonalError", 0x004d2210, rwMatrixOrthogonalError, 1, DT_RET_DOUBLE, setup_matrix, 400, 0},
    {"_rwPluginRegistryCopyObject", 0x004e8a60, _rwPluginRegistryCopyObject, 3, DT_RET_INT,
     setup_registry_copy, 100, 0},
    {"_rwPluginRegistryGetSize", 0x004e8d60, _rwPluginRegistryGetSize, 2, DT_RET_INT, setup_registry_size,
     100, 0},
};
