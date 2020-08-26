#ifndef _MEM_LOG_WRITER_H_
#define _MEM_LOG_WRITER_H_

#include <stdint.h>

typedef struct MLW_FILE MLW_FILE;

MLW_FILE *mlw_open(const char *path, uint64_t column_length, uint64_t row_length);
int64_t mlw_column_length(MLW_FILE *f);
int64_t mlw_row_length(MLW_FILE *f);
int mlw_write(MLW_FILE *f, const uint64_t *data_array);
int64_t mlw_available(MLW_FILE *f);
int mlw_set_index(MLW_FILE *f, const char **index_array);
int mlw_close(MLW_FILE *f);

#endif
