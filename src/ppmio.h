#ifndef __PNMIO_H__
#define __PNMIO_H__ 1

#include <stdint.h>
#include <stdio.h>
#include "graphics.h"

#define PPM_OK 0
#define PPM_FILEERROR 1
#define PPM_BADFILE 2
#define PPM_BADALLOC 3

#define ppm_data(i,x,y,j) (i->data[i->width*y+x].c[j])


void ignorewhitespace(FILE *f);
void ignorecomment(FILE *f);
bitmap_t *ppm_read(bitmap_t *recycle, char *fn, int *status);
bitmap_t *ppm_fread(bitmap_t *recycle, FILE *f, int *status);
int ppm_write(char *fn, bitmap_t *img);
int ppm_fwrite(FILE *f, bitmap_t *img);
bitmap_t *ppm_new(bitmap_t *recycle, unsigned int width, unsigned int height);
bitmap_t *ppm_free(bitmap_t *prisoner);

#endif
