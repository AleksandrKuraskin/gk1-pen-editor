#include "view/renderer.h"

#include "config/config.h"
#include <cairo.h>
#include <math.h>
#include <stdio.h>
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

static int renderer_bezier_segment_count(Point p0, Point p1, Point p2,
                                         Point p3) {
  double polygon_length = hypot(p1.x - p0.x, p1.y - p0.y) +
                          hypot(p2.x - p1.x, p2.y - p1.y) +
                          hypot(p3.x - p2.x, p3.y - p2.y);

  if (polygon_length <= 0.0) {
    return CONFIG_BEZIER_MIN_SEGMENTS;
  }

  int segments = (int)ceil(polygon_length / CONFIG_BEZIER_SEGMENT_LENGTH);
  if (segments < CONFIG_BEZIER_MIN_SEGMENTS) {
    segments = CONFIG_BEZIER_MIN_SEGMENTS;
  }
  if (segments > CONFIG_BEZIER_MAX_SEGMENTS) {
    segments = CONFIG_BEZIER_MAX_SEGMENTS;
  }

  return segments;
}

static void renderer_draw_bezier_incremental(PixelBuffer *buffer, cairo_t *cr,
                                             Point p0, Point p1, Point p2,
                                             Point p3, uint32_t color,
                                             gboolean use_bresenham) {
  int segments = renderer_bezier_segment_count(p0, p1, p2, p3);
  if (segments <= 0)
    return;

  double h = 1.0 / (double)segments;
  double h2 = h * h;
  double h3 = h2 * h;

  double ax = -p0.x + 3.0 * p1.x - 3.0 * p2.x + p3.x;
  double ay = -p0.y + 3.0 * p1.y - 3.0 * p2.y + p3.y;

  double bx = 3.0 * p0.x - 6.0 * p1.x + 3.0 * p2.x;
  double by = 3.0 * p0.y - 6.0 * p1.y + 3.0 * p2.y;

  double cx = -3.0 * p0.x + 3.0 * p1.x;
  double cy = -3.0 * p0.y + 3.0 * p1.y;

  double dx = p0.x;
  double dy = p0.y;

  double x = dx;
  double y = dy;

  double delta_x = ax * h3 + bx * h2 + cx * h;
  double delta_y = ay * h3 + by * h2 + cy * h;

  double delta2_x = 6.0 * ax * h3 + 2.0 * bx * h2;
  double delta2_y = 6.0 * ay * h3 + 2.0 * by * h2;

  double delta3_x = 6.0 * ax * h3;
  double delta3_y = 6.0 * ay * h3;

  double prev_x = x;
  double prev_y = y;

  if (!use_bresenham) {
    cairo_save(cr);
    renderer_set_source_argb(cr, color);
    cairo_set_line_width(cr, CONFIG_EDGE_WIDTH);
    cairo_move_to(cr, prev_x, prev_y);
  }

  for (int i = 0; i < segments; i++) {
    x += delta_x;
    y += delta_y;
    delta_x += delta2_x;
    delta_y += delta2_y;
    delta2_x += delta3_x;
    delta2_y += delta3_y;

    if (use_bresenham && buffer != NULL) {
      renderer_draw_line_bresenham(buffer, prev_x, prev_y, x, y, color);
    } else if (!use_bresenham) {
      cairo_line_to(cr, x, y);
    }
    prev_x = x;
    prev_y = y;
  }

  if (!use_bresenham) {
    cairo_stroke(cr);
    cairo_restore(cr);
  }
}

