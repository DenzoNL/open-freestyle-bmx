/*
 * Tests for the scene database (zscene.c). Scene.Txt style text comes from a fake file device, the
 * renderer, the IFF reader, the asset library and the object table are replaced with logging stubs that
 * answer from the test's random stream.
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zfile.h"

#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwworld.h"
#include "zge/zasset.h"
#include "zge/zgemods/zfont.h"
#include "zge/zgemods/zobjlight.h"
#include "zge/zgemods/zscnpts.h"
#include "zge/zgemods/ztaginf.h"
#include "zge/ziff.h"
#include "zge/zobj.h"
#include "zge/zscene.h"
#include "zge/ztoken.h"

/* ---- stubs --------------------------------------------------------------------------------------- */

static uint8_t gFakeObjects[8][0x40];
static uint8_t gFakeRecord[0x80];
static int gAnswerPct = 80; /* chance of a stub answering success */

static int yes(void)
{
    return (int)(dt_u32() % 100) < gAnswerPct;
}

static void *fake_object(void)
{
    return gFakeObjects[dt_u32() % 8];
}

static int BMX_CDECL stub_obj_free(void *object)
{
    dtLog(0xe01);
    dtLog((uint32_t)(uintptr_t)object);
    return 1;
}

static void *BMX_CDECL stub_obj_model(const void *object)
{
    dtLog(0xe02);
    dtLog((uint32_t)(uintptr_t)object);
    return yes() ? fake_object() : NULL;
}

static void BMX_CDECL stub_release_model(void *model)
{
    dtLog(0xe03);
    dtLog((uint32_t)(uintptr_t)model);
}

static void *BMX_CDECL stub_model_from_object(void *model)
{
    dtLog(0xe04);
    dtLog((uint32_t)(uintptr_t)model);
    return yes() ? fake_object() : NULL;
}

static void *BMX_CDECL stub_create_object(void *model)
{
    dtLog(0xe05);
    dtLog((uint32_t)(uintptr_t)model);
    return yes() ? fake_object() : NULL;
}

static void *BMX_CDECL stub_load_model(void *source)
{
    dtLog(0xe06);
    dtLog((uint32_t)(uintptr_t)source);
    return yes() ? fake_object() : NULL;
}

static void *BMX_CDECL stub_load_chunk(void *iff)
{
    dtLog(0xe07);
    dtLog((uint32_t)(uintptr_t)iff);
    return yes() ? fake_object() : NULL;
}

static int BMX_CDECL stub_world_load(void *data, const char *directory)
{
    dtLog(0xe08);
    dtLog((uint32_t)(uintptr_t)data);
    dtLogStr(directory);
    return yes();
}

static void BMX_CDECL stub_world_close(void)
{
    dtLog(0xe09);
}

static uint8_t *BMX_CDECL stub_world_find(void *data, int kind, uint32_t *size)
{
    dtLog(0xe0a);
    dtLog((uint32_t)kind);
    if (!yes()) {
        return NULL;
    }
    *size = (uint32_t)dt_range(1, 64);
    *(uint32_t *)(gFakeRecord + 4) = yes();
    memcpy(gFakeRecord + 8, "comp.dat", 9);
    return gFakeRecord;
}

static void *BMX_CDECL stub_iff_open(void *file, uint32_t form, uint32_t size)
{
    dtLog(0xe10);
    dtLog(form);
    dtLog(size);
    return yes() ? (void *)(uintptr_t)0x20001000 : NULL;
}

static void BMX_CDECL stub_iff_close(void *iff)
{
    dtLog(0xe11);
}

static int gChunks;

static int BMX_CDECL stub_iff_find(void *iff, uint32_t id)
{
    dtLog(0xe12);
    dtLog(id);
    gChunks = dt_range(0, 3);
    return gChunks > 0;
}

static int BMX_CDECL stub_iff_next(void *iff, uint32_t id)
{
    dtLog(0xe13);
    dtLog(id);
    return --gChunks > 0;
}

static void *BMX_CDECL stub_read_anim(void *iff)
{
    dtLog(0xe14);
    return yes() ? fake_object() : NULL;
}

