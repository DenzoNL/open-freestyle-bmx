/*
 * The game's window, its message queue and the USER32 functions the game calls, on SDL3 (docs/PLATFORM.md
 * P15, P17, slice S10). SDL creates and owns the window; the handle the game keeps (gZrwWindow 0x697780, and
 * the HWND every Win32 call takes) is an opaque token that only goes back into our stand-ins (today the
 * SDL_Window pointer itself; bmxWindowNative gives the HWND to the dev checks that call real DLLs).
 *
 * The game's window procedure (zrwMainWndProc 0x48a800) gets the messages it handled under Windows:
 * - sent (called at once, as Win32 sends them): WM_GETMINMAXINFO and WM_CREATE at creation; WM_ACTIVATE
 *   WA_ACTIVE / WA_INACTIVE when SDL's keyboard focus comes to / leaves the window (activation through a
 * click is WA_CLICKACTIVE under Windows, which the procedure treats as a deactivation; SDL does not tell them
 * apart, and the game's poll re-acquires the devices either way); WM_CLOSE when SDL reports a close request
 * (the close box, Alt+F4), whose default handling destroys the window: WM_ACTIVATE WA_INACTIVE (if active),
 *   WM_DESTROY (ClipCursor + PostQuitMessage in the procedure), WM_NCDESTROY.
 * - posted (returned by PeekMessageA, dispatched by the loop): WM_KEYDOWN / WM_KEYUP (WM_SYSKEYDOWN / UP with
 *   Alt or F10) with the scan code in lParam (the procedure's Esc quit until the key lock), from SDL's key
 *   events, or in a BMX_KEYS run from the script's key edges; anything bmxWindowPost gets from other threads
 *   (waveout.c's MM_WOM_*); WM_QUIT after PostQuitMessage once nothing else is queued.
 * Posted messages come before input messages, as in Windows' queue.
 *
 * MessageBoxA becomes SDL_ShowMessageBox with the same buttons and return codes; in a BMX_KEYS run the box is
 * answered without a window, and the main menu's quit prompt is answered Yes without a box unless
 * `[ui] confirm_quit = 1` (the user's call, PLAN.md Decisions).
 */
#include <SDL3/SDL.h>
#include <string.h>

#include "platform/input.h"
#include "platform/standins.h"

enum {
    kWmCreate = 0x1,
    kWmDestroy = 0x2,
    kWmActivate = 0x6,
    kWmClose = 0x10,
    kWmQuit = 0x12,
    kWmGetMinMaxInfo = 0x24,
    kWmNcDestroy = 0x82,
    kWmKeyDown = 0x100,
    kWmKeyUp = 0x101,
    kWmSysKeyDown = 0x104,
    kWmSysKeyUp = 0x105,
    kWaInactive = 0,
    kWaActive = 1,
    kPmRemove = 1,
    kGwlHinstance = -6,
    kQueueSize = 512,
    kDikF10 = 0x44,
    kDikLAlt = 0x38,
    kDikRAlt = 0xb8,
};

/* MessageBoxA's type bits and answers. */
enum {
    kMbTypeMask = 0xf,
    kMbIconMask = 0xf0,
    kMbDefMask = 0xf00,
    kMbOk = 0,
    kMbOkCancel = 1,
    kMbAbortRetryIgnore = 2,
    kMbYesNoCancel = 3,
    kMbYesNo = 4,
    kMbRetryCancel = 5,
    kMbCancelTryContinue = 6,
    kMbIconHand = 0x10,
    kMbIconExclamation = 0x30,
    kIdOk = 1,
    kIdCancel = 2,
    kIdAbort = 3,
    kIdRetry = 4,
    kIdIgnore = 5,
    kIdYes = 6,
    kIdNo = 7,
    kIdTryAgain = 10,
    kIdContinue = 11,
};

/* The main menu's quit prompt (zrwConfirmQuit 0x48a9b0): MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 |
 * MB_SETFOREGROUND. */
static const char kQuitPrompt[] = "Do you want to quit the game?";
static const uint32_t kQuitPromptType = 0x10124;

