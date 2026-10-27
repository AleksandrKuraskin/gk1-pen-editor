#ifndef POLYPEN_POINT_MENU_CONTROLLER_H
#define POLYPEN_POINT_MENU_CONTROLLER_H

#include <gtk/gtk.h>

#include "model/app_state.h"

Point *point_menu_find_point_near_cursor(AppState *state, double x, double y);
void point_menu_controller_connect(GtkWidget *drawing_area, AppState *state);
void point_menu_controller_dismiss(void);

#endif
