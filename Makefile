TARGET := example
CC := gcc
CFLGAS := -Wall -g -O0

$(TARGET): example.c mem_log_writer.c mem_log_writer.h
	$(CC) $(CFLGAS) -o $(TARGET) example.c mem_log_writer.c
