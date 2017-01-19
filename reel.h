
#ifndef REEL_H
#define REEL_H

#include <stdint.h>

typedef enum {
    REEL_UINT,
    REEL_ITEM,
    REEL_ITEM2UINT,
    REEL_ITEM2STR
} reel_var_type;

typedef struct {
    reel_var_type type;
    const char *name;
    uintptr_t value;

    tdb_field table_field;
} reel_var;

/* utility functions */

void reel_parse(reel_var *var, const char *data, const tdb *db);

#endif /* REEL_H */
