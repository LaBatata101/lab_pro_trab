#ifndef JPEG_HANDLER_H
#define JPEG_HANDLER_H

#include <stdio.h>
#include "image.h"

void write_jpeg_file(Image image, FILE *file);
void read_jpeg_file(Image *image, FILE *file);

#endif
