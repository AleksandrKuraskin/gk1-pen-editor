#ifndef POLYPEN_OVERLAY_H
#define POLYPEN_OVERLAY_H

#include <cairo.h>
#include "model/app_state.h"

void overlay_set_source_argb(cairo_t *cr, uint32_t color);
void overlay_draw_background(cairo_t *cr, int width, int height);
void overlay_draw_edge_constraints_labels(cairo_t *cr, const Edge *edge);
void overlay_draw_point_continuity_label(cairo_t *cr, const Point *p);
void overlay_draw_points(cairo_t *cr, const PointList *points, const Point *hovered_point);
void overlay_draw_edge_hover(cairo_t *cr, const EdgeNode *hovered_edge);
void overlay_draw_pairing_edge(cairo_t *cr, const EdgeNode *pairing_edge);

#endif