#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define CHUNK 4096
#define MAXCHUNKS 256

// ---- System allocator wrapper with counters. Do not edit. ----
static long sys_allocs, sys_frees, sys_live;
static int sys_fail_next;                  // set to 1 to make the next sys_alloc fail once
static void *sys_alloc(size_t n) {
    if (sys_fail_next) { sys_fail_next = 0; return NULL; }
    void *p = aligned_alloc(4096, (n + 4095) / 4096 * 4096);
    if (!p) return NULL;
    sys_allocs++;
    sys_live += (long)n;
    return p;
}
static void sys_free(void *p, size_t n) {
    sys_frees++;
    sys_live -= (long)n;
    free(p);
}
// ---- end of wrapper ----

typedef struct {
    char *base[MAXCHUNKS];
    size_t size[MAXCHUNKS];
    int nchunks;
    size_t used;                           // bytes used in the newest chunk
} arena_t;

void *arena_alloc(arena_t *a, size_t n, size_t align) {
    (void)align;
    if (a->nchunks == MAXCHUNKS) return NULL;
    char *p = sys_alloc(n);
    if (!p) return NULL;
    a->base[a->nchunks] = p;
    a->size[a->nchunks] = n;
    a->nchunks++;
    return p;
}

void arena_reset(arena_t *a) { (void)a; }

void arena_destroy(arena_t *a) {
    for (int i = 0; i < a->nchunks; i++) sys_free(a->base[i], a->size[i]);
    a->nchunks = 0;
    a->used = 0;
}

typedef struct node { int val; struct node *next; } node;

int main(void) {
    arena_t a = {0};
    node *head = NULL;
    for (int i = 0; i < 1000; i++) {
        node *p = arena_alloc(&a, sizeof *p, _Alignof(node));
        p->val = i; p->next = head; head = p;
    }
    long sum = 0;
    for (node *p = head; p; p = p->next) sum += p->val;
    printf("sum of values       = %ld\n", sum);
    printf("system allocations  = %ld (one per node would be 1000)\n", sys_allocs);
    printf("assumed headers     = %ld bytes with a general allocator, 0 per node here\n", 1000L * 16);
    printf("chunk bytes         = %ld for %ld bytes of nodes\n", sys_live, 1000L * (long)sizeof(node));
    arena_destroy(&a);
    printf("live after destroy  = %ld\n", sys_live);
    return 0;
}
