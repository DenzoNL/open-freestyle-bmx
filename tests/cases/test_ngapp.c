/*
 * Tests for ngapp.c: the application shell. Everything it calls (the renderer's parameter interface, the
 * asset and file functions, the other modules of the game) is a stub that logs its arguments; globals the
 * code branches on are set from the iteration's seed.
 */
#include "bmx/com/zge/ngif/ngapp.h"
#include "dt.h"
#include "dt_s5.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwprim.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zfile.h"
#include "zge/zgemods/zprop.h"
#include "zge/zmem.h"
#include "zge/zobj.h"

#define gCamera        ORIG_VAR(0x0068ff98, void *, gCamera)
#define gTickNode      ORIG_VAR(0x0068ffa4, void *, gTickNode)
#define gMode          ORIG_VAR(0x005ab3a0, uint8_t, gMode)
#define gArt           ORIG_VAR(0x005ab46c, void *, gArt)
#define gRamDisk       ORIG_VAR(0x0055bc38, uint32_t, gRamDisk)
#define gElapsed       ORIG_VAR(0x005ab454, float, gElapsed)
#define gLegal         ORIG_VAR(0x0055bc3c, int, gLegal)
#define gWake          ORIG_VAR(0x005ab460, uint32_t, gWake)
#define gCountdown     ORIG_VAR(0x005ab478, uint32_t, gCountdown)
#define gGuiReady      ORIG_VAR(0x00573250, int, gGuiReady)
#define gRollerNode    ORIG_VAR(0x005fb740, void *, gRollerNode)
#define gPlayers       ORIG_VAR(0x0069621c, uint8_t, gPlayers)
#define gLevel         ORIG_VAR(0x0056b614, uint8_t, gLevel)
#define gPlayer        ORIG_VAR(0x0056b615, uint8_t, gPlayer)
#define gKeySel        ORIG_VAR(0x006952eb, int8_t, gKeySel)
#define gMainLight     ORIG_VAR(0x005ab458, void *, gMainLight)
#define gSunTick       ORIG_VAR(0x005ab47c, uint32_t, gSunTick)
#define gPrevMark      ORIG_VAR(0x005ab468, uint32_t *, gPrevMark)
#define gLoadNode      ORIG_VAR(0x005ab470, void *, gLoadNode)
#define gLoadTex       ORIG_VAR(0x005ab48c, void *, gLoadTex)
#define gSetWindowText ORIG_VAR(0x006997c0, void *, gSetWindowText)
#define gSpot0         ORIG_VAR(0x005ab3ac, void *, gSpot0)
#define gLights0       ORIG_VAR(0x005ab438, void *, gLights0)
typedef void *PtrTable[3];
#define gSpots  ORIG_VAR(0x005ab3ac, PtrTable, gSpots)
#define gLights ORIG_VAR(0x005ab438, PtrTable, gLights)

/* ---- stubs ---------------------------------------------------------------------------------------- */

static uint32_t gTime;

static uint32_t BMX_CDECL stubTime(void)
{
    s5Log(0x489cb0);
    gTime += (uint32_t)dt_range(0, 1500);
    return gTime;
}

