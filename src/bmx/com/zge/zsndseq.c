/* Sound sequencer (C:\zaxis\Bmx\src\com\zge\zsndseq.c). */
#include "bmx/com/zge/zsndseq.h"

#include "bmx/pc/zge/sound/winchan.h"
#include "bmx/pc/zge/sound/winload.h"
#include "compat/crt.h"

static const char zsndSeqFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\zsndseq.c";

/* Handlers are stored into events as the exe's own entry points in the hook build. */
#define HANDLER(addr, fn) BMX_FNPTR(ZsndSeqHandler, addr, fn)

/* Track of a handle, with the original's two asserts (the lines differ per caller). */
static ZsndSeqTrack *trackOf(uint8_t hTrack, unsigned handleLine, const char *expr, unsigned trackLine)
{
    ZsndSeqTrack *track;

    if (hTrack >= kzsndSeqMaxTracks) {
        _assert("hTrack < kzsndSeqMaxTracks", zsndSeqFile, handleLine);
    }
    track = gZsndTracks[hTrack];
    if (track == 0) {
        _assert(expr, zsndSeqFile, trackLine);
    }
    return track;
}

/* Volume of a note on its channel: velocity, track volume and expression, each scaled by 1/128. */
static uint8_t noteVolume(const ZsndSeqTrack *track, const ZsndNote *note)
{
    const uint32_t v = ((uint32_t)track->volume * note->velocity >> 7 & 0xff) * track->expression;

    return (uint8_t)(v >> 7);
}

BMX_ADDR(0x0047c4c0)
int zsndSeqReset(void)
{
    int i;

    for (i = 0; i < kzsndSeqMaxTracks; i++) {
        gZsndTracks[i] = 0;
    }
    for (i = 0; i < kzsndSeqMaxTracks; i++) {
        gZsndTrackPool[i].flags &= (uint16_t)~kzsndSeqFlagUsed;
    }
    gZsndNextTrack = 0;
    for (i = 0; i < kzsndSeqMaxNotes; i++) {
        gZsndNotes[i].flags = 0;
    }
    gZsndNextNote = 0;
    gZsndMuteAll = 0;
    return 0;
}

BMX_ADDR(0x0047c530)
int zsndSeqStopAll(void)
{
    uint32_t h;

    for (h = 0; h < kzsndSeqMaxTracks; h++) {
        if (gZsndTracks[h] != 0) {
            zsndSeqDestroy(h);
        }
    }
    return 0;
}

BMX_ADDR(0x0047c580)
int zsndSeqUpdate(void)
{
    /* Declared outside the loops: an event the decoder leaves unset keeps the previous handler. */
    ZsndSeqEvent event = {0};
    uint32_t h;

    if (gZsndSeqLock != 0) {
        return 0;
    }
    for (h = 0; h < kzsndSeqMaxTracks; h++) {
        ZsndSeqTrack *track = gZsndTracks[h];
        ZsndNote *note;
        uint32_t elapsed, wait;

        if (track == 0) {
            continue;
        }
        if (track->notes != 0) {
            track->notes->remaining--;
        }
        for (note = track->notes; note != 0 && note->remaining == 0; note = note->next) {
            zsndSeqReleaseNote(note);
        }
        track->notes = note;
        if (zsndSeqIsRunning(track) == 0) {
            continue;
        }
        elapsed = track->step;
        wait = track->wait;
        if (elapsed >= wait) {
            for (;;) {
                elapsed -= track->wait;
                zsndSeqDecodeEvent(track, &event);
                if (event.handler != 0) {
                    event.handler(&event);
                }
                if (zsndSeqIsRunning(track) == 0) {
                    if ((track->flags & kzsndSeqFlagOneShot) != 0) {
                        zsndSeqDestroy(h);
                    }
                    break;
                }
                wait = zsndSeqPeekDelta(track) << 12;
                track->wait = wait;
                if (elapsed < wait) {
                    break;
                }
            }
            wait = track->wait;
        }
        track->wait = wait - elapsed;
    }
    return 0;
}

