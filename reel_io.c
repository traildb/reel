
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#include <traildb.h>

#define CSV_BUFFER_SIZE 100000
#define MAX_NON_INDEX_SIZE 1000

static reel_parse_error reel_parse_uint(uint64_t *dst, const char *src)
{
    char *end;
    errno = 0;
    *dst = strtoull(src, &end, 10);
    if (errno || *end)
        return REEL_PARSE_INVALID_VALUE;
    return 0;
}

static reel_parse_error reel_parse_item(const tdb *db,
                                        tdb_item *dst,
                                        const char *src)
{
    char *val;
    tdb_field field;

    *dst = 0;
    if (src[0] != '$')
        return REEL_PARSE_INVALID_VALUE;
    if (!(val = strchr(src, '=')))
        return REEL_PARSE_INVALID_VALUE;
    *val = 0;
    ++val;
    if (tdb_get_field(db, &src[1], &field))
        return REEL_PARSE_UNKNOWN_FIELD;
    if (val[0] == '\'' || val[0] == '"'){
        size_t len = strlen(val);
        if (val[0] != val[len - 1])
            return REEL_PARSE_INVALID_VALUE;
        else{
            val = &val[1];
            val[len - 1] = 0;
        }
    }
    if (!(*dst = tdb_get_item(db, field, val, strlen(val))))
        return REEL_PARSE_VALUE_UNKNOWN;
    else
        return 0;
}

static Pvoid_t create_index(const tdb *db, tdb_field field)
{
    uint64_t i, len;
    Pvoid_t index = NULL;
    for (i = 0; i < tdb_lexicon_size(db, field); i++){
        const char *val = tdb_get_value(db, field, i, &len);
        Word_t *ptr;
        JHSI(ptr, index, (char*)val, len);
        *ptr = tdb_make_item(field, i);
    }
    return index;
}

static reel_parse_error reel_parse_uinttable(const tdb *db,
                                             reel_var *var,
                                             const char *src)
{
    tdb_item item;
    reel_parse_error err = 0;
    uint64_t len, size = strlen(src);
    const char *end = src + size;
    uint64_t *table = (uint64_t*)var->value;
    char *val;

    Pvoid_t index = NULL;
    Word_t tmp;

    if (!var->table_field)
        return REEL_PARSE_EMPTY_TABLE;

    if (size > MAX_NON_INDEX_SIZE)
        index = create_index(db, var->table_field);

    while (src < end){

        if (!(val = strchr(src, ' ')))
            return REEL_PARSE_INVALID_VALUE;

        len = val - src;

        if (index){
            Word_t *ptr;
            JHSG(ptr, index, (char*)src, len);
            if (ptr)
                item = *ptr;
            else
                item = 0;
        }else
            item = tdb_get_item(db, var->table_field, src, len);
        if (item){
            char *p;
            uint64_t uint = strtoull(val, &p, 10);
            if (*p != '\n')
                return REEL_PARSE_INVALID_VALUE;
            table[tdb_item_val(item)] = uint;
            src = p + 1;
        }else{
            if (!(src = strchr(val, '\n')))
                return REEL_PARSE_INVALID_VALUE;
            ++src;
            err = REEL_PARSE_SOME_VALUES_UNKNOWN;
        }
    }
    JHSFA(tmp, index);
    return err;
}

static reel_parse_error reel_parse_var(reel_ctx *ctx,
                                       const char *var_name,
                                       const char *value)
{
    reel_var *var = NULL;
    reel_parse_error ret;
    uint64_t i;

    for (i = 0; i < sizeof(ctx->vars) / sizeof(reel_var); i++)
        if (!strcmp(ctx->vars[i].name, var_name)){
            var = &ctx->vars[i];
            break;
        }
    if (!var)
        return REEL_PARSE_UNKNOWN_VARIABLE;
    switch (var->type) {
        case REEL_UINT:
            if ((ret = reel_parse_uint(&var->value, value)))
                return ret;
            break;
        case REEL_ITEM:
            if ((ret = reel_parse_item(ctx->db, &var->value, value)))
                return ret;
            break;
        case REEL_UINTTABLE:
            if ((ret = reel_parse_uinttable(ctx->db, var, value)))
                return ret;
            break;
    }
    return 0;
}

static char *reel_str_append(char *buf,
                             uint64_t *offset,
                             uint64_t *size,
                             const char *fmt,
                             ...)
{
    int len;
    va_list ap;

    while (1){
        va_start(ap, fmt);
        len = vsnprintf(&buf[*offset], *size - *offset, fmt, ap);
        va_end(ap);

        if (len < *size - *offset){
            *offset += len;
            return buf;
        }else{
            char *p;
            *size *= 2;
            if ((p = realloc(buf, *size)))
                buf = p;
            else{
                free(buf);
                return NULL;
            }
        }
    }
}

