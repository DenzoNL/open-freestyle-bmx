/*
 * C runtime layer of bmxnative.exe. The original NgBMX.exe links MSVC6's C runtime statically, and none of
 * its code is loaded here, so every CRT entry the game or RenderWare reaches has an implementation in this
 * file (or in crtasm.S for the compiler helpers with special calling conventions). The table below lists each
 * original address and its replacement: tools/gen_origmap.py reads it, so ORIG_FN declarations of those
 * addresses call these functions and the CRT pointers in the original's data (RenderWare's string and file
 * tables) are relocated to them. The functions are global for that reason.
 *
 * Where the game can observe a difference from the host's C library the MSVC6 behaviour is reproduced:
 * string compares return -1/0/1, qsort is MSVC's algorithm (not stable, so the order of equal elements
 * matters), printf-family floats use 17 significant digits, round half up and 3-digit exponents,
 * realloc(p, 0) frees. Everything else (heap, stdio, strtoul, sscanf, pow, ceil) is the host's. The game
 * only uses the FILE pointers it is given back, never their contents, so FILE is the host's.
 * See docs/NATIVE.md.
 */
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include "native/native.h"
#include "platform/platform.h"

/* ------------------------------------------------------------------------------------------------ */
/* Memory                                                                                            */
/* ------------------------------------------------------------------------------------------------ */

void *crtMalloc(size_t size)
{
    /* MSVC6 malloc(0) returns a valid block. */
    return malloc(size ? size : 1);
}

void crtFree(void *p)
{
    free(p);
}

void *crtRealloc(void *p, size_t size)
{
    if (!p) {
        return crtMalloc(size);
    }
    if (size == 0) {
        free(p); /* MSVC6: realloc(p, 0) frees and returns NULL */
        return NULL;
    }
    return realloc(p, size);
}

/* ------------------------------------------------------------------------------------------------ */
/* Strings                                                                                           */
/* ------------------------------------------------------------------------------------------------ */

static int sign3(int d)
{
    return (d > 0) - (d < 0);
}

size_t crtStrlen(const char *s)
{
    return strlen(s);
}

char *crtStrcpy(char *dst, const char *src)
{
    return strcpy(dst, src);
}

char *crtStrcat(char *dst, const char *src)
{
    return strcat(dst, src);
}

char *crtStrncpy(char *dst, const char *src, size_t n)
{
    return strncpy(dst, src, n); /* pads with NULs like MSVC */
}

char *crtStrncat(char *dst, const char *src, size_t n)
{
    return strncat(dst, src, n);
}

int crtStrcmp(const char *a, const char *b)
{
    for (; *a && *a == *b; a++, b++) {
    }
    return sign3((unsigned char)*a - (unsigned char)*b);
}

int crtStrncmp(const char *a, const char *b, size_t n)
{
    for (; n; n--, a++, b++) {
        if (*a != *b || !*a) {
            return sign3((unsigned char)*a - (unsigned char)*b);
        }
    }
    return 0;
}

static int lowerAscii(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + 0x20 : c;
}

/* _strcmpi / _stricmp (0x51d390): compares lower-cased bytes, result -1/0/1. */
int crtStrcmpi(const char *a, const char *b)
{
    for (;; a++, b++) {
        const int x = lowerAscii((unsigned char)*a);
        const int y = lowerAscii((unsigned char)*b);

        if (x != y) {
            return sign3(x - y);
        }
        if (!x) {
            return 0;
        }
    }
}

char *crtStrupr(char *s)
{
    for (char *p = s; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = (char)(*p - 0x20);
        }
    }
    return s;
}

char *crtStrchr(const char *s, int c)
{
    return strchr(s, c);
}

char *crtStrstr(const char *s, const char *sub)
{
    return strstr(s, sub);
}

char *crtStrtok(char *s, const char *delim)
{
    return strtok(s, delim);
}

int crtToupper(int c)
{
    return (c >= 'a' && c <= 'z') ? c - 0x20 : c;
}

/* RenderWare's own string helpers (0x4ee1d0..0x4ee2d0, not CRT but the engine's string table points there).
 */
