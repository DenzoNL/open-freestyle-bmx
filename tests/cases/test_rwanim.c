/*
 * Tests for the animation plugin in rw/plugins.c (frame sequences, the frame and clump plugin callbacks, the
 * stream chunks and the playback with its x87 interpolation) and the bone plugin's matrix walk and geometry
 * reader. The engine instance is the fixture of dt_rw.h, enlarged by the plugin's link free list; string
 * functions are small test implementations, the stream functions read and write a scripted buffer and fail on
 * request. The x87 entries are called through their register / ST0 conventions by asm shims.
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwanim.h"

#define REG1(addr, ret, ta)     ((ret(__attribute__((regparm(1))) *)(ta))(uintptr_t)(addr))
#define REG2(addr, ret, ta, tb) ((ret(__attribute__((regparm(2))) *)(ta, tb))(uintptr_t)(addr))

enum {
    kEngineBytes = 0x400,
    kAnimEngineOffset = 0x3f0,
    kFrameBytes = 0xc0,
    kFrameOffset = 0xa8,
    kClumpOffset = 0x40,
    kClumpBytes = 0x60,
};

#define FREELIST_ANIM ((RwFreeList *)0x20004000)

/* ---- environment -------------------------------------------------------------------------------------- */

static int BMX_CDECL tStrcmp(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return *(const uint8_t *)a < *(const uint8_t *)b ? -1 : *(const uint8_t *)a > *(const uint8_t *)b;
}

