/* Application glue around the renderer (C:\NgEngine\Zge\Com\ZglRw\zrwZgl.c): parameters, update-function
 * lists, render states and the frame loop. WinMain, the window procedure and the device dialogs (Win32 UI)
 * stay in the exe. */
#ifndef ZGE_COM_ZGLRW_ZRWZGL_H
#define ZGE_COM_ZGLRW_ZRWZGL_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zfile.h"
#include "zge/zmem.h"

/* Intrusive doubly linked list: nodes start with { next, prev } pointers. */
typedef struct ZrwList {
    void *head;
    void *tail;
    int count;
} ZrwList;

void zrwListInit(ZrwList *list);
/* Inserts a node at the head. */
void zrwListPrepend(ZrwList *list, void *node);
/* Appends a node. */
void zrwListAdd(ZrwList *list, void *node);
void zrwListRemove(ZrwList *list, void *node);

/* A registered per-frame callback; it returns 0 on failure. */
typedef int(BMX_CDECL *ZrwUpdateFn)(void *arg);
typedef struct ZrwUpdateNode {
    struct ZrwUpdateNode *next; /* 0x00 */
    struct ZrwUpdateNode *prev; /* 0x04 */
    ZrwList *list;              /* 0x08 */
    ZrwUpdateFn fn;             /* 0x0c */
    void *arg;                  /* 0x10 */
} ZrwUpdateNode;

/* Number of callback lists: 0 before the cameras, 1 per camera, 2..5 render passes, 7 and 8 idle. */
enum { kZrwUpdateListCount = 9 };
typedef ZrwList ZrwUpdateListTable[kZrwUpdateListCount];

/* Render state snapshot pushed by zrwPushRenderState. */
typedef struct ZrwStateEntry {
    uint32_t flags;
    const void *raster;
} ZrwStateEntry;
enum { kZrwStateStackSize = 4 };
typedef ZrwStateEntry ZrwStateStack[kZrwStateStackSize];

#define gZrwInitialized ORIG_VAR(0x00672760, int, gZrwInitialized)
#define gZrwArgument    ORIG_VAR(0x00672764, char *, gZrwArgument)
#define gZrwUpdateLists ORIG_VAR(0x00672768, ZrwUpdateListTable, gZrwUpdateLists)
#define gZrwStateStack  ORIG_VAR(0x006727d4, ZrwStateStack, gZrwStateStack)
#define gZrwStateDepth  ORIG_VAR(0x006727f4, int, gZrwStateDepth)
#define gZrwNodePool    ORIG_VAR(0x006727f8, ZMemFreeList *, gZrwNodePool)
#define gZrwSkipDevice  ORIG_VAR(0x006727fc, int, gZrwSkipDevice)
#define gZrwEngineOpen                                                                                       \
    ORIG_VAR(0x00672800, void *, gZrwEngineOpen) /* RwEngineOpen's parameters: the window */
#define gZrwEngineInit ORIG_VAR(0x00672804, uint32_t, gZrwEngineInit)
#define gZrwArgBuffer  ORIG_VAR(0x00672808, char, gZrwArgBuffer)

#define gZrwRaster       ORIG_VAR(0x006976c0, void *, gZrwRaster)
#define gZrwDebugLight   ORIG_VAR(0x006976c8, void *, gZrwDebugLight)
#define gZrwOriginX      ORIG_VAR(0x006976cc, float, gZrwOriginX)
#define gZrwOriginY      ORIG_VAR(0x006976d0, float, gZrwOriginY)
#define gZrwOriginZ      ORIG_VAR(0x006976d4, float, gZrwOriginZ)
#define gZrwDebugLightOn ORIG_VAR(0x006976e8, int, gZrwDebugLightOn)
#define gZrwLightParamA  ORIG_VAR(0x006976ec, uint32_t, gZrwLightParamA)
#define gZrwLightParamB  ORIG_VAR(0x006976f0, uint32_t, gZrwLightParamB)
typedef char ZrwName[0x20];
#define gZrwName        ORIG_VAR(0x006976fc, ZrwName, gZrwName)
#define gZrwFrameLimit  ORIG_VAR(0x0069771c, float, gZrwFrameLimit)
#define gZrwInvScale    ORIG_VAR(0x00697720, float, gZrwInvScale)
#define gZrwScale       ORIG_VAR(0x00697724, float, gZrwScale)
#define gZrwFlags       ORIG_VAR(0x00697728, uint32_t, gZrwFlags)
#define gZrwClearOn     ORIG_VAR(0x0069772c, int, gZrwClearOn)
#define gZrwClearColor  ORIG_VAR(0x00697730, uint32_t, gZrwClearColor)
#define gZrwClearColor2 ORIG_VAR(0x00697734, uint32_t, gZrwClearColor2)
#define gZrwLastTime    ORIG_VAR(0x00697738, uint32_t, gZrwLastTime)
#define gZrwFpsTime     ORIG_VAR(0x0069773c, uint32_t, gZrwFpsTime)
#define gZrwFps         ORIG_VAR(0x00697740, int, gZrwFps)
#define gZrwFrames      ORIG_VAR(0x00697744, int, gZrwFrames)
#define gZrwScreenW     ORIG_VAR(0x00697770, uint32_t, gZrwScreenW)
#define gZrwScreenH     ORIG_VAR(0x00697774, uint32_t, gZrwScreenH)
#define gZrwRenderFlags ORIG_VAR(0x00697778, uint32_t, gZrwRenderFlags)
#define gZrwRenderExtra ORIG_VAR(0x0069777c, const void *, gZrwRenderExtra)
#define gZrwWindow      ORIG_VAR(0x00697780, void *, gZrwWindow)
#define gZrwFullscreen  ORIG_VAR(0x00697788, int, gZrwFullscreen)
#define gZrwWantW       ORIG_VAR(0x00697798, int, gZrwWantW)
#define gZrwWantH       ORIG_VAR(0x0069779c, int, gZrwWantH)
#define gZrwQuit        ORIG_VAR(0x006977ac, int, gZrwQuit)
#define gZrwKeysLocked  ORIG_VAR(0x006977b0, int, gZrwKeysLocked)

