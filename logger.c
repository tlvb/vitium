#include <stdlib.h>
#include <assert.h>
#include "logger.h"
void log_init(logger_t *l, FILE *target) {
	l->m = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	assert(l->m != NULL);
	pthread_mutex_init(l->m, NULL);
	l->f = target;
}

void log_destroy(logger_t *l) {
	pthread_mutex_destroy(l->m);
	free(l->m);
}