static size_t BMX_CDECL tStrlen(const char *s)
{
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

static char *BMX_CDECL tStrcpy(char *dst, const char *src)
{
    rwtLog(0x5c9);
    rwtLogPtr(dst);
    size_t i = 0;
    do {
        dst[i] = src[i];
    } while (src[i++] != '\0');
    return dst;
}

static RwGlobals *animEnv(const DtCtx *c)
{
    dt_seed(c);
    RwGlobals *small = rwtEnv();
    RwGlobals *g = dt_alloc(kEngineBytes);
    __builtin_memcpy(g, small, kRwtEngineBytes);
    RwEngineInstance = g;
    rpAnimEngineOffset = kAnimEngineOffset;
    *(RwFreeList **)((uint8_t *)g + kAnimEngineOffset) = FREELIST_ANIM;
    rpAnimFrameOffset = kFrameOffset;
    rpAnimClumpOffset = kClumpOffset;
    g->stringFuncs.rwstrcmp = (void *)tStrcmp;
    g->stringFuncs.rwstrlen = (void *)tStrlen;
    g->stringFuncs.rwstrcpy = tStrcpy;
    return g;
}

static const char *const kNames[] = {"walk", "run", "jump", "a", "", "walk2"};

static char *mkName(void)
{
    const char *n = kNames[dt_u32() % 6];
    char *s = dt_alloc((uint32_t)tStrlen(n) + 1);
    tStrcpy(s, n);
    return s;
}

static float animTime(void)
{
    switch (dt_u32() % 8) {
    case 0:
        return 0.0f;
    case 1:
        return (float)dt_range(0, 4) * 0.5f;
    default:
        return dt_float(0.0f, 3.0f);
    }
}

/* A frame sequence of the type (1 or 2) with 1..5 keyframes and 1..5 keys between them. */
static RpAnimFrameSequence *mkSeq(int32_t type)
{
    RpAnimFrameSequence *s = dt_alloc(sizeof *s);
    s->refCount = dt_range(1, 3);
    s->name = mkName();
    s->type = type;
    s->numFrames = dt_range(1, 5);
    const int32_t width = type == 2 ? 4 : 3;
    float *frames = dt_alloc((uint32_t)(s->numFrames * width * 4));
    for (int32_t i = 0; i < s->numFrames * width; i++) {
        frames[i] = dt_u32() % 16 == 0 ? dt_float_edgy(-2.0f, 2.0f) : dt_float(-1.0f, 1.0f);
    }
    if (type == 2 && s->numFrames > 1 && dt_u32() % 4 == 0) {
        /* neighbours that are (almost) the same rotation or opposite */
        for (int i = 0; i < 4; i++) {
            frames[4 + i] = dt_u32() % 2 ? frames[i] : -frames[i];
        }
    }
    s->frames = frames;
    s->numKeys = dt_range(1, 5);
    RpAnimKey *keys = dt_alloc((uint32_t)(s->numKeys * 12));
    for (int32_t i = 0; i < s->numKeys; i++) {
        keys[i].start = dt_range(0, s->numFrames - 1);
        keys[i].end = dt_range(0, s->numFrames - 1);
        keys[i].duration = dt_u32() % 10 == 0 ? (float)dt_range(0, 2) * 0.5f : dt_float(0.1f, 1.5f);
    }
    s->keys = (uint8_t *)keys;
    return s;
}

static RpAnimFrameLink *mkLinks(int32_t type, int32_t *count)
{
    RpAnimFrameLink *head = NULL;
    *count = 0;
    for (int n = dt_range(0, 3); n > 0; n--) {
        RpAnimFrameLink *link = dt_alloc(sizeof *link);
        link->sequence = mkSeq(type);
        link->next = head;
        head = link;
        (*count)++;
    }
    return head;
}

static RwFrame *mkFrame(int withData)
{
    RwFrame *f = dt_alloc(kFrameBytes);
    f->root = f;
    RpAnimFramePlugin *p = (RpAnimFramePlugin *)((uint8_t *)f + kFrameOffset);
    p->userData = dt_u32() % 3 == 0 ? 0xffffffffU : dt_u32();
    p->priority = dt_range(-1, 3);
    if (withData && dt_u32() % 5 != 0) {
        RpAnimFrameData *d = dt_alloc(sizeof *d);
        d->listA = mkLinks(1, &d->numA);
        d->listB = mkLinks(2, &d->numB);
        if (dt_u32() % 6 == 0) {
            d->numB = dt_range(-1, 0);
        }
        p->data = d;
    }
    return f;
}

static RpAnimFramePlugin *plug(void *frame)
{
    return (RpAnimFramePlugin *)((uint8_t *)frame + kFrameOffset);
}

/* ---- frame sequences ------------------------------------------------------------------------------------
 */

static void setup_seq_deinit(DtCtx *c)
{
    animEnv(c);
    RpAnimFrameSequence *s = mkSeq(dt_range(1, 2));
    if (dt_u32() % 3 == 0) {
        s->name = NULL;
    }
    if (dt_u32() % 3 == 0) {
        s->frames = NULL;
    }
    if (dt_u32() % 3 == 0) {
        s->keys = NULL;
    }
    DT_ARG_PTR(c, 0, s);
    rwtEnd();
}

static void setup_seq_init(DtCtx *c)
{
    animEnv(c);
    RpAnimFrameSequence *s = dt_alloc(sizeof *s);
    s->frames = dt_u32() % 2 ? dt_alloc(4) : NULL; /* left alone for other types */
    gRwtMallocFailAt = dt_u32() % 3 == 0 ? dt_range(1, 3) : 0;
    DT_ARG_PTR(c, 0, s);
    DT_ARG_PTR(c, 1, dt_u32() % 6 == 0 ? NULL : mkName());
    DT_ARG_INT(c, 2, dt_range(0, 3));
    DT_ARG_INT(c, 3, dt_range(0, 5));
    DT_ARG_INT(c, 4, dt_range(0, 5));
    rwtEnd();
}

typedef RpAnimFrameSequence *(__attribute__((regparm(3))) * SeqCreateFn)(const char *, int32_t, int32_t,
                                                                         uint32_t, uint32_t, uint32_t,
                                                                         int32_t);

static RpAnimFrameSequence *seqCreateOrig(const char *name, int32_t type, int32_t frames, int32_t keys)
{
    return ((SeqCreateFn)(uintptr_t)0x004fffdc)(name, type, frames, 0, 0, 0, keys);
}

static RpAnimFrameSequence *seqCreateImpl(const char *name, int32_t type, int32_t frames, int32_t keys)
{
    return rpAnimSequenceCreate(name, type, frames, 0, 0, 0, keys);
}

static void setup_seq_create(DtCtx *c)
{
    animEnv(c);
    gRwtMallocFailAt = dt_u32() % 3 == 0 ? dt_range(1, 2) : 0;
    DT_ARG_PTR(c, 0, mkName());
    DT_ARG_INT(c, 1, dt_range(1, 2));
    DT_ARG_INT(c, 2, dt_range(0, 5));
    DT_ARG_INT(c, 3, dt_range(0, 5));
    rwtEnd();
}

static int seqReleaseOrig(RpAnimFrameSequence *s)
{
    return REG1(0x004fd244, int, RpAnimFrameSequence *)(s);
}

static int seqReleaseImpl(RpAnimFrameSequence *s)
{
    return rpAnimSequenceRelease(s);
}

static void setup_seq_release(DtCtx *c)
{
    animEnv(c);
    DT_ARG_PTR(c, 0, mkSeq(dt_range(1, 2)));
    rwtEnd();
}

/* ---- the frame plugin ------------------------------------------------------------------------------------
 */

static int dataDestroyOrig(RpAnimFrameData *d)
{
    return REG1(0x004fbbc0, int, RpAnimFrameData *)(d);
}

static int dataDestroyImpl(RpAnimFrameData *d)
{
    return rpAnimFrameDataDestroy(d);
}

static void setup_data_destroy(DtCtx *c)
{
    animEnv(c);
    RwFrame *f = mkFrame(1);
    RpAnimFrameData *d = plug(f)->data;
    if (d == NULL) {
        d = dt_alloc(sizeof *d);
    }
    DT_ARG_PTR(c, 0, d);
    rwtEnd();
}

static void setup_frame_deinit(DtCtx *c)
{
    animEnv(c);
    DT_ARG_PTR(c, 0, mkFrame(1));
    rwtEnd();
}

static void *frameAddOrig(void *frame, RpAnimFrameSequence *s)
{
    return REG2(0x004fbdf8, void *, void *, RpAnimFrameSequence *)(frame, s);
}

static void *frameAddImpl(void *frame, RpAnimFrameSequence *s)
{
    return rpAnimFrameAddSequence(frame, s);
}

static void setup_frame_add(DtCtx *c)
{
    animEnv(c);
    gRwtMallocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    RwFrame *f = mkFrame(1);
    RpAnimFrameSequence *s = mkSeq(dt_range(1, 2));
    if (dt_u32() % 8 == 0) {
        s->type = dt_range(-1, 3);
    }
    DT_ARG_PTR(c, 0, f);
    DT_ARG_PTR(c, 1, s);
    rwtEnd();
}

static void setup_frame_copy(DtCtx *c)
{
    animEnv(c);
    gRwtMallocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    DT_ARG_PTR(c, 0, mkFrame(dt_u32() % 4 == 0));
    DT_ARG_PTR(c, 1, mkFrame(1));
    rwtEnd();
}

/* ---- the clump plugin -------------------------------------------------------------------------------------
 */

static void *BMX_CDECL stChildren(void *frame, void *callback, void *data)
{
    rwtLog(0x4d2eb0);
    rwtLogPtr(frame);
    rwtLogPtr(callback);
    rwtLogPtr(data);
    return frame;
}

static void setup_find_nodes(DtCtx *c)
{
    animEnv(c);
    dt_stub(0x004d2eb0, NULL, (void *)stChildren);
    gRwtMallocFailAt = dt_u32() % 4 == 0 ? dt_range(1, 3) : 0;
    RpAnimNodeSearch *search = dt_alloc(sizeof *search);
    search->nodes = dt_u32() % 2 ? dt_alloc(sizeof(RpAnimNode)) : NULL;
    search->name = kNames[dt_u32() % 6];
    DT_ARG_PTR(c, 0, mkFrame(1));
    DT_ARG_PTR(c, 1, search);
    rwtEnd();
}

static RpAnimClumpSequence *mkClumpSeqs(int n)
{
    RpAnimClumpSequence *head = dt_alloc(sizeof *head);
    head->name = mkName();
    RpAnimClumpSequence *tail = head;
    for (; n > 0; n--) {
        RpAnimClumpSequence *s = dt_alloc(sizeof *s);
        s->name = dt_u32() % 5 == 0 ? NULL : mkName();
        RpAnimNode **at = &s->nodes;
        for (int k = dt_range(0, 2); k > 0; k--) {
            RpAnimNode *node = dt_alloc(sizeof *node);
            node->a = dt_u32() % 2 ? dt_alloc(sizeof(RpAnimInstance)) : NULL;
            node->b = dt_u32() % 2 ? dt_alloc(sizeof(RpAnimInstance)) : NULL;
            *at = node;
            at = &node->next;
        }
        tail->next = s;
        tail = s;
    }
    return head;
}

static void setup_clump_init(DtCtx *c)
{
    animEnv(c);
    /* the failure of the fourth allocation (the second head's name) crashes the original */
    gRwtMallocFailAt = dt_u32() % 3 == 0 ? dt_range(1, 2) : 0;
    uint8_t *clump = dt_alloc(kClumpBytes);
    DT_ARG_PTR(c, 0, clump);
    rwtEnd();
}

static void setup_clump_data_init(DtCtx *c)
{
    setup_clump_init(c);
    c->args[0] += kClumpOffset;
}

static void setup_clump_deinit(DtCtx *c)
{
    animEnv(c);
    uint8_t *clump = dt_alloc(kClumpBytes);
    RpAnimClumpData *d = (RpAnimClumpData *)(clump + kClumpOffset);
    d->numSequences = dt_range(0, 4);
    d->sequences = dt_u32() % 6 == 0 ? NULL : mkClumpSeqs(dt_range(0, 3));
    d->others = dt_u32() % 6 == 0 ? NULL : mkClumpSeqs(dt_range(0, 2));
    d->frameList.numFrames = dt_range(0, 2);
    d->frameList.frames = (RwFrame **)dt_alloc(8);
    DT_ARG_PTR(c, 0, clump);
    rwtEnd();
}

static void setup_clump_data_deinit(DtCtx *c)
{
    setup_clump_deinit(c);
    c->args[0] += kClumpOffset;
}

/* ---- streams ----------------------------------------------------------------------------------------------
 */

enum { kStreamBytes = 0x400 };
static uint8_t gStream[kStreamBytes];
static uint32_t gStreamAt, gStreamSize;
static int gStreamCalls, gStreamFailAt;

static int streamFails(void)
{
    gStreamCalls++;
    return gStreamFailAt != 0 && gStreamCalls == gStreamFailAt;
}

static uint32_t streamIn(void *dst, uint32_t size)
{
    uint32_t n = 0;
    while (n < size && gStreamAt < gStreamSize) {
        ((uint8_t *)dst)[n++] = gStream[gStreamAt++];
    }
    return n;
}

static void *BMX_CDECL stReadInt32(void *stream, void *ints, uint32_t size)
{
    rwtLog(0x4e3270);
    rwtLog(size);
    if (streamFails()) {
        return NULL;
    }
    return streamIn(ints, size) == size ? stream : NULL;
}

static void *BMX_CDECL stReadReal(void *stream, void *reals, uint32_t size)
{
    rwtLog(0x4e3200);
    rwtLogPtr(reals);
    rwtLog(size);
    if (streamFails()) {
        return NULL;
    }
    return streamIn(reals, size) == size ? stream : NULL;
}

static uint32_t BMX_CDECL stRead(void *stream, void *buffer, uint32_t size)
{
    rwtLog(0x4e2800);
    rwtLog(size);
    if (streamFails()) {
        return 0;
    }
    return streamIn(buffer, size);
}

static void *BMX_CDECL stWrite(void *stream, const void *data, uint32_t size)
{
    rwtLog(0x4e29d0);
    rwtLog(size);
    for (uint32_t i = 0; i < size && i < 64; i++) {
        rwtLog(((const uint8_t *)data)[i]);
    }
    return streamFails() ? NULL : stream;
}

static void *BMX_CDECL stWriteWords(void *stream, const void *data, uint32_t size)
{
    rwtLog(0x4e3130);
    rwtLog(size);
    for (uint32_t i = 0; i < size / 4 && i < 32; i++) {
        rwtLog(((const uint32_t *)data)[i]);
    }
    return streamFails() ? NULL : stream;
}

static void streamStubs(void)
{
    dt_stub(0x004e3270, NULL, (void *)stReadInt32);
    dt_stub(0x004e3200, NULL, (void *)stReadReal);
    dt_stub(0x004e2800, NULL, (void *)stRead);
    dt_stub(0x004e29d0, NULL, (void *)stWrite);
    dt_stub(0x004e3130, NULL, (void *)stWriteWords);
    dt_stub(0x004e3040, NULL, (void *)stWriteWords);
    gStreamAt = 0;
    gStreamSize = 0;
    gStreamCalls = 0;
    gStreamFailAt = dt_u32() % 3 == 0 ? dt_range(1, 12) : 0;
}

static void putWord(uint32_t w)
{
    if (gStreamSize + 4 <= kStreamBytes) {
        __builtin_memcpy(gStream + gStreamSize, &w, 4);
        gStreamSize += 4;
    }
}

/* A sequence in stream form: name, type, counts, keyframes and keys (now and then cut short). */
static void putSequence(int32_t type)
{
    const char *name = kNames[dt_u32() % 6];
    const uint32_t length = (uint32_t)tStrlen(name) + (dt_u32() % 4 == 0 ? 0 : 1);
    putWord(length);
    for (uint32_t i = 0; i < length; i++) {
        gStream[gStreamSize++] = (uint8_t)name[i];
    }
    if (dt_u32() % 10 == 0) {
        type = dt_range(0, 3);
    }
    const int32_t frames = dt_range(0, 3);
    /* another type leaves the sequence without keys, which the original then writes through NULL */
    const int32_t keys = type == 1 || type == 2 ? dt_range(0, 3) : 0;
    putWord((uint32_t)type);
    putWord((uint32_t)frames);
    putWord((uint32_t)keys);
    for (int32_t i = 0; i < frames * (type == 2 ? 4 : 3); i++) {
        putWord(dt_u32());
    }
    for (int32_t i = 0; i < keys; i++) {
        putWord((uint32_t)dt_range(0, 3));
        putWord((uint32_t)dt_range(0, 3));
        putWord(dt_u32());
    }
}

static RpAnimFrameSequence *seqReadOrig(void *stream)
{
    return REG1(0x004fcf20, RpAnimFrameSequence *, void *)(stream);
}

static RpAnimFrameSequence *seqReadImpl(void *stream)
{
    return rpAnimSequenceRead(stream);
}

/* Memory failures only where the original survives them: the sequence block itself. */
static void setup_seq_read(DtCtx *c)
{
    animEnv(c);
    streamStubs();
    putSequence(dt_range(1, 2));
    gRwtMallocFailAt = dt_u32() % 8 == 0 ? 1 : 0;
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    rwtEnd();
}

static void setup_frame_read(DtCtx *c)
{
    animEnv(c);
    streamStubs();
    putWord(dt_u32());
    putWord(dt_u32() % 4 == 0 ? 0 : 1);
    putWord((uint32_t)dt_range(-2, 3));
    const int32_t numA = dt_range(0, 2);
    const int32_t numB = dt_range(0, 2);
    putWord((uint32_t)numA);
    putWord((uint32_t)numB);
    for (int32_t i = 0; i < numA; i++) {
        putSequence(1);
    }
    for (int32_t i = 0; i < numB; i++) {
        putSequence(2);
    }
    if (gStreamFailAt == 0 && dt_u32() % 4 == 0) {
        gStreamSize -= (uint32_t)dt_range(1, 8); /* cut short */
    }
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, dt_range(0, 100));
    DT_ARG_PTR(c, 2, mkFrame(dt_u32() % 2));
    rwtEnd();
}

