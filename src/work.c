#include "work.h"
#include <stdio.h>

time_t get_time(const char *fn, logger_t *l) { /*{{{*/
	log(l, "trying to open \"%s\"\n", fn);
	FILE *fd = fopen(fn, "rb");
	assert(fd);
	char dstr[20];
	fseek(fd, 188, SEEK_SET);
	fread(dstr, sizeof(char), 19, fd);
	fclose(fd);
	struct tm t;
	dstr[19] = '\0';
	// 2013:12:27 18:20:32
	// 0123456789012345678
	t.tm_year = atoi(dstr);
	t.tm_mon = atoi(dstr+5);
	t.tm_mday = atoi(dstr+8);
	t.tm_hour = atoi(dstr+10);
	t.tm_min = atoi(dstr+14);
	t.tm_sec = atoi(dstr+17);

	time_t sse = mktime(&t);

	log(l, "%s -> %s -> %d-%d-%d %d:%d:%d -> %u\n", fn, dstr,
		t.tm_year, t.tm_mon, t.tm_mday,
		t.tm_hour, t.tm_min, t.tm_sec,
		sse
	);
	return sse;
} /*}}}*/
void scalepow2(ppm_t *out, ppm_t *in, unsigned int spow) { /*{{{*/
	unsigned int limit = 1<<spow;
	for (size_t ty=0; ty<out->height; ++ty) {
		for (size_t tx=0; tx<out->width; ++tx) {
			size_t tp = tx + ty*out->width;
			unsigned int ys = 0;
			unsigned int cbs = 0;
			unsigned int crs = 0;
			size_t ctr = 0;
			for (size_t y=0; y<limit; ++y) {
				size_t sy = ty*limit + y;
				if (sy >= in->height) { break; }
				for (size_t x=0; x<limit; ++x) {
					size_t sx = tx*limit + x;
					if (sx >= in->width) { break; }
					size_t sp = sx + sy*in->width;
					ys += in->data[sp].x[0];
					cbs += in->data[sp].x[1];
					crs += in->data[sp].x[2];
					++ctr;
				}
			}
			ctr = ctr==0?1:ctr;
			out->data[tp].x[0] = ys/ctr;
			out->data[tp].x[1] = cbs/ctr;
			out->data[tp].x[2] = crs/ctr;
		}
	}
} /*}}}*/
void init_threaddata(threaddata_t *td, const char *ofmt, const char *ifmt, int start, int stop) { /*{{{*/
	log_init(&td->common.l, stdout);
	log(&td->common.l, "- logger created\n");

	td->common.writerstr = calloc(strlen(ofmt)+32, sizeof(char));
	assert(td->common.writerstr != NULL);
	sprintf(td->common.writerstr, "pnmtojpeg -quality=100 > %s", ofmt);
	log(&td->common.l, "- output shape is '%s'\n", td->common.writerstr);

	td->common.readerfmt = ifmt;
	td->common.readerstr = calloc(strlen(ifmt)+16, sizeof(char));
	assert(td->common.readerstr != NULL);
	sprintf(td->common.readerstr, "jpegtopnm < %s", ifmt);
	log(&td->common.l, "- input shape is '%s'\n", td->common.readerstr);

	log(&td->common.l, "- initializing counter\n");
	counter_init(&td->counter, start, stop);

	log(&td->common.l, "- initializing buffer\n");
	buffer_init(&td->buffer, BUFFERSIZE, loader, unloader);
} /*}}}*/
void destroy_threaddata(threaddata_t *td) { /*{{{*/
	buffer_destroy(&td->buffer, &td->common);
	log(&td->common.l, "- destroying counter\n");
	counter_destroy(&td->counter);
	log(&td->common.l, "- destroying logger\n");
	log_destroy(&td->common.l);
	free(td->common.writerstr);
	free(td->common.readerstr);
} /*}}}*/
void *loader(unsigned int index, void* old, void* state, int *status) { /*{{{*/
	ioglobals_t *common = (ioglobals_t*) state;
	char *command = calloc(strlen(common->readerstr)+128, sizeof(char));
	char *fn = calloc(strlen(common->readerfmt)+128, sizeof(char));
	sprintf(command, common->readerstr, index);
	sprintf(fn, common->readerfmt, index);

	imagedata_t *imd = (imagedata_t*)old;
	if (imd == NULL) {
		imd = calloc(sizeof(imagedata_t), 1);
	}
	assert(imd != NULL);

	imd->stamp = get_time(fn, &common->l);
	free(fn);

	FILE *pfd = popen(command, "r");
	log(&common->l, "reading file %u (%s) with command '%s'\n", index, fn, command);
	if (!pfd) {
		perror(NULL);
	}
	assert(pfd != NULL);
	int flag = PPM_OK;
	imd->orig = ppm_fread(imd->orig, pfd, &flag);
	pclose(pfd);
	free(command);
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
	return imd;
} /*}}}*/
void unloader(void *data, unsigned int index, void *state, bool kill) { /*{{{*/
	ioglobals_t *common = (ioglobals_t*) state;
	if (!kill) {
		log(&common->l, "relinquishing index %u\n", index);
	}
	else {
		imagedata_t *imd = (imagedata_t*) data;
		ppm_free(imd->orig);
		for (size_t i=0; i<NSC; ++i) {
			ppm_free(imd->tfsc[i]);
		}
		free(data);
	}
} /*}}}*/
void saver(unsigned int index, ppm_t *data, ioglobals_t *common) { /*{{{*/
	char *command = calloc(strlen(common->writerstr)+128, sizeof(char));
	to_rgb(data->data, data->data, data->width*data->height);
	sprintf(command, common->writerstr, index);
	log(&common->l, "writing file %u with command '%s'\n", index, command);
	FILE *pfd = popen(command, "w");
	if (!pfd) {
		perror(NULL);
	}
	assert(pfd);
	ppm_fwrite(pfd, data);
	pclose(pfd);
	free(command);
} /*}}}*/
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
			int status = OK;
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

			log(&td->common.l, "@@@ work allocating out image (%p)\n", out);
			out = ppm_new(out, cimgs[0]->width, cimgs[0]->height);
			log(&td->common.l, "@@@ work out image done (%p)\n", out);

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
			saver(index, out, &td->common);
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
