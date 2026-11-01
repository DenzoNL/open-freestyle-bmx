/*
 * Profiler and the renderer's file callbacks (C:\NgEngine\Zge\zprofile.c). The file callbacks replace
 * RenderWare's file interface while a texture is read from memory (see zrwramdisk.h): every call goes to the
 * zfile layer through the handle slot RenderWare passes in.
 */
#ifndef ZGE_ZPROFILE_H
#define ZGE_ZPROFILE_H

#include <stdint.h>

#include "compat/orig.h"
#include "rw/rwstream.h"
#include "zge/com/zglrw/zrwramdisk.h"
#include "zge/zfile.h"

/* RenderWare's file interface: ten function pointers (open, close, read, write, gets, puts, eof, seek,
 * flush). */
typedef void *ZrwFileInterface[10];
#define gZrwSavedFileInterface ORIG_VAR(0x00676410, ZrwFileInterface, gZrwSavedFileInterface)
/* Storage the RAM disk info pointers point into while the callbacks are installed. */
typedef ZrwRamDiskInfo ZrwRamDiskInfoSlots[2];
#define gZrwRamDiskInfoSlots ORIG_VAR(0x006763ec, ZrwRamDiskInfoSlots, gZrwRamDiskInfoSlots)

/*
 * State of the profiler (0x676438, 0x68 bytes, cleared at initialisation). The update draws two bars per
 * entry (the peak and this frame's time, scaled so the entry's budget fills the screen width less the
 * margins) with tick marks along them, from (x, y) downwards.
 */
typedef struct ZProfileState {
    int initialized;            /* 0x00 */
    void *entries;              /* 0x04 free list of ZProfileEntry */
    void *updateNode;           /* 0x08 per-frame update registration */
    int32_t holdFrames;         /* 0x0c 10: frames before the peaks are tracked and drawn */
    int32_t peakResetCountdown; /* 0x10 */
    int32_t peakResetFrames;    /* 0x14 copy of limit60: the peaks are cleared this often (0: never) */
    float x;                    /* 0x18 32.0 */
    float y;                    /* 0x1c 32.0 */
    float scale20;              /* 0x20 2.0 */
    uint8_t pad24[0x14];        /* 0x24 */
    uint32_t margin;            /* 0x38 0x20 */
    int32_t zero3c;             /* 0x3c */
    int32_t zero40;             /* 0x40 */
    uint32_t barHeight;         /* 0x44 8: height of the peak bar */
    uint32_t timeBarHeight;     /* 0x48 6: height of the time bar, centred on the peak bar */
    uint32_t barGap;            /* 0x4c 2 */
    uint32_t tickAbove;         /* 0x50 4: the tick marks reach this far above the bars */
    uint32_t tickWidth;         /* 0x54 8 */
    uint32_t tickBelow;         /* 0x58 4 */
    int32_t limit5c;            /* 0x5c 2 */
    int32_t limit60;            /* 0x60 0x78 */
    uint32_t unitsPerSecond;    /* 0x64 0x3c: the budgets are in 1/60 s */
} ZProfileState;

BMX_LAYOUT32_ASSERT(sizeof(ZProfileState) == 0x68, "profile state");

#define gZProfile ORIG_VAR(0x00676438, ZProfileState, gZProfile)

/* A profiled section (payload of the free list, 0x24 bytes). */
typedef struct ZProfileEntry {
    uint32_t flags;   /* 0x00 bit 0: running */
    uint32_t pad04;   /* 0x04 */
    const char *name; /* 0x08 printed beside the bars when set */
    uint32_t ticks;   /* 0x0c counted this frame */
    uint32_t peak;    /* 0x10 */
    float budget;     /* 0x14 units the full bar width stands for */
    uint32_t color;   /* 0x18 of the time bar */
    int32_t start[2]; /* 0x1c counter when the section was started */
} ZProfileEntry;

BMX_LAYOUT32_ASSERT(sizeof(ZProfileEntry) == 0x24, "profile entry");

/* Callbacks of the file interface; `file` is the slot RenderWare keeps the handle in. */
int zrwfclose(ZfileHandle **file);
int zrwfread(void *buffer, int size, int count, ZfileHandle **file);
int zrwfwrite(const void *buffer, int size, int count, ZfileHandle **file);
char *zrwfgets(char *buffer, int size, ZfileHandle **file);
int zrwfflush(void);
int zrwfputs(void);
int zrwfeof(ZfileHandle **file);
uint32_t zrwfseek(ZfileHandle **file, uint32_t a, uint32_t b);

/* Installs the callbacks into RenderWare's interface (keeping a copy of the old one), resets the RAM disk
 * state. */
void RegisterCallBacksForFileSystem(void);
/* Appends a memory block to the RAM disk info list. */
void ImageBufferSizeType(const void *address, uint32_t size, const char *extension);
/* Restores RenderWare's own file interface. */
void zrwFileRamDiskClear(void);

int zprofileInitialize(void);
/*
 * Per-frame update (render list 4): adds the running sections' time, tracks the peaks and draws the
 * profile bars; returns 1.
 */
int zprofileUpdate(void *arg);

#endif
