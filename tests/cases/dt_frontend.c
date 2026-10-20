#include "dt_frontend.h"

#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/menu/gui/guilabel.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "compat/crt.h"
#include "zge/zstring.h"

DtFrontLog dt_front_log;

void dtFrontLog(uint32_t who, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    const uint32_t words[5] = {who, a, b, c, d};
    for (int i = 0; i < 5; i++) {
        dt_front_log.hash = (dt_front_log.hash ^ words[i]) * 16777619u + 0x9e3779b9u;
    }
    if (dt_front_log.calls < 48) {
        for (int i = 0; i < 5; i++) {
            dt_front_log.entries[dt_front_log.calls][i] = words[i];
        }
    }
    dt_front_log.calls++;
}

/*
 * A string set with ids first..first+count-1, each starting with its id in hex, so every lookup is distinct.
 * Every fourth id carries caption markup ("#Y" / "#b" first line, "#B" / "#y" second line).
 */
static uint8_t *dtFrontendMakeStrings(uint16_t first, uint16_t count, char prefix, uint32_t *sizeOut)
{
    enum { kStride = 20 };
    const uint32_t size = 8 + 2 * (uint32_t)count + kStride * (uint32_t)count;
    uint8_t *blob = dt_alloc(size);
    memcpy(blob, "STRS", 4);
    *(uint16_t *)(blob + 4) = first;
    *(uint16_t *)(blob + 6) = count;
    uint32_t at = 8 + 2 * (uint32_t)count;
    for (uint32_t i = 0; i < count; i++) {
        const uint32_t id = first + i;
        ((uint16_t *)(blob + 8))[i] = (uint16_t)at;
        const char *tail = "";
        switch (id % 4) {
        case 1:
            tail = "#Yab#Bcd";
            break;
        case 2:
            tail = "#bef#ygh";
            break;
        case 3:
            tail = "#Yij";
            break;
        default:
            break;
        }
        blob[at++] = (uint8_t)prefix;
        blob[at++] = (uint8_t)"0123456789abcdef"[id >> 8 & 15];
        blob[at++] = (uint8_t)"0123456789abcdef"[id >> 4 & 15];
        blob[at++] = (uint8_t)"0123456789abcdef"[id & 15];
        for (const char *t = tail; *t != '\0'; t++) {
            blob[at++] = (uint8_t)*t;
        }
        blob[at++] = 0;
        at = 8 + 2 * (uint32_t)count + (i + 1) * kStride;
    }
    if (sizeOut != NULL) {
        *sizeOut = size;
    }
    return blob;
}

static const uint8_t *dt_asset_blob;
static uint32_t dt_asset_blob_size;

static void dtFrontendStrings(void)
{
    gZstringSets[0] = (const ZStringSet *)dtFrontendMakeStrings(0, 0x400, 's', NULL);
    /* sets 1 and 2: extras strings (0x1000..) and the level information texts (0x2000..) */
    gZstringSets[1] = (const ZStringSet *)dtFrontendMakeStrings(0x1000, 0x100, 't', NULL);
    gZstringSets[2] = (const ZStringSet *)dtFrontendMakeStrings(0x2000, 0x100, 'u', NULL);
    /* what the asset stubs hand out when a string set is loaded */
    dt_asset_blob = dtFrontendMakeStrings(0x1000, 0x100, 'v', &dt_asset_blob_size);
}

const uint8_t *dtFrontendAssetBlob(uint32_t *size)
{
    *size = dt_asset_blob_size;
    return dt_asset_blob;
}

void dtFrontendReset(void)
{
    dtFrontendStrings();
    dtGuiReset(DT_GUI_HEAP);
    dt_front_log.calls = 0;
    memset(dt_front_log.entries, 0, sizeof dt_front_log.entries);
    dt_front_log.hash = 2166136261u;
}

void dtFrontendRandomText(GuiElement *el)
{
    guiSetText(el, dtGuiRandomText(10, 0));
}

GuiElement *dtFrontendElement(uint32_t id)
{
    const int slot = gGuiElementCount;
    GuiElement *el = dtGuiRandomElement(slot);
    el->id = id;
    el->type = kGuiTypeLabel;
    dtGuiRegister(slot);
    if (dt_u32() % 3 != 0) {
        dtFrontendRandomText(el);
    }
    return el;
}

void dtFrontendElementRange(uint32_t first, int count)
{
    for (int i = 0; i < count; i++) {
        dtFrontendElement(first + (uint32_t)i);
    }
}

void dtFrontendElements(const uint32_t *ids, int n)
{
    for (int i = 0; i < n; i++) {
        dtFrontendElement(ids[i]);
    }
}

