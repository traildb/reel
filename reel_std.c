
#include <string.h>

/* inc */

static inline void reelfunc_inc_uintptr_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t rval){
    *lval += rval;
}

static inline void reelfunc_inc_tableptr_tableptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, reel_var *lval, reel_var *rval){

    if (!(lval->table_value_type == REEL_UINT &&
          lval->table_field == rval->table_field &&
          lval->table_value_type == rval->table_value_type)){
        ctx->error = REEL_TABLE_MISMATCH;
    }else{
        uint64_t *dst = (uint64_t*)lval->value;
        const uint64_t *src = (uint64_t*)rval->value;
        uint64_t i;
        for (i = 0; i < lval->table_length; i++)
            dst[i] += src[i];
    }
}

/* set */

static inline void reelfunc_set_uintptr_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t rval){
    *lval = rval;
}

static inline void reelfunc_set_uintptr_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t *rval){
    *lval = *rval;
}

static inline void reelfunc_set_itemptr_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item *lval, tdb_item rval){
    *lval = rval;
}

static inline void reelfunc_set_itemptr_itemptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item *lval, tdb_item *rval){
    *lval = *rval;
}

static inline void reelfunc_set_tableitem_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, reel_var *lval, uint64_t rval){

    if (lval->table_value_type != REEL_UINT){
        ctx->error = REEL_TABLE_MISMATCH;
    }else if (lval->table_field){
        uint64_t *dst = (uint64_t*)lval->value;
        dst[tdb_item_val(ev->items[lval->table_field - 1])] = rval;
    }
}

static inline void reelfunc_set_uintptr_tableitem(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, reel_var *rval){

    if (rval->table_value_type != REEL_UINT){
        ctx->error = REEL_TABLE_MISMATCH;
    }else if (rval->table_field){
        const uint64_t *src = (uint64_t*)rval->value;
        *lval = src[tdb_item_val(ev->items[rval->table_field - 1])];
    }else
        *lval = 0;
}

/* unset */

static inline void reelfunc_unset_tableptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, reel_var *val){
    if (val->table_field){
        uint64_t *dst = (uint64_t*)val->value;
        memset(dst, 0, val->table_length * 8);
    }
}

static inline void reelfunc_unset_tableitem(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, reel_var *val){
    if (val->table_field){
        uint64_t *dst = (uint64_t*)val->value;
        dst[tdb_item_val(ev->items[val->table_field - 1])] = 0;
    }
}

static inline void reelfunc_unset_itemptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item *val){
    *val = 0;
}

static inline void reelfunc_unset_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item *val){
    *val = 0;
}

/* if */

static inline int reelfunc_if_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item item){
    return item != 0;
}

static inline int reelfunc_if_itemptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item *item){
    return *item != 0;
}

static inline int reelfunc_if_item_itemptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item lval, tdb_item *rval){
    return lval == *rval;
}

static inline int reelfunc_if_itemptr_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item *lval, tdb_item rval){
    return *lval == rval;
}

static inline int reelfunc_if_item_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item lval, tdb_item rval){
    return lval == rval;
}

static inline int reelfunc_if_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t val){
    return val != 0;
}

static inline int reelfunc_if_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *val){
    return *val != 0;
}

static inline int reelfunc_if_uintptr_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t *rval){
    return *lval == *rval;
}

static inline int reelfunc_if_uintptr_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t rval){
    return *lval == rval;
}

/* within_time */

static inline int reelfunc_within_time_uintptr_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t rval){
    if (*lval < ev->timestamp)
        return 0;
    else
        return *lval - ev->timestamp < rval;
}

/* if_greater */

static inline int reelfunc_if_greater_uint_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t lval, uint64_t *rval){
    return lval > *rval;
}

/* tables */

static int reel_init_table(reel_var *var, const tdb *db, const char *field_name)
{
    uintptr_t *p;
    if (tdb_get_field(db, field_name, &var->table_field)){
        var->value = var->table_field = var->table_length = 0;
        return 0;
    }
    var->table_length = tdb_lexicon_size(db, var->table_field);
    if (!(p = calloc(1, var->table_length * sizeof(uintptr_t))))
        return -1;
    var->value = (uintptr_t)p;
    return 0;
}

/* fork */

static int reel_fork(reel_ctx *ctx, Word_t key)
{
    int not_exists;
    J1S(not_exists, ctx->root->evaluated_contexts, key);
    if (!not_exists){
        return 0;
    }else{
        Word_t *ptr;
        JLI(ptr, ctx->root->child_contexts, key);
        if (!*ptr){
            reel_ctx *child = malloc(sizeof(reel_ctx));
            if (!child){
                ctx->error = REEL_FORK_FAILED;
                return 0;
            }
            memcpy(child, ctx, sizeof(reel_ctx));
            child->child_contexts = NULL;
            child->evaluated_contexts = NULL;
            *ptr = (Word_t)child;
        }
        ctx->child = (reel_ctx*)*ptr;
        return 1;
    }
}

static inline int reelfunc_fork_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t val){
    return reel_fork(ctx, (Word_t)val);
}

static inline int reelfunc_fork_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t val){
    return reel_fork(ctx, (Word_t)val);
}

static inline int reelfunc_fork_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *val){
    return reel_fork(ctx, (Word_t)*val);
}

static inline void reelfunc_fork_reset_active(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx){
    Word_t tmp;
    J1FA(tmp, ctx->root->evaluated_contexts);
}

static int reel_get_forks(const reel_ctx *ctx, reel_ctx **ctxs, uint64_t *num_ctxs, uint64_t *ctxs_size)
{
    Word_t num, key = 0;
    Word_t *ptr;

    JLC(num, ctx->child_contexts, 0, -1);
    if (num > *ctxs_size){
        if (!(ctxs = realloc(ctxs, num * sizeof(reel_ctx*))))
            return -1;
        *ctxs_size = num;
    }
    *num_ctxs = num;
    num = 0;
    JLF(ptr, ctx->child_contexts, key);
    while (ptr){
        ctxs[num++] = (reel_ctx*)*ptr;
        JLN(ptr, ctx->child_contexts, key);
    }
    return 0;
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

