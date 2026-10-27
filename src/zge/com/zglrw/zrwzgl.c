/* Application glue around the renderer (C:\NgEngine\Zge\Com\ZglRw\zrwZgl.c). */
#include "zge/com/zglrw/zrwzgl.h"

#include <stddef.h>

#include "compat/crt.h"
#include "rw/rwcamera.h"
#include "rw/rwcore.h"
#include "rw/rwmath.h"
#include "rw/rwplugins.h"
#include "rw/rwtexture.h"
#include "zge/pc/pcinput.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zramdisk.h"
#include "zge/zmem.h"
#include "zge/zmovie.h"
#include "zge/zstring.h"

static const char kZglFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwZgl.c";
static const char kFailedCall[] = "Failed in call to 0x%x\n";
static const char kNotInitialized[] = "Not initialized";
static const char kUnknownParameter[] = "Unknown parameter:%d";

/* Statically linked RenderWare and engine routines this module calls (not ported). */
ORIG_FN(0x004929f0, void, zrwSetClipPlane, (const void *plane, int zero, float value));
ORIG_FN(0x004930a0, void, zrwSetViewport, (const void *a, const void *b));
ORIG_FN(0x004930b0, void, zrwSetViewWindow, (const void *a, const void *b));
ORIG_FN(0x004930c0, void, zrwSetCameraPlane, (const void *a, const void *b));
ORIG_FN(0x004930e0, void, zrwSetCameraOffset, (const void *a, const void *b));
ORIG_FN(0x00493100, void, zrwSetCameraFlags, (const void *a, int flags));
/* Ported in zrwtxr.c (its header cannot be included here: it clashes with the macros of zrwzgl.h). */
void zrwSetMaterialParam(const void *value);
void zrwSetMaterialValue(int value);
int zrwMatRegisterPlugin(void);

ORIG_FN(0x0049ea10, int, zrwFileInitialize, (void));
ORIG_FN(0x0049ea60, void, zrwFileShutdown, (void));
ORIG_FN(0x00496c40, void *, zrwCreateWorld, (void));
ORIG_FN(0x00496de0, int, zrwWorldShutdown, (void));

ORIG_FN(0x00488890, int, zrwMdlRegisterPlugin, (void));
int zgeInitializeSubsystems(const char *argument);
int ngifInitialize(void);
int ngifRunFrame(float dt);

ORIG_FN(0x00492eb0, void *, zrwcamFirst, (void));
ORIG_FN(0x00492ed0, void *, zrwcamNext, (void *camera));
ORIG_FN(0x00492f00, int, zrwcamBegin, (void *camera));
ORIG_FN(0x00492fe0, int, zrwcamEnd, (void));
ORIG_FN(0x00493060, void *, zrwcamHandle, (void *camera));
ORIG_FN(0x00497620, int, zrwtxrFlush, (void));
ORIG_FN(0x004cc270, void, rwLightBegin, (void *light));
ORIG_FN(0x004cc510, void, rwLightSetMatrix, (void *light, const void *matrix));
ORIG_FN(0x004cc670, void, rwLightSetParams, (void *light, const void *a, const void *b));
ORIG_FN(0x004cc2c0, void, rwLightEnd, (void *light));

ORIG_FN(0x004a5c98, void *, rwfopen, (const char *path, const char *mode));
ORIG_FN(0x004a5cab, int, rwfclose, (void *file));
ORIG_FN(0x004a5a46, void, rwExit, (int code));

/* RwVideoMode: size, depth and flags (bit 0: full screen). */
typedef struct RwVideoMode {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint8_t flags;
    uint8_t pad[3];
} RwVideoMode;

enum { kMessageQuit = 0x12, kMessageAudioTick = 0x3bd, kMaxSubSystems = 8, kSubSystemNameBytes = 0x50 };
static const char kReadBinary[] = "rb";

/* Windows entry points called through the exe's import slots. */
typedef int(BMX_STDCALL *PeekMessageFn)(void *msg, void *window, uint32_t min, uint32_t max, uint32_t remove);
typedef int(BMX_STDCALL *MessageFn)(const void *msg);
typedef void(BMX_STDCALL *SleepFn)(uint32_t ms);
typedef int(BMX_STDCALL *ShowCursorFn)(int show);
typedef int(BMX_STDCALL *DialogBoxFn)(void *instance, uintptr_t name, void *parent, void *proc, int param);
#define gPeekMessage      ORIG_VAR(0x006997b4, PeekMessageFn, gPeekMessage)
#define gTranslateMessage ORIG_VAR(0x00699768, MessageFn, gTranslateMessage)
#define gDispatchMessage  ORIG_VAR(0x00699758, MessageFn, gDispatchMessage)
#define gSleep            ORIG_VAR(0x006995f8, SleepFn, gSleep)
#define gShowCursor       ORIG_VAR(0x006997a8, ShowCursorFn, gShowCursor)
#define gDialogBox        ORIG_VAR(0x00699778, DialogBoxFn, gDialogBox)
/* winmm timeGetTime import slot. Not gTimeGetTime (0x561480): that game pointer holds 0x489cb0, i.e. this
 * port. */
#define gTimeGetTimeImport ORIG_VAR(0x00699838, ZrwTimeFn, gTimeGetTimeImport)
#define gZglInstance       ORIG_VAR(0x00697784, void *, gZglInstance)
#define gZglWantDepth      ORIG_VAR(0x006977a8, int, gZglWantDepth)
#define gZglWantW2         ORIG_VAR(0x006977a0, int, gZglWantW2)
#define gZglWantH2         ORIG_VAR(0x006977a4, int, gZglWantH2)
#define gZglSubSystemCount ORIG_VAR(0x00672750, int, gZglSubSystemCount)
#define gZglSubSystem      ORIG_VAR(0x00672754, int, gZglSubSystem)
#define gZglVideoMode      ORIG_VAR(0x00672758, uint32_t, gZglVideoMode)
#define gZglSubSystemNames ORIG_VAR(0x006724d0, uint8_t, gZglSubSystemNames)

typedef void *(BMX_STDCALL *GetDlgItemFn)(void *dialog, int id);
typedef uintptr_t(BMX_STDCALL *SendMessageFn)(void *window, uint32_t message, uintptr_t wparam,
                                              uintptr_t lparam);
