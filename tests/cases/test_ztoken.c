/*
 * Tests for the script tokenizer (ztoken.c) on generated text served by a fake file device. Setup opens
 * the text with the original code and consumes a few tokens first, so the tests start in many different
 * states (inside includes, with symbols defined, with comments open).
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zfile.h"

#include "zge/ztoken.h"

static const char kInclude1[] = "alpha beta \"two words\"\n$INC1 ; comment\n#DEFINE FROMINC 42\n";
static const char kInclude2[] = "gamma\n#INCLUDE inc1.txt\ndelta\n";
static const char kInclude3[] = "/* open block comment\nstill inside\n";

static const char *const kWords[] = {"foo", "Bar", "BAZ_1", "x", "0x1F", "10",
                                     "17",  "-3",  "SCENE", "{", "}",    "name"};
static const char *const kSymbols[] = {"$AA", "$bb", "$CCC", "$ZED"};
static const char *const kDirectives[] = {"#INCLUDE inc1.txt",
                                          "#include tx00:inc2.txt",
                                          "#INCLUDE_PATH tx00:",
                                          "#ifndef FOO",
                                          "#endif",
                                          "#DEFINE AA 7",
                                          "#define bb \"two words\"",
                                          "#DEFINE CCC 0x20",
                                          "$$~",
                                          "$$ZED 99",
                                          "$$$inc1.txt",
                                          "$* 3 4",
                                          "$+ 10 0x10",
                                          "$- 7 2",
                                          "$/ 9 3",
                                          "$AA",
                                          "$BB",
                                          "$ccc",
                                          "$ZED",
                                          "$NOPE",
                                          "/*",
                                          "*/",
                                          "/* a /* b */ c */",
                                          "// line comment",
                                          "; semicolon",
                                          "# hash comment"};

static void add_piece(char *dst, int *n)
{
    const char *s;
    char one[24];
    switch (dt_u32() % 5) {
    case 0:
        s = kWords[dt_u32() % (sizeof kWords / sizeof kWords[0])];
        break;
    case 1:
    case 2:
        s = kDirectives[dt_u32() % (sizeof kDirectives / sizeof kDirectives[0])];
        break;
    case 3: {
        int len = dt_range(1, 12), i;
        for (i = 0; i < len; i++) {
            one[i] = (char)dt_range('a', 'z');
        }
        one[len] = '\0';
        s = one;
        break;
    }
    default:
        s = kSymbols[dt_u32() % 4];
    }
    while (*s) {
        dst[(*n)++] = *s++;
    }
    dst[(*n)++] = (char)(dt_u32() % 4 == 0 ? ',' : dt_u32() % 8 == 0 ? '\t' : ' ');
}

static char *make_text(void)
{
    char *t = dt_alloc(2048);
    int n = 0, lines = dt_range(2, 14), l;
    for (l = 0; l < lines; l++) {
        int pieces = dt_range(0, 5), p;
        for (p = 0; p < pieces; p++) {
            add_piece(t, &n);
        }
        t[n++] = '\n';
    }
    /* Filler: an operand read at the end of the text would use uninitialised stack in the original. */
    for (l = 0; l < 6; l++) {
        t[n++] = (char)('w' - l % 2);
        t[n++] = (char)('1' + l);
        t[n++] = ' ';
    }
    if (dt_u32() % 5 == 0) {
        n--; /* no newline after the last line */
    } else {
        t[n++] = '\n';
    }
    if (dt_u32() % 13 == 0) {
        n = 0;
        memcpy(t, "\"unterminated quote\n", 21);
        n = 21;
    }
    t[n] = '\0';
    return t;
}

static int (*const orig_open)(const char *) = (int (*)(const char *))(uintptr_t)0x004a1120;
static int (*const orig_next)(char *, int) = (int (*)(char *, int))(uintptr_t)0x004a1220;
static int (*const orig_define)(const char *,
                                const char *) = (int (*)(const char *, const char *))(uintptr_t)0x004a20c0;

/* The tokenizer opened on generated text; `consume` tokens have been read by the original code. */
static void tok_begin(DtCtx *c, int consume)
{
    char out[0x100];
    int i;
    dt_seed(c);
    dtFileReset();
    dtFileAdd("main.txt", make_text());
    dtFileAdd("inc1.txt", kInclude1);
    dtFileAdd("inc2.txt", kInclude2);
    dtFileAdd("inc3.txt", kInclude3);
    dtFileInstall();
    memset(&gToken, 0, sizeof gToken);
    orig_open(dt_u32() & 1 ? "tx00:main.txt" : "main.txt");
    gToken.debug = dt_u32() % 5 == 0;
    gToken.upperCase = dt_u32() % 4 != 0;
    gToken.hashComments = dt_u32() % 3 == 0;
    gToken.macros = dt_u32() % 6 != 0;
    for (i = 0; i < consume; i++) {
        if (orig_next(out, 0x100) == 0) {
            break;
        }
    }
}

static void setup_next(DtCtx *c)
{
    char *out;
    tok_begin(c, dt_range(0, 6));
    out = dt_alloc(0x200);
    dt_fill(out, 0x200);
    DT_ARG_PTR(c, 0, out);
    DT_ARG_INT(c, 1, c->seed % 7 == 3 ? dt_range(2, 6) : c->seed % 5 == 1 ? 0x80 : 0x100);
}

