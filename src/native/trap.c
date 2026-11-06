/*
 * Reports traps (an unported original function reached through a trap stub, see bmxOrigTrap) and faults.
 * Names come from data/functions.csv and work/labels.raw when they can be found (development convenience, not
 * required).
 */
#if defined(__linux__)
#define _GNU_SOURCE /* glibc under -std=c17: sigaction, siginfo_t, REG_RIP, readlink, strsignal */
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <signal.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif
#endif

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

#if defined(_WIN32)
void bmxExitProcess(int code)
{
    ExitProcess((UINT)code);
    __builtin_unreachable();
}

void bmxLogOpen(void)
{
    char path[MAX_PATH];
    char *slash;

    GetModuleFileNameA(NULL, path, sizeof path);
    slash = strrchr(path, '\\');
    strcpy(slash ? slash + 1 : path, "bmxnative.log");
    logFile = fopen(path, "w");
}
#else
/* ExitProcess: no atexit handlers; the C library's streams flushed, as Windows' CRT does at DLL detach. */
void bmxExitProcess(int code)
{
    fflush(NULL);
    _exit(code);
}

/* bmxnative.log next to the executable, as on Windows. */
void bmxLogOpen(void)
{
    char path[4096];
    char *slash;
    ssize_t n = -1;

#if defined(__APPLE__)
    uint32_t size = sizeof path;

    if (_NSGetExecutablePath(path, &size) == 0) {
        n = (ssize_t)strlen(path);
    }
#else
    n = readlink("/proc/self/exe", path, sizeof path - 1);
#endif
    if (n < 0 || (size_t)n + sizeof "bmxnative.log" > sizeof path) {
        n = 0;
    }
    path[n] = 0;
    slash = strrchr(path, '/');
    strcpy(slash ? slash + 1 : path, "bmxnative.log");
    logFile = fopen(path, "w");
}
#endif

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
        const size_t cap = symCap ? symCap * 2 : 4096;
        Sym *const grown = realloc(syms, cap * sizeof *syms);

        if (!grown) {
            return; /* names are a convenience: keep the ones we have */
        }
        syms = grown;
        symCap = cap;
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
                char *const name = tab + 1;
                char *const end = strpbrk(name, "\r\n");

                *tab = 0;
                if (end) {
                    *end = 0;
                }
                addSym((uint32_t)strtoul(line, NULL, 16), name);
            }
        }
        fclose(f);
    }
    qsort(syms, symCount, sizeof *syms, symCmp);
}

/* Describe an address: an original address by symbol, an address in our code by the port it is inside. */
static void describe(uintptr_t a, char *out, size_t n)
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
            const uintptr_t impl = (uintptr_t)bmxOrigMap[i].impl;

            if (impl <= a && (!best || impl > (uintptr_t)best->impl)) {
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
    bmxExitProcess(3);
}

#if defined(_WIN32)
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
    describe(pc, what, sizeof what);
    describe(ret, who, sizeof who);
    if (InterlockedExchange(&reported, 1)) {
        bmxExitProcess(3);
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
    bmxExitProcess(3);
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
#else
/* POSIX: the faulting pc, the stack pointer and the return address (arm64's link register; x86_64's top of
 * stack) from the signal context. */
static void contextRegisters(const void *context, uintptr_t *pc, uintptr_t *sp, uintptr_t *ret)
{
    const ucontext_t *const uc = context;

    *pc = *sp = *ret = 0;
#if defined(__APPLE__) && defined(__aarch64__)
    *pc = (uintptr_t)uc->uc_mcontext->__ss.__pc;
    *sp = (uintptr_t)uc->uc_mcontext->__ss.__sp;
    *ret = (uintptr_t)uc->uc_mcontext->__ss.__lr;
#elif defined(__APPLE__) && defined(__x86_64__)
    *pc = (uintptr_t)uc->uc_mcontext->__ss.__rip;
    *sp = (uintptr_t)uc->uc_mcontext->__ss.__rsp;
    *ret = *(const uintptr_t *)*sp;
#elif defined(__linux__) && defined(__aarch64__)
    *pc = (uintptr_t)uc->uc_mcontext.pc;
    *sp = (uintptr_t)uc->uc_mcontext.sp;
    *ret = (uintptr_t)uc->uc_mcontext.regs[30];
#elif defined(__linux__) && defined(__x86_64__)
    *pc = (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
    *sp = (uintptr_t)uc->uc_mcontext.gregs[REG_RSP];
    *ret = *(const uintptr_t *)*sp;
#else
    (void)uc;
#endif
}

static void trapHandler(int sig, siginfo_t *info, void *context)
{
    static volatile sig_atomic_t reported;
    const uintptr_t addr = (uintptr_t)info->si_addr;
    uintptr_t pc, sp, ret;
    char what[160], who[200];

    if (reported) {
        _exit(3);
    }
    reported = 1;
    contextRegisters(context, &pc, &sp, &ret);
    describe(pc, what, sizeof what);
    describe(ret, who, sizeof who);
    bmxLog("SIGNAL %d (%s) at %p  %s\n", sig, strsignal(sig), (void *)pc, what);
    if (sig == SIGSEGV || sig == SIGBUS) {
        bmxLog("  access address %p%s\n", (void *)addr,
               addr >= kBmxImageBase && addr < kImageEnd ? ": an original address, not converted?" : "");
    }
    if (pc >= kBmxImageBase && pc < kImageEnd) {
        bmxLog(
            "  (a jump to original code, which is not loaded: an ORIG_FN or code pointer not converted?)\n");
    }
    bmxLog("  caller (return address): %p  %s\n", (void *)ret, who);
    if (logFile) {
        fclose(logFile);
    }
    _exit(3);
}

void bmxTrapInstall(void)
{
    static const int kSignals[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGTRAP};
    struct sigaction action;

    if (symCount == 0) {
        loadSymbols();
    }
    /* Under a debugger (BMX_NO_TRAP=1) the debugger reports the fault with a backtrace. */
    if (getenv("BMX_NO_TRAP") != NULL) {
        return;
    }
    memset(&action, 0, sizeof action);
    action.sa_sigaction = trapHandler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < sizeof kSignals / sizeof kSignals[0]; i++) {
        sigaction(kSignals[i], &action, NULL);
    }
}
#endif
