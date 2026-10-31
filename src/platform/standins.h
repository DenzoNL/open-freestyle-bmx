/* The stand-in functions the import table points at (docs/PLATFORM.md): Win32 signatures, stdcall. */
#ifndef BMX_PLATFORM_STANDINS_H
#define BMX_PLATFORM_STANDINS_H

#include <stdint.h>

#include "platform/platform.h"

/* kernel.c */
uint32_t BMX_WINAPI bmxStandTimeGetTime(void);
uint32_t BMX_WINAPI bmxStandGetTickCount(void);
int32_t BMX_WINAPI bmxStandQueryPerformanceFrequency(void *out);
int32_t BMX_WINAPI bmxStandQueryPerformanceCounter(void *out);
void BMX_WINAPI bmxStandSleep(uint32_t ms);
void BMX_WINAPI bmxStandOutputDebugStringA(const char *text);
void BMX_WINAPI bmxStandDebugBreak(void);
void BMX_WINAPI bmxStandInitializeCriticalSection(void *section);
void BMX_WINAPI bmxStandEnterCriticalSection(void *section);
void BMX_WINAPI bmxStandLeaveCriticalSection(void *section);
void BMX_WINAPI bmxStandDeleteCriticalSection(void *section);

/* winmm.c: the multimedia timer (TIMECAPS is two uint32_t). */
typedef void(BMX_WINAPI *BmxTimeProc)(uint32_t id, uint32_t message, uint32_t user, uint32_t arg1,
                                      uint32_t arg2);
uint32_t BMX_WINAPI bmxStandTimeGetDevCaps(void *caps, uint32_t size);
uint32_t BMX_WINAPI bmxStandTimeBeginPeriod(uint32_t period);
uint32_t BMX_WINAPI bmxStandTimeEndPeriod(uint32_t period);
uint32_t BMX_WINAPI bmxStandTimeSetEvent(uint32_t delay, uint32_t resolution, BmxTimeProc proc, uint32_t user,
                                         uint32_t flags);
uint32_t BMX_WINAPI bmxStandTimeKillEvent(uint32_t id);

/* fileio.c: the file functions of the sound loader (the handle is our pointer). */
void *BMX_WINAPI bmxStandCreateFileA(const char *name, uint32_t access, uint32_t share, void *security,
                                     uint32_t disposition, uint32_t flags, void *templateFile);
int32_t BMX_WINAPI bmxStandReadFile(void *file, void *buffer, uint32_t bytes, uint32_t *read,
                                    void *overlapped);
uint32_t BMX_WINAPI bmxStandSetFilePointer(void *file, int32_t distance, int32_t *high, uint32_t method);
int32_t BMX_WINAPI bmxStandCloseHandle(void *file);

/* input.c: DirectInput on SDL3 (iid and out as in DirectInputCreateEx). */
int32_t BMX_WINAPI bmxStandDirectInputCreateEx(void *instance, uint32_t version, const void *iid, void **out,
                                               void *outer);

/* dsound.c: DirectSound on SDL3 audio, and the DLL loading that hands it to the game. */
void *BMX_WINAPI bmxStandLoadLibraryA(const char *name);
void *BMX_WINAPI bmxStandGetProcAddress(void *module, const char *name);
int32_t BMX_WINAPI bmxStandFreeLibrary(void *module);
int32_t BMX_WINAPI bmxStandDirectSoundCreate(const void *guid, void **out, void *outer);

/* avi.c */
int32_t BMX_WINAPI bmxStandAVISaveOptionsFree(int count, void **options);

/* The BMX_KEYS stand-in for MessageBoxA (src/native/keys.c, with the Windows types): a table of its own,
 * since only the definition knows the stdcall argument sizes that decorate the symbols. */
extern const BmxStandIn bmxKeysStandIns[];
extern const size_t bmxKeysStandInCount;

#endif
