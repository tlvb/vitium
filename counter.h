#ifndef __COUNTER_H__
#define __COUNTER_H__

#include <stdlib.h>

typedef struct {
	pthread_mutex_t *m;
	size_t start;
	size_t end;
	size_t next;
} counter_t;

void counter_init(counter_t *c, const size_t start, const size_t end);
size_t counter_get(counter_t *c);
void counter_destroy(counter_t *c);

#endif
