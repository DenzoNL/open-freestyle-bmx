/*
 * Tests for the sound file loader (winload.c). The accessors run against random but well formed file
 * tables; winloadRegister relocates synthetic .zsd images; winloadLoad reads the real files under
 * bin\audio (DT_NEEDS_ASSETS). The unload path stubs the channel mixer's winchanStopVoice, which logs the
 * voices it is asked to silence. Assert paths end the run on both sides (zdebugFail, or the real CRT
 * assert), which the harness compares.
 *
 * Not covered: winloadLoad's out-of-memory and short-read results (the harness heap is large and the
 * file layer never reads short).
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "bmx/pc/zge/sound/winchan.h"
#include "bmx/pc/zge/sound/winload.h"
#include "compat/crt.h"

static uint32_t gLog[512];
static uint32_t gLogN;

static void logU(uint32_t v)
{
    if (gLogN < sizeof gLog / sizeof gLog[0]) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static void BMX_CDECL stub_stopVoice(const ZsdVoice *voice)
{
    logU(0x100);
    logU(dt_canon((uint32_t)(uintptr_t)voice));
}

static uint32_t garbage_high(uint32_t low)
{
    return low | (dt_u32() & 0xffff0000u);
}

static void *blob(uint32_t bytes)
{
    void *p = dt_alloc(bytes);
    dt_fill(p, bytes);
    return p;
}

static const char kMagic[4] = {'Z', 'S', 'N', 'D'};
static const char kPlatform[4] = {'W', 'I', 'N', ' '};

/* A header with random tables of 0..4 entries each. */
static ZsdHeader *make_header(int owned)
{
    ZsdHeader *h = owned ? zmemAlloc(sizeof *h) : dt_alloc(sizeof *h);
    uint32_t i;

    memset(h, 0, sizeof *h);
    memcpy(h->magic, kMagic, 4);
    memcpy(h->platform, kPlatform, 4);
    h->voiceCount = (uint32_t)dt_range(0, 4);
    h->voices = blob(0x14 * 5);
    h->sampleCount = (uint32_t)dt_range(0, 4);
    h->samples = blob(0x18 * 5);
    h->sampleFileCount = (uint32_t)dt_range(0, 4);
    h->sampleFiles = blob(8 * 5);
    h->phraseCount = (uint32_t)dt_range(0, 4);
    h->phrases = (ZsdPhrase **)dt_alloc(4 * 5);
    for (i = 0; i < 5; i++) {
        h->phrases[i] = blob(0x20);
    }
    h->trackDefCount = (uint32_t)dt_range(0, 4);
    h->trackDefs = blob(0x10 * 5);
    h->keymapCount = (uint32_t)dt_range(0, 4);
    h->keymaps = (ZsdKeymap **)dt_alloc(4 * 5);
    for (i = 0; i < 5; i++) {
        h->keymaps[i] = blob(0x20);
    }
    return h;
}

/* Random file table: each slot loaded or not, with random flags and name. */
static void make_files(void)
{
    int i;

    for (i = 0; i < kWinloadMaxFiles; i++) {
        WinloadFile *f = &gWinloadFiles[i];
        const int loaded = (int)(dt_u32() & 1);
        const int owned = loaded && (dt_u32() % 3 == 0);

        f->flags = (loaded ? kWinloadLoaded : 0) | ((dt_u32() & 1) ? kWinloadSamplesResident : 0) |
                   (owned ? kWinloadOwnsData : 0);
        f->header = loaded ? make_header(owned) : (ZsdHeader *)(uintptr_t)dt_u32();
        memset(f->name, 0, sizeof f->name);
        memcpy(f->name, "bin\\audio\\x.zsd", 16);
        f->name[10] = (char)('a' + i);
    }
    gWinloadCounts.voices = dt_range(20, 100);
    gWinloadCounts.samples = dt_range(20, 100);
    gWinloadCounts.residentSampleFiles = dt_range(20, 100);
    gWinloadCounts.streamedSampleFiles = dt_range(20, 100);
    gWinloadCounts.phrases = dt_range(20, 100);
    gWinloadCounts.trackDefs = dt_range(20, 100);
    gWinloadCounts.keymaps = dt_range(20, 100);
}

