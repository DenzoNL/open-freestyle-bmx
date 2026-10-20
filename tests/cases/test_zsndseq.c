/*
 * Tests for the sound sequencer (zsndseq.c). The mixer channel functions and the sound file lookups are
 * replaced with dt_stub() stubs that log their (masked) arguments and answer from the test's random
 * stream. Tracks, notes and event streams are random but well formed: note lists are acyclic, streams
 * end in an end marker and always make progress, so the update loop terminates.
 *
 * Not covered: the assert paths (handle >= kzsndSeqMaxTracks or an empty slot). The CRT assert would
 * continue into a dereference of the missing track.
 */
#include <stddef.h>

#include "dt.h"

#include "bmx/com/zge/zsndseq.h"
#include "bmx/pc/zge/sound/winchan.h"
#include "bmx/pc/zge/sound/winload.h"
#include "compat/crt.h"

/* ---- stub log ------------------------------------------------------------------------------------ */

static uint32_t gLog[2048];
static uint32_t gLogN;

static void logU(uint32_t v)
{
    if (gLogN < sizeof gLog / sizeof gLog[0]) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static uint8_t gVoices[8][0x14];
static uint8_t gKeymaps[4][0x60];

static void BMX_CDECL stub_assert(const char *expr, const char *file, unsigned line)
{
    logU(0x100);
    logU(line);
}

static uint8_t BMX_CDECL stub_alloc(const void *voice, int priority)
{
    uint32_t r = dt_u32();
    logU(0x101);
    logU((uint32_t)(uintptr_t)voice);
    logU((uint32_t)priority);
    return r % 4 == 0 ? 0xff : (uint8_t)(r >> 8) % 60;
}

static int BMX_CDECL stub_free(uint8_t ch)
{
    logU(0x102);
    logU(ch);
    return 0;
}

static int BMX_CDECL stub_start(uint8_t ch)
{
    logU(0x103);
    logU(ch);
    return 0;
}

static int BMX_CDECL stub_stop(uint8_t ch, uint8_t flags)
{
    logU(0x104);
    logU(ch);
    logU(flags);
    return 0;
}

static int BMX_CDECL stub_volume(uint8_t ch, uint8_t vol)
{
    logU(0x105);
    logU(ch);
    logU(vol);
    return 0;
}

static int BMX_CDECL stub_pan(uint8_t ch, int8_t pan)
{
    logU(0x106);
    logU(ch);
    logU((uint8_t)pan);
    return 0;
}

static int BMX_CDECL stub_pitch(uint8_t ch, int16_t cents)
{
    logU(0x107);
    logU(ch);
    logU((uint16_t)cents);
    return 0;
}

static void *BMX_CDECL stub_voice(uint16_t file, uint32_t index)
{
    uint32_t r = dt_u32();
    logU(0x108);
    logU(file);
    logU(index);
    return r % 5 == 0 ? NULL : gVoices[(file + index) % 8];
}

static const uint8_t *BMX_CDECL stub_keymap(uint16_t file, uint32_t index)
{
    uint32_t r = dt_u32();
    logU(0x109);
    logU(file);
    logU(index);
    return r % 5 == 0 ? NULL : gKeymaps[(file + index) % 4];
}

static void BMX_CDECL end_callback(uint32_t arg, uint32_t type, uint32_t value)
{
    logU(0x10a);
    logU(arg);
    logU(type);
    logU(value);
}

static int gStubAssert = 1;

static void stubs_on(void)
{
    gLogN = 0;
    if (gStubAssert) {
        dt_stub(0x004a4de8, NULL, (void *)stub_assert);
    }
    dt_stub(0x0046e410, NULL, (void *)stub_alloc);
    dt_stub(0x0046e890, NULL, (void *)stub_free);
    dt_stub(0x0046e9a0, NULL, (void *)stub_start);
    dt_stub(0x0046eb60, NULL, (void *)stub_stop);
    dt_stub(0x0046ed20, NULL, (void *)stub_volume);
    dt_stub(0x0046ee80, NULL, (void *)stub_pan);
    dt_stub(0x0046efb0, NULL, (void *)stub_pitch);
    dt_stub(0x004704a0, NULL, (void *)stub_voice);
    dt_stub(0x00470880, NULL, (void *)stub_keymap);
}

/* ---- fixtures ------------------------------------------------------------------------------------ */

static uint8_t rand_byte(void)
{
    return (uint8_t)dt_u32();
}

static uint32_t garbage_high(uint32_t low)
{
    return low | (dt_u32() & 0xffffff00u);
}

static const uint8_t *rand_voice(void)
{
    return gVoices[dt_u32() % 8];
}

static void make_keymaps(void)
{
    int k, i;
    for (k = 0; k < 4; k++) {
        uint8_t *m = gKeymaps[k];
        const int count = dt_range(0, 40);
        m[0] = (uint8_t)dt_range(0, 100);
        m[1] = (uint8_t)count;
        m[2] = rand_byte();
        m[3] = rand_byte();
        for (i = 0; i < count; i++) {
            m[4 + i * 2] = dt_u32() % 4 == 0 ? 0xff : (uint8_t)dt_range(0, 7);
            m[5 + i * 2] = rand_byte();
        }
    }
}

enum { kStreamBytes = 400 };

typedef struct EvSpec {
    int kind, type, off, size;
    uint32_t arg;
} EvSpec;

static const int kDeltaBytes[8] = {0, 1, 4, 1, 2, 5, 4, 8};

static int arg_bytes(int type)
{
    return type == 0x08 ? 1 : type == 0x30 ? 4 : 2;
}

/*
 * A well formed event stream: [4 header bytes][events]. The first event has a non-zero wait (the byte
 * the peek reads), jumps only go forward, and the last event is an end marker.
 */
static uint8_t *make_stream(int loops)
{
    static const int types[] = {0x00, 0x08, 0x10, 0x18, 0x30};
    EvSpec ev[16];
    uint8_t *data = dt_alloc(kStreamBytes + 4);
    const int n = dt_range(3, 14);
    int i, off = 0;

    for (i = 0; i < n; i++) {
        ev[i].type = i == n - 1 ? 0x18 : types[dt_u32() % 5];
        ev[i].kind = i == 0 ? 4 : (int)(dt_u32() % 8);
        ev[i].size = 1 + kDeltaBytes[ev[i].kind] + arg_bytes(ev[i].type);
        ev[i].off = off;
        off += ev[i].size;
    }
    for (i = 0; i < n; i++) {
        uint8_t *p = data + 4 + ev[i].off;
        int j;
        p[0] = (uint8_t)(ev[i].type | ev[i].kind);
        for (j = 0; j < kDeltaBytes[ev[i].kind]; j++) {
            p[1 + j] = (uint8_t)dt_range(0, 3);
        }
        if (ev[i].kind == 2 || ev[i].kind == 5 || ev[i].kind == 7) {
            const int at = ev[i].kind == 2 ? 1 : ev[i].kind == 5 ? 2 : 5;
            p[at] = (uint8_t)dt_range(0, 200); /* note length */
        } else if (ev[i].kind == 1 || ev[i].kind == 4) {
            p[ev[i].kind == 1 ? 1 : 2] = (uint8_t)dt_range(0, 200);
        }
        if (i == 0) {
            p[1] = (uint8_t)dt_range(1, 3); /* time to the first event as the peek reads it */
        }
        p += 1 + kDeltaBytes[ev[i].kind];
        switch (ev[i].type) {
        case 0x00:
        case 0x08:
            p[0] = (uint8_t)dt_range(20, 110); /* key */
            if (ev[i].type == 0x00) {
                p[1] = (uint8_t)dt_range(0, 127); /* velocity; type 8 reuses the last one */
            }
            break;
        case 0x10:
            p[0] = (uint8_t)dt_range(0, 6);
            p[1] = dt_u32() % 5 == 0 ? 0xff : rand_byte();
            break;
        case 0x18:
            p[0] = (uint8_t)dt_range(0, 2);
            if (i == n - 1) {
                p[0] = 1; /* the last event always stops (or loops) the track */
            }
            p[1] = rand_byte();
            break;
        default: {
            const int target = i + 1 < n ? dt_range(i + 1, n - 1) : n - 1;
            const uint32_t v = (uint32_t)ev[target].off;
            p[0] = (uint8_t)v;
            p[1] = (uint8_t)(v >> 8);
            p[2] = 0;
            p[3] = 0;
        }
        }
    }
    return data;
}

static ZsndSeqHeader *make_header(int loops)
{
    ZsndSeqHeader *h = dt_alloc(sizeof *h);
    h->volume = rand_byte();
    h->pan = rand_byte();
    h->voice = dt_u32() % 3 == 0 ? 0xff : (uint8_t)dt_range(0, 7);
    h->keymap = dt_u32() % 3 == 0 ? 0xff : (uint8_t)dt_range(0, 3);
    h->data = make_stream(loops);
    h->bank = (uint16_t)dt_u32();
    return h;
}

static ZsndNote *take_note(int *used)
{
    int i;
    do {
        i = dt_range(0, kzsndSeqMaxNotes - 1);
    } while (used[i]);
    used[i] = 1;
    return &gZsndNotes[i];
}

/* Fills the pools: `tracks` live tracks with random state and a few notes each. */
static void make_world(int tracks, int loops)
{
    int used[kzsndSeqMaxNotes] = {0};
    int slotUsed[kzsndSeqMaxTracks] = {0};
    int i, t;

    memset(&gZsndNotes, 0, sizeof gZsndNotes);
    memset(&gZsndTrackPool, 0, sizeof gZsndTrackPool);
    memset(&gZsndTracks, 0, sizeof gZsndTracks);
    for (i = 0; i < kzsndSeqMaxNotes; i++) {
        gZsndNotes[i].channel = rand_byte();
        gZsndNotes[i].remaining = dt_u32() % 7;
    }
    gZsndNextNote = dt_range(0, kzsndSeqMaxNotes - 1);
    gZsndNextTrack = dt_range(0, kzsndSeqMaxTracks - 1);
    gZsndMuteAll = (int32_t)(dt_u32() & 1);
    gZsndSeqLock = 0;
    make_keymaps();
    for (t = 0; t < tracks; t++) {
        ZsndSeqTrack *tr;
        ZsndNote **link;
        int count;

        do {
            i = dt_range(0, kzsndSeqMaxTracks - 1);
        } while (slotUsed[i]);
        slotUsed[i] = 1;
        tr = &gZsndTrackPool[i];
        tr->header = make_header(loops);
        tr->flags = (uint16_t)(kzsndSeqFlagUsed | (dt_u32() & 0x7fff));
        if (!loops) {
            tr->flags &= (uint16_t)~kzsndSeqFlagLoop;
        }
        tr->state = (uint8_t)(dt_u32() % 3);
        tr->lastVelocity = rand_byte();
        tr->volume = rand_byte();
        tr->expression = rand_byte();
        tr->pan = rand_byte();
        tr->voice = dt_u32() % 3 == 0 ? NULL : rand_voice();
        tr->keymap = dt_u32() % 3 == 0 ? NULL : gKeymaps[dt_u32() % 4];
        tr->start = tr->header->data + 4;
        tr->cursor = tr->start;
        tr->wait = (uint32_t)dt_range(0, 3) << 12;
        tr->step = (uint32_t)dt_range(0x800, 0x30000);
        tr->noteScale = (uint32_t)dt_range(0, 0x3000);
        tr->bank = (uint16_t)dt_u32();
        tr->onEvent = dt_u32() & 1 ? end_callback : NULL;
        tr->onEventArg = dt_u32();
        tr->registers[0] = dt_u32();
        count = dt_range(0, 5);
        link = &tr->notes;
        while (count-- > 0) {
            ZsndNote *n = take_note(used);
            n->flags =
                (uint8_t)(kzsndNoteEventFlagActive | (dt_u32() % 3 == 0 ? kzsndNoteEventFlagSilent : 0));
            n->velocity = (uint8_t)dt_range(0, 127);
            n->cents = (int16_t)dt_u32();
            n->voice = rand_voice();
            n->remaining = dt_u32() % 40;
            *link = n;
            link = &n->next;
        }
        *link = NULL;
        gZsndTracks[i] = tr;
    }
}

/* Handle of a random live track. */
static uint8_t live_handle(void)
{
    int n = 0, pick, i;
    for (i = 0; i < kzsndSeqMaxTracks; i++) {
        n += gZsndTracks[i] != NULL;
    }
    pick = dt_range(0, n - 1);
    for (i = 0; i < kzsndSeqMaxTracks; i++) {
        if (gZsndTracks[i] != NULL && pick-- == 0) {
            return (uint8_t)i;
        }
    }
    return 0;
}

static void begin(DtCtx *c, int tracks)
{
    dt_seed(c);
    stubs_on();
    make_world(tracks, (int)(c->seed & 1));
}

/* ---- setups -------------------------------------------------------------------------------------- */

static void setup_world(DtCtx *c)
{
    begin(c, dt_range(0, 5));
}

static void setup_update(DtCtx *c)
{
    int i;
    begin(c, dt_range(1, 8));
    /* A stopped track with a pending change counts as running for ever and would decode past its stream. */
    for (i = 0; i < kzsndSeqMaxTracks; i++) {
        gZsndTrackPool[i].flags &= (uint16_t)~kzsndSeqFlagPending;
    }
    if (c->seed % 11 == 5) {
        gZsndSeqLock = 1;
    }
}

static void setup_handle(DtCtx *c)
{
    begin(c, dt_range(1, 6));
    DT_ARG_INT(c, 0, garbage_high(live_handle()));
}

/* Handle out of range: the CRT assert is not stubbed here, so the run ends there on both sides. */
static void setup_bad_handle(DtCtx *c)
{
    int h;
    gStubAssert = 0;
    begin(c, dt_range(1, 3));
    gStubAssert = 1;
    if (c->seed % 2 == 0) {
        h = 0x20 + (int)(c->seed % 100);
    } else {
        /* a slot without a track */
        for (h = 0; h < kzsndSeqMaxTracks && gZsndTracks[h] != NULL; h++) {
        }
    }
    DT_ARG_INT(c, 0, garbage_high((uint8_t)h));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_handle_int(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_INT(c, 1, (c->seed & 2) ? dt_u32() : 0);
}

static void setup_set_mode(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_tempo(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_FLOAT(c, 1, dt_float_edgy(-2.0f, 8.0f));
    gZsndTicksPerSec = (uint32_t)dt_range(1, 120);
}

static void setup_volume(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_callback(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_PTR(c, 1, c->seed & 1 ? (void *)end_callback : NULL);
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_create(DtCtx *c)
{
    int i;
    begin(c, dt_range(0, 3));
    if (c->seed % 7 == 3) {
        /* every slot taken */
        for (i = 0; i < kzsndSeqMaxTracks; i++) {
            gZsndTrackPool[i].flags |= kzsndSeqFlagUsed;
        }
    } else if (c->seed % 3 == 1) {
        for (i = 0; i < kzsndSeqMaxTracks - 1; i++) {
            gZsndTrackPool[(gZsndNextTrack + i) % kzsndSeqMaxTracks].flags |= kzsndSeqFlagUsed;
        }
    }
    gZsndTicksPerSec = (uint32_t)dt_range(1, 120);
    DT_ARG_PTR(c, 0, make_header(0));
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_play_once(DtCtx *c)
{
    setup_create(c);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

/* A track pointer. */
static void setup_track_ptr(DtCtx *c)
{
    begin(c, dt_range(1, 4));
    DT_ARG_PTR(c, 0, gZsndTracks[live_handle()]);
    if (c->seed % 4 == 0) {
        ((ZsndSeqTrack *)(uintptr_t)c->args[0])->state = kzsndSeqStatePlaying;
    }
}

static void setup_peek(DtCtx *c)
{
    ZsndSeqTrack *t;
    int i;
    begin(c, 1);
    t = gZsndTracks[live_handle()];
    for (i = 0; i < 12; i++) {
        ((uint8_t *)t->start)[i] = rand_byte();
    }
    DT_ARG_PTR(c, 0, t);
}

/* Decoder fixture: random bytes (any event type, extended events, unknown types). */
static void setup_decode(DtCtx *c)
{
    ZsndSeqTrack *t;
    ZsndSeqEvent *ev;
    uint8_t *data;
    int i;
    begin(c, 1);
    t = gZsndTracks[live_handle()];
    data = (uint8_t *)t->header->data;
    for (i = 0; i < 64; i++) {
        data[4 + i] = rand_byte();
    }
    switch (c->seed % 5) {
    case 0:
        data[4] = (uint8_t)(0xf8 | (dt_u32() & 7)); /* extended */
        data[5] = (uint8_t)(dt_u32() % 0x49);
        break;
    case 1:
        data[4] = (uint8_t)((dt_u32() % 9) * 8 | (dt_u32() & 7)); /* known types, any delta kind */
        break;
    case 2:
        data[4] = (uint8_t)(0xf8 | (dt_u32() & 7));
        data[5] = dt_u32() & 1 ? 0x18 : 0x30;
        data[6] = 0xff; /* every operand "byte at +3" */
        break;
    default:
        break;
    }
    ev = dt_alloc(sizeof *ev);
    ev->delta = dt_u32();
    ev->data.value = dt_u32();
    ev->handler = (ZsndSeqHandler)(uintptr_t)dt_u32();
    DT_ARG_PTR(c, 0, t);
    DT_ARG_PTR(c, 1, ev);
}

/* Handler fixture: an event for a live track. */
static void setup_event(DtCtx *c)
{
    ZsndSeqTrack *t;
    ZsndSeqEvent *ev;
    begin(c, dt_range(1, 4));
    t = gZsndTracks[live_handle()];
    ev = dt_alloc(sizeof *ev);
    ev->track = t;
    ev->delta = (uint32_t)dt_range(0, 300);
    ev->data.byte.a = dt_u32() % 4 == 0 ? (uint8_t)dt_u32() : (uint8_t)dt_range(20, 110);
    ev->data.byte.b = rand_byte();
    ev->handler = NULL;
    if (c->seed % 6 == 1) {
        t->voice = NULL; /* key map path */
    }
    if (c->seed % 13 == 7) {
        t->voice = NULL;
        t->keymap = NULL; /* neither: the note is dropped */
    }
    if (c->seed % 9 == 4) {
        int i;
        for (i = 0; i < kzsndSeqMaxNotes; i++) {
            gZsndNotes[i].flags |= kzsndNoteEventFlagActive; /* no free note slot */
        }
    }
    DT_ARG_PTR(c, 0, ev);
}

static void setup_controller(DtCtx *c)
{
    setup_event(c);
    ((ZsndSeqEvent *)(uintptr_t)c->args[0])->data.byte.a = (uint8_t)(dt_u32() % 8);
}

static void setup_end(DtCtx *c)
{
    setup_event(c);
    ((ZsndSeqEvent *)(uintptr_t)c->args[0])->data.byte.a = (uint8_t)(dt_u32() % 3);
}

static void setup_jump(DtCtx *c)
{
    setup_event(c);
    ((ZsndSeqEvent *)(uintptr_t)c->args[0])->data.value = (uint32_t)dt_range(0, 20);
}

static void setup_insert(DtCtx *c)
{
    ZsndSeqTrack *t;
    ZsndNote *n;
    int used[kzsndSeqMaxNotes] = {0};
    int i;
    begin(c, 1);
    t = gZsndTracks[live_handle()];
    for (n = t->notes; n; n = n->next) {
        used[n - gZsndNotes] = 1;
    }
    /* The list is ordered by end time, as the sequencer keeps it. */
    for (n = t->notes, i = 0; n; n = n->next, i++) {
        n->remaining = (uint32_t)dt_range(0, 12);
    }
    n = take_note(used);
    n->flags = kzsndNoteEventFlagActive;
    n->remaining = (uint32_t)dt_range(0, 60);
    n->next = NULL;
    DT_ARG_PTR(c, 0, t);
    DT_ARG_PTR(c, 1, n);
}

static void setup_release(DtCtx *c)
{
    ZsndNote *n;
    int used[kzsndSeqMaxNotes] = {0};
    begin(c, 0);
    n = take_note(used);
    n->flags = (uint8_t)(dt_u32() % 4); /* includes inactive notes: the assert is stubbed */
    DT_ARG_PTR(c, 0, n);
}

static void setup_lookup(DtCtx *c)
{
    const uint8_t *m;
    ZsndSeqTrack *t;
    begin(c, 1);
    m = gKeymaps[dt_u32() % 4];
    DT_ARG_PTR(c, 0, m);
    DT_ARG_INT(c, 1, garbage_high((uint8_t)(m[0] + (int)(dt_u32() % 50) - 5)));
    DT_ARG_PTR(c, 2, dt_alloc(8));
    DT_ARG_PTR(c, 3, dt_alloc(8));
    t = NULL;
    (void)t;
}

static void setup_read(DtCtx *c)
{
    uint8_t *p;
    dt_seed(c);
    p = dt_alloc(8);
    dt_fill(p, 8);
    DT_ARG_PTR(c, 0, p);
}

DT_CASES(zsndseq_cases) = {
    {"zsndSeqReset", 0x0047c4c0, zsndSeqReset, 0, DT_RET_INT, setup_world, 20, 0},
    {"zsndSeqStopAll", 0x0047c530, zsndSeqStopAll, 0, DT_RET_INT, setup_world, 100, 0},
    {"zsndSeqUpdate", 0x0047c580, zsndSeqUpdate, 0, DT_RET_INT, setup_update, 800, 0},
    {"zsndSeqCreate", 0x0047c6b0, zsndSeqCreate, 2, DT_RET_INT8, setup_create, 300, 0},
    {"zsndSeqDestroy", 0x0047c870, zsndSeqDestroy, 1, DT_RET_INT, setup_handle, 200, 0},
    {"zsndSeqDestroy_badhandle", 0x0047c870, zsndSeqDestroy, 1, DT_RET_INT, setup_bad_handle, 5, 0},
    {"zsndSeqStart_badhandle", 0x0047c940, zsndSeqStart, 1, DT_RET_INT, setup_bad_handle, 12, 0},
    {"zsndSeqStop_badhandle", 0x0047ca50, zsndSeqStop, 1, DT_RET_INT, setup_bad_handle, 12, 0},
    {"zsndSeqPause_badhandle", 0x0047cb30, zsndSeqPause, 1, DT_RET_INT, setup_bad_handle, 12, 0},
    {"zsndSeqMute_badhandle", 0x0047cc20, zsndSeqMute, 2, DT_RET_INT, setup_bad_handle, 12, 0},
    {"zsndSeqSetGroupMute_badhandle", 0x0047ce30, zsndSeqSetGroupMute, 2, DT_RET_INT, setup_bad_handle, 12,
     0},
    {"zsndSeqGetStatus_badhandle", 0x0047cf10, zsndSeqGetStatus, 1, DT_RET_INT, setup_bad_handle, 12, 0},
    {"zsndSeqSetMode_badhandle", 0x0047cfe0, zsndSeqSetMode, 2, DT_RET_INT, setup_bad_handle, 12, 0},
    {"zsndSeqSetTempo_badhandle", 0x0047d0d0, zsndSeqSetTempo, 2, DT_RET_INT, setup_bad_handle, 12, 0},
    {"zsndSeqSetVolume_badhandle", 0x0047d1b0, zsndSeqSetVolume, 3, DT_RET_INT, setup_bad_handle, 12, 0},
    {"zsndSeqSetLoop_badhandle", 0x0047d290, zsndSeqSetLoop, 2, DT_RET_INT, setup_bad_handle, 12, 0},
    {"zsndSeqSetEndCallback_badhandle", 0x0047d3a0, zsndSeqSetEndCallback, 3, DT_RET_INT, setup_bad_handle,
     12, 0},
    {"zsndSeqStart", 0x0047c940, zsndSeqStart, 1, DT_RET_INT, setup_handle, 300, 0},
    {"zsndSeqStop", 0x0047ca50, zsndSeqStop, 1, DT_RET_INT, setup_handle, 300, 0},
    {"zsndSeqPause", 0x0047cb30, zsndSeqPause, 1, DT_RET_INT, setup_handle, 300, 0},
    {"zsndSeqMute", 0x0047cc20, zsndSeqMute, 2, DT_RET_INT, setup_handle_int, 400, 0},
    {"zsndSeqSetGroupMute", 0x0047ce30, zsndSeqSetGroupMute, 2, DT_RET_INT, setup_handle_int, 300, 0},
    {"zsndSeqGetStatus", 0x0047cf10, zsndSeqGetStatus, 1, DT_RET_INT, setup_handle, 200, 0},
    {"zsndSeqSetMode", 0x0047cfe0, zsndSeqSetMode, 2, DT_RET_INT, setup_set_mode, 400, 0},
    {"zsndSeqSetTempo", 0x0047d0d0, zsndSeqSetTempo, 2, DT_RET_INT, setup_tempo, 300, 0},
    {"zsndSeqSetVolume", 0x0047d1b0, zsndSeqSetVolume, 3, DT_RET_INT, setup_volume, 300, 0},
    {"zsndSeqSetLoop", 0x0047d290, zsndSeqSetLoop, 2, DT_RET_INT, setup_handle_int, 100, 0},
    {"zsndSeqPlayOnce", 0x0047d320, zsndSeqPlayOnce, 3, DT_RET_INT, setup_play_once, 300, 0},
    {"zsndSeqSetEndCallback", 0x0047d3a0, zsndSeqSetEndCallback, 3, DT_RET_INT, setup_callback, 100, 0},
    {"zsndSeqIsRunning", 0x0047d420, zsndSeqIsRunning, 1, DT_RET_INT, setup_track_ptr, 100, 0},
    {"zsndSeqPeekDelta", 0x0047d450, zsndSeqPeekDelta, 1, DT_RET_INT, setup_peek, 300, 0},
    {"zsndSeqDecodeEvent", 0x0047d490, zsndSeqDecodeEvent, 2, DT_RET_INT, setup_decode, 1500, 0},
    {"zsndSeqOnNote", 0x0047d7b0, zsndSeqOnNote, 1, DT_RET_INT, setup_event, 800, 0},
    {"zsndSeqOnController", 0x0047da10, zsndSeqOnController, 1, DT_RET_INT, setup_controller, 400, 0},
    {"zsndSeqOnEnd", 0x0047db90, zsndSeqOnEnd, 1, DT_RET_INT, setup_end, 300, 0},
    {"zsndSeqOnJump", 0x0047dc10, zsndSeqOnJump, 1, DT_RET_INT, setup_jump, 100, 0},
    {"zsndSeqInsertNote", 0x0047dc50, zsndSeqInsertNote, 2, DT_RET_VOID, setup_insert, 500, 0},
    {"zsndSeqReleaseNote", 0x0047dcd0, zsndSeqReleaseNote, 1, DT_RET_VOID, setup_release, 100, 0},
    {"zsndSeqLookupKey", 0x0047dd40, zsndSeqLookupKey, 4, DT_RET_INT, setup_lookup, 500, 0},
    {"zsndSeqReadU32", 0x0047dde0, zsndSeqReadU32, 1, DT_RET_INT, setup_read, 50, 0},
};
