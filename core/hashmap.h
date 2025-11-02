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
#ifndef U_HASHMAP_H_
#define U_HASHMAP_H_
#include "static-tests.h"

#include "int.h"
#include "linked-list.h"

#define HM_TABLE_SIZE  10

#define HM_RESIZING_FACTOR 5
#define HM_MAX_KEY_LENGTH 256

struct hashmap_record {
    char key[HM_MAX_KEY_LENGTH];
    void *value;
};

/* Chained hash map
 * Collisions are resolved by chaining them together on a linked list
 */
struct hashmap {
    u32 length;
    u32 n_elements;
    struct linked_list **bucket_lists;
};

/* I'm too lazy to write explanations, if you know how a hash map works,
 * you know how to use these functions */

struct hashmap * hashmap_create(u32 intial_size);

i32 hashmap_insert(struct hashmap *map, const char *key, const void *entry);

void * hashmap_lookup_record(struct hashmap *map, const char *key);

void hashmap_delete_record(struct hashmap *map, const char *key);

void hashmap_destroy(struct hashmap **map);

#endif /* U_HASHMAP_H_ */
