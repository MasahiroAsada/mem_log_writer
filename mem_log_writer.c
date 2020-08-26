#include "mem_log_writer.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define __inline__ inline __attribute__((always_inline))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define TMPFILE_PATH_MAXLEN 32
#define TMPFILE_PATH_TEMPLATE "/tmp/mlwXXXXXX"

typedef struct MLW_FILE {
	FILE *fp_out;
	char path_map[TMPFILE_PATH_MAXLEN];
	int fd_map;
	char *map;
	size_t offset;
	size_t size;
	uint64_t column_length;
	uint64_t row_length;
	uint64_t row_remain;
	const char **index_array;
} MLW_FILE;

static __inline__ int
write_buf(MLW_FILE *f, const uint64_t *buf)
{
	const size_t nb_write = sizeof(uint64_t) * f->column_length;
	if (f->row_remain == 0) return -1;
	memcpy((void *)(f->map + f->offset), buf, nb_write);
	f->offset += nb_write;
	f->row_remain--;
	return 1;
}

/**
 * Create a writer instance.
 * 
 * @param path File path to be written
 * @param column_length Number of column (0 < x< 2^63)
 * @param row_length Number of row (0 < x < 2^63)
 * @return File handle, NULL if an error occurs.
 */
MLW_FILE *
mlw_open(const char *path, uint64_t column_length, uint64_t row_length)
{
	MLW_FILE *f = NULL;
	int page_size;

	if (!path) return NULL;
	if (column_length == 0) return NULL;
	if (column_length >= (1ull << 63)) return NULL;
	if (row_length == 0) return NULL;
	if (row_length >= (1ull << 63)) return NULL;
	f = malloc(sizeof(MLW_FILE));
	if (!f) return NULL;
	f->offset = 0;
	f->size = sizeof(uint64_t) * column_length * row_length;
	f->column_length = column_length;
	f->row_length = row_length;
	f->row_remain = row_length;
	f->index_array = NULL;

	f->fp_out = fopen(path, "w");
	if (!f->fp_out) {
		free(f);
		return NULL;
	}

	strncpy(f->path_map, TMPFILE_PATH_TEMPLATE, sizeof(f->path_map));
	f->fd_map = mkstemp(f->path_map);
	if (f->fd_map == -1) {
		free(f);
		return NULL;
	}

	page_size = getpagesize();
	if (f->size % page_size > 0) {
		f->size = (f->size / page_size + 1) * page_size;
	}
	if (ftruncate(f->fd_map, f->size) == -1) {
		free(f);
		return NULL;
	}

	f->map = (char*)mmap(NULL, f->size, PROT_WRITE, MAP_SHARED, f->fd_map, 0);
	if (f->map == MAP_FAILED) {
		close(f->fd_map);
		free(f);
		return NULL;
	}

	return f;
}

/**
 * Get the number of column of the file.
 * 
 * @param f File handle
 * @return The number of column, -1 if an error occurs.
 */
int64_t
mlw_column_length(MLW_FILE *f)
{
	if (!f) return -1;
	return (int64_t)f->column_length;
}

/**
 * Get the number of row of the file.
 * 
 * @param f File handle
 * @return The number of row, -1 if an error occurs.
 */
int64_t
mlw_row_length(MLW_FILE *f)
{
	if (!f) return -1;
	return (int64_t)f->row_length;
}

/**
 * Write data array to the buffer.
 * 
 * @param f File handle
 * @param data_array Data array to be written
 * @return 0 if data are successfully written, -1 if an error occurs.
 */
int
mlw_write(MLW_FILE *f, const uint64_t *data_array)
{
	if (!f) return -1;
	if (!data_array) return -1;

	return write_buf(f, data_array);
}

/**
 * Get the number of remaining row which can be written.
 * 
 * @param f File handle
 * @return The number of remaining row, -1 if an error occurs.
 */
int64_t
mlw_available(MLW_FILE *f)
{
	if (!f) return -1;
	return f->row_remain;
}

/**
 * Set the index array.
 * 
 * @param f File handle
 * @param index_array Index array, the number of index must be equal to column_length
 * @return 0 if successfully set, -1 if an error occurs.
 */
int
mlw_set_index(MLW_FILE *f, const char **index_array)
{
	if (!f) return -1;
	if (!index_array) return -1;
	f->index_array = index_array;
	return 0;
}

/**
 * Flush buffer and close the file.
 * 
 * @param f File handle
 * @return 0 if data are successfully written, -1 if an error occurs.
 */
int
mlw_close(MLW_FILE *f)
{
	const char DELIM[] = ",";
	uint64_t n_write;

	if (!f) return -1;

	/* flush */
	if (f->index_array) {
		for (int64_t i = 0; i < f->column_length; i++) {
			const char *name = f->index_array[i];
			fprintf(f->fp_out, "%s%s",
				name, (i < f->column_length - 1) ? DELIM : "");
		}
		fputc('\n', f->fp_out);
	}
	n_write = f->row_length - f->row_remain;
	for (uint64_t i = 0; i < n_write; i++) {
		const uint64_t *buf = &((uint64_t *)f->map)[f->column_length * i];
		for (uint64_t j = 0; j < f->column_length; j++) {
			fprintf(f->fp_out, "%"PRIu64"%s",
				buf[j], (j < f->column_length - 1) ? DELIM : "");
		}
		fputc('\n', f->fp_out);
	}

	fclose(f->fp_out);
	close(f->fd_map);
	unlink(f->path_map);
	free(f);
	return 0;
}
