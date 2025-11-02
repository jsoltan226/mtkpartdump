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
#include "linked-list.h"
#include "log.h"
#include "util.h"
#include <stdlib.h>

#define MODULE_NAME "linked-list"

struct linked_list * linked_list_create(void *head_content)
{
    struct linked_list *ll = malloc(sizeof(struct linked_list));
    s_assert(ll != NULL, "malloc() failed for struct linked_list");

    struct ll_node *first_node = linked_list_create_node(head_content);
    if (first_node == NULL) {
        s_log_error("linked_list_create_node() returned NULL!");
        u_nzfree(&ll);
        return NULL;
    }

    ll->head = first_node;
    ll->tail = first_node;
    return ll;
}

struct ll_node * linked_list_append(struct ll_node *at, void *content)
{
    struct ll_node *new_node = calloc(1, sizeof(struct ll_node));
    s_assert(new_node != NULL, "calloc() for new_node failed!");

    if (at != NULL) {
        new_node->next = at->next;
        new_node->prev = at;
        if (at->next != NULL) at->next->prev= new_node;
        at->next = new_node;
    }

    new_node->content = content;
    return new_node;
}

struct ll_node * linked_list_prepend(struct ll_node *at, void *content)
{
    struct ll_node *new_node = calloc(1, sizeof(struct ll_node));
    s_assert(new_node != NULL, "calloc() for new_node failed!");

    if (at != NULL) {
        new_node->prev = at->prev;
        new_node->next = at;
        if (at->prev != NULL) at->prev->next = new_node;
        at->prev = new_node;
    }

    new_node->content = content;
    return new_node;
}

void linked_list_destroy_node(struct ll_node **node_p)
{
    if (node_p == NULL || *node_p == NULL) return;

    struct ll_node *node = *node_p;

    if (node->prev != NULL) node->prev->next = node->next;
    if (node->next != NULL) node->next->prev = node->prev;
    u_nzfree(node_p);
}

void linked_list_destroy(struct linked_list **list_p, bool free_content)
{
    if (list_p == NULL || *list_p == NULL) return;

    struct linked_list *list = *list_p;

    linked_list_recursive_destroy_nodes(&list->head, free_content);
    u_nzfree(list_p);
}

void linked_list_recursive_destroy_nodes(struct ll_node **head_p, bool free_content)
{
    if (head_p == NULL || *head_p == NULL) return;

    struct ll_node *curr_node = *head_p;
    while (curr_node != NULL) {
        struct ll_node *next_node = curr_node->next;
        if (free_content) u_nfree(&curr_node->content);
        u_nzfree(&curr_node);
        curr_node = next_node;
    };
}
