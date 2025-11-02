/* mtkpartdump - Mediatek partition dump tool
 * Copyright (C) 2025 Jan Sołtan <jsoltan226@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>
*/
#include "arg.h"
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
static void print_usage(void);
static void print_version(void);

i32 main(i32 argc, char **argv)
{
    VECTOR(const char *) file_paths = NULL;
    u32 flags = 0;

    if (setup_log()) {
        fprintf(stderr, "Log setup failed. Stop.\n");
        return EXIT_FAILURE;
    }

    s_log_debug("mtkpartdump");

    if (arg_parse(argc, argv, &file_paths, &flags)) {
        print_usage();
        goto err;
    }

    const bool exit_early =
        (flags & ARG_FLAG_HELP) || (flags & ARG_FLAG_VERSION);

    if (flags & ARG_FLAG_VERSION) print_version();
    if (flags & ARG_FLAG_HELP) print_usage();

    if (exit_early) {
        goto cleanup;
    } else if (!exit_early && vector_size(file_paths) == 0) {
        s_log_error("No files were specified");
        goto err;
    }

    if (flags & ARG_FLAG_VERBOSE)
        s_configure_log_level(S_LOG_DEBUG);

    for (u32 i = 0; i < vector_size(file_paths); i++) {
        const char *path = file_paths[i];

        FILE *fp = fopen(path, "rb");
        if (fp == NULL) {
            s_log_error("Failed to open \"%s\": %s", path, strerror(errno));
            goto err;
        }
        s_log_verbose("Processing file \"%s\"...", path);

        mtkpart_dump_file(fp, flags);

        s_log_verbose("Done processing \"%s\"", path);
        if (fclose(fp)) {
            s_log_error("Failed to close \"%s\": %s", path, strerror(errno));
            goto err;
        }
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

static void print_usage(void)
{
    s_log_info("Usage: mtkpartdump [OPTIONS...] <FILE1> [FILE2 FILE3 ...]");
    s_log_info("Inspect and extract MediaTek logical partitions "
        "from firmware blobs");

    const char *old_line = NULL;
    s_configure_log_line(S_LOG_INFO, "%s", &old_line);

    s_log_info("%s", arg_get_help_options_string());

    s_configure_log_line(S_LOG_INFO, old_line, NULL);
}

static void print_version(void)
{
    s_log_info("mtkpartdump v1.0 (Mediatek partition header dump tool)");
    s_log_info("Copyright (C) 2025, Jan Sołtan <jsoltan226@gmail.com>");
    s_log_info("License GPLv3+: GNU GPL version 3 or later "
        "<https://gnu.org/licenses/gpl.html>");
}
