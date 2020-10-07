#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

#define STR_SIZE 512

bool endswith(char *str, char *end);
char *get_filename_ext(const char *filename);

#endif
