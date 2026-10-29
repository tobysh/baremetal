#include <stddef.h>
enum { READY, BLOCKED };
typedef struct { int id, state; } Task;
typedef struct { Task *items[8]; unsigned head, count; } WaitQueue;
void wait_init(WaitQueue *q) {}
int sleep_on(WaitQueue *q, Task *t) { return -1; }
Task *wake_one(WaitQueue *q) { return NULL; }
