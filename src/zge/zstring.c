/* Strings (C:\NgEngine\Zge\zstring.c): string-table sets, CRT-style helpers and the name tables. */
#include "zge/zstring.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"

static const char zstringFile[] = "C:\\NgEngine\\Zge\\zstring.c";

/* Tag at the start of every string set. */
static const char kZstringMagic[4] = {'S', 'T', 'R', 'S'};

/* Clears the table of registered string sets. */
BMX_ADDR(0x00482420)
int zstringInit(void)
{
    for (int i = 0; i < 16; i++) {
        gZstringSets[i] = NULL;
    }
    return 1;
}

/* Registers a set in the slot selected by the top nibble of its first id. */
BMX_ADDR(0x00482450)
int zstringRegister(const ZStringSet *set)
{
    if (strncmp(set->magic, kZstringMagic, 4) == 0) {
        gZstringSets[set->firstId >> 12] = set;
        return 1;
    }
    zdebugFail("Error", zstringFile, 0x9a, 0);
    return 0;
}

BMX_ADDR(0x004824a0)
int zstringUnregister(const ZStringSet *set)
{
    const unsigned slot = set->firstId >> 12;
    if (gZstringSets[slot] == set) {
        gZstringSets[slot] = NULL;
        return 1;
    }
    zdebugFail("Error", zstringFile, 0xbd, 0);
    return 0;
}

/* Looks an id up in its registered set. */
BMX_ADDR(0x004824f0)
const char *zstringGet(uint16_t id)
{
    const unsigned slot = id >> 12;
    const ZStringSet *set = gZstringSets[slot];
    if (set != NULL && id >= set->firstId && (int)id < (int)set->count + set->firstId) {
        return (const char *)set + set->offsets[id - set->firstId];
    }
    zdebugFail(zstrFormatTemp("String(0x%x) not found in set:%d", (unsigned)id, slot), zstringFile, 0xeb, 0);
    return NULL;
}

BMX_ADDR(0x00482700)
char *zstrcpy(char *dst, const char *src)
{
    return strcpy(dst, src); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)
}

BMX_ADDR(0x00482730)
char *zstrncpy(char *dst, const char *src, size_t n)
{
    return strncpy(dst, src, n);
}

BMX_ADDR(0x00482750)
char *zstrcat(char *dst, const char *src)
{
    char *end = dst + strlen(dst);
    memcpy(end, src, strlen(src) + 1);
    return dst;
}

/* Returns -1, 0 or 1 (not the byte difference), comparing unsigned bytes. */
BMX_ADDR(0x00482790)
int zstrcmp(const char *a, const char *b)
{
    const uint8_t *x = (const uint8_t *)a;
    const uint8_t *y = (const uint8_t *)b;
    for (;; x++, y++) {
        if (*x != *y) {
            return *x < *y ? -1 : 1;
        }
        if (*x == 0) {
            return 0;
        }
    }
}

BMX_ADDR(0x004827d0)
int zstrlen(const char *s)
{
    return (int)strlen(s);
}

BMX_ADDR(0x004827f0)
char *zstrchr(const char *s, int c)
{
    return strchr(s, c);
}

/* strtoul; the original passes the address of its own stack argument as the end pointer. */
BMX_ADDR(0x00482810)
unsigned long zstrtoul(const char *s, int base)
{
    char *end;
    return strtoul(s, &end, base);
}

BMX_ADDR(0x00482830)
char *zstrupr(char *s)
{
    return _strupr(s);
}

/* sprintf into dst; returns the character count. */
BMX_ADDR(0x00482840)
int zstrFormat(char *dst, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int n = vsprintf(dst, fmt, args);
    va_end(args);
    return n;
}

/* sprintf into the shared 256 byte scratch buffer and returns it (no length check). */
BMX_ADDR(0x00482860)
char *zstrFormatTemp(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsprintf(gZstrTempBuffer, fmt, args);
    va_end(args);
    return gZstrTempBuffer;
}

BMX_ADDR(0x00482880)
int zstrcmpi(const char *a, const char *b)
{
    return _stricmp(a, b);
}

/* strncmp; like the MSVC6 CRT it returns -1, 0 or 1. */
BMX_ADDR(0x004828a0)
int zstrncmp(const char *a, const char *b, size_t n)
{
    return strncmp(a, b, n);
}

/*
 * Case-insensitive name hash: a position-weighted sum of the upper-cased characters, then a second
 * pass mixing each character with its predecessor at bit 16. Characters are signed.
 */
BMX_ADDR(0x004828c0)
int zstrHash(const char *s)
{
    uint32_t hash = 0;
    signed char prev = 0;
    uint32_t weight = 1;
    for (const char *p = s; *p != '\0'; p++) {
        hash += (uint32_t)toupper(*p) * weight;
        weight++;
    }
    for (const char *p = s; *p != '\0'; p++) {
        hash += ((uint32_t)toupper(*p) + (uint32_t)prev * 0xffffu) << 16;
        prev = (signed char)toupper(*p);
    }
    return (int)hash;
}

