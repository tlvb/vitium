#include "ppmio.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

//#define loc() fprintf(stderr, "%s : line %4d\n", __FILE__, __LINE__)

void ignorewhitespace(FILE *f) { /*{{{*/
	int c = 0;
	do {
		c = fgetc(f);
	} while (isspace(c));
	ungetc(c, f);
} /*}}}*/
void ignorecomment(FILE *f) { /*{{{*/
	int t = fgetc(f);
	if (t == '#') {
		while (fgetc(f) != '\n');
	}
	else {
		ungetc(t, f);
	}
	ignorewhitespace(f);
} /*}}}*/
bitmap_t *ppm_read(bitmap_t *recycle, char *fn, int *status) { /*{{{*/
	FILE *f = fopen(fn, "r");
	if (f == NULL) {
		//loc();
		*status = PPM_FILEERROR;
		return recycle;
	}
	bitmap_t *ret = ppm_fread(recycle, f, status);
	fclose(f);
	return ret;
} /*}}}*/
bitmap_t *ppm_fread(bitmap_t *recycle, FILE *f, int *status) { /*{{{*/
	assert(f);
	/* magic {{{ */
	unsigned int magic;
	if (1 != fscanf(f, "P%u", &magic)) {
		//loc();
		*status = PPM_BADFILE;
		return recycle;
	}
	if (magic != 6) { /* only binary ppm supported currently */
		//loc();
		*status = PPM_BADFILE;
		return recycle;
	}
	ignorewhitespace(f);
	ignorecomment(f);
	/* }}} */
	/* width and height {{{ */
	unsigned int width, height, count;
	if (2 != fscanf(f, "%u %u", &width, &height)) {
		//loc();
		*status = PPM_BADFILE;
		return recycle;
	}

	count = width*height;
	ignorewhitespace(f);
	ignorecomment(f);
	/* }}} */
	/* maxval {{{ */
	unsigned int maxval;
	if (1 != fscanf(f, "%u%*c", &maxval)) { //NOTE: *c to read the 1 char whitespace
		//loc();
		*status = PPM_BADFILE;
		return recycle;
	}
	if (maxval != 255) {
		//loc();
		*status = PPM_BADFILE;
		return recycle;
	}
	/* }}} */
	/* allocate memory {{{ */
	if (recycle == NULL) {
		recycle = calloc(1, sizeof(bitmap_t));
		if (recycle == NULL) {
			//loc();
			*status = PPM_BADALLOC;
			return recycle;
		}
	}
	if (recycle->data != NULL) {
		if (count != recycle->width * recycle->height) {
			free(recycle->data);
			recycle->data = malloc(count*sizeof(u83_t));
			if (recycle->data == NULL) {
				//loc();
				*status = PPM_BADALLOC;
				return recycle;
			}
		}
	}
	else {
		recycle->data = malloc(count*sizeof(u83_t));
		if (recycle->data == NULL) {
			//loc();
			*status = PPM_BADALLOC;
			return recycle;
		}
	}
	/* }}} */
	recycle->width = width;
	recycle->height = height;
	if (count != fread(recycle->data, sizeof(u83_t), count, f)) {
		//loc();
		*status = PPM_BADFILE;
		return recycle;
	}
	*status = PPM_OK;
	return recycle;
} /*}}}*/
int ppm_write(char *fn, bitmap_t *img) { /*{{{*/
	FILE *f = fopen(fn, "w");
	if (f == NULL)
		return PPM_FILEERROR;
	int ret = ppm_fwrite(f, img);
	if (ret != PPM_OK)
		return ret;
	fclose(f);
	return PPM_OK;
} /*}}}*/
int ppm_fwrite(FILE *f, bitmap_t *img) { /*{{{*/
	assert(f);
	size_t count = img->width * img->height;
	fprintf(f, "P6\n%d %d\n255\n", img->width, img->height);
	if (count != fwrite(img->data, sizeof(u83_t), count, f)) {
		fclose(f);
		return PPM_BADFILE;
	}
	return PPM_OK;
} /*}}}*/
bitmap_t *ppm_new(bitmap_t *recycle, unsigned int width, unsigned int height) { /*{{{*/
	if (recycle != NULL) {
		if (recycle->data != NULL) {
			if (recycle->width*recycle->height < width*height) {
				printf("@@@ ppmio old image to small, reallocating data\n");
				free(recycle->data);
				recycle->data = calloc(width*height, sizeof(u83_t));
				if (recycle->data == NULL) {
					free(recycle);
					return NULL;
				}
			}
			else {
				printf("@@@ ppmio old image big enough, reusing data\n");
				memset(recycle->data, 0, width*height*sizeof(u83_t));
			}
		}
		else {
			printf("@@@ ppmio allocating new data -- should probably be an else here so only data==NULL cases go here?\n");
			recycle->data = calloc(width*height, sizeof(u83_t));
			if (recycle->data == NULL) {
				free(recycle);
				return NULL;
			}
		}
		recycle->width = width;
		recycle->height = height;
	}
	else {
		recycle = malloc(sizeof(bitmap_t));
		if (recycle == NULL) {
			return NULL;
		}
		recycle->data = calloc(width*height, sizeof(u83_t));
		if (recycle->data == NULL) {
			free(recycle);
			return NULL;
		}
		recycle->width = width;
		recycle->height = height;
		return recycle;
	}
} /*}}}*/
bitmap_t *ppm_free(bitmap_t *prisoner) { /*{{{*/
	if (prisoner != NULL) {
		if (prisoner->data != NULL) {
			free(prisoner->data);
		}
		free(prisoner);
	}
	return NULL;
} /*}}}*/

