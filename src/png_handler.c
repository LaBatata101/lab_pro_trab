#include <png.h>
#include <stdlib.h>
#include "image.h"

/* 
 * Initalize all things needed before reading the PNG image
 *
 */
void prepare_to_read(png_structp png, png_infop info) {
    // read the PNG image information
    png_read_info(png, info);

    png_byte color_type = png_get_color_type(png, info);
    // bit_depth the number of bits used to indicate the color of a single pixel 
    // or the number of bits used for each color component of a single pixel.
    png_byte bit_depth  = png_get_bit_depth(png, info);

    if(bit_depth == 16)
        // strip 16 bit PNG file to 8 bit depth
        png_set_strip_16(png);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        // Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel
        png_set_expand_gray_1_2_4_to_8(png);

    if(png_get_valid(png, info, PNG_INFO_tRNS))
        // The tRNS chunk specifies that the image uses simple transparency: 
        // either alpha values associated with palette entries (for indexed-color images) or 
        // a single transparent color (for grayscale and truecolor images).

        // Expand paletted or RGB images with transparency to full alpha channels
        // so the data will be available as RGBA quartets.
        png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff = 255.
    if(color_type == PNG_COLOR_TYPE_RGB ||color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if(color_type == PNG_COLOR_TYPE_GRAY ||color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    // After setting the transformations, libpng can update your png_info
    // structure to reflect any transformations you've requested with this call.
    png_read_update_info(png, info);
}

/*
 * Read a PNG image and return a 2D matrix containing the image pixels
 */
void read_png_file(Image *image, FILE *file) {
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(png == NULL) {
        printf("ERRO: creating read_struct");
        fclose(file);
        exit(1);
    }

    // Allocate/initialize the image information data.
    png_infop info = png_create_info_struct(png);
    if (info == NULL) {
        fclose(file);
        // Free all of the memory associated with the png and info
        png_destroy_write_struct(&png, &info);
        printf("ERRO: creating info_struct\n");
        exit(1);
    }
    if (setjmp(png_jmpbuf(png))) {
        fclose(file);
        // Free all of the memory associated with the png and info
        png_destroy_read_struct(&png, &info, NULL);
        exit(1);
    }
    // Initialize the default input/output functions for the PNG file to standard C streams
    png_init_io(png, file);

    prepare_to_read(png, info);

    image->width = png_get_image_width(png, info);
    image->height = png_get_image_height(png, info);

    // allocate space for the image data
    image->buffer = malloc_image_buffer(image->width, image->height, RGBA_S);

    image->bytes_per_pixel = RGBA_S;

    // get the actual data from image
    png_read_image(png, image->buffer);

    // Free all of the memory associated with the png and info
    png_destroy_read_struct(&png, &info, NULL);
}

/*
 * Write the PNG image contents to disk
 */
void write_png_file(Image *image, FILE *file) {
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (png == NULL) {
        fclose(file);
        printf("ERRO: creating write_struct\n");
        exit(1);
    }
    // Allocate/initialize the image information data.
    png_infop info = png_create_info_struct(png);
    if (info == NULL) {
        fclose(file);
        // Free all of the memory associated with the png and info
        png_destroy_write_struct(&png, &info);
        printf("ERRO: creating info_struct\n");
        exit(1);
    }

    if (setjmp(png_jmpbuf(png))) {
        fclose(file);
        // Free all of the memory associated with the png and info
        png_destroy_write_struct(&png, &info);
        exit(1);
    }

    png_init_io(png, file);
    int temp;
    if (image->width != image->height) {
        temp = image->height;
        image->height = image->width;
        image->width = temp;
    }    
    // set image header information in info
    png_set_IHDR(
            png,
            info,
            image->width, image->height,
            8,
            PNG_COLOR_TYPE_RGBA,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    png_write_image(png, image->buffer);
    png_write_end(png, NULL);

    // Free all of the memory associated with the png and info
    png_destroy_write_struct(&png, &info);
}
