#define NTHREADS 8
#define HISTORY 2
#define BUFFERSIZE (NTHREADS*HISTORY)

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

typedef struct {
	counter_t counter;
	buffer_t buffer;
	ioglobals_t common;
} threaddata_t;

void *ppmgenreader(unsigned int index, void* old, void* state, int *status) { /*{{{*/
	ioglobals_t *common = (ioglobals_t*) state;
	char *command = calloc(strlen(common->readerstr)+128, sizeof(char));
	sprintf(command, common->readerstr, index);
	log(&common->l, "reading file %u with command '%s'\n", index, command);

	FILE *pfd = popen(command, "r");
	int flag = PPM_OK;
	ppm_t *cimg = ppm_fread(old, pfd, &flag);
	pclose(pfd);
	if (flag != PPM_OK) {
		log(&common->l, "reading failed...\n");
		*status = READERROR;
		return NULL;
	}
	log(&common->l, "wxh: %u x %u\n", cimg->width, cimg->height);
	to_ycbcr(cimg->data, cimg->data, cimg->width*cimg->height);
	free(command);
	return cimg;
} /*}}}*/
void ppmrelinquisher(void *data, unsigned int index, void *state, bool kill) { /*{{{*/
	ioglobals_t *common = (ioglobals_t*) state;
	if (!kill) {
		log(&common->l, "relinquishing index %u\n", index);
	}
	else {
		log(&common->l, "destroying index %u\n", index);
		ppm_free(data);
	}
} /*}}}*/
void ppmsaver(unsigned int index, ppm_t *data, ioglobals_t *common) { /*{{{*/
	char *command = calloc(strlen(common->writerstr)+128, sizeof(char));
	to_rgb(data->data, data->data, data->width*data->height);
	sprintf(command, common->writerstr, index);
	log(&common->l, "writing file %u with command '%s'\n", index, command);
	FILE *pfd = popen(command, "w");
	ppm_fwrite(pfd, data);
	pclose(pfd);
	free(command);
} /*}}}*/
void *workfunc(void *data) { /*{{{*/
	threaddata_t *td = (threaddata_t*) data;
	size_t index;
	ppm_t *out = NULL;
	while ((index = counter_get(&td->counter)) < td->counter.end) {
		if (index > 1) {
		log(&td->common.l, "processing index %u\n", index)
			int status;
			ppm_t *cimg = buffer_get(&td->buffer, index, &td->common, &status, NULL);
			ppm_t *pimg = buffer_get(&td->buffer, index-1, &td->common, &status, NULL);
			//size_t sz = cimg->width*cimg->height;
			out = ppm_new(out, cimg->width, cimg->height);
			size_t t = index;

			for (size_t y0=0; y0<cimg->height; ++y0) {
				for (size_t x0=0; x0<cimg->width; ++x0) {

					size_t p0 = (y0)*cimg->width+(x0);
					int ydelta = (int)cimg->data[p0].x[0] - (int)pimg->data[p0].x[0];
					int cbdelta = (int)cimg->data[p0].x[1] - (int)pimg->data[p0].x[1];
					int crdelta = (int)cimg->data[p0].x[2] - (int)pimg->data[p0].x[2];
					if (abs(ydelta)>=abs(240-t)>>1 ||
							abs(cbdelta)>=abs(240-t)>>3 ||
							abs(crdelta)>=abs(240-t)>>3) {

						// input data
						uint8_t Y = cimg->data[p0].x[0];
						uint8_t Cb = cimg->data[p0].x[1];
						uint8_t Cr = cimg->data[p0].x[2];

						size_t x1 = x0^(Y+t);
						size_t y1 = (y0^Cb-t);
						if (Y > 127) {
							y1 ^= x1&0x600;
						}
						else {
							y1 ^= ~x1&0x600;
						}

						size_t x2 = x0^((0x55+t)^Cb);
						size_t y2 = (y0^(Cr+t));

						size_t x3 = x0^((0x0f-t)^Cr);
						size_t y3 = (y0^(Cb+t));

						// restricting
						x1 %= cimg->width;
						y1 %= cimg->height;
						x2 %= cimg->width;
						y2 %= cimg->height;
						x3 %= cimg->width;
						y3 %= cimg->height;

						size_t p1 = (y1)*cimg->width+(x1);
						size_t p2 = (y2)*cimg->width+(x2);
						size_t p3 = (y3)*cimg->width+(x3);

						out->data[p0].x[0] = ((((int)cimg->data[p1].x[0])*3+((int)cimg->data[p0].x[0]))>>2)&0xf0;
						out->data[p0].x[1] = (((int)cimg->data[p1].x[1])*3+((int)cimg->data[p0].x[1]))>>2;
						out->data[p0].x[2] = (((int)cimg->data[p1].x[2])*3+((int)cimg->data[p0].x[2]))>>2;
					}
					else {
						out->data[p0].x[0] = cimg->data[p0].x[0];
						out->data[p0].x[1] = cimg->data[p0].x[1];
						out->data[p0].x[2] = cimg->data[p0].x[2];
					}
				}
			}
			ppmsaver(index, out, &td->common);
			buffer_relinquish(&td->buffer, index-1, &td->common, NULL);
			buffer_relinquish(&td->buffer, index, &td->common, NULL);
		}
	}
	pthread_exit(NULL);
} /*}}}*/
int main(int argc, char **argv) { /*{{{*/
	if (argc != 5) {
		fprintf(stderr,
				"Wrong argument count.\n"\
				"Expects: output-format input-format start end\n"\
				"e.g. out/o_%%04.jpg in/i_%%04.jpg 0 1024\n");
		return -1;
	}

	threaddata_t td;
	log_init(&td.common.l, stdout);
	log(&td.common.l, "- logger created\n");

	td.common.writerstr = calloc(strlen(argv[1])+32, sizeof(char));
	assert(td.common.writerstr != NULL);
	sprintf(td.common.writerstr, "pnmtojpeg -quality=100 > %s", argv[1]);

	td.common.readerstr = calloc(strlen(argv[2])+16, sizeof(char));
	assert(td.common.readerstr != NULL);
	sprintf(td.common.readerstr, "jpegtopnm < %s", argv[2]);

	log(&td.common.l, "- output shape is '%s'\n", td.common.writerstr);
	log(&td.common.l, "- input shape is '%s'\n", td.common.readerstr);

	log(&td.common.l, "- initializing counter\n");
	counter_init(&td.counter,
			strtoul(argv[3], NULL, 0),
			strtoul(argv[4], NULL, 0));

	log(&td.common.l, "- initializing buffer\n");
	buffer_init(&td.buffer, BUFFERSIZE, ppmgenreader, ppmrelinquisher);

	log(&td.common.l, "- spawning threads\n");
	pthread_t threads[NTHREADS];
	for (size_t i=0; i<NTHREADS; ++i) {
		int rc = pthread_create(&threads[i], NULL, workfunc, &td);
		if (rc != 0) {
			fprintf(stderr, "pthread_create returned %d\n", rc);
			return -1;
		}
	}
	for (size_t i=0; i<NTHREADS; ++i) {
		pthread_join(threads[i], NULL);
	}
	log(&td.common.l, "- threads done\n");
	log(&td.common.l, "- destroying buffer\n");
	buffer_destroy(&td.buffer, &td.common);

	log(&td.common.l, "- destroying counter\n");
	counter_destroy(&td.counter);

	log(&td.common.l, "- destroying logger\n");
	log_destroy(&td.common.l);

	pthread_exit(NULL);
} /*}}}*/
