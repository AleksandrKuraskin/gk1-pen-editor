#ifndef POLYPEN_EDGE_MENU_CONTROLLER_H
#define POLYPEN_EDGE_MENU_CONTROLLER_H

#include <gtk/gtk.h>

#include "model/app_state.h"

EdgeNode *edge_menu_find_edge_near_cursor(AppState *state, double x, double y,
                                          double *closest_x,
                                          double *closest_y);
void edge_menu_controller_connect(GtkWidget *drawing_area, AppState *state);
void edge_menu_controller_dismiss(void);

#endif