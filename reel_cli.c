
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <traildb.h>

#include "reel_script.h"
#include "reel_util.h"

static void evaluate(reel_script_ctx *ctx, tdb *db)
{
    uint64_t trail_id, num_events;
    const tdb_event **events;
    tdb_cursor *cursor = tdb_cursor_new(db);
    reel_event_buffer *buf = reel_event_buffer_new();
    tdb_error err;

    if (!(cursor && buf)){
        fprintf(stderr, "Evaluate out of memory\n");
        exit(1);
    }

    for (trail_id = 0; trail_id < tdb_num_trails(db); trail_id++)
    {
        tdb_get_trail(cursor, trail_id);

        if (!(events = reel_event_buffer_fill(buf, cursor, &num_events))){
            fprintf(stderr,
                    "[trail %"PRIu64"] Event buffer out of memory\n",
                    trail_id);
            exit(1);
        }

        if ((err = reel_script_eval_trail(ctx, events, num_events))){
            fprintf(stderr,
                    "[trail %"PRIu64"] Script failed: %s\n",
                    trail_id,
                    reel_error_str(err));
            exit(1);
        }
    }

    reel_event_buffer_free(buf);
}

int main(int argc, char **argv)
{
    tdb *db = tdb_init();
    tdb_error err;

    if (argc < 2){
        fprintf(stderr, "Usage: %s traildb.tdb\n", argv[0]);
        exit(1);
    }
    if ((err = tdb_open(db, argv[1]))){
        fprintf(stderr, "Could not open tdb at %s\n", argv[1]);
        exit(1);
    }
    reel_script_ctx *ctx = reel_script_new(db);

    evaluate(ctx, db);
    printf("%s\n", reel_script_output_csv(ctx, ','));
    reel_script_free(ctx);
    return 0;
}
