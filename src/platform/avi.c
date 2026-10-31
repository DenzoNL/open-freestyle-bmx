/*
 * AVIFIL32 stand-ins (docs/PLATFORM.md P12, slice S8): our own RIFF/AVI reader and, through cinepak.c, the
 * frames AVIStreamGetFrame hands out. The subset the game calls (zge/zmovie.c): AVIFileInit / Exit / OpenA /
 * GetStream / Release, AVIStreamInfoA / ReadFormat / Read / Start / Length / TimeToSample / SampleToTime /
 * GetFrameOpen / GetFrame / GetFrameClose / AddRef / Release, AVISaveOptionsFree. Results (values, rounding,
 * error codes, what is written into the caller's buffers) are avifil32.dll's, measured on the game's seven
 * files by tests/platform/test_movie.c.
 *
 * The handles are our records (avifil32's are COM objects; the game only passes them back). A stream keeps
 * its file alive; the file is read through SDL_IOStream, the index (idx1) once at open.
 */
#include <SDL3/SDL.h>
#include <string.h>

#include "platform/avi.h"
#include "platform/cinepak.h"
#include "platform/standins.h"

enum {
    kMaxStreams = 8,
    kAviifList = 0x01,
    kAviifKeyframe = 0x10,
    kFccVids = 0x73646976, /* 'vids' */
    kFccAuds = 0x73647561, /* 'auds' */
    kFccCvid = 0x64697663, /* 'cvid' */
    kInfoFixed = 0x4c,     /* AVISTREAMINFOA up to szName */
    kDibHeader = 40,
};

/* AVISTREAMINFOA (0x8c bytes). */
typedef struct AviStreamInfo {
    uint32_t type, handler, flags, caps;
    uint16_t priority, language;
    uint32_t scale, rate, start, length, initialFrames, suggestedBufferSize, quality, sampleSize;
    int32_t frame[4];
    uint32_t editCount, formatChangeCount;
    char name[64];
} AviStreamInfo;

_Static_assert(sizeof(AviStreamInfo) == 0x8c, "AVISTREAMINFOA");
_Static_assert(offsetof(AviStreamInfo, name) == kInfoFixed, "AVISTREAMINFOA");

typedef struct AviChunk {
    uint64_t offset;     /* of the data in the file */
    uint32_t size;       /* bytes */
    uint32_t flags;      /* idx1 flags */
    int64_t firstSample; /* fixed-size samples: the stream's sample at the chunk's first byte */
} AviChunk;

struct BmxAviFile;

typedef struct BmxAviStream {
    uint32_t magic;
    int32_t refs;
    struct BmxAviFile *file;
    AviStreamInfo info;
    uint8_t *format;
    uint32_t formatSize;
    AviChunk *chunks;
    uint32_t chunkCount;
    uint32_t chunkCapacity;
    uint32_t reads;     /* AVIStreamRead calls that returned data, */
    uint64_t readBytes; /* their bytes */
    uint64_t readHash;  /* and the FNV-1a 64 of those bytes in order (logged at the end) */
} BmxAviStream;

typedef struct BmxAviFile {
    uint32_t magic;
    int32_t refs;
    SDL_IOStream *io;
    int32_t streamCount;
    BmxAviStream *streams[kMaxStreams];
} BmxAviFile;

typedef struct BmxGetFrame {
    uint32_t magic;
    BmxAviStream *stream;
    int32_t current; /* frame in the buffer, -1 none */
    uint8_t *dib;    /* BITMAPINFOHEADER + bits */
    uint8_t *chunk;  /* compressed data of one frame */
    uint32_t chunkCapacity;
    int32_t stride;
    CinepakDecoder decoder;
} BmxGetFrame;

static const uint32_t kFileMagic = 0x46495641, kStreamMagic = 0x53495641, kFrameMagic = 0x46474641;

