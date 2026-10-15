#ifndef POLYPEN_RENDERER_H
#define POLYPEN_RENDERER_H

#include <gtk/gtk.h>

void renderer_draw_pixel_buffer(GtkDrawingArea *area, cairo_t *cr, int width,
                                int height, gpointer user_data);

#endif