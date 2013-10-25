#include "work.h"
#include <stdio.h>

void get_time(imagedata_t *img, const char *fn, logger_t *l) { /*{{{*/
	log(l, "trying to open \"%s\"\n", fn);
	FILE *fd = fopen(fn, "rb");
	assert(fd);
	char dstr[20];
	fseek(fd, 188, SEEK_SET);
	fread(dstr, sizeof(char), 19, fd);
	fclose(fd);
	dstr[19] = '\0';
	// 2013:12:27 18:20:32
	// 0123456789012345678
	img->time.tm_year = atoi(dstr);
	img->time.tm_mon = atoi(dstr+5);
	img->time.tm_mday = atoi(dstr+8);
	img->time.tm_hour = atoi(dstr+10);
	img->time.tm_min = atoi(dstr+14);
	img->time.tm_sec = atoi(dstr+17);

	img->stamp = mktime(&img->time);

	log(l, "%s -> %s -> %d-%d-%d %d:%d:%d -> %u\n", fn, dstr,
		img->time.tm_year, img->time.tm_mon, img->time.tm_mday,
		img->time.tm_hour, img->time.tm_min, img->time.tm_sec,
		img->stamp
	);
} /*}}}*/
void scalepow2(bitmap_t *out, bitmap_t *in, unsigned int spow) { /*{{{*/
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

	font_load(&td->font, "data/sun12x22.psfu");
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

	get_time(imd, fn, &common->l);
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

	unsigned int powers[NSC] = {0, 4}; // zero never used
	for (size_t i=0; i<NSC; ++i) {
		unsigned int additive = (1<<powers[i])-1;
		imd->tfsc[i] = bitmap_new(imd->tfsc[i], (imd->orig->width+additive)>>i, (imd->orig->height+additive)>>i);
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
		bitmap_free(imd->orig);
		for (size_t i=0; i<NSC; ++i) {
			bitmap_free(imd->tfsc[i]);
		}
		free(data);
	}
} /*}}}*/
void saver(unsigned int index, bitmap_t *data, ioglobals_t *common) { /*{{{*/
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
	bitmap_t *out = NULL;
	bitmap_t *dsz = NULL;
	u83_t white = { .x[0] = 0xff, .x[1] = 0x7f, .x[2] = 0x7f };
	u83_t red = { .x[0] = 0x7f, .x[1] = 0x7f, .x[2] = 0xff };

	size_t index = counter_get(&td->counter);
	log(&td->common.l, "thread says hello, starts with index %u, start is %u, end is %u\n", index, td->counter.start, td->counter.end);
	char str[256]; // should be enough
	while (index < td->counter.end) {
		int status = OK;
		imagedata_t *cimd = buffer_get(&td->buffer, index, &td->common, &status, NULL);
		imagedata_t *pimd;
		if (index > 0) {
			pimd = buffer_get(&td->buffer, index-1, &td->common, &status, NULL);
		}
		else {
			pimd = buffer_get(&td->buffer, index, &td->common, &status, NULL);
		}
		double deltat = difftime(cimd->stamp, pimd->stamp);
		assert(status == OK);
		bitmap_t **cimgs = cimd->tfsc;
		out = bitmap_new(out, cimgs[0]->width, cimgs[0]->height);
		memcpy(out->data, cimgs[0]->data, sizeof(u83_t)*cimgs[0]->width*cimgs[0]->height);
		for (unsigned int y0=0; y0<108; ++y0) {
			unsigned int y1 = y0 >> 4;
			for (unsigned int x0=0; x0<out->width; ++x0) {
				unsigned int x1 = x0 >> 4;
				unsigned int outi = y0*out->width + x0;
				unsigned int sci = y1*cimgs[1]->width + x1;
				out->data[outi].x[0] = cimgs[1]->data[sci].x[0]>>2;
			}
		}
		snprintf(str, 256, "%04u", index);
		draw_string_asc(out, 32, 15, str, &td->font, &white, 0.5f, 2);
		snprintf(str, 256, "% 4.0lf", str, deltat);
		if (deltat < 14 || deltat > 16) {
			draw_string_asc(out, 32+7*12*4, 15, str, &td->font, &red, 0.5f, 2);
		}
		else {
			draw_string_asc(out, 32+7*12*4, 15, str, &td->font, &white, 0.5f, 2);
		}
		snprintf(str, 256, "%04u-%02u-%02u %02u:%02u:%02u",
			cimd->time.tm_year, cimd->time.tm_mon, cimd->time.tm_mday,
			cimd->time.tm_hour, cimd->time.tm_min, cimd->time.tm_sec
		);
		draw_string_asc(out,
			cimgs[0]->width-strlen(str)*td->font.header.width*4-32,
			15, str, &td->font, &white, 0.5, 2
		);
		saver(index, out, &td->common);
		if (index > 0) {
			buffer_relinquish(&td->buffer, index-1, &td->common, NULL);
		}
		else {
			buffer_relinquish(&td->buffer, index, &td->common, NULL);
		}

		buffer_relinquish(&td->buffer, index, &td->common, NULL);
		index = counter_get(&td->counter);
	}
	bitmap_free(out);
	pthread_exit(NULL);
} /*}}}*/