int rwStricmp(const char *a, const char *b)
{
    if (!a || !b) {
        return 0;
    }
    for (;; a++, b++) {
        const int x = lowerAscii((signed char)*a);
        const int y = lowerAscii((signed char)*b);

        if (x != y) {
            return x - y; /* a plain difference, not -1/0/1 */
        }
        if (!x) {
            return 0;
        }
    }
}

char *rwStrupr(char *s)
{
    if (s) {
        crtStrupr(s);
    }
    return s;
}

char *rwStrlwr(char *s)
{
    if (s) {
        for (char *p = s; *p; p++) {
            *p = (char)lowerAscii((signed char)*p);
        }
    }
    return s;
}

char *rwStrrchr(const char *s, int c)
{
    const char *last = NULL;

    for (;; s++) {
        if (*s == (char)c) {
            last = s;
        }
        if (!*s) {
            return (char *)(uintptr_t)last;
        }
    }
}

int crtAtoi(const char *s)
{
    uint32_t v = 0;
    int neg = 0;

    while (*s == ' ' || (*s >= 9 && *s <= 13)) {
        s++;
    }
    if (*s == '-' || *s == '+') {
        neg = *s == '-';
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (uint32_t)(*s++ - '0'); /* wraps like the original, no saturation */
    }
    return (int)(neg ? 0u - v : v);
}

unsigned long crtStrtoul(const char *s, char **end, int base)
{
    return strtoul(s, end, base);
}

/* ------------------------------------------------------------------------------------------------ */
/* qsort: MSVC6's algorithm (0x4a4aa7), so that equal elements end up in the same order                */
/* ------------------------------------------------------------------------------------------------ */

typedef int (*CmpFn)(const void *, const void *);

static void swapBytes(char *a, char *b, size_t width)
{
    if (a != b) {
        while (width--) {
            const char t = *a;
            *a++ = *b;
            *b++ = t;
        }
    }
}

static void shortSort(char *lo, char *hi, size_t width, CmpFn cmp)
{
    while (hi > lo) {
        char *max = lo;

        for (char *p = lo + width; p <= hi; p += width) {
            if (cmp(p, max) > 0) {
                max = p;
            }
        }
        swapBytes(max, hi, width);
        hi -= width;
    }
}