/* The open frame objects (for bmxAviFrameOf). */
enum { kMaxFrameObjects = 16 };
static BmxGetFrame *gFrames[kMaxFrameObjects];

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static int readAt(SDL_IOStream *io, uint64_t offset, void *buffer, size_t size)
{
    return SDL_SeekIO(io, (Sint64)offset, SDL_IO_SEEK_SET) == (Sint64)offset &&
           SDL_ReadIO(io, buffer, size) == size;
}

/* ---- opening: the header list and the index ---- */

static int addChunk(BmxAviStream *s, uint64_t offset, uint32_t size, uint32_t flags)
{
    AviChunk *c;

    if (s->chunkCount == s->chunkCapacity) {
        const uint32_t capacity = s->chunkCapacity != 0 ? s->chunkCapacity * 2 : 256;
        AviChunk *const grown = SDL_realloc(s->chunks, capacity * sizeof *grown);

        if (grown == NULL) {
            return 0;
        }
        s->chunks = grown;
        s->chunkCapacity = capacity;
    }
    c = &s->chunks[s->chunkCount++];
    c->offset = offset;
    c->size = size;
    c->flags = flags;
    c->firstSample = 0;
    return 1;
}

/* strh -> the stream's info (the frame rectangle is four int16 in the 56-byte header, four int32 in 64). */
static void readStreamHeader(BmxAviStream *s, const uint8_t *h, uint32_t size)
{
    AviStreamInfo *const info = &s->info;

    if (size < 48) {
        return;
    }
    info->type = le32(h);
    info->handler = le32(h + 4);
    info->flags = le32(h + 8);
    info->priority = le16(h + 12);
    info->language = le16(h + 14);
    info->initialFrames = le32(h + 16);
    info->scale = le32(h + 20);
    info->rate = le32(h + 24);
    info->start = le32(h + 28);
    info->length = le32(h + 32);
    info->suggestedBufferSize = le32(h + 36);
    info->quality = le32(h + 40);
    info->sampleSize = le32(h + 44);
    if (size >= 64) {
        for (int i = 0; i < 4; i++) {
            info->frame[i] = (int32_t)le32(h + 48 + 4 * i);
        }
    } else if (size >= 56) {
        for (int i = 0; i < 4; i++) {
            info->frame[i] = (int16_t)le16(h + 48 + 2 * i);
        }
    }
}

/* Walks the chunks in [offset, end): the hdrl's strl lists, the movi list's position, idx1. */
typedef struct AviScan {
    BmxAviFile *file;
    uint64_t movi; /* position of the 'movi' fourcc */
    uint64_t index;
    uint32_t indexSize;
} AviScan;

static int scanList(AviScan *scan, uint64_t offset, uint64_t end, BmxAviStream *current)
{
    uint8_t h[12];

    while (offset + 8 <= end) {
        uint32_t id, size;

        if (!readAt(scan->file->io, offset, h, 8)) {
            return 0;
        }
        id = le32(h);
        size = le32(h + 4);
        if (id == 0x5453494c /* LIST */) {
            uint32_t type;

            if (size < 4 || !readAt(scan->file->io, offset + 8, h + 8, 4)) {
                return 0;
            }
            type = le32(h + 8);
            if (type == 0x6c726468 /* hdrl */) {
                if (!scanList(scan, offset + 12, offset + 8 + size, NULL)) {
                    return 0;
                }
            } else if (type == 0x6c727473 /* strl */) {
                BmxAviStream *s;

                if (scan->file->streamCount == kMaxStreams) {
                    return 0;
                }
                s = SDL_calloc(1, sizeof *s);
                if (s == NULL) {
                    return 0;
                }
                s->magic = kStreamMagic;
                s->file = scan->file;
                s->readHash = 0xcbf29ce484222325ull;
                scan->file->streams[scan->file->streamCount++] = s;
                if (!scanList(scan, offset + 12, offset + 8 + size, s)) {
                    return 0;
                }
            } else if (type == 0x69766f6d /* movi */) {
                scan->movi = offset + 8;
            }
        } else if (id == 0x68727473 /* strh */ && current != NULL) {
            uint8_t data[64] = {0};

            if (!readAt(scan->file->io, offset + 8, data, size < 64 ? size : 64)) {
                return 0;
            }
            readStreamHeader(current, data, size);
        } else if (id == 0x66727473 /* strf */ && current != NULL && current->format == NULL) {
            current->format = SDL_malloc(size != 0 ? size : 1);
            if (current->format == NULL || !readAt(scan->file->io, offset + 8, current->format, size)) {
                return 0;
            }
            current->formatSize = size;
        } else if (id == 0x31786469 /* idx1 */) {
            scan->index = offset + 8;
            scan->indexSize = size;
        }
        offset += 8 + (uint64_t)size + (size & 1);
    }
    return 1;
}

