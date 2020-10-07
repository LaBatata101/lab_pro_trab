#ifndef IMAGE_H
#define IMAGE_H

#define RGBA_S 4
#define RGB_S  3

typedef unsigned char pixel;

struct Image {
    int width;
    int height;
    int bytes_per_pixel;
    pixel **buffer;
};

typedef struct Image Image;

pixel **malloc_image_buffer(int width, int heigth, int pixel_size);
void free_image_buffer(pixel **buffer, int size);

void rotate_image_clockwise(Image image, pixel **buffer);
void rotate_image_anticlockwise(Image image, pixel **buffer);
void flip_image_horizontally(Image image, pixel **buffer);
void flip_image_vertically(Image image, pixel **buffer);
void gray_scalar(Image image, pixel **buffer);

#endif
