#include <png.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <jpeglib.h>

#define MAX 240

static int height = 0, width = 0, rowbytes_size = 0;

int is_png(char *filename);
png_bytepp read_png_file(FILE *file);
unsigned char **read_jpeg_file(FILE *file);
void get_user_input(char *str, int size);
void write_png_file(FILE *file, png_bytepp data);
void write_jpeg_file(FILE *file, unsigned char **data);
void rotate_image_clockwise(png_bytepp data, png_bytepp out, int bytes_per_pixel);
void rotate_image_anticlockwise(png_bytepp data, png_bytepp out, int bytes_per_pixel);
void flip_image_vertically(png_bytepp data, png_bytepp out, int bytes_per_pixel);
void flip_image_horizontally(png_bytepp data, png_bytepp out, int bytes_per_pixel);
void gray_scalar(png_bytepp data, png_bytepp out, int bytes_per_pixel);
void copy_image(char filename[], char out_filename[],
        png_bytepp data, png_bytepp out, int bytes_per_pixel);

int main(void) {
    unsigned char  **data, **out = NULL;
    char *menu = "Digitação a operação à ser realizada na imagem:\n "\
                  "1) Rotacionar imagem 90º sentido horario\n "\
                  "2) Rotacionar imagem 90º sentido anti-horario\n "\
                  "3) Espelhar imagem horizontal\n "\
                  "4) Espelhar imagem vertical\n "\
                  "5) Escalar cinza\n "\
                  "6) Copiar imagem\n";

    FILE *file;
    char input_filename[MAX], output_filename[MAX];
    while (1) {
        printf("Digite o nome do arquivo da imagem: ");
        get_user_input(input_filename, MAX);
        file = fopen(input_filename, "rb");
        if (file == NULL)
            printf("ERRO: arquivo não existe!\n");
        else
            break;
    }
    printf("Digite o nome do arquivo de saida da imagem: ");
    get_user_input(output_filename, MAX);

    if (is_png(input_filename)) {
        data = read_png_file(file);
        // allocate space for the PNG output image
        out = (png_bytep*)malloc(sizeof(png_bytep) * height);
        for(int y = 0; y < height; y++) {
            out[y] = (png_byte*)malloc(rowbytes_size);
        }
    } else {
        data = read_jpeg_file(file);
        // allocate space for the JPEG output image
        out = (unsigned char **) malloc(height*sizeof(unsigned char **));
        for (int y = 0; y < height; y++){
            out[y] = malloc(width*3);
        }
    }

    int op;
    printf("%s\n-> ", menu);
    scanf("%d", &op);
    switch (op) {
        case 1:
            rotate_image_clockwise(data, out, (is_png(input_filename))? 4 : 3);
            break;
        case 2:
            rotate_image_anticlockwise(data, out, (is_png(input_filename))? 4 : 3);
            break;
        case 3:
            flip_image_horizontally(data, out, (is_png(input_filename))? 4 : 3);
            break;
        case 4:
            flip_image_vertically(data, out, (is_png(input_filename))? 4 : 3);
            break;
        case 5:
            gray_scalar(data, out, (is_png(input_filename))? 4 : 3);
            break;
        case 6:
            copy_image(input_filename, output_filename, data, out, (is_png(input_filename))? 4 : 3);
            break;
        default:
            printf("Operação inválida!\n");
            break;
    }

    fclose(file); // end reading

    // start writing
    file = fopen(output_filename, "wb");
    if (is_png(input_filename)){
        write_png_file(file, out);
    } else {
        write_jpeg_file(file, out);
    }
    fclose(file); // end writing

    return 0;
}

/*
 * Return the file extension.
 */
char *get_filename_ext(const char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

int is_png(char *filename) {
    if (!strcmp(get_filename_ext(filename), "png"))
        return 1;
    return 0;
}

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
png_bytepp read_png_file(FILE *file) {
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

/*
 * Write the PNG image contents to disk
 */
void write_png_file(FILE *file, png_bytepp data) {
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

/*
 * Copy the src arry into dest array.
 */
void copy_array(png_bytep dest, png_bytep src, int size) {
    for (int i = 0; i < size; i++)
        dest[i] = src[i];
}

void get_user_input(char *str, int size) {
    fgets(str, size, stdin);
    str = strtok(str, "\n");
}

/*
 * bytes_per_pixel: the size of bytes per pixel 3 for JPEG(RGB) or 4 for PNG(RGBA), the same is
 * valid for other functions with this parameter
 *
 * Example:
 *    px1 px2 px3     px7 px4 px1
 *    px4 px5 px6 --> px8 px5 px2 
 *    px7 px8 px9     px9 px6 px3
 */
void rotate_image_clockwise(png_bytepp data, png_bytepp out, int bytes_per_pixel) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*bytes_per_pixel]);
            png_bytep out_pixel = &(out[x][(height-y-1)*bytes_per_pixel]);

            copy_array(out_pixel, pixel, bytes_per_pixel);
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
void rotate_image_anticlockwise(png_bytepp data, png_bytepp out, int bytes_per_pixel) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*bytes_per_pixel]);
            png_bytep out_pixel = &(out[width-x-1][y*bytes_per_pixel]);

            copy_array(out_pixel, pixel, bytes_per_pixel);
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
void flip_image_horizontally(png_bytepp data, png_bytepp out, int bytes_per_pixel) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*bytes_per_pixel]);
            png_bytep out_pixel = &(out[y][(width-x-1)*bytes_per_pixel]);

            copy_array(out_pixel, pixel, bytes_per_pixel);
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
void flip_image_vertically(png_bytepp data, png_bytepp out, int bytes_per_pixel) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*bytes_per_pixel]);
            png_bytep out_pixel = &(out[height-y-1][x*bytes_per_pixel]);

            copy_array(out_pixel, pixel, bytes_per_pixel);
        }
    }
}