typedef void *(BMX_STDCALL *SetFocusFn)(void *window);
typedef int(BMX_STDCALL *EndDialogFn)(void *dialog, intptr_t result);
typedef int(BMX_STDCALL *MessageBoxFn)(void *owner, const char *text, const char *caption, uint32_t flags);
typedef void(BMX_STDCALL *DebugBreakFn)(void);
typedef void(BMX_STDCALL *PostQuitFn)(int code);
typedef int(BMX_STDCALL *ClipCursorFn)(const void *rect);
typedef intptr_t(BMX_STDCALL *DefWindowProcFn)(void *window, uint32_t message, uintptr_t wparam,
                                               uintptr_t lparam);
typedef int(BMX_STDCALL *AdjustWindowRectFn)(int32_t *rect, uint32_t style, int menu);
typedef int(BMX_CDECL *FormatLabelFn)(char *dst, const char *format, ...);
#define gMessageBox       ORIG_VAR(0x006997bc, MessageBoxFn, gMessageBox)
#define gDebugBreak       ORIG_VAR(0x006995f4, DebugBreakFn, gDebugBreak)
#define gPostQuit         ORIG_VAR(0x00699774, PostQuitFn, gPostQuit)
#define gClipCursor       ORIG_VAR(0x00699790, ClipCursorFn, gClipCursor)
#define gDefWindowProc    ORIG_VAR(0x0069978c, DefWindowProcFn, gDefWindowProc)
#define gAdjustWindowRect ORIG_VAR(0x00699780, AdjustWindowRectFn, gAdjustWindowRect)
#define gGetDlgItem       ORIG_VAR(0x00699760, GetDlgItemFn, gGetDlgItem)
#define gSendMessage      ORIG_VAR(0x00699764, SendMessageFn, gSendMessage)
#define gSetFocus         ORIG_VAR(0x0069977c, SetFocusFn, gSetFocus)
#define gEndDialog        ORIG_VAR(0x0069975c, EndDialogFn, gEndDialog)
#define gErrorCaption     ORIG_VAR(0x005611d8, const char *, gErrorCaption)
/* Message buffer of the assert box (0x100 bytes; a long message overflows into the next global). */
typedef char ZrwEpicBuffer[0x100];
#define gEpicMessage ORIG_VAR(0x006723d0, ZrwEpicBuffer, gEpicMessage)
typedef uint32_t ZrwKeyTable[256];
#define gZrwKeyActions ORIG_VAR(0x0054dce8, ZrwKeyTable, gZrwKeyActions)
/* The string interface object (+0xe8: sprintf). */
typedef struct ZrwStringApi {
    uint8_t pad[0xe8];
    FormatLabelFn format;
} ZrwStringApi;
#define gZrwStringApi ORIG_VAR(0x0067696c, ZrwStringApi *, gZrwStringApi)
/* IDirectDraw and IDirectDrawSurface, as far as the quit prompt uses them. */
typedef struct DdSurfaceVtbl {
    uint8_t pad[0x28];
    uint32_t(BMX_STDCALL *release)(void *self);
} DdSurfaceVtbl;
typedef struct DdVtbl {
    uint8_t pad[0x90];
    int(BMX_STDCALL *getGdiSurface)(void *self, void **surface);
} DdVtbl;
#define gDirectDraw ORIG_VAR(0x0069730c, void *, gDirectDraw)
#define gGdiSurface ORIG_VAR(0x006977cc, void *, gGdiSurface)

#define gZglMaterials     ORIG_VAR(0x006723b0, ZMemFreeList *, gZglMaterials)
#define gMovie            ORIG_VAR(0x006973a0, ZMovie, gMovie)
#define gMovieSkip        ORIG_VAR(0x0067288c, int, gMovieSkip)
#define gZglFileDeviceRw  ORIG_VAR(0x00562ff8, uint8_t, gZglFileDeviceRw)
#define gZglFileDeviceRam ORIG_VAR(0x00563240, uint8_t, gZglFileDeviceRam)

/* Subsystem hooks the original keeps as function pointers in data; they return 0 on failure. */
typedef int(BMX_CDECL *ZglInitFn)(void);
typedef void(BMX_CDECL *ZglRenderStateFn)(uint32_t flags, int extra);
typedef void(BMX_CDECL *ZglNopFn)(void);
#define gZglInitTexture     ORIG_VAR(0x005623f0, ZglInitFn, gZglInitTexture)
#define gZglInitMaterial    ORIG_VAR(0x00561018, ZglInitFn, gZglInitMaterial)
#define gZglInitModel       ORIG_VAR(0x00560e78, ZglInitFn, gZglInitModel)
#define gZglInitLight       ORIG_VAR(0x00563420, ZglInitFn, gZglInitLight)
#define gZglInitCol         ORIG_VAR(0x00562060, ZglInitFn, gZglInitCol)
#define gZglInitPrim        ORIG_VAR(0x00561720, ZglInitFn, gZglInitPrim)
#define gZglExitPrim        ORIG_VAR(0x00561724, ZglInitFn, gZglExitPrim)
#define gZglExitCol         ORIG_VAR(0x00562064, ZglInitFn, gZglExitCol)
#define gZglExitLight       ORIG_VAR(0x00563424, ZglInitFn, gZglExitLight)
#define gZglExitModel       ORIG_VAR(0x00560e7c, ZglInitFn, gZglExitModel)
#define gZglExitMaterial    ORIG_VAR(0x0056101c, ZglInitFn, gZglExitMaterial)
#define gZglExitTexture     ORIG_VAR(0x005623f4, ZglInitFn, gZglExitTexture)
#define gZglRenderStateHook ORIG_VAR(0x00561490, ZglRenderStateFn, gZglRenderStateHook)
#define gZglAllocHookNop    ORIG_VAR(0x005614ac, ZglNopFn, gZglAllocHookNop)
#define gZglMemoryFuncs     ORIG_VAR(0x005614a8, uint32_t, gZglMemoryFuncs)

