#include "dt_zprim.h"

#include "compat/crt.h"

DtRwLog dt_rw_log;

int32_t BMX_CDECL dt_rw_load(const void *bmp, uint32_t size, const char *name, int a, int b)
{
    const uint32_t slot = dt_rw_log.loads % 8;
    dt_rw_log.hash[slot] = dt_fnv(bmp, size) ^ (uint32_t)(a + b * 7);
    dt_rw_log.sizes[slot] = size;
    for (int i = 0; i < 15; i++) {
        dt_rw_log.names[slot][i] = name[i];
        if (name[i] == 0) {
            break;
        }
    }
    dt_rw_log.loads++;
    return (int32_t)(0x1000 + dt_rw_log.loads);
}

void BMX_CDECL dt_rw_free(int32_t texture)
{
    dt_rw_log.frees++;
    dt_rw_log.freeSum += (uint32_t)texture;
}

int32_t BMX_CDECL dt_rw_clone(int32_t texture)
{
    dt_rw_log.clones++;
    return texture + 0x100;
}

static const uint8_t *fake_file;
static uint32_t fake_pos;

static int BMX_CDECL fake_read(void *handle, void *dst, uint32_t n, int flags)
{
    memcpy(dst, fake_file + fake_pos, n);
    fake_pos += n;
    return (int)n;
}

static void BMX_CDECL fake_seek(void *handle, uint32_t pos, int a, int b)
{
    fake_pos = pos;
}

void dt_fake_lib_begin(DtFakeLib *l, uint32_t maxEntries, uint32_t maxBytes)
{
    uint32_t *vtab = dt_alloc(0x40);
    uint8_t *handle = dt_alloc(0x20);
    vtab[0x28 / 4] = (uint32_t)(uintptr_t)fake_read;
    vtab[0x30 / 4] = (uint32_t)(uintptr_t)fake_seek;
    *(uint32_t *)(handle + 4) = (uint32_t)(uintptr_t)vtab;
    l->lib = dt_alloc(0x120);
    *(uint32_t *)l->lib = (uint32_t)(uintptr_t)handle;
    *(uint16_t *)(l->lib + 8) = 0;
    *(uint16_t *)(l->lib + 0xa) = 1;
    *(uint32_t *)(l->lib + 0x10) = (uint32_t)(uintptr_t)dt_alloc(maxEntries * 12);
    *(uint16_t *)(l->lib + 0x18) = 8;
    *(uint16_t *)(l->lib + 0x1a) = 0;
    l->fileSize = 0x10 + maxBytes;
    l->file = dt_alloc(l->fileSize);
    l->used = 0x10;
    l->count = 0;
    fake_file = l->file;
    fake_pos = 0;
}

void dt_fake_lib_add(DtFakeLib *l, uint32_t id, const void *data, uint32_t size)
{
    uint32_t *entry = (uint32_t *)(*(uint32_t *)(l->lib + 0x10) + l->count * 12);
    entry[0] = id;
    entry[1] = l->used - 0x10;
    entry[2] = size;
    memcpy(l->file + l->used, data, size);
    l->used += (size + 3) & ~3u;
    l->count++;
    *(uint16_t *)(l->lib + 8) = (uint16_t)l->count;
}
