---
title: Trie
summary: Store words in a tree with one level per letter. Insert, look up, test prefixes and count every word that starts with a prefix.
minutes: 35
difficulty: medium
tags: trees, strings, recursion, data-structures
id: c-data-structures/trie
mode: harness
---
How does a phone suggest completions as you type? It needs to answer "which words start with `ca`?" quickly. A hash table finds exact words but knows nothing about prefixes. A **trie** (pronounced "try", from re*trie*val) is a tree where each edge is a letter, so a word is a path from the root.

```c
typedef struct Trie {
    struct Trie *child[26];   // one slot per letter a-z, NULL if absent
    int end;                  // 1 if a word finishes at this node
} Trie;
```

The words `car`, `cat` and `do` form this shape (`*` marks `end`):

```text
root
├─ c ─ a ─ r*
│       └─ t*
└─ d ─ o*
```

`car` and `cat` share the path `c-a`. Lookup time depends only on the word's length, not on how many words are stored.

## Walking and building

Turn a letter into a child index with `ch - 'a'`.

```c
Trie *node = root;
for (const char *p = word; *p; p++) {
    int i = *p - 'a';
    if (node->child[i] == NULL) node->child[i] = new_node();
    node = node->child[i];
}
node->end = 1;
```

A word is **in** the trie only if you can walk its whole path *and* the last node has `end` set. `ca` is a prefix of `car`, but not a word unless `end` is set on it.

## Counting words with a prefix

Walk to the node for the prefix (if the path breaks, the answer is 0), then count the `end` flags in that whole subtree with a recursive function: this node's `end` plus the count of each of the 26 children.

## Validating input first

If a word contains something that is not `a`-`z`, `ch - 'a'` would index outside the array. Check the whole word **before** touching the trie, so a rejected word leaves no half-built path behind.

## Freeing

Free children first, then the node itself (a post-order walk).

## Common mistakes

- Using `char` as an index without subtracting `'a'`.
- Treating "path exists" as "word exists".
- Creating nodes for a word and then discovering it has an invalid character.
- Not freeing the 26 child pointers recursively.

Implement the trie.
