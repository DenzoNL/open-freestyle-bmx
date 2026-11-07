/*
 * Android (docs/PORTING.md "X6 step 1 (Android arm64)"): bmxnative is libmain.so, SDL's Java activity calls
 * its SDL_main on SDL's thread. An app has no executable folder and starts in `/`, so before anything else
 * main() makes the app's external files folder the place for everything:
 *
 * - <files>/game/: the game folder in the install layout (Scene.Txt, Assets/, bin/, save/) and the working
 *   directory (saves go to its save/, as on Windows);
 * - <files>/OpenFreestyleBMX.ini: the config (BMX_CONFIG, unless the environment names one);
 * - <files>/OpenFreestyleBMX.log: the log (also in logcat, tag OpenFreestyleBMX).
 *
 * <files> is SDL_GetAndroidExternalStoragePath(): /storage/emulated/0/Android/data/<package>/files, which
 * `adb push` can write to without any storage permission. Without a game there, the first start asks the
 * player for the game folder and copies it in (BmxActivity.pickGameFolder; docs/RELEASE-PLAN.md D2-A).
 */
#if defined(__ANDROID__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <SDL3/SDL.h>
#include <android/log.h>
#include <jni.h>

#include "compat/product.h"
#include "native/native.h"
#include "platform/platform.h"

static char gFolder[1024];

const char *bmxAndroidFolder(void)
{
    return gFolder;
}

void bmxAndroidLog(const char *fmt, va_list ap)
{
    __android_log_vprint(ANDROID_LOG_INFO, BMX_PRODUCT_FILE, fmt, ap);
}

/* The game is in dir: its Scene.Txt is there (any case, as the game's own lookups). */
static int hasGame(const char *dir)
{
    char path[sizeof gFolder + 64];

    snprintf(path, sizeof path, "%s/Scene.Txt", dir);
    return SDL_GetPathInfo(path, NULL) || SDL_GetPathInfo(bmxPathResolve(path), NULL);
}

/* BmxActivity.pickGameFolder(target) (Java, docs/RELEASE-PLAN.md D2-A): the player picks the game folder, it
 * is copied to target. Runs on SDL's thread, which may block; 1 when copied. */
static int pickGameFolder(const char *target)
{
    JNIEnv *const env = (JNIEnv *)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass cls;
    jmethodID method;
    jstring arg;
    jboolean ok;

    if (env == NULL || activity == NULL) {
        return 0;
    }
    cls = (*env)->GetObjectClass(env, activity);
    method = (*env)->GetStaticMethodID(env, cls, "pickGameFolder", "(Ljava/lang/String;)Z");
    if (method == NULL) {
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, cls);
        (*env)->DeleteLocalRef(env, activity);
        return 0;
    }
    arg = (*env)->NewStringUTF(env, target);
    ok = (*env)->CallStaticBooleanMethod(env, cls, method, arg);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        ok = JNI_FALSE;
    }
    (*env)->DeleteLocalRef(env, arg);
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
    return ok == JNI_TRUE;
}

int bmxAndroidPrepare(void)
{
    const char *files = SDL_GetAndroidExternalStoragePath();
    char path[sizeof gFolder + 32];

    if (files == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, BMX_PRODUCT_FILE, "no external storage: %s", SDL_GetError());
        return -1;
    }
    snprintf(gFolder, sizeof gFolder, "%s", files);
    snprintf(path, sizeof path, "%s/" BMX_PRODUCT_INI, gFolder);
    setenv("BMX_CONFIG", path, 0);
    snprintf(path, sizeof path, "%s/game", gFolder);
    if (!hasGame(path)) {
        __android_log_print(ANDROID_LOG_INFO, BMX_PRODUCT_FILE, "no game in %s: asking for the game folder",
                            path);
        if (!pickGameFolder(path) || !hasGame(path)) {
            __android_log_print(ANDROID_LOG_ERROR, BMX_PRODUCT_FILE, "no game folder %s", path);
            return -1;
        }
    }
    if (chdir(path) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, BMX_PRODUCT_FILE, "cannot enter %s", path);
        return -1;
    }
    return 0;
}
#endif
