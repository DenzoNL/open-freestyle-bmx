# Music: the game's edits of its ten tracks

`data/music-edits.json` describes how each of the game's music tracks (`bin\audio\music.zsd`, 4-bit IMA
ADPCM, mono, 44.1 kHz; decoder: `winstrmDecodeAdpcm` in src/bmx/pc/zge/sound/winstrm.c) was cut from the
CD recording. It holds timings only, no audio. The music override ("Override" below, opt-in) rebuilds each
game edit from the user's own CD-quality files.

Measured 2026-10-31 by cross-correlating the decoded game tracks with these FLAC releases (the user's copies;
other masters of the same songs may need new offsets):

| # (sample file) | Track | Release |
|---|---|---|
| 0 | 59 Times the Pain - Got It All in Sight | End of the Millennium (1999), track 9 |
| 1 | Cypress Hill - Dust | Skull & Bones (2000), CD 2 track 5 |
| 2 | Deftones - Be Quiet and Drive (Far Away) | Around the Fur (1997), track 6 |
| 3 | Dropkick Murphys - Never Alone | Do or Die (1998), track 4 |
| 4 | Pennywise - Greed | Straight Ahead (1999), track 1 |
| 5 | Rancid - Maxwell Murder | ...And Out Come the Wolves (1995), track 1 |
| 6 | Social Distortion - Don't Drag Me Down | White Light White Heat White Trash (1996), track 2 |
| 7 | Sublime - What I Got | Sublime (1996), track 2 |
| 8 | Swingin' Utters - Stupid Lullabies | The Sounds Wrong E.P. (1995), track 2 |
| 9 | Primer 55 - Loose | Introduction to Mayhem (2000), track 1 |

The sample-file index equals the order of the credits strings in `assets/miscdata.zal` (checked by
correlation). The game picks a level's first track from `gBmxLevelTracks` (bmxsound.c) and then advances.

## Format

One entry per track: `track_index`, `title`, `flac_file`, `flac_samples`, `game_samples`, `sample_rate`
(44100), `segments`, `unmatched_game_ranges`, `fade_out`, `gain_linear` / `gain_db` (track level, fades
excluded), `match`, `confidence`. Every position is in samples at 44.1 kHz. A segment maps
`[game_start, game_end)` to `[flac_start, flac_end)` with `speed`, a per-segment `gain` fit (includes fade
regions), `corr` and `snr_db` (~19-23 dB is the ADPCM noise floor, i.e. the same master).

## Findings

- Every track is a custom edit (1-8 splices; Primer 55 ~23 small edits), not a straight excerpt; most run
  2:00-2:15. Deftones drops the first 24 s and a 61 s middle section; Rancid (1:26 on CD) is lengthened to
  2:12 by repeating and reordering parts.
- The game is 0.6-4.7 dB quieter (per-track normalisation). Sublime has extra level processing (gain drifts
  1.65-1.28 against loudness, SNR ~14 dB). Cypress Hill plays ~0.5% slow (about 9 cents flat) with some
  wander, so it aligns to roughly +-10 ms only.
- Low-confidence pieces (Primer 55 around game 60-62 s and 79.8-81.2 / 100.2-104.5 s; Cypress Hill splice
  points) are marked per segment; crossfades at splices are not modelled.
- Tracks 0 and 3 start 0.115 s / 0.043 s before the FLAC's first sample (lead-in the CD lacks).

## Override

