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
    return -1;
}

int oa_put(OAMap *m, int key, int val) {
    return -1;
}

int oa_get(const OAMap *m, int key, int *out) {
    return -1;
}

int oa_del(OAMap *m, int key) {
    return -1;
}

void oa_free(OAMap *m) {
}

// Optional: try it with Run.
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
