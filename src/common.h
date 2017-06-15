#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;
typedef float f32;
typedef double f64;

#define ARRAY_SIZE(x) sizeof(x) / sizeof(*x)
#define STRUCT_OFFSET(st, m) ((size_t)(&((st *)0)->m))
#define COMPRESS_THREE(x, y, z, x_max, y_max) ((z) * (x_max) * (y_max)) + ((y) * (x_max)) + (x)
#define COMPRESS_TWO(x, y, x_max) ((y) * (x_max)) + (x)

// Allocates a string, must be freed by user
char *file_to_string(const char *filename) {
	FILE *file = fopen(filename, "r");

	if (file == NULL) {
		printf("%s not found!\n", filename);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	u64 length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *file_string = (char *)malloc(length + 1);
	length = fread(file_string, 1, length, file);
	file_string[length] = 0;

	fclose(file);
	return file_string;
}

#endif
