#include "model/app_state.h"

#include "edge.h"
#include <math.h>
#include <stdlib.h>

static Point *app_state_add_point(AppState *state, double x, double y) {
  Point *point = point_create(x, y);
  if (point == NULL)
    return NULL;

  size_t previous_count = state->points->count;
  point_list_append(state->points, point);
  if (state->points->count == previous_count) {
    point_destroy(point);
    return NULL;
  }

  return point;
}

static EdgeNode *app_state_add_edge_and_get(AppState *state, Point *start,
                                            Point *end) {
  if (!edge_list_append(state->edges, start, end))
    return NULL;
  EdgeNode *node = state->edges->head;
  while (node->next != NULL)
    node = node->next;
  return node;
}

static int app_state_init_scene(AppState *state) {
  static const double poly1[5][2] = {
      {200, 150}, {460, 150}, {560, 320}, {400, 480}, {180, 380}};
  Point *pts1[5];
  for (int i = 0; i < 5; i++) {
    pts1[i] = app_state_add_point(state, poly1[i][0], poly1[i][1]);
    if (!pts1[i])
      return 0;
  }
  EdgeNode *edges1[5];
  for (int i = 0; i < 5; i++) {
    edges1[i] = app_state_add_edge_and_get(state, pts1[i], pts1[(i + 1) % 5]);
    if (!edges1[i])
      return 0;
  }

  Point c1 = {.x = 280,
              .y = 80,
              .prev_edge = NULL,
              .next_edge = NULL,
              .continuity = CONTINUITY_G1};
  Point c2 = {.x = 380,
              .y = 80,
              .prev_edge = NULL,
              .next_edge = NULL,
              .continuity = CONTINUITY_G1};
  edge_set_bezier(&edges1[0]->edge, c1, c2);

  edge_set_length_constraint(&edges1[1]->edge, hypot(pts1[2]->x - pts1[1]->x,
                                                     pts1[2]->y - pts1[1]->y));

  static const double poly2[4][2] = {
      {750, 180}, {1000, 180}, {950, 420}, {700, 380}};
  Point *pts2[4];
  for (int i = 0; i < 4; i++) {
    pts2[i] = app_state_add_point(state, poly2[i][0], poly2[i][1]);
    if (!pts2[i])
      return 0;
  }
  EdgeNode *edges2[4];
  for (int i = 0; i < 4; i++) {
    edges2[i] = app_state_add_edge_and_get(state, pts2[i], pts2[(i + 1) % 4]);
    if (!edges2[i])
      return 0;
  }
  state->next_parallel_group_id = 1;
  edge_set_parallel_constraint(edges1[3], edges2[0],
                               state->next_parallel_group_id++);

  edges2[2]->edge.type = EDGE_TYPE_ARC;
  edges2[2]->edge.geometry.arc.side = ARC_SIDE_LEFT;
  pts2[2]->continuity = CONTINUITY_G1;
  pts2[2]->continuity_set_by_user = TRUE;
  pts2[3]->continuity = CONTINUITY_G0;
  pts2[3]->continuity_set_by_user = TRUE;

  app_state_link_point_edges(state);
  return 1;
}

AppState *app_state_create(void) {
  AppState *state = malloc(sizeof(*state));
  if (state == NULL)
    return NULL;

  state->points = point_list_create();
  state->edges = edge_list_create();
  state->hover_x = 0.0;
  state->hover_y = 0.0;
  state->hovered_point = NULL;
  state->hovered_edge = NULL;
  state->pairing_edge = NULL;
  state->use_bresenham = TRUE;
  state->next_parallel_group_id = 1;
  state->msaa_mode = MSAA_OFF;
  state->drawing_area = NULL;

  if (state->points == NULL || state->edges == NULL) {
    point_list_destroy(state->points);
    edge_list_destroy(state->edges);
    free(state);
    return NULL;
  }

  if (!app_state_init_scene(state)) {
    app_state_destroy(state);
    return NULL;
  }

  return state;
}

void app_state_destroy(AppState *state) {
  if (state == NULL)
    return;
  point_list_destroy(state->points);
  edge_list_destroy(state->edges);
  free(state);
}

void app_state_link_point_edges(AppState *state) {
  if (state == NULL || state->edges == NULL)
    return;
  for (EdgeNode *edge = state->edges->head; edge != NULL; edge = edge->next) {
    if (edge->edge.start != NULL)
      point_attach_edge(edge->edge.start, edge);
    if (edge->edge.end != NULL)
      point_attach_edge(edge->edge.end, edge);
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
  if (state == NULL || state->points == NULL || point == NULL)
    return 0;

  EdgeNode *prev_edge = point->prev_edge;
  EdgeNode *next_edge = point->next_edge;
  if (prev_edge == NULL || next_edge == NULL || prev_edge == next_edge)
    return 0;

  edge_remove_all_constraints(prev_edge);
  edge_remove_all_constraints(next_edge);

  app_state_disconnect_point_edges(state, point);

  PointNode *previous = NULL;
  for (PointNode *cur = state->points->head; cur != NULL; cur = cur->next) {
    if (cur->point == point) {
      if (previous == NULL)
        state->points->head = cur->next;
      else
        previous->next = cur->next;
      point_destroy(point);
      free(cur);
      state->points->count -= 1;
      return 1;
    }
    previous = cur;
  }

  return 0;
}

int app_state_insert_point(AppState *state, EdgeNode *edge, double x,
                           double y) {
  if (state == NULL || state->points == NULL || state->edges == NULL ||
      edge == NULL) {
    return 0;
  }

  edge_remove_all_constraints(edge);

  Point *point = point_create(x, y);
  PointNode *point_node = calloc(1, sizeof(*point_node));
  EdgeNode *split_edge = calloc(1, sizeof(*split_edge));
  if (point == NULL || point_node == NULL || split_edge == NULL) {
    point_destroy(point);
    free(point_node);
    free(split_edge);
    return 0;
  }

  Point *old_start = edge->edge.start;
  Point *old_end = edge->edge.end;

  split_edge->edge = edge->edge;
  split_edge->edge.start = point;
  split_edge->edge.end = old_end;
  split_edge->edge.constraint_mask = EDGE_CONSTRAINT_NONE;
  split_edge->edge.parallel.target = NULL;
  split_edge->edge.parallel.group_id = 0;
  split_edge->next = edge->next;

  edge->edge.end = point;

  if (old_start != NULL) {
    if (old_start->prev_edge == edge)
      old_start->prev_edge = NULL;
    else if (old_start->next_edge == edge)
      old_start->next_edge = NULL;
    old_start->next_edge = edge;
  }

  if (old_end != NULL) {
    if (old_end->prev_edge == edge)
      old_end->prev_edge = NULL;
    else if (old_end->next_edge == edge)
      old_end->next_edge = NULL;
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
    while (tail->next != NULL)
      tail = tail->next;
    tail->next = point_node;
  }
  state->points->count += 1;

  return 1;
}