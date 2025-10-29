#define S_LOG_LEVEL_LIST_DEF__
#include "log.h"
#undef S_LOG_LEVEL_LIST_DEF__
#include "math.h"
#include "spinlock.h"
#include "ringbuffer.h"
#include "ansi-esc-sequences.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>

#define MODULE_NAME "log"

static void write_msg_to_file(FILE *fp,
    const char *linefmt, const char *module_name,
    const char *fmt, va_list vlist, bool strip_escape_sequences);
static void write_msg_to_membuf(struct ringbuffer *membuf,
    const char *linefmt, const char *module_name,
    const char *fmt, va_list vlist, bool strip_escape_sequences);

static enum linefmt_ret {
    LINEFMT_END,
    LINEFMT_SHORT,
    LINEFMT_MODULE_NAME,
    LINEFMT_MESSAGE,
} linefmt_next_token(const char *linefmt, u64 *linefmt_index_p,
    char *short_buf, u64 short_buf_size);

static noreturn void do_abort_v(const char *module_name,
    const char *function_name, const char *fmt, va_list vlist);

static void read_output_config(struct s_log_output_cfg *o,
    enum s_log_level level);
static i32 try_set_output_config(const struct s_log_output_cfg *i,
    enum s_log_level level, bool force);

/* Only used in `try_set_output_config` */
union tmp_output_data;
struct output;
static i32 try_init_new_output(enum s_log_level level,
    const struct s_log_output_cfg *i, union tmp_output_data *o);
static void destroy_old_output(struct output *o);
static void store_new_output(struct output *o,
    const struct s_log_output_cfg *i, const union tmp_output_data *i_tmp_data);

static void copy_old_data(union tmp_output_data *new_output,
    enum s_log_output_type new_type, enum s_log_level level);

static void strip_escape_sequences(char *out, u32 out_size, const char *in);

#define X_(name) [name] = #name,
static const char *const log_level_strings[S_LOG_N_LEVELS_] = {
    S_LOG_LEVEL_LIST
};
#undef X_

/*** GLOBAL CONFIGURATION ***/

static _Atomic enum s_log_level g_log_level = ATOMIC_VAR_INIT(S_LOG_TRACE);

/** LOG LINE STRINGS **/
#define LINE_STRING_LIST                                                    \
    init_line_string(S_LOG_TRACE, es_DIM "T [%m] %s" es_COLOR_RESET "\n")   \
    init_line_string(S_LOG_DEBUG, es_GRAY "D " es_COLOR_RESET "[%m] %s\n")  \
    init_line_string(S_LOG_VERBOSE, "V [%m] %s\n")                          \
    init_line_string(S_LOG_INFO, es_BOLD "I " es_COLOR_RESET "[%m] %s\n")   \
    init_line_string(S_LOG_WARNING, \
            es_BOLD es_FG_YELLOW  "W " es_COLOR_RESET "[%m] %s\n")          \
    init_line_string(S_LOG_ERROR,   \
            es_UNDERLINE es_BOLD es_FG_RED "E " es_COLOR_RESET \
            es_UNDERLINE "[%m] %s" es_COLOR_RESET "\n")                     \
    init_line_string(S_LOG_FATAL_ERROR, "[%m] %s\n")                        \

