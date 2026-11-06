/*
 * Portable x87 (docs/PORTING.md "X5 part 2 (portable x87)"). Integer arithmetic only: the result does not
 * depend on the host FPU's rounding mode, its NaN conventions or a libm.
 *
 * Registers (BmxX87Reg) are unpacked to sign / exponent / 64-bit significand; every operation computes the
 * exact result (or, for the transcendental instructions, a 128-bit approximation, ~2^-120 relative) and
 * rounds it once to the precision control's width with the rounding control's mode, in the x87's 15-bit
 * exponent range. Stores round the register value again, to float or double (with denormals): the double
 * rounding the original has.
 */
#include "compat/x87soft.h"

#if !defined(__i386__) && !defined(__x86_64__)
uint16_t bmxX87SoftCw = 0x027f;
#endif

enum { RC_NEAREST, RC_DOWN, RC_UP, RC_ZERO };

#define EXP_MAX  0x7fff
#define BIAS     16383
#define QUIET    0x4000000000000000ull
#define TOP      0x8000000000000000ull
#define SIGN     0x8000u
#define REG_ZERO ((BmxX87Reg){0, 0})

static const BmxX87Reg kIndefinite = {0xc000000000000000ull, 0xffff};
static const BmxX87Reg kOne = {TOP, BIAS};

static int gPrecisePi;

void bmxX87SoftSetReductionPi(int precise)
{
    gPrecisePi = precise;
}

/* ---- 64-bit helpers ---- */

static void mul64(uint64_t a, uint64_t b, uint64_t *hi, uint64_t *lo)
{
#if defined(__SIZEOF_INT128__)
    const unsigned __int128 p = (unsigned __int128)a * b;
    *hi = (uint64_t)(p >> 64);
    *lo = (uint64_t)p;
#else
    const uint64_t a0 = (uint32_t)a, a1 = a >> 32, b0 = (uint32_t)b, b1 = b >> 32;
    const uint64_t p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
    const uint64_t mid = (p00 >> 32) + (uint32_t)p01 + (uint32_t)p10;
    *lo = (mid << 32) | (uint32_t)p00;
    *hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
#endif
}

/* n / d with the quotient below 2^32 (no 64-bit division helper on the freestanding i386 builds). */
static uint32_t div64by32(uint64_t n, uint32_t d, uint32_t *rem)
{
#if defined(__i386__)
    uint32_t q, r;
    __asm__("divl %4" : "=a"(q), "=d"(r) : "a"((uint32_t)n), "d"((uint32_t)(n >> 32)), "rm"(d));
    *rem = r;
    return q;
#else
    *rem = (uint32_t)(n % d);
    return (uint32_t)(n / d);
#endif
}

static int clz64(uint64_t v)
{
    return __builtin_clzll(v);
}

/* ---- 256-bit integers (w[0] least significant) ---- */

typedef struct U256 {
    uint64_t w[4];
} U256;

static U256 u256Shl(U256 a, int s)
{
    U256 r = {{0, 0, 0, 0}};
    const int words = s / 64, bits = s % 64;
    for (int i = 3; i >= words; i--) {
        uint64_t v = a.w[i - words] << bits;
        if (bits != 0 && i - words - 1 >= 0) {
            v |= a.w[i - words - 1] >> (64 - bits);
        }
        r.w[i] = v;
    }
    return r;
}

static U256 u256Shr(U256 a, int s)
{
    U256 r = {{0, 0, 0, 0}};
    const int words = s / 64, bits = s % 64;
    for (int i = 0; i + words < 4; i++) {
        uint64_t v = a.w[i + words] >> bits;
        if (bits != 0 && i + words + 1 < 4) {
            v |= a.w[i + words + 1] << (64 - bits);
        }
        r.w[i] = v;
    }
    return r;
}

static int u256Cmp(const U256 *a, const U256 *b)
{
    for (int i = 3; i >= 0; i--) {
        if (a->w[i] != b->w[i]) {
            return a->w[i] < b->w[i] ? -1 : 1;
        }
    }
    return 0;
}

static U256 u256Sub(U256 a, U256 b)
{
    uint64_t borrow = 0;
    for (int i = 0; i < 4; i++) {
        const uint64_t d = a.w[i] - b.w[i];
        const uint64_t b1 = a.w[i] < b.w[i];
        a.w[i] = d - borrow;
        borrow = b1 | (d < borrow);
    }
    return a;
}

static int u256Top(const U256 *a)
{
    for (int i = 3; i >= 0; i--) {
        if (a->w[i] != 0) {
            return 64 * i + 63 - clz64(a->w[i]);
        }
    }
    return -1;
}

/* ---- the register format ---- */

typedef struct Unp {
    int neg;
    int32_t exp; /* biased; value = m * 2^(exp - BIAS - 63) */
    uint64_t m;
} Unp;

static int regIsNan(BmxX87Reg r)
{
    return (r.se & EXP_MAX) == EXP_MAX && (r.m << 1) != 0;
}

static int regIsInf(BmxX87Reg r)
{
    return (r.se & EXP_MAX) == EXP_MAX && (r.m << 1) == 0;
}

static int regIsZero(BmxX87Reg r)
{
    return r.m == 0 && (r.se & EXP_MAX) != EXP_MAX;
}

static int regNeg(BmxX87Reg r)
{
    return (r.se & SIGN) != 0;
}

/* Finite and nonzero: the significand normalised (the x87 never makes unnormals; denormals get normalised).
 */
