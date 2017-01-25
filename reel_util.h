
#ifndef REEL_UTIL_H
#define REEL_UTIL_H

#include <stdint.h>

#include <traildb.h>
#include <reel.h>

typedef struct _reel_event_buffer reel_event_buffer;

reel_event_buffer *reel_event_buffer_new();

void reel_event_buffer_free(reel_event_buffer *buf);

const tdb_event **reel_event_buffer_fill(reel_event_buffer *buf,
                                         tdb_cursor *cursor,
                                         uint64_t *num_events);

const char *reel_error_str(reel_error error);

#endif /* REEL_UTIL_H */