/* Ensure that all the default level strings are <= S_LOG_LINEFMT_MAX_SIZE */
#define init_line_string(level, str)                                        \
    static_assert(sizeof(str) <= S_LOG_LINEFMT_MAX_SIZE,                    \
        "Default line format string for log level \"" #level "\" is too long");
LINE_STRING_LIST
#undef init_line_string

/* Actually define the default level strings */
#define init_line_string(level, str) [level] = str,
static const char *const g_default_log_lines[S_LOG_N_LEVELS_] = {
    LINE_STRING_LIST
};
#undef init_line_string

#undef LINE_STRING_LIST

/* The actual configuration variable that's read from and written to */
#define X_(name) [name] = ATOMIC_VAR_INIT(g_default_log_lines[name]),
static const char *_Atomic g_log_lines[S_LOG_N_LEVELS_] = {
    S_LOG_LEVEL_LIST
};
#undef X_
/** END LOG LINE STRINGS **/

/** LOG OUTPUT **/

struct output {
        enum s_log_output_type type;
        spinlock_t cfg_lock;

        /* Used by `S_LOG_OUTPUT_FILE` and `S_LOG_OUTPUT_FILEPATH` */
        FILE *fp;

        /* Used only by `S_LOG_OUTPUT_FILEPATH` */
        const char *filepath;

        /* Only used by `S_LOG_OUTPUT_MEMORYBUF` */
        struct ringbuffer *membuf;

        bool strip_esc_sequences;
};

/* The default output configuration
 * (All levels use the `MEMBUF` output type,
 * where `TRACE`, `DEBUG`, `VERBOSE` and `INFO` share the `out` buffer,
 * while `WARNING`, `ERROR` and `FATAL_ERROR` use the `err` buffer. */

/* The size of the `static` membufs used for initial logging */
#define S_LOG_DEFAULT_MEMBUF_SIZE 4096
static char g_default_out_membuf_buf[S_LOG_DEFAULT_MEMBUF_SIZE] = { 0 };
static struct ringbuffer g_default_out_membuf = {
    .buf = g_default_out_membuf_buf,
    .buf_size = S_LOG_DEFAULT_MEMBUF_SIZE,
    .write_index = ATOMIC_VAR_INIT(0),
};
static char g_default_err_membuf_buf[S_LOG_DEFAULT_MEMBUF_SIZE] = { 0 };
static struct ringbuffer g_default_err_membuf = {
    .buf = g_default_err_membuf_buf,
    .buf_size = S_LOG_DEFAULT_MEMBUF_SIZE,
    .write_index = ATOMIC_VAR_INIT(0),
};
static const struct output g_default_output_cfgs[S_LOG_N_LEVELS_] = {
#define default_output_config_template(out_membuf)                          \
    {                                                                       \
        .cfg_lock = SPINLOCK_INIT,                                          \
        .type = S_LOG_OUTPUT_MEMORYBUF,                                     \
        .membuf = &out_membuf,                                              \
    }

    [S_LOG_TRACE] = default_output_config_template(g_default_out_membuf),
    [S_LOG_DEBUG] = default_output_config_template(g_default_out_membuf),
    [S_LOG_VERBOSE] = default_output_config_template(g_default_out_membuf),
    [S_LOG_INFO] = default_output_config_template(g_default_out_membuf),

    [S_LOG_WARNING] = default_output_config_template(g_default_err_membuf),
    [S_LOG_ERROR] = default_output_config_template(g_default_err_membuf),
    [S_LOG_FATAL_ERROR] = default_output_config_template(g_default_err_membuf),

#undef default_output_config_template
};

/* The actual configuration variable that's read from and written to */
#define X_(name) [name] = g_default_output_cfgs[name],
static struct output g_output_cfgs[S_LOG_N_LEVELS_] = {
    S_LOG_LEVEL_LIST
};
#undef X_

#undef S_LOG_LEVEL_LIST

void s_log(enum s_log_level level, const char *module_name,
    const char *fmt, ...)
{
    if (!(level >= 0 && level < S_LOG_N_LEVELS_))
        s_log_fatal("Invalid parameters: `level` (%d) "
            "not in range <0, S_LOG_N_LEVELS_ (%d)>",
            level, S_LOG_N_LEVELS_);

    if (module_name == NULL)
        s_log_fatal("Invalid parameters: `module_name` is NULL");

    if (fmt == NULL)
        s_log_fatal("Invalid parameters: `fmt` is NULL");

    if (level < atomic_load(&g_log_level))
        return;

    va_list fmt_list;
    va_start(fmt_list, fmt);

    if (level == S_LOG_FATAL_ERROR)
        do_abort_v(module_name, "(unknown)", fmt, fmt_list);

    struct output *const output = &g_output_cfgs[level];
    const char *linefmt_string = atomic_load(&g_log_lines[level]);

    switch (output->type) {
    case S_LOG_OUTPUT_FILE:
    case S_LOG_OUTPUT_FILEPATH:
        write_msg_to_file(output->fp,
            linefmt_string, module_name, fmt, fmt_list,
            output->strip_esc_sequences);
        break;
    case S_LOG_OUTPUT_MEMORYBUF:
        write_msg_to_membuf(output->membuf,
            linefmt_string, module_name, fmt, fmt_list,
            output->strip_esc_sequences);
        break;
    case S_LOG_OUTPUT_NONE:
        break;
    }

    va_end(fmt_list);
}

noreturn void s_abort(const char *module_name, const char *function_name,
    const char *fmt, ...)
{
    va_list vlist;
    va_start(vlist, fmt);
    do_abort_v(module_name, function_name, fmt, vlist);
    va_end(vlist); /* Technically unnecessary */
}

void s_configure_log_level(enum s_log_level new_log_level)
{
    atomic_store(&g_log_level, new_log_level);
}

enum s_log_level s_get_log_level(void)
{
    return atomic_load(&g_log_level);
}

i32 s_configure_log_output(enum s_log_level level,
    const struct s_log_output_cfg *in_new_cfg,
    struct s_log_output_cfg *out_old_cfg)
{
    if (!(level >= 0 && level < S_LOG_N_LEVELS_))
        s_log_fatal("Invalid parameters: `level` (%d) "
            "not in range <0, S_LOG_N_LEVELS_ (%d)>",
            level, S_LOG_N_LEVELS_);

    i32 ret = 0;
    if (out_old_cfg != NULL) {
        spinlock_acquire(&g_output_cfgs[level].cfg_lock);
        read_output_config(out_old_cfg, level);
        spinlock_release(&g_output_cfgs[level].cfg_lock);
    }

    if (in_new_cfg != NULL) {
        ret = try_set_output_config(in_new_cfg, level, false);
    }

    return ret;
}

i32 s_configure_log_outputs(u32 level_mask, const struct s_log_output_cfg *cfg)
{
    i32 n_failed = 0;
    for (u32 i = 0; i < S_LOG_N_LEVELS_; i++) {
        if (level_mask & (1 << i))
            n_failed += 1 & s_configure_log_output(i, cfg, NULL);
    }
    return n_failed;
}

void s_configure_log_line(enum s_log_level level,
    const char *in_new_line, const char **out_old_line)
{
    if (!(level >= 0 && level < S_LOG_N_LEVELS_))
        s_log_fatal("Invalid parameters: `level` (%d) "
            "not in range <0, S_LOG_N_LEVELS_ (%d)>",
            level, S_LOG_N_LEVELS_);

    if (out_old_line != NULL)
        *out_old_line = atomic_load(&g_log_lines[level]);

    if (in_new_line != NULL) {
        u64 new_line_size = strlen(in_new_line) + 1;
        if (new_line_size > S_LOG_LINEFMT_MAX_SIZE) {
            s_log_fatal("Invalid parameters: `in_new_line` is too long "
                "(%lu - max is %u)", new_line_size, S_LOG_LINEFMT_MAX_SIZE);
        }

        atomic_store(&g_log_lines[level], in_new_line);
    }
}

void s_log_cleanup_all(void)
{
    /* Close all the open log file streams */
    const struct s_log_output_cfg close_cfg = { .type = S_LOG_OUTPUT_NONE };
    for (u32 i = 0; i < S_LOG_N_LEVELS_; i++)
        (void) try_set_output_config(&close_cfg, i, true);
}

static void write_msg_to_file(FILE *fp,
    const char *linefmt, const char *module_name,
    const char *fmt, va_list vlist, bool strip_esc_sequences)
{
    char tmp_linefmt[S_LOG_LINEFMT_MAX_SIZE] = { 0 };
    if (strip_esc_sequences) {
        strip_escape_sequences(tmp_linefmt, sizeof(tmp_linefmt), linefmt);
    } else {
        (void) strncpy(tmp_linefmt, linefmt, sizeof(tmp_linefmt));
        tmp_linefmt[S_LOG_LINEFMT_MAX_SIZE - 1] = '\0';
    }

    char short_token_buf[S_LOG_LINE_SHORTFMT_MAX_SIZE] = { 0 };
    u64 tmp_linefmt_index = 0;
    enum linefmt_ret token_ret = linefmt_next_token(tmp_linefmt,
            &tmp_linefmt_index, short_token_buf, sizeof(short_token_buf));

    while (token_ret != LINEFMT_END) {
        switch (token_ret) {
        case LINEFMT_SHORT:
            fputs(short_token_buf, fp);
            break;
        case LINEFMT_MODULE_NAME:
            fputs(module_name, fp);
            break;
        case LINEFMT_MESSAGE:
            vfprintf(fp, fmt, vlist);
            break;
        default:
        case LINEFMT_END:
            s_log_fatal("Impossible outcome "
                "(invalid return value of `linefmt_next_token`)");
        }

        token_ret = linefmt_next_token(tmp_linefmt,
            &tmp_linefmt_index, short_token_buf, sizeof(short_token_buf));
    }
}

static void write_msg_to_membuf(struct ringbuffer *membuf,
    const char *linefmt, const char *module_name,
    const char *fmt, va_list vlist, bool strip_esc_sequences)
{
    if (membuf->buf_size < S_LOG_MINIMAL_MEMBUF_SIZE) {
        s_log_fatal("membuf size %lu is too small (the minimum is %lu)",
            membuf->buf_size, S_LOG_MINIMAL_MEMBUF_SIZE);
    }
    char tmp_linefmt[S_LOG_LINEFMT_MAX_SIZE] = { 0 };
    if (strip_esc_sequences) {
        strip_escape_sequences(tmp_linefmt, sizeof(tmp_linefmt), linefmt);
    } else {
        (void) strncpy(tmp_linefmt, linefmt, sizeof(tmp_linefmt));
        tmp_linefmt[S_LOG_LINEFMT_MAX_SIZE - 1] = '\0';
    }

    char short_token_buf[S_LOG_LINE_SHORTFMT_MAX_SIZE] = { 0 };
    u64 tmp_linefmt_index = 0;
    enum linefmt_ret token_ret = linefmt_next_token(tmp_linefmt,
            &tmp_linefmt_index, short_token_buf, sizeof(short_token_buf));

    char message_buf[S_LOG_MAX_SIZE] = { 0 };
    va_list vcopy;

    while (token_ret != LINEFMT_END) {
        switch (token_ret) {
        case LINEFMT_SHORT:
            ringbuffer_write_string(membuf, short_token_buf);
            break;
        case LINEFMT_MODULE_NAME:
            ringbuffer_write_string(membuf, module_name);
            break;
        case LINEFMT_MESSAGE:
            va_copy(vcopy, vlist);
            (void) vsnprintf(message_buf, S_LOG_MAX_SIZE, fmt, vcopy);
            va_end(vcopy);
            message_buf[S_LOG_MAX_SIZE - 1] = '\0';
            ringbuffer_write_string(membuf, message_buf);
            break;
        default:
        case LINEFMT_END:
            s_log_fatal("Impossible outcome "
                "(invalid return value of `linefmt_next_token`)");
        }

        token_ret = linefmt_next_token(tmp_linefmt,
            &tmp_linefmt_index, short_token_buf, sizeof(short_token_buf));
    }
}

static enum linefmt_ret linefmt_next_token(const char *linefmt,
    u64 *linefmt_index_p, char *short_buf, u64 short_buf_size)
{
    if (short_buf == NULL || short_buf_size <= 4)
        return LINEFMT_END;

    u64 i = *linefmt_index_p;

    if (linefmt[i] == '\0') {
        return LINEFMT_END;
    } else if (linefmt[i] != '%') {
        u64 j = 0;
        while (linefmt[i] != '%' && linefmt[i] != '\0'
                && i < short_buf_size - 1)
        {
            short_buf[j++] = linefmt[i++];
        }
        short_buf[j] = '\0';
        *linefmt_index_p = i;
        return LINEFMT_SHORT;
    } else if (linefmt[i] == '%') {
        const char c = linefmt[i + 1];

        /* Handle the special case where a the string ends
         * immidiately after the '%' character */
        if (c == '\0') {
            short_buf[0] = '%';
            short_buf[1] = '\0';
            *linefmt_index_p += 1;
            return LINEFMT_SHORT;
        }

        /* Advance by 2 chars - '%', '<c>' */
        *linefmt_index_p += 2;

        switch (c) {
        case 'm': return LINEFMT_MODULE_NAME;
        case 's': return LINEFMT_MESSAGE;
        default: case '%':
            short_buf[0] = '%';
            short_buf[1] = c;
            short_buf[2] = '\0';
            return LINEFMT_SHORT;
        }
    }

    return LINEFMT_END;
}


static noreturn void do_abort_v(const char *module_name,
    const char *function_name, const char *fmt, va_list vlist)
{
    FILE *err_fp = NULL;
    switch (g_output_cfgs[S_LOG_FATAL_ERROR].type) {
    case S_LOG_OUTPUT_FILE:
    case S_LOG_OUTPUT_FILEPATH:
        err_fp = g_output_cfgs[S_LOG_FATAL_ERROR].fp;
        break;
    case S_LOG_OUTPUT_MEMORYBUF:
    case S_LOG_OUTPUT_NONE:
        err_fp = stderr;
        break;
    }

    fprintf(err_fp, "[%s] FATAL ERROR: %s: ", module_name, function_name);
    vfprintf(err_fp, fmt, vlist);
    fprintf(err_fp, "\nFatal error encountered. Calling abort().\n");
    fflush(err_fp);

    s_log_cleanup_all();

    abort();
}

static void read_output_config(struct s_log_output_cfg *o,
    enum s_log_level level)
{
    memset(o, 0, sizeof(struct s_log_output_cfg));

    o->type = g_output_cfgs[level].type;
    const struct output *const cfg = &g_output_cfgs[level];
    switch (o->type) {
        case S_LOG_OUTPUT_FILE:
            o->out.file = cfg->fp;
            break;
        case S_LOG_OUTPUT_FILEPATH:
            o->out.filepath = cfg->filepath;
            break;
        case S_LOG_OUTPUT_MEMORYBUF:
            o->out.membuf = o->out.membuf;
            break;
        case S_LOG_OUTPUT_NONE:
            break;
    }

    o->flags = 0;
}

union tmp_output_data {
    FILE *fp;
    struct ringbuffer *buf;
};
static i32 try_set_output_config(const struct s_log_output_cfg *i,
    enum s_log_level level, bool force)
{
    struct output *const cfg = &g_output_cfgs[level];
    if (!force)
        spinlock_acquire(&cfg->cfg_lock);

    /* Initialize the new output */
    union tmp_output_data tmp_new_output = { 0 };
    if (try_init_new_output(level, i, &tmp_new_output)) {
        if (!force)
            spinlock_release(&cfg->cfg_lock);
        return 1;
    }

    /* Handle the "copy" flag */
    if (cfg->type == S_LOG_OUTPUT_MEMORYBUF &&
        i->flags & S_LOG_CONFIG_FLAG_COPY)
    {
        copy_old_data(&tmp_new_output, i->type, level);
        /* Clear the buffer after copying data,
         * to prevent duplication of messages when switching
         * to an output shared by multiple levels */
        memset(cfg->membuf->buf, 0, cfg->membuf->buf_size);
    }

    /* Destroy the old output */
    destroy_old_output(cfg);

    /* Switch to the new output */
    store_new_output(cfg, i, &tmp_new_output);

    if (!force)
        spinlock_release(&cfg->cfg_lock);
    return 0;
}

static i32 try_init_new_output(enum s_log_level level,
    const struct s_log_output_cfg *i, union tmp_output_data *o)
{
    switch (i->type) {
    case S_LOG_OUTPUT_FILE:
        if (i->out.file == NULL) {
            s_log_error("Invalid parameters: new log file handle "
                "(for level %s) is NULL", log_level_strings[level]);
            return 1;
        }
        o->fp = i->out.file;
        break;
    case S_LOG_OUTPUT_FILEPATH:
        if (i->out.filepath == NULL) {
            s_log_error("Invalid parameters: new log file path "
                "(for level %s) is NULL", log_level_strings[level]);
            return 1;
        }
        o->fp = fopen(i->out.filepath,
            i->flags & S_LOG_CONFIG_FLAG_APPEND ? "ab" : "wb");
        if (o->fp == NULL) {
            s_log_error("Failed to open new log file \"%s\" "
                "(for level %s): %s",
                log_level_strings[level], strerror(errno)
            );
            return 1;
        }
        break;
    case S_LOG_OUTPUT_MEMORYBUF:
        if (i->out.membuf == NULL) {
            s_log_error("Invalid parameters: new log ringbuffer handle "
                "(for level %s) is NULL", log_level_strings[level]);
            return 1;
        } else if (i->out.membuf->buf == NULL) {
            s_log_error("Invalid parameters: new log ringbuffer "
                "(for level %s) is NULL", log_level_strings[level]);
            return 1;
        } else if (i->out.membuf->buf_size < S_LOG_MINIMAL_MEMBUF_SIZE) {
            s_log_error("Invalid parameters: new log ringbuffer size "
                "(for level %s) is smaller than S_LOG_MINIMAL_MEMBUF_SIZE (%u)",
                log_level_strings[level], S_LOG_MINIMAL_MEMBUF_SIZE);
            return 1;
        }
        o->buf = i->out.membuf;
        break;
    case S_LOG_OUTPUT_NONE:
        break;
    }

    return 0;
}

static void destroy_old_output(struct output *o)
{
    if (o->type == S_LOG_OUTPUT_FILEPATH  || o->type == S_LOG_OUTPUT_FILE)
        (void) fflush(o->fp);

    if (o->type == S_LOG_OUTPUT_FILEPATH)
        (void) fclose(o->fp);

    memset(o, 0, sizeof(struct output));
}

static void store_new_output(struct output *o,
    const struct s_log_output_cfg *i, const union tmp_output_data *i_tmp_data)
{
    switch (i->type) {
    case S_LOG_OUTPUT_FILE:
        o->fp = i->out.file;
        break;
    case S_LOG_OUTPUT_FILEPATH:
        o->filepath = i->out.filepath;
        o->fp = i_tmp_data->fp;
        break;
    case S_LOG_OUTPUT_MEMORYBUF:
        o->membuf = i_tmp_data->buf;
        break;
    case S_LOG_OUTPUT_NONE:
        break;
    }
    o->strip_esc_sequences = (i->flags & S_LOG_CONFIG_FLAG_STRIP_ESC_SEQUENCES);
    o->type = i->type;
}

static void copy_old_data(union tmp_output_data *new_output,
    enum s_log_output_type new_type, enum s_log_level level)
{
    struct output *const cfg = &g_output_cfgs[level];
    const char *const c_p = memchr(cfg->membuf->buf,
        '\0', cfg->membuf->buf_size);
    const u64 n_bytes = c_p ?
        (u64)(c_p - cfg->membuf->buf) :
        cfg->membuf->buf_size;

    switch (new_type) {
    case S_LOG_OUTPUT_FILE:
    case S_LOG_OUTPUT_FILEPATH:
        (void) fwrite(cfg->membuf->buf, 1, n_bytes, new_output->fp);
        if (ferror(new_output->fp)) {
            s_log_error("Failed to copy over data from old membuf "
                "(for level %s): %s", log_level_strings[level],
                strerror(errno));
        }
        break;
    case S_LOG_OUTPUT_MEMORYBUF:
        memcpy(new_output->buf->buf, cfg->membuf->buf,
            u_min(cfg->membuf->buf_size, new_output->buf->buf_size)
        );
        break;
    case S_LOG_OUTPUT_NONE:
        break;
    }
}

static void strip_escape_sequences(char *out, u32 out_size, const char *in)
{
    memset(out, 0, out_size);

    bool esc = false, csi = false, csi_done_parameter = false;
    u32 i = 0, j = 0;
    do {
        /* C0 control codes */
        if (in[i] == es_ESC_chr) {
            esc = true;
            continue;
        } else if (es_is_C0_control_code(in[i]) &&
                in[i] != '\n' && in[i] != '\r') /* Keep the newlines */
        {
            esc = false;
            continue;
        }

        /* Fe escape sequences */
        if (esc && in[i] == '[') {
            esc = false;
            csi = true;
            continue;
        } else if (esc && es_is_Fe_code(in[i])) {
            esc = false;
            continue;
        }

        /* CSI commands */
        if (csi && es_is_CSI_terminator(in[i])) {
            csi = false;
            csi_done_parameter = false;
            continue;
        } else if (csi && es_is_CSI_parameter(in[i]) && !csi_done_parameter) {
            continue;
        } else if (csi && es_is_CSI_intermediate(in[i]) && !csi_done_parameter) {
            csi_done_parameter = true;
            continue;
        } else if ((csi && es_is_CSI_parameter(in[i]) && csi_done_parameter) ||
                    (csi && !es_is_CSI_parameter(in[i])
                     && !es_is_CSI_intermediate(in[i])
                    )
        ) {
            /* Malformed escape sequence */
            esc = false;
            csi = false;
            csi_done_parameter = false;
            continue;
        }

        out[j++] = in[i];
    } while (j < out_size && in[++i]);
}
