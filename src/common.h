#ifndef COMMON_H
#define COMMON_H

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef long i64;
typedef int i32;
typedef short i16;
typedef char i8;
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
