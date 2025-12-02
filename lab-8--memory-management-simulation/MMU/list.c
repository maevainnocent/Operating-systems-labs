// list/list.c
// Polished linked-list implementation for MMU assignment
// Updated: stable descending tie-handling and robust list ops.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

list_t *list_alloc() {
    list_t *list = (list_t*) malloc(sizeof(list_t));
    if (list == NULL) {
        fprintf(stderr, "Fatal: malloc failed in list_alloc\n");
        exit(1);
    }
    list->head = NULL;
    return list;
}

node_t *node_alloc(block_t *blk) {
    node_t *node = (node_t*) malloc(sizeof(node_t));
    if (node == NULL) {
        fprintf(stderr, "Fatal: malloc failed in node_alloc\n");
        exit(1);
    }
    node->blk = blk;
    node->next = NULL;
    return node;
}

/* Frees the list struct and all remaining nodes & blocks it owns.
 * Use carefully: only call when blocks are not used elsewhere. */
void list_free(list_t *l) {
    if (l == NULL) return;
    node_t *cur = l->head;
    while (cur != NULL) {
        node_t *next = cur->next;
        if (cur->blk) free(cur->blk);
        free(cur);
        cur = next;
    }
    free(l);
}

void node_free(node_t *node) {
    if (node) free(node);
}

void list_print(list_t *l) {
    node_t *current = (l == NULL) ? NULL : l->head;
    block_t *b;
    if (current == NULL) {
        printf("list is empty\n");
        return;
    }
    while (current != NULL) {
        b = current->blk;
        printf("PID=%d START:%d END:%d\n", b->pid, b->start, b->end);
        current = current->next;
    }
}

int list_length(list_t *l) {
    node_t *current = (l == NULL) ? NULL : l->head;
    int i = 0;
    while (current != NULL) {
        i++;
        current = current->next;
    }
    return i;
}

void list_add_to_back(list_t *l, block_t *blk) {
    node_t *newNode = node_alloc(blk);
    if (l->head == NULL) {
        l->head = newNode;
        return;
    }
    node_t *cur = l->head;
    while (cur->next != NULL) cur = cur->next;
    cur->next = newNode;
}

void list_add_to_front(list_t *l, block_t *blk) {
    node_t *newNode = node_alloc(blk);
    newNode->next = l->head;
    l->head = newNode;
}

void list_add_at_index(list_t *l, block_t *blk, int index) {
    if (l == NULL) return;
    if (index <= 0 || l->head == NULL) {
        list_add_to_front(l, blk);
        return;
    }

    node_t *newNode = node_alloc(blk);
    node_t *cur = l->head;
    int i = 0;

    while (cur->next != NULL && i < index - 1) {
        cur = cur->next;
        i++;
    }
    newNode->next = cur->next;
    cur->next = newNode;
}

/* Insert in ascending order by start address */
void list_add_ascending_by_address(list_t *l, block_t *newblk) {
    if (l == NULL) return;
    node_t *newNode = node_alloc(newblk);
    if (l->head == NULL) {
        l->head = newNode;
        return;
    }

    node_t *cur = l->head;
    node_t *prev = NULL;

    while (cur != NULL && cur->blk->start < newblk->start) {
        prev = cur;
        cur = cur->next;
    }

    if (prev == NULL) {
        newNode->next = l->head;
        l->head = newNode;
    } else {
        prev->next = newNode;
        newNode->next = cur;
    }
}

/* Insert in ascending order by blocksize (small -> large).
 * blocksize = end - start + 1 */
void list_add_ascending_by_blocksize(list_t *l, block_t *newblk) {
    if (l == NULL) return;
    int new_size = (newblk->end - newblk->start) + 1;
    node_t *newNode = node_alloc(newblk);

    if (l->head == NULL) {
        l->head = newNode;
        return;
    }

    node_t *cur = l->head;
    node_t *prev = NULL;

    while (cur != NULL) {
        int cur_size = (cur->blk->end - cur->blk->start) + 1;
        if (new_size < cur_size) break;
        prev = cur;
        cur = cur->next;
    }

    if (prev == NULL) {
        newNode->next = l->head;
        l->head = newNode;
    } else {
        prev->next = newNode;
        newNode->next = cur;
    }
}

/* Insert in descending order by blocksize (large -> small).
 * blocksize = end - start + 1
 * Tie-handling: place new block **after** existing blocks of same size (stable) */
void list_add_descending_by_blocksize(list_t *l, block_t *blk) {
    if (l == NULL) return;
    int new_size = (blk->end - blk->start) + 1;
    node_t *newNode = node_alloc(blk);

    if (l->head == NULL) {
        l->head = newNode;
        return;
    }

    node_t *cur = l->head;
    node_t *prev = NULL;

    while (cur != NULL) {
        int cur_size = (cur->blk->end - cur->blk->start) + 1;
        /* break if new_size >= cur_size so ties go AFTER existing nodes of equal size */
        if (new_size >= cur_size) break;
        prev = cur;
        cur = cur->next;
    }

    if (prev == NULL) {
        newNode->next = l->head;
        l->head = newNode;
    } else {
        prev->next = newNode;
        newNode->next = cur;
    }
}

