#ifndef POLYPEN_PIXEL_BUFFER_H
#define POLYPEN_PIXEL_BUFFER_H

#include "data.h"

PixelBuffer *pixel_buffer_create(int width, int height);
void pixel_buffer_destroy(PixelBuffer *pb);

#endif