typedef void *(__attribute__((regparm(3))) * WriteKeysFn)(const void *, int32_t, void *, uint32_t, uint32_t,
                                                          uint32_t, int32_t *);

static void *writeRotOrig(const void *f, int32_t n, void *s, int32_t *size)
{
    return ((WriteKeysFn)(uintptr_t)0x004fbb30)(f, n, s, 0, 0, 0, size);
}

static void *writeRotImpl(const void *f, int32_t n, void *s, int32_t *size)
{
    return rpAnimWriteRotations(f, n, s, 0, 0, 0, size);
}

static void *writeTransOrig(const void *f, int32_t n, void *s, int32_t *size)
{
    return ((WriteKeysFn)(uintptr_t)0x004fbb70)(f, n, s, 0, 0, 0, size);
}

static void *writeTransImpl(const void *f, int32_t n, void *s, int32_t *size)
{
    return rpAnimWriteTranslations(f, n, s, 0, 0, 0, size);
}

static void setup_write_keys(DtCtx *c)
{
    animEnv(c);
    streamStubs();
    gStreamFailAt = dt_u32() % 4 == 0 ? 1 : 0;
    const int32_t n = dt_range(0, 2);
    float *frames = dt_alloc(32 + 1);
    for (int i = 0; i < 8; i++) {
        frames[i] = dt_float(-1.0f, 1.0f);
    }
    int32_t *size = dt_alloc(4);
    *size = (int32_t)dt_u32();
    DT_ARG_PTR(c, 0, frames);
    DT_ARG_INT(c, 1, n);
    DT_ARG_PTR(c, 2, dt_alloc(0x20));
    DT_ARG_PTR(c, 3, size);
    rwtEnd();
}

