#ifndef POLYPEN_MODEL_POINT_H
#define POLYPEN_MODEL_POINT_H

#include <stddef.h>

struct EdgeNode;

typedef struct {
  double x;
  double y;
  struct EdgeNode *prev_edge;
  struct EdgeNode *next_edge;
} Point;

typedef struct PointNode {
  Point *point;
  struct PointNode *next;
} PointNode;

typedef struct {
  PointNode *head;
  size_t count;
} PointList;

Point *point_create(double x, double y);
void point_destroy(Point *point);
int point_attach_edge(Point *point, struct EdgeNode *edge);
int point_detach_edge(Point *point, struct EdgeNode *edge);

PointList *point_list_create(void);
void point_list_destroy(PointList *list);
void point_list_append(PointList *list, Point *point);

#endif
