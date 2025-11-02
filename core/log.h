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
#ifndef S_LOG_H_
#define S_LOG_H_
#include "ringbuffer.h"
#include "static-tests.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdnoreturn.h>
#include "int.h"

/* `core/log` - a simple, portable and thread-safe logging API.
 *
 * THE FOLLOWING SECTIONS ARE INFORMATIVE.
 *
 * To use the logger, simply include this
 * and define `MODULE_NAME` to a string literal of your choosing
 * (e.g. `#define MODULE_NAME "test"`)
 * at the start of your source file.
 * Using any logging macros before the `MODULE_NAME` definition
 * will result in a compiler error.
 *
 * Now you can use any of the logging functions:
 *     `s_log_trace`, `s_log_debug`, `s_log_verbose`, s_log_info`,
 *     `s_log_warn`, `s_log_error` and `s_log_fatal`
 * to log messages. That's it!
 *
 * Well, not really.
 * At program startup, the messages will be logged to an in-memory ring buffer
 * of size `S_LOG_DEFAULT_MEMBUF_SIZE`, so it's recommended to set the output
 * to an actual file stream as early as possible.
 * To do this, you should use `s_configure_log_output`(s).
 *
 * This is a simple example that will set the output streams to stdout/stderr:
 * ```
 * struct s_log_output_cfg log_out_cfg = {
 *     .type = S_LOG_OUTPUT_FILE,
 *     .out.file = stdout,
 *
 *      // To preserve any previous logs already stored in the buffer
 *     .flag_copy = true,
 * };
 * if (s_configure_log_outputs(S_LOG_STDOUT_MASKS, &log_out_cfg))
 *     s_log_fatal("Failed to configure stdout log output. Stop.");
 *
 *
 * log_out_cfg.out.file = stderr;
 * if (s_configure_log_outputs(S_LOG_STDERR_MASKS, &log_out_cfg))
 *     s_log_fatal("Failed to configure error log output. Stop.");
 *
 * ```
 *
 * If you set the output to something you manage yourself, then don't forget
 * to clean it up by first setting the output type to `S_LOG_OUTPUT_NONE`
 * (or calling `s_log_cleanup_all()` if you want to close all levels at once),
 * and then closing your handle.
 *
 * This module has no dependencies outside of `core/`.
 * Thread-safety is implemented through the use of `stdatomic`
 * and spinlocks (see `core/spinlock.h`) on global variables.
 *
 * Note that due to the use of stdio `FILE *` handles,
 * this API should not be considered async-signal safe.
 * While theoretically writing to a `S_LOG_OUTPUT_MEMORYBUF` is okay
 * due to the use `stdatomic`, please don't use this inside signal handlers.
 */

#define S_LOG_LEVEL_LIST                                                    \
    /* Trace logging - used to debug pieces of code                         \
     * that run multiple times (e.g. in a nested loop).                     \
     * Needs to be explicitly enabled by defining `CGD_ENABLE_TRACE`        \
     * and is always disabled in release builds */                          \
    X_(S_LOG_TRACE)                                                         \
                                                                            \
    /* Debug logging - used for "print debugging" of code that runs once    \
     * (or maybe a couple of times). Disabled entirely in release builds. */\
    X_(S_LOG_DEBUG)                                                         \
                                                                            \
    /* Verbose logging - basically the same as `S_LOG_DEBUG`,               \
     * but doesn't get disabled in release builds. */                       \
    X_(S_LOG_VERBOSE)                                                       \
                                                                            \
    /* Info - used for marking important points in the execution            \
     * of the program.                                                      \
     * Messages should be somewhat understandable to the end user.          \
     * This level should be the default in release builds. */               \
    X_(S_LOG_INFO)                                                          \
                                                                            \
    /* Warning - used for warning the user of some minor issue              \
     * that might be useful in diagnosing a more important error,           \
     * if one occurs.                                                       \
     * Messages should be somewhat understandable to the end user.          \
     * Should be printed to an "error" stream, if applicable. */            \
    X_(S_LOG_WARNING)                                                       \
                                                                            \
    /* Error - used to notify the user about some non-fatal error.          \
     * The messages should be clear and understandable.                     \
     * Obviously, they should be printed to an "error" stream. */           \
    X_(S_LOG_ERROR)                                                         \
                                                                            \
    /* Fatal error - used only in cases where the program can't             \
     * continue execution and must terminate immediately,                   \
     * such as in out-of-memory conditions.                                 \
     * The messages should be very clear about exactly what went wrong. */  \
    X_(S_LOG_FATAL_ERROR)                                                   \