S5_STUB0(stubShutdownNgif, 0x463a60)
S5_STUB0(stubPoll, 0x4803f0)
S5_STUB0(stubSoundClose, 0x44e090)
S5_STUB0(stubSoundOpen, 0x44dda0)
S5_STUB0(stubNgifInit, 0x4639f0)
S5_STUB0(stubNgifBegin, 0x463ab0)
S5_STUB1(stubUnregister, 0x48bad0)
S5_STUB1(stubOpenLibrary, 0x4804d0)
S5_STUB1(stubAssetNext, 0x480730)
S5_STUB1(stubAssetSize, 0x480850)
S5_STUB2(stubAssetRead, 0x480930)
S5_STUB1(stubAssetClose, 0x4806c0)
S5_STUB0(stubStaticMark, 0x485e40)
S5_STUB1(stubStaticAlloc, 0x485d80)
S5_STUB1(stubStaticRelease, 0x485e10)
S5_STUB1(stubStaticInit, 0x485c80)
S5_STUB1(stubReleaseTexture, 0x489530)
S5_STUB0(stubCamFirst, 0x492eb0)
S5_STUB1(stubCamBegin, 0x492f00)
S5_STUB0(stubCamEnd, 0x492fe0)
S5_STUB1(stubShowRaster, 0x48b950)
S5_STUB1(stubDrawSprite, 0x45b6e0)
S5_STUB0(stubOnIdle, 0x48b570)
S5_STUB0(stubMemNop, 0x486300)
S5_STUB1(stubGetDevice, 0x480400)
S5_STUB1(stubCreateDevice, 0x480370)
S5_STUB1(stubDestroyDevice, 0x4803c0)
S5_STUB0(stubAdaptQuality, 0x45c710)
S5_STUB0(stubMenuShutdown, 0x44dc30)
S5_STUB0(stubSoundUpdate, 0x44e120)
S5_STUB0(stubSoundStreams, 0x44e2e0)
S5_STUB0(stubInitApp, 0x44d990)
S5_STUB0(stubAllocMacros, 0x44d940)
S5_STUB0(stubActorTable, 0x472630)
S5_STUB0(stubOpenMotions, 0x403c30)
S5_STUB0(stubActionShutdown, 0x404ed0)
S5_STUB0(stubActionUpdate, 0x4040e0)
S5_STUB0(stubActionInit, 0x403d10)
S5_STUB0(stubPickDemo, 0x403ce0)
S5_STUB0(stubCameraPlayback, 0x4056a0)
S5_STUB0(stubSceneClose, 0x4913d0)
S5_STUB1(stubSceneOpen, 0x4911c0)
S5_STUB0(stubFreeLists, 0x46c3b0)
S5_STUB0(stubFreeModelTable, 0x46c1d0)
S5_STUB0(stubSyncActors, 0x4659e0)
S5_STUB0(stubUnk463820, 0x463820)
S5_STUB0(stubUnk4638b0, 0x4638b0)
S5_STUB0(stubUnk4638f0, 0x4638f0)
S5_STUB0(stubUnk463930, 0x463930)
S5_STUB0(stubFreeArt, 0x45b240)
S5_STUB0(stubLoadArt, 0x45b020)
S5_STUB0(stubShowLoading, 0x45b5c0)
S5_STUB0(stubUpdateSun, 0x45c040)
S5_STUB0(stubLinkFaces, 0x45afb0)
S5_STUB0(stubInitSpots, 0x45adc0)
S5_STUB0(stubResetColors, 0x45ac90)
S5_STUB0(stubAttachLights, 0x45ac40)
S5_STUB2(stubKeyMap, 0x493cc0)
S5_STUB1(stubFreeObj, 0x487400)
S5_STUB0(stubCloseMotions, 0x475000)
S5_STUB0(stubResUnload, 0x46be30)
S5_STUB1(stubListDestroy, 0x48c2b0)
S5_STUB1(stubPoolDestroy, 0x485fb0)
S5_STUB1(stubSetAssetFile, 0x437850)
S5_STUB2(stubModelRenderCb, 0x48fec0)
S5_STUB2(stubBuildName, 0x485460)

static uint32_t BMX_CDECL stubLoadSprite(void *data, uint32_t size, const char *name, int type)
{
    s5Log(0x4896d0);
    s5LogPtr(data);
    s5Log(size);
    s5Log(s5Hash(name));
    s5Log((uint32_t)type);
    return s5Ret(0x4896d0);
}

static int BMX_CDECL stubPlayMovie(const char *name)
{
    s5Log(0x48bed0);
    s5Log(s5Hash(name));
    return 1;
}

static int BMX_CDECL stubMenuUpdate(void)
{
    s5Log(0x44dc70);
    return dt_u32() % 5 == 0;
}

static void *BMX_CDECL stubRegister(int type, ZrwUpdateFn fn, void *arg, int append)
{
    s5Log(0x48ba30);
    s5Log((uint32_t)type);
    s5Log((uint32_t)(uintptr_t)fn);
    s5LogPtr(arg);
    s5Log((uint32_t)append);
    return (void *)(uintptr_t)(0x40000000u + (dt_u32() & 0xfff0u));
}

static int BMX_CDECL stubSetParameter(int id, const void *value, const void *extra)
{
    s5Log(0x48ab10);
    s5Log((uint32_t)id);
    switch (id) {
    case 1:
        s5LogWords(value, 3);
        s5LogPtr(extra);
        break;
    case 0:
        s5LogWords(value, 1);
        s5LogPtr(extra);
        break;
    case 5:
    case 9:
    case 8:
    case 4:
        s5LogPtr(value);
        s5LogWords(extra, 1);
        break;
    default:
        s5LogPtr(value);
        s5LogPtr(extra);
        break;
    }
    return 1;
}

static uint32_t gWindowHolder[2];

