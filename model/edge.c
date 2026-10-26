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
  node->edge.constraint_type = EDGE_CONSTRAINT_LENGTH;
  node->edge.constraints.length.length =
      hypot(end->x - start->x, end->y - start->y);
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

int edge_set_bezier(Edge *edge, Point control1, Point control2) {
  if (edge == NULL) {
    return 0;
  }

  edge->type = EDGE_TYPE_BEZIER;
  edge->geometry.bezier.control1 = control1;
  edge->geometry.bezier.control2 = control2;
  return 1;
}

static void edge_solve_length_constraint(Edge *edge, Point *fixed_point) {
  if (edge == NULL || edge->constraint_type != EDGE_CONSTRAINT_LENGTH ||
      edge->start == NULL || edge->end == NULL) {
    return;
  }

  double dx = edge->end->x - edge->start->x;
  double dy = edge->end->y - edge->start->y;
  double distance = hypot(dx, dy);
  double target_length = edge->constraints.length.length;
  if (target_length < 0.0) {
    return;
  }

  if (distance > 0.0) {
    dx /= distance;
    dy /= distance;
  } else {
    dx = 1.0;
    dy = 0.0;
  }

  double start_weight = edge->start == fixed_point ? 0.0 : 1.0;
  double end_weight = edge->end == fixed_point ? 0.0 : 1.0;
  double total_weight = start_weight + end_weight;
  if (total_weight == 0.0) {
    return;
  }

  double correction = (distance - target_length) / total_weight;
  edge->start->x += start_weight * correction * dx;
  edge->start->y += start_weight * correction * dy;
  edge->end->x -= end_weight * correction * dx;
  edge->end->y -= end_weight * correction * dy;
}

static double edge_length_constraint_error(const Edge *edge) {
  if (edge == NULL || edge->constraint_type != EDGE_CONSTRAINT_LENGTH ||
      edge->start == NULL || edge->end == NULL) {
    return 0.0;
  }

  double current_length =
      hypot(edge->end->x - edge->start->x, edge->end->y - edge->start->y);
  return fabs(current_length - edge->constraints.length.length);
}

void edge_list_solve_length_constraints(EdgeList *list, Point *fixed_point) {
  if (list == NULL || fixed_point == NULL || list->head == NULL) {
    return;
  }

  size_t edge_count = 0;
  for (EdgeNode *node = list->head; node != NULL; node = node->next) {
    edge_count += 1;
  }

  EdgeNode **nodes = malloc(edge_count * sizeof(*nodes));
  if (nodes == NULL) {
    return;
  }

  size_t index = 0;
  for (EdgeNode *node = list->head; node != NULL; node = node->next) {
    nodes[index++] = node;
  }

  size_t max_iterations = edge_count * 4;
  if (max_iterations < EDGE_CONSTRAINT_SOLVER_MIN_ITERATIONS) {
    max_iterations = EDGE_CONSTRAINT_SOLVER_MIN_ITERATIONS;
  }
  if (max_iterations > EDGE_CONSTRAINT_SOLVER_MAX_ITERATIONS) {
    max_iterations = EDGE_CONSTRAINT_SOLVER_MAX_ITERATIONS;
  }

  for (size_t iteration = 0; iteration < max_iterations; iteration++) {
    for (size_t i = 0; i < edge_count; i++) {
      edge_solve_length_constraint(&nodes[i]->edge, fixed_point);
    }
    for (size_t i = edge_count; i > 0; i--) {
      edge_solve_length_constraint(&nodes[i - 1]->edge, fixed_point);
    }

    double maximum_error = 0.0;
    for (size_t i = 0; i < edge_count; i++) {
      double error = edge_length_constraint_error(&nodes[i]->edge);
      if (error > maximum_error) {
        maximum_error = error;
      }
    }
    if (maximum_error <= EDGE_CONSTRAINT_SOLVER_TOLERANCE) {
      break;
    }
  }

  free(nodes);
}