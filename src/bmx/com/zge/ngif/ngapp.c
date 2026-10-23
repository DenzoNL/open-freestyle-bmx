/* The NgIF application shell (C:\zaxis\Bmx\src\com\zge\NgIF\NgApp.c). */
#include "bmx/com/zge/ngif/ngapp.h"

#include <stddef.h>

#include "bmx/com/action/camera.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/ngif/ngiffx.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzres.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/com/zge/zactor.h"
#include "compat/crt.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/com/zglrw/zrwcol.h"
#include "zge/com/zglrw/zrwdbg.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwprim.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/pc/pcinput.h"
#include "zge/zasset.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zgemods/zobjlight.h"
#include "zge/zgemods/zprop.h"
#include "zge/zgemods/ztaginf.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zscene.h"
#include "zge/zstring.h"

static const char kNgAppFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgApp.c";

/* Declared elsewhere, but their headers pull in far more than this file needs. */
uint32_t actionInitialize(void);
uint32_t actionPickDemo(void);
uint32_t actionUpdate(void);
void actionShutdown(void);
int actionOpenMotions(void);
int frontendInitApp(void);
int frontendAllocMacros(void);
/* zrwzgl.h and zrwmodelrender.h disagree over the scale globals, so the billboard hook is declared here. */
typedef void *(BMX_CDECL *NgObjectCallback)(void *object);
void zrwModelSetRenderCallback(void *model, NgObjectCallback callback);
void *zrwModelRenderBillboard(void *object);
/* The camera frames of the renderer. */
ORIG_FN(0x00492eb0, void *, ngZrwcamFirst, (void));
ORIG_FN(0x00492f00, int, ngZrwcamBegin, (void *camera));
ORIG_FN(0x00492fe0, int, ngZrwcamEnd, (void));
/* ngifpsx.c functions whose purpose is not known yet, by address. */
ORIG_FN(0x00463820, void, ngifUnk463820, (void));
ORIG_FN(0x004638b0, void, ngifUnk4638b0, (void));
ORIG_FN(0x004638f0, void, ngifUnk4638f0, (void));
ORIG_FN(0x00463930, void, ngifUnk463930, (void));
/* The colour bytes of the surface a collision hit found (shared with the level's material, not a copy). */
ORIG_FN(0x004947e0, uint8_t *, zrwcolHitColor, (const void *hit));
/* Window title call in the exe's import table. */
typedef int(BMX_STDCALL *SetWindowTextFn)(void *window, const char *text);
#define gNgSetWindowText ORIG_VAR(0x006997c0, SetWindowTextFn, gNgSetWindowText)

