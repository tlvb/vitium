#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


void font_load(pcf2_font_t *font, const char *fn) { /*{{{*/
	FILE *fd = fopen(fn, "rb");
	assert(fd);
	fread(&font->header, sizeof(pcf2_header_t), 1, fd);
	font->bitmap = malloc(sizeof(uint8_t)*
		font->header.charsize*font->header.length
	);
	fread(font->bitmap, sizeof(uint8_t), font->header.charsize*font->header.length, fd);
	fclose(fd);
	font->bpr = font->header.charsize / font->header.height;
} /*}}}*/
void font_destroy(pcf2_font_t *font) { /*{{{*/
	free(font->bitmap);
} /*}}}*/
void draw_glyph(bitmap_t *canvas,  int x,  int y, unsigned int c, const pcf2_font_t *f, const u83_t *fg) { /*{{{*/
	for (int dy=0; dy<f->header.height; ++dy) {
		for (int dbx=0; dbx<f->bpr; ++dbx) {
			uint8_t fb = f->bitmap[c*f->header.charsize + dy*f->bpr + dbx];
			for (int dx=0; dx<8 && dx+dbx*8<f->header.width; ++dx) {
				int canvasx = x + dbx*8 + dx;
				int canvasy = y + dy;
				if (canvasx>=0 && canvasy>=0 && canvasx<canvas->width && canvasy<canvas->height) {
					unsigned int canvasi = canvasy*canvas->width + canvasx;
					if ((fb & (1<<(7-dx))) != 0) {
						memcpy(canvas->data+canvasi, fg, sizeof(u83_t));
					}
				}
			}
		}
	}
} /*}}}*/
void draw_string(bitmap_t *canvas,  int x,  int y, const char *s, const pcf2_font_t *f, const u83_t *fg){ /*{{{*/
	for (unsigned int i=0; i<strlen(s); ++i) {
		draw_glyph(canvas, x+i*f->header.width, y, (uint8_t)s[i], f, fg);
	}
} /*}}}*/
void draw_glyph_sc(bitmap_t *canvas, int x, int y, unsigned int c, const pcf2_font_t *f, const u83_t *fg, unsigned int sc) { /*{{{*/
	for (int dy=0; dy<f->header.height; ++dy) {
		for (int dbx=0; dbx<f->bpr; ++dbx) {
			uint8_t fb = f->bitmap[c*f->header.charsize + dy*f->bpr + dbx];
			for (int dx=0; dx<8 && dx+dbx*8<f->header.width; ++dx) {
				for (int sy=0; sy<1<<sc; ++sy) {
					for (int sx=0; sx<1<<sc; ++sx) {
						int canvasx = sx+x + ((dbx*8 + dx)<<sc);
						int canvasy = sy+y + (dy<<sc);
						if (canvasx>=0 && canvasy>=0 && canvasx<canvas->width && canvasy<canvas->height) {
							unsigned int canvasi = canvasy*canvas->width + canvasx;
							if ((fb & (1<<(7-dx))) != 0) {
								memcpy(canvas->data+canvasi, fg, sizeof(u83_t));
							}
						}
					}
				}
			}
		}
	}
} /*}}}*/
void draw_string_sc(bitmap_t *canvas, int x, int y, const char *s, const pcf2_font_t *f, const u83_t *fg, unsigned int sc) { /*{{{*/
	for (unsigned int i=0; i<strlen(s); ++i) {
		draw_glyph_sc(canvas, x+i*f->header.width*(1<<sc), y, (uint8_t)s[i], f, fg, sc);
	}
} /*}}}*/
