/*
 * Development-only loader for the hook phase: built as dinput.dll and dropped next to NgBMX.exe.
 * On load it redirects every ported function to our implementation and forwards the one DirectInput
 * export the game imports to the system DLL. The final port replaces the exe instead.
 */
#include <stddef.h>
#include <stdint.h>

#include "hook/hook.h"

typedef int BOOL;
typedef unsigned long DWORD;
typedef void *HANDLE;
typedef void *HMODULE;
typedef void *(__stdcall *FARPROC)(void);

__declspec(dllimport) BOOL __stdcall VirtualProtect(void *addr, size_t size, DWORD prot, DWORD *old);
__declspec(dllimport) BOOL __stdcall FlushInstructionCache(HANDLE process, const void *addr, size_t size);
__declspec(dllimport) HANDLE __stdcall GetCurrentProcess(void);
__declspec(dllimport) unsigned __stdcall GetSystemDirectoryA(char *buf, unsigned size);
__declspec(dllimport) HMODULE __stdcall LoadLibraryA(const char *name);
__declspec(dllimport) FARPROC __stdcall GetProcAddress(HMODULE module, const char *name);

#define PAGE_EXECUTE_READWRITE 0x40
#define DLL_PROCESS_ATTACH     1

static void patchJump(uint32_t from, const void *to)
{
    uint8_t *p = (uint8_t *)(uintptr_t)from;
    const int32_t rel = (int32_t)((uintptr_t)to - (from + 5));
    DWORD old;

    VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &old);
    p[0] = 0xE9;
    __builtin_memcpy(p + 1, &rel, 4);
    VirtualProtect(p, 5, old, &old);
}

typedef long(__stdcall *DirectInputCreateExFn)(void *, DWORD, const void *, void **, void *);

__declspec(dllexport) long __stdcall bmxDirectInputCreateEx(void *inst, DWORD ver, const void *iid,
                                                            void **out, void *outer)
{
    static DirectInputCreateExFn real;
    if (!real) {
        char path[260];
        const unsigned n = GetSystemDirectoryA(path, sizeof path - 16);
        __builtin_memcpy(path + n, "\\dinput.dll", 12);
        real = (DirectInputCreateExFn)(void *)GetProcAddress(LoadLibraryA(path), "DirectInputCreateEx");
    }
    return real(inst, ver, iid, out, outer);
}

BOOL __stdcall DllMain(HMODULE module, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        for (uint32_t i = 0; i < bmxHookCount; i++) {
            patchJump(bmxHooks[i].orig, bmxHooks[i].impl);
        }
        FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    }
    return 1;
}
