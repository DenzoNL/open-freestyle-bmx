/*
 * The original's .idata up to the end of its import slots, 0x00699000..0x0069987c, as one object with the
 * original's layout (src/data/layout.h, docs/PORTING.md X1b): code relies on globals being next to each other
 * (the save data copied as one range, tables used past their declared ends), so the members keep their
 * original offsets; ORIG_VAR(addr) reaches into it (src/compat/orig.h).
 *
 * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.
 */
#include "data/data.h"

struct BmxImports bmxImports __attribute__((aligned(4096)));

/* What the loader fills each slot with (src/native/loader.c). */
const BmxImportSlot bmxImportSlots[] = {
    {"ADVAPI32.dll", "RegQueryValueExA", &bmxImports.ADVAPI32_RegQueryValueExA},
    {"ADVAPI32.dll", "RegCloseKey", &bmxImports.ADVAPI32_RegCloseKey},
    {"ADVAPI32.dll", "RegOpenKeyExA", &bmxImports.ADVAPI32_RegOpenKeyExA},
    {"ADVAPI32.dll", "RegCreateKeyExA", &bmxImports.ADVAPI32_RegCreateKeyExA},
    {"AVIFIL32.dll", "AVIStreamGetFrame", &bmxImports.AVIFIL32_AVIStreamGetFrame},
    {"AVIFIL32.dll", "AVIStreamReadFormat", &bmxImports.AVIFIL32_AVIStreamReadFormat},
    {"AVIFIL32.dll", "AVIFileExit", &bmxImports.AVIFIL32_AVIFileExit},
    {"AVIFIL32.dll", "AVIStreamGetFrameClose", &bmxImports.AVIFIL32_AVIStreamGetFrameClose},
    {"AVIFIL32.dll", "AVISaveOptionsFree", &bmxImports.AVIFIL32_AVISaveOptionsFree},
    {"AVIFIL32.dll", "AVIStreamRelease", &bmxImports.AVIFIL32_AVIStreamRelease},
    {"AVIFIL32.dll", "AVIFileOpenA", &bmxImports.AVIFIL32_AVIFileOpenA},
    {"AVIFIL32.dll", "AVIStreamTimeToSample", &bmxImports.AVIFIL32_AVIStreamTimeToSample},
    {"AVIFIL32.dll", "AVIStreamLength", &bmxImports.AVIFIL32_AVIStreamLength},
    {"AVIFIL32.dll", "AVIStreamStart", &bmxImports.AVIFIL32_AVIStreamStart},
    {"AVIFIL32.dll", "AVIFileRelease", &bmxImports.AVIFIL32_AVIFileRelease},
    {"AVIFIL32.dll", "AVIStreamSampleToTime", &bmxImports.AVIFIL32_AVIStreamSampleToTime},
    {"AVIFIL32.dll", "AVIStreamGetFrameOpen", &bmxImports.AVIFIL32_AVIStreamGetFrameOpen},
    {"AVIFIL32.dll", "AVIStreamInfoA", &bmxImports.AVIFIL32_AVIStreamInfoA},
    {"AVIFIL32.dll", "AVIStreamRead", &bmxImports.AVIFIL32_AVIStreamRead},
    {"AVIFIL32.dll", "AVIFileInit", &bmxImports.AVIFIL32_AVIFileInit},
    {"AVIFIL32.dll", "AVIStreamAddRef", &bmxImports.AVIFIL32_AVIStreamAddRef},
    {"AVIFIL32.dll", "AVIFileGetStream", &bmxImports.AVIFIL32_AVIFileGetStream},
    {"DDRAW.dll", "DirectDrawCreate", &bmxImports.DDRAW_DirectDrawCreate},
    {"DDRAW.dll", "DirectDrawEnumerateA", &bmxImports.DDRAW_DirectDrawEnumerateA},
    {"DINPUT.dll", "DirectInputCreateEx", &bmxImports.DINPUT_DirectInputCreateEx},
    {"KERNEL32.dll", "ReadFile", &bmxImports.KERNEL32_ReadFile},
    {"KERNEL32.dll", "FreeLibrary", &bmxImports.KERNEL32_FreeLibrary},
    {"KERNEL32.dll", "CloseHandle", &bmxImports.KERNEL32_CloseHandle},
    {"KERNEL32.dll", "LoadLibraryA", &bmxImports.KERNEL32_LoadLibraryA},
    {"KERNEL32.dll", "SetFilePointer", &bmxImports.KERNEL32_SetFilePointer},
    {"KERNEL32.dll", "EnterCriticalSection", &bmxImports.KERNEL32_EnterCriticalSection},
    {"KERNEL32.dll", "LeaveCriticalSection", &bmxImports.KERNEL32_LeaveCriticalSection},
    {"KERNEL32.dll", "DeleteCriticalSection", &bmxImports.KERNEL32_DeleteCriticalSection},
    {"KERNEL32.dll", "InitializeCriticalSection", &bmxImports.KERNEL32_InitializeCriticalSection},
    {"KERNEL32.dll", "CreateFileA", &bmxImports.KERNEL32_CreateFileA},
    {"KERNEL32.dll", "FreeEnvironmentStringsW", &bmxImports.KERNEL32_FreeEnvironmentStringsW},
    {"KERNEL32.dll", "LCMapStringA", &bmxImports.KERNEL32_LCMapStringA},
    {"KERNEL32.dll", "GetTimeZoneInformation", &bmxImports.KERNEL32_GetTimeZoneInformation},
    {"KERNEL32.dll", "LCMapStringW", &bmxImports.KERNEL32_LCMapStringW},
    {"KERNEL32.dll", "OutputDebugStringA", &bmxImports.KERNEL32_OutputDebugStringA},
    {"KERNEL32.dll", "DebugBreak", &bmxImports.KERNEL32_DebugBreak},
    {"KERNEL32.dll", "Sleep", &bmxImports.KERNEL32_Sleep},
    {"KERNEL32.dll", "GetTickCount", &bmxImports.KERNEL32_GetTickCount},
    {"KERNEL32.dll", "QueryPerformanceFrequency", &bmxImports.KERNEL32_QueryPerformanceFrequency},
    {"KERNEL32.dll", "QueryPerformanceCounter", &bmxImports.KERNEL32_QueryPerformanceCounter},
    {"KERNEL32.dll", "HeapFree", &bmxImports.KERNEL32_HeapFree},
    {"KERNEL32.dll", "GetModuleFileNameA", &bmxImports.KERNEL32_GetModuleFileNameA},
    {"KERNEL32.dll", "HeapAlloc", &bmxImports.KERNEL32_HeapAlloc},
    {"KERNEL32.dll", "GetModuleHandleA", &bmxImports.KERNEL32_GetModuleHandleA},
    {"KERNEL32.dll", "GetStartupInfoA", &bmxImports.KERNEL32_GetStartupInfoA},
    {"KERNEL32.dll", "GetCommandLineA", &bmxImports.KERNEL32_GetCommandLineA},
    {"KERNEL32.dll", "GetVersion", &bmxImports.KERNEL32_GetVersion},
    {"KERNEL32.dll", "ExitProcess", &bmxImports.KERNEL32_ExitProcess},
    {"KERNEL32.dll", "TerminateProcess", &bmxImports.KERNEL32_TerminateProcess},
    {"KERNEL32.dll", "GetCurrentProcess", &bmxImports.KERNEL32_GetCurrentProcess},
    {"KERNEL32.dll", "GetEnvironmentVariableA", &bmxImports.KERNEL32_GetEnvironmentVariableA},
    {"KERNEL32.dll", "GetVersionExA", &bmxImports.KERNEL32_GetVersionExA},
    {"KERNEL32.dll", "HeapDestroy", &bmxImports.KERNEL32_HeapDestroy},
    {"KERNEL32.dll", "HeapCreate", &bmxImports.KERNEL32_HeapCreate},
    {"KERNEL32.dll", "VirtualFree", &bmxImports.KERNEL32_VirtualFree},
    {"KERNEL32.dll", "VirtualAlloc", &bmxImports.KERNEL32_VirtualAlloc},
    {"KERNEL32.dll", "HeapReAlloc", &bmxImports.KERNEL32_HeapReAlloc},
    {"KERNEL32.dll", "IsBadWritePtr", &bmxImports.KERNEL32_IsBadWritePtr},
    {"KERNEL32.dll", "GetLastError", &bmxImports.KERNEL32_GetLastError},
    {"KERNEL32.dll", "SetConsoleCtrlHandler", &bmxImports.KERNEL32_SetConsoleCtrlHandler},
    {"KERNEL32.dll", "UnhandledExceptionFilter", &bmxImports.KERNEL32_UnhandledExceptionFilter},
    {"KERNEL32.dll", "FreeEnvironmentStringsA", &bmxImports.KERNEL32_FreeEnvironmentStringsA},
    {"KERNEL32.dll", "GetProcAddress", &bmxImports.KERNEL32_GetProcAddress},
    {"KERNEL32.dll", "WideCharToMultiByte", &bmxImports.KERNEL32_WideCharToMultiByte},
    {"KERNEL32.dll", "GetEnvironmentStrings", &bmxImports.KERNEL32_GetEnvironmentStrings},
    {"KERNEL32.dll", "GetEnvironmentStringsW", &bmxImports.KERNEL32_GetEnvironmentStringsW},
    {"KERNEL32.dll", "SetHandleCount", &bmxImports.KERNEL32_SetHandleCount},
    {"KERNEL32.dll", "GetStdHandle", &bmxImports.KERNEL32_GetStdHandle},
    {"KERNEL32.dll", "GetFileType", &bmxImports.KERNEL32_GetFileType},
    {"KERNEL32.dll", "RtlUnwind", &bmxImports.KERNEL32_RtlUnwind},
    {"KERNEL32.dll", "WriteFile", &bmxImports.KERNEL32_WriteFile},
    {"KERNEL32.dll", "RaiseException", &bmxImports.KERNEL32_RaiseException},
    {"KERNEL32.dll", "MultiByteToWideChar", &bmxImports.KERNEL32_MultiByteToWideChar},
    {"KERNEL32.dll", "GetSystemTime", &bmxImports.KERNEL32_GetSystemTime},
    {"KERNEL32.dll", "GetLocalTime", &bmxImports.KERNEL32_GetLocalTime},
    {"KERNEL32.dll", "FlushFileBuffers", &bmxImports.KERNEL32_FlushFileBuffers},
    {"KERNEL32.dll", "GetCPInfo", &bmxImports.KERNEL32_GetCPInfo},
    {"KERNEL32.dll", "GetACP", &bmxImports.KERNEL32_GetACP},
    {"KERNEL32.dll", "GetOEMCP", &bmxImports.KERNEL32_GetOEMCP},
    {"KERNEL32.dll", "GetStringTypeA", &bmxImports.KERNEL32_GetStringTypeA},
    {"KERNEL32.dll", "GetStringTypeW", &bmxImports.KERNEL32_GetStringTypeW},
    {"KERNEL32.dll", "SetStdHandle", &bmxImports.KERNEL32_SetStdHandle},
    {"KERNEL32.dll", "SetEndOfFile", &bmxImports.KERNEL32_SetEndOfFile},
    {"KERNEL32.dll", "CompareStringA", &bmxImports.KERNEL32_CompareStringA},
    {"KERNEL32.dll", "CompareStringW", &bmxImports.KERNEL32_CompareStringW},
    {"KERNEL32.dll", "SetEnvironmentVariableA", &bmxImports.KERNEL32_SetEnvironmentVariableA},
    {"MSVFW32.dll", "DrawDibDraw", &bmxImports.MSVFW32_DrawDibDraw},
    {"MSVFW32.dll", "DrawDibOpen", &bmxImports.MSVFW32_DrawDibOpen},
    {"MSVFW32.dll", "DrawDibClose", &bmxImports.MSVFW32_DrawDibClose},
    {"USER32.dll", "DispatchMessageA", &bmxImports.USER32_DispatchMessageA},
    {"USER32.dll", "EndDialog", &bmxImports.USER32_EndDialog},
    {"USER32.dll", "GetDlgItem", &bmxImports.USER32_GetDlgItem},
    {"USER32.dll", "SendMessageA", &bmxImports.USER32_SendMessageA},
    {"USER32.dll", "TranslateMessage", &bmxImports.USER32_TranslateMessage},
    {"USER32.dll", "ShowWindow", &bmxImports.USER32_ShowWindow},
    {"USER32.dll", "SetWindowPos", &bmxImports.USER32_SetWindowPos},
    {"USER32.dll", "PostQuitMessage", &bmxImports.USER32_PostQuitMessage},
    {"USER32.dll", "DialogBoxParamA", &bmxImports.USER32_DialogBoxParamA},
    {"USER32.dll", "SetFocus", &bmxImports.USER32_SetFocus},
    {"USER32.dll", "AdjustWindowRect", &bmxImports.USER32_AdjustWindowRect},
    {"USER32.dll", "RegisterClassA", &bmxImports.USER32_RegisterClassA},
    {"USER32.dll", "LoadCursorA", &bmxImports.USER32_LoadCursorA},
    {"USER32.dll", "DefWindowProcA", &bmxImports.USER32_DefWindowProcA},
    {"USER32.dll", "ClipCursor", &bmxImports.USER32_ClipCursor},
    {"USER32.dll", "GetWindowLongA", &bmxImports.USER32_GetWindowLongA},
    {"USER32.dll", "GetClientRect", &bmxImports.USER32_GetClientRect},
    {"USER32.dll", "ReleaseDC", &bmxImports.USER32_ReleaseDC},
    {"USER32.dll", "GetDC", &bmxImports.USER32_GetDC},
    {"USER32.dll", "CreateWindowExA", &bmxImports.USER32_CreateWindowExA},
    {"USER32.dll", "ShowCursor", &bmxImports.USER32_ShowCursor},
    {"USER32.dll", "SetWindowLongA", &bmxImports.USER32_SetWindowLongA},
    {"USER32.dll", "DestroyWindow", &bmxImports.USER32_DestroyWindow},
    {"USER32.dll", "PeekMessageA", &bmxImports.USER32_PeekMessageA},
    {"USER32.dll", "UpdateWindow", &bmxImports.USER32_UpdateWindow},
    {"USER32.dll", "MessageBoxA", &bmxImports.USER32_MessageBoxA},
    {"USER32.dll", "SetWindowTextA", &bmxImports.USER32_SetWindowTextA},
    {"USER32.dll", "ClientToScreen", &bmxImports.USER32_ClientToScreen},
    {"WINMM.dll", "waveOutWrite", &bmxImports.WINMM_waveOutWrite},
    {"WINMM.dll", "waveOutOpen", &bmxImports.WINMM_waveOutOpen},
    {"WINMM.dll", "timeGetDevCaps", &bmxImports.WINMM_timeGetDevCaps},
    {"WINMM.dll", "timeBeginPeriod", &bmxImports.WINMM_timeBeginPeriod},
    {"WINMM.dll", "sndPlaySoundA", &bmxImports.WINMM_sndPlaySoundA},
    {"WINMM.dll", "waveOutRestart", &bmxImports.WINMM_waveOutRestart},
    {"WINMM.dll", "waveOutPause", &bmxImports.WINMM_waveOutPause},
    {"WINMM.dll", "timeSetEvent", &bmxImports.WINMM_timeSetEvent},
    {"WINMM.dll", "waveOutReset", &bmxImports.WINMM_waveOutReset},
    {"WINMM.dll", "waveOutUnprepareHeader", &bmxImports.WINMM_waveOutUnprepareHeader},
    {"WINMM.dll", "waveOutClose", &bmxImports.WINMM_waveOutClose},
    {"WINMM.dll", "timeKillEvent", &bmxImports.WINMM_timeKillEvent},
    {"WINMM.dll", "timeGetTime", &bmxImports.WINMM_timeGetTime},
    {"WINMM.dll", "waveOutPrepareHeader", &bmxImports.WINMM_waveOutPrepareHeader},
    {"WINMM.dll", "timeEndPeriod", &bmxImports.WINMM_timeEndPeriod},
};
const uint32_t bmxImportSlotCount = 137;

