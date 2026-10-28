#include "view/antialiasing.h"
#include "model/app_state.h"
#include "view/renderer.h"

#include <stdlib.h>

SuperBuffer *super_buffer_create(int width, int height, MsaaMode mode) {
  if (width <= 0 || height <= 0)
    return NULL;
  SuperBuffer *sb = malloc(sizeof(*sb));
  if (!sb)
    return NULL;

  sb->width = width;
  sb->height = height;
  sb->scale = (int)mode;
  sb->super_width = width * sb->scale;
  sb->super_height = height * sb->scale;
  sb->raw = pixel_buffer_create(sb->super_width, sb->super_height);
  if (!sb->raw) {
    free(sb);
    return NULL;
  }
  return sb;
}

void super_buffer_destroy(SuperBuffer *sb) {
  if (!sb)
    return;
  if (sb->raw)
    pixel_buffer_destroy(sb->raw);
  free(sb);
}

void super_buffer_put_pixel(SuperBuffer *sb, int sx, int sy, uint32_t color) {
  if (!sb || !sb->raw || sx < 0 || sx >= sb->super_width || sy < 0 ||
      sy >= sb->super_height)
    return;
  sb->raw->pixels[(size_t)sy * (size_t)sb->super_width + (size_t)sx] = color;
}

uint32_t super_buffer_get_pixel(const SuperBuffer *sb, int sx, int sy) {
  if (!sb || !sb->raw || sx < 0 || sx >= sb->super_width || sy < 0 ||
      sy >= sb->super_height)
    return 0;
  return sb->raw->pixels[(size_t)sy * (size_t)sb->super_width + (size_t)sx];
}

void super_buffer_resolve(const SuperBuffer *sb, PixelBuffer *target) {
  if (!sb || !sb->raw || !target || !target->pixels)
    return;
  int scale = sb->scale;
  int num_samples = scale * scale;
  int w = target->width, h = target->height;

  for (int py = 0; py < h; py++) {
    int base_sy = py * scale;
    for (int px = 0; px < w; px++) {
      int base_sx = px * scale;
      uint32_t sum_a = 0, sum_r = 0, sum_g = 0, sum_b = 0;
      for (int dy = 0; dy < scale; dy++) {
        const uint32_t *row =
            &sb->raw->pixels[(size_t)(base_sy + dy) * (size_t)sb->super_width +
                             (size_t)base_sx];
        for (int dx = 0; dx < scale; dx++) {
          uint32_t s = row[dx];
          sum_a += (s >> 24) & 0xFF;
          sum_r += (s >> 16) & 0xFF;
          sum_g += (s >> 8) & 0xFF;
          sum_b += s & 0xFF;
        }
      }
      uint32_t avg_a = (sum_a + num_samples / 2) / num_samples;
      uint32_t avg_r = (sum_r + num_samples / 2) / num_samples;
      uint32_t avg_g = (sum_g + num_samples / 2) / num_samples;
      uint32_t avg_b = (sum_b + num_samples / 2) / num_samples;

      target->pixels[(size_t)py * (size_t)w + (size_t)px] =
          (avg_a << 24) | (avg_r << 16) | (avg_g << 8) | avg_b;
    }
  }
}

static void msaa_render_state_run(RenderState *self, cairo_t *cr, int width,
                                  int height, AppState *state) {
  (void)self;
  SuperBuffer *sb = super_buffer_create(width, height, state->msaa_mode);
  if (!sb)
    return;

  cairo_surface_t *super_surface = cairo_image_surface_create_for_data(
      (unsigned char *)sb->raw->pixels, CAIRO_FORMAT_ARGB32, sb->super_width,
      sb->super_height, sb->super_width * (int)sizeof(uint32_t));
  cairo_t *super_cr = cairo_create(super_surface);
  cairo_set_antialias(super_cr, CAIRO_ANTIALIAS_NONE);

  renderer_draw_scene_scaled(super_cr, sb->raw, sb->scale, width, height,
                             state);

  cairo_destroy(super_cr);
  cairo_surface_destroy(super_surface);

  PixelBuffer *resolved_pb = pixel_buffer_create(width, height);
  if (resolved_pb) {
    super_buffer_resolve(sb, resolved_pb);
    cairo_surface_t *disp_surf = cairo_image_surface_create_for_data(
        (unsigned char *)resolved_pb->pixels, CAIRO_FORMAT_ARGB32, width,
        height, width * (int)sizeof(uint32_t));
    cairo_save(cr);
    cairo_set_source_surface(cr, disp_surf, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
    cairo_surface_destroy(disp_surf);
    pixel_buffer_destroy(resolved_pb);
  }
  super_buffer_destroy(sb);
}

static RenderState msaa_state_instance = {.render = msaa_render_state_run};

RenderState *antialiasing_get_render_state(void) {
  return &msaa_state_instance;
}