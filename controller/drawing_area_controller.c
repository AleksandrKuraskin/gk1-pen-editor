#include "controller/drawing_area_controller.h"

#include <math.h>

#include "config/config.h"
#include "controller/edge_menu_controller.h"
#include "controller/point_menu_controller.h"

static DragState drag_state;
static double last_drag_x = 0.0;
static double last_drag_y = 0.0;

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

static void translate_connected_component(AppState *state, Point *start_point,
                                          double dx, double dy) {
  if (state == NULL || start_point == NULL) {
    return;
  }

  GHashTable *visited_points = g_hash_table_new(g_direct_hash, g_direct_equal);
  GHashTable *visited_edges = g_hash_table_new(g_direct_hash, g_direct_equal);
  GQueue *queue = g_queue_new();

  g_hash_table_add(visited_points, start_point);
  g_queue_push_tail(queue, start_point);

  while (!g_queue_is_empty(queue)) {
    Point *p = g_queue_pop_head(queue);
    p->x += dx;
    p->y += dy;

    EdgeNode *edges[2] = {p->prev_edge, p->next_edge};
    for (int i = 0; i < 2; i++) {
      EdgeNode *en = edges[i];
      if (en == NULL)
        continue;

      if (!g_hash_table_contains(visited_edges, en)) {
        g_hash_table_add(visited_edges, en);
        if (en->edge.type == EDGE_TYPE_BEZIER) {
          en->edge.geometry.bezier.control1.x += dx;
          en->edge.geometry.bezier.control1.y += dy;
          en->edge.geometry.bezier.control2.x += dx;
          en->edge.geometry.bezier.control2.y += dy;
        }
      }

      Point *neighbor = (en->edge.start == p) ? en->edge.end : en->edge.start;
      if (neighbor != NULL &&
          !g_hash_table_contains(visited_points, neighbor)) {
        g_hash_table_add(visited_points, neighbor);
        g_queue_push_tail(queue, neighbor);
      }
    }
  }

  g_queue_free(queue);
  g_hash_table_destroy(visited_points);
  g_hash_table_destroy(visited_edges);
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
    if (hypot(dx, dy) <= CONFIG_POINT_RADIUS + CONFIG_HOVER_OUTLINE_WIDTH) {
      return point;
    }
  }

  return NULL;
}

static EdgeNode *drawing_area_find_edge_under_cursor(AppState *state, double x,
                                                     double y) {
  if (state == NULL || state->edges == NULL) {
    return NULL;
  }

  double closest_x = x;
  double closest_y = y;
  EdgeNode *edge =
      edge_menu_find_edge_near_cursor(state, x, y, &closest_x, &closest_y);
  if (edge == NULL) {
    return NULL;
  }

  double distance = hypot(x - closest_x, y - closest_y);
  if (distance <= CONFIG_PICK_TOLERANCE + CONFIG_HOVER_OUTLINE_WIDTH) {
    return edge;
  }

  return NULL;
}

