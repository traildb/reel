
#include <string.h>

/* setpos */

#define reelfunc_setpos_uintptr(ctx, ev, idx, val)\
    if (*(val) < num_events) { evidx = *(val); goto loopstart; }\
    else { return REEL_SETPOS_OUT_OF_BOUNDS; }

#define reelfunc_setpos_uint(ctx, ev, idx, val)\
    if (val < num_events) { evidx = val; goto loopstart; }\
    else { return REEL_SETPOS_OUT_OF_BOUNDS; }

/* num_events */

static inline void reelfunc_numevents_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval){
    *lval = ctx->num_events;
}

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

static inline void reelfunc_inc_tableitem_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, reel_var *lval, uint64_t rval){

    if (lval->table_value_type != REEL_UINT){
        ctx->error = REEL_TABLE_MISMATCH;
    }else if (lval->table_field){
        uint64_t *dst = (uint64_t*)lval->value;
        dst[tdb_item_val(ev->items[lval->table_field - 1])] += rval;
    }
}

static inline void reelfunc_inc_tableitem_tableitem(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, reel_var *lval, reel_var *rval){

    if (!(lval->table_value_type == REEL_UINT &&
          rval->table_value_type == REEL_UINT &&
          lval->table_field == rval->table_field))
        ctx->error = REEL_TABLE_MISMATCH;
    else if (lval->table_field){
        const uint64_t *src = (const uint64_t*)rval->value;
        uint64_t *dst = (uint64_t*)lval->value;
        uint64_t idx = tdb_item_val(ev->items[lval->table_field - 1]);
        dst[idx] += src[idx];
    }
}

/* dec */

static inline void safe_dec(uint64_t *dst, uint64_t src)
{
    if (*dst > src)
        *dst -= src;
    else
        *dst = 0;
}


static inline void reelfunc_dec_uintptr_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t rval){
    safe_dec(lval, rval);
}

static inline void reelfunc_dec_tableptr_tableptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, reel_var *lval, reel_var *rval){

    if (!(lval->table_value_type == REEL_UINT &&
          lval->table_field == rval->table_field &&
          lval->table_value_type == rval->table_value_type)){
        ctx->error = REEL_TABLE_MISMATCH;
    }else{
        uint64_t *dst = (uint64_t*)lval->value;
        const uint64_t *src = (uint64_t*)rval->value;
        uint64_t i;
        for (i = 0; i < lval->table_length; i++)
            safe_dec(&dst[i], src[i]);
    }
}

static inline void reelfunc_dec_tableitem_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, reel_var *lval, uint64_t rval){

    if (lval->table_value_type != REEL_UINT){
        ctx->error = REEL_TABLE_MISMATCH;
    }else if (lval->table_field){
        uint64_t *dst = (uint64_t*)lval->value;
        safe_dec(&dst[tdb_item_val(ev->items[lval->table_field - 1])], rval);
    }
}

static inline void reelfunc_dec_tableitem_tableitem(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, reel_var *lval, reel_var *rval){

    if (!(lval->table_value_type == REEL_UINT &&
          rval->table_value_type == REEL_UINT &&
          lval->table_field == rval->table_field))
        ctx->error = REEL_TABLE_MISMATCH;
    else if (lval->table_field){
        const uint64_t *src = (const uint64_t*)rval->value;
        uint64_t *dst = (uint64_t*)lval->value;
        uint64_t idx = tdb_item_val(ev->items[lval->table_field - 1]);
        safe_dec(&dst[idx], src[idx]);
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

static inline void reelfunc_set_tableitem_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, reel_var *lval, uint64_t *rval){

    if (lval->table_value_type != REEL_UINT){
        ctx->error = REEL_TABLE_MISMATCH;
    }else if (lval->table_field){
        uint64_t *dst = (uint64_t*)lval->value;
        dst[tdb_item_val(ev->items[lval->table_field - 1])] = *rval;
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
    return tdb_item_val(item) != 0;
}

static inline int reelfunc_if_itemptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, tdb_item *item){
    return tdb_item_val(*item) != 0;
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

static inline int reelfunc_if_tableitem(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, reel_var *val){
    if (val->table_value_type != REEL_UINT)
        ctx->error = REEL_TABLE_MISMATCH;
    else if (val->table_field){
        const uint64_t *src = (const uint64_t*)val->value;
        return src[tdb_item_val(ev->items[val->table_field - 1])] != 0;
    }
    return 0;
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

/* identity */

#define MAX_ID_ITEMS 6

static int reel_identity(reel_ctx *ctx, uint64_t *dst, const uint64_t args[MAX_ID_ITEMS])
{
    Word_t *ptr;
    JHSI(ptr, ctx->identities, (uint8_t*)args, MAX_ID_ITEMS * sizeof(uint64_t))
    if (!*ptr){
        *ptr = *dst = ++ctx->identity_counter;
        return 1;
    }else{
        *dst = *ptr;
        return 0;
    }
}

static inline int reelfunc_id_uintptr_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t v1){
    uint64_t args[MAX_ID_ITEMS] = {v1, 0, 0, 0, 0, 0};
    return reel_identity(ctx, lval, args);
}

static inline int reelfunc_id_uintptr_item_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t v1, uint64_t v2){
    uint64_t args[MAX_ID_ITEMS] = {v1, v2, 0, 0, 0, 0};
    return reel_identity(ctx, lval, args);
}

static inline int reelfunc_id_uintptr_item_item_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t v1, uint64_t v2, uint64_t v3){
    uint64_t args[MAX_ID_ITEMS] = {v1, v2, v3, 0, 0, 0};
    return reel_identity(ctx, lval, args);
}

