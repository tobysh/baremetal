#include <stddef.h>
#include <stdint.h>
static unsigned char arena[256] __attribute__((aligned(8)));
typedef struct { size_t off, len; int busy; } Block;
static Block blocks[32]; static unsigned count;
void heap_init(void) { count=1; blocks[0]=(Block){0,256,0}; }
void *heap_alloc(size_t n) { if (!n || n>256) return NULL; n=(n+7)&~(size_t)7; for(unsigned i=0;i<count;i++) { if(blocks[i].busy || blocks[i].len<n) continue; if(blocks[i].len>n) { if(count==32) return NULL; for(unsigned j=count;j>i+1;j--) blocks[j]=blocks[j-1]; blocks[i+1]=(Block){blocks[i].off+n,blocks[i].len-n,0}; count++; blocks[i].len=n; } blocks[i].busy=1; return arena+blocks[i].off; } return NULL; }
int heap_free(void *p) { for(unsigned i=0;i<count;i++) if(p==arena+blocks[i].off && blocks[i].busy) { blocks[i].busy=0; if(i && !blocks[i-1].busy) { blocks[i-1].len+=blocks[i].len; for(unsigned j=i;j+1<count;j++) blocks[j]=blocks[j+1]; count--; i--; } if(i+1<count && !blocks[i+1].busy) { blocks[i].len+=blocks[i+1].len; for(unsigned j=i+1;j+1<count;j++) blocks[j]=blocks[j+1]; count--; } return 0; } return -1; }
