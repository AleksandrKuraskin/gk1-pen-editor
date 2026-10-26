#ifndef POLYPEN_APP_STATE_H
#define POLYPEN_APP_STATE_H

#include "model/point.h"
#include "model/edge.h"

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
int app_state_insert_vertex(AppState *state, EdgeNode *edge, double x,
                            double y);

#endif
