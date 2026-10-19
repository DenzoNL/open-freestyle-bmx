/*
 * C runtime surface used by ported code.
 *
 * Hook phase: stateless routines (mem*, str*) are our own implementations; stateful ones (heap,
 * rand, stdio) forward to the exe's MSVC6 CRT so old and new code share one heap and one RNG.
 */
#ifndef BMX_COMPAT_CRT_H
#define BMX_COMPAT_CRT_H

#include <stdarg.h>
#include <stddef.h>

#if defined(BMX_HOOK)
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
char *strchr(const char *s, int c);
int toupper(int c);
int tolower(int c);

#include "compat/orig.h"
ORIG_FN(0x004a50d2, void *, malloc, (size_t n));
/* Locale- or format-dependent CRT routines stay in the exe. */
ORIG_FN(0x004a5338, int, vsprintf, (char *dst, const char *fmt, va_list args));
ORIG_FN(0x004a56fe, unsigned long, strtoul, (const char *s, char **end, int base));
ORIG_FN(0x0051d2f0, char *, _strupr, (char *s));
ORIG_FN(0x0051d390, int, _stricmp, (const char *a, const char *b));
ORIG_FN(0x004a4aa7, void, qsort, (void *base, size_t n, size_t size, int (*cmp)(const void *, const void *)));
#else
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define _stricmp strcasecmp
static inline char *_strupr(char *s)
{
    for (char *p = s; *p; p++) {
        *p = (char)toupper((unsigned char)*p);
    }
    return s;
}
#endif

#endif