/* idx1: per entry the chunk id ("00dc", "01wb": the stream number in decimal), flags, offset, size. */
static int readIndex(AviScan *scan)
{
    BmxAviFile *const file = scan->file;
    uint8_t *index;
    uint64_t base = 0;
    int ok = 1;

    if (scan->indexSize < 16 || scan->movi == 0) {
        return 0;
    }
    index = SDL_malloc(scan->indexSize);
    if (index == NULL || !readAt(file->io, scan->index, index, scan->indexSize)) {
        SDL_free(index);
        return 0;
    }
    /* Offsets are relative to the 'movi' fourcc, or (some writers) absolute. */
    if ((uint64_t)le32(index + 8) >= scan->movi) {
        base = 0;
    } else {
        base = scan->movi;
    }
    for (uint32_t i = 0; ok && i + 16 <= scan->indexSize; i += 16) {
        const uint8_t *const e = index + i;
        const uint32_t flags = le32(e + 4);
        const int n = (e[0] - '0') * 10 + (e[1] - '0');

        if ((flags & kAviifList) != 0 || e[0] < '0' || e[0] > '9' || e[1] < '0' || e[1] > '9' ||
            n >= file->streamCount) {
            continue;
        }
        ok = addChunk(file->streams[n], base + le32(e + 8) + 8, le32(e + 12), flags);
    }
    SDL_free(index);
    return ok;
}

/* The derived parts of the info avifil32 reports: name, buffer size; and the sample positions of chunks. */
static void finishStream(BmxAviStream *s, const char *fileName, int number)
{
    AviStreamInfo *const info = &s->info;
    uint32_t largest = 0;
    int64_t sample = 0;

    for (uint32_t i = 0; i < s->chunkCount; i++) {
        AviChunk *const c = &s->chunks[i];

        if (c->size > largest) {
            largest = c->size;
        }
        c->firstSample = sample;
        sample += info->sampleSize != 0 ? c->size / info->sampleSize : 1;
    }
    info->suggestedBufferSize = largest;
    SDL_snprintf(info->name, sizeof info->name, "%s %s #%d", fileName,
                 info->type == kFccVids   ? "Video"
                 : info->type == kFccAuds ? "Audio"
                                          : "Stream",
                 number);
}

static void destroyStream(BmxAviStream *s)
{
    if (s->reads != 0) {
        bmxLog("avi: \"%s\": %u reads, %llu bytes (FNV-1a 64 %016llx)\n", s->info.name, (unsigned)s->reads,
               (unsigned long long)s->readBytes, (unsigned long long)s->readHash);
    }
    s->magic = 0;
    SDL_free(s->format);
    SDL_free(s->chunks);
    SDL_free(s);
}

static void destroyFile(BmxAviFile *f)
{
    for (int32_t i = 0; i < f->streamCount; i++) {
        if (f->streams[i] != NULL && f->streams[i]->refs == 0) {
            destroyStream(f->streams[i]);
        }
    }
    if (f->io != NULL) {
        SDL_CloseIO(f->io);
    }
    f->magic = 0;
    SDL_free(f);
}

static uint32_t releaseFile(BmxAviFile *f)
{
    const int32_t refs = --f->refs;

    if (refs == 0) {
        destroyFile(f);
    }
    return (uint32_t)refs;
}