Opt-in, native exe only: with `[audio] music_dir = <folder>` in bmxnative.ini the game's music plays from the
user's own FLAC files, each track cut with the game's edit from this list, in CD quality and stereo, instead of the
4-bit ADPCM mono stream. Unset (the default) is the original. Code: `src/platform/music.c` (no game state: plan,
renderer, sources, file matching, render thread), `src/platform/musicgame.c` (ties it to the game's stream),
the seam in `src/platform/dsound.c` (`BmxSoundOverride`, sound.h); the edit list is compiled in by
`tools/gen_music_edits.py` (build step) from `data/music-edits.json`.

### The seam: the music buffer plays the override's frames

The game streams the music through winstrm.c: bmxsound.c creates the stream (flags 0x44: 44.1 kHz ADPCM mono, a
256 KB ring), winstrm decodes the ADPCM into a looping 16-bit mono DirectSound buffer, our dsound.c stand-in mixes
that buffer. A mono buffer cannot carry stereo, so the override does not replace the decode. It replaces what the
mixer reads for that buffer, and nothing else:

- `BmxSoundOverride` (sound.h) is a registration in the DirectSound stand-in. At every Play from stopped,
  `attach(buffer, pos)` may claim the play; the mixer then feeds `read(token, pos, n)` frames (16-bit stereo at
  the buffer's frequency) into a second, stereo resampler instead of the ring's frames, for the same frame range.
  Cursors, status, the end of a non-looping play, the lock rules and every value the game reads stay exactly as
  without the override (`BMX_SOUND_CHECK` stays clean). The game still decodes and writes its ADPCM; it is not
  heard. Volume, pan and the focus mute are the buffer's, applied to the override's frames as to the ring's.
- musicgame.c's `attach` claims a play when the buffer is the music stream's (`gBmxMusicStream->buffer`) and
  names the track by the sample file the stream reads (its source's offset and size, matched against MUSIC.ZSD's
  sample file table, which music.c reads itself). bmxsound.c and winstrm.c stay as ported: no jump, no option
  check. The hook DLL and the oracle tests never see any of it.
- "An override stream alongside, slaved to the game's buffer" is this, without a second set of state: the
  game's own buffer is the slave, so start / stop / pause / volume / pan / focus / release cannot drift apart.
- **Position.** The track position is the buffer's frame count since its last SetCurrentPosition (that offset
  included), counted across Stop / Play and the ring's wrap: dsound.c keeps the track position of the play's
  base frame (`trackBase`) and, at Stop, of the play cursor (`trackStop`, unwrapped: the 30 ms between the play
  and the write cursor are fed again after a resume, as the ring's are). winstrm starts every track the same
  way: winstrmReplaceSource stops the buffer and refills the ring from the source's first byte at offset 0, then
  winstrmStart does Stop, SetCurrentPosition(0), Play. So buffer frame n of a play from 0 is game sample n of the
  track (ADPCM: 2 samples per byte, no headers), and the override renders game sample positions.

### Behaviour kept

All of it is the game's, because the override only follows the buffer:

- **Track selection** (`gBmxLevelTracks[level]`, then +1 with wrap when looping, or the same track again; event
  1005): bmxSoundUpdate picks and calls winstrmReplaceSource / winstrmStart as before; attach just reads which
  sample file the stream now has.
- **Music volume slider** and the music-mode fade (event 1001: winstrmFade): SetVolume on the buffer, with
  DirectSound's gain law as for the ring.
- **Pause / resume** (winstrmPause): Stop / Play; the override resumes at the play cursor's track position (log:
  `music: track 7 (...) resumes at 37419 ms`).
- **Track changes and level exit**: a new track is ReplaceSource + Start (attach at 0); a change while playing goes
  through a short Play at the old position before Start's Stop / SetCurrentPosition(0) / Play, the same window
  in which the original plays the new ring from the old cursor. Level exit stops and releases the buffer.
- **End of a track**: the edit is exactly `game_samples` long; past it the override plays silence, while the
  game's data runs dry, winstrm drains and stops the stream when the cursor passes the end, and bmxSoundUpdate
  starts the next track. The timing of the end is the game's own.

### Edit rendering

`bmxMusicPlan` turns an edit into pieces covering `[0, game_samples)`; `bmxMusicRender` renders any range of them.

- **Splices**: raised-cosine crossfades of 2 x 128 frames (5.8 ms) centred on every boundary, narrower when a
  piece is shorter than 256 frames (half of the shorter neighbour). The FLAC side reads on past its segment's end
  (or before its start), so a crossfade blends the recording's own continuation. Overlapping segments (Sublime
  158426..158678) are split in the middle. Unit-tested: pure on both sides, halfway at the splice, monotonic,
  largest step 12000 * pi / 512 for a 12000 jump (no click).
- **Fades**: the edit's `fade_out` (linear from `start_game` to `end_game` down to `final_ratio`, then that ratio)
  on the FLAC pieces; the game's own pieces are already faded.
