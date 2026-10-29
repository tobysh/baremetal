#include <stddef.h>
#include <stdint.h>
static uint8_t file_data[32];
typedef struct { int open; size_t offset; } Descriptor;
static Descriptor fds[4];
void files_init(void) { for(unsigned i=0;i<32;i++) file_data[i]=i; for(unsigned i=0;i<4;i++) fds[i]=(Descriptor){0,0}; }
int file_open(void) { for(int i=0;i<4;i++) if(!fds[i].open) { fds[i]=(Descriptor){1,0}; return i; } return -1; }
int file_read(int fd, void *dst, size_t n) { if(fd<0 || fd>=4 || !fds[fd].open) return -1; size_t left=32-fds[fd].offset; if(n>left) n=left; uint8_t *d=dst; for(size_t i=0;i<n;i++) d[i]=file_data[fds[fd].offset+i]; fds[fd].offset+=n; return (int)n; }
int file_close(int fd) { if(fd<0 || fd>=4 || !fds[fd].open) return -1; fds[fd].open=0; return 0; }
