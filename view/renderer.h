#ifndef POLYPEN_RENDERER_H
#define POLYPEN_RENDERER_H

#include <gtk/gtk.h>
#include <stdint.h>

#include "model/app_state.h"
#include "view/pixel_buffer.h"

typedef struct RenderState {
  void (*render)(struct RenderState *self, cairo_t *cr, int width, int height, AppState *state);
} RenderState;

void renderer_draw_scene_scaled(cairo_t *cr, PixelBuffer *raw_super_buffer, int scale,
                                int width, int height, AppState *state);

void renderer_draw_pixel_buffer(GtkDrawingArea *area, cairo_t *cr, int width,
                                int height, gpointer user_data);

#endif