void crtQsort(void *base, size_t num, size_t width, CmpFn cmp)
{
    char *lostk[30], *histk[30];
    int stk = 0;
    char *lo, *hi;

    if (num < 2 || width == 0) {
        return;
    }
    lo = base;
    hi = lo + width * (num - 1);
    for (;;) {
        const size_t size = (size_t)(hi - lo) / width + 1;

        if (size <= 8) {
            shortSort(lo, hi, width, cmp);
        } else {
            char *loguy, *higuy;

            swapBytes(lo + (size / 2) * width, lo, width); /* pivot goes to lo */
            loguy = lo;
            higuy = hi + width;
            for (;;) {
                do {
                    loguy += width;
                } while (loguy <= hi && cmp(loguy, lo) <= 0);
                do {
                    higuy -= width;
                } while (higuy > lo && cmp(higuy, lo) >= 0);
                if (higuy < loguy) {
                    break;
                }
                swapBytes(loguy, higuy, width);
            }
            swapBytes(lo, higuy, width);
            if (higuy - 1 - lo >= hi - loguy) {
                if (lo + width < higuy) {
                    lostk[stk] = lo;
                    histk[stk++] = higuy - width;
                }
                if (loguy < hi) {
                    lo = loguy;
                    continue;
                }
            } else {
                if (loguy < hi) {
                    lostk[stk] = loguy;
                    histk[stk++] = hi;
                }
                if (lo + width < higuy) {
                    hi = higuy - width;
                    continue;
                }
            }
        }
        if (--stk < 0) {
            return;
        }
        lo = lostk[stk];
        hi = histk[stk];
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* printf family with MSVC6 float formatting                                                         */
/* ------------------------------------------------------------------------------------------------ */

typedef struct {
    char *buf;
    size_t cap; /* characters that fit (without the terminator); counting continues beyond */
    size_t len;
} Out;

static void put(Out *o, char c)
{
    if (o->len < o->cap) {
        o->buf[o->len] = c;
    }
    o->len++;
}

enum { kLeft = 1, kPlus = 2, kSpace = 4, kAlt = 8, kZero = 16 };

/* Keeps n of the 17 digits, rounding half up on the next one. Returns 1 when the value rolled over to 10^k.
 */
static int roundDigits(char *d, int n)
{
    int up;

    if (n >= 17) {
        return 0;
    }
    if (n < 0) {
        memset(d, '0', 17);
        return 0;
    }
    up = d[n] >= '5';
    memset(d + n, '0', (size_t)(17 - n));
    if (!up) {
        return 0;
    }
    for (int i = n - 1; i >= 0; i--) {
        if (d[i] == '9') {
            d[i] = '0';
        } else {
            d[i]++;
            return 0;
        }
    }
    d[0] = '1';
    return 1;
}

static char digitAt(const char *d, int i)
{
    return (i >= 0 && i < 17) ? d[i] : '0';
}

/* d.ddd with prec decimals (exponent suffix not included). Returns the length. */
static int mantissaE(char *out, const char *d, int prec, int alt)
{
    int n = 0;

    out[n++] = d[0];
    if (prec > 0 || alt) {
        out[n++] = '.';
    }
    for (int i = 1; i <= prec; i++) {
        out[n++] = digitAt(d, i);
    }
    return n;
}

static int expSuffix(char *out, int exp, int upper)
{
    int n = 0;
    const int a = exp < 0 ? -exp : exp;

    out[n++] = upper ? 'E' : 'e';
    out[n++] = exp < 0 ? '-' : '+';
    out[n++] = (char)('0' + a / 100 % 10); /* MSVC6 always prints three digits */
    out[n++] = (char)('0' + a / 10 % 10);
    out[n++] = (char)('0' + a % 10);
    return n;
}

static int fixedF(char *out, const char *d, int exp, int prec, int alt)
{
    int n = 0;

    if (exp >= 0) {
        for (int i = 0; i <= exp; i++) {
            out[n++] = digitAt(d, i);
        }
    } else {
        out[n++] = '0';
    }
    if (prec > 0 || alt) {
        out[n++] = '.';
    }
    for (int k = 1; k <= prec; k++) {
        out[n++] = digitAt(d, exp + k);
    }
    return n;
}

/* Removes trailing fraction zeros (and a bare point) for %g without '#'. */
static int stripZeros(char *s, int n)
{
    int dot = 0;

    for (int i = 0; i < n; i++) {
        dot |= s[i] == '.';
    }
    if (dot) {
        while (n > 0 && s[n - 1] == '0') {
            n--;
        }
        if (n > 0 && s[n - 1] == '.') {
            n--;
        }
    }
    return n;
}

/* Digits of a finite double: 17 significant digits (like the original's conversion) and the decimal exponent.
 */
static void doubleDigits(double v, char d[17], int *exp)
{
    char tmp[48];
    char *e;

    if (v == 0.0) {
        memset(d, '0', 17);
        *exp = 0;
        return;
    }
    snprintf(tmp, sizeof tmp, "%.16e", fabs(v));
    d[0] = tmp[0];
    memcpy(d + 1, tmp + 2, 16);
    e = strchr(tmp, 'e');
    *exp = atoi(e + 1);
}

/* Formats |v| for conversion conv (e E f g G) into body; returns the length. body must hold 1100 bytes. */
static int formatFloatBody(char *body, double v, char conv, int prec, int alt, uint64_t bits, int neg)
{
    const int upper = conv == 'E' || conv == 'G';
    int n = 0;

    if (((bits >> 52) & 0x7ff) == 0x7ff) {
        /* MSVC6 prints 1.#INF, 1.#QNAN, 1.#IND (the x87 default NaN) and 1.#SNAN, padded with zeros. */
        const uint64_t mant = bits & 0xfffffffffffffull;
        const char *name = !mant              ? "#INF"
                           : (mant >> 51) & 1 ? ((neg && mant == 0x8000000000000ull) ? "#IND" : "#QNAN")
                                              : "#SNAN";
        int fill;

        body[n++] = '1';
        body[n++] = '.';
        for (; *name; name++) {
            body[n++] = *name;
        }
        fill = prec - (n - 2);
        while (fill-- > 0) {
            body[n++] = '0';
        }
        if (conv == 'e' || conv == 'E') {
            n += expSuffix(body + n, 0, upper);
        }
        return n;
    }
    {
        char d[17];
        int exp, carry;

        doubleDigits(v, d, &exp);
        if (conv == 'e' || conv == 'E') {
            carry = roundDigits(d, prec + 1);
            exp += carry;
            n = mantissaE(body, d, prec, alt);
            n += expSuffix(body + n, exp, upper);
        } else if (conv == 'f') {
            carry = roundDigits(d, exp + 1 + prec);
            exp += carry;
            n = fixedF(body, d, exp, prec, alt);
        } else {
            const int p = prec < 0 ? 6 : prec == 0 ? 1 : prec;

            carry = roundDigits(d, p);
            exp += carry;
            if (exp < -4 || exp >= p) {
                n = mantissaE(body, d, p - 1, alt);
                if (!alt) {
                    n = stripZeros(body, n);
                }
                n += expSuffix(body + n, exp, upper);
            } else {
                n = fixedF(body, d, exp, p - 1 - exp, alt);
                if (!alt) {
                    n = stripZeros(body, n);
                }
            }
        }
    }
    return n;
}

static void pad(Out *o, char c, int count)
{
    while (count-- > 0) {
        put(o, c);
    }
}

/* prefix (sign, 0x) + body, padded to width. zeroPad inserts zeros between prefix and body. */
static void emit(Out *o, const char *prefix, int prefixLen, const char *body, int bodyLen, int width,
                 int flags, int zeroPad)
{
    const int total = prefixLen + bodyLen;
    const int fill = width > total ? width - total : 0;

    if (!(flags & kLeft) && !zeroPad) {
        pad(o, ' ', fill);
    }
    for (int i = 0; i < prefixLen; i++) {
        put(o, prefix[i]);
    }
    if (zeroPad) {
        pad(o, '0', fill);
    }
    for (int i = 0; i < bodyLen; i++) {
        put(o, body[i]);
    }
    if (flags & kLeft) {
        pad(o, ' ', fill);
    }
}

static void formatCore(Out *o, const char *fmt, va_list ap)
{
    for (; *fmt; fmt++) {
        int flags = 0, width = 0, prec = -1, size = 0; /* size: 1 h, 2 l, 3 64-bit */
        char conv;

        if (*fmt != '%') {
            put(o, *fmt);
            continue;
        }
        fmt++;
        for (;; fmt++) {
            if (*fmt == '-') {
                flags |= kLeft;
            } else if (*fmt == '+') {
                flags |= kPlus;
            } else if (*fmt == ' ') {
                flags |= kSpace;
            } else if (*fmt == '#') {
                flags |= kAlt;
            } else if (*fmt == '0') {
                flags |= kZero;
            } else {
                break;
            }
        }
        if (*fmt == '*') {
            width = va_arg(ap, int);
            if (width < 0) {
                flags |= kLeft;
                width = -width;
            }
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt++ - '0');
            }
        }
        if (*fmt == '.') {
            fmt++;
            prec = 0;
            if (*fmt == '*') {
                prec = va_arg(ap, int);
                fmt++;
            } else {
                while (*fmt >= '0' && *fmt <= '9') {
                    prec = prec * 10 + (*fmt++ - '0');
                }
            }
        }
        for (;; fmt++) {
            if (*fmt == 'h') {
                size = 1;
            } else if (*fmt == 'l' || *fmt == 'L' || *fmt == 'w') {
                size = size == 2 ? 3 : 2; /* ll */
            } else if (fmt[0] == 'I' && fmt[1] == '6' && fmt[2] == '4') {
                size = 3;
                fmt += 2;
            } else if (fmt[0] == 'I' && fmt[1] == '3' && fmt[2] == '2') {
                size = 2;
                fmt += 2;
            } else {
                break;
            }
        }
        conv = *fmt;
        if (!conv) {
            break;
        }
        switch (conv) {
        case 'd':
        case 'i':
        case 'u':
        case 'o':
        case 'x':
        case 'X':
        case 'p': {
            char digits[72];
            char prefix[3];
            int nd = 0, np = 0, zeroPad;
            uint64_t v;
            int neg = 0;
            const int isSigned = conv == 'd' || conv == 'i';
            const unsigned radix = conv == 'o' ? 8 : (conv == 'u' || isSigned) ? 10 : 16;
            const char *alpha = (conv == 'X' || conv == 'p') ? "0123456789ABCDEF" : "0123456789abcdef";

            if (conv == 'p') {
                prec = 8;
                v = (uint32_t)(uintptr_t)va_arg(ap, void *);
            } else if (size == 3) {
                v = va_arg(ap, uint64_t);
            } else if (isSigned) {
                const int s = va_arg(ap, int);
                v = (uint64_t)(int64_t)(size == 1 ? (short)s : s);
            } else {
                const unsigned u = va_arg(ap, unsigned);
                v = size == 1 ? (unsigned short)u : u;
            }
            if (isSigned && (int64_t)v < 0) {
                neg = 1;
                v = 0 - v;
            }
            if (prec >= 0) {
                flags &= ~kZero;
            }
            if (prec < 0) {
                prec = 1;
            }
            if (v != 0 || prec != 0) {
                do {
                    digits[nd++] = alpha[v % radix];
                    v /= radix;
                } while (v);
            }
            while (nd < prec && nd < 70) {
                digits[nd++] = '0';
            }
            if (conv == 'o' && (flags & kAlt) && (nd == 0 || digits[nd - 1] != '0')) {
                digits[nd++] = '0';
            }
            for (int i = 0; i < nd / 2; i++) {
                const char t = digits[i];
                digits[i] = digits[nd - 1 - i];
                digits[nd - 1 - i] = t;
            }
            if (isSigned) {
                if (neg) {
                    prefix[np++] = '-';
                } else if (flags & kPlus) {
                    prefix[np++] = '+';
                } else if (flags & kSpace) {
                    prefix[np++] = ' ';
                }
            } else if ((conv == 'x' || conv == 'X') && (flags & kAlt) && nd > 0 &&
                       !(nd == 1 && digits[0] == '0')) {
                prefix[np++] = '0';
                prefix[np++] = conv;
            }
            zeroPad = (flags & kZero) && !(flags & kLeft);
            emit(o, prefix, np, digits, nd, width, flags, zeroPad);
            break;
        }
        case 'c':
        case 'C': {
            const char c = (char)va_arg(ap, int);
            emit(o, "", 0, &c, 1, width, flags, 0);
            break;
        }
        case 's':
        case 'S': {
            const char *s = va_arg(ap, const char *);
            int len = 0;

            if (!s) {
                s = "(null)";
            }
            while (s[len] && (prec < 0 || len < prec)) {
                len++;
            }
            emit(o, "", 0, s, len, width, flags, (flags & kZero) && !(flags & kLeft));
            break;
        }
        case 'n':
            *va_arg(ap, int *) = (int)o->len;
            break;
        case 'e':
        case 'E':
        case 'f':
        case 'g':
        case 'G': {
            static char body[1100]; /* single threaded: the engine is */
            char prefix[1];
            int np = 0, nb;
            double v = va_arg(ap, double);
            uint64_t bits;
            int neg;
            int special;

            memcpy(&bits, &v, 8);
            neg = (int)(bits >> 63);
            special = ((bits >> 52) & 0x7ff) == 0x7ff;
            if (prec < 0 && conv != 'g' && conv != 'G') {
                prec = 6; /* %g applies its default in formatFloatBody */
            }
            if (prec > 500) {
                prec = 500;
            }
            nb = formatFloatBody(body, v, conv, prec, (flags & kAlt) != 0, bits, neg);
            if (neg) {
                prefix[np++] = '-';
            } else if (flags & kPlus) {
                prefix[np++] = '+';
            } else if (flags & kSpace) {
                prefix[np++] = ' ';
            }
            emit(o, prefix, np, body, nb, width, flags, (flags & kZero) && !(flags & kLeft) && !special);
            break;
        }
        case '%':
            put(o, '%');
            break;
        default:
            put(o, conv);
            break;
        }
    }
}