static uint32_t BMX_CDECL stub_anim_name(void *anim)
{
    dtLog(0xe15);
    dtLog((uint32_t)(uintptr_t)anim);
    return dt_u32();
}

static void BMX_CDECL stub_free_anim(void *anim)
{
    dtLog(0xe16);
    dtLog((uint32_t)(uintptr_t)anim);
}

static int BMX_CDECL stub_begin_model(void *iff)
{
    dtLog(0xe17);
    return yes();
}

static void BMX_CDECL stub_end_model(void *iff)
{
    dtLog(0xe18);
}

static int gLibRemaining;

static int BMX_CDECL stub_open_library(const char *name)
{
    dtLog(0xe20);
    dtLogStr(name);
    return yes() ? 0x7001 : 0;
}

static int BMX_CDECL stub_lib_count(int lib)
{
    gLibRemaining = dt_range(0, 3);
    dtLog(0xe21);
    dtLog((uint32_t)gLibRemaining);
    return gLibRemaining;
}

static uint32_t BMX_CDECL stub_lib_size(int lib)
{
    dtLog(0xe22);
    return (uint32_t)dt_range(8, 40);
}

static int BMX_CDECL stub_lib_read(int lib, void *dst)
{
    dtLog(0xe23);
    return yes();
}

static void BMX_CDECL stub_lib_next(int lib)
{
    dtLog(0xe24);
}

static void BMX_CDECL stub_lib_close(int lib)
{
    dtLog(0xe25);
}

static void *BMX_CDECL stub_light_create(int type, int param)
{
    dtLog(0xe30);
    dtLog((uint32_t)type);
    dtLog((uint32_t)param);
    return yes() ? fake_object() : NULL;
}

static void *BMX_CDECL stub_world_object(void)
{
    dtLog(0xe31);
    return yes() ? fake_object() : NULL;
}

static void BMX_CDECL stub_tag_release(void)
{
    dtLog(0xe32);
}

static void BMX_CDECL stub_free_point(void *point, void *arg)
{
    dtLog(0xe33);
    dtLog((uint32_t)(uintptr_t)point);
}

static void *BMX_CDECL stub_light_color(void *handle, const uint8_t *rgba)
{
    dtLog(0xe34);
    dtLog((uint32_t)(uintptr_t)handle);
    dtLog(*(const uint32_t *)rgba);
    return NULL;
}

static void *BMX_CDECL stub_light_flags(void *handle, uint8_t flags)
{
    dtLog(0xe35);
    dtLog((uint32_t)(uintptr_t)handle);
    dtLog(flags);
    return NULL;
}

static void *BMX_CDECL stub_light_dir(void *handle, const ZVec3 *dir)
{
    dtLog(0xe36);
    dtLog((uint32_t)(uintptr_t)handle);
    dtLog(*(const uint32_t *)&dir->x);
    dtLog(*(const uint32_t *)&dir->y);
    return NULL;
}

/* Scene component chunk handlers (the entries of the tables at 0x561900.. in the exe). */
static int BMX_CDECL stub_chunk_handler(void *iff)
{
    dtLog(0xe40);
    return yes();
}