#define gNgCamera       ORIG_VAR(0x0068ff98, ZObjCamera *, gNgCamera)
#define gNgTickNode     ORIG_VAR(0x0068ffa4, ZrwUpdateNode *, gNgTickNode)
#define gNgFlags68ffa0  ORIG_VAR(0x0068ffa0, uint8_t, gNgFlags68ffa0)
#define gNgFlags68ffa1  ORIG_VAR(0x0068ffa1, uint8_t, gNgFlags68ffa1)
#define gNgFlags68ffa2  ORIG_VAR(0x0068ffa2, uint8_t, gNgFlags68ffa2)
#define gNgFlags68ffa3  ORIG_VAR(0x0068ffa3, uint8_t, gNgFlags68ffa3)
#define gNgWord68ff84   ORIG_VAR(0x0068ff84, uint32_t, gNgWord68ff84)
#define gNgFontPages    ORIG_VAR(0x0068ff94, ZMemFreeList *, gNgFontPages)
#define gNgMode         ORIG_VAR(0x005ab3a0, uint8_t, gNgMode)
#define gNgArt          ORIG_VAR(0x005ab46c, void *, gNgArt)
#define gNgElapsed      ORIG_VAR(0x005ab454, float, gNgElapsed)
#define gNgWake         ORIG_VAR(0x005ab460, uint32_t, gNgWake)
#define gNgCountdown    ORIG_VAR(0x005ab478, uint32_t, gNgCountdown)
#define gNgFading       ORIG_VAR(0x005ab474, uint32_t, gNgFading)
#define gNgPrevMark     ORIG_VAR(0x005ab468, uint32_t *, gNgPrevMark)
#define gNgLoadNode     ORIG_VAR(0x005ab470, ZrwUpdateNode *, gNgLoadNode)
#define gNgLoadFlag     ORIG_VAR(0x005ab480, uint16_t, gNgLoadFlag)
#define gNgLoadTexture  ORIG_VAR(0x005ab48c, void *, gNgLoadTexture)
#define gNgLegal1       ORIG_VAR(0x005ab488, void *, gNgLegal1)
#define gNgLegal2       ORIG_VAR(0x005ab490, void *, gNgLegal2)
#define gNgLegalTime    ORIG_VAR(0x0055bd04, uint32_t, gNgLegalTime)
#define gNgLegalPending ORIG_VAR(0x0055bc3c, int, gNgLegalPending)
#define gNgRamDisk      ORIG_VAR(0x0055bc38, uint32_t, gNgRamDisk)
#define gNgAssetsDir    ORIG_VAR(0x0055ba04, const char *, gNgAssetsDir)
#define gNgWrapName     ORIG_VAR(0x0055bc40, const char *, gNgWrapName)
#define gNgArtName      ORIG_VAR(0x005ab3b8, char, gNgArtName)
#define gNgSunTick      ORIG_VAR(0x005ab47c, uint32_t, gNgSunTick)
#define gNgMainLight    ORIG_VAR(0x005ab458, void *, gNgMainLight)
#define gNgGuiReady     ORIG_VAR(0x00573250, int, gNgGuiReady)
#define gNgQuit         ORIG_VAR(0x00551fd4, uint32_t, gNgQuit)
#define gNgRollerNode   ORIG_VAR(0x005fb740, ZrwUpdateNode *, gNgRollerNode)
#define gNgWord5fb73c   ORIG_VAR(0x005fb73c, uint32_t, gNgWord5fb73c)
#define gNgRoller       ORIG_VAR(0x00678494, ZList *, gNgRoller)
#define gNgPlayerCount  ORIG_VAR(0x0069621c, uint8_t, gNgPlayerCount)
#define gNgLevel        ORIG_VAR(0x0056b614, uint8_t, gNgLevel)
#define gNgPlayer       ORIG_VAR(0x0056b615, uint8_t, gNgPlayer)
#define gNgKeyMapSel    ORIG_VAR(0x006952eb, int8_t, gNgKeyMapSel)

typedef ZVec3 NgVec16[4];
typedef struct NgVec16Slot {
    ZVec3 v;
    uint32_t pad;
} NgVec16Slot;
typedef NgVec16Slot NgLightDirs[3];
typedef uint8_t NgLightColors[3][4];
typedef const char *NgLevelNames[32];
typedef void *NgLightSlots[3];
typedef uint8_t NgKeyMaps[8][0x20];
#define gNgLightDir    ORIG_VAR(0x0055bc90, NgLightDirs, gNgLightDir)
#define gNgLightRef    ORIG_VAR(0x0055bcc0, NgLightDirs, gNgLightRef)
#define gNgLightColors ORIG_VAR(0x0055bcf0, NgLightColors, gNgLightColors)
#define gNgLights      ORIG_VAR(0x005ab438, NgLightSlots, gNgLights)
#define gNgSpotLights  ORIG_VAR(0x005ab3ac, NgLightSlots, gNgSpotLights)
#define gNgLevelNames  ORIG_VAR(0x0055bc44, NgLevelNames, gNgLevelNames)
#define gNgKeyMaps     ORIG_VAR(0x0055bb98, NgKeyMaps, gNgKeyMaps)
/* Pointers into the tables (the data is not a struct the code names). */
#define gNgMainLightColor ORIG_VAR(0x0055bc8c, uint8_t, gNgMainLightColor)
#define gNgSpotMainColor  ORIG_VAR(0x0055bcfc, uint8_t, gNgSpotMainColor)

