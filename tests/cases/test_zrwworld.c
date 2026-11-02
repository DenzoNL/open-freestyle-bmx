/*
 * Tests for zrwworld.c, zrwcam.c and zrwshadow.c: the effect object, world loading and statistics, the camera
 * rasters and the shadow start-up. RenderWare, the IFF reader, files and the update lists are stubs that log
 * their arguments.
 */
#include "dt.h"
#include "dt_s5.h"
#include "dt_zmem.h"
#include "rw/rwworld.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/com/zglrw/zrwmodelcreate.h"
#include "zge/com/zglrw/zrwshadow.h"
#include "zge/com/zglrw/zrwtxr.h"
#include "zge/com/zglrw/zrwworld.h"
#include "zge/zobj.h"

#define gWorldName ORIG_VAR(0x00674208, char, gWorldName)
#define gClearRec  ORIG_VAR(0x00697730, uint32_t, gClearRec)
#define gCamReady  ORIG_VAR(0x00674018, int, gCamReady)
#define gCamPool   ORIG_VAR(0x0067401c, void *, gCamPool)
#define gCamRaster ORIG_VAR(0x006976c0, void *, gCamRaster)
#define gZRaster   ORIG_VAR(0x006976c4, void *, gZRaster)
#define gCamWidth  ORIG_VAR(0x00697770, int, gCamWidth)
#define gCamHeight ORIG_VAR(0x00697774, int, gCamHeight)

/* Print that also logs %f arguments (as the two words of the double) and %s by hash. */
static void BMX_CDECL sPrint(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    s5Log(0x480240);
    s5Log(s5Hash(fmt));
    for (const char *p = fmt; *p; p++) {
        if (p[0] != '%') {
            continue;
        }
        const char *spec = p + 1;

        while (*spec >= '0' && *spec <= '9') {
            spec++;
        }
        if (spec[0] == 's') {
            s5Log(s5Hash(va_arg(ap, const char *)));
        } else if (spec[0] == 'd' || spec[0] == 'x') {
            s5Log((uint32_t)va_arg(ap, int));
        } else if (spec[0] == 'f') {
            double d = va_arg(ap, double);
            uint32_t w[2];

            memcpy(w, &d, 8);
            s5Log(w[0]);
            s5Log(w[1]);
        }
    }
    va_end(ap);
}

static void begin(DtCtx *c, int stubFail)
{
    s5Begin(c, stubFail);
    gZdebugPrint = sPrint;
}

static void *handle(void)
{
    return (void *)(uintptr_t)(0x34000000u + (dt_u32() & 0xfffffu) * 16u);
}

/* ---- effect object ----------------------------------------------------------------------------------- */
S5_STUB0(sObjAlloc, 0x487370)
S5_STUB2(sObjInit, 0x4873a0)
S5_STUB1(sMemAlloc, 0x485aa0)
S5_STUB1(sListCreate, 0x48c260)
S5_STUB4(sRegister, 0x48ba30)
S5_STUB1(sNameFind, 0x482da0)

static int BMX_CDECL effectRender(const ZfxEffect *type, ZfxEmitter *item, int arg)
{
    s5Log(0xe0);
    s5LogPtr(type);
    s5LogPtr(item);
    s5Log((uint32_t)arg);
    return 0;
}

static ZfxEmitter *BMX_CDECL effectCreate(ZfxEffect *type, uint16_t size)
{
    s5Log(0xe1);
    s5LogPtr(type);
    s5Log((uint32_t)size);
    return (ZfxEmitter *)(uintptr_t)s5Ret(0xe1);
}

static void BMX_CDECL effectApply(const ZfxEffect *type, const void *object, ZfxEmitter *item, uint32_t flags,
                                  void *a, const float *b)
{
    s5Log(0xe2);
    s5LogPtr(type);
    s5LogPtr(object);
    s5LogPtr(item);
    s5Log(flags);
    s5Log((uint32_t)(uintptr_t)a);
    s5Log((uint32_t)(uintptr_t)b);
}

static ZfxEffect *newType(void)
{
    ZfxEffect *t = dt_alloc(sizeof *t);

    dt_fill(t, sizeof *t);
    t->create = effectCreate;
    t->spawn = effectApply;
    t->render = effectRender;
    return t;
}

/* An effect record: only the effect pointer at +0x2c is read. */
static ZfxEmitter *newItem(ZfxEffect *type)
{
    ZfxEmitter *item = dt_alloc(0x40);

    dt_fill(item, 0x40);
    item->effect = type;
    return item;
}

