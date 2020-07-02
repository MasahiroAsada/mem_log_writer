#ifndef _MEM_LOG_WRITER_H_
#define _MEM_LOG_WRITER_H_

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

typedef struct MLW_FILE MLW_FILE;

MLW_FILE *mlw_open(const char *path, size_t max_size);
ssize_t mlw_write(MLW_FILE *f, const void *buf, size_t count);
ssize_t mlw_fputs(MLW_FILE *f, const char *str);
ssize_t mlw_fprintf(MLW_FILE *f, const char *format, ...);
ssize_t mlw_available(MLW_FILE *f);
void mlw_close(MLW_FILE *f);

#endif
