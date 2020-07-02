#include "mem_log_writer.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define __inline__ inline __attribute__((always_inline))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct MLW_FILE {
	int fd;
	char *map;
	size_t offset;
	size_t size;
} MLW_FILE;

static __inline__ size_t
write_buf(MLW_FILE *f, const void *buf, size_t count)
{
	const size_t n_write = MIN(f->size - f->offset, count);
	memcpy((void *)(f->map + f->offset), buf, n_write);
	f->offset += n_write;
	return (ssize_t)n_write;
}

/**
 * Open a memory mapped file.
 * Actual file size is rounded up to the integral multiple of page size.
 * 
 * @param path File path to be written
 * @param max_size Maximum file size
 * @return File handle, NULL if an error occurs.
 */
MLW_FILE*
mlw_open(const char *path, size_t max_size)
{
	MLW_FILE *f = NULL;
	int page_size;

	if (!path) return NULL;
	if (max_size == 0u) return NULL;
	f = malloc(sizeof(MLW_FILE));
	if (!f) return NULL;
	f->offset = 0u;
	f->size = max_size;

	f->fd = open(
		path,
		O_CREAT | O_RDWR | O_TRUNC,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (f->fd == -1) {
		free(f);
		return NULL;
	}

	page_size = getpagesize();
	if (f->size % page_size > 0) {
		f->size = (f->size / page_size + 1) * page_size;
	}
	if (ftruncate(f->fd, f->size) == -1) {
		free(f);
		return NULL;
	}

	f->map = (char*)mmap(NULL, f->size, PROT_WRITE, MAP_SHARED, f->fd, 0);
	if (f->map == MAP_FAILED) {
		close(f->fd);
		free(f);
		return NULL;
	}

	return f;
}

/**
 * Write binary data to the file.
 * 
 * @param f File handle
 * @param buf Data to be written
 * @param count Data size
 * @return Data size actually written, -1 if an error occurs.
 */
ssize_t mlw_write(MLW_FILE *f, const void *buf, size_t count)
{
	if (!f) return -1;
	if (!buf) return -1;

	return write_buf(f, buf, count);
}

/**
 * Write a string to the file.
 * Newline code is not outputted automatically.
 * 
 * @param f File handle
 * @param str String to be written
 * @return Data size actually written, -1 if an error occurs.
 */
ssize_t mlw_fputs(MLW_FILE *f, const char *str)
{
	if (!f) return -1;
	if (!str) return -1;

	return write_buf(f, str, strlen(str));
}

/**
 * Write a string with a format to the file.
 * 
 * @param f File handle
 * @param format String format, its specification follows printf() series.
 * @return Data size actually written, -1 if an error occurs.
 */
ssize_t mlw_fprintf(MLW_FILE *f, const char *format, ...)
{
	int size = 0;
	char *p = NULL;
	va_list ap;
	size_t ret;
	if (!f) return -1;
	if (!format) return -1;
	
	va_start(ap, format);
	size = vsnprintf(p, size, format, ap);
	va_end(ap);
	if (size < 0) {
		return -1;
	}
	size++;  /* For '\0' */
	p = malloc(size);
	if (!p) {
		return -1;
	}
	va_start(ap, format);
	size = vsnprintf(p, size, format, ap);
	va_end(ap);
	if (size < 0) {
		free(p);
		return -1;
	}

	ret = write_buf(f, p, size);
	free(p);
	return ret;
}

/**
 * Get the remaining data size which can be written.
 * 
 * @param f File handle
 * @return The remaining size, -1 if an error occurs.
 */
ssize_t
mlw_available(MLW_FILE *f)
{
	if (!f) return -1;
	return f->size - f->offset;
}

/**
 * Close the file.
 * 
 * @param f File handle
 */
void
mlw_close(MLW_FILE *f)
{
	ftruncate(f->fd, f->offset);
	msync(f->map, f->offset, MS_SYNC);
	munmap(f->map, f->offset);
	close(f->fd);
	free(f);
}
