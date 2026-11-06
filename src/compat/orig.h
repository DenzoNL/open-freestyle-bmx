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
 *   is loaded; the original's globals are C definitions of ours (src/data). ORIG_VAR and BMX_ORIG name the
 *   definition at that address (BMX_DATA_<addr>, src/data/addrs.h, plus the offset in it); ORIG_FN calls the
 *   function serving that address (the generated origmap.h, tools/gen_origmap.py); BMX_FNPTR is the function
 *   itself. BMX_HOOK stays defined there too, for what it says about the target (i686 MSVC-style
 *   conventions, x87 asm, assembler names of stdcall functions, the compat/crt.h shims); only the address
 *   machinery is BMX_IMAGE's. (BMX_DATA_PROBE: tools/gen_data.py's type probes, which compile the code before
 *   src/data exists.)
 * - neither (host builds without the exe): globals and functions by name.
 */
#ifndef BMX_COMPAT_ORIG_H
#define BMX_COMPAT_ORIG_H

#include <stdint.h>

#include "compat/layout.h"

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
void *bmxOrigPtr(uint32_t addr);
#if defined(BMX_DATA_PROBE)
#define BMX_DATA_AT(addr) ((char *)(uintptr_t)(addr))
#else
#include "data/addrs.h"
/* The address must be written as a number (the macro's name is made from it), as in the code. */
#define BMX_DATA_AT(addr) BMX_DATA_##addr
#endif
#define BMX_ORIG(addr)     ((uintptr_t)BMX_DATA_AT(addr))
#define BMX_ORIG_PTR(addr) ((uintptr_t)bmxOrigPtr((uint32_t)(addr)))
#include "origmap.h"
#else
#define BMX_ORIG(addr)     ((uintptr_t)(addr))
#define BMX_ORIG_PTR(addr) ((uintptr_t)(addr))
#endif

/*
 * The body of a naked entry that takes the original's own register convention (EAX / EDX / ECX, ST0) and
 * calls the C implementation: i386 only. Only the original's code and the oracle call such an entry; on
 * x86_64 nothing does (our callers call the implementation), and the entry is a trap (docs/PORTING.md X3).
 */
#if defined(__i386__)
#define BMX_X86_ENTRY(code) __asm__ volatile(code)
#elif defined(__x86_64__)
#define BMX_X86_ENTRY(code) __asm__ volatile("ud2")
#else
#define BMX_X86_ENTRY(code) __asm__ volatile("brk #0x1")
#endif

/*
 * Opt-in replacements of our own functions (src/native/loader.c: smooth mixer, wide HUD, draw distance). On
 * Windows x86 the loader writes a jump over the function's entry. Elsewhere code is not written at run time
 * (macOS arm64 signs every code page): such a function starts with BMX_REDIRECT_VOID, a call through the slot
 * BMX_REDIRECT_SLOT defines, which the replacement's provider fills. Void functions only (all three are).
 */
#if defined(BMX_IMAGE) && !(defined(_WIN32) && (defined(__i386__) || defined(__x86_64__)))
#define BMX_REDIRECTS                   1
#define BMX_REDIRECT_EXTERN(fn, params) extern void(*bmxRedirect_##fn) params
#define BMX_REDIRECT_SLOT(fn, params)   void(*bmxRedirect_##fn) params
#define BMX_REDIRECT_SET(fn, impl)      (bmxRedirect_##fn = (impl))
#define BMX_REDIRECT_VOID(fn, args)                                                                          \
    do {                                                                                                     \
        if (bmxRedirect_##fn != 0) {                                                                         \
            bmxRedirect_##fn args;                                                                           \
            return;                                                                                          \
        }                                                                                                    \
    } while (0)
#else
#define BMX_REDIRECTS                   0
#define BMX_REDIRECT_EXTERN(fn, params) _Static_assert(1, #fn)
#define BMX_REDIRECT_SLOT(fn, params)   _Static_assert(1, #fn)
#define BMX_REDIRECT_SET(fn, impl)      ((void)0)
#define BMX_REDIRECT_VOID(fn, args)     ((void)0)
#endif

/*
 * Keeps a floating-point value opaque to the optimiser (no folding of a negation into the next operation):
 * an empty asm that claims to change the register holding it (an SSE register on x86, an FP / SIMD one on
 * arm64).
 */
#if defined(__i386__) || defined(__x86_64__)
#define BMX_FP_OPAQUE(v) __asm__("" : "+x"(v))
#else
#define BMX_FP_OPAQUE(v) __asm__("" : "+w"(v))
#endif

/*
 * The original's calling conventions: regparm(n) (the first n arguments in EAX, EDX, ECX), cdecl and stdcall.
 * They mean something on i386 only. The Windows targets accept the keywords on x86_64 too (and ignore them);
 * elsewhere (macOS / Linux arm64 and x86_64, docs/PORTING.md "X5 part 1") they are empty.
 */
#if defined(__i386__)
#define BMX_REGPARM(n) __attribute__((regparm(n)))
#else
#define BMX_REGPARM(n)
#endif
#if defined(__i386__) || defined(_WIN32)
#define BMX_CC_CDECL   __cdecl
#define BMX_CC_STDCALL __stdcall
#else
#define BMX_CC_CDECL
#define BMX_CC_STDCALL
#endif

/* Macro arguments are types and parameter lists, which cannot be parenthesised. */
// NOLINTBEGIN(bugprone-macro-parentheses)
#if defined(BMX_IMAGE)
#define BMX_CDECL   BMX_CC_CDECL
#define BMX_STDCALL BMX_CC_STDCALL
/* The generated BMX_ORIGFN_<addr> is the assembler name of the function serving addr (a port under another
 * name, the C runtime layer, the PNG reader, the driver, a trap or an import thunk stub). */
#define ORIG_FN(addr, ret, name, params)            ret BMX_CDECL name params __asm__(BMX_ORIGFN_##addr)
#define ORIG_FN_STDCALL(addr, ret, name, params)    ret BMX_STDCALL name params __asm__(BMX_ORIGFN_##addr)
#define ORIG_VAR(addr, type, name)                  (*(type *)BMX_DATA_AT(addr))
#define ORIG_FN_REGPARM(n, addr, ret, name, params) BMX_REGPARM(n) ret name params __asm__(BMX_ORIGFN_##addr)
#elif defined(BMX_HOOK)
#define BMX_CDECL   BMX_CC_CDECL
#define BMX_STDCALL BMX_CC_STDCALL
#define ORIG_FN(addr, ret, name, params)                                                                     \
    static ret(BMX_CDECL *const name) params = (ret(BMX_CDECL *) params)(addr)
#define ORIG_FN_STDCALL(addr, ret, name, params)                                                             \
    static ret(BMX_STDCALL *const name) params = (ret(BMX_STDCALL *) params)(addr)
#define ORIG_VAR(addr, type, name) (*(type *)(uintptr_t)(addr))
/* Original function taking its first n arguments in EAX, EDX, ECX (the rest on the stack, cdecl). */
#define ORIG_FN_REGPARM(n, addr, ret, name, params)                                                          \
    static ret(BMX_REGPARM(n) *const name) params = (ret(BMX_REGPARM(n) *) params)(addr)
#else
#define BMX_CDECL
#define BMX_STDCALL
#define ORIG_FN(addr, ret, name, params)            ret name params
#define ORIG_FN_STDCALL(addr, ret, name, params)    ret name params
#define ORIG_VAR(addr, type, name)                  name
#define ORIG_FN_REGPARM(n, addr, ret, name, params) BMX_REGPARM(n) ret name params
#endif
// NOLINTEND(bugprone-macro-parentheses)

#endif
