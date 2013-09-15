#ifndef __PPM_SUPPORT_H__
#define __PPM_SUPPORT_H__

#define NSC 3

#include "ppmio.h"

typedef struct {
	ppm_t *orig;
	ppm_t *tfsc[NSC];
} imagedata_t;

void scalepow2(ppm_t *out, ppm_t *in, unsigned int spow);

#endif