BMX_ADDR(0x0048c110)
void zrwListInit(ZrwList *list)
{
    list->tail = NULL;
    list->head = NULL;
    list->count = 0;
}

typedef struct ZrwNode {
    struct ZrwNode *next;
    struct ZrwNode *prev;
} ZrwNode;

BMX_ADDR(0x0048c120)
void zrwListPrepend(ZrwList *list, void *node)
{
    ZrwNode *n = node;

    n->prev = NULL;
    n->next = list->head;
    list->head = n;
    if (list->tail == NULL) {
        list->tail = n;
    }
    if (n->next != NULL) {
        n->next->prev = n;
    }
    list->count++;
}

BMX_ADDR(0x0048c150)
void zrwListAdd(ZrwList *list, void *node)
{
    ZrwNode *n = node;

    n->next = NULL;
    n->prev = list->tail;
    list->tail = n;
    if (list->head == NULL) {
        list->head = n;
    }
    if (n->prev != NULL) {
        n->prev->next = n;
    }
    list->count++;
}

BMX_ADDR(0x0048c1f0)
void zrwListRemove(ZrwList *list, void *node)
{
    ZrwNode *n = node;

    if (n->prev == NULL) {
        list->head = n->next;
    } else {
        n->prev->next = n->next;
    }
    if (n->next == NULL) {
        list->tail = n->prev;
    } else {
        n->next->prev = n->prev;
    }
    list->count--;
}

BMX_ADDR(0x00489c90)
void zrwPrintMaterialUsage(void)
{
    gZdebugPrint("ZGLMAT: Objects in use:%d\n", zmemFreeListUsedCount(gZglMaterials));
}

BMX_ADDR(0x00489cb0)
uint32_t zrwTimeGetTime(void)
{
    return gTimeGetTimeImport(); /* the original is JMP [0x699838] */
}

BMX_ADDR(0x00489ce0)
void zrwDebugOutput(const char *text)
{
    gOutputDebugString(text);
}

BMX_ADDR(0x0048a940)
int zrwFileSystemsInitialize(void *table)
{
    uint32_t *words = table;

    if (zrwFileInitialize() == 1) {
        if (zramdiskInitialize() != 1) {
            zrwFileShutdown();
            return 0;
        }
        words[0] = 2;
        words[1] = (uint32_t)(uintptr_t)&gZglFileDeviceRw;
        words[2] = (uint32_t)(uintptr_t)&gZglFileDeviceRam;
    }
    return 1;
}

BMX_ADDR(0x0048a9a0)
int zrwReturnZero(void)
{
    return 0;
}

BMX_ADDR(0x0048af80)
void zrwNop(void) {}

/* Parameter ids 6, 10, 11 and 17..19 (and everything above 20) are unknown to the setter. */
BMX_ADDR(0x0048ab10)
int zrwSetParameter(int id, const void *value, const void *extra)
{
    switch (id) {
    case 0:
        gZrwFrameLimit = *(const float *)value;
        return 1;
    case 1: {
        const float v = *(const float *)value;
        gZrwInvScale = (float)(1.0 / (double)v);
        gZrwScale = v;
        return 1;
    }
    case 2:
        zmemCopy(gZrwName, value, 0x20);
        return 1;
    case 3:
        zrwSetViewport(value, extra);
        return 1;
    case 4:
        zrwSetViewWindow(value, extra);
        return 1;
    case 5:
        zrwSetClipPlane(value, 0, *(const float *)extra);
        return 1;
    case 7:
        zrwSetCameraPlane(value, extra);
        return 1;
    case 8:
        zrwSetCameraOffset(value, extra);
        return 1;
    case 9:
        zrwSetCameraFlags(value, *(const int *)extra);
        return 1;
    case 0xc:
        gZrwFlags ^= (gZrwFlags ^ (uint32_t)(uintptr_t)value) & 1;
        return 1;
    case 0xd:
        gZrwFlags = (((uint32_t)(uintptr_t)value & 1) << 2) | (gZrwFlags & 0xfffffffbu);
        return 1;
    case 0xe:
        zrwSetMaterialParam(value);
        return 1;
    case 0xf:
        zrwSetMaterialValue(*(const int *)value);
        return 1;
    case 0x10:
        gZrwFlags = (((uint32_t)(uintptr_t)value & 1) << 1) | (gZrwFlags & 0xfffffffdu);
        return 1;
    case 0x14:
        gZrwKeysLocked = 1;
        return 1;
    default:
        zdebugFail(zstrFormatTemp(kUnknownParameter, id), kZglFile, 0x106, 0);
        return 0;
    }
}

BMX_ADDR(0x0048ad20)
int zrwGetParameter(int id, void *out, const void *extra)
{
    float *f = out;

    switch (id) {
    case 0:
        *f = gZrwFrameLimit;
        return 1;
    case 1:
        *f = gZrwScale;
        return 1;
    case 2:
        zmemCopy(out, gZrwName, 0x20);
        return 1;
    case 3:
        *(uint32_t *)out = gZrwClearOn;
        return 1;
    case 4:
        *(uint32_t *)out = gZrwClearColor;
        return 1;
    case 5:
        *(uint32_t *)out = 0;
        return 0;
    case 6:
        if (extra == NULL) {
            zdebugFail("Invalid camera specified.", kZglFile, 0x13e, 0);
        }
        *f = RwIm2DGetNearScreenZ();
        return 1;
    case 7:
    case 8:
    case 9:
        return 1;
    case 10:
        *f = (float)gZrwScreenW;
        return 1;
    case 0xb:
        *f = (float)gZrwScreenH;
        return 1;
    case 0x11: {
        const float *v = extra;

        f[0] = (float)((double)gZrwScale * v[0]);
        f[1] = (float)-((double)v[1] * gZrwScale);
        f[2] = (float)-((double)v[2] * gZrwScale);
        return 1;
    }
    case 0x12:
        *f = (float)((double)gZrwInvScale * *(const float *)extra);
        return 1;
    case 0x13:
        *f = (float)((double)gZrwScale * *(const float *)extra);
        return 1;
    case 0x15:
        *(uint32_t *)out = (uint32_t)(uintptr_t)&gZrwWindow;
        return 1;
    default:
        zdebugFail(zstrFormatTemp(kUnknownParameter, id), kZglFile, 0x171, 0);
        return 0;
    }
}

