#include "dt.h"
#include "dt_zmem.h"
#include "zge/zlist.h"

/* List with `count` distinct non-NULL items in an array of `cap` slots, built with the original zmemAlloc. */
static ZList *make_list(int cap, int count, int cursor)
{
    ZList *list = dt_zmemalloc(sizeof *list);
    list->items = (void **)dt_zmemalloc((uint32_t)cap * 4);
    for (int i = 0; i < cap; i++) {
        list->items[i] = i < count ? (void *)(uintptr_t)(0x1000 + i * 0x10 + (int)(dt_u32() & 7)) : NULL;
    }
    list->count = count;
    list->capacity = cap;
    list->cursor = cursor;
    return list;
}

static ZList *random_list(void)
{
    const int cap = dt_range(1, 40);
    const int count = dt_range(0, cap);
    return make_list(cap, count, dt_range(-1, count));
}

static void setup_create(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    DT_ARG_INT(c, 0, dt_range(1, 70));
}

static void setup_destroy(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    DT_ARG_PTR(c, 0, random_list());
}

/* Visitor that records its calls and stops after `arg[0]` items. */
static int BMX_CDECL visit(void *item, void *arg)
{
    uint32_t *state = arg;
    state[1]++;
    state[2] += (uint32_t)(uintptr_t)item;
    return state[1] < state[0];
}

static void setup_clear(DtCtx *c)
{
    uint32_t *state;
    dt_seed(c);
    dt_zmem_env();
    state = dt_alloc(3 * sizeof *state);
    state[0] = (uint32_t)dt_range(1, 12);
    DT_ARG_PTR(c, 0, random_list());
    DT_ARG_PTR(c, 1, c->seed % 5 == 0 ? NULL : (void *)visit);
    DT_ARG_PTR(c, 2, state);
}

static void setup_list(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    DT_ARG_PTR(c, 0, random_list());
}

static void setup_get(DtCtx *c)
{
    ZList *list;
    dt_seed(c);
    dt_zmem_env();
    list = random_list();
    DT_ARG_PTR(c, 0, list);
    DT_ARG_INT(c, 1, dt_range(-1, list->count + 2));
}

static void setup_insert(DtCtx *c)
{
    ZList *list;
    dt_seed(c);
    dt_zmem_env();
    if (c->seed % 3 == 0) { /* full: exercises growth */
        const int cap = dt_range(1, 16);
        list = make_list(cap, cap, dt_range(-1, cap - 1));
    } else {
        list = random_list();
        if (list->count == list->capacity) {
            list->count--;
        }
    }
    DT_ARG_PTR(c, 0, list);
    DT_ARG_PTR(c, 1,
               c->seed == 4 ? NULL : (void *)(uintptr_t)(0x9000 + dt_u32() % 0x1000)); /* seed 4: assert */
}

static void setup_remove(DtCtx *c)
{
    ZList *list;
    dt_seed(c);
    dt_zmem_env();
    list = make_list(dt_range(1, 40), 1, -1);
    list->count = dt_range(1, list->capacity);
    for (int i = 0; i < list->count; i++) {
        list->items[i] = (void *)(uintptr_t)(0x1000 + i * 0x10 + (int)(dt_u32() & 7));
    }
    list->cursor = dt_range(-1, list->count - 1);
    DT_ARG_PTR(c, 0, list);
}

DT_CASES(zlist_cases) = {
    {"zlistRemoveCurrent", 0x0048c570, zlistRemoveCurrent, 1, DT_RET_INT, setup_remove, 300, 0},
    {"zlistCreate", 0x0048c260, zlistCreate, 1, DT_RET_INT, setup_create, 50, 0},
    {"zlistDestroy", 0x0048c2b0, zlistDestroy, 1, DT_RET_VOID, setup_destroy, 50, 0},
    {"zlistClear", 0x0048c2d0, zlistClear, 3, DT_RET_VOID, setup_clear, 200, 0},
    {"zlistFirst", 0x0048c330, zlistFirst, 1, DT_RET_INT, setup_list, 200, 0},
    {"zlistNext", 0x0048c360, zlistNext, 1, DT_RET_INT, setup_list, 200, 0},
    {"zlistCurrent", 0x0048c390, zlistCurrent, 1, DT_RET_INT, setup_list, 200, 0},
    {"zlistGet", 0x0048c3b0, zlistGet, 2, DT_RET_INT, setup_get, 200, 0},
    {"zlistCount", 0x0048c400, zlistCount, 1, DT_RET_INT, setup_list, 50, 0},
    {"zlistInsert", 0x0048c460, zlistInsert, 2, DT_RET_VOID, setup_insert, 300, 0},
};
