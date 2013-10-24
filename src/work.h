#ifndef __WORK_H__
#define __WORK_H__

#define NTHREADS 4
#define HISTORY 4
#define BUFFERSIZE (NTHREADS*HISTORY)
#define NSC 3


#include <pthread.h>
#include <assert.h>
#include <string.h>
#include "ibuffer.h"
#include "logger.h"
#include "ppmio.h"
#include "ycbcr.h"
#include "counter.h"


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
} threaddata_t;

typedef struct {
	ppm_t *orig;
	ppm_t *tfsc[NSC];
	time_t stamp;
} imagedata_t;

time_t get_time(const char *fn, logger_t *l);
void scalepow2(ppm_t *out, ppm_t *in, unsigned int spow);
void init_threaddata(threaddata_t *td, const char *ofmt, const char *ifmt, int start, int stop);
void destroy_threaddata(threaddata_t *td);
void *loader(unsigned int index, void* old, void* state, int *status);
void unloader(void *data, unsigned int index, void *state, bool kill);
void saver(unsigned int index, ppm_t *data, ioglobals_t *common);
void *workfunc(void *data);

#endif