typedef RpAnimFrameSequence *(__attribute__((regparm(3))) * SeqWriteFn)(RpAnimFrameSequence *, void *,
                                                                        int32_t *);

static RpAnimFrameSequence *seqWriteOrig(RpAnimFrameSequence *s, void *stream, int32_t *size)
{
    return ((SeqWriteFn)(uintptr_t)0x004fd400)(s, stream, size);
}

static RpAnimFrameSequence *seqWriteImpl(RpAnimFrameSequence *s, void *stream, int32_t *size)
{
    return rpAnimSequenceWrite(s, stream, size);
}

static void setup_seq_write(DtCtx *c)
{
    animEnv(c);
    streamStubs();
    RpAnimFrameSequence *s = mkSeq(dt_range(1, 2));
    if (dt_u32() % 8 == 0) {
        s->type = dt_range(-1, 3);
    }
    if (dt_u32() % 8 == 0) {
        s->keys = NULL;
        s->numKeys = 0;
    }
    int32_t *size = dt_alloc(4);
    *size = (int32_t)dt_u32();
    DT_ARG_PTR(c, 0, s);
    DT_ARG_PTR(c, 1, dt_alloc(0x20));
    DT_ARG_PTR(c, 2, size);
    rwtEnd();
}

static void setup_frame_write(DtCtx *c)
{
    animEnv(c);
    streamStubs();
    gStreamFailAt = dt_u32() % 3 == 0 ? dt_range(1, 30) : 0;
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, dt_range(0, 100));
    DT_ARG_PTR(c, 2, mkFrame(1));
    rwtEnd();
}