static void stubs_on(void)
{
    dt_stub(0x00487400, NULL, (void *)stub_obj_free);
    dt_stub(0x00487440, NULL, (void *)stub_obj_model);
    dt_stub(0x004890b0, NULL, (void *)stub_release_model);
    dt_stub(0x004890c0, NULL, (void *)stub_model_from_object);
    dt_stub(0x00488fb0, NULL, (void *)stub_create_object);
    dt_stub(0x00489070, NULL, (void *)stub_load_model);
    dt_stub(0x00489090, NULL, (void *)stub_load_chunk);
    dt_stub(0x00496ff0, NULL, (void *)stub_world_load);
    dt_stub(0x00497200, NULL, (void *)stub_world_close);
    dt_stub(0x004962e0, NULL, (void *)stub_world_find);
    dt_stub(0x0049a790, NULL, (void *)stub_iff_open);
    dt_stub(0x0049a8a0, NULL, (void *)stub_iff_close);
    dt_stub(0x0049ad90, NULL, (void *)stub_iff_find);
    dt_stub(0x0049aed0, NULL, (void *)stub_iff_next);
    dt_stub(0x0049a4e0, NULL, (void *)stub_read_anim);
    dt_stub(0x0049a530, NULL, (void *)stub_anim_name);
    dt_stub(0x0049a630, NULL, (void *)stub_free_anim);
    dt_stub(0x0049b130, NULL, (void *)stub_begin_model);
    dt_stub(0x0049b250, NULL, (void *)stub_end_model);
    dt_stub(0x004804d0, NULL, (void *)stub_open_library);
    dt_stub(0x00480720, NULL, (void *)stub_lib_count);
    dt_stub(0x00480850, NULL, (void *)stub_lib_size);
    dt_stub(0x00480930, NULL, (void *)stub_lib_read);
    dt_stub(0x00480730, NULL, (void *)stub_lib_next);
    dt_stub(0x004806c0, NULL, (void *)stub_lib_close);
    dt_stub(0x00490010, NULL, (void *)stub_light_create);
    dt_stub(0x00495c10, NULL, (void *)stub_world_object);
    dt_stub(0x0048d8d0, NULL, (void *)stub_tag_release);
    dt_stub(0x00491f10, NULL, (void *)stub_free_point);
    dt_stub(0x0049f720, NULL, (void *)stub_light_color);
    dt_stub(0x0049f7a0, NULL, (void *)stub_light_flags);
    dt_stub(0x0049f840, NULL, (void *)stub_light_dir);
    dt_stub(0x0048dae0, NULL, (void *)stub_chunk_handler);
    dt_stub(0x00491f50, NULL, (void *)stub_chunk_handler);
    dt_stub(0x004a10a0, NULL, (void *)stub_chunk_handler);
    dt_stub(0x00486a20, NULL, (void *)stub_chunk_handler);
    dt_stub(0x00486be0, NULL, (void *)stub_chunk_handler);
}

/* ---- fixtures ------------------------------------------------------------------------------------ */

#define ORIG_ALLOC(list) DT_ORIG(0x00486050, void *(*)(ZMemFreeList *))(list)
#define ORIG_HASH(s)     DT_ORIG(0x004828c0, int (*)(const char *))(s)

static const char *const kNames[] = {"bike", "rider", "track", "ramp", "sky", "shadow", "wheel", "pegs"};

static ZsceneNode *new_node(int type, const char *name, const char *file)
{
    ZsceneNode *n = ORIG_ALLOC(gScene.nodes);
    memset(n, 0, sizeof *n);
    n->type = type;
    if (name) {
        memcpy(n->name, name, strlen(name) + 1);
        n->nameHash = (uint32_t)ORIG_HASH(name);
    }
    if (file) {
        memcpy(n->file, file, strlen(file) + 1);
    }
    return n;
}

static const char *const kFiles[] = {"bike.zmd",  "rider.ZMD", "motion.zal", "anim.zan",
                                     "level.zsc", "weird.xyz", "x.zmd",      "y.ZAL"};

/* Children (models, animations) hanging off a node. */
static void add_children(ZsceneNode *parent, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        const int type = dt_u32() & 1 ? kZsceneNodeModel : kZsceneNodeAnim;
        ZsceneNode *c = new_node(type, kNames[dt_u32() % 8], kFiles[dt_u32() % 8]);
        c->sibling = parent->child;
        parent->child = c;
    }
}

static void begin(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    dtFileReset();
    dtFileInstall();
    stubs_on();
    memset(gFakeObjects, 0, sizeof gFakeObjects);
    gAnswerPct = 80;
    memset(&gScene, 0, sizeof gScene);
    memset(&gScnpts, 0, sizeof gScnpts);
    memset(&gToken, 0, sizeof gToken);
    gPanorama = NULL;
    gScene.entries = DT_ORIG(0x00485e80, ZMemFreeList * (*)(uint32_t, uint32_t, void *))(0x10, 0x100, NULL);
    gScene.nodes = DT_ORIG(0x00485e80, ZMemFreeList * (*)(uint32_t, uint32_t, void *))(0x78, 0x100, NULL);
}

