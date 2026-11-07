/*
 * The config file (docs/PLATFORM.md P19, S6): OpenFreestyleBMX.ini next to the exe (bmxnative.ini, its name
 * before the rename, when that one is missing), or the file BMX_CONFIG names.
 *
 *   # comment            ; comment
 *   [input]
 *   dpad_steering = 1    -> key "input.dpad_steering" (also stick_8way)
 *   [audio]
 *   smooth_resample = 2  -> key "audio.smooth_resample" (0 original, 1 linear, 2 cubic)
 *   music_dir = D:\Music -> key "audio.music_dir" (text: the music override's folder, docs/MUSIC.md)
 *   [video]
 *   width = 640          -> the mode when the command line has no -r (also height, depth, fullscreen)
 *   [ui]
 *   confirm_quit = 0     -> 1: the quit prompt as in the original
 *
 * Read once, on the first query. A missing file means every option at its default: the enhanced one for the
 * options read with bmxConfigOption (`[general] vanilla = 1` gives the original's), the original's for the
 * rest. Values are integers (decimal or 0x hex), paths are UTF-8 text used as written; unknown keys and bad
 * lines are logged and ignored. Options that would change the simulation would have to go into replays
 * (PLAN.md); none of the current ones does.
 */
#include <SDL3/SDL.h>

#include "compat/product.h"
#include "platform/platform.h"

enum { kMaxEntries = 64, kKeyLen = 64, kValueLen = 512 };

typedef struct Entry {
    char key[kKeyLen];
    char value[kValueLen];
} Entry;

static Entry gEntries[kMaxEntries];
static int gCount, gLoaded;
static char gPath[1024]; /* the file load() read or would have read */

/* The keys this build knows. */
static const char *const kKnownKeys[] = {
    "general.vanilla",     /* 1: every option without a value in the file at the original's behaviour */
    "general.game_dir",    /* the game folder, when the program is not started in it (src/native/gamedir.c) */
    "input.dpad_steering", /* P7: the d-pad also sets the stick axes */
    "input.stick_8way",    /* P7-B: the left stick snaps to 8 directions */
    "audio.smooth_resample", /* P9 note: 0 original mixer, 1 linear, 2 cubic resampling (smoothmix.c) */
    "audio.music_dir",       /* the music override: a folder with the user's FLACs (docs/MUSIC.md) */
    "audio.mute",            /* 1: no sound at all (the same as BMX_MUTE=1) */
    "video.width", /* P16: the mode without the device dialog (default 0 x 0 x 16 full screen, 0: the desktop
                      size) */
    "video.height",
    "video.depth",
    "video.fullscreen", /* 0: a window of width x height (the game renders at that size) */
    "ui.confirm_quit",  /* 1: the main menu's QUIT asks "Do you want to quit the game?" (0: quits at once) */
    /* docs/WIDESCREEN.md W9 (width / height 0: the desktop size) */
    "video.widescreen",   /* 1 (default): hor+ 3D, pillarboxed menus and movies (0: a wide mode stretches) */
    "video.aspect",       /* 0 stretch, 1 hor+ keeping the 4:3 view inside (default), 2 vert- */
    "video.fov_scale",    /* percent on both view window axes (default 100) */
    "video.max_hfov",     /* cap of the horizontal field of view in degrees (0: none) */
    "video.hud",          /* 0 centred 4:3, 1 edge anchored (default, WS5), 2 stretch */
    "video.hud_aspect",   /* width of the anchored HUD: aspect * 100 (default 178, 16:9); 0 = full width */
    "video.menus",        /* 0 pillarbox, 1 stretch, 2 pillarbox on a blurred fill (default) */
    "video.movies",       /* 0 pillarbox (default), 1 stretch */
    "video.scale_filter", /* present: 0 linear (default), 1 nearest, 2 integer scale */
    "video.ui_filter",    /* 2D / HUD textures: 0 the game's nearest, 1 linear, 2 sharp bilinear (default) */
    /* docs/HIGHFPS.md H8 */
    "video.interpolate", /* 1 (default): extra draws between the 30 Hz ticks in levels, one tick behind */
    "video.max_fps",     /* draws per second with interpolate = 1 (0: the display's refresh rate) */
    "video.vsync",       /* 1: present with vsync (default), 0: immediate (mailbox when not supported) */
    "video.backend",     /* "null": no GPU device, no visible window (headless; BMX_GPU=null does the same) */
    /* src/native/drawdist.c */
    "video.full_draw_distance", /* 1 (default): far clip and fog at their maximum, no frame rate adapter */
};

