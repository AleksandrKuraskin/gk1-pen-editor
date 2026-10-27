#include "controller/edge_menu_controller.h"

#include <math.h>

#include "config/config.h"
#include "controller/point_menu_controller.h"

typedef struct {
  GtkWidget *drawing_area;
  GtkWidget *popover;
  GtkWidget *length_spin;
  AppState *state;
  EdgeNode *edge;
  double insert_x;
  double insert_y;
} EdgeMenuState;

static EdgeMenuState edge_menu_state;

EdgeNode *edge_menu_find_edge_near_cursor(AppState *state, double x, double y,
                                          double *closest_x,
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
      .x = edge->edge.start->x + dx * 0.33 + normal_x * offset,
      .y = edge->edge.start->y + dy * 0.33 + normal_y * offset,
      .prev_edge = NULL,
      .next_edge = NULL,
      .continuity = CONTINUITY_G1,
      .continuity_set_by_user = FALSE,
  };
  Point control2 = {
      .x = edge->edge.end->x - dx * 0.33 + normal_x * offset,
      .y = edge->edge.end->y - dy * 0.33 + normal_y * offset,
      .prev_edge = NULL,
      .next_edge = NULL,
      .continuity = CONTINUITY_G1,
      .continuity_set_by_user = FALSE,
  };

  edge_set_bezier(&edge->edge, control1, control2);
}

static void edge_menu_set_arc_defaults(EdgeNode *edge) {
  if (edge == NULL || edge->edge.start == NULL || edge->edge.end == NULL) {
    return;
  }

  double dx = edge->edge.end->x - edge->edge.start->x;
  double dy = edge->edge.end->y - edge->edge.start->y;
  if (dx == 0.0 && dy == 0.0) {
    edge_set_arc(&edge->edge, ARC_SIDE_LEFT);
    return;
  }

  double length = hypot(dx, dy);
  double normal_x = -dy / length;
  double normal_y = dx / length;
  if (normal_x == 0.0 && normal_y == 0.0) {
    edge_set_arc(&edge->edge, ARC_SIDE_LEFT);
    return;
  }

  ArcSide side =
      (dx * normal_x + dy * normal_y) >= 0.0 ? ARC_SIDE_LEFT : ARC_SIDE_RIGHT;
  edge_set_arc(&edge->edge, side);
}

static void edge_menu_set_type(GtkButton *button, gpointer user_data) {
  (void)button;
  if (edge_menu_state.edge != NULL) {
    EdgeType type = (EdgeType)GPOINTER_TO_INT(user_data);
    if (type == EDGE_TYPE_BEZIER) {
      edge_menu_set_bezier_defaults(edge_menu_state.edge);
      if (!edge_menu_state.edge->edge.start->continuity_set_by_user) {
        edge_menu_state.edge->edge.start->continuity = CONTINUITY_G1;
      }
      if (!edge_menu_state.edge->edge.end->continuity_set_by_user) {
        edge_menu_state.edge->edge.end->continuity = CONTINUITY_G1;
      }
      point_enforce_continuity(edge_menu_state.edge->edge.start, NULL);
      point_enforce_continuity(edge_menu_state.edge->edge.end, NULL);
    } else if (type == EDGE_TYPE_ARC) {
      edge_menu_set_arc_defaults(edge_menu_state.edge);
    } else {
      edge_menu_state.edge->edge.type = type;
    }
    gtk_widget_queue_draw(edge_menu_state.drawing_area);
  }
  edge_menu_close();
}

static void edge_menu_apply_length_cb(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  if (edge_menu_state.edge != NULL && edge_menu_state.length_spin != NULL) {
    double target_len =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(edge_menu_state.length_spin));
    edge_set_length_constraint(&edge_menu_state.edge->edge, target_len);
    edge_list_solve_constraints(edge_menu_state.state->edges,
                                edge_menu_state.edge->edge.start);
    gtk_widget_queue_draw(edge_menu_state.drawing_area);
  }
  edge_menu_close();
}

static void edge_menu_remove_length_cb(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  if (edge_menu_state.edge != NULL) {
    edge_remove_length_constraint(&edge_menu_state.edge->edge);
    gtk_widget_queue_draw(edge_menu_state.drawing_area);
  }
  edge_menu_close();
}

static void edge_menu_pair_parallel_cb(GtkButton *button, gpointer user_data) {
  (void)button;
  EdgeNode *target = (EdgeNode *)user_data;
  if (edge_menu_state.edge != NULL && target != NULL &&
      edge_menu_state.state != NULL) {
    edge_set_parallel_constraint(
        edge_menu_state.edge, target,
        edge_menu_state.state->next_parallel_group_id++);
    edge_list_solve_constraints(edge_menu_state.state->edges,
                                edge_menu_state.edge->edge.start);
    gtk_widget_queue_draw(edge_menu_state.drawing_area);
  }
  edge_menu_close();
}

static void edge_menu_remove_parallel_cb(GtkButton *button,
                                         gpointer user_data) {
  (void)button;
  (void)user_data;
  if (edge_menu_state.edge != NULL) {
    edge_remove_parallel_constraint(edge_menu_state.edge);
    gtk_widget_queue_draw(edge_menu_state.drawing_area);
  }
  edge_menu_close();
}

static void edge_menu_insert_point(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  if (edge_menu_state.edge != NULL && edge_menu_state.state != NULL) {
    app_state_insert_point(edge_menu_state.state, edge_menu_state.edge,
                           edge_menu_state.insert_x, edge_menu_state.insert_y);
    gtk_widget_queue_draw(edge_menu_state.drawing_area);
  }
  edge_menu_close();
}

