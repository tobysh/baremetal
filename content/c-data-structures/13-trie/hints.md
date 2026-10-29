Write a helper `static const Trie *walk(const Trie *root, const char *s)` that follows the letters of `s` and returns the node reached, or `NULL` if the path breaks or a character is invalid. It makes `contains`, `count_prefix` and `has_prefix` short.
===
`trie_insert`: first loop over the word to check every character is `'a'..'z'` and that it is non-empty. Only then walk again, creating missing children with `calloc(1, sizeof(Trie))`, and set `end = 1` at the last node. (If allocation fails halfway you may leave empty nodes behind; invalid characters must be caught before creating any.)
===
`trie_count_prefix`: `node = walk(root, prefix)`; if `NULL` return 0; otherwise return `count_words(node)`, where `count_words(n)` is `n->end` plus the sum of `count_words(n->child[i])` for each non-NULL child. `trie_has_prefix` is `count_prefix(...) > 0`... but be careful: with a rule that rejected words leave no nodes, a node always has a word below it, so `walk(...) != NULL` also works.
