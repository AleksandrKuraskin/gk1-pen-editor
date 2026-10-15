#include "view/pixel_buffer.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

PixelBuffer *pixel_buffer_create(int width, int height) {
  if (width <= 0 || height <= 0 ||
      (size_t)width > SIZE_MAX / (size_t)height / sizeof(uint32_t)) {
    return NULL;
  }

  PixelBuffer *pb = malloc(sizeof(*pb));
  if (pb == NULL) {
    return NULL;
  }

  pb->width = width;
  pb->height = height;
  pb->pixels = calloc((size_t)width * (size_t)height, sizeof(*pb->pixels));

  if (pb->pixels == NULL) {
    free(pb);
    return NULL;
  }

  return pb;
}

void pixel_buffer_destroy(PixelBuffer *pb) {
  if (pb != NULL) {
    free(pb->pixels);
    free(pb);
  }
}