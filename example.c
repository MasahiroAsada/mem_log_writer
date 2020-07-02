#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem_log_writer.h"

#define DEFAULT_FILE_SIZE 1000

#define ERR_EXIT(format, ...) \
do { \
	fprintf(stderr, format, ##__VA_ARGS__); \
	exit(EXIT_FAILURE); \
} while(0);

int main(int argc, char **argv)
{
	const char *file_path;
	size_t file_size = DEFAULT_FILE_SIZE;
	unsigned int i = 0;
	if (argc < 2) {
		ERR_EXIT("Usage: %s <output_file> [file_size]\n", argv[0]);
	}
	file_path = argv[1];
	if (argc >= 3) {
		file_size = atoll(argv[2]);
	}
	MLW_FILE *f = mlw_open(file_path, file_size);
	if (!f) {
		ERR_EXIT("Error: Cannot open file %s.\n", file_path);
	}
	while (mlw_available(f) > 0) {
		const char *str = "Test writing a string\n";
		const size_t str_len = strlen(str);
		ssize_t res;
		res = mlw_write(f, str, str_len);
		if (res < 0) {
			ERR_EXIT("Error: Cannot write data by mlw_write().\n");
		}
		res = mlw_fputs(f, str);
		if (res < 0) {
			ERR_EXIT("Error: Cannot write data by mlw_fputs().\n");
		}
		res = mlw_fprintf(f, "Test writing a string i=%u\n", i++);
		if (res < 0) {
			ERR_EXIT("Error: Cannot write data by mlw_fprintf().\n");
		}
	}
	mlw_close(f);
	return 0;
}
