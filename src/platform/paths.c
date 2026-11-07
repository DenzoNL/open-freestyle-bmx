/*
 * Path resolution (docs/PLATFORM.md P18). The game names its files relative to the install folder (the
 * working directory) with `\` separators, in whatever case the code or the data happens to use
 * (`Assets\Tables.ZAL`, `bin\audio\MUSIC.ZSD`, `save\SAVE.DAT`).
 *
 * Windows: the identity. macOS / Linux (docs/PORTING.md "X5 part 1"): `\` becomes `/`, and a path that does
 * not exist as written is looked up one component at a time, each matched without regard to case in its
 * directory (the first match in directory order). Components that do not exist at all (a file about to be
 * created) stay as written. The result lives in a per-thread buffer until that thread's next call (the sound
 * stream opens files on the timer thread).
 */
#include "platform/platform.h"

#if defined(_WIN32)
const char *bmxPathResolve(const char *gamePath)
{
    return gamePath;
}
#else
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

enum { kPathMax = 1024 };

/* Appends the entry of dir (`dir` "" = the working directory) whose name equals name without regard to case
 * to out, after a '/' when out is not empty; 0 when there is none. */
static int appendMatch(char *out, size_t outLen, const char *name)
{
    DIR *const d = opendir(outLen == 0 ? "." : out);
    const struct dirent *e;
    int found = 0;

    if (d == NULL) {
        return 0;
    }
    while ((e = readdir(d)) != NULL) {
        if (strcasecmp(e->d_name, name) == 0) {
            const size_t n = strlen(e->d_name);

            if (outLen + 1 + n < kPathMax) {
                if (outLen != 0 && out[outLen - 1] != '/') {
                    out[outLen++] = '/';
                }
                memcpy(out + outLen, e->d_name, n + 1);
                found = 1;
            }
            break;
        }
    }
    closedir(d);
    return found;
}

const char *bmxPathResolve(const char *gamePath)
{
    static _Thread_local char path[kPathMax], out[kPathMax];
    size_t n = strlen(gamePath), len = 0, start = 0;
    char *component, *next;

#if defined(__ANDROID__)
    /* SDL_IOFromFile on Android opens a relative path in the app's internal storage (then the APK's assets),
     * not in the working directory (the game folder, src/native/android.c): game paths become absolute. */
    if (gamePath[0] != '/' && gamePath[0] != '\\') {
        if (getcwd(path, kPathMax) == NULL) {
            return gamePath;
        }
        start = strlen(path);
        path[start++] = '/';
    }
#endif
    if (start + n >= kPathMax) {
        return gamePath;
    }
    for (size_t i = 0; i <= n; i++) {
        path[start + i] = gamePath[i] == '\\' ? '/' : gamePath[i];
    }
    if (access(path, F_OK) == 0) {
        return path;
    }
    out[0] = 0;
    component = path;
    if (path[0] == '/') {
        out[len++] = '/';
        out[len] = 0;
        component++;
    }
    for (; component != NULL; component = next) {
        next = strchr(component, '/');
        if (next != NULL) {
            *next++ = 0;
        }
        if (*component == 0) {
            continue; /* "a//b", a trailing '/' */
        }
        /* As written first (".", "..", the right case), then the case-insensitive match. */
        const size_t mark = len;
        const size_t c = strlen(component);

        if (len + 1 + c >= kPathMax) {
            return gamePath;
        }
        if (len != 0 && out[len - 1] != '/') {
            out[len++] = '/';
        }
        memcpy(out + len, component, c + 1);
        if (access(out, F_OK) == 0) {
            len += c;
            continue;
        }
        len = mark;
        out[len] = 0;
        if (appendMatch(out, len, component)) {
            len = strlen(out);
            continue;
        }
        /* Not there in any case: the rest as written. */
        if (len != 0 && out[len - 1] != '/') {
            out[len++] = '/';
        }
        snprintf(out + len, kPathMax - len, "%s%s%s", component, next != NULL ? "/" : "",
                 next != NULL ? next : "");
        break;
    }
    return out;
}
#endif
