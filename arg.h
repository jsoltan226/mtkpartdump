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
#ifndef PARSE_ARGS_H_
#define PARSE_ARGS_H_

#include <core/int.h>
#include <core/vector.h>

#define ARG_OPTIONS_LIST                                                       \
    X_(HELP, h, "help", "Show this message and exit")                          \
    X_(VERSION, V, "version", "Print the program version and exit")            \
    X_(VERBOSE, v, "verbose", "Enable verbose logging")                        \
    X_(CHAIN, c, "chain", "Process all headers found in a header chain")       \
    X_(SAVE_HDR, s, "save-headers", "Save binary header contents to disk")     \
    X_(EXTRACT_PART, e, "extract-parts", "Extract binary partition contents")  \

#define X_(name, short, long, desc) ARG_OPT_##name,
enum mtkpartdump_arg_options {
    ARG_OPTIONS_LIST
    ARG_MAX_
};
#undef X_

#define X_(name, short, long, decs) ARG_FLAG_##name = 1 << ARG_OPT_##name,
enum mtkpartdump_arg_option_flags {
    ARG_OPTIONS_LIST
};
#undef X_

i32 arg_parse(i32 argc, char **argv,
    VECTOR(const char *) *o_file_paths, u32 *o_flags);

const char * arg_get_help_options_string(void);


#ifndef ARG_OPTIONS_LIST_DEF__
#undef ARG_OPTIONS_LIST
#endif /* ARG_OPTIONS_LIST_DEF__ */

#endif /* PARSE_ARGS_H_ */
