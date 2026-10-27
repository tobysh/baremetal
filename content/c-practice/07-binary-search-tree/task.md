`TreeNode` is defined in the starter. An empty tree is a `NULL` pointer. Implement (include `<stdlib.h>`):

- `TreeNode *bst_insert(TreeNode *root, int key)` inserts `key` and returns the root of the tree. A key already present is ignored (no duplicates). If `malloc` fails, return `root` unchanged. Smaller keys go left, larger go right.
- `int bst_contains(const TreeNode *root, int key)` returns `1` if `key` is in the tree, else `0`. An empty tree contains nothing.
- `int bst_height(const TreeNode *root)` returns the number of nodes on the longest root-to-leaf path: `0` for an empty tree, `1` for a single node.
- `int bst_inorder(const TreeNode *root, int *out, int cap)` writes the keys into `out` in ascending order, writing at most `cap` of them, and returns how many it wrote.
- `void bst_free(TreeNode *root)` frees every node. Passing `NULL` is fine.
