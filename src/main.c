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

int main(int argc, char **argv) { /*{{{*/
	if (argc != 5) {
		fprintf(stderr,
				"Wrong argument count.\n"\
				"Expects: output-format input-format start end\n"\
				"e.g. out/o_%%04.jpg in/i_%%04.jpg 0 1024\n");
		return -1;
	}

	threaddata_t td;

	init_threaddata(&td, argv[1], argv[2], strtoul(argv[3], NULL, 0), strtoul(argv[4], NULL, 0));

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

	destroy_threaddata(&td);


	pthread_exit(NULL);
} /*}}}*/