static int formatTo(char *dst, size_t cap, const char *fmt, va_list ap)
{
    Out o;

    o.buf = dst;
    o.cap = cap;
    o.len = 0;
    formatCore(&o, fmt, ap);
    if (dst) {
        dst[o.len < cap ? o.len : cap] = 0; /* callers pass room for the terminator */
    }
    return (int)o.len;
}

int crtVsprintf(char *dst, const char *fmt, va_list ap)
{
    return formatTo(dst, (size_t)0x7fffffff, fmt, ap);
}

int crtSprintf(char *dst, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = crtVsprintf(dst, fmt, ap);
    va_end(ap);
    return n;
}

static int vfprintfMsvc(FILE *f, const char *fmt, va_list ap)
{
    va_list ap2;
    int n;
    char *buf;

    va_copy(ap2, ap);
    n = formatTo(NULL, 0, fmt, ap2);
    va_end(ap2);
    buf = malloc((size_t)n + 1);
    if (!buf) {
        return -1;
    }
    formatTo(buf, (size_t)n, fmt, ap);
    fwrite(buf, 1, (size_t)n, f);
    free(buf);
    return n;
}

int crtPrintf(const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vfprintfMsvc(stdout, fmt, ap);
    va_end(ap);
    return n;
}

int crtFprintf(FILE *f, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vfprintfMsvc(f, fmt, ap);
    va_end(ap);
    return n;
}

