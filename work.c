#include "work.h"
#include "ppmsupport.h"

void *workfunc(void *data) { /*{{{*/
	threaddata_t *td = (threaddata_t*) data;
	ppm_t *out = NULL;
	ppm_t *dsz = NULL;
	size_t index = counter_get(&td->counter);
	log(&td->common.l, "thread says hello, starts with index %u, start is %u, end is %u\n", index, td->counter.start, td->counter.end);
	imagedata_t *couchonly = NULL;
	imagedata_t *nocouch = NULL;
	const int couchstart = 755;
	const int couchend = 760;
	while (index < td->counter.end) {
		if (index > td->counter.start+1) { // required delta depends on glitch implementation required history depth
			log(&td->common.l, "processing index %u\n", index)
			int status;
			imagedata_t *cimd = buffer_get(&td->buffer, index, &td->common, &status, NULL);
			imagedata_t *pimd = buffer_get(&td->buffer, index-1, &td->common, &status, NULL);
			imagedata_t *oimd = buffer_get(&td->buffer, index-2, &td->common, &status, NULL);
			if (index >= 670 && couchonly == NULL) {
				couchonly = buffer_get(&td->buffer, 850, &td->common, &status, NULL);
			}
			if ((int)index >= couchstart && (int)index < couchend) {
				//nocouch = buffer_get(&td->buffer, 616, &td->common, &status, NULL);
				nocouch = buffer_get(&td->buffer, index-400, &td->common, &status, NULL);
			}
			ppm_t **cimgs = cimd->tfsc;
			ppm_t **pimgs = pimd->tfsc;
			ppm_t **oimgs = oimd->tfsc;
			//size_t sz = cimg->width*cimg->height;

			out = ppm_new(out, cimgs[0]->width, cimgs[0]->height);

			int t = index;

			for (size_t y0=0; y0<cimgs[0]->height; ++y0) {
				int xdiff = 0;
				for (size_t x0=0; x0<cimgs[0]->width; ++x0) {
					size_t x0mod = x0+xdiff;
					x0mod %= cimgs[0]->width;
					size_t p0 = (y0)*cimgs[0]->width+(x0);
					size_t p0o = (y0)*cimgs[0]->width+(x0mod);
					size_t p0s = (y0>>3)*cimgs[1]->width+(x0>>3);
					size_t p0ss = (y0>>4)*cimgs[2]->width+(x0>>4);
					int ydelta = (int)cimgs[0]->data[p0].x[0] - (int)pimgs[0]->data[p0].x[0];
					int ydelta1 = (int)cimgs[1]->data[p0s].x[0] - (int)pimgs[1]->data[p0s].x[0];
					int ydelta2 = (int)cimgs[2]->data[p0ss].x[0] - (int)pimgs[2]->data[p0ss].x[0];
					int ydelta2v = (int)oimgs[2]->data[p0ss].x[0] - (int)cimgs[2]->data[p0ss].x[0];
					int ydelta2w = (int)oimgs[2]->data[p0ss].x[0] - (int)pimgs[2]->data[p0ss].x[0];
					int ydeltav = (int)cimgs[0]->data[p0].x[0] - (int)oimgs[0]->data[p0].x[0];
					int ydeltaw = (int)oimgs[0]->data[p0].x[0] - (int)pimgs[0]->data[p0].x[0];

					int cbdelta = (int)cimgs[0]->data[p0].x[1] - (int)pimgs[0]->data[p0].x[1];
					int crdelta = (int)cimgs[0]->data[p0].x[2] - (int)pimgs[0]->data[p0].x[2];
					int cy = (int)cimgs[0]->data[p0].x[0];
					uint8_t Y = cimgs[0]->data[p0].x[0];
					uint8_t Cb = cimgs[0]->data[p0].x[1];
					uint8_t Cr = cimgs[0]->data[p0].x[2];

					size_t x1 = x0^(Y+t)^t;
					size_t y1 = (y0^Cb-t);
					size_t x2 = x0^((0x55+t)^Cb);
					size_t y2 = (y0^(Cr+t));

					size_t x3 = x0^((0x0f-t)^Cr);
					size_t y3 = (y0^(Cb+t));
					x1 %= cimgs[0]->width;
					y1 %= cimgs[0]->height;
					x2 %= cimgs[0]->width;
					y2 %= cimgs[0]->height;
					x3 %= cimgs[0]->width;
					y3 %= cimgs[0]->height;
					size_t p1 = (y1)*cimgs[0]->width+(x1);
					size_t p2 = (y2)*cimgs[0]->width+(x2);
					size_t p3 = (y3)*cimgs[0]->width+(x3);
					int xz = x0+(((y0+((t>>4)<<2&0xff)))&0x08);
					int yz = y0+(((x0+((t>>4)<<2&0xff)))&0x08);
					int yw = (y0-t)%cimgs[0]->height;
					int xzm = xz % cimgs[0]->width;
					int yzm = yz % cimgs[0]->height;
					int p0z = (yzm)*cimgs[0]->width+(xzm);
					int p0w = (yw)*cimgs[0]->width+(x2);
					if (t > 10 && t < 157) {
						// walk-in from right
						out->data[p0].x[0] = cimgs[0]->data[p0o].x[0];
						if ((int)cimgs[0]->width-(int)x0 < t-10 + cy) {
							xdiff += ydelta*2;
							out->data[p0].x[0] -= t>>8;
						}
						if ((int)cimgs[0]->width-(int)x0 < t-30 + cy && xdiff > 75) {
							out->data[p0].x[1] = cimgs[0]->data[p0].x[1]^oimgs[0]->data[p0].x[2]^t^ydelta;
							out->data[p0].x[2] = cimgs[0]->data[p0].x[2]^~oimgs[0]->data[p0].x[1]+t^ydelta;
							out->data[p0].x[1] ^= cimgs[0]->data[p2].x[2]^0x10^pimgs[0]->data[p3].x[0];
							out->data[p0].x[2] ^= cimgs[0]->data[p3].x[1]^0x10^pimgs[0]->data[p2].x[0];
						}
						else if ((int)cimgs[0]->width-(int)x0 < t-30 + cy && xdiff > 25) {
							out->data[p0].x[1] = cimgs[0]->data[p0w].x[1]+((int)crdelta-127)*8;
							out->data[p0].x[2] = cimgs[0]->data[p0w].x[2]+((int)cbdelta-127)*8;
						}
						else {
							out->data[p0].x[1] = cimgs[0]->data[p2].x[1];
							out->data[p0].x[2] = cimgs[0]->data[p2].x[2];
						}
					}
					else if (t > 400 && t < 450) {
						// walk-in from left
						if ((int)x0-cy < (int)(t*4-1600)) {
							out->data[p0].x[0] = cimgs[0]->data[p0o].x[0];
						}
						else {
							out->data[p0].x[0] = cimgs[0]->data[p0].x[0];
						}
						if ((int)x0 < t-410 + cy) {
							xdiff += ydelta*3;
							out->data[p0].x[0] -= t>>8;
						}
						if ((int)x0+cy < t-430 && abs(xdiff) > 50) {
							if (abs(xdiff) > 25 && x0<320 && out->data[p0].x[0]>=10) out->data[p0].x[0] -= 10;
							out->data[p0].x[0] = cimgs[0]->data[p0].x[0]^oimgs[0]->data[p0].x[0]^t^ydelta;
							out->data[p0].x[2] = cimgs[0]->data[p0].x[1]^oimgs[0]->data[p0].x[2]^t^ydelta;
							out->data[p0].x[1] = cimgs[0]->data[p0].x[2]^~oimgs[0]->data[p0].x[1]+t^ydelta;
							out->data[p0].x[1] ^= cimgs[0]->data[p2].x[2]^0x07^pimgs[0]->data[p3].x[0];
						}
						else if (((int)x0)-abs(xdiff/2) < (t*3-1200) && abs(xdiff) > 25 && x0 < 320) {
							//out->data[p0].x[0] >>=1;
							out->data[p0].x[2] = cimgs[0]->data[p0w].x[1]+(127-(int)crdelta)*8-ydeltav;
							out->data[p0].x[1] = cimgs[0]->data[p0w].x[2]+(127-(int)cbdelta)*8-ydeltav;
							//out->data[p0].x[2] ^= (cimgs[0]->data[p2].x[2]^pimgs[0]->data[p3].x[1])&0x3f;
							/*
							out->data[p0].x[1] ^= (cimgs[0]->data[p3].x[2]^pimgs[0]->data[p2].x[1])&0x3f;
							out->data[p0].x[2] ^= cimgs[0]->data[p3].x[1]^0x07^pimgs[0]->data[p2].x[0];
							*/
						}
						else if (((int)x0)-abs(xdiff/2) < (t*3-1200) && x0 < 320 && abs(xdiff) > 0 && abs(xdiff)+abs(ydeltaw)*5 > 25) {
							out->data[p0].x[0] >>=1;
							out->data[p0].x[2] = cimgs[0]->data[p0w].x[1]+(127-(int)crdelta)*8-ydeltav;
							out->data[p0].x[1] = cimgs[0]->data[p0w].x[2]+(127-(int)cbdelta)*8-ydeltav;
							out->data[p0].x[1] ^= cimgs[0]->data[p2].x[2]^0x38^pimgs[0]->data[p3].x[0];
							out->data[p0].x[2] ^= cimgs[0]->data[p3].x[1]^0x38^pimgs[0]->data[p2].x[0];
						}
						else {
							out->data[p0].x[1] = cimgs[0]->data[p0o].x[1];
							out->data[p0].x[2] = cimgs[0]->data[p0o].x[2];
						}
/*
						if ((int)x0 < (int)(t*3-1200 + xdiff/3)) {
							if ((int)xdiff < (int)t) xdiff += 2*ydelta;
							out->data[p0].x[0] = (cimgs[0]->data[p0o].x[0]>>1);
							out->data[p0].x[1] = cimgs[0]->data[p2].x[1];
							out->data[p0].x[2] = cimgs[0]->data[p2].x[2];
						}
						else {
							out->data[p0].x[0] = cimgs[0]->data[p0].x[0];
							out->data[p0].x[1] = cimgs[0]->data[p0].x[1];
							out->data[p0].x[2] = cimgs[0]->data[p0].x[2];
						}
*/
					}
					else if (t >= 565 && t <= 650 && xz > 555) {
						if (xz < 575) {
							/*
							out->data[p0].x[0] = cimgs[0]->data[p0].x[0]&oimgs[0]->data[p0z].x[0];
							out->data[p0].x[2] = cimgs[0]->data[p0].x[2]^oimgs[0]->data[p0z].x[1]^(((int)cbdelta-127)*32+127);
							out->data[p0].x[1] = cimgs[0]->data[p0].x[1]^~oimgs[0]->data[p0z].x[2]^(((int)crdelta-127)*32+127);
							*/
							out->data[p0].x[0] = cimgs[0]->data[p0].x[0]^oimgs[0]->data[p0].x[0]^t^ydelta;
							out->data[p0].x[1] = cimgs[0]->data[p0].x[1]^oimgs[0]->data[p0].x[2]^t^ydelta;
							out->data[p0].x[2] = cimgs[0]->data[p0].x[2]^~oimgs[0]->data[p0].x[1]+t^ydelta;
							out->data[p0].x[1] ^= cimgs[0]->data[p2].x[2]^0x10^pimgs[0]->data[p3].x[0];
							out->data[p0].x[2] ^= cimgs[0]->data[p3].x[1]^0x10^pimgs[0]->data[p2].x[0];
						}
						else {
							out->data[p0].x[0] = (cimgs[0]->data[p0w].x[0]|((cimgs[0]->data[p0w].x[0]&0x1f)<<1)&0xf0);
							out->data[p0].x[1] = cimgs[0]->data[p0w].x[1];//+(127-(int)crdelta)*8;
							out->data[p0].x[2] = cimgs[0]->data[p0w].x[2];//+((int)cbdelta-127)*8;
						}
					}
					else if (t > 670) {
						int cydelta = (int)couchonly->tfsc[0]->data[p0].x[0] - (int)cimgs[0]->data[p0].x[0];
						if (yz < 180) {
							// couchonly
							out->data[p0].x[0] = couchonly->tfsc[0]->data[p0].x[0];
							out->data[p0].x[1] = cimgs[0]->data[p0].x[1];
							out->data[p0].x[2] = cimgs[0]->data[p0].x[2];
						}
						else if (yz < 200 && x0 > 290 && abs(cydelta) > 6) {
							// glitch if diffy
							out->data[p0].x[0] = cimgs[0]->data[p0].x[0]^couchonly->tfsc[0]->data[p0].x[0]^t^ydelta;
							out->data[p0].x[2] = cimgs[0]->data[p0].x[1]^couchonly->tfsc[1]->data[p0].x[2]^t^ydelta;
							out->data[p0].x[1] = cimgs[0]->data[p0].x[2]^~couchonly->tfsc[2]->data[p0].x[1]+t^ydelta;
							out->data[p0].x[2] ^= cimgs[0]->data[p2].x[2]^0x10^pimgs[0]->data[p3].x[0];
							out->data[p0].x[1] ^= cimgs[0]->data[p3].x[1]^0x10^pimgs[0]->data[p2].x[0];
						}
						else {
							out->data[p0].x[0] = cimgs[0]->data[p0].x[0];
							out->data[p0].x[1] = cimgs[0]->data[p0].x[1];
							out->data[p0].x[2] = cimgs[0]->data[p0].x[2];
						}
						if (t > couchstart && t < couchend) {
							int ncydelta = (int)nocouch->tfsc[0]->data[p0].x[0] - (int)cimgs[0]->data[p0].x[0];
							if (xz > 200 && xz < 300) {
								out->data[p0].x[0] = nocouch->tfsc[0]->data[p0o].x[0]+2;
								out->data[p0].x[1] = cimgs[0]->data[p0].x[1];
								out->data[p0].x[2] = cimgs[0]->data[p0].x[2];
							}
							else if (((xz > 185 && xz <=200) || (xz>=300 && xz < 315)) && abs(ncydelta) > 10) {
								out->data[p0].x[0] = cimgs[0]->data[p0].x[0]^nocouch->tfsc[0]->data[p0].x[0]^t^ydelta;
								out->data[p0].x[1] = cimgs[0]->data[p0].x[1]^nocouch->tfsc[0]->data[p0].x[2]^t^ydelta;
								out->data[p0].x[2] = cimgs[0]->data[p0].x[2]^~nocouch->tfsc[0]->data[p0].x[1]+t^ydelta;
								out->data[p0].x[1] ^= cimgs[0]->data[p2].x[2]^0x10^pimgs[0]->data[p3].x[0];
								out->data[p0].x[2] ^= cimgs[0]->data[p3].x[1]^0x10^pimgs[0]->data[p2].x[0];
							}
						}
					}
					else {
						out->data[p0].x[0] = cimgs[0]->data[p0].x[0];
						out->data[p0].x[1] = cimgs[0]->data[p0].x[1];
						out->data[p0].x[2] = cimgs[0]->data[p0].x[2];
					}
					/*
					if (abs(ydelta)>=abs(td->counter.end-t)>>3 ||
							abs(cbdelta)>=abs(td->counter.end-t)>>4 ||
							abs(crdelta)>=abs(td->counter.end-t)>>4) {
							*/
					if (t>128 && (abs(ydelta)>=128 || abs(ydelta1) >= 128 || abs(ydelta2) >= 32 )) {

						if (abs(ydelta) >= 64 ) {
							xdiff += 1;
						}

						// input data
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

						out->data[p0].x[0] = ((((int)out->data[p1].x[0])*3+((int)cimgs[0]->data[p2].x[0]))>>2);
						out->data[p0].x[1] = (uint8_t)(((int)cimgs[0]->data[p0].x[2])*3+((int)cimgs[0]->data[p0o].x[0]))>>2;
						out->data[p0].x[2] = (uint8_t)(((int)cimgs[0]->data[p0].x[1])*3+((int)cimgs[0]->data[p0o].x[0]))>>2;
						out->data[p0].x[1] = out->data[p0].x[1]|128^xdiff^t;
						out->data[p0].x[2] ^= 127+t&255;
					}
					else if (t>128&&(abs(ydeltav) > 128 || abs(ydeltaw) > 128 )) {

						// input data
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

						out->data[p0].x[0] = 0;
						out->data[p0].x[1] = 127;//(((int)out->data[p1].x[2])*3+((int)cimgs[0]->data[p0].x[0]))>>2;
						out->data[p0].x[2] = 127;//(((int)out->data[p1].x[1])*3+((int)cimgs[0]->data[p0].x[0]))>>2;
					}
					if ((t < 376) || (t > 380 && t<562) || (t>651)) {
						if (abs(ydelta2) > 8) {
							out->data[p0].x[0] ^= 0x40;
						}
						if (abs(ydelta2v) > 8) {
							out->data[p0].x[0] ^= 0x20;
						}
						if (abs(ydelta2w) > 8) {
							out->data[p0].x[0] ^= 0x10;
						}
					}
				}
			}
			td->saver(index, out, &td->common);
			if ((int)index >= couchstart && (int)index < couchend) {
				buffer_relinquish(&td->buffer, index-400, &td->common, NULL);
			}

			buffer_relinquish(&td->buffer, index-2, &td->common, NULL);
			buffer_relinquish(&td->buffer, index-1, &td->common, NULL);
			buffer_relinquish(&td->buffer, index, &td->common, NULL);
		}
		index = counter_get(&td->counter);
	}
	//buffer_relinquish(&td->buffer, 616, &td->common, NULL); // nocouch
	buffer_relinquish(&td->buffer, 680, &td->common, NULL); // couchonly
	ppm_free(out);
	pthread_exit(NULL);
} /*}}}*/
