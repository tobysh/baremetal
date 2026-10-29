#include <stddef.h>
enum { READY, BLOCKED };
typedef struct { int id, state; } Task;
typedef struct { Task *items[8]; unsigned head, count; } WaitQueue;
void wait_init(WaitQueue *q) { q->head=q->count=0; }
int sleep_on(WaitQueue *q, Task *t) { if(t->state!=READY || q->count==8) return -1; q->items[(q->head+q->count)%8]=t; q->count++; t->state=BLOCKED; return 0; }
Task *wake_one(WaitQueue *q) { if(!q->count) return NULL; Task *t=q->items[q->head]; q->head=(q->head+1)%8; q->count--; t->state=READY; return t; }
