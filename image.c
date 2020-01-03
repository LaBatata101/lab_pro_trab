/*
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define PNG_DEBUG 3
#include <png.h>

void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;
png_bytep * out_pointers;

void read_png_file(char* file_name)
{
        char header[8];    // 8 is the maximum size that can be checked

        /* open file and test for it being a png */
        FILE *fp = fopen(file_name, "rb");
        if (!fp)
                abort_("[read_png_file] File %s could not be opened for reading", file_name);
        fread(header, 1, 8, fp);
        if (png_sig_cmp(header, 0, 8))
                abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


        /* initialize stuff */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[read_png_file] png_create_read_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[read_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during init_io");

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        /*number_of_passes = png_set_interlace_handling(png_ptr);*/
        if(bit_depth == 16)
            png_set_strip_16(png_ptr);

        if(color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png_ptr);

        // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
        if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            png_set_expand_gray_1_2_4_to_8(png_ptr);

        if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(png_ptr);

        // These color_type don't have an alpha channel then fill it with 0xff.
        if(color_type == PNG_COLOR_TYPE_RGB ||
                color_type == PNG_COLOR_TYPE_GRAY ||
                color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

        if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png_ptr);

        png_read_update_info(png_ptr, info_ptr);


        /* read file */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during read_image");

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        for (y=0; y<height; y++)
                row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

        out_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        for (y=0; y<height; y++)
                out_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
        png_read_image(png_ptr, row_pointers);

        fclose(fp);
}


void write_png_file(char* file_name, png_bytep *_row_pointers)
{
        /* create file */
        FILE *fp = fopen(file_name, "wb");
        if (!fp)
                abort_("[write_png_file] File %s could not be opened for writing", file_name);


        /* initialize stuff */
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[write_png_file] png_create_write_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[write_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during init_io");

        png_init_io(png_ptr, fp);


        /* write header */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during writing header");

        png_set_IHDR(png_ptr, info_ptr, width, height,
                     bit_depth, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);


        /* write bytes */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during writing bytes");

        png_write_image(png_ptr, _row_pointers);


        /* end write */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during end of write");

        png_write_end(png_ptr, NULL);

        /* cleanup heap allocation */
        for (y=0; y<height; y++)
                png_free(png_ptr, _row_pointers[y]);
        png_free(png_ptr, _row_pointers);

        fclose(fp);
}


void process_file(void)
{


    for (y=0; y<height; y++) {
        png_byte* row = row_pointers[y];
        png_byte* outr = out_pointers[height-y-1];
        for (x=0; x<width; x++) {
            png_byte* pixel = &(row[(x)*4]); // Get RGB values
            png_byte* outp = &(outr[(x)*4]);
            /*printf("Pixel at position [ %d - %d ] has RGB values: %d - %d - %d\n",*/
            /*x, y, ptr[0], ptr[1], ptr[2]);*/

            /* set red value to 0 and green value to the blue one */
            /*unsigned char gray = pixel[0]*0.299 + pixel[1]*0.587 + pixel[2]*0.114;*/
            /*memset(pixel, gray, 4);*/
            /*memcpy(outr, row, 4);*/
            /*printf("height= %d | width=%d| height-y=%d | width-x=%d\n",height, width, height-y, width-x);*/
            /*memcpy(out_pointers[abs(y-height)], pixel, 4); */
            /*printf("size: %d\n", pixel[5]);*/
            /*memcpy(outp, pixel, sizeof(png_byte*));*/
            for (int i = 0; i < 3; i++)
                outp[i] = pixel[i];
            /*for (int p = 0; p < 4; p++)*/
                /*out_pointers[y-height-1][x]= pixel[p];*/
            /*png_byte* temp = pixel;*/
            /*printf("row_pointers[height-y-1][width-x-1] = %d\n", row_pointers[height-y-1][width-x-1]);*/
            /*row_pointers[y][x] = row_pointers[height-y-1][width-x-1];*/
            /*row_pointers[height-y-1][width-x-1] = temp;*/
            /*out_pointers[y][width-x-1] = row_pointers[y][x];*/
        }
    }
}


int main(int argc, char **argv)
{
        if (argc != 3)
                abort_("Usage: program_name <file_in> <file_out>");

        read_png_file(argv[1]);
        process_file();
        write_png_file(argv[2], out_pointers);

        return 0;
}
