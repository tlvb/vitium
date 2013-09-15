#ifndef __WORK_H__
#define __WORK_H__

#include <pthread.h>
#include <assert.h>
#include "ibuffer.h"
#include "logger.h"
#include "ppmio.h"
#include "ycbcr.h"
#include "counter.h"


typedef struct {
	logger_t l;
	char *readerstr;
	char *writerstr;
} ioglobals_t;

typedef void (*saver_func)(unsigned int index, ppm_t *data, ioglobals_t *common);

typedef struct {
	counter_t counter;
	buffer_t buffer;
	ioglobals_t common;
	saver_func saver;
} threaddata_t;

void *workfunc(void *data);

#endif
