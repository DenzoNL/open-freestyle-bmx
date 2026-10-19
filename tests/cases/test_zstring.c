#include "dt.h"
#include "dt_zmem.h"
#include "zge/zstring.h"

/* ---- string sets ------------------------------------------------------------------------------ */

/* "STRS" blob with ids firstId..firstId+count-1. */
static ZStringSet *make_set(uint16_t firstId, uint16_t count, int goodMagic)
{
    const uint32_t header = 8 + 2u * count;
    ZStringSet *set = dt_alloc(header + 16u * count);
    uint32_t at = header;
    __builtin_memcpy(set->magic, goodMagic ? "STRS" : "STRX", 4);
    set->firstId = firstId;
    set->count = count;
    for (uint16_t i = 0; i < count; i++) {
        char *dst = (char *)set + at;
        const int len = dt_range(0, 12);
        set->offsets[i] = (uint16_t)at;
        for (int k = 0; k < len; k++) {
            dst[k] = (char)dt_range('a', 'z');
        }
        at += (uint32_t)len + 1;
    }
    return set;
}

static void fill_table_randomly(void)
{
    for (int i = 0; i < 16; i++) {
        gZstringSets[i] = dt_u32() & 1 ? (const ZStringSet *)(uintptr_t)dt_u32() : NULL;
    }
}

static void setup_string_init(DtCtx *c)
{
    dt_seed(c);
    fill_table_randomly();
}

static void setup_string_register(DtCtx *c)
{
    dt_seed(c);
    fill_table_randomly();
    DT_ARG_PTR(c, 0, make_set((uint16_t)dt_u32(), (uint16_t)dt_range(0, 8), c->seed % 7 != 3));
}

static void setup_string_unregister(DtCtx *c)
{
    ZStringSet *set;
    dt_seed(c);
    fill_table_randomly();
    set = make_set((uint16_t)dt_u32(), (uint16_t)dt_range(0, 8), 1);
    if (c->seed % 4 != 3) {
        gZstringSets[set->firstId >> 12] = set;
    }
    DT_ARG_PTR(c, 0, set);
}

static void setup_string_get(DtCtx *c)
{
    ZStringSet *set;
    uint16_t id;
    dt_seed(c);
    for (int i = 0; i < 16; i++) {
        gZstringSets[i] = NULL;
    }
    set = make_set((uint16_t)dt_range(0, 0xfff0), (uint16_t)dt_range(0, 10), 1);
    if (c->seed % 6 != 5) {
        gZstringSets[set->firstId >> 12] = set;
    }
    switch (c->seed % 4) {
    case 0: /* inside the set */
        id = (uint16_t)(set->firstId + (set->count ? dt_range(0, set->count - 1) : 0));
        break;
    case 1: /* around its edges */
        id = (uint16_t)(set->firstId + dt_range(-2, set->count + 2));
        break;
    default:
        id = (uint16_t)dt_u32();
    }
    DT_ARG_INT(c, 0, id);
}

/* ---- C string helpers ------------------------------------------------------------------------- */

static void setup_copy(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_alloc(160));
    DT_ARG_PTR(c, 1, dt_rand_str(100, 1));
}

static void setup_ncopy(DtCtx *c)
{
    char *src;
    dt_seed(c);
    src = dt_rand_str(40, 1);
    DT_ARG_PTR(c, 0, dt_alloc(100));
    DT_ARG_PTR(c, 1, src);
    DT_ARG_INT(c, 2, dt_range(0, 60));
}

static void setup_cat(DtCtx *c)
{
    char *dst, *head;
    dt_seed(c);
    dst = dt_alloc(160);
    head = dt_rand_str(60, 1);
    for (uint32_t i = 0; head[i]; i++) {
        dst[i] = head[i];
    }
    DT_ARG_PTR(c, 0, dst);
    DT_ARG_PTR(c, 1, dt_rand_str(60, 1));
}

static void setup_two_strings(DtCtx *c)
{
    char *a, *b;
    dt_seed(c);
    a = dt_rand_str(12, c->seed % 2);
    if (c->seed % 3 == 0) { /* equal strings */
        b = dt_alloc(64);
        for (uint32_t i = 0; a[i]; i++) {
            b[i] = a[i];
        }
    } else if (c->seed % 3 == 1) { /* shared prefix */
        b = dt_rand_str(12, c->seed % 2);
        for (uint32_t i = 0; a[i] && i < 3; i++) {
            b[i] = a[i];
        }
    } else {
        b = dt_rand_str(12, c->seed % 2);
    }
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
    DT_ARG_INT(c, 2, dt_range(0, 14));
}

static void setup_one_string(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_rand_str(40, c->seed % 2));
}

