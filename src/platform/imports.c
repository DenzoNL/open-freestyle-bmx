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
    /* Moved here from the loader */
    STAND("AVIFIL32.dll", "AVISaveOptionsFree", bmxStandAVISaveOptionsFree),
};

const size_t bmxStandInCount = sizeof bmxStandIns / sizeof bmxStandIns[0];

void *bmxStandInFind(const char *dll, const char *name)
{
    for (size_t i = 0; i < bmxStandInCount; i++) {
        if (SDL_strcasecmp(bmxStandIns[i].dll, dll) == 0 && strcmp(bmxStandIns[i].name, name) == 0) {
            return bmxStandIns[i].fn;
        }
    }
    for (size_t i = 0; i < bmxKeysStandInCount; i++) {
        if (SDL_strcasecmp(bmxKeysStandIns[i].dll, dll) == 0 && strcmp(bmxKeysStandIns[i].name, name) == 0) {
            return bmxKeysStandIns[i]
                .fn; /* BMX_KEYS: acts in a scripted run, forwards to the real one else */
        }
    }
    return NULL;
}

/*
 * Slots only original code that is int3 in the native exe calls (docs/PLATFORM.md 1b): the MSVC CRT (replaced
 * by src/native/crt.c), WinMain 0x48a460 (main.c does its job), the CD check 0x48aa10 (nothing calls it) and
 * the D3D7 driver (src/driver). Every ADVAPI32 and DDRAW import is dead.
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
    "SetWindowLongA", "SetWindowPos",    "ShowWindow",    "UpdateWindow",
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
