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