int crtSscanf(const char *s, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsscanf(s, fmt, ap);
    va_end(ap);
    return n;
}

/* ------------------------------------------------------------------------------------------------ */
/* stdio (the FILE is the host's: the engine only passes it back)                                    */
/* ------------------------------------------------------------------------------------------------ */

FILE *crtFopen(const char *name, const char *mode)
{
    return fopen(bmxPathResolve(name), mode);
}

int crtFclose(FILE *f)
{
    return fclose(f);
}

size_t crtFread(void *buf, size_t size, size_t count, FILE *f)
{
    return fread(buf, size, count, f);
}

size_t crtFwrite(const void *buf, size_t size, size_t count, FILE *f)
{
    return fwrite(buf, size, count, f);
}

char *crtFgets(char *buf, int n, FILE *f)
{
    return fgets(buf, n, f);
}

int crtFputs(const char *s, FILE *f)
{
    return fputs(s, f) >= 0 ? 0 : -1; /* the original returns 0 or -1 */
}

int crtFeof(FILE *f)
{
    return feof(f);
}

int crtFseek(FILE *f, long offset, int whence)
{
    return fseek(f, offset, whence);
}

long crtFtell(FILE *f)
{
    return ftell(f);
}

int crtFflush(FILE *f)
{
    return fflush(f);
}