static Unp unpack(BmxX87Reg r)
{
    Unp u;
    u.neg = regNeg(r);
    u.exp = r.se & EXP_MAX;
    u.m = r.m;
    if (u.exp == 0) {
        u.exp = 1;
    }
    const int z = clz64(u.m);
    u.m <<= z;
    u.exp -= z;
    return u;
}

static BmxX87Reg quiet(BmxX87Reg r)
{
    r.m |= QUIET;
    return r;
}

/* The x87's choice between two NaN operands (SDM table 4-7; SoftFloat's x87 rules). */
static BmxX87Reg nanPick(BmxX87Reg a, BmxX87Reg b)
{
    const int aNan = regIsNan(a), bNan = regIsNan(b);
    const int aSig = aNan && !(a.m & QUIET), bSig = bNan && !(b.m & QUIET);
    if (aSig) {
        if (bSig) {
            goto larger;
        }
        return quiet(bNan ? b : a);
    }
    if (aNan) {
        if (bSig || !bNan) {
            return quiet(a);
        }
    larger:
        if (a.m < b.m) {
            return quiet(b);
        }
        if (b.m < a.m) {
            return quiet(a);
        }
        return quiet(a.se < b.se ? a : b);
    }
    return quiet(b);
}

static int precisionBits(uint16_t cw)
{
    switch ((cw >> 8) & 3) {
    case 0:
        return 24;
    case 2:
        return 53;
    default:
        return 64;
    }
}

static int roundingMode(uint16_t cw)
{
    return (cw >> 10) & 3;
}

/*
 * Rounds (hi, lo, sticky) to keep bits (1..64) of hi: hi:lo is the significand with the binary point after
 * the top bit of hi, sticky any nonzero bits below lo. Returns 1 when the rounding carried out of hi.
 */
static int roundSig(uint64_t *hi, uint64_t lo, int sticky, int keep, int neg, int rc, int *inexact)
{
    const int drop = 64 - keep;
    int above, half;
    if (drop == 0) {
        above = lo > TOP || (lo == TOP && sticky);
        half = lo == TOP && !sticky;
        *inexact = lo != 0 || sticky;
    } else {
        const uint64_t mask = (1ull << drop) - 1;
        const uint64_t rest = *hi & mask, halfBit = 1ull << (drop - 1);
        above = rest > halfBit || (rest == halfBit && (lo != 0 || sticky));
        half = rest == halfBit && lo == 0 && !sticky;
        *inexact = rest != 0 || lo != 0 || sticky;
        *hi &= ~mask;
    }
    int inc;
    switch (rc) {
    case RC_NEAREST:
        inc = above || (half && ((*hi >> drop) & 1));
        break;
    case RC_DOWN:
        inc = *inexact && neg;
        break;
    case RC_UP:
        inc = *inexact && !neg;
        break;
    default:
        inc = 0;
        break;
    }
    if (!inc) {
        return 0;
    }
    const uint64_t unit = 1ull << drop;
    *hi += unit;
    return *hi == 0;
}

/* A register from an exact (or 128-bit) value: hi normalised, value = hi:lo * 2^(exp - BIAS - 63). */
static BmxX87Reg roundToReg(int neg, int32_t exp, uint64_t hi, uint64_t lo, int sticky, int prec, int rc)
{
    BmxX87Reg r;
    const uint16_t sign = neg ? SIGN : 0;
    int inexact;
    if (exp <= 0) {
        /* Below the normal range (never reached by the game's values): gradual underflow. */
        const int shift = 1 - exp;
        if (shift >= 128) {
            sticky |= hi != 0 || lo != 0;
            hi = 0;
            lo = 0;
        } else if (shift >= 64) {
            sticky |= lo != 0 || (shift > 64 && (hi << (128 - shift)) != 0);
            lo = shift == 64 ? hi : hi >> (shift - 64);
            hi = 0;
        } else {
            sticky |= (lo << (64 - shift)) != 0;
            lo = (lo >> shift) | (hi << (64 - shift));
            hi >>= shift;
        }
        if (roundSig(&hi, lo, sticky, prec, neg, rc, &inexact)) {
            hi = TOP;
        }
        r.m = hi;
        r.se = (uint16_t)(sign | ((hi & TOP) ? 1 : 0));
        return r;
    }
    if (roundSig(&hi, lo, sticky, prec, neg, rc, &inexact)) {
        hi = TOP;
        exp++;
    }
    if (exp >= EXP_MAX) {
        const int toInf = rc == RC_NEAREST || (rc == RC_UP && !neg) || (rc == RC_DOWN && neg);
        if (toInf) {
            r.m = TOP;
            r.se = (uint16_t)(sign | EXP_MAX);
        } else {
            r.m = prec == 64 ? ~0ull : ~((1ull << (64 - prec)) - 1);
            r.se = (uint16_t)(sign | (EXP_MAX - 1));
        }
        return r;
    }
    r.m = hi;
    r.se = (uint16_t)(sign | exp);
    return r;
}

/* ---- loads and stores ---- */

static BmxX87Reg fromBinary(uint64_t frac, int expField, int neg, int fracBits, int expMax, int bias)
{
    BmxX87Reg r;
    const uint16_t sign = neg ? SIGN : 0;
    if (expField == expMax) {
        r.m = TOP | (frac << (63 - fracBits));
        r.se = (uint16_t)(sign | EXP_MAX);
        return r;
    }
    if (expField == 0) {
        if (frac == 0) {
            r.m = 0;
            r.se = sign;
            return r;
        }
        const int z = clz64(frac);
        r.m = frac << z;
        r.se = (uint16_t)(sign | (1 - bias + BIAS - (z - (63 - fracBits))));
        return r;
    }
    r.m = TOP | (frac << (63 - fracBits));
    r.se = (uint16_t)(sign | (expField - bias + BIAS));
    return r;
}

