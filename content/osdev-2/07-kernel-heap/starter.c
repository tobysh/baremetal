#include <stddef.h>
#include <stdint.h>
static unsigned char arena[256] __attribute__((aligned(8)));
typedef struct { size_t off, len; int busy; } Block;
static Block blocks[32]; static unsigned count;
void heap_init(void) {}
void *heap_alloc(size_t n) { return NULL; }
int heap_free(void *p) { return -1; }