BMX_ADDR(0x0047c6b0)
uint8_t zsndSeqCreate(const ZsndSeqHeader *header, uint16_t flags)
{
    int slot = gZsndNextTrack;
    ZsndSeqTrack *track;
    uint32_t rate;

    gZsndSeqLock++;
    if ((gZsndTrackPool[slot].flags & kzsndSeqFlagUsed) != 0) {
        const int first = slot;

        do {
            slot++;
            if (slot == kzsndSeqMaxTracks) {
                slot = 0;
            }
            if (slot == first) {
                gZsndSeqLock--;
                return kzsndSeqNoTrack;
            }
        } while ((gZsndTrackPool[slot].flags & kzsndSeqFlagUsed) != 0);
    }
    track = &gZsndTrackPool[slot];
    gZsndNextTrack = slot + 1;
    track->flags = kzsndSeqFlagUsed;
    if (slot + 1 == kzsndSeqMaxTracks) {
        gZsndNextTrack = 0;
    }
    track->flags |= flags & 0xfffc;
    track->state = kzsndSeqStateStopped;
    track->start = header->data + 4;
    track->cursor = header->data + 4;
    rate = gZsndTicksPerSec;
    track->step = 0x3e8000u / rate;
    track->noteScale = (rate << 12) / 1000;
    track->notes = 0;
    track->lastVelocity = 0x7f;
    track->volume = header->volume;
    track->expression = 0x7f;
    track->header = header;
    track->pan = header->pan;
    /* Both lookups use the bank the slot held before; it is replaced below. */
    track->voice = header->voice != 0xff ? winloadGetVoice(track->bank, header->voice) : 0;
    track->keymap = header->keymap != 0xff ? winloadGetKeymap(track->bank, header->keymap) : 0;
    track->bank = header->bank;
    gZsndTracks[slot] = track;
    gZsndSeqLock--;
    return (uint8_t)slot;
}

BMX_ADDR(0x0047c870)
int zsndSeqDestroy(uint8_t hTrack)
{
    ZsndSeqTrack *track;
    ZsndNote *note;

    gZsndSeqLock++;
    track = trackOf(hTrack, 0x143, "track", 0x145);
    for (note = track->notes; note != 0; note = note->next) {
        zsndSeqReleaseNote(note);
    }
    gZsndNextTrack = hTrack;
    gZsndTracks[hTrack] = 0;
    gZsndTrackPool[hTrack].flags = 0;
    gZsndSeqLock--;
    return 0;
}

BMX_ADDR(0x0047c940)
int zsndSeqStart(uint8_t hTrack)
{
    ZsndSeqTrack *track;
    ZsndNote *note;

    gZsndSeqLock++;
    track = trackOf(hTrack, 0x162, "track", 0x164);
    note = track->notes;
    if (track->state == kzsndSeqStatePaused) {
        for (; note != 0; note = note->next) {
            if ((note->flags & kzsndNoteEventFlagSilent) == 0) {
                winchanVoiceStart(note->channel);
            }
        }
    } else {
        for (; note != 0; note = note->next) {
            zsndSeqReleaseNote(note);
        }
        track->notes = 0;
        track->cursor = track->start;
        track->wait = zsndSeqPeekDelta(track) << 12;
    }
    track->state = kzsndSeqStatePlaying;
    gZsndSeqLock--;
    return 0;
}

BMX_ADDR(0x0047ca50)
int zsndSeqStop(uint8_t hTrack)
{
    ZsndSeqTrack *track;

    gZsndSeqLock++;
    track = trackOf(hTrack, 0x189, "track", 0x18b);
    if (zsndSeqIsRunning(track) != 0) {
        const uint16_t mode = track->flags & kzsndSeqModeMask;

        if (mode == 0) {
            ZsndNote *note;

            for (note = track->notes; note != 0; note = note->next) {
                zsndSeqReleaseNote(note);
            }
            track->notes = 0;
        } else if (mode != 1) {
            track->flags |= kzsndSeqFlagPending;
        }
    }
    track->state = kzsndSeqStateStopped;
    gZsndSeqLock--;
    return 0;
}

BMX_ADDR(0x0047cb30)
int zsndSeqPause(uint8_t hTrack)
{
    ZsndSeqTrack *track;

    gZsndSeqLock++;
    track = trackOf(hTrack, 0x1af, "track", 0x1b1);
    if (zsndSeqIsRunning(track) != 0) {
        const uint16_t mode = track->flags & kzsndSeqModeMask;

        if (mode == 0) {
            ZsndNote *note;

            for (note = track->notes; note != 0; note = note->next) {
                if ((note->flags & kzsndNoteEventFlagSilent) == 0) {
                    winchanVoiceStop(note->channel, 0);
                }
            }
        } else if (mode != 1) {
            track->flags |= kzsndSeqFlagPending;
        }
    }
    track->state = kzsndSeqStatePaused;
    gZsndSeqLock--;
    return 0;
}