/* ---- playback
 * ----------------------------------------------------------------------------------------------- */

static double *mkDouble(double v)
{
    double *p = dt_alloc(8);
    *p = v;
    return p;
}

/* EAX = instance, EDX = direction, ST0 = time. */
static uint32_t resetCall(uint32_t target, RpAnimInstance *instance, int32_t forward, const double *t)
{
    uint32_t eax = (uint32_t)(uintptr_t)instance, edx = (uint32_t)forward;
    __asm__ volatile("fldl (%2)\n\tcall *%3"
                     : "+a"(eax), "+d"(edx)
                     : "r"(t), "r"(target)
                     : "memory", "cc", "st", "ecx");
    return eax;
}

static uint32_t resetOrig(RpAnimInstance *i, int32_t f, const double *t)
{
    return resetCall(0x004fbf30, i, f, t);
}

static uint32_t resetImpl(RpAnimInstance *i, int32_t f, const double *t)
{
    return resetCall((uint32_t)(uintptr_t)rpAnimInstanceReset, i, f, t);
}

static RpAnimInstance *mkInstance(RpAnimFrameSequence *s)
{
    RpAnimInstance *i = dt_alloc(sizeof *i);
    i->sequence = s;
    i->keys = s->keys + 12 * dt_range(0, s->numKeys - 1);
    i->time = animTime();
    return i;
}

static void setup_reset(DtCtx *c)
{
    animEnv(c);
    DT_ARG_PTR(c, 0, mkInstance(mkSeq(dt_range(1, 2))));
    DT_ARG_INT(c, 1, dt_range(0, 2));
    DT_ARG_PTR(c, 2,
               mkDouble(dt_u32() % 4 == 0 ? (double)dt_float_edgy(-5.0f, 5.0f) : dt_float(0.0f, 3.0f) / 3.0));
    rwtEnd();
}

static float quatComponent(void)
{
    return dt_u32() % 12 == 0 ? dt_float_edgy(-2.0f, 2.0f) : dt_float(-1.0f, 1.0f);
}

static void setup_slerp(DtCtx *c)
{
    animEnv(c);
    float *q0 = dt_alloc(16);
    float *q1 = dt_alloc(16);
    for (int i = 0; i < 4; i++) {
        q0[i] = quatComponent();
        q1[i] = quatComponent();
    }
    switch (dt_u32() % 6) {
    case 0: /* the same: linear */
        __builtin_memcpy(q1, q0, 16);
        break;
    case 1: /* opposite */
        for (int i = 0; i < 4; i++) {
            q1[i] = -q0[i];
        }
        break;
    case 2: /* a little apart */
        for (int i = 0; i < 4; i++) {
            q1[i] = q0[i] + dt_float(-0.002f, 0.002f);
        }
        break;
    default:
        break;
    }
    DT_ARG_PTR(c, 0, q0);
    DT_ARG_PTR(c, 1, q1);
    DT_ARG_PTR(c, 2, dt_alloc(sizeof(RpAnimSlerpCache)));
    rwtEnd();
}

static RwFrame *BMX_CDECL stTranslate(RwFrame *frame, const RwV3d *v, int32_t combine)
{
    rwtLog(0x4d2f40);
    rwtLogPtr(frame);
    rwtLogFloatBits(v->x);
    rwtLogFloatBits(v->y);
    rwtLogFloatBits(v->z);
    rwtLog((uint32_t)combine);
    return frame;
}

/* A node for the frame with instances of a translation and / or a rotation sequence. */
static RpAnimNode *mkNode(RwFrame *frame)
{
    RpAnimNode *node = dt_alloc(sizeof *node);
    node->frame = frame;
    node->state = dt_range(0, 3);
    node->a = dt_u32() % 4 == 0 ? NULL : mkInstance(mkSeq(1));
    node->b = dt_u32() % 4 == 0 ? NULL : mkInstance(mkSeq(2));
    float *cache = (float *)&node->cache;
    for (int i = 0; i < 9; i++) {
        cache[i] = dt_float(-2.0f, 2.0f);
    }
    node->cache.linear = (int32_t)(dt_u32() % 2);
    return node;
}

static RwFrame *mkPoseFrame(void)
{
    RwFrame *f = mkFrame(0);
    float *m = (float *)&f->modelling;
    for (int i = 0; i < 16; i++) {
        m[i] = dt_float(-3.0f, 3.0f);
    }
    f->modelling.flags = dt_u32();
    return f;
}

