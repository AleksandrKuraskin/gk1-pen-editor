#ifndef POLYPEN_MODEL_EDGE_H
#define POLYPEN_MODEL_EDGE_H

#include <stddef.h>

#include "model/point.h"

typedef enum {
  EDGE_TYPE_STRAIGHT,
  EDGE_TYPE_BEZIER,
  EDGE_TYPE_ARC
} EdgeType;

typedef struct {
  Point control1;
  Point control2;
} CubicBezier;

typedef enum {
  ARC_SIDE_LEFT,
  ARC_SIDE_RIGHT
} ArcSide;

typedef struct {
  ArcSide side;
} ArcGeometry;

typedef struct {
  Point *start;
  Point *end;
  EdgeType type;
  union {
    CubicBezier bezier;
    ArcGeometry arc;
  } geometry;
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
int edge_set_bezier(Edge *edge, Point control1, Point control2);

#endif
