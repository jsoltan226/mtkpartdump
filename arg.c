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
#define ARG_OPTIONS_LIST_DEF__
#include "arg.h"
#undef ARG_OPTIONS_LIST_DEF__
#include <core/log.h>
#include <core/int.h>
#include <core/util.h>
#include <core/vector.h>
#include <string.h>

#define MODULE_NAME "arg"

i32 arg_parse(i32 argc, char **argv,
    VECTOR(const char *) *o_file_paths, u32 *o_flags)
{
    if (argc <= 1) {
        s_log_error("Not enough arguments");
        return 1;
    }

    if (*o_file_paths == NULL)
        *o_file_paths = vector_new(const char *);

    for (i32 i = 1; i < argc; i++) {
        if (argv[i] == NULL)
            goto_error("argv[%d] is NULL!", i);

        const char first_char = argv[i][0];
        if (first_char != '-') {
            /* Not an option, just a file argument */
            vector_push_back(o_file_paths, argv[i]);
            continue;
        }

        /* Until we've checked that the first char isn't a null terminator,
         * we can't guarantee that the 2nd one exists */
        const char second_char = argv[i][1];

        /* Long (`--`) option */
        if (argv[i][1] == '-') {
#define X_(name, short, long, desc) "--"long,
            static const char *long_opts[ARG_MAX_] = {
                ARG_OPTIONS_LIST
            };
#undef X_

            bool found = false;
            for (u32 opt = 0; opt < ARG_MAX_; opt++) {
                if (!strcmp(argv[i], long_opts[opt])) {
                    *o_flags |= 1 << opt; /* ARG_FLAG_... */
                    found = true;
                    break;
                }
            }

            if (!found)
                goto_error("Unknown option \"%s\"", argv[i]);
        }

        /* Short (`-`) option */
        else if (
            (second_char >= 'a' && second_char <= 'z') ||
            (second_char >= 'A' && second_char <= 'Z') ||
            (second_char >= '0' && second_char <= '9')
        ) {
            for (char *chr_p = &argv[i][1]; *chr_p != '\0'; chr_p++) {
                /* We can't use a switch/case here due to the preprocessor's
                 * limitations (can't expand >c< into char literal >'c'<). */

#define X_(name, short, long, desc) #short,
                static const char *short_opts[ARG_MAX_] = {
                    ARG_OPTIONS_LIST
                };
#undef X_
                bool found = false;
                for (u32 opt = 0; opt < ARG_MAX_; opt++) {
                    if (*chr_p == short_opts[opt][0]) {
                        *o_flags |= 1 << opt; /* ARG_FLAG_... */
                        found = true;
                        break;
                    }
                }

                if (!found)
                    goto_error("Unknown option \"-%c\"", *chr_p);
            }
        }

        else
            goto_error("Unknown option \"%s\"", argv[i]);
    }

    return 0;

err:
    vector_destroy(o_file_paths);
    return 1;
}

const char *arg_get_help_options_string(void)
{
    return "Available options:\n"
#define X_(name, short, long, desc) \
    "    " "-"#short", --"long": "desc"\n"
        ARG_OPTIONS_LIST
#undef X_
        ;
}
