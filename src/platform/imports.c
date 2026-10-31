/*
 * The import table of the native exe (docs/PLATFORM.md P2): which Windows function each slot of the
 * original's import directory gets. The loader (src/native/loader.c) asks bmxStandInFind first, then
 * bmxImportIsDead, and only then loads the real DLL.
 */
#include <SDL3/SDL.h>
#include <string.h>

#include "platform/standins.h"

#define STAND(dll, name, fn) {dll, name, (void *)(fn)}

const BmxStandIn bmxStandIns[] = {
    /* S2: time and small kernel32 functions */
    STAND("WINMM.dll", "timeGetTime", bmxStandTimeGetTime),
    STAND("KERNEL32.dll", "GetTickCount", bmxStandGetTickCount),
    STAND("KERNEL32.dll", "QueryPerformanceFrequency", bmxStandQueryPerformanceFrequency),
    STAND("KERNEL32.dll", "QueryPerformanceCounter", bmxStandQueryPerformanceCounter),
    STAND("KERNEL32.dll", "Sleep", bmxStandSleep),
    STAND("KERNEL32.dll", "OutputDebugStringA", bmxStandOutputDebugStringA),
    STAND("KERNEL32.dll", "DebugBreak", bmxStandDebugBreak),
    STAND("KERNEL32.dll", "InitializeCriticalSection", bmxStandInitializeCriticalSection),
    STAND("KERNEL32.dll", "EnterCriticalSection", bmxStandEnterCriticalSection),
    STAND("KERNEL32.dll", "LeaveCriticalSection", bmxStandLeaveCriticalSection),
    STAND("KERNEL32.dll", "DeleteCriticalSection", bmxStandDeleteCriticalSection),
    /* S3: the winmm timer */
    STAND("WINMM.dll", "timeGetDevCaps", bmxStandTimeGetDevCaps),
    STAND("WINMM.dll", "timeBeginPeriod", bmxStandTimeBeginPeriod),
    STAND("WINMM.dll", "timeEndPeriod", bmxStandTimeEndPeriod),
    STAND("WINMM.dll", "timeSetEvent", bmxStandTimeSetEvent),
    STAND("WINMM.dll", "timeKillEvent", bmxStandTimeKillEvent),
    /* S4: the sound loader's files */
    STAND("KERNEL32.dll", "CreateFileA", bmxStandCreateFileA),
    STAND("KERNEL32.dll", "ReadFile", bmxStandReadFile),
    STAND("KERNEL32.dll", "SetFilePointer", bmxStandSetFilePointer),
    STAND("KERNEL32.dll", "CloseHandle", bmxStandCloseHandle),
    /* S5: DirectInput (keyboard and pad on SDL3; BMX_KEYS is a source of its keyboard) */
    STAND("DINPUT.dll", "DirectInputCreateEx", bmxStandDirectInputCreateEx),
    /* S7: DirectSound (LoadLibraryA("DSOUND.DLL") / GetProcAddress("DirectSoundCreate") give our own) */
    STAND("KERNEL32.dll", "LoadLibraryA", bmxStandLoadLibraryA),
    STAND("KERNEL32.dll", "GetProcAddress", bmxStandGetProcAddress),
    STAND("KERNEL32.dll", "FreeLibrary", bmxStandFreeLibrary),
    /* S8: our AVI reader and Cinepak decoder (AVISaveOptionsFree: moved here from the loader) */
    STAND("AVIFIL32.dll", "AVIFileInit", bmxStandAVIFileInit),
    STAND("AVIFIL32.dll", "AVIFileExit", bmxStandAVIFileExit),
    STAND("AVIFIL32.dll", "AVIFileOpenA", bmxStandAVIFileOpenA),
    STAND("AVIFIL32.dll", "AVIFileRelease", bmxStandAVIFileRelease),
    STAND("AVIFIL32.dll", "AVIFileGetStream", bmxStandAVIFileGetStream),
    STAND("AVIFIL32.dll", "AVIStreamAddRef", bmxStandAVIStreamAddRef),
    STAND("AVIFIL32.dll", "AVIStreamRelease", bmxStandAVIStreamRelease),
    STAND("AVIFIL32.dll", "AVIStreamInfoA", bmxStandAVIStreamInfoA),
    STAND("AVIFIL32.dll", "AVIStreamStart", bmxStandAVIStreamStart),
    STAND("AVIFIL32.dll", "AVIStreamLength", bmxStandAVIStreamLength),
    STAND("AVIFIL32.dll", "AVIStreamReadFormat", bmxStandAVIStreamReadFormat),
    STAND("AVIFIL32.dll", "AVIStreamTimeToSample", bmxStandAVIStreamTimeToSample),
    STAND("AVIFIL32.dll", "AVIStreamSampleToTime", bmxStandAVIStreamSampleToTime),
    STAND("AVIFIL32.dll", "AVIStreamRead", bmxStandAVIStreamRead),
    STAND("AVIFIL32.dll", "AVIStreamGetFrameOpen", bmxStandAVIStreamGetFrameOpen),
    STAND("AVIFIL32.dll", "AVIStreamGetFrame", bmxStandAVIStreamGetFrame),
    STAND("AVIFIL32.dll", "AVIStreamGetFrameClose", bmxStandAVIStreamGetFrameClose),
    STAND("AVIFIL32.dll", "AVISaveOptionsFree", bmxStandAVISaveOptionsFree),
    /* S9: the movies' output: DrawDib and the window DC on the SDL3 driver, waveOut on SDL3 audio */
    STAND("MSVFW32.dll", "DrawDibOpen", bmxStandDrawDibOpen),
    STAND("MSVFW32.dll", "DrawDibDraw", bmxStandDrawDibDraw),
    STAND("MSVFW32.dll", "DrawDibClose", bmxStandDrawDibClose),
    STAND("USER32.dll", "GetDC", bmxStandGetDC),
    STAND("USER32.dll", "ReleaseDC", bmxStandReleaseDC),
    STAND("USER32.dll", "GetClientRect", bmxStandGetClientRect),
    STAND("WINMM.dll", "waveOutOpen", bmxStandWaveOutOpen),
    STAND("WINMM.dll", "waveOutPrepareHeader", bmxStandWaveOutPrepareHeader),
    STAND("WINMM.dll", "waveOutUnprepareHeader", bmxStandWaveOutUnprepareHeader),
    STAND("WINMM.dll", "waveOutWrite", bmxStandWaveOutWrite),
    STAND("WINMM.dll", "waveOutPause", bmxStandWaveOutPause),
    STAND("WINMM.dll", "waveOutRestart", bmxStandWaveOutRestart),
    STAND("WINMM.dll", "waveOutReset", bmxStandWaveOutReset),
    STAND("WINMM.dll", "waveOutClose", bmxStandWaveOutClose),
    STAND("WINMM.dll", "sndPlaySoundA", bmxStandSndPlaySoundA),
    /* S10: the window, its message queue and message boxes on SDL3; the device dialog picks the mode */
    STAND("USER32.dll", "PeekMessageA", bmxStandPeekMessageA),
    STAND("USER32.dll", "TranslateMessage", bmxStandTranslateMessage),
    STAND("USER32.dll", "DispatchMessageA", bmxStandDispatchMessageA),
    STAND("USER32.dll", "DefWindowProcA", bmxStandDefWindowProcA),
    STAND("USER32.dll", "AdjustWindowRect", bmxStandAdjustWindowRect),
    STAND("USER32.dll", "ClipCursor", bmxStandClipCursor),
    STAND("USER32.dll", "PostQuitMessage", bmxStandPostQuitMessage),
    STAND("USER32.dll", "ShowCursor", bmxStandShowCursor),
    STAND("USER32.dll", "SetWindowTextA", bmxStandSetWindowTextA),
    STAND("USER32.dll", "GetWindowLongA", bmxStandGetWindowLongA),
    STAND("USER32.dll", "MessageBoxA", bmxStandMessageBoxA),
    STAND("USER32.dll", "DialogBoxParamA", bmxStandDialogBoxParamA),
};