static int BMX_CDECL stubGetParameter(int id, void *out, const void *extra)
{
    s5Log(0x48ad20);
    s5Log((uint32_t)id);
    s5LogPtr(extra);
    if (id == 0x15) {
        gWindowHolder[0] = 0x41000000u;
        *(void **)out = gWindowHolder;
    } else {
        *(float *)out = dt_float(100.0f, 1000.0f);
    }
    return 1;
}

static int BMX_STDCALL stubSetWindowText(void *window, const char *text)
{
    s5Log(0x41000001);
    s5LogPtr(window);
    s5Log(s5Hash(text));
    return 1;
}

static void *BMX_CDECL stubCameraCreate(void)
{
    s5Log(0x4821c0);
    return (void *)(uintptr_t)0x42000000u;
}

static void *BMX_CDECL stubGetModel(const void *object)
{
    s5Log(0x487440);
    s5LogPtr(object);
    return (void *)(uintptr_t)0x42100000u;
}

static void *BMX_CDECL stubFindMarker(int hash, int index);
static uint8_t gMarkers[8][0x60];
static uint8_t gMarkerParam[8][8];
static int gMarkerCount;

static void *BMX_CDECL stubFindMarker(int hash, int index)
{
    s5Log(0x492130);
    s5Log((uint32_t)hash);
    s5Log((uint32_t)index);
    return dt_u32() % 3 == 0 ? NULL : gMarkers[(dt_u32() + (uint32_t)gMarkerCount) & 7];
}

static void *BMX_CDECL stubAddLight(int flags, int type, int param)
{
    s5Log(0x491ba0);
    s5Log((uint32_t)flags);
    s5Log((uint32_t)type);
    s5Log(dt_canon((uint32_t)param));
    return (void *)(uintptr_t)(0x43000000u + (dt_u32() & 0xfff0u));
}

static void BMX_CDECL stubLightFlags(void *light, uint8_t flags)
{
    s5Log(0x4900f0);
    s5LogPtr(light);
    s5Log(flags);
}

static void BMX_CDECL stubLightDirection(void *light, const ZVec3 *direction)
{
    s5Log(0x490120);
    s5LogPtr(light);
    s5LogWords(direction, 3);
}

static void BMX_CDECL stubLightColor(void *light, const uint8_t *rgba)
{
    s5Log(0x4900d0);
    s5LogPtr(light);
    s5LogWords(rgba, 1);
}

static char gBuiltName[40] = "assets\\intwrap.zal";

static char *BMX_CDECL stubBuildFileName(const char *dir, const char *file)
{
    s5Log(0x485460);
    s5Log(s5Hash(dir));
    s5Log(s5Hash(file));
    return gBuiltName;
}

/* ---- simple functions ----------------------------------------------------------------------------- */

static void setup_nothing(DtCtx *c)
{
    s5Begin(c, 0);
}

static ZMatrix gCameraMatrix;

static ZMatrix *BMX_CDECL stubCameraMatrix(void *camera, uint32_t kind)
{
    s5Log(0x482310);
    s5LogPtr(camera);
    s5Log(kind);
    return &gCameraMatrix;
}

static void setup_to_listener(DtCtx *c)
{
    ZVec3 *world = dt_alloc(sizeof *world);
    ZVec3 *local = dt_alloc(sizeof *local);

    s5Begin(c, 0);
    S5_ON(0x00482310, stubCameraMatrix);
    gCamera = (void *)(uintptr_t)(0x44000000u + (dt_u32() & 0xfff0u));
    for (int i = 0; i < 16; i++) {
        ((float *)&gCameraMatrix)[i] = dt_float_edgy(-100.0f, 100.0f);
    }
    s5RandVec(world);
    DT_ARG_PTR(c, 0, world);
    DT_ARG_PTR(c, 1, local);
}

typedef struct FakeMovie {
    const char *file;
    uint32_t pad[5];
} FakeMovie;

static void setup_movie(DtCtx *c)
{
    FakeMovie *movie = dt_alloc(sizeof *movie);
    FakeMovie **list = (FakeMovie **)dt_alloc(sizeof *list);
    char *name = dt_alloc(40);

    s5Begin(c, 0);
    gTime = dt_u32() & 0xfffff;
    S5_ON(0x00463a60, stubShutdownNgif);
    S5_ON(0x00489cb0, stubTime);
    S5_ON(0x004803f0, stubPoll);
    S5_ON(0x0044e090, stubSoundClose);
    S5_ON(0x0048bed0, stubPlayMovie);
    S5_ON(0x0044dda0, stubSoundOpen);
    S5_ON(0x004639f0, stubNgifInit);
    S5_ON(0x00463ab0, stubNgifBegin);
    for (int i = 0; i < 12; i++) {
        name[i] = (char)dt_range('A', 'Z');
    }
    name[dt_range(5, 12)] = 0;
    movie->file = name;
    *list = movie;
    DT_ARG_PTR(c, 0, list);
}