/* Initialises the renderer subsystems in order; each returns 0 on failure. */
BMX_ADDR(0x0048afe0)
int zrwGraphicLibraryInitialize(void)
{
    zmemFill(&gZrwScreenW, 0, 0x10);
    gZrwScreenH = (uint32_t)gZrwWantH;
    gZrwScreenW = (uint32_t)gZrwWantW;
    gZrwFlags = (gZrwFlags & 0xfffffffbu) | 1;
    gZrwFrameLimit = 0.0f;
    gZrwClearOn = 1;
    gZrwClearColor2 = 0xffffffffu;
    gZrwClearColor = 0x00ddae7fu;
    gZrwInvScale = 1.0f;
    gZrwScale = 1.0f;
    if (gZglInitTexture() == 0 || gZglInitMaterial() == 0 || gZglInitModel() == 0 || gZglInitLight() == 0 ||
        gZglInitCol() == 0 || gZglInitPrim() == 0) {
        return 0;
    }
    if (zrwCreateWorld() == NULL) {
        zrwApplicationError("Cannot create world.");
        return 0;
    }
    gZrwNodePool = zmemFreeListCreate(0x14, 0x20, NULL);
    return 1;
}

BMX_ADDR(0x0048b110)
int zrwGraphicLibraryShutdown(void)
{
    if (gZrwInitialized == 0) {
        zdebugFail(kNotInitialized, kZglFile, 0x243, 0);
    }
    gZrwInitialized = 0;
    if (zrwWorldShutdown() == 0 || gZglExitPrim() == 0 || gZglExitCol() == 0 || gZglExitLight() == 0 ||
        gZglExitModel() == 0 || gZglExitMaterial() == 0 || gZglExitTexture() == 0) {
        return 0;
    }
    zmemFreeListDestroy(gZrwNodePool);
    return 1;
}

/* Records the chosen full screen video mode and hides the cursor. */
static void enterFullscreen(const RwVideoMode *mode)
{
    gZrwWantW = (int)mode->width;
    gZrwWantH = (int)mode->height;
    gZrwFullscreen = 1;
    gShowCursor(0);
}

/* Selects the default sub system and video mode (without the dialog). */
BMX_ADDR(0x00489ef0)
int zrwSelectDefaultDevice(void)
{
    RwVideoMode mode;

    if (RwEngineSetSubSystem(gZglSubSystem) == 0) {
        return 0;
    }
    if (RwEngineSetVideoMode(gZglVideoMode) == 0) {
        return 0;
    }
    RwEngineGetVideoModeInfo(&mode, gZglVideoMode);
    if ((mode.flags & 1) != 0) {
        enterFullscreen(&mode);
    }
    return 1;
}

/*
 * Lists the sub systems (at most 8), then either asks the user (device dialog) or, when width, height
 *
 * and depth were given on the command line, looks for the matching full screen mode.
 */
BMX_ADDR(0x00489d70)
int zrwSelectDevice(void)
{
    void *const instance = gZglInstance;
    void *const parent = gZrwWindow;
    RwVideoMode mode;

    gZglSubSystemCount = RwEngineGetNumSubSystems();
    if (gZglSubSystemCount == 0) {
        return 0;
    }
    if (gZglSubSystemCount > kMaxSubSystems) {
        gZglSubSystemCount = kMaxSubSystems;
    }
    for (int i = 0; i < gZglSubSystemCount; i++) {
        RwEngineGetSubSystemInfo(&gZglSubSystemNames + i * kSubSystemNameBytes, i);
    }
    gZglSubSystem = RwEngineGetCurrentSubSystem();
    if (gZglWantW2 != 0 && gZglWantH2 != 0 && gZglWantDepth != 0) {
        const uint32_t count = RwEngineGetNumVideoModes();

        for (uint32_t i = 0; i < count; i++) {
            RwEngineGetVideoModeInfo(&mode, i);
            if ((int)mode.depth == gZglWantDepth) {
                mode.width = (uint32_t)gZglWantW2;
                if (gZglWantW2 != 0 && (int)mode.height == gZglWantH2 && (mode.flags & 1) != 0) {
                    gZglVideoMode = i;
                }
            }
        }
    } else if (gDialogBox(instance, 0x68, parent, BMX_FNPTR(void *, 0x0048a330, zrwDialogProc), 0) == 0) {
        return 0;
    }
    if (RwEngineSetSubSystem(gZglSubSystem) != 0 && RwEngineSetVideoMode(gZglVideoMode) != 0) {
        RwEngineGetVideoModeInfo(&mode, gZglVideoMode);
        if ((mode.flags & 1) != 0) {
            enterFullscreen(&mode);
        }
    }
    return 1;
}

/* Starts the RenderWare engine: plugins, device selection, engine start. */
BMX_ADDR(0x0048b1a0)
int zrwAttachPlugins(void *window)
{
    if (gZrwSkipDevice == 0) {
        gZrwEngineInit = (uint32_t)zrwReturnZero();
    }
    RwEngineInit((const RwMemoryFunctions *)(uintptr_t)gZrwEngineInit, 0);
    if (RpWorldPluginAttach() == 0) {
        zrwApplicationError("RpWorldPluginAttach failed");
        return 0;
    }
    if (RpAnimPluginAttach() == 0) {
        zrwApplicationError("RpAnimPluginAttach failed");
        return 0;
    }
    if (RpBonePluginAttach() == 0) {
        zrwApplicationError("RpBonePluginAttach failed");
        return 0;
    }
    if (RpLODAtomicPluginAttach() == 0) {
        zrwApplicationError("RpLODAtomicPluginAttach failed");
        return 0;
    }
    if (RpPVSPluginAttach() == 0) {
        zrwApplicationError("RpPVSPluginAttach failed");
        return 0;
    }
    if (zrwMatRegisterPlugin() == 0) {
        zrwApplicationError("rwMatRegisterPlugin failed");
        return 0;
    }
    if (zrwMdlRegisterPlugin() == 0) {
        zrwApplicationError("zrwMdlRegisterPlugin failed");
        return 0;
    }
    gZrwEngineOpen = (uint32_t)(uintptr_t)window;
    RwEngineOpen(&gZrwEngineOpen);
    if (gZrwSkipDevice == 0) {
        if (zrwSelectDevice() == 0) {
            return 0;
        }
    } else if (zrwSelectDefaultDevice() == 0) {
        return 0;
    }
    return RwEngineStart() != 0;
}

