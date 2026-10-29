#include <stddef.h>
#include <stdint.h>
typedef struct { uint8_t data[16]; unsigned head,count; int closed; } Pipe;
void pipe_init(Pipe *p) { p->head=p->count=0; p->closed=0; }
int pipe_write(Pipe *p,const void *src,size_t n) { if(p->closed) return -1; const uint8_t *s=src; size_t i=0; while(i<n && p->count<16) { p->data[(p->head+p->count)%16]=s[i++]; p->count++; } return (int)i; }
int pipe_read(Pipe *p,void *dst,size_t n) { if(!n) return 0; if(!p->count) return p->closed?0:-1; uint8_t *d=dst; size_t i=0; while(i<n && p->count) { d[i++]=p->data[p->head]; p->head=(p->head+1)%16; p->count--; } return (int)i; }
void pipe_close_writer(Pipe *p) { p->closed=1; }
