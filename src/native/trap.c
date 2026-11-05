/*
 * Reports traps (an unported original function reached through a trap stub, see bmxOrigTrap) and faults.
 * Names come from data/functions.csv and work/labels.raw when they can be found (development convenience, not
 * required).
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include "native/native.h"

enum { kImageEnd = 0x6b3000 };

typedef struct Sym {
    uint32_t addr;
    uint32_t seq; /* load order: on equal addresses the later (labels.raw) entry wins */
    char name[64];
} Sym;

static Sym *syms;
static size_t symCount, symCap;
static FILE *logFile;

void bmxLogOpen(void)
{
    char path[MAX_PATH];
    char *slash;

    GetModuleFileNameA(NULL, path, sizeof path);
    slash = strrchr(path, '\\');
    strcpy(slash ? slash + 1 : path, "bmxnative.log");
    logFile = fopen(path, "w");
}

void bmxLog(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (logFile) {
        va_start(ap, fmt);
        vfprintf(logFile, fmt, ap);
        va_end(ap);
        fflush(logFile);
    }
}

static void addSym(uint32_t addr, const char *name)
{
    if (symCount == symCap) {
        symCap = symCap ? symCap * 2 : 4096;
        syms = realloc(syms, symCap * sizeof *syms);
    }
    syms[symCount].addr = addr;
    syms[symCount].seq = (uint32_t)symCount;
    snprintf(syms[symCount].name, sizeof syms[symCount].name, "%s", name);
    symCount++;
}

static int symCmp(const void *a, const void *b)
{
    const Sym *x = a, *y = b;

    if (x->addr != y->addr) {
        return x->addr < y->addr ? -1 : 1;
    }
    return x->seq < y->seq ? -1 : x->seq > y->seq;
}

static FILE *openFirst(const char *a, const char *b)
{
    FILE *f = fopen(a, "r");
    return f ? f : fopen(b, "r");
}

static void loadSymbols(void)
{
    char line[512];
    FILE *f = openFirst("functions.csv", BMX_SOURCE_DIR "/data/functions.csv");

    if (f) {
        while (fgets(line, sizeof line, f)) {
            char *col[6];
            char *p = line;
            int n = 0;

            for (; n < 6 && p; n++) {
                col[n] = p;
                p = strchr(p, ',');
                if (p) {
                    *p++ = 0;
                }
            }
            if (n == 6) {
                addSym((uint32_t)strtoul(col[0], NULL, 16), col[4]);
            }
        }
        fclose(f);
    }
    f = openFirst("labels.raw", BMX_WORK_DIR "/labels.raw");
    if (f) {
        while (fgets(line, sizeof line, f)) {
            char *tab = strchr(line, '\t');

            if (tab) {
                *tab = 0;
                tab[strcspn(tab + 1, "\r\n") + 1] = 0;
                addSym((uint32_t)strtoul(line, NULL, 16), tab + 1);
            }
        }
        fclose(f);
    }
    qsort(syms, symCount, sizeof *syms, symCmp);
}

/* Describe an address: an original address by symbol, an address in our code by the port it is inside. */
static void describe(uint32_t a, char *out, size_t n)
{
    if (a >= kBmxImageBase && a < kImageEnd) {
        const Sym *best = NULL;

        for (size_t i = 0; i < symCount && syms[i].addr <= a; i++) {
            best = &syms[i];
        }
        if (best) {
            snprintf(out, n, "%s+%#x", best->name, (unsigned)(a - best->addr));
            return;
        }
    } else {
        const BmxOrigEntry *best = NULL;

        for (uint32_t i = 0; i < bmxOrigMapCount; i++) {
            const uint32_t impl = (uint32_t)(uintptr_t)bmxOrigMap[i].impl;

            if (impl <= a && (!best || impl > (uint32_t)(uintptr_t)best->impl)) {
                best = &bmxOrigMap[i];
            }
        }
        if (best) {
            char inner[96];

            describe(best->orig, inner, sizeof inner);
            snprintf(out, n, "in port of %s [%#x]", inner, (unsigned)best->orig);
            return;
        }
    }
    snprintf(out, n, "?");
}

void bmxDescribeAddress(uint32_t addr, char *out, size_t size)
{
    if (symCount == 0) {
        loadSymbols();
    }
    describe(addr, out, size);
}

void bmxOrigTrap(uint32_t orig, uint32_t callerRet)
{
    char what[160], who[200];

    bmxDescribeAddress(orig, what, sizeof what);
    bmxDescribeAddress(callerRet, who, sizeof who);
    bmxLog("TRAP: unported original function %08x %s reached\n  caller: %08x %s\n", (unsigned)orig, what,
           (unsigned)callerRet, who);
    if (logFile) {
        fclose(logFile);
    }
    ExitProcess(3);
}

static LONG CALLBACK trapHandler(EXCEPTION_POINTERS *ep)
{
    static volatile LONG reported;
    const DWORD code = ep->ExceptionRecord->ExceptionCode;
#if defined(__x86_64__)
    const uintptr_t pc = (uintptr_t)ep->ContextRecord->Rip;
    const uintptr_t sp = (uintptr_t)ep->ContextRecord->Rsp;
#else
    const uintptr_t pc = ep->ContextRecord->Eip;
    const uintptr_t sp = ep->ContextRecord->Esp;
#endif
    char what[160], who[200];
    uintptr_t ret = 0;

    if (code != EXCEPTION_BREAKPOINT && code != EXCEPTION_ACCESS_VIOLATION &&
        code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_INT_DIVIDE_BY_ZERO &&
        code != EXCEPTION_PRIV_INSTRUCTION) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (!IsBadReadPtr((void *)sp, sizeof(uintptr_t))) {
        ret = *(const uintptr_t *)sp;
    }
    /* Our code is below 4 GB (fixed image base, CMakeLists.txt): its addresses fit the symbol table's. */
    describe((uint32_t)pc, what, sizeof what);
    describe((uint32_t)ret, who, sizeof who);
    if (InterlockedExchange(&reported, 1)) {
        ExitProcess(3);
    }
    bmxLog("EXCEPTION %08lx at %p  %s\n", code, (void *)pc, what);
    if (code == EXCEPTION_ACCESS_VIOLATION) {
        const uintptr_t addr = ep->ExceptionRecord->ExceptionInformation[1];

        bmxLog("  access address %p (%s)%s\n", (void *)addr,
               ep->ExceptionRecord->ExceptionInformation[0] ? "write" : "read",
               addr >= kBmxImageBase && addr < kImageEnd ? ": an original address, not converted?" : "");
    }
    if (pc >= kBmxImageBase && pc < kImageEnd) {
        bmxLog(
            "  (a jump to original code, which is not loaded: an ORIG_FN or code pointer not converted?)\n");
    }
    bmxLog("  caller (return address at the stack pointer): %p  %s\n", (void *)ret, who);
    for (int i = 1; i < 5; i++) {
        const uintptr_t *slot = (const uintptr_t *)sp + i;

        if (IsBadReadPtr(slot, sizeof *slot)) {
            break;
        }
        bmxLog("  stack[%d]: %p\n", i, (void *)*slot);
    }
    if (logFile) {
        fclose(logFile);
    }
    ExitProcess(3);
    return EXCEPTION_CONTINUE_SEARCH;
}

void bmxTrapInstall(void)
{
    if (symCount == 0) {
        loadSymbols();
    }
    /* Under a debugger the debugger reports the fault (with a backtrace); the handler would end the process
     * before it sees it. */
    if (!IsDebuggerPresent()) {
        AddVectoredExceptionHandler(1, trapHandler);
    }
}