static double stepTime(void)
{
    switch (dt_u32() % 8) {
    case 0:
        return 0.0;
    case 1:
        return -(double)dt_float(0.0f, 2.0f);
    case 2:
        return (double)dt_float(2.0f, 8.0f);
    default:
        return (double)dt_float(0.0f, 1.5f) / 3.0;
    }
}

/* EAX = instance, EDX = interpolator, ECX = frame, 4th stack slot = direction, ST0 = dt. */
static uint32_t rotationCall(uint32_t target, RpAnimInstance *instance, RpAnimInterpolator *interp,
                             RwFrame *frame, int32_t forward, const double *dt)
{
    uint32_t eax = (uint32_t)(uintptr_t)instance, edx = (uint32_t)(uintptr_t)interp,
             ecx = (uint32_t)(uintptr_t)frame;
    __asm__ volatile("pushl %3\n\t"
                     "subl $12, %%esp\n\t"
                     "fldl (%4)\n\t"
                     "call *%5\n\t"
                     "addl $16, %%esp"
                     : "+a"(eax), "+d"(edx), "+c"(ecx)
                     : "r"(forward), "r"(dt), "r"(target)
                     : "memory", "cc", "st");
    return eax;
}

static uint32_t rotationOrig(RpAnimInstance *i, RpAnimInterpolator *p, RwFrame *f, int32_t d,
                             const double *dt)
{
    return rotationCall(0x004fbf60, i, p, f, d, dt);
}

static uint32_t rotationImpl(RpAnimInstance *i, RpAnimInterpolator *p, RwFrame *f, int32_t d,
                             const double *dt)
{
    return rotationCall((uint32_t)(uintptr_t)rpAnimRotationUpdate, i, p, f, d, dt);
}

static void setup_rotation(DtCtx *c)
{
    animEnv(c);
    dt_stub(0x004d2f40, NULL, (void *)stTranslate);
    RwFrame *frame = mkPoseFrame();
    RpAnimNode *node = mkNode(frame);
    if (node->b == NULL) {
        node->b = mkInstance(mkSeq(2));
    }
    DT_ARG_PTR(c, 0, node->b);
    DT_ARG_PTR(c, 1, &node->state);
    DT_ARG_PTR(c, 2, frame);
    DT_ARG_INT(c, 3, dt_range(0, 2));
    DT_ARG_PTR(c, 4, mkDouble(stepTime()));
    rwtEnd();
}

/* EAX = interpolator, EDX = frame, ECX = direction, ST0 = dt. */
static uint32_t interpCall(uint32_t target, RpAnimInterpolator *interp, RwFrame *frame, int32_t forward,
                           const double *dt)
{
    uint32_t eax = (uint32_t)(uintptr_t)interp, edx = (uint32_t)(uintptr_t)frame, ecx = (uint32_t)forward;
    __asm__ volatile("fldl (%3)\n\tcall *%4"
                     : "+a"(eax), "+d"(edx), "+c"(ecx)
                     : "r"(dt), "r"(target)
                     : "memory", "cc", "st");
    return eax;
}

static uint32_t interpOrig(RpAnimInterpolator *p, RwFrame *f, int32_t d, const double *dt)
{
    return interpCall(0x004fc2e0, p, f, d, dt);
}

static uint32_t interpImpl(RpAnimInterpolator *p, RwFrame *f, int32_t d, const double *dt)
{
    return interpCall((uint32_t)(uintptr_t)rpAnimInterpolatorUpdate, p, f, d, dt);
}

static void setup_interp(DtCtx *c)
{
    animEnv(c);
    dt_stub(0x004d2f40, NULL, (void *)stTranslate);
    RwFrame *frame = mkPoseFrame();
    RpAnimNode *node = mkNode(frame);
    DT_ARG_PTR(c, 0, &node->state);
    DT_ARG_PTR(c, 1, frame);
    DT_ARG_INT(c, 2, dt_range(0, 2));
    DT_ARG_PTR(c, 3, mkDouble(stepTime()));
    rwtEnd();
}

static RpClump *BMX_CDECL cbSequenceEnd(RpClump *clump, RpAnimClumpSequence *sequence, float time, void *data)
{
    rwtLog(0xe4d);
    rwtLogPtr(clump);
    rwtLogPtr(sequence);
    rwtLogFloatBits(time);
    rwtLogPtr(data);
    return clump;
}

/*
 * A clump with sequences in its first list (names may repeat) and some in the running list; their nodes
 * animate frames with sequences of their own. The sequence started is one of them or an unknown name.
 */
static RpClump *mkAnimClump(const char **name)
{
    RpClump *clump = dt_alloc(kClumpBytes + 0x20);
    RpAnimClumpData *d = (RpAnimClumpData *)((uint8_t *)clump + kClumpOffset);
    d->sequences = dt_alloc(sizeof(RpAnimClumpSequence));
    d->others = dt_alloc(sizeof(RpAnimClumpSequence));
    d->sequences->name = mkName();
    d->others->name = mkName();
    *name = kNames[dt_u32() % 6];
    for (int list = 0; list < 2; list++) {
        RpAnimClumpSequence *tail = list == 0 ? d->sequences : d->others;
        for (int n = dt_range(0, 3); n > 0; n--) {
            RpAnimClumpSequence *s = dt_alloc(sizeof *s);
            s->name = mkName();
            s->duration = dt_u32() % 8 == 0 ? (float)dt_range(1, 3) * 0.5f : dt_float(0.3f, 3.0f);
            s->time = animTime();
            s->direction = dt_range(0, 1);
            s->priority = dt_range(-1, 3);
            s->callBack = dt_u32() % 3 == 0 ? (void *)rpAnimClumpDefaultCallBack : (void *)cbSequenceEnd;
            s->data = dt_alloc(4);
            RpAnimNode **at = &s->nodes;
            for (int k = dt_range(0, 2); k > 0; k--) {
                RwFrame *frame = mkPoseFrame();
                RpAnimNode *node = mkNode(frame);
                *at = node;
                at = &node->next;
            }
            tail->next = s;
            tail = s;
        }
    }
    return clump;
}