static void *BMX_CDECL stubRegisterOnce(int type, ZrwUpdateFn fn, void *arg, int append)
{
    return stubRegister(type, fn, arg, append);
}

static void setup_register_tick(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x0048ba30, stubRegisterOnce);
    S5_ON(0x0048bad0, stubUnregister);
    gTickNode = dt_u32() % 2 ? (void *)(uintptr_t)(0x45000000u + (dt_u32() & 0xfff0u)) : NULL;
}

static int BMX_CDECL stubBlya(int16_t x, int16_t y, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    s5Log(0x47f940);
    s5Log((uint32_t)(uint16_t)x);
    s5Log((uint32_t)(uint16_t)y);
    s5Log(s5Hash(format));
    s5Log(s5Hash(va_arg(ap, const char *)));
    va_end(ap);
    return 0;
}

static void setup_version(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x0047f940, stubBlya);
}

/* ---- props and lights ------------------------------------------------------------------------------ */

typedef ZPropInst *PropTable[8];

static void makeProps(void)
{
    ZPropInst *prev = NULL;
    ZrwList *list = (ZrwList *)zobjPropList();
    const int count = dt_range(0, 5);

    memset(list, 0, sizeof *list);
    for (int i = 0; i < count; i++) {
        ZPropInst *p = dt_alloc(sizeof *p);
        ZObj *object = dt_alloc(sizeof *object);

        dt_fill(p, sizeof *p);
        dt_fill(object, sizeof *object);
        object->model = (void *)(uintptr_t)(0x46000000u + (uint32_t)i * 16u);
        p->object = object;
        p->next = NULL;
        p->id = dt_u32() % 2 == 0 ? (uint32_t)DT_ORIG(0x004828c0, int (*)(const char *))("light_") : dt_u32();
        if (dt_u32() % 2 == 0) {
            uint8_t *color = dt_alloc(8);

            dt_fill(color, 8);
            if (dt_u32() % 3 == 0) {
                memset(color, 0, 4);
            }
            p->extra = color;
        } else {
            p->extra = NULL;
        }
        if (prev != NULL) {
            prev->next = p;
        } else {
            list->head = (void *)p;
        }
        prev = p;
    }
}

static void BMX_CDECL stubSetColor(void *model, const uint8_t *rgba)
{
    s5Log(0x488ca0);
    s5LogPtr(model);
    s5LogWords(rgba, 1);
}

static void setup_props(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x0048fec0, stubModelRenderCb);
    S5_ON(0x00488ca0, stubSetColor);
    makeProps();
}

static void setup_init_lights(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x00491ba0, stubAddLight);
    S5_ON(0x004900f0, stubLightFlags);
    S5_ON(0x00490120, stubLightDirection);
    memset(&gLights, 0, sizeof(PtrTable));
}

static void setup_init_spots(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x00491ba0, stubAddLight);
    S5_ON(0x004900f0, stubLightFlags);
    S5_ON(0x00490120, stubLightDirection);
    S5_ON(0x00492130, stubFindMarker);
    gMarkerCount = (int)dt_u32();
    for (int i = 0; i < 8; i++) {
        float *f = (float *)gMarkers[i];

        dt_fill(gMarkers[i], sizeof gMarkers[i]);
        for (int k = 2; k < 18; k++) {
            f[k] = dt_float_edgy(-100.0f, 100.0f);
        }
        *(uint32_t *)&gMarkers[i][0x48] = dt_u32() % 3 == 0 ? 0u : 0x47000000u + (uint32_t)i * 16u;
    }
    memset(&gSpots, 0, sizeof(PtrTable));
}

/* ---- faces ------------------------------------------------------------------------------------------ */

static uint8_t *gFaces;
static uint32_t gFaceCount;

static void makeFaces(void)
{
    gFaceCount = (uint32_t)dt_range(0, 6);
    gFaces = dt_alloc(gFaceCount * 0x6c + 16);
    dt_fill(gFaces, gFaceCount * 0x6c);
    for (uint32_t i = 0; i < gFaceCount; i++) {
        uint8_t *f = gFaces + i * 0x6c;
        uint16_t *tags = dt_alloc(16);

        dt_fill(tags, 16);
        for (int k = 0; k < 3; k++) {
            uint16_t *e = dt_alloc(8);

            e[0] = (uint16_t)dt_range(0, 5);
            *(uint16_t **)(f + 0x44 + k * 4) = dt_u32() % 4 == 0 ? NULL : e;
        }
        tags[0] = (uint16_t)(dt_u32() % 3 == 0 ? 0 : 0xf000u | (uint16_t)dt_range(0, 5));
        *(uint16_t **)(f + 0x40) = dt_u32() % 5 == 0 ? NULL : tags;
    }
}

