#include "ycbcr.h"
#include <math.h>

#define clip(v,l,h) ((v<l)?l:((v>h)?h:v))

void to_ycbcr(u83_t *ido, u83_t *idi, size_t n) {
	for (size_t i=0; i<n; ++i) {
		double r = (double) idi[i].x[0];
		double g = (double) idi[i].x[1];
		double b = (double) idi[i].x[2];
		double y  =   0 + (0.299*r) + (0.587*g) + (0.114*b);
		double cb = 128 - (0.169*r) - (0.331*g) + (0.500*b);
		double cr = 128 + (0.500*r) - (0.419*g) - (0.081*b);
		y  = clip(y, 0, 255);
		cb = clip(cb, 0, 255);
		cr = clip(cr, 0, 255);
		ido[i].x[0] = (uint8_t)  round(y);
		ido[i].x[1] = (uint8_t) round(cb);
		ido[i].x[2] = (uint8_t) round(cr);
	}
}

void to_rgb(u83_t *ido, u83_t *idi, size_t n) {
	for (size_t i=0; i<n; ++i) {
		double y  = (double) idi[i].x[0];
		double cb = (double) idi[i].x[1];
		double cr = (double) idi[i].x[2];
		double r = y                    + 1.40200*(cr-128);
		double g = y - 0.34414*(cb-128) - 0.71414*(cr-128);
		double b = y + 1.77200*(cb-128)                   ;
		r  = clip(r, 0, 255);
		g  = clip(g, 0, 255);
		b  = clip(b, 0, 255);
		ido[i].x[0] = (uint8_t) round(r);
		ido[i].x[1] = (uint8_t) round(g);
		ido[i].x[2] = (uint8_t) round(b);
	}
}





