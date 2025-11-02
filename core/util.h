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
#ifndef UTIL_H_
#define UTIL_H_
#include "static-tests.h"

#include "log.h"
#include "int.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define u_BUF_SIZE  1024
#define u_PATH_FROM_BIN_TO_ASSETS "../assets/"

#define u_FILEPATH_SIZE 256
#define u_FILEPATH_MAX (u_FILEPATH_SIZE - 1)
typedef char u_filepath_t[u_FILEPATH_SIZE];

#define goto_error(...) do {    \
    s_log_error(__VA_ARGS__);   \
    goto err;                   \
} while (0)

#define u_check_params(expr) s_assert(expr, "invalid parameters");

#define u_color_arg_expand(color) color.r, color.g, color.b, color.a

#define u_arr_size(arr) (sizeof(arr) / sizeof(*arr))

#define u_strlen(str_literal) (sizeof(str_literal) - 1)

#define u_nbits(x) ((((x) - 1) / (8 * sizeof(u64))) + 1)

#define u_str_helper_(x) #x
#define u_str(x) u_str_helper_(x)

#define u_generic64_zero(x) (_Generic((x),              \
    char: 0, signed char: 0, unsigned char: 0,          \
    short: 0, int: 0, long: 0, long long: 0,            \
    unsigned short: 0, unsigned int: 0,                 \
        unsigned long: 0, unsigned long long: 0,        \
    float: 0., double: 0., long double: 0.,             \
    default: (void *)0))

#define u_macro_type_check(macro_name, type, x)             \
static_assert(                                              \
    _Generic((x), type: 1, default: 0),                     \
    MODULE_NAME ":" u_str(__LINE__) ":" #macro_name ": "    \
        "Type of `" #x "` invalid (should be `" #type "`)"  \
)                                                           \

#define u_rgba_swap_b_r(color) do {     \
    register const u8 tmp = color.b;    \
    color.b = color.r;                  \
    color.r = tmp;                      \
} while (0)

/* Free and nullify */
#define u_nfree(ptr_ptr) do {   \
    free(*(ptr_ptr));           \
    *(ptr_ptr) = NULL;          \
} while (0)

/* Zero-out and free */
#define u_zfree(ptr_ptr) do {                   \
    memset(*(ptr_ptr), 0, sizeof(**(ptr_ptr))); \
    free(*(ptr_ptr));                           \
} while (0)

/* Zero-out, free and nullify */
#define u_nzfree(ptr_ptr) do {                  \
    memset(*(ptr_ptr), 0, sizeof(**(ptr_ptr))); \
    free(*(ptr_ptr));                           \
    *(ptr_ptr) = NULL;                          \
} while (0)

#endif /* UTIL_H_ */
