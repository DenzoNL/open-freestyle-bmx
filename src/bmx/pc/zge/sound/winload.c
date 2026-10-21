/* Sound file loader (C:\zaxis\Bmx\src\Pc\zge\sound\winLoad.c). */
#include "bmx/pc/zge/sound/winload.h"

#include "bmx/pc/zge/sound/winapi.h"
#include "bmx/pc/zge/sound/winchan.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"

static const char winloadFile[] = "C:\\zaxis\\Bmx\\src\\Pc\\zge\\sound\\winLoad.c";

BMX_ADDR(0x0046fb20)
int winloadLoad(const char *path, int residentSamples, uint16_t *handle)
{
    ZsdHeader header;
    uint32_t bytes, read;
    WinHandle file;
    uint8_t *image;
    int rc;

    *handle = kWinloadNoSample;
    file = gWinCreateFile(path, kWinGenericRead, kWinFileShareRead, 0, kWinOpenExisting,
                          kWinFileAttributeNormal, 0);
    if (file == kWinInvalidHandle) {
        return 1;
    }
    if (residentSamples != 0) {
        bytes = gWinSetFilePointer(file, 0, 0, kWinFileEnd);
    } else {
        gWinReadFile(file, &header, sizeof header, &read, 0);
        bytes = header.tableBytes;
    }
    gWinSetFilePointer(file, 0, 0, kWinFileBegin);
    image = zmemAlloc(bytes);
    if (image == 0) {
        gWinCloseHandle(file);
        return 3;
    }
    gWinReadFile(file, image, bytes, &read, 0);
    if (read != bytes) {
        rc = 2;
    } else {
        rc = winloadRegister(image, residentSamples, handle);
        if (rc == 0) {
            WinloadFile *slot = &gWinloadFiles[*handle];

            slot->flags |= kWinloadOwnsData;
            if (residentSamples == 0) {
                /* Original quirk: the check measures the slot's (still empty) name, not `path`, and the
                 * copy below is not bounded by the name field. */
                if (strlen(slot->name) >= kWinloadNameBytes) {
                    zdebugFail("file name too long", winloadFile, 0x8f, 0);
                }
                memcpy(slot->name, path, strlen(path) + 1);
            }
            gWinCloseHandle(file);
            return 0;
        }
    }
    zmemFree(image);
    gWinCloseHandle(file);
    return rc;
}

/* Adds `base` to a stored file offset. */
static void *relocate(const uint8_t *base, uint32_t offset)
{
    return (void *)(base + offset);
}

