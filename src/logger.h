#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <pthread.h>
#include <stdio.h>

#define log(l, ...) {\
	pthread_mutex_lock((l)->m); \
	fprintf((l)->f, __VA_ARGS__); \
	pthread_mutex_unlock((l)->m); \
}

typedef struct {
	pthread_mutex_t *m;
	FILE *f;
} logger_t;

void log_init(logger_t *l, FILE *target);
void log_destroy(logger_t *l);

#endif
