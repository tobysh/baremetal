#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Entry {
    char *key;
    int val;
    struct Entry *next;
} Entry;

typedef struct {
    Entry **buckets;
    size_t nbuckets;
    size_t count;
} StrMap;

static unsigned long str_hash(const char *s) {
    unsigned long h = 5381;
    for (const char *p = s; *p; p++) h = h * 33 + (unsigned char)*p;
    return h;
}

int sm_init(StrMap *m, size_t nbuckets) {
    return -1;
}

int sm_put(StrMap *m, const char *key, int val) {
    return -1;
}

int sm_get(const StrMap *m, const char *key, int *out) {
    return -1;
}

int sm_del(StrMap *m, const char *key) {
    return -1;
}

void sm_free(StrMap *m) {
}

// Optional: try it with Run.
int main(void) {
    StrMap m;
    sm_init(&m, 4);
    sm_put(&m, "apple", 3);
    sm_put(&m, "pear", 5);
    int v = 0;
    if (sm_get(&m, "pear", &v) == 0) printf("pear -> %d (expected 5)\n", v);
    sm_free(&m);
    return 0;
}