/* RenderWare's rwfexist (0x4e8290): opens the file through the engine's own file table and closes it again.
 */
int rwFexist(const char *name)
{
    typedef void *(*OpenFn)(const char *, const char *);
    typedef int (*CloseFn)(void *);
    const uint8_t *engine = *(const uint8_t *const *)(uintptr_t)0x0067696c;
    void *file = ((OpenFn) * (void *const *)(engine + 0xc4))(name, (const char *)(uintptr_t)0x0069bdb0);

    if (!file) {
        return 0;
    }
    ((CloseFn) * (void *const *)(engine + 0xc8))(file);
    return 1;
}

int crtFinite(double v)
{
    return isfinite(v);
}

double crtCeil(double v)
{
    return ceil(v);
}

double crtPow(double base, double exponent)
{
    return pow(base, exponent);
}

/* ------------------------------------------------------------------------------------------------ */
/* Process                                                                                           */
/* ------------------------------------------------------------------------------------------------ */

void crtAssert(const char *expr, const char *file, unsigned line)
{
    bmxLog("Assertion failed: %s, file %s, line %u\n", expr, file, line);
    fflush(NULL);
    ExitProcess(3);
}

void crtExit(int code)
{
    fflush(NULL);
    ExitProcess((UINT)code);
}

/* ------------------------------------------------------------------------------------------------ */
/* The table                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

extern void bmxCrtFtol(void), bmxCrtCIsqrt(void), bmxCrtChkstk(void), bmxCrtAllmul(void), bmxCrtAulldiv(void),
    bmxCrtAullrem(void), bmxCrtAllshl(void), bmxCrtSetjmp3(void), bmxCrtLongjmp(void);

typedef struct {
    uint32_t orig;
    const void *impl;
    const char *name;
} CrtEntry;

#define E(addr, fn, name) {addr, (const void *)(fn), name}

const CrtEntry bmxCrtTable[] = {
    /* compiler helpers (assembly) */
    E(0x004a4a80, bmxCrtFtol, "_ftol"),
    E(0x004a5390, bmxCrtCIsqrt, "_CIsqrt"),
    E(0x004a4d50, bmxCrtChkstk, "_chkstk"),
    E(0x004ab3b0, bmxCrtAllmul, "__allmul"),
    E(0x004ad3f0, bmxCrtAulldiv, "__aulldiv"),
    E(0x004ad460, bmxCrtAullrem, "__aullrem"),
    E(0x004ae8d0, bmxCrtAllshl, "__allshl"),
    E(0x0051e008, bmxCrtSetjmp3, "_setjmp3"),
    E(0x0051e084, bmxCrtLongjmp, "_longjmp"),
    /* heap */
    E(0x004a50d2, crtMalloc, "malloc"),
    E(0x004a4d7f, crtFree, "free"),
    E(0x0051d67e, crtRealloc, "realloc"),
    /* strings, ctype, conversion */
    E(0x004a8c20, crtStrlen, "strlen"),
    E(0x004a8ca0, crtStrcpy, "strcpy"),
    E(0x004a8cb0, crtStrcat, "strcat"),
    E(0x004a5600, crtStrncpy, "strncpy"),
    E(0x004a8af0, crtStrncat, "strncat"),
    E(0x0051ddf0, crtStrcmp, "strcmp"),
    E(0x004a5190, crtStrncmp, "strncmp"),
    E(0x0051d390, crtStrcmpi, "_strcmpi"),
    E(0x0051d2f0, crtStrupr, "_strupr"),
    E(0x004a4c90, crtStrchr, "strchr"),
    E(0x004ad060, crtStrstr, "strstr"),
    E(0x0051de74, crtStrtok, "strtok"),
    E(0x004a593c, crtToupper, "toupper"),
    E(0x004a5bb3, crtAtoi, "atoi"),
    E(0x004a56fe, crtStrtoul, "strtoul"),
    E(0x004a4aa7, crtQsort, "qsort"),
    E(0x004a60d9, crtFinite, "_finite"),
    E(0x004a552a, crtCeil, "ceil"),
    E(0x0051d939, crtPow, "pow"),
    /* formatted I/O */
    E(0x004a5338, crtVsprintf, "vsprintf"),
    E(0x0051dd9c, crtSprintf, "sprintf"),
    E(0x004a51c8, crtPrintf, "printf"),
    E(0x004a847d, crtFprintf, "fprintf"),
    E(0x004a6391, crtSscanf, "sscanf"),
    /* stdio */
    E(0x004a5c98, crtFopen, "rwfopen"),
    E(0x004a5cab, crtFclose, "rwfclose"),
    E(0x0051db3d, crtFread, "rwfread"),
    E(0x0051dc25, crtFwrite, "rwfwrite"),
    E(0x0051dd2f, crtFgets, "rwfgets"),
    E(0x0051d4d2, crtFputs, "rwfputs"),
    E(0x0051dd86, crtFeof, "rwfeof"),
    E(0x004a61ad, crtFseek, "rwfseek"),
    E(0x004a6239, crtFtell, "rwftell"),
    E(0x004a8370, crtFflush, "fflush"),
    /* process */
    E(0x004a4de8, crtAssert, "_assert"),
    E(0x004a5a46, crtExit, "_exit"),
    /* RenderWare's own helpers that the engine's string and file tables point at */
    E(0x004ee1d0, rwStricmp, "rwstricmp"),
    E(0x004ee230, rwStrupr, "rwstrupr"),
    E(0x004ee260, rwStrlwr, "rwstrlwr"),
    E(0x004ee290, crtStrchr, "rwstrchr"),
    E(0x004ee2d0, rwStrrchr, "rwstrrchr"),
    E(0x004e8290, rwFexist, "rwfexist"),
};