- **Gain**: the edit list's gains are least-squares fits, i.e. the level ratio times the correlation; the
  generator divides the correlation out (otherwise Cypress Hill, correlated at ~0.8, plays 2 dB quiet: measured
  -2.03 dB before, +0.01 dB after). The track level is the duration-weighted mean over segments correlated above
  0.97 (else the median), as `gain_linear` was computed. A segment uses its own level when it is more than 1 dB off
  the track's and outside the fade out (Sublime's first 11 s at +1.8 dB: the game's extra level processing;
  Primer 55's reused intro pieces at +4.4 dB and a piece at 38.5 s at +2.0 dB). Above -1 dB a soft clip (tanh
  knee) keeps the sum from wrapping: Sublime goes over full scale on 66751 of 11.5 M samples (0.6 %; its level is
  +2.6 dB over the CD), Primer 55 on 116, the rest never. Result: every track's level within 0.05 dB of the
  game's (table below).
- **Speed**: segments within 0.1% of speed 1 (all but Cypress Hill: the game runs ~14 ppm off the CD) play the
  CD's own samples, bit exact at gain 1, with the offset centred on the segment (at most 39 frames, 0.9 ms, off at
  the segment's ends, which the crossfade hides). **Cypress Hill (speed 0.995) is resampled to match the game**
  (16-tap Blackman-windowed sinc, 512 interpolated phases; a 1 kHz and a 2 kHz sine come out within 4 LSB): the
  edit's splices sit on the game's timeline, so at CD speed every segment would run 0.5% short (the long one by
  0.44 s) and each splice would jump in the beat, or the track would end 0.6 s early against the game's data; and
  the 9 cents flat are the version players know. The ~10 ms wander of the game's copy is not followed (the
  unknown source of the warp would have to be modelled).
- **Low-confidence segments** (Sublime's 161-frame piece at 35.7 s, Primer 55's pieces at 60.1-61.7 s and
  79.8-81.2 s: five pieces, 3.0 s in all) play the game's own audio (mono, 4-bit), crossfaded: the mapping is not
  trusted, the game's data is the exact content. Medium and high play the FLAC.
