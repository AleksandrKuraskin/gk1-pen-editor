#include "view/renderer.h"
#include "config/config.h"
#include "view/antialiasing.h"
#include "view/overlay.h"
#include "view/rasterizer.h"
#include <cairo.h>
#include <stdlib.h>

void renderer_draw_scene_scaled(cairo_t *cr, PixelBuffer *raw_super_buffer,
                                int scale, int width, int height,
                                AppState *state) {
  cairo_save(cr);
  cairo_scale(cr, scale, scale);
  overlay_draw_background(cr, width, height);

  PixelBuffer *bres_buf = NULL;
  if (state->use_bresenham) {
    bres_buf =
        (scale > 1) ? raw_super_buffer : pixel_buffer_create(width, height);
  }

  if (state->edges != NULL) {
    for (EdgeNode *node = state->edges->head; node != NULL; node = node->next) {
      if (!node->edge.start || !node->edge.end)
        continue;

      switch (node->edge.type) {
      case EDGE_TYPE_BEZIER:
        overlay_set_source_argb(cr, CONFIG_EDGE_ANCHOR_COLOR);
        cairo_set_line_width(cr, CONFIG_EDGE_ANCHOR_LINE_WIDTH);
        cairo_move_to(cr, node->edge.start->x, node->edge.start->y);
        cairo_line_to(cr, node->edge.end->x, node->edge.end->y);
        cairo_stroke(cr);

        overlay_set_source_argb(cr, CONFIG_EDGE_ANCHOR_COLOR);
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

        rasterizer_draw_bezier_forward_diff(
            bres_buf, cr, *(node->edge.start),
            node->edge.geometry.bezier.control1,
            node->edge.geometry.bezier.control2, *(node->edge.end),
            CONFIG_EDGE_COLOR, state->use_bresenham, scale);

        overlay_set_source_argb(cr, CONFIG_CONTROL_POINT_COLOR);
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
        overlay_set_source_argb(cr, CONFIG_EDGE_ANCHOR_COLOR);
        cairo_set_line_width(cr, CONFIG_EDGE_ANCHOR_LINE_WIDTH);
        cairo_move_to(cr, node->edge.start->x, node->edge.start->y);
        cairo_line_to(cr, node->edge.end->x, node->edge.end->y);
        cairo_stroke(cr);

        rasterizer_draw_arc(bres_buf, cr, node, CONFIG_EDGE_COLOR,
                            state->use_bresenham, scale);
        break;

      case EDGE_TYPE_STRAIGHT:
      default:
        if (state->use_bresenham && bres_buf != NULL) {
          rasterizer_draw_line_bresenham(
              bres_buf, node->edge.start->x * scale,
              node->edge.start->y * scale, node->edge.end->x * scale,
              node->edge.end->y * scale, CONFIG_EDGE_COLOR);
        } else {
          cairo_save(cr);
          overlay_set_source_argb(cr, CONFIG_EDGE_COLOR);
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

  if (state->use_bresenham && scale == 1 && bres_buf != NULL) {
    cairo_surface_t *surf = cairo_image_surface_create_for_data(
        (unsigned char *)bres_buf->pixels, CAIRO_FORMAT_ARGB32, width, height,
        width * (int)sizeof(uint32_t));
    if (cairo_surface_status(surf) == CAIRO_STATUS_SUCCESS) {
      cairo_save(cr);
      cairo_set_source_surface(cr, surf, 0, 0);
      cairo_paint(cr);
      cairo_restore(cr);
    }
    cairo_surface_destroy(surf);
    pixel_buffer_destroy(bres_buf);
  }

  overlay_draw_pairing_edge(cr, state->pairing_edge);
  overlay_draw_edge_hover(cr, state->hovered_edge);
  overlay_draw_points(cr, state->points, state->hovered_point);

  if (state->edges != NULL) {
    for (EdgeNode *en = state->edges->head; en != NULL; en = en->next) {
      overlay_draw_edge_constraints_labels(cr, &en->edge);
    }
    if (state->points != NULL) {
      for (PointNode *pn = state->points->head; pn != NULL; pn = pn->next) {
        overlay_draw_point_continuity_label(cr, pn->point);
      }
    }
  }
  cairo_restore(cr);
}

static void direct_render_state_run(RenderState *self, cairo_t *cr, int width,
                                    int height, AppState *state) {
  (void)self;
  renderer_draw_scene_scaled(cr, NULL, 1, width, height, state);
}

static RenderState direct_state_instance = {.render = direct_render_state_run};

static RenderState *renderer_get_state(MsaaMode mode) {
  if (mode == MSAA_OFF)
    return &direct_state_instance;
  return antialiasing_get_render_state();
}

void renderer_draw_pixel_buffer(GtkDrawingArea *area, cairo_t *cr, int width,
                                int height, gpointer user_data) {
  (void)area;
  AppState *state = user_data;
  if (!state)
    return;

  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  RenderState *render_state = renderer_get_state(state->msaa_mode);
  render_state->render(render_state, cr, width, height, state);
}