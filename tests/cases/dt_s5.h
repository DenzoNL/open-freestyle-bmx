/*
 * Shared helpers of the zobj / zrwmodelcreate / zrwlight / zprop / ngapp / zge tests: a call log, a debug
 * print that logs its formatted arguments, and the usual stubs for the renderer pointers they call.
 */
#ifndef DT_S5_H
#define DT_S5_H

#include <stdarg.h>
#include <stdint.h>

#include "compat/crt.h"
#include "dt.h"
#include "dt_zmem.h"
#include "zge/zdebug.h"
#include "zge/zmath.h"

#define S5_FN __attribute__((unused)) static

enum { kS5LogWords = 8192 };
static uint32_t gS5Log[kS5LogWords];
static uint32_t gS5LogN;
/* Number of arguments the debug print reads (the original passes fewer than some formats ask for). */
static int gS5PrintMax;
/* Return values of the generic stubs, by address of the stubbed function. */
enum { kS5Rets = 64 };
static uint32_t gS5RetAddr[kS5Rets];
static uint32_t gS5RetVal[kS5Rets];
static int gS5RetN;

static inline void s5Log(uint32_t v)
{
    if (gS5LogN < kS5LogWords) {
        gS5Log[gS5LogN] = v;
    }
    gS5LogN++;
}

static inline void s5LogPtr(const void *p)
{
    s5Log(dt_canon((uint32_t)(uintptr_t)p));
}

static inline void s5LogFloat(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    s5Log(u);
}

static inline void s5LogWords(const void *p, int n)
{
    const uint32_t *w = p;
    for (int i = 0; i < n; i++) {
        s5Log(w[i]);
    }
}

static inline uint32_t s5Hash(const char *s)
{
    uint32_t h = 5381;
    while (*s) {
        h = h * 33 + (uint8_t)*s++;
    }
    return h;
}

/* The engine's printf-style debug print: logs the format and every %s / %d / %x argument. */
S5_FN void BMX_CDECL s5Print(const char *fmt, ...)
{
    va_list ap;
    int taken = 0;

    va_start(ap, fmt);
    s5Log(0x480240);
    s5Log(s5Hash(fmt));
    for (const char *p = fmt; *p; p++) {
        if (p[0] != '%') {
            continue;
        }
        if (gS5PrintMax != 0 && taken >= gS5PrintMax) {
            break;
        }
        if (p[1] == 's') {
            s5Log(s5Hash(va_arg(ap, const char *)));
            taken++;
        } else if (p[1] == 'd' || p[1] == 'x') {
            s5Log((uint32_t)va_arg(ap, int));
            taken++;
        }
    }
    va_end(ap);
}

S5_FN void BMX_CDECL s5Indent(int delta)
{
    s5Log(0x4802c0);
    s5Log((uint32_t)delta);
}

/* Replacement for zdebugFail: records the message and returns, so the code after the assert runs. */
S5_FN void BMX_CDECL s5Fail(const char *why, const char *file, int line, int flags)
{
    s5Log(0x47ed10);
    s5Log(s5Hash(why));
    s5Log(s5Hash(file));
    s5Log((uint32_t)line);
    s5Log((uint32_t)flags);
}

/* Fresh log; the debug print and indent hooks of the exe point at the logging versions. */
static inline void s5Begin(const DtCtx *c, int stubFail)
{
    dt_seed(c);
    dt_zmem_env();
    gS5LogN = 0;
    gS5PrintMax = 0;
    gS5RetN = 0;
    gZdebugPrint = s5Print;
    gZdebugIndent = s5Indent;
    if (stubFail) {
        dt_stub(0x0047ed10, NULL, (void *)s5Fail);
    }
}

static inline float s5Rf(void)
{
    return dt_float_edgy(-1000.0f, 1000.0f);
}

static inline void s5RandVec(ZVec3 *v)
{
    v->x = s5Rf();
    v->y = s5Rf();
    v->z = s5Rf();
}

static inline void s5RandQuat(ZQuat *q)
{
    q->x = dt_float(-1.0f, 1.0f);
    q->y = dt_float(-1.0f, 1.0f);
    q->z = dt_float(-1.0f, 1.0f);
    q->w = dt_float(-1.0f, 1.0f);
}

/* ---- scripted IFF reader --------------------------------------------------------------------------- */

