#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUCKETS 16

typedef struct Entry {
    char *key;
    int value;
    struct Entry *next;
} Entry;

typedef struct {
    Entry *buckets[BUCKETS];
} HashTable;

unsigned long hash_str(const char *s) {
    unsigned long h = 5381;
    for (; *s != '\0'; s++) {
        h = h * 33 + (unsigned char)*s;
    }
    return h;
}

void ht_init(HashTable *t) {
}

int ht_put(HashTable *t, const char *key, int value) {
    return -1;
}

int ht_get(const HashTable *t, const char *key, int *out) {
    return 0;
}

void ht_free(HashTable *t) {
}

// Optional: try them with Run.
int main(void) {
    HashTable t;
    ht_init(&t);
    ht_put(&t, "alice", 31);
    int age = 0;
    if (ht_get(&t, "alice", &age)) printf("alice is %d\n", age);
    ht_free(&t);
    return 0;
}