/* EAX = clump, EDX = name, ECX = direction, 4th stack slot = priority, ST0 = time. */
static uint32_t startCall(uint32_t target, RpClump *clump, const char *name, int32_t forward,
                          int32_t priority, const double *t)
{
    uint32_t eax = (uint32_t)(uintptr_t)clump, edx = (uint32_t)(uintptr_t)name, ecx = (uint32_t)forward;
    __asm__ volatile("pushl %3\n\t"
                     "subl $12, %%esp\n\t"
                     "fldl (%4)\n\t"
                     "call *%5\n\t"
                     "addl $16, %%esp"
                     : "+a"(eax), "+d"(edx), "+c"(ecx)
                     : "r"(priority), "r"(t), "r"(target)
                     : "memory", "cc", "st");
    return eax;
}

static uint32_t startOrig(RpClump *clump, const char *name, int32_t forward, int32_t priority,
                          const double *t)
{
    return startCall(0x004ff960, clump, name, forward, priority, t);
}

static uint32_t startImpl(RpClump *clump, const char *name, int32_t forward, int32_t priority,
                          const double *t)
{
    return startCall((uint32_t)(uintptr_t)rpAnimClumpStartSequence, clump, name, forward, priority, t);
}

static double startTime(void)
{
    switch (dt_u32() % 6) {
    case 0:
        return 0.0;
    case 1:
        return (double)dt_float(3.0f, 9.0f); /* past the end */
    default:
        return (double)dt_float(0.0f, 3.0f) / 3.0;
    }
}

static void setup_start(DtCtx *c)
{
    animEnv(c);
    dt_stub(0x004d2f40, NULL, (void *)stTranslate);
    const char *name;
    RpClump *clump = mkAnimClump(&name);
    DT_ARG_PTR(c, 0, clump);
    DT_ARG_PTR(c, 1, name);
    DT_ARG_INT(c, 2, dt_range(0, 2));
    DT_ARG_INT(c, 3, dt_range(-1, 3));
    DT_ARG_PTR(c, 4, mkDouble(startTime()));
    rwtEnd();
}

static void setup_default_cb(DtCtx *c)
{
    animEnv(c);
    dt_stub(0x004d2f40, NULL, (void *)stTranslate);
    const char *name;
    RpClump *clump = mkAnimClump(&name);
    RpAnimClumpData *d = (RpAnimClumpData *)((uint8_t *)clump + kClumpOffset);
    RpAnimClumpSequence *s = d->sequences->next != NULL && dt_u32() % 4 != 0 ? d->sequences->next : d->others;
    if (s == d->others && s->next != NULL) {
        s = s->next;
    }
    s->duration = dt_float(0.3f, 3.0f);
    DT_ARG_PTR(c, 0, clump);
    DT_ARG_PTR(c, 1, s);
    DT_ARG_FLOAT(c, 2, (float)startTime());
    DT_ARG_PTR(c, 3, dt_alloc(4));
    rwtEnd();
}

/* ---- bone plugin
 * ---------------------------------------------------------------------------------------------- */

static RwMatrix *BMX_CDECL stTransform(RwMatrix *m, const RwMatrix *t, int32_t combine)
{
    rwtLog(0x4d16f0);
    rwtLogPtr(m);
    rwtLogWords(m, 16);
    rwtLogPtr(t);
    rwtLog((uint32_t)combine);
    m->right.x += 1.0f;
    return m;
}

static RwMatrix gBoneLtm;

static RwMatrix *BMX_CDECL stLtm(RwFrame *frame)
{
    rwtLog(0x4d2b10);
    rwtLogPtr(frame);
    return &gBoneLtm;
}

static RwFrame *BMX_CDECL stUpdateObjects(RwFrame *frame)
{
    rwtLog(0x4d2a80);
    rwtLogPtr(frame);
    return frame;
}

static void setup_bone_matrices(DtCtx *c)
{
    animEnv(c);
    rpBoneFrameOffset = 0xb4;
    dt_stub(0x004d2eb0, NULL, (void *)stChildren);
    dt_stub(0x004d16f0, NULL, (void *)stTransform);
    dt_stub(0x004d2b10, NULL, (void *)stLtm);
    dt_stub(0x004d2a80, NULL, (void *)stUpdateObjects);
    RwFrame *frame = mkFrame(0);
    frame->root = mkFrame(0);
    frame->root->object.privateFlags = (uint8_t)dt_range(0, 4);
    if (dt_u32() % 4 != 0) {
        RpSkinBone *bone = dt_alloc(sizeof *bone);
        uint32_t *words = (uint32_t *)bone;
        for (int i = 0; i < 0x22; i++) {
            words[i] = dt_u32();
        }
        bone->frame = frame;
        *(RpSkinBone **)((uint8_t *)frame + 0xb4) = bone;
    }
    RpSkinChildSearch *walk = dt_alloc(sizeof *walk);
    walk->reserved = dt_u32();
    walk->parentInverse = dt_alloc(sizeof(RwMatrix));
    walk->zero = dt_u32();
    DT_ARG_PTR(c, 0, frame);
    DT_ARG_PTR(c, 1, walk);
    rwtEnd();
}