/* An operand as an arithmetic instruction reads it from memory (a signalling NaN stays signalling). */
static BmxX87Reg memFloat(float v)
{
    uint32_t b;
    __builtin_memcpy(&b, &v, 4);
    return fromBinary(b & 0x7fffff, (b >> 23) & 0xff, b >> 31, 23, 0xff, 127);
}

static BmxX87Reg memDouble(double v)
{
    uint64_t b;
    __builtin_memcpy(&b, &v, 8);
    return fromBinary(b & 0xfffffffffffffull, (int)((b >> 52) & 0x7ff), (int)(b >> 63), 52, 0x7ff, 1023);
}

/* FLD: a signalling NaN is quietened. */
static BmxX87Reg fldFloat(float v)
{
    const BmxX87Reg r = memFloat(v);
    return regIsNan(r) ? quiet(r) : r;
}

static BmxX87Reg fldDouble(double v)
{
    const BmxX87Reg r = memDouble(v);
    return regIsNan(r) ? quiet(r) : r;
}

BmxX87Reg bmxX87SoftLoadDouble(double v)
{
    return fldDouble(v);
}

static BmxX87Reg fild(int32_t n)
{
    BmxX87Reg r;
    if (n == 0) {
        return REG_ZERO;
    }
    const uint64_t a = n < 0 ? (uint64_t)0 - (uint64_t)(int64_t)n : (uint64_t)n;
    const int z = clz64(a);
    r.m = a << z;
    r.se = (uint16_t)((n < 0 ? SIGN : 0) | (BIAS + 63 - z));
    return r;
}

/* FST to a binary format: NaNs keep their top significand bits, the value is rounded with denormals. */
static uint64_t toBinary(BmxX87Reg r, int fracBits, int expMax, int bias, int rc)
{
    const uint64_t sign = (uint64_t)regNeg(r) << (fracBits + (expMax == 0xff ? 8 : 11));
    const uint64_t expAll = (uint64_t)expMax << fracBits;
    if (regIsNan(r)) {
        return sign | expAll | ((r.m << 1) >> (64 - fracBits)) | (1ull << (fracBits - 1));
    }
    if (regIsInf(r)) {
        return sign | expAll;
    }
    if (regIsZero(r)) {
        return sign;
    }
    const Unp u = unpack(r);
    const int32_t e = u.exp - BIAS; /* value = m * 2^(e - 63), m in [2^63, 2^64) */
    const int32_t emin = 1 - bias;
    int keep = fracBits + 1;
    if (e < emin) {
        keep -= emin - e;
    }
    uint64_t q;
    int inexact;
    if (keep <= 0) {
        /* Below half the smallest denormal (keep < 0), or in [half, one) of it (keep == 0). */
        const int up =
            rc == RC_NEAREST ? (keep == 0 && u.m > TOP) : (rc == RC_UP && !u.neg) || (rc == RC_DOWN && u.neg);
        q = up ? 1 : 0;
    } else {
        uint64_t hi = u.m;
        const int carry = roundSig(&hi, 0, 0, keep, u.neg, rc, &inexact);
        q = carry ? (1ull << keep) : hi >> (64 - keep);
    }
    if (e < emin) {
        return sign | q; /* a carry into bit fracBits makes the smallest normal */
    }
    uint64_t bits =
        e + bias >= expMax ? expAll : ((uint64_t)(e + bias) << fracBits) + (q - (1ull << fracBits));
    if (bits >= expAll) {
        const int toInf = rc == RC_NEAREST || (rc == RC_UP && !u.neg) || (rc == RC_DOWN && u.neg);
        bits = toInf ? expAll : expAll - 1;
    }
    return sign | bits;
}

static float fstFloat(BmxX87Reg r, int rc)
{
    const uint32_t b = (uint32_t)toBinary(r, 23, 0xff, 127, rc);
    float v;
    __builtin_memcpy(&v, &b, 4);
    return v;
}

static double fstDouble(BmxX87Reg r, int rc)
{
    const uint64_t b = toBinary(r, 52, 0x7ff, 1023, rc);
    double v;
    __builtin_memcpy(&v, &b, 8);
    return v;
}

double bmxX87SoftStoreDouble(BmxX87Reg r)
{
    return fstDouble(r, roundingMode(bmxX87ControlWord()));
}

/* ---- arithmetic (FADD / FSUB / FMUL / FSQRT) ---- */

typedef struct Ctl {
    int prec, rc;
} Ctl;

static Ctl control(void)
{
    const uint16_t cw = bmxX87ControlWord();
    Ctl c = {precisionBits(cw), roundingMode(cw)};
    return c;
}

/* A register value rounded to the precision control (what FADD with a zero operand returns). */
static BmxX87Reg roundReg(BmxX87Reg a, Ctl c)
{
    const Unp u = unpack(a);
    return roundToReg(u.neg, u.exp, u.m, 0, 0, c.prec, c.rc);
}

