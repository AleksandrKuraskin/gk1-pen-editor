#include "view/rasterizer.h"
#include "config/config.h"
#include "model/edge.h"

#include <math.h>
#include <stdlib.h>

void rasterizer_draw_line_bresenham(PixelBuffer *buffer, double start_x,
                                    double start_y, double end_x, double end_y,
                                    uint32_t color) {
  if (!buffer || !buffer->pixels || buffer->width <= 0 || buffer->height <= 0)
    return;
  if (!isfinite(start_x) || !isfinite(start_y) || !isfinite(end_x) ||
      !isfinite(end_y))
    return;

  int x0 = (int)lround(start_x), y0 = (int)lround(start_y);
  int x1 = (int)lround(end_x), y1 = (int)lround(end_y);
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;

  while (1) {
    if (x0 >= 0 && x0 < buffer->width && y0 >= 0 && y0 < buffer->height) {
      buffer->pixels[(size_t)y0 * (size_t)buffer->width + (size_t)x0] = color;
    }
    if (x0 == x1 && y0 == y1)
      break;
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

static int bezier_segment_count(Point p0, Point p1, Point p2, Point p3) {
  double len = hypot(p1.x - p0.x, p1.y - p0.y) +
               hypot(p2.x - p1.x, p2.y - p1.y) +
               hypot(p3.x - p2.x, p3.y - p2.y);
  if (len <= 0.0)
    return CONFIG_BEZIER_MIN_SEGMENTS;
  int segs = (int)ceil(len / CONFIG_BEZIER_SEGMENT_LENGTH);
  return fmax(CONFIG_BEZIER_MIN_SEGMENTS,
              fmin(CONFIG_BEZIER_MAX_SEGMENTS, segs));
}

void rasterizer_draw_bezier_forward_diff(PixelBuffer *buffer, cairo_t *cr,
                                         Point p0, Point p1, Point p2, Point p3,
                                         uint32_t color, gboolean use_bresenham,
                                         int scale) {
  int segments = bezier_segment_count(p0, p1, p2, p3);
  if (segments <= 0)
    return;

  double h = 1.0 / (double)segments;
  double h2 = h * h, h3 = h2 * h;

  double ax = -p0.x + 3.0 * p1.x - 3.0 * p2.x + p3.x;
  double ay = -p0.y + 3.0 * p1.y - 3.0 * p2.y + p3.y;
  double bx = 3.0 * p0.x - 6.0 * p1.x + 3.0 * p2.x;
  double by = 3.0 * p0.y - 6.0 * p1.y + 3.0 * p2.y;
  double cx = -3.0 * p0.x + 3.0 * p1.x;
  double cy = -3.0 * p0.y + 3.0 * p1.y;

  double x = p0.x, y = p0.y;
  double delta_x = ax * h3 + bx * h2 + cx * h;
  double delta_y = ay * h3 + by * h2 + cy * h;
  double delta2_x = 6.0 * ax * h3 + 2.0 * bx * h2;
  double delta2_y = 6.0 * ay * h3 + 2.0 * by * h2;
  double delta3_x = 6.0 * ax * h3, delta3_y = 6.0 * ay * h3;
  double prev_x = x, prev_y = y;

  if (!use_bresenham) {
    cairo_save(cr);
    cairo_set_source_rgba(cr, ((color >> 16) & 0xFF) / 255.0,
                          ((color >> 8) & 0xFF) / 255.0, (color & 0xFF) / 255.0,
                          ((color >> 24) & 0xFF) / 255.0);
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
      rasterizer_draw_line_bresenham(buffer, prev_x * scale, prev_y * scale,
                                     x * scale, y * scale, color);
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

void rasterizer_draw_arc(PixelBuffer *buffer, cairo_t *cr, EdgeNode *node,
                         uint32_t color, gboolean use_bresenham, int scale) {
  if (!node || !edge_update_arc_geometry(node))
    return;
  ArcGeometry *arc = &node->edge.geometry.arc;
  double a1 =
      atan2(node->edge.start->y - arc->cy, node->edge.start->x - arc->cx);
  double a2 = atan2(node->edge.end->y - arc->cy, node->edge.end->x - arc->cx);
  gboolean ccw = (arc->side == ARC_SIDE_LEFT);

  if (use_bresenham && buffer != NULL) {
    double span = ccw ? a2 - a1 : a1 - a2;
    while (span <= 0)
      span += 2 * M_PI;
    int segs = (int)ceil(arc->radius * span / CONFIG_BEZIER_SEGMENT_LENGTH);
    segs = fmax(12, fmin(300, segs));
    double dt = (ccw ? 1.0 : -1.0) * (span / segs);
    double px = node->edge.start->x, py = node->edge.start->y;

    for (int i = 1; i <= segs; i++) {
      double t = a1 + i * dt;
      double nx = arc->cx + arc->radius * cos(t);
      double ny = arc->cy + arc->radius * sin(t);
      rasterizer_draw_line_bresenham(buffer, px * scale, py * scale, nx * scale,
                                     ny * scale, color);
      px = nx;
      py = ny;
    }
  } else if (cr != NULL) {
    cairo_save(cr);
    cairo_set_source_rgba(cr, ((color >> 16) & 0xFF) / 255.0,
                          ((color >> 8) & 0xFF) / 255.0, (color & 0xFF) / 255.0,
                          ((color >> 24) & 0xFF) / 255.0);
    cairo_set_line_width(cr, CONFIG_EDGE_WIDTH);
    if (ccw)
      cairo_arc(cr, arc->cx, arc->cy, arc->radius, a1, a2);
    else
      cairo_arc_negative(cr, arc->cx, arc->cy, arc->radius, a1, a2);
    cairo_stroke(cr);
    cairo_restore(cr);
  }
}