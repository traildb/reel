
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <reel.h>

#include "reel_util.h"

#define INITIAL_BUFFER_SIZE 1000000
#define INITIAL_EVENTS_SIZE 10000

struct _reel_event_buffer {
    char *buffer;
    uint64_t buffer_size;
    const tdb_event **events;
    uint64_t events_size;
};

reel_event_buffer *reel_event_buffer_new()
{
    reel_event_buffer *buf = NULL;

    if (!(buf = calloc(1, sizeof(reel_event_buffer))))
        return NULL;
    if (!(buf->buffer = malloc(INITIAL_BUFFER_SIZE)))
        goto error;
    buf->buffer_size = INITIAL_BUFFER_SIZE;
    if (!(buf->events = malloc(INITIAL_EVENTS_SIZE * sizeof(tdb_event*))))
        goto error;
    buf->events_size = INITIAL_EVENTS_SIZE;
    return buf;
error:
    reel_event_buffer_free(buf);
    return NULL;
}

void reel_event_buffer_free(reel_event_buffer *buf)
{
    free(buf->buffer);
    free(buf->events);
    free(buf);
}

const tdb_event **reel_event_buffer_fill(reel_event_buffer *buf,
                                         tdb_cursor *cursor,
                                         uint64_t *num_events)
{
    const tdb_event *e;
    uint64_t offset = 0;
    char *p;
    *num_events = 0;

    while ((e = tdb_cursor_next(cursor))){
        uint64_t size = sizeof(tdb_event) + e->num_items * sizeof(tdb_item);

        while (offset + size >= buf->buffer_size){
            buf->buffer_size *= 2;
            if ((p = realloc(buf->buffer, buf->buffer_size)))
                buf->buffer = p;
            else
                return NULL;
        }

        memcpy(&buf->buffer[offset], e, size);
        buf->events[*num_events] = (const tdb_event*)&buf->buffer[offset];
        offset += size;

        if (++*num_events == buf->events_size){
            buf->events_size *= 2;
            if ((p = realloc(buf->events, buf->events_size * sizeof(tdb_event*))))
                buf->events = (const tdb_event**)p;
            else
                return NULL;
        }
    }

    return buf->events;
}

const char *reel_error_str(reel_error error)
{
    switch (error){
        case REEL_OUT_OF_MEMORY:
            return "Out of memory";
        case REEL_TABLE_MISMATCH:
            return "Table mismatch";
        case REEL_FORK_FAILED:
            return "Fork failed";
    };
    return "Unknown error";
}

const char *reel_parse_error_str(reel_parse_error error)
{
    switch (error){
        case REEL_PARSE_OK:
            return "Parsing ok";
        case REEL_PARSE_UNKNOWN_VARIABLE:
            return "Unknown variable";
        case REEL_PARSE_INVALID_VALUE:
            return "Malformed value";
        case REEL_PARSE_UNKNOWN_FIELD:
            return "Unknown field";
        case REEL_PARSE_EMPTY_TABLE:
            return "Table is empty (unknown field)";
        case REEL_PARSE_VALUE_UNKNOWN:
            return "Unknown value was set to nil";
        case REEL_PARSE_SOME_VALUES_UNKNOWN:
            return "Some unknown values were not set in the table";
    }
    return "Unknown error";
}
