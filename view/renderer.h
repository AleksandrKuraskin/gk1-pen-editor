#ifndef POLYPEN_RENDERER_H
#define POLYPEN_RENDERER_H

#include <gtk/gtk.h>
#include <stdint.h>

#include "model/app_state.h"
#include "view/pixel_buffer.h"

void renderer_draw_line_bresenham(PixelBuffer *buffer, double x0, double y0,
                                  double x1, double y1, uint32_t color);
void renderer_draw_pixel_buffer(GtkDrawingArea *area, cairo_t *cr, int width,
                                int height, gpointer user_data);

#endif