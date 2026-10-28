#ifndef POLYPEN_RASTERIZER_H
#define POLYPEN_RASTERIZER_H

#include <stdint.h>
#include <cairo.h>
#include "model/edge.h"
#include "model/point.h"
#include "view/pixel_buffer.h"

void rasterizer_draw_line_bresenham(PixelBuffer *buffer, double start_x,
                                    double start_y, double end_x, double end_y,
                                    uint32_t color);

void rasterizer_draw_bezier_forward_diff(PixelBuffer *buffer, cairo_t *cr,
                                         Point p0, Point p1, Point p2, Point p3,
                                         uint32_t color, gboolean use_bresenham,
                                         int scale);

void rasterizer_draw_arc(PixelBuffer *buffer, cairo_t *cr, EdgeNode *node,
                         uint32_t color, gboolean use_bresenham, int scale);

#endif