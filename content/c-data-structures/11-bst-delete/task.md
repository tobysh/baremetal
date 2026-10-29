`Node`, `bst_insert` (ignores duplicates) and `bst_free` are given. Implement:

- `const Node *bst_min(const Node *root)` returns the node with the smallest key, or `NULL` for an empty tree.
- `Node *bst_delete(Node *root, int key)` removes the key if present (freeing exactly one node) and returns the new root of the subtree. If the key is absent the tree is unchanged. For a node with two children, replace its key with its in-order successor's key and delete the successor.
- `int bst_size(const Node *root)` returns the number of nodes.
- `int bst_is_valid(const Node *root)` returns 1 if the tree satisfies the search-tree property (strictly smaller on the left, strictly larger on the right, for every node and its whole subtrees), else 0. An empty tree is valid. The tests pass in hand-built trees, including broken ones.
