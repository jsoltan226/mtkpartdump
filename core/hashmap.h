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
