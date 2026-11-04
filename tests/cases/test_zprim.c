#include "bmx/com/zge/zprim.h"
#include "dt.h"
#include "dt_zprim.h"

/* ---- state stack ------------------------------------------------------------------------------- */

/*
 * Fresh environment with random states. `lockedByCaller` leaves the state block locked (as zprimInit does),
 * so the function under test skips its own lock / unlock; otherwise it has to take and release it.
 */
static void state_env(DtCtx *c, int depthLimit)
{
    dt_seed(c);
    dt_zprim_env();
    gZprimDepth = dt_range(0, depthLimit);
    dt_fill(gZprimStates, 0x348);
    if (c->seed & 1) {
        gZprimStates = NULL;
    }
}

static void setup_state_none(DtCtx *c)
{
    state_env(c, 9);
}

static void setup_state_push(DtCtx *c)
{
    state_env(c, 10); /* at depth 10 the push does nothing */
}

static void setup_state_pop(DtCtx *c)
{
    state_env(c, 10);
    if (c->seed % 7 == 0) {
        gZprimDepth = 0;
    }
}

static void setup_state_prop(DtCtx *c)
{
    state_env(c, 9);
    uint32_t prop;
    switch (c->seed % 5) {
    case 0:
        prop = (uint32_t)dt_range(1, 24);
        break;
    case 1:
        prop = (uint32_t)dt_range(0, 30);
        break;
    default:
        prop = (uint32_t)dt_range(1, 24);
    }
    DT_ARG_INT(c, 0, (int32_t)(prop | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 1, (c->seed % 3 == 0) ? 0 : (int32_t)dt_u32());
}

static void setup_state_asset(DtCtx *c)
{
    state_env(c, 9);
    DT_ARG_INT(c, 0, (c->seed % 4 == 0) ? 0 : (int32_t)dt_u32());
}

static void setup_state_source(DtCtx *c)
{
    state_env(c, 9);
    DT_ARG_INT(c, 0, (c->seed % 4 == 0) ? 0 : (int32_t)dt_u32());
    DT_ARG_INT(c, 1, (int32_t)((c->seed % 3 == 0 ? 3 : dt_range(0, 5)) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 2, (int32_t)dt_u32());
    DT_ARG_INT(c, 3, (int32_t)dt_u32());
    DT_ARG_INT(c, 4, (int32_t)dt_u32());
}

static void setup_state_scale(DtCtx *c)
{
    state_env(c, 9);
    DT_ARG_FLOAT(c, 0, dt_rf());
    DT_ARG_FLOAT(c, 1, dt_rf());
}

static void setup_state_color(DtCtx *c)
{
    state_env(c, 9);
    DT_ARG_INT(c, 0, (int32_t)dt_u32());
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
    DT_ARG_INT(c, 2, (int32_t)dt_u32());
    DT_ARG_INT(c, 3, (int32_t)(dt_range(1, 24) | (dt_u32() & ~0xffu)));
}

static void setup_clip(DtCtx *c)
{
    state_env(c, 9);
    DT_ARG_INT(c, 0, (int32_t)dt_u32());
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
    DT_ARG_INT(c, 2, (int32_t)dt_u32());
    DT_ARG_INT(c, 3, (int32_t)dt_u32());
}

static void setup_shape(DtCtx *c)
{
    state_env(c, 9);
    gZprimClip.x0 = 0;
    gZprimClip.y0 = 0;
    gZprimClip.x1 = 640;
    gZprimClip.y1 = 480;
    for (int i = 0; i < 4; i++) {
        DT_ARG_INT(c, i,
                   (c->seed % 3 == 0) ? (int32_t)dt_u32()
                                      : (int32_t)((uint32_t)dt_range(-50, 700) | (dt_u32() & 0xffff0000u)));
    }
}

DT_CASES(zprim_state_cases) = {
    {"zprimStateReset", 0x00478c80, zprimStateReset, 0, DT_RET_VOID, setup_state_none, 20, 0},
    {"zprimStateSave", 0x00477df0, zprimStateSave, 0, DT_RET_VOID, setup_state_none, 20, 0},
    {"zprimStateRestore", 0x00477e80, zprimStateRestore, 0, DT_RET_VOID, setup_state_none, 20, 0},
    {"zprimStatePush", 0x00477f10, zprimStatePush, 0, DT_RET_VOID, setup_state_push, 60, 0},
    {"zprimStatePop", 0x00477fb0, zprimStatePop, 0, DT_RET_VOID, setup_state_pop, 60, 0},
    {"zprimSet", 0x00478050, zprimSet, 2, DT_RET_VOID, setup_state_prop, 400, 0},
    {"zprimGet", 0x00478370, zprimGet, 1, DT_RET_INT, setup_state_prop, 400, 0},
    {"zprimSetScreen", 0x004788c0, zprimSetScreen, 1, DT_RET_VOID, setup_state_asset, 40, 0},
    {"zprimSetSource", 0x00478980, zprimSetSource, 5, DT_RET_VOID, setup_state_source, 80, 0},
    {"zprimSetFont", 0x00478a50, zprimSetFont, 1, DT_RET_VOID, setup_state_asset, 40, 0},
    {"zprimGetFont", 0x00478ae0, zprimGetFont, 0, DT_RET_INT, setup_state_none, 20, 0},
    {"zprimSetPicture", 0x00478b60, zprimSetPicture, 1, DT_RET_VOID, setup_state_asset, 40, 0},
    {"zprimGetPicture", 0x00478d00, zprimGetPicture, 0, DT_RET_INT, setup_state_none, 20, 0},
    {"zprimSetScale", 0x00478bf0, zprimSetScale, 2, DT_RET_VOID, setup_state_scale, 40, 0},
    {"zprimSetColor", 0x00478d80, zprimSetColor, 4, DT_RET_VOID, setup_state_color, 200, 0},
    {"zprimGetColor", 0x00478dc0, zprimGetColor, 0, DT_RET_VOID, setup_state_none, 1, 0},
    {"zprimSetClip", 0x00477da0, zprimSetClip, 4, DT_RET_VOID, setup_clip, 60, 0},
    {"zprimDrawLine", 0x00478df0, zprimDrawLine, 4, DT_RET_VOID, setup_shape, 200, 0},
    {"zprimFillRect", 0x00478f00, zprimFillRect, 4, DT_RET_VOID, setup_shape, 200, 0},
};

/* ---- asset table ------------------------------------------------------------------------------- */

/* Gives the used list a few nodes with random ids; returns how many. */
static int add_assets(ZPrimAsset **out, int n)
{
    for (int i = 0; i < n; i++) {
        out[i] = dt_new_asset((uint16_t)dt_range(1, 0x1fff) | (uint16_t)((dt_u32() & 3) << 14),
                              (uint16_t)(dt_u32() & 3), 0x40);
    }
    return n;
}

static void asset_env(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
}

static void setup_asset_alloc(DtCtx *c)
{
    ZPrimAsset *a[6];
    asset_env(c);
    add_assets(a, dt_range(0, 5));
    if (c->seed % 6 == 5) {
        gZprimFree = NULL;
    }
}

static void setup_asset_unlink(DtCtx *c)
{
    ZPrimAsset *a[6];
    asset_env(c);
    const int n = add_assets(a, dt_range(1, 6));
    ZPrimAsset *pick = a[dt_range(0, n - 1)];
    if (c->seed % 8 == 7) {
        pick = DT_ASSET_NEW(); /* freshly linked at the head */
    }
    if (c->seed % 9 == 8) {
        pick = dt_alloc(sizeof(ZPrimAsset)); /* not in the list at all */
    }
    DT_ARG_PTR(c, 0, pick);
}

static void setup_asset_one(DtCtx *c)
{
    ZPrimAsset *a[4];
    asset_env(c);
    const int n = add_assets(a, dt_range(1, 4));
    DT_ARG_PTR(c, 0, a[dt_range(0, n - 1)]);
}

static void setup_asset_find(DtCtx *c)
{
    ZPrimAsset *a[6];
    asset_env(c);
    const int n = add_assets(a, dt_range(0, 6));
    int id = dt_range(1, 0x3fff);
    if (n && (c->seed & 1)) {
        id = a[dt_range(0, n - 1)]->id;
    }
    DT_ARG_INT(c, 0, (int32_t)((uint32_t)id | (dt_u32() & 0xffff0000u)));
}

/* A group asset holding `entries` ids; the data is {count, {id, offset}...}. */
static ZPrimAsset *make_group(uint32_t entries, const uint32_t *ids)
{
    const uint32_t size = 4 + entries * 8 + 0x40;
    ZPrimAsset *g = dt_new_asset((uint16_t)dt_range(0x100, 0x1ff), 1, size);
    uint32_t *t = (uint32_t *)dt_asset_data(g);
    t[0] = entries;
    for (uint32_t i = 0; i < entries; i++) {
        t[1 + i * 2] = ids[i];
        t[2 + i * 2] = (uint32_t)dt_range(0x20, 0x3c);
    }
    dt_asset_unlock(g);
    return g;
}

static void setup_group_find(DtCtx *c)
{
    asset_env(c);
    uint32_t ids[5];
    uint32_t pick = 0;
    for (int slot = 0; slot < 4; slot++) {
        if (dt_u32() % 3 == 0) {
            continue;
        }
        const uint32_t n = (uint32_t)dt_range(0, 5);
        for (uint32_t i = 0; i < n; i++) {
            ids[i] = (uint32_t)dt_range(1, 0x3000);
        }
        if (n) {
            pick = ids[dt_u32() % n];
        }
        gZprimGroups[slot] = make_group(n, ids);
    }
    DT_ARG_INT(
        c, 0,
        (int32_t)((pick && (c->seed & 1) ? pick : (uint32_t)dt_range(1, 0x3000)) | (dt_u32() & 0xffff0000u)));
}

static void setup_group_slot(DtCtx *c)
{
    asset_env(c);
    uint32_t ids[5];
    for (int slot = 0; slot < 4; slot++) {
        const uint32_t n = (uint32_t)dt_range(1, 5);
        for (uint32_t i = 0; i < n; i++) {
            ids[i] = (uint32_t)dt_range(1, 0x3000);
        }
        if (dt_u32() % 4 != 0) {
            gZprimGroups[slot] = make_group(n, ids);
        }
    }
    DT_ARG_INT(c, 0, (int32_t)((uint32_t)dt_range(0, 3) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 1, (int32_t)((uint32_t)dt_range(0, 1) | (dt_u32() & 0xffff0000u)));
}

static void setup_group_slot_present(DtCtx *c)
{
    setup_group_slot(c);
    const int32_t slot = (int32_t)(c->args[0] & 3);
    if (gZprimGroups[slot] == NULL) {
        uint32_t ids[2] = {5, 9};
        gZprimGroups[slot] = make_group(2, ids);
    }
}

DT_CASES(zprim_asset_cases) = {
    {"zprimAssetAlloc", 0x0047b830, zprimAssetAlloc, 0, DT_RET_INT, setup_asset_alloc, 60, 0},
    {"zprimAssetUnlink", 0x0047b880, zprimAssetUnlink, 1, DT_RET_VOID, setup_asset_unlink, 100, 0},
    {"zprimAssetData", 0x0047b900, zprimAssetData, 1, DT_RET_INT, setup_asset_one, 20, 0},
    {"zprimAssetRelease", 0x0047b930, zprimAssetRelease, 1, DT_RET_VOID, setup_asset_one, 20, 0},
    {"zprimAssetFind", 0x0047bac0, zprimAssetFind, 1, DT_RET_INT, setup_asset_find, 100, 0},
    {"zprimGroupFind", 0x0047baf0, zprimGroupFind, 1, DT_RET_INT, setup_group_find, 200, 0},
    {"zprimGroupLoaded", 0x0047b3d0, zprimGroupLoaded, 1, DT_RET_INT, setup_group_slot, 40, 0},
    {"zprimGroupCount", 0x0047ba10, zprimGroupCount, 1, DT_RET_INT16, setup_group_slot_present, 40, 0},
    {"zprimGroupSelect", 0x0047ba50, zprimGroupSelect, 2, DT_RET_INT, setup_group_slot_present, 40, 0},
};

/* ---- textures ---------------------------------------------------------------------------------- */

static void setup_create_texture(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    static const uint32_t bpps[] = {1, 2, 4, 8};
    const uint32_t bpp = bpps[dt_u32() % 4];
    const uint32_t w = (uint32_t)dt_range(1, 40);
    const uint32_t h = (uint32_t)dt_range(1, 40);
    const uint32_t rowBytes = (uint32_t)((uint64_t)w / (8 / (uint64_t)bpp));
    uint8_t *pixels = dt_alloc(rowBytes * h + 8);
    dt_fill(pixels, rowBytes * h + 8);
    uint16_t *palette = dt_alloc(8 + 2 * 300);
    palette[0] = (uint16_t)dt_u32();
    palette[1] = (uint16_t)dt_range(0, (1 << bpp) + 2);
    for (int i = 0; i < 300; i++) {
        palette[2 + i] = dt_rand_color();
    }
    gZprimTextureCounter = dt_u32();
    DT_ARG_INT(c, 0, (int32_t)w);
    DT_ARG_INT(c, 1, (int32_t)h);
    DT_ARG_INT(c, 2, (int32_t)bpp);
    DT_ARG_PTR(c, 3, pixels);
    DT_ARG_PTR(c, 4, palette);
}

static ZPrimAsset *new_picture_asset(uint16_t id, uint32_t w, uint32_t h, uint32_t bpp, uint32_t colors,
                                     uint16_t flags)
{
    DtBlob b = dt_gen_picture(w, h, bpp, colors);
    ZPrimAsset *a = dt_new_asset(id, flags, b.size);
    uint8_t *d = dt_asset_data(a);
    for (uint32_t i = 0; i < b.size; i++) {
        d[i] = b.data[i];
    }
    dt_asset_unlock(a);
    return a;
}

static void setup_picture_pair(DtCtx *c)
{
    asset_env(c);
    static const uint32_t bpps[] = {4, 8};
    const uint32_t bpp = bpps[dt_u32() % 2];
    ZPrimAsset *src = new_picture_asset((uint16_t)dt_range(1, 0x1fff), (uint32_t)dt_range(4, 40),
                                        (uint32_t)dt_range(4, 40), bpp, (uint32_t)dt_range(1, 1 << bpp), 1);
    ZPrimAsset *dst = new_picture_asset((uint16_t)dt_range(1, 0x1fff), (uint32_t)dt_range(4, 40),
                                        (uint32_t)dt_range(4, 40), bpp, (uint32_t)dt_range(1, 1 << bpp), 1);
    uint8_t *d = dt_asset_data(dst);
    *(int32_t *)(d + 0x20) = (c->seed & 1) ? (int32_t)dt_range(1, 0x5000) : 0;
    dt_asset_unlock(dst);
    d = dt_asset_data(src);
    *(int32_t *)(d + 0x20) = (int32_t)dt_range(1, 0x5000);
    dt_asset_unlock(src);
    DT_ARG_PTR(c, 0, src);
    DT_ARG_PTR(c, 1, dst);
    DT_ARG_INT(c, 2, (int32_t)dt_u32());
    DT_ARG_INT(c, 3, (int32_t)dt_u32());
}

static void setup_picture_pair_slice(DtCtx *c)
{
    asset_env(c);
    ZPrimAsset *src = new_picture_asset((uint16_t)dt_range(1, 0x1fff), (uint32_t)dt_range(4, 40),
                                        (uint32_t)dt_range(4, 40), 8, 256, 1);
    ZPrimAsset *dst = new_picture_asset((uint16_t)dt_range(1, 0x1fff), (uint32_t)dt_range(4, 40),
                                        (uint32_t)dt_range(4, 40), 8, 16, 1);
    uint8_t *d = dt_asset_data(dst);
    *(int32_t *)(d + 0x20) = (c->seed & 1) ? (int32_t)dt_range(1, 0x5000) : 0;
    dt_asset_unlock(dst);
    DT_ARG_PTR(c, 0, src);
    DT_ARG_PTR(c, 1, dst);
    DT_ARG_INT(c, 2, (int32_t)dt_u32());
    DT_ARG_INT(c, 3, (int32_t)((uint32_t)dt_range(0, 15) | (dt_u32() & ~0xffu)));
}

static void setup_picture_size(DtCtx *c)
{
    asset_env(c);
    DT_ARG_PTR(c, 0,
               new_picture_asset((uint16_t)dt_range(1, 0x1fff), (uint32_t)dt_range(1, 60),
                                 (uint32_t)dt_range(1, 60), 8, 8, 1));
}

DT_CASES(zprim_texture_cases) = {
    {"zprimCreateTexture", 0x0047c030, zprimCreateTexture, 5, DT_RET_INT, setup_create_texture, 400, 0},
    {"zprimCopyTexture", 0x00479280, zprimCopyTexture, 2, DT_RET_VOID, setup_picture_pair, 40, 0},
    {"zprimReplaceVramGrey", 0x00479300, zprimReplaceVramGrey, 4, DT_RET_VOID, setup_picture_pair, 200, 0},
    {"zprimReplaceVramPalette", 0x00479500, zprimReplaceVramPalette, 4, DT_RET_VOID, setup_picture_pair_slice,
     100, 0},
    {"zprimPictureWidth", 0x004791e0, zprimPictureWidth, 1, DT_RET_INT16, setup_picture_size, 40, 0},
    {"zprimPictureHeight", 0x00479210, zprimPictureHeight, 1, DT_RET_INT16, setup_picture_size, 40, 0},
    {"zprimNop1", 0x00479240, zprimNop1, 0, DT_RET_VOID, setup_state_none, 1, 0},
    {"zprimNop2", 0x00479260, zprimNop2, 4, DT_RET_VOID, setup_state_none, 1, 0},
    {"zprimNop3", 0x0047a600, zprimNop3, 0, DT_RET_VOID, setup_state_none, 1, 0},
    {"zprimNop4", 0x0047aa60, zprimNop4, 0, DT_RET_VOID, setup_state_none, 1, 0},
    {"zprimNop5", 0x0047c010, zprimNop5, 0, DT_RET_VOID, setup_state_none, 1, 0},
};

/* ---- loaders (through a fake library read by the exe's own zasset code) ------------------------- */

enum { kLoadPicture, kLoadSprite, kLoadFont, kLoadScreen };

static DtBlob gen_asset_blob(int kind)
{
    static const uint32_t bpps[] = {4, 8};
    const uint32_t bpp = bpps[dt_u32() % 2];
    switch (kind) {
    case kLoadPicture:
        return dt_gen_picture((uint32_t)dt_range(2, 30), (uint32_t)dt_range(2, 30), bpp,
                              (uint32_t)dt_range(1, 1 << bpp));
    case kLoadSprite:
        return dt_gen_sprite((uint32_t)dt_range(0, 4), (uint32_t)dt_range(2, 20), (uint32_t)dt_range(2, 20),
                             bpp, (int)(dt_u32() & 1));
    case kLoadFont:
        return dt_gen_font((uint32_t)dt_range(1, 20), (uint32_t)dt_range(0x20, 0x60));
    default: {
        DtBlob b = {dt_alloc(0x40), 0x40};
        dt_fill(b.data, 0x40);
        return b;
    }
    }
}

static uint32_t kind_type_bits(int kind)
{
    static const uint32_t spriteTypes[] = {0x4000, 0x6000, 0xc000};
    switch (kind) {
    case kLoadPicture:
        return 0;
    case kLoadSprite:
        return spriteTypes[dt_u32() % 3];
    case kLoadFont:
        return 0x8000;
    default:
        return 0xa000;
    }
}

/* Group asset with the blob stored inside it at offset 0x40. */
static ZPrimAsset *make_group_with(uint32_t id, const DtBlob *blob)
{
    const uint32_t size = 0x40 + blob->size;
    ZPrimAsset *g = dt_new_asset((uint16_t)dt_range(0x100, 0x1ff), (uint16_t)(dt_u32() & 1), size);
    uint8_t *d = dt_asset_data(g);
    uint32_t *t = (uint32_t *)d;
    t[0] = 1;
    t[1] = id & 0xffff;
    t[2] = 0x40;
    for (uint32_t i = 0; i < blob->size; i++) {
        d[0x40 + i] = blob->data[i];
    }
    dt_asset_unlock(g);
    return g;
}

/*
 * variant: 0 library, 1 missing, 2 already loaded, 3 inside a group, 4 asset table exhausted, 5 wrong type
 * bits. The result is compared through the returned node, the asset table, the DMM pool and the texture log.
 */
static void setup_loader(DtCtx *c, int kind)
{
    DtFakeLib lib;
    dt_seed(c);
    dt_zprim_env();
    dt_fake_lib_begin(&lib, 12, 0x8000);
    gZprimLibrary = (struct ZassetLibrary *)lib.lib;
    const uint32_t variant = c->seed % 8;
    const uint32_t id = (uint32_t)dt_range(1, 0x1f00) | kind_type_bits(kind);
    DtBlob blob = gen_asset_blob(kind);
    for (int i = 0; i < 3; i++) {
        DtBlob other = gen_asset_blob(kind);
        dt_fake_lib_add(&lib, (uint32_t)dt_range(0x1f01, 0x1fff) | kind_type_bits(kind), other.data,
                        other.size);
    }
    switch (variant) {
    case 1:
        break; /* not added: the loader reports it missing */
    case 2:
        dt_fake_lib_add(&lib, id, blob.data, blob.size);
        dt_new_asset((uint16_t)id, 0, 0x40);
        break;
    case 3:
        gZprimGroups[dt_range(0, 3)] = make_group_with(id, &blob);
        break;
    case 4:
        dt_fake_lib_add(&lib, id, blob.data, blob.size);
        gZprimFree = NULL;
        break;
    default:
        dt_fake_lib_add(&lib, id, blob.data, blob.size);
    }
    uint32_t arg = id;
    if (variant == 5) {
        arg = (id & 0x1fff) | (kind == kLoadPicture ? 0xe000u : 0x0000u);
    }
    DT_ARG_INT(c, 0, (int32_t)(arg | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 1, (int32_t)(dt_u32() & 1));
}

static void setup_load_picture(DtCtx *c)
{
    setup_loader(c, kLoadPicture);
}

static void setup_load_sprite(DtCtx *c)
{
    setup_loader(c, kLoadSprite);
}

static void setup_load_font(DtCtx *c)
{
    setup_loader(c, kLoadFont);
}

static void setup_load_screen(DtCtx *c)
{
    setup_loader(c, kLoadScreen);
}

static void setup_load_asset(DtCtx *c)
{
    static const int kinds[] = {kLoadPicture, kLoadSprite, kLoadFont, kLoadScreen};
    setup_loader(c, kinds[(c->seed / 8) % 4]);
    if (c->seed % 41 == 40) {
        c->args[0] = 0xe123; /* unknown type: asserts */
    }
}

DT_CASES(zprim_loader_cases) = {
    {"zprimLoadPicture", 0x00479010, zprimLoadPicture, 2, DT_RET_INT, setup_load_picture, 160, 0},
    {"zprimLoadSprite", 0x00479660, zprimLoadSprite, 2, DT_RET_INT, setup_load_sprite, 160, 0},
    {"zprimLoadFont", 0x0047a420, zprimLoadFont, 2, DT_RET_INT, setup_load_font, 160, 0},
    {"zprimLoadScreen", 0x0047bb90, zprimLoadScreen, 2, DT_RET_INT, setup_load_screen, 160, 0},
    {"zprimLoadAsset", 0x0047b950, zprimLoadAsset, 1, DT_RET_INT, setup_load_asset, 320, 0},
};

/* ---- groups ------------------------------------------------------------------------------------ */

/* Group file: {count, {id, offset}...} followed by the member blobs. Returns the blob. */
static DtBlob gen_group_blob(int members, int pictureOnly, uint32_t firstCountField)
{
    uint32_t ids[4];
    DtBlob blobs[4];
    uint32_t total = 4 + (uint32_t)members * 8;
    for (int i = 0; i < members; i++) {
        const int kind = pictureOnly ? kLoadPicture : dt_range(0, 3);
        ids[i] = (uint32_t)dt_range(1, 0x1f00) | kind_type_bits(kind);
        blobs[i] = gen_asset_blob(kind);
        if (pictureOnly && firstCountField) {
            *(uint16_t *)blobs[i].data = 1; /* the count field zprimLoadGroupPicture looks for */
        }
        total += (blobs[i].size + 3) & ~3u;
    }
    DtBlob g = {dt_alloc(total + 16), total + 16};
    uint32_t *t = (uint32_t *)g.data;
    t[0] = (uint32_t)members;
    uint32_t at = 4 + (uint32_t)members * 8;
    for (int i = 0; i < members; i++) {
        t[1 + i * 2] = ids[i];
        t[2 + i * 2] = at;
        for (uint32_t k = 0; k < blobs[i].size; k++) {
            g.data[at + k] = blobs[i].data[k];
        }
        at += (blobs[i].size + 3) & ~3u;
    }
    return g;
}

typedef int (*OrigLoadGroupFn)(uint32_t, uint32_t);
#define ORIG_LOAD_GROUP DT_ORIG(0x0047b400, OrigLoadGroupFn)

static void setup_load_group(DtCtx *c)
{
    DtFakeLib lib;
    dt_seed(c);
    dt_zprim_env();
    dt_fake_lib_begin(&lib, 12, 0x10000);
    gZprimLibrary = (struct ZassetLibrary *)lib.lib;
    /* Ids >= 0x8000 too (the frontend loads 0xe000): the original zero-extends the group. */
    const uint32_t group =
        c->seed % 3 == 2 ? (uint32_t)dt_range(0x8000, 0xffff) : (uint32_t)dt_range(0x100, 0x1ff);
    DtBlob g = gen_group_blob(dt_range(1, 4), 0, 0);
    dt_fake_lib_add(&lib, group, g.data, g.size);
    const uint32_t slot = (uint32_t)dt_range(0, 3);
    if (c->seed % 6 == 5) {
        gZprimGroups[slot] = dt_new_asset(0x123, 1, 0x40); /* slot in use */
    }
    DT_ARG_INT(c, 0, (int32_t)((c->seed % 11 == 10 ? slot + 4 : slot) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 1, (int32_t)(group | (dt_u32() & 0xffff0000u)));
}

static void setup_free_group(DtCtx *c)
{
    DtFakeLib lib;
    dt_seed(c);
    dt_zprim_env();
    dt_fake_lib_begin(&lib, 12, 0x10000);
    gZprimLibrary = (struct ZassetLibrary *)lib.lib;
    /* Ids >= 0x8000 too (the frontend loads 0xe000): the original zero-extends the group. */
    const uint32_t group =
        c->seed % 3 == 2 ? (uint32_t)dt_range(0x8000, 0xffff) : (uint32_t)dt_range(0x100, 0x1ff);
    DtBlob g = gen_group_blob(dt_range(1, 4), 0, 0);
    dt_fake_lib_add(&lib, group, g.data, g.size);
    const uint32_t slot = (uint32_t)dt_range(0, 3);
    if (c->seed % 5 != 4) {
        ORIG_LOAD_GROUP(slot, group);
    }
    DT_ARG_INT(c, 0, (int32_t)((c->seed % 13 == 12 ? 4 + slot : slot) | (dt_u32() & 0xffff0000u)));
}

static void setup_group_picture(DtCtx *c)
{
    DtFakeLib lib;
    dt_seed(c);
    dt_zprim_env();
    dt_fake_lib_begin(&lib, 12, 0x10000);
    gZprimLibrary = (struct ZassetLibrary *)lib.lib;
    const uint32_t group = (uint32_t)dt_range(0x100, 0x1ff);
    DtBlob g = gen_group_blob(dt_range(1, 3), 1, c->seed % 3 != 0);
    dt_fake_lib_add(&lib, group, g.data, g.size);
    uint32_t entry = *(uint32_t *)(g.data + 4);
    if (c->seed % 7 == 6) {
        entry = 0x77; /* not in the group */
    }
    DT_ARG_INT(c, 0, (int32_t)(group | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 1, (int32_t)(entry | (dt_u32() & 0xffff0000u)));
}

/* ---- freeing assets ---------------------------------------------------------------------------- */

typedef void *(*OrigFreeListAllocFn)(struct ZMemFreeList *);
#define ORIG_FREELIST_ALLOC DT_ORIG(0x00486050, OrigFreeListAllocFn)

static void setup_asset_free(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    const int kind = dt_range(0, 4);
    const uint16_t flags = (uint16_t)(dt_u32() & 3);
    ZPrimAsset *extra = dt_new_asset(0x55, 0, 0x40);
    ZPrimAsset *a;
    if (kind == 0) {
        DtBlob b = gen_asset_blob(kLoadPicture);
        a = dt_new_asset((uint16_t)dt_range(1, 0x1fff), flags, b.size);
        uint8_t *d = dt_asset_data(a);
        for (uint32_t i = 0; i < b.size; i++) {
            d[i] = b.data[i];
        }
        *(int32_t *)(d + 0x20) = (c->seed & 1) ? (int32_t)dt_range(1, 0x6000) : 0;
        dt_asset_unlock(a);
    } else if (kind == 1) {
        DtBlob b = gen_asset_blob(kLoadSprite);
        a = dt_new_asset((uint16_t)(dt_range(1, 0x1fff) | kind_type_bits(kLoadSprite)), flags, b.size);
        uint8_t *d = dt_asset_data(a);
        for (uint32_t i = 0; i < b.size; i++) {
            d[i] = b.data[i];
        }
        for (uint32_t f = 0; f < *(uint16_t *)d; f++) {
            *(int32_t *)(d + 0x20 + f * 16) = dt_u32() & 1 ? (int32_t)dt_range(1, 0x6000) : 0;
        }
        dt_asset_unlock(a);
    } else if (kind == 2) {
        DtBlob b = gen_asset_blob(kLoadFont);
        a = dt_new_asset((uint16_t)(dt_range(1, 0x1fff) | 0x8000), flags, b.size);
        uint8_t *d = dt_asset_data(a);
        for (uint32_t i = 0; i < b.size; i++) {
            d[i] = b.data[i];
        }
        if (c->seed & 1) {
            int32_t *pages = ORIG_FREELIST_ALLOC(gZprimFontPageList);
            for (int i = 0; i < 32; i++) {
                pages[i] = (dt_u32() % 3 == 0) ? 0 : (int32_t)dt_range(1, 0x6000);
            }
            *(int32_t **)(d + 0xc) = pages;
        } else {
            *(int32_t *)(d + 0xc) = 0;
        }
        dt_asset_unlock(a);
    } else {
        DtBlob b = gen_asset_blob(kLoadScreen);
        a = dt_new_asset((uint16_t)(dt_range(1, 0x1fff) | 0xa000), flags, b.size);
        uint8_t *d = dt_asset_data(a);
        for (uint32_t i = 0; i < b.size; i++) {
            d[i] = b.data[i];
        }
        dt_asset_unlock(a);
    }
    (void)extra;
    DT_ARG_PTR(c, 0, a);
}

DT_CASES(zprim_group_cases) = {
    {"zprimLoadGroup", 0x0047b400, zprimLoadGroup, 2, DT_RET_INT, setup_load_group, 120, 0},
    {"zprimFreeGroup", 0x0047b5c0, zprimFreeGroup, 1, DT_RET_VOID, setup_free_group, 120, 0},
    {"zprimLoadGroupPicture", 0x0047b220, zprimLoadGroupPicture, 2, DT_RET_INT, setup_group_picture, 120, 0},
    {"zprimAssetFree", 0x0047b6b0, zprimAssetFree, 1, DT_RET_VOID, setup_asset_free, 200, 0},
    {"zprimFreePicture", 0x004791c0, zprimFreePicture, 1, DT_RET_VOID, setup_asset_free, 40, 0},
    {"zprimFreeSprite", 0x00479cd0, zprimFreeSprite, 1, DT_RET_VOID, setup_asset_free, 40, 0},
    {"zprimFreeFont", 0x0047a5e0, zprimFreeFont, 1, DT_RET_VOID, setup_asset_free, 40, 0},
    {"zprimFreeScreen", 0x0047bd00, zprimFreeScreen, 1, DT_RET_VOID, setup_asset_free, 40, 0},
};

/* ---- sprites ----------------------------------------------------------------------------------- */

static void setup_create_sprite(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    gZprimNextId = (uint16_t)dt_u32();
    if (c->seed % 9 == 8) {
        gZprimFree = NULL;
    }
    DT_ARG_INT(c, 0, (int32_t)((uint32_t)dt_range(1, 24) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 1, (int32_t)((uint32_t)dt_range(1, 24) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 2, (int32_t)((uint32_t)dt_range(1, 6) | (dt_u32() & 0xffff0000u)));
}

static void setup_create_sprite_small(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    gZprimNextId = (uint16_t)dt_u32();
    if (c->seed % 9 == 8) {
        gZprimFree = NULL;
    }
    DT_ARG_INT(c, 0, (int32_t)((uint32_t)dt_range(1, 64) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 1, (int32_t)((uint32_t)dt_range(1, 64) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 2, (int32_t)((uint32_t)dt_range(1, 8) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 3,
               (int32_t)((uint32_t)(c->seed % 5 == 0 ? 0 : dt_range(1, 200)) | (dt_u32() & 0xffff0000u)));
}

/* A sprite asset (type `typeBits`) in the DMM pool built from a generated blob. */
static ZPrimAsset *new_sprite_asset(uint32_t typeBits, uint32_t frames, uint32_t w, uint32_t h, uint32_t bpp,
                                    int perFramePalette)
{
    DtBlob b = dt_gen_sprite(frames, w, h, bpp, perFramePalette);
    ZPrimAsset *a =
        dt_new_asset((uint16_t)(dt_range(1, 0x1fff) | typeBits), (uint16_t)(dt_u32() & 3), b.size);
    uint8_t *d = dt_asset_data(a);
    for (uint32_t i = 0; i < b.size; i++) {
        d[i] = b.data[i];
    }
    for (uint32_t f = 0; f < frames; f++) {
        *(int32_t *)(d + 0x20 + f * 16) = (dt_u32() & 1) ? (int32_t)dt_range(1, 0x6000) : 0;
    }
    dt_asset_unlock(a);
    return a;
}

static void setup_sprite_touch(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    DT_ARG_PTR(c, 0, new_sprite_asset(0xc000, 2, 8, 8, 8, 0));
}

static void setup_sprite_reset(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    DT_ARG_PTR(c, 0, new_sprite_asset(0xc000, (uint32_t)dt_range(1, 3), 8, 8, 8, 0));
    DT_ARG_INT(c, 1, (int32_t)((uint32_t)dt_range(1, 24) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 2, (int32_t)((uint32_t)dt_range(1, 24) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 3, (int32_t)((uint32_t)dt_range(1, 8) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 4,
               (int32_t)((uint32_t)(c->seed % 4 == 0 ? 0 : dt_range(1, 100)) | (dt_u32() & 0xffff0000u)));
}

static void setup_sprite_getter(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    gZprimDepth = dt_range(0, 5);
    static const uint32_t types[] = {0x4000, 0x6000, 0xc000};
    ZPrimAsset *a = new_sprite_asset(types[dt_u32() % 3], (uint32_t)dt_range(1, 5), (uint32_t)dt_range(2, 16),
                                     (uint32_t)dt_range(2, 16), 8, (int)(dt_u32() & 1));
    if (c->seed % 3 == 0) {
        gZprimStates[gZprimDepth].picture = a; /* use the state's picture */
        a = NULL;
    } else if (c->seed % 11 == 10) {
        a = NULL;
        gZprimStates[gZprimDepth].picture = NULL;
    }
    if (c->seed & 1) {
        gZprimStates = NULL;
    }
    DT_ARG_PTR(c, 0, a);
    DT_ARG_INT(c, 1, (int32_t)((uint32_t)dt_range(0, 6) | (dt_u32() & 0xffff0000u)));
}

static void setup_sprite_frame_data(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    ZPrimAsset *a = new_sprite_asset(0x6000, (uint32_t)dt_range(1, 5), 8, 8, 8, 0);
    DT_ARG_PTR(c, 0, dt_asset_data(a));
    DT_ARG_INT(c, 1, (int32_t)((uint32_t)dt_range(0, 6) | (dt_u32() & 0xffff0000u)));
}

/* Run length coded stream of about `tokens` tokens; returns its size. */
static uint32_t put_rle(uint8_t *p, int tokens)
{
    uint8_t *q = p + 2;
    for (int i = 0; i < tokens; i++) {
        switch (dt_u32() % 4) {
        case 0: /* fill: 0x80, value, u16 count */
            *q++ = 0x80;
            *q++ = (uint8_t)dt_u32();
            *(uint16_t *)q = (uint16_t)dt_range(0, 40);
            q += 2;
            break;
        case 1: /* zeros: 0, u16 count */
            *q++ = 0;
            *(uint16_t *)q = (uint16_t)dt_range(0, 40);
            q += 3 - 1;
            break;
        case 2: { /* literal */
            const uint8_t n = (uint8_t)dt_range(1, 30);
            *q++ = n;
            for (uint8_t k = 0; k < n; k++) {
                *q++ = (uint8_t)dt_u32();
            }
            break;
        }
        default: /* short fill */
            *q++ = (uint8_t)dt_range(0x81, 0xff);
            *q++ = (uint8_t)dt_u32();
        }
    }
    *(uint16_t *)p = (uint16_t)(q - p);
    return (uint32_t)(q - p);
}

static void setup_rle(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    uint8_t *src = dt_alloc(0x1000);
    dt_fill(src, 0x1000);
    put_rle(src, dt_range(0, 12));
    uint8_t *dst = dt_alloc(0x4000);
    dt_fill(dst, 0x4000);
    DT_ARG_PTR(c, 0, src);
    DT_ARG_PTR(c, 1, dst);
}

static void setup_decompress(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    ZPrimAsset *src = new_sprite_asset(0x6000, (uint32_t)dt_range(1, 3), 16, 16, 8, 0);
    ZPrimAsset *dst = new_sprite_asset(0xc000, (uint32_t)dt_range(1, 3), 60, 30, 8, 0);
    uint8_t *sd = dt_asset_data(src);
    const uint32_t frames = *(uint16_t *)sd;
    /* turn the pixel data of each source frame into a run length stream (at the end of the block is spare
     * room) */
    for (uint32_t f = 0; f < frames; f++) {
        uint8_t *stream = sd + *(int32_t *)(sd + 0x18 + f * 16);
        put_rle(stream, dt_range(0, 4)); /* at most 134 bytes, frames are 256 */
    }
    dt_asset_unlock(src);
    if (c->seed % 7 == 6) {
        ZPrimAsset *t = src; /* wrong types */
        src = dst;
        dst = t;
    }
    DT_ARG_PTR(c, 0, src);
    DT_ARG_INT(c, 1, (int32_t)((uint32_t)dt_range(0, 3) | (dt_u32() & 0xffff0000u)));
    DT_ARG_PTR(c, 2, dst);
    DT_ARG_INT(c, 3, (int32_t)((uint32_t)dt_range(0, 3) | (dt_u32() & 0xffff0000u)));
}

static void setup_draw_sprite(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    gZprimDepth = dt_range(0, 5);
    static const uint32_t types[] = {0x4000, 0x6000, 0xc000};
    ZPrimState *st = &gZprimStates[gZprimDepth];
    const uint32_t frames = (uint32_t)dt_range(1, 4);
    st->picture = new_sprite_asset(types[dt_u32() % 3], frames, (uint32_t)dt_range(2, 16),
                                   (uint32_t)dt_range(2, 16), 8, (int)(dt_u32() & 1));
    st->flagsB = (uint16_t)dt_u32();
    st->field20 = (int32_t)dt_u32();
    st->field24 = (uint16_t)dt_u32();
    st->field30 = (int32_t)dt_u32();
    st->field34 = (c->seed % 3 == 0) ? 0 : dt_range(1, 9);
    st->field38 = (int32_t)dt_u32();
    if (c->seed % 13 == 12) {
        st->picture = NULL;
    }
    if (c->seed & 1) {
        gZprimStates = NULL;
    }
    uint32_t frame = (uint32_t)dt_range(0, (int)frames + 1);
    if (c->seed % 17 == 16) {
        frame = 0xffff;
    }
    DT_ARG_INT(c, 0, (int32_t)(frame | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 1, (int32_t)((uint32_t)dt_range(-40, 700) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 2, (int32_t)((uint32_t)dt_range(-40, 500) | (dt_u32() & 0xffff0000u)));
}

DT_CASES(zprim_sprite_cases) = {
    {"zprimCreateSprite", 0x00479880, zprimCreateSprite, 3, DT_RET_INT, setup_create_sprite, 120, 0},
    {"zprimCreateSpriteSmall", 0x00479b90, zprimCreateSpriteSmall, 4, DT_RET_INT, setup_create_sprite_small,
     120, 0},
    {"zprimSpriteTouch", 0x00479a80, zprimSpriteTouch, 1, DT_RET_VOID, setup_sprite_touch, 10, 0},
    {"zprimSpriteReset", 0x00479ab0, zprimSpriteReset, 5, DT_RET_VOID, setup_sprite_reset, 100, 0},
    {"zprimSpriteFrameWidth", 0x00479fa0, zprimSpriteFrameWidth, 2, DT_RET_INT16, setup_sprite_getter, 120,
     0},
    {"zprimSpriteFrameHeight", 0x0047a070, zprimSpriteFrameHeight, 2, DT_RET_INT16, setup_sprite_getter, 120,
     0},
    {"zprimSpriteFrameCount", 0x0047a140, zprimSpriteFrameCount, 1, DT_RET_INT16, setup_sprite_getter, 120,
     0},
    {"zprimSpriteFrameData", 0x0047a300, zprimSpriteFrameData, 2, DT_RET_INT, setup_sprite_frame_data, 60, 0},
    {"zprimRleDecode", 0x0047a330, zprimRleDecode, 2, DT_RET_INT, setup_rle, 200, 0},
    {"zprimDecompress", 0x0047a1e0, zprimDecompress, 4, DT_RET_VOID, setup_decompress, 120, 0},
    {"zprimDrawSprite", 0x00479cf0, zprimDrawSprite, 3, DT_RET_VOID, setup_draw_sprite, 400, 0},
};

/* ---- text -------------------------------------------------------------------------------------- */

static ZPrimAsset *new_font_asset(uint32_t glyphs, uint32_t first)
{
    DtBlob b = dt_gen_font(glyphs, first);
    ZPrimAsset *a = dt_new_asset((uint16_t)(dt_range(1, 0x1fff) | 0x8000), (uint16_t)(dt_u32() & 3), b.size);
    uint8_t *d = dt_asset_data(a);
    for (uint32_t i = 0; i < b.size; i++) {
        d[i] = b.data[i];
    }
    dt_asset_unlock(a);
    return a;
}

/* Random text: characters 3 and up (0 to 2 end the string), mostly printable, some outside the glyph range.
 */
static uint8_t *rand_text(int maxLen, int low, int high)
{
    uint8_t *t = dt_alloc((uint32_t)maxLen + 4);
    const int len = dt_range(0, maxLen);
    for (int i = 0; i < len; i++) {
        t[i] = (uint8_t)dt_range(low, high);
    }
    t[len] = (uint8_t)dt_range(0, 2);
    return t;
}

static void text_env(DtCtx *c, int big, int allowNoFont)
{
    dt_seed(c);
    dt_zprim_env();
    gZprimDepth = dt_range(0, 5);
    ZPrimState *st = &gZprimStates[gZprimDepth];
    st->font = big ? new_font_asset((uint32_t)dt_range(0x100, 0x130), 0)
                   : new_font_asset((uint32_t)dt_range(8, 64), (uint32_t)dt_range(0x20, 0x30));
    st->flagsC = (uint16_t)dt_u32();
    st->field40 = (int32_t)((uint32_t)dt_range(0, 3) | (dt_u32() & 0xffff0000u));
    st->field4c = (int32_t)dt_u32();
    st->field48 = (int32_t)dt_u32();
    st->field34 = (c->seed % 3 == 0) ? 0 : dt_range(1, 9);
    st->field38 = (int32_t)dt_u32();
    st->spacing = (int32_t)((uint32_t)dt_range(0, 4) | (dt_u32() & 0xffff0000u));
    gZprimClip.x0 = (int16_t)dt_range(-5, 20);
    gZprimClip.y0 = (int16_t)dt_range(-5, 20);
    gZprimClip.x1 = (int16_t)dt_range(30, 200);
    gZprimClip.y1 = (int16_t)dt_range(30, 200);
    if (allowNoFont && c->seed % 23 == 22) {
        st->font = NULL;
    }
    if (c->seed & 1) {
        gZprimStates = NULL;
    }
}

static void setup_draw_text_small(DtCtx *c)
{
    text_env(c, 0, 1);
    DT_ARG_PTR(c, 0, rand_text(8, 3, 0x7e));
    DT_ARG_INT(c, 1, (int32_t)((uint32_t)dt_range(-10, 120) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 2, (int32_t)((uint32_t)dt_range(-10, 120) | (dt_u32() & 0xffff0000u)));
}

static void setup_draw_text_big(DtCtx *c)
{
    text_env(c, 1, 1);
    DT_ARG_PTR(c, 0, rand_text(8, 3, 0x7e));
    DT_ARG_INT(c, 1, (int32_t)((uint32_t)dt_range(-10, 120) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 2, (int32_t)((uint32_t)dt_range(-10, 120) | (dt_u32() & 0xffff0000u)));
}

static void setup_draw_text_mixed(DtCtx *c)
{
    if (c->seed & 2) {
        setup_draw_text_big(c);
    } else {
        setup_draw_text_small(c);
    }
}

static void setup_text_width(DtCtx *c)
{
    text_env(c, (int)((c->seed >> 1) & 1), 0);
    ZPrimAsset *font = NULL;
    if (c->seed % 3 == 0) {
        font = new_font_asset((uint32_t)dt_range(8, 64), (uint32_t)dt_range(0x20, 0x30));
    }
    DT_ARG_PTR(c, 0, font);
    DT_ARG_PTR(c, 1, rand_text(10, 3, 0x7e));
}

static void setup_font_flags(DtCtx *c)
{
    text_env(c, 0, 0);
    ZPrimAsset *font = NULL;
    if (c->seed % 3 != 0) {
        font = new_font_asset((uint32_t)dt_range(8, 64), (uint32_t)dt_range(0x20, 0x30));
    }
    DT_ARG_PTR(c, 0, font);
}

static void setup_text_to_sprite(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    gZprimDepth = dt_range(0, 5);
    ZPrimState *st = &gZprimStates[gZprimDepth];
    ZPrimAsset *font = new_font_asset(64, 0x20);
    ZPrimAsset *sprite = new_sprite_asset(0xc000, (uint32_t)dt_range(1, 3), 60, 30, 8, 0);
    st->spacing = (int32_t)dt_range(0, 3);
    if (c->seed % 9 == 8) {
        sprite = new_sprite_asset(0x4000, 1, 60, 30, 8, 0); /* wrong type */
    }
    if (c->seed & 1) {
        gZprimStates = NULL;
    }
    DT_ARG_PTR(c, 0, font);
    DT_ARG_PTR(c, 1, rand_text(6, 0x20, 0x5f));
    DT_ARG_INT(c, 2, (int32_t)((uint32_t)dt_range(4, 10) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 3, (int32_t)((uint32_t)dt_range(4, 10) | (dt_u32() & 0xffff0000u)));
    DT_ARG_PTR(c, 4, sprite);
    DT_ARG_INT(c, 5,
               (int32_t)((uint32_t)(c->seed % 10 == 9 ? 7 : dt_range(0, 2)) | (dt_u32() & 0xffff0000u)));
    DT_ARG_INT(c, 6, (int32_t)dt_u32());
}

DT_CASES(zprim_text_cases) = {
    {"zprimDrawTextMono", 0x0047a620, zprimDrawTextMono, 3, DT_RET_VOID, setup_draw_text_mixed, 400, 0},
    {"zprimDrawText", 0x0047aa80, zprimDrawText, 3, DT_RET_VOID, setup_draw_text_mixed, 400, 0},
    {"zprimTextWidth", 0x0047afd0, zprimTextWidth, 2, DT_RET_INT16, setup_text_width, 300, 0},
    {"zprimFontFlags", 0x0047af10, zprimFontFlags, 1, DT_RET_INT16, setup_font_flags, 60, 0},
    {"zprimDrawTextToSprite", 0x0047bd20, zprimDrawTextToSprite, 7, DT_RET_VOID, setup_text_to_sprite, 200,
     0},
};

/* ---- setup and teardown, frame start, flush ----------------------------------------------------- */

static void BMX_CDECL dt_ngif_destroy_stub(int object)
{
    dt_rw_log.freeSum += (uint32_t)object * 3;
}

typedef void(BMX_CDECL *NgifDestroyFn)(int);
#define gNgifDestroy ORIG_VAR(0x00561474, NgifDestroyFn, gNgifDestroy)

static void setup_init(DtCtx *c)
{
    dt_seed(c);
    /* seed % 5 == 4: room for the DMM pool below, but too small for zprimInit's asset table */
    dt_static_region(c->seed % 5 == 4 ? 0x40000 + 0x300 : 0x80000);
    dt_zmem_env();
    gNgifObject = 1;
    DT_DMM_INIT(0, c->seed % 5 == 3 ? 0x200 : 0x40000); /* too small for the state stack */
}

static void setup_shutdown(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    gNgifDestroy = dt_ngif_destroy_stub;
    gZprimLibrary = 0;
    if (c->seed % 3 == 0) {
        gZprimHandle = 0;
    }
}

static void setup_open_library(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    gZprimLibrary = 0;
    DT_ARG_PTR(c, 0, "missing-prim.lib");
}

static void setup_clear(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    gZprimDepth = dt_range(0, 5);
    ZPrimState *st = &gZprimStates[gZprimDepth];
    st->kind = (uint16_t)(c->seed % 7);
    st->color = dt_u32();
    gZprimClip.x0 = (int16_t)dt_range(-5, 20);
    gZprimClip.y0 = (int16_t)dt_range(-5, 20);
    gZprimClip.x1 = (int16_t)dt_range(30, 700);
    gZprimClip.y1 = (int16_t)dt_range(30, 500);
    st->asset = new_picture_asset((uint16_t)dt_range(1, 0x1fff), (uint32_t)dt_range(2, 30),
                                  (uint32_t)dt_range(2, 30), 8, 8, 1);
    if (c->seed & 1) {
        gZprimStates = NULL;
    }
}

static void setup_flush(DtCtx *c)
{
    dt_seed(c);
    dt_zprim_env();
    gZprimDepth = dt_range(0, 5);
    const int n = dt_range(0, 6);
    for (int i = 0; i < n; i++) {
        switch (dt_range(0, 3)) {
        case 0:
            new_picture_asset((uint16_t)dt_range(1, 0x1fff), 8, 8, 8, 8, (uint16_t)(dt_u32() & 3));
            break;
        case 1:
            new_sprite_asset(0x4000, (uint32_t)dt_range(1, 3), 8, 8, 8, 0);
            break;
        case 2:
            new_font_asset((uint32_t)dt_range(4, 20), 0x20);
            break;
        default:
            dt_new_asset((uint16_t)(dt_range(1, 0x1fff) | 0xa000), (uint16_t)(dt_u32() & 3), 0x40);
        }
    }
    gZprimNextId = (uint16_t)dt_u32();
    dt_fill(gZprimStates, 0x348);
}

DT_CASES(zprim_misc_cases) = {
    {"zprimInit", 0x004779e0, zprimInit, 0, DT_RET_INT, setup_init, 50, 0},
    {"zprimShutdown", 0x00477b50, zprimShutdown, 0, DT_RET_VOID, setup_shutdown, 30, 0},
    {"zprimOpenLibrary", 0x00477ba0, zprimOpenLibrary, 1, DT_RET_VOID, setup_open_library, 2, 0},
    {"zprimClear", 0x00477be0, zprimClear, 0, DT_RET_VOID, setup_clear, 100, 0},
    {"zprimFlush", 0x00477d20, zprimFlush, 0, DT_RET_VOID, setup_flush, 80, 0},
};