static void *BMX_CDECL stubGetFaces(void)
{
    s5Log(0x48d980);
    return gFaces;
}

static uint32_t BMX_CDECL stubGetFaceCount(void)
{
    s5Log(0x48d990);
    return gFaceCount;
}

static void setup_find_face(DtCtx *c)
{
    s5Begin(c, 0);
    makeFaces();
    S5_ON(0x0048d990, stubGetFaceCount);
    DT_ARG_PTR(c, 0, gFaces);
    DT_ARG_INT(c, 1, dt_range(0, 5));
}

static void setup_link_faces(DtCtx *c)
{
    s5Begin(c, 0);
    makeFaces();
    S5_ON(0x0048d990, stubGetFaceCount);
    S5_ON(0x0048d980, stubGetFaces);
}

/* ---- interface art ----------------------------------------------------------------------------------- */

static ZfileDeviceTable *gDeviceTable;

static ZfileDeviceTable *BMX_CDECL stubGetDevices(void)
{
    s5Log(0x483cb0);
    return gDeviceTable;
}

static const char *BMX_CDECL stubDeviceName(void *device)
{
    s5Log(0x483ce0);
    s5LogPtr(device);
    return ((uintptr_t)device & 0x20) != 0 ? "rd00" : "hd00";
}

static uint32_t BMX_CDECL stubDeviceControl(void *device, uint32_t command, const char *const *names,
                                            const uint32_t *values)
{
    s5Log(0x483d30);
    s5LogPtr(device);
    s5Log(command);
    s5Log(s5Hash(names[0]));
    s5Log(s5Hash(names[1]));
    s5Log(values[0]);
    s5Log(values[1]);
    return dt_u32() % 5 == 0 ? 0u : (uint32_t)(uintptr_t)gBuiltName;
}

static void setup_load_art(DtCtx *c)
{
    const int devices = dt_range(0, 4);

    s5Begin(c, 1);
    S5_ON(0x00485460, stubBuildFileName);
    S5_ON(0x004804d0, stubOpenLibrary);
    S5_ON(0x00480850, stubAssetSize);
    S5_ON(0x00480930, stubAssetRead);
    S5_ON(0x004806c0, stubAssetClose);
    S5_ON(0x00483cb0, stubGetDevices);
    S5_ON(0x00483ce0, stubDeviceName);
    S5_ON(0x00483d30, stubDeviceControl);
    S5_ON(0x00437850, stubSetAssetFile);
    s5SetRet(0x4804d0, dt_u32() % 8 == 0 ? 0u : 0x48000000u);
    s5SetRet(0x480850, (uint32_t)dt_range(16, 300));
    gDeviceTable = dt_alloc(sizeof *gDeviceTable);
    memset(gDeviceTable, 0, sizeof *gDeviceTable);
    gDeviceTable->count = (uint32_t)devices;
    for (int i = 0; i < devices; i++) {
        gDeviceTable->device[i] = (ZfileDevice *)(uintptr_t)(0x49000000u + (dt_u32() & 0xffu) * 0x10u);
    }
    if (dt_u32() % 7 == 0) {
        gDeviceTable = NULL;
    }
    gArt = dt_u32() % 6 == 0 ? (void *)(uintptr_t)0x4a000000u : NULL;
    gRamDisk = dt_u32() % 5 == 0 ? 0x4b000000u : 0xffffffffu;
}

static void setup_free_art(DtCtx *c)
{
    s5Begin(c, 0);
    gArt = dt_u32() % 3 == 0 ? NULL : dt_zmemalloc(32);
}

/* ---- start-up and shutdown -------------------------------------------------------------------------- */

static void *BMX_CDECL stubPad(int kind)
{
    s5Log(0x480370);
    s5Log((uint32_t)kind);
    return kind == 2 && dt_u32() % 2 == 0 ? NULL : (void *)(uintptr_t)(0x4c000000u + (uint32_t)kind * 16u);
}

