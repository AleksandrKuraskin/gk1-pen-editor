#include "controller/edge_menu_controller.h"

#include <math.h>

#include "config/config.h"

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

static EdgeNode *edge_menu_find_edge_near_cursor(AppState *state, double x,
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

void edge_menu_controller_dismiss(void) { edge_menu_close(); }

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
    gtk_popover_popdown(constraint_popover);
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

static void edge_menu_secondary_click_cb(GtkGestureClick *gesture, int n_press,
                                         double x, double y,
                                         gpointer user_data) {
  (void)n_press;
  AppState *state = user_data;
  if (state == NULL || edge_menu_state.popover == NULL) {
    return;
  }

  double closest_x = x;
  double closest_y = y;
  EdgeNode *edge =
      edge_menu_find_edge_near_cursor(state, x, y, &closest_x, &closest_y);
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

static void edge_menu_create(GtkWidget *drawing_area) {
  edge_menu_state.drawing_area = drawing_area;
  edge_menu_state.popover = gtk_popover_new();
  if (edge_menu_state.popover == NULL) {
    return;
  }

  gtk_popover_set_autohide(GTK_POPOVER(edge_menu_state.popover), TRUE);
  gtk_widget_set_parent(edge_menu_state.popover, drawing_area);
  g_signal_connect(drawing_area, "destroy", G_CALLBACK(edge_menu_destroy),
                   NULL);

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

void edge_menu_controller_connect(GtkWidget *drawing_area, AppState *state) {
  if (drawing_area == NULL || state == NULL) {
    return;
  }

  edge_menu_state.state = state;
  edge_menu_create(drawing_area);

  GtkGesture *secondary_click = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(secondary_click),
                                GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(drawing_area,
                            GTK_EVENT_CONTROLLER(secondary_click));
  g_signal_connect(secondary_click, "pressed",
                   G_CALLBACK(edge_menu_secondary_click_cb), state);
}