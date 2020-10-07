#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "image.h"

pixel **malloc_image_buffer(int width, int heigth, int pixel_size) {
    pixel **buffer;
    buffer = malloc(sizeof(pixel *) * width);
    for (int y = 0; y < width; y++)
        buffer[y] = malloc(pixel_size * heigth);

    return buffer;
}

void free_image_buffer(pixel **buffer, int size) {
    for (int i = 0; i < size; i++)
        free(buffer[i]);
    free(buffer);
}

/*
 * image.bytes_per_pixel: the size of bytes per pixel 3 for JPEG(RGB) or 4 for PNG(RGBA), the same is
 * valid for other functions with this parameter
 *
 * Example:
 *    px1 px2 px3     px7 px4 px1
 *    px4 px5 px6 --> px8 px5 px2 
 *    px7 px8 px9     px9 px6 px3
 */
void rotate_image_clockwise(Image image, pixel **buffer) {
    for (int y = 0; y < image.width; y++) {
        for (int x = 0; x < image.height; x++) {
            pixel *src_pixel = &(image.buffer[x][y * image.bytes_per_pixel]);
            pixel *dest = &(buffer[y][(image.width - 1 - x) * image.bytes_per_pixel]);

            memcpy(dest, src_pixel, image.bytes_per_pixel);
        }
    }
}

/*
 * Example: 
 *   px1 px2 px3     px3 px6 px9 
 *   px4 px5 px6 --> px2 px5 px8
 *   px7 px8 px9     px1 px4 px7
 *
 */
void rotate_image_anticlockwise(Image image, pixel **buffer) {
    for (int y = 0; y < image.width; y++) {
        for (int x = 0; x < image.height; x++) {
            pixel *src_pixel = &(image.buffer[x][(image.width - 1 - y) * image.bytes_per_pixel]);
            pixel *dest = &(buffer[y][x * image.bytes_per_pixel]);

            memcpy(dest, src_pixel, image.bytes_per_pixel);
        }
    }
}

/*
 * Example: 
 *  px1 px2 px3     px3 px2 px1
 *  px4 px5 px6 --> px6 px5 px4
 *  px7 px8 px9     px9 px8 px7
 *
 */
void flip_image_horizontally(Image image, pixel **buffer) {
    for (int y = 0; y < image.height; y++) {
        for (int x = 0; x < image.width; x++) {
            pixel *src_pixel = &(image.buffer[y][x * image.bytes_per_pixel]);
            pixel *dest = &(buffer[y][(image.width - x - 1) * image.bytes_per_pixel]);

            memcpy(dest, src_pixel, image.bytes_per_pixel);
        }
    }
}

/*
 * Example:
 *  px1 px2 px3     px7 px8 px9
 *  px4 px5 px6 --> px4 px5 px6
 *  px7 px8 px9     px1 px2 px3
 *
 */
void flip_image_vertically(Image image, pixel **buffer) {
    for (int y = 0; y < image.height; y++) {
        for (int x = 0; x < image.width; x++) {
            pixel *src_pixel = &(image.buffer[y][x * image.bytes_per_pixel]);
            pixel *dest = &(buffer[image.height - y - 1][x * image.bytes_per_pixel]);

            memcpy(dest, src_pixel, image.bytes_per_pixel);
        }
    }
}

/*
 * Transform all image pixels to gray
 */
void gray_scalar(Image image, pixel **buffer) {
    for (int y = 0; y < image.height; y++) {
        for (int x = 0; x < image.width; x++) {
            pixel *src_pixel = &(image.buffer[y][x * image.bytes_per_pixel]);
            pixel *dest = &(buffer[y][x * image.bytes_per_pixel]);

            pixel gray = src_pixel[0]*0.299 + src_pixel[1]*0.587 + src_pixel[2]*0.114;
            memset(src_pixel, gray, RGB_S);

            memcpy(dest, src_pixel, image.bytes_per_pixel);
        }
    }
}
