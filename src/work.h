#ifndef __WORK_H__
#define __WORK_H__

#define NTHREADS 4
#define HISTORY 4
#define BUFFERSIZE (NTHREADS*HISTORY)
#define NSC 2


#include <pthread.h>
#include <assert.h>
#include <string.h>
#include "ibuffer.h"
#include "logger.h"
#include "graphics.h"
#include "ppmio.h"
#include "ycbcr.h"
#include "counter.h"
#include "font.h"


typedef struct {
	logger_t l;
	const char *readerfmt;
	char *readerstr;
	char *writerstr;
} ioglobals_t;

typedef struct {
	counter_t counter;
	buffer_t buffer;
	ioglobals_t common;
	pcf2_font_t font;
} threaddata_t;

typedef struct {
	bitmap_t *orig;
	bitmap_t *tfsc[NSC];
	struct tm time;
	time_t stamp;
} imagedata_t;

void get_time(imagedata_t *img, const char *fn, logger_t *l);
void scalepow2(bitmap_t *out, bitmap_t *in, unsigned int spow);
void init_threaddata(threaddata_t *td, const char *ofmt, const char *ifmt, int start, int stop);
void destroy_threaddata(threaddata_t *td);
void *loader(unsigned int index, void* old, void* state, int *status);
void unloader(void *data, unsigned int index, void *state, bool kill);
void saver(unsigned int index, bitmap_t *data, ioglobals_t *common);
void *workfunc(void *data);

#endif
