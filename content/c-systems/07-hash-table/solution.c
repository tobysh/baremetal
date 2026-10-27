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
    for (int i = 0; i < BUCKETS; i++) {
        t->buckets[i] = NULL;
    }
}

int ht_put(HashTable *t, const char *key, int value) {
    size_t b = hash_str(key) % BUCKETS;
    for (Entry *e = t->buckets[b]; e != NULL; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            e->value = value;
            return 0;
        }
    }
    Entry *e = malloc(sizeof *e);
    if (e == NULL) {
        return -1;
    }
    size_t len = strlen(key);
    e->key = malloc(len + 1);
    if (e->key == NULL) {
        free(e);
        return -1;
    }
    memcpy(e->key, key, len + 1);
    e->value = value;
    e->next = t->buckets[b];
    t->buckets[b] = e;
    return 0;
}

int ht_get(const HashTable *t, const char *key, int *out) {
    size_t b = hash_str(key) % BUCKETS;
    for (Entry *e = t->buckets[b]; e != NULL; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            *out = e->value;
            return 1;
        }
    }
    return 0;
}

void ht_free(HashTable *t) {
    for (int i = 0; i < BUCKETS; i++) {
        Entry *e = t->buckets[i];
        while (e != NULL) {
            Entry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
        t->buckets[i] = NULL;
    }
}

int main(void) {
    HashTable t;
    ht_init(&t);
    ht_put(&t, "alice", 31);
    int age = 0;
    if (ht_get(&t, "alice", &age)) printf("alice is %d\n", age);
    ht_free(&t);
    return 0;
}
