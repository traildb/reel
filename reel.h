
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
    REEL_SETPOS_OUT_OF_BOUNDS = 3,
    REEL_TABLE_MISMATCH = -200
} reel_error;

typedef enum {
    REEL_PARSE_OK = 0,
    REEL_PARSE_UNKNOWN_VARIABLE = -1,
    REEL_PARSE_INVALID_VALUE = -2,
    REEL_PARSE_UNKNOWN_FIELD = 1,
    REEL_PARSE_EMPTY_TABLE = 2,
    REEL_PARSE_VALUE_UNKNOWN = 3,
    REEL_PARSE_SOME_VALUES_UNKNOWN = 4
} reel_parse_error;

typedef enum {
    REEL_FLAG_IS_CONST = 1
} reel_flags;

typedef struct {
    reel_var_type type;
    const char *name;
    uintptr_t value;

    uint64_t table_length;
    tdb_field table_field;
    reel_var_type table_value_type;

    uint64_t flags;
} reel_var;

#endif /* REEL_H */
