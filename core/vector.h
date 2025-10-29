#ifndef VECTOR_H_
#define VECTOR_H_
#include "static-tests.h"

#include "int.h"
#include "log.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#define VECTOR_MINIMUM_CAPACITY__ 8U
#define VECTOR_METADATA_SIZE__ 16U
#define VECTOR_METADATA_N_ITEMS_OFFSET__ 0U

/* Used for a cleaner declaration of vector variables */
#define VECTOR(T) T *

/* Create a new vector of type `T` */
#define vector_new(T) ((T *)vector_init(sizeof(T)))
void * vector_init(u32 item_size);

/* Get the element at `index` from `v` */
#define vector_at(v, index) ((v) != NULL && (index) < vector_size((v))      \
    ? (v)[(index)]                                                          \
    : (s_abort("vector", "vector_at",                                       \
        "index out of bounds"), u_generic64_zero(*v))                       \
)

/* Append `item` to `v` */
#define vector_push_back(v_p, ...) do {                                     \
    vector_push_back_prepare__((void **)((void)**v_p, v_p));                \
    (*(v_p))[vector_size((*(v_p))) - 1] = __VA_ARGS__;                      \
} while (0)
void vector_push_back_prepare__(void **v_p);

/* Remove the last element from `*v_p` */
#define vector_pop_back(v_p) vector_pop_back__((void **)((void)**v_p, v_p))
void vector_pop_back__(void **v_p);

/* Insert `item` to `*v_p` at index `at` */
#define vector_insert(v_p, at, ...) do {                                    \
    vector_insert_prepare__((void **)((void)**v_p, v_p), (at));             \
    (*(v_p))[at] = __VA_ARGS__;                                             \
} while (0)
void * vector_insert_prepare__(void **v_p, u32 at);

/* Remove element from `v` at index `at` */
#define vector_erase(v_p, at) vector_erase__((void **)((void)**v_p, v_p), at)
void vector_erase__(void **v_p, u32 at);

/* Return the pointer to the first element of `v` */
#define vector_begin(v) (vector_size(v) > 0 ? v : NULL)

/* Return the first element of `v` */
#define vector_front(v) vector_at((v), 0)

/* Return the pointer to the element immidiately after the last one in `v` */
void * vector_end(void *v);

/* Return the last element */
#define vector_back(v) (vector_at((v), vector_size((v)) - 1U))

/* Check whether `v` is empty */
bool vector_empty(void *v);

/* Return the size of `v` (number of elements) */
#define vector_size(v) ((u32)(*((u32 *)(                                    \
    ((u8 *)(v)) - VECTOR_METADATA_SIZE__ + VECTOR_METADATA_N_ITEMS_OFFSET__)\
)))

/* Return the allocated capacity of `v` */
u32 vector_capacity(void *v);

/* Shrink `capacity` to `size` */
#define vector_shrink_to_fit(v_p) \
    vector_shrink_to_fit__((void **)((void)**v_p, v_p))
void vector_shrink_to_fit__(void **v_p);

/* Reset the size of `*v_p` and memset is to 0,
 * but leave the allocated capacity unchanged */
#define vector_clear(v_p) vector_clear__((void **)((void)**v_p, v_p))
void vector_clear__(void **v_p);

/* Increase the capacity of `v` to `count`
 * (if `count` is greater than the capacity of `v`) */
#define vector_reserve(v_p, count) \
    vector_reserve__((void **)((void)**v_p, v_p), count)
void vector_reserve__(void **v_p, u32 count);

/* Resize `v` to `new_size`,
 * cutting off any elements at index greater than `new_size` */
#define vector_resize(v_p, new_size) \
    vector_resize__((void **)((void)**v_p, v_p), new_size)
void vector_resize__(void **v_p, u32 new_size);

#define vector_copy vector_clone
void * vector_clone(void *v);

/* Destroy the vector that `v_p` points to and set `*v_p` to `NULL` */
#define vector_destroy(v_p) vector_destroy__((void **)((void)**v_p, v_p))
void vector_destroy__(void **v_p);

#endif /* VECTOR_H_ */
