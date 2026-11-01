/*
 * The platform layer (docs/PLATFORM.md): stand-ins for the Windows functions the game imports, built on SDL3
 * and our own code. The loader (src/native/loader.c) fills the exe's import slots from the table in
 * imports.c. Everything here has the Win32 signature of the function it replaces; with no option set the game
 * sees the same values and behaviour as from Windows.
 */
#ifndef BMX_PLATFORM_PLATFORM_H
#define BMX_PLATFORM_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#if defined(__i386__)
#define BMX_WINAPI __attribute__((stdcall))
#else
#define BMX_WINAPI
#endif

/* ---- the import table (imports.c) ---- */

/* One stand-in: replaces dll!name (the DLL name compared without regard to case). */
typedef struct BmxStandIn {
    const char *dll;
    const char *name;
    void *fn;
} BmxStandIn;

extern const BmxStandIn bmxStandIns[];
extern const size_t bmxStandInCount;

/* The stand-in for dll!name or NULL. */
void *bmxStandInFind(const char *dll, const char *name);
/* 1 when the native exe never reaches dll!name (docs/PLATFORM.md 1b): the loader gives it a trap stub. */
int bmxImportIsDead(const char *dll, const char *name);
/* 1 when a stand-in of this DLL forwards to the real DLL when its option is off (loaded even if every slot of
 * it is a stand-in). */
int bmxImportKeepsDll(const char *dll);

/* ---- provided by the loader ---- */

/* The real dll!name (loads the DLL when needed), for stand-ins that forward or check against Windows. */
void *bmxRealProc(const char *dll, const char *name);
/* Log to stderr and bmxnative.log (src/native/trap.c). */
void bmxLog(const char *fmt, ...);

/* BMX_KEYS (src/native/keys.c): 1 when the keyboard is scripted; the script's DIK state at the current shown
 * frame (state is zeroed by the caller). */
int bmxKeysScripted(void);
void bmxKeysFillState(uint8_t *state, uint32_t size);

/* ---- time and misc kernel32 (kernel.c) ---- */

/* Sets the clock origin; idempotent. Call before the game runs. BMX_TIME_BASE overrides the base. */
void bmxPlatformInit(void);
/* The milliseconds the game's timeGetTime and GetTickCount see (32-bit, wrapping). */
uint32_t bmxTimeGetTime(void);
uint32_t bmxGetTickCount(void);
/* The clock behind bmxTimeGetTime in ns since the origin, and the millisecond value it shows at such an
 * instant (bmxTimeGetTime() == bmxTimeMsAt(bmxTimeNs()); the high frame rate scheduler, hfinterp.c). */
uint64_t bmxTimeNs(void);
uint32_t bmxTimeMsAt(uint64_t ns);
/* Test hook: the value both clocks show at the origin. */
void bmxTimeSetBase(uint32_t base);

/* ---- winmm timer thread (winmm.c) ---- */

/* Stops every timer (joins the threads); logs the interval statistics when BMX_TIMER_LOG is set. */
void bmxTimerShutdown(void);

/* At exit (kernel.c): stops the timers and logs what the dev checks collected (BMX_INPUT_CHECK). */
void bmxPlatformShutdown(void);

/* ---- the config file (config.c, P19) ---- */

/*
 * bmxnative.ini next to the exe (BMX_CONFIG=<path> names another), read on first use: `key = value` lines,
 * `[section]` headers prefix the keys ("input.dpad_steering"), `#` and `;` start comments; unknown keys are
 * logged. bmxConfigInt: the key's value, or `fallback` when unset.
 */
int bmxConfigInt(const char *key, int fallback);
/*
 * An option with an enhanced default (PLAN.md Decisions, 2026-11-01): the key's value when set, else
 * `vanilla` when `[general] vanilla = 1`, else `enhanced`. One switch gives the original game back; any
 * option set in the file still wins.
 */
int bmxConfigOption(const char *key, int enhanced, int vanilla);
/* A text value (paths), NULL when unset or empty. */
const char *bmxConfigString(const char *key);
/* Replaces the configuration by the text (tests; `origin` names it in the log). */
void bmxConfigLoadText(const char *text, const char *origin);

/* ---- movies (avi.c) ---- */

/* The frame number and stream name ("INTRO.AVI Video #1") of a DIB AVIStreamGetFrame handed out, or -1. */
int32_t bmxAviFrameOf(const void *dib, const char **streamName);

/* ---- the window and its message queue (window.c, P15) ---- */

/* The game's window procedure (zrwMainWndProc 0x48a800): WNDPROC. */
typedef intptr_t(BMX_WINAPI *BmxWndProc)(void *window, uint32_t message, uintptr_t wParam, uintptr_t lParam);
struct SDL_Window;
struct SDL_MessageBoxData;

/* MSG (32-bit): what PeekMessageA fills. */
typedef struct BmxMsg {
    void *window;
    uint32_t message;
    uintptr_t wParam;
    uintptr_t lParam;
    uint32_t time;
    int32_t x, y;
} BmxMsg;

#if defined(__i386__)
_Static_assert(sizeof(BmxMsg) == 0x1c, "MSG");
#endif

/*
 * Creates the game's window (hidden, `width` x `height` client size) and sends the procedure WM_GETMINMAXINFO
 * and WM_CREATE; returns the handle the game keeps (an opaque token), NULL on failure. `background`: a
 * scripted run's window, never focused or activated.
 */
void *bmxWindowCreate(const char *title, int width, int height, BmxWndProc proc, int background);
/*
 * `[video] width` x `height` (docs/WIDESCREEN.md W1): `fallbackW` x `fallbackH` when unset; 0 in either
 * means the primary display's desktop size (in pixels) for that axis.
 */
void bmxVideoSize(int *width, int *height, int fallbackW, int fallbackH);
/* The full screen style (D4: borderless over the desktop, the display mode unchanged). */
void bmxWindowSetFullscreen(void *window);
/* ShowWindow: shows it and delivers the activation (WM_ACTIVATE) at once, as Windows does. */
void bmxWindowShow(void *window);
/* DestroyWindow: WM_ACTIVATE (inactive), WM_DESTROY, WM_NCDESTROY; the window is hidden. */
void bmxWindowDestroy(void *window);
/* What GetWindowLongA(GWL_HINSTANCE) returns. */
void bmxWindowSetInstance(void *instance);
/* Posts a message (any thread), as PostMessageA. */
int bmxWindowPost(void *window, uint32_t message, uintptr_t wParam, uintptr_t lParam);
/* The HWND behind the token, for the dev checks that hand it to real DLLs (other values unchanged). */
void *bmxWindowNative(void *window);
/* The SDL window behind the token (the driver's swapchain), NULL for anything else. */
struct SDL_Window *bmxWindowSdl(void *window);
/* Test hook: shows message boxes instead of SDL_ShowMessageBox; returns the button id (-1: closed). */
typedef int (*BmxMessageBoxHook)(const struct SDL_MessageBoxData *box);
void bmxWindowSetMessageBoxHook(BmxMessageBoxHook hook);

/* ---- paths (paths.c) ---- */

/* The host path for a path as the game spells it (relative to the install folder, backslashes). Identity
 * today; the seam for an asset folder, profile saves and case-insensitive lookup on other platforms (P18). */
const char *bmxPathResolve(const char *gamePath);

#endif
