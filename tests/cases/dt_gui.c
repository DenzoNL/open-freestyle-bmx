#include "dt_gui.h"

#include <stdarg.h>

#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/menu/gui/guitoggl.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "compat/crt.h"

/* Handle heap bookkeeping of the original zmemdmm module (see zmemdmmInit at 0x46aca0). */
#define DMM_LIMIT   ORIG_VAR(0x005f8f30, uintptr_t, DMM_LIMIT)
#define DMM_UNKNOWN ORIG_VAR(0x005f8f34, uint32_t, DMM_UNKNOWN)
#define DMM_FREE    ORIG_VAR(0x005f8f38, uint32_t *, DMM_FREE)
#define DMM_SIZE    ORIG_VAR(0x005f8f40, uint32_t, DMM_SIZE)
#define DMM_BASE    ORIG_VAR(0x005f8f44, uint8_t *, DMM_BASE)
#define DMM_LOCKS   ORIG_VAR(0x005f8f48, uint32_t, DMM_LOCKS)
#define DMM_TABLE   ORIG_VAR(0x005f8f4c, uintptr_t, DMM_TABLE)
#define DMM_ARENA   ORIG_VAR(0x005f8f50, uint8_t *, DMM_ARENA)
#define DMM_READY   ORIG_VAR(0x005f8f54, uint32_t, DMM_READY)

/* Asset cache (list of records {u16 id, next at +4, handle at +8, offset at +0xc}) and zprim state. */
#define ZASSET_LIST   ORIG_VAR(0x0067840c, void *, ZASSET_LIST)
#define ZPRIM_STATE   ORIG_VAR(0x00678430, void *, ZPRIM_STATE)
#define ZPRIM_CURRENT ORIG_VAR(0x0067842c, uint32_t, ZPRIM_CURRENT)
#define ZPRIM_CLIP    ORIG_VAR(0x00678414, int16_t, ZPRIM_CLIP)
/* GPU primitive buffer (0x7c byte packets) and the ordering table pointer the packets are linked into. */
#define GPU_PRIMS ORIG_VAR(0x005d2510, uint8_t *, GPU_PRIMS)
#define GPU_OT    ORIG_VAR(0x0068c3d0, uint8_t *, GPU_OT)

static uint32_t dbgCount;
static uint32_t dbgHash;
static uint32_t procCount;
static uint32_t procLog[96];

void BMX_CDECL dtGuiDebugPrint(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dbgCount++;
    for (const char *p = fmt; *p != '\0'; p++) {
        dbgHash = dbgHash * 31 + (uint8_t)*p;
        if (*p == '%') {
            if (p[1] == '%') {
                p++;
                continue;
            }
            dbgHash = dbgHash * 31 + (uint32_t)va_arg(ap, int);
        }
    }
    va_end(ap);
}

static void logProc(uint32_t who, int msg, GuiElement *el, int arg3, int arg4)
{
    const uint32_t at = (procCount * 5) % 90;
    procLog[at] = who;
    procLog[at + 1] = (uint32_t)msg;
    procLog[at + 2] = (uint32_t)(uintptr_t)el;
    procLog[at + 3] = (uint32_t)arg3;
    procLog[at + 4] = (uint32_t)arg4;
    procCount++;
}

int BMX_CDECL dtGuiProc(int msg, GuiElement *el, int arg3, int arg4)
{
    logProc(1, msg, el, arg3, arg4);
    return (int)(((uint32_t)msg * 2654435761u >> 28) & 1);
}

int BMX_CDECL dtGuiProcOther(int msg, GuiElement *el, int arg3, int arg4)
{
    logProc(2, msg, el, arg3, arg4);
    return msg & 1;
}

int BMX_CDECL dtGuiHook(int msg, GuiElement *el, int arg3, int arg4)
{
    logProc(3, msg, el, arg3, arg4);
    return (msg + arg3) & 1;
}

