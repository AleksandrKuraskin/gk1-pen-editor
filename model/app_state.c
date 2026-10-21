#include "model/app_state.h"

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
