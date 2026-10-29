#include <stdio.h>
#include <stdlib.h>

typedef struct LNode {
    int key;
    int val;
    struct LNode *prev;
    struct LNode *next;
    struct LNode *hnext;
} LNode;

typedef struct {
    size_t cap;
    size_t count;
    LNode *head;
    LNode *tail;
    LNode **buckets;
    size_t nb;
} Lru;

Lru *lru_new(size_t cap) {
    return NULL;
}

int lru_get(Lru *c, int key, int *out) {
    return -1;
}

void lru_put(Lru *c, int key, int val) {
}

size_t lru_size(const Lru *c) {
    return 0;
}

int lru_oldest(const Lru *c, int *key) {
    return -1;
}

void lru_free(Lru *c) {
}

// Optional: try it with Run.
int main(void) {
    Lru *c = lru_new(2);
    if (c == NULL) {
        printf("lru_new is not implemented yet\n");
        return 0;
    }
    lru_put(c, 1, 10);
    lru_put(c, 2, 20);
    int v;
    lru_get(c, 1, &v);
    lru_put(c, 3, 30);
    printf("key 2 evicted: %s (expected yes)\n", lru_get(c, 2, &v) == -1 ? "yes" : "no");
    lru_free(c);
    return 0;
}