static inline int reelfunc_id_uintptr_item_item_item_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t v1, uint64_t v2, uint64_t v3, uint64_t v4){
    uint64_t args[MAX_ID_ITEMS] = {v1, v2, v3, v4, 0, 0};
    return reel_identity(ctx, lval, args);
}

static inline int reelfunc_id_uintptr_item_item_item_item_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t v1, uint64_t v2, uint64_t v3, uint64_t v4, uint64_t v5){
    uint64_t args[MAX_ID_ITEMS] = {v1, v2, v3, v4, v5, 0};
    return reel_identity(ctx, lval, args);
}

static inline int reelfunc_id_uintptr_item_item_item_item_item_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t v1, uint64_t v2, uint64_t v3, uint64_t v4, uint64_t v5, uint64_t v6){
    uint64_t args[MAX_ID_ITEMS] = {v1, v2, v3, v4, v5, v6};
    return reel_identity(ctx, lval, args);
}

/* time_before */

static inline int reelfunc_time_before_uintptr_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t rval){
    return ev->timestamp <= *lval + rval;
}

static inline int reelfunc_time_before_uintptr_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t *rval){
    return ev->timestamp <= *lval + *rval;
}

/* time_after */

static inline int reelfunc_time_after_uintptr_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t rval){
    return ev->timestamp >= *lval + rval;
}

static inline int reelfunc_time_after_uintptr_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t *rval){
    return ev->timestamp >= *lval + *rval;
}

static inline int reelfunc_time_after_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *val){
    return ev->timestamp >= *val;
}

/* if_greater */

static inline int reelfunc_if_greater_uint_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t lval, uint64_t *rval){
    return lval > *rval;
}

static inline int reelfunc_if_greater_uintptr_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t rval){
    return *lval > rval;
}

static inline int reelfunc_if_greater_or_equal_uint_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t lval, uint64_t *rval){
    return lval >= *rval;
}

static inline int reelfunc_if_greater_or_equal_uintptr_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *lval, uint64_t rval){
    return *lval >= rval;
}

/* print */

static inline void reelfunc_print_uintptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *val){
    fprintf(stderr, "debug %lu\n", *val);
}

static inline void reelfunc_print_itemptr(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t *val){
    fprintf(stderr, "debug %lu\n", *val);
}

static inline void reelfunc_print_item(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t val){
    fprintf(stderr, "debug %lu\n", val);
}

static inline void reelfunc_print_uint(reel_ctx *ctx, const tdb_event *ev, uint32_t func_idx, uint64_t val){
    fprintf(stderr, "debug %lu\n", val);
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

static reel_ctx *reel_clone(const reel_ctx *src,
                            tdb *db,
                            int do_reset,
                            int do_deep_copy)
{
    uint64_t i;
    reel_ctx *ctx;

    if (!(ctx = malloc(sizeof(reel_ctx))))
        return NULL;

    memcpy(ctx, src, sizeof(reel_ctx));
    ctx->trail_id = 0;

    if (db)
        ctx->db = db;

    /* handle variables */
    for (i = 0; i < sizeof(ctx->vars) / sizeof(reel_var); i++){
        reel_var *v = &ctx->vars[i];
        if (v->table_field){
            /* const tables are shared */
            if (!(v->flags & REEL_FLAG_IS_CONST)){
                char *p;
                if (!(p = calloc(v->table_length, sizeof(uintptr_t))))
                    goto out_of_mem;
                v->value = (uintptr_t)p;
                if (!do_reset)
                    memcpy(p,
                           (const char*)v->value,
                           v->table_length * sizeof(uintptr_t));

            }
        }else if (do_reset)
            /* zero scalar variables */
            v->value = 0;
    }

    /*
    Make the new context a parent context.
    This may be changed later (see below).
    */
    ctx->root = ctx;
    ctx->child_contexts = NULL;
    ctx->evaluated_contexts = NULL;

    if (do_deep_copy){
        Word_t *ptr;
        Word_t key = 0;

        JLF(ptr, src->child_contexts, key);
        while (ptr){
            const reel_ctx *src_child = (const reel_ctx*)*ptr;
            reel_ctx *dst_child = reel_clone(src_child, db, do_reset, 0);
            if (!dst_child)
                goto out_of_mem;
            dst_child->root = ctx;
            JLI(ptr, ctx->child_contexts, key);
            *ptr = (Word_t)dst_child;
            JLN(ptr, src->child_contexts, key);
        }
    }
    return ctx;
out_of_mem:
    /* XXX we leak some memory here, tables and children are not freed */
    free(ctx);
    return NULL;
}

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
            reel_ctx *child = reel_clone(ctx, NULL, 1, 0);
            if (!child){
                ctx->error = REEL_FORK_FAILED;
                return 0;
            }
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

