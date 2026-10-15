#ifndef POLYPEN_DATA_H
#define POLYPEN_DATA_H

#include <stdint.h>

typedef struct {
  int width;
  int height;
  uint32_t *pixels;
} PixelBuffer;

#endif