/* A handle of a loaded slot (there is always at least one). */
static uint16_t loaded_slot(void)
{
    int i = dt_range(0, kWinloadMaxFiles - 1);

    while ((gWinloadFiles[i].flags & kWinloadLoaded) == 0) {
        i = (i + 1) % kWinloadMaxFiles;
    }
    return (uint16_t)i;
}

static void begin(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    gLogN = 0;
    dt_stub(0x0046f1c0, NULL, (void *)stub_stopVoice);
    make_files();
    {
        WinloadFile *f = &gWinloadFiles[dt_range(0, kWinloadMaxFiles - 1)];

        if ((f->flags & kWinloadLoaded) == 0) {
            f->flags |= kWinloadLoaded;
            f->header = make_header(0);
        }
    }
    memset(&gWinloadSampleFile, 0, sizeof gWinloadSampleFile);
    gWinchanInitialized = 1;
}

static void setup_unload(DtCtx *c)
{
    begin(c);
    if (c->seed % 4 == 0) {
        DT_ARG_INT(c, 0, garbage_high(kWinloadAllFiles) & 0xffff0000u | kWinloadAllFiles);
    } else {
        DT_ARG_INT(c, 0, garbage_high(loaded_slot()));
    }
}

static void setup_unload_file(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, garbage_high(loaded_slot()));
}

static void setup_unload_bad(DtCtx *c)
{
    int i;

    begin(c);
    for (i = 0; i < kWinloadMaxFiles; i++) {
        if ((gWinloadFiles[i].flags & kWinloadLoaded) == 0) {
            break;
        }
    }
    if (i == kWinloadMaxFiles) {
        gWinloadFiles[3].flags = 0;
        i = 3;
    }
    gWinloadFiles[i].header = make_header(0);
    DT_ARG_INT(c, 0, garbage_high((uint32_t)i));
}

static void setup_is_loaded(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, garbage_high((uint32_t)dt_range(0, c->seed % 4 == 0 ? 0x30 : 15)));
}

/* Handle 0..15 of a loaded file, with some garbage in the upper half. */
static void setup_count(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, garbage_high(loaded_slot()));
}

/* Out of range handle or an unloaded file: the accessor asserts. */
static void setup_count_bad(DtCtx *c)
{
    int i;

    begin(c);
    if (c->seed % 2 == 0) {
        DT_ARG_INT(c, 0, garbage_high((uint32_t)dt_range(0x10, 0x2000)));
        return;
    }
    for (i = 0; i < kWinloadMaxFiles; i++) {
        if ((gWinloadFiles[i].flags & kWinloadLoaded) == 0) {
            break;
        }
    }
    if (i == kWinloadMaxFiles) {
        gWinloadFiles[5].flags = 0;
        i = 5;
    }
    DT_ARG_INT(c, 0, garbage_high((uint32_t)i));
}

static void setup_index(DtCtx *c)
{
    setup_count(c);
    DT_ARG_INT(c, 1, (uint32_t)dt_range(0, 3));
}

/* Index past the table (assert); the accessor keeps going on the original only until the assert ends it. */
static void setup_index_bad(DtCtx *c)
{
    setup_count(c);
    DT_ARG_INT(c, 1, c->seed % 2 ? (uint32_t)dt_range(5, 9) : dt_u32() | 0x80000000u);
}

static void setup_index_bad_handle(DtCtx *c)
{
    setup_count_bad(c);
    DT_ARG_INT(c, 1, (uint32_t)dt_range(0, 3));
}

/* ---- winloadRegister ----------------------------------------------------------------------------- */

typedef struct ImageSpec {
    uint32_t voices, samples, sampleFiles, phrases, trackDefs, keymaps;
} ImageSpec;

static void put32(uint8_t *p, uint32_t v)
{
    memcpy(p, &v, 4);
}

/*
 * A synthetic .zsd image. Offsets in the tables are relative to the image start. With `valid` unset the
 * sample file numbers may point past the sample file table.
 */