BMX_ADDR(0x0047cc20)
int zsndSeqMute(uint8_t hTrack, int mute)
{
    ZsndSeqTrack *track;
    ZsndNote *note;

    gZsndSeqLock++;
    track = trackOf(hTrack, 0x1d5, "track", 0x1d7);
    if (mute != 0) {
        if (zsndSeqIsRunning(track) != 0) {
            const uint16_t mode = track->flags & kzsndSeqModeMask;

            if (mode == 0) {
                for (note = track->notes; note != 0; note = note->next) {
                    if ((note->flags & kzsndNoteEventFlagSilent) == 0) {
                        winchanVoiceStop(note->channel, 0);
                        winchanVoiceFree(note->channel);
                        note->flags |= kzsndNoteEventFlagSilent;
                    }
                }
            } else if (mode != 1) {
                track->flags |= kzsndSeqFlagPending | kzsndSeqFlagMute;
                gZsndSeqLock--;
                return 0;
            }
        }
        track->flags |= kzsndSeqFlagMute;
        gZsndSeqLock--;
        return 0;
    }
    if ((track->flags & kzsndSeqModeMask) < 2) {
        for (note = track->notes; note != 0; note = note->next) {
            if ((note->flags & kzsndNoteEventFlagSilent) != 0) {
                note->channel = winchanVoiceAlloc(note->voice, 0xff);
                if (note->channel != 0xff) {
                    winchanVoiceSetPitch(note->channel, note->cents);
                    winchanVoiceSetVolume(note->channel, noteVolume(track, note));
                    winchanVoiceSetPan(note->channel, (int8_t)track->pan);
                    winchanVoiceStart(note->channel);
                    note->flags &= (uint8_t)~kzsndNoteEventFlagSilent;
                }
            }
        }
    } else {
        track->flags |= kzsndSeqFlagPending;
    }
    track->flags &= (uint16_t)~kzsndSeqFlagMute;
    gZsndSeqLock--;
    return 0;
}

BMX_ADDR(0x0047ce30)
int zsndSeqSetGroupMute(uint8_t hTrack, int enable)
{
    ZsndSeqTrack *track = trackOf(hTrack, 0x214, "track", 0x216);

    if (enable == 0) {
        if ((track->flags & kzsndSeqModeMask) > 1) {
            track->flags |= kzsndSeqFlagPending;
        }
        track->flags &= (uint16_t)~kzsndSeqFlagGroupMute;
        gZsndMuteAll = 0;
        return 0;
    }
    if (zsndSeqIsRunning(track) != 0 && (track->flags & kzsndSeqModeMask) > 1) {
        track->flags |= kzsndSeqFlagPending;
    }
    track->flags |= kzsndSeqFlagGroupMute;
    gZsndMuteAll = 1;
    return 0;
}

BMX_ADDR(0x0047cf10)
int zsndSeqGetStatus(uint8_t hTrack)
{
    ZsndSeqTrack *track = trackOf(hTrack, 0x23e, "Track[hTrack]", 0x23f);

    if (zsndSeqIsRunning(track) != 0) {
        return kzsndSeqStatusRunning;
    }
    if (track->state == kzsndSeqStateStopped) {
        return kzsndSeqStatusReady;
    }
    if (track->state != kzsndSeqStatePaused) {
        return kzsndSeqStatusStopped;
    }
    return kzsndSeqStatusPaused;
}

BMX_ADDR(0x0047cfe0)
int zsndSeqSetMode(uint8_t hTrack, uint8_t mode)
{
    ZsndSeqTrack *track = trackOf(hTrack, 0x25a, "track", 0x25c);

    track->flags = (uint16_t)(track->flags ^ ((track->flags ^ mode) & kzsndSeqModeMask));
    if ((track->flags & kzsndSeqFlagPending) != 0) {
        if (track->state == kzsndSeqStateStopped) {
            zsndSeqStop(hTrack);
            return 0;
        }
        if (track->state == kzsndSeqStatePaused) {
            zsndSeqPause(hTrack);
            return 0;
        }
        if ((track->flags & kzsndSeqFlagMute) != 0) {
            zsndSeqMute(hTrack, 1);
            return 0;
        }
        if ((track->flags & kzsndSeqFlagGroupMute) != 0) {
            zsndSeqSetGroupMute(hTrack, 1);
        }
    }
    return 0;
}