/* Rider objects, 0xf70 bytes apart; the first word is the rider's scene object. */
#define gNgRiderObject(i) (*(const uint8_t *const *)(uintptr_t)(0x00692ad4u + 0xf70u * (uint32_t)(i)))

static uint32_t elapsedMs(uint32_t since)
{
    return zrwTimeGetTime() - since;
}

BMX_ADDR(0x0045a340)
void zsystemToListener(const ZVec3 *world, ZVec3 *local)
{
    zmatrixTransformPoint(local, zobjCameraGetMatrix(gNgCamera, 0), world);
}

BMX_ADDR(0x0045a380)
int ngappReturnZeroA(void)
{
    return 0;
}

BMX_ADDR(0x0045a3a0)
int ngappReturnZeroB(void)
{
    return 0;
}

BMX_ADDR(0x0045a3c0)
void moviePlay(struct MovieDesc *const *list)
{
    char name[0x80];
    uint32_t start;

    ngifShutdown();
    zstrcpy(name, **(const char *const **)(uintptr_t)list);
    zstrcpy(name + zstrlen(name) - 3, "AVI");
    start = zrwTimeGetTime();
    while (elapsedMs(start) < 3000) {
        zpadPoll();
    }
    bmxSoundClose();
    zrwPlayMovie(name);
    start = zrwTimeGetTime();
    while (elapsedMs(start) < 3000) {
        zpadPoll();
    }
    bmxSoundOpen();
    ngifInit();
    ngifBegin();
}

BMX_ADDR(0x0045ab50)
void ngappRegisterTick(void)
{
    if (gNgTickNode == NULL) {
        /* The original passes the address of the jump to the (unported) callback at 0x45a4d0. */
        gNgTickNode = zrwRegisterUpdateFunction(2, (ZrwUpdateFn)(uintptr_t)0x00401708, NULL, 1);
    }
}

BMX_ADDR(0x0045ab90)
void ngappUnregisterTick(void)
{
    if (gNgTickNode != NULL) {
        zrwUnregisterUpdateFunction(gNgTickNode);
        gNgTickNode = NULL;
    }
}

BMX_ADDR(0x0045abc0)
void ngappPrintVersion(void)
{
    char text[256];

    zstrcpy(text, "NextGen BMX : ");
    zstrcat(text, "Nov 22 2000");
    zstrcat(text, ",");
    zstrcat(text, "15:54:17");
    ZDebugPrintBlya(0x140, 0, "%s", text);
}

BMX_ADDR(0x0045ac40)
void ngappAttachLightObjects(void)
{
    for (ZPropInst *prop = (ZPropInst *)*zobjPropList(); prop != NULL; prop = prop->next) {
        if ((uint32_t)zstrHash("light_") == prop->id) {
            zrwModelSetRenderCallback(((ZObj *)prop->object)->model,
                                      BMX_FNPTR(NgObjectCallback, 0x0048f3e0, zrwModelRenderBillboard));
        }
    }
}

BMX_ADDR(0x0045ac90)
void ngappResetPropColors(void)
{
    for (ZPropInst *prop = (ZPropInst *)*zobjPropList(); prop != NULL; prop = prop->next) {
        uint8_t *color = prop->extra;

        if (color != NULL && *(uint32_t *)color != 0) {
            color[3] = 0xff;
            gZobjSetColor(((ZObj *)prop->object)->model, color);
        }
    }
}

BMX_ADDR(0x0045ace0)
void ngappInitLights(void)
{
    gNgMainLight = zsceneAddLight(0, 1, (int)(uintptr_t)&gNgMainLightColor);
    zobjLightSetFlags(gNgMainLight, 1);
    for (int i = 0; i < 3; i++) {
        ZVec3 direction;

        gNgLights[i] = zsceneAddLight(0, 0, (int)(uintptr_t)gNgLightColors[i]);
        direction = gNgLightDir[i].v;
        zvec3Sub(&direction, &direction, &gNgLightRef[i].v);
        zvec3NormalizeScaled(&direction, &direction);
        zobjLightSetDirection(gNgLights[i], &direction);
    }
}