static uint8_t *make_image(ImageSpec *spec, int valid)
{
    static const uint32_t kBytes = 0x800;
    uint8_t *img = dt_alloc(kBytes);
    uint32_t off = 0x48, i, j;
    uint32_t voices, samples, sampleFiles, phrases, trackDefs, keymaps, phraseBlobs, keymapBlobs;

    dt_fill(img, kBytes);
    memcpy(img, kMagic, 4);
    memcpy(img + 4, kPlatform, 4);
    spec->voices = (uint32_t)dt_range(0, 5);
    spec->samples = (uint32_t)dt_range(0, 5);
    spec->sampleFiles = (uint32_t)dt_range(0, 4);
    spec->phrases = (uint32_t)dt_range(0, 3);
    spec->trackDefs = (uint32_t)dt_range(0, 4);
    spec->keymaps = (uint32_t)dt_range(0, 3);
    voices = off;
    off += 0x14 * spec->voices;
    samples = off;
    off += 0x18 * spec->samples;
    sampleFiles = off;
    off += 8 * spec->sampleFiles;
    phrases = off;
    off += 4 * spec->phrases;
    phraseBlobs = off;
    off += 0x40 * spec->phrases;
    trackDefs = off;
    off += 0x10 * spec->trackDefs;
    keymaps = off;
    off += 4 * spec->keymaps;
    keymapBlobs = off;
    put32(img + 0x10, spec->voices);
    put32(img + 0x14, voices);
    put32(img + 0x18, spec->samples);
    put32(img + 0x1c, samples);
    put32(img + 0x20, spec->sampleFiles);
    put32(img + 0x24, sampleFiles);
    put32(img + 0x28, spec->phrases);
    put32(img + 0x2c, phrases);
    put32(img + 0x30, spec->trackDefs);
    put32(img + 0x34, trackDefs);
    put32(img + 0x40, spec->keymaps);
    put32(img + 0x44, keymaps);
    for (i = 0; i < spec->samples; i++) {
        uint8_t *s = img + samples + 0x18 * i;
        const uint32_t n = spec->sampleFiles ? dt_u32() % (spec->sampleFiles + (valid ? 0 : 2)) : 0xffff;

        s[0] = (uint8_t)(spec->sampleFiles == 0 || dt_u32() % 4 == 0 ? 0xff : n);
        s[1] = (spec->sampleFiles == 0 || s[0] == 0xff) ? 0xff : (uint8_t)(n >> 8);
    }
    for (i = 0; i < spec->sampleFiles; i++) {
        put32(img + sampleFiles + 8 * i, 0x400 + 0x10 * i);
    }
    for (i = 0; i < spec->phrases; i++) {
        uint8_t *p = img + phraseBlobs + 0x40 * i;

        put32(img + phrases + 4 * i, phraseBlobs + 0x40 * i);
        p[0] = (uint8_t)dt_range(0, 3);
        p[1] = (uint8_t)dt_range(0, 4);
        for (j = 0; j < 4; j++) {
            put32(p + 0x10 + 8 * j, 0x500 + 4 * j);
        }
    }
    for (i = 0; i < spec->trackDefs; i++) {
        put32(img + trackDefs + 0x10 * i + 8, 0x600 + 4 * i);
    }
    for (i = 0; i < spec->keymaps; i++) {
        put32(img + keymaps + 4 * i, keymapBlobs + 0x10 * i);
    }
    return img;
}

static void begin_register(DtCtx *c, int valid, int freeSlots)
{
    ImageSpec spec;
    int i;

    begin(c);
    for (i = 0; i < kWinloadMaxFiles; i++) {
        if (freeSlots ? i >= 6 : 1) {
            gWinloadFiles[i].flags |= kWinloadLoaded;
        }
        if (i < 6 && freeSlots && dt_u32() % 2 == 0) {
            gWinloadFiles[i].flags &= ~(uint32_t)kWinloadLoaded;
        }
    }
    DT_ARG_PTR(c, 0, make_image(&spec, valid));
    DT_ARG_INT(c, 1, dt_u32() & 1 ? dt_u32() | 1u : 0);
    DT_ARG_PTR(c, 2, dt_alloc(4));
    if (freeSlots) {
        gWinloadFiles[dt_range(0, 5)].flags &= ~(uint32_t)kWinloadLoaded;
    }
}

static void setup_register(DtCtx *c)
{
    begin_register(c, 1, 1);
}

/* Sample file numbers past the table (resident files assert). */
static void setup_register_badsample(DtCtx *c)
{
    begin_register(c, 0, 1);
    DT_ARG_INT(c, 1, 1);
}