void dtGuiReset(uint32_t heapSize)
{
    memset(&gGuiElements, 0, sizeof gGuiElements);
    memset(&gGuiSorted, 0, sizeof gGuiSorted);
    memset(&gGuiScreen, 0, sizeof gGuiScreen);
    gGuiElementCount = 0;
    gGuiDebugPrint = dtGuiDebugPrint;

    uint8_t *arena = dt_alloc(heapSize);
    DMM_LIMIT = (uintptr_t)arena + heapSize;
    DMM_UNKNOWN = 0;
    DMM_SIZE = heapSize;
    DMM_BASE = arena;
    DMM_LOCKS = 0;
    DMM_TABLE = (uintptr_t)arena + heapSize;
    DMM_ARENA = arena;
    DMM_FREE = (uint32_t *)arena;
    DMM_FREE[0] = 0;
    DMM_FREE[1] = 0;
    DMM_FREE[2] = heapSize - 0x10;
    DMM_READY = 1;

    ZASSET_LIST = NULL;
    ZPRIM_STATE = dt_alloc(0x54 * 4);
    ZPRIM_CURRENT = 0;
    ZPRIM_CLIP = 480;
    GPU_PRIMS = dt_alloc(0x7c * 600);
    GPU_OT = (uint8_t *)dt_alloc(4 * 800) + 4 * 700;
}

GuiElement *dtGuiRandomElement(int slot)
{
    static const int16_t types[] = {1, 2, 3, 5, 6, 7, 8};
    GuiElement *el = &gGuiElements[slot];
    memset(el, 0, sizeof *el);
    el->type = types[dt_u32() % 7];
    el->flags = (uint16_t)dt_u32();
    if (dt_u32() % 4 != 0) {
        el->flags &= (uint16_t)~kGuiFlagHidden;
    }
    el->id = dt_u32();
    for (int i = 0; i < 4; i++) {
        el->nav[i] = dt_u32() % 3 == 0 ? 0 : (uint8_t)dt_u32();
    }
    el->x = (int16_t)dt_range(-300, 700);
    el->y = (int16_t)dt_range(-300, 500);
    el->w = (int16_t)dt_range(-5, 400);
    el->h = (int16_t)dt_range(-5, 300);
    el->slideX = (int16_t)dt_range(-40, 40);
    el->slideY = (int16_t)dt_range(-40, 40);
    el->slideStartX = (int16_t)dt_range(-40, 40);
    el->slideStartY = (int16_t)dt_range(-40, 40);
    el->slideDelayX = (uint16_t)dt_range(0, 12);
    el->slideDelayY = (uint16_t)dt_range(0, 12);
    el->scaleX = dt_float_edgy(0.25f, 3.0f);
    el->scaleY = dt_float_edgy(0.25f, 3.0f);
    el->order = (uint16_t)dt_u32();
    el->tintRgb = dt_u32() % 8 == 0 ? dt_u32() & 0xffffff
                                    : (uint32_t)dt_range(0, 15); /* used as a palette index by text */
    el->tintAlpha = dt_u32() % 256;
    el->spriteX = (int16_t)dt_range(-20, 60);
    el->spriteY = (int16_t)dt_range(-20, 60);
    el->frame = (uint16_t)dt_range(0, 3);
    el->palette = dt_u32() % 16 == 0 ? dt_u32() & 0xffff : (uint32_t)dt_range(0, 15); /* below 16 to draw */
    el->alpha = (uint16_t)dt_u32() % 256;
    el->textFlags = (uint16_t)dt_u32();
    el->textSpacing = (int16_t)dt_range(-2, 12);
    el->textX = (int16_t)dt_range(-10, 40);
    el->textY = (int16_t)dt_range(-10, 40);
    el->textStyle = dt_u32() % 16 == 0 ? dt_u32() & 0xffff : (uint32_t)dt_range(0, 15); /* texture variant */
    for (int i = 0; i < 3; i++) {
        el->textRgb[i] = (uint8_t)dt_u32();
    }
    switch (dt_u32() % 4) {
    case 0:
        el->proc = NULL;
        break;
    case 1:
        el->proc = dtGuiProc;
        break;
    default:
        el->proc = dtGuiProcOther;
    }
    el->defaultProc = dt_u32() % 3 == 0 ? NULL : (dt_u32() & 1 ? dtGuiProc : dtGuiProcOther);
    return el;
}

