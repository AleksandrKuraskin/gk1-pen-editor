#include "model/app_state.h"

#include <stdlib.h>

AppState *app_state_create(void) {
  AppState *state = malloc(sizeof(*state));
  if (state == NULL) {
    return NULL;
  }

  state->points = point_list_create();
  if (state->points == NULL) {
    free(state);
    return NULL;
  }

  return state;
}

void app_state_destroy(AppState *state) {
  if (state == NULL) {
    return;
  }

  point_list_destroy(state->points);
  free(state);
}

void app_state_add_point(AppState *state, double x, double y) {
  if (state == NULL || state->points == NULL) {
    return;
  }

  Point *point = point_create(x, y);
  if (point == NULL) {
    return;
  }

  point_list_append(state->points, point);
}