const size_t bmxStandInCount = sizeof bmxStandIns / sizeof bmxStandIns[0];

void *bmxStandInFind(const char *dll, const char *name)
{
    for (size_t i = 0; i < bmxStandInCount; i++) {
        if (SDL_strcasecmp(bmxStandIns[i].dll, dll) == 0 && strcmp(bmxStandIns[i].name, name) == 0) {
            return bmxStandIns[i].fn;
        }
    }
    return NULL;
}

/*
 * Slots only original code that is int3 in the native exe calls (docs/PLATFORM.md 1b): the MSVC CRT (replaced
 * by src/native/crt.c), WinMain 0x48a460 (main.c does its job), the CD check 0x48aa10 (nothing calls it),
 * the D3D7 driver (src/driver) and, since S10, the device dialog's procedure (zrwDialogProc 0x48a330 and its
 * helpers: DialogBoxParamA's stand-in never runs it). Every ADVAPI32 and DDRAW import is dead.
 */
static const char *const kDeadKernel32[] = {
    "CompareStringA",
    "CompareStringW",
    "ExitProcess",
    "FlushFileBuffers",
    "FreeEnvironmentStringsA",
    "FreeEnvironmentStringsW",
    "GetACP",
    "GetCommandLineA",
    "GetCPInfo",
    "GetCurrentProcess",
    "GetEnvironmentStrings",
    "GetEnvironmentStringsW",
    "GetEnvironmentVariableA",
    "GetFileType",
    "GetLastError",
    "GetLocalTime",
    "GetModuleFileNameA",
    "GetModuleHandleA",
    "GetOEMCP",
    "GetStartupInfoA",
    "GetStdHandle",
    "GetStringTypeA",
    "GetStringTypeW",
    "GetSystemTime",
    "GetTimeZoneInformation",
    "GetVersion",
    "GetVersionExA",
    "HeapAlloc",
    "HeapCreate",
    "HeapDestroy",
    "HeapFree",
    "HeapReAlloc",
    "IsBadWritePtr",
    "LCMapStringA",
    "LCMapStringW",
    "MultiByteToWideChar",
    "RaiseException",
    "RtlUnwind",
    "SetConsoleCtrlHandler",
    "SetEndOfFile",
    "SetEnvironmentVariableA",
    "SetHandleCount",
    "SetStdHandle",
    "TerminateProcess",
    "UnhandledExceptionFilter",
    "VirtualAlloc",
    "VirtualFree",
    "WideCharToMultiByte",
    "WriteFile",
};

