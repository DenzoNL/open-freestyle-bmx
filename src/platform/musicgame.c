/*
 * The music override tied to the game (music.h, docs/MUSIC.md "Override"): native exe only. The seam is the
 * DirectSound stand-in (sound.h, BmxSoundOverride), so bmxsound.c and winstrm.c stay as ported, without an
 * option check: when a buffer starts playing, attach() looks whether it is the music stream's buffer
 * (gBmxMusicStream) and which of MUSIC.ZSD's sample files that stream reads (its source's file offset and
 * size); if that track has a FLAC, the mixer plays the override's render of it at the buffer's own track
 * position, with the buffer's volume (the music slider, the fades), pan and focus, and the game's ADPCM data
 * is not heard. Everything the game sees (cursors, status, the end of the data) stays the game's own.
 *
 * Compiled with BMX_HOOK=1 (the game's globals at their original addresses, like smoothmix.c).
 */
#include <stddef.h>

#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/pc/zge/sound/winstrm.h"
#include "platform/music.h"
#include "platform/platform.h"
#include "platform/sound.h"

static int attach(const BmxDsBuffer *buffer, uint64_t pos)
{
    const WinstrmStream *const s = gBmxMusicStream;
    int track;

    if (s == NULL || (const void *)s->buffer != (const void *)buffer || s->node.source == NULL) {
        return -1;
    }
    track = bmxMusicTrackOfSample(s->node.source->startOffset, s->node.source->size);
    if (track < 0) {
        return -1;
    }
    return bmxMusicAttach(track, pos) ? track : -1;
}

static const BmxSoundOverride kOverride = {attach, bmxMusicRead, bmxMusicClose};

void bmxMusicInstall(void)
{
    const char *const dir = bmxConfigString("audio.music_dir");

    if (dir == NULL) {
        return;
    }
    if (bmxMusicOpen(bmxMusicEdits, bmxMusicEditCount, dir, bmxPathResolve("bin\\audio\\MUSIC.ZSD")) > 0) {
        bmxSoundSetOverride(&kOverride);
    } else {
        bmxMusicClose();
    }
}