static void edge_menu_secondary_click_cb(GtkGestureClick *gesture, int n_press,
                                         double x, double y,
                                         gpointer user_data) {
  (void)n_press;
  (void)gesture;
  AppState *state = user_data;
  if (state == NULL || edge_menu_state.popover == NULL) {
    return;
  }

  if (point_menu_find_point_near_cursor(state, x, y) != NULL) {
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

  gtk_widget_set_visible(edge_menu_state.popover, FALSE);
  gtk_popover_popdown(GTK_POPOVER(edge_menu_state.popover));
  edge_menu_state.state = state;
  edge_menu_state.edge = edge;
  edge_menu_state.insert_x = closest_x;
  edge_menu_state.insert_y = closest_y;

  if (edge->edge.start != NULL && edge->edge.end != NULL &&
      edge_menu_state.length_spin != NULL) {
    double current_len = hypot(edge->edge.end->x - edge->edge.start->x,
                               edge->edge.end->y - edge->edge.start->y);
    if (edge->edge.constraint_mask & EDGE_CONSTRAINT_LENGTH) {
      current_len = edge->edge.length.length;
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(edge_menu_state.length_spin),
                              current_len);
  }

  GdkRectangle pointing_to = {(int)lround(x), (int)lround(y), 1, 1};
  gtk_popover_set_pointing_to(GTK_POPOVER(edge_menu_state.popover),
                              &pointing_to);
  gtk_popover_popup(GTK_POPOVER(edge_menu_state.popover));
}

static void edge_menu_create(GtkWidget *drawing_area) {
  edge_menu_state.drawing_area = drawing_area;
  edge_menu_state.popover = gtk_popover_new();
  if (edge_menu_state.popover == NULL) {
    return;
  }

  gtk_widget_set_parent(edge_menu_state.popover, drawing_area);
  gtk_popover_set_autohide(GTK_POPOVER(edge_menu_state.popover), TRUE);
  g_signal_connect(drawing_area, "destroy", G_CALLBACK(edge_menu_destroy),
                   NULL);

  GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  GtkWidget *section_label = gtk_label_new("Edge type:");
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

  section_label = gtk_label_new("Length constraint:");
  gtk_widget_set_halign(section_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(menu_box), section_label);

  GtkWidget *spin_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  edge_menu_state.length_spin =
      gtk_spin_button_new_with_range(10.0, 2000.0, 5.0);
  gtk_widget_set_hexpand(edge_menu_state.length_spin, TRUE);
  gtk_box_append(GTK_BOX(spin_box), edge_menu_state.length_spin);

  GtkWidget *apply_len_btn = gtk_button_new_with_label("Set");
  g_signal_connect(apply_len_btn, "clicked",
                   G_CALLBACK(edge_menu_apply_length_cb), NULL);
  gtk_box_append(GTK_BOX(spin_box), apply_len_btn);
  gtk_box_append(GTK_BOX(menu_box), spin_box);

  GtkWidget *remove_len_btn =
      gtk_button_new_with_label("Remove length constraint");
  g_signal_connect(remove_len_btn, "clicked",
                   G_CALLBACK(edge_menu_remove_length_cb), NULL);
  gtk_box_append(GTK_BOX(menu_box), remove_len_btn);

  gtk_box_append(GTK_BOX(menu_box),
                 gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  section_label = gtk_label_new("Parallel constraint:");
  gtk_widget_set_halign(section_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(menu_box), section_label);

  GtkWidget *parallel_btn = gtk_menu_button_new();
  gtk_menu_button_set_label(GTK_MENU_BUTTON(parallel_btn), "Pair with edge...");
  GtkWidget *parallel_pop = gtk_popover_new();
  GtkWidget *parallel_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

  if (edge_menu_state.state != NULL && edge_menu_state.state->edges != NULL) {
    int idx = 1;
    for (EdgeNode *node = edge_menu_state.state->edges->head; node != NULL;
         node = node->next) {
      char btn_text[64];
      snprintf(btn_text, sizeof(btn_text), "Edge #%d", idx++);
      GtkWidget *p_item = gtk_button_new_with_label(btn_text);
      g_signal_connect(p_item, "clicked",
                       G_CALLBACK(edge_menu_pair_parallel_cb), node);
      gtk_box_append(GTK_BOX(parallel_list_box), p_item);
    }
  }
  gtk_popover_set_child(GTK_POPOVER(parallel_pop), parallel_list_box);
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(parallel_btn), parallel_pop);
  gtk_box_append(GTK_BOX(menu_box), parallel_btn);

  GtkWidget *remove_par_btn =
      gtk_button_new_with_label("Remove parallel constraint");
  g_signal_connect(remove_par_btn, "clicked",
                   G_CALLBACK(edge_menu_remove_parallel_cb), NULL);
  gtk_box_append(GTK_BOX(menu_box), remove_par_btn);

  gtk_box_append(GTK_BOX(menu_box),
                 gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  GtkWidget *insert_button = gtk_button_new_with_label("Insert point");
  gtk_widget_set_halign(insert_button, GTK_ALIGN_FILL);
  gtk_box_append(GTK_BOX(menu_box), insert_button);
  g_signal_connect(insert_button, "clicked", G_CALLBACK(edge_menu_insert_point),
                   NULL);

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