BMX_ADDR(0x0045adc0)
void ngappInitSpotLights(void)
{
    gNgMainLight = zsceneAddLight(0, 1, (int)(uintptr_t)&gNgSpotMainColor);
    zobjLightSetFlags(gNgMainLight, 1);
    for (int i = 1; i <= 3; i++) {
        uint8_t *marker = (uint8_t *)zrwcamFindMarker(zstrHash("SpotLight"), i);
        uint8_t *target = (uint8_t *)zrwcamFindMarker(zstrHash("SpotLightT"), i);
        int param;

        if (marker == NULL) {
            continue;
        }
        param = *(int *)(marker + 0x48);
        if (param == 0) {
            continue;
        }
        if (target == NULL) {
            ZVec3 direction;

            direction.x = *(float *)(marker + 8);
            direction.y = *(float *)(marker + 0xc);
            direction.z = *(float *)(marker + 0x10);
            gNgSpotLights[i - 1] = zsceneAddLight(0, 0, param);
            zvec3NormalizeScaled(&direction, &direction);
            zobjLightSetDirection(gNgSpotLights[i - 1], &direction);
        } else {
            ZVec3 direction;

            direction.x = (float)((double)*(float *)(target + 0x38) - (double)*(float *)(marker + 0x38));
            direction.y = (float)((double)*(float *)(target + 0x3c) - (double)*(float *)(marker + 0x3c));
            direction.z = (float)((double)*(float *)(target + 0x40) - (double)*(float *)(marker + 0x40));
            zvec3NormalizeScaled(&direction, &direction);
            gNgSpotLights[i - 1] = zsceneAddLight(0, 0, *(int *)(marker + 0x48));
            zobjLightSetDirection(gNgSpotLights[i - 1], &direction);
            zobjLightSetFlags(gNgSpotLights[i - 1], 1);
        }
    }
}

BMX_ADDR(0x0045af40)
struct ZbspLevelFace *ngappFindFace(struct ZbspLevelFace *faces, uint32_t id)
{
    const uint32_t count = ztaginfGetFaceCount();
    ZbspLevelFace *face = faces;

    for (uint32_t i = 0; (int32_t)i < (int32_t)count; i++, face++) {
        for (int j = 0; j < 3; j++) {
            const uint16_t *tag = (const uint16_t *)face->edgeTag[j];

            if (tag != NULL && *tag == id) {
                return (struct ZbspLevelFace *)face;
            }
        }
    }
    return NULL;
}

BMX_ADDR(0x0045afb0)
void ngappLinkFaces(void)
{
    int32_t count = (int32_t)ztaginfGetFaceCount();
    ZbspLevelFace *faces = (ZbspLevelFace *)ztaginfGetFaces();

    for (ZbspLevelFace *face = faces; count > 0; face++, count--) {
        uint16_t *tag = (uint16_t *)(uintptr_t)face->tag;

        if (tag != NULL && *tag != 0) {
            *(void **)(tag + 4) = ngappFindFace((struct ZbspLevelFace *)faces, *tag & 0xfffu);
        }
    }
}

