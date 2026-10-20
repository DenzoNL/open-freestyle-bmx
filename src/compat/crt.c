#include "compat/crt.h"

#if defined(BMX_HOOK)
/* Referenced by any object that uses floating point under the MSVC ABI. */
int _fltused = 0;

/* rep movsb / rep stosb: same results as byte loops, but far cheaper under the emulator. */
void *memcpy(void *dst, const void *src, size_t n)
{
    void *d = dst;
    __asm__ volatile("rep movsb" : "+D"(d), "+S"(src), "+c"(n) : : "memory");
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        while (n--) {
            d[n] = s[n];
        }
    }
    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    void *d = dst;
    __asm__ volatile("rep stosb" : "+D"(d), "+c"(n) : "a"(c) : "memory");
    return dst;
}

/* The MSVC6 comparison routines return -1, 0 or 1 rather than the byte difference. */
static int sign(int d)
{
    return (d > 0) - (d < 0);
}

int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *x = a;
    const unsigned char *y = b;
    for (; n; n--, x++, y++) {
        if (*x != *y) {
            return sign(*x - *y);
        }
    }
    return 0;
}

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p) {
        p++;
    }
    return (size_t)(p - s);
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return sign((unsigned char)*a - (unsigned char)*b);
}

int strncmp(const char *a, const char *b, size_t n)
{
    for (; n; n--, a++, b++) {
        if (*a != *b || !*a) {
            return sign((unsigned char)*a - (unsigned char)*b);
        }
    }
    return 0;
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++)) {
    }
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t i = 0;
    for (; i < n && src[i]; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = '\0';
    }
    return dst;
}

char *strchr(const char *s, int c)
{
    for (;; s++) {
        if (*s == (char)c) {
            return (char *)s;
        }
        if (!*s) {
            return NULL;
        }
    }
}

/*
 * MSVC 64-bit arithmetic helpers (emitted by the compiler for 64-bit division, multiplication and
 * shifts). The exe contains the MSVC6 versions; forward to them so the calling convention (arguments on
 * the stack, callee pops) matches exactly.
 */
#define BMX_FORWARD(name, addr)                                                                              \
    __attribute__((naked)) void name(void)                                                                   \
    {                                                                                                        \
        __asm__ volatile("movl $" #addr ", %eax\n\tjmp *%eax");                                              \
    }
BMX_FORWARD(_allmul, 0x004ab3b0)
BMX_FORWARD(_aulldiv, 0x004ad3f0)
BMX_FORWARD(_aullrem, 0x004ad460)
BMX_FORWARD(_allshl, 0x004ae8d0)

/* The game runs in the "C" locale, so ASCII-only case mapping matches MSVC6. */
int toupper(int c)
{
    return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

int tolower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}
#endif
