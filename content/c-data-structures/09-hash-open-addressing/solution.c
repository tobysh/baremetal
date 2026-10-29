#include <stdio.h>
#include <stdlib.h>

typedef enum { EMPTY, USED, DELETED } State;

typedef struct {
    int key;
    int val;
    State state;
} Slot;

typedef struct {
    Slot *slots;
    size_t cap;
    size_t count;
} OAMap;

static size_t oa_hash(int key) {
    return (size_t)((unsigned)key * 2654435761u);
}

int oa_init(OAMap *m, size_t cap) {
    m->slots = NULL;
    m->cap = 0;
    m->count = 0;
    if (cap < 2 || (cap & (cap - 1)) != 0) return -1;
    m->slots = calloc(cap, sizeof *m->slots);
    if (m->slots == NULL) return -1;
    m->cap = cap;
    return 0;
}

static long find_slot(const OAMap *m, int key) {
    size_t i = oa_hash(key) & (m->cap - 1);
    for (size_t step = 0; step < m->cap; step++) {
        if (m->slots[i].state == EMPTY) return -1;
        if (m->slots[i].state == USED && m->slots[i].key == key) return (long)i;
        i = (i + 1) & (m->cap - 1);
    }
    return -1;
}

int oa_put(OAMap *m, int key, int val) {
    long f = find_slot(m, key);
    if (f >= 0) {
        m->slots[f].val = val;
        return 0;
    }
    size_t i = oa_hash(key) & (m->cap - 1);
    for (size_t step = 0; step < m->cap; step++) {
        if (m->slots[i].state != USED) {
            m->slots[i].key = key;
            m->slots[i].val = val;
            m->slots[i].state = USED;
            m->count++;
            return 0;
        }
        i = (i + 1) & (m->cap - 1);
    }
    return -1;
}

int oa_get(const OAMap *m, int key, int *out) {
    long f = find_slot(m, key);
    if (f < 0) return -1;
    *out = m->slots[f].val;
    return 0;
}

int oa_del(OAMap *m, int key) {
    long f = find_slot(m, key);
    if (f < 0) return -1;
    m->slots[f].state = DELETED;
    m->count--;
    return 0;
}

void oa_free(OAMap *m) {
    free(m->slots);
    m->slots = NULL;
    m->cap = 0;
    m->count = 0;
}

int main(void) {
    OAMap m;
    oa_init(&m, 8);
    oa_put(&m, 10, 100);
    oa_put(&m, 20, 200);
    int v = 0;
    if (oa_get(&m, 20, &v) == 0) printf("20 -> %d (expected 200)\n", v);
    oa_free(&m);
    return 0;
}