typedef struct Queued {
    BmxMsg msg;
    int input; /* keyboard: after the posted messages */
} Queued;

static SDL_Window *gWindow;
static BmxWndProc gProc;
static int gDestroyed, gActive, gBackground;
static void *gInstance;
static int gCursorCount; /* ShowCursor's display counter: shown while >= 0 */
static uint64_t gLastPump;
static uint8_t gScriptKeys[256]; /* the script's DIK state at the last pump (BMX_KEYS) */
static BmxMessageBoxHook gBoxHook;

static SDL_SpinLock gQueueLock;
static Queued gQueue[kQueueSize];
static int gHead, gCount, gOverflowLogged;
static int gQuitPosted, gQuitCode;

static intptr_t send(uint32_t message, uintptr_t wParam, uintptr_t lParam)
{
    if (gProc == NULL || gDestroyed) {
        return 0;
    }
    return gProc(gWindow, message, wParam, lParam);
}

static int post(void *window, uint32_t message, uintptr_t wParam, uintptr_t lParam, int input)
{
    int ok = 0;

    if (window != NULL && window == (void *)gWindow && gDestroyed) {
        return 0; /* a destroyed window takes no messages */
    }
    SDL_LockSpinlock(&gQueueLock);
    if (gCount < kQueueSize) {
        Queued *const q = &gQueue[(gHead + gCount) % kQueueSize];

        q->msg.window = window;
        q->msg.message = message;
        q->msg.wParam = wParam;
        q->msg.lParam = lParam;
        q->msg.time = bmxGetTickCount();
        q->msg.x = 0;
        q->msg.y = 0;
        q->input = input;
        gCount++;
        ok = 1;
    }
    SDL_UnlockSpinlock(&gQueueLock);
    if (!ok && !gOverflowLogged) {
        gOverflowLogged = 1;
        bmxLog("window: message queue full, message 0x%x dropped\n", (unsigned)message);
    }
    return ok;
}

int bmxWindowPost(void *window, uint32_t message, uintptr_t wParam, uintptr_t lParam)
{
    return post(window, message, wParam, lParam, 0);
}

/* A key message as Windows builds it: repeat count 1, scan code, extended bit, previous state, transition. */
static void postKey(uint8_t dik, int down, int repeat, int system)
{
    uint32_t lParam = 1u | (uint32_t)(dik & 0x7f) << 16;
    uint32_t message;

    if ((dik & 0x80) != 0) {
        lParam |= 1u << 24;
    }
    if (down) {
        lParam |= repeat ? 1u << 30 : 0;
        message = system ? kWmSysKeyDown : kWmKeyDown;
    } else {
        lParam |= 3u << 30;
        message = system ? kWmSysKeyUp : kWmKeyUp;
    }
    post(gWindow, message, 0, lParam, 1);
}

static void setActive(int active)
{
    if (active == gActive) {
        return;
    }
    gActive = active;
    bmxLog("window: %s (WM_ACTIVATE %s)\n", active ? "focus gained" : "focus lost",
           active ? "WA_ACTIVE" : "WA_INACTIVE");
    /* Before the procedure runs: the keys already held now read as down, as in DirectInput. */
    if (active) {
        bmxInputFocusGained();
    } else {
        bmxInputFocusLost();
    }
    send(kWmActivate, active ? kWaActive : kWaInactive, 0);
}

/* DestroyWindow's messages; the SDL window is only hidden (the GPU device still holds it). */
static void destroyWindow(void)
{
    if (gDestroyed || gWindow == NULL) {
        return;
    }
    setActive(0);
    send(kWmDestroy, 0, 0);
    send(kWmNcDestroy, 0, 0);
    gDestroyed = 1;
    SDL_HideWindow(gWindow);
    bmxLog("window: destroyed (WM_CLOSE -> WM_DESTROY)%s\n", gQuitPosted ? ", WM_QUIT posted" : "");
}