void drawing_area_update_hover_state(AppState *state, double x, double y) {
  if (state == NULL) {
    return;
  }

  state->hover_x = x;
  state->hover_y = y;
  state->hovered_point = drawing_area_find_point_under_cursor(state, x, y);
  if (state->hovered_point != NULL) {
    state->hovered_edge = NULL;
    return;
  }

  state->hovered_edge = drawing_area_find_edge_under_cursor(state, x, y);
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
  drag_state.selected_edge = NULL;
  drag_state.dragging = FALSE;
  edge_menu_controller_connect(drawing_area, state);
  point_menu_controller_connect(drawing_area, state);

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
  (void)user_data;
  GtkWidget *widget =
      gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));

  if (!drag_state.dragging) {
    if (drag_state.state != NULL) {
      drawing_area_update_hover_state(drag_state.state, x, y);
    }
    if (widget != NULL)
      gtk_widget_queue_draw(widget);
    return;
  }

  double next_x = x;
  double next_y = y;
  drawing_area_clamp_to_canvas(widget, &next_x, &next_y);

  if (drag_state.selected_point != NULL) {
    double dx = next_x - drag_state.selected_point->x;
    double dy = next_y - drag_state.selected_point->y;
    drag_state.selected_point->x = next_x;
    drag_state.selected_point->y = next_y;

    if (drag_state.selected_point->prev_edge &&
        drag_state.selected_point->prev_edge->edge.type == EDGE_TYPE_BEZIER) {
      Edge *e = &drag_state.selected_point->prev_edge->edge;
      if (e->end == drag_state.selected_point) {
        e->geometry.bezier.control2.x += dx;
        e->geometry.bezier.control2.y += dy;
      }
    }
    if (drag_state.selected_point->next_edge &&
        drag_state.selected_point->next_edge->edge.type == EDGE_TYPE_BEZIER) {
      Edge *e = &drag_state.selected_point->next_edge->edge;
      if (e->start == drag_state.selected_point) {
        e->geometry.bezier.control1.x += dx;
        e->geometry.bezier.control1.y += dy;
      }
    }

    edge_list_solve_constraints(drag_state.state->edges,
                                drag_state.selected_point);

    if (drag_state.state != NULL && drag_state.state->points != NULL) {
      for (PointNode *node = drag_state.state->points->head; node != NULL;
           node = node->next) {
        if (node->point != NULL && node->point->continuity != CONTINUITY_G0) {
          point_enforce_continuity(node->point, NULL);
        }
      }
    }
  } else if (drag_state.selected_bezier_edge != NULL &&
             drag_state.selected_bezier_edge->edge.type == EDGE_TYPE_BEZIER) {
    if (drag_state.selected_bezier_handle == 0) {
      drag_state.selected_bezier_edge->edge.geometry.bezier.control1.x = next_x;
      drag_state.selected_bezier_edge->edge.geometry.bezier.control1.y = next_y;
    } else if (drag_state.selected_bezier_handle == 1) {
      drag_state.selected_bezier_edge->edge.geometry.bezier.control2.x = next_x;
      drag_state.selected_bezier_edge->edge.geometry.bezier.control2.y = next_y;
    }

    Point *v = (drag_state.selected_bezier_handle == 0)
                   ? drag_state.selected_bezier_edge->edge.start
                   : drag_state.selected_bezier_edge->edge.end;
    point_enforce_continuity(v, drag_state.selected_bezier_edge);
  } else if (drag_state.selected_edge != NULL &&
             drag_state.selected_edge->edge.start != NULL) {
    double dx = next_x - last_drag_x;
    double dy = next_y - last_drag_y;
    translate_connected_component(drag_state.state,
                                  drag_state.selected_edge->edge.start, dx, dy);
    edge_list_solve_constraints(drag_state.state->edges, NULL);
  }

  last_drag_x = next_x;
  last_drag_y = next_y;

  if (widget != NULL)
    gtk_widget_queue_draw(widget);
}

void drawing_area_click_cb(GtkGestureClick *gesture, int n_press, double x,
                           double y, gpointer user_data) {
  (void)n_press;
  (void)gesture;

  AppState *state = user_data;
  if (state == NULL) {
    return;
  }

  edge_menu_controller_dismiss();
  point_menu_controller_dismiss();
  state->hovered_point = NULL;
  state->hovered_edge = NULL;

  last_drag_x = x;
  last_drag_y = y;

  drag_state.selected_point = drawing_area_find_point_under_cursor(state, x, y);
  if (drag_state.selected_point != NULL) {
    drag_state.dragging = TRUE;
    return;
  }

  drag_state.selected_bezier_edge =
      drawing_area_find_bezier_handle_under_cursor(
          state, x, y, &drag_state.selected_bezier_handle);
  if (drag_state.selected_bezier_edge != NULL) {
    drag_state.dragging = TRUE;
    return;
  }

  drag_state.selected_edge = drawing_area_find_edge_under_cursor(state, x, y);
  if (drag_state.selected_edge != NULL) {
    drag_state.dragging = TRUE;
    return;
  }

  drag_state.dragging = FALSE;
}

void drawing_area_release_cb(GtkGestureClick *gesture, int n_press, double x,
                             double y, gpointer user_data) {
  (void)gesture;
  (void)n_press;

  AppState *state = user_data;
  if (state != NULL) {
    drawing_area_update_hover_state(state, x, y);
  }

  drag_state.dragging = FALSE;
  drag_state.selected_point = NULL;
  drag_state.selected_bezier_edge = NULL;
  drag_state.selected_bezier_handle = -1;
  drag_state.selected_edge = NULL;
}