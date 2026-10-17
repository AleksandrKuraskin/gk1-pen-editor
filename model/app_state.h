#ifndef POLYPEN_APP_STATE_H
#define POLYPEN_APP_STATE_H

#include "model/point.h"

typedef struct {
  PointList *points;
} AppState;

AppState *app_state_create(void);
void app_state_destroy(AppState *state);
void app_state_add_point(AppState *state, double x, double y);

#endif