static BmxAviFile *asFile(void *p)
{
    BmxAviFile *const f = p;

    return f != NULL && f->magic == kFileMagic ? f : NULL;
}

static BmxAviStream *asStream(void *p)
{
    BmxAviStream *const s = p;

    return s != NULL && s->magic == kStreamMagic ? s : NULL;
}

void BMX_WINAPI bmxStandAVIFileInit(void) {}

void BMX_WINAPI bmxStandAVIFileExit(void) {}

int32_t BMX_WINAPI bmxStandAVIFileOpenA(void **out, const char *path, uint32_t mode, void *handler)
{
    BmxAviFile *f;
    AviScan scan;
    const char *base = path;
    int counts[3] = {0, 0, 0};
    uint8_t riff[12];

    (void)handler;
    if (out == NULL || path == NULL) {
        return kAvierrBadParam;
    }
    *out = NULL;
    if ((mode & 0x3) != 0) { /* OF_WRITE / OF_READWRITE */
        bmxLog("avi: AVIFileOpenA(\"%s\", mode 0x%x): only reading is supported\n", path, mode);
        return kAvierrReadOnly;
    }
    f = SDL_calloc(1, sizeof *f);
    if (f == NULL) {
        return kAvierrMemory;
    }
    f->magic = kFileMagic;
    f->refs = 1;
    f->io = SDL_IOFromFile(bmxPathResolve(path), "rb");
    if (f->io == NULL) {
        destroyFile(f);
        return kAvierrFileOpen;
    }
    memset(&scan, 0, sizeof scan);
    scan.file = f;
    if (!readAt(f->io, 0, riff, 12) || le32(riff) != 0x46464952 /* RIFF */ ||
        le32(riff + 8) != 0x20495641 /* AVI  */ || !scanList(&scan, 12, 8 + (uint64_t)le32(riff + 4), NULL) ||
        !readIndex(&scan) || f->streamCount == 0) {
        bmxLog("avi: \"%s\" is not an AVI file we can read\n", path);
        destroyFile(f);
        return kAvierrBadFormat;
    }
    for (const char *p = path; *p != 0; p++) {
        if (*p == '\\' || *p == '/' || *p == ':') {
            base = p + 1;
        }
    }
    for (int32_t i = 0; i < f->streamCount; i++) {
        BmxAviStream *const s = f->streams[i];
        const int kind = s->info.type == kFccVids ? 0 : s->info.type == kFccAuds ? 1 : 2;

        finishStream(s, base, ++counts[kind]);
    }
    *out = f;
    return 0;
}

uint32_t BMX_WINAPI bmxStandAVIFileRelease(void *file)
{
    BmxAviFile *const f = asFile(file);

    return f != NULL ? releaseFile(f) : 0;
}

/* type 0: the index-th stream; else the index-th stream of that type. The stream holds a reference on the
 * file. */
int32_t BMX_WINAPI bmxStandAVIFileGetStream(void *file, void **out, uint32_t type, int32_t index)
{
    BmxAviFile *const f = asFile(file);

    if (out == NULL) {
        return kAvierrBadParam;
    }
    *out = NULL;
    if (f == NULL) {
        return kAvierrBadHandle;
    }
    for (int32_t i = 0; i < f->streamCount; i++) {
        BmxAviStream *const s = f->streams[i];

        if (type != 0 && s->info.type != type) {
            continue;
        }
        if (index-- == 0) {
            s->refs++;
            f->refs++;
            *out = s;
            return 0;
        }
    }
    return kAvierrNoData;
}

uint32_t BMX_WINAPI bmxStandAVIStreamAddRef(void *stream)
{
    BmxAviStream *const s = asStream(stream);

    if (s == NULL) {
        return 0;
    }
    s->file->refs++;
    return (uint32_t)++s->refs;
}

