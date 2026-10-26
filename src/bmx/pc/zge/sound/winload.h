/*
 * Sound file loader (C:\zaxis\Bmx\src\Pc\zge\sound\winLoad.c). A .zsd asset file (SFXCOM, MUSIC, SPEECH
 * under bin\audio\) holds voice, sample, phrase, track definition and key map tables. Loading reads the
 * file into a zmem block and turns the stored offsets into pointers; up to 16 files are kept at once and
 * addressed by a 16 bit file handle (the index into gWinloadFiles).
 */
#ifndef BMX_PC_ZGE_SOUND_WINLOAD_H
#define BMX_PC_ZGE_SOUND_WINLOAD_H

#include <stdint.h>

#include "compat/orig.h"

enum {
    kWinloadMaxFiles = 16,
    kWinloadNameBytes = 0x40,
    kWinloadNoSample = 0xffff,
    kWinloadAllFiles = 0xfffe, /* winloadUnload: every loaded file */
};

/* WinloadFile.flags */
enum {
    kWinloadLoaded = 0x1,
    kWinloadSamplesResident = 0x2, /* sample data was read with the file (otherwise it is streamed) */
    kWinloadOwnsData = 0x4,        /* the table block was allocated by the loader and is freed on unload */
};

/*
 * Voice (instrument): a sample with its playback parameters. Envelope times are indices into a 128 entry
 * table of milliseconds (0x7f: never), levels are 0..127. The loader fills in the file handle.
 */
typedef struct ZsdVoice {
    uint16_t sample; /* 0x00 sample number, 0xffff none */
    uint16_t pitch;  /* 0x02 playback rate factor, 0x1000 is 1.0 */
    uint8_t volume;  /* 0x04 */
    int8_t pan;      /* 0x05 */
    uint8_t attack;  /* 0x06 time to full level */
    uint8_t decay;   /* 0x07 time from full level to the sustain level */
    uint8_t fade;    /* 0x08 time from the sustain level to silence */
    uint8_t release; /* 0x09 time to silence after a stop */
    uint8_t sustain; /* 0x0a sustain level */
    uint8_t pad0b[7];
    uint16_t file; /* 0x12 */
} ZsdVoice;

/*
 * Sample description. The sample file number selects an entry of the sample file table; for resident files
 * the loader stores the data pointer at 0x08.
 */
typedef struct ZsdSample {
    uint16_t sampleFile; /* 0x00 index into the sample file table, 0xffff none */
    uint16_t format;     /* 0x02 bit 0: loops, bit 1: stereo, bit 2: 8 bit, bit 3: ping-pong loop */
    uint32_t rate;       /* 0x04 */
    const uint8_t *data; /* 0x08 resident data (set by the loader) */
    int32_t loopStart;   /* 0x0c first frame of the loop */
    uint8_t pad10[6];
    uint16_t file; /* 0x16 */
} ZsdSample;

/* Location of a sample file inside the .zsd (`offset` is relative to the file start). */
typedef struct ZsdSampleFileEntry {
    uint32_t offset;
    uint32_t size;
} ZsdSampleFileEntry;

/* Phrase: when bit 0 of the flags is set, `count` entries of {pointer, value} start at 0x10. */
typedef struct ZsdPhrase {
    uint8_t flags; /* 0x00 */
    uint8_t count; /* 0x01 */
    uint8_t pad02[0x0e];
    struct {
        uint32_t data;
        uint32_t value;
    } entries[]; /* 0x10 */
} ZsdPhrase;

typedef struct ZsdTrackDef {
    uint8_t pad00[8];
    uint32_t data; /* 0x08 */
    uint8_t pad0c[2];
    uint16_t file; /* 0x0e */
} ZsdTrackDef;

/* Key map: the loader stores the file handle at 0x02. */
typedef struct ZsdKeymap {
    uint8_t pad00[2];
    uint16_t file; /* 0x02 */
} ZsdKeymap;