static void setup_initialize(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x0048ab10, stubSetParameter);
    S5_ON(0x0048ad20, stubGetParameter);
    S5_ON(0x004821c0, stubCameraCreate);
    S5_ON(0x00487440, stubGetModel);
    S5_ON(0x00485c80, stubStaticInit);
    S5_ON(0x00480370, stubPad);
    S5_ON(0x004803c0, stubDestroyDevice);
    S5_ON(0x0048bed0, stubPlayMovie);
    S5_ON(0x0044dda0, stubSoundOpen);
    S5_ON(0x00472630, stubActorTable);
    S5_ON(0x00403c30, stubOpenMotions);
    S5_ON(0x0044d940, stubAllocMacros);
    gSetWindowText = (void *)stubSetWindowText;
    gElapsed = dt_float(0.0f, 10.0f);
    gWake = dt_u32();
}

static void setup_shutdown(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x00463930, stubUnk463930);
    S5_ON(0x00487400, stubFreeObj);
    S5_ON(0x0048bad0, stubUnregister);
    S5_ON(0x00404ed0, stubActionShutdown);
    S5_ON(0x004913d0, stubSceneClose);
    S5_ON(0x0046c3b0, stubFreeLists);
    S5_ON(0x0046c1d0, stubFreeModelTable);
    S5_ON(0x0044dc30, stubMenuShutdown);
    S5_ON(0x0048c2b0, stubListDestroy);
    S5_ON(0x00475000, stubCloseMotions);
    S5_ON(0x0046be30, stubResUnload);
    S5_ON(0x00485fb0, stubPoolDestroy);
    S5_ON(0x0044e090, stubSoundClose);
    gCamera = dt_u32() % 2 ? (void *)(uintptr_t)0x4d000000u : NULL;
    gMode = (uint8_t)dt_range(0, 3);
    gRollerNode = dt_u32() % 2 ? (void *)(uintptr_t)0x4d100000u : NULL;
    gArt = dt_u32() % 2 ? dt_zmemalloc(32) : NULL;
}

static void stubsAssets(DtCtx *c)
{
    (void)c;
    S5_ON(0x00485460, stubBuildFileName);
    S5_ON(0x004804d0, stubOpenLibrary);
    S5_ON(0x00480730, stubAssetNext);
    S5_ON(0x00480850, stubAssetSize);
    S5_ON(0x00485e40, stubStaticMark);
    S5_ON(0x00485d80, stubStaticAlloc);
    S5_ON(0x00480930, stubAssetRead);
    S5_ON(0x004806c0, stubAssetClose);
    S5_ON(0x004896d0, stubLoadSprite);
    S5_ON(0x00485e10, stubStaticRelease);
    S5_ON(0x00492eb0, stubCamFirst);
    S5_ON(0x00492f00, stubCamBegin);
    S5_ON(0x00492fe0, stubCamEnd);
    S5_ON(0x0048b950, stubShowRaster);
    S5_ON(0x00489530, stubReleaseTexture);
    S5_ON(0x0045b6e0, stubDrawSprite);
    s5SetRet(0x4804d0, dt_u32() % 5 == 0 ? 0u : 0x4e000000u);
    s5SetRet(0x480850, (uint32_t)dt_range(16, 300));
    s5SetRet(0x485e40, 0x4e100000u);
    s5SetRet(0x485d80, dt_u32() % 6 == 0 ? 0u : 0x4e200000u);
    s5SetRet(0x4896d0, 0x4e300000u);
    s5SetRet(0x492eb0, 0x4e400000u);
}

static void setup_show_loading(DtCtx *c)
{
    s5Begin(c, 0);
    stubsAssets(c);
    /* Neither stubbed allocation nor sprite fails: the original's flow then depends on the caller's ECX. */
    s5SetRet(0x485d80, 0x4e200000u);
    s5SetRet(0x4896d0, 0x4e300000u);
}

static void BMX_CDECL stubDrawQuads(uint32_t flags, const ZrwPrimQuad *quads, int count, uint32_t arg)
{
    s5Log(0x48d660);
    s5Log(flags);
    s5Log((uint32_t)count);
    s5Log(arg);
    for (int i = 0; i < count; i++) {
        s5Log(quads[i].texture);
        for (int v = 0; v < 4; v++) {
            const ZrwPrimVertex *p = quads[i].vertices[v];

            s5LogFloat(p->x);
            s5LogFloat(p->y);
            s5LogFloat(p->z);
            s5LogFloat(p->u);
            s5LogFloat(p->v);
            s5LogWords(p->color, 1);
        }
    }
}

static void setup_draw_sprite(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x0048ad20, stubGetParameter);
    S5_ON(0x0048d660, stubDrawQuads);
    DT_ARG_INT(c, 0, dt_u32());
}

/* ---- the frame function ------------------------------------------------------------------------------ */