BMX_ADDR(0x0046fd00)
int winloadRegister(uint8_t *image, int residentSamples, uint16_t *handle)
{
    ZsdHeader *h = (ZsdHeader *)image;
    WinloadFile *slot = 0;
    uint32_t i, j;
    int index;

    for (index = 0; index < kWinloadMaxFiles; index++) {
        if ((gWinloadFiles[index].flags & kWinloadLoaded) == 0) {
            slot = &gWinloadFiles[index];
            slot->name[0] = 0;
            slot->flags = (residentSamples != 0 ? kWinloadSamplesResident : 0) | kWinloadLoaded;
            *handle = (uint16_t)index;
            break;
        }
    }
    if (slot == 0) {
        zdebugFail("too many audio asset files", winloadFile, 0xcb, 0);
    }
    slot->header = h;
    if (strncmp((const char *)image, "ZSND", 4) != 0) {
        zdebugFail("invalid audio asset file header", winloadFile, 0xce, 0);
    }
    if (strncmp((const char *)image + 4, "WIN ", 4) != 0) {
        zdebugFail("invalid audio asset file platform", winloadFile, 0xcf, 0);
    }
    h->voices = relocate(image, (uint32_t)(uintptr_t)h->voices);
    h->samples = relocate(image, (uint32_t)(uintptr_t)h->samples);
    h->trackDefs = relocate(image, (uint32_t)(uintptr_t)h->trackDefs);
    h->sampleFiles = relocate(image, (uint32_t)(uintptr_t)h->sampleFiles);
    h->phrases = (ZsdPhrase **)relocate(image, (uint32_t)(uintptr_t)h->phrases);
    h->keymaps = (ZsdKeymap **)relocate(image, (uint32_t)(uintptr_t)h->keymaps);
    for (i = 0; i < h->voiceCount; i++) {
        h->voices[i].file = *handle;
    }
    gWinloadCounts.voices += (int32_t)h->voiceCount;
    for (i = 0; i < h->sampleCount; i++) {
        h->samples[i].file = *handle;
    }
    gWinloadCounts.samples += (int32_t)h->sampleCount;
    for (i = 0; i < h->phraseCount; i++) {
        ZsdPhrase *phrase;

        h->phrases[i] = relocate(image, (uint32_t)(uintptr_t)h->phrases[i]);
        phrase = h->phrases[i];
        if ((phrase->flags & 1) == 1) {
            for (j = 0; j < phrase->count; j++) {
                phrase->entries[j].data += (uint32_t)(uintptr_t)image;
            }
        }
    }
    gWinloadCounts.phrases += (int32_t)h->phraseCount;
    for (i = 0; i < h->trackDefCount; i++) {
        h->trackDefs[i].data += (uint32_t)(uintptr_t)image;
        h->trackDefs[i].file = *handle;
    }
    gWinloadCounts.trackDefs += (int32_t)h->trackDefCount;
    for (i = 0; i < h->keymapCount; i++) {
        h->keymaps[i] = relocate(image, (uint32_t)(uintptr_t)h->keymaps[i]);
        h->keymaps[i]->file = *handle;
    }
    gWinloadCounts.keymaps += (int32_t)h->keymapCount;
    if (residentSamples == 0) {
        gWinloadCounts.streamedSampleFiles += (int32_t)h->sampleFileCount;
        return 0;
    }
    for (i = 0; i < h->sampleCount; i++) {
        ZsdSample *sample = &h->samples[i];

        if (sample->sampleFile != kWinloadNoSample) {
            if (h->sampleFileCount <= sample->sampleFile) {
                zdebugFail("invalid sample file number", winloadFile, 0xfe, 0);
            }
            sample->data = relocate(image, h->sampleFiles[sample->sampleFile].offset);
        }
    }
    gWinloadCounts.residentSampleFiles += (int32_t)h->sampleFileCount;
    return 0;
}

BMX_ADDR(0x004700b0)
void winloadUnload(uint16_t file)
{
    int i;

    if (file != kWinloadAllFiles) {
        winloadUnloadFile(file);
        return;
    }
    for (i = 0; i < kWinloadMaxFiles; i++) {
        if ((gWinloadFiles[i].flags & kWinloadLoaded) != 0) {
            winloadUnloadFile((uint16_t)i);
        }
    }
}

BMX_ADDR(0x00470100)
int winloadIsLoaded(uint16_t file)
{
    if (file > 0xf || (gWinloadFiles[file].flags & kWinloadLoaded) == 0) {
        return 0;
    }
    return 1;
}

/* The original asserts that the handle is in range and loaded (the lines differ per accessor). */
static const ZsdHeader *requireFile(uint16_t file, int handleLine)
{
    if (file > 0xf) {
        zdebugFail("invalid file handle", winloadFile, handleLine, 0);
    }
    if ((gWinloadFiles[file].flags & kWinloadLoaded) == 0) {
        zdebugFail("file not loaded", winloadFile, handleLine + 2, 0);
    }
    return gWinloadFiles[file].header;
}

BMX_ADDR(0x00470140)
uint32_t winloadGetVoiceCount(uint16_t file)
{
    return requireFile(file, 0x15d)->voiceCount;
}

BMX_ADDR(0x004701d0)
uint32_t winloadGetSampleCount(uint16_t file)
{
    return requireFile(file, 0x16f)->sampleCount;
}

BMX_ADDR(0x00470260)
uint32_t winloadGetSampleFileCount(uint16_t file)
{
    return requireFile(file, 0x181)->sampleFileCount;
}

BMX_ADDR(0x004702f0)
uint32_t winloadGetPhraseCount(uint16_t file)
{
    return requireFile(file, 0x193)->phraseCount;
}