static void setup_register_full(DtCtx *c)
{
    begin_register(c, 1, 0);
}

static void setup_register_badmagic(DtCtx *c)
{
    begin_register(c, 1, 1);
    ((uint8_t *)(uintptr_t)c->args[0])[c->seed % 4] ^= 0x20;
}

static void setup_register_badplatform(DtCtx *c)
{
    begin_register(c, 1, 1);
    ((uint8_t *)(uintptr_t)c->args[0])[4 + c->seed % 4] ^= 0x20;
}

/* ---- winloadLoad ---------------------------------------------------------------------------------- */

static const char *const kFiles[] = {
    "bin\\audio\\sfxcom.zsd", "bin\\audio\\SFXLEV05.ZSD", "bin\\audio\\music.zsd",
    "bin\\audio\\speech.zsd", "bin\\audio\\sfxgame.zsd",  "bin\\audio\\sfxmen00.zsd",
};

static void setup_load(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, kFiles[c->seed % (sizeof kFiles / sizeof kFiles[0])]);
    /* music.zsd is 28 MB: only its table is read unless it is loaded resident, which the harness heap
     * can hold but which takes long; keep it to the table-only mode. */
    DT_ARG_INT(c, 1, c->seed % 6 == 2 ? 0 : (c->seed & 1 ? 1 : 0));
    DT_ARG_PTR(c, 2, dt_alloc(4));
}

static void setup_load_missing(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, c->seed & 1 ? "bin\\audio\\nothere.zsd" : "bin\\nothere\\sfxcom.zsd");
    DT_ARG_INT(c, 1, (int)(c->seed >> 1) & 1);
    DT_ARG_PTR(c, 2, dt_alloc(4));
}

/* The file table is full: the load fails after reading the file. */
static void setup_load_full(DtCtx *c)
{
    int i;

    setup_load(c);
    for (i = 0; i < kWinloadMaxFiles; i++) {
        gWinloadFiles[i].flags |= kWinloadLoaded;
    }
    DT_ARG_PTR(c, 0, "bin\\audio\\sfxmen00.zsd");
}

