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
#include "hashmap.h"
#include "int.h"
#include "log.h"
#include "util.h"
#include "linked-list.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MODULE_NAME "hashmap"

static inline u32 hash(const char *key, u32 max);
static struct ll_node * lookup_bucket_list_node(struct hashmap *map, const char *key);

struct hashmap * hashmap_create(u32 initial_length)
{
    struct hashmap *map = malloc(sizeof(struct hashmap));
    s_assert(map != NULL, "malloc() failed for map");

    map->length = initial_length;
    map->n_elements = 0;

    map->bucket_lists = calloc(initial_length, sizeof(struct linked_list *));
    if (map->bucket_lists == NULL) {
        s_log_error("calloc() for map->bucket_lists failed!");
        u_nzfree(&map);
        return NULL;
    }

    return map;
}

i32 hashmap_insert(struct hashmap *map, const char *key, const void *entry)
{
    if (map == NULL) return 1;

    u32 index = hash(key, map->length);

    struct hashmap_record *new_record = malloc(sizeof(struct hashmap_record));
    s_assert(new_record != NULL, "malloc() for new record failed!");

    new_record->value = (void *)entry;
    strncpy(new_record->key, key, HM_MAX_KEY_LENGTH);
    new_record->key[HM_MAX_KEY_LENGTH - 1] = '\0';

    if (map->bucket_lists[index] == NULL) {
        map->bucket_lists[index] = linked_list_create(new_record);
        if (map->bucket_lists[index] == NULL) {
            s_log_error(
                "hashmap_insert: linked_list_create() for bucket list @ index %i returned NULL!",
                index
            );
            u_nzfree(&new_record);
            return 1;
        }
    } else {
        map->bucket_lists[index]->head = linked_list_append(
            map->bucket_lists[index]->head,
            new_record
        );
    }

    map->n_elements++;

    return 0;
}

void * hashmap_lookup_record(struct hashmap *map, const char *key)
{
    struct ll_node *node = lookup_bucket_list_node(map, key);

    if (node == NULL || node->content == NULL)
        return NULL;
    else
        return ((struct hashmap_record *)(node->content))->value;
}

void hashmap_delete_record(struct hashmap *map, const char *key)
{
    struct ll_node *node = lookup_bucket_list_node(map, key);
    if (node == NULL)
        return;

    u_nfree(&node->content);
    linked_list_destroy_node(&node);
}

void hashmap_destroy(struct hashmap **map_p)
{
    if (map_p == NULL || *map_p == NULL) return;
    struct hashmap *map = *map_p;

    for (u32 i = 0; i < map->length; i++) {
        if (map->bucket_lists[i] != NULL) {
            linked_list_destroy(&map->bucket_lists[i], true);
        }
    }
    free(map->bucket_lists);

    u_nzfree(map_p);
}

static inline u32 hash(const char *key, u32 max)
{
    return ((key[0] % max) * key[strlen(key) - 1]) % max;
}

static struct ll_node * lookup_bucket_list_node(struct hashmap *map, const char *key)
{
    u32 index = hash(key, map->length);

    if (map->bucket_lists[index] == NULL) return NULL;

    struct ll_node *curr_node = (map->bucket_lists[index])->tail;
    while (curr_node != NULL) {
        if (curr_node->content != NULL) {
            /* Return the current node if the keys match */
            if (!strncmp(
                ((struct hashmap_record *)curr_node->content)->key,
                key,
                HM_MAX_KEY_LENGTH
            )) return curr_node;
        }
        curr_node = curr_node->next;
    }

    return NULL;
}