/* The original's `jmp [import slot]` thunks that ported code calls by address (ORIG_FN_STDCALL). */
BMX_IMPORT_THUNK(004af9d0, 0x510); /* AVIFIL32!AVIFileInit */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIFileInit) == 0x510, "AVIFIL32_AVIFileInit");
BMX_IMPORT_THUNK(004af9d6, 0x4dc); /* AVIFIL32!AVIFileExit */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIFileExit) == 0x4dc, "AVIFIL32_AVIFileExit");
BMX_IMPORT_THUNK(004af9dc, 0x4d8); /* AVIFIL32!AVIStreamReadFormat */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIStreamReadFormat) == 0x4d8,
               "AVIFIL32_AVIStreamReadFormat");
BMX_IMPORT_THUNK(004af9e2, 0x508); /* AVIFIL32!AVIStreamInfoA */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIStreamInfoA) == 0x508, "AVIFIL32_AVIStreamInfoA");
BMX_IMPORT_THUNK(004af9e8, 0x50c); /* AVIFIL32!AVIStreamRead */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIStreamRead) == 0x50c, "AVIFIL32_AVIStreamRead");
BMX_IMPORT_THUNK(004af9ee, 0x4f8); /* AVIFIL32!AVIStreamStart */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIStreamStart) == 0x4f8, "AVIFIL32_AVIStreamStart");
BMX_IMPORT_THUNK(004af9f4, 0x4f4); /* AVIFIL32!AVIStreamLength */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIStreamLength) == 0x4f4, "AVIFIL32_AVIStreamLength");
BMX_IMPORT_THUNK(004af9fa, 0x4f0); /* AVIFIL32!AVIStreamTimeToSample */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIStreamTimeToSample) == 0x4f0,
               "AVIFIL32_AVIStreamTimeToSample");
