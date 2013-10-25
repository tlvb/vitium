#include <stdlib.h>
#include <string.h>
#include "graphics.h"
bitmap_t *bitmap_new(bitmap_t *recycle, unsigned int width, unsigned int height) { /*{{{*/
	if (recycle != NULL) {
		if (recycle->data != NULL) {
			if (recycle->width*recycle->height < width*height) {
				free(recycle->data);
				recycle->data = calloc(width*height, sizeof(u83_t));
				if (recycle->data == NULL) {
					free(recycle);
					return NULL;
				}
			}
			else {
				memset(recycle->data, 0, width*height*sizeof(u83_t));
			}
		}
		else {
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
bitmap_t *bitmap_free(bitmap_t *prisoner) { /*{{{*/
	if (prisoner != NULL) {
		if (prisoner->data != NULL) {
			free(prisoner->data);
		}
		free(prisoner);
	}
	return NULL;
} /*}}}*/