static ZObj *newHostObject(ZfxEffect **types, int n)
{
    ZObj *o = dt_alloc(sizeof *o);
    ZobjPEmitState *host = dt_alloc(sizeof *host);

    dt_fill(o, sizeof *o);
    dt_fill(host, sizeof *host);
    host->items = S5_LIST_CREATE(8);
    for (int i = 0; i < n; i++) {
        S5_LIST_INSERT(host->items, newItem(types[i]));
    }
    o->data = host;
    return o;
}

static void setup_update_effects(DtCtx *c)
{
    ZfxEffect *types[4];
    int n = dt_range(0, 4);

    begin(c, 0);
    for (int i = 0; i < n; i++) {
        types[i] = newType();
    }
    DT_ARG_PTR(c, 0, newHostObject(types, n));
}

static void setup_create_object(DtCtx *c)
{
    ZObj *o = dt_alloc(sizeof *o);
    void *host = dt_alloc(0x3c);

    begin(c, 0);
    dt_fill(o, sizeof *o);
    dt_fill(host, 0x3c);
    S5_ON(0x487370, sObjAlloc);
    S5_ON(0x4873a0, sObjInit);
    S5_ON(0x485aa0, sMemAlloc);
    S5_ON(0x48c260, sListCreate);
    S5_ON(0x48ba30, sRegister);
    s5SetRet(0x487370, dt_u32() % 6 == 0 ? 0 : (uint32_t)(uintptr_t)o);
    s5SetRet(0x485aa0, (uint32_t)(uintptr_t)host);
    s5SetRet(0x48c260, (uint32_t)(uintptr_t)handle());
    s5SetRet(0x48ba30, (uint32_t)(uintptr_t)handle());
}

static void setup_apply_effect(DtCtx *c)
{
    ZfxEffect *types[4];
    ZfxEffect *wanted = newType();
    int n = dt_range(0, 3);

    begin(c, 0);
    S5_ON(0x482da0, sNameFind);
    for (int i = 0; i < n; i++) {
        types[i] = dt_u32() % 2 ? wanted : newType();
    }
    types[n] = dt_u32() % 3 == 0 ? wanted : newType();
    n++;
    s5SetRet(0x482da0, dt_u32() % 8 == 0 ? 0 : (uint32_t)(uintptr_t)wanted);
    s5SetRet(0xe1, dt_u32() % 4 == 0 ? 0 : (uint32_t)(uintptr_t)newItem(wanted));
    DT_ARG_PTR(c, 0, newHostObject(types, n));
    DT_ARG_PTR(c, 1, dt_rand_str(8, 0));
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
    DT_ARG_INT(c, 4, dt_u32());
}

/* ---- statistics -------------------------------------------------------------------------------------- */
S5_STUB1(sSectorPolys, 0x4c2060)
S5_STUB1(sSectorVerts, 0x4c2050)
S5_STUB1(sGeomTris, 0x4b4720)
S5_STUB1(sGeomVerts, 0x4b4550)
S5_STUB3(sClumpAtoms, 0x4b1990)
S5_STUB3(sForLights, 0x4bba60)
S5_STUB3(sForSectors, 0x4bbb80)
S5_STUB3(sForClumps, 0x4bb7d0)
S5_STUB1(sNumMaterials, 0x4ba0c0)
S5_STUB2(sGetMaterial, 0x4ba140)
S5_STUB1(sMaterialTexture, 0x4b93d0)
S5_STUB1(sNumClumps, 0x4ba1f0)

static const char *gTexNames[3] = {"tex_a", "wall", "x"};

static uint32_t BMX_CDECL sTextureName(uint32_t texture)
{
    s5Log(0x4df990);
    s5Log(dt_canon(texture));
    return (uint32_t)(uintptr_t)gTexNames[texture % 3];
}

