#include "model/app_state.h"

#include <math.h>
#include <stdlib.h>

static Point *app_state_add_point(AppState *state, double x, double y) {
  Point *point = point_create(x, y);
  if (point == NULL) {
    return NULL;
  }

  size_t previous_count = state->points->count;
  point_list_append(state->points, point);
  if (state->points->count == previous_count) {
    point_destroy(point);
    return NULL;
  }

  return point;
}

static int app_state_add_edge(AppState *state, Point *start, Point *end) {
  return edge_list_append(state->edges, start, end);
}

static int app_state_add_sample_polygon(AppState *state) {
  static const double vertices[][2] = {
      {340, 180}, {700, 180}, {850, 390}, {610, 560}, {350, 430},
  };
  Point *points[sizeof(vertices) / sizeof(vertices[0])];
  size_t point_count = sizeof(points) / sizeof(points[0]);

  for (size_t i = 0; i < point_count; i++) {
    points[i] = app_state_add_point(state, vertices[i][0], vertices[i][1]);
    if (points[i] == NULL) {
      return 0;
    }
  }

  for (size_t i = 0; i < point_count; i++) {
    if (!app_state_add_edge(state, points[i], points[(i + 1) % point_count])) {
      return 0;
    }
  }

  if (state->edges == NULL || state->edges->head == NULL) {
    return 0;
  }

  EdgeNode *bezier_edge = state->edges->head;
  const Point *start = bezier_edge->edge.start;
  const Point *end = bezier_edge->edge.end;
  double dx = end->x - start->x;
  double dy = end->y - start->y;
  double length = hypot(dx, dy);

  if (length == 0.0) {
    return 0;
  }

  double normal_x = -dy / length;
  double normal_y = dx / length;
  double offset = length * 0.28;

  Point control1 = {
      start->x + dx * 0.33 + normal_x * offset,
      start->y + dy * 0.33 + normal_y * offset,
  };
  Point control2 = {
      end->x - dx * 0.33 + normal_x * offset,
      end->y - dy * 0.33 + normal_y * offset,
  };

  if (!edge_set_bezier(&bezier_edge->edge, control1, control2)) {
    return 0;
  }
  if (!edge_set_constraint(&bezier_edge->edge, EDGE_CONSTRAINT_LENGTH)) {
    return 0;
  }

  return 1;
}

AppState *app_state_create(void) {
  AppState *state = malloc(sizeof(*state));
  if (state == NULL) {
    return NULL;
  }

  state->points = point_list_create();
  state->edges = edge_list_create();
  if (state->points == NULL || state->edges == NULL) {
    point_list_destroy(state->points);
    edge_list_destroy(state->edges);
    free(state);
    return NULL;
  }

  if (!app_state_add_sample_polygon(state)) {
    app_state_destroy(state);
    return NULL;
  }

  return state;
}

void app_state_destroy(AppState *state) {
  if (state == NULL) {
    return;
  }

  point_list_destroy(state->points);
  edge_list_destroy(state->edges);
  free(state);
}

int app_state_insert_vertex(AppState *state, EdgeNode *edge, double x,
                            double y) {
  if (state == NULL || state->points == NULL || state->edges == NULL ||
      edge == NULL) {
    return 0;
  }

  Point *point = point_create(x, y);
  PointNode *point_node = malloc(sizeof(*point_node));
  EdgeNode *split_edge = malloc(sizeof(*split_edge));
  if (point == NULL || point_node == NULL || split_edge == NULL) {
    point_destroy(point);
    free(point_node);
    free(split_edge);
    return 0;
  }

  split_edge->edge = edge->edge;
  split_edge->edge.start = point;
  split_edge->next = edge->next;
  if (edge->edge.constraint_type == EDGE_CONSTRAINT_LENGTH) {
    edge->edge.constraints.length.length =
        hypot(point->x - edge->edge.start->x, point->y - edge->edge.start->y);
    split_edge->edge.constraints.length.length =
        hypot(edge->edge.end->x - point->x, edge->edge.end->y - point->y);
  }
  edge->edge.end = point;
  edge->next = split_edge;
  state->edges->count += 1;

  point_node->point = point;
  point_node->next = NULL;
  if (state->points->head == NULL) {
    state->points->head = point_node;
  } else {
    PointNode *tail = state->points->head;
    while (tail->next != NULL) {
      tail = tail->next;
    }
    tail->next = point_node;
  }
  state->points->count += 1;

  return 1;
}
