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
#include "work.h"
#include "ppmsupport.h"

void *ppmgenreader(unsigned int index, void* old, void* state, int *status) { /*{{{*/
	ioglobals_t *common = (ioglobals_t*) state;
	char *command = calloc(strlen(common->readerstr)+128, sizeof(char));
	sprintf(command, common->readerstr, index);
	log(&common->l, "reading file %u with command '%s'\n", index, command);

	imagedata_t *imd = (imagedata_t*)old;
	if (imd == NULL) {
		imd = calloc(sizeof(imagedata_t), 1);
	}
	assert(imd != NULL);

	FILE *pfd = popen(command, "r");
	int flag = PPM_OK;
	imd->orig = ppm_fread(imd->orig, pfd, &flag);
	pclose(pfd);
	assert(flag == PPM_OK);
	assert(imd->orig != NULL);
	log(&common->l, "wxh: %u x %u\n", imd->orig->width, imd->orig->height);

	unsigned int powers[NSC] = {0, 3, 4}; // zero never used
	for (size_t i=0; i<NSC; ++i) {
		unsigned int additive = (1<<powers[i])-1;
		imd->tfsc[i] = ppm_new(imd->tfsc[i], (imd->orig->width+additive)>>i, (imd->orig->height+additive)>>i);
		assert(imd->tfsc[i] != NULL);
		log(&common->l, "index %u: created image %u/%u for colour transform and dimensions %u x %u\n", index, i, NSC, imd->tfsc[i]->width, imd->tfsc[i]->height);
	}
	to_ycbcr(imd->tfsc[0]->data, imd->orig->data, imd->orig->width*imd->orig->height);
	for (size_t i=1; i<NSC; ++i) {
		scalepow2(imd->tfsc[i], imd->tfsc[0], powers[i]);
	}
	free(command);
	return imd;
} /*}}}*/

void ppmrelinquisher(void *data, unsigned int index, void *state, bool kill) { /*{{{*/
	ioglobals_t *common = (ioglobals_t*) state;
	if (!kill) {
		log(&common->l, "relinquishing index %u\n", index);
	}
	else {
		log(&common->l, "destroying index %u\n", index);
		imagedata_t *imd = (imagedata_t*) data;
		ppm_free(imd->orig);
		for (size_t i=0; i<NSC; ++i) {
			ppm_free(imd->tfsc[i]);
		}
		free(data);
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

	td.saver = &ppmsaver;

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