static int BMX_CDECL stubSound(int id, const void *position, int a, int volume, int b)
{
    dtFrontLog(0x100, (uint32_t)id, (uint32_t)(uintptr_t)position, (uint32_t)a,
               (uint32_t)volume * 31u + (uint32_t)b);
    return 0;
}

static void BMX_CDECL stubSet(uint16_t prop, int value)
{
    dtFrontLog(0x101, prop, (uint32_t)value, 0, 0);
}

static void BMX_CDECL stubFillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    dtFrontLog(0x102, (uint16_t)x0, (uint16_t)y0, (uint16_t)x1, (uint16_t)y1);
}

static void BMX_CDECL stubDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    dtFrontLog(0x103, (uint16_t)x0, (uint16_t)y0, (uint16_t)x1, (uint16_t)y1);
}

static void BMX_CDECL stubSetFont(ZPrimAsset *font)
{
    dtFrontLog(0x105, (uint32_t)(uintptr_t)font, 0, 0, 0);
}

static void BMX_CDECL stubDrawText(const uint8_t *str, int x, int16_t y)
{
    uint32_t hash = 0;
    for (const uint8_t *p = str; *p != 0; p++) {
        hash = hash * 31 + *p;
    }
    dtFrontLog(0x106, hash, (uint32_t)x, (uint16_t)y, 0);
}

static void BMX_CDECL stubSetClip(int x0, int y0, int x1, int y1)
{
    dtFrontLog(0x104, (uint32_t)x0, (uint32_t)y0, (uint32_t)x1, (uint32_t)y1);
}

static void BMX_CDECL stubRequestScreen(uint16_t id, GuiHookFn hook)
{
    dtFrontLog(0x110, id, (uint32_t)(uintptr_t)hook, 0, 0);
}

static void BMX_CDECL stubDestroyAll(void)
{
    dtFrontLog(0x111, 0, 0, 0, 0);
}

static void BMX_CDECL stubLoadBackdrop(uint16_t id, int arg)
{
    dtFrontLog(0x112, id, (uint32_t)arg, 0, 0);
}

void BMX_CDECL dtFrontStep(void)
{
    dtFrontLog(0x120, 0, 0, 0, 0);
}

typedef struct DtQueue {
    int values[4];
    uint32_t count;
    uint32_t next;
} DtQueue;

static DtQueue queues[DT_Q_COUNT];

void dtFrontQueue(int which, const int *values, int count)
{
    queues[which].count = (uint32_t)count;
    queues[which].next = 0;
    for (int i = 0; i < count && i < 4; i++) {
        queues[which].values[i] = values[i];
    }
}

void dtFrontRandomCardResults(void)
{
    static const int codes[] = {0, 0, 1, 2, 4, 4, 5, 5, 6, 6, 6, 7, 3, -1};
    for (int q = 0; q < DT_Q_GROUPLOADED; q++) {
        int values[4];
        const int n = dt_range(1, 3);
        for (int i = 0; i < n; i++) {
            values[i] = codes[dt_u32() % (sizeof codes / sizeof codes[0])];
        }
        dtFrontQueue(q, values, n);
    }
}

static int takeResultPlain(int which)
{
    DtQueue *q = &queues[which];
    const int value = q->count != 0 ? q->values[q->next % q->count % 4] : 0;
    q->next++;
    return value;
}

static int takeResult(int which, uint32_t who)
{
    DtQueue *q = &queues[which];
    const int value = q->count != 0 ? q->values[q->next % q->count % 4] : 0;
    dtFrontLog(who, q->next, (uint32_t)value, 0, 0);
    q->next++;
    return value;
}

static int BMX_CDECL stubSaveClose(void)
{
    return takeResult(DT_Q_CLOSE, 0x130);
}

static int BMX_CDECL stubSaveRead(void)
{
    return takeResult(DT_Q_READ, 0x131);
}

static int BMX_CDECL stubSaveWrite(void)
{
    return takeResult(DT_Q_WRITE, 0x132);
}

static int BMX_CDECL stubCardFormat(void)
{
    return takeResult(DT_Q_FORMAT, 0x133);
}

static int BMX_CDECL stubCardQuery(void)
{
    return takeResult(DT_Q_QUERY, 0x134);
}

static int BMX_CDECL stubConfirmQuit(void)
{
    return takeResult(DT_Q_CONFIRM, 0x135);
}

static void BMX_CDECL stubRequestExit(int code)
{
    dtFrontLog(0x136, (uint32_t)code, 0, 0, 0);
}

