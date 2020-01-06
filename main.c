#include <png.h>
#include <stdio.h>
#include <stdlib.h>


static int height = 0, width = 0, rowbytes_size = 0;

png_bytepp read_image(const char *filename);
void write_image(const char *filename, png_bytepp data);
void rotate_image_clockwise(png_bytepp data, png_bytepp out);


int main(void) {
    char *menu = "MEEENNUUUUUU:\n "\
                  "1) Rotacionar imagem sentido horario\n "\
                  "2) Rotacionar imagem sentido anti-horario\n "\
                  "3) Espelhar imagem horizontal\n "\
                  "4) Espelhar imagem vertical\n "\
                  "5) Escalar cinza\n "\
                  "6) Copiar imagem\n"\
                  "0) Sair \n";
    int op;

    png_bytepp data, out;
    do {
        printf("-> %s", menu);
        scanf("%d", &op);
        switch (op) {
            case 1:
                // TODO: melhorar isso
                data = read_image("lena.png");
                out = (png_bytep*)malloc(sizeof(png_bytep) * height);
                for(int y = 0; y < height; y++) {
                    out[y] = (png_byte*)malloc(rowbytes_size);
                }
                rotate_image_clockwise(data, out);
                write_image("out.png", out);
                break;
            default:
                break;
        }
    } while (op != 0);
    return 0;
}

/* 
 * Initalize all things needed before reading the image
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

png_bytepp read_image(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("ERRO: arquivo não existe!\n");
        //		return 1;
    }
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png) 
        printf("Erro"); // TODO: handle error
    png_infop info = png_create_info_struct(png);
    // TODO: implement setjmp/png_jmpbuf
    // Initialize the default input/output functions for the PNG file to standard C streams
    png_init_io(png, file);

    prepare_to_read(png, info);

    width = png_get_image_width(png, info);
    height = png_get_image_height(png, info);

    // if row_pointers wasn't static the variabe would be allocated in the stack and when the
    // function reached the end it would be freed, therefore, it needs the keyword static to be 
    // allocated in the heap and don't be freed.
    static png_bytepp row_pointers;
    // allocate space for the image data
    rowbytes_size = png_get_rowbytes(png, info);
    row_pointers =  (png_bytepp)malloc(sizeof(png_bytep) * height);
    for (int row = 0; row < height; row++)
        row_pointers[row] = (png_bytep) malloc(rowbytes_size);

    // get the actual data from image
    png_read_image(png, row_pointers);

    fclose(file);
    // Free all of the memory associated with the png and info
    png_destroy_read_struct(&png, &info, NULL);

    return row_pointers;
}

void write_image(const char *filename, png_bytepp data) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL)
        printf("ERRO: ao escrever o arquivo\n");
    

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    png_init_io(png, file);
    png_set_IHDR(
            png,
            info,
            width, height,
            8,
            PNG_COLOR_TYPE_RGBA,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT
            );
    png_write_info(png, info);
    png_write_image(png, data);
    png_write_end(png, NULL);

    for (int y = 0; y < height; y++) 
        free(data[y]);

    free(data);
    fclose(file);
    png_destroy_write_struct(&png, &info);
}

void copy_array(png_bytep dest, png_bytep src, int size) {
    for (int i = 0; i < size; i++)
        dest[i] = src[i];
}

void rotate_image_clockwise(png_bytepp data, png_bytepp out) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*4]);
            png_bytep out_pixel = &(out[x][(height-y-1)*4]);

            copy_array(out_pixel, pixel, 4);
        }
    }
}
