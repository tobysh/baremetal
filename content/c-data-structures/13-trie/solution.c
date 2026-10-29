#include <stdio.h>
#include <stdlib.h>

typedef struct Trie {
    struct Trie *child[26];
    int end;
} Trie;

Trie *trie_new(void) {
    return calloc(1, sizeof(Trie));
}

static int valid(const char *s) {
    for (const char *p = s; *p; p++)
        if (*p < 'a' || *p > 'z') return 0;
    return 1;
}

int trie_insert(Trie *root, const char *word) {
    if (word[0] == '\0' || !valid(word)) return -1;
    Trie *node = root;
    for (const char *p = word; *p; p++) {
        int i = *p - 'a';
        if (node->child[i] == NULL) {
            node->child[i] = trie_new();
            if (node->child[i] == NULL) return -1;
        }
        node = node->child[i];
    }
    node->end = 1;
    return 0;
}

static const Trie *walk(const Trie *root, const char *s) {
    if (!valid(s)) return NULL;
    const Trie *node = root;
    for (const char *p = s; *p && node; p++) node = node->child[*p - 'a'];
    return node;
}

int trie_contains(const Trie *root, const char *word) {
    if (word[0] == '\0') return 0;
    const Trie *n = walk(root, word);
    return n != NULL && n->end;
}

static int count_words(const Trie *n) {
    int c = n->end;
    for (int i = 0; i < 26; i++)
        if (n->child[i]) c += count_words(n->child[i]);
    return c;
}

int trie_count_prefix(const Trie *root, const char *prefix) {
    const Trie *n = walk(root, prefix);
    return n ? count_words(n) : 0;
}

int trie_has_prefix(const Trie *root, const char *prefix) {
    return trie_count_prefix(root, prefix) > 0;
}

void trie_free(Trie *root) {
    if (root == NULL) return;
    for (int i = 0; i < 26; i++) trie_free(root->child[i]);
    free(root);
}

int main(void) {
    Trie *t = trie_new();
    trie_insert(t, "car");
    trie_insert(t, "cat");
    trie_insert(t, "do");
    printf("words starting with ca: %d (expected 2)\n", trie_count_prefix(t, "ca"));
    trie_free(t);
    return 0;
}
