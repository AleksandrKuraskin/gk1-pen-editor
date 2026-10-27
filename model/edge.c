#include "model/edge.h"

#include <math.h>
#include <stdlib.h>

#define EDGE_CONSTRAINT_SOLVER_TOLERANCE 0.01
#define EDGE_CONSTRAINT_SOLVER_MIN_ITERATIONS 16
#define EDGE_CONSTRAINT_SOLVER_MAX_ITERATIONS 256

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
  node->edge.type = EDGE_TYPE_STRAIGHT;
  node->edge.constraint_type = EDGE_CONSTRAINT_NONE;
  node->next = NULL;

  if (!point_attach_edge(start, node) || !point_attach_edge(end, node)) {
    free(node);
    return 0;
  }

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

int edge_set_bezier(Edge *edge, Point control1, Point control2) {
  if (edge == NULL) {
    return 0;
  }

  edge->type = EDGE_TYPE_BEZIER;
  edge->geometry.bezier.control1 = control1;
  edge->geometry.bezier.control2 = control2;
  return 1;
}

int edge_set_arc(Edge *edge, ArcSide side) {
  if (edge == NULL || edge->start == NULL || edge->end == NULL) {
    return 0;
  }

  edge->type = EDGE_TYPE_ARC;
  edge->geometry.arc.side = side;
  return 1;
}

int edge_set_constraint(Edge *edge, int constraint_flag) {
  if (edge == NULL)
    return 0;
  if (constraint_flag == EDGE_CONSTRAINT_NONE) {
    edge->constraint_mask = EDGE_CONSTRAINT_NONE;
    edge->length.length = 0.0;
    return 1;
  }
  if (constraint_flag & EDGE_CONSTRAINT_LENGTH) {
    if (edge->start == NULL || edge->end == NULL)
      return 0;
    double cur_len =
        hypot(edge->end->x - edge->start->x, edge->end->y - edge->start->y);
    edge_set_length_constraint(edge, cur_len);
    return 1;
  }
  return 0;
}

void edge_set_length_constraint(Edge *edge, double length) {
  if (edge == NULL)
    return;
  edge->constraint_mask |= EDGE_CONSTRAINT_LENGTH;
  edge->length.length = length;
}

void edge_remove_length_constraint(Edge *edge) {
  if (edge == NULL)
    return;
  edge->constraint_mask &= ~EDGE_CONSTRAINT_LENGTH;
  edge->length.length = 0.0;
}

void edge_set_parallel_constraint(EdgeNode *e1, EdgeNode *e2, int group_id) {
  if (e1 == NULL || e2 == NULL || e1 == e2)
    return;
  e1->edge.constraint_mask |= EDGE_CONSTRAINT_PARALLEL;
  e1->edge.parallel.target = e2;
  e1->edge.parallel.group_id = group_id;

  e2->edge.constraint_mask |= EDGE_CONSTRAINT_PARALLEL;
  e2->edge.parallel.target = e1;
  e2->edge.parallel.group_id = group_id;
}

void edge_remove_parallel_constraint(EdgeNode *edge_node) {
  if (edge_node == NULL)
    return;
  EdgeNode *target = edge_node->edge.parallel.target;
  edge_node->edge.constraint_mask &= ~EDGE_CONSTRAINT_PARALLEL;
  edge_node->edge.parallel.target = NULL;
  edge_node->edge.parallel.group_id = 0;

  if (target != NULL && target->edge.parallel.target == edge_node) {
    target->edge.constraint_mask &= ~EDGE_CONSTRAINT_PARALLEL;
    target->edge.parallel.target = NULL;
    target->edge.parallel.group_id = 0;
  }
}

void edge_remove_all_constraints(EdgeNode *edge_node) {
  if (edge_node == NULL)
    return;
  edge_remove_length_constraint(&edge_node->edge);
  edge_remove_parallel_constraint(edge_node);
}

static void edge_solve_length_constraint(Edge *edge, Point *fixed_point) {
  if (edge == NULL || !(edge->constraint_mask & EDGE_CONSTRAINT_LENGTH) ||
      edge->start == NULL || edge->end == NULL) {
    return;
  }

  double dx = edge->end->x - edge->start->x;
  double dy = edge->end->y - edge->start->y;
  double distance = hypot(dx, dy);
  double target = edge->length.length;
  if (target <= 0.0)
    return;

  if (distance > 1e-7) {
    dx /= distance;
    dy /= distance;
  } else {
    dx = 1.0;
    dy = 0.0;
  }

  double start_weight = (edge->start == fixed_point) ? 0.0 : 1.0;
  double end_weight = (edge->end == fixed_point) ? 0.0 : 1.0;
  double total_weight = start_weight + end_weight;
  if (total_weight <= 0.0)
    return;

  double correction = (distance - target) / total_weight;
  edge->start->x += start_weight * correction * dx;
  edge->start->y += start_weight * correction * dy;
  edge->end->x -= end_weight * correction * dx;
  edge->end->y -= end_weight * correction * dy;
}

static void edge_solve_parallel_constraint(Edge *edge, Point *fixed_point) {
  if (edge == NULL || !(edge->constraint_mask & EDGE_CONSTRAINT_PARALLEL) ||
      edge->parallel.target == NULL || edge->start == NULL ||
      edge->end == NULL) {
    return;
  }

  Edge *target = &edge->parallel.target->edge;
  if (target->start == NULL || target->end == NULL)
    return;

  double dx = target->end->x - target->start->x;
  double dy = target->end->y - target->start->y;
  double len = hypot(dx, dy);
  if (len < 1e-7)
    return;
  dx /= len;
  dy /= len;

  double edx = edge->end->x - edge->start->x;
  double edy = edge->end->y - edge->start->y;
  double elen = hypot(edx, edy);
  if (elen < 1e-7)
    return;

  if (edx * dx + edy * dy < 0.0) {
    dx = -dx;
    dy = -dy;
  }

  if (edge->start == fixed_point && edge->end != fixed_point) {
    edge->end->x = edge->start->x + dx * elen;
    edge->end->y = edge->start->y + dy * elen;
  } else if (edge->end == fixed_point && edge->start != fixed_point) {
    edge->start->x = edge->end->x - dx * elen;
    edge->start->y = edge->end->y - dy * elen;
  } else {
    double cx = (edge->start->x + edge->end->x) * 0.5;
    double cy = (edge->start->y + edge->end->y) * 0.5;
    edge->start->x = cx - dx * (elen * 0.5);
    edge->start->y = cy - dy * (elen * 0.5);
    edge->end->x = cx + dx * (elen * 0.5);
    edge->end->y = cy + dy * (elen * 0.5);
  }
}

void edge_list_solve_constraints(EdgeList *list, Point *fixed_point) {
  if (list == NULL || list->head == NULL)
    return;

  for (int iter = 0; iter < EDGE_CONSTRAINT_SOLVER_MAX_ITERATIONS; iter++) {
    for (EdgeNode *node = list->head; node != NULL; node = node->next) {
      if (node->edge.constraint_mask & EDGE_CONSTRAINT_LENGTH) {
        edge_solve_length_constraint(&node->edge, fixed_point);
      }
      if (node->edge.constraint_mask & EDGE_CONSTRAINT_PARALLEL) {
        edge_solve_parallel_constraint(&node->edge, fixed_point);
      }
    }
  }
}