static BmxX87Reg fadd(BmxX87Reg a, BmxX87Reg b, Ctl c)
{
    if (regIsNan(a) || regIsNan(b)) {
        return nanPick(a, b);
    }
    if (regIsInf(a) || regIsInf(b)) {
        if (regIsInf(a) && regIsInf(b) && regNeg(a) != regNeg(b)) {
            return kIndefinite;
        }
        return regIsInf(a) ? a : b;
    }
    if (regIsZero(a) && regIsZero(b)) {
        BmxX87Reg z = REG_ZERO;
        if (regNeg(a) == regNeg(b)) {
            z.se = a.se & SIGN;
        } else if (c.rc == RC_DOWN) {
            z.se = SIGN;
        }
        return z;
    }
    if (regIsZero(a)) {
        return roundReg(b, c);
    }
    if (regIsZero(b)) {
        return roundReg(a, c);
    }
    Unp x = unpack(a), y = unpack(b);
    if (y.exp > x.exp || (y.exp == x.exp && y.m > x.m)) {
        const Unp t = x;
        x = y;
        y = t;
    }
    const int32_t d = x.exp - y.exp;
    /* y shifted into a 128-bit window below x's top bit; bits beyond it only make the sticky flag. */
    uint64_t yh, yl;
    int sticky = 0;
    if (d == 0) {
        yh = y.m;
        yl = 0;
    } else if (d < 64) {
        yh = y.m >> d;
        yl = y.m << (64 - d);
    } else if (d < 128) {
        yh = 0;
        yl = d == 64 ? y.m : y.m >> (d - 64);
        sticky = d > 64 && (y.m << (128 - d)) != 0;
    } else {
        yh = 0;
        yl = 0;
        sticky = 1;
    }
    uint64_t hi, lo;
    int32_t exp = x.exp;
    if (x.neg == y.neg) {
        lo = yl;
        hi = x.m + yh;
        if (hi < x.m) {
            sticky |= lo & 1;
            lo = (lo >> 1) | (hi << 63);
            hi = (hi >> 1) | TOP;
            exp++;
        }
    } else {
        /* x - (y + eps) = (x - y - 1 unit) + (1 - eps): the window minus one, with the sticky flag. */
        lo = 0 - yl;
        hi = x.m - yh - (yl != 0);
        if (sticky) {
            hi -= lo == 0;
            lo -= 1;
        }
        if (hi == 0 && lo == 0 && !sticky) {
            BmxX87Reg z = REG_ZERO;
            z.se = c.rc == RC_DOWN ? SIGN : 0;
            return z;
        }
        int z;
        if (hi == 0) {
            hi = lo;
            lo = 0;
            exp -= 64;
            z = clz64(hi);
        } else {
            z = clz64(hi);
        }
        if (z != 0) {
            hi = (hi << z) | (lo >> (64 - z));
            lo <<= z;
            exp -= z;
        }
    }
    return roundToReg(x.neg, exp, hi, lo, sticky, c.prec, c.rc);
}

static BmxX87Reg fsub(BmxX87Reg a, BmxX87Reg b, Ctl c)
{
    if (!regIsNan(b)) {
        b.se ^= SIGN;
    }
    return fadd(a, b, c);
}

static BmxX87Reg fmul(BmxX87Reg a, BmxX87Reg b, Ctl c)
{
    if (regIsNan(a) || regIsNan(b)) {
        return nanPick(a, b);
    }
    const uint16_t sign = (uint16_t)((a.se ^ b.se) & SIGN);
    if (regIsInf(a) || regIsInf(b)) {
        if (regIsZero(a) || regIsZero(b)) {
            return kIndefinite;
        }
        BmxX87Reg r = {TOP, (uint16_t)(sign | EXP_MAX)};
        return r;
    }
    if (regIsZero(a) || regIsZero(b)) {
        BmxX87Reg r = {0, sign};
        return r;
    }
    const Unp x = unpack(a), y = unpack(b);
    uint64_t hi, lo;
    mul64(x.m, y.m, &hi, &lo);
    int32_t exp = x.exp + y.exp - BIAS + 1;
    if (!(hi & TOP)) {
        hi = (hi << 1) | (lo >> 63);
        lo <<= 1;
        exp--;
    }
    return roundToReg(sign != 0, exp, hi, lo, 0, c.prec, c.rc);
}

/* floor(sqrt(hi:lo)) for hi:lo >= 2^126 (a 64-bit root) and whether the remainder is nonzero / above root. */
static uint64_t isqrt128(uint64_t hi, uint64_t lo, int *nonzero, int *aboveHalf)
{
    uint64_t root = 0, remHi = 0, remLo = 0;
    for (int i = 0; i < 64; i++) {
        /* rem = rem * 4 + next two bits */
        const uint64_t two = hi >> 62;
        hi = (hi << 2) | (lo >> 62);
        lo <<= 2;
        remHi = (remHi << 2) | (remLo >> 62);
        remLo = (remLo << 2) | two;
        /* trial = root * 4 + 1 (root is the root so far, at most 63 bits here) */
        const uint64_t trialHi = root >> 62, trialLo = (root << 2) | 1;
        root <<= 1;
        if (remHi > trialHi || (remHi == trialHi && remLo >= trialLo)) {
            remHi = remHi - trialHi - (remLo < trialLo);
            remLo -= trialLo;
            root |= 1;
        }
    }
    *nonzero = remHi != 0 || remLo != 0;
    *aboveHalf = remHi != 0 || remLo > root; /* N > r^2 + r + 1/4 */
    return root;
}

