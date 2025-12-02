#include "list.h"

// Allocate a new empty list
list_t* list_alloc(void) {
    list_t *list = (list_t*) malloc(sizeof(list_t));
    list->head = NULL;
    return list;
}

// Free the list and all nodes
void list_free(list_t *list) {
    node_t *current = list->head;
    while (current != NULL) {
        node_t *temp = current;
        current = current->next;
        free(temp);
    }
    free(list);
}

// Get the length of the list
int list_length(list_t *list) {
    int len = 0;
    node_t *current = list->head;
    while (current != NULL) {
        len++;
        current = current->next;
    }
    return len;
}

// Add value to the front
void list_add_to_front(list_t *list, int value) {
    node_t *new_node = (node_t*) malloc(sizeof(node_t));
    new_node->data = value;
    new_node->next = list->head;
    list->head = new_node;
}

// Add value to the back
void list_add_to_back(list_t *list, int value) {
    node_t *new_node = (node_t*) malloc(sizeof(node_t));
    new_node->data = value;
    new_node->next = NULL;
    if (list->head == NULL) {
        list->head = new_node;
        return;
    }
    node_t *current = list->head;
    while (current->next != NULL)
        current = current->next;
    current->next = new_node;
}

// Add value at a specific index
void list_add_at_index(list_t *list, int index, int value) {
    if (index <= 0) {
        list_add_to_front(list, value);
        return;
    }
    int i = 0;
    node_t *current = list->head;
    while (current != NULL && i < index - 1) {
        current = current->next;
        i++;
    }
    if (current == NULL) {
        list_add_to_back(list, value);
        return;
    }
    node_t *new_node = (node_t*) malloc(sizeof(node_t));
    new_node->data = value;
    new_node->next = current->next;
    current->next = new_node;
}

// Remove value from front
int list_remove_from_front(list_t *list) {
    if (list->head == NULL) return -1;
    node_t *temp = list->head;
    int value = temp->data;
    list->head = temp->next;
    free(temp);
    return value;
}

// Remove value from back
int list_remove_from_back(list_t *list) {
    if (list->head == NULL) return -1;
    node_t *current = list->head;
    if (current->next == NULL) {
        int value = current->data;
        free(current);
        list->head = NULL;
        return value;
    }
    while (current->next->next != NULL)
        current = current->next;
    int value = current->next->data;
    free(current->next);
    current->next = NULL;
    return value;
}

// Remove value at a specific index
int list_remove_at_index(list_t *list, int index) {
    if (list->head == NULL) return -1;
    if (index <= 0) return list_remove_from_front(list);
    int i = 0;
    node_t *current = list->head;
    while (current->next != NULL && i < index - 1) {
        current = current->next;
        i++;
    }
    if (current->next == NULL) return -1;
    node_t *temp = current->next;
    int value = temp->data;
    current->next = temp->next;
    free(temp);
    return value;
}

// Get element at a specific index
int list_get_elem_at(list_t *list, int index) {
    int i = 0;
    node_t *current = list->head;
    while (current != NULL && i < index) {
        current = current->next;
        i++;
    }
    if (current == NULL) return -1;
    return current->data;
}

// Convert list to string
char* listToString(list_t *list) {
    static char str[1024];
    str[0] = '\0';
    node_t *current = list->head;
    while (current != NULL) {
        char buf[32];
        sprintf(buf, "%d -> ", current->data);
        strcat(str, buf);
        current = current->next;
    }
    strcat(str, "NULL");
    return str;
}
