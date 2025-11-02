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
#include "vector.h"
#include "int.h"
#include "log.h"
#include "math.h"
#include "util.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct vector_metadata__ {
    u32 n_items;
    u32 item_size;
    u32 capacity;
    u32 dummy_;
} vector_meta_t;

static_assert(sizeof(struct vector_metadata__) == VECTOR_METADATA_SIZE__,
    "The size of struct vector_metadata must be equal to "
    "VECTOR_METADATA_SIZE__ (16 bytes)");

#define MODULE_NAME "vector"

#define get_metadata_ptr(v) \
    ((vector_meta_t *)(((u8 *)v) - sizeof(vector_meta_t)))

#define element_at(v, at) (((u8 *)v) + (at * get_metadata_ptr(v)->item_size))

static void * vector_realloc(void *v, u32 new_capacity);
static void vector_increase_size(void **v_p);
static void vector_memmove(void *v, u32 src_index, u32 dst_index, u32 nmemb);

void * vector_init(u32 item_size)
{
    const u32 total_size = sizeof(vector_meta_t) +
        (item_size * VECTOR_MINIMUM_CAPACITY__);
    void *v = malloc(total_size);
    s_assert(v != NULL, "malloc() failed for vector");
    memset(v, 0, total_size);

    vector_meta_t *metadata_ptr = (vector_meta_t *)v;
    metadata_ptr->item_size = item_size;
    metadata_ptr->n_items = 0;
    metadata_ptr->capacity = VECTOR_MINIMUM_CAPACITY__;

    u8 *const vector_base = ((u8 *)v) + sizeof(vector_meta_t);
    return vector_base;
}

void vector_push_back_prepare__(void **v_p)
{
    u_check_params(v_p != NULL && *v_p != NULL);

    /* Allocate memory for the new item */
    vector_increase_size(v_p);
}

void vector_pop_back__(void **v_p)
{
    u_check_params(v_p != NULL && *v_p != NULL);

    vector_meta_t *meta = get_metadata_ptr(*v_p);

    meta->n_items--;
    memset(element_at(*v_p, meta->n_items), 0, meta->item_size);

    if (meta->n_items <= (meta->capacity / 2) &&
        meta->n_items >= VECTOR_MINIMUM_CAPACITY__) {
        *v_p = vector_realloc(*v_p, meta->capacity / 2);
    }
}

void * vector_insert_prepare__(void **v_p, u32 at)
{
    u_check_params(v_p != NULL && *v_p != NULL && at < vector_size(*v_p));

    /* Expand the vector by one */
    vector_increase_size(v_p);

    /* Move everything after and including `at` one spot to the right */
    vector_memmove(*v_p, at, at + 1, vector_size(*v_p) - at);

    return *v_p;
}

bool vector_empty(void *v)
{
    return v == NULL ? true : get_metadata_ptr(v)->n_items == 0;
}

u32 vector_capacity(void *v)
{
    return v == NULL ? 0 : get_metadata_ptr(v)->capacity;
}

void * vector_end(void *v)
{
    if (v == NULL) return NULL;
    vector_meta_t *meta = get_metadata_ptr(v);
    return ((u8 *)v) + (meta->n_items * meta->item_size);
}

void vector_shrink_to_fit__(void **v_p)
{
    u_check_params(v_p != NULL && *v_p != NULL);

    vector_meta_t *meta = get_metadata_ptr(*v_p);
    *v_p = vector_realloc(*v_p, meta->n_items);
}

void vector_clear__(void **v_p)
{
    u_check_params(v_p != NULL && *v_p != NULL);

    vector_meta_t *meta = get_metadata_ptr(*v_p);

    memset(*v_p, 0, meta->n_items * meta->item_size);
    meta->n_items = 0;
}

void vector_erase__(void **v_p, u32 index)
{
    u_check_params(v_p != NULL && *v_p != NULL);

    vector_meta_t *meta = get_metadata_ptr(*v_p);

    s_assert(index < meta->n_items,
        "Attempt to erase element outside of array bounds "
        "(index: %u, n_items: %u", index, meta->n_items);

    /* Move all memory after `index` one spot to the left,
     * and then deallocate the last (now unused) spot */
    vector_memmove(*v_p, index + 1, index, meta->n_items - index - 1);
    vector_pop_back__(v_p);
}

