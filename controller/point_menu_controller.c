#include "controller/point_menu_controller.h"

#include <math.h>

#include "config/config.h"

typedef struct {
  GtkWidget *drawing_area;
  GtkWidget *popover;
  AppState *state;
  Point *point;
} PointMenuState;

static PointMenuState point_menu_state;

static void point_menu_close(void) {
  if (point_menu_state.popover != NULL) {
    gtk_popover_popdown(GTK_POPOVER(point_menu_state.popover));
    gtk_widget_set_visible(point_menu_state.popover, FALSE);
  }
  point_menu_state.point = NULL;
}

void point_menu_controller_dismiss(void) { point_menu_close(); }

static void point_menu_destroy(void) {
  if (point_menu_state.popover == NULL) {
    return;
  }

  gtk_popover_popdown(GTK_POPOVER(point_menu_state.popover));
  if (gtk_widget_get_parent(point_menu_state.popover) != NULL) {
    gtk_widget_unparent(point_menu_state.popover);
  }
  point_menu_state.popover = NULL;
}

static void point_menu_remove_cb(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  if (point_menu_state.state == NULL || point_menu_state.point == NULL) {
    point_menu_close();
    return;
  }

  if (!app_state_remove_point(point_menu_state.state, point_menu_state.point)) {
    point_menu_close();
    return;
  }

  gtk_widget_queue_draw(point_menu_state.drawing_area);
  point_menu_close();
}

static void point_menu_show(AppState *state, Point *point, double x, double y) {
  if (state == NULL || point == NULL || point_menu_state.popover == NULL) {
    return;
  }

  point_menu_state.state = state;
  point_menu_state.point = point;
  GdkRectangle pointing_to = {(int)lround(x), (int)lround(y), 1, 1};
  gtk_popover_set_pointing_to(GTK_POPOVER(point_menu_state.popover),
                              &pointing_to);
  gtk_popover_popup(GTK_POPOVER(point_menu_state.popover));
}

Point *point_menu_find_point_near_cursor(AppState *state, double x, double y) {
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

static void point_menu_secondary_click_cb(GtkGestureClick *gesture, int n_press,
                                          double x, double y,
                                          gpointer user_data) {
  (void)n_press;
  AppState *state = user_data;
  if (state == NULL || point_menu_state.popover == NULL) {
    return;
  }

  Point *point = point_menu_find_point_near_cursor(state, x, y);
  if (point == NULL) {
    point_menu_close();
    return;
  }

  gtk_widget_set_visible(point_menu_state.popover, FALSE);
  gtk_popover_popdown(GTK_POPOVER(point_menu_state.popover));
  point_menu_show(state, point, x, y);
  (void)gesture;
}

static void point_menu_create(GtkWidget *drawing_area) {
  point_menu_state.drawing_area = drawing_area;
  point_menu_state.popover = gtk_popover_new();
  if (point_menu_state.popover == NULL) {
    return;
  }

  gtk_widget_set_parent(point_menu_state.popover, drawing_area);

  gtk_popover_set_autohide(GTK_POPOVER(point_menu_state.popover), TRUE);
  g_signal_connect(drawing_area, "destroy", G_CALLBACK(point_menu_destroy),
                   NULL);

  GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  GtkWidget *section_label = gtk_label_new("Point");
  gtk_widget_set_halign(section_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(menu_box), section_label);

  GtkWidget *remove_button = gtk_button_new_with_label("Remove point");
  gtk_widget_set_halign(remove_button, GTK_ALIGN_FILL);
  gtk_box_append(GTK_BOX(menu_box), remove_button);
  g_signal_connect(remove_button, "clicked", G_CALLBACK(point_menu_remove_cb),
                   NULL);

  gtk_box_append(GTK_BOX(menu_box),
                 gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
  GtkWidget *continuity_button =
      gtk_button_new_with_label("Set continuity type (future)");
  gtk_widget_set_halign(continuity_button, GTK_ALIGN_FILL);
  gtk_widget_set_sensitive(continuity_button, FALSE);
  gtk_box_append(GTK_BOX(menu_box), continuity_button);

  gtk_popover_set_child(GTK_POPOVER(point_menu_state.popover), menu_box);
}

void point_menu_controller_connect(GtkWidget *drawing_area, AppState *state) {
  if (drawing_area == NULL || state == NULL) {
    return;
  }

  point_menu_state.state = state;
  point_menu_create(drawing_area);

  GtkGesture *secondary_click = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(secondary_click),
                                GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(drawing_area,
                            GTK_EVENT_CONTROLLER(secondary_click));
  g_signal_connect(secondary_click, "pressed",
                   G_CALLBACK(point_menu_secondary_click_cb), state);
}