#define X_(name) name,
/* Log levels. These specify the "priority" of a given message.
 * Any messages with a level below the active one will not be logged
 * (e.g. when the level is `S_LOG_INFO`, any
 *  `S_LOG_VERBOSE`, `S_LOG_DEBUG` and `S_LOG_TRACE` messages
 *  will be ignored).
 *
 * To disable all logging entirely, call
 * `s_configure_log_level(S_LOG_DISABLED)`.
 *
 * Use `s_configure_log_level` and `s_get_log_level`
 * to manage the current log level.
 *
 * See the above declarations (`S_LOG_LEVEL_LIST`)
 * for more detailed explanations of each level.
 */
enum s_log_level {
    S_LOG_LEVEL_LIST
    S_LOG_N_LEVELS_,
    S_LOG_DISABLED,
};
#undef X_

/* Messages (after format conversion!) longer than this
 * are not guaranteed to be logged properly */
#define S_LOG_MAX_SIZE 4096

/* The core function of the logging API.
 * Should not be called directly.
 * Use the below log level-specific macros instead. */
void s_log(enum s_log_level level, const char *module_name,
    const char *fmt, ...);

#ifndef CGD_BUILDTYPE_RELEASE

#ifdef CGD_ENABLE_TRACE
/* Logs a message with the level `S_LOG_TRACE` (see `S_LOG_LEVEL_LIST`)
 * Must be explicitly enabled by defining `CGD_ENABLE_TRACE`.
 * Always disabled in release builds. */
#define s_log_trace(...) s_log(S_LOG_TRACE, MODULE_NAME, __VA_ARGS__)
#else
#define s_log_trace(...) ((void)0)
#endif /* CGD_ENABLE_TRACE */

/* Logs a message with the level `S_LOG_DEBUG` (see `S_LOG_LEVEL_LIST`).
 * Disabled in release builds. */
#define s_log_debug(...) s_log(S_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)

#else
#define s_log_trace(...) ((void)0)
#define s_log_debug(...) ((void)0)
#endif /* CGD_BUILDTYPE_RELEASE */

/* Logs a message with the level `S_LOG_VERBOSE` (see `S_LOG_LEVEL_LIST`). */
#define s_log_verbose(...) s_log(S_LOG_VERBOSE, MODULE_NAME, __VA_ARGS__)

/* Logs a message with the level `S_LOG_INFO` (see `S_LOG_LEVEL_LIST`). */
#define s_log_info(...) s_log(S_LOG_INFO, MODULE_NAME, __VA_ARGS__)

/* Logs a message with the level `S_LOG_WARNING` (see `S_LOG_LEVEL_LIST`). */
#define s_log_warn(...) s_log(S_LOG_WARNING, MODULE_NAME, __VA_ARGS__)

/* Logs a message with the level `S_LOG_ERROR` (see `S_LOG_LEVEL_LIST`). */
#define s_log_error(...) s_log(S_LOG_ERROR, MODULE_NAME, __VA_ARGS__)

/* abort()s the program, while printing `error_msg_fmt` along with the
 * `module_name` and `function_name` to an error stream.
 * Don't call this function directly; use `s_log_fatal` instead. */
