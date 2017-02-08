
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

static long num_threads;

struct job_arg{
    tdb *db;
    reel_script_ctx *ctx;
    uint64_t shard_idx;
    uint64_t start_trail;
    uint64_t end_trail;
};

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

        if (!((trail_id - arg->start_trail) & 65535)){
            uint64_t perc = (100 * (trail_id - arg->start_trail)) /
                            (arg->end_trail - arg->start_trail);
            fprintf(stderr, "[thread %lu] %lu%% trails evaluated\n", arg->shard_idx, perc);
        }

        if (tdb_get_trail(cursor, trail_id))
            DIE("tdb_get_trail failed\n");

        if (!(events = reel_event_buffer_fill(buf, cursor, &num_events)))
            DIE("Event buffer out of memory\n");

        if ((err = reel_script_eval_trail(arg->ctx, events, num_events)))
            DIE("[trail %"PRIu64"] Script failed: %s\n",
                trail_id,
                reel_error_str(err));
    }
    fprintf(stderr, "[thread %lu] 100%% trails evaluated\n", arg->shard_idx);

    reel_event_buffer_free(buf);
    tdb_cursor_free(cursor);
    return NULL;
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
"-s --set var=value    Set a variable in the Reel script.\n"
"                      Use var=@filename to read value from a file.\n"
"-T --threads N        Use N parallel threads to execute the query.\n"
"\n");
    exit(1);
}

long int safely_to_int(const char *str, const char *field)
{
    char *end = NULL;
    errno = 0;
    long int x = strtol(str, &end, 10);
    if (errno || *end)
        DIE("Invalid %s: %s", field, str);
    return x;
}

static void initialize(reel_script_ctx *ctx, int argc, char **argv)
{
    static struct option long_options[] = {
        {"set", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    int c, option_index = 1;

    num_threads = 1;

    do{
        c = getopt_long(argc,
                        argv,
                        "s:T:",
                        long_options,
                        &option_index);

        switch (c){
            case -1:
                break;
            case 's':
                set_var(ctx, optarg);
                break;
            case 'T':
                num_threads = safely_to_int(optarg, "number of threads");
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
    initialize(ctx, argc, argv);
    evaluate(db, ctx, path);

    printf("%s\n", reel_script_output_csv(ctx, ','));

    reel_script_free(ctx);
    tdb_close(db);
    return 0;
}