static const char *const kDeadUser32[] = {
    "ClientToScreen", "CreateWindowExA", "DestroyWindow", "LoadCursorA",  "RegisterClassA",
    "SetWindowLongA", "SetWindowPos",    "ShowWindow",    "UpdateWindow", "EndDialog",
    "GetDlgItem",     "SendMessageA",    "SetFocus",
};

static int inList(const char *const *list, size_t n, const char *name)
{
    for (size_t i = 0; i < n; i++) {
        if (strcmp(list[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

int bmxImportIsDead(const char *dll, const char *name)
{
    if (SDL_strcasecmp(dll, "ADVAPI32.dll") == 0 || SDL_strcasecmp(dll, "DDRAW.dll") == 0) {
        return 1;
    }
    if (SDL_strcasecmp(dll, "KERNEL32.dll") == 0) {
        return inList(kDeadKernel32, sizeof kDeadKernel32 / sizeof kDeadKernel32[0], name);
    }
    if (SDL_strcasecmp(dll, "USER32.dll") == 0) {
        return inList(kDeadUser32, sizeof kDeadUser32 / sizeof kDeadUser32[0], name);
    }
    return 0;
}

int bmxImportKeepsDll(const char *dll)
{
    /* DINPUT.dll only for BMX_INPUT_CHECK, which calls the real one: the game folder's copy may be the hook
     * DLL, which has to patch the range before our jumps are written (docs/NATIVE.md). */
    const char *check = SDL_getenv("BMX_INPUT_CHECK");

    return SDL_strcasecmp(dll, "DINPUT.dll") == 0 && check != NULL && *check != 0 && *check != '0';
}