/* A scene tree: common node, two scenes, models and animations below them. */
static void make_tree(void)
{
    ZsceneNode *s0, *s1;
    gScene.common.type = dt_u32() % 8 == 0 ? 1 : 0;
    gScene.common.nameHash = (uint32_t)ORIG_HASH("common");
    add_children(&gScene.common, dt_range(0, 3));
    s0 = new_node(kZsceneNodeScene, "level1", "level1.zsc");
    s1 = new_node(kZsceneNodeScene, "level2", "dir\\level2.zsc");
    add_children(s0, dt_range(0, 4));
    add_children(s1, dt_range(0, 3));
    s0->sibling = s1;
    gScene.current = s0;
    gScene.initialized = 1;
}

/* Registered objects: random types, kinds and scene nodes. */
static void make_entries(int count)
{
    int i;
    for (i = 0; i < count; i++) {
        ZsceneEntry *e = ORIG_ALLOC(gScene.entries);
        const int type = dt_range(1, 5);
        e->node = dt_u32() % 3 == 0 ? NULL : gScene.current;
        e->id = (uint32_t)ORIG_HASH(kNames[dt_u32() % 8]);
        e->flags = (uint32_t)type |
                   ((uint32_t)(type == 1   ? dt_range(0, 1)
                               : type == 2 ? 2
                                           : 0)
                    << 8) |
                   (dt_u32() & 1 ? kZsceneEntryScoped : 0);
        if (type == 2) {
            /* animation library: a list of {name, anim, next} nodes from zmemAlloc */
            uint32_t *list = NULL;
            int n = dt_range(0, 3);
            while (n-- > 0) {
                uint32_t *a = DT_ORIG(0x00485aa0, void *(*)(uint32_t))(12);
                a[0] = dt_u32();
                a[1] = (uint32_t)(uintptr_t)fake_object();
                a[2] = (uint32_t)(uintptr_t)list;
                list = a;
            }
            e->object = list;
        } else {
            e->object = fake_object();
        }
    }
}

static void open_scene(void)
{
    gScene.open = gScene.current;
}

static void init_objects(void)
{
    int i;
    for (i = 0; i < 8; i++) {
        *(uint32_t *)(gFakeObjects[i] + 0x2c) = dt_u32() & 1 ? 0x30000000u + (uint32_t)i : 0;
    }
}

/* ---- text generators ----------------------------------------------------------------------------- */

static int put(char *t, int n, const char *s)
{
    while (*s) {
        t[n++] = *s++;
    }
    return n;
}

static int block(char *t, int n, int depth, int common)
{
    int children, i;
    n = put(t, n, "{\n");
    if (dt_u32() % 9 != 0) {
        n = put(t, n, "NAME ");
        n = put(t, n,
                dt_u32() % 25 == 0 ? "a_name_that_is_far_too_long_for_the_buffer" : kNames[dt_u32() % 8]);
        n = put(t, n, "\n");
    }
    if (dt_u32() % 3 != 0) {
        n = put(t, n, "FILE ");
        n = put(t, n,
                dt_u32() % 30 == 0
                    ? "a_file_name_that_is_far_too_long_for_the_buffer_0123456789abcdefghijkl.zmd"
                    : kFiles[dt_u32() % 8]);
        n = put(t, n, "\n");
    }
    if (dt_u32() % 3 == 0) {
        n = put(t, n, dt_u32() % 2 ? "TYPE 3\n" : "TYPE 0x10\n");
    }
    if (dt_u32() % 4 == 0) {
        n = put(t, n, "SUBTYPE 2\n");
    }
    children = depth < 2 ? dt_range(0, 2) : 0;
    for (i = 0; i < children; i++) {
        n = put(t, n, dt_u32() & 1 ? "MODEL " : "ANIM ");
        n = block(t, n, depth + 1, 0);
    }
    if (dt_u32() % 40 == 0) {
        n = put(t, n, "BOGUS 1\n");
    }
    if (common || dt_u32() % 25 != 0) {
        n = put(t, n, "}\n");
    }
    return n;
}

