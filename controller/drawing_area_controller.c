#include "controller/drawing_area_controller.h"

#include <math.h>
#include <stdio.h>

#include "config/config.h"

#include "model/app_state.h"

static DragState drag_state;

typedef struct {
  GtkWidget *drawing_area;
  GtkWidget *popover;
  GtkWidget *constraint_dropdown;
  AppState *state;
  EdgeNode *edge;
  double insert_x;
  double insert_y;
} EdgeMenuState;

static EdgeMenuState edge_menu_state;

static void drawing_area_clamp_to_canvas(GtkWidget *widget, double *x,
                                         double *y) {
  if (widget == NULL || x == NULL || y == NULL) {
    return;
  }

  int width = gtk_widget_get_width(widget);
  int height = gtk_widget_get_height(widget);
  if (width <= 0 || height <= 0) {
    return;
  }

  *x = fmin(fmax(*x, 0.0), (double)width);
  *y = fmin(fmax(*y, 0.0), (double)height);
}

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
    gtk_widget_set_visible(edge_menu_state.popover, FALSE);
  }
  edge_menu_state.edge = NULL;
}

static void edge_menu_destroy(void) {
  if (edge_menu_state.popover == NULL) {
    return;
  }

  gtk_popover_popdown(GTK_POPOVER(edge_menu_state.popover));
  if (gtk_widget_get_parent(edge_menu_state.popover) != NULL) {
    gtk_widget_unparent(edge_menu_state.popover);
  }
  edge_menu_state.popover = NULL;
}

static void edge_menu_set_bezier_defaults(EdgeNode *edge) {
  if (edge == NULL || edge->edge.start == NULL || edge->edge.end == NULL) {
    return;
  }

  double dx = edge->edge.end->x - edge->edge.start->x;
  double dy = edge->edge.end->y - edge->edge.start->y;
  double length = hypot(dx, dy);

  if (length == 0.0) {
    edge_set_bezier(&edge->edge, *edge->edge.start, *edge->edge.end);
    return;
  }

  double normal_x = -dy / length;
  double normal_y = dx / length;
  double offset = length * 0.28;

  Point control1 = {
      edge->edge.start->x + dx * 0.33 + normal_x * offset,
      edge->edge.start->y + dy * 0.33 + normal_y * offset,
  };
  Point control2 = {
      edge->edge.end->x - dx * 0.33 + normal_x * offset,
      edge->edge.end->y - dy * 0.33 + normal_y * offset,
  };

  edge_set_bezier(&edge->edge, control1, control2);
}

static void edge_menu_set_type(GtkButton *button, gpointer user_data) {
  (void)button;
  if (edge_menu_state.edge != NULL) {
    EdgeType type = (EdgeType)GPOINTER_TO_INT(user_data);
    if (type == EDGE_TYPE_BEZIER) {
      edge_menu_set_bezier_defaults(edge_menu_state.edge);
    } else {
      edge_menu_state.edge->edge.type = type;
    }
    gtk_widget_queue_draw(edge_menu_state.drawing_area);
  }
  edge_menu_close();
}