/* The script's key edges (BMX_KEYS) as key messages: the script is the keyboard of a scripted run. */
static void scriptKeys(void)
{
    uint8_t now[256];

    memset(now, 0, sizeof now);
    bmxKeysFillState(now, sizeof now);
    for (int k = 1; k < 256; k++) {
        if ((now[k] & 0x80) != (gScriptKeys[k] & 0x80)) {
            const int alt = (now[kDikLAlt] | now[kDikRAlt]) & 0x80;

            postKey((uint8_t)k, now[k] != 0, 0, alt != 0 || k == kDikF10);
        }
    }
    memcpy(gScriptKeys, now, sizeof now);
}

/* SDL's events for our window into the game's messages (sent ones are dispatched here, inside PeekMessage,
 * as Windows dispatches sent messages). At most once per millisecond: the main loop polls in a busy wait. */
static void pump(int force)
{
    const uint64_t now = SDL_GetTicksNS();
    const int scripted = bmxKeysScripted();
    SDL_Event e;

    if (gWindow == NULL || gDestroyed || (!force && now - gLastPump < SDL_NS_PER_MS)) {
        return;
    }
    gLastPump = now;
    SDL_PumpEvents();
    while (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) == 1) {
        switch (e.type) {
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            setActive(1);
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            setActive(0);
            break;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            send(kWmClose, 0, 0);
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if (!scripted) {
                const uint8_t dik = bmxDikFromScancode((int)e.key.scancode);

                if (dik != 0) {
                    postKey(dik, e.key.down, e.key.repeat, (e.key.mod & SDL_KMOD_ALT) != 0 || dik == kDikF10);
                }
            }
            break;
        default:
            break;
        }
        if (gDestroyed) {
            break;
        }
    }
    if (scripted && !gDestroyed) {
        scriptKeys();
    }
}

/* ---- the window ---- */

/* WM_GETMINMAXINFO: the procedure locks the size to the mode (AdjustWindowRect below is the identity, so the
 * sizes are client sizes, as SDL's are). */
static void applyMinMax(void)
{
    int32_t info[10];

    memset(info, 0, sizeof info);
    send(kWmGetMinMaxInfo, 0, (uintptr_t)info);
    if (info[8] > 0 && info[9] > 0) {
        SDL_SetWindowMaximumSize(gWindow, info[8], info[9]);
    }
    if (info[6] > 0 && info[7] > 0) {
        SDL_SetWindowMinimumSize(gWindow, info[6], info[7]);
    }
}

void bmxVideoSize(int *width, int *height, int fallbackW, int fallbackH)
{
    /* The desktop size by default (0), the caller's (the original's 640 x 480) with vanilla = 1. */
    *width = bmxConfigOption("video.width", 0, fallbackW);
    *height = bmxConfigOption("video.height", 0, fallbackH);
    if (*width == 0 || *height == 0) {
        const SDL_DisplayMode *desktop = NULL;

        if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
            desktop = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
        }
        if (desktop == NULL) {
            bmxLog("window: no desktop size (%s), using %dx%d\n", SDL_GetError(), fallbackW, fallbackH);
            *width = *width == 0 ? fallbackW : *width;
            *height = *height == 0 ? fallbackH : *height;
        } else {
            *width = *width == 0 ? desktop->w : *width;
            *height = *height == 0 ? desktop->h : *height;
        }
        /* The subsystem stays up: the window needs it next. */
    }
}

void *bmxWindowCreate(const char *title, int width, int height, BmxWndProc proc, int background)
{
    SDL_WindowFlags flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;

    gBackground = background;
    if (background) {
        /* A scripted run must not take the focus from what the user is doing. */
        SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, "0");
        SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED, "0");
        flags |= SDL_WINDOW_NOT_FOCUSABLE;
    }
    /* The game decides when to quit (WM_CLOSE -> WM_DESTROY -> PostQuitMessage). */
    SDL_SetHint(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0");
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        bmxLog("window: SDL_InitSubSystem(VIDEO) failed: %s\n", SDL_GetError());
        return NULL;
    }
    gWindow = SDL_CreateWindow(title, width, height, flags);
    if (gWindow == NULL) {
        bmxLog("window: SDL_CreateWindow failed: %s\n", SDL_GetError());
        return NULL;
    }
    gProc = proc;
    gDestroyed = 0;
    gActive = 0;
    applyMinMax();
    send(kWmCreate, 0, 0);
    bmxLog("window: %dx%d \"%s\"%s\n", width, height, title,
           background ? " (background: never focused)" : "");
    return gWindow;
}

