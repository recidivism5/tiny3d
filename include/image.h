#pragma once

#include <base.h>

typedef struct {
    int width, height;
    uint32_t *pixels;
} Image;

void load_image(Image *i, char *path);