static BmxX87Reg fsqrt(BmxX87Reg a, Ctl c)
{
    if (regIsNan(a)) {
        return quiet(a);
    }
    if (regIsZero(a)) {
        return a;
    }
    if (regNeg(a)) {
        return kIndefinite;
    }
    if (regIsInf(a)) {
        return a;
    }
    const Unp u = unpack(a);
    const int32_t e = u.exp - BIAS;
    const int s = (e & 1) ? 64 : 63; /* m * 2^s in [2^126, 2^128), e - 63 - s even */
    const uint64_t nh = s == 64 ? u.m : u.m >> 1, nl = s == 64 ? 0 : u.m << 63;
    int nonzero, aboveHalf;
    const uint64_t root = isqrt128(nh, nl, &nonzero, &aboveHalf);
    const int32_t re = (e - 63 - s) / 2 + 63;
    const uint64_t lo = aboveHalf ? TOP | 1 : (nonzero ? 1 : 0);
    return roundToReg(0, re + BIAS, root, lo, 0, c.prec, c.rc);
}

float bmxX87SoftStoreFloat(BmxX87Reg r)
{
    return fstFloat(r, roundingMode(bmxX87ControlWord()));
}

BmxX87Reg bmxX87SoftArith(int op, BmxX87Reg a, BmxX87Reg b)
{
    const Ctl c = control();
    switch (op) {
    case 0:
        return fadd(a, b, c);
    case 1:
        return fsub(a, b, c);
    case 2:
        return fmul(a, b, c);
    default:
        return fsqrt(a, c);
    }
}

/* ---- 128-bit floating point for the transcendental instructions ---- */

/* value = (h:l) * 2^(e - 127), h:l normalised (top bit set) or zero. Operations truncate. */
typedef struct Xf {
    uint64_t h, l;
    int32_t e;
    int neg;
} Xf;

static const Xf kXfOne = {TOP, 0, 0, 0};
static const Xf kXfPi = {0xc90fdaa22168c234ull, 0xc4c6628b80dc1cd1ull, 1, 0};

static int xfZero(Xf a)
{
    return a.h == 0 && a.l == 0;
}

/* W * 2^x (W 256-bit), truncated to 128 bits. */
static Xf xfFromWords(U256 w, int32_t x, int neg)
{
    Xf r = {0, 0, 0, neg};
    const int p = u256Top(&w);
    if (p < 0) {
        return r;
    }
    const U256 t = p >= 127 ? u256Shr(w, p - 127) : u256Shl(w, 127 - p);
    r.h = t.w[1];
    r.l = t.w[0];
    r.e = x + p;
    return r;
}

static Xf xfFromReg(BmxX87Reg a)
{
    const Unp u = unpack(a);
    Xf r = {u.m, 0, u.exp - BIAS, u.neg};
    return r;
}

static BmxX87Reg xfToReg(Xf a)
{
    if (xfZero(a)) {
        BmxX87Reg z = {0, (uint16_t)(a.neg ? SIGN : 0)};
        return z;
    }
    return roundToReg(a.neg, a.e + BIAS, a.h, a.l, 0, 64, RC_NEAREST);
}

static Xf xfNeg(Xf a)
{
    a.neg = !a.neg;
    return a;
}

static Xf xfAdd(Xf a, Xf b)
{
    if (xfZero(a)) {
        return b;
    }
    if (xfZero(b)) {
        return a;
    }
    if (b.e > a.e || (b.e == a.e && (b.h > a.h || (b.h == a.h && b.l > a.l)))) {
        const Xf t = a;
        a = b;
        b = t;
    }
    const int32_t d = a.e - b.e;
    if (d >= 128) {
        return a;
    }
    const U256 aw = {{0, 0, a.l, a.h}};
    const U256 bw = u256Shr((U256){{0, 0, b.l, b.h}}, d);
    if (a.neg == b.neg) {
        U256 r;
        uint64_t carry = 0;
        for (int i = 0; i < 4; i++) {
            const uint64_t s1 = aw.w[i] + carry;
            const uint64_t c1 = s1 < carry;
            r.w[i] = s1 + bw.w[i];
            carry = c1 | (r.w[i] < bw.w[i]);
        }
        if (carry) {
            r = u256Shr(r, 1);
            r.w[3] |= TOP;
            return xfFromWords(r, a.e - 255 + 1, a.neg);
        }
        return xfFromWords(r, a.e - 255, a.neg);
    }
    return xfFromWords(u256Sub(aw, bw), a.e - 255, a.neg);
}

static Xf xfSub(Xf a, Xf b)
{
    return xfAdd(a, xfNeg(b));
}

static void add64(uint64_t *acc, uint64_t v, uint64_t *carry)
{
    *acc += v;
    *carry += *acc < v;
}

static Xf xfMul(Xf a, Xf b)
{
    if (xfZero(a) || xfZero(b)) {
        Xf z = {0, 0, 0, a.neg != b.neg};
        return z;
    }
    uint64_t hh1, hh0, hl1, hl0, lh1, lh0, ll1, ll0;
    mul64(a.h, b.h, &hh1, &hh0);
    mul64(a.h, b.l, &hl1, &hl0);
    mul64(a.l, b.h, &lh1, &lh0);
    mul64(a.l, b.l, &ll1, &ll0);
    U256 w;
    uint64_t c1 = 0, c2 = 0;
    w.w[0] = ll0;
    w.w[1] = ll1;
    add64(&w.w[1], hl0, &c1);
    add64(&w.w[1], lh0, &c1);
    w.w[2] = hh0;
    add64(&w.w[2], hl1, &c2);
    add64(&w.w[2], lh1, &c2);
    add64(&w.w[2], c1, &c2);
    w.w[3] = hh1 + c2;
    return xfFromWords(w, a.e + b.e - 254, a.neg != b.neg);
}

