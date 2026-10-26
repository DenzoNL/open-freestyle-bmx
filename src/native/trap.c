/*
 * Reports int3 traps: the first unported function the game reaches. Names come from data/functions.csv
 * and work/labels.raw when they can be found (development convenience, not required).
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include "hook/hook.h"
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
static int skipLeft;

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
        const BmxHook *best = NULL;

        for (uint32_t i = 0; i < bmxHookCount; i++) {
            const uint32_t impl = (uint32_t)(uintptr_t)bmxHooks[i].impl;

            if (impl <= a && (!best || impl > (uint32_t)(uintptr_t)best->impl)) {
                best = &bmxHooks[i];
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

static LONG CALLBACK trapHandler(EXCEPTION_POINTERS *ep)
{
    static volatile LONG reported;
    const DWORD code = ep->ExceptionRecord->ExceptionCode;
    const uint32_t eip = ep->ContextRecord->Eip;
    const uint32_t esp = ep->ContextRecord->Esp;
    char what[160], who[200];
    uint32_t ret = 0;

    if (code != EXCEPTION_BREAKPOINT && code != EXCEPTION_ACCESS_VIOLATION &&
        code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_INT_DIVIDE_BY_ZERO &&
        code != EXCEPTION_PRIV_INSTRUCTION) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (!IsBadReadPtr((void *)(uintptr_t)esp, 4)) {
        ret = *(const uint32_t *)(uintptr_t)esp;
    }
    describe(eip, what, sizeof what);
    describe(ret, who, sizeof who);
    /* BMX_TRAP_SKIP=N: log the first N traps and fake a cdecl "return 1" (success for most init calls)
     * instead of stopping, to list what the game needs next. Only a survey aid: the game state is bogus after
     * the first skip. */
    if (code == EXCEPTION_BREAKPOINT && skipLeft > 0) {
        skipLeft--;
        bmxLog("TRAP (skipped): %08x %s  <- %08x %s\n", (unsigned)eip, what, (unsigned)ret, who);
        ep->ContextRecord->Eax = 1;
        ep->ContextRecord->Eip = ret;
        ep->ContextRecord->Esp = esp + 4;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (InterlockedExchange(&reported, 1)) {
        ExitProcess(3);
    }
    if (code == EXCEPTION_BREAKPOINT) {
        bmxLog("TRAP: unported code reached at %08x  %s\n", (unsigned)eip, what);
    } else {
        bmxLog("EXCEPTION %08lx at %08x  %s\n", code, (unsigned)eip, what);
        if (code == EXCEPTION_ACCESS_VIOLATION) {
            bmxLog("  access address %08lx (%s)\n",
                   (unsigned long)ep->ExceptionRecord->ExceptionInformation[1],
                   ep->ExceptionRecord->ExceptionInformation[0] ? "write" : "read");
        }
    }
    bmxLog("  caller (return address at esp): %08x  %s\n", (unsigned)ret, who);
    for (int i = 1; i < 5 && !IsBadReadPtr((void *)(uintptr_t)(esp + (uint32_t)i * 4), 4); i++) {
        bmxLog("  stack[%d]: %08x\n", i, (unsigned)*(const uint32_t *)(uintptr_t)(esp + (uint32_t)i * 4));
    }
    if (logFile) {
        fclose(logFile);
    }
    ExitProcess(3);
    return EXCEPTION_CONTINUE_SEARCH;
}

void bmxTrapInstall(void)
{
    const char *skip = getenv("BMX_TRAP_SKIP");

    skipLeft = skip ? atoi(skip) : 0;
    loadSymbols();
    AddVectoredExceptionHandler(1, trapHandler);
}