static void setup_open(DtCtx *c)
{
    dt_seed(c);
    dtFileReset();
    dtFileAdd("main.txt", make_text());
    dtFileInstall();
    memset(&gToken, 0, sizeof gToken);
    dt_fill(&gToken, 64);
    switch (c->seed % 4) {
    case 0:
        DT_ARG_PTR(c, 0, "tx00:main.txt");
        break;
    case 1:
        DT_ARG_PTR(c, 0, "MAIN.TXT");
        break;
    case 2:
        DT_ARG_PTR(c, 0, "missing.txt");
        break;
    default:
        DT_ARG_PTR(c, 0, "main.txt");
    }
}

static void setup_close(DtCtx *c)
{
    tok_begin(c, dt_range(0, 3));
}

static void fill_symbols(void)
{
    int n = dt_range(0, 12), i;
    for (i = 0; i < n; i++) {
        char name[24], value[40];
        int ln = dt_range(1, 6), lv = dt_range(1, dt_u32() % 6 == 0 ? 60 : 12), k;
        name[0] = '$';
        for (k = 1; k <= ln; k++) {
            name[k] = (char)dt_range('A', 'Z');
        }
        name[ln + 1] = '\0';
        for (k = 0; k < lv; k++) {
            value[k] = (char)dt_range('a', 'z');
        }
        value[lv] = '\0';
        orig_define(name, value);
    }
}

static void setup_define(DtCtx *c)
{
    char *name = dt_alloc(24), *value = dt_alloc(80);
    int ln = dt_range(1, 6), lv = dt_range(0, 30), k;
    tok_begin(c, 0);
    fill_symbols();
    name[0] = '$';
    for (k = 1; k <= ln; k++) {
        name[k] = (char)dt_range('A', 'Z');
    }
    if (c->seed % 3 == 0 && gToken.symbolBytes > 0) {
        /* an existing symbol: copy a name from the table */
        const char *t = gToken.symbols;
        int a = 0;
        while (a < (int)gToken.symbolBytes && t[a] != '$') {
            a++;
        }
        for (k = 0; t[a + k] != '@' && k < 20; k++) {
            name[k] = t[a + k];
        }
        name[k] = '\0';
    }
    for (k = 0; k < lv; k++) {
        value[k] = (char)dt_range('a', 'z');
    }
    if (c->seed % 11 == 4) {
        value[0] = '$'; /* cannot be stored */
        if (lv == 0) {
            value[1] = '\0';
        }
    }
    DT_ARG_PTR(c, 0, name);
    DT_ARG_PTR(c, 1, value);
}

static void setup_lookup(DtCtx *c)
{
    char *name = dt_alloc(80);
    int k;
    tok_begin(c, 0);
    fill_symbols();
    name[0] = '$';
    name[1] = (char)dt_range('A', 'Z');
    name[2] = '\0';
    if (c->seed % 2 == 0 && gToken.symbolBytes > 0) {
        const char *t = gToken.symbols;
        int a = (int)(dt_u32() % gToken.symbolBytes);
        while (a < (int)gToken.symbolBytes && t[a] != '$') {
            a++;
        }
        if (a < (int)gToken.symbolBytes) {
            for (k = 0; t[a + k] != '@' && k < 20; k++) {
                name[k] = t[a + k];
            }
            name[k] = '\0';
        }
    }
    DT_ARG_PTR(c, 0, name);
}

static void setup_read_int(DtCtx *c)
{
    tok_begin(c, dt_range(0, 5));
    DT_ARG_PTR(c, 0, dt_alloc(8));
}

static void setup_read_string(DtCtx *c)
{
    setup_next(c);
}

static void setup_expect(DtCtx *c)
{
    char *s = dt_alloc(24);
    static const char *const kExpect[] = {"{", "scene", "SCENE", "name", "x", "foo", "}"};
    const char *e = kExpect[dt_u32() % (sizeof kExpect / sizeof kExpect[0])];
    tok_begin(c, dt_range(0, 3));
    memcpy(s, e, strlen(e) + 1);
    DT_ARG_PTR(c, 0, s);
}

static void setup_trace(DtCtx *c)
{
    tok_begin(c, dt_range(0, 12));
}

DT_CASES(ztoken_cases) = {
    {"ztokenOpen", 0x004a1120, ztokenOpen, 1, DT_RET_INT, setup_open, 100, 0},
    {"ztokenClose", 0x004a11e0, ztokenClose, 0, DT_RET_INT, setup_close, 50, 0},
    {"ztokenNext", 0x004a1220, ztokenNext, 2, DT_RET_INT, setup_next, 3000, 0},
    {"ztokenDefine", 0x004a20c0, ztokenDefine, 2, DT_RET_INT, setup_define, 600, 0},
    {"ztokenLookup", 0x004a22a0, ztokenLookup, 1, DT_RET_INT, setup_lookup, 400, 0},
    {"ztokenReadInt", 0x004a2330, ztokenReadInt, 1, DT_RET_INT, setup_read_int, 400, 0},
    {"ztokenReadString", 0x004a2440, ztokenReadString, 2, DT_RET_INT, setup_read_string, 200, 0},
    {"ztokenExpect", 0x004a2460, ztokenExpect, 1, DT_RET_INT, setup_expect, 300, 0},
    {"ztokenPrintTrace", 0x004a24f0, ztokenPrintTrace, 0, DT_RET_VOID, setup_trace, 100, 0},
};