noreturn void s_abort(const char *module_name, const char *function_name,
    const char *error_msg_fmt, ...);

/* A wrapper around `s_abort`.
 * Used to immediately terminate the program's execution
 * while logging a message with the level `S_LOG_FATAL_ERROR`. */
#define s_log_fatal(...) s_abort(MODULE_NAME, __func__, __VA_ARGS__)

/* A better `assert` */
#define s_assert(expr, /* msg on fail */...) do {                               \
    if (!(expr)) {                                                              \
        s_log_error("Assertion failed: '%s'", #expr);                           \
        s_log_fatal(__VA_ARGS__);                                               \
    }                                                                           \
} while (0)

/* A better `assert` that also returns the expression result.
 * Used e.g. to put `s_assert` in an `if` statement, like so:
 *  `if (s_assert(expr1) && expr2) { ... }`
 */
#define s_assert_and_eval(expr, /* msg on fail */...) ((expr) ? 1 : (       \
    s_log_error("Assertion failed: '%s'", #expr),                           \
    s_log_fatal(__VA_ARGS__), 0)                                            \
)

/* Sets the active log level. */
void s_configure_log_level(enum s_log_level new_log_level);

/* Retrieves the current log level. */
enum s_log_level s_get_log_level(void);

/* The struct used to configure an output stream
 * for a given log level. */
struct s_log_output_cfg {
    enum s_log_output_type {
        /* Messages are written to a user-specified `FILE *` handle,
         * which shall not be closed before changing
         * the output stream to something else.
         *
         * The user is responsible for managing the lifetime of the handle. */
        S_LOG_OUTPUT_FILE,

        /* `s_configure_log_output` will try to open a file
         * at the user-specified path, and write the logs to it.
         *
         * The `FILE *` handle is automatically closed
         * on the next output configuration change. */
        S_LOG_OUTPUT_FILEPATH,

        /* The messages are logged to an in-memory ring buffer,
         * so when the end of it is reached, any new text will
         * wrap around to the beginning, overwriting the previous content.
         *
         * See `struct s_log_output_memorybuf` for more details. */
        S_LOG_OUTPUT_MEMORYBUF,

        /* The log level is completely disabled;
         * any messages using it are ignored. */
        S_LOG_OUTPUT_NONE,
    } type;
    union s_log_output_handle {
        /* `S_LOG_OUTPUT_FILE`: The file handle to which logs will be written */
        FILE *file;

        /* `S_LOG_OUTPUT_FILEPATH`: The path to the file
         * to which logs will be written */
        const char *filepath;

        /* `S_LOG_OUTPUT_MEMORYBUF`: The in-memory ring buffer
         * to which logs will be written.
         *
         * Important note: The user is fully responsible for managing
         * the `membuf` pointer. It must not go out of scope or get otherwise
         * freed or invalidated before the output is set to something else.
         *
         * If `membuf->buf_size` is smaller than `S_LOG_MINIMAL_MEMBUF_SIZE`,
         * the configuration will be rejected. */
#define S_LOG_MINIMAL_MEMBUF_SIZE 16
        struct ringbuffer *membuf;

    } out;

    /* Additional log output configuration parameters */
    enum s_log_config_flags {
        /* Used only by `S_LOG_OUTPUT_FILEPATH`.
         * If set, the new log file will be opened in append mode,
         * avoiding overwriting it's previous contents. */
        S_LOG_CONFIG_FLAG_APPEND = 1 << 0,

        /* Used only when the previous output type is `S_LOG_OUTPUT_MEMORYBUF`.
         * If set, the entire contents of the buffer will be "dumped"
         * to the new output stream, avoiding the loss of logs.
         *
         * It's always recommended to set this flag when changing the output
         * from `S_LOG_OUTPUT_MEMORYBUF` to (hopefully) an actual file
         *
         * Note that copying from one buffer to another is supported,
         * PROVIDED THAT THE NEW AND OLD BUFFERS DON'T OVERLAP! */
        S_LOG_CONFIG_FLAG_COPY = 1 << 1,

