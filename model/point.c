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

  if (e1->edge.type == EDGE_TYPE_BEZIER && e2->edge.type == EDGE_TYPE_BEZIER) {
    Point *c1 = (e1->edge.start == v) ? &e1->edge.geometry.bezier.control1
                                      : &e1->edge.geometry.bezier.control2;
    Point *c2 = (e2->edge.start == v) ? &e2->edge.geometry.bezier.control1
                                      : &e2->edge.geometry.bezier.control2;

    Point *c_driver = (driving_edge == e2) ? c2 : c1;
    Point *c_target = (driving_edge == e2) ? c1 : c2;

    double vx = c_driver->x - v->x;
    double vy = c_driver->y - v->y;
    double len = hypot(vx, vy);
    if (len < 1e-5)
      return;

    if (v->continuity == CONTINUITY_C1) {
      c_target->x = v->x - vx;
      c_target->y = v->y - vy;
    } else if (v->continuity == CONTINUITY_G1) {
      double target_len = hypot(c_target->x - v->x, c_target->y - v->y);
      if (target_len < 1.0)
        target_len = len;
      c_target->x = v->x - (vx / len) * target_len;
      c_target->y = v->y - (vy / len) * target_len;
    }
    return;
  }

  EdgeNode *e_bez = (e1->edge.type == EDGE_TYPE_BEZIER)
                        ? e1
                        : ((e2->edge.type == EDGE_TYPE_BEZIER) ? e2 : NULL);
  EdgeNode *e_str = (e1->edge.type == EDGE_TYPE_STRAIGHT)
                        ? e1
                        : ((e2->edge.type == EDGE_TYPE_STRAIGHT) ? e2 : NULL);

  if (e_bez != NULL && e_str != NULL) {
    Point *c_bez = (e_bez->edge.start == v)
                       ? &e_bez->edge.geometry.bezier.control1
                       : &e_bez->edge.geometry.bezier.control2;
    Point *p_other =
        (e_str->edge.start == v) ? e_str->edge.end : e_str->edge.start;
    if (p_other == NULL)
      return;

    double sx = v->x - p_other->x;
    double sy = v->y - p_other->y;
    double slen = hypot(sx, sy);
    if (slen < 1e-5)
      return;
    double tx = sx / slen;
    double ty = sy / slen;

    if (v->continuity == CONTINUITY_C1) {
      c_bez->x = v->x + tx * (slen / 3.0);
      c_bez->y = v->y + ty * (slen / 3.0);
    } else if (v->continuity == CONTINUITY_G1) {
      double cur_len = hypot(c_bez->x - v->x, c_bez->y - v->y);
      if (cur_len < 1.0)
        cur_len = slen / 3.0;

      if (driving_edge == e_bez) {
        double dot = (c_bez->x - v->x) * tx + (c_bez->y - v->y) * ty;
        if (dot < 5.0)
          dot = 5.0;
        c_bez->x = v->x + tx * dot;
        c_bez->y = v->y + ty * dot;
      } else {
        c_bez->x = v->x + tx * cur_len;
        c_bez->y = v->y + ty * cur_len;
      }
    }
  }
}
