#include "counter.h"
#include <pthread.h>
#include <assert.h>

void counter_init(counter_t *c, const size_t start, const size_t end) {
	c->m = malloc(sizeof(pthread_mutex_t));
	assert(c->m != NULL);
	pthread_mutex_init(c->m, NULL);
	c->start = start;
	c->end = end;
	c->next = start;
}

size_t counter_get(counter_t *c) {
	size_t current;
	pthread_mutex_lock(c->m);
	current = c->next++;
	pthread_mutex_unlock(c->m);
	return current;
}

void counter_destroy(counter_t *c) {
	pthread_mutex_destroy(c->m);
	free(c->m);
	c->m = NULL;
}