static void setup_chr(DtCtx *c)
{
    char *s;
    dt_seed(c);
    s = dt_rand_str(30, 0);
    DT_ARG_PTR(c, 0, s);
    switch (c->seed % 3) {
    case 0:
        DT_ARG_INT(c, 1, s[0] ? s[dt_range(0, 3) % 2] : 'a');
        break;
    case 1:
        DT_ARG_INT(c, 1, 0);
        break;
    default:
        DT_ARG_INT(c, 1, dt_range(0, 255));
    }
}

static void setup_toul(DtCtx *c)
{
    static const char *const samples[] = {"123",        "0x1F",        "  42abc", "-17", "zz",    "0777",
                                          "4294967295", "99999999999", "",        "+5",  "0b101", "\t 0Xff"};
    dt_seed(c);
    DT_ARG_PTR(c, 0, (void *)samples[c->seed % (sizeof samples / sizeof samples[0])]);
    DT_ARG_INT(c, 1, c->seed < 12 ? 0 : (uint32_t)dt_range(0, 36));
}

static void setup_upr(DtCtx *c)
{
    char *s;
    dt_seed(c);
    s = dt_rand_str(40, c->seed % 2);
    DT_ARG_PTR(c, 0, s);
}

static void setup_format(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_alloc(200));
    DT_ARG_PTR(c, 1, "v=%d hex=%x s=%s c=%c");
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
    DT_ARG_PTR(c, 4, dt_rand_str(20, 0));
    DT_ARG_INT(c, 5, dt_range('a', 'z'));
}

static void setup_format_temp(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, "%s/%d/%08x");
    DT_ARG_PTR(c, 1, dt_rand_str(30, 0));
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_pbrk(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_PTR(c, 0, dt_rand_str(20, 0));
    DT_ARG_PTR(c, 1, dt_rand_str(6, 0));
}

/* ---- memory helpers --------------------------------------------------------------------------- */

static void setup_memcopy(DtCtx *c)
{
    uint8_t *dst, *src;
    uint32_t n;
    dt_seed(c);
    dst = dt_alloc(160);
    src = dt_alloc(160);
    for (int i = 0; i < 160; i++) {
        src[i] = (uint8_t)dt_u32();
        dst[i] = (uint8_t)dt_u32();
    }
    n = (uint32_t)dt_range(0, 64);
    switch (c->seed % 4) { /* aligned, 2 byte aligned, odd, everything mixed */
    case 0:
        n &= ~3u;
        break;
    case 1:
        n &= ~1u;
        dst += 2;
        src += 2 * dt_range(0, 1);
        break;
    case 2:
        dst += 1;
        break;
    default:
        dst += dt_range(0, 3);
        src += dt_range(0, 3);
    }
    DT_ARG_PTR(c, 0, dst);
    DT_ARG_PTR(c, 1, src);
    DT_ARG_INT(c, 2, n);
}

static void setup_memfill(DtCtx *c)
{
    uint8_t *dst;
    dt_seed(c);
    dst = dt_alloc(100);
    for (int i = 0; i < 100; i++) {
        dst[i] = (uint8_t)dt_u32();
    }
    DT_ARG_PTR(c, 0, dst + dt_range(0, 3));
    DT_ARG_INT(c, 1, dt_u32()); /* only the low byte counts */
    DT_ARG_INT(c, 2, dt_range(0, 70));
}

/* ---- name tables ------------------------------------------------------------------------------ */

static uint32_t handler_log[4];

static void BMX_CDECL handler(uint32_t a, uint32_t b)
{
    handler_log[0]++;
    handler_log[1] = a;
    handler_log[2] = b;
}

static const char *const name_pool[] = {"lightning", "spark",   "shard",     "splash", "poster",
                                        "ripple",    "gravity", "floorcoll", "SPARK",  "Splash"};
#define NAME_COUNT ((int)(sizeof name_pool / sizeof name_pool[0]))

/* Builds the three tables with the original code and fills them with random entries. */
static void make_tables(int populate)
{
    dt_zmem_env();
    DT_ORIG(0x00482bd0, void (*)(void))();
    if (!populate) {
        return;
    }
    for (int i = dt_range(0, 12); i > 0; i--) {
        DT_ORIG(0x00482d00, void (*)(void *, const char *))((void *)(uintptr_t)dt_u32(),
                                                            name_pool[dt_range(0, NAME_COUNT - 1)]);
    }
    for (int i = dt_range(0, 5); i > 0; i--) {
        DT_ORIG(0x00482df0, void (*)(void *, const char *))((void *)(uintptr_t)dt_u32(),
                                                            name_pool[dt_range(0, NAME_COUNT - 1)]);
    }
    for (int i = dt_range(1, 6); i > 0; i--) {
        DT_ORIG(0x00482f20, void (*)(void *, const char *))((void *)handler, name_pool[dt_range(0, 3)]);
    }
}

static void setup_names_init(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    gNameTablesReady = (int)(c->seed % 2);
}