- **Lead-ins and gaps**: everything the FLAC does not cover plays the game's own ADPCM, decoded by music.c (the
  same IMA decoder as winstrmDecodeAdpcm, random access through decoder checkpoints every 32768 samples) from
  MUSIC.ZSD: track 0's first 5512 samples (0.125 s, the CD starts 0.115 s into the game's track), track 3's first
  ~2000 (its FLAC starts at -1894; the offset is centred, plus the crossfade's reach), the unmatched tails after
  the fade outs, Primer 55's 0.6 s gap at 61.7 s. Where MUSIC.ZSD cannot be read these parts are silent.

### Files

`[audio] music_dir` (a UTF-8 path; the config's line limit was raised to 512 bytes for it). Per track: the edit's
`flac_file` in that folder; else a `*.flac` whose name, reduced to letters and digits without case, equals the
title so reduced (`Swingin' Utters - Stupid Lullabies.flac` for the edit's `Swingin’ ...`); else one whose
name contains the song's name (`03 Stupid Lullabies.flac`). The file must be 44.1 kHz, 1 or 2 channels and exactly
`flac_samples` frames long (any other length is another release or rip, whose offsets differ): otherwise that
track plays the game's music, and the log says why. Each start and resume is logged (`music: track N (...)
starts at 0 ms from <file>`, or `... : the game's music`).

FLAC decoding: [dr_flac](https://github.com/mackron/dr_libs) v0.13.3 (public domain or MIT-0), vendored unmodified
in `third_party/dr_flac` (dr_flac.h at commit 69d777c482775858e8ea8a7b047c9bcd451febc8, sha256
a35468f278f17cf9538ce189debf76a5854a77e139980f7b5185d67566a0c837; license and pin in `third_party/dr_flac/LICENSE`),
built in its own unit without warnings, clang-format or clang-tidy, with `DR_FLAC_NO_STDIO` (files through
SDL_IOStream, for UTF-8 paths) and `DR_FLAC_NO_OGG`. Native only. Pinned to the established v0.13.3 (2026-01) rather than
v0.13.4 (2026-10-30, two days old when vendored; policy: no releases younger than ~2 weeks). Later versions
fix seeking edge cases (the renderer seeks at every splice); with v0.13.3 the offline comparison below gives
the same numbers for all ten tracks as v0.13.4 did.

### Decoding ahead

The mixer runs every 10 ms and must never wait. A whole track as 16-bit stereo is ~23 MB (float would be ~45 MB),
so: when the override opens, a render thread renders the first 2 s of every track (3.5 MB in all); when a track
starts, the thread renders the whole track from its start into one buffer (108-227 ms per track offline, 138-312 ms
in the game on this machine), publishing its progress every 4096 frames. The mixer copies from the head or from the
render, whichever has the frames, under a short lock, and plays silence for frames that are not there yet (counted:
`frames not ready` at exit). One whole render exists at a time: a track change frees the previous one. In the runs
below, 0 frames were not ready.

### Verification

- `bmxplatformtest music` (tests/platform/test_music.c, 52 checks, synthetic audio only: a small FLAC writer with
  VERBATIM subframes makes the test files): the FLAC source against the generator (seeks forward and back, the
  resampler's overlapping reads, both ends, mono); ADPCM random access against a sequential decode; the plan
  (lead-in split, overlap split, gap + low confidence as one game piece, gains, crossfade widths, a short piece,
  speed 0.995); the render (game pieces exact, FLAC pieces at gain 0.5 and bit exact at 1, the fade, the crossfade
  without a click, the resampler on two sines, the soft clip and the over-full-scale count); the position the
  stand-in hands an override (contiguous from 0, the override's stereo plays instead of the ring, the volume,
  resume at the play cursor, past the ring's wrap, SetCurrentPosition(0) and while playing, a declined attach
  plays the ring); the service (exact / by-title / by-song-name matches, a missing file and a wrong length fall
  back, track by sample file, reads equal the renderer's output, past the end silence, the head after a track
  change, a missing render counted); the config's long UTF-8 path.
- `bmxmusictool <game folder> <FLAC folder> [--track N] [--lag N] [--wav <dir>]` (tests/platform/musictool.c)
  renders every track with the same renderer and compares the mono downmix with the game's decoded ADPCM track:
  normalised correlation in 1 s windows at the best lag within +-2 ms (the CD plays at its own speed), level, the
  samples over full scale. 2026-10-31, the user's FLACs:

| # | Track | corr, 1 s windows (mean) | windows >= 0.9 | worst FLAC-only window | level | over full scale | render |
|---|---|---|---|---|---|---|---|
| 0 | 59 Times the Pain | 0.983 | 128/135 | 0.71 (121 s, a splice) | +0.00 dB | 0 | 133 ms |
| 1 | Cypress Hill | 0.711 (0.798 within +-15 ms) | 0/120 | 0.07 | +0.01 dB | 0 | 227 ms |
| 2 | Deftones | 0.992 | 129/129 | 0.91 | -0.05 dB | 0 | 127 ms |
| 3 | Dropkick Murphys | 0.992 | 130/131 | 0.79 | -0.00 dB | 0 | 117 ms |
| 4 | Pennywise | 0.993 | 130/131 | 0.86 | -0.00 dB | 0 | 127 ms |
| 5 | Rancid | 0.993 | 129/131 | 0.84 | -0.00 dB | 0 | 119 ms |
| 6 | Social Distortion | 0.992 | 125/127 | 0.73 | +0.00 dB | 0 | 109 ms |
| 7 | Sublime | 0.981 | 128/130 | 0.86 | -0.03 dB | 66751 | 136 ms |
| 8 | Swingin' Utters | 0.994 | 128/129 | 0.88 | +0.01 dB | 0 | 108 ms |
| 9 | Primer 55 | 0.982 | 119/124 | 0.66 | -0.01 dB | 116 | 114 ms |

  Eight tracks at 0.98-0.99 (the ADPCM noise floor is ~0.99). Cypress Hill is the known caveat: its time warp keeps
  the game's copy within ~10 ms of a straight line only (0.80 with a +-15 ms search; the edit list's own fit has
  0.79). Primer 55's low windows are around its low-confidence pieces. The single low windows of the others sit on
  splices.
- Scripted runs from native-run2 (`BMX_MUTE=1`, `BMX_SOUND_CHECK=1`, `BMX_KEYS`, the option set): ProQuest into Vail
  Trails (track 7 overridden, 61 s); ProQuest, pause and continue (resume at 37.4 s), end run, exit to the main
  menu, Freeride into another level (track 1), riding past the end of track 1 into track 2 (338 s, 10.35 M frames
  from the renders, 0 not ready); a folder with only Cypress Hill (track 7 falls back to the game's music, the
  override stays registered). No trap, `soundcheck: RESULT clean`, 0 resampler underruns in each.
- The listening test is the user's: `musictest.ps1` in native-run2 (`-Original` for the game's music to compare).