void vector_reserve__(void **v_p, u32 count)
{
    u_check_params(v_p != NULL && *v_p != NULL);

    vector_meta_t *meta = get_metadata_ptr(*v_p);
    if (meta->capacity < count)
        *v_p = vector_realloc(*v_p, count);
}

void vector_resize__(void **v_p, u32 new_size)
{
    u_check_params(v_p != NULL && *v_p != NULL);

    vector_meta_t *meta = get_metadata_ptr(*v_p);

    /* Clean up the items that are to be cut off */
    if (new_size < meta->capacity) {
        memset(element_at(*v_p, new_size), 0,
            (meta->capacity - new_size) * meta->item_size
        );
    }

    *v_p = vector_realloc(*v_p, new_size);
    meta = get_metadata_ptr(*v_p);
    meta->n_items = u_min(new_size, meta->n_items);
}

void * vector_clone(void *v)
{
    u_check_params(v != NULL);

    vector_meta_t *meta_p = get_metadata_ptr(v);

    void *new_v = vector_init(meta_p->item_size);

    new_v = vector_realloc(new_v, meta_p->capacity);

    memcpy(get_metadata_ptr(new_v), meta_p,
        (meta_p->capacity * meta_p->item_size) + sizeof(vector_meta_t)
    );

    return new_v;
}

void vector_destroy__(void **v_p)
{
    if (v_p == NULL || *v_p == NULL) return;

    vector_meta_t *meta_ptr = get_metadata_ptr(*v_p);

    /* Reset the entire vector, including the metadata */
    memset(meta_ptr, 0, sizeof(vector_meta_t) + meta_ptr->capacity);
    free(meta_ptr);

    *v_p = NULL;
}

static void * vector_realloc(void *v, u32 new_cap)
{
    /* Unfortunately if `v` is NULL we do not know the element size,
     * and so we cannot make it work like realloc(NULL, size) would */
    u_check_params(v != NULL);

    void *new_v = NULL;

    /* Never shrink the vector beyond the minimal capacity */
    if (new_cap < VECTOR_MINIMUM_CAPACITY__)
        new_cap = VECTOR_MINIMUM_CAPACITY__;

    vector_meta_t *meta_p = get_metadata_ptr(v);
    new_v = realloc(meta_p,
        (new_cap * meta_p->item_size) + sizeof(vector_meta_t));


    s_assert(new_v != NULL, "realloc() failed!");
    meta_p = new_v;
    meta_p->capacity = new_cap;

    return ((u8 *)new_v) + sizeof(vector_meta_t);
}

static void vector_increase_size(void **v_p)
{
    u_check_params(v_p != NULL && *v_p != NULL);

    vector_meta_t *meta = get_metadata_ptr(*v_p);

    if (meta->n_items >= meta->capacity) {
        u32 new_cap = meta->capacity;
        if (new_cap > 0)
            new_cap *= 2;
        else
            new_cap++;

        *v_p = vector_realloc(*v_p, new_cap);

        /* `meta` might have been moved by `realloc()` */
        meta = get_metadata_ptr(*v_p);
        meta->capacity = new_cap;
    }

    meta->n_items++;
}

static void vector_memmove(void *v, u32 src_index, u32 dst_index, u32 nmemb)
{
    u_check_params(v != NULL);

    if (src_index == dst_index || nmemb == 0)
        return;

    vector_meta_t *meta = get_metadata_ptr(v);

    s_assert(src_index + nmemb <= meta->n_items,
        "Attempt to read from memory beyond the vector bounds");
    s_assert(dst_index + nmemb <= meta->capacity,
        "Attempt to write to memory beyond the vector bounds");

    memmove(
        element_at(v, dst_index),
        element_at(v, src_index),
        nmemb * meta->item_size
    );
}