static void setup_names_shutdown(DtCtx *c)
{
    dt_seed(c);
    make_tables(1);
    if (c->seed % 5 == 4) {
        gNameTablesReady = 0; /* nothing happens */
    }
}

static void setup_name_add(DtCtx *c)
{
    dt_seed(c);
    make_tables(1);
    DT_ARG_PTR(c, 0, c->seed % 2 ? (void *)handler : (void *)(uintptr_t)dt_u32());
    DT_ARG_PTR(c, 1, c->seed % 4 == 3 ? dt_rand_str(60, 0) : (char *)name_pool[dt_range(0, NAME_COUNT - 1)]);
}

static void setup_name_find(DtCtx *c)
{
    dt_seed(c);
    make_tables(1);
    DT_ARG_PTR(c, 0, c->seed % 5 == 4 ? "missing" : name_pool[dt_range(0, NAME_COUNT - 1)]);
}

static void setup_name_call(DtCtx *c)
{
    dt_seed(c);
    make_tables(1);
    DT_ARG_PTR(c, 0, c->seed % 5 == 4 ? "missing" : name_pool[dt_range(0, 3)]);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

DT_CASES(zstring_cases) = {
    {"zstringInit", 0x00482420, zstringInit, 0, DT_RET_INT, setup_string_init, 20, 0},
    {"zstringRegister", 0x00482450, zstringRegister, 1, DT_RET_INT, setup_string_register, 100, 0},
    {"zstringUnregister", 0x004824a0, zstringUnregister, 1, DT_RET_INT, setup_string_unregister, 100, 0},
    {"zstringGet", 0x004824f0, zstringGet, 1, DT_RET_INT, setup_string_get, 300, 0},
    {"zstrcpy", 0x00482700, zstrcpy, 2, DT_RET_INT, setup_copy, 200, 0},
    {"zstrncpy", 0x00482730, zstrncpy, 3, DT_RET_INT, setup_ncopy, 200, 0},
    {"zstrcat", 0x00482750, zstrcat, 2, DT_RET_INT, setup_cat, 200, 0},
    {"zstrcmp", 0x00482790, zstrcmp, 2, DT_RET_INT, setup_two_strings, 300, 0},
    {"zstrlen", 0x004827d0, zstrlen, 1, DT_RET_INT, setup_one_string, 200, 0},
    {"zstrchr", 0x004827f0, zstrchr, 2, DT_RET_INT, setup_chr, 300, 0},
    {"zstrtoul", 0x00482810, zstrtoul, 2, DT_RET_INT, setup_toul, 120, 0},
    {"zstrupr", 0x00482830, zstrupr, 1, DT_RET_INT, setup_upr, 200, 0},
    {"zstrFormat", 0x00482840, zstrFormat, 6, DT_RET_INT, setup_format, 100, 0},
    {"zstrFormatTemp", 0x00482860, zstrFormatTemp, 4, DT_RET_INT, setup_format_temp, 100, 0},
    {"zstrcmpi", 0x00482880, zstrcmpi, 2, DT_RET_INT, setup_two_strings, 300, 0},
    {"zstrncmp", 0x004828a0, zstrncmp, 3, DT_RET_INT, setup_two_strings, 300, 0},
    {"zstrHash", 0x004828c0, zstrHash, 1, DT_RET_INT, setup_one_string, 300, 0},
    {"zstrpbrk", 0x00482930, zstrpbrk, 2, DT_RET_INT, setup_pbrk, 300, 0},
    {"zmemCopy", 0x00482a50, zmemCopy, 3, DT_RET_INT, setup_memcopy, 400, 0},
    {"zmemFill", 0x00482b90, zmemFill, 3, DT_RET_INT, setup_memfill, 200, 0},
    {"zstrNameTablesInit", 0x00482bd0, zstrNameTablesInit, 0, DT_RET_VOID, setup_names_init, 20, 0},
    {"zstrNameTablesShutdown", 0x00482c10, zstrNameTablesShutdown, 0, DT_RET_VOID, setup_names_shutdown, 40,
     0},
    {"zstrNameTable0Add", 0x00482d00, zstrNameTable0Add, 2, DT_RET_VOID, setup_name_add, 100, 0},
    {"zstrNameTable0Find", 0x00482da0, zstrNameTable0Find, 1, DT_RET_INT, setup_name_find, 100, 0},
    {"zstrNameTable1Add", 0x00482df0, zstrNameTable1Add, 2, DT_RET_VOID, setup_name_add, 100, 0},
    {"zstrNameTable1Find", 0x00482e90, zstrNameTable1Find, 1, DT_RET_INT, setup_name_find, 100, 0},
    {"zstrNameTable2Add", 0x00482f20, zstrNameTable2Add, 2, DT_RET_VOID, setup_name_add, 100, 0},
    {"zstrNameTable2Call", 0x00482fc0, zstrNameTable2Call, 3, DT_RET_VOID, setup_name_call, 100, 0},
};
