#ifndef PNG_HANDLER_H
#define PNG_HANDLER_H

#include "image.h"
#include <stdio.h>

void read_png_file(Image *image, FILE *file);
void write_png_file(Image *image, FILE *file);

#endif
