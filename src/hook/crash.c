/*
 * Crash report for the in-game hook (dev only). A vectored exception handler writes bmxcrash.txt into the
 * game's working directory on the first fatal exception: the exception, the registers and every stack word
 * that points into code (the original's .text, or this DLL), so a crash can be traced back to a function
 * offline (data/functions.csv for exe addresses, llvm-symbolizer on dinput.dll for ours). The game still
 * crashes afterwards as it would have. The file is rewritten on every crash-type exception (see handler).
 */
#include <stddef.h>
#include <stdint.h>

#include "hook/hook.h"

typedef int BOOL;
typedef unsigned long DWORD;
typedef void *HANDLE;

typedef struct ExceptionRecord {
    DWORD code;
    DWORD flags;
    struct ExceptionRecord *next;
    void *address;
    DWORD numParams;
    uintptr_t info[15];
} ExceptionRecord;

/* x86 CONTEXT, general registers only (offsets from winnt.h). */
typedef struct Context {
    DWORD flags;
    DWORD debug[6];
    uint8_t floatSave[112];
    DWORD segGs, segFs, segEs, segDs;
    DWORD edi, esi, ebx, edx, ecx, eax;
    DWORD ebp, eip, segCs, eflags, esp, segSs;
} Context;
_Static_assert(offsetof(Context, eip) == 0xb8, "CONTEXT.Eip");
_Static_assert(offsetof(Context, esp) == 0xc4, "CONTEXT.Esp");

typedef struct ExceptionPointers {
    ExceptionRecord *record;
    Context *context;
} ExceptionPointers;

__declspec(dllimport) void *__stdcall
AddVectoredExceptionHandler(DWORD first, long(__stdcall *handler)(ExceptionPointers *));
__declspec(dllimport) HANDLE __stdcall CreateFileA(const char *name, DWORD access, DWORD share, void *sec,
                                                   DWORD disposition, DWORD flags, HANDLE templ);
__declspec(dllimport) BOOL __stdcall WriteFile(HANDLE file, const void *data, DWORD size, DWORD *written,
                                               void *overlapped);
__declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE handle);
__declspec(dllimport) BOOL __stdcall IsBadReadPtr(const void *p, size_t size);

#define INVALID_HANDLE_VALUE      ((HANDLE)(intptr_t)-1)
#define EXCEPTION_CONTINUE_SEARCH 0

enum {
    kExeCodeLo = 0x401000,
    kExeCodeHi = 0x54b000,
    kStackWords = 512,
};

static uintptr_t gDllLo, gDllHi;
static volatile long gBusy;
static char gOut[16384];
static uint32_t gLen;

static void put(const char *s)
{
    while (*s != '\0' && gLen < sizeof gOut - 1) {
        gOut[gLen++] = *s++;
    }
}

static void putHex(const char *label, uint32_t v)
{
    static const char digits[] = "0123456789abcdef";
    char buf[9];

    for (int i = 7; i >= 0; i--) {
        buf[i] = digits[v & 15];
        v >>= 4;
    }
    buf[8] = '\0';
    put(label);
    put(buf);
}

static int isCode(uint32_t v)
{
    return (v >= kExeCodeLo && v < kExeCodeHi) || (v >= gDllLo && v < gDllHi);
}

static long __stdcall handler(ExceptionPointers *ep)
{
    const ExceptionRecord *r = ep->record;
    const Context *c = ep->context;
    HANDLE file;
    DWORD written;

    /* Crash-type exceptions only. Drivers may raise and handle some themselves (first chance), so the file is
     * rewritten every time: after a crash it holds the last one, the one that killed the game. */
    if (r->code != 0xc0000005u && r->code != 0xc000001du && r->code != 0xc0000094u &&
        r->code != 0xc0000096u && r->code != 0xc00000fdu) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    while (__atomic_exchange_n(&gBusy, 1, __ATOMIC_ACQUIRE) != 0) {
    }
    gLen = 0;
    putHex("exception ", r->code);
    putHex(" at ", (uint32_t)(uintptr_t)r->address);
    if (r->code == 0xc0000005u && r->numParams >= 2) {
        put(r->info[0] == 0 ? " reading " : r->info[0] == 1 ? " writing " : " executing ");
        putHex("", (uint32_t)r->info[1]);
    }
    putHex("\ndll ", (uint32_t)gDllLo);
    putHex("..", (uint32_t)gDllHi);
    putHex("\neax ", c->eax);
    putHex(" ebx ", c->ebx);
    putHex(" ecx ", c->ecx);
    putHex(" edx ", c->edx);
    putHex(" esi ", c->esi);
    putHex(" edi ", c->edi);
    putHex("\nebp ", c->ebp);
    putHex(" esp ", c->esp);
    putHex(" eip ", c->eip);
    put("\ncode addresses on the stack (offset from esp: value):\n");
    for (uint32_t i = 0; i < kStackWords; i++) {
        const uint32_t *p = (const uint32_t *)(uintptr_t)(c->esp + i * 4);

        if (IsBadReadPtr(p, 4)) {
            break;
        }
        if (isCode(*p)) {
            putHex("  +", i * 4);
            putHex(": ", *p);
            put("\n");
        }
    }
    file = CreateFileA("bmxcrash.txt", 0x40000000, 1, NULL, 2, 0x80, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        WriteFile(file, gOut, gLen, &written, NULL);
        CloseHandle(file);
    }
    __atomic_store_n(&gBusy, 0, __ATOMIC_RELEASE);
    return EXCEPTION_CONTINUE_SEARCH;
}

void bmxCrashInstall(void *module)
{
    const uint8_t *base = module;
    const uint32_t pe = *(const uint32_t *)(base + 0x3c);

    gDllLo = (uintptr_t)base;
    gDllHi = gDllLo + *(const uint32_t *)(base + pe + 0x50); /* OptionalHeader.SizeOfImage */
    AddVectoredExceptionHandler(1, handler);
}