static int known(const char *key)
{
    for (size_t i = 0; i < sizeof kKnownKeys / sizeof kKnownKeys[0]; i++) {
        if (SDL_strcasecmp(kKnownKeys[i], key) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Copies [begin, end) without surrounding blanks. */
static void trimCopy(char *out, size_t size, const char *begin, const char *end)
{
    size_t n;

    while (begin < end && (*begin == ' ' || *begin == '\t')) {
        begin++;
    }
    while (end > begin && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) {
        end--;
    }
    n = (size_t)(end - begin) < size - 1 ? (size_t)(end - begin) : size - 1;
    SDL_memcpy(out, begin, n);
    out[n] = 0;
}

void bmxConfigLoadText(const char *text, const char *origin)
{
    char section[kKeyLen] = "";
    int lineNo = 0;

    gCount = 0;
    gLoaded = 1;
    for (const char *p = text; p != NULL && *p != 0;) {
        const char *eol = SDL_strchr(p, '\n');
        const char *end = eol != NULL ? eol : p + SDL_strlen(p);
        const char *comment = p;
        char name[kKeyLen + kValueLen], value[kValueLen]; /* name: the whole line */

        lineNo++;
        while (comment < end && *comment != '#' && *comment != ';') {
            comment++;
        }
        trimCopy(name, sizeof name, p, comment);
        if (name[0] == '[') {
            char *close = SDL_strchr(name, ']');

            if (close == NULL) {
                bmxLog("config: %s:%d: bad section header\n", origin, lineNo);
            } else {
                *close = 0;
                trimCopy(section, sizeof section, name + 1, close);
            }
        } else if (name[0] != 0) {
            const char *eq = SDL_strchr(name, '=');

            if (eq == NULL) {
                bmxLog("config: %s:%d: expected key = value\n", origin, lineNo);
            } else if (gCount == kMaxEntries) {
                bmxLog("config: %s:%d: too many entries\n", origin, lineNo);
            } else {
                Entry *const e = &gEntries[gCount];
                char key[kKeyLen];

                trimCopy(key, sizeof key, name, eq);
                trimCopy(value, sizeof value, eq + 1, name + SDL_strlen(name));
                SDL_snprintf(e->key, sizeof e->key, "%s%s%s", section, section[0] ? "." : "", key);
                SDL_snprintf(e->value, sizeof e->value, "%s", value);
                if (!known(e->key)) {
                    bmxLog("config: %s:%d: unknown key \"%s\" ignored\n", origin, lineNo, e->key);
                } else {
                    bmxLog("config: %s = %s\n", e->key, e->value);
                    gCount++;
                }
            }
        }
        p = eol != NULL ? eol + 1 : NULL;
    }
}

/* The settings file a first start writes: every option commented out at its default, so it changes nothing
 * until the player removes a ';'. Keep the values in step with the bmxConfig* calls' defaults. */
static const char kTemplate[] =
    "; Open Freestyle BMX settings.\n"
    "; Remove the ';' at the start of a line to change that setting. The values shown are the defaults.\n"
    "\n"
    "[general]\n"
    "; vanilla = 0             ; 1: the original game's look and behaviour for every option not set below\n"
    "; game_dir =              ; the game folder, when this program is not in it (the folder picker sets "
    "it)\n"
    "\n"
    "[video]\n"
    "; fullscreen = 1          ; 0: a window of width x height\n"
    "; width = 0               ; 0: the screen's size\n"
    "; height = 0\n"
    "; widescreen = 1          ; 0: 4:3, stretched on a wide screen\n"
    "; hud = 1                 ; 0: the HUD in the 4:3 middle, 1: at the screen's edges, 2: stretched\n"
    "; hud_aspect = 178        ; with hud = 1: how wide the HUD spreads, aspect ratio x 100 (0: the whole "
    "screen)\n"
    "; menus = 2               ; 0: black bars beside the menus, 1: stretched, 2: blurred sides\n"
    "; movies = 0              ; 0: black bars beside the movies, 1: stretched\n"
    "; ui_filter = 2           ; 0: pixelated 2D as the original, 1: smooth, 2: sharp and smooth\n"
    "; interpolate = 1         ; 1: smooth motion above the game's 30 frames per second, 0: 30 like the "
    "original\n"
    "; max_fps = 0             ; with interpolate = 1: frames per second at most (0: the display's refresh "
    "rate)\n"
    "; vsync = 1               ; 0: no vertical sync (may tear)\n"
    "; full_draw_distance = 1  ; 0: the original's shorter draw distance and fog\n"
    "; fov_scale = 100         ; field of view in percent\n"
    "\n"
    "[audio]\n"
    "; mute = 0                ; 1: no sound\n"
    "; music_dir =             ; a folder with your own music as FLAC files (see the README)\n"
    "\n"
    "[input]\n"
    "; dpad_steering = 1       ; 1: the d-pad steers like the stick\n"
    "; stick_8way = 1          ; 1: the stick snaps to 8 directions, as the original's d-pad did\n"
    "\n"
    "[ui]\n"
    "; confirm_quit = 0        ; 1: the main menu's QUIT asks first\n";

/* A first start (no settings file at path): the commented template, so players find the options. */
static void writeTemplate(const char *path)
{
    SDL_IOStream *const io = SDL_IOFromFile(path, "wb");

    if (io == NULL) {
        bmxLog("config: cannot write a settings template to %s: %s\n", path, SDL_GetError());
        return;
    }
    const size_t written = SDL_WriteIO(io, kTemplate, sizeof kTemplate - 1);

    if (SDL_CloseIO(io) && written == sizeof kTemplate - 1) {
        bmxLog("config: wrote the settings template %s\n", path);
    }
}

static void load(void)
{
    const char *path = SDL_getenv("BMX_CONFIG");
    char *own = NULL;
    void *text;

    gLoaded = 1;
    if (path == NULL || *path == 0) {
        const char *base = SDL_GetBasePath();

        if (base == NULL || SDL_asprintf(&own, "%s" BMX_PRODUCT_INI, base) < 0) {
            return;
        }
        if (!SDL_GetPathInfo(own, NULL)) {
            char *old = NULL;

            if (SDL_asprintf(&old, "%s" BMX_PRODUCT_OLD_INI, base) >= 0 && SDL_GetPathInfo(old, NULL)) {
                SDL_free(own);
                own = old;
            } else {
                SDL_free(old);
            }
        }
        path = own;
#if !defined(_WIN32)
        /* macOS / Linux: next to the executable (an app bundle's Resources folder), else the user's
         * preference folder (macOS: ~/Library/Application Support/OpenFreestyleBMX/; docs/PORTING.md "X5
         * part 1"). */
        if (!SDL_GetPathInfo(own, NULL)) {
            char *const pref = SDL_GetPrefPath(NULL, BMX_PRODUCT_FILE);
            char *other = NULL;

            if (pref != NULL && SDL_asprintf(&other, "%s" BMX_PRODUCT_INI, pref) >= 0 &&
                SDL_GetPathInfo(other, NULL)) {
                SDL_free(own);
                own = other;
                path = own;
            } else {
                SDL_free(other);
            }
            SDL_free(pref);
        }
#endif
    }
    SDL_snprintf(gPath, sizeof gPath, "%s", path);
    text = SDL_LoadFile(path, NULL);
    if (text == NULL) {
        bmxLog("config: no %s, every option at its default\n", path);
        writeTemplate(path);
    } else {
        bmxLog("config: %s\n", path);
        bmxConfigLoadText(text, path);
        SDL_free(text);
    }
    SDL_free(own);
}

const char *bmxConfigString(const char *key)
{
    if (!gLoaded) {
        load();
    }
    for (int i = gCount - 1; i >= 0; i--) {
        if (SDL_strcasecmp(gEntries[i].key, key) == 0) {
            return gEntries[i].value[0] != 0 ? gEntries[i].value : NULL;
        }
    }
    return NULL;
}

int bmxConfigInt(const char *key, int fallback)
{
    if (!gLoaded) {
        load();
    }
    for (int i = gCount - 1; i >= 0; i--) {
        if (SDL_strcasecmp(gEntries[i].key, key) == 0) {
            char *end;
            const long v = SDL_strtol(gEntries[i].value, &end, 0);

            if (end == gEntries[i].value || *end != 0) {
                bmxLog("config: %s = \"%s\" is not a number, using %d\n", key, gEntries[i].value, fallback);
                return fallback;
            }
            return (int)v;
        }
    }
    return fallback;
}

int bmxConfigOption(const char *key, int enhanced, int vanilla)
{
    const int fallback = bmxConfigInt("general.vanilla", 0) == 1 ? vanilla : enhanced;

    return bmxConfigInt(key, fallback);
}

const char *bmxConfigPath(void)
{
    if (!gLoaded) {
        load();
    }
    return gPath[0] != 0 ? gPath : NULL;
}

int bmxConfigAppend(const char *section, const char *key, const char *value)
{
    const char *const path = bmxConfigPath();
    SDL_IOStream *io;
    int ok;

    if (path == NULL || (io = SDL_IOFromFile(path, "ab")) == NULL) {
        bmxLog("config: cannot write %s: %s\n", path != NULL ? path : "(no file)", SDL_GetError());
        return 0;
    }
    /* A section header of its own: the later entry wins over an earlier one of the same key
     * (bmxConfigString). */
    ok = SDL_IOprintf(io, "\n[%s]\n%s = %s\n", section, key, value) > 0;
    ok = SDL_CloseIO(io) && ok;
    if (ok && gCount < kMaxEntries) {
        SDL_snprintf(gEntries[gCount].key, sizeof gEntries[gCount].key, "%s.%s", section, key);
        SDL_snprintf(gEntries[gCount].value, sizeof gEntries[gCount].value, "%s", value);
        gCount++;
    }
    bmxLog("config: %s.%s = %s written to %s\n", section, key, value, path);
    return ok;
}

int bmxAudioMuted(void)
{
    const char *const env = SDL_getenv("BMX_MUTE");

    if (env != NULL && *env != 0) {
        return *env != '0';
    }
    return bmxConfigInt("audio.mute", 0) != 0;
}