void bmxWindowSetFullscreen(void *window)
{
    if (window != (void *)gWindow || gWindow == NULL) {
        return;
    }
    /* The size lock is for a framed window only. */
    SDL_SetWindowMaximumSize(gWindow, 0, 0);
    SDL_SetWindowMinimumSize(gWindow, 0, 0);
    if (gBackground) {
        /* A scripted run keeps its window where it is and its size (borderless, as WS_POPUP alone gave), so
         * it never covers the user's screen. */
        SDL_SetWindowBordered(gWindow, false);
        SDL_SetWindowResizable(gWindow, false);
        bmxLog("window: full screen mode, background run: a borderless window instead\n");
        return;
    }
    /* D4: the mode is rendered at its size and scaled into the screen; the display mode does not change. */
    if (!SDL_SetWindowFullscreenMode(gWindow, NULL) || !SDL_SetWindowFullscreen(gWindow, true)) {
        bmxLog("window: full screen failed: %s\n", SDL_GetError());
    }
}

typedef int(BMX_WINAPI *SetWindowPosFn)(void *hwnd, void *after, int x, int y, int cx, int cy,
                                        uint32_t flags);

void bmxWindowShow(void *window)
{
    if (window != (void *)gWindow || gWindow == NULL) {
        return;
    }
    SDL_ShowWindow(gWindow);
    if (gBackground) {
        /* Dev aid (BMX_KEYS): behind the other windows, without activation. Windows only, through the real
         * SetWindowPos (HWND_BOTTOM, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE); no import of USER32. */
        const SetWindowPosFn setPos = (SetWindowPosFn)bmxRealProc("USER32.dll", "SetWindowPos");
        void *const hwnd = bmxWindowNative(gWindow);

        if (setPos != NULL && hwnd != NULL) {
            setPos(hwnd, (void *)(uintptr_t)1, 0, 0, 0, 0, 0x13);
        }
    }
    /* ShowWindow activates the window and sends WM_ACTIVATE before it returns: deliver it now. */
    pump(1);
}

void bmxWindowDestroy(void *window)
{
    if (window == (void *)gWindow) {
        destroyWindow();
    }
}

void bmxWindowSetInstance(void *instance)
{
    gInstance = instance;
}

