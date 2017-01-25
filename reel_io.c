
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#define CSV_BUFFER_SIZE 100000

static reel_error reel_parse_var(reel_ctx *ctx, const char *var_name, const char *value)
{

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

