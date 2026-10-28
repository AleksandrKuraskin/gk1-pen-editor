#include "model/point.h"
#include "model/edge.h"

#include <math.h>
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
  point->continuity = CONTINUITY_G1;
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

void point_enforce_continuity(Point *v, struct EdgeNode *driving_edge) {
  if (v == NULL || v->continuity == CONTINUITY_G0) {
    return;
  }

  EdgeNode *e1 = v->prev_edge;
  EdgeNode *e2 = v->next_edge;
  if (e1 == NULL || e2 == NULL) {
    return;
  }

  EdgeNode *t_node = (e1->edge.type == EDGE_TYPE_BEZIER)
                         ? e1
                         : (e2->edge.type == EDGE_TYPE_BEZIER ? e2 : NULL);
  if (!t_node)
    return;
  EdgeNode *d_node = (t_node == e1) ? e2 : e1;
  if (e1->edge.type == EDGE_TYPE_BEZIER && e2->edge.type == EDGE_TYPE_BEZIER) {
    d_node = (driving_edge == e2) ? e2 : e1;
    t_node = (d_node == e1) ? e2 : e1;
  }
  double tx = 0, ty = 0, c1_l = 0;
  if (!edge_get_tangent_at_point(d_node, v, FALSE, &tx, &ty, &c1_l))
    return;
  Point *c_target = (t_node->edge.start == v)
                        ? &t_node->edge.geometry.bezier.control1
                        : &t_node->edge.geometry.bezier.control2;
  double h_l = (v->continuity == CONTINUITY_C1)
                   ? c1_l
                   : hypot(c_target->x - v->x, c_target->y - v->y);
  if (h_l < 1.0)
    h_l = c1_l;
  c_target->x = v->x - tx * h_l;
  c_target->y = v->y - ty * h_l;
}