void *bmxWindowNative(void *window)
{
    if (window == NULL || window != (void *)gWindow) {
        return window; /* not ours (tests pass a real HWND) */
    }
    return SDL_GetPointerProperty(SDL_GetWindowProperties(gWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
}

struct SDL_Window *bmxWindowSdl(void *window)
{
    return window == (void *)gWindow ? gWindow : NULL;
}

void bmxWindowSetMessageBoxHook(BmxMessageBoxHook hook)
{
    gBoxHook = hook;
}

/* ---- the USER32 stand-ins ---- */

int32_t BMX_WINAPI bmxStandPeekMessageA(void *out, void *window, uint32_t min, uint32_t max, uint32_t remove)
{
    BmxMsg *const msg = out;
    int found = -1;

    pump(0);
    SDL_LockSpinlock(&gQueueLock);
    for (int pass = 0; pass < 2 && found < 0; pass++) {
        for (int i = 0; i < gCount; i++) {
            const Queued *const q = &gQueue[(gHead + i) % kQueueSize];

            if (q->input == pass && (window == NULL || q->msg.window == window) &&
                ((min == 0 && max == 0) || (q->msg.message >= min && q->msg.message <= max))) {
                found = i;
                break;
            }
        }
    }
    if (found >= 0) {
        *msg = gQueue[(gHead + found) % kQueueSize].msg;
        if ((remove & kPmRemove) != 0) {
            for (int i = found; i > 0; i--) {
                gQueue[(gHead + i) % kQueueSize] = gQueue[(gHead + i - 1) % kQueueSize];
            }
            gHead = (gHead + 1) % kQueueSize;
            gCount--;
        }
    }
    SDL_UnlockSpinlock(&gQueueLock);
    if (found >= 0) {
        return 1;
    }
    /* "PeekMessage always retrieves WM_QUIT messages, no matter which values you specify for wMsgFilterMin
     * and wMsgFilterMax" (Win32 documentation). */
    if (gQuitPosted) {
        memset(msg, 0, sizeof *msg);
        msg->message = kWmQuit;
        msg->wParam = (uintptr_t)gQuitCode;
        msg->time = bmxGetTickCount();
        if ((remove & kPmRemove) != 0) {
            gQuitPosted = 0;
        }
        return 1;
    }
    return 0;
}

int32_t BMX_WINAPI bmxStandTranslateMessage(const void *msg)
{
    const uint32_t message = ((const BmxMsg *)msg)->message;

    /* WM_CHAR is not generated: the procedure leaves it to DefWindowProc. */
    return message == kWmKeyDown || message == kWmKeyUp || message == kWmSysKeyDown || message == kWmSysKeyUp;
}

intptr_t BMX_WINAPI bmxStandDispatchMessageA(const void *msg)
{
    const BmxMsg *const m = msg;

    if (m->window == NULL || m->window != (void *)gWindow) {
        return 0;
    }
    return send(m->message, m->wParam, m->lParam);
}

intptr_t BMX_WINAPI bmxStandDefWindowProcA(void *window, uint32_t message, uintptr_t wParam, uintptr_t lParam)
{
    (void)wParam;
    (void)lParam;
    if (message == kWmClose && window == (void *)gWindow) {
        destroyWindow();
    }
    return 0;
}

int32_t BMX_WINAPI bmxStandAdjustWindowRect(const int32_t *rect, uint32_t style, int32_t menu)
{
    /* SDL's window sizes are client sizes: the frame adds nothing. */
    (void)rect;
    (void)style;
    (void)menu;
    return 1;
}

int32_t BMX_WINAPI bmxStandClipCursor(const void *rect)
{
    if (rect != NULL) {
        bmxLog("window: ClipCursor with a rectangle ignored\n");
    }
    return 1;
}

void BMX_WINAPI bmxStandPostQuitMessage(int32_t code)
{
    gQuitPosted = 1;
    gQuitCode = code;
}

int32_t BMX_WINAPI bmxStandShowCursor(int32_t show)
{
    gCursorCount += show ? 1 : -1;
    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        if (gCursorCount >= 0) {
            SDL_ShowCursor();
        } else {
            SDL_HideCursor();
        }
    }
    return gCursorCount;
}

int32_t BMX_WINAPI bmxStandSetWindowTextA(void *window, const char *text)
{
    if (window != (void *)gWindow || gWindow == NULL) {
        return 0;
    }
    return SDL_SetWindowTitle(gWindow, text != NULL ? text : "") ? 1 : 0;
}

intptr_t BMX_WINAPI bmxStandGetWindowLongA(void *window, int32_t index)
{
    static int logged;

    (void)window;
    if (index == kGwlHinstance) {
        return (intptr_t)gInstance;
    }
    if (!logged) {
        logged = 1;
        bmxLog("window: GetWindowLongA(%d) not supported, 0\n", (int)index);
    }
    return 0;
}

/* ---- message boxes ---- */

static void addButton(SDL_MessageBoxButtonData *buttons, int *n, int id, const char *text)
{
    buttons[*n].flags = 0;
    buttons[*n].buttonID = id;
    buttons[*n].text = text;
    (*n)++;
}

/* The buttons of a MessageBoxA type in Windows' order; returns the id Esc / the close box gives (0: none). */
static int boxButtons(uint32_t type, SDL_MessageBoxButtonData *buttons, int *n)
{
    *n = 0;
    switch (type & kMbTypeMask) {
    case kMbOkCancel:
        addButton(buttons, n, kIdOk, "OK");
        addButton(buttons, n, kIdCancel, "Cancel");
        return kIdCancel;
    case kMbAbortRetryIgnore:
        addButton(buttons, n, kIdAbort, "Abort");
        addButton(buttons, n, kIdRetry, "Retry");
        addButton(buttons, n, kIdIgnore, "Ignore");
        return 0;
    case kMbYesNoCancel:
        addButton(buttons, n, kIdYes, "Yes");
        addButton(buttons, n, kIdNo, "No");
        addButton(buttons, n, kIdCancel, "Cancel");
        return kIdCancel;
    case kMbYesNo:
        addButton(buttons, n, kIdYes, "Yes");
        addButton(buttons, n, kIdNo, "No");
        return 0;
    case kMbRetryCancel:
        addButton(buttons, n, kIdRetry, "Retry");
        addButton(buttons, n, kIdCancel, "Cancel");
        return kIdCancel;
    case kMbCancelTryContinue:
        addButton(buttons, n, kIdCancel, "Cancel");
        addButton(buttons, n, kIdTryAgain, "Try Again");
        addButton(buttons, n, kIdContinue, "Continue");
        return kIdCancel;
    default:
        addButton(buttons, n, kIdOk, "OK");
        return kIdOk;
    }
}

/* What a scripted run answers (no window): Yes for Yes/No (the quit prompt quits), Cancel for Yes/No/Cancel
 * (an assert terminates), else OK. */
static int scriptedAnswer(uint32_t type)
{
    const uint32_t buttons = type & kMbTypeMask;

    return buttons == kMbYesNo ? kIdYes : buttons == kMbYesNoCancel ? kIdCancel : kIdOk;
}

static int32_t showBox(void *owner, const char *text, const char *caption, uint32_t type)
{
    SDL_MessageBoxButtonData buttons[3];
    SDL_MessageBoxData data;
    int count, answer = -1;
    const int escape = boxButtons(type, buttons, &count);
    const int def = (int)((type & kMbDefMask) >> 8);
    const uint32_t icon = type & kMbIconMask;

    buttons[def < count ? def : 0].flags |= SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    for (int i = 0; i < count; i++) {
        if (buttons[i].buttonID == escape) {
            buttons[i].flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
        }
    }
    SDL_zero(data);
    data.flags = (icon == kMbIconHand          ? SDL_MESSAGEBOX_ERROR
                  : icon == kMbIconExclamation ? SDL_MESSAGEBOX_WARNING
                                               : SDL_MESSAGEBOX_INFORMATION) |
                 SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT;
    data.window = owner != NULL && owner == (void *)gWindow && !gDestroyed ? gWindow : NULL;
    data.title = caption != NULL ? caption : "Error";
    data.message = text != NULL ? text : "";
    data.numbuttons = count;
    data.buttons = buttons;
    if (gBoxHook != NULL) {
        answer = gBoxHook(&data);
    } else if (!SDL_ShowMessageBox(&data, &answer)) {
        bmxLog("window: SDL_ShowMessageBox failed: %s\n", SDL_GetError());
        return 0;
    }
    if (answer < 0) {
        answer = escape != 0 ? escape : buttons[def < count ? def : 0].buttonID; /* closed */
    }
    return answer;
}

int32_t BMX_WINAPI bmxStandMessageBoxA(void *owner, const char *text, const char *caption, uint32_t type)
{
    int32_t answer;

    if (text != NULL && strcmp(text, kQuitPrompt) == 0 && type == kQuitPromptType &&
        bmxConfigOption("ui.confirm_quit", 0, 1) == 0) {
        bmxLog("window: quit prompt answered Yes without a box ([ui] confirm_quit = 0)\n");
        return kIdYes;
    }
    if (bmxKeysScripted()) {
        answer = scriptedAnswer(type);
        bmxLog("BMX_KEYS: message box \"%s\" answered %d\n", text != NULL ? text : "", (int)answer);
        return answer;
    }
    answer = showBox(owner, text, caption, type);
    bmxLog("window: message box \"%s\" (type 0x%x): %d\n", text != NULL ? text : "", (unsigned)type,
           (int)answer);
    return answer;
}