static void statsStubs(void)
{
    S5_ON(0x4c2060, sSectorPolys);
    S5_ON(0x4c2050, sSectorVerts);
    S5_ON(0x4b4720, sGeomTris);
    S5_ON(0x4b4550, sGeomVerts);
    S5_ON(0x4b1990, sClumpAtoms);
    S5_ON(0x4bba60, sForLights);
    S5_ON(0x4bbb80, sForSectors);
    S5_ON(0x4bb7d0, sForClumps);
    S5_ON(0x4ba0c0, sNumMaterials);
    S5_ON(0x4ba140, sGetMaterial);
    S5_ON(0x4b93d0, sMaterialTexture);
    S5_ON(0x4df990, sTextureName);
    S5_ON(0x4ba1f0, sNumClumps);
    s5SetRet(0x4c2060, dt_u32() % 1000);
    s5SetRet(0x4c2050, dt_u32() % 1000);
    s5SetRet(0x4b4720, dt_u32() % 1000);
    s5SetRet(0x4b4550, dt_u32() % 1000);
    s5SetRet(0x4ba0c0, (uint32_t)dt_range(0, 4));
    s5SetRet(0x4ba140, dt_u32() % 4 == 0 ? 0 : (uint32_t)(uintptr_t)handle());
    s5SetRet(0x4b93d0, dt_u32() % 4 == 0 ? 0 : (uint32_t)(uintptr_t)handle());
    s5SetRet(0x4ba1f0, dt_u32() % 50);
    gZrwWorld = handle();
    dt_fill(&gZrwWorldStats, sizeof gZrwWorldStats);
    memset(&gWorldName, 'n', 0x20);
    (&gWorldName)[0x20] = 0;
}