static Xf xfDivSmall(Xf a, uint32_t d)
{
    const uint32_t limbs[6] = {
        0, 0, (uint32_t)a.l, (uint32_t)(a.l >> 32), (uint32_t)a.h, (uint32_t)(a.h >> 32)};
    uint32_t q[6];
    uint64_t rem = 0;
    for (int i = 5; i >= 0; i--) {
        uint32_t r;
        q[i] = div64by32((rem << 32) | limbs[i], d, &r);
        rem = r;
    }
    U256 w = {{(uint64_t)q[1] << 32 | q[0], (uint64_t)q[3] << 32 | q[2], (uint64_t)q[5] << 32 | q[4], 0}};
    return xfFromWords(w, a.e - 127 - 64, a.neg);
}

/* a / b by Newton's iteration from a double estimate (the estimate's rounding does not matter). */
static Xf xfDiv(Xf a, Xf b)
{
    Xf bn = b, an = a;
    bn.e = 0;
    bn.neg = 0;
    an.e = 0;
    an.neg = 0;
    /* The top 52 fraction bits of b as a double in [1, 2) (built from bits: no integer conversion). */
    uint64_t bits = (1023ull << 52) | ((b.h << 1) >> 12);
    double est;
    __builtin_memcpy(&est, &bits, 8);
    est = 1.0 / est;
    __builtin_memcpy(&bits, &est, 8);
    Xf y = {((bits & 0xfffffffffffffull) | 0x10000000000000ull) << 11, 0,
            (int32_t)((bits >> 52) & 0x7ff) - 1023, 0};
    for (int i = 0; i < 2; i++) {
        y = xfAdd(y, xfMul(y, xfSub(kXfOne, xfMul(bn, y))));
    }
    Xf q = xfMul(an, y);
    q = xfAdd(q, xfMul(y, xfSub(an, xfMul(bn, q))));
    q.e += a.e - b.e;
    q.neg = a.neg != b.neg;
    return q;
}

/*
 * main + tail rounded to the register for an exact main and a tail far smaller than it: when the tail falls
 * below the 128-bit window, only its sign matters (main plus or minus a sliver), as in fadd's sticky flag.
 */
static BmxX87Reg xfToRegSum(Xf main, Xf tail)
{
    if (xfZero(tail) || xfZero(main)) {
        return xfToReg(xfAdd(main, tail));
    }
    if (tail.e >= main.e - 120) {
        return xfToReg(xfAdd(main, tail));
    }
    uint64_t h = main.h, l = main.l;
    int32_t e = main.e;
    if (tail.neg != main.neg) {
        h -= l == 0;
        l -= 1;
        if (!(h & TOP)) {
            h = (h << 1) | (l >> 63);
            l = (l << 1) | 1;
            e--;
        }
    }
    return roundToReg(main.neg, e + BIAS, h, l, 1, 64, RC_NEAREST);
}

/* sin(r), cos(r) for |r| <= ~pi/4 + a little: Taylor series to ~2^-125. sin(r) = r + tail (r exact). */
#define SERIES_TERMS 19
static Xf xfSinTail(Xf r)
{
    Xf c[SERIES_TERMS];
    c[0] = kXfOne;
    for (int k = 1; k < SERIES_TERMS; k++) {
        c[k] = xfDivSmall(c[k - 1], (uint32_t)((2 * k) * (2 * k + 1)));
    }
    const Xf z = xfMul(r, r);
    Xf s = c[SERIES_TERMS - 1];
    for (int k = SERIES_TERMS - 2; k >= 1; k--) {
        s = xfSub(c[k], xfMul(z, s));
    }
    return xfNeg(xfMul(r, xfMul(z, s)));
}

static Xf xfSinSeries(Xf r)
{
    return xfAdd(r, xfSinTail(r));
}

static Xf xfCosSeries(Xf r)
{
    Xf c[SERIES_TERMS];
    c[0] = kXfOne;
    for (int k = 1; k < SERIES_TERMS; k++) {
        c[k] = xfDivSmall(c[k - 1], (uint32_t)((2 * k - 1) * (2 * k)));
    }
    const Xf z = xfMul(r, r);
    Xf s = c[SERIES_TERMS - 1];
    for (int k = SERIES_TERMS - 2; k >= 0; k--) {
        s = xfSub(c[k], xfMul(z, s));
    }
    return s;
}

/* atan(t) for 0 <= t <= 1. */
#define ATAN_TERMS 54
static Xf xfAtanSeries(Xf t)
{
    const Xf z = xfMul(t, t);
    Xf s = xfDivSmall(kXfOne, 2 * (ATAN_TERMS - 1) + 1);
    for (int k = ATAN_TERMS - 2; k >= 0; k--) {
        s = xfSub(xfDivSmall(kXfOne, (uint32_t)(2 * k + 1)), xfMul(z, s));
    }
    return xfMul(t, s);
}

