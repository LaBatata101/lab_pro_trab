#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>
#include "image.h"
#include "utils.h"
#include "png_handler.h"
#include "jpeg_handler.h"

const char *argp_program_version = "simp v1.0";

static char doc[] = "S.I.M.P - Simple Image Manipulation Program.";
static char args_doc[] = "OUTPUT_FILE";

static struct argp_option options[] =
{
    {"rotate_left",  'a', "FILE", 0, "Rotate image 90 degrees to the left"},
    {"rotate_rigth", 'b', "FILE", 0, "Rotate image 90 degrees to the rigth"},
    {"mirror_hor",   'c', "FILE", 0, "Mirror the image horizontally"},
    {"mirror_ver",   'd', "FILE", 0, "Mirror the image vertically"},
    {"grey",         'e', "FILE", 0, "Turn image colors to greyscalar"},
    { 0 }
};

struct arguments {
    int rotate_left, rotate_rigth, mirror_hor, mirror_ver, grey_scalar;
    char *output_file;
    char *input_file;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
      case 'a':
          arguments->rotate_left = 1;
          arguments->input_file = arg;
          break;
      case 'b':
          arguments->rotate_rigth = 1;
          arguments->input_file = arg;
          break;
      case 'c':
          arguments->mirror_hor = 1;
          arguments->input_file = arg;
          break;
      case 'd':
          arguments->mirror_ver = 1;
          arguments->input_file = arg;
          break;
      case 'f':
          arguments->grey_scalar = 1;
          arguments->input_file = arg;
          break;
      case ARGP_KEY_NO_ARGS:
          argp_usage(state);
      case ARGP_KEY_ARGS:
          arguments->output_file = state->argv[state->next];
          break;
      default:
          return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void exec_options(struct arguments args, Image image, pixel **buffer) {
    if (args.rotate_rigth) {
        rotate_image_clockwise(image, buffer);
    } else if (args.rotate_left) {
        rotate_image_anticlockwise(image, buffer);
    } else if (args.mirror_hor) {
        flip_image_horizontally(image, buffer);
    } else if (args.mirror_ver) {
        flip_image_vertically(image, buffer);
    } else if (args.grey_scalar) {
        grey_scalar(image, buffer);
    }
}

int main(int argc, char **argv) {
    FILE *file;
    pixel **out_buffer;
    Image image;

    struct arguments arguments;

    arguments.mirror_hor = 0;
    arguments.mirror_ver = 0;
    arguments.rotate_left = 0;
    arguments.rotate_rigth = 0;
    arguments.grey_scalar = 0;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    file = fopen(arguments.input_file, "rb");
    if (file == NULL) {
        printf("ERROR: File doesn't exists!\n");
        exit(EXIT_FAILURE);
    }

    if (endswith(arguments.input_file, "png")) {
        read_png_file(&image, file);
        if (image.width == image.height) {
            // allocate space for the PNG output image
            out_buffer = malloc_image_buffer(image.height, image.width, image.bytes_per_pixel);
        } else {
            out_buffer = malloc_image_buffer(image.width, image.height, image.bytes_per_pixel);
        }
    } else if (endswith(arguments.input_file, "jpg")) {
        read_jpeg_file(&image, file);
        if (image.width == image.height) {
            // allocate space for the JPEG output image
            out_buffer = malloc_image_buffer(image.height, image.width, image.bytes_per_pixel);
        } else {
            out_buffer = malloc_image_buffer(image.width, image.height, image.bytes_per_pixel);
        }
    } else {
        printf("File format not supported \"%s\"\n", get_filename_ext(arguments.input_file));
        return 1;
    }

    fclose(file); // end reading

    exec_options(arguments, image, out_buffer);

    // start writing
    image.buffer = out_buffer;
    file = fopen(arguments.output_file, "wb");
    if (endswith(arguments.input_file, "png")) {
        write_png_file(&image, file);
    } else if (endswith(arguments.input_file, "jpg")) {
        write_jpeg_file(image, file);
    }
    fclose(file); // end writing

    return 0;
}
