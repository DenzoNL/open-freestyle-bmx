/*
 * bmxnative: our own exe. main() does the job of the original WinMain (0x48a460, not ported): command
 * line, window, init calls, message/frame loop. The functions it calls live at their original addresses,
 * where the loader put a jump to our port or an int3 trap.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#include <windows.h>

#include <initguid.h>

#include <audiopolicy.h>
#include <mmdeviceapi.h>

#include "native/native.h"
#include "platform/platform.h"
#include "platform/standins.h"

#define VAR(addr, type) (*(type *)(uintptr_t)(addr))

/* Globals of the original's window code (all in the exe's .data). */
#define gHwnd       VAR(0x00697780, void *)
#define gHinstance  VAR(0x00697784, void *)
#define gFullscreen VAR(0x00697788, int)
#define gAppTitle   VAR(0x00697794, const char *)
#define gWidth      VAR(0x00697798, int)
#define gHeight     VAR(0x0069779c, int)
#define gResX       VAR(0x006977a0, int)
#define gResY       VAR(0x006977a4, int)
#define gResZ       VAR(0x006977a8, int)
#define gQuit       VAR(0x006977ac, int)
#define gLastTick   VAR(0x0067275c, uint32_t)
#define gRateA      VAR(0x0069771c, float)
#define gRateB      VAR(0x0054d05c, float)
#define gRateC      VAR(0x0054dbb8, float)

typedef int(__cdecl *IntFn)(void);
typedef int(__cdecl *IntFnPtr)(void *);
#define attachPlugins ((IntFnPtr)0x0048b1a0)
#define initApp       ((IntFnPtr)0x0048b450)
#define onIdle        ((IntFn)0x0048b570)
#define drawCall      ((IntFn)0x0048b690)
#define mainWndProc   ((BmxWndProc)0x0048a800)

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

static const char *skipProgramName(const char *cl)
{
    if (*cl == '"') {
        cl++;
        while (*cl && *cl != '"') {
            cl++;
        }
        if (*cl) {
            cl++;
        }
    } else {
        while (*cl && *cl != ' ' && *cl != '\t') {
            cl++;
        }
    }
    while (*cl == ' ' || *cl == '\t') {
        cl++;
    }
    return cl;
}

/*
 * Dev aid for automated runs (BMX_MUTE=1): mutes this process's audio session on the default output, so the
 * game's waveOut / DirectSound output (movies, menu music) stays silent. Windows remembers a session's mute
 * per executable, so every other run unmutes it again (else one muted run silences all later ones). COM is
 * left initialised.
 */
static void muteProcess(BOOL mute)
{
    IMMDeviceEnumerator *devices = NULL;
    IMMDevice *device = NULL;
    IAudioSessionManager *sessions = NULL;
    ISimpleAudioVolume *volume = NULL;

    if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) ||
        FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                                (void **)&devices)) ||
        FAILED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(devices, eRender, eConsole, &device)) ||
        FAILED(IMMDevice_Activate(device, &IID_IAudioSessionManager, CLSCTX_ALL, NULL, (void **)&sessions)) ||
        FAILED(IAudioSessionManager_GetSimpleAudioVolume(sessions, NULL, FALSE, &volume)) ||
        FAILED(ISimpleAudioVolume_SetMute(volume, mute, NULL))) {
        bmxLog("audio session: could not set its mute\n");
    } else {
        bmxLog(mute ? "BMX_MUTE: audio session muted\n" : "audio session: unmuted\n");
    }
    if (volume) {
        ISimpleAudioVolume_Release(volume);
    }
    if (sessions) {
        IAudioSessionManager_Release(sessions);
    }
    if (device) {
        IMMDevice_Release(device);
    }
    if (devices) {
        IMMDeviceEnumerator_Release(devices);
    }
}

static void initGlobals(void)
{
    memset((void *)(uintptr_t)0x00697780, 0, 0x34);
    gAppTitle = (const char *)(uintptr_t)0x0056137c;
    gWidth = 0x280;
    gHeight = 0x1e0;
    gResX = gResY = gResZ = 0;
    gQuit = 0;
    gFullscreen = 0;
    VAR(0x0069778c, int) = 0;
    VAR(0x00697790, int) = 0;
}

int main(void)
{
    char cmd[1024];
    BmxMsg msg;
    int background;

    bmxLogOpen();
    bmxPlatformInit();
    if (bmxLoadImage("NgBMX.exe") != 0) {
        return 1;
    }
    bmxTrapInstall();

    /* ---- WinMain ---- */
    initGlobals();
    /* Dev aids: BMX_NO_MOVIES=1 sets the original's own movie skip switch (0x67288c, never set by the game);
     * BMX_MUTE=1 silences the process. */
    if (getenv("BMX_NO_MOVIES")) {
        VAR(0x0067288c, int) = 1;
    }
    muteProcess(getenv("BMX_MUTE") != NULL);
    /* BMX_KEYS: scripted keyboard (keys.c). Such a run must not take the focus from what the user is doing:
     * its window is never focused or activated (window.c). */
    background = bmxKeysInstall();
    snprintf(cmd, sizeof cmd, "%s", skipProgramName(GetCommandLineA()));
    parseResolution(cmd);
    /* Without -r the mode comes from the config (P16, the device dialog's stand-in in src/driver/device.c);
     * a windowed one sets the window's size, which the game then renders at (it stays out of full screen). */
    if (!(gResX != 0 && gResY != 0 && gResZ != 0) && bmxConfigInt("video.fullscreen", 1) == 0) {
        gWidth = bmxConfigInt("video.width", gWidth);
        gHeight = bmxConfigInt("video.height", gHeight);
    }

    /* RegisterClassA + CreateWindowExA (WS_OVERLAPPEDWINDOW, gWidth x gHeight client area): SDL's window. */
    gHwnd = bmxWindowCreate(gAppTitle, gWidth, gHeight, mainWndProc, background);
    if (!gHwnd) {
        return 0;
    }
    gHinstance = GetModuleHandleA(NULL);
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
            if (idle) {
                if (!onIdle()) {
                    break;
                }
                drawCall();
            }
        }
    }
    bmxPlatformShutdown(); /* timers; BMX_INPUT_CHECK summary */
    ExitProcess((UINT)-1); /* the original's _exit(-1) */
    return 0;
}