static char *scene_text(int withHeader)
{
    char *t = dt_alloc(4096);
    int n = 0, scenes = dt_range(0, 3), i;
    if (withHeader) {
        n = put(t, n, "SCENE_COMMON ");
        n = block(t, n, 0, 1);
    }
    for (i = 0; i < scenes; i++) {
        n = put(t, n, "SCENE ");
        n = block(t, n, 0, 0);
    }
    if (dt_u32() % 12 != 0) {
        n = put(t, n, "END\n");
    }
    t[n] = '\0';
    return t;
}

static int (*const orig_token_open)(const char *) = (int (*)(const char *))(uintptr_t)0x004a1120;

/* ---- setups -------------------------------------------------------------------------------------- */

static void setup_light(DtCtx *c)
{
    uint8_t *light;
    begin(c);
    init_objects();
    light = dt_alloc(0x40);
    *(uint32_t *)(light + 0x2c) = 0x20002222;
    DT_ARG_PTR(c, 0, light);
}

static void setup_light_color(DtCtx *c)
{
    setup_light(c);
    DT_ARG_PTR(c, 1, dt_alloc(8));
    dt_fill((void *)(uintptr_t)c->args[1], 8);
}

static void setup_light_flags(DtCtx *c)
{
    setup_light(c);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_light_dir(DtCtx *c)
{
    ZVec3 *v;
    setup_light(c);
    v = dt_alloc(sizeof *v);
    v->x = dt_float(-1, 1);
    v->y = dt_float(-1, 1);
    v->z = dt_float(-1, 1);
    DT_ARG_PTR(c, 1, v);
}

static void setup_free_node(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, new_node(1, "bike", "bike.zmd"));
}

static void setup_parse(DtCtx *c)
{
    char *t = dt_alloc(2048);
    int n = 0;
    begin(c);
    gAnswerPct = 100;
    n = block(t, n, 0, 0);
    t[n] = '\0';
    dtFileAdd("main.txt", t);
    orig_token_open("main.txt");
    gToken.upperCase = 1;
    gToken.macros = 1;
    /* the block text starts with "{": the parser expects it as its first token */
    DT_ARG_PTR(c, 0, c->seed % 7 == 3 ? &gScene.common : new_node(dt_range(0, 2), NULL, NULL));
}

static void setup_load_db(DtCtx *c)
{
    begin(c);
    dtFileAdd("scene.txt", scene_text(c->seed % 17 != 5));
    DT_ARG_PTR(c, 0, c->seed % 11 == 4 ? "missing.txt" : "scene.txt");
}

static void setup_find_node(DtCtx *c)
{
    int32_t *q = dt_alloc(16);
    begin(c);
    make_tree();
    q[0] = dt_range(0, 2);
    q[1] = dt_u32() % 4 == 0 ? 0 : (int32_t)ORIG_HASH(kNames[dt_u32() % 8]);
    DT_ARG_PTR(c, 0,
               dt_u32() % 2          ? gScene.current
               : gScene.common.child ? gScene.common.child
                                     : gScene.current);
    DT_ARG_PTR(c, 1, q);
}

static void setup_entry_match(DtCtx *c)
{
    int32_t *q = dt_alloc(16);
    ZsceneEntry *e;
    begin(c);
    make_tree();
    make_entries(1);
    e = zmemFreeNodePayload(zmemFreeListFirst(gScene.entries));
    q[0] = dt_u32() % 2 ? (int8_t)e->flags : dt_range(0, 5);
    q[1] = dt_u32() % 2 ? (int32_t)e->id : (int32_t)dt_u32();
    DT_ARG_PTR(c, 0, e);
    DT_ARG_PTR(c, 1, q);
}

static void setup_add_entry(DtCtx *c)
{
    begin(c);
    make_tree();
    if (c->seed % 4 != 3) {
        open_scene();
    }
    DT_ARG_PTR(c, 0, gScene.current);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
    DT_ARG_PTR(c, 4, fake_object());
}