        /* Specifies whether the line format string (see `s_configure_log_line`)
         * should be stripped of any ANSI terminal escape sequences.
         *
         * Note that this only applies to the line format string,
         * NOT to the main message (in `fmt` and the varargs).
         *
         * It's recommended to set this flag when writing to a file on disk,
         * while leaving it `false` when writing to `stdout`/`stderr`. */
        S_LOG_CONFIG_FLAG_STRIP_ESC_SEQUENCES = 1 << 2,
    } flags;
};
/* Set the output stream of `level` to one specified in `in_new_cfg`,
 * while returning the previous configuration to `out_old_cfg`.
 *
 * If `in_new_cfg` is `NULL`, the configuration won't be changed,
 * while if `out_old_cfg` is NULL, the configuration won't be read.
 * Both can be `NULL` (although in that case nothing happens).
 *
 * Returns 0 on success and non-zero on failure.
 */
i32 s_configure_log_output(enum s_log_level level,
    const struct s_log_output_cfg *in_new_cfg,
    struct s_log_output_cfg *out_old_cfg);

#define X_(name) name##_MASK = 1 << name,
/* Used to specify which log levels should be configured */
enum s_log_level_mask {
    S_LOG_LEVEL_LIST
};
#undef X_
static_assert(sizeof(enum s_log_level_mask) <= sizeof(u32),
    "The size of the log level mask enum must be within the size of a u32.");

/* Specifies the levels that are associated with the `stdout` stream */
#define S_LOG_STDOUT_MASKS (S_LOG_TRACE_MASK | S_LOG_DEBUG_MASK \
        | S_LOG_VERBOSE_MASK | S_LOG_INFO_MASK)

/* Specifies the levels that are associated with the `stderr` stream */
#define S_LOG_STDERR_MASKS (S_LOG_WARNING_MASK | S_LOG_ERROR_MASK \
        | S_LOG_FATAL_ERROR_MASK)

/* Specifies all the log levels */
#define S_LOG_ALL_MASKS (S_LOG_STDOUT_MASKS | S_LOG_STDERR_MASKS)

/* A short-hand for configuring multiple log levels, specified in `level_mask`,
 * to use the same output stream configured with `cfg`.
 *
 * `level_mask` is a bit wise OR of members of `enum s_log_level_mask`.
 *
 * Returns the number of levels that failed to initialize (so 0 on success).
 */
i32 s_configure_log_outputs(u32 level_mask, const struct s_log_output_cfg *cfg);

#define S_LOG_LINEFMT_MAX_SIZE 64
#define S_LOG_LINE_SHORTFMT_MAX_SIZE 128
/* Used to configure the format of the log messages for a given `level`.
 *
 * `in_new_line` specifies the new format string to be used.
 * If it's `NULL`, the current configuration is left unchanged.
 *
 * If `out_old_line` is not `NULL`, the current line configuration
 * will be stored in it.
 *
 * The line is a printf-style format string, where:
 *  - `%m` expands to the module name,
 *  - `%s` expands to the full message.
 *
 * Example: `s_configure_log_line(S_LOG_INFO, "[%m] %s\n", NULL);`
 *
 * Note that configuring the log line for `S_LOG_FATAL_ERROR`
 * is not supported and does nothing.
 */
void s_configure_log_line(enum s_log_level level,
    const char *in_new_line, const char **out_old_line);

/* Closes all `S_LOG_OUTPUT_FILEPATH` handles
 * and frees all `S_LOG_OUTPUT_MEMORYBUF`-managed buffers. */
void s_log_cleanup_all(void);

#ifndef S_LOG_LEVEL_LIST_DEF__
#undef S_LOG_LEVEL_LIST
#endif /* S_LOG_LEVEL_LIST_DEF__ */

#endif /* S_LOG_H_ */
