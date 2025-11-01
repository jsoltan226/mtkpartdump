#include "mtkpartdump.h"
#include <core/log.h>
#include <core/int.h>
#include <core/util.h>
#include <core/vector.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "main"

static i32 setup_log(void);

static i32 parse_args(i32 argc, char **argv,
    VECTOR(const char *) *o_file_paths, u32 *o_flags);

#define print_usage() do {                                                     \
    s_log_info("Usage: mtkpartdump [OPTIONS...] <FILE1> [FILE2 FILE3 ...]");   \
    s_log_info("Available options:\n"                                          \
            "    -h, --help: show this message\n"                              \
            "    -c, --chain: dump all headers found in a header chain");      \
} while (0)

enum mtkpartdump_arg_flags {
    ARG_HELP            = 1 << 0,
    ARG_CHAIN           = 1 << 1,
    ARG_MAX_
};

i32 main(i32 argc, char **argv)
{
    VECTOR(const char *) file_paths = NULL;
    u32 flags = 0;

    if (setup_log()) {
        fprintf(stderr, "Log setup failed. Stop.\n");
        return EXIT_FAILURE;
    }

    s_log_debug("mtkpartdump");

    if (parse_args(argc, argv, &file_paths, &flags)) {
        print_usage();
        goto err;
    } else if (flags & ARG_HELP) {
        print_usage();
        goto cleanup;
    }

    for (u32 i = 0; i < vector_size(file_paths); i++) {
        const char *path = file_paths[i];

        FILE *fp = fopen(path, "rb");
        if (fp == NULL) {
            s_log_error("Failed to open \"%s\": %s", path, strerror(errno));
            goto err;
        }
        s_log_verbose("Processing file \"%s\"...", path);

        mtkpart_dump_file(fp, flags & ARG_CHAIN);

        s_log_verbose("Done processing \"%s\"", path);
        if (fclose(fp))
            s_log_error("Failed to close \"%s\": %s", path, strerror(errno));
    }

cleanup:
    if (file_paths != NULL) vector_destroy(&file_paths);
    s_log_verbose("Exiting with code EXIT_SUCCESS");
    s_log_cleanup_all();
    return EXIT_SUCCESS;

err:
    if (file_paths != NULL) vector_destroy(&file_paths);
    s_log_error("Exiting with code EXIT_FAILURE");
    s_log_cleanup_all();
    return EXIT_FAILURE;
}

static i32 setup_log(void) {
    struct s_log_output_cfg cfg = {
        .type = S_LOG_OUTPUT_FILE,
        .out.file = stdout,
        .flags = S_LOG_CONFIG_FLAG_COPY
    };

    if (s_configure_log_outputs(S_LOG_STDOUT_MASKS, &cfg)) {
        fprintf(stderr, "Failed to configure stdout log outputs\n");
        return 1;
    }

    cfg.out.file = stderr;
    if (s_configure_log_outputs(S_LOG_STDERR_MASKS, &cfg)) {
        fprintf(stderr, "Failed to configure stderr log outputs\n");
        return 1;
    }

    s_configure_log_level(S_LOG_INFO);

    return 0;
}

static i32 parse_args(i32 argc, char **argv,
    VECTOR(const char *) *o_file_paths, u32 *o_flags)
{
    if (*o_file_paths == NULL)
        *o_file_paths = vector_new(const char *);

    for (i32 i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            /* Some kind of an option */

            if (argv[i][1] == '-') {
                /* Long (`--`) option */
                if (!strcmp(argv[i], "--help")) {
                    *o_flags |= ARG_HELP;
                } else if (!strcmp(argv[i], "--chain")) {
                    *o_flags |= ARG_CHAIN;
                } else {
                    s_log_error("Unknown option \"%s\"", argv[i]);
                    return 1;
                }
            } else if (argv[i][1] >= 'a' && argv[i][1] <= 'z') {
                /* Short (`-`) option */
                for (char *chr_p = &argv[i][1]; *chr_p != '\0'; chr_p++) {
                    switch (*chr_p) {
                    case 'h':
                        *o_flags |= ARG_HELP;
                        break;
                    case 'c':
                        *o_flags |= ARG_CHAIN;
                        break;
                    default:
                        s_log_error("Unknown option \"-%c\"", *chr_p);
                        return 1;
                    }
                }
            }
        } else {
            /* Not an option, just a file argument */
            vector_push_back(o_file_paths, argv[i]);
        }
    }

    /* If the argument count is too small or
     * no file arguments were found (and `--help` wansn't supplied),
     * error out */
    if (argc <= 1 ||
        (!(*o_flags & ARG_HELP) && vector_size(o_file_paths) == 0))
    {
        s_log_error("Not enough arguments!");
        vector_destroy(o_file_paths);
        return 1;
    }

    return 0;
}