static Xf xfAtanUnit(Xf t)
{
    /* Above 0.4140625, atan(t) = pi/4 + atan((t - 1) / (t + 1)), |(t - 1) / (t + 1)| < 0.4146. */
    if (xfZero(t) || t.e < -2 || (t.e == -2 && t.h < 0xd400000000000000ull)) {
        return xfAtanSeries(t);
    }
    const Xf u = xfDiv(xfSub(t, kXfOne), xfAdd(t, kXfOne));
    Xf quarterPi = kXfPi;
    quarterPi.e -= 2;
    const Xf au = u.neg ? xfNeg(xfAtanSeries(xfNeg(u))) : xfAtanSeries(u);
    return xfAdd(quarterPi, au);
}

/*
 * The reduction of FSIN / FCOS / FPTAN: |x| = k * P/2 + r with k the nearest integer and r exact, where P is
 * the reduction's pi (Intel's 66-bit pi, or pi to 128 bits). |x| < 2^63.
 */
static Xf reduce(Unp u, unsigned *quadrant)
{
    const int32_t e = u.exp - BIAS;
    Xf r = {u.m, 0, e, 0};
    *quadrant = 0;
    if (e < -1) {
        return r;
    }
    U256 p;
    int fracBits;
    if (gPrecisePi) {
        p = (U256){{0xc4c6628b80dc1cd1ull, 0xc90fdaa22168c234ull, 0, 0}}; /* pi * 2^126 */
        fracBits = 126;
    } else {
        p = (U256){{0x243f6a8885a308d3ull, 3, 0, 0}}; /* pi * 2^64, 66 bits */
        fracBits = 64;
    }
    /* |x| / (P/2) = m * 2^(e - 62 + fracBits) / Pint */
    U256 n = u256Shl((U256){{u.m, 0, 0, 0}}, e - 62 + fracBits);
    uint64_t k = 0;
    for (int i = 63; i >= 0; i--) {
        const U256 t = u256Shl(p, i);
        if (u256Cmp(&n, &t) >= 0) {
            n = u256Sub(n, t);
            k |= 1ull << i;
        }
    }
    const U256 twice = u256Shl(n, 1);
    int neg = 0;
    if (u256Cmp(&twice, &p) > 0) {
        n = u256Sub(p, n);
        neg = 1;
        k++;
    }
    *quadrant = (unsigned)(k & 3);
    return xfFromWords(n, -(fracBits + 1), neg);
}

static int outOfRange(BmxX87Reg x)
{
    return (x.se & EXP_MAX) >= BIAS + 63;
}

BmxX87Reg bmxX87SoftFsinReg(BmxX87Reg x)
{
    if (regIsNan(x)) {
        return quiet(x);
    }
    if (regIsInf(x)) {
        return kIndefinite;
    }
    if (regIsZero(x) || outOfRange(x)) {
        return x;
    }
    const Unp u = unpack(x);
    unsigned q;
    const Xf r = reduce(u, &q);
    const unsigned negate = ((q >> 1) & 1) ^ (u.neg != 0);
    if (!(q & 1)) {
        Xf main = r, tail = xfSinTail(r);
        if (negate) {
            main = xfNeg(main);
            tail = xfNeg(tail);
        }
        return xfToRegSum(main, tail);
    }
    Xf v = xfCosSeries(r);
    if (negate) {
        v = xfNeg(v);
    }
    return xfToReg(v);
}

BmxX87Reg bmxX87SoftFcosReg(BmxX87Reg x)
{
    if (regIsNan(x)) {
        return quiet(x);
    }
    if (regIsInf(x)) {
        return kIndefinite;
    }
    if (regIsZero(x)) {
        return kOne;
    }
    if (outOfRange(x)) {
        return x;
    }
    const Unp u = unpack(x);
    unsigned q;
    const Xf r = reduce(u, &q);
    if (q & 1) {
        Xf main = r, tail = xfSinTail(r);
        if (q == 1) {
            main = xfNeg(main);
            tail = xfNeg(tail);
        }
        return xfToRegSum(main, tail);
    }
    Xf v = xfCosSeries(r);
    if (q == 2) {
        v = xfNeg(v);
    }
    return xfToReg(v);
}

/* FPTAN's ST(1) (the tangent); *pushed = 0 when the operand is out of range (C2, nothing pushed). */
static BmxX87Reg fptan(BmxX87Reg x, int *pushed)
{
    *pushed = 1;
    if (regIsNan(x)) {
        return quiet(x);
    }
    if (regIsInf(x)) {
        return kIndefinite;
    }
    if (regIsZero(x)) {
        return x;
    }
    if (outOfRange(x)) {
        *pushed = 0;
        return x;
    }
    const Unp u = unpack(x);
    unsigned q;
    const Xf r = reduce(u, &q);
    const Xf s = xfSinSeries(r), c = xfCosSeries(r);
    Xf v = (q & 1) ? xfNeg(xfDiv(c, s)) : xfDiv(s, c);
    if (u.neg) {
        v = xfNeg(v);
    }
    return xfToReg(v);
}

