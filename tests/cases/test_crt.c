/* compat/crt.c against the exe's MSVC6 CRT: our stateless routines must behave identically. */
#include "compat/crt.h"
#include "dt.h"

static char *rand_str(int maxLen)
{
    const int len = dt_range(0, maxLen);
    char *s = dt_alloc((uint32_t)len + 1);
    for (int i = 0; i < len; i++) {
        s[i] = (char)dt_range(1, 255);
    }
    return s;
}

/* Two strings sharing a random prefix so comparisons reach interesting positions. */
static void setup_str2(DtCtx *c)
{
    char *a, *b;
    int n;
    dt_seed(c);
    a = rand_str(12);
    b = rand_str(12);
    n = dt_range(0, (int)(strlen(a) < strlen(b) ? strlen(a) : strlen(b)));
    memcpy(b, a, (size_t)n);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
    DT_ARG_INT(c, 2, dt_range(0, 16));
}

static void setup_str1(DtCtx *c)
{
    char *s;
    dt_seed(c);
    s = rand_str(20);
    DT_ARG_PTR(c, 0, s);
    DT_ARG_INT(c, 1,
               dt_u32() & 1 && *s ? (unsigned char)s[dt_range(0, (int)strlen(s))] : dt_range(-300, 300));
}

static void setup_memset(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_alloc(64));
    DT_ARG_INT(c, 1, dt_range(-300, 300));
    DT_ARG_INT(c, 2, dt_range(0, 64));
}

static void setup_strncpy(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_alloc(32));
    DT_ARG_PTR(c, 1, rand_str(20));
    DT_ARG_INT(c, 2, dt_range(0, 31));
}

DT_CASES(crt_cases) = {
    {"crt_strcmp", 0x0051ddf0, strcmp, 2, DT_RET_INT, setup_str2, 500, 0},
    {"crt_strncmp", 0x004a5190, strncmp, 3, DT_RET_INT, setup_str2, 500, 0},
    {"crt_strlen", 0x004a8c20, strlen, 1, DT_RET_INT, setup_str1, 300, 0},
    {"crt_strchr", 0x004a4c90, strchr, 2, DT_RET_INT, setup_str1, 500, 0},
    {"crt_memset", 0x004accc0, memset, 3, DT_RET_INT, setup_memset, 300, 0},
    {"crt_strncpy", 0x004a5600, strncpy, 3, DT_RET_INT, setup_strncpy, 300, 0},
};
