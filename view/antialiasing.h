#ifndef POLYPEN_ANTIALIASING_H
#define POLYPEN_ANTIALIASING_H

#include <stdint.h>
#include <cairo.h>
#include "view/pixel_buffer.h"

typedef enum {
  MSAA_OFF = 1,
  MSAA_2X2 = 2,
  MSAA_4X4 = 4
} MsaaMode;

typedef struct SuperBuffer {
  int width;
  int height;
  int scale;
  int super_width;
  int super_height;
  PixelBuffer *raw;
} SuperBuffer;

SuperBuffer *super_buffer_create(int width, int height, MsaaMode mode);
void super_buffer_destroy(SuperBuffer *sb);
void super_buffer_put_pixel(SuperBuffer *sb, int sx, int sy, uint32_t color);
uint32_t super_buffer_get_pixel(const SuperBuffer *sb, int sx, int sy);
void super_buffer_resolve(const SuperBuffer *sb, PixelBuffer *target);

typedef struct RenderState RenderState;
RenderState *antialiasing_get_render_state(void);

#endif