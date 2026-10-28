#include "view/overlay.h"
#include "config/config.h"
#include <math.h>
#include <stdio.h>

void overlay_set_source_argb(cairo_t *cr, uint32_t color) {
  double a = ((color >> 24) & 0xFF) / 255.0;
  double r = ((color >> 16) & 0xFF) / 255.0;
  double g = ((color >> 8) & 0xFF) / 255.0;
  double b = (color & 0xFF) / 255.0;
  cairo_set_source_rgba(cr, r, g, b, a);
}

void overlay_draw_background(cairo_t *cr, int width, int height) {
  overlay_set_source_argb(cr, CONFIG_CANVAS_BACKGROUND);
  cairo_paint(cr);

  overlay_set_source_argb(cr, CONFIG_GRID_COLOR);
  for (int y = CONFIG_GRID_SPACING / 2; y < height; y += CONFIG_GRID_SPACING) {
    for (int x = CONFIG_GRID_SPACING / 2; x < width; x += CONFIG_GRID_SPACING) {
      cairo_move_to(cr, x, y);
      cairo_arc(cr, x, y, CONFIG_GRID_DOT_RADIUS, 0, 2 * G_PI);
    }
  }
  cairo_fill(cr);
}

void overlay_draw_edge_constraints_labels(cairo_t *cr, const Edge *edge) {
  if (!cr || !edge || !edge->start || !edge->end)
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
  cairo_text_extents_t ext;
  cairo_text_extents(cr, label, &ext);

  overlay_set_source_argb(cr, CONFIG_LABEL_BACKGROUND_COLOR);
  cairo_rectangle(cr, mid_x - ext.width * 0.5 - CONFIG_LENGTH_LABEL_PADDING,
                  mid_y - ext.height * 0.5 - CONFIG_LENGTH_LABEL_PADDING,
                  ext.width + CONFIG_LENGTH_LABEL_PADDING * 2.0,
                  ext.height + CONFIG_LENGTH_LABEL_PADDING * 2.0);
  cairo_fill(cr);

  overlay_set_source_argb(cr, CONFIG_LABEL_TEXT_COLOR);
  cairo_move_to(cr, mid_x - ext.width * 0.5 - ext.x_bearing,
                mid_y - ext.height * 0.5 - ext.y_bearing);
  cairo_show_text(cr, label);
  cairo_new_path(cr);
}

void overlay_draw_point_continuity_label(cairo_t *cr, const Point *p) {
  if (!cr || !p || p->continuity == CONTINUITY_G0)
    return;
  const char *label = (p->continuity == CONTINUITY_G1) ? "G1" : "C1";
  cairo_set_font_size(cr, 11.0);
  cairo_text_extents_t ext;
  cairo_text_extents(cr, label, &ext);

  overlay_set_source_argb(cr, 0xFF38BDF8);
  cairo_move_to(cr, p->x + 8.0 - ext.x_bearing,
                p->y - 8.0 - ext.y_bearing - ext.height);
  cairo_show_text(cr, label);
  cairo_new_path(cr);
}

void overlay_draw_points(cairo_t *cr, const PointList *points,
                         const Point *hovered_point) {
  if (!points)
    return;
  for (PointNode *n = points->head; n != NULL; n = n->next) {
    if (!n->point)
      continue;
    cairo_arc(cr, n->point->x, n->point->y, CONFIG_POINT_RADIUS, 0, 2 * G_PI);
    if (n->point == hovered_point) {
      overlay_set_source_argb(cr, CONFIG_POINT_COLOR);
      cairo_fill_preserve(cr);
      overlay_set_source_argb(cr, CONFIG_HOVER_COLOR);
      cairo_set_line_width(cr, CONFIG_HOVER_OUTLINE_WIDTH);
      cairo_stroke(cr);
    } else {
      overlay_set_source_argb(cr, CONFIG_POINT_COLOR);
      cairo_fill(cr);
    }
  }
}

void overlay_draw_edge_hover(cairo_t *cr, const EdgeNode *hovered_edge) {
  if (!hovered_edge || !hovered_edge->edge.start || !hovered_edge->edge.end)
    return;
  cairo_save(cr);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
  overlay_set_source_argb(cr, CONFIG_HOVER_COLOR);
  cairo_set_line_width(cr, CONFIG_EDGE_WIDTH + CONFIG_HOVER_OUTLINE_WIDTH);
  cairo_move_to(cr, hovered_edge->edge.start->x, hovered_edge->edge.start->y);
  cairo_line_to(cr, hovered_edge->edge.end->x, hovered_edge->edge.end->y);
  cairo_stroke(cr);
  cairo_restore(cr);
}

void overlay_draw_pairing_edge(cairo_t *cr, const EdgeNode *pairing_edge) {
  if (!pairing_edge || !pairing_edge->edge.start || !pairing_edge->edge.end)
    return;
  cairo_save(cr);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
  overlay_set_source_argb(cr, 0xFFF59E0B);
  cairo_set_line_width(cr, CONFIG_EDGE_WIDTH + 4.0);
  cairo_move_to(cr, pairing_edge->edge.start->x, pairing_edge->edge.start->y);
  cairo_line_to(cr, pairing_edge->edge.end->x, pairing_edge->edge.end->y);
  cairo_stroke(cr);
  cairo_restore(cr);
}