BMX_ADDR(0x0047d0d0)
int zsndSeqSetTempo(uint8_t hTrack, float speed)
{
    ZsndSeqTrack *track = trackOf(hTrack, 0x27e, "track", 0x280);
    const uint32_t rate = gZsndTicksPerSec;

    track->step = (uint32_t)bmxFtol((double)speed * 4096000.0f / (double)(int32_t)rate);
    track->noteScale = (uint32_t)bmxFtol((double)rate * speed * 4.0960001945495605f);
    return 0;
}

BMX_ADDR(0x0047d1b0)
int zsndSeqSetVolume(uint8_t hTrack, uint8_t volume, uint8_t pan)
{
    ZsndSeqTrack *track = trackOf(hTrack, 0x29a, "track", 0x29c);
    ZsndNote *note;

    track->volume = volume;
    track->pan = pan;
    for (note = track->notes; note != 0; note = note->next) {
        if ((note->flags & kzsndNoteEventFlagSilent) == 0) {
            winchanVoiceSetVolume(note->channel, noteVolume(track, note));
            winchanVoiceSetPan(note->channel, (int8_t)track->pan);
        }
    }
    return 0;
}

BMX_ADDR(0x0047d290)
int zsndSeqSetLoop(uint8_t hTrack, int loop)
{
    ZsndSeqTrack *track = trackOf(hTrack, 0x2b3, "track", 0x2b5);

    if (loop != 0) {
        track->flags |= kzsndSeqFlagLoop;
        return 0;
    }
    track->flags &= (uint16_t)~kzsndSeqFlagLoop;
    return 0;
}

/* Both calls return 0, so the track is started and kept; the destroy only follows a failed start. */
BMX_ADDR(0x0047d320)
uint32_t zsndSeqPlayOnce(const ZsndSeqHeader *header, uint8_t volume, uint8_t pan)
{
    const uint8_t h = zsndSeqCreate(header, kzsndSeqFlagOneShot);

    if (h != kzsndSeqNoTrack) {
        if (zsndSeqSetVolume(h, volume, pan) == 0 && zsndSeqStart(h) == 0) {
            return 0;
        }
        zsndSeqDestroy(h);
    }
    return 0xff;
}

BMX_ADDR(0x0047d3a0)
int zsndSeqSetEndCallback(uint8_t hTrack, ZsndSeqEndFn callback, uint32_t arg)
{
    ZsndSeqTrack *track = trackOf(hTrack, 0x2e1, "track", 0x2e3);

    track->onEvent = callback;
    track->onEventArg = arg;
    return 0;
}

BMX_ADDR(0x0047d420)
uint32_t zsndSeqIsRunning(const ZsndSeqTrack *track)
{
    if (track->state != kzsndSeqStatePlaying) {
        return (track->flags & kzsndSeqFlagPending) >> 14;
    }
    return 1;
}

BMX_ADDR(0x0047d450)
uint32_t zsndSeqPeekDelta(const ZsndSeqTrack *track)
{
    const uint8_t kind = *track->cursor & 7;
    const uint8_t *p = track->cursor + 1;

    if (kind < 3) {
        return 0;
    }
    if (kind < 6) {
        return *p;
    }
    return (uint32_t)zsndSeqReadU32(p);
}

