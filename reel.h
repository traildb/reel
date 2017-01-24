
#ifndef REEL_H
#define REEL_H

#include <stdint.h>

typedef enum {
    REEL_UINT = 1,
    REEL_ITEM = 2,
    REEL_UINTTABLE = 3,
    REEL_STRINGTABLE = 4
} reel_var_type;

typedef enum {
    REEL_OUT_OF_MEMORY = -1,
    REEL_TABLE_MISMATCH = -200
} reel_error;

typedef struct {
    reel_var_type type;
    const char *name;
    uintptr_t value;

    uint64_t table_length;
    tdb_field table_field;
    reel_var_type table_value_type;
} reel_var;

/* utility functions */

void reel_parse(reel_var *var, const char *data, const tdb *db);

#endif /* REEL_H */