/* Chunk handler table for zsceneLoadChunks */
static int BMX_CDECL test_chunk(void *iff)
{
    dtLog(0xe41);
    return dt_u32() % 5 != 0;
}

static struct {
    uint32_t id;
    int(BMX_CDECL *handler)(void *iff);
} gChunkTable[3];

static void setup_load_chunks(DtCtx *c)
{
    int i;
    begin(c);
    gAnswerPct = 85;
    dtFileAdd("comp.dat", "chunk data");
    for (i = 0; i < 3; i++) {
        gChunkTable[i].id = dt_u32();
        gChunkTable[i].handler = test_chunk;
    }
    DT_ARG_PTR(c, 0, (void *)0x20003000);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, c->seed % 3 == 0 ? "tx00:" : "");
    DT_ARG_INT(c, 3, dt_range(0, 6));
    DT_ARG_INT(c, 4, dt_range(0, 3));
    DT_ARG_PTR(c, 5, gChunkTable);
}

static void setup_release(DtCtx *c)
{
    begin(c);
    init_objects();
    make_tree();
    make_entries(dt_range(0, 7));
    if (c->seed % 3 == 0) {
        open_scene();
    }
    DT_ARG_INT(c, 0, c->seed % 4 == 1);
}

static void setup_forget(DtCtx *c)
{
    ZMemFreeNode *n;
    ZsceneEntry *e = NULL;
    begin(c);
    make_tree();
    make_entries(dt_range(1, 6));
    n = zmemFreeListFirst(gScene.entries);
    while (n && dt_u32() % 2) {
        n = zmemFreeNodeNext(n);
    }
    if (n) {
        e = zmemFreeNodePayload(n);
    }
    if (e && c->seed % 3 != 0) {
        DT_ARG_INT(c, 0, (int8_t)e->flags);
        DT_ARG_INT(c, 1, (int8_t)(e->flags >> 8));
        DT_ARG_PTR(c, 2, e->object);
    } else {
        DT_ARG_INT(c, 0, dt_range(1, 5));
        DT_ARG_INT(c, 1, dt_range(0, 2));
        DT_ARG_PTR(c, 2, fake_object());
    }
    DT_ARG_PTR(c, 3, c->seed % 5 == 0 ? e : NULL);
}

static void setup_model_file(DtCtx *c)
{
    uint32_t *req = dt_alloc(16);
    begin(c);
    req[0] = (uint32_t)(uintptr_t)"bike.zmd";
    req[1] = dt_u32();
    req[2] = dt_u32();
    DT_ARG_PTR(c, 0, req);
}

static void setup_anim_file(DtCtx *c)
{
    uint32_t *req = dt_alloc(16);
    begin(c);
    dtFileAdd("anim.zan", "form data");
    req[0] = (uint32_t)(uintptr_t)(c->seed % 6 == 1 ? "missing.zan" : "anim.zan");
    req[1] = dt_u32();
    DT_ARG_PTR(c, 0, req);
}

static void setup_anim_lib(DtCtx *c)
{
    uint32_t *req = dt_alloc(16);
    begin(c);
    req[0] = (uint32_t)(uintptr_t)"motion.zal";
    req[1] = dt_u32();
    DT_ARG_PTR(c, 0, req);
}

static void setup_initialize(DtCtx *c)
{
    begin(c);
    gAnswerPct = 100;
    stub_tag_release();
    dt_stub(0x0049aad0, NULL, (void *)stub_tag_release); /* ziffSetupCallback: logs a bare call */
    dtFileAdd("scene.txt", scene_text(1));
    dtFileAdd("Scene.Txt", scene_text(1));
    memset(&gScene, 0, sizeof gScene);
    gScene.initialized = c->seed % 9 == 4; /* seed 4: "Already initialized" */
    DT_ARG_PTR(c, 0, c->seed % 3 == 0 ? NULL : "scene.txt");
}