/*
 * Resets the module state, captures the renderer's memory interface and takes the first space
 * separated word of the command line as the argument for the engine start-up. The scan steps over the
 * terminator even at the end of the string, like the original.
 */
BMX_ADDR(0x0048b450)
int zrwInitApp(const char *commandLine)
{
    if (gZrwInitialized != 0) {
        zdebugFail("Already initialized", kZglFile, 0x38c, 0);
    }
    zmemFill((void *)&gZrwRaster, 0, 0x88);
    zmemFill(&gZrwInitialized, 0, 0xa8);
    gZrwInitialized = 1;
    gRwMalloc = (ZMemAllocFn)(uintptr_t)RwOsGetMemoryInterface()->rwmalloc;
    gRwFree = (ZMemFreeFn)(uintptr_t)RwOsGetMemoryInterface()->rwfree;
    gRwRealloc = (ZMemReallocFn)(uintptr_t)RwOsGetMemoryInterface()->rwrealloc;
    gZglMemoryFuncs = (uint32_t)(uintptr_t)RwOsGetMemoryInterface()->rwcalloc;
    gZglAllocHookNop = BMX_FNPTR(ZglNopFn, 0x0048af80, zrwNop);
    if (commandLine != NULL) {
        while (*commandLine != '\0') {
            char *dst = &gZrwArgBuffer;

            while (*commandLine != '\0' && *commandLine != ' ') {
                *dst++ = *commandLine++;
            }
            *dst = '\0';
            commandLine++;
            if (gZrwArgBuffer != '\0') {
                gZrwArgument = &gZrwArgBuffer;
                break;
            }
        }
    }
    if (zgeInitializeSubsystems(gZrwArgument) == 0) {
        return 0;
    }
    if (ngifInitialize() == 0) {
        return 0;
    }
    gZrwFpsTime = zrwTimeGetTime();
    gZrwLastTime = gZrwFpsTime;
    return 1;
}

/* Calls the callbacks of a list until one fails (and says which). */
BMX_ADDR(0x0048b9e0)
int zrwRunCallbacks(ZrwList *list)
{
    ZrwUpdateNode *node = list->head;

    while (node != NULL) {
        ZrwUpdateNode *const next = node->next;

        if (node->fn(node->arg) == 0) {
            gZdebugPrint(kFailedCall, node->fn);
            return 0;
        }
        node = next;
    }
    return 1;
}

/* Runs a list through its head pointer stored in `head` (the original inlines this at every use). */
static void runListAt(ZrwUpdateNode *head)
{
    while (head != NULL) {
        ZrwUpdateNode *const next = head->next;

        if (head->fn(head->arg) == 0) {
            gZdebugPrint(kFailedCall, head->fn);
            return;
        }
        head = next;
    }
}

/* One idle step: measures the frame time, runs the idle lists around the game frame; 0 ends the program. */
BMX_ADDR(0x0048b570)
int zrwOnIdle(void)
{
    const uint32_t now = zrwTimeGetTime();
    float dt;
    int frames;

    if (now < gZrwLastTime) {
        dt = 0.0f;
    } else {
        dt = (float)((double)(now - gZrwLastTime) * 0x1.0624dep-10f);
    }
    frames = gZrwFrames + 1;
    gZrwFrames = frames;
    if (gZrwFpsTime + 1000 <= now) {
        gZrwFrames = 0;
        gZrwFpsTime = now;
        gZrwFps = frames;
    }
    gZrwLastTime = now;
    runListAt(gZrwUpdateLists[7].head);
    if (ngifRunFrame(dt) == 0) {
        gZrwQuit = 1;
        return 0;
    }
    runListAt(gZrwUpdateLists[8].head);
    return 1;
}

/* Renders one frame: clears the cameras, runs the camera and render pass lists, shows the raster. */
BMX_ADDR(0x0048b690)
int zrwDrawFrame(void)
{
    void *camera;

    if (gZrwRaster == NULL) {
        return 1;
    }
    if (gZrwClearOn != 0) {
        for (camera = zrwcamFirst(); camera != NULL; camera = zrwcamNext(camera)) {
            if ((((const uint8_t *)camera)[8] & 4) != 0) {
                RwCameraClear(zrwcamHandle(camera), &gZrwClearColor, 3);
            }
        }
    }
    runListAt(gZrwUpdateLists[0].head);
    for (camera = zrwcamFirst(); camera != NULL; camera = zrwcamNext(camera)) {
        RwError error;

        if ((((const uint8_t *)camera)[8] & 4) == 0) {
            continue;
        }
        RwErrorGet(&error);
        if (zrwcamBegin(camera) == 0) {
            continue;
        }
        if (gZrwDebugLightOn != 0) {
            rwLightBegin(gZrwDebugLight);
            /* The matrix position (+0x30 bytes); `+ 0x30` broke once RwFrameGetMatrix returned RwMatrix *. */
            rwLightSetMatrix(gZrwDebugLight, &RwFrameGetMatrix(RwCameraGetFrame(zrwcamHandle(camera)))->pos);
        }
        runListAt(gZrwUpdateLists[1].head);
        if (zrwtxrFlush() == 0) {
            return 0;
        }
        if (gZrwDebugLightOn != 0) {
            rwLightSetParams(gZrwDebugLight, &gZrwLightParamA, &gZrwLightParamB);
            rwLightEnd(gZrwDebugLight);
        }
        zrwResetRenderState();
        runListAt(gZrwUpdateLists[2].head);
        zrwResetRenderState();
        runListAt(gZrwUpdateLists[5].head);
        runListAt(gZrwUpdateLists[4].head);
        if (zrwcamEnd() == 0) {
            return 0;
        }
        RwErrorGet(&error);
        if (error.errorCode != rwERRNONE) {
            zdebugFail("There is a pending RenderWare error. Rendering has been aborted.", kZglFile, 0x45f,
                       0);
            return 0;
        }
    }
    runListAt(gZrwUpdateLists[3].head);
    RwRasterShowRaster(gZrwRaster, gZrwWindow, 1);
    return 1;
}

