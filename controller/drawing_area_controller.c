#include "controller/drawing_area_controller.h"

#include <math.h>
#include <stdio.h>

#include "config/config.h"

#include "model/app_state.h"

static DragState drag_state = {
    .state = NULL, .selected_point = NULL, .dragging = FALSE};

typedef struct {
  GtkWidget *drawing_area;
  GtkWidget *popover;
  AppState *state;
  EdgeNode *edge;
  double insert_x;
  double insert_y;
} EdgeMenuState;

static EdgeMenuState edge_menu_state;

static EdgeNode *drawing_area_find_edge_near_cursor(AppState *state, double x,
                                                    double y, double *closest_x,
                                                    double *closest_y) {
  if (state == NULL || state->edges == NULL) {
    return NULL;
  }

  EdgeNode *nearest_edge = NULL;
  double nearest_distance = CONFIG_PICK_TOLERANCE;
  for (EdgeNode *node = state->edges->head; node != NULL; node = node->next) {
    Point *start = node->edge.start;
    Point *end = node->edge.end;
    if (start == NULL || end == NULL) {
      continue;
    }

    double dx = end->x - start->x;
    double dy = end->y - start->y;
    double length_squared = dx * dx + dy * dy;
    double parameter = 0.0;
    if (length_squared > 0.0) {
      parameter = ((x - start->x) * dx + (y - start->y) * dy) / length_squared;
      parameter = fmax(0.0, fmin(1.0, parameter));
    }

    double projected_x = start->x + parameter * dx;
    double projected_y = start->y + parameter * dy;
    double distance = hypot(x - projected_x, y - projected_y);
    if (distance <= nearest_distance) {
      nearest_distance = distance;
      nearest_edge = node;
      *closest_x = projected_x;
      *closest_y = projected_y;
    }
  }

  return nearest_edge;
}

static void edge_menu_close(void) {
  if (edge_menu_state.popover != NULL) {
    gtk_popover_popdown(GTK_POPOVER(edge_menu_state.popover));
  }
  edge_menu_state.edge = NULL;
}

static void edge_menu_set_type(GtkButton *button, gpointer user_data) {
  (void)button;
  if (edge_menu_state.edge != NULL) {
    edge_menu_state.edge->edge.type = (EdgeType)GPOINTER_TO_INT(user_data);
    gtk_widget_queue_draw(edge_menu_state.drawing_area);
  }
  edge_menu_close();
}

static void edge_menu_insert_vertex(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  if (edge_menu_state.edge != NULL) {
    app_state_insert_vertex(edge_menu_state.state, edge_menu_state.edge,
                            edge_menu_state.insert_x, edge_menu_state.insert_y);
    gtk_widget_queue_draw(edge_menu_state.drawing_area);
  }
  edge_menu_close();
}

static void drawing_area_secondary_click_cb(GtkGestureClick *gesture,
                                            int n_press, double x, double y,
                                            gpointer user_data) {
  (void)n_press;
  AppState *state = user_data;
  if (state == NULL || edge_menu_state.popover == NULL) {
    return;
  }

  double closest_x = x;
  double closest_y = y;
  EdgeNode *edge =
      drawing_area_find_edge_near_cursor(state, x, y, &closest_x, &closest_y);
  if (edge == NULL) {
    edge_menu_close();
    return;
  }

  edge_menu_state.state = state;
  edge_menu_state.edge = edge;
  edge_menu_state.insert_x = closest_x;
  edge_menu_state.insert_y = closest_y;

  GdkRectangle pointing_to = {(int)lround(x), (int)lround(y), 1, 1};
  gtk_popover_set_pointing_to(GTK_POPOVER(edge_menu_state.popover),
                              &pointing_to);
  gtk_popover_popup(GTK_POPOVER(edge_menu_state.popover));
  (void)gesture;
}

static void drawing_area_create_edge_menu(GtkWidget *drawing_area) {
  edge_menu_state.drawing_area = drawing_area;
  edge_menu_state.popover = gtk_popover_new();
  gtk_widget_set_parent(edge_menu_state.popover, drawing_area);

  GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  const char *labels[] = {"Straight", "Bezier", "Arc", "Insert vertex"};
  for (int i = 0; i < 4; i++) {
    GtkWidget *button = gtk_button_new_with_label(labels[i]);
    gtk_widget_set_halign(button, GTK_ALIGN_FILL);
    gtk_box_append(GTK_BOX(menu_box), button);
    if (i < 3) {
      g_signal_connect(button, "clicked", G_CALLBACK(edge_menu_set_type),
                       GINT_TO_POINTER(i));
    } else {
      g_signal_connect(button, "clicked", G_CALLBACK(edge_menu_insert_vertex),
                       NULL);
    }
  }
  gtk_popover_set_child(GTK_POPOVER(edge_menu_state.popover), menu_box);
}

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
  drawing_area_create_edge_menu(drawing_area);

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

  GtkGesture *secondary_click = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(secondary_click),
                                GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(drawing_area,
                            GTK_EVENT_CONTROLLER(secondary_click));
  g_signal_connect(secondary_click, "pressed",
                   G_CALLBACK(drawing_area_secondary_click_cb), state);
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