BMX_ADDR(0x00470380)
uint32_t winloadGetTrackDefCount(uint16_t file)
{
    return requireFile(file, 0x1a5)->trackDefCount;
}

BMX_ADDR(0x00470410)
uint32_t winloadGetKeymapCount(uint16_t file)
{
    return requireFile(file, 0x1b7)->keymapCount;
}

BMX_ADDR(0x004704a0)
ZsdVoice *winloadGetVoice(uint16_t file, uint32_t index)
{
    const ZsdHeader *h = requireFile(file, 0x1d6);

    if (h->voiceCount <= index) {
        zdebugFail("invalid voice number", winloadFile, 0x1da, 0);
    }
    return h->voices + index;
}

BMX_ADDR(0x00470560)
ZsdSample *winloadGetSample(uint16_t file, uint32_t index)
{
    const ZsdHeader *h = requireFile(file, 0x1ec);

    if (h->sampleCount <= index) {
        zdebugFail("sample number out of range", winloadFile, 0x1f0, 0);
    }
    return h->samples + index;
}

BMX_ADDR(0x00470620)
WinloadSampleFile *winloadGetSampleFile(uint16_t file, uint32_t index)
{
    const ZsdHeader *h = requireFile(file, 0x203);
    const ZsdSampleFileEntry *entry;

    if (h->sampleFileCount <= index) {
        zdebugFail("sample file number out of range", winloadFile, 0x207, 0);
    }
    entry = h->sampleFiles + index;
    gWinloadSampleFile.name = gWinloadFiles[file].name;
    gWinloadSampleFile.offset = entry->offset;
    gWinloadSampleFile.size = entry->size;
    return &gWinloadSampleFile;
}

BMX_ADDR(0x00470700)
ZsdPhrase *winloadGetPhrase(uint16_t file, uint32_t index)
{
    const ZsdHeader *h = requireFile(file, 0x21b);

    if (h->phraseCount <= index) {
        zdebugFail("phrase number out of range", winloadFile, 0x21f, 0);
    }
    return h->phrases[index];
}

BMX_ADDR(0x004707c0)
ZsdTrackDef *winloadGetTrackDef(uint16_t file, uint32_t index)
{
    const ZsdHeader *h = requireFile(file, 0x230);

    if (h->trackDefCount <= index) {
        zdebugFail("track def number out of range", winloadFile, 0x234, 0);
    }
    return h->trackDefs + index;
}

BMX_ADDR(0x00470880)
const uint8_t *winloadGetKeymap(uint16_t file, uint32_t index)
{
    const ZsdHeader *h = requireFile(file, 0x245);

    if (h->keymapCount <= index) {
        zdebugFail("keymap number out of range", winloadFile, 0x249, 0);
    }
    return (const uint8_t *)h->keymaps[index];
}

BMX_ADDR(0x00470940)
void winloadUnloadFile(uint16_t file)
{
    WinloadFile *slot = &gWinloadFiles[file];
    ZsdHeader *h = slot->header;
    int32_t i;

    if ((slot->flags & kWinloadLoaded) == 0) {
        zdebugFail("file not loaded", winloadFile, 0x25c, 0);
    }
    for (i = (int32_t)h->voiceCount - 1; i >= 0; i--) {
        winchanStopVoice(&h->voices[i]);
    }
    gWinloadCounts.voices -= (int32_t)h->voiceCount;
    gWinloadCounts.samples -= (int32_t)h->sampleCount;
    if ((slot->flags & kWinloadSamplesResident) == 0) {
        gWinloadCounts.streamedSampleFiles -= (int32_t)h->sampleFileCount;
    } else {
        gWinloadCounts.residentSampleFiles -= (int32_t)h->sampleFileCount;
    }
    gWinloadCounts.phrases -= (int32_t)h->phraseCount;
    gWinloadCounts.trackDefs -= (int32_t)h->trackDefCount;
    gWinloadCounts.keymaps -= (int32_t)h->keymapCount;
    if ((slot->flags & kWinloadOwnsData) != 0) {
        zmemFree(slot->header);
    }
    slot->flags = 0;
}
