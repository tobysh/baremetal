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
    if (cap == 0) return NULL;
    Lru *c = calloc(1, sizeof *c);
    if (c == NULL) return NULL;
    c->nb = 2 * cap + 1;
    c->buckets = calloc(c->nb, sizeof *c->buckets);
    if (c->buckets == NULL) {
        free(c);
        return NULL;
    }
    c->cap = cap;
    return c;
}

static size_t bucket_of(const Lru *c, int key) {
    return (unsigned)key % c->nb;
}

static LNode *find(const Lru *c, int key) {
    for (LNode *n = c->buckets[bucket_of(c, key)]; n; n = n->hnext)
        if (n->key == key) return n;
    return NULL;
}

static void list_unlink(Lru *c, LNode *n) {
    if (n->prev) n->prev->next = n->next; else c->head = n->next;
    if (n->next) n->next->prev = n->prev; else c->tail = n->prev;
    n->prev = n->next = NULL;
}

static void list_push_front(Lru *c, LNode *n) {
    n->prev = NULL;
    n->next = c->head;
    if (c->head) c->head->prev = n; else c->tail = n;
    c->head = n;
}

static void chain_remove(Lru *c, LNode *n) {
    LNode **link = &c->buckets[bucket_of(c, n->key)];
    while (*link && *link != n) link = &(*link)->hnext;
    if (*link) *link = n->hnext;
}

int lru_get(Lru *c, int key, int *out) {
    LNode *n = find(c, key);
    if (n == NULL) return -1;
    list_unlink(c, n);
    list_push_front(c, n);
    *out = n->val;
    return 0;
}

void lru_put(Lru *c, int key, int val) {
    LNode *n = find(c, key);
    if (n) {
        n->val = val;
        list_unlink(c, n);
        list_push_front(c, n);
        return;
    }
    n = malloc(sizeof *n);
    if (n == NULL) return;
    n->key = key;
    n->val = val;
    size_t b = bucket_of(c, key);
    n->hnext = c->buckets[b];
    c->buckets[b] = n;
    list_push_front(c, n);
    c->count++;
    if (c->count > c->cap) {
        LNode *victim = c->tail;
        list_unlink(c, victim);
        chain_remove(c, victim);
        free(victim);
        c->count--;
    }
}

size_t lru_size(const Lru *c) {
    return c->count;
}

int lru_oldest(const Lru *c, int *key) {
    if (c->tail == NULL) return -1;
    *key = c->tail->key;
    return 0;
}

void lru_free(Lru *c) {
    if (c == NULL) return;
    LNode *n = c->head;
    while (n) {
        LNode *next = n->next;
        free(n);
        n = next;
    }
    free(c->buckets);
    free(c);
}

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
