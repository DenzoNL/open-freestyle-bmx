/*
 * The music override offline (docs/MUSIC.md "Override"): renders each game track from the user's FLACs
 * through the edit list with the same renderer the game uses, decodes the game's own ADPCM track from
 * MUSIC.ZSD, and compares the two (mono downmix): correlation in 1 s windows (the best lag within +-2 ms,
 * since the CD plays at its own speed and the game ran ~14 ppm off it), level and the render time. Writes
 * nothing into the repo; with --wav <dir> it writes each render as a WAV file there.
 *
 *   bmxmusictool <game folder> <FLAC folder> [--track N] [--lag N] [--wav <dir>]
 */
#include <SDL3/SDL.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/music.h"
#include "platform/platform.h"

enum { kWindow = 44100 };

static int gMaxLag = 88; /* +-2 ms; --lag N */

void bmxLog(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

/* Normalised correlation of a and b over n samples. */
static double ncc(const float *a, const float *b, uint32_t n)
{
    double ab = 0, aa = 0, bb = 0;

    for (uint32_t i = 0; i < n; i++) {
        ab += (double)a[i] * b[i];
        aa += (double)a[i] * a[i];
        bb += (double)b[i] * b[i];
    }
    return aa > 0 && bb > 0 ? ab / sqrt(aa * bb) : 0.0;
}

static void writeWav(const char *path, const int16_t *frames, uint32_t count)
{
    SDL_IOStream *const io = SDL_IOFromFile(path, "wb");
    const uint32_t bytes = count * 4;

    if (io == NULL) {
        printf("  cannot write %s\n", path);
        return;
    }
    SDL_WriteIO(io, "RIFF", 4);
    SDL_WriteU32LE(io, 36 + bytes);
    SDL_WriteIO(io, "WAVEfmt ", 8);
    SDL_WriteU32LE(io, 16);
    SDL_WriteU16LE(io, 1);
    SDL_WriteU16LE(io, 2);
    SDL_WriteU32LE(io, 44100);
    SDL_WriteU32LE(io, 44100 * 4);
    SDL_WriteU16LE(io, 4);
    SDL_WriteU16LE(io, 16);
    SDL_WriteIO(io, "data", 4);
    SDL_WriteU32LE(io, bytes);
    SDL_WriteIO(io, frames, bytes);
    SDL_CloseIO(io);
}

/* 1 when [t, t + n) lies inside FLAC pieces only (crossfades included). */
static int flacOnly(const BmxMusicPiece *p, int count, uint32_t t, uint32_t n)
{
    for (int i = 0; i < count; i++) {
        if (p[i].kind == kBmxMusicGame && p[i].start - p[i].xfadeIn < t + n && p[i].end + p[i].xfadeOut > t) {
            return 0;
        }
    }
    return 1;
}

static int compareTrack(const BmxMusicEdit *e, const char *zsdPath, const BmxMusicZsdEntry *zsd,
                        const char *musicDir, const char *wavDir)
{
    char path[1024];
    BmxMusicPiece pieces[kBmxMusicMaxPieces];
    BmxMusicFlac *flac;
    BmxMusicAdpcm *adpcm;
    BmxMusicSource fs, gs;
    SDL_IOStream *io;
    uint8_t *bytes;
    int16_t *render, *gameFrames;
    float *ours, *game;
    uint64_t clipped = 0;
    Uint64 began;
    int count, how, flacPieces = 0;
    uint32_t windows = 0, flacWindows = 0, good = 0;
    double sum = 0, flacSum = 0, worst = 2, rmsOurs = 0, rmsGame = 0, lagSum = 0, worstAt = 0;
    const uint32_t n = e->gameSamples;

    how = bmxMusicFindFile(musicDir, e, path, sizeof path);
    flac = how != 0 ? bmxMusicFlacOpen(path) : NULL;
    if (flac == NULL || bmxMusicFlacFrames(flac) != e->flacSamples) {
        printf("%d %-42s no usable FLAC (%s)\n", e->index, e->title, how != 0 ? path : "not found");
        bmxMusicFlacClose(flac);
        return 0;
    }
    io = SDL_IOFromFile(zsdPath, "rb");
    bytes = malloc(zsd->size);
    adpcm = NULL;
    if (io != NULL && bytes != NULL && SDL_SeekIO(io, zsd->offset, SDL_IO_SEEK_SET) >= 0 &&
        SDL_ReadIO(io, bytes, zsd->size) == zsd->size) {
        adpcm = bmxMusicAdpcmCreate(bytes, zsd->size);
    }
    free(bytes);
    if (io != NULL) {
        SDL_CloseIO(io);
    }
    if (adpcm == NULL) {
        printf("%d cannot read the game's track\n", e->index);
        bmxMusicFlacClose(flac);
        return 0;
    }
    count = bmxMusicPlan(e, bmxMusicFlacFrames(flac), pieces, kBmxMusicMaxPieces);
    for (int i = 0; i < count; i++) {
        flacPieces += pieces[i].kind == kBmxMusicFlac;
    }
    fs = bmxMusicFlacSource(flac);
    gs = bmxMusicAdpcmSource(adpcm);
    render = malloc(sizeof(int16_t) * 2 * (size_t)n);
    gameFrames = malloc(sizeof(int16_t) * 2 * (size_t)n);
    ours = malloc(sizeof(float) * n);
    game = malloc(sizeof(float) * n);
    if (render == NULL || gameFrames == NULL || ours == NULL || game == NULL) {
        printf("out of memory\n");
        free(render);
        free(gameFrames);
        free(ours);
        free(game);
        bmxMusicAdpcmDestroy(adpcm);
        bmxMusicFlacClose(flac);
        return 0;
    }
    began = SDL_GetTicksNS();
    bmxMusicRender(e, pieces, count, &fs, &gs, 0, n, render, &clipped);
    began = SDL_GetTicksNS() - began;
    gs.fetch(gs.user, 0, n, gameFrames);
    for (uint32_t i = 0; i < n; i++) {
        ours[i] = (render[2 * i] + render[2 * i + 1]) / 65536.0f;
        game[i] = gameFrames[2 * i] / 32768.0f;
        rmsOurs += (double)ours[i] * ours[i];
        rmsGame += (double)game[i] * game[i];
    }
    for (uint32_t t = gMaxLag; t + kWindow + gMaxLag <= n; t += kWindow) {
        double best = -2;
        int bestLag = 0;

        for (int lag = -gMaxLag; lag <= gMaxLag; lag++) {
            const double c = ncc(game + t, ours + t + lag, kWindow);

            if (c > best) {
                best = c;
                bestLag = lag;
            }
        }
        windows++;
        sum += best;
        good += best >= 0.9;
        if (flacOnly(pieces, count, t - gMaxLag, kWindow + 2 * gMaxLag)) {
            flacWindows++;
            flacSum += best;
            lagSum += abs(bestLag);
            if (best < worst) {
                worst = best;
                worstAt = t / 44100.0;
            }
        }
    }
    printf(
        "%d %-42s corr lag0 %.4f | 1 s windows: mean %.4f, %u/%u >= 0.9 | FLAC-only windows: %u, mean "
        "%.4f, worst %.4f at %.0f s, mean |lag| %.1f | level %+.2f dB | %d FLAC pieces of %d | clipped %llu "
        "| render %.0f ms\n",
        e->index, e->title, ncc(game, ours, n), sum / windows, good, windows, flacWindows,
        flacWindows ? flacSum / flacWindows : 0.0, worst, worstAt, flacWindows ? lagSum / flacWindows : 0.0,
        10 * log10(rmsOurs / rmsGame), flacPieces, count, (unsigned long long)clipped, began / 1e6);
    if (wavDir != NULL) {
        char out[1100];

        SDL_snprintf(out, sizeof out, "%s/track%d.wav", wavDir, e->index);
        writeWav(out, render, n);
    }
    free(render);
    free(gameFrames);
    free(ours);
    free(game);
    bmxMusicAdpcmDestroy(adpcm);
    bmxMusicFlacClose(flac);
    return 1;
}

int main(int argc, char **argv)
{
    const char *gameDir = NULL, *musicDir = NULL, *wavDir = NULL;
    int only = -1, done = 0;
    char zsdPath[1024];
    BmxMusicZsdEntry zsd[64];
    int zsdCount;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--wav") == 0 && i + 1 < argc) {
            wavDir = argv[++i];
        } else if (strcmp(argv[i], "--lag") == 0 && i + 1 < argc) {
            gMaxLag = (int)strtol(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--track") == 0 && i + 1 < argc) {
            only = (int)strtol(argv[++i], NULL, 10);
        } else if (gameDir == NULL) {
            gameDir = argv[i];
        } else {
            musicDir = argv[i];
        }
    }
    if (gameDir == NULL || musicDir == NULL) {
        printf("usage: bmxmusictool <game folder> <FLAC folder> [--track N] [--lag N] [--wav <dir>]\n");
        return 2;
    }
    SDL_snprintf(zsdPath, sizeof zsdPath, "%s/bin/audio/MUSIC.ZSD", gameDir);
    zsdCount = bmxMusicZsdRead(zsdPath, zsd, 64);
    if (zsdCount < (int)bmxMusicEditCount) {
        printf("cannot read %s\n", zsdPath);
        return 1;
    }
    for (uint32_t i = 0; i < bmxMusicEditCount; i++) {
        const BmxMusicEdit *const e = &bmxMusicEdits[i];

        if (only < 0 || only == e->index) {
            done += compareTrack(e, zsdPath, &zsd[e->index], musicDir, wavDir);
        }
    }
    printf("%d tracks compared\n", done);
    return done == 0;
}
