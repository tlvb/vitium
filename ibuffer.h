#ifndef __IBUFFER_H__
#define __IBUFFER_H__

#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <limits.h>

#define OK 0
#define FULL 1
#define NOTFOUND 2
#define READERROR 4

typedef struct {
	unsigned int index;
	unsigned int readercount;
	void *data;
} buffer_point_t;

typedef void *(*reader_func)(unsigned int index, void* old, void *state, int *status);
typedef void (*relinquisher_func)(void *data, unsigned int index, void *state, bool kill);

typedef struct {
	pthread_mutex_t *m;
	buffer_point_t *storage;
	size_t bsize;
	size_t pop;
	reader_func reader;
	relinquisher_func relinquisher;
} buffer_t;

int buffer_init(buffer_t *b, size_t size, reader_func reader, relinquisher_func relinquisher);
int buffer_destroy(buffer_t *b, void *state);
void *buffer_get(buffer_t *b, unsigned int index, void *state, int *status, pthread_cond_t *wait_var);
void buffer_relinquish(buffer_t *b, unsigned int index, void *state, pthread_cond_t *wait_var);





#endif
