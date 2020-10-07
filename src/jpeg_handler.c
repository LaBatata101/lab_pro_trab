#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include "image.h"


/*
 * Read a JPEG image and return a 2D matrix containing the image pixels
 */
void read_jpeg_file(Image *image, FILE *file) {
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	JSAMPROW row_pointer[1];
    pixel *row_image = NULL;
	
	unsigned long location = 0;
	
	cinfo.err = jpeg_std_error(&jerr);

    // Allocate and initialize a JPEG decompression object
	jpeg_create_decompress(&cinfo);

    // specify data source
	jpeg_stdio_src(&cinfo, file);

	jpeg_read_header(&cinfo, TRUE);
    image->width = cinfo.image_width;
    image->height = cinfo.image_height;

    image->bytes_per_pixel = RGB_S;

	jpeg_start_decompress(&cinfo);

    // alocate space for the image data
	row_image = malloc(cinfo.output_width*cinfo.output_height*cinfo.num_components);
	row_pointer[0] = malloc(cinfo.output_width*cinfo.num_components);

    // Read the contents of the image
	while(cinfo.output_scanline < cinfo.image_height) {
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
		for(int i = 0; i < cinfo.image_width*cinfo.num_components; i++) {
            row_image[location++] = row_pointer[0][i];
        }
	}

    // allocate space for the 2d array
    image->buffer = malloc_image_buffer(cinfo.output_width, cinfo.output_height * sizeof(pixel **),
            RGB_S);

    // transform raw_image(1D array) to a 2D array
    unsigned long count = 0;
    for (int i = 0; i < cinfo.image_width; i++) {
        for (int j = 0; j < cinfo.image_height * cinfo.num_components; j++) {
            image->buffer[i][j] = row_image[count++];
        }
    }

	jpeg_finish_decompress(&cinfo);
    // deallocate JPEG compression object
	jpeg_destroy_decompress(&cinfo);
	free(row_pointer[0]);
}

/*
 * Write the JPEG image contents to disk
 */
void write_jpeg_file(Image image, FILE *file) {
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	
	JSAMPROW row_pointer[1];
	
	cinfo.err = jpeg_std_error(&jerr);
    // initialize the JPEG compression object
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);

    int temp;
    if (image.width != image.height) {
        temp = image.width;
        cinfo.image_width = image.height;    
        cinfo.image_height = temp;
    }
    cinfo.input_components = RGB_S;   /* or 1 for GRACYSCALE images */
	cinfo.in_color_space = JCS_RGB; /* or JCS_GRAYSCALE for grayscale images */

    // use the library's routine to set default compression parameters.
	jpeg_set_defaults(&cinfo);

	jpeg_start_compress(&cinfo, TRUE);
    pixel *image_buffer_1D = malloc(cinfo.image_width*cinfo.image_height*cinfo.num_components);

    // transform out(2D array) to a 1D array
    unsigned long count = 0;
    for (int i = 0; i < cinfo.image_height; i++) {
        for (int j = 0; j < cinfo.image_width*cinfo.num_components; j++) {
            image_buffer_1D[count++] = image.buffer[i][j];
        }
    }

    // write the pixels to file
	while(cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &image_buffer_1D[cinfo.next_scanline*cinfo.image_width*cinfo.input_components];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

    free(image_buffer_1D);
	jpeg_finish_compress(&cinfo);
    // release JPEG compression object
	jpeg_destroy_compress(&cinfo);
}