uint32_t BMX_WINAPI bmxStandAVIStreamRelease(void *stream)
{
    BmxAviStream *const s = asStream(stream);
    int32_t refs;

    if (s == NULL) {
        return 0;
    }
    refs = --s->refs;
    releaseFile(s->file); /* destroys the stream too when it was the last reference */
    return (uint32_t)refs;
}

/* Copies the fixed part and the name up to its terminator (avifil32 leaves the rest of szName alone). */
int32_t BMX_WINAPI bmxStandAVIStreamInfoA(void *stream, void *out, int32_t size)
{
    BmxAviStream *const s = asStream(stream);
    size_t n;

    if (s == NULL || out == NULL || size < 0) {
        return kAvierrBadParam;
    }
    n = SDL_min((size_t)size, (size_t)kInfoFixed + SDL_strlen(s->info.name) + 1);
    memcpy(out, &s->info, n);
    return 0;
}

int32_t BMX_WINAPI bmxStandAVIStreamStart(void *stream)
{
    BmxAviStream *const s = asStream(stream);

    return s != NULL ? (int32_t)s->info.start : -1;
}

int32_t BMX_WINAPI bmxStandAVIStreamLength(void *stream)
{
    BmxAviStream *const s = asStream(stream);

    return s != NULL ? (int32_t)s->info.length : -1;
}

/* *size in: the buffer's size; out: the format's. A NULL buffer only asks for the size. */
int32_t BMX_WINAPI bmxStandAVIStreamReadFormat(void *stream, int32_t pos, void *format, int32_t *size)
{
    BmxAviStream *const s = asStream(stream);

    int32_t result = 0;

    (void)pos; /* one format per stream (no format changes in these files) */
    if (s == NULL || size == NULL) {
        return kAvierrBadParam;
    }
    if (format != NULL && *size > 0) {
        memcpy(format, s->format, SDL_min((uint32_t)*size, s->formatSize));
        if ((uint32_t)*size < s->formatSize) {
            result = kAvierrBufferTooSmall; /* the part that fits is copied all the same */
        }
    }
    *size = (int32_t)s->formatSize;
    return result;
}

/* a * b / c rounded down (up = 0) or up (up = 1), 64-bit intermediate; -1 when it does not fit 31 bits. */
static int32_t mulDiv(uint32_t a, uint32_t b, uint32_t c, int up)
{
    uint64_t q;

    if (c == 0) {
        return -1;
    }
    q = ((uint64_t)a * b + (up ? c - 1 : 0)) / c;
    return q > 0x7fffffffu ? -1 : (int32_t)q;
}

/* Milliseconds -> sample: -1 before 0; rounded down for rates below 1000 per second, else up; clamped to
 * [start, start + length] (an overflow becomes -1 and so the start). */
int32_t BMX_WINAPI bmxStandAVIStreamTimeToSample(void *stream, int32_t time)
{
    BmxAviStream *const s = asStream(stream);
    const AviStreamInfo *info;
    int32_t sample;

    if (time < 0) {
        return -1;
    }
    if (s == NULL) {
        return -1;
    }
    info = &s->info;
    if (info->scale == 0) {
        return time;
    }
    sample = mulDiv((uint32_t)time, info->rate, info->scale * 1000u, info->rate / info->scale >= 1000);
    if (sample < (int32_t)info->start) {
        sample = (int32_t)info->start;
    }
    if (sample > (int32_t)(info->start + info->length)) {
        sample = (int32_t)(info->start + info->length);
    }
    return sample;
}

/* Sample -> milliseconds: the sample clamped to [start, start + length], then rounded like TimeToSample. */
int32_t BMX_WINAPI bmxStandAVIStreamSampleToTime(void *stream, int32_t sample)
{
    BmxAviStream *const s = asStream(stream);
    const AviStreamInfo *info;

    if (s == NULL) {
        return -1;
    }
    info = &s->info;
    if (info->rate == 0 || info->scale == 0) {
        return sample;
    }
    if (sample < (int32_t)info->start) {
        sample = (int32_t)info->start;
    }
    if (sample > (int32_t)(info->start + info->length)) {
        sample = (int32_t)(info->start + info->length);
    }
    return mulDiv((uint32_t)sample, info->scale * 1000u, info->rate, info->rate / info->scale >= 1000);
}

