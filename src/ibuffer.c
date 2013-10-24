#include "ibuffer.h"
#include <assert.h>

int buffer_init(buffer_t *b, size_t size, reader_func reader, relinquisher_func relinquisher) { /*{{{*/
	b->m = malloc(sizeof(pthread_mutex_t));
	assert(b->m != NULL);
	pthread_mutex_init(b->m, NULL);
	b->storage = calloc(size, sizeof(buffer_point_t));
	assert(b->storage != NULL);
	b->bsize = size;
	b->pop = 0;
	b->reader = reader;
	b->relinquisher = relinquisher;
} /*}}}*/
int buffer_destroy(buffer_t *b, void *state) { /*{{{*/
	for (size_t i=0; i<b->pop; ++i) {
		(*b->relinquisher)(b->storage[i].data, b->storage[i].index, state, true);
	}
	free(b->storage);
	b->bsize = 0;
	pthread_mutex_destroy(b->m);
	free(b->m);
} /*}}}*/
void *buffer_get(buffer_t *b, unsigned int index, void *state, int *status, pthread_cond_t *wait_var) { /*{{{*/
	pthread_mutex_lock(b->m);
	// buffer is assumed to be small relative to the number of searches
	// so no fancy sorting/searching wrt indexes is done
	for (;;) {
		for (size_t i=0; i<b->pop; ++i) {
			buffer_point_t *bp = &b->storage[i];
			if (bp->index == index) { // found it!
				++bp->readercount;
				*status = OK;
				pthread_mutex_unlock(b->m);
				return bp->data;
			}
		}
		// if we get here it is not in the buffer
		if (b->pop < b->bsize) {
			// there is unused space, so load it
			buffer_point_t *bp = &b->storage[b->pop];
			void *dtmp = (*b->reader)(index, bp->data, state, status);
			if (*status == OK) {
				++b->pop;
				bp->index = index;
				bp->readercount = 1;
				bp->data = dtmp;
			}
			pthread_mutex_unlock(b->m);
			return dtmp;
		}
		else {
			// there is no unused space, so check if
			// a point has no readers, and purge it in that case
			for (size_t i=0; i<b->pop; ++i) {
				buffer_point_t *bp = &b->storage[i];
				if (bp->readercount == 0) {
					void *dtmp = (*b->reader)(index, bp->data, state, status);
					if (*status == OK) {
						bp->index = index;
						bp->readercount = 1;
						bp->data = dtmp;
					}
					pthread_mutex_unlock(b->m);
					return dtmp;
				}
			}
		}
		// there is no unused space, and all points have readers
		if (wait_var == NULL) {
			*status = FULL;
			pthread_mutex_unlock(b->m);
			return NULL;
		}
		else {
			pthread_cond_wait(wait_var, b->m);
		}
	}
} /*}}}*/
void buffer_relinquish(buffer_t *b, unsigned int index, void *state, pthread_cond_t *wait_var) { /*{{{*/
	pthread_mutex_lock(b->m);
	for (size_t i=0; i<b->pop; ++i) {
		buffer_point_t *bp = &b->storage[i];
		if (bp->index == index && bp->readercount > 0) {
			--bp->readercount;
			(*b->relinquisher)(bp->data, index, state, false);
			if (wait_var != NULL) {
				pthread_cond_signal(wait_var);
			}
			break;
		}
	}
	pthread_mutex_unlock(b->m);
} /*}}}*/
