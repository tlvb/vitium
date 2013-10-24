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

#endif