static void edge_menu_set_constraint(GtkButton *button, gpointer user_data) {
  (void)button;
  EdgeConstraintType constraint_type =
      (EdgeConstraintType)GPOINTER_TO_INT(user_data);
  GtkPopover *constraint_popover = gtk_menu_button_get_popover(
      GTK_MENU_BUTTON(edge_menu_state.constraint_dropdown));
  if (edge_menu_state.edge != NULL &&
      edge_set_constraint(&edge_menu_state.edge->edge, constraint_type)) {
    gtk_widget_queue_draw(edge_menu_state.drawing_area);
  }
  gtk_menu_button_set_label(
      GTK_MENU_BUTTON(edge_menu_state.constraint_dropdown),
      constraint_type == EDGE_CONSTRAINT_LENGTH ? "Length" : "None");
  if (constraint_popover != NULL) {
    gtk_popover_popdown(GTK_POPOVER(constraint_popover));
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
  gtk_menu_button_set_label(
      GTK_MENU_BUTTON(edge_menu_state.constraint_dropdown),
      edge->edge.constraint_type == EDGE_CONSTRAINT_LENGTH ? "Length" : "None");

  GdkRectangle pointing_to = {(int)lround(x), (int)lround(y), 1, 1};
  gtk_popover_set_pointing_to(GTK_POPOVER(edge_menu_state.popover),
                              &pointing_to);
  gtk_popover_popup(GTK_POPOVER(edge_menu_state.popover));
  (void)gesture;
}

static void drawing_area_create_edge_menu(GtkWidget *drawing_area) {
  edge_menu_state.drawing_area = drawing_area;
  edge_menu_state.popover = gtk_popover_new();
  if (edge_menu_state.popover != NULL) {
    gtk_popover_set_autohide(GTK_POPOVER(edge_menu_state.popover), TRUE);
    gtk_widget_set_parent(edge_menu_state.popover, drawing_area);
    g_signal_connect(drawing_area, "destroy", G_CALLBACK(edge_menu_destroy),
                     NULL);
  }

  GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  GtkWidget *section_label = gtk_label_new("Edge type");
  gtk_widget_set_halign(section_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(menu_box), section_label);

  const char *type_labels[] = {"Straight", "Bezier", "Arc"};
  for (int i = 0; i < 3; i++) {
    GtkWidget *button = gtk_button_new_with_label(type_labels[i]);
    gtk_widget_set_halign(button, GTK_ALIGN_FILL);
    gtk_box_append(GTK_BOX(menu_box), button);
    g_signal_connect(button, "clicked", G_CALLBACK(edge_menu_set_type),
                     GINT_TO_POINTER(i));
  }

  gtk_box_append(GTK_BOX(menu_box),
                 gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
  section_label = gtk_label_new("Constraints");
  gtk_widget_set_halign(section_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(menu_box), section_label);
  const char *constraint_labels[] = {"None", "Length"};
  edge_menu_state.constraint_dropdown = gtk_menu_button_new();
  gtk_menu_button_set_label(
      GTK_MENU_BUTTON(edge_menu_state.constraint_dropdown), "None");
  gtk_widget_set_halign(edge_menu_state.constraint_dropdown, GTK_ALIGN_FILL);
  GtkWidget *constraint_popover = gtk_popover_new();
  GtkWidget *constraint_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  for (int i = 0; i < 2; i++) {
    GtkWidget *button = gtk_button_new_with_label(constraint_labels[i]);
    gtk_widget_set_halign(button, GTK_ALIGN_FILL);
    gtk_box_append(GTK_BOX(constraint_box), button);
    g_signal_connect(button, "clicked", G_CALLBACK(edge_menu_set_constraint),
                     GINT_TO_POINTER(i));
  }
  gtk_popover_set_child(GTK_POPOVER(constraint_popover), constraint_box);
  gtk_menu_button_set_popover(
      GTK_MENU_BUTTON(edge_menu_state.constraint_dropdown), constraint_popover);
  gtk_box_append(GTK_BOX(menu_box), edge_menu_state.constraint_dropdown);

  gtk_box_append(GTK_BOX(menu_box),
                 gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
  section_label = gtk_label_new("Add point");
  gtk_widget_set_halign(section_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(menu_box), section_label);
  GtkWidget *insert_button = gtk_button_new_with_label("Insert vertex");
  gtk_widget_set_halign(insert_button, GTK_ALIGN_FILL);
  gtk_box_append(GTK_BOX(menu_box), insert_button);
  g_signal_connect(insert_button, "clicked",
                   G_CALLBACK(edge_menu_insert_vertex), NULL);

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

static EdgeNode *
drawing_area_find_bezier_handle_under_cursor(AppState *state, double x,
                                             double y, int *handle_index) {
  if (state == NULL || state->edges == NULL || handle_index == NULL) {
    return NULL;
  }

  for (EdgeNode *node = state->edges->head; node != NULL; node = node->next) {
    if (node->edge.type != EDGE_TYPE_BEZIER) {
      continue;
    }

    double dx1 = node->edge.geometry.bezier.control1.x - x;
    double dy1 = node->edge.geometry.bezier.control1.y - y;
    if (hypot(dx1, dy1) <= CONFIG_CONTROL_POINT_RADIUS + 4.0) {
      *handle_index = 0;
      return node;
    }

    double dx2 = node->edge.geometry.bezier.control2.x - x;
    double dy2 = node->edge.geometry.bezier.control2.y - y;
    if (hypot(dx2, dy2) <= CONFIG_CONTROL_POINT_RADIUS + 4.0) {
      *handle_index = 1;
      return node;
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
  drag_state.selected_bezier_edge = NULL;
  drag_state.selected_bezier_handle = -1;
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

  if (!drag_state.dragging) {
    return;
  }

  GtkWidget *widget =
      gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));

  if (drag_state.selected_point != NULL) {
    double next_x = x;
    double next_y = y;
    drawing_area_clamp_to_canvas(widget, &next_x, &next_y);
    drag_state.selected_point->x = next_x;
    drag_state.selected_point->y = next_y;
    edge_list_solve_length_constraints(drag_state.state->edges,
                                       drag_state.selected_point);
  } else if (drag_state.selected_bezier_edge != NULL &&
             drag_state.selected_bezier_edge->edge.type == EDGE_TYPE_BEZIER) {
    double next_x = x;
    double next_y = y;
    drawing_area_clamp_to_canvas(widget, &next_x, &next_y);

    if (drag_state.selected_bezier_handle == 0) {
      drag_state.selected_bezier_edge->edge.geometry.bezier.control1.x = next_x;
      drag_state.selected_bezier_edge->edge.geometry.bezier.control1.y = next_y;
    } else if (drag_state.selected_bezier_handle == 1) {
      drag_state.selected_bezier_edge->edge.geometry.bezier.control2.x = next_x;
      drag_state.selected_bezier_edge->edge.geometry.bezier.control2.y = next_y;
    }
  }

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

  edge_menu_close();
  drag_state.selected_point = drawing_area_find_point_under_cursor(state, x, y);
  if (drag_state.selected_point != NULL) {
    drag_state.dragging = TRUE;
    return;
  }

  drag_state.selected_bezier_edge =
      drawing_area_find_bezier_handle_under_cursor(
          state, x, y, &drag_state.selected_bezier_handle);
  drag_state.dragging = (drag_state.selected_bezier_edge != NULL);
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
  drag_state.selected_bezier_edge = NULL;
  drag_state.selected_bezier_handle = -1;
}
