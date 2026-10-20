/*
 * A fake file device that serves text files from memory, for tests of code built on zfile (the tokenizer,
 * the scene loader). Paths may carry its "tx00:" prefix. The debug prints are logged.
 */
#ifndef DT_ZFILE_H
#define DT_ZFILE_H

#include <stdint.h>

#include "compat/crt.h"
#include "dt.h"
#include "dt_zmem.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"

enum { kDtFiles = 8, kDtCursors = 16 };

typedef struct DtFileEntry {
    const char *path;
    const char *text;
} DtFileEntry;

typedef struct DtCursor {
    const char *text;
    uint32_t pos, len;
    int used;
} DtCursor;

static DtFileEntry gDtFiles[kDtFiles];
static DtCursor gDtCursors[kDtCursors];
static ZfileDevice gDtDevice, gDtDummy;
static uint32_t gDtFileLog[1024];
static uint32_t gDtFileLogN;

static void dtLog(uint32_t v)
{
    if (gDtFileLogN < sizeof gDtFileLog / sizeof gDtFileLog[0]) {
        gDtFileLog[gDtFileLogN] = v;
    }
    gDtFileLogN++;
}

static void dtLogStr(const char *s)
{
    uint32_t h = 5381;
    while (*s) {
        h = h * 33 + (uint8_t)*s++;
    }
    dtLog(h);
}

static int dtSameName(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
    }
    return *a == *b;
}

static const char *BMX_CDECL dtDevName(ZfileDevice *d)
{
    return d == &gDtDummy ? "dm00" : "tx00";
}

static int BMX_CDECL dtDevOpen(ZfileHandle *f, const char *path, int mode, int flags)
{
    int i, c;
    dtLog(0xd01);
    dtLogStr(path);
    for (i = 0; i < kDtFiles; i++) {
        if (gDtFiles[i].path && dtSameName(gDtFiles[i].path, path)) {
            for (c = 0; c < kDtCursors; c++) {
                if (!gDtCursors[c].used) {
                    gDtCursors[c].used = 1;
                    gDtCursors[c].text = gDtFiles[i].text;
                    gDtCursors[c].pos = 0;
                    gDtCursors[c].len = (uint32_t)strlen(gDtFiles[i].text);
                    f->data = &gDtCursors[c];
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int BMX_CDECL dtDevClose(ZfileHandle *f, int flags)
{
    DtCursor *c = f->data;
    dtLog(0xd02);
    if (c) {
        c->used = 0;
    }
    return 1;
}

static int BMX_CDECL dtDevRead(ZfileHandle *f, void *buf, int count, int flags)
{
    DtCursor *c = f->data;
    uint8_t *p = buf;
    int n = 0;
    while (n < count && c->pos < c->len) {
        p[n++] = (uint8_t)c->text[c->pos++];
    }
    return n;
}

static uint32_t BMX_CDECL dtDevSize(ZfileHandle *f, int *size, uint32_t flags)
{
    *size = (int)((DtCursor *)f->data)->len;
    return 1;
}

/* Debug print stand-in: logs the format and its arguments (%s as text hash, %d / %x / %X as value). */
static void BMX_CDECL dtPrint(const char *fmt, ...)
{
    const uint32_t *arg = (const uint32_t *)((const uint8_t *)&fmt + sizeof fmt);
    const char *p;
    dtLog(0xd10);
    dtLogStr(fmt);
    for (p = fmt; *p; p++) {
        if (*p == '%') {
            p++;
            if (*p == 's') {
                dtLogStr((const char *)(uintptr_t)*arg++);
            } else if (*p == 'd' || *p == 'x' || *p == 'X') {
                dtLog(*arg++);
            }
        }
    }
}

static void dtFileReset(void)
{
    memset(gDtFiles, 0, sizeof gDtFiles);
    memset(gDtCursors, 0, sizeof gDtCursors);
    gDtFileLogN = 0;
}

static void dtFileAdd(const char *path, const char *text)
{
    int i;
    for (i = 0; i < kDtFiles; i++) {
        if (!gDtFiles[i].path) {
            gDtFiles[i].path = path;
            gDtFiles[i].text = text;
            return;
        }
    }
}

/*
 * Installs the fake device as the default device of an initialised file layer. A dummy device sits at
 * index 0, because the device lookup asserts when a path names device 0.
 */
static void dtFileInstall(void)
{
    gDtDevice.name = dtDevName;
    gDtDevice.open = dtDevOpen;
    gDtDevice.close = dtDevClose;
    gDtDevice.read = dtDevRead;
    gDtDevice.getSize = dtDevSize;
    dt_zmem_env();
    gZdebugPrint = dtPrint;
    memset(&gZfileDevices, 0, sizeof gZfileDevices);
    gDtDummy.name = dtDevName;
    gZfileDevices.count = 2;
    gZfileDevices.device[0] = &gDtDummy;
    gZfileDevices.device[1] = &gDtDevice;
    gZfileDefaultDevice = &gDtDevice;
    gZfileInitialized = 1;
    gZfileHandles = DT_ORIG(0x00485e80, ZMemFreeList * (*)(uint32_t, uint32_t, void *))(8, 8, NULL);
    gZfileLoadPool = NULL;
    gZfileErrorMessage[0] = '\0';
}

#endif
