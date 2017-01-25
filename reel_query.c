
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <traildb.h>

#include "reel_script.h"
#include "reel_util.h"

#define DIE(msg, ...)\
    do { fprintf(stderr, msg"\n", ##__VA_ARGS__);   \
         exit(EXIT_FAILURE); } while (0)

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

static char *open_file(const char *arg, const char *path)
{
    int fd;
    struct stat stats;

    if ((fd = open(path, O_RDONLY)) == -1)
        DIE("Could not open file '%s' for variable '%s'", path, arg);

    if (fstat(fd, &stats))
        DIE("Could not read file '%s' for variable '%s'", path, arg);

    if (stats.st_size){
        char *ret;
        char *p = mmap(NULL, stats.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED)
            DIE("Could not read file '%s' for variable '%s'", path, arg);
        close(fd);
        if (!(ret = calloc(1, stats.st_size + 1)))
            DIE("Out of memory\n");
        memcpy(ret, p, stats.st_size);
        munmap(p, stats.st_size);
        return ret;
    }else
        DIE("File '%s' for variable '%s' is empty", path, arg);
}

static void set_var(reel_script_ctx *ctx, const char *arg)
{
    char *val;
    reel_parse_error err;

    if (!(val = strchr(arg, '=')))
        DIE("Invalid argument '%s'\n", arg);

    *val = 0;
    ++val;
    if (val[0] == '@')
        val = open_file(arg, &val[1]);

    if ((err = reel_script_parse_var(ctx, arg, val)) < 0)
        DIE("Could not set variable '%s': %s\n",
            arg,
            reel_parse_error_str(err));
    else if (err > 0)
        fprintf(stderr,
                "Note about variable '%s': %s\n",
                arg,
                reel_parse_error_str(err));
}

static void print_usage_and_exit()
{
    fprintf(stderr,
"\nreel_query - execute a Reel query with a TrailDB\n"
"\n"
"USAGE:\n"
"reel_query [options] traildb\n"
"\n"
"OPTIONS:\n"
"-s --set var=value    Set a variable in the Reel script.\n"
"                      Use var=@filename to read value from a file.\n"
"\n");
    exit(1);
}

static void initialize(reel_script_ctx *ctx, int argc, char **argv)
{
    static struct option long_options[] = {
        {"set", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    int c, option_index = 1;

    do{
        c = getopt_long(argc,
                        argv,
                        "s:",
                        long_options,
                        &option_index);

        switch (c){
            case -1:
                break;
            case 's':
                set_var(ctx, optarg);
                break;
            default:
                print_usage_and_exit();
        }
    }while (c != -1);
}

int main(int argc, char **argv)
{
    tdb *db = tdb_init();
    tdb_error err;

    if (argc < 2)
        print_usage_and_exit();

    if ((err = tdb_open(db, argv[argc - 1]))){
        DIE("Could not open tdb at %s\n", argv[argc - 1]);
    }
    reel_script_ctx *ctx = reel_script_new(db);
    if (!ctx)
        DIE("Couldn't initialize the Reel script. Out of memory?\n");
    initialize(ctx, argc, argv);
    evaluate(ctx, db);
    printf("%s\n", reel_script_output_csv(ctx, ','));
    reel_script_free(ctx);
    return 0;
}