GuiPadDevice *dt_front_pad;

static GuiPadDevice *BMX_CDECL stubGetDevice(int index)
{
    dtFrontLog(0x140, (uint32_t)index, 0, 0, 0);
    return dt_front_pad;
}

static int BMX_CDECL stubLabelProc(int msg, GuiElement *el, int arg3, int arg4)
{
    uint32_t textHash = 0;
    if (el->textHandle != 0) {
        const char *text = zmemdmmLock(el->textHandle);
        for (const char *p = text; *p != '\0'; p++) {
            textHash = textHash * 31 + (uint8_t)*p;
        }
        zmemdmmUnlock(el->textHandle);
    }
    dtFrontLog(0x150, (uint32_t)msg, el->id, textHash,
               (uint32_t)(uint16_t)el->textX << 16 | (uint16_t)el->textY);
    dtFrontLog(0x151, el->textStyle, (uint32_t)arg3, (uint32_t)arg4,
               (uint32_t)(uint16_t)el->spriteX << 16 | (uint16_t)el->spriteY);
    return 1;
}

static int BMX_CDECL stubGroupLoaded(uint32_t slot)
{
    const int value = takeResultPlain(DT_Q_GROUPLOADED);
    dtFrontLog(0x170, slot, (uint32_t)value, 0, 0);
    return value;
}

static int BMX_CDECL stubLoadGroup(uint16_t slot, uint16_t group)
{
    dtFrontLog(0x171, slot, group, 0, 0);
    return 0;
}

static void BMX_CDECL stubFreeGroup(uint16_t slot)
{
    dtFrontLog(0x172, slot, 0, 0, 0);
}

static uint16_t BMX_CDECL stubGroupCount(uint32_t slot)
{
    const int value = takeResultPlain(DT_Q_GROUPCOUNT);
    dtFrontLog(0x173, slot, (uint32_t)value, 0, 0);
    return (uint16_t)value;
}

static ZPrimAsset *BMX_CDECL stubGroupSelect(uint32_t slot, uint32_t index)
{
    dtFrontLog(0x174, slot, index, 0, 0);
    return (ZPrimAsset *)(uintptr_t)(0x60000000u + index * 0x10);
}

static void BMX_CDECL stubReplaceGrey(const ZPrimAsset *src, const ZPrimAsset *dst, int unused,
                                      uint32_t brightness)
{
    dtFrontLog(0x175, (uint32_t)(uintptr_t)src, (uint32_t)(uintptr_t)dst, (uint32_t)unused, brightness);
}

static void BMX_CDECL stubReplacePalette(const ZPrimAsset *src, const ZPrimAsset *dst, int unused,
                                         uint32_t index)
{
    dtFrontLog(0x176, (uint32_t)(uintptr_t)src, (uint32_t)(uintptr_t)dst, (uint32_t)unused, index & 0xff);
}

static void BMX_CDECL stubAssetFree(ZPrimAsset *a)
{
    dtFrontLog(0x177, (uint32_t)(uintptr_t)a, 0, 0, 0);
}

static int BMX_CDECL stubOptimize(void)
{
    dtFrontLog(0x178, 0, 0, 0, 0);
    return 0;
}

static ZPrimAsset *BMX_CDECL stubCreateSpriteSmall(uint16_t w, uint16_t h, uint16_t bpp, uint16_t colors)
{
    static uint32_t created;
    dtFrontLog(0x160, w, h, bpp, colors);
    return (ZPrimAsset *)(uintptr_t)(0x50000000u + (created++) * 0x20);
}

static ZPrimAsset *BMX_CDECL stubLoadAsset(uint32_t id)
{
    /* the real function only looks at the low 16 bits */
    dtFrontLog(0x161, id & 0xffff, 0, 0, 0);
    return (ZPrimAsset *)(uintptr_t)(0x40000000u + (id & 0xffff) * 0x10);
}

static void BMX_CDECL stubCopyTexture(const ZPrimAsset *src, const ZPrimAsset *dst)
{
    dtFrontLog(0x162, (uint32_t)(uintptr_t)src, (uint32_t)(uintptr_t)dst, 0, 0);
}

static void BMX_CDECL stubFreeSprite(ZPrimAsset *a)
{
    dtFrontLog(0x163, (uint32_t)(uintptr_t)a, 0, 0, 0);
}

static int takeResultOr(int which, int fallback, uint32_t who)
{
    DtQueue *q = &queues[which];
    const int value = q->count != 0 ? q->values[q->next % q->count % 4] : fallback;
    dtFrontLog(who, q->next, (uint32_t)value, 0, 0);
    q->next++;
    return value;
}