static void setup_count_sector(DtCtx *c)
{
    begin(c, 0);
    S5_ON(0x4c2060, sSectorPolys);
    S5_ON(0x4c2050, sSectorVerts);
    s5SetRet(0x4c2060, dt_u32());
    s5SetRet(0x4c2050, dt_u32());
    dt_fill(&gZrwWorldStats, sizeof gZrwWorldStats);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_count_atomic(DtCtx *c)
{
    void *atomic = dt_alloc(0x20);

    begin(c, 0);
    S5_ON(0x4b4720, sGeomTris);
    S5_ON(0x4b4550, sGeomVerts);
    s5SetRet(0x4b4720, dt_u32());
    s5SetRet(0x4b4550, dt_u32());
    dt_fill(&gZrwWorldStats, sizeof gZrwWorldStats);
    dt_fill(atomic, 0x20);
    DT_ARG_PTR(c, 0, atomic);
}

static void setup_count_clump(DtCtx *c)
{
    begin(c, 0);
    S5_ON(0x4b1990, sClumpAtoms);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_log_info(DtCtx *c)
{
    begin(c, 0);
    statsStubs();
}

static float gColor[3];

static uint32_t BMX_CDECL sLightType(uint32_t light)
{
    s5Log(0x4c2bf0);
    s5Log(dt_canon(light));
    return s5Ret(0x4c2bf0);
}

static uint32_t BMX_CDECL sLightColor(uint32_t light)
{
    s5Log(0x4c2560);
    s5Log(dt_canon(light));
    return (uint32_t)(uintptr_t)gColor;
}

S5_STUB1(sLightFlags, 0x4c2c20)

static void setup_log_light(DtCtx *c)
{
    begin(c, 0);
    S5_ON(0x4c2bf0, sLightType);
    S5_ON(0x4c2560, sLightColor);
    S5_ON(0x4c2c20, sLightFlags);
    s5SetRet(0x4c2bf0, (uint32_t)dt_range(0, 9));
    s5SetRet(0x4c2c20, (uint32_t)dt_range(0, 3));
    for (int i = 0; i < 3; i++) {
        gColor[i] = dt_float_edgy(-2.0f, 2.0f);
    }
    gZrwWorldStats.lightIndex = (uint32_t)dt_range(0, 20);
    DT_ARG_INT(c, 0, dt_u32());
}

/* ---- scene info lookup ------------------------------------------------------------------------------ */
S5_STUB2(sRamDisk, 0x485580)
S5_STUB3(sIffOpen, 0x49a790)
S5_STUB1(sIffClose, 0x49a8a0)
S5_STUB2(sFileClose, 0x484a60)

static int gScifLeft;
static int gScifSize;
static uint32_t gScifKinds[4];
static int gScifIndex;

static uint32_t BMX_CDECL sFindScif(uint32_t iff, uint32_t id)
{
    s5Log(0x49ad90);
    s5Log(dt_canon(iff));
    s5Log(id);
    gScifIndex = 0;
    return gScifLeft > 0;
}

static uint32_t BMX_CDECL sNextScif(uint32_t iff, uint32_t id)
{
    s5Log(0x49aed0);
    s5Log(dt_canon(iff));
    s5Log(id);
    gScifIndex++;
    return gScifIndex < gScifLeft;
}

static uint32_t BMX_CDECL sScifSize(uint32_t iff)
{
    s5Log(0x49abc0);
    s5Log(dt_canon(iff));
    return (uint32_t)gScifSize;
}

static uint32_t BMX_CDECL sScifRead(uint32_t iff, uint32_t dst)
{
    uint32_t record[10];

    s5Log(0x49abe0);
    s5Log(dt_canon(iff));
    for (int i = 0; i < 10; i++) {
        record[i] = 0x20202020u + (uint32_t)i;
    }
    record[0] = gScifKinds[gScifIndex & 3];
    record[1] = 0x1234u + (uint32_t)gScifIndex;
    record[9] = 0;
    memcpy((void *)(uintptr_t)dst, record, 0x28);
    return 0x28;
}

static void setup_find_record(DtCtx *c)
{
    uint32_t *image = dt_alloc(8);

    begin(c, 0);
    S5_ON(0x485580, sRamDisk);
    S5_ON(0x49a790, sIffOpen);
    S5_ON(0x49a8a0, sIffClose);
    S5_ON(0x484a60, sFileClose);
    S5_ON(0x49ad90, sFindScif);
    S5_ON(0x49aed0, sNextScif);
    S5_ON(0x49abc0, sScifSize);
    S5_ON(0x49abe0, sScifRead);
    s5SetRet(0x485580, dt_u32() % 6 == 0 ? 0 : (uint32_t)(uintptr_t)handle());
    s5SetRet(0x49a790, dt_u32() % 6 == 0 ? 0 : (uint32_t)(uintptr_t)handle());
    gScifLeft = dt_range(0, 3);
    gScifSize = 0x28;
    for (int i = 0; i < 4; i++) {
        gScifKinds[i] = (uint32_t)dt_range(1, 5) | (dt_u32() & 0xffffff00u);
    }
    dt_fill(&gZrwWorldRecord, sizeof gZrwWorldRecord);
    image[0] = dt_u32();
    image[1] = dt_u32();
    DT_ARG_PTR(c, 0, image);
    DT_ARG_INT(c, 1, (uint32_t)dt_range(1, 5));
    DT_ARG_PTR(c, 2, dt_u32() % 3 == 0 ? NULL : dt_alloc(4));
}

S5_STUB1(sRunUpdateList, 0x48bb20)

static void setup_add_material(DtCtx *c)
{
    begin(c, 0);
    S5_ON(0x48bb20, sRunUpdateList);
    gZrwWorldMaterials = S5_LIST_CREATE(4);
    DT_ARG_INT(c, 0, dt_u32() | 1);
}

/* ---- PVS progress ----------------------------------------------------------------------------------- */
S5_STUB0(sCurrentCamera, 0x492eb0)
S5_STUB1(sCameraHandle, 0x493060)
S5_STUB3(sCameraClear, 0x4de8a0)
S5_STUB1(sCamBegin, 0x492f00)
S5_STUB0(sCamEnd, 0x492fe0)
S5_STUB1(sShowRaster, 0x48b950)
S5_STUB3(sDbgDirect, 0x47f640)

static char gFormatBuffer[] = "Percent text";

static uint32_t BMX_CDECL sFormatTemp(const char *fmt, ...)
{
    va_list ap;
    double d;
    uint32_t w[2];

    va_start(ap, fmt);
    d = va_arg(ap, double);
    va_end(ap);
    memcpy(w, &d, 8);
    s5Log(0x482860);
    s5Log(s5Hash(fmt));
    s5Log(w[0]);
    s5Log(w[1]);
    return (uint32_t)(uintptr_t)gFormatBuffer;
}

static void setup_pvs_callback(DtCtx *c)
{
    begin(c, 0);
    S5_ON(0x492eb0, sCurrentCamera);
    S5_ON(0x493060, sCameraHandle);
    S5_ON(0x4de8a0, sCameraClear);
    S5_ON(0x492f00, sCamBegin);
    S5_ON(0x492fe0, sCamEnd);
    S5_ON(0x48b950, sShowRaster);
    S5_ON(0x47f640, sDbgDirect);
    S5_ON(0x482860, sFormatTemp);
    s5SetRet(0x492eb0, dt_u32() % 5 == 0 ? 0 : (uint32_t)(uintptr_t)handle());
    s5SetRet(0x493060, (uint32_t)(uintptr_t)handle());
    s5SetRet(0x492f00, dt_u32() % 3 != 0);
    s5SetRet(0x492fe0, dt_u32() % 3 != 0);
    gClearRec = dt_u32();
    DT_ARG_INT(c, 0, dt_u32() % 4 == 0 ? 3u : 0xcu);
    DT_ARG_FLOAT(c, 1, dt_float_edgy(0.0f, 100.0f));
}

/* ---- loading and creating worlds -------------------------------------------------------------------- */
S5_STUB1(sStreamRead, 0x4c3e60)
S5_STUB2(sStreamClose, 0x4e2660)
S5_STUB4(sStreamFind, 0x4e2ec0)
S5_STUB1(sWorldDestroy, 0x4ba9c0)
S5_STUB1(sPvsReady, 0x4cc640)
S5_STUB1(sPvsUnload, 0x4cc320)
S5_STUB1(sResources, 0x4dc6e0)
S5_STUB0(sDefaultDevice, 0x483f10)

static float gOrigin[3];
static float gBox[6];
static float gBounds[8];
static int gHaveWorld;

static uint32_t BMX_CDECL sWorldCreate(const float *box)
{
    s5Log(0x4bb080);
    s5LogWords(box, 6);
    return gHaveWorld ? (uint32_t)(uintptr_t)handle() : 0;
}

static uint32_t BMX_CDECL sWorldOrigin(uint32_t world)
{
    s5Log(0x4ba050);
    s5Log(dt_canon(world));
    return (uint32_t)(uintptr_t)gOrigin;
}

static uint32_t BMX_CDECL sWorldBBox(uint32_t world)
{
    s5Log(0x4b9fe0);
    s5Log(dt_canon(world));
    return (uint32_t)(uintptr_t)gBox;
}

static uint32_t BMX_CDECL sSetBounds(uint32_t mode, const float *bounds, uint32_t flags)
{
    s5Log(0x48ab10);
    s5Log(mode);
    s5LogWords(bounds, 3);
    s5LogWords(bounds + 4, 3);
    s5Log(flags);
    (void)gBounds;
    return 1;
}

static uint32_t BMX_CDECL sStreamOpen(uint32_t type, uint32_t access, const char *path)
{
    s5Log(0x4e23e0);
    s5Log(type);
    s5Log(access);
    s5Log(s5Hash(path));
    return s5Ret(0x4e23e0);
}

static char gResolved[] = "C:\\levels\\scene.rws";

static uint32_t BMX_CDECL sDeviceControl(uint32_t device, uint32_t command, uint32_t a, const char **b)
{
    s5Log(0x483d30);
    s5Log(dt_canon(device));
    s5Log(command);
    s5Log(dt_canon(a));
    s5Log(s5Hash(*b));
    return s5Ret(0x483d30);
}

static uint32_t BMX_CDECL sFindWorldRecord(uint32_t image, uint32_t kind, uint32_t *size)
{
    s5Log(0x4962e0);
    s5Log(dt_canon(image));
    s5Log(kind);
    if (size != NULL) {
        *size = s5Ret(0x4962e1);
    }
    return s5Ret(0x4962e0);
}

static void worldStubs(void)
{
    gHaveWorld = dt_u32() % 5 != 0;
    for (int i = 0; i < 3; i++) {
        gOrigin[i] = s5Rf();
    }
    for (int i = 0; i < 6; i++) {
        gBox[i] = s5Rf();
    }
    S5_ON(0x48c260, sListCreate);
    S5_ON(0x4bb080, sWorldCreate);
    S5_ON(0x4ba050, sWorldOrigin);
    S5_ON(0x4b9fe0, sWorldBBox);
    S5_ON(0x48ab10, sSetBounds);
    S5_ON(0x4ba9c0, sWorldDestroy);
    S5_ON(0x4cc640, sPvsReady);
    dt_stub(0x4cc320, (void *)RpPVSUnload, (void *)sPvsUnload);
    S5_ON(0x4dc6e0, sResources);
    s5SetRet(0x48c260, (uint32_t)(uintptr_t)handle());
    s5SetRet(0x4cc640, dt_u32() % 2);
    gZrwInvScale = dt_float_edgy(-5.0f, 5.0f);
    gZrwWorld = dt_u32() % 4 == 0 ? NULL : handle();
    gZrwPvsLoaded = (int)(dt_u32() % 2);
    gZrwPvsEnabled = (int)(dt_u32() % 2);
    dt_fill(&gZrwWorldCenter, sizeof gZrwWorldCenter);
    dt_fill(&gZrwWorldCenterScaled, sizeof gZrwWorldCenterScaled);
}

static void setup_create_world(DtCtx *c)
{
    begin(c, 0);
    worldStubs();
    memset(&gZrwWorldReady, 0, 4);
    gZrwWorldReady = dt_u32() % 8 == 0;
}

static ZList *materialList(int n)
{
    ZList *list = S5_LIST_CREATE(8);

    for (int i = 0; i < n; i++) {
        S5_LIST_INSERT(list, dt_alloc(0x80));
    }
    return list;
}

S5_STUB2(sMatTexture, 0x489a40)
S5_STUB1(sTexDrop, 0x498810)
S5_STUB1(sMatRelease, 0x489530)

static void setup_shutdown(DtCtx *c)
{
    begin(c, 0);
    worldStubs();
    S5_ON(0x489a40, sMatTexture);
    S5_ON(0x498810, sTexDrop);
    S5_ON(0x489530, sMatRelease);
    gZrwWorldReady = dt_u32() % 4 != 0;
    gZrwWorldActive = (int)(dt_u32() % 3 != 0);
    gZrwWorldMaterials = dt_u32() % 5 == 0 ? NULL : materialList(dt_range(0, 3));
    if (gZrwWorldMaterials == NULL) {
        gZrwWorldReady = gZrwWorldReady && gZrwWorldActive == 0;
    }
    s5SetRet(0x489a40, (uint32_t)(uintptr_t)handle());
    if (gZrwWorldReady != 0 && gZrwWorldMaterials == NULL) {
        gZrwWorldMaterials = materialList(0);
    }
}

static void setup_read_stream(DtCtx *c)
{
    begin(c, 0);
    worldStubs();
    statsStubs();
    S5_ON(0x4962e0, sFindWorldRecord);
    S5_ON(0x483f10, sDefaultDevice);
    S5_ON(0x483d30, sDeviceControl);
    S5_ON(0x4e23e0, sStreamOpen);
    S5_ON(0x4e2ec0, sStreamFind);
    S5_ON(0x4c3e60, sStreamRead);
    S5_ON(0x4e2660, sStreamClose);
    s5SetRet(0x4962e0, dt_u32() % 6 == 0 ? 0 : (uint32_t)(uintptr_t)dt_rand_str(8, 0) - 8);
    s5SetRet(0x483f10, (uint32_t)(uintptr_t)handle());
    s5SetRet(0x483d30, dt_u32() % 6 == 0 ? 0 : (uint32_t)(uintptr_t)gResolved);
    s5SetRet(0x4e23e0, dt_u32() % 6 == 0 ? 0 : (uint32_t)(uintptr_t)handle());
    s5SetRet(0x4e2ec0, dt_u32() % 5 != 0);
    s5SetRet(0x4c3e60, dt_u32() % 5 == 0 ? 0 : (uint32_t)(uintptr_t)handle());
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_PTR(c, 1, dt_rand_str(8, 0));
}

S5_STUB3(sZfileOpen, 0x484700)
S5_STUB3(sIffProcessOpen, 0x49a790)
S5_STUB2(sIffProcess, 0x49a8e0)
S5_STUB2(sReadStream, 0x4966e0)
S5_STUB2(sSetMatTexture, 0x489b80)
S5_STUB1(sTextureRw, 0x498ac0)
S5_STUB1(sTexNamePtr, 0x4988a0)
S5_STUB1(sHookRet, 0x4896c0)

static void setup_load(DtCtx *c)
{
    begin(c, 0);
    statsStubs();
    S5_ON(0x4962e0, sFindWorldRecord);
    S5_ON(0x484700, sZfileOpen);
    S5_ON(0x49a790, sIffProcessOpen);
    S5_ON(0x49a8e0, sIffProcess);
    S5_ON(0x49a8a0, sIffClose);
    S5_ON(0x484a60, sFileClose);
    S5_ON(0x4966e0, sReadStream);
    S5_ON(0x489a40, sMatTexture);
    S5_ON(0x489b80, sSetMatTexture);
    S5_ON(0x498ac0, sTextureRw);
    S5_ON(0x4896c0, sHookRet);
    S5_ON(0x4b93d0, sMaterialTexture);
    S5_ON(0x4ba140, sGetMaterial);
    S5_ON(0x4ba0c0, sNumMaterials);
    /* the texture name pointer of the exe returns a string */
    S5_ON(0x4988a0, sTexNamePtr);
    s5SetRet(0x4988a0, (uint32_t)(uintptr_t)gFormatBuffer);
    gZrwWorldActive = dt_u32() % 10 == 0;
    s5SetRet(0x4962e0, dt_u32() % 6 == 0 ? 0 : (uint32_t)(uintptr_t)dt_rand_str(8, 0) - 8);
    s5SetRet(0x4962e1, dt_u32() % 3 == 0 ? 0 : dt_u32() % 100 + 1);
    s5SetRet(0x484700, (uint32_t)(uintptr_t)handle());
    s5SetRet(0x49a790, dt_u32() % 4 == 0 ? 0 : (uint32_t)(uintptr_t)handle());
    s5SetRet(0x4966e0, dt_u32() % 4 != 0);
    s5SetRet(0x489a40, (uint32_t)(uintptr_t)handle());
    s5SetRet(0x498ac0, (uint32_t)(uintptr_t)handle());
    gZrwWorldMaterials = dt_u32() % 4 == 0 ? NULL : materialList(dt_range(0, 3));
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_PTR(c, 1, dt_rand_str(8, 0));
}

/* ---- cameras ---------------------------------------------------------------------------------------- */
S5_STUB3(sPoolCreate, 0x485e80)
S5_STUB1(sFirstNode, 0x486250)
S5_STUB1(sNodePayload, 0x486270)
S5_STUB1(sRasterDestroy, 0x4dcbd0)
S5_STUB4(sRasterCreate, 0x4dcb10)
S5_STUB0(sShadowStart, 0x49b330)
S5_STUB0(sShadowStop, 0x49b4e0)
S5_STUB1(sPoolDestroy, 0x485fb0)

static void camStubs(void)
{
    S5_ON(0x485e80, sPoolCreate);
    S5_ON(0x486250, sFirstNode);
    S5_ON(0x486270, sNodePayload);
    S5_ON(0x4dcbd0, sRasterDestroy);
    S5_ON(0x4dcb10, sRasterCreate);
    S5_ON(0x49b330, sShadowStart);
    S5_ON(0x49b4e0, sShadowStop);
    S5_ON(0x485fb0, sPoolDestroy);
    s5SetRet(0x485e80, (uint32_t)(uintptr_t)handle());
    s5SetRet(0x486250, dt_u32() % 6 == 0 ? (uint32_t)(uintptr_t)handle() : 0);
    s5SetRet(0x486270, dt_u32() % 8 == 0 ? 0 : (uint32_t)(uintptr_t)handle());
    s5SetRet(0x4dcb10, dt_u32() % 5 == 0 ? 0 : (uint32_t)(uintptr_t)handle());
    s5SetRet(0x49b330, dt_u32() % 4 != 0);
    s5SetRet(0x49b4e0, dt_u32() % 4 != 0);
    gCamReady = 0;
    gCamRaster = dt_u32() % 2 ? handle() : NULL;
    gZRaster = dt_u32() % 2 ? handle() : NULL;
    gCamWidth = dt_range(1, 2000);
    gCamHeight = dt_range(1, 2000);
    gCamPool = handle();
}

static void setup_find_marker(DtCtx *c)
{
    ZrwMarker *head = NULL;
    int n = dt_range(0, 4);
    int hash = dt_range(0, 3);
    int index = dt_range(0, 3);

    begin(c, 0);
    for (int i = 0; i < n; i++) {
        ZrwMarker *m = dt_alloc(sizeof *m);

        dt_fill(m, sizeof *m);
        m->hash = dt_range(0, 3);
        m->index = dt_range(0, 3);
        m->next = head;
        head = m;
    }
    gZrwMarkers = head;
    DT_ARG_INT(c, 0, hash);
    DT_ARG_INT(c, 1, index);
}

static void setup_cam_init(DtCtx *c)
{
    begin(c, 0);
    camStubs();
    gCamReady = dt_u32() % 8 == 0;
}

static void setup_cam_shutdown(DtCtx *c)
{
    begin(c, 0);
    camStubs();
    gCamReady = dt_u32() % 4 != 0;
}

static void setup_cam_resolution(DtCtx *c)
{
    begin(c, 0);
    camStubs();
    DT_ARG_INT(c, 0, dt_range(1, 2000));
    DT_ARG_INT(c, 1, dt_range(1, 2000));
}

/* ---- shadow ----------------------------------------------------------------------------------------- */
S5_STUB2(sFileLoad, 0x484d30)
S5_STUB1(sFreeBuffer, 0x485370)

static uint32_t BMX_CDECL sLoadSprite(uint32_t data, uint32_t size, const char *name, uint32_t type,
                                      uint32_t zero)
{
    s5Log(0x4896d0);
    s5Log(dt_canon(data));
    s5Log(size);
    s5Log(s5Hash(name));
    s5Log(type);
    s5Log(zero);
    return s5Ret(0x4896d0);
}

static uint32_t BMX_CDECL sFileLoadSized(const char *path, int *size)
{
    s5Log(0x484d30);
    s5Log(s5Hash(path));
    *size = (int)(dt_u32() % 5000);
    return s5Ret(0x484d30);
}

static void setup_shadow_start(DtCtx *c)
{
    begin(c, 0);
    S5_ON(0x484d30, sFileLoadSized);
    S5_ON(0x485370, sFreeBuffer);
    S5_ON(0x4896d0, sLoadSprite);
    S5_ON(0x48ba30, sRegister);
    (void)sFileLoad;
    s5SetRet(0x484d30, dt_u32() % 4 == 0 ? 0 : (uint32_t)(uintptr_t)handle());
    s5SetRet(0x4896d0, (uint32_t)(uintptr_t)handle());
    s5SetRet(0x48ba30, (uint32_t)(uintptr_t)handle());
    gZrwShadowActive = dt_u32() % 8 == 0;
    gZrwShadowTexture = dt_u32();
    gZrwShadowReady = dt_u32();
}

DT_CASES(zrwworld_cases) = {
    {"zrwworldUpdateEffects", 0x00495bd0, zrwworldUpdateEffects, 1, DT_RET_INT, setup_update_effects, 200, 0},
    {"zrwworldCreateObject", 0x00495c10, zrwworldCreateObject, 0, DT_RET_INT, setup_create_object, 100, 0},
    {"zrwworldApplyEffect", 0x00495d50, zrwworldApplyEffect, 5, DT_RET_INT, setup_apply_effect, 400, 0},
    {"zrwworldCountSector", 0x00495df0, zrwworldCountSector, 1, DT_RET_INT, setup_count_sector, 100, 0},
    {"zrwworldCountAtomic", 0x00495e30, zrwworldCountAtomic, 1, DT_RET_INT, setup_count_atomic, 100, 0},
    {"zrwworldCountClump", 0x00495e70, zrwworldCountClump, 2, DT_RET_INT, setup_count_clump, 100, 0},
    {"zrwworldLogInfo", 0x00495f60, zrwworldLogInfo, 0, DT_RET_INT, setup_log_info, 300, 0},
    {"zrwworldLogLight", 0x00496120, zrwworldLogLight, 1, DT_RET_INT, setup_log_light, 300, 0},
    {"zrwworldFindRecord", 0x004962e0, zrwworldFindRecord, 3, DT_RET_INT, setup_find_record, 400, 0},
    {"zrwworldAddMaterial", 0x004963c0, zrwworldAddMaterial, 1, DT_RET_INT, setup_add_material, 100, 0},
    {"zrwWorldPvsCallback", 0x00496620, zrwWorldPvsCallback, 2, DT_RET_INT, setup_pvs_callback, 400, 0},
    {"zrwworldReadStream", 0x004966e0, zrwworldReadStream, 2, DT_RET_INT, setup_read_stream, 500, 0},
    {"zrwCreateWorld", 0x00496c40, zrwCreateWorld, 0, DT_RET_INT, setup_create_world, 300, 0},
    {"zrwWorldShutdown", 0x00496de0, zrwWorldShutdown, 0, DT_RET_INT, setup_shutdown, 300, 0},
    {"zrwworldLoad", 0x00496ff0, zrwworldLoad, 2, DT_RET_INT, setup_load, 500, 0},
    {"zrwcamFindMarker", 0x00492130, zrwcamFindMarker, 2, DT_RET_INT, setup_find_marker, 300, 0},
    {"zrwCameraInitialize", 0x00492340, zrwCameraInitialize, 0, DT_RET_INT, setup_cam_init, 300, 0},
    {"zrwCameraShutdown", 0x00492480, zrwCameraShutdown, 0, DT_RET_INT, setup_cam_shutdown, 200, 0},
    {"zrwCameraSetResolution", 0x004924f0, zrwCameraSetResolution, 2, DT_RET_INT, setup_cam_resolution, 300,
     0},
    {"zrollerShadowStart", 0x0049b330, zrollerShadowStart, 0, DT_RET_INT, setup_shadow_start, 200, 0},
};
