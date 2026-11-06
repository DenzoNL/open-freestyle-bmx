/*
 * bmxnative: our own exe. main() does the job of the original WinMain (0x48a460, not ported): command
 * line, window, init calls, message/frame loop. It calls the ports of the functions WinMain called, and keeps
 * WinMain's globals in the original's data (the loader's relocated copy).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <unistd.h> /* _exit */
#endif

#include "compat/fpu.h"
#include "compat/orig.h"
#include "native/native.h"
#include "platform/platform.h"
#include "platform/standins.h"

/* Globals of the original's window code (all in the exe's .data, now the relocated copy). */
#define gHwnd       ORIG_VAR(0x00697780, void *, gHwnd)
#define gHinstance  ORIG_VAR(0x00697784, void *, gHinstance)
#define gFullscreen ORIG_VAR(0x00697788, int, gFullscreen)
#define gAppTitle   ORIG_VAR(0x00697794, const char *, gAppTitle)
#define gWidth      ORIG_VAR(0x00697798, int, gWidth)
#define gHeight     ORIG_VAR(0x0069779c, int, gHeight)
#define gResX       ORIG_VAR(0x006977a0, int, gResX)
#define gResY       ORIG_VAR(0x006977a4, int, gResY)
#define gResZ       ORIG_VAR(0x006977a8, int, gResZ)
#define gQuit       ORIG_VAR(0x006977ac, int, gQuit)
#define gLastTick   ORIG_VAR(0x0067275c, uint32_t, gLastTick)
#define gRateA      ORIG_VAR(0x0069771c, float, gRateA)
#define gRateB      ORIG_VAR(0x0054d05c, float, gRateB)
#define gRateC      ORIG_VAR(0x0054dbb8, float, gRateC)

/* The functions WinMain calls (ported: zrwzgl.c). */
ORIG_FN(0x0048b1a0, int, attachPlugins, (void *window));
ORIG_FN(0x0048b450, int, initApp, (const char *commandLine));
ORIG_FN(0x0048b570, int, onIdle, (void));
ORIG_FN(0x0048b690, int, drawCall, (void));
ORIG_FN_STDCALL(0x0048a800, intptr_t, mainWndProc,
                (void *window, uint32_t message, uintptr_t wParam, uintptr_t lParam));

enum { kWmQuit = 0x12 };

/* The original's "-r X.Y.Z" option; it edits the command line in place, and InitApp gets the edited text. */
static void parseResolution(char *cmd)
{
    for (int i = 0; cmd[i]; i++) {
        char *p;
        int j = 0, n = 0;

        if (cmd[i] != '-' || cmd[i + 1] != 'r') {
            continue;
        }
        p = cmd + i + 2;
        if (!*p) {
            continue;
        }
        do {
            int next;

            if (n == 3) {
                break;
            }
            if (p[j] == '.' || p[j] == ' ') {
                p[j] = 0;
                if (n == 0) {
                    gResX = (int)strtol(p, NULL, 10);
                } else if (n == 1) {
                    gResY = (int)strtol(p, NULL, 10);
                } else if (n == 2) {
                    gResZ = (int)strtol(p, NULL, 10);
                }
                n++;
                p += j + 1;
                j = -1;
            }
            next = j + 1;
            j++;
            if (!p[next]) {
                break;
            }
        } while (1);
        if (n == 2) {
            gResZ = (int)strtol(p, NULL, 10);
        }
    }
}

/*
 * The text the original's WinMain gets as lpCmdLine: the arguments after the program name, joined by single
 * spaces (an argument with a space or a tab, or an empty one, in quotes again), from the CRT's argv.
 */
static void commandLineText(char *out, size_t size, int argc, char **argv)
{
    size_t n = 0;

    out[0] = 0;
    for (int i = 1; i < argc; i++) {
        const int quote = argv[i][0] == 0 || strpbrk(argv[i], " \t") != NULL;
        const int w = snprintf(out + n, size - n, "%s%s%s%s", i > 1 ? " " : "", quote ? "\"" : "", argv[i],
                               quote ? "\"" : "");

        if (w < 0 || (size_t)w >= size - n) {
            return;
        }
        n += (size_t)w;
    }
}

static void initGlobals(void)
{
    /* The original clears 0x00697780..0x006977b4 in one memset; the rest is set below. */
    gHwnd = NULL;
    gHinstance = NULL;
    ORIG_VAR(0x006977b0, int, gZrwKeysLocked) = 0;
    gAppTitle = (const char *)BMX_ORIG(0x0056137c);
    gWidth = 0x280;
    gHeight = 0x1e0;
    gResX = gResY = gResZ = 0;
    gQuit = 0;
    gFullscreen = 0;
    ORIG_VAR(0x0069778c, int, gWindow0) = 0;
    ORIG_VAR(0x00697790, int, gWindow1) = 0;
}