/* The exe's timeGetTime import (winmm), called through its import slot. */
typedef uint32_t(BMX_STDCALL *ZrwTimeFn)(void);
#define gTimeGetTime ORIG_VAR(0x00561480, ZrwTimeFn, gTimeGetTime)
/* The exe's OutputDebugStringA import. */
typedef void(BMX_STDCALL *ZrwOutputDebugFn)(const char *text);
#define gOutputDebugString ORIG_VAR(0x006995f0, ZrwOutputDebugFn, gOutputDebugString)

/* Prints the number of live material objects (debug). */
void zrwPrintMaterialUsage(void);
/* timeGetTime() through the import slot. */
uint32_t zrwTimeGetTime(void);
void zrwDebugOutput(const char *text);
/* Registers the platform file devices into a ZfileDeviceTable (count, then device pointers); 0 on failure. */
int zrwFileSystemsInitialize(ZfileDeviceTable *table);
int zrwReturnZero(void);
/* Sets a renderer parameter; 0 for an unknown id. */
int zrwSetParameter(int id, const void *value, const void *extra);
/* Reads a renderer parameter into *out; 0 for an unknown id (or id 5). */
int zrwGetParameter(int id, void *out, const void *extra);
void zrwNop(void);
int zrwGraphicLibraryInitialize(void);
int zrwGraphicLibraryShutdown(void);
/* Message boxes and the Win32 window / dialog procedures. */
void zrwApplicationError(const char *text);
void zrwEpicFail(const char *why, const char *file, int line, const char *function);
int zrwConfirmQuit(void);
void zrwDialogInit(void *dialog);
void zrwDialogSelectDevice(void *dialog);
/* stdcall; the label keeps the symbol undecorated for the generated hook table. */
#if defined(BMX_HOOK)
int BMX_STDCALL zrwDialogProc(void *dialog, uint32_t message, uintptr_t wparam,
                              uintptr_t lparam) __asm__("_zrwDialogProc");
intptr_t BMX_STDCALL zrwMainWndProc(void *window, uint32_t message, uintptr_t wparam,
                                    uintptr_t lparam) __asm__("_zrwMainWndProc");
#else
int BMX_STDCALL zrwDialogProc(void *dialog, uint32_t message, uintptr_t wparam, uintptr_t lparam);

#endif

/* Sub system and video mode selection (the device dialog stays in the exe). */
int zrwSelectDevice(void);
int zrwSelectDefaultDevice(void);
int zrwAttachPlugins(void *window);
int zrwInitApp(const char *commandLine);
int zrwOnIdle(void);
int zrwDrawFrame(void);
void zrwShowRaster(int vsync);
/* Registers a callback in list `list`, at the head when `append` is 0; returns the node or NULL. */
ZrwUpdateNode *zrwAddCallback(ZrwList *list, ZrwUpdateFn fn, void *arg, int append);
/* Calls the callbacks of a list until one fails; 0 if one failed. */
int zrwRunCallbacks(ZrwList *list);
ZrwUpdateNode *zrwRegisterUpdateFunction(int type, ZrwUpdateFn fn, void *arg, int append);
int zrwUnregisterUpdateFunction(ZrwUpdateNode *node);
void zrwRunUpdateList(int type);
void zrwResetRenderState(void);
int zrwSetRenderState(uint32_t flags, const void *raster);
int zrwPushRenderState(void);
int zrwPopRenderState(void);
void zrwConvertMatrix(float *out, const float *in);
int zrwPlayMovie(const char *name);

#endif
