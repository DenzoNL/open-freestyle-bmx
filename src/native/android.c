/*
 * Android (docs/PORTING.md "X6 step 1 (Android arm64)"): bmxnative is libmain.so, SDL's Java activity calls
 * its SDL_main on SDL's thread. An app has no executable folder and starts in `/`, so before anything else
 * main() makes the app's external files folder the place for everything:
 *
 * - <files>/game/: the game folder in the install layout (Scene.Txt, Assets/, bin/, save/) and the working
 *   directory (saves go to its save/, as on Windows);
 * - <files>/bmxnative.ini: the config (BMX_CONFIG, unless the environment names one);
 * - <files>/bmxnative.log: the log (also in logcat, tag "opendmfbmx").
 *
 * <files> is SDL_GetAndroidExternalStoragePath(): /storage/emulated/0/Android/data/<package>/files, which
 * `adb push` can write to without any storage permission.
 */
#if defined(__ANDROID__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <SDL3/SDL.h>
#include <android/log.h>

#include "native/native.h"

static char gFolder[1024];

const char *bmxAndroidFolder(void)
{
    return gFolder;
}

void bmxAndroidLog(const char *fmt, va_list ap)
{
    __android_log_vprint(ANDROID_LOG_INFO, "opendmfbmx", fmt, ap);
}

int bmxAndroidPrepare(void)
{
    const char *files = SDL_GetAndroidExternalStoragePath();
    char path[sizeof gFolder + 32];

    if (files == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, "opendmfbmx", "no external storage: %s", SDL_GetError());
        return -1;
    }
    snprintf(gFolder, sizeof gFolder, "%s", files);
    snprintf(path, sizeof path, "%s/bmxnative.ini", gFolder);
    setenv("BMX_CONFIG", path, 0);
    snprintf(path, sizeof path, "%s/game", gFolder);
    if (chdir(path) != 0) {
        char text[sizeof path + 256];

        __android_log_print(ANDROID_LOG_ERROR, "opendmfbmx", "no game folder %s", path);
        snprintf(
            text, sizeof text,
            "No game folder at\n%s\n\nCopy the game's files there (Scene.Txt, Assets, bin, save):\nadb push "
            "<game>/. %s/",
            path, path);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "opendmfbmx", text, NULL);
        return -1;
    }
    return 0;
}
#endif
