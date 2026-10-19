#ifndef POLYPEN_APP_STATE_H
#define POLYPEN_APP_STATE_H

#include "model/point.h"
#include "model/edge.h"

typedef struct {
  PointList *points;
  EdgeList *edges;
} AppState;

AppState *app_state_create(void);
void app_state_destroy(AppState *state);

#endif
