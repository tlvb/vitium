#include "work.h"
#include "ppmsupport.h"

void *workfunc(void *data) { /*{{{*/
	threaddata_t *td = (threaddata_t*) data;
	ppm_t *out = NULL;
	ppm_t *dsz = NULL;
	size_t index = counter_get(&td->counter);
	log(&td->common.l, "thread says hello, starts with index %u, start is %u, end is %u\n", index, td->counter.start, td->counter.end);
	while (index < td->counter.end) {
		if (index > td->counter.start) { // required delta depends on glitch implementation required history depth
			log(&td->common.l, "processing index %u\n", index)
			int status;
			imagedata_t *cimd = buffer_get(&td->buffer, index, &td->common, &status, NULL);
			imagedata_t *pimd = buffer_get(&td->buffer, index-1, &td->common, &status, NULL);
			ppm_t **cimgs = cimd->tfsc;
			ppm_t **pimgs = pimd->tfsc;
			//size_t sz = cimg->width*cimg->height;

			out = ppm_new(out, cimgs[0]->width, cimgs[0]->height);

			size_t t = index;

			for (size_t y0=0; y0<cimgs[0]->height; ++y0) {
				size_t xdiff = 0;
				for (size_t x0=0; x0<cimgs[0]->width; ++x0) {
					size_t x0mod = x0+xdiff;
					x0mod %= cimgs[0]->width;
					size_t p0 = (y0)*cimgs[0]->width+(x0);
					size_t p0o = (y0)*cimgs[0]->width+(x0mod);
					size_t p0s = (y0>>3)*cimgs[1]->width+(x0>>3);
					size_t p1;
					int ydelta = (int)cimgs[0]->data[p0].x[0] - (int)pimgs[0]->data[p0].x[0];
					int ydelta1 = (int)cimgs[1]->data[p0s].x[0] - (int)pimgs[1]->data[p0s].x[0];

					int cbdelta = (int)cimgs[0]->data[p0].x[1] - (int)pimgs[0]->data[p0].x[1];
					int crdelta = (int)cimgs[0]->data[p0].x[2] - (int)pimgs[0]->data[p0].x[2];
					int cy = (int)cimgs[0]->data[p0].x[0];

					/*
					if (abs(ydelta)>=abs(td->counter.end-t)>>3 ||
							abs(cbdelta)>=abs(td->counter.end-t)>>4 ||
							abs(crdelta)>=abs(td->counter.end-t)>>4) {
							*/
					if (abs(ydelta)>=32 || abs(ydelta1) >= 48) {
						if (abs(ydelta) >= 63) {
							xdiff += 3;
						}

						// input data
						uint8_t Y = cimgs[0]->data[p0].x[0];
						uint8_t Cb = cimgs[0]->data[p0].x[1];
						uint8_t Cr = cimgs[0]->data[p0].x[2];

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
						x1 %= cimgs[0]->width;
						y1 %= cimgs[0]->height;
						x2 %= cimgs[0]->width;
						y2 %= cimgs[0]->height;
						x3 %= cimgs[0]->width;
						y3 %= cimgs[0]->height;

						size_t p1 = (y1)*cimgs[0]->width+(x1);
						size_t p2 = (y2)*cimgs[0]->width+(x2);
						size_t p3 = (y3)*cimgs[0]->width+(x3);

						out->data[p0].x[0] = ((((int)cimgs[0]->data[p1].x[0])*3+((int)cimgs[0]->data[p2].x[0]))>>2)&0xf0;
						out->data[p0].x[1] = (((int)cimgs[0]->data[p1].x[1])*3+((int)cimgs[0]->data[p0].x[2]))>>2;
						out->data[p0].x[2] = (((int)cimgs[0]->data[p1].x[2])*3+((int)cimgs[0]->data[p0].x[1]))>>2;
						out->data[p0].x[1] ^= 0x40;
						out->data[p0].x[2] ^= 0x40;
					}
					else {
						out->data[p0].x[0] = cimgs[0]->data[p0o].x[0];
						out->data[p0].x[1] = cimgs[0]->data[p0o].x[1];
						out->data[p0].x[2] = cimgs[0]->data[p0o].x[2];
					}
				}
			}
			td->saver(index, out, &td->common);
			buffer_relinquish(&td->buffer, index-1, &td->common, NULL);
			buffer_relinquish(&td->buffer, index, &td->common, NULL);
		}
		index = counter_get(&td->counter);
	}
	ppm_free(out);
	pthread_exit(NULL);
} /*}}}*/
