/*
 * The music override (docs/MUSIC.md "Override"): native exe only, opt-in with `[audio] music_dir = <folder>`.
 * Each of the game's ten music tracks is rebuilt from the user's own FLAC files with the game's edit
 * (data/music-edits.json, compiled into bmxMusicEdits) in CD quality and stereo, and played in place of the
 * game's 4-bit ADPCM mono stream through the DirectSound stand-in (sound.h: BmxSoundOverride).
 *
 * music.c is the part without game state (edit plan, renderer, FLAC and ADPCM sources, file matching, the
 * render thread and its cache); musicgame.c ties it to the game's music stream.
 */
#ifndef BMX_PLATFORM_MUSIC_H
#define BMX_PLATFORM_MUSIC_H

#include <stddef.h>
#include <stdint.h>

/* ---- the edit list (generated from data/music-edits.json by tools/gen_music_edits.py) ---- */

enum { kBmxMusicLow, kBmxMusicMedium, kBmxMusicHigh };

/* Game samples [gameStart, gameEnd) come from FLAC frames [flacStart, flacEnd) (44.1 kHz). */
typedef struct BmxMusicSegment {
    uint32_t gameStart, gameEnd;
    int32_t flacStart, flacEnd; /* flacStart < 0: the game has a lead-in the CD lacks */
    float gain;                 /* fitted on this segment (fade regions included) */
    int confidence;             /* kBmxMusic* */
} BmxMusicSegment;

typedef struct BmxMusicEdit {
    int index; /* sample file index in MUSIC.ZSD */
    const char *title, *flacFile;
    uint32_t flacSamples, gameSamples;
    float gain;                  /* the track's level against the CD, fades excluded */
    uint32_t fadeStart, fadeEnd; /* linear fade out to fadeFinal (fadeEnd 0: none) */
    float fadeFinal;
    const BmxMusicSegment *segments;
    uint32_t segmentCount;
} BmxMusicEdit;

extern const BmxMusicEdit bmxMusicEdits[];
extern const uint32_t bmxMusicEditCount;

/* ---- the plan: the track as pieces, spliced with short crossfades ---- */

enum {
    kBmxMusicFlac = 0, /* from the FLAC (at gain, with the fade out) */
    kBmxMusicGame = 1, /* the game's own ADPCM audio (lead-ins, gaps, low-confidence pieces) */
    kBmxMusicMaxPieces = 96,
    kBmxMusicXfade = 128,  /* half the crossfade at a splice, frames (2 x 128 = 5.8 ms) */
    kBmxMusicSincHalf = 8, /* taps on each side of the resampler (a speed other than 1) */
    kBmxMusicSampleRate = 44100,
};

typedef struct BmxMusicPiece {
    uint32_t start, end; /* game samples */
    int kind;
    /* FLAC: the frame for game sample t is t + offset (rate 0) or origin + (t - start) * rate. */
    int32_t offset;
    double origin, rate;
    float gain;
    uint32_t xfadeIn, xfadeOut; /* half widths of the crossfades at start and end (0 at the track's ends) */
} BmxMusicPiece;

/*
 * Builds the plan of an edit for a FLAC of `flacFrames` frames: segments become FLAC pieces (low confidence
 * ones game pieces), overlaps are split in the middle, gaps and the parts outside the FLAC (including the
 * crossfade's reach) become game pieces. Speeds within 0.1% of 1 play the CD's own samples (rate 0, the
 * offset centred on the segment); others are resampled. Returns the number of pieces.
 */
int bmxMusicPlan(const BmxMusicEdit *edit, uint64_t flacFrames, BmxMusicPiece *out, int max);

/* The gain of a segment's piece: the track's gain, or the segment's own when it differs by more than 1 dB
 * and the segment is outside the fade out (level processing in the game's edit). */
float bmxMusicSegmentGain(const BmxMusicEdit *edit, const BmxMusicSegment *segment);

/* ---- sources: 16-bit stereo frames by position, zeros outside ---- */

typedef struct BmxMusicSource {
    void *user;
    void (*fetch)(void *user, int64_t pos, uint32_t frames, int16_t *stereo);
} BmxMusicSource;

/* A FLAC file (dr_flac, third_party/dr_flac): NULL when it cannot be opened or decoded. Mono plays on both
 * sides. */