BMX_ADDR(0x0048b950)
void zrwShowRaster(int vsync)
{
    RwRasterShowRaster(gZrwRaster, gZrwWindow, vsync != 0);
}

BMX_ADDR(0x0048b990)
ZrwUpdateNode *zrwAddCallback(ZrwList *list, ZrwUpdateFn fn, void *arg, int append)
{
    ZrwUpdateNode *node = zmemFreeListAlloc(gZrwNodePool);

    if (node == NULL) {
        return NULL;
    }
    if (append == 0) {
        zrwListPrepend(list, node);
    } else {
        zrwListAdd(list, node);
    }
    node->list = list;
    node->fn = fn;
    node->arg = arg;
    return node;
}

BMX_ADDR(0x0048ba30)
ZrwUpdateNode *zrwRegisterUpdateFunction(int type, ZrwUpdateFn fn, void *arg, int append)
{
    ZrwList *list;
    ZrwUpdateNode *node;

    if (gZrwInitialized == 0) {
        zdebugFail(kNotInitialized, kZglFile, 0x4ed, 0);
    }
    if (type >= kZrwUpdateListCount) {
        zdebugFail("Invalid type", kZglFile, 0x4f0, 0);
    }
    list = &gZrwUpdateLists[0] + type;
    node = zmemFreeListAlloc(gZrwNodePool);
    if (node == NULL) {
        return NULL;
    }
    if (append == 0) {
        zrwListPrepend(list, node);
    } else {
        zrwListAdd(list, node);
    }
    node->list = list;
    node->arg = arg;
    node->fn = fn;
    return node;
}

BMX_ADDR(0x0048bad0)
int zrwUnregisterUpdateFunction(ZrwUpdateNode *node)
{
    if (node == NULL) {
        zdebugFail("NULL", kZglFile, 0x506, 0);
        return 0;
    }
    zrwListRemove(node->list, node);
    zmemFreeListFree(gZrwNodePool, node);
    return 1;
}

BMX_ADDR(0x0048bb20)
void zrwRunUpdateList(int type)
{
    runListAt(gZrwUpdateLists[type].head);
}

/* Sets the default render states (flags 0x6502, extra 0). */
BMX_ADDR(0x0048bb70)
void zrwResetRenderState(void)
{
    int level;

    gZrwRenderExtra = 0;
    gZrwRenderFlags = 0x6502;
    RwRenderStateSet(1, 0);
    level = (int)((gZrwRenderFlags >> 4 & 2) + 1);
    RwRenderStateSet(2, level);
    RwRenderStateSet(0x12, level);
    RwRenderStateSet(0x13, level);
    RwRenderStateSet(3, 1);
    RwRenderStateSet(4, 1);
    RwRenderStateSet(5, 2);
    RwRenderStateSet(6, 1);
    RwRenderStateSet(7, 2);
    RwRenderStateSet(10, (int)(~gZrwRenderFlags >> 3 & 1));
    RwRenderStateSet(0xb, -1);
    RwRenderStateSet(0xc, 0);
    RwRenderStateSet(0xd, -1);
    RwRenderStateSet(0xe, 1);
    RwRenderStateSet(0x11, 1);
    RwRenderStateSet(8, (int)(gZrwRenderFlags >> 8 & 0xf));
    RwRenderStateSet(9, (int)(gZrwRenderFlags >> 12 & 0xf));
}

/* Applies only the render states whose flag bits differ from the current ones. */
BMX_ADDR(0x0048bc60)
int zrwSetRenderState(uint32_t flags, int extra)
{
    uint32_t diff = gZrwRenderFlags ^ flags;

    if (diff != 0) {
        if ((diff & 1) != 0) {
            const int off = (flags & 1) == 0;

            RwRenderStateSet(4, off);
            RwRenderStateSet(6, off);
        }
        if ((diff & 4) != 0) {
            RwRenderStateSet(7, (flags & 4) != 0 ? 1 : 2);
        }
        if ((diff & 0x20) != 0) {
            const int level = (int)((flags >> 4 & 2) + 1);

            RwRenderStateSet(2, level);
            RwRenderStateSet(0x12, level);
            RwRenderStateSet(0x13, level);
        }
        if ((diff & 0xff02) != 0) {
            if ((flags & 2) == 0) {
                flags = (flags & 0xffff12ffu) | 0x1208;
                diff = gZrwRenderFlags ^ flags;
            }
            if ((diff >> 8 & 0xff) != 0) {
                RwRenderStateSet(8, (int)(flags >> 8 & 0xf));
                RwRenderStateSet(9, (int)(flags >> 12 & 0xf));
            }
        }
        if ((diff & 8) != 0) {
            RwRenderStateSet(10, (flags & 8) == 0);
        }
        gZrwRenderFlags = flags;
    }
    if (extra != gZrwRenderExtra) {
        RwRenderStateSet(1, extra);
        gZrwRenderExtra = extra;
    }
    return 1;
}

BMX_ADDR(0x0048bd80)
int zrwPushRenderState(void)
{
    const int extra = gZrwRenderExtra;

    if (gZrwStateDepth == kZrwStateStackSize) {
        gZdebugPrint("Render state stack is full");
        return 0;
    }
    gZrwStateStack[gZrwStateDepth].flags = gZrwRenderFlags;
    gZrwStateStack[gZrwStateDepth].extra = extra;
    gZrwStateDepth++;
    return 1;
}

BMX_ADDR(0x0048bdd0)
int zrwPopRenderState(void)
{
    ZrwStateEntry entry;

    if (gZrwStateDepth == 0) {
        gZdebugPrint("Render state stack is empty");
        return 0;
    }
    gZrwStateDepth--;
    entry = gZrwStateStack[gZrwStateDepth];
    if (entry.flags != gZrwRenderFlags || entry.extra != gZrwRenderExtra) {
        gZglRenderStateHook(entry.flags, entry.extra);
    }
    return 1;
}

