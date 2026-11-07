/*
 * Finding the game folder (docs/RELEASE-PLAN.md D1-B). The game reads everything relative to its working
 * directory (Scene.Txt, Assets\, bin\, save\), as the original did when started from its install folder. The
 * program may now live anywhere; before the game starts, the working directory becomes the first of:
 *
 * 1. the working directory itself, when it holds Scene.Txt (started in the game folder: the drop-in way, and
 * how the tools run it);
 * 2. the program's own folder, when it holds Scene.Txt (copied into the game folder, started from a
 * shortcut);
 * 3. `[general] game_dir` from the settings file;
 * 4. a folder the player picks (SDL's folder dialog), checked for Scene.Txt and saved as `[general]
 * game_dir`.
 *
 * Desktop builds only: on Android the app's own files folder is the game folder (android.c).
 */
#if !defined(__ANDROID__)
#include <stdio.h>
#include <string.h>

#include <SDL3/SDL.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "compat/product.h"
#include "native/native.h"
#include "platform/platform.h"

static int hasGame(const char *dir)
{
    char path[1100];

    if (dir == NULL || *dir == 0) {
        return 0;
    }
    SDL_snprintf(path, sizeof path, "%s%sScene.Txt", dir,
                 (dir[SDL_strlen(dir) - 1] == '/' || dir[SDL_strlen(dir) - 1] == '\\') ? "" : "/");
    return SDL_GetPathInfo(path, NULL);
}

/* The working directory: dir (UTF-8). */
static int enter(const char *dir)
{
#if defined(_WIN32)
    wchar_t wide[1024];

    if (MultiByteToWideChar(CP_UTF8, 0, dir, -1, wide, (int)(sizeof wide / sizeof wide[0])) == 0 ||
        !SetCurrentDirectoryW(wide)) {
        bmxLog("game folder: cannot enter %s\n", dir);
        return 0;
    }
#else
    if (chdir(dir) != 0) {
        bmxLog("game folder: cannot enter %s\n", dir);
        return 0;
    }
#endif
    bmxLog("game folder: %s\n", dir);
    return 1;
}

static int gPickState; /* 0 waiting, 1 picked, -1 cancelled or failed */
static char gPicked[1024];

static void SDLCALL onPicked(void *userdata, const char *const *files, int filter)
{
    (void)userdata;
    (void)filter;
    if (files == NULL || files[0] == NULL) {
        if (files == NULL) {
            bmxLog("game folder: the folder dialog failed: %s\n", SDL_GetError());
        }
        gPickState = -1;
        return;
    }
    SDL_snprintf(gPicked, sizeof gPicked, "%s", files[0]);
    gPickState = 1;
}

/* Asks for the folder until the player picks one with the game in it, or cancels (NULL). */
static const char *pick(void)
{
    static const char kAsk[] =
        "Choose the folder where the game is installed: the one with Scene.Txt and the "
        "Assets folder in it.\n\nYou only need to do this once; the choice is saved in " BMX_PRODUCT_INI ".";

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        bmxLog("game folder: no video subsystem for the folder dialog: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, BMX_PRODUCT_NAME, kAsk, NULL);
    for (;;) {
        gPickState = 0;
        SDL_ShowOpenFolderDialog(onPicked, NULL, NULL, NULL, false);
        while (gPickState == 0) {
            SDL_Event event;

            SDL_WaitEventTimeout(&event, 100);
        }
        if (gPickState < 0) {
            return NULL;
        }
        if (hasGame(gPicked)) {
            return gPicked;
        }
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_WARNING, BMX_PRODUCT_NAME,
            "That folder does not contain the game (no Scene.Txt in it).\n\nPlease choose "
            "the folder the game was installed to.",
            NULL);
    }
}

int bmxGameDirPrepare(void)
{
    const char *dir;

    if (hasGame(".")) {
        return 0;
    }
    dir = SDL_GetBasePath();
    if (hasGame(dir)) {
        return enter(dir) ? 0 : -1;
    }
    dir = bmxConfigString("general.game_dir");
    if (hasGame(dir) && enter(dir)) {
        return 0;
    }
    if (dir != NULL) {
        bmxLog("game folder: [general] game_dir = %s has no Scene.Txt\n", dir);
    }
    dir = pick();
    if (dir == NULL || !enter(dir)) {
        bmxLog("game folder: none chosen, quitting\n");
        return -1;
    }
    bmxConfigAppend("general", "game_dir", dir);
    return 0;
}
#endif