BMX_ADDR(0x0047d490)
int zsndSeqDecodeEvent(ZsndSeqTrack *track, ZsndSeqEvent *event)
{
    /* Operand bytes. The original decodes into the stack slot of its first argument, so bytes the
     * stream does not supply keep the bytes of the track pointer. */
    uint8_t args[4];
    const uint8_t *p = track->cursor;
    const uint8_t head = *p;
    uint8_t type = head & 0xf8;
    uint8_t used = 0;
    int extended = 0, ok = 1, i;
    uint32_t slot = (uint32_t)(uintptr_t)track;

    for (i = 0; i < 4; i++) {
        args[i] = (uint8_t)(slot >> (8 * i));
    }
    args[0] = head & 7;
    event->track = track;
    p++;
    switch (head & 7) {
    case 0:
        event->delta = 0;
        break;
    case 1:
        event->delta = *p++;
        break;
    case 2:
        event->delta = (uint32_t)zsndSeqReadU32(p);
        p += 4;
        break;
    case 3:
        event->delta = 0;
        p++;
        break;
    case 4:
        event->delta = p[1];
        p += 2;
        break;
    case 5:
        event->delta = (uint32_t)zsndSeqReadU32(p + 1);
        p += 5;
        break;
    case 6:
        event->delta = 0;
        p += 4;
        break;
    default:
        event->delta = (uint32_t)zsndSeqReadU32(p + 4);
        p += 8;
        break;
    }
    if (type == 0xf8) {
        uint8_t kinds = p[1];

        type = p[0];
        p += 2;
        extended = 1;
        /* The original masks the operand kind with 2 instead of 3: only "skip" and "byte at +3" occur. */
        for (i = 0; i < 4; i++) {
            if ((kinds & 2) != 0) {
                args[i] = p[3];
                p += 3;
            }
            kinds >>= 2;
        }
    } else {
        args[0] = p[0];
        args[1] = p[1];
    }
    switch (type) {
    case 0x00:
        event->handler = HANDLER(0x0047d7b0, zsndSeqOnNote);
        event->data.byte.a = args[0];
        event->data.byte.b = args[1];
        used = 2;
        break;
    case 0x08:
        event->handler = HANDLER(0x0047d7b0, zsndSeqOnNote);
        event->data.byte.a = args[0];
        event->data.byte.b = track->lastVelocity;
        used = 1;
        break;
    case 0x10:
        event->handler = HANDLER(0x0047da10, zsndSeqOnController);
        event->data.byte.a = args[0];
        event->data.byte.b = args[1];
        used = 2;
        break;
    case 0x18:
        event->handler = HANDLER(0x0047db90, zsndSeqOnEnd);
        event->data.byte.a = args[0];
        event->data.byte.b = args[1];
        used = 2;
        break;
    case 0x30:
        event->handler = HANDLER(0x0047dc10, zsndSeqOnJump);
        event->data.value = (uint32_t)zsndSeqReadU32(p);
        used = 4;
        break;
    case 0x20:
    case 0x28:
    case 0x38:
    case 0x40:
        break; /* known but ignored: no handler is set */
    default:
        ok = 0;
        break;
    }
    if (!extended) {
        p += used;
    }
    track->cursor = p;
    return ok;
}

BMX_ADDR(0x0047d7b0)
int zsndSeqOnNote(ZsndSeqEvent *event)
{
    ZsndSeqTrack *track = event->track;
    const void *voice = 0;
    int16_t cents = 0;
    int slot;
    ZsndNote *note;
    uint32_t length;

    track->lastVelocity = event->data.byte.b;
    if (track->voice != 0) {
        voice = track->voice;
        cents = (int16_t)(((int)event->data.byte.a - 0x3c) * 100);
    } else if (track->keymap != 0) {
        zsndSeqLookupKey(track->keymap, event->data.byte.a, &voice, &cents);
    } else {
        return 0;
    }
    if (voice == 0) {
        return 0;
    }
    slot = gZsndNextNote;
    if ((gZsndNotes[slot].flags & kzsndNoteEventFlagActive) != 0) {
        const int first = slot;

        do {
            slot++;
            if (slot == kzsndSeqMaxNotes) {
                slot = 0;
            }
            if (slot == first) {
                return 0;
            }
        } while ((gZsndNotes[slot].flags & kzsndNoteEventFlagActive) != 0);
    }
    note = &gZsndNotes[slot];
    slot++;
    note->flags = kzsndNoteEventFlagActive;
    note->next = 0;
    if (slot == kzsndSeqMaxNotes) {
        slot = 0;
    }
    gZsndNextNote = slot;
    note->voice = voice;
    note->velocity = event->data.byte.b;
    note->cents = cents;
    length = track->noteScale * event->delta;
    note->remaining = (length & 0xfffff000u) < 0x1000 ? 1 : length >> 12;
    if ((track->flags & kzsndSeqFlagMute) != 0 ||
        (gZsndMuteAll != 0 && (track->flags & kzsndSeqFlagGroupMute) != 0)) {
        note->flags |= kzsndNoteEventFlagSilent;
        zsndSeqInsertNote(track, note);
        return 1;
    }
    note->channel = winchanVoiceAlloc(voice, 0xff);
    if (note->channel != 0xff) {
        winchanVoiceSetPitch(note->channel, note->cents);
        winchanVoiceSetVolume(note->channel, noteVolume(track, note));
        winchanVoiceSetPan(note->channel, (int8_t)track->pan);
        winchanVoiceStart(note->channel);
        zsndSeqInsertNote(track, note);
        return 1;
    }
    /* No channel free: give the slot back. */
    note->flags = 0;
    gZsndNextNote = slot == 0 ? kzsndSeqMaxNotes - 1 : slot - 1;
    return 0;
}