/*
 * Transform all image pixels to gray
 */
void gray_scalar(png_bytepp data, png_bytepp out, int bytes_per_pixel) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*bytes_per_pixel]);
            png_bytep out_pixel = &(out[y][x*bytes_per_pixel]);

            png_byte gray = pixel[0]*0.299 + pixel[1]*0.587 + pixel[2]*0.114;
            memset(pixel, gray, 3);

            copy_array(out_pixel, pixel, bytes_per_pixel);
        }
    }
}

/*
 * Return the filename concatenated with "_copia.(png or jpg)"
 */
void copy_image(char filename[], char out_filename[],
        png_bytepp data, png_bytepp out, int bytes_per_pixel) {
    for (int y = 0; y < height; y++) {
        png_bytep row = data[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x*bytes_per_pixel]);
            png_bytep out_pixel = &(out[y][x*bytes_per_pixel]);

            copy_array(out_pixel, pixel, bytes_per_pixel);
        }
    }
    char *name = NULL;
    if (is_png(filename)) {
        name = strtok(filename, ".");
        strcat(name, "_copia.png");
    } else {
        name = strtok(filename, ".");
        strcat(name, "_copia.jpg");
    }
    memcpy(out_filename, name, MAX);
}

/*
 * Read a JPEG image and return a 2D matrix containing the image pixels
 */
unsigned char **read_jpeg_file(FILE *file) {
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	JSAMPROW row_pointer[1];
    unsigned char *row_image = NULL;
	
	unsigned long location = 0;
	
	cinfo.err = jpeg_std_error(&jerr);

    // Allocate and initialize a JPEG decompression object
	jpeg_create_decompress(&cinfo);

    // specify data source
	jpeg_stdio_src(&cinfo, file);

	jpeg_read_header(&cinfo, TRUE);
    width = cinfo.image_width;
    height = cinfo.image_height;

	jpeg_start_decompress(&cinfo);

    // alocate space for the image data
	row_image = (unsigned char *)malloc(cinfo.output_width*cinfo.output_height*cinfo.num_components);
	row_pointer[0] = (unsigned char *)malloc(cinfo.output_width*cinfo.num_components);

    // Read the contents of the image
	while(cinfo.output_scanline < cinfo.image_height) {
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
		for(int i = 0; i < cinfo.image_width*cinfo.num_components; i++) {
            row_image[location++] = row_pointer[0][i];
        }
	}

    // allocate space for the 2d array
    static unsigned char **array_2d = NULL;
    array_2d = (unsigned char **) malloc(cinfo.output_height*sizeof(unsigned char **));
    for (int i = 0; i < height; i++){
        array_2d[i] = malloc(cinfo.output_width*cinfo.num_components);
    }

    // transform raw_image(1D array) to a 2D array
    unsigned long count = 0;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width*cinfo.num_components; j++) {
            array_2d[i][j] = row_image[count++];
        }

	jpeg_finish_decompress(&cinfo);
    // deallocate JPEG compression object
	jpeg_destroy_decompress(&cinfo);
	free(row_pointer[0]);

	return array_2d;
}

/*
 * Write the JPEG image contents to disk
 */
void write_jpeg_file(FILE *file, unsigned char **out) {
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	
	JSAMPROW row_pointer[1];
	
	cinfo.err = jpeg_std_error(&jerr);
    // initialize the JPEG compression object
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);

	cinfo.image_width = width;	
	cinfo.image_height = height;
	cinfo.input_components = 3;   /* or 1 for GRACYSCALE images */
	cinfo.in_color_space = JCS_RGB; /* or JCS_GRAYSCALE for grayscale images */

    // use the library's routine to set default compression parameters.
	jpeg_set_defaults(&cinfo);

	jpeg_start_compress(&cinfo, TRUE);
    unsigned char *temp = NULL;
	temp = (unsigned char*)malloc(cinfo.image_width*cinfo.image_height*cinfo.num_components);

    // transform out(2D array) to a 1D array
    unsigned long count = 0;
    for (int i = 0; i < cinfo.image_height; i++)
        for (int j = 0; j < cinfo.image_width*cinfo.num_components; j++) {
            temp[count++] = out[i][j];
        }

    // write the pixels to file
	while(cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &temp[cinfo.next_scanline*cinfo.image_width*cinfo.input_components];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
    // release JPEG compression object
	jpeg_destroy_compress(&cinfo);
}
