
#include <string.h>

/* inc */

static inline void reelfunc_inc_uintptr_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t rval){
    *lval += rval;
}
/* set */

static inline void reelfunc_set_uintptr_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t rval){
    *lval = rval;
}

static inline void reelfunc_set_itemptr_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item *lval, tdb_item rval){
    *lval = rval;
}

/* if */

static inline int reelfunc_if_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item item){
    return ev->items[tdb_item_field(item) - 1] == item;
}

static inline int reelfunc_if_itemptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item *item){
    return ev->items[tdb_item_field(*item) - 1] == *item;
}

static inline int reelfunc_if_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *val){
    return *val;
}

/* utilities */

static tdb_item reel_resolve_item_literal(const tdb *db, const char *field_name, const char *value)
{
    tdb_field field;
    if (tdb_get_field(db, field_name, &field))
        return 0;
    return tdb_get_item(db, field, value, strlen(value));
}

static tdb_field reel_resolve_field(const tdb *db, const char *field_name)
{
    tdb_field field;
    if (tdb_get_field(db, field_name, &field))
        return 0;
    return field;
}