void dtGuiRegister(int slot)
{
    gGuiSorted[gGuiElementCount++] = &gGuiElements[slot];
}

void dtGuiRandomScreen(int count)
{
    for (int i = 0; i < count; i++) {
        dtGuiRandomElement(i);
        dtGuiRegister(i);
    }
    /* insertion sort, highest order first */
    for (int i = 1; i < count; i++) {
        GuiElement *el = gGuiSorted[i];
        int j = i - 1;
        while (j >= 0 && gGuiSorted[j]->order < el->order) {
            gGuiSorted[j + 1] = gGuiSorted[j];
            j--;
        }
        gGuiSorted[j + 1] = el;
    }
}

static uint32_t randButtons(void)
{
    const uint32_t v = dt_u32();
    switch (dt_u32() % 4) {
    case 0:
        return v & 0xf;
    case 1:
        return 1u << (v & 31);
    case 2:
        return v & 0x0003f1ff;
    default:
        return v;
    }
}

void dtGuiRandomPad(void)
{
    GuiPadDevice *dev = dt_alloc(sizeof *dev);
    dev->unknown0 = dt_u32();
    dev->status = (uint8_t)dt_u32();
    dev->held = randButtons();
    dev->pressed = randButtons();
    gGuiPad.device = dev;
    gGuiPad.held = randButtons();
    gGuiPad.pressed = randButtons() | (dt_u32() & 1 ? gGuiPad.held : 0);
    gGuiPad.flags = (uint16_t)(dt_u32() % 5 == 0 ? dt_u32() : (dt_u32() & 0xfffe));
    gGuiPad.flags2 = (uint16_t)dt_u32();
    for (int i = 0; i < 4; i++) {
        gGuiPad.axis[i] = dt_u32();
    }
    gGuiPad.prevAxis[0] = dt_u32();
    gGuiPad.prevAxis[1] = dt_u32();
    gGuiPadRepeatHeld = randButtons();
    gGuiPadRepeatCount = dt_u32() & 1 ? (uint32_t)dt_range(0x10, 0x30) : dt_u32();
    if (dt_u32() % 3 == 0) { /* a d-pad direction held long enough for the auto-repeat to fire */
        const uint32_t bit = 1u << (dt_u32() % 4);
        gGuiPad.held = bit | (dt_u32() & 0x3ff0);
        gGuiPad.pressed = dt_u32() & 0xfffffff0u;
        gGuiPad.flags &= (uint16_t)~1u;
        gGuiPadRepeatHeld = gGuiPad.held | (dt_u32() & 0xf0);
        gGuiPadRepeatCount = 5u * (uint32_t)dt_range(4, 9) - 1 + (dt_u32() % 5 == 0);
    }
    gGuiAnalogPad = (uint8_t)dt_range(0, 2);
}

char *dtGuiRandomText(int maxLen, int allowNewline)
{
    const int len = dt_range(0, maxLen);
    char *s = dt_alloc((uint32_t)len + 1);
    for (int i = 0; i < len; i++) {
        s[i] = (char)dt_range('a', 'z');
        if (allowNewline && dt_u32() % 8 == 0) {
            s[i] = '\n';
        }
        if (dt_u32() % 9 == 0) {
            s[i] = ' ';
        }
    }
    return s;
}

int dtGuiTextHandle(const char *text)
{
    const uint32_t size = (uint32_t)strlen(text) + 1;
    const int handle = zmemdmmAlloc(size);
    memcpy(zmemdmmLock(handle), text, size);
    zmemdmmUnlock(handle);
    return handle;
}