static void reel_merge_vars(reel_ctx *dst, const reel_ctx *src, reel_merge_mode mode)
{
    uint64_t i, j;
    uint64_t *dst_table;
    const uint64_t *src_table;

    if (mode == REEL_MERGE_ADD){
        for (i = 0; i < sizeof(src->vars) / sizeof(reel_var); i++){
            switch (src->vars[i].type) {
                case REEL_UINT:
                    dst->vars[i].value += src->vars[i].value;
                    break;
                case REEL_ITEM:
                    dst->vars[i].value = src->vars[i].value;
                    break;
                case REEL_UINTTABLE:
                    if (!(src->vars[i].flags & REEL_FLAG_IS_CONST)){
                        src_table = (const uint64_t*)src->vars[i].value;
                        dst_table = (uint64_t*)dst->vars[i].value;
                        for (j = 0; j < src->vars[i].table_length; j++)
                            dst_table[j] += src_table[j];
                    }
                    break;
            }
        }
    }else if (mode == REEL_MERGE_OVERWRITE){
        for (i = 0; i < sizeof(src->vars) / sizeof(reel_var); i++){
            switch (src->vars[i].type) {
                case REEL_UINT:
                case REEL_ITEM:
                    dst->vars[i].value = src->vars[i].value;
                    break;
                case REEL_UINTTABLE:
                    if (!(src->vars[i].flags & REEL_FLAG_IS_CONST)){
                        src_table = (const uint64_t*)src->vars[i].value;
                        dst_table = (uint64_t*)dst->vars[i].value;
                        memcpy(dst_table,
                               src_table,
                               src->vars[i].table_length * sizeof(uintptr_t));
                    }
                    break;
            }
        }
    }
}

static reel_error reel_merge_ctx(reel_ctx *dst, const reel_ctx *src, reel_merge_mode mode)
{
    Word_t *ptr;
    Word_t key = 0;

    if (!(dst == dst->root && src == src->root))
        return REEL_MERGE_NOT_PARENT;

    reel_merge_vars(dst, src, mode);

    /* handle children */
    JLF(ptr, src->child_contexts, key);
    while (ptr){
        const reel_ctx *src_child = (const reel_ctx*)*ptr;
        reel_ctx *dst_child;

        JLI(ptr, dst->child_contexts, key);
        if (*ptr)
            dst_child = (reel_ctx*)*ptr;
        else{
            if (!(dst_child = reel_clone(src_child, NULL, 1, 0)))
                return REEL_OUT_OF_MEMORY;
            dst_child->root = dst;
            dst_child->db = dst->db;
            *ptr = (Word_t)dst_child;
        }
        reel_merge_vars(dst_child, src_child, mode);

        JLN(ptr, src->child_contexts, key);
    }

    return 0;
}

#define STRADD(...) if (!(buf = reel_str_append(buf, offset, size, __VA_ARGS__))) return NULL;

static char *reel_output_csv_ctx(const reel_ctx *ctx,
                                 char delimiter,
                                 char *buf,
                                 uint64_t *offset,
                                 uint64_t *size)
{
    uint64_t len, m, k, i, j;
    const char *val;
    const uint64_t *uinttable;

    for (i = 0; i < sizeof(ctx->vars) / sizeof(reel_var); i++)
        if (!strcmp(ctx->vars[i].name, "_HIDE") && ctx->vars[i].value)
            return buf;

    for (i = 0, j = 0; i < sizeof(ctx->vars) / sizeof(reel_var); i++){
        const reel_var *v = &ctx->vars[i];
        if (v->name[0] == '_' || !isupper(v->name[0]))
            continue;
        if (j++){
            STRADD("%c", delimiter)
        }
        switch (ctx->vars[i].type) {
            case REEL_UINT:
                STRADD("%"PRIu64, v->value)
                break;
            case REEL_ITEM:
                if (v->value){
                    val = tdb_get_item_value(ctx->db, v->value, &len);
                    STRADD("%.*s", len, val)
                }
                break;
            case REEL_UINTTABLE:
                uinttable = (const uint64_t*)v->value;
                for (m = 0, k = 0; k < v->table_length; k++){
                    if (m++){
                        STRADD("%c", delimiter)
                    }
                    STRADD("%"PRIu64, uinttable[k])
                }
                break;
        }
    }
    STRADD("\n");
    return buf;
}

static char *reel_output_csv(const reel_ctx *ctx, char delimiter)
{
    uint64_t size0 = CSV_BUFFER_SIZE;
    uint64_t *size = &size0;
    uint64_t offset0 = 0;
    uint64_t *offset = &offset0;
    uint64_t len, i, k, m, j;
    const char *val;
    char *buf;
    Word_t key = 0;
    Word_t *ptr;

    if (!(buf = malloc(*size)))
        return NULL;

    /* output header */
    for (i = 0, j = 0; i < sizeof(ctx->vars) / sizeof(reel_var); i++){
        const reel_var *v = &ctx->vars[i];
        if (v->name[0] == '_' || !isupper(v->name[0]))
            continue;
        if (j++){
            STRADD("%c", delimiter)
        }
        switch (ctx->vars[i].type){
            case REEL_UINT:
            case REEL_ITEM:
                STRADD("%s", v->name);
                break;
            case REEL_UINTTABLE:
                for (m = 0, k = 0; k < v->table_length; k++){
                    if (m++){
                        STRADD("%c", delimiter)
                    }
                    val = tdb_get_value(ctx->db, v->table_field, k, &len);
                    STRADD("%s:%.*s", v->name, len, val)
                }
                break;
        }
    }
    STRADD("\n");

    /* output parent */
    if (!(buf = reel_output_csv_ctx(ctx, delimiter, buf, offset, size)))
        return NULL;

    /* output children */
    JLF(ptr, ctx->child_contexts, key);
    while (ptr){
        const reel_ctx *child = (const reel_ctx*)*ptr;
        if (!(buf = reel_output_csv_ctx(child, delimiter, buf, offset, size)))
            return NULL;
        JLN(ptr, ctx->child_contexts, key);
    }
    return buf;
}


