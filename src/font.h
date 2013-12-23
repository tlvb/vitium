#ifndef __FONT_H__
#define __FONT_H__
#include "graphics.h"
#include <stdint.h>
#include <stdio.h>

// No unicode stuff supported yet.

typedef struct {
    uint8_t  magic[4];
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t length;
    uint32_t charsize;
    uint32_t height;
    uint32_t width;
} pcf2_header_t;

typedef struct {
	pcf2_header_t header;
	unsigned int bpr;
	uint8_t *bitmap;
} pcf2_font_t;

void font_load_fd(pcf2_font_t *font, FILE *fd);
void font_load(pcf2_font_t *font, const char* fn);
void font_load_gz(pcf2_font_t *font, const char* fn);
void font_destroy(pcf2_font_t *font);
void draw_glyph(bitmap_t *canvas, int x, int y, unsigned int c, const pcf2_font_t *f, const u83_t *fg);
void draw_string(bitmap_t *canvas, int x, int y, const char *s, const pcf2_font_t *f, const u83_t *fg);
void draw_glyph_sc(bitmap_t *canvas, int x, int y, unsigned int c, const pcf2_font_t *f, const u83_t *fg, unsigned int sc);
void draw_glyph_asc(bitmap_t *canvas, int x, int y, unsigned int c, const pcf2_font_t *f, const u83_t *fg, float a, unsigned int sc);
void draw_string_sc(bitmap_t *canvas, int x, int y, const char *s, const pcf2_font_t *f, const u83_t *fg, unsigned int sc);
void draw_string_asc(bitmap_t *canvas, int x, int y, const char *s, const pcf2_font_t *f, const u83_t *fg, float a, unsigned int sc);
#endif