typedef struct BmxMusicFlac BmxMusicFlac;
BmxMusicFlac *bmxMusicFlacOpen(const char *path);
void bmxMusicFlacClose(BmxMusicFlac *flac);
uint64_t bmxMusicFlacFrames(const BmxMusicFlac *flac);
uint32_t bmxMusicFlacRate(const BmxMusicFlac *flac);
uint32_t bmxMusicFlacChannels(const BmxMusicFlac *flac);
BmxMusicSource bmxMusicFlacSource(BmxMusicFlac *flac);

/* The game's 4-bit IMA ADPCM track (winstrmDecodeAdpcm), random access through decoder checkpoints. The bytes
 * are copied. */
typedef struct BmxMusicAdpcm BmxMusicAdpcm;
BmxMusicAdpcm *bmxMusicAdpcmCreate(const uint8_t *bytes, uint32_t size);
void bmxMusicAdpcmDestroy(BmxMusicAdpcm *adpcm);
BmxMusicSource bmxMusicAdpcmSource(BmxMusicAdpcm *adpcm);

/* ---- the renderer ---- */

/*
 * Renders game samples [t0, t0 + frames) of the plan as 16-bit stereo: FLAC pieces at their gain (and the
 * edit's fade out), game pieces as they are, raised-cosine crossfades at the splices, a soft clip above
 * -1 dB. `game` may be NULL (silence instead). Adds the samples that went over full scale before the soft
 * clip to *clipped (may be NULL).
 */
void bmxMusicRender(const BmxMusicEdit *edit, const BmxMusicPiece *pieces, int count, BmxMusicSource *flac,
                    BmxMusicSource *game, uint32_t t0, uint32_t frames, int16_t *out, uint64_t *clipped);

/* The soft clip (linear up to -1 dB, then towards full scale). */
float bmxMusicSoftClip(float x);

/* ---- files ---- */

/* The sample files of a .zsd (offset and size of each in the file): count, or -1. */
typedef struct BmxMusicZsdEntry {
    uint32_t offset, size;
} BmxMusicZsdEntry;
int bmxMusicZsdRead(const char *path, BmxMusicZsdEntry *out, int max);

/*
 * The FLAC for an edit in `dir`: the edit's flac_file, else a .flac whose name matches the title (letters and
 * digits only, case-insensitive), else one whose name contains the song's name (the title after " - ").
 * Returns 1 exact, 2 by title, 3 by song name, 0 none; the path in out.
 */
int bmxMusicFindFile(const char *dir, const BmxMusicEdit *edit, char *out, size_t size);

/* ---- the service (render thread, cache, the mixer's reads) ---- */

/*
 * Opens the override for `edits`: finds and checks each track's FLAC in `dir` (44.1 kHz, 1 or 2 channels,
 * exactly the edit's frame count), reads the game's ADPCM tracks from `zsdPath` (may be NULL: game pieces
 * silent) and starts the render thread, which renders the first seconds of every track at once and then
 * whole tracks on demand. Logs per track what plays. Returns the number of tracks overridden.
 */
int bmxMusicOpen(const BmxMusicEdit *edits, uint32_t count, const char *dir, const char *zsdPath);
/* Stops the thread and frees everything; logs the totals. */
void bmxMusicClose(void);
/* The track the game's sample file at (offset, size) of MUSIC.ZSD is, or -1. */
int bmxMusicTrackOfSample(uint32_t offset, uint32_t size);
/* The game starts playing `track` at game sample `pos` (0: from its start, else a resume): 1 when it is
 * overridden (and asks for its render), 0 when the game's own data plays. Logs it. Never blocks. */
int bmxMusicAttach(int track, uint64_t pos);
/* The mixer: `frames` stereo frames of `track` from game sample `pos`; silence past the end, and where the
 * render is not ready yet (counted). Never waits for the render thread. */
void bmxMusicRead(int track, uint64_t pos, uint32_t frames, int16_t *out);
/* Tests: blocks until the render thread has rendered the whole of `track` (0 on failure). */
int bmxMusicWaitRendered(int track, uint32_t timeoutMs);
/* Frames read before the render was ready (silence played instead). */
uint64_t bmxMusicMisses(void);

/* musicgame.c: reads [audio] music_dir and, when set, opens the override for the game's music stream. */
void bmxMusicInstall(void);

#endif
