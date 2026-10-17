#ifndef POLYPEN_MODEL_POINT_H
#define POLYPEN_MODEL_POINT_H

#include <stddef.h>

typedef struct {
  double x;
  double y;
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

PointList *point_list_create(void);
void point_list_destroy(PointList *list);
void point_list_append(PointList *list, Point *point);

#endif
