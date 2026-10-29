#include <stddef.h>
#include <stdint.h>
typedef struct { uint8_t data[16]; unsigned head,count; int closed; } Pipe;
void pipe_init(Pipe *p) {}
int pipe_write(Pipe *p, const void *src, size_t n) { return -1; }
int pipe_read(Pipe *p, void *dst, size_t n) { return -1; }
void pipe_close_writer(Pipe *p) {}
