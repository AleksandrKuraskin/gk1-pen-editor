#ifndef POLYPEN_PIXEL_BUFFER_H
#define POLYPEN_PIXEL_BUFFER_H

#include <stdint.h>

typedef struct {
	int width;
	int height;
	uint32_t *pixels;
} PixelBuffer;

PixelBuffer *pixel_buffer_create(int width, int height);
void pixel_buffer_destroy(PixelBuffer *pb);

#endif