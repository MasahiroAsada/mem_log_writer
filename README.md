# Memory Log Writer
Memory Log Writer allocates the large memory, writes data like the way of file stream, and saves it to the file.
It is expected to be used as a fast log writer.

## Requirements
This software is expected to be used in Linux/GCC environment.  It calls the following system calls or library functions to create a memory mapped file.
- open
- close
- mmap
- munmap
- getpagesize
- ftruncate
- msync