static void setup_open(DtCtx *c)
{
    begin(c);
    make_tree();
    gScene.open = c->seed % 11 == 6 ? gScene.current : NULL;
    dtFileAdd("level1.zsc", "scene file");
    dtFileAdd("dir\\level2.zsc", "scene file 2");
    DT_ARG_PTR(c, 0,
               c->seed % 5 == 0   ? "nonesuch"
               : c->seed % 5 == 1 ? ""
               : c->seed % 2      ? "level1"
                                  : "level2");
}

static void setup_close(DtCtx *c)
{
    begin(c);
    init_objects();
    make_tree();
    make_entries(dt_range(0, 5));
    gScene.open = c->seed % 4 == 0 ? NULL : gScene.current;
    gScnpts.points = DT_ORIG(0x00485e80, ZMemFreeList * (*)(uint32_t, uint32_t, void *))(0x50, 0, NULL);
}

static void setup_find_model(DtCtx *c)
{
    begin(c);
    init_objects();
    make_tree();
    make_entries(dt_range(0, 5));
    if (c->seed % 5 != 4) {
        open_scene();
    }
    if (c->seed % 7 == 2) {
        gAnswerPct = 40;
    }
    DT_ARG_INT(c, 0, dt_u32() % 8);
    DT_ARG_PTR(c, 1, dt_u32() % 9 == 0 ? "" : kNames[dt_u32() % 8]);
}

static void setup_find_anim(DtCtx *c)
{
    setup_find_model(c);
}

static void setup_anim_table(DtCtx *c)
{
    uint32_t *lib = dt_alloc(16);
    uint32_t *list = NULL;
    int n = dt_range(0, 4);
    begin(c);
    while (n-- > 0) {
        uint32_t *a = dt_alloc(12);
        a[0] = dt_u32();
        a[1] = dt_u32();
        a[2] = (uint32_t)(uintptr_t)list;
        list = a;
    }
    lib[3] = (uint32_t)(uintptr_t)list;
    DT_ARG_PTR(c, 0, lib);
    DT_ARG_PTR(c, 1, dt_alloc(64));
    DT_ARG_INT(c, 2, dt_range(0, 6));
}

static void setup_add_light(DtCtx *c)
{
    begin(c);
    init_objects();
    make_tree();
    if (c->seed % 4 != 3) {
        open_scene();
    }
    DT_ARG_INT(c, 0, dt_u32() % 4);
    DT_ARG_INT(c, 1, dt_range(0, 3));
    DT_ARG_INT(c, 2, dt_range(0, 100));
}

static void setup_add_world(DtCtx *c)
{
    setup_add_light(c);
}

static void setup_scnpts(DtCtx *c)
{
    begin(c);
    dt_fill(&gScnpts, sizeof gScnpts);
    gScnpts.points = c->seed % 2
                         ? NULL
                         : DT_ORIG(0x00485e80, ZMemFreeList * (*)(uint32_t, uint32_t, void *))(0x50, 0, NULL);
}

static void setup_scnpts_free(DtCtx *c)
{
    uint32_t *node = dt_alloc(0x60);
    begin(c);
    node[0x48 / 4] = c->seed % 2 ? 0 : (uint32_t)(uintptr_t)DT_ORIG(0x00485aa0, void *(*)(uint32_t))(16);
    DT_ARG_PTR(c, 0, c->seed % 9 == 4 ? NULL : node);
}

static void setup_pano_init(DtCtx *c)
{
    begin(c);
    gPanorama = (void *)(uintptr_t)dt_u32();
}

static void setup_pano_chunk(DtCtx *c)
{
    begin(c);
    init_objects();
    make_tree();
    if (c->seed % 6 != 5) {
        open_scene();
    }
    DT_ARG_PTR(c, 0, (void *)0x20004000);
}

static void setup_nothing(DtCtx *c)
{
    begin(c);
}