/*
 * Converts a frame matrix (4x4 floats, rows right/up/at/pos) to the engine's handedness: the Y and Z
 * columns flip and the position is re-based on the scene origin and scaled.
 */
BMX_ADDR(0x0048be30)
void zrwConvertMatrix(float *out, const float *in)
{
    out[0] = in[0];
    out[1] = -in[1];
    out[2] = -in[2];
    out[4] = -in[4];
    out[5] = in[5];
    out[6] = in[6];
    out[8] = -in[8];
    out[9] = in[9];
    out[10] = in[10];
    out[12] = (float)(((double)in[12] - gZrwOriginX) * gZrwScale);
    out[13] = (float)-(((double)in[13] - gZrwOriginY) * gZrwScale);
    out[14] = (float)-(((double)in[14] - gZrwOriginZ) * gZrwScale);
}

/*
 * Plays a movie full screen and pumps the window messages until it ends, a pad button is pressed or
 * the program quits. Returns 0 when the program is quitting.
 */
BMX_ADDR(0x0048bed0)
int zrwPlayMovie(const char *name)
{
    char path[0x200];
    uint8_t msg[0x1c];
    uint32_t aborted = 0;
    void *file;

    zstrcpy(path, name);
    file = rwfopen(path, kReadBinary);
    if (file == NULL) {
        return 1;
    }
    rwfclose(file);
    if (gMovieSkip == 0) {
        uint32_t tick;
        uint32_t end;

        zmovieInit(&gMovie);
        if (zmovieOpen(&gMovie, path) == 0) {
            return 1;
        }
        zmoviePlay(&gMovie, gZrwWindow, gZrwFullscreen, 0, NULL);
        tick = gTimeGetTime();
        do {
            if (gPeekMessage(msg, NULL, 0, 0, 1) != 0) {
                const uint32_t message = *(const uint32_t *)(msg + 4);

                if (message == kMessageAudioTick) {
                    gZdebugPrint("Movie : Audio Tick\n");
                    zmovieRefillAudio(&gMovie);
                } else {
                    if (message == kMessageQuit) {
                        rwExit(-1);
                    }
                    gTranslateMessage(msg);
                    gDispatchMessage(msg);
                }
            } else {
                if (gTimeGetTime() - tick >= 0x28) {
                    tick = gTimeGetTime();
                    zmovieUpdate(&gMovie, NULL, NULL);
                    zpadPoll();
                    for (int i = 0; i < 2; i++) {
                        const uint32_t *pad = (const uint32_t *)zpadGetDevice(i);

                        if (pad != NULL && pad[2] != 0) {
                            aborted |= (pad[3] & pad[2]) != 0;
                        }
                    }
                }
                gSleep(1);
            }
            if (aborted != 0) {
                gZdebugPrint("Movie : Key Abort\n");
            }
            aborted |= (uint32_t)(zmovieIsPlaying(&gMovie) == 0) | (uint32_t)gZrwQuit;
        } while (aborted == 0);
        gZdebugPrint("Movie : Done Playing\n");
        zmovieShutdown(&gMovie);
        end = gTimeGetTime();
        while (gTimeGetTime() - end < 500) {
            gSleep(0x14);
            zpadPoll();
            if (gPeekMessage(msg, NULL, 0, 0, 1) != 0 && *(const uint32_t *)(msg + 4) != kMessageAudioTick) {
                gTranslateMessage(msg);
                gDispatchMessage(msg);
            }
        }
    }
    return gZrwQuit == 0;
}

BMX_ADDR(0x00489cc0)
void zrwApplicationError(const char *text)
{
    gMessageBox(NULL, text, gErrorCaption, 0x42010);
}

/* Assertion box: Yes breaks into the debugger, Cancel quits, No carries on. */
BMX_ADDR(0x00489cf0)
void zrwEpicFail(const char *why, const char *file, int line, const char *function)
{
    char *const gMessage = gEpicMessage;
    int answer;

    if (function == NULL) {
        zstrFormat(gMessage,
                   "***ASSERT***\n\nFile:%s\nLine:%d\nWhy:%s\n\nDo you wish to debug the "
                   "application?\n('Cancel' will terminate the application, 'No' will continue)",
                   file, line, why);
    } else {
        zstrFormat(gMessage,
                   "***ASSERT***\n\nFunc:%s\nFile:%s\nLine:%d\nWhy:%s\n\nDo you wish to debug the "
                   "application?\n('Cancel' will terminate the application, 'No' will continue)",
                   function, file, line, why);
    }
    answer = gMessageBox(NULL, gMessage, gErrorCaption, 0x42013);
    if (answer == 6) {
        gDebugBreak();
        return;
    }
    if (answer == 2) {
        gPostQuit(0);
    }
}

/* "Do you want to quit the game?" (releases the DirectDraw GDI surface first when full screen). */
BMX_ADDR(0x0048a9b0)
int zrwConfirmQuit(void)
{
    if (gZrwFullscreen != 0 && gDirectDraw != NULL) {
        void *const dd = gDirectDraw;

        (*(const DdVtbl *const *)dd)->getGdiSurface(dd, &gGdiSurface);
        (*(const DdSurfaceVtbl *const *)gGdiSurface)->release(gGdiSurface);
    }
    return gMessageBox(gZrwWindow, "Do you want to quit the game?", "Dave Mirra Freestyle BMX", 0x10124) == 6;
}

/* Does the width/height pair belong to the listed modes (480p..1200p)? */
static int isListedMode(uint32_t width, uint32_t height)
{
    switch (height) {
    case 480:
        return width == 640;
    case 600:
        return width == 800;
    case 768:
        return width == 1024;
    case 1024:
        return width == 1280;
    case 1200:
        return width == 1600;
    default:
        return 0;
    }
}

/* Adds the label of video mode `index` to the dialog's list: "%lu x %lu x %lu %s". */
static uintptr_t addModeLabel(void *list, const RwVideoMode *mode, int index)
{
    char label[100];
    uintptr_t item;

    gZrwStringApi->format(label, "%lu x %lu x %lu %s", mode->width, mode->height, mode->depth,
                          (mode->flags & 1) != 0 ? "(Fullscreen)" : "");
    item = gSendMessage(list, 0x143, 0, (uintptr_t)label);
    gSendMessage(list, 0x151, item, (uintptr_t)index);
    return item;
}