/*
 * The chunk functions of the IFF reader replaced by a script: which chunk ids exist, the bytes ziffRead
 * hands out for the chunk found last, how often ziffNextChunk finds one, and the results of dispatching.
 */
typedef struct S5Chunk {
    uint32_t id;
    uint32_t size;
    const uint8_t *data;
} S5Chunk;

enum { kS5Chunks = 16 };
static S5Chunk gS5Chunk[kS5Chunks];
static int gS5ChunkN;
static int gS5Cur;
static uint32_t gS5ReadPos;
static int gS5BeginOk, gS5EndOk;
static int gS5NextLeft;
static int gS5Dispatch[8];
static int gS5DispatchN;

static inline void s5AddChunk(uint32_t id, const void *data, uint32_t size)
{
    gS5Chunk[gS5ChunkN].id = id;
    gS5Chunk[gS5ChunkN].size = size;
    gS5Chunk[gS5ChunkN].data = data;
    gS5ChunkN++;
}

S5_FN int BMX_CDECL s5IffBegin(void *iff)
{
    s5Log(0x49b130);
    s5LogPtr(iff);
    return gS5BeginOk;
}

S5_FN int BMX_CDECL s5IffEnd(void *iff)
{
    s5Log(0x49b250);
    s5LogPtr(iff);
    return gS5EndOk;
}

S5_FN int BMX_CDECL s5IffFind(void *iff, uint32_t id)
{
    s5Log(0x49ad90);
    s5LogPtr(iff);
    s5Log(id);
    for (int i = 0; i < gS5ChunkN; i++) {
        if (gS5Chunk[i].id == id) {
            gS5Cur = i;
            gS5ReadPos = 0;
            return 1;
        }
    }
    return 0;
}

S5_FN int BMX_CDECL s5IffRead(void *iff, void *dst, uint32_t size)
{
    uint8_t *d = dst;

    s5Log(0x49ac30);
    s5LogPtr(iff);
    s5Log(size);
    for (uint32_t i = 0; i < size; i++) {
        d[i] = gS5Cur >= 0 && gS5ReadPos < gS5Chunk[gS5Cur].size ? gS5Chunk[gS5Cur].data[gS5ReadPos] : 0;
        gS5ReadPos++;
    }
    return (int)size;
}

S5_FN uint32_t BMX_CDECL s5IffSize(void *iff)
{
    s5Log(0x49abc0);
    s5LogPtr(iff);
    return gS5Cur >= 0 ? gS5Chunk[gS5Cur].size : 0;
}

S5_FN int BMX_CDECL s5IffNext(void *iff, uint32_t id)
{
    s5Log(0x49aed0);
    s5LogPtr(iff);
    s5Log(id);
    if (gS5NextLeft > 0) {
        gS5NextLeft--;
        return 1;
    }
    return 0;
}

S5_FN int BMX_CDECL s5IffDispatch(void *iff, void *arg)
{
    s5Log(0x49b2a0);
    s5LogPtr(iff);
    s5LogPtr(arg);
    return gS5DispatchN > 0 ? gS5Dispatch[--gS5DispatchN] : 1;
}

/* Starts a script: every chunk function is stubbed; add chunks with s5AddChunk afterwards. */
static inline void s5IffBeginScript(int beginOk, int endOk)
{
    gS5ChunkN = 0;
    gS5Cur = -1;
    gS5ReadPos = 0;
    gS5BeginOk = beginOk;
    gS5EndOk = endOk;
    gS5NextLeft = 0;
    gS5DispatchN = 0;
    dt_stub(0x0049b130, NULL, (void *)s5IffBegin);
    dt_stub(0x0049b250, NULL, (void *)s5IffEnd);
    dt_stub(0x0049ad90, NULL, (void *)s5IffFind);
    dt_stub(0x0049ac30, NULL, (void *)s5IffRead);
    dt_stub(0x0049abc0, NULL, (void *)s5IffSize);
    dt_stub(0x0049aed0, NULL, (void *)s5IffNext);
    dt_stub(0x0049b2a0, NULL, (void *)s5IffDispatch);
}

/* ---- generic logging stubs -------------------------------------------------------------------------- */

/*
 * A stub of arity N logs its address and its arguments (through dt_canon) and returns the value the test set
 * for that address (0 by default). Stubs read exactly as many arguments as the callee takes.
 */