/* Header of a loaded .zsd; the offsets at 0x14 .. 0x44 are pointers after loading. */
typedef struct ZsdHeader {
    char magic[4];        /* 0x00 "ZSND" */
    char platform[4];     /* 0x04 "WIN " */
    uint32_t fileBytes;   /* 0x08 size of the whole file */
    uint32_t tableBytes;  /* 0x0c bytes to read when the sample data is streamed (everything but samples) */
    uint32_t voiceCount;  /* 0x10 */
    ZsdVoice *voices;     /* 0x14 */
    uint32_t sampleCount; /* 0x18 */
    ZsdSample *samples;   /* 0x1c */
    uint32_t sampleFileCount;        /* 0x20 */
    ZsdSampleFileEntry *sampleFiles; /* 0x24 */
    uint32_t phraseCount;            /* 0x28 */
    ZsdPhrase **phrases;             /* 0x2c */
    uint32_t trackDefCount;          /* 0x30 */
    ZsdTrackDef *trackDefs;          /* 0x34 */
    uint8_t pad38[8];
    uint32_t keymapCount; /* 0x40 */
    ZsdKeymap **keymaps;  /* 0x44 */
} ZsdHeader;

_Static_assert(sizeof(ZsdVoice) == 0x14, "voice layout");
_Static_assert(sizeof(ZsdSample) == 0x18, "sample layout");
_Static_assert(sizeof(ZsdHeader) == 0x48, "header layout");

typedef struct WinloadFile {
    uint32_t flags;    /* 0x00 */
    ZsdHeader *header; /* 0x04 the file image */
    char name[kWinloadNameBytes];
} WinloadFile;

/* Where a sample file of a .zsd lives on disk; handed to the stream player. */
typedef struct WinloadSampleFile {
    const char *name; /* file name of the .zsd */
    uint32_t offset;
    uint32_t size;
} WinloadSampleFile;

/* Running totals over all loaded files. */
typedef struct WinloadCounts {
    int32_t voices;
    int32_t samples;
    int32_t residentSampleFiles;
    int32_t streamedSampleFiles;
    int32_t phrases;
    int32_t trackDefs;
    int32_t keymaps;
} WinloadCounts;

_Static_assert(sizeof(WinloadFile) == 0x48, "file slot layout");

typedef WinloadFile WinloadFileTable[kWinloadMaxFiles];

#define gWinloadFiles      ORIG_VAR(0x00600d98, WinloadFileTable, gWinloadFiles)
#define gWinloadCounts     ORIG_VAR(0x00601218, WinloadCounts, gWinloadCounts)
#define gWinloadSampleFile ORIG_VAR(0x00600d88, WinloadSampleFile, gWinloadSampleFile)

/*
 * Loads a .zsd. With `residentSamples` the whole file (sample data included) is read; otherwise only the
 * leading tableBytes bytes are, and the samples stay on disk for the stream player. Stores the file handle
 * in *handle (0xffff on failure). Returns 0, or 1 file missing, 2 short read, 3 out of memory.
 */
int winloadLoad(const char *path, int residentSamples, uint16_t *handle);
/* Claims a file slot for the image and relocates its tables. Returns 0. */
int winloadRegister(uint8_t *image, int residentSamples, uint16_t *handle);
/* Frees a file (kWinloadAllFiles: every loaded file). */
void winloadUnload(uint16_t file);
/* Returns 1 if the handle is a loaded file. */
int winloadIsLoaded(uint16_t file);
uint32_t winloadGetVoiceCount(uint16_t file);
uint32_t winloadGetSampleCount(uint16_t file);
uint32_t winloadGetSampleFileCount(uint16_t file);
uint32_t winloadGetPhraseCount(uint16_t file);
uint32_t winloadGetTrackDefCount(uint16_t file);
uint32_t winloadGetKeymapCount(uint16_t file);
/* Voice number `index` (a 0x14 byte descriptor) of the loaded sound file `file` (0-15). */
ZsdVoice *winloadGetVoice(uint16_t file, uint32_t index);
ZsdSample *winloadGetSample(uint16_t file, uint32_t index);
/* Fills the shared gWinloadSampleFile for sample file `index` and returns it. */
WinloadSampleFile *winloadGetSampleFile(uint16_t file, uint32_t index);
ZsdPhrase *winloadGetPhrase(uint16_t file, uint32_t index);
ZsdTrackDef *winloadGetTrackDef(uint16_t file, uint32_t index);
/* Key map number `index` of the loaded sound file `file`. */
const uint8_t *winloadGetKeymap(uint16_t file, uint32_t index);
/* Releases the file's data for real: stops its voices and frees the tables. */
void winloadUnloadFile(uint16_t file);

#endif