/* The chunk holding sample `sample` (fixed-size samples) or -1. */
static int32_t findChunk(const BmxAviStream *s, int64_t sample)
{
    uint32_t lo = 0, hi = s->chunkCount;

    while (lo < hi) {
        const uint32_t mid = lo + (hi - lo) / 2;
        const AviChunk *const c = &s->chunks[mid];
        const int64_t count = c->size / s->info.sampleSize;

        if (sample < c->firstSample) {
            hi = mid;
        } else if (sample >= c->firstSample + count) {
            lo = mid + 1;
        } else {
            return (int32_t)mid;
        }
    }
    return -1;
}

static void setCounts(int32_t *bytesOut, int32_t *samplesOut, int32_t bytes, int32_t samples)
{
    if (bytesOut != NULL) {
        *bytesOut = bytes;
    }
    if (samplesOut != NULL) {
        *samplesOut = samples;
    }
}

/* One frame per call whatever `samples` says; a NULL buffer only asks for its size. */
static int32_t readFrame(const BmxAviStream *s, int32_t start, void *buffer, int32_t bufferBytes,
                         int32_t *bytesOut, int32_t *samplesOut)
{
    const AviChunk *const c = &s->chunks[start - (int32_t)s->info.start];

    if (buffer != NULL) {
        if (bufferBytes < 0) {
            return kAvierrBufferTooSmall;
        }
        if (bufferBytes == 0) {
            return kAvierrError;
        }
        if ((uint32_t)bufferBytes < c->size) {
            setCounts(bytesOut, samplesOut, (int32_t)c->size, 0);
            return kAvierrBufferTooSmall;
        }
        if (c->size != 0 && !readAt(s->file->io, c->offset, buffer, c->size)) {
            return kAvierrFileRead;
        }
    }
    setCounts(bytesOut, samplesOut, (int32_t)c->size, 1);
    return 0;
}

/*
 * avifil32's rules, measured (tests/platform/test_movie.c): a start before the stream fails with BADPARAM and
 * leaves the counts alone; else they are zeroed first. Past the end: ERROR for a 0-byte buffer,
 * BUFFERTOOSMALL for less than a sample, else FILEREAD. Fixed-size samples (audio): samples -1 = the rest of
 * the chunk (the "convenient" amount), 0 = as many as fit, another negative count = no limit; with a buffer
 * the count is clipped to the stream's end and to the buffer (a negative size is BUFFERTOOSMALL, less than
 * one sample reads nothing); without one only to bufferBytes / size, and not at all when bufferBytes is 0 (a
 * negative size gives nothing). Reads go across chunks.
 */
