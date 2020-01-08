#include <png.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX 240

static int height = 0, width = 0, rowbytes_size = 0;

png_bytepp read_image(FILE *file);
void get_user_input(char *str, int size);
void write_image(FILE *file, png_bytepp data);
void rotate_image_clockwise(png_bytepp data, png_bytepp out);
void rotate_image_anticlockwise(png_bytepp data, png_bytepp out);
void flip_image_vertically(png_bytepp data, png_bytepp out);
void flip_image_horizontally(png_bytepp data, png_bytepp out);
void gray_scalar(png_bytepp data, png_bytepp out);
void copy_image(char filename[], png_bytepp data, png_bytepp out);

int main(void) {
    png_bytepp data, out = NULL;
    char *menu = "Digitação a operação à ser realizada na imagem:\n "\
                  "1) Rotacionar imagem sentido horario\n "\
                  "2) Rotacionar imagem sentido anti-horario\n "\
                  "3) Espelhar imagem horizontal\n "\
                  "4) Espelhar imagem vertical\n "\
                  "5) Escalar cinza\n "\
                  "6) Copiar imagem\n";

    FILE *file;
    char input_filename[MAX], output_filename[MAX];
    while (1) {
        printf("Digite o nome do arquivo: ");
        get_user_input(input_filename, MAX);
        file = fopen(input_filename, "rb");
        if (file == NULL)
            printf("ERRO: arquivo não existe!\n");
        else
            break;
    }
    printf("Digite o nome do arquivo de saida: ");
    get_user_input(output_filename, MAX);

    data = read_image(file);
    // allocate space for the output image
    out = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int y = 0; y < height; y++) {
        out[y] = (png_byte*)malloc(rowbytes_size);
    }

    int op;
    printf("%s\n-> ", menu);
    scanf("%d", &op);
    switch (op) {
        case 1:
            rotate_image_clockwise(data, out);
            break;
        case 2:
            rotate_image_anticlockwise(data, out);
            break;
        case 3:
            flip_image_horizontally(data, out);
            break;
        case 4:
            flip_image_vertically(data, out);
            break;
        case 5:
            gray_scalar(data, out);
            break;
        case 6:
            copy_image(input_filename, data, out);
            break;
        default:
            printf("Operação inválida!\n");
            break;
    }

    fclose(file); // end reading

    if (op != 6) {
        file = fopen(output_filename, "wb");
        write_image(file, out);

        fclose(file); // end writing
    }

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

png_bytepp read_image(FILE *file) {
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(png == NULL) {
        printf("ERRO: creating read_struct");
        fclose(file);
        exit(1);
    }

    // Check if file is png
    char sig[8]; // 8 is the maximum size that can be checked
    // Read in some of the signature bytes 
    if (fread(sig, 1, 8, file) != 8)
        exit(1);

    // png_sig_cmp() returns zero if the image is a PNG and nonzero if it isn't a PNG.
    if (png_sig_cmp(sig, 0, 8)) {
        printf("Formato de arquivo não suportado!\n");
        exit(1);
    }
    // end check

    // let libpng know that there are some bytes missing from the start of the file
    png_set_sig_bytes(png, 8);

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

    // Free all of the memory associated with the png and info
    png_destroy_read_struct(&png, &info, NULL);

    return row_pointers;
}

void write_image(FILE *file, png_bytepp data) {
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
    // set image header information in info
    png_set_IHDR(
            png,
            info,
            width, height,
            8,
            PNG_COLOR_TYPE_RGBA,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    png_write_image(png, data);
    png_write_end(png, NULL);

    for (int y = 0; y < height; y++) 
        free(data[y]);

    free(data);
    // Free all of the memory associated with the png and info
    png_destroy_write_struct(&png, &info);
}

void copy_array(png_bytep dest, png_bytep src, int size) {
    for (int i = 0; i < size; i++)
        dest[i] = src[i];
}

void get_user_input(char *str, int size) {
    fgets(str, size, stdin);
    str = strtok(str, "\n");
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

void rotate_image_anticlockwise(png_bytepp data, png_bytepp out) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*4]);
            png_bytep out_pixel = &(out[width-x-1][y*4]);

            copy_array(out_pixel, pixel, 4);
        }
    }
}

void flip_image_horizontally(png_bytepp data, png_bytepp out) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*4]);
            png_bytep out_pixel = &(out[y][(width-x-1)*4]);

            copy_array(out_pixel, pixel, 4);
        }
    }
}

void flip_image_vertically(png_bytepp data, png_bytepp out) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*4]);
            png_bytep out_pixel = &(out[height-y-1][x*4]);

            copy_array(out_pixel, pixel, 4);
        }
    }
}

void gray_scalar(png_bytepp data, png_bytepp out) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*4]);
            png_bytep out_pixel = &(out[y][x*4]);

            png_byte gray = pixel[0]*0.299 + pixel[1]*0.587 + pixel[2]*0.114;
            memset(pixel, gray, 3);

            copy_array(out_pixel, pixel, 4);
        }
    }
}

/*
 * Return the filename concatenated with "_copia.png"
 */
void copy_image(char filename[], png_bytepp data, png_bytepp out) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*4]);
            png_bytep out_pixel = &(out[y][x*4]);

            copy_array(out_pixel, pixel, 4);
        }
    }
    char *name = strtok(filename, ".");
    strcat(name, "_copia.png");
    FILE *file = fopen(name, "wb");
    write_image(file, out);
    fclose(file);
}