BMX_ADDR(0x0045b020)
void ngappLoadInterfaceArt(void)
{
    int library;
    uint32_t size;
    ZfileDeviceTable *devices;
    const char *names[2];
    uint32_t values[2];
    uint32_t path;
    uint32_t i;

    if (gNgArt != NULL) {
        zdebugFail("Interface art already loaded!", kNgAppFile, 0x3c2, 0);
    }
    library = zassetOpenLibrary(zfileBuildFileName(gNgAssetsDir, gNgWrapName));
    if (library == 0) {
        zdebugFail("Interface asset wrapper does not exist!", kNgAppFile, 0x3c5, 0);
    }
    size = zassetSize(library);
    gNgArt = zmemAlloc(size);
    zassetRead(library, gNgArt);
    if (gNgArt == NULL) {
        zdebugFail("Could not load menu art asset file", kNgAppFile, 0x3d1, 0);
    }
    zassetCloseLibrary(library);
    devices = zfileGetDevices();
    if (devices == NULL) {
        zdebugFail("No devices!", kNgAppFile, 0x3d5, 0);
    }
    for (i = 0; i < devices->count;) {
        if (zstrcmp(zfileDeviceName(devices->device[i]), "rd00") == 0) {
            gNgRamDisk = (uint32_t)(uintptr_t)devices->device[i];
            break;
        }
        i++;
    }
    if (gNgRamDisk == 0xffffffffu) {
        zdebugFail("Could not locate a RAM disk device on this system!", kNgAppFile, 0x3e1, 0);
    }
    values[0] = (uint32_t)(uintptr_t)gNgArt;
    values[1] = size;
    names[0] = "ADDRESS";
    names[1] = "SIZE";
    path = zfileDeviceControl((ZfileDevice *)(uintptr_t)gNgRamDisk, 2, (uint32_t)(uintptr_t)names,
                              (uint32_t)(uintptr_t)values);
    if (path == 0) {
        zdebugFail("Could not generate file name for RAM disk", kNgAppFile, 0x3ec, 0);
    }
    zstrcpy(&gNgArtName, (const char *)(uintptr_t)path);
    guiSetAssetFile(&gNgArtName);
}

BMX_ADDR(0x0045b240)
void ngappFreeInterfaceArt(void)
{
    if (gNgArt != NULL) {
        zmemFree(gNgArt);
        gNgArt = NULL;
    }
}

BMX_ADDR(0x0045b270)
int ngifInitialize(void)
{
    const uint8_t black[4] = {0, 0, 0, 0};
    const ZVec3 light = {100.0f, 200.0f, 200.0f};
    const float range = 200.0f;
    void *window[1];
    const float fov = 60.0f;
    struct GuiPadDevice *pad;

    gNgFlags68ffa2 = 0;
    gNgFlags68ffa3 = 0;
    gNgFlags68ffa0 = 0;
    gNgFlags68ffa1 = 0;
    gNgTickNode = NULL;
    zrwSetParameter(1, &light, NULL);
    zrwSetParameter(0x14, (const void *)1, NULL);
    zrwSetParameter(0x10, (const void *)1, NULL);
    gNgCamera = zobjCameraCreate();
    zrwSetParameter(7, zobjGetModel(gNgCamera), NULL);
    zrwSetParameter(8, zobjGetModel(gNgCamera), black);
    zrwSetParameter(5, zobjGetModel(gNgCamera), &range);
    zrwSetParameter(9, zobjGetModel(gNgCamera), &range);
    zrwSetParameter(3, zobjGetModel(gNgCamera), (const void *)1);
    zrwSetParameter(4, zobjGetModel(gNgCamera), black);
    {
        void **holder = NULL;
        zrwGetParameter(0x15, (void *)&holder, NULL);
        window[0] = *holder;
    }
    gNgSetWindowText(window[0], "Dave Mirra Freestyle BMX");
    zrwSetParameter(0, &fov, NULL);
    gNgElapsed = 0.0f;
    zmemStaticInit(0x400000);
    gNgWake = 0;
    gNgWord68ff84 = 0;
    pad = zpadCreateDevice(2);
    if (pad == NULL) {
        pad = zpadCreateDevice(0);
    }
    zrwPlayMovie("bin\\movie\\ACCLAIM.AVI");
    zrwPlayMovie("bin\\movie\\ZAXIS.AVI");
    zrwPlayMovie("bin\\movie\\INTRO.AVI");
    if (pad != NULL) {
        zpadDestroyDevice(pad);
    }
    gNgFontPages = zmemFreeListCreate(0x80, 0x100, NULL);
    bmxSoundOpen();
    zactorTableInit();
    actionOpenMotions();
    frontendAllocMacros();
    return 1;
}

