#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

node_t* getNode(elem value) {
  node_t* newNode = malloc(sizeof(node_t));
  newNode->value = value;
  newNode->next = NULL;
  return newNode;
}

list_t* list_alloc() {
  list_t* l = malloc(sizeof(list_t));
  l->head = NULL;
  return l;
}

void list_free(list_t* l) {
  if (!l) return;
  node_t* curr = l->head;
  while (curr) {
    node_t* tmp = curr;
    curr = curr->next;
    free(tmp);
  }
  free(l);
}

void list_print(list_t* l) {
  if (!l) return;
  node_t* curr = l->head;
  while (curr) {
    printf("%d ", curr->value);
    curr = curr->next;
  }
  printf("\n");
}

char* listToString(list_t *l) {
  if (!l) return NULL;

  char *buffer = malloc(1024);
  buffer[0] = '\0';

  node_t* curr = l->head;
  char temp[50];

  while (curr) {
    sprintf(temp, "%d->", curr->value);
    strcat(buffer, temp);
    curr = curr->next;
  }

  strcat(buffer, "NULL");
  return buffer;
}

int list_length(list_t* l) {
  int count = 0;
  node_t* curr = l->head;
  while (curr) {
    count++;
    curr = curr->next;
  }
  return count;
}

void list_add_to_back(list_t* l, elem value) {
  node_t* n = getNode(value);
  if (!l->head) {
    l->head = n;
    return;
  }

  node_t* curr = l->head;
  while (curr->next) {
    curr = curr->next;
  }
  curr->next = n;
}

void list_add_to_front(list_t* l, elem value) {
  node_t* n = getNode(value);
  n->next = l->head;
  l->head = n;
}

void list_add_at_index(list_t* l, elem value, int index) {
  if (!l) return;
  if (index <= 0 || !l->head) {
    list_add_to_front(l, value);
    return;
  }

  node_t* curr = l->head;
  int i = 0;
  while (curr->next && i < index - 1) {
    curr = curr->next;
    i++;
  }

  node_t* n = getNode(value);
  n->next = curr->next;
  curr->next = n;
}

elem list_remove_from_back(list_t* l) {
  if (!l || !l->head) return -1;

  if (!l->head->next) {
    elem val = l->head->value;
    free(l->head);
    l->head = NULL;
    return val;
  }

  node_t* curr = l->head;
  while (curr->next->next) {
    curr = curr->next;
  }

  elem val = curr->next->value;
  free(curr->next);
  curr->next = NULL;
  return val;
}

elem list_remove_from_front(list_t* l) {
  if (!l || !l->head) return -1;
  node_t* tmp = l->head;
  elem val = tmp->value;
  l->head = tmp->next;
  free(tmp);
  return val;
}

elem list_remove_at_index(list_t* l, int index) {
  if (!l || !l->head) return -1;
  if (index <= 0) return list_remove_from_front(l);

  node_t* curr = l->head;
  int i = 0;

  while (curr->next && i < index - 1) {
    curr = curr->next;
    i++;
  }

  if (!curr->next) return -1;

  node_t* tmp = curr->next;
  elem val = tmp->value;
  curr->next = tmp->next;
  free(tmp);
  return val;
}

bool list_is_in(list_t* l, elem value) {
  node_t* curr = l->head;
  while (curr) {
    if (curr->value == value) return true;
    curr = curr->next;
  }
  return false;
}

elem list_get_elem_at(list_t* l, int index) {
  if (!l || index < 0) return -1;
  node_t* curr = l->head;
  int i = 0;
  while (curr) {
    if (i == index) return curr->value;
    curr = curr->next;
    i++;
  }
  return -1;
}

int list_get_index_of(list_t* l, elem value) {
  node_t* curr = l->head;
  int index = 0;
  while (curr) {
    if (curr->value == value) return index;
    curr = curr->next;
    index++;
  }
  return -1;
}
