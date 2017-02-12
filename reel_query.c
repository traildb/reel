
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

#include <traildb.h>

#include "reel_script.h"
#include "reel_util.h"
#include "thread_util.h"

struct job_arg{
    tdb *db;
    reel_script_ctx *ctx;
    uint64_t shard_idx;
    uint64_t start_trail;
    uint64_t end_trail;
};

struct selected_trail{
    uint64_t trail_id;
    struct tdb_event_filter *filter;
};

static long num_threads;
static struct selected_trail *selected_trails;
static uint64_t num_selected;
static int show_progress;

static void *job_query_shard(void *arg0)
{
    struct job_arg *arg = (struct job_arg*)arg0;
    const tdb_event **events;
    tdb_cursor *cursor = tdb_cursor_new(arg->db);
    reel_event_buffer *buf = reel_event_buffer_new();
    uint64_t trail_id, num_events;
    reel_error err;

    if (!(cursor && buf))
        DIE("Query shard out of memory\n");

    for (trail_id = arg->start_trail; trail_id < arg->end_trail; trail_id++){
        if (show_progress){
            if (!((trail_id - arg->start_trail) & 65535)){
                uint64_t perc = (100 * (trail_id - arg->start_trail)) /
                                (arg->end_trail - arg->start_trail);
                fprintf(stderr,
                        "[thread %lu] %lu%% trails evaluated\n",
                        arg->shard_idx, perc);
            }
        }

        if (tdb_get_trail(cursor, trail_id))
            DIE("tdb_get_trail failed\n");

        if (!(events = reel_event_buffer_fill(buf, cursor, &num_events)))
            DIE("Event buffer out of memory\n");

        if (num_events)
            if ((err = reel_script_eval_trail(arg->ctx, events, num_events)))
                DIE("[trail %"PRIu64"] Script failed: %s\n",
                    trail_id,
                    reel_error_str(err));
    }
    if (show_progress)
        fprintf(stderr,
                "[thread %lu] 100%% trails evaluated\n",
                arg->shard_idx);

    reel_event_buffer_free(buf);
    tdb_cursor_free(cursor);
    return NULL;
}

static void apply_filters(tdb *db)
{
    uint64_t i;
    struct tdb_event_filter *empty = tdb_event_filter_new_match_none();
    tdb_opt_value value = {.ptr = empty};

    if (!empty)
        DIE("Couldn't allocate an empty filter\n");

    if (tdb_set_opt(db, TDB_OPT_EVENT_FILTER, value))
        DIE("Blacklisting trails failed\n");

    for (i = 0; i < num_selected; i++){
        value.ptr = selected_trails[i].filter;
        if (tdb_set_trail_opt(db,
                              selected_trails[i].trail_id,
                              TDB_OPT_EVENT_FILTER,
                              value))
            DIE("Setting a trail filter failed\n");
    }
}

static void evaluate(const tdb *db, reel_script_ctx *ctx, const char *tdb_path)
{
    uint64_t i, num_trails, trails_per_shard;
    struct job_arg *args;
    struct thread_job *jobs;

    num_trails = tdb_num_trails(db);
    if (num_threads > num_trails)
        num_threads = num_trails;

    trails_per_shard = num_trails / num_threads;

    if (!(args = calloc(num_threads, sizeof(struct job_arg))))
        DIE("Couldn't allocate args\n");

    if (!(jobs = calloc(num_threads, sizeof(struct thread_job))))
        DIE("Couldn't allocate jobs\n");

    for (i = 0; i < num_threads; i++){
        args[i].db = tdb_init();
        if (tdb_open(args[i].db, tdb_path))
            DIE("Could not open tdb at %s\n", tdb_path);

        if (selected_trails)
            apply_filters(args[i].db);

        if (!(args[i].ctx = reel_script_clone(ctx, args[i].db, 0, 0)))
            DIE("Could not clone a Reel context. Out of memory?\n");

        args[i].shard_idx = i;
        args[i].start_trail = i * trails_per_shard;
        args[i].end_trail = (i + 1) * trails_per_shard;
        if (args[i].end_trail > num_trails)
            args[i].end_trail = num_trails;

        jobs[i].arg = &args[i];
    }

    execute_jobs(job_query_shard, jobs, num_threads, num_threads);

    for (i = 0; i < num_threads; i++){
        reel_error err = reel_script_merge(ctx, args[i].ctx, REEL_MERGE_ADD);
        if (err)
            DIE("Merging results failed: %s\n", reel_error_str(err));
        reel_script_free(args[i].ctx);
        tdb_close(args[i].db);
    }
    free(args);
    free(jobs);
}