BMX_ADDR(0x0045b4e0)
int ngappShutdown(void)
{
    ngifUnk463930();
    if (gNgCamera != NULL) {
        zobjFree(gNgCamera);
    }
    if (gNgMode == 2) {
        if (gNgRollerNode != NULL) {
            zrwUnregisterUpdateFunction(gNgRollerNode);
        }
        gNgRollerNode = NULL;
        actionShutdown();
        zsceneClose();
        gNgWord5fb73c = 0;
        ngifUnk463930();
        zrollerFreeLists();
    }
    zrollerFreeModelTable();
    if (gNgMode == 1) {
        bmxMenuShutdown();
        ngappFreeInterfaceArt();
    }
    zlistDestroy(gNgRoller);
    gNgRoller = NULL;
    zmotionCloseLibrary();
    ngifzresUnload();
    zmemFreeListDestroy(gNgFontPages);
    bmxSoundClose();
    return 1;
}

BMX_ADDR(0x0045b5c0)
void ngappShowLoading(void)
{
    int library = zassetOpenLibrary(zfileBuildFileName(gNgAssetsDir, gNgWrapName));

    if (library != 0) {
        uint32_t *mark = zmemStaticMark();
        void *sprite = NULL;
        uint32_t size;
        void *data;

        zassetNext(library);
        zassetNext(library);
        size = zassetSize(library);
        data = zmemStaticAlloc(size);
        zassetRead(library, data);
        zassetCloseLibrary(library);
        if (data != NULL) {
            sprite = zrwmatLoadSpriteTexture(data, size, "LOADING", 0);
        }
        zmemStaticRelease(mark);
        if (sprite != NULL) {
            ngZrwcamBegin(ngZrwcamFirst());
            ngappDrawSprite(sprite);
            ngZrwcamEnd();
            zrwShowRaster(1);
            ngZrwcamBegin(ngZrwcamFirst());
            ngappDrawSprite(sprite);
            ngZrwcamEnd();
            zrwShowRaster(1);
            zrwmatReleaseTexture(sprite);
        }
    }
}

BMX_ADDR(0x0045b6e0)
int ngappDrawSprite(void *sprite)
{
    uint32_t width;
    uint32_t height;
    ZrwPrimVertex bottomRight;
    ZrwPrimVertex topRight;
    ZrwPrimVertex bottomLeft;
    ZrwPrimVertex topLeft;
    ZrwPrimQuad quad;

    zrwGetParameter(10, &width, NULL);
    zrwGetParameter(11, &height, NULL);
    memset(&bottomRight, 0, sizeof bottomRight);
    memset(&topRight, 0, sizeof topRight);
    memset(&bottomLeft, 0, sizeof bottomLeft);
    memset(&topLeft, 0, sizeof topLeft);
    bottomRight.x = *(float *)&width;
    bottomRight.y = *(float *)&height;
    bottomRight.u = 1.0f;
    bottomRight.v = 1.0f;
    topRight.x = *(float *)&width;
    topRight.u = 1.0f;
    bottomLeft.y = *(float *)&height;
    bottomLeft.v = 1.0f;
    for (int i = 0; i < 4; i++) {
        bottomRight.color[i] = 0xff;
        topRight.color[i] = 0xff;
        bottomLeft.color[i] = 0xff;
        topLeft.color[i] = 0xff;
    }
    quad.texture = (uint32_t)(uintptr_t)sprite;
    quad.vertices[0] = &bottomLeft;
    quad.vertices[1] = &bottomRight;
    quad.vertices[2] = &topRight;
    quad.vertices[3] = &topLeft;
    zrwprimDrawQuads(0x2000000, &quad, 1, 0);
    return 1;
}

/* Waits (polling the pads) until three seconds passed since `since`. */
static void waitThreeSeconds(uint32_t since)
{
    if (elapsedMs(since) < 3000) {
        do {
            zpadPoll();
        } while (elapsedMs(since) < 3000);
    }
}

