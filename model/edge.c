#include "model/edge.h"

#include <stdlib.h>

EdgeList *edge_list_create(void) {
  EdgeList *list = malloc(sizeof(*list));
  if (list == NULL) {
    return NULL;
  }

  list->head = NULL;
  list->count = 0;
  return list;
}

void edge_list_destroy(EdgeList *list) {
  if (list == NULL) {
    return;
  }

  EdgeNode *current = list->head;
  while (current != NULL) {
    EdgeNode *next = current->next;
    free(current);
    current = next;
  }

  free(list);
}

int edge_list_append(EdgeList *list, Point *start, Point *end) {
  if (list == NULL || start == NULL || end == NULL) {
    return 0;
  }

  EdgeNode *node = malloc(sizeof(*node));
  if (node == NULL) {
    return 0;
  }

  node->edge.start = start;
  node->edge.end = end;
  node->next = NULL;

  if (list->head == NULL) {
    list->head = node;
  } else {
    EdgeNode *current = list->head;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = node;
  }

  list->count += 1;
  return 1;
}