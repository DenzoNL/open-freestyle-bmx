/*
 * Bridges to the original NgBMX.exe during the hook phase.
 *
 * ORIG_FN declares a function that has not been ported yet: in the hook build it is a constant
 * pointer to the original code; in a native build it is a plain prototype, so anything still
 * unported fails to link. ORIG_VAR does the same for globals that still live in the exe's .data.
 *
 * Every ported function is annotated with BMX_ADDR(0x...) so tools/progress.py can track coverage.
 */
#ifndef BMX_COMPAT_ORIG_H
#define BMX_COMPAT_ORIG_H

#include <stdint.h>

#define BMX_ADDR(addr) /* original address: addr */

/*
 * Function pointer that ported code stores into exe-owned data. In the hook build it is the original
 * address (so old and new code see identical bytes and the in-game hook redirects the call); natively
 * it is the function itself.
 */
#if defined(BMX_HOOK)
#define BMX_FNPTR(type, addr, fn) ((type)(uintptr_t)(addr))
#else
#define BMX_FNPTR(type, addr, fn) ((type)(fn))
#endif

/* Macro arguments are types and parameter lists, which cannot be parenthesised. */
// NOLINTBEGIN(bugprone-macro-parentheses)
#if defined(BMX_HOOK)
#define BMX_CDECL   __cdecl
#define BMX_STDCALL __stdcall
#define ORIG_FN(addr, ret, name, params)                                                                     \
    static ret(BMX_CDECL *const name) params = (ret(BMX_CDECL *) params)(addr)
#define ORIG_FN_STDCALL(addr, ret, name, params)                                                             \
    static ret(BMX_STDCALL *const name) params = (ret(BMX_STDCALL *) params)(addr)
#define ORIG_VAR(addr, type, name) (*(type *)(uintptr_t)(addr))
#else
#define BMX_CDECL
#define BMX_STDCALL
#define ORIG_FN(addr, ret, name, params)         ret name params
#define ORIG_FN_STDCALL(addr, ret, name, params) ret name params
#define ORIG_VAR(addr, type, name)               name
#endif
// NOLINTEND(bugprone-macro-parentheses)

#endif
