#include "controller/drawing_area_controller.h"

#include <math.h>
#include <stdio.h>

#include "config/config.h"

#include "model/app_state.h"

static DragState drag_state = {
    .state = NULL, .selected_point = NULL, .dragging = FALSE};

Point *drawing_area_find_point_under_cursor(AppState *state, double x,
                                            double y) {
  if (state == NULL || state->points == NULL) {
    return NULL;
  }

  for (PointNode *node = state->points->head; node != NULL; node = node->next) {
    Point *point = node->point;
    if (point == NULL) {
      continue;
    }

    double dx = point->x - x;
    double dy = point->y - y;
    if (hypot(dx, dy) <= CONFIG_POINT_RADIUS) {
      return point;
    }
  }

  return NULL;
}

void drawing_area_connect_drag(GtkWidget *drawing_area, AppState *state) {
  if (drawing_area == NULL || state == NULL) {
    return;
  }

  drag_state.state = state;
  drag_state.selected_point = NULL;
  drag_state.dragging = FALSE;

  GtkEventController *motion_controller = gtk_event_controller_motion_new();
  gtk_widget_add_controller(drawing_area, motion_controller);
  g_signal_connect(motion_controller, "motion",
                   G_CALLBACK(drawing_area_motion_cb), NULL);

  GtkGesture *click_gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_gesture),
                                GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller(drawing_area, GTK_EVENT_CONTROLLER(click_gesture));
  g_signal_connect(click_gesture, "pressed", G_CALLBACK(drawing_area_click_cb),
                   state);
  g_signal_connect(click_gesture, "released",
                   G_CALLBACK(drawing_area_release_cb), state);
}

void drawing_area_motion_cb(GtkEventControllerMotion *controller, double x,
                            double y, gpointer user_data) {
  (void)controller;
  (void)user_data;

  if (!drag_state.dragging || drag_state.selected_point == NULL) {
    return;
  }

  drag_state.selected_point->x = x;
  drag_state.selected_point->y = y;

  GtkWidget *widget =
      gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
  if (widget != NULL) {
    gtk_widget_queue_draw(widget);
  }
}

void drawing_area_click_cb(GtkGestureClick *gesture, int n_press, double x,
                           double y, gpointer user_data) {
  (void)n_press;
  (void)gesture;

  AppState *state = user_data;
  if (state == NULL) {
    return;
  }

  drag_state.selected_point = drawing_area_find_point_under_cursor(state, x, y);
  drag_state.dragging = (drag_state.selected_point != NULL);
}

void drawing_area_release_cb(GtkGestureClick *gesture, int n_press, double x,
                             double y, gpointer user_data) {
  (void)gesture;
  (void)n_press;
  (void)x;
  (void)y;
  (void)user_data;

  drag_state.dragging = FALSE;
  drag_state.selected_point = NULL;
}