/* The exe's own base address, which is its HINSTANCE (the linker defines the symbol; elsewhere the image's
 * header, __dso_handle, stands in). */
#if defined(_WIN32)
extern char __ImageBase;
#define BMX_IMAGE_BASE (&__ImageBase)
#else
extern char __dso_handle;
#define BMX_IMAGE_BASE ((void *)&__dso_handle)
#endif

int main(int argc, char **argv)
{
    char cmd[1024];
    BmxMsg msg;
    int background;

    bmxFpuWindowsDefaults(); /* x87 at 53 bits as on Windows (Linux starts at 64): compat/fpu.h */
    bmxLogOpen();
    bmxPlatformInit();
    bmxGpuNull(); /* BMX_GPU=null: picks SDL's dummy drivers before any subsystem starts (headless) */
    if (bmxLoaderInit() != 0) {
        return 1;
    }
    bmxTrapInstall();

    /* ---- WinMain ---- */
    initGlobals();
    /* Dev aids: BMX_NO_MOVIES=1 sets the original's own movie skip switch (0x67288c, never set by the game);
     * BMX_MUTE=1 is the audio stand-ins' (dsound.c, waveout.c: they output silence). */
    if (getenv("BMX_NO_MOVIES")) {
        ORIG_VAR(0x0067288c, int, gNoMovies) = 1;
    }
    /* BMX_KEYS: scripted keyboard (keys.c); BMX_REPLAY: a recorded session (replay.c). Such a run must not
     * take the focus from what the user is doing: its window is never focused or activated (window.c). */
    background = bmxKeysInstall();
    background = background || bmxReplayMode() == 2;
    commandLineText(cmd, sizeof cmd, argc, argv);
    parseResolution(cmd);
    /* Without -r the mode comes from the config (P16, the device dialog's stand-in in src/driver/device.c);
     * a windowed one sets the window's size, which the game then renders at (it stays out of full screen). */
    if (!(gResX != 0 && gResY != 0 && gResZ != 0) && bmxConfigInt("video.fullscreen", 1) == 0) {
        int width, height;

        bmxVideoSize(&width, &height, gWidth, gHeight); /* 0: the desktop size */
        gWidth = width;
        gHeight = height;
    }

    /* RegisterClassA + CreateWindowExA (WS_OVERLAPPEDWINDOW, gWidth x gHeight client area): SDL's window. */
    gHwnd = bmxWindowCreate(gAppTitle, gWidth, gHeight, mainWndProc, background);
    if (!gHwnd) {
        return 0;
    }
    gHinstance = BMX_IMAGE_BASE;
    bmxWindowSetInstance(gHinstance);
    if (!attachPlugins(gHwnd)) {
        bmxWindowDestroy(gHwnd);
        return 0;
    }
    if (gFullscreen) {
        bmxWindowSetFullscreen(gHwnd); /* SetWindowLongA(WS_POPUP) + SetWindowPos(SWP_FRAMECHANGED) */
    }
    bmxWindowShow(gHwnd); /* ShowWindow + UpdateWindow (WM_PAINT goes to DefWindowProc) */
    if (initApp(cmd)) {
        const int guard = bmxHfGuardInstall(); /* BMX_HF_GUARD: measures the draws (dev aid) */
        const int hf = bmxHfInstall(guard);    /* [video] interpolate = 1: docs/HIGHFPS.md */

        while (!gQuit) {
            int idle = 0;

            if (bmxStandPeekMessageA(&msg, NULL, 0, 0, 1)) {
                if (msg.message == kWmQuit) {
                    break;
                }
                bmxStandTranslateMessage(&msg);
                bmxStandDispatchMessageA(&msg);
                continue;
            }
            if (hf) {
                if (!bmxHfStep()) {
                    break;
                }
                continue;
            }
            if (gRateA == gRateB) {
                idle = 1;
            } else if (gLastTick == 0) {
                gLastTick = bmxTimeGetTime();
                idle = 1;
            } else {
                const uint32_t elapsed = bmxTimeGetTime() - gLastTick;
                /* The original divides on the x87 at 53-bit precision: double arithmetic on the floats. */
                const double limit = (double)gRateC / (double)gRateA;

                if (!(limit > (double)elapsed)) {
                    gLastTick += elapsed;
                    idle = 1;
                }
            }
            if (idle && guard != 0) {
                if (!bmxHfGuardFrame()) {
                    break;
                }
            } else if (idle) {
                if (!onIdle()) {
                    break;
                }
                drawCall();
            }
        }
    }
    bmxReplayClose();      /* BMX_RECORD: the last records */
    bmxPlatformShutdown(); /* timers; BMX_INPUT_CHECK summary */
    _exit(-1);             /* the original's own _exit(-1) */
    return 0;
}