static void *BMX_CDECL stubControllerGet(uint32_t index)
{
    static uint32_t record[4];

    s5Log(0x4086f0);
    s5Log(index);
    record[0] = 0x4f000000u;
    return record;
}

static void setup_frame(DtCtx *c)
{
    s5Begin(c, 1);
    gTime = dt_u32() & 0xfffff;
    stubsAssets(c);
    S5_ON(0x00489cb0, stubTime);
    S5_ON(0x0045c710, stubAdaptQuality);
    S5_ON(0x004803f0, stubPoll);
    S5_ON(0x00480400, stubGetDevice);
    S5_ON(0x0044dc30, stubMenuShutdown);
    S5_ON(0x0044e120, stubSoundUpdate);
    S5_ON(0x0044e2e0, stubSoundStreams);
    S5_ON(0x0044d990, stubInitApp);
    S5_ON(0x0044dc70, stubMenuUpdate);
    S5_ON(0x0048ab10, stubSetParameter);
    S5_ON(0x00487440, stubGetModel);
    S5_ON(0x00463820, stubUnk463820);
    S5_ON(0x004638b0, stubUnk4638b0);
    S5_ON(0x004638f0, stubUnk4638f0);
    S5_ON(0x00463930, stubUnk463930);
    S5_ON(0x0045b240, stubFreeArt);
    S5_ON(0x0045b020, stubLoadArt);
    S5_ON(0x0045b5c0, stubShowLoading);
    S5_ON(0x0045c040, stubUpdateSun);
    S5_ON(0x0045afb0, stubLinkFaces);
    S5_ON(0x0045adc0, stubInitSpots);
    S5_ON(0x0045ac90, stubResetColors);
    S5_ON(0x0045ac40, stubAttachLights);
    S5_ON(0x0048ba30, stubRegister);
    S5_ON(0x0048bad0, stubUnregister);
    S5_ON(0x00486300, stubMemNop);
    S5_ON(0x00403ce0, stubPickDemo);
    S5_ON(0x004911c0, stubSceneOpen);
    S5_ON(0x004913d0, stubSceneClose);
    S5_ON(0x00492130, stubFindMarker);
    S5_ON(0x00472630, stubActorTable);
    S5_ON(0x00403d10, stubActionInit);
    S5_ON(0x004040e0, stubActionUpdate);
    S5_ON(0x00404ed0, stubActionShutdown);
    S5_ON(0x004056a0, stubCameraPlayback);
    S5_ON(0x004086f0, stubControllerGet);
    S5_ON(0x00493cc0, stubKeyMap);
    S5_ON(0x0048b570, stubOnIdle);
    S5_ON(0x004659e0, stubSyncActors);
    S5_ON(0x0046c3b0, stubFreeLists);
    gMarkerCount = (int)dt_u32();
    dt_fill(gMarkerParam, sizeof gMarkerParam);
    for (int i = 0; i < 8; i++) {
        dt_fill(gMarkers[i], sizeof gMarkers[i]);
        *(uint32_t *)&gMarkers[i][0x48] = dt_u32() % 2 == 0 ? 0u : (uint32_t)(uintptr_t)gMarkerParam[i];
    }
    gCamera = (void *)(uintptr_t)0x4d000000u;
    gLegal = dt_u32() % 4 == 0 ? 1 : (dt_u32() % 2 == 0 ? 0 : -1);
    gMode = (uint8_t)dt_range(0, 3);
    gWake = dt_u32() % 2;
    gCountdown = dt_u32() % 3 == 0 ? 0 : (uint32_t)dt_range(1, 3);
    gGuiReady = dt_u32() % 3 == 0;
    gPlayers = (uint8_t)dt_range(0, 2);
    gLevel = (uint8_t)dt_range(0, 3);
    gKeySel = (int8_t)dt_range(0, 3);
    gRollerNode = dt_u32() % 2 ? (void *)(uintptr_t)0x4d100000u : NULL;
    gArt = dt_u32() % 5 == 0 ? NULL : (void *)(uintptr_t)0x4a000000u;
    gElapsed = dt_float(0.0f, 100.0f);
    gLoadTex = dt_u32() % 2 ? (void *)(uintptr_t)0x4e300000u : NULL;
    gLoadNode = (void *)(uintptr_t)0x4d200000u;
    DT_ARG_FLOAT(c, 0, dt_float(0.0f, 0.2f));
}

/* ---- sun ---------------------------------------------------------------------------------------------- */