/* Merge physically adjacent nodes in ascending-by-address order */
void list_coalese_nodes(list_t *l) {
    if (l == NULL || l->head == NULL) return;

    node_t *prev = l->head;
    node_t *cur = prev->next;

    while (cur != NULL) {
        if (prev->blk->end + 1 == cur->blk->start) {
            prev->blk->end = cur->blk->end;
            node_t *to_free = cur;
            cur = cur->next;
            prev->next = cur;
            if (to_free->blk) free(to_free->blk);
            node_free(to_free);
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
}

/* remove last node, return its block pointer (caller frees block when appropriate) */
block_t* list_remove_from_back(list_t *l) {
    if (l == NULL || l->head == NULL) return NULL;

    node_t *cur = l->head;
    node_t *prev = NULL;

    /* only one node */
    if (cur->next == NULL) {
        block_t *blk = cur->blk;
        free(cur);
        l->head = NULL;
        return blk;
    }

    /* move to penultimate node */
    while (cur->next != NULL) {
        prev = cur;
        cur = cur->next;
    }

    /* cur is last, prev is penultimate */
    block_t *blk = cur->blk;
    free(cur);
    prev->next = NULL;
    return blk;
}

block_t* list_get_from_front(list_t *l) {
    if (l == NULL || l->head == NULL) return NULL;
    return l->head->blk;
}

block_t* list_remove_from_front(list_t *l) {
    if (l == NULL || l->head == NULL) return NULL;
    node_t *cur = l->head;
    block_t *blk = cur->blk;
    l->head = cur->next;
    free(cur);
    return blk;
}

block_t* list_remove_at_index(list_t *l, int index) {
    if (l == NULL || l->head == NULL) return NULL;
    if (index <= 0) return list_remove_from_front(l);

    node_t *cur = l->head;
    node_t *prev = NULL;
    int i = 0;

    while (cur != NULL && i < index) {
        prev = cur;
        cur = cur->next;
        i++;
    }

    if (cur == NULL) return NULL;

    prev->next = cur->next;
    block_t *blk = cur->blk;
    free(cur);
    return blk;
}

bool compareBlks(block_t* a, block_t *b) {
    if (a == NULL || b == NULL) return false;
    return (a->pid == b->pid && a->start == b->start && a->end == b->end);
}

bool compareSize(int a, block_t *b) {
    if (b == NULL) return false;
    return (a <= (b->end - b->start) + 1);
}

bool comparePid(int a, block_t *b) {
    if (b == NULL) return false;
    return (a == b->pid);
}

bool list_is_in(list_t *l, block_t* value) {
    if (l == NULL) return false;
    node_t *cur = l->head;
    while (cur != NULL) {
        if (compareBlks(value, cur->blk)) return true;
        cur = cur->next;
    }
    return false;
}

block_t* list_get_elem_at(list_t *l, int index) {
    if (l == NULL || l->head == NULL) return NULL;
    node_t *cur = l->head;
    int i = 0;
    while (cur != NULL) {
        if (i == index) return cur->blk;
        cur = cur->next;
        i++;
    }
    return NULL;
}

int list_get_index_of(list_t *l, block_t* value) {
    if (l == NULL || l->head == NULL) return -1;
    node_t *cur = l->head;
    int i = 0;
    while (cur != NULL) {
        if (compareBlks(value, cur->blk)) return i;
        cur = cur->next;
        i++;
    }
    return -1;
}

bool list_is_in_by_size(list_t *l, int Size) {
    if (l == NULL) return false;
    node_t *cur = l->head;
    while (cur != NULL) {
        if (compareSize(Size, cur->blk)) return true;
        cur = cur->next;
    }
    return false;
}

bool list_is_in_by_pid(list_t *l, int pid) {
    if (l == NULL) return false;
    node_t *cur = l->head;
    while (cur != NULL) {
        if (comparePid(pid, cur->blk)) return true;
        cur = cur->next;
    }
    return false;
}

int list_get_index_of_by_Size(list_t *l, int Size) {
    if (l == NULL || l->head == NULL) return -1;
    node_t *cur = l->head;
    int i = 0;
    while (cur != NULL) {
        if (compareSize(Size, cur->blk)) return i;
        cur = cur->next;
        i++;
    }
    return -1;
}

int list_get_index_of_by_Pid(list_t *l, int pid) {
    if (l == NULL || l->head == NULL) return -1;
    node_t *cur = l->head;
    int i = 0;
    while (cur != NULL) {
        if (comparePid(pid, cur->blk)) return i;
        cur = cur->next;
        i++;
    }
    return -1;
}

/* Return element in front or NULL if empty */
block_t* list_get_from_front(list_t *l) {
    if (l == NULL || l->head == NULL) return NULL;
    return l->head->blk;
}
