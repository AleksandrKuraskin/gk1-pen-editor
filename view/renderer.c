#include "view/renderer.h"

#include "config/config.h"
#include <cairo.h>
#include <math.h>
#include <stdlib.h>

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

void renderer_draw_line_bresenham(PixelBuffer *buffer, double start_x,
                                  double start_y, double end_x, double end_y,
                                  uint32_t color) {
  if (buffer == NULL || buffer->pixels == NULL || buffer->width <= 0 ||
      buffer->height <= 0) {
    return;
  }

  int x0 = (int)lround(start_x);
  int y0 = (int)lround(start_y);
  int x1 = (int)lround(end_x);
  int y1 = (int)lround(end_y);
  int dx = abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;

  while (1) {
    if (x0 >= 0 && x0 < buffer->width && y0 >= 0 && y0 < buffer->height) {
      buffer->pixels[(size_t)y0 * (size_t)buffer->width + (size_t)x0] = color;
    }

    if (x0 == x1 && y0 == y1) {
      break;
    }

    int doubled_error = 2 * error;
    if (doubled_error >= dy) {
      error += dy;
      x0 += sx;
    }
    if (doubled_error <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}

void renderer_draw_pixel_buffer(GtkDrawingArea *area, cairo_t *cr, int width,
                                int height, gpointer user_data) {
  (void)area;

  AppState *state = user_data;
  if (state == NULL) {
    return;
  }

  renderer_draw_background(cr, width, height);

  PixelBuffer *buffer = pixel_buffer_create(width, height);
  if (buffer != NULL) {
    if (state->edges != NULL) {
      for (EdgeNode *node = state->edges->head; node != NULL;
           node = node->next) {
        if (node->edge.start == NULL || node->edge.end == NULL) {
          continue;
        }

        renderer_draw_line_bresenham(buffer, node->edge.start->x,
                                     node->edge.start->y, node->edge.end->x,
                                     node->edge.end->y, CONFIG_DEMO_GREEN);
      }
    }

    cairo_surface_t *surface = cairo_image_surface_create_for_data(
        (unsigned char *)buffer->pixels, CAIRO_FORMAT_ARGB32, width, height,
        width * (int)sizeof(uint32_t));
    if (cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
      cairo_save(cr);
      cairo_set_source_surface(cr, surface, 0, 0);
      cairo_paint(cr);
      cairo_restore(cr);
    }
    cairo_surface_destroy(surface);
    pixel_buffer_destroy(buffer);
  }

  if (state->points == NULL) {
    return;
  }

  renderer_set_source_argb(cr, CONFIG_DEMO_RED);
  for (PointNode *node = state->points->head; node != NULL; node = node->next) {
    if (node->point == NULL) {
      continue;
    }

    cairo_arc(cr, node->point->x, node->point->y, CONFIG_POINT_RADIUS, 0,
              2 * G_PI);
    cairo_fill(cr);
  }
}