static void renderer_draw_arc(cairo_t *cr, Point start, Point end,
                              ArcSide side) {
  if (cr == NULL) {
    return;
  }

  double dx = end.x - start.x;
  double dy = end.y - start.y;
  double chord_length = hypot(dx, dy);
  if (chord_length <= 0.0) {
    return;
  }

  double mid_x = (start.x + end.x) * 0.5;
  double mid_y = (start.y + end.y) * 0.5;
  double normal_x = -dy / chord_length;
  double normal_y = dx / chord_length;

  double radius = chord_length * 0.5;
  double center_x = mid_x + normal_x * 0.0;
  double center_y = mid_y + normal_y * 0.0;

  double start_angle = atan2(start.y - center_y, start.x - center_x);
  double end_angle = atan2(end.y - center_y, end.x - center_x);

  if (side == ARC_SIDE_LEFT) {
    if (end_angle <= start_angle) {
      end_angle += 2.0 * M_PI;
    }
  } else {
    if (start_angle <= end_angle) {
      start_angle += 2.0 * M_PI;
    }
  }

  cairo_set_line_width(cr, CONFIG_EDGE_WIDTH);
  cairo_arc(cr, center_x, center_y, radius, start_angle, end_angle);
  cairo_stroke(cr);
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

static void renderer_draw_edge_constraints_labels(cairo_t *cr,
                                                  const Edge *edge) {
  if (cr == NULL || edge == NULL || edge->start == NULL || edge->end == NULL)
    return;

  double mid_x = (edge->start->x + edge->end->x) * 0.5;
  double mid_y = (edge->start->y + edge->end->y) * 0.5;

  char label[64] = "";
  if ((edge->constraint_mask & EDGE_CONSTRAINT_LENGTH) &&
      (edge->constraint_mask & EDGE_CONSTRAINT_PARALLEL)) {
    snprintf(label, sizeof(label), "%.1f | || %d", edge->length.length,
             edge->parallel.group_id);
  } else if (edge->constraint_mask & EDGE_CONSTRAINT_LENGTH) {
    snprintf(label, sizeof(label), "%.1f", edge->length.length);
  } else if (edge->constraint_mask & EDGE_CONSTRAINT_PARALLEL) {
    snprintf(label, sizeof(label), "|| %d", edge->parallel.group_id);
  } else {
    return;
  }

  cairo_set_font_size(cr, CONFIG_LENGTH_LABEL_FONT_SIZE);
  cairo_text_extents_t extents;
  cairo_text_extents(cr, label, &extents);

  renderer_set_source_argb(cr, CONFIG_LABEL_BACKGROUND_COLOR);
  cairo_rectangle(cr, mid_x - extents.width * 0.5 - CONFIG_LENGTH_LABEL_PADDING,
                  mid_y - extents.height * 0.5 - CONFIG_LENGTH_LABEL_PADDING,
                  extents.width + CONFIG_LENGTH_LABEL_PADDING * 2.0,
                  extents.height + CONFIG_LENGTH_LABEL_PADDING * 2.0);
  cairo_fill(cr);

  renderer_set_source_argb(cr, CONFIG_LABEL_TEXT_COLOR);
  cairo_move_to(cr, mid_x - extents.width * 0.5 - extents.x_bearing,
                mid_y - extents.height * 0.5 - extents.y_bearing);
  cairo_show_text(cr, label);
  cairo_new_path(cr);
}

void renderer_draw_pixel_buffer(GtkDrawingArea *area, cairo_t *cr, int width,
                                int height, gpointer user_data) {
  (void)area;

  AppState *state = user_data;
  if (state == NULL) {
    return;
  }

  renderer_draw_background(cr, width, height);

  PixelBuffer *buffer = NULL;
  if (state->use_bresenham) {
    buffer = pixel_buffer_create(width, height);
  }

  if (state->edges != NULL) {
    for (EdgeNode *node = state->edges->head; node != NULL; node = node->next) {
      if (node->edge.start == NULL || node->edge.end == NULL) {
        continue;
      }

      switch (node->edge.type) {
      case EDGE_TYPE_BEZIER:
        renderer_set_source_argb(cr, CONFIG_EDGE_ANCHOR_COLOR);
        cairo_set_line_width(cr, CONFIG_EDGE_ANCHOR_LINE_WIDTH);
        cairo_move_to(cr, node->edge.start->x, node->edge.start->y);
        cairo_line_to(cr, node->edge.end->x, node->edge.end->y);
        cairo_stroke(cr);

        renderer_set_source_argb(cr, CONFIG_EDGE_ANCHOR_COLOR);
        cairo_set_dash(cr,
                       (double[]){CONFIG_EDGE_GUIDE_DASH_LENGTH,
                                  CONFIG_EDGE_GUIDE_DASH_GAP},
                       2, 0.0);
        cairo_set_line_width(cr, CONFIG_EDGE_GUIDE_LINE_WIDTH);
        cairo_move_to(cr, node->edge.start->x, node->edge.start->y);
        cairo_line_to(cr, node->edge.geometry.bezier.control1.x,
                      node->edge.geometry.bezier.control1.y);
        cairo_line_to(cr, node->edge.geometry.bezier.control2.x,
                      node->edge.geometry.bezier.control2.y);
        cairo_line_to(cr, node->edge.end->x, node->edge.end->y);
        cairo_stroke(cr);
        cairo_set_dash(cr, NULL, 0, 0.0);

        renderer_draw_bezier_incremental(buffer, cr, *(node->edge.start),
                                         node->edge.geometry.bezier.control1,
                                         node->edge.geometry.bezier.control2,
                                         *(node->edge.end), CONFIG_EDGE_COLOR,
                                         state->use_bresenham);

        renderer_set_source_argb(cr, CONFIG_CONTROL_POINT_COLOR);
        cairo_arc(cr, node->edge.geometry.bezier.control1.x,
                  node->edge.geometry.bezier.control1.y,
                  CONFIG_CONTROL_POINT_RADIUS, 0, 2 * G_PI);
        cairo_fill(cr);
        cairo_arc(cr, node->edge.geometry.bezier.control2.x,
                  node->edge.geometry.bezier.control2.y,
                  CONFIG_CONTROL_POINT_RADIUS, 0, 2 * G_PI);
        cairo_fill(cr);
        break;

      case EDGE_TYPE_ARC:
        renderer_set_source_argb(cr, CONFIG_EDGE_ANCHOR_COLOR);
        cairo_set_line_width(cr, CONFIG_EDGE_ANCHOR_LINE_WIDTH);
        cairo_move_to(cr, node->edge.start->x, node->edge.start->y);
        cairo_line_to(cr, node->edge.end->x, node->edge.end->y);
        cairo_stroke(cr);

        renderer_set_source_argb(cr, CONFIG_EDGE_COLOR);
        renderer_draw_arc(cr, *(node->edge.start), *(node->edge.end),
                          node->edge.geometry.arc.side);
        break;

      case EDGE_TYPE_STRAIGHT:
      default:
        if (state->use_bresenham && buffer != NULL) {
          renderer_draw_line_bresenham(buffer, node->edge.start->x,
                                       node->edge.start->y, node->edge.end->x,
                                       node->edge.end->y, CONFIG_EDGE_COLOR);
        } else {
          cairo_save(cr);
          renderer_set_source_argb(cr, CONFIG_EDGE_COLOR);
          cairo_set_line_width(cr, CONFIG_EDGE_WIDTH);
          cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
          cairo_move_to(cr, node->edge.start->x, node->edge.start->y);
          cairo_line_to(cr, node->edge.end->x, node->edge.end->y);
          cairo_stroke(cr);
          cairo_restore(cr);
        }
        break;
      }
    }
  }

  if (state->use_bresenham && buffer != NULL) {
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

  if (state->pairing_edge != NULL && state->pairing_edge->edge.start != NULL &&
      state->pairing_edge->edge.end != NULL) {
    cairo_save(cr);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    renderer_set_source_argb(cr, 0xFFF59E0B);
    cairo_set_line_width(cr, CONFIG_EDGE_WIDTH + 4.0);

    Edge *p_edge = &state->pairing_edge->edge;
    cairo_move_to(cr, p_edge->start->x, p_edge->start->y);
    cairo_line_to(cr, p_edge->end->x, p_edge->end->y);
    cairo_stroke(cr);
    cairo_restore(cr);
  }

  if (state->hovered_edge != NULL && state->hovered_edge->edge.start != NULL &&
      state->hovered_edge->edge.end != NULL) {
    cairo_save(cr);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    renderer_set_source_argb(cr, CONFIG_HOVER_COLOR);
    cairo_set_line_width(cr, CONFIG_EDGE_WIDTH + CONFIG_HOVER_OUTLINE_WIDTH);

    Edge *edge = &state->hovered_edge->edge;
    cairo_move_to(cr, edge->start->x, edge->start->y);
    cairo_line_to(cr, edge->end->x, edge->end->y);
    cairo_stroke(cr);
    cairo_restore(cr);
  }

  if (state->points != NULL) {
    for (PointNode *node = state->points->head; node != NULL;
         node = node->next) {
      if (node->point == NULL) {
        continue;
      }

      cairo_arc(cr, node->point->x, node->point->y, CONFIG_POINT_RADIUS, 0,
                2 * G_PI);
      if (node->point == state->hovered_point) {
        renderer_set_source_argb(cr, CONFIG_POINT_COLOR);
        cairo_fill_preserve(cr);
        renderer_set_source_argb(cr, CONFIG_HOVER_COLOR);
        cairo_set_line_width(cr, CONFIG_HOVER_OUTLINE_WIDTH);
        cairo_stroke(cr);
      } else {
        renderer_set_source_argb(cr, CONFIG_POINT_COLOR);
        cairo_fill(cr);
      }
    }
  }

  if (state->edges != NULL) {
    for (EdgeNode *node = state->edges->head; node != NULL; node = node->next) {
      renderer_draw_edge_constraints_labels(cr, &node->edge);
    }
  }
}