int32_t BMX_WINAPI bmxStandAVIStreamRead(void *stream, int32_t start, int32_t samples, void *buffer,
                                         int32_t bufferBytes, int32_t *bytesOut, int32_t *samplesOut)
{
    BmxAviStream *const s = asStream(stream);
    const AviStreamInfo *info;
    int32_t end, bytes, size;
    int64_t count;
    uint8_t *out = buffer;

    if (s == NULL) {
        return kAvierrBadHandle;
    }
    info = &s->info;
    if (start < (int32_t)info->start) {
        return kAvierrBadParam;
    }
    setCounts(bytesOut, samplesOut, 0, 0);
    size = (int32_t)info->sampleSize;
    end = (int32_t)(info->start + info->length);
    if (start >= end || (size == 0 && (uint32_t)(start - (int32_t)info->start) >= s->chunkCount)) {
        return bufferBytes == 0                        ? kAvierrError
               : bufferBytes < size || bufferBytes < 0 ? kAvierrBufferTooSmall
                                                       : kAvierrFileRead;
    }
    if (size == 0) {
        return readFrame(s, start, buffer, bufferBytes, bytesOut, samplesOut);
    }
    if (bufferBytes < 0) {
        return buffer != NULL ? kAvierrBufferTooSmall : 0;
    }
    if (samples == -1) {
        const int32_t k = findChunk(s, start - (int32_t)info->start);

        count =
            k < 0 ? 0 : s->chunks[k].firstSample + s->chunks[k].size / size - (start - (int32_t)info->start);
    } else if (samples == 0) {
        count = bufferBytes / size;
    } else if (samples < 0) {
        count = INT32_MAX;
    } else {
        count = samples;
    }
    if (buffer != NULL) {
        count = SDL_min(count, (int64_t)(end - start));
        count = SDL_min(count, (int64_t)(bufferBytes / size));
    } else if (bufferBytes != 0) {
        count = SDL_min(count, (int64_t)(bufferBytes / size));
    }
    count = SDL_min(count, (int64_t)(INT32_MAX / size));
    samples = (int32_t)count;
    bytes = samples * size;
    if (buffer != NULL) {
        int64_t sample = start - (int32_t)info->start;
        int32_t left = samples;
        int32_t k = findChunk(s, sample);

        while (left > 0) {
            const AviChunk *c;
            int64_t inChunk, n;

            if (k < 0 || (uint32_t)k >= s->chunkCount) {
                return kAvierrFileRead;
            }
            c = &s->chunks[k];
            inChunk = sample - c->firstSample;
            n = SDL_min((int64_t)left, (int64_t)(c->size / (uint32_t)size) - inChunk);
            if (!readAt(s->file->io, c->offset + (uint64_t)(inChunk * size), out, (size_t)(n * size))) {
                return kAvierrFileRead;
            }
            out += n * size;
            sample += n;
            left -= (int32_t)n;
            k++;
        }
        if (bytes > 0) {
            const uint8_t *const data = buffer;

            for (int32_t i = 0; i < bytes; i++) {
                s->readHash = (s->readHash ^ data[i]) * 0x100000001b3ull;
            }
            s->readBytes += (uint64_t)bytes;
            s->reads++;
        }
    }
    setCounts(bytesOut, samplesOut, bytes, samples);
    return 0;
}

/* ---- frames ---- */

/* Only the stream's own format (wanted NULL, as the game asks) of a Cinepak stream: a 24-bit bottom-up DIB.
 */
void *BMX_WINAPI bmxStandAVIStreamGetFrameOpen(void *stream, void *wanted)
{
    BmxAviStream *const s = asStream(stream);
    BmxGetFrame *g;
    int32_t width, height;
    uint8_t *h;

    if (s == NULL || s->info.type != kFccVids || s->formatSize < kDibHeader) {
        return NULL;
    }
    width = (int32_t)le32(s->format + 4);
    height = (int32_t)le32(s->format + 8);
    if (wanted != NULL || le32(s->format + 16) != kFccCvid || width <= 0 || height <= 0 || width > 4096 ||
        height > 4096) {
        bmxLog("avi: AVIStreamGetFrameOpen: only Cinepak to the stream's own format is supported\n");
        return NULL;
    }
    g = SDL_calloc(1, sizeof *g);
    if (g == NULL) {
        return NULL;
    }
    g->magic = kFrameMagic;
    g->stream = s;
    g->current = -1;
    g->stride = (width * 3 + 3) & ~3;
    g->dib = SDL_calloc(1, kDibHeader + (size_t)g->stride * (size_t)height);
    if (g->dib == NULL) {
        SDL_free(g);
        return NULL;
    }
    cinepakInit(&g->decoder, width, height);
    h = g->dib;
    h[0] = kDibHeader;
    memcpy(h + 4, s->format + 4, 8); /* width, height */
    h[12] = 1;                       /* planes */
    h[14] = 24;                      /* bits, BI_RGB */
    h[20] = (uint8_t)(g->stride * height);
    h[21] = (uint8_t)((g->stride * height) >> 8);
    h[22] = (uint8_t)((g->stride * height) >> 16);
    h[23] = (uint8_t)((g->stride * height) >> 24);
    s->refs++;
    s->file->refs++;
    for (int i = 0; i < kMaxFrameObjects; i++) {
        if (gFrames[i] == NULL) {
            gFrames[i] = g;
            break;
        }
    }
    return g;
}

