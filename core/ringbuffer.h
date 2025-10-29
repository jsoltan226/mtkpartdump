#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_
#include "static-tests.h"

#include "int.h"

/* An in-memory buffer used to store text.
 * Works the same as a normal linear buffer,
 * except that where the text would normally overrun the buffer's boundaries,
 * it instead "wraps" to the beginning overwriting the previous contents.
 *
 * Thanks to atomic operations, the writes can be safely performed
 * from multiple threads simultaneously. */
struct ringbuffer {
    char *buf; /* The buffer base pointer */
    u64 buf_size; /* The buffer size */

    /* The current "position" (where new text gets appended) */
    _Atomic u64 write_index;
};

/* Initializes a new ringbuffer of size `buf_size`.
 * Returns `NULL` on failure. */
struct ringbuffer *ringbuffer_init(u64 buf_size);

/* Deallocates and destroys all resources used by `*buf_p`,
 * and invalidates the handle by setting `*buf_p` to `NULL`. */
void ringbuffer_destroy(struct ringbuffer **buf_p);

/* Appends `string` to the buffer `buf`. */
void ringbuffer_write_string(struct ringbuffer *buf, const char *string);

#endif /* RINGBUFFER_H_ */