static int BMX_CDECL stubLineTest(const float *a, const float *b, int mode, int flags, float *out)
{
    s5Log(0x494c70);
    s5LogWords(a, 3);
    s5LogWords(b, 3);
    s5Log((uint32_t)mode);
    s5Log((uint32_t)flags);
    for (int i = 0; i < 19; i++) {
        out[i] = dt_float(-10.0f, 10.0f);
    }
    return dt_u32() % 3 != 0;
}

static uint8_t gSunColor[8];

static uint8_t *BMX_CDECL stubHitColor(const void *hit)
{
    s5Log(0x4947e0);
    s5LogPtr(hit);
    return gSunColor;
}

static void setup_sun(DtCtx *c)
{
    uint8_t *rider = dt_alloc(0x40);

    s5Begin(c, 0);
    S5_ON(0x00494c70, stubLineTest);
    S5_ON(0x004947e0, stubHitColor);
    S5_ON(0x004900d0, stubLightColor);
    dt_fill(gSunColor, sizeof gSunColor);
    dt_fill(rider, 0x40);
    s5RandVec((ZVec3 *)(rider + 0xc));
    gPlayer = (uint8_t)dt_range(0, 1);
    *(uint8_t **)(uintptr_t)(0x00692ad4u + 0xf70u * (uint32_t)gPlayer) = rider;
    gSunTick = dt_u32() % 5 < 2 ? 2 : (uint32_t)dt_range(0, 3);
    gMainLight = dt_u32() % 4 == 0 ? NULL : (void *)(uintptr_t)0x4d300000u;
}

DT_CASES(ngapp_cases) = {
    {"zsystemToListener", 0x0045a340, zsystemToListener, 2, DT_RET_VOID, setup_to_listener, 300, 0},
    {"ngappReturnZeroA", 0x0045a380, ngappReturnZeroA, 0, DT_RET_INT, setup_nothing, 2, 0},
    {"ngappReturnZeroB", 0x0045a3a0, ngappReturnZeroB, 0, DT_RET_INT, setup_nothing, 2, 0},
    {"moviePlay", 0x0045a3c0, moviePlay, 1, DT_RET_VOID, setup_movie, 100, 0},
    {"ngappRegisterTick", 0x0045ab50, ngappRegisterTick, 0, DT_RET_VOID, setup_register_tick, 20, 0},
    {"ngappUnregisterTick", 0x0045ab90, ngappUnregisterTick, 0, DT_RET_VOID, setup_register_tick, 20, 0},
    {"ngappPrintVersion", 0x0045abc0, ngappPrintVersion, 0, DT_RET_VOID, setup_version, 5, 0},
    {"ngappAttachLightObjects", 0x0045ac40, ngappAttachLightObjects, 0, DT_RET_VOID, setup_props, 100, 0},
    {"ngappResetPropColors", 0x0045ac90, ngappResetPropColors, 0, DT_RET_VOID, setup_props, 100, 0},
    {"ngappInitLights", 0x0045ace0, ngappInitLights, 0, DT_RET_VOID, setup_init_lights, 20, 0},
    {"ngappInitSpotLights", 0x0045adc0, ngappInitSpotLights, 0, DT_RET_VOID, setup_init_spots, 300, 0},
    {"ngappFindFace", 0x0045af40, ngappFindFace, 2, DT_RET_INT, setup_find_face, 200, 0},
    {"ngappLinkFaces", 0x0045afb0, ngappLinkFaces, 0, DT_RET_VOID, setup_link_faces, 100, 0},
    {"ngappLoadInterfaceArt", 0x0045b020, ngappLoadInterfaceArt, 0, DT_RET_VOID, setup_load_art, 300, 0},
    {"ngappFreeInterfaceArt", 0x0045b240, ngappFreeInterfaceArt, 0, DT_RET_VOID, setup_free_art, 20, 0},
    {"ngifInitialize", 0x0045b270, ngifInitialize, 0, DT_RET_INT, setup_initialize, 50, 0},
    {"ngappShutdown", 0x0045b4e0, ngappShutdown, 0, DT_RET_INT, setup_shutdown, 100, 0},
    {"ngappShowLoading", 0x0045b5c0, ngappShowLoading, 0, DT_RET_VOID, setup_show_loading, 100, 0},
    {"ngappDrawSprite", 0x0045b6e0, ngappDrawSprite, 1, DT_RET_INT, setup_draw_sprite, 20, 0},
    {"ngifRunFrame", 0x0045b8a0, ngifRunFrame, 1, DT_RET_INT, setup_frame, 1500, 0},
    {"ngappUpdateSun", 0x0045c040, ngappUpdateSun, 0, DT_RET_VOID, setup_sun, 300, 0},
};
