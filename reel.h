
#ifndef REEL_H
#define REEL_H

#include <stdint.h>

#include <traildb.h>

typedef enum {
    REEL_UINT = 1,
    REEL_ITEM = 2,
    REEL_UINTTABLE = 3
} reel_var_type;

typedef enum {
    REEL_OUT_OF_MEMORY = -1,
    REEL_FORK_FAILED = -2,
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

#endif /* REEL_H */