static inline void s5SetRet(uint32_t addr, uint32_t value)
{
    for (int i = 0; i < gS5RetN; i++) {
        if (gS5RetAddr[i] == addr) {
            gS5RetVal[i] = value;
            return;
        }
    }
    gS5RetAddr[gS5RetN] = addr;
    gS5RetVal[gS5RetN] = value;
    gS5RetN++;
}

static inline uint32_t s5Ret(uint32_t addr)
{
    for (int i = 0; i < gS5RetN; i++) {
        if (gS5RetAddr[i] == addr) {
            return gS5RetVal[i];
        }
    }
    return 0;
}

#define S5_STUB0(name, addr)                                                                                 \
    S5_FN uint32_t BMX_CDECL name(void)                                                                      \
    {                                                                                                        \
        s5Log(addr);                                                                                         \
        return s5Ret(addr);                                                                                  \
    }
#define S5_STUB1(name, addr)                                                                                 \
    S5_FN uint32_t BMX_CDECL name(uint32_t a)                                                                \
    {                                                                                                        \
        s5Log(addr);                                                                                         \
        s5Log(dt_canon(a));                                                                                  \
        return s5Ret(addr);                                                                                  \
    }
#define S5_STUB2(name, addr)                                                                                 \
    S5_FN uint32_t BMX_CDECL name(uint32_t a, uint32_t b)                                                    \
    {                                                                                                        \
        s5Log(addr);                                                                                         \
        s5Log(dt_canon(a));                                                                                  \
        s5Log(dt_canon(b));                                                                                  \
        return s5Ret(addr);                                                                                  \
    }
#define S5_STUB3(name, addr)                                                                                 \
    S5_FN uint32_t BMX_CDECL name(uint32_t a, uint32_t b, uint32_t c)                                        \
    {                                                                                                        \
        s5Log(addr);                                                                                         \
        s5Log(dt_canon(a));                                                                                  \
        s5Log(dt_canon(b));                                                                                  \
        s5Log(dt_canon(c));                                                                                  \
        return s5Ret(addr);                                                                                  \
    }
#define S5_STUB4(name, addr)                                                                                 \
    S5_FN uint32_t BMX_CDECL name(uint32_t a, uint32_t b, uint32_t c, uint32_t d)                            \
    {                                                                                                        \
        s5Log(addr);                                                                                         \
        s5Log(dt_canon(a));                                                                                  \
        s5Log(dt_canon(b));                                                                                  \
        s5Log(dt_canon(c));                                                                                  \
        s5Log(dt_canon(d));                                                                                  \
        return s5Ret(addr);                                                                                  \
    }
#define S5_STUB5(name, addr)                                                                                 \
    S5_FN uint32_t BMX_CDECL name(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)                \
    {                                                                                                        \
        s5Log(addr);                                                                                         \
        s5Log(dt_canon(a));                                                                                  \
        s5Log(dt_canon(b));                                                                                  \
        s5Log(dt_canon(c));                                                                                  \
        s5Log(dt_canon(d));                                                                                  \
        s5Log(dt_canon(e));                                                                                  \
        return s5Ret(addr);                                                                                  \
    }

/* Installs a stub on the original function at addr and on our port of it. */
#define S5_ON(addr, fn) dt_stub(addr, NULL, (void *)(fn))

/*
 * Zeroes the stack below the caller. Run at the end of a setup (whose real work was done by a deeper
 * function) it leaves the memory a function under test later uses as locals zero, so a read of an
 * uninitialised local of the original sees the same zero as ours.
 */
__attribute__((noinline, unused)) static void s5ClearStack(void)
{
    volatile uint8_t pad[0x1000];

    for (uint32_t i = 0; i < sizeof pad; i++) {
        pad[i] = 0;
    }
}

/* Original functions used to build fixtures independently of the port. */
#define S5_LIST_CREATE DT_ORIG(0x0048c260, void *(*)(int))
#define S5_LIST_INSERT DT_ORIG(0x0048c460, void (*)(void *, void *))
#define S5_POOL_CREATE DT_ORIG(0x00485e80, void *(*)(uint32_t, uint32_t, void *))
#define S5_POOL_ALLOC  DT_ORIG(0x00486050, void *(*)(void *))

#endif
