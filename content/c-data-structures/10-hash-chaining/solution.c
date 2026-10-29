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
    m->buckets = NULL;
    m->nbuckets = 0;
    m->count = 0;
    if (nbuckets == 0) return -1;
    m->buckets = calloc(nbuckets, sizeof *m->buckets);
    if (m->buckets == NULL) return -1;
    m->nbuckets = nbuckets;
    return 0;
}

static Entry *lookup(const StrMap *m, const char *key) {
    for (Entry *e = m->buckets[str_hash(key) % m->nbuckets]; e; e = e->next)
        if (strcmp(e->key, key) == 0) return e;
    return NULL;
}

static int grow(StrMap *m) {
    size_t new_n = m->nbuckets * 2;
    Entry **nb = calloc(new_n, sizeof *nb);
    if (nb == NULL) return -1;
    for (size_t b = 0; b < m->nbuckets; b++) {
        Entry *e = m->buckets[b];
        while (e) {
            Entry *next = e->next;
            size_t nbi = str_hash(e->key) % new_n;
            e->next = nb[nbi];
            nb[nbi] = e;
            e = next;
        }
    }
    free(m->buckets);
    m->buckets = nb;
    m->nbuckets = new_n;
    return 0;
}

int sm_put(StrMap *m, const char *key, int val) {
    Entry *e = lookup(m, key);
    if (e) {
        e->val = val;
        return 0;
    }
    e = malloc(sizeof *e);
    if (e == NULL) return -1;
    e->key = malloc(strlen(key) + 1);
    if (e->key == NULL) {
        free(e);
        return -1;
    }
    strcpy(e->key, key);
    e->val = val;
    size_t b = str_hash(key) % m->nbuckets;
    e->next = m->buckets[b];
    m->buckets[b] = e;
    m->count++;
    if (m->count > 2 * m->nbuckets) return grow(m);
    return 0;
}

int sm_get(const StrMap *m, const char *key, int *out) {
    Entry *e = lookup(m, key);
    if (e == NULL) return -1;
    *out = e->val;
    return 0;
}

int sm_del(StrMap *m, const char *key) {
    Entry **link = &m->buckets[str_hash(key) % m->nbuckets];
    while (*link && strcmp((*link)->key, key) != 0) link = &(*link)->next;
    if (*link == NULL) return -1;
    Entry *dead = *link;
    *link = dead->next;
    free(dead->key);
    free(dead);
    m->count--;
    return 0;
}

void sm_free(StrMap *m) {
    for (size_t b = 0; b < m->nbuckets; b++) {
        Entry *e = m->buckets[b];
        while (e) {
            Entry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(m->buckets);
    m->buckets = NULL;
    m->nbuckets = 0;
    m->count = 0;
}

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
