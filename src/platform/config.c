/*
 * The config file (docs/PLATFORM.md P19, S6): bmxnative.ini next to the exe, or the file BMX_CONFIG names.
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
 * Read once, on the first query. A missing file means every option at its default (vanilla). Values are
 * integers (decimal or 0x hex), paths are UTF-8 text used as written; unknown keys and bad lines are logged
 * and ignored. Options that would change
 * the simulation would have to go into replays (PLAN.md); none of the current ones does.
 */
#include <SDL3/SDL.h>

#include "platform/platform.h"

enum { kMaxEntries = 64, kKeyLen = 64, kValueLen = 512 };

typedef struct Entry {
    char key[kKeyLen];
    char value[kValueLen];
} Entry;

static Entry gEntries[kMaxEntries];
static int gCount, gLoaded;

/* The keys this build knows (all off by default). */
static const char *const kKnownKeys[] = {
    "input.dpad_steering",   /* P7: the d-pad also sets the stick axes */
    "input.stick_8way",      /* P7-B: the left stick snaps to 8 directions */
    "audio.smooth_resample", /* P9 note: 0 original mixer, 1 linear, 2 cubic resampling (smoothmix.c) */
    "audio.music_dir",       /* the music override: a folder with the user's FLACs (docs/MUSIC.md) */
    "video.width", /* P16: the mode without the device dialog (default 640 x 480 x 16 full screen) */
    "video.height",
    "video.depth",
    "video.fullscreen", /* 0: a window of width x height (the game renders at that size) */
    "ui.confirm_quit",  /* 1: the main menu's QUIT asks "Do you want to quit the game?" (0: quits at once) */
    /* docs/WIDESCREEN.md W9 (width / height 0: the desktop size) */
    "video.widescreen",   /* 1: hor+ 3D, centred 4:3 HUD and menus, pillarboxed movies (0: a wide mode
                             stretches) */
    "video.aspect",       /* 0 stretch, 1 hor+ keeping the 4:3 view inside (default), 2 vert- */
    "video.fov_scale",    /* percent on both view window axes (default 100) */
    "video.max_hfov",     /* cap of the horizontal field of view in degrees (0: none) */
    "video.hud",          /* 0 centred 4:3 (default), 1 edge anchored (WS5), 2 stretch */
    "video.hud_aspect",   /* width of the anchored HUD: aspect * 100 (default 178, 16:9); 0 = full width */
    "video.menus",        /* 0 pillarbox (default), 1 stretch, 2 pillarbox on a blurred fill */
    "video.movies",       /* 0 pillarbox (default), 1 stretch */
    "video.scale_filter", /* present: 0 linear (default), 1 nearest, 2 integer scale */
    "video.ui_filter",    /* 2D / HUD textures: 0 the game's nearest (default), 1 linear, 2 sharp bilinear */
    /* docs/HIGHFPS.md H8 */
    "video.interpolate", /* 1: extra draws between the 30 Hz ticks in levels, one tick behind (0: vanilla) */
    "video.max_fps",     /* draws per second with interpolate = 1 (0: the display's refresh rate) */
    "video.vsync",       /* 1: present with vsync (default), 0: immediate (mailbox when not supported) */
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

static void load(void)
{
    const char *path = SDL_getenv("BMX_CONFIG");
    char *own = NULL;
    void *text;

    gLoaded = 1;
    if (path == NULL || *path == 0) {
        const char *base = SDL_GetBasePath();

        if (base == NULL || SDL_asprintf(&own, "%sbmxnative.ini", base) < 0) {
            return;
        }
        path = own;
    }
    text = SDL_LoadFile(path, NULL);
    if (text == NULL) {
        bmxLog("config: no %s, every option off\n", path);
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