BMX_IMPORT_THUNK(004afa00, 0x724); /* MSVFW32!DrawDibOpen */
_Static_assert(offsetof(struct BmxImports, MSVFW32_DrawDibOpen) == 0x724, "MSVFW32_DrawDibOpen");
BMX_IMPORT_THUNK(004afa06, 0x504); /* AVIFIL32!AVIStreamGetFrameOpen */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIStreamGetFrameOpen) == 0x504,
               "AVIFIL32_AVIStreamGetFrameOpen");
BMX_IMPORT_THUNK(004afa0c, 0x500); /* AVIFIL32!AVIStreamSampleToTime */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIStreamSampleToTime) == 0x500,
               "AVIFIL32_AVIStreamSampleToTime");
BMX_IMPORT_THUNK(004afa12, 0x4fc); /* AVIFIL32!AVIFileRelease */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIFileRelease) == 0x4fc, "AVIFIL32_AVIFileRelease");
BMX_IMPORT_THUNK(004afa18, 0x518); /* AVIFIL32!AVIFileGetStream */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIFileGetStream) == 0x518, "AVIFIL32_AVIFileGetStream");
BMX_IMPORT_THUNK(004afa24, 0x4ec); /* AVIFIL32!AVIFileOpenA */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIFileOpenA) == 0x4ec, "AVIFIL32_AVIFileOpenA");
BMX_IMPORT_THUNK(004afa2a, 0x4e8); /* AVIFIL32!AVIStreamRelease */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIStreamRelease) == 0x4e8, "AVIFIL32_AVIStreamRelease");
BMX_IMPORT_THUNK(004afa30, 0x4e4); /* AVIFIL32!AVISaveOptionsFree */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVISaveOptionsFree) == 0x4e4,
               "AVIFIL32_AVISaveOptionsFree");
