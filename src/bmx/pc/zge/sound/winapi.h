/*
 * The Win32 imports of the sound system, called through the exe's import table slots (the test DLL
 * imports nothing). With winds.h this is the complete platform surface of winChan / winStrm / winLoad /
 * bmxsound: files (sound assets and streamed samples), critical sections (stream list), DSOUND.DLL
 * loading, and the winmm multimedia timer that drives the mixer and the sequencer.
 */
#ifndef BMX_PC_ZGE_SOUND_WINAPI_H
#define BMX_PC_ZGE_SOUND_WINAPI_H

#include <stdint.h>

#include "compat/orig.h"

typedef void *WinHandle;

enum {
    kWinGenericRead = 0x80000000u,
    kWinFileShareRead = 1,
    kWinOpenExisting = 3,
    kWinFileAttributeNormal = 0x80,
    kWinFileFlagSequentialScan = 0x08000000,
    kWinFileBegin = 0,
    kWinFileEnd = 2,
};

#define kWinInvalidHandle ((WinHandle)(intptr_t)-1)

typedef WinHandle(BMX_STDCALL *WinCreateFileFn)(const char *name, uint32_t access, uint32_t share,
                                                void *security, uint32_t disposition, uint32_t flags,
                                                void *templateFile);
typedef int32_t(BMX_STDCALL *WinReadFileFn)(WinHandle file, void *buffer, uint32_t bytes, uint32_t *read,
                                            void *overlapped);
typedef uint32_t(BMX_STDCALL *WinSetFilePointerFn)(WinHandle file, int32_t distance, int32_t *high,
                                                   uint32_t method);
typedef int32_t(BMX_STDCALL *WinCloseHandleFn)(WinHandle file);
typedef WinHandle(BMX_STDCALL *WinLoadLibraryFn)(const char *name);
typedef void *(BMX_STDCALL *WinGetProcAddressFn)(WinHandle module, const char *name);
typedef int32_t(BMX_STDCALL *WinFreeLibraryFn)(WinHandle module);
typedef void(BMX_STDCALL *WinCriticalSectionFn)(void *section);

/* TIMECAPS and the timer callback of timeSetEvent. */
typedef struct WinTimeCaps {
    uint32_t periodMin;
    uint32_t periodMax;
} WinTimeCaps;
typedef uint32_t(BMX_STDCALL *WinTimeCallback)(uint32_t id, uint32_t message, uint32_t user, uint32_t arg1,
                                               uint32_t arg2);
typedef uint32_t(BMX_STDCALL *WinTimeGetDevCapsFn)(WinTimeCaps *caps, uint32_t size);
typedef uint32_t(BMX_STDCALL *WinTimePeriodFn)(uint32_t period);
typedef uint32_t(BMX_STDCALL *WinTimeSetEventFn)(uint32_t delay, uint32_t resolution,
                                                 WinTimeCallback callback, uint32_t user, uint32_t flags);
typedef uint32_t(BMX_STDCALL *WinTimeKillEventFn)(uint32_t id);

#define gWinReadFile              ORIG_VAR(0x006995b8, WinReadFileFn, gWinReadFile)
#define gWinFreeLibrary           ORIG_VAR(0x006995bc, WinFreeLibraryFn, gWinFreeLibrary)
#define gWinCloseHandle           ORIG_VAR(0x006995c0, WinCloseHandleFn, gWinCloseHandle)
#define gWinLoadLibrary           ORIG_VAR(0x006995c4, WinLoadLibraryFn, gWinLoadLibrary)
#define gWinSetFilePointer        ORIG_VAR(0x006995c8, WinSetFilePointerFn, gWinSetFilePointer)
#define gWinEnterCriticalSection  ORIG_VAR(0x006995cc, WinCriticalSectionFn, gWinEnterCriticalSection)
#define gWinLeaveCriticalSection  ORIG_VAR(0x006995d0, WinCriticalSectionFn, gWinLeaveCriticalSection)
#define gWinDeleteCriticalSection ORIG_VAR(0x006995d4, WinCriticalSectionFn, gWinDeleteCriticalSection)
#define gWinInitCriticalSection   ORIG_VAR(0x006995d8, WinCriticalSectionFn, gWinInitCriticalSection)
#define gWinCreateFile            ORIG_VAR(0x006995dc, WinCreateFileFn, gWinCreateFile)
#define gWinGetProcAddress        ORIG_VAR(0x00699660, WinGetProcAddressFn, gWinGetProcAddress)
#define gWinTimeGetDevCaps        ORIG_VAR(0x00699810, WinTimeGetDevCapsFn, gWinTimeGetDevCaps)
#define gWinTimeBeginPeriod       ORIG_VAR(0x00699814, WinTimePeriodFn, gWinTimeBeginPeriod)
#define gWinTimeSetEvent          ORIG_VAR(0x00699824, WinTimeSetEventFn, gWinTimeSetEvent)
#define gWinTimeKillEvent         ORIG_VAR(0x00699834, WinTimeKillEventFn, gWinTimeKillEvent)
#define gWinTimeEndPeriod         ORIG_VAR(0x00699840, WinTimePeriodFn, gWinTimeEndPeriod)

#endif
