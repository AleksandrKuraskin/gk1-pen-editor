#ifndef POLYPEN_APP_STATE_H
#define POLYPEN_APP_STATE_H

#include "model/edge.h"
#include "model/point.h"

typedef struct {
  PointList *points;
  EdgeList *edges;
  double hover_x;
  double hover_y;
  Point *hovered_point;
  EdgeNode *hovered_edge;
} AppState;

AppState *app_state_create(void);
void app_state_destroy(AppState *state);
void app_state_link_point_edges(AppState *state);
void app_state_disconnect_point_edges(AppState *state, Point *point);
int app_state_insert_point(AppState *state, EdgeNode *edge, double x, double y);
int app_state_remove_point(AppState *state, Point *point);

#endif