/*
 * Fills the sub system list (software renderers are left out) and the video mode list of the device
 *
 * dialog, and selects the current sub system and 640x480x16 (or the current mode).
 */
BMX_ADDR(0x00489f70)
void zrwDialogInit(void *dialog)
{
    void *const subSystems = gGetDlgItem(dialog, 0x3e8);
    void *const modes = gGetDlgItem(dialog, 0x3e9);
    uintptr_t item = (uintptr_t)modes;
    uintptr_t selected = 0;
    uintptr_t selectedMode;
    int modeIndex = 0;
    int count;

    for (int i = 0; i < gZglSubSystemCount; i++) {
        const char *name = (const char *)&gZglSubSystemNames + i * kSubSystemNameBytes;

        if (zstrcmp(name + zstrlen(name) - 3, "S/W") != 0) {
            item = gSendMessage(subSystems, 0x143, 0, (uintptr_t)name);
            gSendMessage(subSystems, 0x151, item, (uintptr_t)i);
        }
        if (i == gZglSubSystem) {
            selected = item;
        }
    }
    gSendMessage(subSystems, 0x14e, selected, 0);
    count = (int)RwEngineGetNumVideoModes();
    selectedMode = (uintptr_t)RwEngineGetCurrentVideoMode();
    for (int i = 0; i < count; i++) {
        RwVideoMode mode;

        RwEngineGetVideoModeInfo(&mode, (uint32_t)i);
        if ((mode.depth == 16 || mode.depth == 32) && isListedMode(mode.width, mode.height)) {
            item = addModeLabel(modes, &mode, i);
        }
        if (mode.depth == 16 && mode.height == 480 && mode.width == 640) {
            selectedMode = item;
            modeIndex = i;
        }
    }
    gZglVideoMode = (uint32_t)modeIndex;
    gSendMessage(modes, 0x14e, selectedMode, 0);
    gSetFocus(subSystems);
}

/*
 * The sub system list changed: refills the video mode list. It applies the previous sub system, not the
 *
 * new selection (original quirk).
 */
BMX_ADDR(0x0048a1a0)
void zrwDialogSelectDevice(void *dialog)
{
    void *const subSystems = gGetDlgItem(dialog, 0x3e8);
    void *modes;
    int count;

    gGetDlgItem(dialog, 0x3e9);
    if ((int)gSendMessage(subSystems, 0x150, gSendMessage(subSystems, 0x147, 0, 0), 0) == gZglSubSystem) {
        return;
    }
    RwEngineSetSubSystem(gZglSubSystem);
    modes = gGetDlgItem(dialog, 0x3e9);
    gSendMessage(modes, 0x14b, 0, 0);
    count = (int)RwEngineGetNumVideoModes();
    for (int i = 0; i < count; i++) {
        RwVideoMode mode;

        RwEngineGetVideoModeInfo(&mode, (uint32_t)i);
        if ((mode.depth == 16 || mode.depth == 32) && isListedMode(mode.width, mode.height)) {
            addModeLabel(modes, &mode, i);
        }
    }
    gZglVideoMode = (uint32_t)RwEngineGetCurrentVideoMode();
    gSendMessage(modes, 0x14e, gZglVideoMode, 0);
}

/* Device dialog: 0x3e8 sub system list, 0x3e9 video mode list, 1 OK, 0x3ea cancel. */
BMX_ADDR(0x0048a330)
int BMX_STDCALL zrwDialogProc(void *dialog, uint32_t message, uint32_t wparam, uintptr_t lparam)
{
    switch (message) {
    case 0x10:
        gEndDialog(dialog, 0);
        return 1;
    case 0x110:
        zrwDialogInit(dialog);
        return 0;
    case 0x111:
        switch (wparam & 0xffff) {
        case 0x3e8:
            zrwDialogSelectDevice(dialog);
            return 1;
        case 1:
            if (wparam >> 16 == 0) {
                gEndDialog(dialog, 1);
            }
            return 1;
        case 0x3e9:
            if (wparam >> 16 == 1) {
                void *const modes = gGetDlgItem(dialog, 0x3e9);

                gZglVideoMode = (uint32_t)gSendMessage(modes, 0x150, gSendMessage(modes, 0x147, 0, 0), 0);
            }
            return 1;
        case 0x3ea:
            if (wparam >> 16 == 0) {
                gEndDialog(dialog, 0);
            }
            return 1;
        default:
            return 0;
        }
    default:
        return 0;
    }
}

/* Main window procedure: activation, destruction, size limits and the quit key. */
BMX_ADDR(0x0048a800)
intptr_t BMX_STDCALL zrwMainWndProc(void *window, uint32_t message, uint32_t wparam, uintptr_t lparam)
{
    switch (message) {
    case 6:
        if (wparam == 1) {
            zrwOnActivate();
        } else {
            zrwOnDeactivate();
        }
        return 0;
    case 1:
    case 3:
        return 0;
    case 2:
        gClipCursor(NULL);
        gPostQuit(0);
        return 0;
    case 0x24: {
        int32_t rect[4] = {0, 0, gZrwWantW, gZrwWantH};
        uint32_t *const info = (uint32_t *)lparam;

        gAdjustWindowRect(rect, 0xcf0000, 0);
        info[2] = (uint32_t)(rect[2] - rect[0]);
        info[3] = (uint32_t)(rect[3] - rect[1]);
        info[8] = info[2];
        info[9] = info[3];
        if (gZrwFullscreen != 0) {
            info[6] = info[2];
            info[7] = info[3];
        }
        return 0;
    }
    case 0x100: {
        uint32_t key = (uint8_t)(lparam >> 16);

        if ((lparam & 0x1000000) != 0) {
            key |= 0x80;
        }
        if (gZrwKeysLocked == 0 && gZrwKeyActions[key] == 0x80) {
            gZrwQuit = 1;
        }
        return 0;
    }
    default:
        return gDefWindowProc(window, message, wparam, lparam);
    }
}
