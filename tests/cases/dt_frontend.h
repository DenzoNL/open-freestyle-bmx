/* Shared fixtures for the front-end tests (test_frontend*.c). */
#ifndef DT_FRONTEND_H
#define DT_FRONTEND_H

#include <stdint.h>

#include "bmx/com/menu/gui/gui.h"
#include "bmx/com/menu/gui/guiinput.h"
#include "compat/orig.h"
#include "dt.h"
#include "dt_gui.h"

/* Calls to stubbed callees are folded into this log; it lives in the DLL so both runs get compared. */
typedef struct DtFrontLog {
    uint32_t calls;
    uint32_t hash;
    uint32_t entries[48][5]; /* the first calls verbatim, so a mismatch shows which argument differs */
} DtFrontLog;

extern DtFrontLog dt_front_log;

/* Records one stubbed call: `who` identifies the callee, then up to four argument words. */
void dtFrontLog(uint32_t who, uint32_t a, uint32_t b, uint32_t c, uint32_t d);

/* Clears the GUI state and the log and sets up the handle heap. */
void dtFrontendReset(void);

/*
 * Creates a registered label element with the given id in the next free slot: visible or hidden at
 * random, random text (as a handle) and tint state. Returns it.
 */
GuiElement *dtFrontendElement(uint32_t id);

/* Elements first, first+1, ..., first+count-1. */
void dtFrontendElementRange(uint32_t first, int count);

/* `n` elements with ids ids[0..n-1]. */
void dtFrontendElements(const uint32_t *ids, int n);

/* Stub groups for dtFrontendStub(). */
enum {
    DT_FS_SOUND = 1,   /* bmxSoundPlayAt */
    DT_FS_ZPRIM = 2,   /* zprimSet, zprimFillRect, zprimDrawLine, zprimSetClip */
    DT_FS_SCREENS = 4, /* guiRequestScreen, guiDestroyAll, guiLoadBackdrop */
    DT_FS_ZASSET =
        256, /* group/texture calls of the zprim layer: zprimGroupLoaded/Count/Select, zprimLoadGroup, ... */
    DT_FS_FRAME = 128, /* zprimCreateSpriteSmall, zprimLoadAsset, zprimCopyTexture, zprimFreeSprite
                          (menuFrameInit/Free) */
    DT_FS_PAD = 32,    /* zpadGetDevice returns dtFrontPad */
    DT_FS_LABEL = 64,  /* guiLabelProc: logs the message and the element's text state instead of drawing */
    DT_FS_QUIT = 16,   /* zrwConfirmQuit (answer from dtFrontQueue) and frontendRequestExit */
    DT_FS_CARD = 8,    /* saveFileClose/Read/Write, memcardFormat, memcardQuery: results from dtFrontQueue */
    DT_FS_ASSET = 512, /* zassetFind/Size/Read: the string set blob of dtFrontendAssetBlob() */
    DT_FS_EXTRAS = 1024, /* moviePlay, guiModeInit/Shutdown, zprimSetPicture, zprimDrawSprite */
    DT_FS_INIT = 2048,   /* start-up calls: appLog, appInitCard*, guiInit, guiPadInit, the debug print */
};

/* The device the stubbed zpadGetDevice returns; `held` is up to the test. */
extern GuiPadDevice *dt_front_pad;

/* The display list's ordering table pointer (0x68c3d0). */
#define gNgifOrderTable ORIG_VAR(0x0068c3d0, uint32_t *, gNgifOrderTable)

/* Results of the stubbed card functions: the n-th call returns values[n % count]. */
enum {
    DT_Q_CLOSE,
    DT_Q_READ,
    DT_Q_WRITE,
    DT_Q_FORMAT,
    DT_Q_QUERY,
    DT_Q_CONFIRM,
    DT_Q_GROUPLOADED,
    DT_Q_GROUPCOUNT,
    DT_Q_ASSETFIND, /* default 1 */
    DT_Q_ASSETSIZE, /* 0 = no data, otherwise the size of the asset blob */
    DT_Q_MODEINIT,
    DT_Q_GUIINIT,
    DT_Q_COUNT
};
void dtFrontQueue(int which, const int *values, int count);
/* Queues for all five: each gets 1..3 random values from the card status codes. */
void dtFrontRandomCardResults(void);

/* The string set the asset stubs return (ids 0x1000..) and its size. */
const uint8_t *dtFrontendAssetBlob(uint32_t *size);

/* Stubs the callees of the groups for this iteration; the stubs log their arguments. */
void dtFrontendStub(uint32_t groups);

/* A step function (gFrontendStep) that only logs that it ran. */
void BMX_CDECL dtFrontStep(void);

/*
 * Arguments of a screen hook call (msg, element, arg3, arg4): a message from the list (one in eight is a
 * random other message), an element of the screen, random arguments.
 */
void dtFrontendHookArgs(DtCtx *c, const int *msgs, int count);

/* Replaces the screen hook-visible text of an element (empty or a short random one). */
void dtFrontendRandomText(GuiElement *el);

#endif
