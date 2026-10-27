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

  app_state_link_point_edges(state);

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
      .x = start->x + dx * 0.33 + normal_x * offset,
      .y = start->y + dy * 0.33 + normal_y * offset,
      .prev_edge = NULL,
      .next_edge = NULL,
  };
  Point control2 = {
      .x = end->x - dx * 0.33 + normal_x * offset,
      .y = end->y - dy * 0.33 + normal_y * offset,
      .prev_edge = NULL,
      .next_edge = NULL,
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
  state->hover_x = 0.0;
  state->hover_y = 0.0;
  state->hovered_point = NULL;
  state->hovered_edge = NULL;
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

void app_state_link_point_edges(AppState *state) {
  if (state == NULL || state->edges == NULL) {
    return;
  }

  for (EdgeNode *edge = state->edges->head; edge != NULL; edge = edge->next) {
    if (edge->edge.start != NULL) {
      point_attach_edge(edge->edge.start, edge);
    }
    if (edge->edge.end != NULL) {
      point_attach_edge(edge->edge.end, edge);
    }
  }
}

void app_state_disconnect_point_edges(AppState *state, Point *point) {
  if (state == NULL || point == NULL) {
    return;
  }

  EdgeNode *prev_edge = point->prev_edge;
  EdgeNode *next_edge = point->next_edge;
  if (prev_edge == NULL || next_edge == NULL || prev_edge == next_edge) {
    return;
  }

  Point *prev_point = prev_edge->edge.start == point ? prev_edge->edge.end
                                                     : prev_edge->edge.start;
  Point *next_point = next_edge->edge.start == point ? next_edge->edge.end
                                                     : next_edge->edge.start;
  if (prev_point == NULL || next_point == NULL) {
    return;
  }

  if (prev_edge->edge.start == point) {
    prev_edge->edge.start = next_point;
  } else {
    prev_edge->edge.end = next_point;
  }

  point_detach_edge(prev_point, prev_edge);
  point_detach_edge(next_point, next_edge);
  point_detach_edge(point, prev_edge);
  point_detach_edge(point, next_edge);

  EdgeNode *previous = NULL;
  for (EdgeNode *current = state->edges->head; current != NULL;
       current = current->next) {
    if (current == next_edge) {
      if (previous == NULL) {
        state->edges->head = current->next;
      } else {
        previous->next = current->next;
      }
      free(current);
      state->edges->count -= 1;
      break;
    }
    previous = current;
  }

  point_attach_edge(prev_point, prev_edge);
  point_attach_edge(next_point, prev_edge);
}

int app_state_remove_point(AppState *state, Point *point) {
  if (state == NULL || state->points == NULL || point == NULL) {
    return 0;
  }

  EdgeNode *prev_edge = point->prev_edge;
  EdgeNode *next_edge = point->next_edge;
  if (prev_edge == NULL || next_edge == NULL || prev_edge == next_edge) {
    return 0;
  }

  Point *prev_point = prev_edge->edge.start == point ? prev_edge->edge.end
                                                     : prev_edge->edge.start;
  Point *next_point = next_edge->edge.start == point ? next_edge->edge.end
                                                     : next_edge->edge.start;
  if (prev_point == NULL || next_point == NULL) {
    return 0;
  }

  prev_edge->edge.constraint_type = EDGE_CONSTRAINT_NONE;
  next_edge->edge.constraint_type = EDGE_CONSTRAINT_NONE;

  app_state_disconnect_point_edges(state, point);

  PointNode *previous = NULL;
  for (PointNode *current = state->points->head; current != NULL;
       current = current->next) {
    if (current->point == point) {
      if (previous == NULL) {
        state->points->head = current->next;
      } else {
        previous->next = current->next;
      }
      point_destroy(point);
      free(current);
      state->points->count -= 1;
      return 1;
    }
    previous = current;
  }

  return 0;
}

int app_state_insert_point(AppState *state, EdgeNode *edge, double x, double y) {
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

  Point *old_start = edge->edge.start;
  Point *old_end = edge->edge.end;

  if (old_start == NULL || old_end == NULL) {
    point_destroy(point);
    free(point_node);
    free(split_edge);
    return 0;
  }

  EdgeNode *old_next = edge->next;
  split_edge->edge = edge->edge;
  split_edge->edge.start = point;
  split_edge->edge.end = old_end;
  split_edge->next = old_next;

  if (edge->edge.constraint_type == EDGE_CONSTRAINT_LENGTH) {
    edge->edge.constraints.length.length =
        hypot(point->x - old_start->x, point->y - old_start->y);
    split_edge->edge.constraints.length.length =
        hypot(old_end->x - point->x, old_end->y - point->y);
  }

  edge->edge.end = point;

  if (old_start != NULL) {
    if (old_start->prev_edge == edge) {
      old_start->prev_edge = NULL;
    } else if (old_start->next_edge == edge) {
      old_start->next_edge = NULL;
    }
    old_start->next_edge = edge;
  }

  if (old_end != NULL) {
    if (old_end->prev_edge == edge) {
      old_end->prev_edge = NULL;
    } else if (old_end->next_edge == edge) {
      old_end->next_edge = NULL;
    }
    old_end->prev_edge = split_edge;
  }

  point->prev_edge = edge;
  point->next_edge = split_edge;

  point_attach_edge(old_start, edge);
  point_attach_edge(point, edge);
  point_attach_edge(point, split_edge);
  point_attach_edge(old_end, split_edge);

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