DT_CASES(winload_cases) = {
    {"winloadLoad", 0x0046fb20, winloadLoad, 3, DT_RET_INT | DT_NEEDS_ASSETS, setup_load, 36, 0},
    {"winloadLoad_missing", 0x0046fb20, winloadLoad, 3, DT_RET_INT, setup_load_missing, 8, 0},
    {"winloadLoad_full", 0x0046fb20, winloadLoad, 3, DT_RET_INT | DT_NEEDS_ASSETS, setup_load_full, 4, 0},
    {"winloadRegister", 0x0046fd00, winloadRegister, 3, DT_RET_INT, setup_register, 400, 0},
    {"winloadRegister_badsample", 0x0046fd00, winloadRegister, 3, DT_RET_INT, setup_register_badsample, 40,
     0},
    {"winloadRegister_full", 0x0046fd00, winloadRegister, 3, DT_RET_INT, setup_register_full, 4, 0},
    {"winloadRegister_badmagic", 0x0046fd00, winloadRegister, 3, DT_RET_INT, setup_register_badmagic, 8, 0},
    {"winloadRegister_badplatform", 0x0046fd00, winloadRegister, 3, DT_RET_INT, setup_register_badplatform, 8,
     0},
    {"winloadUnload", 0x004700b0, winloadUnload, 1, DT_RET_VOID, setup_unload, 200, 0},
    {"winloadIsLoaded", 0x00470100, winloadIsLoaded, 1, DT_RET_INT, setup_is_loaded, 100, 0},
    {"winloadGetVoiceCount", 0x00470140, winloadGetVoiceCount, 1, DT_RET_INT, setup_count, 40, 0},
    {"winloadGetVoiceCount_bad", 0x00470140, winloadGetVoiceCount, 1, DT_RET_INT, setup_count_bad, 8, 0},
    {"winloadGetSampleCount", 0x004701d0, winloadGetSampleCount, 1, DT_RET_INT, setup_count, 40, 0},
    {"winloadGetSampleCount_bad", 0x004701d0, winloadGetSampleCount, 1, DT_RET_INT, setup_count_bad, 8, 0},
    {"winloadGetSampleFileCount", 0x00470260, winloadGetSampleFileCount, 1, DT_RET_INT, setup_count, 40, 0},
    {"winloadGetSampleFileCount_bad", 0x00470260, winloadGetSampleFileCount, 1, DT_RET_INT, setup_count_bad,
     8, 0},
    {"winloadGetPhraseCount", 0x004702f0, winloadGetPhraseCount, 1, DT_RET_INT, setup_count, 40, 0},
    {"winloadGetPhraseCount_bad", 0x004702f0, winloadGetPhraseCount, 1, DT_RET_INT, setup_count_bad, 8, 0},
    {"winloadGetTrackDefCount", 0x00470380, winloadGetTrackDefCount, 1, DT_RET_INT, setup_count, 40, 0},
    {"winloadGetTrackDefCount_bad", 0x00470380, winloadGetTrackDefCount, 1, DT_RET_INT, setup_count_bad, 8,
     0},
    {"winloadGetKeymapCount", 0x00470410, winloadGetKeymapCount, 1, DT_RET_INT, setup_count, 40, 0},
    {"winloadGetKeymapCount_bad", 0x00470410, winloadGetKeymapCount, 1, DT_RET_INT, setup_count_bad, 8, 0},
    {"winloadGetVoice", 0x004704a0, winloadGetVoice, 2, DT_RET_INT, setup_index, 100, 0},
    {"winloadGetVoice_badindex", 0x004704a0, winloadGetVoice, 2, DT_RET_INT, setup_index_bad, 12, 0},
    {"winloadGetVoice_badfile", 0x004704a0, winloadGetVoice, 2, DT_RET_INT, setup_index_bad_handle, 12, 0},
    {"winloadGetSample", 0x00470560, winloadGetSample, 2, DT_RET_INT, setup_index, 100, 0},
    {"winloadGetSample_badindex", 0x00470560, winloadGetSample, 2, DT_RET_INT, setup_index_bad, 12, 0},
    {"winloadGetSample_badfile", 0x00470560, winloadGetSample, 2, DT_RET_INT, setup_index_bad_handle, 12, 0},
    {"winloadGetSampleFile", 0x00470620, winloadGetSampleFile, 2, DT_RET_INT, setup_index, 100, 0},
    {"winloadGetSampleFile_badindex", 0x00470620, winloadGetSampleFile, 2, DT_RET_INT, setup_index_bad, 12,
     0},
    {"winloadGetSampleFile_badfile", 0x00470620, winloadGetSampleFile, 2, DT_RET_INT, setup_index_bad_handle,
     12, 0},
    {"winloadGetPhrase", 0x00470700, winloadGetPhrase, 2, DT_RET_INT, setup_index, 100, 0},
    {"winloadGetPhrase_badindex", 0x00470700, winloadGetPhrase, 2, DT_RET_INT, setup_index_bad, 12, 0},
    {"winloadGetPhrase_badfile", 0x00470700, winloadGetPhrase, 2, DT_RET_INT, setup_index_bad_handle, 12, 0},
    {"winloadGetTrackDef", 0x004707c0, winloadGetTrackDef, 2, DT_RET_INT, setup_index, 100, 0},
    {"winloadGetTrackDef_badindex", 0x004707c0, winloadGetTrackDef, 2, DT_RET_INT, setup_index_bad, 12, 0},
    {"winloadGetTrackDef_badfile", 0x004707c0, winloadGetTrackDef, 2, DT_RET_INT, setup_index_bad_handle, 12,
     0},
    {"winloadGetKeymap", 0x00470880, winloadGetKeymap, 2, DT_RET_INT, setup_index, 100, 0},
    {"winloadGetKeymap_badindex", 0x00470880, winloadGetKeymap, 2, DT_RET_INT, setup_index_bad, 12, 0},
    {"winloadGetKeymap_badfile", 0x00470880, winloadGetKeymap, 2, DT_RET_INT, setup_index_bad_handle, 12, 0},
    {"winloadUnloadFile", 0x00470940, winloadUnloadFile, 1, DT_RET_VOID, setup_unload_file, 200, 0},
    {"winloadUnloadFile_notloaded", 0x00470940, winloadUnloadFile, 1, DT_RET_VOID, setup_unload_bad, 8, 0},
};
