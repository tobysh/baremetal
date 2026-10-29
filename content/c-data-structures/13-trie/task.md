Words are made of lowercase letters `a`-`z`. Implement (the struct is given):

- `Trie *trie_new(void)` returns a fresh empty root, or `NULL` on allocation failure.
- `int trie_insert(Trie *root, const char *word)` adds a word. Returns `0` on success (adding a word twice is fine), `-1` if the word is empty, contains any character outside `a`-`z`, or memory runs out. A rejected word must leave **no** new nodes behind.
- `int trie_contains(const Trie *root, const char *word)` returns 1 if exactly this word was inserted, else 0. Invalid or empty words are not contained.
- `int trie_count_prefix(const Trie *root, const char *prefix)` returns how many distinct stored words start with `prefix` (a word is a prefix of itself). The empty prefix counts every word. A prefix with an invalid character gives 0.
- `int trie_has_prefix(const Trie *root, const char *prefix)` returns 1 if at least one stored word starts with `prefix`, else 0.
- `void trie_free(Trie *root)` frees every node (`NULL` is fine).
