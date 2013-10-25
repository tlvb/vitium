#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__
#include <inttypes.h>

typedef struct {
	uint8_t x[3];
} u83_t;

typedef struct {
	unsigned int width;
	unsigned int height;
	u83_t *data;
} bitmap_t;

bitmap_t *bitmap_new(bitmap_t *recycle, unsigned int width, unsigned int height);
bitmap_t *bitmap_free(bitmap_t *prisoner);

#endif
