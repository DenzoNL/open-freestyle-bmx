/*
 * Bridges to the original NgBMX.exe during the hook phase.
 *
 * ORIG_FN declares a function that has not been ported yet: in the hook build it is a constant
 * pointer to the original code; in a native build it is a plain prototype, so anything still
 * unported fails to link. ORIG_VAR does the same for globals that still live in the exe's .data.
 *
 * Every ported function is annotated with BMX_ADDR(0x...) so tools/progress.py can track coverage.
 *
 * Three modes:
 * - BMX_HOOK (hook DLL, oracle test DLL): the original's code and data at their addresses; ORIG_FN and
 *   BMX_FNPTR are those addresses, ORIG_VAR reads the global there.
 * - BMX_IMAGE (bmxnative.exe, docs/NATIVE.md "The original's data in the source"): no original code or data
 *   is loaded; the original's data sections are C objects of ours with the original's layout (src/data, one
 *   per section). ORIG_VAR and BMX_ORIG give the place of an original address in its block (BMX_DATA_AT,
 *   src/data/blocks.h); ORIG_FN calls the function serving that address (the generated origmap.h,
 *   tools/gen_origmap.py); BMX_FNPTR is the function itself. BMX_HOOK stays defined there too, for what it
 *   says about the target (i686 MSVC-style conventions, x87 asm, assembler names of stdcall functions, the
 *   compat/crt.h shims); only the address machinery is BMX_IMAGE's.
 * - neither (host builds without the exe): globals and functions by name.
 */
#ifndef BMX_COMPAT_ORIG_H
#define BMX_COMPAT_ORIG_H

#include <stdint.h>

/*
 * In the hook build a ported function must stay a real call: test stubs and in-game hooks patch its
 * entry, which an inlined copy would bypass (clang 23 inlined gameBigGameFunction into gameStartMode).
 * `weak` stops interprocedural facts too: clang inferred zcollideFillPropEdges returns 0..4 and unrolled
 * its caller's loop for that range, so a stubbed -1 still listed 4 edges.
 */
#ifdef BMX_HOOK
#define BMX_ADDR(addr) __attribute__((noinline, weak)) /* original address: addr */
#else
#define BMX_ADDR(addr) /* original address: addr */
#endif

/*
 * Function pointer that ported code stores into exe-owned data. In the hook build it is the original
 * address (so old and new code see identical bytes and the in-game hook redirects the call); natively
 * it is the function itself.
 */
#if defined(BMX_HOOK) && !defined(BMX_IMAGE)
#define BMX_FNPTR(type, addr, fn) ((type)(uintptr_t)(addr))
#elif defined(BMX_IMAGE)
/* Through an integer, as the hook build's address is: the tables give a function of another type. */
#define BMX_FNPTR(type, addr, fn) ((type)(uintptr_t)(fn))
#else
#define BMX_FNPTR(type, addr, fn) ((type)(fn))
#endif

/*
 * BMX_ORIG(addr): where the original's data at `addr` is now, as a uintptr_t (an original data address
 * written as a number: a table, a string, a global used as an array base). BMX_ORIG_PTR(addr): the same for
 * an original code address (RenderWare callbacks): natively a lookup (bmxOrigPtr, src/native/loader.c) that
 * gives the function serving it.
 */
#if defined(BMX_IMAGE)
#include "data/blocks.h"
void *bmxOrigPtr(uint32_t addr);
#define BMX_ORIG(addr)     ((uintptr_t)BMX_DATA_AT(addr))
#define BMX_ORIG_PTR(addr) ((uintptr_t)bmxOrigPtr((uint32_t)(addr)))
#include "origmap.h"
#else
#define BMX_ORIG(addr)     ((uintptr_t)(addr))
#define BMX_ORIG_PTR(addr) ((uintptr_t)(addr))
#endif

/* Macro arguments are types and parameter lists, which cannot be parenthesised. */
// NOLINTBEGIN(bugprone-macro-parentheses)
#if defined(BMX_IMAGE)
#define BMX_CDECL   __cdecl
#define BMX_STDCALL __stdcall
/* The generated BMX_ORIGFN_<addr> is the assembler name of the function serving addr (a port under another
 * name, the C runtime layer, the PNG reader, the driver, a trap or an import thunk stub). */
#define ORIG_FN(addr, ret, name, params)         ret BMX_CDECL name params __asm__(BMX_ORIGFN_##addr)
#define ORIG_FN_STDCALL(addr, ret, name, params) ret BMX_STDCALL name params __asm__(BMX_ORIGFN_##addr)
#define ORIG_VAR(addr, type, name)               (*(type *)BMX_DATA_AT(addr))
#define ORIG_FN_REGPARM(n, addr, ret, name, params)                                                          \
    __attribute__((regparm(n))) ret name params __asm__(BMX_ORIGFN_##addr)
#elif defined(BMX_HOOK)
#define BMX_CDECL   __cdecl
#define BMX_STDCALL __stdcall
#define ORIG_FN(addr, ret, name, params)                                                                     \
    static ret(BMX_CDECL *const name) params = (ret(BMX_CDECL *) params)(addr)
#define ORIG_FN_STDCALL(addr, ret, name, params)                                                             \
    static ret(BMX_STDCALL *const name) params = (ret(BMX_STDCALL *) params)(addr)
#define ORIG_VAR(addr, type, name) (*(type *)(uintptr_t)(addr))
/* Original function taking its first n arguments in EAX, EDX, ECX (the rest on the stack, cdecl). */
#define ORIG_FN_REGPARM(n, addr, ret, name, params)                                                          \
    static ret(__attribute__((regparm(n))) *const name) params =                                             \
        (ret(__attribute__((regparm(n))) *) params)(addr)
#else
#define BMX_CDECL
#define BMX_STDCALL
#define ORIG_FN(addr, ret, name, params)            ret name params
#define ORIG_FN_STDCALL(addr, ret, name, params)    ret name params
#define ORIG_VAR(addr, type, name)                  name
#define ORIG_FN_REGPARM(n, addr, ret, name, params) __attribute__((regparm(n))) ret name params
#endif
// NOLINTEND(bugprone-macro-parentheses)

#endif