BMX_ADDR(0x0047da10)
int zsndSeqOnController(ZsndSeqEvent *event)
{
    ZsndSeqTrack *track = event->track;
    ZsndNote *note;

    switch (event->data.byte.a) {
    case 0: /* expression */
        track->expression = event->data.byte.b;
        for (note = track->notes; note != 0; note = note->next) {
            if ((note->flags & kzsndNoteEventFlagSilent) == 0) {
                winchanVoiceSetVolume(note->channel, noteVolume(track, note));
            }
        }
        return 1;
    case 1: /* pan */
        track->pan = event->data.byte.b;
        for (note = track->notes; note != 0; note = note->next) {
            if ((note->flags & kzsndNoteEventFlagSilent) == 0) {
                winchanVoiceSetPan(note->channel, (int8_t)track->pan);
            }
        }
        return 1;
    case 2:
    case 3:
        return 1;
    case 4: /* voice */
        track->voice = event->data.byte.b == 0xff ? 0 : winloadGetVoice(track->bank, event->data.byte.b);
        return 1;
    case 5: /* key map */
        track->keymap = event->data.byte.b == 0xff ? 0 : winloadGetKeymap(track->bank, event->data.byte.b);
        return 1;
    default:
        return 0;
    }
}

BMX_ADDR(0x0047db90)
int zsndSeqOnEnd(ZsndSeqEvent *event)
{
    ZsndSeqTrack *track = event->track;

    if (event->data.byte.a == 1) {
        if ((track->flags & kzsndSeqFlagLoop) == 0) {
            track->state = kzsndSeqStateStopped;
        } else {
            track->notes = 0;
            track->cursor = track->start;
            track->wait = zsndSeqPeekDelta(track) << 12;
        }
    }
    if (track->onEvent != 0) {
        track->onEvent(track->onEventArg, event->data.byte.a, event->data.byte.b);
    }
    return 1;
}

BMX_ADDR(0x0047dc10)
int zsndSeqOnJump(ZsndSeqEvent *event)
{
    ZsndSeqTrack *track = event->track;

    track->cursor = track->start + event->data.value;
    track->wait = zsndSeqPeekDelta(track) << 12;
    return 1;
}

BMX_ADDR(0x0047dc50)
void zsndSeqInsertNote(ZsndSeqTrack *track, ZsndNote *note)
{
    ZsndNote *next = track->notes;

    if (next != 0) {
        ZsndNote *prev = 0;
        uint32_t sum = 0;

        do {
            const uint32_t end = next->remaining + sum;

            if (note->remaining < end) {
                break;
            }
            sum = end;
            prev = next;
            next = next->next;
        } while (next != 0);
        if (prev != 0) {
            prev->next = note;
            note->remaining -= sum;
            goto link;
        }
    }
    track->notes = note;
link:
    note->next = next;
    if (next != 0) {
        next->remaining -= note->remaining;
    }
}

BMX_ADDR(0x0047dcd0)
void zsndSeqReleaseNote(ZsndNote *note)
{
    if ((note->flags & kzsndNoteEventFlagActive) == 0) {
        _assert("noteEvent->flags & kzsndNoteEventFlagActive", zsndSeqFile, 0x46c);
    }
    if ((note->flags & kzsndNoteEventFlagSilent) == 0) {
        winchanVoiceStop(note->channel, 0);
        winchanVoiceFree(note->channel);
    }
    note->flags = 0;
}

BMX_ADDR(0x0047dd40)
int zsndSeqLookupKey(const uint8_t *keymap, uint8_t key, const void **voice, int16_t *cents)
{
    const uint8_t first = keymap[0];

    *voice = 0;
    if (first <= key && (uint32_t)key < (uint32_t)keymap[1] + first) {
        const uint8_t *entry = keymap + (key - first) * 2 + 4;

        if (entry[0] != 0xff) {
            *voice = winloadGetVoice((uint16_t)(keymap[2] | keymap[3] << 8), entry[0]);
            *cents = (int16_t)((int8_t)entry[1] * 100);
            return 1;
        }
    }
    return 0;
}

BMX_ADDR(0x0047dde0)
int32_t zsndSeqReadU32(const uint8_t *p)
{
    return (int32_t)((((uint32_t)p[3] * 0x100 + p[2]) * 0x100 + p[1]) * 0x100 + p[0]);
}