DT_CASES(zscene_cases) = {
    {"zobjLightSetColor", 0x004900d0, zobjLightSetColor, 2, DT_RET_VOID, setup_light_color, 20, 0},
    {"zobjLightSetFlags", 0x004900f0, zobjLightSetFlags, 2, DT_RET_VOID, setup_light_flags, 50, 0},
    {"zobjLightSetDirection", 0x00490120, zobjLightSetDirection, 2, DT_RET_VOID, setup_light_dir, 50, 0},
    {"zsceneFreeNode", 0x004901b0, zsceneFreeNode, 1, DT_RET_INT, setup_free_node, 10, 0},
    {"zsceneParseNode", 0x00490230, zsceneParseNode, 1, DT_RET_INT, setup_parse, 500, 0},
    {"zsceneLoadDatabase", 0x004904e0, zsceneLoadDatabase, 1, DT_RET_INT, setup_load_db, 500, 0},
    {"zsceneFindNode", 0x00490630, zsceneFindNode, 2, DT_RET_INT, setup_find_node, 300, 0},
    {"zsceneEntryMatch", 0x004906a0, zsceneEntryMatch, 2, DT_RET_INT, setup_entry_match, 100, 0},
    {"zsceneAddEntry", 0x00490710, zsceneAddEntry, 5, DT_RET_INT, setup_add_entry, 100, 0},
    {"zsceneLoadChunks", 0x00490790, zsceneLoadChunks, 6, DT_RET_INT, setup_load_chunks, 400, 0},
    {"zsceneReleaseEntries", 0x004909d0, zsceneReleaseEntries, 1, DT_RET_VOID, setup_release, 400, 0},
    {"zsceneForgetEntry", 0x00490b70, zsceneForgetEntry, 4, DT_RET_INT, setup_forget, 400, 0},
    {"zsceneLoadModelFile", 0x00490cc0, zsceneLoadModelFile, 1, DT_RET_INT, setup_model_file, 50, 0},
    {"zsceneLoadAnimFile", 0x00490d90, zsceneLoadAnimFile, 1, DT_RET_INT, setup_anim_file, 200, 0},
    {"zsceneLoadAnimLibrary", 0x00490e90, zsceneLoadAnimLibrary, 1, DT_RET_INT, setup_anim_lib, 200, 0},
    {"zsceneInitialize", 0x00490ff0, zsceneInitialize, 1, DT_RET_INT, setup_initialize, 100, 0},
    {"zsceneOpen", 0x004911c0, zsceneOpen, 1, DT_RET_INT, setup_open, 300, 0},
    {"zsceneClose", 0x004913d0, zsceneClose, 0, DT_RET_INT, setup_close, 200, 0},
    {"zsceneFindModel", 0x00491410, zsceneFindModel, 2, DT_RET_INT, setup_find_model, 600, 0},
    {"zsceneFindAnim", 0x00491720, zsceneFindAnim, 2, DT_RET_INT, setup_find_anim, 600, 0},
    {"zsceneAnimTable", 0x00491a80, zsceneAnimTable, 3, DT_RET_INT, setup_anim_table, 200, 0},
    {"zsceneAddLight", 0x00491ba0, zsceneAddLight, 3, DT_RET_INT, setup_add_light, 200, 0},
    {"zsceneAddWorldObject", 0x00491c50, zsceneAddWorldObject, 1, DT_RET_INT, setup_add_world, 200, 0},
    {"zscnptsReset", 0x00491d80, zscnptsReset, 0, DT_RET_INT, setup_scnpts, 20, 0},
    {"zscnptsFreeNode", 0x00491dc0, zscnptsFreeNode, 1, DT_RET_INT, setup_scnpts_free, 20, 0},
    {"zscnptsInitialize", 0x00491df0, zscnptsInitialize, 0, DT_RET_INT, setup_scnpts, 10, 0},
    {"zscnptsShutdown", 0x00491ee0, zscnptsShutdown, 0, DT_RET_VOID, setup_close, 50, 0},
    {"zpanoramaInitialize", 0x004a1060, zpanoramaInitialize, 0, DT_RET_INT, setup_pano_init, 5, 0},
    {"zpanoramaShutdown", 0x004a1080, zpanoramaShutdown, 0, DT_RET_VOID, setup_nothing, 2, 0},
    {"zpanoramaLoadChunk", 0x004a10a0, zpanoramaLoadChunk, 1, DT_RET_INT, setup_pano_chunk, 100, 0},
};
