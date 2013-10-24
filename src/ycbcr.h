#ifndef __YCBCR_H__
#define __YCBCR_H__

#include "graphics.h"
#include <inttypes.h>
#include <stddef.h>

void to_ycbcr(u83_t *ido, u83_t *idi, size_t n);
void to_rgb(u83_t *ido, u83_t *idi, size_t n);

#endif
