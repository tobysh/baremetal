#include <stdio.h>
#include <stdlib.h>

typedef struct Trie {
    struct Trie *child[26];
    int end;
} Trie;

Trie *trie_new(void) {
    return NULL;
}

int trie_insert(Trie *root, const char *word) {
    return -1;
}

int trie_contains(const Trie *root, const char *word) {
    return 0;
}

int trie_count_prefix(const Trie *root, const char *prefix) {
    return -1;
}

int trie_has_prefix(const Trie *root, const char *prefix) {
    return 0;
}

void trie_free(Trie *root) {
}

// Optional: try it with Run.
int main(void) {
    Trie *t = trie_new();
    trie_insert(t, "car");
    trie_insert(t, "cat");
    trie_insert(t, "do");
    printf("words starting with ca: %d (expected 2)\n", trie_count_prefix(t, "ca"));
    trie_free(t);
    return 0;
}
