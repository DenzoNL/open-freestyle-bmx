/* Renderer world (C:\NgEngine\Zge\Com\ZglRw\zrwworld.c). Not ported yet; callees only. */
#ifndef ZGE_COM_ZGLRW_ZRWWORLD_H
#define ZGE_COM_ZGLRW_ZRWWORLD_H

#include <stdint.h>

#include "compat/orig.h"

/* World interface, function pointers in the exe's data (0x5620e0..). */
typedef int(BMX_CDECL *ZrwWorldLoadFn)(void *data, const char *directory);
typedef void(BMX_CDECL *ZrwWorldCloseFn)(void);
/* A scene component of the file image: returns a record {?, hasFile, name...} and its size, or NULL. */
typedef uint8_t *(BMX_CDECL *ZrwWorldFindFn)(void *data, int kind, uint32_t *size);
#define gZrwWorldLoad  ORIG_VAR(0x005620e0, ZrwWorldLoadFn, gZrwWorldLoad)
#define gZrwWorldClose ORIG_VAR(0x005620e4, ZrwWorldCloseFn, gZrwWorldClose)
#define gZrwWorldFind  ORIG_VAR(0x005620e8, ZrwWorldFindFn, gZrwWorldFind)

/* Creates a world object (type 5); NULL on failure. */
ORIG_FN(0x00495c10, void *, zrwworldCreateObject, (void));

#endif
