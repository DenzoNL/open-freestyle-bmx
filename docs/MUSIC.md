# Music: the game's edits of its ten tracks

`data/music-edits.json` describes how each of the game's music tracks (`bin\audio\music.zsd`, 4-bit IMA
ADPCM, mono, 44.1 kHz; decoder: `winstrmDecodeAdpcm` in src/bmx/pc/zge/sound/winstrm.c) was cut from the
CD recording. It holds timings only, no audio. A later music override (PLAN.md backlog: higher-quality
audio, custom music) can rebuild each game edit from the user's own CD-quality files.

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