BMX_IMPORT_THUNK(004afa36, 0x728); /* MSVFW32!DrawDibClose */
_Static_assert(offsetof(struct BmxImports, MSVFW32_DrawDibClose) == 0x728, "MSVFW32_DrawDibClose");
BMX_IMPORT_THUNK(004afa3c, 0x4e0); /* AVIFIL32!AVIStreamGetFrameClose */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIStreamGetFrameClose) == 0x4e0,
               "AVIFIL32_AVIStreamGetFrameClose");
BMX_IMPORT_THUNK(004afa42, 0x720); /* MSVFW32!DrawDibDraw */
_Static_assert(offsetof(struct BmxImports, MSVFW32_DrawDibDraw) == 0x720, "MSVFW32_DrawDibDraw");
BMX_IMPORT_THUNK(004afa48, 0x4d4); /* AVIFIL32!AVIStreamGetFrame */
_Static_assert(offsetof(struct BmxImports, AVIFIL32_AVIStreamGetFrame) == 0x4d4,
               "AVIFIL32_AVIStreamGetFrame");
BMX_IMPORT_THUNK(004afc58, 0x588); /* DINPUT!DirectInputCreateEx */
_Static_assert(offsetof(struct BmxImports, DINPUT_DirectInputCreateEx) == 0x588,
               "DINPUT_DirectInputCreateEx");
