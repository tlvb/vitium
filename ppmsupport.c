#include "ppmsupport.h"

void scalepow2(ppm_t *out, ppm_t *in, unsigned int spow) {
	unsigned int limit = 1<<spow;
	for (size_t ty=0; ty<out->height; ++ty) {
		for (size_t tx=0; tx<out->width; ++tx) {
			size_t tp = tx + ty*out->width;
			unsigned int ys = 0;
			unsigned int cbs = 0;
			unsigned int crs = 0;
			size_t ctr = 0;
			for (size_t y=0; y<limit; ++y) {
				size_t sy = ty*limit + y;
				if (sy >= in->height) { break; }
				for (size_t x=0; x<limit; ++x) {
					size_t sx = tx*limit + x;
					if (sx >= in->width) { break; }
					size_t sp = sx + sy*in->width;
					ys += in->data[sp].x[0];
					cbs += in->data[sp].x[1];
					crs += in->data[sp].x[2];
					++ctr;
				}
			}
			ctr = ctr==0?1:ctr;
			out->data[tp].x[0] = ys/ctr;
			out->data[tp].x[1] = cbs/ctr;
			out->data[tp].x[2] = crs/ctr;
		}
	}
}