/* FPATAN: atan2(ST(1), ST(0)). */
BmxX87Reg bmxX87SoftFpatanReg(BmxX87Reg y, BmxX87Reg x)
{
    if (regIsNan(y) || regIsNan(x)) {
        return nanPick(y, x);
    }
    const int yNeg = regNeg(y), xNeg = regNeg(x);
    Xf v;
    if (regIsInf(y) || (regIsZero(x) && !regIsZero(y))) {
        v = kXfPi;
        v.e = 0;
        if (regIsInf(y) && regIsInf(x)) {
            v.e = -1; /* pi/4, or 3pi/4 for x = -inf */
            if (xNeg) {
                v = xfMul(v, (Xf){0xc000000000000000ull, 0, 1, 0});
            }
        }
    } else if (regIsZero(y) || regIsInf(x)) {
        v = xNeg ? kXfPi : (Xf){0, 0, 0, 0};
    } else {
        Xf a = xfFromReg(y), b = xfFromReg(x);
        a.neg = 0;
        b.neg = 0;
        const int swap = a.e > b.e || (a.e == b.e && (a.h > b.h || (a.h == b.h && a.l > b.l)));
        v = swap ? xfDiv(b, a) : xfDiv(a, b);
        v = xfAtanUnit(v);
        if (swap) {
            Xf halfPi = kXfPi;
            halfPi.e = 0;
            v = xfSub(halfPi, v);
        }
        if (xNeg) {
            v = xfSub(kXfPi, v);
        }
    }
    v.neg = yNeg;
    return xfToReg(v);
}

/* ---- the helpers (compat/x87.h has the instruction sequences) ---- */

float bmxX87SoftSinScaled(int32_t n, float k)
{
    const Ctl c = control();
    const BmxX87Reg x = fmul(fild(n), memFloat(k), c);
    return fstFloat(bmxX87SoftFsinReg(x), c.rc);
}

double bmxX87SoftCosScaled(int32_t n, double k)
{
    const Ctl c = control();
    const BmxX87Reg x = fmul(fild(n), memDouble(k), c);
    return fstDouble(bmxX87SoftFcosReg(x), c.rc);
}

double bmxX87SoftFsin(double a)
{
    return fstDouble(bmxX87SoftFsinReg(fldDouble(a)), control().rc);
}

double bmxX87SoftFcos(double a)
{
    return fstDouble(bmxX87SoftFcosReg(fldDouble(a)), control().rc);
}

double bmxX87SoftAsinScaled(int32_t n, double step, float scale)
{
    const Ctl c = control();
    const BmxX87Reg x = fmul(fild(n), memDouble(step), c);
    const BmxX87Reg p = fmul(fadd(kOne, x, c), fsub(kOne, x, c), c);
    const BmxX87Reg angle = bmxX87SoftFpatanReg(x, fsqrt(p, c));
    return fstDouble(fmul(angle, memFloat(scale), c), c.rc);
}

float bmxX87SoftAngleOfCosine(double cosine)
{
    const Ctl c = control();
    const BmxX87Reg x = fldDouble(cosine);
    const BmxX87Reg p = fmul(fadd(kOne, x, c), fsub(kOne, x, c), c);
    return fstFloat(bmxX87SoftFpatanReg(fsqrt(p, c), x), c.rc);
}

float bmxX87SoftTanFloat(float a)
{
    const Ctl c = control();
    int pushed;
    const BmxX87Reg t = fptan(fldFloat(a), &pushed);
    /* Out of range nothing is pushed: FSTP ST(0) pops the operand, the FSTP after it underflows. */
    return fstFloat(pushed ? t : kIndefinite, c.rc);
}

double bmxX87SoftHalfPiScaled(int negative, float scale)
{
    const Ctl c = control();
    const BmxX87Reg halfPi = {0xc90fdaa22168c235ull, (uint16_t)(BIAS | (negative ? SIGN : 0))};
    return fstDouble(fmul(halfPi, memFloat(scale), c), c.rc);
}

double bmxX87SoftAtanScaled(int32_t n, double step, float scale)
{
    const Ctl c = control();
    const BmxX87Reg x = fmul(fild(n), memDouble(step), c);
    const BmxX87Reg angle = bmxX87SoftFpatanReg(x, kOne);
    return fstDouble(fmul(angle, memFloat(scale), c), c.rc);
}

void bmxX87SoftRotate(float *r1, float *r2, float a, float b, const float *cp, const float *sp)
{
    const Ctl c = control();
    const BmxX87Reg ra = fldFloat(a), rb = fldFloat(b), rc = fldFloat(*cp), rs = fldFloat(*sp);
    *r1 = fstFloat(fsub(fmul(rc, ra, c), fmul(rs, rb, c), c), c.rc);
    *r2 = fstFloat(fadd(fmul(rc, rb, c), fmul(rs, ra, c), c), c.rc);
}

/* One element: x0 * b[col] (+ b[12 + col] for the position row) + (x1 * b[4 + col] + x2 * b[8 + col]). */
static BmxX87Reg matrixElement(const BmxX87Reg x[3], const float *b, int col, int position, Ctl c)
{
    BmxX87Reg first = fmul(x[0], memFloat(b[col]), c);
    if (position) {
        first = fadd(first, memFloat(b[12 + col]), c);
    }
    const BmxX87Reg q = fmul(x[1], memFloat(b[4 + col]), c);
    const BmxX87Reg r = fmul(x[2], memFloat(b[8 + col]), c);
    return fadd(first, fadd(q, r, c), c);
}

void bmxX87SoftMatrixMultiply(float *out, const float *a, const float *b)
{
    const Ctl c = control();
    for (int row = 0; row < 4; row++) {
        /* Every load of a row comes before its stores, as in the original (out may alias a or b). */
        const BmxX87Reg x[3] = {fldFloat(a[row * 4]), fldFloat(a[row * 4 + 1]), fldFloat(a[row * 4 + 2])};
        BmxX87Reg v[3];
        for (int col = 0; col < 3; col++) {
            v[col] = matrixElement(x, b, col, row == 3, c);
        }
        for (int col = 0; col < 3; col++) {
            out[row * 4 + col] = fstFloat(v[col], c.rc);
        }
    }
}
