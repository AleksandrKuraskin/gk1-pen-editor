#include "model/point.h"

#include <stdlib.h>

int point_attach_edge(Point *point, struct EdgeNode *edge) {
  if (point == NULL || edge == NULL) {
    return 0;
  }

  if (point->prev_edge == edge || point->next_edge == edge) {
    return 1;
  }

  if (point->prev_edge == NULL) {
    point->prev_edge = edge;
    return 1;
  }

  if (point->next_edge == NULL) {
    point->next_edge = edge;
    return 1;
  }

  return 0;
}

int point_detach_edge(Point *point, struct EdgeNode *edge) {
  if (point == NULL || edge == NULL) {
    return 0;
  }

  if (point->prev_edge == edge) {
    point->prev_edge = NULL;
    return 1;
  }

  if (point->next_edge == edge) {
    point->next_edge = NULL;
    return 1;
  }

  return 0;
}

Point *point_create(double x, double y) {
  Point *point = malloc(sizeof(*point));
  if (point == NULL) {
    return NULL;
  }

  point->x = x;
  point->y = y;
  point->prev_edge = NULL;
  point->next_edge = NULL;
  return point;
}

void point_destroy(Point *point) {
  if (point == NULL) {
    return;
  }

  free(point);
}

PointList *point_list_create(void) {
  PointList *list = malloc(sizeof(*list));
  if (list == NULL) {
    return NULL;
  }

  list->head = NULL;
  list->count = 0;
  return list;
}

void point_list_destroy(PointList *list) {
  if (list == NULL) {
    return;
  }

  PointNode *current = list->head;
  while (current != NULL) {
    PointNode *next = current->next;
    point_destroy(current->point);
    free(current);
    current = next;
  }

  free(list);
}

void point_list_append(PointList *list, Point *point) {
  if (list == NULL || point == NULL) {
    return;
  }

  PointNode *node = malloc(sizeof(*node));
  if (node == NULL) {
    return;
  }

  node->point = point;
  node->next = NULL;

  if (list->head == NULL) {
    list->head = node;
  } else {
    PointNode *current = list->head;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = node;
  }

  list->count += 1;
}
