/* Strings (C:\NgEngine\Zge\zstring.c): string-table sets, CRT-style helpers and the name tables. */
#ifndef ZGE_ZSTRING_H
#define ZGE_ZSTRING_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"
#include "zge/zlist.h"

/*
 * A string table ("STRS" blob): ids firstId .. firstId+count-1 map to NUL-terminated strings stored at
 * byte offsets from the start of the blob. The top nibble of an id selects one of 16 registered sets.
 */
typedef struct ZStringSet {
    char magic[4];
    uint16_t firstId;
    uint16_t count;
    uint16_t offsets[]; /* count entries */
} ZStringSet;

typedef const ZStringSet *ZStringSetTable[16];
#define gZstringSets ORIG_VAR(0x006704d8, ZStringSetTable, gZstringSets)

/* Scratch buffer returned by zstrFormatTemp. */
typedef char ZstrTempBuffer[256];
#define gZstrTempBuffer ORIG_VAR(0x006703d8, ZstrTempBuffer, gZstrTempBuffer)

/* Entry of a name table: a value (or handler) registered under a case-sensitive name. */
typedef struct ZNameEntry {
    void *value;
    char name[64];
    int hash; /* zstrHash(name) */
} ZNameEntry;

/* Handler stored in name table 2, invoked by zstrNameTable2Call (the effect creators: object, parameters). */
typedef void(BMX_CDECL *ZNameHandlerFn)(const void *a, void *b);

typedef ZList *ZNameTables[3];
#define gNameTablesReady ORIG_VAR(0x00670518, int, gNameTablesReady)
#define gNameTables      ORIG_VAR(0x0067051c, ZNameTables, gNameTables)

int zstringInit(void);
int zstringRegister(const ZStringSet *set);
int zstringUnregister(const ZStringSet *set);
const char *zstringGet(uint16_t id);

char *zstrcpy(char *dst, const char *src);
char *zstrncpy(char *dst, const char *src, size_t n);
char *zstrcat(char *dst, const char *src);
int zstrcmp(const char *a, const char *b);
int zstrlen(const char *s);
char *zstrchr(const char *s, int c);
unsigned long zstrtoul(const char *s, int base);
char *zstrupr(char *s);
int zstrFormat(char *dst, const char *fmt, ...);
char *zstrFormatTemp(const char *fmt, ...);
int zstrcmpi(const char *a, const char *b);
int zstrncmp(const char *a, const char *b, size_t n);
int zstrHash(const char *s);
char *zstrpbrk(const char *s, const char *set);

void *zmemCopy(void *dst, const void *src, uint32_t n);
void *zmemFill(void *dst, uint8_t value, uint32_t n);

void zstrNameTablesInit(void);
void zstrNameTablesShutdown(void);
void zstrNameTable0Add(void *value, const char *name);
void *zstrNameTable0Find(const char *name);
void zstrNameTable1Add(void *value, const char *name);
void *zstrNameTable1Find(const char *name);
void zstrNameTable2Add(ZNameHandlerFn handler, const char *name);
void zstrNameTable2Call(const char *name, const void *a, void *b);

#endif
