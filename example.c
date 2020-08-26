#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "mem_log_writer.h"

#define DEFAULT_COLUMN_LENGTH  10
#define DEFAULT_ROW_LENGTH     1000

#define ERR_EXIT(format, ...) \
do { \
	fprintf(stderr, format, ##__VA_ARGS__); \
	exit(EXIT_FAILURE); \
} while(0);

static void
free_index_array(char **index_array, uint64_t length) {
	if (!index_array) return;
	for (uint64_t i = 0; i < length; i++) {
		free(index_array[i]);
	}
	free(index_array);
}

static char **
make_index_array(uint64_t length) {
	const size_t index_buflen = 16;
	char **index_array = malloc(sizeof(char*) * length);
	if (!index_array) return NULL;
	for (uint64_t i = 0; i < length; i++) {
		index_array[i] = malloc(index_buflen);
		if (index_array[i] == NULL) {
			free_index_array(index_array, i);
			return NULL;
		}
		snprintf(index_array[i], index_buflen, "index%"PRIu64, i);
	}
	return index_array;
}

int
main(int argc, char **argv)
{
	const char *file_path;
	uint64_t column_length = DEFAULT_COLUMN_LENGTH;
	uint64_t row_length = DEFAULT_ROW_LENGTH;
	int64_t res;
	MLW_FILE *f = NULL;
	uint64_t counter = 0, n_wrote;
	char **index_array = NULL;

	if (argc < 2) {
		ERR_EXIT("Usage: %s <output_file> [column_length] [row_length]\n", argv[0]);
	}
	file_path = argv[1];
	if (argc >= 3) {
		column_length = atoll(argv[2]);
	}
	if (argc >= 4) {
		row_length = atoll(argv[3]);
	}
	f = mlw_open(file_path, column_length, row_length);
	if (!f) {
		ERR_EXIT("Error: Cannot open file %s.\n", file_path);
	}
	printf("File path: %s\n", file_path);

	res = mlw_column_length(f);
	if ((uint64_t)res != column_length) {
		ERR_EXIT("Error: Cannot get correct column length.\n");
	}
	printf("Column length: %"PRIu64"\n", column_length);
	res = mlw_row_length(f);
	if ((uint64_t)res != row_length) {
		ERR_EXIT("Error: Cannot get correct row lendth.\n");
	}
	printf("Row length: %"PRIu64"\n", row_length);

	n_wrote = 0;
	while (mlw_available(f) > 0) {
		uint64_t data_array[column_length];
		for (uint64_t i = 0; i < column_length; i++) {
			data_array[i] = counter++;
		}
		res = mlw_write(f, data_array);
		if (res < 0) {
			ERR_EXIT("Error: Cannot write data by mlw_write().\n");
		}
		n_wrote++;
	}
	if (n_wrote != row_length) {
		ERR_EXIT("Error: The number of written array is inconsistent.\n");
	}

	index_array = make_index_array(column_length);
	res = mlw_set_index(f, (const char **)index_array);
	if (res < 0) {
		ERR_EXIT("Error: Cannot set index.\n");
	}

	res = mlw_close(f);
	if (res < 0) {
		ERR_EXIT("Error: Cannot close successfully.\n");
	}
	free_index_array(index_array, column_length);

	return 0;
}