/* strpbrk. No callers in the exe (only reachable through pointers). */
BMX_ADDR(0x00482930)
char *zstrpbrk(const char *s, const char *set)
{
    const int slen = zstrlen(s);
    const int setLen = zstrlen(set);
    for (int i = 0; i < slen; i++) {
        for (int j = 0; j < setLen; j++) {
            if (s[i] == set[j]) {
                return (char *)s + i;
            }
        }
    }
    return NULL;
}

/*
 * Forward copy in the widest unit that dst, src and n are all aligned to (4, 2 or 1 bytes).
 * Returns dst.
 */
BMX_ADDR(0x00482a50)
void *zmemCopy(void *dst, const void *src, uint32_t n)
{
    const uintptr_t any = (uintptr_t)dst | (uintptr_t)src | n;
    if ((any & 3) == 0) {
        uint32_t *d = dst;
        const uint32_t *s = src;
        for (uint32_t i = n >> 2; i != 0; i--) {
            *d++ = *s++;
        }
    } else if ((any & 1) == 0) {
        uint16_t *d = dst;
        const uint16_t *s = src;
        for (uint32_t i = n >> 1; i != 0; i--) {
            *d++ = *s++;
        }
    } else {
        uint8_t *d = dst;
        const uint8_t *s = src;
        for (int32_t i = (int32_t)n; i > 0; i--) {
            *d++ = *s++;
        }
    }
    return dst;
}

/* memset with a byte value. Returns dst. */
BMX_ADDR(0x00482b90)
void *zmemFill(void *dst, uint8_t value, uint32_t n)
{
    return memset(dst, value, n);
}

/*
 * TODO(file): the name tables below probably belong to a separate effects/definitions module, not
 * zstring.c. Three lists of ZNameEntry: 0 and 1 map names to data, 2 maps names to handlers.
 */

BMX_ADDR(0x00482bd0)
void zstrNameTablesInit(void)
{
    gNameTables[0] = zlistCreate(0x40);
    gNameTables[1] = zlistCreate(0x10);
    gNameTables[2] = zlistCreate(0x10);
    gNameTablesReady = 1;
}

static void zstrNameTableDrain(ZList *table)
{
    void *entry = zlistFirst(table);
    while (entry != NULL) {
        zmemFree(entry);
        zlistRemoveCurrent(table);
        entry = zlistFirst(table);
    }
}

/* Frees every entry and the three lists. */
BMX_ADDR(0x00482c10)
void zstrNameTablesShutdown(void)
{
    if (gNameTablesReady != 0) {
        for (int i = 0; i < 3; i++) {
            zstrNameTableDrain(gNameTables[i]);
            zlistDestroy(gNameTables[i]);
        }
        gNameTablesReady = 0;
    }
}

static void zstrNameTableAdd(ZList *table, void *value, const char *name)
{
    ZNameEntry *entry = zmemAlloc(sizeof *entry);
    entry->value = value;
    zstrcpy(entry->name, name);
    entry->hash = zstrHash(entry->name);
    zlistInsert(table, entry);
}

static ZNameEntry *zstrNameTableFindEntry(ZList *table, const char *name)
{
    ZNameEntry *entry = zlistFirst(table);
    while (entry != NULL) {
        if (zstrcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = zlistNext(table);
    }
    return NULL;
}

static void *zstrNameTableFind(ZList *table, const char *name)
{
    const ZNameEntry *entry = zstrNameTableFindEntry(table, name);
    return entry != NULL ? entry->value : NULL;
}

BMX_ADDR(0x00482d00)
void zstrNameTable0Add(void *value, const char *name)
{
    zstrNameTableAdd(gNameTables[0], value, name);
}

BMX_ADDR(0x00482da0)
void *zstrNameTable0Find(const char *name)
{
    return zstrNameTableFind(gNameTables[0], name);
}

BMX_ADDR(0x00482df0)
void zstrNameTable1Add(void *value, const char *name)
{
    zstrNameTableAdd(gNameTables[1], value, name);
}

BMX_ADDR(0x00482e90)
void *zstrNameTable1Find(const char *name)
{
    return zstrNameTableFind(gNameTables[1], name);
}

BMX_ADDR(0x00482f20)
void zstrNameTable2Add(ZNameHandlerFn handler, const char *name)
{
    zstrNameTableAdd(gNameTables[2], (void *)handler, name);
}

/* Calls the handler registered under name (if any) with a and b. */
BMX_ADDR(0x00482fc0)
void zstrNameTable2Call(const char *name, uint32_t a, uint32_t b)
{
    const ZNameEntry *entry = zstrNameTableFindEntry(gNameTables[2], name);
    if (entry != NULL) {
        ((ZNameHandlerFn)entry->value)(a, b);
    }
}
