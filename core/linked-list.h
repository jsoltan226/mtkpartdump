#ifndef U_LINKED_LIST_H
#define U_LINKED_LIST_H
#include "static-tests.h"

#include <stdlib.h>
#include <stdbool.h>

struct ll_node {
    struct ll_node *next, *prev;
    void *content;
};

struct linked_list {
    struct ll_node *head, *tail;
};

struct linked_list * linked_list_create(void *head_content);

/* Creates a node after `at` with content `content`. Returns a pointer to the new node. */
struct ll_node * linked_list_append(struct ll_node *at, void *content);

/* Same thing as `linked_list_append`, but the node is created BEFORE `at`. */
struct ll_node * linked_list_prepend(struct ll_node *at, void *content);

#define linked_list_create_node(content) linked_list_append(NULL, content)

void linked_list_destroy(struct linked_list **list_p, bool free_content);

void linked_list_destroy_node(struct ll_node **node_p);

/* Iterates over all nodes starting from `head` until node->next is NULL,
 * destroying every single one of them */
void linked_list_recursive_destroy_nodes(struct ll_node **head_p, bool free_content);

#endif /* U_LINKED_LIST_H */
