/*
 * File shims of the sound loader (docs/PLATFORM.md P18, S4): CreateFileA, ReadFile, SetFilePointer and
 * CloseHandle as winload.c / winstrm.c use them (read-only: GENERIC_READ, OPEN_EXISTING, FILE_BEGIN /
 * FILE_END, no overlapped I/O) on SDL_IOStream. The HANDLE is a pointer to our own record. Paths go through
 * bmxPathResolve. Anything else (write access, other dispositions, overlapped reads) is refused and logged.
 *
 * Semantics kept from Windows: a failed open returns INVALID_HANDLE_VALUE (-1); ReadFile at the end of the
 * file succeeds with 0 bytes read, a short read at the end succeeds with the bytes there are; SetFilePointer
 * returns the new position's low 32 bits (high part through the pointer when given) and 0xFFFFFFFF on
 * failure.
 *
 * Checks, both off by default:
 *   BMX_FILE_LOG=1    on CloseHandle log the path, the number of reads, the bytes read and the FNV-1a 64 hash
 *                     of the bytes read, in read order.
 *   BMX_FILE_CHECK=1  also open every file with the real kernel32 and repeat each call there: results, byte
 *                     counts, bytes read and positions must be equal; each mismatch is logged, the totals at
 *                     CloseHandle (implies BMX_FILE_LOG).
 */
#include <SDL3/SDL.h>
#include <string.h>

#include "platform/standins.h"

enum {
    kGenericRead = 0x80000000u,
    kOpenExisting = 3,
    kFileBegin = 0,
    kFileCurrent = 1,
    kFileEnd = 2,
    kInvalidSetFilePointer = 0xffffffffu,
    kCheckBuffer = 1 << 16,
};

typedef void *(BMX_WINAPI *CreateFileFn)(const char *, uint32_t, uint32_t, void *, uint32_t, uint32_t,
                                         void *);
typedef int32_t(BMX_WINAPI *ReadFileFn)(void *, void *, uint32_t, uint32_t *, void *);
typedef uint32_t(BMX_WINAPI *SetFilePointerFn)(void *, int32_t, int32_t *, uint32_t);
typedef int32_t(BMX_WINAPI *CloseHandleFn)(void *);

typedef struct GameFile {
    SDL_IOStream *io;
    void *real; /* BMX_FILE_CHECK: the same file through kernel32 */
    char path[260];
    uint64_t hash, bytes, reads, mismatches;
} GameFile;

static int envOn(const char *name)
{
    const char *e = SDL_getenv(name);

    return e != NULL && *e != 0 && *e != '0';
}

static int checking(void)
{
    static int state = -1;

    if (state < 0) {
        state = envOn("BMX_FILE_CHECK");
#if !defined(_WIN32)
        if (state) {
            bmxLog("BMX_FILE_CHECK compares with kernel32 (Windows only): no check; BMX_FILE_LOG=1 still "
                   "logs\n");
            state = 0;
        }
#endif
    }
    return state;
}

static int logging(void)
{
    static int state = -1;

    if (state < 0) {
        state = envOn("BMX_FILE_LOG") || checking();
    }
    return state;
}

void *BMX_WINAPI bmxStandCreateFileA(const char *name, uint32_t access, uint32_t share, void *security,
                                     uint32_t disposition, uint32_t flags, void *templateFile)
{
    GameFile *f;
    const char *path;

    if (name == NULL || access != kGenericRead || disposition != kOpenExisting) {
        bmxLog("CreateFileA(\"%s\", access %#x, disposition %u): only read-only opens are supported\n",
               name != NULL ? name : "(null)", (unsigned)access, (unsigned)disposition);
        return (void *)(intptr_t)-1;
    }
    path = bmxPathResolve(name);
    f = SDL_calloc(1, sizeof *f);
    if (f == NULL) {
        return (void *)(intptr_t)-1;
    }
    f->io = SDL_IOFromFile(path, "rb");
    if (f->io == NULL) {
        SDL_free(f);
        return (void *)(intptr_t)-1;
    }
    SDL_strlcpy(f->path, name, sizeof f->path);
    f->hash = 0xcbf29ce484222325ull;
    if (checking()) {
        const CreateFileFn real = (CreateFileFn)bmxRealProc("KERNEL32.dll", "CreateFileA");

        f->real = real(path, access, share, security, disposition, flags, templateFile);
        if (f->real == (void *)(intptr_t)-1) {
            bmxLog("filecheck: %s opens here but not through kernel32\n", f->path);
            f->mismatches++;
        }
    }
    return f;
}

static void mismatch(GameFile *f, const char *what)
{
    f->mismatches++;
    bmxLog("filecheck: %s: %s differs from kernel32 (read %llu)\n", f->path, what,
           (unsigned long long)f->reads);
}

