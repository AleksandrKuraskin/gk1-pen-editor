#ifndef POLYPEN_DRAWING_AREA_CONTROLLER_H
#define POLYPEN_DRAWING_AREA_CONTROLLER_H

#include <gtk/gtk.h>

#include "model/app_state.h"

typedef struct {
  AppState *state;
  Point *selected_point;
  EdgeNode *selected_bezier_edge;
  int selected_bezier_handle;
  gboolean dragging;
} DragState;

Point *drawing_area_find_point_under_cursor(AppState *state, double x, double y);
void drawing_area_connect_drag(GtkWidget *drawing_area, AppState *state);
void drawing_area_motion_cb(GtkEventControllerMotion *controller, double x,
                            double y, gpointer user_data);
void drawing_area_click_cb(GtkGestureClick *gesture, int n_press, double x,
                          double y, gpointer user_data);
void drawing_area_release_cb(GtkGestureClick *gesture, int n_press, double x,
                            double y, gpointer user_data);

#endif
