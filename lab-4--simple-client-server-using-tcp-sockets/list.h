#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct node {
    int data;
    struct node *next;
} node_t;

typedef struct list {
    node_t *head;
} list_t;

// Allocate a new empty list
list_t* list_alloc(void);

// Free the list and its nodes
void list_free(list_t *list);

// Get the length of the list
int list_length(list_t *list);

// Add value to the front
void list_add_to_front(list_t *list, int value);

// Add value to the back
void list_add_to_back(list_t *list, int value);

// Add value at a specific index
void list_add_at_index(list_t *list, int index, int value);

// Remove value from front
int list_remove_from_front(list_t *list);

// Remove value from back
int list_remove_from_back(list_t *list);

// Remove value at a specific index
int list_remove_at_index(list_t *list, int index);

// Get element at a specific index
int list_get_elem_at(list_t *list, int index);

// Convert list to string
char* listToString(list_t *list);

#endif