/* The same read through kernel32, compared with what we returned. */
static void checkRead(GameFile *f, const uint8_t *data, uint32_t bytes, uint32_t read)
{
    const ReadFileFn real = (ReadFileFn)bmxRealProc("KERNEL32.dll", "ReadFile");
    uint8_t *const buffer = SDL_malloc(kCheckBuffer); /* streams are read on the timer thread */
    uint32_t done = 0;

    if (buffer == NULL) {
        return;
    }

    do {
        const uint32_t n = bytes - done < kCheckBuffer ? bytes - done : kCheckBuffer;
        uint32_t got = 0;

        if (real(f->real, buffer, n, &got, NULL) != 1) {
            mismatch(f, "ReadFile result");
            SDL_free(buffer);
            return;
        }
        if (done + got > read || memcmp(buffer, data + done, got) != 0) {
            mismatch(f, "ReadFile bytes");
            SDL_free(buffer);
            return;
        }
        done += got;
        if (got < n) {
            break;
        }
    } while (done < bytes);
    if (done != read) {
        mismatch(f, "ReadFile byte count");
    }
    SDL_free(buffer);
}

int32_t BMX_WINAPI bmxStandReadFile(void *file, void *buffer, uint32_t bytes, uint32_t *read,
                                    void *overlapped)
{
    GameFile *const f = file;
    const uint8_t *const data = buffer;
    size_t got;

    if (f == NULL || f == (void *)(intptr_t)-1 || overlapped != NULL) {
        bmxLog("ReadFile: unsupported call (handle %p, overlapped %p)\n", file, overlapped);
        if (read != NULL) {
            *read = 0;
        }
        return 0;
    }
    got = SDL_ReadIO(f->io, buffer, bytes);
    if (got < bytes && SDL_GetIOStatus(f->io) == SDL_IO_STATUS_ERROR) {
        if (read != NULL) {
            *read = (uint32_t)got;
        }
        return 0;
    }
    if (read != NULL) {
        *read = (uint32_t)got;
    }
    if (logging()) {
        for (size_t i = 0; i < got; i++) {
            f->hash = (f->hash ^ data[i]) * 0x100000001b3ull;
        }
        f->bytes += got;
        f->reads++;
        if (f->real != NULL && f->real != (void *)(intptr_t)-1) {
            checkRead(f, data, bytes, (uint32_t)got);
        }
    }
    return 1;
}

uint32_t BMX_WINAPI bmxStandSetFilePointer(void *file, int32_t distance, int32_t *high, uint32_t method)
{
    GameFile *const f = file;
    const int64_t offset =
        high != NULL ? (int64_t)(((uint64_t)(uint32_t)*high << 32) | (uint32_t)distance) : (int64_t)distance;
    SDL_IOWhence whence;
    int64_t pos;
    uint32_t result;

    if (f == NULL || f == (void *)(intptr_t)-1) {
        return kInvalidSetFilePointer;
    }
    whence = method == kFileEnd       ? SDL_IO_SEEK_END
             : method == kFileCurrent ? SDL_IO_SEEK_CUR
                                      : SDL_IO_SEEK_SET;
    /* SDL's Windows backend returns 0, not -1, for a seek before the start (SDL_SeekIO: a failure sets the
     * error, and the position stays), so a failure is told by the error string. */
    SDL_ClearError();
    pos = SDL_SeekIO(f->io, offset, whence);
    if (pos < 0 || *SDL_GetError() != 0 || pos != SDL_TellIO(f->io)) {
        result = kInvalidSetFilePointer;
    } else {
        result = (uint32_t)pos;
        if (high != NULL) {
            *high = (int32_t)(pos >> 32);
        }
    }
    if (f->real != NULL && f->real != (void *)(intptr_t)-1) {
        const SetFilePointerFn real = (SetFilePointerFn)bmxRealProc("KERNEL32.dll", "SetFilePointer");
        int32_t realHigh = high != NULL ? (int32_t)(offset >> 32) : 0;
        const uint32_t r = real(f->real, distance, high != NULL ? &realHigh : NULL, method);

        if (r != result || (high != NULL && pos >= 0 && realHigh != *high)) {
            mismatch(f, "SetFilePointer");
        }
    }
    return result;
}

int32_t BMX_WINAPI bmxStandCloseHandle(void *file)
{
    GameFile *const f = file;

    if (f == NULL || f == (void *)(intptr_t)-1) {
        return 0;
    }
    if (logging()) {
        bmxLog("file: %s: %llu reads, %llu bytes, fnv1a64 %016llx%s\n", f->path, (unsigned long long)f->reads,
               (unsigned long long)f->bytes, (unsigned long long)f->hash,
               f->real != NULL ? (f->mismatches ? "  filecheck MISMATCH" : "  filecheck ok") : "");
    }
    if (f->real != NULL && f->real != (void *)(intptr_t)-1) {
        ((CloseHandleFn)bmxRealProc("KERNEL32.dll", "CloseHandle"))(f->real);
    }
    SDL_CloseIO(f->io);
    SDL_free(f);
    return 1;
}