static char *open_file(const char *arg, const char *path){
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
"-s --set var=value      Set a variable in the Reel script.\n"
"                        Use var=@filename to read value from a file.\n"
"-T --threads N          Use N parallel threads to execute the query.\n"
"-S --select trailspec   Query limited time ranges on select trails (see below).\n"
"-P --progress           Print progress to stderr.\n"
"\n"
"Trailspec:\n"
"You can query a subset of trails, or query a chosen time range of select\n"
"trails, with the --select option. The --select option takes a file that\n"
"specifies trails to be selected, identified by a UUID, and optionally a\n"
"start time and an optional end time for the trail in each line:\n\n"
"[32-char hex-encoded UUID] <[start-time] [end-time]>\n\n"
"Unknown UUIDs are ignored.\n"
"\n");
    exit(1);
}

uint64_t safely_to_uint(const char *str, const char *field)
{
    char *end = NULL;
    errno = 0;
    long int x = strtoul(str, &end, 10);
    if (errno || *end)
        DIE("Invalid %s: %s", field, str);
    return x;
}

static void parse_select(const char *fname, const tdb *db)
{
    FILE *in;
    size_t n = 0;
    ssize_t line_len;
    char *line = NULL;
    uint8_t uuid[16];
    uint64_t num_lines = 0;

    if (!(in = fopen(fname, "r")))
        DIE("Could not open trailspec in %s\n", fname);

    while ((line_len = getline(&line, &n, in)) != -1)
        ++num_lines;

    if (!(selected_trails = malloc(num_lines * sizeof(struct selected_trail))))
        DIE("Couldn't allocated selected trails\n");

    rewind(in);
    while ((line_len = getline(&line, &n, in)) != -1){
        if (line[line_len - 1] == '\n')
            line[line_len - 1] = 0;

        char *p = NULL;
        char *uuidstr = strtok_r(line, " ", &p);
        char *startstr = strtok_r(NULL, " ", &p);
        char *endstr = strtok_r(NULL, " ", &p);
        uint64_t start_time = 0;
        uint64_t end_time = TDB_MAX_TIMEDELTA + 1;
        uint64_t trail_id;
        struct tdb_event_filter *filter;

        if (tdb_uuid_raw((const uint8_t*)uuidstr, uuid))
            DIE("Invalid UUID: %s\n", uuidstr);

        if (tdb_get_trail_id(db, uuid, &trail_id))
            continue;

        if (startstr){
            start_time = safely_to_uint(startstr, "start time");
            if (endstr)
                end_time = safely_to_uint(endstr, "end time");
        }

        if (!(filter = tdb_event_filter_new()))
            DIE("Creating an event filter failed. Out of memory?\n");
        if (tdb_event_filter_add_time_range(filter, start_time, end_time))
            DIE("Filter add time range failed (start %lu end %lu filter index %lu). Out of memory?\n", start_time, end_time, num_selected);

        selected_trails[num_selected].trail_id = trail_id;
        selected_trails[num_selected].filter = filter;
        ++num_selected;
    }

    fprintf(stderr,
            "Total trails: %lu Selected trails: %lu Unknown UUIDs: %lu\n",
            tdb_num_trails(db),
            num_selected,
            num_lines - num_selected);

    free(line);
    fclose(in);
}

static void initialize(reel_script_ctx *ctx,
                       const tdb *db,
                       int argc,
                       char **argv)
{
    static struct option long_options[] = {
        {"set", required_argument, 0, 's'},
        {"threads", required_argument, 0, 'T'},
        {"select", required_argument, 0, 'S'},
        {"progress", no_argument, 0, 'P'},
        {0, 0, 0, 0}
    };

    int c, option_index = 1;

    num_threads = 1;

    do{
        c = getopt_long(argc,
                        argv,
                        "s:T:S:P",
                        long_options,
                        &option_index);

        switch (c){
            case -1:
                break;
            case 's':
                set_var(ctx, optarg);
                break;
            case 'T':
                num_threads = safely_to_uint(optarg, "number of threads");
                break;
            case 'S':
                parse_select(optarg, db);
                break;
            case 'P':
                show_progress = 1;
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
    const char *path;

    if (argc < 2)
        print_usage_and_exit();

    path = argv[argc - 1];
    if ((err = tdb_open(db, path)))
        DIE("Could not open tdb at %s\n", path);

    reel_script_ctx *ctx = reel_script_new(db);
    if (!ctx)
        DIE("Couldn't initialize the Reel script. Out of memory?\n");
    initialize(ctx, db, argc, argv);

    /*
    if --select was enabled but no trails match,
    we don't need to eval anything
    */
    if (!(selected_trails && !num_selected))
        evaluate(db, ctx, path);
    else
        fprintf(stderr, "No trails match --select. No query executed.\n");

    printf("%s\n", reel_script_output_csv(ctx, ','));

    reel_script_free(ctx);
    tdb_close(db);
    return 0;
}