void *dtGuiMakeAsset(uint16_t id, const void *data, uint32_t size)
{
    uint32_t *node = dt_alloc(0x20);
    const int handle = zmemdmmAlloc(size);
    memcpy(zmemdmmLock(handle), data, size);
    zmemdmmUnlock(handle);
    ((uint16_t *)node)[0] = id;
    node[1] = (uint32_t)(uintptr_t)ZASSET_LIST;
    node[2] = (uint32_t)handle;
    node[3] = 0;
    ZASSET_LIST = node;
    return node;
}

/* Texture cache table the sprite renderer would otherwise build: every entry non-zero (already uploaded). */
static uint32_t dtGuiCacheTable(void)
{
    uint32_t *cache = dt_alloc(0x80);
    for (int i = 0; i < 32; i++) {
        cache[i] = 0x1000u + (uint32_t)i;
    }
    return (uint32_t)(uintptr_t)cache;
}

void *dtGuiMakeSprite(uint16_t id, int frames)
{
    uint8_t data[0x14 + 16 * 8 + 0x40] = {0};
    *(uint16_t *)data = (uint16_t)frames;
    *(uint32_t *)(data + 0xc) = dtGuiCacheTable();
    for (int i = 0; i < frames; i++) {
        *(uint16_t *)(data + 0x14 + 16 * i) = (uint16_t)dt_range(4, 80);
        *(uint16_t *)(data + 0x16 + 16 * i) = (uint16_t)dt_range(4, 60);
    }
    return dtGuiMakeAsset(id, data, sizeof data);
}

void *dtGuiMakeFont(uint16_t id, uint16_t height)
{
    enum { kChars = 96, kFirst = 0x20 };
    uint8_t *data = dt_alloc(0x18 + 10 * (kChars + 2));
    *(uint16_t *)data = kChars;
    *(uint16_t *)(data + 2) = height;
    *(uint32_t *)(data + 0xc) = dtGuiCacheTable();
    for (int i = 0; i <= kChars; i++) {
        uint8_t *glyph = data + 0x18 + 10 * i;
        glyph[2] = (uint8_t)dt_range(2, 12);
        glyph[4] = (uint8_t)dt_range(0, 2);
        glyph[6] = (uint8_t)dt_range(0, 2);
        *(uint16_t *)(glyph + 8) = (uint16_t)(kFirst + i);
    }
    return dtGuiMakeAsset(id, data, 0x18 + 10 * (kChars + 2));
}

void dtGuiMakeToggleData(GuiElement *el, int withChoices, int withText)
{
    el->dataHandle = zmemdmmAlloc(sizeof(GuiToggleData));
    GuiToggleData *data = zmemdmmLock(el->dataHandle);
    memset(data, 0, sizeof *data);
    data->index = (uint16_t)dt_range(0, 4);
    data->count = (uint16_t)dt_range(0, 5);
    data->textFlags = (uint16_t)dt_u32();
    data->textSpacing = (int16_t)dt_range(-2, 10);
    data->textX = (int16_t)dt_range(-10, 40);
    data->textY = (int16_t)dt_range(-10, 40);
    data->textStyle = dt_u32() % 16 == 0 ? dt_u32() & 0xffff : (uint32_t)dt_range(0, 15);
    for (int i = 0; i < 3; i++) {
        data->textRgb[i] = (uint8_t)dt_u32();
    }
    if (withChoices) {
        char list[64];
        char *at = list;
        const int n = dt_range(1, 4);
        for (int i = 0; i < n; i++) {
            const int len = dt_range(0, 6);
            for (int k = 0; k < len; k++) {
                *at++ = (char)dt_range('a', 'z');
            }
            if (i + 1 < n) {
                *at++ = '|';
            }
        }
        *at = '\0';
        data->choicesHandle = dtGuiTextHandle(list);
    }
    if (withText) {
        data->textHandle = dtGuiTextHandle(dtGuiRandomText(8, 0));
    }
    zmemdmmUnlock(el->dataHandle);
}
