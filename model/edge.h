#ifndef POLYPEN_MODEL_EDGE_H
#define POLYPEN_MODEL_EDGE_H

#include <stddef.h>

#include "model/point.h"

typedef struct {
  Point *start;
  Point *end;
} Edge;

typedef struct EdgeNode {
  Edge edge;
  struct EdgeNode *next;
} EdgeNode;

typedef struct {
  EdgeNode *head;
  size_t count;
} EdgeList;

EdgeList *edge_list_create(void);
void edge_list_destroy(EdgeList *list);
int edge_list_append(EdgeList *list, Point *start, Point *end);

#endif