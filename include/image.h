#pragma once

#include <base.h>

typedef union {
	struct {
		uint8_t r,g,b,s;
	};
	uint32_t w;
} Color;

typedef struct {
    int width, height;
    Color *pixels;
} Image;

void load_image(Image *i, char *path);