static void setup_bone_read(DtCtx *c)
{
    animEnv(c);
    streamStubs();
    rpBoneGeometryOffset = 0x40;
    const int32_t blocks = dt_range(0, 4);
    for (int32_t i = 0; i < blocks * 3; i++) {
        putWord(dt_u32());
    }
    gRwtMallocFailAt = dt_u32() % 4 == 0 ? dt_range(1, 2) : 0;
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, blocks * 12);
    DT_ARG_PTR(c, 2, dt_alloc(0x60));
    rwtEnd();
}

DT_CASES(rwanim_cases) = {
    {"rpAnimSequenceDeinit", 0x00502a40, rpAnimSequenceDeinit, 1, DT_RET_INT, setup_seq_deinit, 200, 0},
    {"rpAnimSequenceInit", 0x00502580, rpAnimSequenceInit, 5, DT_RET_INT, setup_seq_init, 500, 0},
    {"rpAnimSequenceCreate", (uint32_t)(uintptr_t)seqCreateOrig, seqCreateImpl, 4, DT_RET_INT,
     setup_seq_create, 200, 0},
    {"rpAnimSequenceRelease", (uint32_t)(uintptr_t)seqReleaseOrig, seqReleaseImpl, 1, DT_RET_INT,
     setup_seq_release, 100, 0},
    {"rpAnimFrameDataDestroy", (uint32_t)(uintptr_t)dataDestroyOrig, dataDestroyImpl, 1, DT_RET_INT,
     setup_data_destroy, 200, 0},
    {"rpAnimFrameDeinit", 0x004fbd00, rpAnimFrameDeinit, 1, DT_RET_INT, setup_frame_deinit, 200, 0},
    {"rpAnimFrameAddSequence", (uint32_t)(uintptr_t)frameAddOrig, frameAddImpl, 2, DT_RET_INT,
     setup_frame_add, 400, 0},
    {"rpAnimFrameCopy", 0x004fbd50, rpAnimFrameCopy, 2, DT_RET_INT, setup_frame_copy, 400, 0},
    {"rpAnimClumpFindNodes", 0x004fc540, rpAnimClumpFindNodes, 2, DT_RET_INT, setup_find_nodes, 600, 0},
    {"rpAnimClumpInit", 0x004fc740, rpAnimClumpInit, 1, DT_RET_INT, setup_clump_init, 100, 0},
    {"rpAnimClumpDataInit", 0x005027c0, rpAnimClumpDataInit, 1, DT_RET_INT, setup_clump_data_init, 100, 0},
    {"rpAnimClumpDeinit", 0x004fc760, rpAnimClumpDeinit, 1, DT_RET_INT, setup_clump_deinit, 200, 0},
    {"rpAnimClumpDataDeinit", 0x005026a0, rpAnimClumpDataDeinit, 1, DT_RET_INT, setup_clump_data_deinit, 200,
     0},
    {"rpAnimSequenceRead", (uint32_t)(uintptr_t)seqReadOrig, seqReadImpl, 1, DT_RET_INT, setup_seq_read, 600,
     0},
    {"rpAnimFrameChunkRead", 0x004fcdd0, rpAnimFrameChunkRead, 3, DT_RET_INT, setup_frame_read, 800, 0},
    {"rpAnimWriteRotations", (uint32_t)(uintptr_t)writeRotOrig, writeRotImpl, 4, DT_RET_INT, setup_write_keys,
     100, 0},
    {"rpAnimWriteTranslations", (uint32_t)(uintptr_t)writeTransOrig, writeTransImpl, 4, DT_RET_INT,
     setup_write_keys, 100, 0},
    {"rpAnimSequenceWrite", (uint32_t)(uintptr_t)seqWriteOrig, seqWriteImpl, 3, DT_RET_INT, setup_seq_write,
     500, 0},
    {"rpAnimFrameChunkWrite", 0x004fd280, rpAnimFrameChunkWrite, 3, DT_RET_INT, setup_frame_write, 600, 0},
    {"rpAnimInstanceReset", (uint32_t)(uintptr_t)resetOrig, resetImpl, 3, DT_RET_INT, setup_reset, 200, 0},
    {"rpAnimSlerpCacheSetup", 0x0051ce10, rpAnimSlerpCacheSetup, 3, DT_RET_VOID, setup_slerp, 1500, 0},
    {"rpAnimRotationUpdate", (uint32_t)(uintptr_t)rotationOrig, rotationImpl, 5, DT_RET_INT, setup_rotation,
     1500, 0},
    {"rpAnimInterpolatorUpdate", (uint32_t)(uintptr_t)interpOrig, interpImpl, 4, DT_RET_INT, setup_interp,
     1500, 0},
    {"rpAnimClumpStartSequence", (uint32_t)(uintptr_t)startOrig, startImpl, 5, DT_RET_INT, setup_start, 1500,
     0},
    {"rpAnimClumpDefaultCallBack", 0x00502990, rpAnimClumpDefaultCallBack, 4, DT_RET_INT, setup_default_cb,
     800, 0},
    {"rpBoneUpdateMatrices", 0x00504340, rpBoneUpdateMatrices, 2, DT_RET_INT, setup_bone_matrices, 300, 0},
    {"rpBoneGeometryRead", 0x00505670, rpBoneGeometryRead, 3, DT_RET_INT, setup_bone_read, 300, 0},
};