static int BMX_CDECL stubAssetFind(int lib, int id, int flags)
{
    const int value = takeResultOr(DT_Q_ASSETFIND, 1, 0x180);
    dtFrontLog(0x181, (uint32_t)lib, (uint32_t)id, (uint32_t)flags, (uint32_t)value);
    return value;
}

static uint32_t BMX_CDECL stubAssetSize(int lib)
{
    const int value = takeResultOr(DT_Q_ASSETSIZE, 0, 0x182);
    uint32_t size = 0;
    dtFrontendAssetBlob(&size);
    dtFrontLog(0x183, (uint32_t)lib, value != 0 ? size : 0, 0, 0);
    return value != 0 ? size : 0;
}

static int BMX_CDECL stubAssetRead(int lib, void *dst)
{
    uint32_t size = 0;
    const uint8_t *blob = dtFrontendAssetBlob(&size);
    memcpy(dst, blob, size);
    dtFrontLog(0x184, (uint32_t)lib, size, 0, 0);
    return 1;
}

static uint32_t hashString(const char *s)
{
    uint32_t hash = 0;
    for (const char *p = s; *p != '\0'; p++) {
        hash = hash * 31 + (uint8_t)*p;
    }
    return hash;
}

static void BMX_CDECL stubMoviePlay(MovieDesc *const *list)
{
    const MovieDesc *desc = *list;
    dtFrontLog(0x190, hashString(desc->file), (uint32_t)desc->width, (uint32_t)desc->height,
               (uint32_t)desc->unk0c * 31u + (uint32_t)desc->unk10 * 7u + (uint32_t)desc->end);
}

static int BMX_CDECL stubModeInit(void)
{
    return takeResultOr(DT_Q_MODEINIT, 0, 0x191);
}

static void BMX_CDECL stubModeShutdown(void)
{
    dtFrontLog(0x192, 0, 0, 0, 0);
}

static void BMX_CDECL stubSetPicture(ZPrimAsset *picture)
{
    dtFrontLog(0x193, (uint32_t)(uintptr_t)picture, 0, 0, 0);
}

static void BMX_CDECL stubDrawSprite(uint32_t frame, int16_t x, int16_t y)
{
    dtFrontLog(0x194, frame, (uint16_t)x, (uint16_t)y, 0);
}

static void BMX_CDECL stubAppLog(const char *message)
{
    dtFrontLog(0x1a0, hashString(message), 0, 0, 0);
}

static void BMX_CDECL stubInitA(void)
{
    dtFrontLog(0x1a1, 0, 0, 0, 0);
}

static void BMX_CDECL stubInitB(int arg)
{
    dtFrontLog(0x1a2, (uint32_t)arg, 0, 0, 0);
}

static void BMX_CDECL stubInitC(void)
{
    dtFrontLog(0x1a3, 0, 0, 0, 0);
}

static void BMX_CDECL stubInitD(int arg)
{
    dtFrontLog(0x1a4, (uint32_t)arg, 0, 0, 0);
}

static void BMX_CDECL stubInitE(int arg)
{
    dtFrontLog(0x1a5, (uint32_t)arg, 0, 0, 0);
}

static int BMX_CDECL stubGuiInit(void)
{
    return takeResultOr(DT_Q_GUIINIT, 0, 0x1a6);
}

static void BMX_CDECL stubPadInit(void)
{
    dtFrontLog(0x1a7, 0, 0, 0, 0);
}

static void BMX_CDECL stubDebugPrint(const char *fmt, ...)
{
    dtFrontLog(0x1a8, hashString(fmt), 0, 0, 0);
}

