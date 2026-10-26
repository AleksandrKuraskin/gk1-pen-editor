#ifndef POLYPEN_EDGE_MENU_CONTROLLER_H
#define POLYPEN_EDGE_MENU_CONTROLLER_H

#include <gtk/gtk.h>

#include "model/app_state.h"

void edge_menu_controller_connect(GtkWidget *drawing_area, AppState *state);
void edge_menu_controller_dismiss(void);

#endif