BMX_ADDR(0x0045b8a0)
int ngifRunFrame(float dt)
{
    uint8_t color[4];

    gNgElapsed = (float)((double)gNgElapsed + (double)dt);
    if (gNgLegalPending == 1) {
        uint32_t *mark = zmemStaticMark();
        int library = zassetOpenLibrary(zfileBuildFileName(gNgAssetsDir, gNgWrapName));
        uint32_t size;
        uint32_t size2;
        void *data;

        zassetNext(library);
        size = zassetSize(library);
        data = zmemStaticAlloc(size);
        zassetRead(library, data);
        gNgLegal1 = zrwmatLoadSpriteTexture(data, size, "LEGAL", 0);
        gNgLegalTime = zrwTimeGetTime();
        ngZrwcamBegin(ngZrwcamFirst());
        ngappDrawSprite(gNgLegal1);
        ngZrwcamEnd();
        zrwShowRaster(1);
        waitThreeSeconds(gNgLegalTime);
        zrwmatReleaseTexture(gNgLegal1);
        gNgLegalTime = zrwTimeGetTime();
        zassetNext(library);
        zassetNext(library);
        zassetNext(library);
        size2 = zassetSize(library);
        /* Original quirk: the buffer is sized for the first picture, not the second. */
        data = zmemStaticAlloc(size);
        zassetRead(library, data);
        gNgLegal2 = zrwmatLoadSpriteTexture(data, size2, "LEGAL2", 0);
        zassetCloseLibrary(library);
        ngZrwcamBegin(ngZrwcamFirst());
        ngappDrawSprite(gNgLegal2);
        ngZrwcamEnd();
        zrwShowRaster(1);
        zrwmatReleaseTexture(gNgLegal2);
        zmemStaticRelease(mark);
        ngappLoadInterfaceArt();
        ngifUnk4638b0();
        frontendInitApp();
        waitThreeSeconds(gNgLegalTime);
        gNgLegalPending = -1;
        return 1;
    }
    ngiffxAdaptQuality();
    zpadPoll();
    if (gNgMode == 1) {
        if (gNgWake == 0) {
            for (int i = 0; i < 4; i++) {
                zpadGetDevice(i);
            }
            if (gNgCountdown == 0) {
                gNgFading = 0;
                goto dispatch;
            }
        } else {
            gNgWake = 0;
            gNgCountdown = 0x3c;
        }
        gNgCountdown--;
        gNgFading = 1;
        if (gNgCountdown == 0) {
            bmxMenuShutdown();
            for (int i = 0; i < 8; i++) {
                bmxSoundUpdate();
            }
            gNgQuit = 1;
            frontendInitApp();
            for (int i = 0; i < 8; i++) {
                bmxSoundUpdate();
            }
            return 1;
        }
    } else {
        gNgCountdown = 0;
        gNgFading = 0;
    }
dispatch:
    if (gNgMode == 1) {
        int result = bmxMenuUpdate();

        for (;;) {
            if (result != 0) {
                return 0;
            }
            if (gNgGuiReady != 0) {
                if (gNgMode != 2) {
                    bmxSoundUpdate();
                    bmxSoundUpdateStreams();
                    return 1;
                }
                break;
            }
            if (gNgMode == 2) {
                break;
            }
            result = bmxMenuUpdate();
        }
        color[0] = 0;
        color[1] = 0;
        color[2] = 0;
        color[3] = 0x80;
        bmxMenuUpdate();
        bmxMenuUpdate();
        bmxMenuUpdate();
        zrwSetParameter(3, zobjGetModel(gNgCamera), (const void *)1);
        bmxMenuShutdown();
        gNgPrevMark = zmemStaticMark();
        ngifUnk4638f0();
        ngifUnk463820();
        ngappFreeInterfaceArt();
        {
            int library = zassetOpenLibrary(zfileBuildFileName(gNgAssetsDir, gNgWrapName));

            if (library != 0) {
                uint32_t *mark = zmemStaticMark();
                uint32_t size;
                void *data;

                zassetNext(library);
                zassetNext(library);
                if (gNgPlayerCount == 0) {
                    zassetNext(library);
                }
                size = zassetSize(library);
                data = zmemStaticAlloc(size);
                zassetRead(library, data);
                zassetCloseLibrary(library);
                if (data != NULL) {
                    gNgLoadTexture = zrwmatLoadSpriteTexture(data, size, "LOADING", 0);
                }
                zmemStaticRelease(mark);
            }
        }
        gNgLoadFlag = 0;
        gNgLoadNode = zrwRegisterUpdateFunction(6, (ZrwUpdateFn)(uintptr_t)0x0045c150, gNgLoadTexture, 0);
        zmemNop();
        if (gNgPlayerCount == 0) {
            actionPickDemo();
        }
        if (zsceneOpen(gNgLevelNames[gNgLevel]) != 1) {
            zdebugFail("Error loading level", kNgAppFile, 0x91f, 0);
        }
        {
            const uint8_t *globals = (const uint8_t *)zrwcamFindMarker(zstrHash("Globals"), 1);
            const void *parameter = color;

            if (globals != NULL && *(const void *const *)(globals + 0x48) != NULL) {
                parameter = *(const void *const *)(globals + 0x48);
            }
            zrwSetParameter(4, zobjGetModel(gNgCamera), parameter);
        }
        zrwSetParameter(8, zobjGetModel(gNgCamera), color);
        ngappLinkFaces();
        ngappInitSpotLights();
        ngappResetPropColors();
        ngappAttachLightObjects();
        zactorTableInit();
        actionInitialize();
        {
            const float fov = 30.0f;
            zrwSetParameter(0, &fov, NULL);
        }
        zmemNop();
        if (gNgLoadTexture != NULL) {
            zrwmatReleaseTexture(gNgLoadTexture);
        }
        zrwUnregisterUpdateFunction(gNgLoadNode);
        zpadSetKeyMap(*(void **)contrlerGet(0), (const uint8_t *)gNgKeyMaps + (int)gNgKeyMapSel * 0x20);
        zrwOnIdle();
        zrwOnIdle();
        zrwOnIdle();
        return 1;
    }
    if (gNgMode != 2) {
        return 1;
    }
    cameraPlaybackActive();
    ngifUnk463820();
    actionUpdate();
    ngappUpdateSun();
    if (gNgMode == 1) {
        if (gNgRollerNode != NULL) {
            zrwUnregisterUpdateFunction(gNgRollerNode);
        }
        gNgRollerNode = NULL;
        actionShutdown();
        zsceneClose();
        gNgWord5fb73c = 0;
        ngappShowLoading();
        zmemNop();
        ngifUnk463930();
        zrollerFreeLists();
        ngappLoadInterfaceArt();
        if (gNgArt == NULL) {
            zdebugFail("Failed to load interface assets", kNgAppFile, 0x86b, 0);
        }
        zrwSetParameter(3, zobjGetModel(gNgCamera), NULL);
        zmemStaticRelease(gNgPrevMark);
        frontendInitApp();
        {
            const float fov = 60.0f;
            zrwSetParameter(0, &fov, NULL);
        }
        zmemNop();
        return 1;
    }
    zactorSyncRiderActors();
    bmxSoundUpdate();
    bmxSoundUpdateStreams();
    return 1;
}

BMX_ADDR(0x0045c040)
void ngappUpdateSun(void)
{
    gNgSunTick++;
    if (gNgSunTick == 3) {
        const uint8_t *rider;
        ZVec3 from;
        ZVec3 to;
        uint8_t hit[0x5c];

        gNgSunTick = 0;
        rider = gNgRiderObject(gNgPlayer);
        from.x = *(const float *)(rider + 0xc);
        from.y = *(const float *)(rider + 0x10);
        from.z = *(const float *)(rider + 0x14);
        to.x = from.x;
        to.y = (float)((double)from.y + (double)1000.0f);
        to.z = from.z;
        if (zrwcolLineTest(&from.x, &to.x, 1, 0, (float *)hit) != 0 && gNgMainLight != NULL) {
            uint8_t *color = zrwcolHitColor(hit);

            color[0] = (uint8_t)((color[0] >> 1) + 0x20);
            color[1] = (uint8_t)((color[1] >> 1) + 0x20);
            color[2] = (uint8_t)((color[2] >> 1) + 0x20);
            zobjLightSetColor(gNgMainLight, color);
        }
    }
}