void dtFrontendStub(uint32_t groups)
{
    if (groups & DT_FS_ASSET) {
        dt_stub(0x00480780, NULL, (void *)stubAssetFind);
        dt_stub(0x00480850, NULL, (void *)stubAssetSize);
        dt_stub(0x00480930, NULL, (void *)stubAssetRead);
    }
    if (groups & DT_FS_EXTRAS) {
        dt_stub(0x0045a3c0, NULL, (void *)stubMoviePlay);
        dt_stub(0x00437640, guiModeInit, (void *)stubModeInit);
        dt_stub(0x004377d0, guiModeShutdown, (void *)stubModeShutdown);
        dt_stub(0x00478b60, zprimSetPicture, (void *)stubSetPicture);
        dt_stub(0x00479cf0, zprimDrawSprite, (void *)stubDrawSprite);
    }
    if (groups & DT_FS_INIT) {
        dt_stub(0x0045a1c0, NULL, (void *)stubAppLog);
        dt_stub(0x0046cbc0, NULL, (void *)stubInitA);
        dt_stub(0x0046cc50, NULL, (void *)stubInitB);
        dt_stub(0x0046c310, NULL, (void *)stubInitC);
        dt_stub(0x0046d3a0, NULL, (void *)stubInitD);
        dt_stub(0x0046d3c0, NULL, (void *)stubInitE);
        dt_stub(0x004375d0, guiInit, (void *)stubGuiInit);
        dt_stub(0x00437200, guiPadInit, (void *)stubPadInit);
        dt_stub(0x00480240, NULL, (void *)stubDebugPrint);
    }
    if (groups & DT_FS_ZASSET) {
        dt_stub(0x0047b3d0, zprimGroupLoaded, (void *)stubGroupLoaded);
        dt_stub(0x0047b400, zprimLoadGroup, (void *)stubLoadGroup);
        dt_stub(0x0047b5c0, zprimFreeGroup, (void *)stubFreeGroup);
        dt_stub(0x0047ba10, zprimGroupCount, (void *)stubGroupCount);
        dt_stub(0x0047ba50, zprimGroupSelect, (void *)stubGroupSelect);
        dt_stub(0x00479300, zprimReplaceVramGrey, (void *)stubReplaceGrey);
        dt_stub(0x00479500, zprimReplaceVramPalette, (void *)stubReplacePalette);
        dt_stub(0x0047b6b0, zprimAssetFree, (void *)stubAssetFree);
        dt_stub(0x0046b570, NULL, (void *)stubOptimize);
    }
    if (groups & DT_FS_FRAME) {
        dt_stub(0x00479b90, zprimCreateSpriteSmall, (void *)stubCreateSpriteSmall);
        dt_stub(0x0047b950, zprimLoadAsset, (void *)stubLoadAsset);
        dt_stub(0x00479280, zprimCopyTexture, (void *)stubCopyTexture);
        dt_stub(0x00479cd0, zprimFreeSprite, (void *)stubFreeSprite);
    }
    if (groups & DT_FS_PAD) {
        dt_stub(0x00480400, NULL, (void *)stubGetDevice);
    }
    if (groups & DT_FS_LABEL) {
        dt_stub(0x00439f80, guiLabelProc, (void *)stubLabelProc);
    }
    if (groups & DT_FS_QUIT) {
        dt_stub(0x0048a9b0, NULL, (void *)stubConfirmQuit);
        dt_stub(0x0044dd10, NULL, (void *)stubRequestExit);
    }
    if (groups & DT_FS_CARD) {
        dt_stub(0x00412130, NULL, (void *)stubSaveClose);
        dt_stub(0x00412190, NULL, (void *)stubSaveRead);
        dt_stub(0x00412340, NULL, (void *)stubSaveWrite);
        dt_stub(0x00412170, NULL, (void *)stubCardFormat);
        dt_stub(0x0046d8a0, NULL, (void *)stubCardQuery);
    }
    if (groups & DT_FS_SOUND) {
        dt_stub(0x0044e300, NULL, (void *)stubSound);
    }
    if (groups & DT_FS_ZPRIM) {
        dt_stub(0x00478050, zprimSet, (void *)stubSet);
        dt_stub(0x00478f00, zprimFillRect, (void *)stubFillRect);
        dt_stub(0x00478df0, zprimDrawLine, (void *)stubDrawLine);
        dt_stub(0x00477da0, zprimSetClip, (void *)stubSetClip);
        dt_stub(0x00478a50, zprimSetFont, (void *)stubSetFont);
        dt_stub(0x0047aa80, zprimDrawText, (void *)stubDrawText);
    }
    if (groups & DT_FS_SCREENS) {
        dt_stub(0x00437930, guiRequestScreen, (void *)stubRequestScreen);
        dt_stub(0x00437870, guiDestroyAll, (void *)stubDestroyAll);
        dt_stub(0x004391c0, guiLoadBackdrop, (void *)stubLoadBackdrop);
    }
}

void dtFrontendHookArgs(DtCtx *c, const int *msgs, int count)
{
    int msg = msgs[dt_u32() % (uint32_t)count];
    if (dt_u32() % 8 == 0) {
        msg = dt_range(0, 0x40);
    }
    GuiElement *el = gGuiElementCount > 0 ? gGuiSorted[dt_u32() % (uint32_t)gGuiElementCount] : NULL;
    DT_ARG_INT(c, 0, msg);
    DT_ARG_PTR(c, 1, el);
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}
