#include "ringbuffer.h"
#include "int.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#define MODULE_NAME "ringbuffer"

struct ringbuffer * ringbuffer_init(u64 buf_size)
{
    struct ringbuffer *ret = malloc(sizeof(struct ringbuffer));
    s_assert(ret != NULL, "malloc failed for new ringbuffer");

    ret->buf = calloc(buf_size, 1);
    if (ret->buf == NULL) {
        s_log_error("Failed to allocate a %lu-byte ringbuffer", buf_size);
        free (ret);
        return NULL;
    }

    ret->buf_size = buf_size;
    atomic_store(&ret->write_index, 0);

    return ret;
}

void ringbuffer_destroy(struct ringbuffer **buf_p)
{
    if (buf_p == NULL || *buf_p == NULL) return;
    struct ringbuffer *const buf = *buf_p;

    if (buf->buf != NULL) {
        free(buf->buf);
        buf->buf = NULL;
    }
    buf->buf_size = 0;
    atomic_store(&buf->write_index, 0);

    free(*buf_p);
    *buf_p = NULL;
}

void ringbuffer_write_string(struct ringbuffer *buf, const char *string)
{
    if (buf == NULL || string == NULL ||
        buf->buf == NULL || buf->buf_size <= 1)
        return;

    /* Keep space for a NULL terminator at the end of the membuf
     * if someone decides to print it out like a normal string */
    const u64 usable_buf_size = buf->buf_size - 1;
    buf->buf[buf->buf_size - 1] = '\0';

    u64 chars_to_write = strlen(string) + 1;
    if (chars_to_write == 1)
        return; /* We would just be overwriting the '\0' @ write_index
                   with another NULL terminator and not moving forward */

    /* If the message is so long that it would loop over itself,
     * we might as well skip the chars that will be overwritten anyway */
    if (chars_to_write > usable_buf_size) {
        const u64 d = chars_to_write - usable_buf_size;
        string += d;
        chars_to_write = usable_buf_size;

        /* Move the write index accordingly */
        u64 write_index_value = atomic_load(&buf->write_index);
        write_index_value += d % usable_buf_size;
        atomic_store(&buf->write_index, write_index_value);
    }


    /* If the message is too long, write the part that would fit
     * and set the write index back to the beginning of the buffer */
    u64 write_index_value = atomic_load(&buf->write_index);
    if (chars_to_write + write_index_value > usable_buf_size) {
        atomic_store(&buf->write_index, 0);
        memcpy(buf->buf + write_index_value, string,
            usable_buf_size - write_index_value);
        chars_to_write -= usable_buf_size - write_index_value;
        string += usable_buf_size - write_index_value;

        write_index_value = atomic_load(&buf->write_index);
    }

    /* Place the write index *ON* the NULL terminator, not after it */
    if (chars_to_write != 0) {
        atomic_store(&buf->write_index, write_index_value + chars_to_write - 1);
    } else {
        /* Go back to the end */
        atomic_store(&buf->write_index, usable_buf_size);
    }
    memcpy(buf->buf + write_index_value, string, chars_to_write);
}