int32_t bmxAviFrameOf(const void *dib, const char **streamName)
{
    for (int i = 0; i < kMaxFrameObjects; i++) {
        if (gFrames[i] != NULL && gFrames[i]->dib == dib) {
            *streamName = gFrames[i]->stream->info.name;
            return gFrames[i]->current + (int32_t)gFrames[i]->stream->info.start;
        }
    }
    *streamName = "?";
    return -1;
}

static int decodeFrame(BmxGetFrame *g, int32_t frame)
{
    const AviChunk *const c = &g->stream->chunks[frame];

    if (c->size == 0) {
        return 1; /* a dropped frame: the previous image stays */
    }
    if (c->size > g->chunkCapacity) {
        uint8_t *const grown = SDL_realloc(g->chunk, c->size);

        if (grown == NULL) {
            return 0;
        }
        g->chunk = grown;
        g->chunkCapacity = c->size;
    }
    if (!readAt(g->stream->file->io, c->offset, g->chunk, c->size)) {
        return 0;
    }
    cinepakDecode(&g->decoder, g->chunk, c->size, g->dib + kDibHeader, g->stride);
    return 1;
}

/* Decodes from the last key frame at or before `pos` (or on from the frame in the buffer when that is
 * nearer). Returns the packed DIB (header + bits) or NULL outside the stream. */
void *BMX_WINAPI bmxStandAVIStreamGetFrame(void *getFrame, int32_t pos)
{
    BmxGetFrame *const g = getFrame;
    int32_t frame, key, from;

    if (g == NULL || g->magic != kFrameMagic) {
        return NULL;
    }
    frame = pos - (int32_t)g->stream->info.start;
    if (frame < 0 || (uint32_t)frame >= g->stream->chunkCount) {
        return NULL;
    }
    if (frame == g->current) {
        return g->dib;
    }
    key = frame;
    while (key > 0 && (g->stream->chunks[key].flags & kAviifKeyframe) == 0) {
        key--;
    }
    from = (g->current >= key && g->current < frame) ? g->current + 1 : key;
    for (int32_t f = from; f <= frame; f++) {
        if (!decodeFrame(g, f)) {
            g->current = -1;
            return NULL;
        }
    }
    g->current = frame;
    return g->dib;
}

int32_t BMX_WINAPI bmxStandAVIStreamGetFrameClose(void *getFrame)
{
    BmxGetFrame *const g = getFrame;

    if (g == NULL || g->magic != kFrameMagic) {
        return kAvierrBadHandle;
    }
    g->magic = 0;
    for (int i = 0; i < kMaxFrameObjects; i++) {
        if (gFrames[i] == g) {
            gFrames[i] = NULL;
        }
    }
    bmxStandAVIStreamRelease(g->stream);
    SDL_free(g->chunk);
    SDL_free(g->dib);
    SDL_free(g);
    return 0;
}

/*
 * AVISaveOptionsFree (AVIFIL32): zmovieClose frees each stream's lpFormat with the CRT (zmemFree) and then
 * hands the AVICOMPRESSOPTIONS to this function, which GlobalFrees lpFormat and lpParms and clears them. In
 * the original the block comes from MSVC6's own heap and that GlobalFree fails harmlessly; with our CRT the
 * block is the process heap's and the second free is fatal (heap corruption). Only the clearing remains.
 */
int32_t BMX_WINAPI bmxStandAVISaveOptionsFree(int count, void **options)
{
    for (int i = 0; i < count; i++) {
        uint8_t *const o = options[i];

        memset(o + 0x18, 0, 4); /* lpFormat */
        memset(o + 0x20, 0, 4); /* lpParms */
    }
    return 0;
}
