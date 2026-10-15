#include "view/renderer.h"

#include "config/config.h"
#include "view/pixel_buffer.h"
#include <cairo.h>

static void renderer_set_source_argb(cairo_t *cr, uint32_t color) {
  double alpha = ((color >> 24) & 0xFF) / 255.0;
  double red = ((color >> 16) & 0xFF) / 255.0;
  double green = ((color >> 8) & 0xFF) / 255.0;
  double blue = (color & 0xFF) / 255.0;

  cairo_set_source_rgba(cr, red, green, blue, alpha);
}

static void renderer_draw_background(cairo_t *cr, int width, int height) {
  renderer_set_source_argb(cr, CONFIG_CANVAS_BACKGROUND);
  cairo_paint(cr);

  renderer_set_source_argb(cr, CONFIG_GRID_COLOR);
  for (int y = CONFIG_GRID_SPACING / 2; y < height; y += CONFIG_GRID_SPACING) {
    for (int x = CONFIG_GRID_SPACING / 2; x < width; x += CONFIG_GRID_SPACING) {
      cairo_move_to(cr, x, y);
      cairo_arc(cr, x, y, CONFIG_GRID_DOT_RADIUS, 0, 2 * G_PI);
    }
  }
  cairo_fill(cr);
}

void renderer_draw_pixel_buffer(GtkDrawingArea *area, cairo_t *cr, int width,
                                int height, gpointer user_data) {
  (void)area;

  renderer_draw_background(cr, width, height);

  PixelBuffer *pb = user_data;
  if (pb == NULL || pb->pixels == NULL) {
    return;
  }

  cairo_surface_t *surface = cairo_image_surface_create_for_data(
      (unsigned char *)pb->pixels, CAIRO_FORMAT_ARGB32, pb->width, pb->height,
      pb->width * (int)sizeof(*pb->pixels));
  cairo_set_source_surface(cr, surface, 0, 0);
  cairo_paint(cr);
  cairo_surface_destroy(surface);
}